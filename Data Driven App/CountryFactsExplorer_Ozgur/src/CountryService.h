#pragma once

#include "ofMain.h"
#include "Country.h"

// Handles settings, web addresses and country data.
class CountryService {
public:
    // Loads the local settings file.
    bool loadSettings();
    bool hasApiKey() const;
    string getConfigurationMessage() const;

    // Builds the web request and reads the returned country data.
    string buildSearchUrl(const string& countryName) const;
    bool parseCountryResponse(const ofBuffer& responseBuffer, Country& country, string& errorMessage) const;

private:
    string apiKey;
    string configurationMessage;

    // Helps read and format values from the returned data.
    string urlEncode(const string& value) const;
    string getStringValue(const ofJson& object, const string& key, const string& fallback) const;
    string getFirstCapital(const ofJson& record) const;
    vector<string> getLanguages(const ofJson& record) const;
    vector<string> getCurrencies(const ofJson& record) const;
    vector<string> getStringArray(const ofJson& record, const string& key) const;
};
