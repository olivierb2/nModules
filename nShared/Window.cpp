/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Window.cpp
 *  The nModules Project
 *
 *  A generic drawable window.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include "LiteStep.h"
#include <windowsx.h>
#include <strsafe.h>
#include "../nCoreCom/Core.h"
#include "Window.hpp"
#include "WindowSettings.hpp"
#include <d2d1.h>
#include <dwmapi.h>
#include <dwrite.h>
#include <Wincodec.h>
#include "Factories.h"
#include "Color.h"
#include "MessageHandler.hpp"
#include "../Utilities/StringUtils.h"
#include "../Utilities/Math.h"


using namespace D2D1;


/// <summary>
/// Constructor used to create a DrawableWindow for a pre-existing window. Used by nDesk.
/// </summary>
/// <param name="drawable">Pointer to the drawable which should be updated.</param>
void Window::TextChangeHandler(LPVOID drawable)
{
    ((Window*)drawable)->UpdateText();
}


/// <summary>
/// Common constructor. Called by the other constructors to initalize common settings.
/// </summary>
/// <param name="settings">The settings to use.</param>
/// <param name="msgHandler">The default message handler for this window.</param>
Window::Window(Settings* settings, MessageHandler* msgHandler)
{
    this->activeChild = nullptr;
    this->animating = false;
    ZeroMemory(&this->drawingArea, sizeof(this->drawingArea));
    this->drawingSettings = new WindowSettings();
    this->initialized = false;
    this->isTrackingMouse = false;
    this->msgHandler = msgHandler;
    this->parsedText = nullptr;
    mParent = nullptr;
    this->renderTarget = nullptr;
    mSettings = settings;
    this->text = nullptr;
    this->visible = false;
    mDontForwardMouse = false;
    mCaptureHandler = nullptr;
    mIsChild = false;
    mNeedsUpdate = false;
    mUpdateLockCount = 0;
    this->timerIDs = nullptr;
    this->userMsgIDs = nullptr;
    this->monitorInfo = nullptr;
    this->window = nullptr;
    mCoveredByFullscreen = false;

    // Create the base state
    State* state = new State(_T(""), new Settings(settings), 0, &this->text);
    state->active = true;
    this->activeState = mBaseState = mStates.insert(mStates.begin(), state);
    mBrushOwners[_T("")] = (IBrushOwner*)state;
}


/// <summary>
/// Constructor used to create a DrawableWindow for a pre-existing window. Used by nDesk.
/// </summary>
/// <param name="window">The window to draw to.</param>
/// <param name="prefix">The settings prefix to use.</param>
/// <param name="msgHandler">The default message handler for this window.</param>
Window::Window(HWND window, LPCTSTR prefix, MessageHandler *msgHandler)
    : Window(new Settings(prefix), msgHandler)
{
    this->monitorInfo = new MonitorInfo();
    this->timerIDs = new UIDGenerator<UINT_PTR>(1);
    this->userMsgIDs = new UIDGenerator<UINT>(WM_FIRSTREGISTERED);
    this->window = window;

    this->initialized = true;
    this->visible = true;

    // Configure the mouse tracking struct
    ZeroMemory(&this->trackMouseStruct, sizeof(TRACKMOUSEEVENT));
    this->trackMouseStruct.cbSize = sizeof(TRACKMOUSEEVENT);
    this->trackMouseStruct.hwndTrack = this->window;
    this->trackMouseStruct.dwFlags = TME_LEAVE;
    this->trackMouseStruct.dwHoverTime = 200;
}


/// <summary>
/// Constructor used to create a window with a "Parent" setting.
/// </summary>
/// <param name="parent">The name of the parent's window.</param>
/// <param name="settings">The settings to use.</param>
/// <param name="msgHandler">The default message handler for this window.</param>
Window::Window(LPCTSTR parent, Settings *settings, MessageHandler *msgHandler)
    : Window(new Settings(settings), msgHandler)
{
    StringCchCopy(mParentName, _countof(mParentName), parent);
    mParent = nCore::System::FindRegisteredWindow(mParentName);
    mIsChild = true;
    if (mParent)
    {
        mParent->children.push_back(this);
        this->monitorInfo = mParent->monitorInfo;
        this->window = mParent->window;
    }
    else
    {
        nCore::System::AddWindowRegistrationListener(mParentName, this);
    }
}


