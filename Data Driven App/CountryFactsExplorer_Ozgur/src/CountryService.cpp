#include "CountryService.h"

#include <cctype>
#include <exception>

// Loads the key from the local settings file.
bool CountryService::loadSettings() {
    apiKey.clear();
    configurationMessage.clear();

    const string settingsPath = ofToDataPath("settings.json", true);
    if (!ofFile::doesFileExist(settingsPath, false)) {
        configurationMessage = "Setup needed: copy settings.example.json to settings.json and add your API key.";
        return false;
    }

    try {
        const ofBuffer settingsBuffer = ofBufferFromFile(settingsPath);
        const ofJson settings = ofJson::parse(settingsBuffer.getText());
        apiKey = getStringValue(settings, "apiKey", "");
    } catch (const std::exception&) {
        configurationMessage = "Could not read settings.json. Check its JSON format.";
        return false;
    }

    if (apiKey.empty() || apiKey.find("PASTE_") != string::npos) {
        configurationMessage = "Setup needed: add a valid REST Countries API key in settings.json.";
        return false;
    }

    configurationMessage = "Ready. Search for a country, for example Japan or United Kingdom.";
    return true;
}

// Checks if a usable key was loaded.
bool CountryService::hasApiKey() const {
    return !apiKey.empty() && apiKey.find("PASTE_") == string::npos;
}

// Returns the setup or ready message.
string CountryService::getConfigurationMessage() const {
    return configurationMessage;
}

// Builds the country search address.
string CountryService::buildSearchUrl(const string& countryName) const {
    return "https://api.restcountries.com/countries/v5/name?q=" + urlEncode(countryName)
        + "&limit=1&api-key=" + urlEncode(apiKey);
}

// Reads one country from the returned JSON data.
bool CountryService::parseCountryResponse(const ofBuffer& responseBuffer, Country& country, string& errorMessage) const {
    country.clear();
    errorMessage.clear();

    try {
        const ofJson root = ofJson::parse(responseBuffer.getText());

        // Shows the error returned by the service.
        if (root.contains("errors") && root["errors"].is_array() && !root["errors"].empty()) {
            const ofJson& firstError = root["errors"][0];
            errorMessage = firstError.is_object()
                ? getStringValue(firstError, "message", "The service returned an error.")
                : "The service returned an error.";
            return false;
        }

        // Checks that at least one country was returned.
        if (!root.contains("data") || !root["data"].is_object()
            || !root["data"].contains("objects")
            || !root["data"]["objects"].is_array()
            || root["data"]["objects"].empty()) {
            errorMessage = "No country was found. Try a full country name.";
            return false;
        }

        const ofJson& record = root["data"]["objects"][0];
        if (!record.is_object()) {
            errorMessage = "The country data was not in the expected format.";
            return false;
        }

        // Reads the main country information.
        if (record.contains("names") && record["names"].is_object()) {
            country.name = getStringValue(record["names"], "common", "Not available");
        } else {
            country.name = "Not available";
        }

        country.capital = getFirstCapital(record);
        country.region = getStringValue(record, "region", "Not available");
        country.subregion = getStringValue(record, "subregion", "Not available");

        if (record.contains("population") && record["population"].is_number()) {
            country.population = record["population"].get<long long>();
        }

        country.languages = getLanguages(record);
        country.currencies = getCurrencies(record);
        country.callingCodes = getStringArray(record, "calling_codes");

        // Reads the country codes.
        if (record.contains("codes") && record["codes"].is_object()) {
            country.alpha2Code = getStringValue(record["codes"], "alpha_2", "");
            country.alpha3Code = getStringValue(record["codes"], "alpha_3", "");
        }

        // Reads the flag address.
        if (record.contains("flag") && record["flag"].is_object()) {
            country.flagUrl = getStringValue(record["flag"], "url_png", "");
        }

        // Reads the map coordinates when both values are available.
        if (record.contains("coordinates") && record["coordinates"].is_object()
            && record["coordinates"].contains("lat") && record["coordinates"]["lat"].is_number()
            && record["coordinates"].contains("lng") && record["coordinates"]["lng"].is_number()) {
            country.latitude = record["coordinates"]["lat"].get<double>();
            country.longitude = record["coordinates"]["lng"].get<double>();
            country.hasCoordinates = true;
        }

        // Stops if the response does not contain a usable country name.
        if (country.name.empty() || country.name == "Not available") {
            errorMessage = "The response did not contain usable country information.";
            country.clear();
            return false;
        }
    } catch (const std::exception&) {
        errorMessage = "The response could not be read as country data. Please try again.";
        country.clear();
        return false;
    }

    return true;
}

