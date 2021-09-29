#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>

DWORD GetProcessID(const wchar_t* processName) {
	DWORD processID = 0;

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32 processEntry;
		processEntry.dwSize = sizeof(processEntry);
		if (Process32First(hSnapshot, &processEntry)) {
			do {
				if (!_wcsicmp(processEntry.szExeFile, processName)) {
					processID = (DWORD)processEntry.th32ProcessID;
				}
			} while (Process32Next(hSnapshot, &processEntry));
		}
	}

	CloseHandle(hSnapshot);
	return processID;
}

int main() {
	const char* dllpath = "your_dll_path_here";
	DWORD processID = 0;
	while (!processID) {
		processID = GetProcessID(L"ac_client.exe");
		Sleep(500);
	}

	std::cout << "Found AssaultCube process, injecting!" << std::endl;

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, processID);
	if (hProcess && hProcess != INVALID_HANDLE_VALUE) {
		void* loc = VirtualAllocEx(hProcess, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (loc) {
			WriteProcessMemory(hProcess, loc, dllpath, strlen(dllpath) + 1, 0);
		}

		HANDLE hThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, 0);
		if (hThread) {
			CloseHandle(hThread);
		}
	}

	if (hProcess) {
		CloseHandle(hProcess);
	}

	return 0;
}