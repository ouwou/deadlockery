#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdio>
#include <cinttypes>
#include <vector>

constexpr static uintptr_t StoreAddr = 0x2382e48 + 0x3f8;
constexpr static uintptr_t GetModifierNameFromSubclassAddr = 0x5d53a0;

using GetModifierNameFromSubclassFunc = const char* (__fastcall*)(void* table, int modifierSubclass);
GetModifierNameFromSubclassFunc GetModifierNameFromSubclass;

struct HashTableEntry {
	int status;
	int unk1;
	int id;
};

struct HashTable {
	HashTableEntry* entries;
	int unk1;
	int status;
	int data;
	int size;
};

struct Store {
	uint8_t padding[104];
	HashTable hashTable;
	int unk1;
	int unk2;
	int unk3;
	void* dataPtr;
};

void DoFunStuff() {
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	uintptr_t base = (uintptr_t)GetModuleHandleA("client.dll");
	printf("Base: 0x%" PRIxPTR "\n", base);

	GetModifierNameFromSubclass = (GetModifierNameFromSubclassFunc)(base + GetModifierNameFromSubclassAddr);
	auto* table = (Store*)(base + StoreAddr);

	printf("GetModifierNameFromSubclass: 0x%" PRIxPTR "\n", GetModifierNameFromSubclass);
	printf("Data table location: 0x%" PRIxPTR "\n", table);

	printf("Hash table has size %d\n\n", table->hashTable.size);

	printf("{\n");
	for (int i = 0; i < table->hashTable.size; i++) {
		int subclassId = table->hashTable.entries[i].id;
		const char* name = GetModifierNameFromSubclass(table, subclassId);
		if (strlen(name) > 0) {
			printf("    \"%u\": \"%s\",\n", (unsigned)subclassId, name);
		}
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
