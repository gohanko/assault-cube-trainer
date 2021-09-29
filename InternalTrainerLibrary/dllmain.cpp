// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <iostream>
#include <Windows.h>
#include <vector>
#include <TlHelp32.h>

void PatchProcessMemory(BYTE* destination, BYTE* source, size_t size) {
    DWORD oldMemoryProtectPermission;
    VirtualProtect(destination, size, PAGE_READWRITE, &oldMemoryProtectPermission);
    memcpy(destination, source, size);
    VirtualProtect(destination, size, oldMemoryProtectPermission, &oldMemoryProtectPermission);
}

void PatchProcessMemoryWithNoOP(BYTE* destination, size_t size) {
    BYTE* noOPArray = new BYTE[size];
    memset(noOPArray, 0x90, size);
    PatchProcessMemory(destination, noOPArray, size);
    delete[] noOPArray;
}

DWORD WINAPI HackThread(HMODULE hModule) {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    uintptr_t moduleBase = (uintptr_t)GetModuleHandle(L"ac_client.exe");

    bool bRecoil = false;
    bool bHealth = false;
    bool bAmmo = false;

    while (true) {
		if (GetAsyncKeyState(VK_NUMPAD1) & 1) {
            bRecoil = !bRecoil;

            if (bRecoil) {
                PatchProcessMemoryWithNoOP((BYTE*)(moduleBase + 0x63786), 10);
            } else {
                PatchProcessMemory((BYTE*)(moduleBase + 0x63786), (BYTE*)"\x50\x8D\x4C\x24\x1C\x51\x8B\xCE\xFF\xD2", 10);
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
                PatchProcessMemory((BYTE*)(moduleBase + 0x637e9), (BYTE*)"\xFF\x06", 2);
            }
            else {
                PatchProcessMemory((BYTE*)(moduleBase + 0x637e9), (BYTE*)"\xFF\x0E", 2);
            }
            std::cout << "Infinite Ammo: " << std::boolalpha << bAmmo << std::endl;
		}
		if (GetAsyncKeyState(VK_NUMPAD4) & 1) {
			return 0;
		}

        uintptr_t* localPlayerPointer = (uintptr_t*)(moduleBase + 0x10F4F4);

        // Continuous write
        if (bHealth) {
            *(int*)(*localPlayerPointer + 0xF8) = 1000;
        }

        Sleep(500);
    }

    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved ) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        {
            CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)HackThread, hModule, 0, nullptr));
        }
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

