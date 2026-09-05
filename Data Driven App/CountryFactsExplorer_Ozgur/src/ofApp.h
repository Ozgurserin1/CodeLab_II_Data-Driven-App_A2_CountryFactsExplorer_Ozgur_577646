#pragma once

#include "ofMain.h"
#include "Button.h"
#include "Country.h"
#include "CountryService.h"

// Stores the main screen state.
enum class AppState {
    Idle,
    Loading,
    Success,
    Error
};

class ofApp : public ofBaseApp {
public:
    // Runs the main openFrameworks app events.
    void setup() override;
    void update() override;
    void draw() override;
    void exit() override;

    // Handles keyboard, mouse and window events.
    void keyPressed(int key) override;
    void mousePressed(int x, int y, int button) override;
    void windowResized(int width, int height) override;
    void urlResponse(ofHttpResponse& response);

private:
    // Stores the current country and app state.
    CountryService countryService;
    Country currentCountry;
    AppState appState = AppState::Idle;

    // Stores search text and saved country lists.
    string inputText;
    string statusMessage;
    vector<string> searchHistory;
    vector<string> favourites;
    vector<ofRectangle> historyBounds;
    vector<ofRectangle> favouriteBounds;

    // Stores the main screen areas and buttons.
    ofRectangle searchBox;
    ofRectangle mapPanel;
    ofRectangle detailPanel;
    ofRectangle themeToggle;
    ofRectangle favouriteButton;
    Button searchButton;
    Button clearButton;
    Button randomButton;

    // Stores images and screen settings.
    ofImage flagImage;
    ofImage worldMapImage;
    bool flagLoading = false;
    bool inputFocused = true;
    bool darkMode = false;
    float loadingAngle = 0.0f;
    float pinPulse = 0.0f;

    // Stores the fonts used by the app.
    ofTrueTypeFont fontSmall;
    ofTrueTypeFont fontBody;
    ofTrueTypeFont fontBodyBold;
    ofTrueTypeFont fontHeading;
    ofTrueTypeFont fontCountry;

    // Runs search, favourite and layout actions.
    void setupLayout();
    void startSearch();
    void searchRandomCountry();
    void clearSearch();
    void addToHistory(const string& countryName);
    void toggleFavourite();
    bool isCurrentCountryFavourite() const;
    void loadFavourites();
    void saveFavourites() const;

    // Draws the main parts of the screen.
    void drawHeader();
    void drawSearchControls();
    void drawStatus();
    void drawWorldMap();
    void drawCountryPanel();
    void drawEmptyPanel();
    void drawLoadingOverlay();
    void drawHistoryAndFavourites();
    void drawFooter();

    // Draws smaller screen parts.
    void drawGlobe(float x, float y, float radius, const ofColor& colour);
    void drawMapPin();
    void drawInfoRow(const string& icon, const string& label, const string& value,
                     float x, float y, const ofColor& accent);
    void drawTagRow(const string& title, const vector<string>& items, float y,
                    vector<ofRectangle>& bounds, bool favouriteStyle);
    void drawControlButton(const Button& button, bool primary, const ofColor& accent);
    void drawText(const string& text, float x, float y, const ofColor& colour,
                  ofTrueTypeFont& font);
    float textWidth(const string& text, ofTrueTypeFont& font) const;

    // Returns the colours for the current theme.
    ofColor pageBackground() const;
    ofColor panelColour() const;
    ofColor primaryText() const;
    ofColor secondaryText() const;
    ofColor borderColour() const;
    ofColor accentColour() const;

    // Helps clean and format text values.
    string trim(const string& value) const;
    string toLower(const string& value) const;
    string formatPopulation(long long value) const;
    string shorten(const string& value, size_t maximumLength) const;
};
