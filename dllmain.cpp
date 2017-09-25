#include <Windows.h>
#include <iostream>
#include <string>
#include <sstream>

#include "jmp.h"

AldosV::CJmpHook config_Jmp,
	build_Jmp;
FILE* dbg;

void StartModule();
void EndModule();

// If true, saves all data passed to the config engine to C:/ra2/ra2_dbg.txt
// If any other functions are true, this may not be called.
bool bShouldSaveConfigs = false;

// Skirmish mode only
// If true, disables AI from being able to build anything. So they just sit there confused.
bool bShouldCrippleEnemies = true;

void SaveConfigurationSetting(const char* pszConfig, const char* pszName, int iValue, int iResult);

BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    lpvReserved
) {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		StartModule();
		break;

	case DLL_PROCESS_DETACH:
		EndModule();
		break;
	}

	return TRUE;
}

typedef int(__thiscall* hGetConfigurationValue)(void*, const char*, const char*, int);
int __fastcall hook_GetConfigurationValue(void* thisptr, void* edx, const char* pszUIName, const char* pszKey, int iValue) {

	// Write the original set of instructions over the start of the sub procedure.
	hGetConfigurationValue original = (hGetConfigurationValue)config_Jmp.GetOriginal();
	
	// If this is an AI configuration entry.
	if (bShouldCrippleEnemies && strcmp(pszUIName, "AI") == 0) {
		// Push a -1 into the value, parameter and also return -1.
		// This seems to disable the AI. Inserting '0' tends to force the AI to use the default settings anyway?

		original(thisptr, pszUIName, pszKey, -1);
		config_Jmp.Reset();

		return -1;
	}

	int result = original(thisptr, pszUIName, pszKey, iValue);
	
	// If nothing has interrupted the flow of execution thus far, user might want to save the config.
	if (bShouldSaveConfigs) {
		SaveConfigurationSetting(pszUIName, pszKey, iValue, result);
	}

	config_Jmp.Reset();
	return result;
}

// Some function I thought controlled part of the building construction process.
// Still heavily under research.
typedef char(__thiscall* hCheckBuild)(void*, int, int);
char __fastcall hook_CheckBuild(void* thisptr, void* edx, int iOne, int iTwo) {
	hCheckBuild original = (hCheckBuild)build_Jmp.GetOriginal();

	char result = original(thisptr, iOne, iTwo);

	build_Jmp.Reset();
	return result;
}

void StartModule() {
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	dbg = fopen("C:/RA2/ra2_dbg.txt", "w");
	if (!dbg) {
		printf("Failed to open file.\n");
	}

	printf("Attempting to get handle to game.exe ...\n");
	HMODULE game = GetModuleHandleA("game.exe");

	if (!game) {
		printf("Failed to get handle to main module (returned null)\n");
		return;
	}

	printf("Base address is %p\n", (DWORD)game);

	intptr_t targetAddress = ((DWORD)game) + 0x10d7f0;
	printf("Hooking config function at %p\n", targetAddress);
	config_Jmp.Hook((void*)targetAddress, &hook_GetConfigurationValue);

	intptr_t buildTargetAddress = ((DWORD)game) + 0x27f050;
	printf("Hooking build function at %p\n", buildTargetAddress);
	build_Jmp.Hook((void*)buildTargetAddress, &hook_CheckBuild);

}

void EndModule() {
	config_Jmp.Remove();
	build_Jmp.Remove();

	fclose(dbg);
}

void SaveConfigurationSetting(const char* pszConfig, const char* pszName, int iValue, int iResult) {
	if (dbg) {
		std::stringstream ss;
		ss.clear();

		ss << "Config: " << pszConfig << "\n";
		ss << "Name: " << pszName << "\n";
		ss << "Value: " << std::to_string(iValue) << "\n";
		ss << "Return: " << std::to_string(iResult) << "\n\n";

		const char* pszBuffer = ss.str().c_str();
		printf(pszBuffer);
		fwrite(pszBuffer, sizeof(char), strlen(pszBuffer), dbg);
		fflush(dbg);
	}
}
