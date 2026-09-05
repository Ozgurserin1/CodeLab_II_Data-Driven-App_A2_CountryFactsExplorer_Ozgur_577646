#include "ofApp.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <exception>

// Sets up fonts, images, settings and the screen layout.
void ofApp::setup() {
    // Sets the main window options.
    ofSetWindowTitle("Country Facts Explorer - Ozgur Serin");
    ofSetFrameRate(60);
    ofSetVerticalSync(true);
    ofSetCircleResolution(48);
    ofRegisterURLNotification(this);

    // Loads the fonts and world map image.
    fontSmall.load("fonts/DejaVuSans.ttf", 11, true, true);
    fontBody.load("fonts/DejaVuSans.ttf", 14, true, true);
    fontBodyBold.load("fonts/DejaVuSans-Bold.ttf", 14, true, true);
    fontHeading.load("fonts/DejaVuSans-Bold.ttf", 24, true, true);
    fontCountry.load("fonts/DejaVuSans-Bold.ttf", 30, true, true);
    worldMapImage.load("world_map.png");

    // Loads settings, favourites and the first layout.
    countryService.loadSettings();
    statusMessage = countryService.getConfigurationMessage();
    loadFavourites();
    setupLayout();
}

// Updates the loading and map pin animation.
void ofApp::update() {
    loadingAngle += 4.0f;
    if (loadingAngle >= 360.0f) {
        loadingAngle -= 360.0f;
    }
    pinPulse = (std::sin(ofGetElapsedTimef() * 3.0f) + 1.0f) * 0.5f;
}

// Draws all parts of the app each frame.
void ofApp::draw() {
    ofBackground(pageBackground());
    setupLayout();

    // Draws the main interface.
    drawHeader();
    drawSearchControls();
    drawStatus();
    drawWorldMap();

    // Shows either country data or the empty help panel.
    if (currentCountry.hasData()) {
        drawCountryPanel();
    } else {
        drawEmptyPanel();
    }

    // Covers the map with a loading message during a search.
    if (appState == AppState::Loading) {
        drawLoadingOverlay();
    }

    drawHistoryAndFavourites();
    drawFooter();
}

// Saves favourites and closes the web listener.
void ofApp::exit() {
    saveFavourites();
    ofUnregisterURLNotification(this);
}

// Handles typing and keyboard shortcuts.
void ofApp::keyPressed(int key) {
    if (key == OF_KEY_RETURN) {
        startSearch();
        return;
    }

    if (key == OF_KEY_ESC) {
        clearSearch();
        return;
    }

    // Only edits the search text when the search box is active.
    if (!inputFocused) {
        return;
    }

    if (key == OF_KEY_BACKSPACE || key == 127) {
        if (!inputText.empty()) {
            inputText.pop_back();
        }
        return;
    }

    if (key >= 32 && key <= 126 && inputText.length() < 60) {
        inputText += static_cast<char>(key);
    }
}

// Handles clicks on the search box, buttons and saved countries.
void ofApp::mousePressed(int x, int y, int) {
    // Gives focus only when the search box is clicked.
    inputFocused = searchBox.inside(x, y);

    // Changes the colour theme.
    if (themeToggle.inside(x, y)) {
        darkMode = !darkMode;
        return;
    }

    if (inputFocused) {
        return;
    }

    // Checks the three main control buttons.
    if (searchButton.contains(x, y)) {
        startSearch();
        return;
    }

    if (clearButton.contains(x, y)) {
        clearSearch();
        return;
    }

    if (randomButton.contains(x, y)) {
        searchRandomCountry();
        return;
    }

    // Saves or removes the current country.
    if (currentCountry.hasData() && favouriteButton.inside(x, y)) {
        toggleFavourite();
        return;
    }

    // Searches again when a recent tag is clicked.
    for (size_t index = 0; index < historyBounds.size(); ++index) {
        if (historyBounds[index].inside(x, y) && index < searchHistory.size()) {
            inputText = searchHistory[index];
            startSearch();
            return;
        }
    }

    // Searches again when a favourite tag is clicked.
    for (size_t index = 0; index < favouriteBounds.size(); ++index) {
        if (favouriteBounds[index].inside(x, y) && index < favourites.size()) {
            inputText = favourites[index];
            startSearch();
            return;
        }
    }
}

// Updates the layout when the window size changes.
void ofApp::windowResized(int, int) {
    setupLayout();
}

