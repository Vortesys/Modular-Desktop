/* * * * * * * *\
	MODESK.CPP -
		Copyright (c) 2024 Vortesys
	DESCRIPTION -
		Modular Desktop's primary file. Contains the
		main function.
	LICENSE INFORMATION -
		MIT License, see LICENSE.txt in the root folder
\* * * * * * * */

/* Headers */
#include "progmgr.h"
// #define WIN32_LEAN_AND_MEAN
#define SECURITY_WIN32
#include <Windows.h>

/* Variables */
// Global
#ifdef _DEBUG
BOOL		g_bIsDebugBuild = TRUE;
#else
BOOL		g_bIsDebugBuild = FALSE;
#endif
BOOL		g_bIsDefaultShell = FALSE;
// Handles
HINSTANCE	g_hAppInstance;
HANDLE		g_hAppHeap;
HWND		g_hWndMain = NULL;
// Icons
HICON		g_hProgMgrIcon = NULL;
HICON		g_hGroupIcon = NULL;
// Global Strings
WCHAR		g_szAppTitle[64];
WCHAR		g_szProgMgr[] = TEXT("progmgr");
WCHAR		g_szWebsite[64];
WCHAR		g_szClass[16];
// Permissions
BOOL		g_bPermAdmin; // Has Administrator permissions
BOOL		g_bPermGuest; // Has Guest permissions
BOOL		g_bPermPower; // Has power option permissions

/* Functions */

/* * * *\
	wWinMain -
		Program Manager's entry point.
\* * * */
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	MSG msg = { 0 };
	HANDLE hAccel;
	HMENU hMenu;
	HMENU hSystemMenu;
	WNDCLASS wc = { 0 };
	WCHAR szBuffer[MAX_PATH];
	RECT rcRoot;
	POINT ptOffset = { 0 };

	// Initialize the instance
	hAppInstance = hInstance;

	// Create our global heap handle
	hAppHeap = GetProcessHeap();

	// Create Strings
	LoadString(hAppInstance, IDS_PMCLASS, szClass, ARRAYSIZE(szClass));
	LoadString(hAppInstance, IDS_APPTITLE, szAppTitle, ARRAYSIZE(szAppTitle));
	LoadString(hAppInstance, IDS_WEBSITE, szWebsite, ARRAYSIZE(szWebsite));

	// Register the Frame Window Class
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hAppInstance;
	wc.hIcon = hProgMgrIcon = LoadImage(hAppInstance, MAKEINTRESOURCE(IDI_PROGMGR), IMAGE_ICON,
		0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	// wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = MAKEINTRESOURCE(IDM_MAIN);
	wc.lpszClassName = szClass;

	if (!RegisterClass(&wc))
		return FALSE;

	// Load the Accelerator table
	hAccel = LoadAccelerators(hAppInstance, MAKEINTRESOURCE(IDA_ACCELS));
	if (!hAccel)
		return FALSE;

	// Perform Registry actions, close if registry is inaccessible.
	if (!InitializeRegistryKeys())
		return FALSE;
	
	bIsDefaultShell = IsProgMgrDefaultShell();

	// Load configuration, but don't load groups yet
	if(LoadConfig(TRUE, TRUE, FALSE) != RCE_SUCCESS)
		return FALSE;

	// Get size of the root HWND
	GetWindowRect(GetDesktopWindow(), &rcRoot);

	// Get the initial window offset
	SystemParametersInfo(SPI_ICONHORIZONTALSPACING, 0, &ptOffset.x, 0);
	SystemParametersInfo(SPI_ICONVERTICALSPACING, 0, &ptOffset.y, 0);

	// Create main window with a default size
	// TODO: i pulled 320x240 out of my ass, make this dynamic later
	if ((hWndProgMgr = CreateWindowW(wc.lpszClassName, szAppTitle,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		rcRoot.left + ptOffset.x, rcRoot.top + ptOffset.y,
		rcRoot.left + ptOffset.x + 320, rcRoot.top + ptOffset.y + 240,
		0, 0, hAppInstance, NULL)) == NULL)
		return 2;

	// Set the window size from the registry, but only if the coords make sense
	if ((rcMainWindow.left != rcMainWindow.right) && (rcMainWindow.top != rcMainWindow.bottom))
		SetWindowPos(hWndProgMgr, NULL,
			rcMainWindow.left, rcMainWindow.top,
			rcMainWindow.right - rcMainWindow.left,
			rcMainWindow.bottom - rcMainWindow.top, SWP_NOZORDER);

	// Load the menus...
	hMenu = GetMenu(hWndProgMgr);
	hSystemMenu = GetSystemMenu(hWndProgMgr, FALSE);

	// Update relevant parts of the window
	UpdateChecks(bAutoArrange, IDM_OPTIONS, IDM_OPTIONS_AUTOARRANGE);
	UpdateChecks(bMinOnRun, IDM_OPTIONS, IDM_OPTIONS_MINONRUN);
	UpdateChecks(bTopMost, IDM_OPTIONS, IDM_OPTIONS_TOPMOST);
	UpdateChecks(bShowUsername, IDM_OPTIONS, IDM_OPTIONS_SHOWUSERNAME);
	UpdateChecks(bSaveSettings, IDM_OPTIONS, IDM_OPTIONS_SAVESETTINGS);

	UpdateWindowTitle();

	// Update settings based on their values
	if (bTopMost)
		SetWindowPos(hWndProgMgr, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	if (bIsDefaultShell)
	{
		// Modify the context menus since we're the default shell
		DeleteMenu(hSystemMenu, SC_CLOSE, MF_BYCOMMAND);

		LoadString(hAppInstance, IDS_SHUTDOWN, szBuffer, ARRAYSIZE(szBuffer));
		InsertMenu(hSystemMenu, 6, MF_BYPOSITION | MF_STRING, IDM_SHUTDOWN, szBuffer);
		ModifyMenu(hMenu, IDM_FILE_EXIT, MF_BYCOMMAND | MF_STRING, IDM_SHUTDOWN, szBuffer);

		LoadString(hAppInstance, IDS_RUN, szBuffer, ARRAYSIZE(szBuffer));
		InsertMenu(hSystemMenu, 6, MF_BYPOSITION | MF_STRING, IDM_FILE_RUN, szBuffer);

		LoadString(hAppInstance, IDS_TASKMGR, szBuffer, ARRAYSIZE(szBuffer));
		InsertMenu(hSystemMenu, 6, MF_BYPOSITION | MF_STRING, IDM_TASKMGR, szBuffer);
		
		// Create the desktop window...
		// CreateDesktopWindow();
	}

	// Create the frame window
	if (!InitializeGroups())
		return FALSE;

	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		if (!TranslateMDISysAccel(hWndMDIClient, &msg) &&
			!TranslateAccelerator(hWndProgMgr, hAccel, &msg))
		{
			DispatchMessage(&msg);
		}
	}

	return 0;
}
