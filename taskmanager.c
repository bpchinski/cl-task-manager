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
    // szProcessName is the name of the process
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
    TCHAR szRAMUsage[32] = TEXT("N/A");
    TCHAR szOwner[256] = TEXT("N/A");  // New variable for the owner

    // Open a handle to the specified process
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);

    // Get the process name
    if (NULL != hProcess) {
        HMODULE hMod;
        DWORD cbNeeded;

        // EnumProcessModules is a WindowsAPI function that retrieves a handle for each module in the specified process
        // GetModuleBaseName is a WindowsAPI function that retrieves the base name of the specified module
        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
            GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
        }

        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
            _stprintf_s(szRAMUsage, sizeof(szRAMUsage) / sizeof(TCHAR), TEXT("%lu KB"), pmc.WorkingSetSize / 1024);
        }

        // Get the owner information
        GetProcessOwner(processID, szOwner, sizeof(szOwner) / sizeof(TCHAR));
    }

   // Skip printing if the process name is "unknown"
    if (_tcscmp(szProcessName, TEXT("<unknown>")) != 0) {
        _tprintf(TEXT("%-32s %-15s %-32s\n"), szProcessName, szRAMUsage, szOwner);
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
    _tprintf(TEXT("%-32s %-15s %-32s\n"), TEXT("Process Name"), TEXT("RAM Usage"), TEXT("Owner"));  // Update the table header
    _tprintf(TEXT("-------------------------------- ---------------- ---------------\n"));

    for (i = 0; i < cProcesses; i++) {
        if (aProcesses[i] != 0) {
            printProcessInformation(aProcesses[i]);
        }
    }
}

int main() {
    printProcesses();
    return 0;
}