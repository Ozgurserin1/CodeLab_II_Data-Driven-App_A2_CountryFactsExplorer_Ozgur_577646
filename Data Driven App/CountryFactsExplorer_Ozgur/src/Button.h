#pragma once

#include "ofMain.h"

// Stores the size, position and label for a button.
class Button {
public:
    ofRectangle bounds;
    string label;

    Button() = default;
    Button(const ofRectangle& buttonBounds, const string& buttonLabel);

    // Checks if the mouse is inside the button.
    bool contains(float mouseX, float mouseY) const;
};