// Handles country and flag responses from the web.
void ofApp::urlResponse(ofHttpResponse& response) {
    // Handles the main country search response.
    if (response.request.name == "countrySearch") {
        if (response.status != 200) {
            appState = AppState::Error;
            statusMessage = "Request failed (HTTP " + ofToString(response.status)
                + "). Check your key, connection and country name.";
            return;
        }

        string errorMessage;
        Country parsedCountry;
        if (!countryService.parseCountryResponse(response.data, parsedCountry, errorMessage)) {
            appState = AppState::Error;
            statusMessage = errorMessage;
            return;
        }

        currentCountry = parsedCountry;
        addToHistory(currentCountry.name);
        appState = AppState::Success;
        statusMessage = "Country found";

        flagImage.clear();
        flagLoading = false;
        if (!currentCountry.flagUrl.empty()) {
            flagLoading = true;
            ofLoadURLAsync(currentCountry.flagUrl, "countryFlag");
        }
        return;
    }

    // Handles the flag image response.
    if (response.request.name == "countryFlag") {
        // Ignores an old flag response if another country is now selected.
        if (response.request.url != currentCountry.flagUrl) {
            return;
        }

        flagLoading = false;
        if (response.status == 200) {
            flagImage.load(response.data);
        }
    }
}

// Sets the position and size of the main screen parts.
void ofApp::setupLayout() {
    const float margin = 32.0f;
    const float gap = 20.0f;
    const float rightWidth = std::max(320.0f, std::min(390.0f, ofGetWidth() * 0.31f));
    const float leftWidth = ofGetWidth() - margin * 2.0f - gap - rightWidth;

    // Sets the country panel and map area.
    detailPanel.set(margin + leftWidth + gap, 136, rightWidth, 600);
    mapPanel.set(margin, 250, leftWidth, 380);

    // Sets the search box and control buttons.
    const float buttonGap = 8.0f;
    const float searchWidth = std::max(170.0f, leftWidth - 326.0f);
    searchBox.set(margin, 142, searchWidth, 56);
    searchButton = Button(ofRectangle(searchBox.getRight() + buttonGap, 142, 104, 56), "SEARCH");
    clearButton = Button(ofRectangle(searchButton.bounds.getRight() + buttonGap, 142, 88, 56), "CLEAR");
    randomButton = Button(ofRectangle(clearButton.bounds.getRight() + buttonGap, 142, 102, 56), "RANDOM");

    // Sets the theme and favourite controls.
    themeToggle.set(ofGetWidth() - 108, 34, 64, 32);
    favouriteButton.set(detailPanel.getRight() - 144, detailPanel.y + 20, 122, 32);
}

// Checks the search text and starts a country request.
void ofApp::startSearch() {
    const string countryName = trim(inputText);

    // Stops an empty search.
    if (countryName.empty()) {
        appState = AppState::Error;
        statusMessage = "Please type a country name before searching.";
        return;
    }

    // Stops the search when settings are not ready.
    if (!countryService.hasApiKey()) {
        appState = AppState::Error;
        statusMessage = countryService.getConfigurationMessage();
        return;
    }

    // Stops a second search while the first one is loading.
    if (appState == AppState::Loading) {
        statusMessage = "A search is already in progress. Please wait.";
        return;
    }

    // Clears old data and sends the new request.
    inputText = countryName;
    currentCountry.clear();
    flagImage.clear();
    flagLoading = false;
    appState = AppState::Loading;
    statusMessage = "Loading country information...";
    ofLoadURLAsync(countryService.buildSearchUrl(countryName), "countrySearch");
}

// Picks a country from a small random list and searches for it.
void ofApp::searchRandomCountry() {
    static const vector<string> countries = {
        "Argentina", "Australia", "Brazil", "Canada", "Egypt", "France",
        "Germany", "Ghana", "Greece", "India", "Italy", "Japan", "Kenya",
        "Mexico", "Morocco", "New Zealand", "Norway", "South Africa",
        "South Korea", "Spain", "Sweden", "Thailand", "Turkey",
        "United Kingdom", "United States"
    };

    const int index = static_cast<int>(ofRandom(static_cast<float>(countries.size())));
    inputText = countries[std::min(index, static_cast<int>(countries.size()) - 1)];
    startSearch();
}

// Clears the current search and country information.
void ofApp::clearSearch() {
    inputFocused = true;
    inputText.clear();
    currentCountry.clear();
    flagImage.clear();
    flagLoading = false;
    appState = AppState::Idle;
    statusMessage = countryService.hasApiKey()
        ? "Ready to explore another country."
        : countryService.getConfigurationMessage();
}