/// <summary>
/// Constructor used by LSModule to create a top-level window.
/// </summary>
/// <param name="parent">The window to use as a parent for this top-level window.</param>
/// <param name="windowClass">The windowclass to use for the top-level window.</param>
/// <param name="instance">Used for creating the window.</param>
/// <param name="settings">The settings to use.</param>
/// <param name="msgHandler">The default message handler for this window.</param>
Window::Window(HWND /* parent */, LPCTSTR windowClass, HINSTANCE instance, Settings* settings, MessageHandler* msgHandler)
    : Window(new Settings(settings), msgHandler)
{
    this->monitorInfo = new MonitorInfo();
    this->timerIDs = new UIDGenerator<UINT_PTR>(1);
    this->userMsgIDs = new UIDGenerator<UINT>(WM_FIRSTREGISTERED);

    // Create the window
    this->window = MessageHandler::CreateMessageWindowEx(WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_COMPOSITED,
        windowClass, settings->GetPrefix(), WS_POPUP, 0, 0, 0, 0, NULL, NULL, instance, this);
    SetWindowPos(this->window, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    SetWindowLongPtr(this->window, GWLP_USERDATA, MAGIC_DWORD);

    // Configure the mouse tracking struct
    ZeroMemory(&this->trackMouseStruct, sizeof(TRACKMOUSEEVENT));
    this->trackMouseStruct.cbSize = sizeof(TRACKMOUSEEVENT);
    this->trackMouseStruct.hwndTrack = this->window;
    this->trackMouseStruct.dwFlags = TME_LEAVE;
    this->trackMouseStruct.dwHoverTime = 200;

    // All frame :p
    MARGINS m;
    ZeroMemory(&m, sizeof(m));
    m.cyTopHeight = INT_MAX;
    DwmExtendFrameIntoClientArea(this->window, &m);
}


/// <summary>
/// Constructor used by CreateChild to create a child window.
/// </summary>
/// <param name="parent">The parent of this window.</param>
/// <param name="settings">The settings to use.</param>
/// <param name="msgHandler">The default message handler for this window.</param>
Window::Window(Window* parent, Settings* settings, MessageHandler* msgHandler)
    : Window(new Settings(settings), msgHandler)
{
    this->monitorInfo = parent->monitorInfo;
    mParent = parent;
    mIsChild = true;
    this->window = parent->window;
}


/// <summary>
/// Destroys all children and frees allocated resources.
/// </summary>
Window::~Window()
{
    this->initialized = false;

    if (mParent)
    {
        mParent->RemoveChild(this);
    }
    else if (mIsChild)
    {
        nCore::System::RemoveWindowRegistrationListener(mParentName, this);
    }

    // Register with the core
    if (this->drawingSettings->registerWithCore)
    {
        nCore::System::UnRegisterWindow(mSettings->GetPrefix());
    }

    if (!mIsChild && this->window)
    {
        DestroyWindow(this->window);
    }

    SAFERELEASE(this->parsedText);

    DiscardDeviceResources();

    // Delete all states
    for (auto state : mStates)
    {
        delete state;
    }
    mStates.clear();

    // Delete all overlays
    ClearOverlays();

    // Let the children know that we are vanishing
    for (Window *child : this->children)
    {
        child->ParentLeft();
    }

    if (!mIsChild)
    {
        SAFERELEASE(this->renderTarget);
        SAFEDELETE(this->monitorInfo);
    }

    SAFEDELETE(this->drawingSettings);
    SAFEDELETE(mSettings);
    free((LPVOID)this->text);
}


/// <summary>
/// Adds a brush owner.
/// </summary>
void Window::AddBrushOwner(IBrushOwner *owner, LPCTSTR name)
{
    mBrushOwners[name] = owner;
}


/// <summary>
/// Adds an overlay icon.
/// </summary>
/// <param name="position">Where to place the overlay, relative to the parent.</param>
/// <param name="icon">The icon to use as an overlay.</param>
/// <returns>An object which can be used to modify/remove this overlay.</returns>
Window::OVERLAY Window::AddOverlay(D2D1_RECT_F position, HICON icon, int zOrder)
{
    IWICBitmap *source = nullptr;
    IWICImagingFactory *factory = nullptr;

    Factories::GetWICFactory(reinterpret_cast<LPVOID*>(&factory));

    // Generate a WIC bitmap and call the overloaded AddOverlay function
    factory->CreateBitmapFromHICON(icon, &source);
    return AddOverlay(position, source, zOrder);
}


/// <summary>
/// Adds an overlay image.
/// </summary>
/// <param name="position">Where to place the overlay, relative to the parent.</param>
/// <param name="bitmap">The bitmap to use as an overlay.</param>
/// <returns>An object which can be used to modify/remove this overlay.</returns>
Window::OVERLAY Window::AddOverlay(D2D1_RECT_F position, HBITMAP bitmap, int zOrder)
{
    IWICBitmap *source = nullptr;
    IWICImagingFactory *factory = nullptr;
    
    Factories::GetWICFactory(reinterpret_cast<LPVOID*>(&factory));

    // Generate a WIC bitmap and call the overloaded AddOverlay function
    factory->CreateBitmapFromHBITMAP(bitmap, nullptr, WICBitmapUseAlpha, &source);
    return AddOverlay(position, source, zOrder);
}


/// <summary>
/// Adds an overlay image.
/// </summary>
/// <param name="position">Where to place the overlay, relative to the parent.</param>
/// <param name="source">The bitmap to use as an overlay.</param>
/// <returns>An object which can be used to modify/remove this overlay.</returns>
Window::OVERLAY Window::AddOverlay(D2D1_RECT_F position, IWICBitmapSource* source, int zOrder)
{
    Overlay *overlay = new Overlay(position, this->drawingArea, source, zOrder);
    overlay->ReCreateDeviceResources(this->renderTarget);

    OVERLAY iter;
    for (iter = this->overlays.begin(); iter != this->overlays.end() && iter->GetZOrder() < overlay->GetZOrder(); ++iter);
    return this->overlays.insert(iter.mIter, overlay);
}


/// <summary>
/// Adds a custom painter which is called after children and overlays.
/// </summary>
/// <param name="painter">The painter.</param>
/// <returns>An object which can be used to modify/remove this painter.</returns>
Window::PAINTER Window::AddPostPainter(IPainter* painter)
{
    PAINTER ret = this->postPainters.insert(this->postPainters.end(), painter);
    ret->ReCreateDeviceResources(this->renderTarget);
    ret->UpdatePosition(this->drawingArea);

    return ret;
}


/// <summary>
/// Adds a custom painter which is called before children and overlays.
/// </summary>
/// <param name="painter">The painter.</param>
/// <returns>An object which can be used to modify/remove this painter.</returns>
Window::PAINTER Window::AddPrePainter(IPainter* painter)
{
    PAINTER ret = this->prePainters.insert(this->prePainters.end(), painter);
    ret->ReCreateDeviceResources(this->renderTarget);
    ret->UpdatePosition(this->drawingArea);

    return ret;
}



/// <summary>
/// Adds a new state.
/// </summary>
/// <param name="prefix">The prefix for this state. This is appended to the prefix of this window.</param>
/// <param name="defaultSettings">The default settings for this state.</param>
/// <param name="defaultPriority">The default priority for this state. Higher priority states take precedence over lower priority states.</param>
/// <returns>An object which can be used to activate/clear this state.</returns>
Window::STATE Window::AddState(LPCTSTR prefix, int defaultPriority, StateSettings* defaultSettings, STATE *stateGroup)
{
    State* state = new State(prefix, mBaseState->settings->CreateChild(prefix), defaultPriority, &this->text);
    state->settings->AppendGroup(stateGroup ? (*stateGroup)->settings : mBaseState->settings);
    state->Load(defaultSettings);
    state->UpdatePosition(this->drawingArea);
    state->ReCreateDeviceResources(this->renderTarget);

    mBrushOwners[prefix] = (IBrushOwner*)state;

    // Insert the state based on its priority.
    STATE iter;
    for (iter = mStates.begin(); iter != mStates.end() && iter->priority > state->priority; ++iter);
    return mStates.insert(iter.mIter, state);
}


/// <summary>
/// Actives a certain state.
/// </summary>
/// <param name="state">The state to activate.</param>
void Window::ActivateState(STATE state, bool repaint)
{
    state->active = true;
    if (this->activeState == mStates.end() || this->activeState->priority < state->priority)
    {
        this->activeState = state;
        if (repaint)
        {
            Repaint();
        }
    }
}


/// <summary>
/// Performs an animation step.
/// </summary>
void Window::Animate()
{
    float progress = Easing::Transform(CLAMP(0.0f, mAnimationClock.GetTime()/mAnimationDuration, 1.0f), this->animationEasing);

    if (progress >= 1.0f)
    {
        this->animating = false;
    }

    RECT step;
    step.left = this->animationStart.left + long(progress*(this->animationTarget.left - this->animationStart.left));
    step.top = this->animationStart.top + long(progress*(this->animationTarget.top - this->animationStart.top));
    step.right = this->animationStart.right + long(progress*(this->animationTarget.right - this->animationStart.right));
    step.bottom = this->animationStart.bottom + long(progress*(this->animationTarget.bottom - this->animationStart.bottom));

    SetPosition(step);
}


/// <summary>
/// Clears the specified callback timer, stoping the timer and unregistering the timer ID from the handler that owns it.
/// </summary>
/// <param name="timer">The ID of the timer to clear.</param>
void Window::ClearCallbackTimer(UINT_PTR timer)
{
    if (!mIsChild)
    {
        KillTimer(this->window, timer);
        this->timers.erase(timer);
        this->timerIDs->ReleaseID(timer);
    }
    else if(mParent)
    {
        mParent->ClearCallbackTimer(timer);
    }
}


/// <summary>
/// Removes all overlays from the window.
/// </summary>
void Window::ClearOverlays()
{
    for (Overlay *overlay : this->overlays)
    {
        delete overlay;
    }
    this->overlays.clear();
}


/// <summary>
/// Clears a certain state.
/// </summary>
/// <param name="state">The state to clear.</param>
void Window::ClearState(STATE state, bool repaint)
{
    state->active = false;
    if (state == this->activeState)
    {
        // We just cleared the active state, find the highest priority next active state.
        for (state++; state != mStates.end() && !state->active; ++state);
        this->activeState = state;
        if (repaint)
        {
            Repaint();
        }
    }
}


/// <summary>
/// Creates a child window.
/// </summary>
/// <param name="childSettings">The settings the child window should use.</param>
/// <param name="msgHandler">The default message handler for the child window.</param>
/// <returns>The child window.</returns>
Window *Window::CreateChild(Settings* childSettings, MessageHandler* msgHandler)
{
    Window* child = new Window(this, childSettings, msgHandler);
    children.push_back(child);
    return child;
}


/// <summary>
/// Discards all device dependent resources.
/// </summary>
void Window::DiscardDeviceResources()
{
    if (!mIsChild)
    {
        SAFERELEASE(this->renderTarget);
    }
    else
    {
        this->renderTarget = nullptr;
    }
    
    for (IPainter *painter : this->prePainters)
    {
        painter->DiscardDeviceResources();
    }
    for (Overlay *overlay : this->overlays)
    {
        overlay->DiscardDeviceResources();
    }
    for (State *state : mStates)
    {
        state->DiscardDeviceResources();
    }
    for (IPainter *painter : this->postPainters)
    {
        painter->DiscardDeviceResources();
    }

    // Discard resources for all children as well.
    for (Window *child : this->children)
    {
        child->DiscardDeviceResources();
    }
}


/// <summary>
/// Disables forwarding of mouse events to children.
/// </summary>
void Window::DisableMouseForwarding()
{
    mDontForwardMouse = true;
}


/// <summary>
/// Enables forwarding of mouse events to children.
/// </summary>
void Window::EnableMouseForwarding()
{
    mDontForwardMouse = false;
}


/// <summary>
/// Should be called when a fullscreen window has 
/// </summary>
void Window::FullscreenActivated(HMONITOR monitor, HWND fullscreenWindow)
{
    if (!mIsChild && IsVisible())
    {
        if (MonitorFromWindow(this->window, MONITOR_DEFAULTTONULL) == monitor)
        {
            mCoveredByFullscreen = true;
            if (this->drawingSettings->alwaysOnTop)
            {
                SetWindowPos(this->window, fullscreenWindow, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
            }
        }
    }
}


/// <summary>
/// Enables forwarding of mouse events to children.
/// </summary>
void Window::FullscreenDeactivated(HMONITOR monitor)
{
    if (!mIsChild && mCoveredByFullscreen)
    {
        if (MonitorFromWindow(this->window, MONITOR_DEFAULTTONULL) == monitor)
        {
            mCoveredByFullscreen = false;
            if (this->drawingSettings->alwaysOnTop)
            {
                SetWindowPos(this->window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
            }
        }
    }
}


IBrushOwner *Window::GetBrushOwner(LPCTSTR name)
{
    return mBrushOwners[name];
}


/// <summary>
/// Gets the "Desired" size of the window, given the specified constraints.
/// </summary>
/// <param name="maxWidth">Out. The maximum width to return.</param>
/// <param name="maxHeight">Out. The maximum height to return.</param>
/// <param name="size">Out. The desired size will be placed in this SIZE.</param>
void Window::GetDesiredSize(int maxWidth, int maxHeight, LPSIZE size)
{
    mBaseState->GetDesiredSize(maxWidth, maxHeight, size);
}


/// <summary>
/// Returns the drawing settings for the default state.
/// </summary>
/// <returns>The drawing settings for the default state.</returns>
WindowSettings* Window::GetDrawingSettings()
{
    return this->drawingSettings;
}


/// <summary>
/// Returns an up-to-date MonitorInfo class.
/// </summary>
/// <returns>An up-to-date MonitorInfo class.</returns>
MonitorInfo* Window::GetMonitorInformation()
{
    return this->monitorInfo;
}


/// <summary>
///
/// </summary>
D2D1_RECT_F Window::GetDrawingRect()
{
    return this->drawingArea;
}


ID2D1RenderTarget *Window::GetRenderTarget()
{
    return this->renderTarget;
}


/// <summary>
/// Gets the screen position of the window.
/// </summary>
/// <param name="rect">Out. The screen position of this window will be placed in this rect.</param>
void Window::GetScreenRect(LPRECT rect)
{
    RECT r;
    GetWindowRect(this->window, &r);
    rect->left = r.left + (LONG)this->drawingArea.left;
    rect->top = r.top + (LONG)this->drawingArea.top;
    rect->right = r.left + (LONG)this->drawingArea.right;
    rect->bottom = r.top + (LONG)this->drawingArea.bottom;
}


/// <summary>
/// Returns the current text of this window.
/// </summary>
/// <returns>The current parsed text.</returns>
LPCWSTR Window::GetText()
{
    return this->text;
}


/// <summary>
/// Returns the window handle of the top-level window this window belongs to.
/// </summary>
/// <returns>the window handle.</returns>
HWND Window::GetWindowHandle()
{
    return this->window;
}


/// <summary>
/// Returns the specified state, if it exists.
/// </summary>
State *Window::GetState(LPCTSTR stateName)
{
    for (State *state : mStates)
    {
        if (_tcsicmp(state->mName, stateName) == 0)
        {
            return state;
        }
    }
    return nullptr;
}


/// <summary>
/// Handles window messages for this drawablewindow. Any messages forwarded from here will have the extra parameter set to this.
/// </summary>
/// <param name="window">The handle of the window this message was sent to.</param>
/// <param name="msg">The message.</param>
/// <param name="wParam">Message data.</param>
/// <param name="lParam">Message data</param>
/// <param name="extra">Message data. Not used.</param>
/// <returns>Something</returns>
LRESULT WINAPI Window::HandleMessage(HWND window, UINT msg, WPARAM wParam, LPARAM lParam, LPVOID extra)
{
    UNREFERENCED_PARAMETER(extra);

    // Forward mouse messages to the lowest level child window which the mouse is over.
    if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST && !mDontForwardMouse)
    {
        UpdateLock updateLock(this);

        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);
        MessageHandler *handler = nullptr;

        if (mCaptureHandler == nullptr)
        {
            for (Window *child : this->children)
            {
                if (!child->drawingSettings->clickThrough)
                {
                    D2D1_RECT_F pos = child->drawingArea;
                    if (xPos >= pos.left && xPos <= pos.right && yPos >= pos.top && yPos <= pos.bottom)
                    {
                        handler = child;
                        break;
                    }
                }
            }

            if (msg == WM_MOUSEMOVE)
            {
                if (!mIsChild && !isTrackingMouse)
                {
                    isTrackingMouse = true;
                    TrackMouseEvent(&this->trackMouseStruct);
                }
                if (handler != activeChild)
                {
                    if (activeChild != nullptr)
                    {
                        activeChild->HandleMessage(window, WM_MOUSELEAVE, 0, 0, this);
                    }
                    else
                    {
                        this->msgHandler->HandleMessage(window, WM_MOUSEMOVE, wParam, lParam, this);
                    }
                    activeChild = (Window*)handler;
                }
            }
        }
        else
        {
            handler = mCaptureHandler;
        }

        if (handler == nullptr)
        {
            handler = this->msgHandler;
        }

        // Let our messagehandler deal with it.
        return handler->HandleMessage(window, msg, wParam, lParam, this);
    }

    // Forward keyboard messages to the active child
    if (msg >= WM_KEYFIRST && msg <= WM_KEYLAST)
    {
        UpdateLock updateLock(this);
        if (activeChild != nullptr)
        {
            activeChild->HandleMessage(window, msg, wParam, lParam, this);
        }
    }

    // Handle DrawableWindow messages.
    switch (msg)
    {
    case WM_MOUSELEAVE:
        {
            UpdateLock updateLock(this);
            isTrackingMouse = false;
            if (activeChild != nullptr)
            {
                activeChild->HandleMessage(window, WM_MOUSELEAVE, 0, 0, this);
                activeChild = nullptr;
            }
        }
        break;

    case WM_ERASEBKGND:
        {
        }
        return 1;

    case WM_PAINT:
        {
            bool inAnimation = false;
            RECT updateRect;

            UpdateLock lock(this);

            if (GetUpdateRect(window, &updateRect, FALSE) != FALSE)
            {
                ValidateRect(this->window, nullptr);
                if (SUCCEEDED(ReCreateDeviceResources()))
                {
                    D2D1_RECT_F d2dUpdateRect = D2D1::RectF(
                        updateRect.left, updateRect.top, updateRect.right, updateRect.bottom);

                    this->renderTarget->BeginDraw();
                    this->renderTarget->PushAxisAlignedClip(&d2dUpdateRect, D2D1_ANTIALIAS_MODE_ALIASED);
                    this->renderTarget->Clear();

                    Paint(inAnimation, &d2dUpdateRect);

                    this->renderTarget->PopAxisAlignedClip();

                    // If EndDraw fails we need to recreate all device-dependent resources
                    if (this->renderTarget->EndDraw() == D2DERR_RECREATE_TARGET)
                    {
                        DiscardDeviceResources();
                    }
                }

                // Paint actual owned/child windows.
                //EnumChildWindows(this->window, [] (HWND hwnd, LPARAM) -> BOOL
                //{
                //    SendMessage(hwnd, WM_PAINT, 0, 0);
                //    return TRUE;
                //}, 0);
            }

            // We just painted, don't update
            mNeedsUpdate = false;

            if (inAnimation)
            {
                PostMessage(window, WM_PAINT, 0, 0);
            }
        }
        return 0;

    case WM_TIMER:
        {
            UpdateLock updateLock(this);
            map<UINT_PTR, MessageHandler*>::const_iterator iter = timers.find(wParam);
            if (iter != timers.end())
            {
                return iter->second->HandleMessage(window, msg, wParam, lParam, this);
            }
        }
        return 0;

    case WM_SETTINGCHANGE:
        {
            switch (wParam)
            {
            case SPI_SETWORKAREA:
                {
                    this->monitorInfo->Update();
                }
                break;
            }
        }
        return 0;

    case WM_DISPLAYCHANGE:
        {
            this->monitorInfo->Update();
        }
        return 0;

    case WM_DWMCOLORIZATIONCOLORCHANGED:
        {
            UpdateLock updateLock(this);

            // When the intensity is really high, the alpha drops to 0 :/
            if (wParam >> 24 == 0 && wParam != 0)
            {
                wParam |= 0xFF000000;
            }

            if (UpdateDWMColor(ARGB(wParam)))
            {
                UpdateWindow(this->window);
            }
        }
        return 0;

    case WM_WINDOWPOSCHANGING:
        {
            if (this->drawingSettings->alwaysOnTop)
            {
                LPWINDOWPOS windowPos = LPWINDOWPOS(lParam);
                if (!mCoveredByFullscreen)
                {
                    windowPos->hwndInsertAfter = HWND_TOPMOST;
                }
            }
        }
        return 0;
    }

    // Forward registered user messages.
    if (msg >= WM_FIRSTREGISTERED)
    {
        UpdateLock updateLock(this);
        map<UINT,MessageHandler*>::const_iterator handler = this->userMessages.find(msg);
        if (handler != this->userMessages.end())
        {
            return handler->second->HandleMessage(window, msg, wParam, lParam, this);
        }
    }

    // Let the default messagehandler deal with anything else, if it is initialized.
    if (this->msgHandler && this->msgHandler->mInitialized)
    {
        return this->msgHandler->HandleMessage(window, msg, wParam, lParam, this);
    }
    else
    {
        return DefWindowProc(window, msg, wParam, lParam);
    }
}


/// <summary>
/// Hides the window.
/// </summary>
void Window::Hide()
{
    this->visible = false;
    if (!mIsChild)
    {
        ShowWindow(this->window, SW_HIDE);
    }
    else
    {
        RECT r = { (LONG)drawingArea.left, (LONG)drawingArea.top, (LONG)drawingArea.right, (LONG)drawingArea.bottom };
        mParent->Repaint(&r);
    }
}


/// <summary>
/// Initalizes this window.
/// </summary>
/// <param name="defaultSettings">The default settings for this window.</param>
/// <param name="baseStateDefaults">The default settings for the base state.</param>
void Window::Initialize(WindowSettings* defaultSettings, StateSettings* baseStateDefaults)
{
    // Load settings.
    this->drawingSettings->Load(mSettings, defaultSettings);

    // Load the base state
    mBaseState->Load(baseStateDefaults);

    // Register with the core.
    if (this->drawingSettings->registerWithCore)
    {
        nCore::System::RegisterWindow(mSettings->GetPrefix(), this);
    }

    // Put the window in its correct position.
    SetPosition(this->drawingSettings->x, this->drawingSettings->y,
        this->drawingSettings->width, this->drawingSettings->height);

    // Create D2D resources.
    ReCreateDeviceResources();

    // AlwaysOnTop
    if (!mIsChild && this->drawingSettings->alwaysOnTop)
    {
        ::SetParent(this->window, nullptr);
        SetWindowPos(this->window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
    }
    
    // Set the text.
    SetText(this->drawingSettings->text);

    this->initialized = true;
}


/// <summary>
/// Check if this window is currently visible.
/// </summary>
/// <returns>True if this window and all its ancestors are visible.</returns>
bool Window::IsChild()
{
    return mIsChild;
}


/// <summary>
/// Check if this window is currently visible.
/// </summary>
/// <returns>True if this window and all its ancestors are visible.</returns>
bool Window::IsVisible()
{
    if (mParent)
    {
        return this->visible && mParent->IsVisible();
    }
    return this->visible;
}


/// <summary>
/// Moves the window.
/// </summary>
/// <param name="x">The x coordinate to move the window to. Relative to the parent.</param>
/// <param name="y">The y coordinate to move the window to. Relative to the parent.</param>
void Window::Move(int x, int y)
{
    SetPosition(x, y, this->drawingSettings->width, this->drawingSettings->height);
}


/// <summary>
/// Removes the specified child.
/// </summary>
void Window::Paint(bool &inAnimation, D2D1_RECT_F *updateRect)
{
    UpdateLock lock(this);
    if (this->visible && Math::RectIntersectArea(updateRect, &this->drawingArea) > 0)
    {
        this->renderTarget->PushAxisAlignedClip(this->drawingArea, D2D1_ANTIALIAS_MODE_ALIASED);

        // Paint the active state.
        this->activeState->Paint(this->renderTarget);
        
        // Pre painters.
        for (IPainter *painter : this->prePainters)
        {
            painter->Paint(this->renderTarget);
        }

        // Paint all overlays.
        PaintOverlays(updateRect);

        // Paint all children.
        PaintChildren(inAnimation, updateRect);

        // Post painters.
        for (IPainter *painter : this->postPainters)
        {
            painter->Paint(this->renderTarget);
        }
        
        inAnimation |= this->animating;
        if (this->animating)
        {
            Animate();
        }
        
        this->renderTarget->PopAxisAlignedClip();
    }
}


/// <summary>
/// Paints all child windows.
/// </summary>
void Window::PaintChildren(bool &inAnimation, D2D1_RECT_F *updateRect)
{
    for (Window *child : this->children)
    {
        child->Paint(inAnimation, updateRect);
    }
}


/// <summary>
/// Paints all overlays.
/// </summary>
void Window::PaintOverlays(D2D1_RECT_F *updateRect)
{
    for (Overlay *overlay : this->overlays)
    {
        overlay->Paint(this->renderTarget);
    }
}


/// <summary>
/// Called by the parent when it is passing away.
/// </summary>
void Window::ParentLeft()
{
    mParent = nullptr;
    UpdateParentVariables();
    SendToAll(nullptr, WM_TOPPARENTLOST, 0, 0, this);
    
    if (*mParentName != '\0') 
    {
        nCore::System::AddWindowRegistrationListener(mParentName, this);
    }
}


/// <summary>
/// Registers an user message (>= WM_USER) which will be forwarded to the specified handler.
/// </summary>
/// <param name="msgHandler">The handler which will receive the message.</param>
/// <returns>The assigned message ID.</returns>
UINT Window::RegisterUserMessage(MessageHandler* msgHandler)
{
    if (!mIsChild)
    {
        UINT ret = this->userMsgIDs->GetNewID();
        this->userMessages.insert(std::pair<UINT, MessageHandler*>(ret, msgHandler));
        return ret;
    }
    else if (mParent)
    {
        return mParent->RegisterUserMessage(msgHandler);
    }
    else
    {
        TRACE("RegisterUserMessage failed!");
        return 0;
    }
}


/// <summary>
/// Releases a user message. It will no longer be forwarded to the specified handler if received.
/// </summary>
/// <param name="message">The ID of the message to release.</param>
void Window::ReleaseUserMessage(UINT message)
{
    if (!mIsChild)
    {
        this->userMessages.erase(message);
        this->userMsgIDs->ReleaseID(message);
    }
    else if (mParent)
    {
        mParent->ReleaseUserMessage(message);
    }
}


/// <summary>
/// Resize the window.
/// </summary>
/// <param name="width">The width to resize the window to.</param>
/// <param name="height">The height to resize the window to.</param>
void Window::Resize(int width, int height)
{
    SetPosition(this->drawingSettings->x, this->drawingSettings->y, width, height);
}


/// <summary>
/// (Re)Creates all device-dependent resources.
/// </summary>
/// <returns>S_OK if successful, an error code otherwise.</returns>
HRESULT Window::ReCreateDeviceResources()
{
    HRESULT hr = S_OK;

    if (!this->renderTarget)
    {
        if (!mIsChild)
        {
            ID2D1Factory *pD2DFactory = nullptr;
            hr = Factories::GetD2DFactory(reinterpret_cast<LPVOID*>(&pD2DFactory));

            // Create the render target
            if (SUCCEEDED(hr))
            {
                D2D1_SIZE_U size = D2D1::SizeU(this->drawingSettings->width, this->drawingSettings->height);
                hr = pD2DFactory->CreateHwndRenderTarget(
                    RenderTargetProperties(
                        D2D1_RENDER_TARGET_TYPE_DEFAULT,
                        PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
                    ),
                    HwndRenderTargetProperties(this->window, size),
                    &this->renderTarget
                );
                if (SUCCEEDED(hr))
                {
                    this->renderTarget->SetTextAntialiasMode(this->drawingSettings->textAntiAliasMode);
                }
            }
        }
        else
        {
            if (!mParent)
            {
                return S_FALSE;
            }
            this->renderTarget = mParent->renderTarget;
        }

        if (SUCCEEDED(hr))
        {
            for (IPainter *painter : this->prePainters)
            {
                painter->ReCreateDeviceResources(this->renderTarget);
            }

            for (State *state : mStates)
            {
                state->ReCreateDeviceResources(this->renderTarget);
            }

            for (Overlay *overlay : this->overlays)
            {
                overlay->ReCreateDeviceResources(this->renderTarget);
            }
    
            for (IPainter *painter : this->postPainters)
            {
                painter->ReCreateDeviceResources(this->renderTarget);
            }

            // Recreate resources for all children as well.
            for (Window *child : this->children)
            {
                child->ReCreateDeviceResources();
            }
        }
    }

    return hr;
}


/// <summary>
/// Releases a SetMouseCapture
/// </summary>
void Window::PopUpdateLock()
{
    if (!mIsChild)
    {
        if (--mUpdateLockCount == 0 && mNeedsUpdate)
        {
            mNeedsUpdate = false;
            UpdateWindow(GetWindowHandle());
        }
    }
    else if (mParent)
    {
        mParent->PopUpdateLock();
    }
}


/// <summary>
/// Releases a SetMouseCapture
/// </summary>
void Window::PushUpdateLock()
{
    if (!mIsChild)
    {
        ++mUpdateLockCount;
    }
    else if (mParent)
    {
        mParent->PushUpdateLock();
    }
}


/// <summary>
/// Releases a SetMouseCapture
/// </summary>
void Window::ReleaseMouseCapture()
{
    if (!mIsChild)
    {
        ReleaseCapture();
        this->mCaptureHandler = nullptr;
    }
    else if(mParent)
    {
        mParent->ReleaseMouseCapture();
    }
}


/// <summary>
/// Removes the specified child.
/// </summary>
/// <param name="child">The child to remove.</param>
void Window::RemoveChild(Window *child)
{
    this->children.remove(child);
    if (child == this->activeChild)
    {
        this->activeChild = nullptr;
    }
}


/// <summary>
/// Removes the specified overlay.
/// </summary>
/// <param name="overlay">The overlay to remove.</param>
void Window::RemoveOverlay(OVERLAY overlay)
{
    if (overlay.mValid)
    {
        delete *overlay.mIter;
        this->overlays.erase(overlay.mIter);
    }
}


/// <summary>
/// Repaints the window.
/// </summary>
/// <param name="region">The area of the window to repaint. If NULL, the whole window is repainted.</param>
void Window::Repaint(LPCRECT region)
{
    if (this->initialized && this->visible)
    {
        if (mIsChild)
        {
            if (mParent != nullptr)
            {
                if (region != nullptr)
                {
                    mParent->Repaint(region);
                }
                else
                {
                    RECT r = { (LONG)drawingArea.left, (LONG)drawingArea.top, (LONG)drawingArea.right, (LONG)drawingArea.bottom };
                    mParent->Repaint(&r);
                }
            }
        }
        else {
            InvalidateRect(this->window, region, TRUE);
            if (mUpdateLockCount == 0)
            {
                UpdateWindow(this->window);
            }
            else
            {
                mNeedsUpdate = true;
            }
        }
    }
}


/// <summary>
/// Repaints the window.
/// </summary>
/// <param name="region">The area of the window to repaint. If NULL, the whole window is repainted.</param>
void Window::Repaint(const D2D1_RECT_F *region)
{
    if (this->initialized && this->visible)
    {
        RECT r;
        if (region == nullptr)
        {
            region = &this->drawingArea;
        }

        r.left = (LONG)(region->left - 1.5f);
        r.top = (LONG)(region->top - 1.5f);
        r.bottom = (LONG)(region->bottom + 1.5f);
        r.right = (LONG)(region->right + 1.5f);

        Repaint(&r);
    }
}


/// <summary>
/// Modifies the AlwaysOnTop setting
/// </summary>
void Window::SetAlwaysOnTop(bool value)
{
    bool oldValue = this->drawingSettings->alwaysOnTop;
    this->drawingSettings->alwaysOnTop = true;
    if (!mIsChild && !mCoveredByFullscreen)
    {
        if (value)
        {
            SetWindowPos(this->window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
        }
        else if (oldValue)
        {
            SetWindowPos(this->window, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
        }
    }
}


/// <summary>
/// Starts a new animation, or updates the parameters of the current one.
/// </summary>
/// <param name="x">The x coordinate to animate to.</param>
/// <param name="y">The y coordinate to animate to.</param>
/// <param name="width">The width to animate to.</param>
/// <param name="height">The height to animate to.</param>
/// <param name="duration">The number of milliseconds to complete the animation in.</param>
/// <param name="easing">The easing to use.</param>
void Window::SetAnimation(int x, int y, int width, int height, int duration, Easing::Type easing)
{
    RECT target = { x, y, x + width, y + height };
    this->animationTarget = target;
    this->animationStart.top = this->drawingSettings->y;
    this->animationStart.left = this->drawingSettings->x;
    this->animationStart.bottom = this->drawingSettings->y + this->drawingSettings->height;
    this->animationStart.right = this->drawingSettings->x + this->drawingSettings->width;
    this->animationEasing = easing;

    mAnimationClock.Clock();
    mAnimationDuration = duration / 1000.0f;

    this->animating = true;

    Repaint();
}


/// <summary>
/// Creates a new timer which is forwarded to the specified handler.
/// </summary>
/// <param name="elapse">The uElapse parameter of SetTimer.</param>
/// <param name="msgHandler">The handler WM_TIMER messags with this ID are sent to.</param>
/// <returns>The assigned timer ID.</returns>
UINT_PTR Window::SetCallbackTimer(UINT elapse, MessageHandler* msgHandler)
{
    if (!mIsChild)
    {
        UINT_PTR ret = SetTimer(this->window, this->timerIDs->GetNewID(), elapse, NULL);
        this->timers.insert(std::pair<UINT_PTR, MessageHandler*>(ret, msgHandler));
        return ret;
    }
    else if (mParent)
    {
        return mParent->SetCallbackTimer(elapse, msgHandler);
    }
    else
    {
        TRACE("SetCallbackTimer failed!");
        return 0;
    }
}


/// <summary>
/// Modifies the ClickThrough setting
/// </summary>
void Window::SetClickThrough(bool value)
{
    this->drawingSettings->clickThrough = value;
}


/// <summary>
/// Redirects input to the selected message handler, regardless of where the mouse is.
/// </summary>
void Window::SetMouseCapture(MessageHandler *captureHandler)
{
    if (!mIsChild)
    {
        SetCapture(this->window);
        this->mCaptureHandler = captureHandler;
    }
    else if (mParent) { // Just ignore this request if we are a child without a parent.
        mParent->SetMouseCapture(captureHandler == nullptr ? this : captureHandler);
    }
}


/// <summary>
/// Moves and resizes the window.
/// </summary>
/// <param name="rect">The new position of the window.</param>
void Window::SetPosition(RECT rect)
{
    SetPosition(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
}


/// <summary>
/// Send the specified message to all children, all the way down the tree.
/// </summary>
void Window::SendToAll(HWND window, UINT msg, WPARAM wParam, LPARAM lParam, LPVOID data)
{
    this->msgHandler->HandleMessage(window, msg, wParam, lParam, data);
    for (Window *child : this->children)
    {
        child->SendToAll(window, msg, wParam, lParam, data);
    }
}

// Called when the DWM color has changed. Windows should invalidate
// if appropriate, but not update, in response to this.
bool Window::UpdateDWMColor(ARGB newColor)
{
    bool ret = false;

    // It's important that ret is on the right hand side, to prevent short-circuiting
    for (IPainter *painter : this->prePainters)
    {
        ret = painter->UpdateDWMColor(newColor, this->renderTarget) || ret;
    }

    for (State *state : mStates)
    {
        ret = state->UpdateDWMColor(newColor, this->renderTarget) || ret;
    }
    
    for (IPainter *painter : this->postPainters) 
    {
        ret = painter->UpdateDWMColor(newColor, this->renderTarget) || ret;
    }

    if (ret)
    {
        RECT r = { (LONG)drawingArea.left, (LONG)drawingArea.top, (LONG)drawingArea.right, (LONG)drawingArea.bottom };
        InvalidateRect(this->window, &r, TRUE);
    }

    for (Window *child : this->children)
    {
        ret = child->UpdateDWMColor(newColor) || ret;
    }

    return ret;
}


/// <summary>
/// Updates variables which are dependent on the parent window.
/// </summary>
void Window::UpdateParentVariables()
{
    if (mParent)
    {
        this->monitorInfo = mParent->monitorInfo;
        this->window = mParent->window;
    }
    else
    {
        this->monitorInfo = nullptr;
        this->window = nullptr;
    }

    for (Window *child : this->children)
    {
        child->UpdateParentVariables();
    }
}


/// <summary>
/// Specifies a new parent for this child.
/// </summary>
void Window::SetParent(Window *newParent)
{
    assert(mParent == nullptr);
    
    mParent = newParent;
    mParent->children.push_back(this);

    UpdateParentVariables();
    SendToAll(this->window, WM_NEWTOPPARENT, 0, 0, this);

    SetPosition(this->drawingSettings->x, this->drawingSettings->y,
        this->drawingSettings->width, this->drawingSettings->height);
    ReCreateDeviceResources();
    Repaint();
}


/// <summary>
/// Moves and resizes the window.
/// </summary>
/// <param name="x">The x coordinate to move the window to. Relative to the parent.</param>
/// <param name="y">The y coordinate to move the window to. Relative to the parent.</param>
/// <param name="width">The width to resize the window to.</param>
/// <param name="height">The height to resize the window to.</param>
void Window::SetPosition(int x, int y, int width, int height, LPARAM extra)
{
    UpdateLock lock(this);

    //
    bool isResize = width != this->drawingSettings->width || height != this->drawingSettings->height;

    if (isResize || mIsChild)
    {
        Repaint();
    }

    // Update the drawing settings.
    this->drawingSettings->x = x;
    this->drawingSettings->y = y;
    this->drawingSettings->width = width;
    this->drawingSettings->height = height;

    // Position the window and/or set the backarea.
    if (!mIsChild)
    {
        SetWindowPos(this->window, 0, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
        this->drawingArea = D2D1::RectF(0, 0, (float)width, (float)height);
        if (this->renderTarget)
        {
            D2D1_SIZE_U size = D2D1::SizeU(width, height);
            this->renderTarget->Resize(size);
        }
    }
    else if(mParent)
    {
        this->drawingArea = D2D1::RectF(
            mParent->drawingArea.left + x,
            mParent->drawingArea.top + y,
            mParent->drawingArea.left + x + width,
            mParent->drawingArea.top + y + height
        );
    }

    // Update all paintables.
    for (State *state : mStates)
    {
        state->UpdatePosition(this->drawingArea);
    }
    for (Overlay *overlay : this->overlays)
    {
        overlay->UpdatePosition(this->drawingArea);
    }
    for (IPainter *painter : this->prePainters)
    {
        painter->UpdatePosition(this->drawingArea);
    }
    for (IPainter *painter : this->postPainters)
    {
        painter->UpdatePosition(this->drawingArea);
    }
    if (isResize || mIsChild)
    {
        for (Window *child : this->children)
        {
            child->Move(child->drawingSettings->x, child->drawingSettings->y);
        }
    }

    if (isResize || mIsChild)
    {
        Repaint();
    }

    //
    if (isResize)
    {
        this->msgHandler->HandleMessage(GetWindowHandle(), WM_SIZECHANGE, MAKEWPARAM(width, height), extra, this);
    }
}


/// <summary>
/// Shows the window.
/// </summary>
void Window::Show(int nCmdShow)
{
    if (!mIsChild)
    {
        ShowWindow(this->window, nCmdShow);
        if (this->drawingSettings->alwaysOnTop)
        {
            SetWindowPos(this->window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
        }
    }
    this->visible = true;
}


/// <summary>
/// Sizes the window to fit its current text.
/// </summary>
/// <param name="maxWidth">The maximum width to size the window to.</param>
/// <param name="maxHeight">The maximum height to size the window to.</param>
/// <param name="minWidth">The minimum width to size the window to.</param>
/// <param name="minHeight">The minimum height to size the window to.</param>
void Window::SizeToText(int maxWidth, int maxHeight, int minWidth, int minHeight)
{
    SIZE s;
    GetDesiredSize(maxWidth, maxHeight, &s);
    s.cx = std::max(s.cx, (long)minWidth);
    s.cy = std::max(s.cy, (long)minHeight);
    this->SetPosition(this->drawingSettings->x, this->drawingSettings->y, s.cx, s.cy);
}


/// <summary>
/// Sets the text for this window.
/// </summary>
/// <param name="text">The text for this window.</param>
void Window::SetText(LPCWSTR text)
{
    if (this->drawingSettings->evaluateText)
    {
        SAFEDELETE(this->parsedText);
        this->parsedText = (IParsedText*)nCore::System::ParseText(text);
        this->parsedText->SetChangeHandler(TextChangeHandler, this);
        UpdateText();
    }
    else
    {
        this->text = StringUtils::ReallocOverwrite(const_cast<LPWSTR>(this->text), text);
    }
}


/// <summary>
/// Sets the text offsets for all states.
/// </summary>
/// <param name="left">The text offset from the left.</param>
/// <param name="top">The text offset from the top.</param>
/// <param name="right">The text offset from the right.</param>
/// <param name="bottom">The text offset from the bottom.</param>
void Window::SetTextOffsets(float left, float top, float right, float bottom)
{
    for (State *state : mStates)
    {
        state->SetTextOffsets(left, top, right, bottom);
    }
}


/// <summary>
/// Toggles the specified state.
/// </summary>
/// <param name="state">The state to toggle</param>
void Window::ToggleState(STATE state)
{
    if (state->active)
    {
        ClearState(state);
    }
    else
    {
        ActivateState(state);
    }
}


/// <summary>
/// Forcibly updates the text.
/// </summary>
void Window::UpdateText()
{
    if (this->drawingSettings->evaluateText)
    {
        WCHAR buf[4096];
        this->parsedText->Evaluate(buf, 4096);
        this->text = StringUtils::ReallocOverwrite(const_cast<LPWSTR>(this->text), buf);
    }
    else
    {
        this->text = StringUtils::ReallocOverwrite(const_cast<LPWSTR>(this->text), this->drawingSettings->text);
    }
    Repaint();
}