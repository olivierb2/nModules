/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *	TrayManager.h													July, 2012
 *	The nModules Project
 *
 *	
 *      
 *													             Erik Welander
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#ifndef TRAYMANAGER_H
#define TRAYMANAGER_H

#include "TrayIcon.hpp"
#include <vector>

using std::vector;

namespace TrayManager {
	typedef struct {
		vector<TrayIcon*>* icons;
		LSNOTIFYICONDATA NID;
	} TRAYICONDATA, *LPTRAYICONDATA;

	typedef vector<LPTRAYICONDATA>::iterator TRAYICONDATAITER;
	typedef map<LPCSTR, Tray*>::const_iterator TRAYSCITER;

	TRAYICONDATAITER FindIcon(LPLSNOTIFYICONDATA pNID);
	void Stop();
	void AddIcon(LPLSNOTIFYICONDATA pNID);
	void DeleteIcon(LPLSNOTIFYICONDATA pNID);
	void ModifyIcon(LPLSNOTIFYICONDATA pNID);
	void SetFocus(LPLSNOTIFYICONDATA pNID);
	void SetVersion(LPLSNOTIFYICONDATA pNID);

	LRESULT ShellMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
}

#endif /* TRAYMANAGER_H */