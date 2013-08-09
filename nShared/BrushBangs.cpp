//--------------------------------------------------------------------------------------
// BrushBangs.cpp
// The nModules Project
//
// Bangs for brushes
//
//--------------------------------------------------------------------------------------
#include "BrushBangs.h"
#include "LiteStep.h"
#include "../nCoreCom/Core.h"

// Used to map window name -> window
static std::function<DrawableWindow* (LPCTSTR)> windowFinder;


/// <summary>
/// Retrieves the brush and owning window
/// </summary>
static Brush *FindBrush(LPCTSTR *args, int numArgs, DrawableWindow *&window)
{
    TCHAR buffer[MAX_RCCOMMAND];
    int numTokens = LiteStep::CommandTokenize(*args, nullptr, 0, nullptr);

    // Order of precedence
    // window
    // window brushowner
    // window brushowner brush
    // window brush
    if (numTokens == numArgs + 1 || numTokens == numArgs + 2 || numTokens == numArgs + 3)
    {
        LiteStep::GetToken(*args, buffer, args, 0);
        window = windowFinder(buffer);
        if (window)
        {
            if (numTokens == numArgs + 2 || numTokens == numArgs + 3)
            {
                LiteStep::GetToken(*args, buffer, args, 0);
                IBrushOwner *owner = window->GetBrushOwner(buffer);
                if (owner)
                {
                    if (numTokens == numArgs + 3) // window brushowner brush
                    { 
                        LiteStep::GetToken(*args, buffer, args, 0);
                        return owner->GetBrush(buffer);
                    }
                    else // window brushowner
                    { 
                        return owner->GetBrush(_T(""));
                    }
                }
                else if (numTokens == numArgs + 2) // Window Brush
                { 
                    owner = window->GetBrushOwner(_T(""));
                    if (owner)
                    {
                        return owner->GetBrush(buffer);
                    }
                }
            }
            else // Window
            { 
                IBrushOwner *owner = window->GetBrushOwner(_T(""));
                if (owner)
                {
                    return owner->GetBrush(_T(""));
                }
            }
        }
    }

    return nullptr;
}


static struct BangItem
{
    BangItem(LPCTSTR name, LiteStep::BANGCOMMANDPROC proc)
    {
        this->name = name;
        this->proc = proc;
    }

