#undef UNICODE // Switch the project to ASCII mode
#include <windows.h>
#include <stdio.h>
#include "Resource.h"
#include <commctrl.h> // Include CommCtrl.h for DateTime_GetSystemtime
#include <processthreadsapi.h>
#include <winuser.h> 
#include <wingdi.h>

#pragma warning(disable : 4996)

HFONT hCustomFont = NULL;

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
DWORD WINAPI ScheduleThread(LPVOID lpParameter) {
    SYSTEMTIME StartTime;
    if (DateTime_GetSystemtime(GetDlgItem(hMainWnd, IDC_TIME), &StartTime)) {
        while (true) {
            SYSTEMTIME CurrentTime;
            GetLocalTime(&CurrentTime);

            if (SystemTimeToInt(CurrentTime) > SystemTimeToInt(StartTime) || Terminate) {
                break;
            }

            Sleep(10);
        }

        char sStartTime[20];
        sprintf_s(sStartTime, "%.2d:%.2d:%.2d", StartTime.wHour, StartTime.wMinute, StartTime.wSecond);

        int itemIndex = SendMessage(GetDlgItem(hMainWnd, IDC_TIMELIST), LB_FINDSTRINGEXACT, -1, (LPARAM)sStartTime);

        if (itemIndex != LB_ERR && !Terminate) {
            char commandLine[512];
            GetDlgItemTextA(hMainWnd, IDC_COMMANDLINE, commandLine, sizeof(commandLine));

            STARTUPINFO si = { sizeof(STARTUPINFO) };
            PROCESS_INFORMATION pi;

            if (CreateProcess(NULL, commandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }

            SendMessage(GetDlgItem(hMainWnd, IDC_TIMELIST), LB_DELETESTRING, itemIndex, 0);
        }
    }

    return 0;
}

BOOL CALLBACK MainWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch (Msg) {
    case WM_INITDIALOG:
        hMainWnd = hWnd;

        hCustomFont = CreateFont(20, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, NULL);

        // Set the font for IDC_CLOCK
        SendMessage(GetDlgItem(hWnd, IDC_CLOCK), WM_SETFONT, (WPARAM)hCustomFont, TRUE);


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

        case IDC_ADD:
        {
            SYSTEMTIME TimeFromIDC_TIME;
            DateTime_GetSystemtime(GetDlgItem(hMainWnd, IDC_TIME), &TimeFromIDC_TIME);
            SYSTEMTIME CurrentTime;
            GetLocalTime(&CurrentTime);

            if (SystemTimeToInt(TimeFromIDC_TIME) <= SystemTimeToInt(CurrentTime)) {
                MessageBox(hMainWnd, "Selected time is in the past.", "Error", MB_OK | MB_ICONERROR);
            }
            else {
                char sTimeFromIDC_TIME[20];
                sprintf_s(sTimeFromIDC_TIME, "%.2d:%.2d:%.2d", TimeFromIDC_TIME.wHour, TimeFromIDC_TIME.wMinute, TimeFromIDC_TIME.wSecond);

                int itemIndex = SendMessage(GetDlgItem(hMainWnd, IDC_TIMELIST), LB_ADDSTRING, 0, (LPARAM)sTimeFromIDC_TIME);

                CreateThread(NULL, 0, ScheduleThread, NULL, 0, NULL);
            }
        }
        case IDC_DELETE:
        {
            int selectedRow = SendMessage(GetDlgItem(hMainWnd, IDC_TIMELIST), LB_GETCURSEL, 0, 0);
            if (selectedRow != LB_ERR) {
                SendMessage(GetDlgItem(hMainWnd, IDC_TIMELIST), LB_DELETESTRING, selectedRow, 0);
            }
        }

        return TRUE;
        }

        return FALSE;

    case WM_CTLCOLORSTATIC:
{
    HDC hdcStatic = (HDC)wParam;
    HWND hwndStatic = (HWND)lParam;

    if (GetDlgItem(hWnd, IDC_CLOCK) == hwndStatic)
    {
        if (ClockPaused)
        {
            SetTextColor(hdcStatic, RGB(255, 0, 0)); // Red
        }
        else
        {
            SetTextColor(hdcStatic, RGB(0, 128, 0)); // Green
        }

        return (INT_PTR)GetStockObject(NULL_BRUSH);
    }
}
break;


    case WM_DESTROY:
        Terminate = true;
        Sleep(500);
        CloseHandle(hClock);

        if (hCustomFont != NULL) {
            DeleteObject(hCustomFont);
            hCustomFont = NULL;
        }

        PostQuitMessage(0);
        return TRUE;
    }
    return FALSE;
}
