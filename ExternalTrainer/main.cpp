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

void PatchProcessMemory(HANDLE hProcess, BYTE* destination, BYTE* source, size_t size) {
	DWORD oldMemoryProtectPermission;
	VirtualProtectEx(hProcess, destination, size, PAGE_READWRITE, &oldMemoryProtectPermission);
	WriteProcessMemory(hProcess, destination, source, size, nullptr);
	VirtualProtectEx(hProcess, destination, size, oldMemoryProtectPermission, &oldMemoryProtectPermission);
}

void PatchProcessMemoryWithNoOP(HANDLE hProcess, BYTE* destination, size_t size) {
	BYTE* noOPArray = new BYTE[size];
	memset(noOPArray, 0x90, size);

	PatchProcessMemory(hProcess, destination, noOPArray, size);
	delete[] noOPArray;
}

void ClearConsole() {
	COORD topLeft = { 0, 0 };
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO screen;
	DWORD written;

	GetConsoleScreenBufferInfo(console, &screen);
	FillConsoleOutputCharacterA(
		console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);
	FillConsoleOutputAttribute(
		console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
		screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);
	SetConsoleCursorPosition(console, topLeft);
}

int main() {
	DWORD processID = GetProcessID(L"ac_client.exe");
	while (!processID) {
		Sleep(1000);
		ClearConsole();
		std::cout << "AssaultCube process is not detected. Waiting..." << std::endl;
		processID = GetProcessID(L"ac_client.exe");
	}

	std::cout << "AssaultCube process is detected. Activating cheat!" << std::endl;

	bool bRecoil = false;
	bool bAmmo = false;
	bool bHealth = false;
	int newValue = 1000;

	uintptr_t moduleBase = GetModuleBaseAddress(processID, L"ac_client.exe");
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, processID);

	DWORD terminationStatus = 0;
	while (GetExitCodeProcess(hProcess, &terminationStatus) && terminationStatus == STILL_ACTIVE) {
		if (GetAsyncKeyState(VK_NUMPAD1) & 1) {
			bRecoil = !bRecoil;

			if (bRecoil) {
				PatchProcessMemoryWithNoOP(hProcess, (BYTE*)(moduleBase + 0x63786), 10);
			}
			else {
				PatchProcessMemory(hProcess, (BYTE*)(moduleBase + 0x63786), (BYTE*)"\x50\x8D\x4C\x24\x1C\x51\x8B\xce\xff\xd2", 10);
			}
			std::cout << "No Recoil: " << std::boolalpha << bRecoil << std::endl;
		}
		if (GetAsyncKeyState(VK_NUMPAD2) & 1) {
			bHealth = !bHealth;
			std::cout << "Infinite Health: " << std::boolalpha << bHealth << std::endl;
		}
		if (GetAsyncKeyState(VK_NUMPAD3) & 1) {
			bAmmo = !bAmmo;
			if (bAmmo) {
				PatchProcessMemory(hProcess, (BYTE*)(moduleBase + 0x637e9), (BYTE*)"\xFF\x06", 2);
			}
			else {
				PatchProcessMemory(hProcess, (BYTE*)(moduleBase + 0x637e9), (BYTE*)"\xFF\x0E", 2);
			}
			std::cout << "Infinite Ammo: " << std::boolalpha << bAmmo << std::endl;
		}
		if (GetAsyncKeyState(VK_NUMPAD4) & 1) {
			return 0;
		}

		if (bHealth) {
			uintptr_t healthAddress = FindDynamicMemoryAddress(hProcess, (moduleBase + 0x10F4F4), {0xF8});
			WriteProcessMemory(hProcess, (BYTE*)healthAddress, &newValue, sizeof(newValue), nullptr);
		}
		
		Sleep(500);
	}
}