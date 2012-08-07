/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *	nDesk.h															July, 2012
 *	The nModules Project
 *
 *	Function declarations for nDesk
 *      
 *													             Erik Welander
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#ifndef NDESK_H
#define NDESK_H

#include <strsafe.h>
#include "../headers/lsapi.h"
#include "../nShared/Export.h"

void CreateMainWindow(HINSTANCE hInst);
LRESULT WINAPI MainProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

EXPORT_CDECL(int) initModuleEx(HWND, HINSTANCE, LPCSTR);
EXPORT_CDECL(void) quitModule(HINSTANCE);

#endif /* NDESK_H */