// Adds a successful search to the recent list.
void ofApp::addToHistory(const string& countryName) {
    // Removes the same country before adding it to the front.
    const string lowerName = toLower(countryName);
    searchHistory.erase(
        std::remove_if(searchHistory.begin(), searchHistory.end(),
            [&](const string& item) { return toLower(item) == lowerName; }),
        searchHistory.end());

    searchHistory.insert(searchHistory.begin(), countryName);
    if (searchHistory.size() > 5) {
        searchHistory.pop_back();
    }
}

// Adds or removes the current country from favourites.
void ofApp::toggleFavourite() {
    if (!currentCountry.hasData()) {
        return;
    }

    // Looks for the country in the saved list.
    const string lowerName = toLower(currentCountry.name);
    const auto found = std::find_if(favourites.begin(), favourites.end(),
        [&](const string& item) { return toLower(item) == lowerName; });

    if (found == favourites.end()) {
        favourites.insert(favourites.begin(), currentCountry.name);
        statusMessage = currentCountry.name + " added to favourites.";
    } else {
        favourites.erase(found);
        statusMessage = currentCountry.name + " removed from favourites.";
    }
    saveFavourites();
}

// Checks if the current country is already saved.
bool ofApp::isCurrentCountryFavourite() const {
    if (!currentCountry.hasData()) {
        return false;
    }
    const string lowerName = toLower(currentCountry.name);
    return std::find_if(favourites.begin(), favourites.end(),
        [&](const string& item) { return toLower(item) == lowerName; }) != favourites.end();
}

// Loads saved favourites from the local JSON file.
void ofApp::loadFavourites() {
    favourites.clear();
    const string path = ofToDataPath("favourites.json", true);
    if (!ofFile::doesFileExist(path, false)) {
        return;
    }

    // Reads saved names from the JSON file.
    try {
        const ofJson data = ofJson::parse(ofBufferFromFile(path).getText());
        if (data.is_array()) {
            for (const auto& item : data) {
                if (item.is_string()) {
                    favourites.push_back(item.get<string>());
                }
            }
        }
    } catch (const std::exception&) {
        favourites.clear();
    }
}

// Saves favourites to the local JSON file.
void ofApp::saveFavourites() const {
    // Builds a JSON list before saving it.
    ofJson data = ofJson::array();
    for (const string& countryName : favourites) {
        data.push_back(countryName);
    }
    ofSavePrettyJson(ofToDataPath("favourites.json", true), data);
}

// Draws the title and theme switch.
void ofApp::drawHeader() {
    ofPushStyle();
    // Draws the dark header bar.
    ofSetColor(11, 37, 69);
    ofDrawRectangle(0, 0, ofGetWidth(), 110);

    drawGlobe(57, 54, 25, ofColor(31, 199, 182));
    drawText("COUNTRY FACTS EXPLORER", 98, 50, ofColor::white, fontHeading);
    drawText("Explore countries and discover the world.", 99, 80,
             ofColor(196, 220, 232), fontBody);

    // Draws the light and dark theme switch.
    drawText(darkMode ? "DARK" : "LIGHT", themeToggle.x - 52,
             themeToggle.y + 22, ofColor(196, 220, 232), fontSmall);
    ofSetColor(darkMode ? ofColor(31, 199, 182) : ofColor(77, 112, 143));
    ofDrawRectRounded(themeToggle, 16);
    ofSetColor(ofColor::white);
    const float knobX = darkMode ? themeToggle.getRight() - 16 : themeToggle.x + 16;
    ofDrawCircle(knobX, themeToggle.getCenter().y, 11);
    ofPopStyle();
}

// Draws the search box and control buttons.
void ofApp::drawSearchControls() {
    ofPushStyle();
    // Draws the search input area.
    ofSetColor(panelColour());
    ofDrawRectRounded(searchBox, 10);
    ofNoFill();
    ofSetLineWidth(inputFocused ? 2.0f : 1.0f);
    ofSetColor(inputFocused ? accentColour() : borderColour());
    ofDrawRectRounded(searchBox, 10);
    ofFill();

    ofSetColor(secondaryText());
    ofSetLineWidth(2.0f);
    ofNoFill();
    ofDrawCircle(searchBox.x + 23, searchBox.getCenter().y - 2, 8);
    ofDrawLine(searchBox.x + 29, searchBox.getCenter().y + 4,
               searchBox.x + 35, searchBox.getCenter().y + 10);
    ofFill();

    const string visibleText = inputText.empty() ? "Type a country name..." : inputText;
    const ofColor textColour = inputText.empty() ? secondaryText() : primaryText();
    drawText(visibleText, searchBox.x + 48, searchBox.y + 35, textColour, fontBody);

    // Draws the search, clear and random buttons.
    drawControlButton(searchButton, true, accentColour());
    drawControlButton(clearButton, false, accentColour());
    drawControlButton(randomButton, false, ofColor(42, 126, 167));
    ofPopStyle();
}

