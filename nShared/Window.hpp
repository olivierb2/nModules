/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Window.hpp
 *  The nModules Project
 *
 *  Essentially, anything that needs to draw.
 *  
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#pragma once

#include "../Utilities/Common.h"
#include "WindowSettings.hpp"
#include "StateSettings.hpp"
#include "State.hpp"
#include "Overlay.hpp"
#include "MessageHandler.hpp"
#include <d2d1.h>
#include <vector>
#include <list>
#include <map>
#include "../Utilities/UIDGenerator.hpp"
#include "MonitorInfo.hpp"
#include "Easing.h"
#include "../nCore/IParsedText.hpp"
#include "IPainter.hpp"
#include "BrushSettings.hpp"
#include "../Utilities/PointerIterator.hpp"
#include "../Utilities/StopWatch.hpp"
#include "IBrushOwner.hpp"
#include "WindowClass.hpp"


using std::vector;
using std::list;
using std::map;


class Window : MessageHandler, IDropTarget
{
    // typedefs
public:
    typedef PointerIterator<list<State*>::iterator, State> STATE;
    typedef PointerIterator<list<Overlay*>::iterator, Overlay> OVERLAY;
    typedef PointerIterator<list<IPainter*>::iterator, IPainter> PAINTER;

public:
    friend class UpdateLock;
    class UpdateLock
    {
    public:
        explicit UpdateLock(Window*);
        ~UpdateLock();

    public:
        void Unlock();

    private:
        Window *mWindow;
        bool mLocked;
    };

    // enums
public:
    // Reserved window messages.
    enum ReservedMessages
    {
        // Sent when the top-level parent has gone away. At this point, all
        // calls to RegisterUserMessage and SetCallbackTimer are invalidated.
        WM_TOPPARENTLOST = WM_USER,

        // Sent when the top-level parent has gone away. At this point, all 
        // calls to RegisterUserMessage and SetCallbackTimer are invalidated.
        // However, the receiver can now feel free to re-register with either
        // of those functions.
        WM_NEWTOPPARENT,

        // Sent after the window has been resized.
        // HIGHWORD(wParam): width
        // LOWORD(wParam): height
        // lParam: Custom value sent to resize
        WM_SIZECHANGE,

        // The first message aviailable for registration.
        WM_FIRSTREGISTERED
    };

    // Constructors & Destructor
public:
    // Constructor used for top-level windows.
    explicit Window(HWND parent, LPCTSTR windowClass, HINSTANCE instance, Settings *settings, MessageHandler *msgHandler);

    // Constructor used to create a window with a "Parent" setting.
    explicit Window(LPCTSTR parent, Settings *settings, MessageHandler *msgHandler);

    // Destructor
    virtual ~Window();

protected:
    // Used by nDesk.
    explicit Window(HWND window, LPCTSTR prefix, MessageHandler *msgHandler);

private:
    // Used to initalize common settings before the other constructors run.
    explicit Window(Settings *settings, MessageHandler *msgHandler);

    // Constructor used by CreateChild to create a child window.
    explicit Window(Window *parent, Settings *settings, MessageHandler *msgHandler);

    // 
public:
    void AddBrushOwner(IBrushOwner *owner, LPCTSTR name);

    // Adds an overlay.
    OVERLAY AddOverlay(D2D1_RECT_F position, HBITMAP image, int zOrder = 5);
    OVERLAY AddOverlay(D2D1_RECT_F position, HICON icon, int zOrder = 5);
    OVERLAY AddOverlay(D2D1_RECT_F position, IWICBitmapSource *source, int zOrder = 5);

    // Adds custom painters
    PAINTER AddPrePainter(IPainter *painter);
    PAINTER AddPostPainter(IPainter *painter);

    // Adds a new state.
    STATE AddState(LPCTSTR prefix, int defaultPriority, StateSettings *defaultSettings = nullptr, STATE *stateGroup = nullptr);

    // Marks a particular state as active.
    void ActivateState(STATE state, bool repaint = true);

    // Stops a timer.
    void ClearCallbackTimer(UINT_PTR);

    // Removes all overlays.
    void ClearOverlays();

    // Clears the active flag of a particular state.
    void ClearState(STATE state, bool repaint = true);

    // Creates a new child window.
    Window *CreateChild(Settings *childSettings, MessageHandler *msgHandler);

