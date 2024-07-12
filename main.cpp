#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>  // for _tcscmp
#include <stdio.h>  // for printf


// put the path to the folder that yourdll.dll is in
const char* dll_path = "C:\\Path\\To\\Dll\\YimMenu.dll";

// Function to check if GTA5 is running
bool IsGTA5Running() {
    HANDLE snapshot;
    PROCESSENTRY32 pe32;

    pe32.dwSize = sizeof(PROCESSENTRY32);

    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        printf("Failed to create snapshot.\n");
        return false;
    }

    if (Process32First(snapshot, &pe32)) {
        do {
            if (_tcscmp(pe32.szExeFile, _T("GTA5.exe")) == 0) {
                CloseHandle(snapshot);
                return true;
            }
        } while (Process32Next(snapshot, &pe32));
    }

    CloseHandle(snapshot);
    return false;
}

int main(int argc, char** argv) {
    if (!IsGTA5Running()) {
        printf("GTA5.exe is not running.\n");
        return 1;
    }

    HANDLE snapshot;
    PROCESSENTRY32 pe32;
    DWORD exitCode = 0;

    pe32.dwSize = sizeof(PROCESSENTRY32);

    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        printf("Failed to create snapshot.\n");
        return 1;
    }

    if (Process32First(snapshot, &pe32)) {
        do {
            if (_tcscmp(pe32.szExeFile, _T("GTA5.exe")) == 0) {
                HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
                if (process == NULL) {
                    printf("Failed to open process.\n");
                    continue;
                }

                void* lpBaseAddress = VirtualAllocEx(process, NULL, strlen(dll_path) + 1, MEM_COMMIT, PAGE_READWRITE);
                if (lpBaseAddress == NULL) {
                    printf("Failed to allocate memory in target process.\n");
                    CloseHandle(process);
                    continue;
                }

                if (!WriteProcessMemory(process, lpBaseAddress, dll_path, strlen(dll_path) + 1, NULL)) {
                    printf("Failed to write to process memory.\n");
                    VirtualFreeEx(process, lpBaseAddress, 0, MEM_RELEASE);
                    CloseHandle(process);
                    continue;
                }

                HMODULE kernel32base = GetModuleHandle(L"kernel32.dll");
                if (kernel32base == NULL) {
                    printf("Failed to get handle to kernel32.dll.\n");
                    VirtualFreeEx(process, lpBaseAddress, 0, MEM_RELEASE);
                    CloseHandle(process);
                    continue;
                }

                HANDLE thread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(kernel32base, "LoadLibraryA"), lpBaseAddress, 0, NULL);
                if (thread == NULL) {
                    printf("Failed to create remote thread.\n");
                    VirtualFreeEx(process, lpBaseAddress, 0, MEM_RELEASE);
                    CloseHandle(process);
                    continue;
                }

                WaitForSingleObject(thread, INFINITE);
                GetExitCodeThread(thread, &exitCode);

                if (exitCode == 0) {
                    printf("DLL injection failed.\n");
                }
                else {
                    printf("DLL injection succeeded.\n");
                }

                VirtualFreeEx(process, lpBaseAddress, 0, MEM_RELEASE);
                CloseHandle(thread);
                CloseHandle(process);

                // Wait for 5 seconds
                Sleep(5000);

                break;
            }
        } while (Process32Next(snapshot, &pe32));
    }
    else {
        printf("Failed to get first process.\n");
    }

    CloseHandle(snapshot);
    return 0;
}