// Shows the current search status or error message.
void ofApp::drawStatus() {
    // Chooses a colour for the current app state.
    ofColor colour = secondaryText();
    if (appState == AppState::Loading) {
        colour = ofColor(33, 139, 181);
    } else if (appState == AppState::Success) {
        colour = ofColor(14, 151, 116);
    } else if (appState == AppState::Error) {
        colour = ofColor(206, 74, 68);
    }

    if (appState == AppState::Success) {
        ofSetColor(colour);
        ofDrawCircle(mapPanel.x + 8, 224, 8);
        drawText("OK", mapPanel.x + 2, 228, ofColor::white, fontSmall);
        drawText(statusMessage, mapPanel.x + 24, 229, colour, fontBodyBold);
    } else {
        drawText(statusMessage, mapPanel.x, 229, colour,
                 appState == AppState::Error ? fontBodyBold : fontBody);
    }
}

// Draws the world map and the selected country marker.
void ofApp::drawWorldMap() {
    ofPushStyle();

    // Draws the map panel background.
    ofSetColor(darkMode ? ofColor(0, 0, 0, 55) : ofColor(25, 55, 80, 18));
    ofDrawRectRounded(mapPanel.x + 4, mapPanel.y + 6,
                      mapPanel.width, mapPanel.height, 16);
    ofSetColor(panelColour());
    ofDrawRectRounded(mapPanel, 16);
    ofNoFill();
    ofSetLineWidth(1.0f);
    ofSetColor(borderColour());
    ofDrawRectRounded(mapPanel, 16);
    ofFill();

    // Draws a simple grid over the map area.
    const ofColor grid = darkMode ? ofColor(76, 105, 126, 45) : ofColor(117, 159, 180, 35);
    ofSetColor(grid);
    for (int index = 1; index < 6; ++index) {
        const float x = mapPanel.x + mapPanel.width * index / 6.0f;
        ofDrawLine(x, mapPanel.y + 26, x, mapPanel.getBottom() - 26);
    }
    for (int index = 1; index < 4; ++index) {
        const float y = mapPanel.y + mapPanel.height * index / 4.0f;
        ofDrawLine(mapPanel.x + 26, y, mapPanel.getRight() - 26, y);
    }

    // Draws the map image if it loaded correctly.
    if (worldMapImage.isAllocated()) {
        const float imageHeight = mapPanel.height - 66.0f;
        const float imageWidth = imageHeight * 2.0f;
        const float imageX = mapPanel.getCenter().x - imageWidth / 2.0f;
        const float imageY = mapPanel.y + 40.0f;
        ofSetColor(darkMode ? ofColor(180, 200, 210) : ofColor::white);
        worldMapImage.draw(imageX, imageY, imageWidth, imageHeight);
    } else {
        drawText("Map image unavailable.", mapPanel.x + 22, mapPanel.y + 58,
                 secondaryText(), fontSmall);
    }

    drawText("INTERACTIVE WORLD MAP", mapPanel.x + 22, mapPanel.y + 30,
             secondaryText(), fontSmall);

    // Draws the marker after a country is found.
    if (currentCountry.hasData() && currentCountry.hasCoordinates) {
        drawMapPin();
    } else if (appState != AppState::Loading) {
        // Shows a clear message inside the map.
        const ofRectangle mapMessageBox(mapPanel.x + 16, mapPanel.getBottom() - 38, 332, 26);
        ofSetColor(darkMode ? ofColor(12, 36, 57, 220) : ofColor(245, 250, 252, 230));
        ofDrawRectRounded(mapMessageBox, 7);

        drawText("Search for a country to locate it on the map.",
                 mapMessageBox.x + 10, mapMessageBox.y + 18,
                 darkMode ? ofColor(225, 236, 242) : ofColor(45, 70, 88), fontSmall);
    }
    ofPopStyle();
}