    // Disables forwarding of mouse events to children.
    void DisableMouseForwarding();

    // Enables forwarding of mouse events to children.
    void EnableMouseForwarding();

    //
    void FullscreenActivated(HMONITOR, HWND);

    //
    void FullscreenDeactivated(HMONITOR);

    //
    IBrushOwner *GetBrushOwner(LPCTSTR name);

    // Returns the current drawing settings.
    WindowSettings *GetDrawingSettings();

    // Returns a MonitorInfo class which will be kept up-to-date for the duration of this windows lifetime.
    MonitorInfo *GetMonitorInformation();

    // Gets the "desired" size for a given width and height.
    void GetDesiredSize(int maxWidth, int maxHeight, LPSIZE size);

    // Returns the position of this window, relative to its top-level parent.
    D2D1_RECT_F GetDrawingRect();

    //
    ID2D1RenderTarget *GetRenderTarget();

    // Returns the screen-coordinate position of this window.
    void GetScreenRect(LPRECT rect);
    
    //
    State *GetState(LPCTSTR stateName);

    // Returns the handle to the top-level window in this window stack.
    HWND GetWindowHandle();

    // Retrives the "parsed" text.
    LPCWSTR GetText();

    // Handles window messages.
    LRESULT WINAPI HandleMessage(HWND, UINT, WPARAM, LPARAM, LPVOID) override;

    // Hides this window.
    void Hide();

    // Initializes the Window.
    void Initialize(WindowSettings* defaultSettings = nullptr, StateSettings* baseStateDefaults = nullptr);

    //
    bool IsChild();

    // Returns whether or not this window is visible.
    bool IsVisible();

    // Moves this window.
    void Move(int x, int y);

    //
    void PopUpdateLock();
    void PushUpdateLock();

    // Stops capturing mouse input.
    void ReleaseMouseCapture();

    // Registers a user message
    UINT RegisterUserMessage(MessageHandler* msgHandler);

    // Unregisters a user message
    void ReleaseUserMessage(UINT message);

    // Removes the specified overlay
    void RemoveOverlay(OVERLAY overlay);

    // Forces this window to redraw.
    void Repaint(LPCRECT region);
    void Repaint(const D2D1_RECT_F *region = nullptr);

    // Resizes the window.
    void Resize(int width, int height);

    //
    void SetAlwaysOnTop(bool value);

    // Performs an animation.
    void SetAnimation(int x, int y, int width, int height, int duration, Easing::Type easing);

    // Registers a timer
    UINT_PTR SetCallbackTimer(UINT elapse, MessageHandler* msgHandler);

    //
    void SetClickThrough(bool value);

    // Sets the message handler for this window.
    void SetMessageHandler(MessageHandler *msgHandler);

    // Captures mouse input.
    void SetMouseCapture(MessageHandler *captureHandler = nullptr);

    // Sets the paragraph alignment of this Window.
    void SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT alignment);

    // Specifies a new parent for this child.
    void SetParent(Window *newParent);

    // Sets the position of this Window, relative to its parent.
    void SetPosition(RECT rect);
    void SetPosition(int x, int y, int width, int height, LPARAM extra = 0);

    // Sets the text of this window.
    void SetText(LPCWSTR text);

    // Sets the text alignment of this Window.
    void SetTextAlignment(DWRITE_TEXT_ALIGNMENT alignment);

    // Sets the text offsets for all states.
    void SetTextOffsets(float left, float top, float right, float bottom);

    // Shows this window.
    void Show(int nCmdShow = SW_SHOWNOACTIVATE);

    // Sizes the window to fit the text.
    void SizeToText(int maxWidth, int maxHeight, int minWidth = 0, int minHeight = 0);

    // Toggles the specified state.
    void ToggleState(STATE state);

    // Forcibly updates the text.
    void UpdateText();


    // IUnknown
public:
    ULONG WINAPI AddRef();
    HRESULT WINAPI QueryInterface(REFIID riid, void **ppvObject);
    ULONG WINAPI Release();


    // IDropTarget
public:
    HRESULT WINAPI DragEnter(IDataObject *dataObj, DWORD keyState, POINTL point, DWORD *effect) override;
    HRESULT WINAPI DragOver(DWORD keyState, POINTL point, DWORD *effect) override;
    HRESULT WINAPI DragLeave() override;
    HRESULT WINAPI Drop(IDataObject *dataObj, DWORD keyState, POINTL point, DWORD *effect) override;


