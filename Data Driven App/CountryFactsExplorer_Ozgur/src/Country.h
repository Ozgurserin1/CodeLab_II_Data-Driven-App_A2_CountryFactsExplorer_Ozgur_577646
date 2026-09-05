#pragma once

#include "ofMain.h"

// Stores the information for one country.
class Country {
public:
    string name;
    string capital;
    string region;
    string subregion;
    long long population = 0;
    vector<string> languages;
    vector<string> currencies;
    vector<string> callingCodes;
    string alpha2Code;
    string alpha3Code;
    string flagUrl;
    double latitude = 0.0;
    double longitude = 0.0;
    bool hasCoordinates = false;

    // Clears the old country information.
    void clear();

    // Checks if a country has been loaded.
    bool hasData() const;

    // Changes list values into text for the screen.
    string getLanguagesText() const;
    string getCurrenciesText() const;
    string getCallingCodesText() const;
};