// Draws the selected country information card.
void ofApp::drawCountryPanel() {
    ofPushStyle();
    ofSetColor(darkMode ? ofColor(0, 0, 0, 65) : ofColor(20, 52, 76, 25));
    ofDrawRectRounded(detailPanel.x + 4, detailPanel.y + 6,
                      detailPanel.width, detailPanel.height, 16);
    ofSetColor(panelColour());
    ofDrawRectRounded(detailPanel, 16);
    ofNoFill();
    ofSetColor(borderColour());
    ofDrawRectRounded(detailPanel, 16);
    ofFill();

    // Draws the favourite button.
    const bool favourite = isCurrentCountryFavourite();
    ofSetColor(favourite ? ofColor(255, 193, 61) : borderColour());
    ofDrawRectRounded(favouriteButton, 16);
    drawText(favourite ? "* FAVOURITE" : "+ FAVOURITE",
             favouriteButton.x + 12, favouriteButton.y + 22,
             favourite ? ofColor(67, 51, 18) : primaryText(), fontSmall);

    // Draws the country flag or a short message.
    const float flagWidth = 132.0f;
    const float flagHeight = 82.0f;
    const float flagX = detailPanel.getCenter().x - flagWidth / 2.0f;
    const float flagY = detailPanel.y + 58.0f;
    ofSetColor(darkMode ? ofColor(43, 61, 80) : ofColor(239, 245, 247));
    ofDrawRectRounded(flagX - 5, flagY - 5, flagWidth + 10, flagHeight + 10, 8);
    if (flagImage.isAllocated()) {
        ofSetColor(ofColor::white);
        flagImage.draw(flagX, flagY, flagWidth, flagHeight);
    } else {
        drawText(flagLoading ? "Loading flag..." : "Flag unavailable",
                 flagX + 10, flagY + 45, secondaryText(), fontSmall);
    }

    // Draws the country name and region badge.
    string title = shorten(currentCountry.name, 22);
    const float titleX = detailPanel.getCenter().x - textWidth(title, fontCountry) / 2.0f;
    drawText(title, titleX, detailPanel.y + 186, primaryText(), fontCountry);

    const string region = currentCountry.region.empty() ? "Not available" : currentCountry.region;
    const float badgeWidth = textWidth(region, fontSmall) + 28.0f;
    const float badgeX = detailPanel.getCenter().x - badgeWidth / 2.0f;
    ofSetColor(darkMode ? ofColor(31, 83, 103) : ofColor(226, 242, 247));
    ofDrawRectRounded(badgeX, detailPanel.y + 202, badgeWidth, 28, 14);
    drawText(region, badgeX + 14, detailPanel.y + 221, accentColour(), fontSmall);

    ofSetColor(borderColour());
    ofDrawLine(detailPanel.x + 24, detailPanel.y + 246,
               detailPanel.getRight() - 24, detailPanel.y + 246);

    // Draws the main country facts.
    const float rowX = detailPanel.x + 24;
    float rowY = detailPanel.y + 270;
    drawInfoRow("C", "CAPITAL", currentCountry.capital, rowX, rowY, ofColor(34, 112, 161));
    rowY += 46;
    drawInfoRow("P", "POPULATION", formatPopulation(currentCountry.population), rowX, rowY, ofColor(42, 153, 92));
    rowY += 46;
    drawInfoRow("R", "REGION", currentCountry.region, rowX, rowY, ofColor(45, 128, 176));
    rowY += 46;
    drawInfoRow("S", "SUBREGION", currentCountry.subregion, rowX, rowY, ofColor(88, 80, 167));
    rowY += 46;
    drawInfoRow("L", "LANGUAGE", currentCountry.getLanguagesText(), rowX, rowY, ofColor(199, 151, 34));
    rowY += 46;
    drawInfoRow("$", "CURRENCY", currentCountry.getCurrenciesText(), rowX, rowY, ofColor(22, 151, 119));

    // Draws the country and calling codes at the bottom.
    const string codes = (currentCountry.alpha2Code.empty() ? "--" : currentCountry.alpha2Code)
        + " / " + (currentCountry.alpha3Code.empty() ? "---" : currentCountry.alpha3Code)
        + "   |   " + currentCountry.getCallingCodesText();
    drawText("COUNTRY CODE / CALLING CODE", detailPanel.x + 24,
             detailPanel.getBottom() - 38, secondaryText(), fontSmall);
    drawText(codes, detailPanel.x + 24, detailPanel.getBottom() - 17,
             primaryText(), fontBodyBold);
    ofPopStyle();
}

