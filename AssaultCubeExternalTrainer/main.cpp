#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <vector>

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

uintptr_t GetModuleBaseAddress(DWORD processID, const wchar_t* moduleName) {
	uintptr_t baseAddress = 0;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processID);
	if (hSnapshot != INVALID_HANDLE_VALUE) {
		MODULEENTRY32 moduleEntry;
		moduleEntry.dwSize = sizeof(moduleEntry);
		if (Module32First(hSnapshot, &moduleEntry)) {
			do {
				std::cout << moduleEntry.szModule << std::endl;
				if (!_wcsicmp(moduleEntry.szModule, moduleName)) {
					baseAddress = (uintptr_t)moduleEntry.modBaseAddr;
				}
			} while (Module32Next(hSnapshot, &moduleEntry));
		}
	}

	CloseHandle(hSnapshot);
	return baseAddress;
}

uintptr_t FindDynamicMemoryAddress(HANDLE hProcess, uintptr_t baseAddress, std::vector<unsigned int> offsets) {
	uintptr_t address = baseAddress;
	for (int i = 0; i < offsets.size(); i++) {
		ReadProcessMemory(hProcess, (BYTE*)address, &address, sizeof(address), 0);
		address += offsets[i];
	}

	return address;
}

int main() {
	DWORD processID = GetProcessID(L"ac_client.exe");
	uintptr_t baseAddress = GetModuleBaseAddress(processID, L"ac_client.exe") + 0x10F4F4;
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, processID);

	int newValue = 1000;
	while (true) {
		// Infinite Ammo
		uintptr_t ammoAddress = FindDynamicMemoryAddress(hProcess, baseAddress, {0x374, 0x14, 0x0});
		WriteProcessMemory(hProcess, (BYTE*)ammoAddress, &newValue, sizeof(newValue), nullptr);
	}
}