/* Kitserver 3 Setup */
/* Version 1.0 (with Win32 GUI) by Juce. */

#include <windows.h>
#include <windef.h>
#include <string.h>
#include <stdio.h>

#define DLL_PATH "kitserver\\kserv\0"
#define BUFLEN 4096

#include "imageutil.h"
#include "setupgui.h"
#include "setup.h"

HWND hWnd = NULL;
bool g_noFiles = false;

/**
 * Installs the kitserver DLL.
 */
void InstallKserv(void)
{
	// disable buttons, 'cause it may take some time
	EnableWindow(g_installButtonControl, FALSE);
	EnableWindow(g_removeButtonControl, FALSE);

	char fileName[BUFLEN];
	ZeroMemory(fileName, BUFLEN);
	lstrcpy(fileName, "..\\");
	char* p = fileName + lstrlen(fileName);

	// get currently selected item and its text
	int idx = (int)SendMessage(g_exeListControl, CB_GETCURSEL, 0, 0);
	SendMessage(g_exeListControl, CB_GETLBTEXT, idx, (LPARAM)p);

	// get the address of LoadLibrary function, which is different 
	// Here, we're taking advantage of the fact that Kernel32.dll, 
	// which contains the LoadLibrary function is never relocated, so 
	// the address of LoadLibrary will always be the same.
	HMODULE krnl = GetModuleHandle("kernel32.dll");
	DWORD loadLib = (DWORD)GetProcAddress(krnl, "LoadLibraryA");
	
	DWORD ep, ib, dVA, rdVA;
	DWORD loadLibAddr, kservAddr, loadLibAddr1;
	DWORD newEntryPoint;

	FILE* f = fopen(fileName, "r+b");
	if (f != NULL)
	{
		// Install
        loadLibAddr1 = getImportThunkRVA(f, "kernel32.dll","LoadLibraryA");

		if (SeekEntryPoint(f))
		{
			fread(&ep, sizeof(DWORD), 1, f);
			//printf("Entry point: %08x\n", ep);
		}
		if (SeekImageBase(f))
		{
			fread(&ib, sizeof(DWORD), 1, f);
			//printf("Image base: %08x\n", ib);
		}
		if (SeekSectionVA(f, ".data"))
		{
			fread(&dVA, sizeof(DWORD), 1, f);
			//printf(".data virtual address: %08x\n", dVA);

			// just before .data starts - in the end of .rdata,
			// write the LoadLibrary address, and the name of kserv.dll
			BYTE buf[0x20], zero[0x20];
			ZeroMemory(zero, 0x20);
			ZeroMemory(buf, 0x20);

			fseek(f, dVA - 0x20, SEEK_SET);
			fread(&buf, 0x20, 1, f);
			if (memcmp(buf, zero, 0x20)==0)
			{
				// ok, we found an empty place. Let's live here.
				fseek(f, -0x20, SEEK_CUR);
				DWORD* p = (DWORD*)buf;
				p[0] = ep; // save old empty pointer for easy uninstall
				p[1] = loadLib;
				memcpy(buf + 8, DLL_PATH, lstrlen(DLL_PATH)+1);
				fwrite(buf, 0x20, 1, f);

				loadLibAddr = ib + dVA - 0x20 + sizeof(DWORD);
				//printf("loadLibAddr = %08x\n", loadLibAddr);
				kservAddr = loadLibAddr + sizeof(DWORD);
				//printf("kservAddr = %08x\n", kservAddr);
                
                loadLibAddr = ib + loadLibAddr1;
			}
			else
			{
				//printf("Already installed.\n");
				fclose(f);

				// show message box with error msg
				char buf[BUFLEN];
				ZeroMemory(buf, BUFLEN);
				sprintf(buf, "\
======== INFORMATION! =========\n\
Kitserver 3 is already installed for\n\
%s.", fileName);

				MessageBox(hWnd, buf, "Kitserver 3 Setup Message", 0);
				return;
			}
		}
		if (SeekSectionVA(f, ".rdata"))
		{
			fread(&rdVA, sizeof(DWORD), 1, f);
			//printf(".rdata virtual address: %08x\n", rdVA);

			// just before .rdata starts, write the new entry point logic
			BYTE buf[0x20], zero[0x20];
			ZeroMemory(zero, 0x20);
			ZeroMemory(buf, 0x20);

			fseek(f, rdVA - 0x20, SEEK_SET);
			fread(&buf, 0x20, 1, f);
			if (memcmp(buf, zero, 0x20)==0)
			{
				// ok, we found an empty place. Let's live here.
				fseek(f, -0x20, SEEK_CUR);
				buf[0] = 0x68;  // push
				DWORD* p = (DWORD*)(buf + 1); p[0] = kservAddr;
				buf[5] = 0xff; buf[6] = 0x15; // call
				p = (DWORD*)(buf + 7); p[0] = loadLibAddr;
				buf[11] = 0xe9; // jmp
				p = (DWORD*)(buf + 12); p[0] = ib + ep - 5 - (ib + rdVA - 0x20 + 11);
				fwrite(buf, 0x20, 1, f);

				newEntryPoint = rdVA - 0x20;
			}
			else
			{
				//printf("Already installed.\n");
				fclose(f);

				// show message box with error msg
				char buf[BUFLEN];
				ZeroMemory(buf, BUFLEN);
				sprintf(buf, "\
======== INFORMATION! =========\n\
Kitserver 3 is already installed for\n\
%s.", fileName);

				MessageBox(hWnd, buf, "Kitserver 3 Setup Message", 0);
				return;
			}
		}
		if (SeekEntryPoint(f))
		{
			// write new entry point
			fwrite(&newEntryPoint, sizeof(DWORD), 1, f);
			//printf("New entry point: %08x\n", newEntryPoint);
		}
		if (SeekCodeSectionFlags(f))
		{
			DWORD flags;
			fread(&flags, sizeof(DWORD), 1, f);
			flags |= 0x80000000; // make code section writeable
			fseek(f, -sizeof(DWORD), SEEK_CUR);
			fwrite(&flags, sizeof(DWORD), 1, f);
		}
		fclose(f);

		SendMessage(g_exeInfoControl, WM_SETTEXT, (WPARAM)0, 
				(LPARAM)"KitServer INSTALLED");
		EnableWindow(g_installButtonControl, FALSE);
		EnableWindow(g_removeButtonControl, TRUE);

		// show message box with success msg
		char buf[BUFLEN];
		ZeroMemory(buf, BUFLEN);
		sprintf(buf, "\
======== SUCCESS! =========\n\
Setup has installed Kitserver 3 for\n\
%s.", fileName);

		MessageBox(hWnd, buf, "Kitserver 3 Setup Message", 0);
	}
	else
	{
		SendMessage(g_exeInfoControl, WM_SETTEXT, (WPARAM)0, 
				(LPARAM)"[ERROR: install failed]");
		EnableWindow(g_installButtonControl, TRUE);
		EnableWindow(g_removeButtonControl, FALSE);

		// show message box with error msg
		char buf[BUFLEN];
		ZeroMemory(buf, BUFLEN);
		sprintf(buf, "\
======== ERROR! =========\n\
Setup failed to install Kitserver 3 for\n\
%s.\n\
\n\
(No modifications made.)\n\
Verify that the executable is not\n\
READ-ONLY, and try again.", fileName);

		MessageBox(hWnd, buf, "Kitserver 3 Setup Message", 0);
	}
}