// Shows help text before a country is loaded.
void ofApp::drawEmptyPanel() {
    ofPushStyle();
    ofSetColor(darkMode ? ofColor(0, 0, 0, 65) : ofColor(20, 52, 76, 25));
    ofDrawRectRounded(detailPanel.x + 4, detailPanel.y + 6,
                      detailPanel.width, detailPanel.height, 16);
    ofSetColor(panelColour());
    ofDrawRectRounded(detailPanel, 16);
    ofNoFill();
    ofSetColor(borderColour());
    ofDrawRectRounded(detailPanel, 16);
    ofFill();

    // Draws the empty state icon and help text.
    drawGlobe(detailPanel.getCenter().x, detailPanel.y + 172, 55, accentColour());
    const string heading = appState == AppState::Error ? "SEARCH PROBLEM" : "START EXPLORING";
    const float headingX = detailPanel.getCenter().x - textWidth(heading, fontHeading) / 2.0f;
    drawText(heading, headingX, detailPanel.y + 274,
             appState == AppState::Error ? ofColor(206, 74, 68) : primaryText(), fontHeading);

    drawText("Search for any country to see:", detailPanel.x + 48,
             detailPanel.y + 326, secondaryText(), fontBody);
    drawText("- Its location on the world map", detailPanel.x + 48,
             detailPanel.y + 370, primaryText(), fontBody);
    drawText("- Capital, population and region", detailPanel.x + 48,
             detailPanel.y + 407, primaryText(), fontBody);
    drawText("- Language, currency and flag", detailPanel.x + 48,
             detailPanel.y + 444, primaryText(), fontBody);
    drawText("- Country and calling codes", detailPanel.x + 48,
             detailPanel.y + 481, primaryText(), fontBody);

    // Draws a small random search tip.
    const string tipText = "TIP: Try the RANDOM button";
    const ofRectangle tipBox(detailPanel.x + 30, detailPanel.getBottom() - 82,
                             detailPanel.width - 60, 48);

    ofSetColor(darkMode ? ofColor(31, 83, 103) : ofColor(230, 246, 246));
    ofDrawRectRounded(tipBox, 10);

    // Keeps the tip text inside the box.
    const float tipX = tipBox.getCenter().x - textWidth(tipText, fontSmall) / 2.0f;
    drawText(tipText, tipX, detailPanel.getBottom() - 52,
             accentColour(), fontSmall);
    ofPopStyle();
}

// Shows a loading animation while waiting for country data.
void ofApp::drawLoadingOverlay() {
    ofPushStyle();
    ofSetColor(darkMode ? ofColor(14, 29, 47, 190) : ofColor(247, 250, 249, 210));
    ofDrawRectRounded(mapPanel, 16);

    // Draws the moving loading circles.
    const glm::vec2 centre(mapPanel.getCenter().x, mapPanel.getCenter().y);
    for (int index = 0; index < 10; ++index) {
        const float angle = ofDegToRad(loadingAngle + index * 36.0f);
        const float alpha = 45.0f + index * 20.0f;
        ofSetColor(accentColour(), static_cast<int>(std::min(225.0f, alpha)));
        ofDrawCircle(centre.x + std::cos(angle) * 28.0f,
                     centre.y - 12 + std::sin(angle) * 28.0f, 4.0f);
    }

    const string message = "Exploring the world...";
    drawText(message, centre.x - textWidth(message, fontBodyBold) / 2.0f,
             centre.y + 42, primaryText(), fontBodyBold);
    ofPopStyle();
}

// Draws recent searches and favourite country tags.
void ofApp::drawHistoryAndFavourites() {
    const float recentY = mapPanel.getBottom() + 34.0f;
    const float favouriteY = recentY + 52.0f;
    drawTagRow("RECENT SEARCHES", searchHistory, recentY, historyBounds, false);
    drawTagRow("FAVOURITES", favourites, favouriteY, favouriteBounds, true);
}

// Draws the data source, keyboard help and creator name.
void ofApp::drawFooter() {
    const float footerY = ofGetHeight() - 42.0f;
    ofPushStyle();
    ofSetColor(borderColour());
    ofDrawLine(32, footerY - 14, ofGetWidth() - 32, footerY - 14);

    drawText("Data source: REST Countries API", 42, footerY + 10,
             secondaryText(), fontSmall);

    const string enterText = "Press Enter to search";
    const string escText = "Esc clears the screen";
    drawText(enterText, ofGetWidth() * 0.43f, footerY + 10,
             secondaryText(), fontSmall);
    drawText(escText, ofGetWidth() * 0.62f, footerY + 10,
             secondaryText(), fontSmall);

    const string creator = "Created by Ozgur Serin";
    drawText(creator, ofGetWidth() - textWidth(creator, fontSmall) - 42,
             footerY + 10, primaryText(), fontSmall);
    ofPopStyle();
}