// Changes spaces and special characters for a web address.
string CountryService::urlEncode(const string& value) const {
    static const char hexCharacters[] = "0123456789ABCDEF";
    string encoded;

    for (unsigned char character : value) {
        if (std::isalnum(character) || character == '-' || character == '_'
            || character == '.' || character == '~') {
            encoded += static_cast<char>(character);
        } else {
            encoded += '%';
            encoded += hexCharacters[(character >> 4) & 0x0F];
            encoded += hexCharacters[character & 0x0F];
        }
    }

    return encoded;
}

// Safely reads a text value from a JSON object.
string CountryService::getStringValue(const ofJson& object, const string& key, const string& fallback) const {
    if (!object.is_object() || !object.contains(key) || !object[key].is_string()) {
        return fallback;
    }
    return object[key].get<string>();
}

// Gets the first capital city from the country data.
string CountryService::getFirstCapital(const ofJson& record) const {
    if (!record.contains("capitals") || !record["capitals"].is_array()
        || record["capitals"].empty()) {
        return "Not available";
    }

    const ofJson& capital = record["capitals"][0];
    if (capital.is_object()) {
        return getStringValue(capital, "name", "Not available");
    }
    if (capital.is_string()) {
        return capital.get<string>();
    }
    return "Not available";
}

// Gets the language names from the country data.
vector<string> CountryService::getLanguages(const ofJson& record) const {
    vector<string> languages;
    if (!record.contains("languages") || !record["languages"].is_array()) {
        languages.push_back("Not available");
        return languages;
    }

    for (const auto& language : record["languages"]) {
        if (language.is_object()) {
            const string name = getStringValue(language, "name", "");
            if (!name.empty()) {
                languages.push_back(name);
            }
        } else if (language.is_string()) {
            languages.push_back(language.get<string>());
        }
    }

    if (languages.empty()) {
        languages.push_back("Not available");
    }
    return languages;
}

// Gets the currency code and name from the country data.
vector<string> CountryService::getCurrencies(const ofJson& record) const {
    vector<string> currencies;
    if (!record.contains("currencies")) {
        currencies.push_back("Not available");
        return currencies;
    }

    const ofJson& currencyData = record["currencies"];

    // Reads the current object format used by the service.
    if (currencyData.is_object()) {
        for (auto item = currencyData.begin(); item != currencyData.end(); ++item) {
            const string code = item.key();
            const ofJson& currency = item.value();
            const string name = currency.is_object() ? getStringValue(currency, "name", "") : "";

            // Shows the currency code and name without an empty symbol.
            string displayValue = code;
            if (!name.empty()) {
                displayValue += displayValue.empty() ? name : " - " + name;
            }
            if (!displayValue.empty()) {
                currencies.push_back(displayValue);
            }
        }
    }

    // Also supports an array format if one is returned.
    if (currencyData.is_array()) {
        for (const auto& currency : currencyData) {
            if (!currency.is_object()) {
                continue;
            }

            const string code = getStringValue(currency, "code", "");
            const string name = getStringValue(currency, "name", "");

            // Shows the currency code and name.
            string displayValue = code;
            if (!name.empty()) {
                displayValue += displayValue.empty() ? name : " - " + name;
            }
            if (!displayValue.empty()) {
                currencies.push_back(displayValue);
            }
        }
    }

    if (currencies.empty()) {
        currencies.push_back("Not available");
    }
    return currencies;
}

// Gets a list of text values from the country data.
vector<string> CountryService::getStringArray(const ofJson& record, const string& key) const {
    vector<string> values;
    if (!record.contains(key) || !record[key].is_array()) {
        return values;
    }

    for (const auto& item : record[key]) {
        if (item.is_string()) {
            const string value = item.get<string>();
            if (!value.empty()) {
                values.push_back(value);
            }
        }
    }
    return values;
}
