#undef UNICODE // Switch the project to ASCII mode
#include <windows.h>
#include <stdio.h>
#include "Resource.h"
#include <commctrl.h> // Include CommCtrl.h for DateTime_GetSystemtime

#pragma warning(disable : 4996)

bool Terminate = false; // Thread termination request flag
HWND hMainWnd = 0; // Main window handle to be retained
HANDLE hClock = 0; // Clock thread handle
bool ClockPaused = false; // Flag to indicate if the clock is paused
SYSTEMTIME StartTime = {}; // To store the start time
PROCESS_INFORMATION pi = {}; // Store the process information

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
        Sleep(10); // Sleep for about 10 milliseconds
    }

    if (pi.hProcess != NULL) {
        TerminateProcess(pi.hProcess, 0); // Terminate the child process
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    return 0;
}

// Define an array to store process handles
const int MAX_PROCESS_HANDLES = 10; // Adjust the size as needed
HANDLE processHandles[MAX_PROCESS_HANDLES] = { NULL };

BOOL CALLBACK MainWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch (Msg) {
    case WM_INITDIALOG:
        SetTimer(hWnd, 1, 250, NULL);
        hMainWnd = hWnd;
        hClock = CreateThread(NULL, 0, ClockThread, NULL, 0, NULL);

        SendMessage(GetDlgItem(hWnd, IDC_TIME), DTM_GETSYSTEMTIME, 0, (LPARAM)&StartTime);

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            Terminate = true; // Set the termination flag

            // Terminate the clock thread
            WaitForSingleObject(hClock, INFINITE);
            CloseHandle(hClock);

            // Terminate all process handles
            for (int i = 0; i < MAX_PROCESS_HANDLES; i++) {
                if (processHandles[i] != NULL) {
                    TerminateProcess(processHandles[i], 0);
                    CloseHandle(processHandles[i]);
                }
            }

            EndDialog(hWnd, 0); // Close the window and exit the application
            return TRUE;

        case IDC_PAUSE:
            if (!ClockPaused) {
                SuspendThread(hClock);
                ClockPaused = true;
            }
            else {
                ResumeThread(hClock);
                ClockPaused = false;
            }
            return TRUE;

        case IDC_ADD:
            SYSTEMTIME enteredTime;
            SendMessage(GetDlgItem(hWnd, IDC_TIME), DTM_GETSYSTEMTIME, 0, (LPARAM)&enteredTime);
            unsigned __int64 enteredTimeInt = SystemTimeToInt(enteredTime);

            SYSTEMTIME currentTime;
            GetLocalTime(&currentTime);
            unsigned __int64 currentTimeInt = SystemTimeToInt(currentTime);

            if (enteredTimeInt <= currentTimeInt) {
                MessageBox(hWnd, "The entered time is earlier than the current time.", "Alert", MB_ICONWARNING | MB_OK);
            }
            else {


                char timestr[9];
                sprintf_s(timestr, sizeof(timestr), "%.2d:%.2d:%.2d",
                    enteredTime.wHour, enteredTime.wMinute, enteredTime.wSecond);

                // Find an available slot in the array and store the process handle
                for (int i = 0; i < MAX_PROCESS_HANDLES; i++) {
                    if (processHandles[i] == NULL) {
                        processHandles[i] = (HANDLE)i; // Replace this with the actual process handle
                        break;
                    }
                }

                SendDlgItemMessage(hWnd, IDC_TIMELIST, LB_ADDSTRING, 0, (LPARAM)timestr);
            }
            return TRUE;
        }
        return FALSE;

    case WM_DESTROY:
        Terminate = true;
        WaitForSingleObject(hClock, INFINITE);
        CloseHandle(hClock);

        // Terminate all process handles
        for (int i = 0; i < MAX_PROCESS_HANDLES; i++) {
            if (processHandles[i] != NULL) {
                TerminateProcess(processHandles[i], 0);
                CloseHandle(processHandles[i]);
            }
        }

        if (pi.hProcess != NULL) {
            TerminateProcess(pi.hProcess, 0);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        PostQuitMessage(0);
        return TRUE;

    case WM_TIMER:
        return TRUE;
    }

    return FALSE;
}
