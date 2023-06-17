// windows.h provides necessary API functions to interact with Windows.
// stdio.h provides functions for performing input and output, such as printf.
// psapi.h provides functions for retrieving system information.
// tchar.h provides functionalities for manipulating C strings.
#include <windows.h> 
#include <stdio.h>
#include <psapi.h>
#include <tchar.h>


// Function to print the process name and ID.
void printProcessNameAndID(DWORD processID) {
    // szProcessName is the name of the process
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

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
    }

    // Skip printing if the process name is "unknown"
    if (_tcscmp(szProcessName, TEXT("<unknown>")) != 0) {
        _tprintf(TEXT("%-32s %-10lu\n"), szProcessName, processID);
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
    _tprintf(TEXT("%-32s %-10s\n"), TEXT("Process Name"), TEXT("Process ID"));
    _tprintf(TEXT("-------------------------------- ----------------\n"));

    // Print the name and additional to-be added information for each process
    for (i = 0; i < cProcesses; i++) {
        if (aProcesses[i] != 0) {
            printProcessNameAndID(aProcesses[i]);
        }
    }
}



int main() {
    printProcesses();
    return 0;
}