/**
 * Uninstalls the kitserver DLL.
 */
void RemoveKserv(void)
{
	// disable buttons, 'cause it may take some time
	EnableWindow(g_installButtonControl, FALSE);
	EnableWindow(g_removeButtonControl, FALSE);

	char fileName[BUFLEN];
	ZeroMemory(fileName, BUFLEN);
	lstrcpy(fileName, "..\\");
	char* p = fileName + lstrlen(fileName);

	// get currently selected item and its text
	int idx = (int)SendMessage(g_exeListControl, CB_GETCURSEL, 0, 0);
	SendMessage(g_exeListControl, CB_GETLBTEXT, idx, (LPARAM)p);

	DWORD ep, ib, dVA, rdVA;
	DWORD loadLibAddr, kservAddr;
	DWORD newEntryPoint;

	FILE* f = fopen(fileName, "r+b");
	if (f != NULL)
	{
		if (SeekEntryPoint(f))
		{
			fread(&ep, sizeof(DWORD), 1, f);
			//printf("Current entry point: %08x\n", ep);
		}
		if (SeekSectionVA(f, ".data"))
		{
			fread(&dVA, sizeof(DWORD), 1, f);
			//printf(".data virtual address: %08x\n", dVA);

			// just before .data starts - in the end of .rdata,
			BYTE zero[0x20];
			ZeroMemory(zero, 0x20);
			fseek(f, dVA - 0x20, SEEK_SET);

			// read saved old entry point
			fread(&newEntryPoint, sizeof(DWORD), 1, f);
			if (newEntryPoint == 0)
			{
				//printf("Already uninstalled.\n");
				fclose(f);

				// show message box with error msg
				char buf[BUFLEN];
				ZeroMemory(buf, BUFLEN);
				sprintf(buf, "\
======== INFORMATION! =========\n\
Kitserver 3 is not installed for\n\
%s.", fileName);

				MessageBox(hWnd, buf, "Kitserver 3 Setup Message", 0);
				return;
			}
			// zero out the bytes
			fseek(f, -sizeof(DWORD), SEEK_CUR);
			fwrite(zero, 0x20, 1, f);
		}
		if (SeekSectionVA(f, ".rdata"))
		{
			fread(&rdVA, sizeof(DWORD), 1, f);
			//printf(".rdata virtual address: %08x\n", rdVA);

			// just before .rdata starts, write the new entry point logic
			// zero out the bytes
			BYTE zero[0x20];
			ZeroMemory(zero, 0x20);
			fseek(f, rdVA - 0x20, SEEK_SET);
			fwrite(&zero, 0x20, 1, f);
		}
		if (SeekEntryPoint(f))
		{
			// write new entry point
			fwrite(&newEntryPoint, sizeof(DWORD), 1, f);
			//printf("New entry point: %08x\n", newEntryPoint);
		}
		if (SeekCodeSectionFlags(f))
		{
			DWORD flags;
			fread(&flags, sizeof(DWORD), 1, f);
			flags &= 0x7fffffff; // make code section non-writeable again
			fseek(f, -sizeof(DWORD), SEEK_CUR);
			fwrite(&flags, sizeof(DWORD), 1, f);
		}
		fclose(f);

		SendMessage(g_exeInfoControl, WM_SETTEXT, (WPARAM)0, 
				(LPARAM)"KitServer not installed");
		EnableWindow(g_installButtonControl, TRUE);
		EnableWindow(g_removeButtonControl, FALSE);

		// show message box with error msg
		char buf[BUFLEN];
		ZeroMemory(buf, BUFLEN);
		sprintf(buf, "\
======== SUCCESS! =========\n\
Setup has removed Kitserver 3 from\n\
%s.", fileName);

		MessageBox(hWnd, buf, "Kitserver 3 Setup Message", 0);
	}
	else
	{
		SendMessage(g_exeInfoControl, WM_SETTEXT, (WPARAM)0, 
				(LPARAM)"[ERROR: remove failed]");
		EnableWindow(g_installButtonControl, FALSE);
		EnableWindow(g_removeButtonControl, TRUE);

		// show message box with error msg
		char buf[BUFLEN];
		ZeroMemory(buf, BUFLEN);
		sprintf(buf, "\
======== ERROR! =========\n\
Setup failed to remove Kitserver 3 from\n\
%s.\n\
\n\
(No modifications made.)\n\
Verify that the executable is not\n\
READ-ONLY, and try again.", fileName);

		MessageBox(hWnd, buf, "Kitserver 3 Setup Message", 0);
	}
}

