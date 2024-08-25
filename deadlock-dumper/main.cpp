#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdio>
#include <cinttypes>
#include <vector>

constexpr static uintptr_t StoreAddr = 0x2382e78 + 0x3f8;
constexpr static uintptr_t GetModifierNameFromSubclassAddr = 0x5d5550;

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

std::vector<unsigned> unknowns{
	3516947824,491391007,1797283378,1142270357,1144549437,381961617,3516947824,876563814,3516947824,491391007,2414191464,381961617,3399065363,731943444,811521119,731943444,2414191464,3403085434,2414191464,2356412290,731943444,2064029594,731943444,84321454,2971868509,1928108461,1928108461,754480263,2521902222,4104549924,3832675871,3832675871,2521902222,1928108461,2971868509,3832675871,754480263,1548066885,380806748,3399065363,3970837787,1710079648,754480263,876563814,2603935618,3261353684,1999680326,1999680326,1842576017,1548066885,1842576017,2829638276,1710079648,754480263,2981692841,1999680326,811521119,2981692841,2981692841,84321454,2981692841,1548066885,968099481,395867183,1144549437,2971868509,876563814,938149308,1976701714,1976701714,938149308,3862866912,395867183,2603935618,876563814,938149308,1366719170,1548066885,2010028405,1366719170,3399065363,380806748,395867183,2152872419,1235347618,3561817145,3561817145,519124136,3561817145,397010810,1548066885,380806748,3561817145,519124136,519124136,395867183,519124136,397010810,397010810,2048438176,537527508,2048438176,1796564033,1710079648,2010028405,395867183,1796564033,537527508,2152872419,1796564033,1656913918,1656913918,754480263,1710079648,2702908623,2702908623,2702908623,2702908623,1656913918,2356412290,1656913918
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

	std::vector<unsigned> failed;

	printf("{\n");

	auto x = [&](unsigned subclassId) {
		const char* name = GetModifierNameFromSubclass(table, subclassId);
		if (strlen(name) > 0) {
			printf("    \"%u\": \"%s\",\n", (unsigned)subclassId, name);
		} else {
			failed.push_back(subclassId);
		}
	};

	for (int i = 0; i < table->hashTable.size; i++) {
		int subclassId = table->hashTable.entries[i].id;
		x(subclassId);
	}
	for (auto subclassId : unknowns) {
		x(subclassId);
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
