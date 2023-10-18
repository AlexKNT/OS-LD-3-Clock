#undef UNICODE // Switch the project to ASCII mode
#include <windows.h>
#include <stdio.h>
#include "Resource.h"
#include <commctrl.h> // Include CommCtrl.h for DateTime_GetSystemtime

#pragma warning(disable : 4996)

bool Terminate = false; // pavedienu pabeigšanas pieprasījuma karodziņš
HWND hMainWnd = 0; // galvenā loga turis, kas jāsaglabā
HANDLE hClock = 0; // pulksteņa pavediena turis
bool ClockPaused = false; // vai pulkstenis ir apstādināts
SYSTEMTIME StartTime = {}; // To store the start time

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

unsigned __int64 SystemTimeToInt(SYSTEMTIME Time) {
    FILETIME ft;
    SystemTimeToFileTime(&Time, &ft);
    ULARGE_INTEGER fti;
    fti.HighPart = ft.dwHighDateTime;
    fti.LowPart = ft.dwLowDateTime;
    return fti.QuadPart;
}

DWORD WINAPI ClockThread(LPVOID lpParameter) {
    while (!Terminate) {
        char timestr[9];
        SYSTEMTIME time;
        GetLocalTime(&time);

        if (!ClockPaused) {
            // Calculate the elapsed time from the start time
            FILETIME startFileTime, currentTime;
            SystemTimeToFileTime(&StartTime, &startFileTime);
            SystemTimeToFileTime(&time, &currentTime);

            ULARGE_INTEGER startTimeInt, currentTimeInt;
            startTimeInt.LowPart = startFileTime.dwLowDateTime;
            startTimeInt.HighPart = startFileTime.dwHighDateTime;
            currentTimeInt.LowPart = currentTime.dwLowDateTime;
            currentTimeInt.HighPart = currentTime.dwHighDateTime;

            ULARGE_INTEGER elapsedTimeInt;
            elapsedTimeInt.QuadPart = currentTimeInt.QuadPart - startTimeInt.QuadPart;

            SYSTEMTIME elapsedTime;
            FileTimeToSystemTime((FILETIME*)&elapsedTimeInt, &elapsedTime);

            // Format and display the elapsed time
            sprintf_s(timestr, sizeof(timestr), "%.2d:%.2d:%.2d",
                elapsedTime.wHour, elapsedTime.wMinute, elapsedTime.wSecond);
        }
        else {
            // Format and display the current time
            sprintf_s(timestr, sizeof(timestr), "%.2d:%.2d:%.2d",
                time.wHour, time.wMinute, time.wSecond);
        }

        SetDlgItemText(hMainWnd, IDC_CLOCK, timestr);
        Sleep(250);
    }
    return 0;
}

// Message handler
BOOL CALLBACK MainWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch (Msg) { // Check the message identifier
    case WM_INITDIALOG: // If it's an initialization message
        SetTimer(hWnd, 1, 250, NULL);
        hMainWnd = hWnd;
        hClock = CreateThread(NULL, 0, ClockThread, NULL, 0, NULL);

        // Get the initial start time from the IDC_TIME field
        SendMessage(GetDlgItem(hWnd, IDC_TIME), DTM_GETSYSTEMTIME, 0, (LPARAM)&StartTime);

        return TRUE;

    case WM_COMMAND: // If it's a command message from a control
        switch (LOWORD(wParam)) { // Check the control ID
        case IDOK: // If the OK button was pressed
            DestroyWindow(hWnd); // Close the window
            return TRUE;

        case IDC_PAUSE: // If the Pause button was pressed
            if (!ClockPaused) {
                SuspendThread(hClock); // Pause the clock thread
                ClockPaused = true;
            }
            else {
                ResumeThread(hClock); // Resume the clock thread
                ClockPaused = false;
            }
            return TRUE;
        }
        return FALSE;

    case WM_DESTROY:
        Terminate = true;
        Sleep(500);
        CloseHandle(hClock);
        PostQuitMessage(0); // Terminate the program
        return TRUE;

    case WM_TIMER:
        return TRUE;
    }

    return FALSE;
}
