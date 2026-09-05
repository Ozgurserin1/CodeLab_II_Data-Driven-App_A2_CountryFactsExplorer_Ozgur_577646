#include "Country.h"

namespace {
// Joins a list of words into one line of text.
string joinValues(const vector<string>& values, const string& separator) {
    string result;
    for (size_t index = 0; index < values.size(); ++index) {
        result += values[index];
        if (index + 1 < values.size()) {
            result += separator;
        }
    }
    return result.empty() ? "Not available" : result;
}
}

// Clears all saved values from the country object.
void Country::clear() {
    name.clear();
    capital.clear();
    region.clear();
    subregion.clear();
    population = 0;
    languages.clear();
    currencies.clear();
    callingCodes.clear();
    alpha2Code.clear();
    alpha3Code.clear();
    flagUrl.clear();
    latitude = 0.0;
    longitude = 0.0;
    hasCoordinates = false;
}

// Checks if the country has a name.
bool Country::hasData() const {
    return !name.empty();
}

// Returns the languages as one line of text.
string Country::getLanguagesText() const {
    return joinValues(languages, ", ");
}

// Returns the currencies as one line of text.
string Country::getCurrenciesText() const {
    return joinValues(currencies, ", ");
}

// Adds a plus sign to calling codes and returns them as text.
string Country::getCallingCodesText() const {
    if (callingCodes.empty()) {
        return "Not available";
    }

    vector<string> formattedCodes;
    for (const string& code : callingCodes) {
        formattedCodes.push_back(code.empty() || code[0] == '+' ? code : "+" + code);
    }
    return joinValues(formattedCodes, ", ");
}