// Draws the small globe icon used in the interface.
void ofApp::drawGlobe(float x, float y, float radius, const ofColor& colour) {
    ofPushStyle();
    ofNoFill();
    ofSetLineWidth(2.0f);
    ofSetColor(colour);
    ofDrawCircle(x, y, radius);
    ofDrawEllipse(x, y, radius * 0.85f, radius * 2.0f);
    ofDrawLine(x - radius, y, x + radius, y);
    ofDrawEllipse(x, y - radius * 0.34f, radius * 1.7f, radius * 0.55f);
    ofDrawEllipse(x, y + radius * 0.34f, radius * 1.7f, radius * 0.55f);
    ofFill();
    ofPopStyle();
}

// Converts country coordinates into a position on the map.
void ofApp::drawMapPin() {
    // Changes longitude and latitude into values from 0 to 1.
    const float normalizedX = static_cast<float>((currentCountry.longitude + 180.0) / 360.0);
    const float normalizedY = static_cast<float>((90.0 - currentCountry.latitude) / 180.0);
    const float imageHeight = mapPanel.height - 66.0f;
    const float imageWidth = imageHeight * 2.0f;
    const float imageX = mapPanel.getCenter().x - imageWidth / 2.0f;
    const float imageY = mapPanel.y + 40.0f;
    const float x = imageX + ofClamp(normalizedX, 0.0f, 1.0f) * imageWidth;
    const float y = imageY + ofClamp(normalizedY, 0.0f, 1.0f) * imageHeight;

    // Draws the animated map marker.
    ofPushStyle();
    ofNoFill();
    ofSetLineWidth(2.0f);
    ofSetColor(accentColour(), static_cast<int>(80 + pinPulse * 90));
    ofDrawCircle(x, y + 12, 18 + pinPulse * 12);
    ofDrawCircle(x, y + 12, 29 + pinPulse * 15);
    ofFill();

    ofSetColor(accentColour());
    ofDrawCircle(x, y, 15);
    ofDrawTriangle(x - 9, y + 9, x + 9, y + 9, x, y + 27);
    ofSetColor(240, 82, 72);
    ofDrawCircle(x, y, 6);

    // Draws the country name next to the marker.
    string label = shorten(currentCountry.name, 24);
    const float labelWidth = textWidth(label, fontSmall) + 22.0f;
    const float labelX = std::min(mapPanel.getRight() - labelWidth - 10,
                                  std::max(mapPanel.x + 10, x + 18));
    const float labelY = std::max(mapPanel.y + 12, y - 30);
    ofSetColor(darkMode ? ofColor(31, 53, 73) : ofColor::white);
    ofDrawRectRounded(labelX, labelY, labelWidth, 28, 14);
    drawText(label, labelX + 11, labelY + 19, primaryText(), fontSmall);
    ofPopStyle();
}

// Draws one row of country information.
void ofApp::drawInfoRow(const string& icon, const string& label, const string& value,
                        float x, float y, const ofColor& accent) {
    ofPushStyle();
    ofSetColor(accent, darkMode ? 72 : 38);
    ofDrawCircle(x + 18, y + 13, 17);
    drawText(icon, x + 12, y + 18, accent, fontBodyBold);
    drawText(label, x + 48, y + 8, secondaryText(), fontSmall);
    drawText(shorten(value, 26), x + 48, y + 31, primaryText(), fontBody);
    ofSetColor(borderColour());
    ofDrawLine(x, y + 43, detailPanel.getRight() - 24, y + 43);
    ofPopStyle();
}

// Draws clickable tags for history or favourites.
void ofApp::drawTagRow(const string& title, const vector<string>& items, float y,
                       vector<ofRectangle>& bounds, bool favouriteStyle) {
    bounds.clear();
    drawText(title, mapPanel.x, y, secondaryText(), fontSmall);

    // Leaves a clear gap after the row title.
    const float itemStartX = mapPanel.x + textWidth(title, fontSmall) + 22.0f;

    // Shows help text if the list is empty.
    if (items.empty()) {
        const string emptyText = favouriteStyle
            ? "Click + FAVOURITE on a country card to save it."
            : "Your latest successful searches will appear here.";
        drawText(emptyText, itemStartX, y, secondaryText(), fontSmall);
        return;
    }

    // Draws each visible country tag.
    float currentX = itemStartX;
    for (const string& item : items) {
        const float tagWidth = std::max(88.0f, textWidth(item, fontSmall) + 30.0f);
        if (currentX + tagWidth > mapPanel.getRight()) {
            break;
        }

        ofRectangle tag(currentX, y - 20, tagWidth, 30);
        bounds.push_back(tag);
        const bool hovered = tag.inside(ofGetMouseX(), ofGetMouseY());
        ofSetColor(favouriteStyle
            ? (hovered ? ofColor(255, 226, 151) : ofColor(255, 238, 193))
            : (darkMode ? ofColor(38, 69, 86) : (hovered ? ofColor(211, 239, 241) : ofColor(228, 243, 245))));
        ofDrawRectRounded(tag, 15);
        drawText((favouriteStyle ? "* " : "") + item,
                 tag.x + 14, tag.y + 20,
                 favouriteStyle ? ofColor(105, 76, 14) : accentColour(), fontSmall);
        currentX += tagWidth + 8.0f;
    }
}