/**
 * Updates the information about exe file.
 */
void UpdateInfo(void)
{
	if (g_noFiles) return;

	EnableWindow(g_installButtonControl, FALSE);
	EnableWindow(g_removeButtonControl, FALSE);

	char fileName[BUFLEN];
	ZeroMemory(fileName, BUFLEN);
	lstrcpy(fileName, "..\\");
	char* p = fileName + lstrlen(fileName);

	// get currently selected item and its text
	int idx = (int)SendMessage(g_exeListControl, CB_GETCURSEL, 0, 0);
	SendMessage(g_exeListControl, CB_GETLBTEXT, idx, (LPARAM)p);

	FILE* f = fopen(fileName, "rb");
	if (f != NULL)
	{
		DWORD dVA = 0;
		if (SeekSectionVA(f, ".data"))
		{
			fread(&dVA, sizeof(DWORD), 1, f);

			// just before .data starts - in the end of .rdata,
			BYTE zero[0x20];
			ZeroMemory(zero, 0x20);
			fseek(f, dVA - 0x20, SEEK_SET);

			// read saved old entry point
			DWORD savedEntryPoint = 0;
			fread(&savedEntryPoint, sizeof(DWORD), 1, f);
			if (savedEntryPoint == 0)
			{
				SendMessage(g_exeInfoControl, WM_SETTEXT, (WPARAM)0, 
						(LPARAM)"KitServer not installed");
				EnableWindow(g_installButtonControl, TRUE);
				EnableWindow(g_removeButtonControl, FALSE);
			}
			else
			{
				SendMessage(g_exeInfoControl, WM_SETTEXT, (WPARAM)0, 
						(LPARAM)"KitServer INSTALLED");
				EnableWindow(g_installButtonControl, FALSE);
				EnableWindow(g_removeButtonControl, TRUE);
			}
		}
		else
		{
			SendMessage(g_exeInfoControl, WM_SETTEXT, (WPARAM)0,
					(LPARAM)"Information unavailable");
		}
		fclose(f);
	}
	else
	{
		SendMessage(g_exeInfoControl, WM_SETTEXT, (WPARAM)0, 
				(LPARAM)"[ERROR: Can't open file.]");
	}
}

