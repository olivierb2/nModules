/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Tray.cpp                                                        July, 2012
 *  The nModules Project
 *
 *  Implementation of the Tray class. Handles layout of the tray buttons.
 *  
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include <strsafe.h>
#include "../headers/lsapi.h"
#include "../nCoreCom/Core.h"
#include "Tray.hpp"
#include "TrayIcon.hpp"

extern HINSTANCE g_hInstance;
extern LPCSTR g_szTrayHandler;
extern HWND g_hWndTrayNotify;


/// <summary>
/// Constructor
/// </summary>
Tray::Tray(LPCSTR pszName) {
    m_pszName = pszName;

    m_pPaintSettings = new PaintSettings(m_pszName);
    m_pWindow = new DrawableWindow(NULL, g_szTrayHandler, m_pPaintSettings, g_hInstance);
    SetWindowLongPtr(m_pWindow->getWindow(), 0, (LONG_PTR)this);
    m_pWindow->Show();

    LoadSettings();
}


/// <summary>
/// Destructor
/// </summary>
Tray::~Tray() {
    // Remove all icons
    for (vector<TrayIcon*>::const_iterator iter = m_icons.begin(); iter != m_icons.end(); iter++) {
        delete *iter;
    }
    m_icons.clear();

    if (m_pWindow) delete m_pWindow;
    if (m_pPaintSettings) delete m_pPaintSettings;
    free((void *)m_pszName);
}


/// <summary>
/// Loads settings from LiteStep's RC files.
/// </summary>
void Tray::LoadSettings(bool /* bIsRefresh */) {
    Settings* pSettings = m_pPaintSettings->GetSettings();
    pSettings->GetOffsetRect("MarginLeft", "MarginTop", "MarginRight", "MarginBottom", &m_rMargin, 2, 2, 5, 2);
    m_iColSpacing = pSettings->GetInt("ColumnSpacing", 2);
    m_iRowSpacing = pSettings->GetInt("RowSpacing", 2);
}


/// <summary>
/// Adds the specified icon to this tray.
/// </summary>
TrayIcon* Tray::AddIcon(LPLSNOTIFYICONDATA pNID) {
    TrayIcon* tIcon = new TrayIcon(m_pWindow->getWindow(), m_pszName, pNID);
    m_icons.push_back(tIcon);
    Relayout();
    tIcon->Show();
    return tIcon;
}


/// <summary>
/// Finds the specified icon.
/// </summary>
vector<TrayIcon*>::const_iterator Tray::FindIcon(TrayIcon* pIcon) {
    for (vector<TrayIcon*>::const_iterator iter = m_icons.begin(); iter != m_icons.end(); iter++) {
        if ((*iter) == pIcon) {
            return iter;
        }
    }
    return m_icons.end();
}


/// <summary>
/// Removes the specified icon from this tray, if it is in it.
/// </summary>
void Tray::RemoveIcon(TrayIcon* pIcon) {
    vector<TrayIcon*>::const_iterator icon = FindIcon(pIcon);
    if (icon != m_icons.end()) {
        delete *icon;
        m_icons.erase(icon);
        Relayout();
    }
}


/// <summary>
/// Repositions/Resizes all icons.
/// </summary>
void Tray::Relayout() {
    int x = m_rMargin.left;
    int y = m_rMargin.top;
    int wrapwidth = m_pPaintSettings->position.right - m_pPaintSettings->position.left - m_rMargin.right - 20;

    for (vector<TrayIcon*>::const_iterator iter = m_icons.begin(); iter != m_icons.end(); iter++) {
        (*iter)->Reposition(x, y, 20, 20);
        x += 20 + m_iColSpacing;
        if (x > wrapwidth) {
            x = m_rMargin.left;
            y += 20 + m_iRowSpacing;
        }
    }
}


/// <summary>
/// Handles window events for the tray.
/// </summary>
LRESULT WINAPI Tray::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_MOUSEMOVE:
        if (IsWindow(g_hWndTrayNotify)) {
            MoveWindow(g_hWndTrayNotify, m_pPaintSettings->position.left, m_pPaintSettings->position.right,
                m_pPaintSettings->position.left - m_pPaintSettings->position.right,
                m_pPaintSettings->position.bottom - m_pPaintSettings->position.top, FALSE);
        }
    default:
        return m_pWindow->HandleMessage(uMsg, wParam, lParam);
    }
}
