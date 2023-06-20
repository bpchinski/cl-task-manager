// windows.h provides necessary API functions to interact with Windows.
// stdio.h provides functions for performing input and output, such as printf.
// psapi.h provides functions for retrieving system information.
// tchar.h provides functionalities for manipulating C strings.
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h> 
#include <stdio.h>
#include <psapi.h>
#include <tchar.h>
#include <sddl.h>
#include <conio.h>  // For _getch()

void GetProcessOwner(DWORD processID, TCHAR* szOwner, size_t ownerBufferSize) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processID);
    if (hProcess == NULL) {
        _tcscpy(szOwner, TEXT("N/A"));
        return;
    }

    HANDLE hToken = NULL;
    if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
        _tcscpy(szOwner, TEXT("N/A"));
        CloseHandle(hProcess);
        return;
    }

    DWORD dwSize = 0;
    GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
    PTOKEN_USER pTokenUser = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);

    if (pTokenUser == NULL) {
        _tcscpy(szOwner, TEXT("N/A"));
        CloseHandle(hToken);
        CloseHandle(hProcess);
        return;
    }

    if (!GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize)) {
        _tcscpy(szOwner, TEXT("N/A"));
        HeapFree(GetProcessHeap(), 0, pTokenUser);
        CloseHandle(hToken);
        CloseHandle(hProcess);
        return;
    }

    TCHAR szName[128];       // Buffer for account name.
    DWORD cchName = 128;     // Size of name.
    TCHAR szDomain[128];     // Buffer for domain name.
    DWORD cchDomain = 128;   // Size of domain name.
    SID_NAME_USE eUse;       // SID type.

    // Translate SID to account name + domain.
    if (!LookupAccountSid(NULL, pTokenUser->User.Sid, szName, &cchName, szDomain, &cchDomain, &eUse)) {
        _tcscpy(szOwner, TEXT("N/A"));
    } else {
        _stprintf_s(szOwner, ownerBufferSize, TEXT("%s\\%s"), szDomain, szName);
    }

    HeapFree(GetProcessHeap(), 0, pTokenUser);
    CloseHandle(hToken);
    CloseHandle(hProcess);
}


// Function to print the process name and ID.
void printProcessInformation(DWORD processID) {
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
    TCHAR szRAMUsage[32] = TEXT("N/A");
    TCHAR szOwner[256] = TEXT("N/A");
    TCHAR szPath[MAX_PATH] = TEXT("N/A");  // New variable for the path
    TCHAR szCPUTime[64] = TEXT("N/A");  // New variable for the CPU time

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);

    if (NULL != hProcess) {
        HMODULE hMod;
        DWORD cbNeeded;

        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
            GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
        }

        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
            _stprintf_s(szRAMUsage, sizeof(szRAMUsage) / sizeof(TCHAR), TEXT("%lu KB"), pmc.WorkingSetSize / 1024);
        }

        GetProcessOwner(processID, szOwner, sizeof(szOwner) / sizeof(TCHAR));

        // Get the path to the executable
        DWORD dwSize = MAX_PATH;
        if (QueryFullProcessImageName(hProcess, 0, szPath, &dwSize)) {
            // Successfully retrieved the path
        } else {
            _tcscpy(szPath, TEXT("N/A"));
        }

        // Get the CPU time
        FILETIME ftCreation, ftExit, ftKernel, ftUser;
        if (GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
            ULARGE_INTEGER ulKernel, ulUser;
            ulKernel.LowPart = ftKernel.dwLowDateTime;
            ulKernel.HighPart = ftKernel.dwHighDateTime;
            ulUser.LowPart = ftUser.dwLowDateTime;
            ulUser.HighPart = ftUser.dwHighDateTime;

            ULONGLONG ullCPUTime = ulKernel.QuadPart + ulUser.QuadPart;
            ullCPUTime /= 10000;  // Convert to milliseconds
            _stprintf_s(szCPUTime, sizeof(szCPUTime) / sizeof(TCHAR), TEXT("%llu ms"), ullCPUTime);
        } else {
            _tcscpy(szCPUTime, TEXT("N/A"));
        }
    }
    if (_tcscmp(szProcessName, TEXT("<unknown>")) != 0) {
        _tprintf(TEXT("%-32s %-12s %-12s %-12s %-244s\n"), szProcessName, szRAMUsage, szOwner, szCPUTime, szPath);
    }
    // Close the process handle as it's no longer needed
    CloseHandle(hProcess);
}

// Function to print the details of all running processes
void printProcesses() {
    // aProcesses is the pointer to an array that receives the list of process identifiers
    DWORD aProcesses[1024];
    // The number of bytes returned in the aProcesses array by EnumProcesses
    DWORD cbNeeded;
    // cdNeeded receives the number of bytes returned in the aProcesses array
    DWORD cProcesses;
    unsigned int i;

    // EnumProcesses is a WindowsAPI function that retrieves the process identifier for each process object in the system
    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
        return;
    }

    // Calculate how many process identifiers were returned
    cProcesses = cbNeeded / sizeof(DWORD);

    // Print header for the table
    printf("Controls:\n\ninput: 'q' to quit \ninput: 'r' to refresh\n\n");
    _tprintf(TEXT("%-32s %-12s %-12s %-12s %-244s\n"), TEXT("Process Name"), TEXT("RAM Usage"), TEXT("Owner"), TEXT("CPU Time"), TEXT("Path"));
    _tprintf(TEXT("-------------------------------- ------------ ------------ ------------ ----------------------------------------------------------------------------------------\n"));

    for (i = 0; i < cProcesses; i++) {
        if (aProcesses[i] != 0) {
            printProcessInformation(aProcesses[i]);
        }
    }
}

int main() {
    while (1) {  // Infinite loop
        printProcesses();  // Print the process table

        printf("\nOption? ");
        char option = _getch();  // Get the user's input

        if (option == 'r') {
            system("cls");  // Clear the console
        } else if (option == 'q') {
            system("cls");  // Clear the console
            break;  // Break the loop
        } else {
            system("cls");  // Clear the console
            printf("Invalid option!\n\n");
        }
    }

    return 0;
}