/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *	nIcon.h															July, 2012
 *	The nModules Project
 *
 *	Function declarations for nIcon
 *      
 *													             Erik Welander
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#ifndef NDESK_H
#define NDESK_H

#include "../nShared/Export.h"

void CreateLSMsgHandler(HINSTANCE hInst);
LRESULT WINAPI LSMsgHandlerProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

EXPORT_CDECL(int) initModuleEx(HWND, HINSTANCE, LPCSTR);
EXPORT_CDECL(void) quitModule(HINSTANCE);

#endif /* NDESK_H */
