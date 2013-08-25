/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  ButtonSettings.hpp
 *  The nModules Project
 *
 *  Contains all the settings used by button for a particular taskbar.
 *  
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#pragma once

class ButtonSettings;

#include "TaskButton.hpp"

class ButtonSettings
{
    // Button Specific settings
public:
    bool mUseFlashing;
    int mFlashInterval;
    bool mNoIcons;

    float mIconSize;
    float mIconX;
    float mIconY;
    D2D1_RECT_F mIconRect;

    float mOverlayIconSize;
    float mOverlayIconOffsetX;
    float mOverlayIconOffsetY;
    D2D1_RECT_F mOverlayIconRect;

    // Window settings
public:
    WindowSettings mWindowSettings;
    StateRender<TaskButton::State> mStateRender;

    //
public:
    void Load(Settings *settings);
};