// Draws one control button and its hover style.
void ofApp::drawControlButton(const Button& button, bool primary, const ofColor& accent) {
    const bool hovered = button.contains(ofGetMouseX(), ofGetMouseY());
    ofPushStyle();
    if (primary) {
        ofSetColor(hovered ? accent.getLerped(ofColor::white, 0.12f) : accent);
    } else {
        ofSetColor(hovered
            ? (darkMode ? ofColor(54, 75, 94) : ofColor(226, 236, 241))
            : panelColour());
    }
    ofDrawRectRounded(button.bounds, 10);
    if (!primary) {
        ofNoFill();
        ofSetColor(borderColour());
        ofDrawRectRounded(button.bounds, 10);
        ofFill();
    }

    const float labelX = button.bounds.getCenter().x - textWidth(button.label, fontSmall) / 2.0f;
    drawText(button.label, labelX, button.bounds.getCenter().y + 5,
             primary ? ofColor::white : primaryText(), fontSmall);
    ofPopStyle();
}

// Draws text using a font or a simple fallback.
void ofApp::drawText(const string& text, float x, float y, const ofColor& colour,
                     ofTrueTypeFont& font) {
    ofPushStyle();
    ofSetColor(colour);
    if (font.isLoaded()) {
        font.drawString(text, x, y);
    } else {
        ofDrawBitmapString(text, x, y);
    }
    ofPopStyle();
}

// Gets the width of text for centring and layout.
float ofApp::textWidth(const string& text, ofTrueTypeFont& font) const {
    if (font.isLoaded()) {
        return font.getStringBoundingBox(text, 0, 0).width;
    }
    return static_cast<float>(text.length()) * 8.0f;
}

// Returns the page background colour.
ofColor ofApp::pageBackground() const {
    return darkMode ? ofColor(12, 26, 43) : ofColor(247, 248, 245);
}

// Returns the panel colour.
ofColor ofApp::panelColour() const {
    return darkMode ? ofColor(25, 43, 62) : ofColor::white;
}

// Returns the main text colour.
ofColor ofApp::primaryText() const {
    return darkMode ? ofColor(235, 243, 247) : ofColor(16, 42, 67);
}

// Returns the smaller text colour.
ofColor ofApp::secondaryText() const {
    return darkMode ? ofColor(161, 185, 199) : ofColor(91, 111, 128);
}

// Returns the border colour.
ofColor ofApp::borderColour() const {
    return darkMode ? ofColor(58, 82, 101) : ofColor(207, 220, 227);
}

// Returns the main accent colour.
ofColor ofApp::accentColour() const {
    return ofColor(31, 199, 182);
}

// Removes spaces from the start and end of search text.
string ofApp::trim(const string& value) const {
    const size_t first = value.find_first_not_of(" \t\n\r");
    if (first == string::npos) {
        return "";
    }
    const size_t last = value.find_last_not_of(" \t\n\r");
    return value.substr(first, last - first + 1);
}

// Changes text to lower case for comparisons.
string ofApp::toLower(const string& value) const {
    string result = value;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return result;
}

// Adds commas to population numbers.
string ofApp::formatPopulation(long long value) const {
    if (value <= 0) {
        return "Not available";
    }

    const string digits = ofToString(value);
    string formatted;
    for (size_t index = 0; index < digits.length(); ++index) {
        formatted += digits[index];
        const size_t remaining = digits.length() - index - 1;
        if (remaining > 0 && remaining % 3 == 0) {
            formatted += ',';
        }
    }
    return formatted;
}

// Shortens long text so it fits in the interface.
string ofApp::shorten(const string& value, size_t maximumLength) const {
    if (value.length() <= maximumLength) {
        return value;
    }
    return value.substr(0, maximumLength - 3) + "...";
}
