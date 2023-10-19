#undef UNICODE // Switch the project to ASCII mode
#include <windows.h>
#include <stdio.h>
#include "Resource.h"
#include <commctrl.h> // Include CommCtrl.h for DateTime_GetSystemtime
#include <processthreadsapi.h>


#pragma warning(disable : 4996)

bool Terminate = false; // Thread termination request flag
HWND hMainWnd = 0; // Main window handle to be retained
HANDLE hClock = 0; // Clock thread handle
bool ClockPaused = false; // Flag to indicate if the clock is paused

// Message handler prototype
BOOL CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);

// Main function
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    DialogBox(
        hInstance,
        MAKEINTRESOURCE(IDD_MAINDIALOG),
        NULL,
        DLGPROC(MainWndProc)
    );
    return 0;
}

DWORD WINAPI ClockThread(LPVOID lpParameter) {
    while (!Terminate) {
        char timestr[9];
        SYSTEMTIME time;
        GetLocalTime(&time);
        sprintf(timestr, "%.2d:%.2d:%.2d",
            time.wHour, time.wMinute, time.wSecond);
        SetDlgItemText(hMainWnd, IDC_CLOCK, timestr);
        Sleep(250);
    }
    return 0;
}


unsigned __int64 SystemTimeToInt(SYSTEMTIME Time) {
    FILETIME ft;
    SystemTimeToFileTime(&Time, &ft);
    ULARGE_INTEGER fti;
    fti.HighPart = ft.dwHighDateTime;
    fti.LowPart = ft.dwLowDateTime;
    return fti.QuadPart;
}
BOOL CALLBACK MainWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch (Msg) { // pārbaudīt ziņojuma identifikatoru
    case WM_INITDIALOG: // ja tas ir inicializācijas paziņojums
        // izpildīt inicializācijas darbības
        hMainWnd = hWnd;
        hClock = CreateThread(NULL, 0, ClockThread, NULL, 0, NULL);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            DestroyWindow(hWnd);
            return TRUE;
        case IDC_PAUSE:
            if (ClockPaused) {
                ResumeThread(hClock);
                ClockPaused = false;
            }
            else {
                SuspendThread(hClock);
                ClockPaused = true;
            }
            return TRUE;
        }
        return FALSE;

    case WM_DESTROY: // ja tas ir pabeigšanas pieprasījums
        Terminate = true;
        Sleep(500);
        CloseHandle(hClock);

        PostQuitMessage(0); // pabeigt programmu
        return TRUE;
    }
    return FALSE;
}
