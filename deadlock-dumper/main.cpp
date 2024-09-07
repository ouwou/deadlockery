#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdio>
#include <cinttypes>
#include <vector>

constexpr static uintptr_t StoreAddr = 0x23d1330;
constexpr static uintptr_t GetModifierNameFromSubclassAddr = 0x5f0de0;

using GetModifierNameFromSubclassFunc = const char* (__fastcall*)(void* table, int modifierSubclass);
GetModifierNameFromSubclassFunc GetModifierNameFromSubclass;

void DoFunStuff() {
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	uintptr_t base = (uintptr_t)GetModuleHandleA("client.dll");
	printf("Base: 0x%" PRIxPTR "\n", base);

	GetModifierNameFromSubclass = (GetModifierNameFromSubclassFunc)(base + GetModifierNameFromSubclassAddr);
	auto* table = (void*)(base + StoreAddr);

	printf("GetModifierNameFromSubclass: 0x%" PRIxPTR "\n", GetModifierNameFromSubclass);
	printf("Data table location: 0x%" PRIxPTR "\n", table);

	std::vector<unsigned> failed;

	printf("{\n");

	auto x = [&](unsigned subclassId) {
		const char* name = GetModifierNameFromSubclass(table, subclassId);
		if (name && name[0] != '\0') {
			printf("    \"%u\": \"%s\",\n", (unsigned)subclassId, name);
		} else {
			failed.push_back(subclassId);
		}
	};

	for (uint32_t i = 0; i < UINT32_MAX; i++) {
		x(i);
	}

	printf("}\n");
}

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		DoFunStuff();
		break;
	default:
		break;
	}
	return true;
}
