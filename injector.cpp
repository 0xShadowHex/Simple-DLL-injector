#include <windows.h>
#include <iostream>
#include <string>

// Simple DLL injector using CreateRemoteThread
// This is for educational purposes only

bool InjectDLL(DWORD processId, const char* dllPath) {
    // Get a handle to the target process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        std::cerr << "Error: Could not open process. Error code: " << GetLastError() << std::endl;
        return false;
    }

    // Allocate memory in the target process for the DLL path
    LPVOID pDllPath = VirtualAllocEx(hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
    if (!pDllPath) {
        std::cerr << "Error: Could not allocate memory in target process. Error code: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    // Write the DLL path to the allocated memory
    if (!WriteProcessMemory(hProcess, pDllPath, dllPath, strlen(dllPath) + 1, NULL)) {
        std::cerr << "Error: Could not write to process memory. Error code: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Get the address of LoadLibraryA function
    LPVOID pLoadLibrary = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
    if (!pLoadLibrary) {
        std::cerr << "Error: Could not get address of LoadLibraryA. Error code: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Create a remote thread that calls LoadLibraryA with the DLL path as argument
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, 
                                       (LPTHREAD_START_ROUTINE)pLoadLibrary, 
                                       pDllPath, 0, NULL);
    if (!hThread) {
        std::cerr << "Error: Could not create remote thread. Error code: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Wait for the thread to finish
    WaitForSingleObject(hThread, INFINITE);

    // Get the thread exit code (should be the base address of the loaded DLL)
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);

    // Clean up
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    // Check if injection was successful
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

    // Get process ID from user
    DWORD processId;
    std::cout << "Enter target process ID: ";
    std::cin >> processId;

    // Get DLL path from user
    char dllPath[MAX_PATH];
    std::cout << "Enter full path to DLL: ";
    std::cin.ignore(); // Clear the input buffer
    std::cin.getline(dllPath, MAX_PATH);

    // Inject the DLL
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