/**
 * Initializes all controls
 */
void InitControls(void)
{
	// Build the drop-down list
	WIN32_FIND_DATA fData;
	char pattern[4096];
	ZeroMemory(pattern, 4096);

	lstrcpy(pattern, "..\\*.exe");

	HANDLE hff = FindFirstFile(pattern, &fData);
	if (hff == INVALID_HANDLE_VALUE) 
	{
		// none found.
		g_noFiles = true;
		return;
	}
	while(true)
	{
		if (lstrcmpi(fData.cFileName, "setting.exe") != 0) // skip setting.exe
		{
			SendMessage(g_exeListControl, CB_ADDSTRING, (WPARAM)0, (LPARAM)fData.cFileName);
			SendMessage(g_exeListControl, WM_SETTEXT, (WPARAM)0, (LPARAM)fData.cFileName);
		}

		// proceed to next file
		if (!FindNextFile(hff, &fData)) break;
	}
	FindClose(hff);

	SendMessage(g_exeListControl, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
	EnableWindow(g_exeListControl, TRUE);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int home, away, timecode;
	char buf[BUFLEN];

	switch(uMsg)
	{
		case WM_DESTROY:
			// Exit the application when the window closes
			PostQuitMessage(1);
			return true;

		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				if ((HWND)lParam == g_installButtonControl)
				{
					InstallKserv();
				}
				else if ((HWND)lParam == g_removeButtonControl)
				{
					RemoveKserv();
				}
			}
			else if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				if ((HWND)lParam == g_exeListControl)
				{
					UpdateInfo();
				}
			}
			break;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}

bool InitApp(HINSTANCE hInstance, LPSTR lpCmdLine)
{
	WNDCLASSEX wcx;

	// cbSize - the size of the structure.
	wcx.cbSize = sizeof(WNDCLASSEX);
	wcx.style = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc = (WNDPROC)WindowProc;
	wcx.cbClsExtra = 0;
	wcx.cbWndExtra = 0;
	wcx.hInstance = hInstance;
	wcx.hIcon = LoadIcon(NULL,IDI_APPLICATION);
	wcx.hCursor = LoadCursor(NULL,IDC_ARROW);
	wcx.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wcx.lpszMenuName = NULL;
	wcx.lpszClassName = "SETUPCLS";
	wcx.hIconSm = NULL;

	// Register the class with Windows
	if(!RegisterClassEx(&wcx))
		return false;

	return true;
}

HWND BuildWindow(int nCmdShow)
{
	DWORD style, xstyle;
	HWND retval;

	style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	xstyle = WS_EX_LEFT;

	retval = CreateWindowEx(xstyle,
        "SETUPCLS",      // class name
        SETUP_WINDOW_TITLE, // title for our window (appears in the titlebar)
        style,
        CW_USEDEFAULT,  // initial x coordinate
        CW_USEDEFAULT,  // initial y coordinate
        WIN_WIDTH, WIN_HEIGHT,   // width and height of the window
        NULL,           // no parent window.
        NULL,           // no menu
        NULL,           // no creator
        NULL);          // no extra data

	if (retval == NULL) return NULL;  // BAD.

	ShowWindow(retval,nCmdShow);  // Show the window
	return retval; // return its handle for future use.
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	MSG msg; int retval;

 	if(InitApp(hInstance, lpCmdLine) == false)
		return 0;

	hWnd = BuildWindow(nCmdShow);
	if(hWnd == NULL)
		return 0;

	// build GUI
	if (!BuildControls(hWnd))
		return 0;

	// Initialize all controls
	InitControls();
	UpdateInfo();

	// show credits
	char buf[BUFLEN];
	ZeroMemory(buf, BUFLEN);
	strncpy(buf, CREDITS, BUFLEN-1);
	SendMessage(g_statusTextControl, WM_SETTEXT, 0, (LPARAM)buf);

	while((retval = GetMessage(&msg,NULL,0,0)) != 0)
	{
		if(retval == -1)
			return 0;	// an error occured while getting a message

		if (!IsDialogMessage(hWnd, &msg)) // need to call this to make WS_TABSTOP work
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}

