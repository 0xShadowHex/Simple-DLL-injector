#include <windows.h>
#include <iostream>
#include <string>

bool InjectDLL(DWORD processId, const char* dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        std::cerr << "Error: Could not open process. Error code: " << GetLastError() << std::endl;
        return false;
    }

    LPVOID pDllPath = VirtualAllocEx(hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
    if (!pDllPath) {
        std::cerr << "Error: Could not allocate memory in target process. Error code: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    if (!WriteProcessMemory(hProcess, pDllPath, dllPath, strlen(dllPath) + 1, NULL)) {
        std::cerr << "Error: Could not write to process memory. Error code: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    LPVOID pLoadLibrary = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
    if (!pLoadLibrary) {
        std::cerr << "Error: Could not get address of LoadLibraryA. Error code: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, 
                                       (LPTHREAD_START_ROUTINE)pLoadLibrary, 
                                       pDllPath, 0, NULL);
    if (!hThread) {
        std::cerr << "Error: Could not create remote thread. Error code: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);

    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    if (exitCode == 0) {
        std::cerr << "Error: DLL injection failed." << std::endl;
        return false;
    }

    std::cout << "DLL successfully injected at address: 0x" << std::hex << exitCode << std::endl;
    return true;
}

int main() {
    std::cout << "Simple DLL Injector (CreateRemoteThread Method)" << std::endl;
    std::cout << "------------------------------------------------" << std::endl;

    DWORD processId;
    std::cout << "Enter target process ID: "; //look up in task manager, u cn set it to show PID
    std::cin >> processId;

    char dllPath[MAX_PATH]; //enter without the string brackets "", full path, not relative
    std::cout << "Enter full path to DLL: ";
    std::cin.ignore(); 
    std::cin.getline(dllPath, MAX_PATH);

    std::cout << "Attempting to inject DLL..." << std::endl;
    if (InjectDLL(processId, dllPath)) {
        std::cout << "Injection successful!" << std::endl;
    } else {
        std::cout << "Injection failed!" << std::endl;
    }

    std::cout << "Press Enter to exit...";
    std::cin.get();
    return 0;
}