    LPCTSTR name;
    LiteStep::BANGCOMMANDPROC proc;
} BangMap [] =
{
    /*(BangItem(TEXT("SetBrushType"),            [] (HWND, LPCTSTR args) -> void
    {
        DrawableWindow *window = nullptr;
        Brush *brush = FindBrush(&args, 1, window);
        if (brush)
        {
            char arg[MAX_RCCOMMAND];
            LiteStep::GetToken(args, arg, nullptr, 0);
            brush->SetType(BrushSetings::ParseType(arg));
            window->Repaint();
        }
    }),*/
    BangItem(TEXT("SetColor"),                [] (HWND, LPCTSTR args) -> void
    {
        DrawableWindow *window = nullptr;
        Brush *brush = FindBrush(&args, 1, window);
        if (brush)
        {
            TCHAR arg[MAX_RCCOMMAND];
            LiteStep::GetToken(args, arg, nullptr, 0);
            IColorVal* color;
            if (ParseColor(arg, &color))
            {
                brush->SetColor(color);
                delete color;
                window->Repaint();
            }
        }
    }),
    /*BangItem(TEXT("SetAlpha"),                [] (HWND, LPCTSTR args) -> void {
        DrawableWindow *window = nullptr;
        Brush *brush = FindBrush(&args, 1, window);
        if (brush) {
            char arg[MAX_RCCOMMAND];
            LiteStep::GetToken(args, arg, nullptr, 0);
            window->Repaint();
        }
    }), 
    BangItem(TEXT("SetGradientColors"),       [] (HWND, LPCTSTR args) -> void {
        DrawableWindow *window = nullptr;
        Brush *brush = FindBrush(&args, 1, window);
        if (brush) {
            char arg[MAX_RCCOMMAND];
            LiteStep::GetToken(args, arg, nullptr, 0);
            window->Repaint();
        }
    }),
    BangItem(TEXT("SetGradientStops"),        [] (HWND, LPCTSTR args) -> void {
        DrawableWindow *window = nullptr;
        Brush *brush = FindBrush(&args, 1, window);
        if (brush) {
            char arg[MAX_RCCOMMAND];
            LiteStep::GetToken(args, arg, nullptr, 0);
            window->Repaint();
        }
    }),
    BangItem(TEXT("SetGradientStart"),        [] (HWND, LPCTSTR args) -> void {
        DrawableWindow *window = nullptr;
        Brush *brush = FindBrush(&args, 1, window);
        if (brush) {
            char arg[MAX_RCCOMMAND];
            LiteStep::GetToken(args, arg, nullptr, 0);
            window->Repaint();
        }
    }),
    BangItem(TEXT("SetGradientEnd"),          [] (HWND, LPCTSTR args) -> void {
        DrawableWindow *window = nullptr;
        Brush *brush = FindBrush(&args, 1, window);
        if (brush) {
            char arg[MAX_RCCOMMAND];
            LiteStep::GetToken(args, arg, nullptr, 0);
            window->Repaint();
        }
    }),
    BangItem(TEXT("SetGradientCenter"),       [] (HWND, LPCTSTR args) -> void {
        DrawableWindow *window = nullptr;
        Brush *brush = FindBrush(&args, 1, window);
        if (brush) {
            char arg[MAX_RCCOMMAND];
            LiteStep::GetToken(args, arg, nullptr, 0);
            window->Repaint();
        }
    }),
    BangItem(TEXT("SetGradientRadius"),       [] (HWND, LPCTSTR args) -> void {
        DrawableWindow *window = nullptr;
        Brush *brush = FindBrush(&args, 1, window);
        if (brush) {
            char arg[MAX_RCCOMMAND];
            LiteStep::GetToken(args, arg, nullptr, 0);
            window->Repaint();
        }
    }),
    BangItem(TEXT("SetGradientOriginOffset"), [] (HWND, LPCTSTR args) -> void {
        DrawableWindow *window = nullptr;
        Brush *brush = FindBrush(&args, 1, window);
        if (brush) {
            char arg[MAX_RCCOMMAND];
            LiteStep::GetToken(args, arg, nullptr, 0);
            window->Repaint();
        }
    }),*/
    BangItem(TEXT("SetImage"),                [] (HWND, LPCTSTR args) -> void
    {
        DrawableWindow *window = nullptr;
        Brush *brush = FindBrush(&args, 1, window);
        if (brush)
        {
            TCHAR arg[MAX_RCCOMMAND];
            LiteStep::GetToken(args, arg, nullptr, 0);
            brush->SetImage(window->GetRenderTarget(), arg);
            window->Repaint();
        }
    }),
    /*BangItem(TEXT("SetImageRotation"),        [] (HWND, LPCTSTR args) -> void {
        DrawableWindow *window = nullptr;
        Brush *brush = FindBrush(&args, 1, window);
        if (brush) {
            char arg[MAX_RCCOMMAND];
            LiteStep::GetToken(args, arg, nullptr, 0);
            brush->SetImageRotation(strtof(arg, nullptr));
            window->Repaint();
        }
    }),*/
    /*BangItem(TEXT("SetImageScalingMode"),     [] (HWND, LPCTSTR args) -> void {
        DrawableWindow *window = nullptr;
        Brush *brush = FindBrush(&args, 1, window);
        if (brush) {
            char arg[MAX_RCCOMMAND];
            LiteStep::GetToken(args, arg, nullptr, 0);
            window->Repaint();
        }
    }),
    BangItem(TEXT("SetTilingMode"),           [] (HWND, LPCTSTR args) -> void {
        DrawableWindow *window = nullptr;
        Brush *brush = FindBrush(&args, 1, window);
        if (brush) {
            char arg[MAX_RCCOMMAND];
            LiteStep::GetToken(args, arg, nullptr, 0);
            window->Repaint();
        }
    })*/
};


/// <summary>
/// Registers bangs.
/// </summary>
void BrushBangs::Register(LPCTSTR prefix, std::function<DrawableWindow* (LPCTSTR)> windowFinder)
{
    TCHAR bangName[64];
    ::windowFinder = windowFinder;
    for (BangItem item : BangMap)
    {
        StringCchPrintf(bangName, _countof(bangName), TEXT("!%s%s"), prefix, item.name);
        LiteStep::AddBangCommand(bangName, item.proc);
    }
}


/// <summary>
/// Unregisters bangs.
/// </summary>
void BrushBangs::UnRegister(LPCTSTR prefix)
{
    TCHAR bangName[64];
    for (BangItem item : BangMap)
    {
        StringCchPrintf(bangName, _countof(bangName), TEXT("!%s%s"), prefix, item.name);
        LiteStep::RemoveBangCommand(bangName);
    }
    ::windowFinder = nullptr;
}