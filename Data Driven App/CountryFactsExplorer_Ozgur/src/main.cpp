#include "ofMain.h"
#include "ofApp.h"

// Creates the app window and starts the program.
int main() {
    ofGLFWWindowSettings settings;
    settings.setSize(1280, 820);
    settings.windowMode = OF_WINDOW;

    const auto window = ofCreateWindow(settings);
    ofRunApp(window, make_shared<ofApp>());
    ofRunMainLoop();
}
