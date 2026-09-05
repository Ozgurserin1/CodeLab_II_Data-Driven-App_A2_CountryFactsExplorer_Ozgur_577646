#include "Button.h"

// Saves the button position and text.
Button::Button(const ofRectangle& buttonBounds, const string& buttonLabel)
    : bounds(buttonBounds), label(buttonLabel) {
}

// Checks if a point is inside the button area.
bool Button::contains(float mouseX, float mouseY) const {
    return bounds.inside(mouseX, mouseY);
}