protected:
    // Paints this window.
    void Paint(bool &inAnimation, D2D1_RECT_F *updateRect);

    // Paints all overlays.
    void PaintOverlays(D2D1_RECT_F *updateRect);

    // Paints all children.
    void PaintChildren(bool &inAnimation, D2D1_RECT_F *updateRect);

    // The render target to draw to.
    ID2D1HwndRenderTarget* renderTarget;

    // Discards device-dependent stuff.
    void DiscardDeviceResources();

    // (Re)Creates D2D device-dependent stuff.
    HRESULT ReCreateDeviceResources();

private:
    // Called by IParsedText objects when we should update the text.
    static void TextChangeHandler(LPVOID drawable);

    // Sends a message to every child window, all the way down the tree.
    void SendToAll(HWND, UINT, WPARAM, LPARAM, LPVOID);

    // Called when the DWM color has changed. Windows should invalidate
    // if appropriate, but not update, in response to this.
    bool UpdateDWMColor(ARGB newColor);

private:
    //
    void Animate();

    // Removes the specified child.
    void RemoveChild(Window* child);

    //
    void ParentLeft();
    void UpdateParentVariables();

protected:
    //
    bool mNeedsUpdate;

private:
    // The child window the mouse is currently over.
    Window* activeChild;

    // The currently active state, or states.end().
    STATE activeState;

    // True if we are currently animating.
    bool animating;

    // The easing we are using for the current animation.
    Easing::Type animationEasing;

    // How long, in seconds, the animation should last.
    float mAnimationDuration;

    // If we are currently doing an animation, the position at the start of the animation.
    RECT animationStart;

    // If we are currently doing an animation, how far along we are.
    StopWatch mAnimationClock;

    // If we are currently doing an animation, the position target of the animation.
    RECT animationTarget;

    // The children of this Window.
    list<Window*> children;

    // The area we draw in.
    D2D1_RECT_F drawingArea;

    // The drawing settings.
    WindowSettings* drawingSettings;

    // If Initalize has been called.
    bool initialized;

    // If the mouse is above the top-level window.
    bool isTrackingMouse;

    // The object which should handle mouse event messages.
    MessageHandler* msgHandler;

    // MonitorInfo...
    MonitorInfo* monitorInfo;

    // All current overlays.
    list<Overlay*> overlays;

    // The Window which is this windows parent.
    Window *mParent;

    // The current text of this window.
    IParsedText* parsedText;

    // Painters called after children and overlays are painted.
    list<IPainter*> postPainters;

    // Painters called before children and overlays are painted.
    list<IPainter*> prePainters;

    // Timer ID generator.
    UIDGenerator<UINT_PTR>* timerIDs;

    // Registered timer ID's.
    std::map<UINT_PTR, MessageHandler*> timers;

    // Used by the top-level window to track the mouse.
    TRACKMOUSEEVENT trackMouseStruct;

    // User msg ID generator.
    UIDGenerator<UINT>* userMsgIDs;

    // Registered user messages.
    std::map<UINT, MessageHandler*> userMessages;

    // Whether or not we are visible.
    bool visible;

    // The HWND we are rendering to.
    HWND window;

    // The text we are currently drawing.
    LPCWSTR text;

    //
    TCHAR mParentName[64];

    // True if this is a child window.
    bool mIsChild;

    // True if this window should handle all mouse events itself, rather than forwarding them to its children.
    bool mDontForwardMouse;

    // If we are capturing mouse input, the message handler which will receieve it.
    MessageHandler *mCaptureHandler;

    //
    bool mCoveredByFullscreen;

    //
    std::map<std::tstring, IBrushOwner*> mBrushOwners;

    // The base state -- the one to use when no others are active.
    STATE mBaseState;

    // Settings.
    Settings* mSettings;

    // All current states.
    std::list<State*> mStates;

    //
    int mUpdateLockCount;

public:
    // Registers a part of this window as a drop-region
    void AddDropRegion(LPRECT region, IDropTarget *handler);
    void RemoveDropRegion(LPRECT region, IDropTarget *handler);

private:
    struct DropRegion
    {
        RECT rect;
        IDropTarget *handler;
    };

    std::list<DropRegion> mDropRegions;
};