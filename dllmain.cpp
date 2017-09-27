#include <Windows.h>
#include <iostream>
#include <string>
#include <sstream>

#include "credits.h"
#include "jmp.h"

AldosV::CJmpHook config_Jmp,
	build_Jmp;
AldosV::CCredits credits;

FILE* dbg;

void StartModule();
void EndModule();

// If true, saves all data passed to the config engine to C:/ra2/ra2_dbg.txt
// If any other functions are true, this may not be called.
bool bShouldSaveConfigs = false;

// Skirmish mode only
// If true, disables AI from being able to build anything. So they just sit there confused.
bool bShouldCrippleEnemies = false;

// Will set and maintain your credits at an infinite amount.
bool bInfiniteCredits = true;

bool shouldrun = true;
bool ingame = false;

DWORD WINAPI InitialiseStateThread();
void CheckForCompatibleVersion(HMODULE base);

bool DoesDirectoryExist(const char* pszDirectoryName);
void SaveConfigurationSetting(const char* pszConfig, const char* pszName, int iValue, int iResult);
std::string AssembleConfigContents(const char* pszKey, int iValue, int iResult);

const char* pszRA2ConfigDirectory = "C:/RA2/Configs/";
const char* pszRA2Debug = "C:/RA2/ra2_dbg.txt";

const char* pszSupportedVersion = "1.006";

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

	if (!DoesDirectoryExist(pszRA2ConfigDirectory)) {
		CreateDirectoryA(pszRA2ConfigDirectory, 0);
	}

	dbg = fopen(pszRA2Debug, "w");
	if (!dbg) {
		printf("Failed to open file.\n");
		return;
	}

	printf("Attempting to get handle to game.exe ...\n");
	HMODULE game = GetModuleHandleA("game.exe");

	if (!game) {
		printf("Failed to get handle to main module (returned null)\n");
		return;
	}

	credits.Setup(game);
	printf("C&C Red Alert 2 Research - github.com/ald0s\nChecking for compatability...\n\n");
	CheckForCompatibleVersion(game);

	printf("Base address is %p\n", (DWORD)game);

	intptr_t targetAddress = ((DWORD)game) + 0x10d7f0;
	printf("Hooking config function at %p\n", targetAddress);
	config_Jmp.Hook((void*)targetAddress, &hook_GetConfigurationValue);

	intptr_t buildTargetAddress = ((DWORD)game) + 0x27f050;
	printf("Hooking build function at %p\n", buildTargetAddress);
	build_Jmp.Hook((void*)buildTargetAddress, &hook_CheckBuild);

	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)InitialiseStateThread, 0, 0, 0);
}

void EndModule() {
	config_Jmp.Remove();
	build_Jmp.Remove();

	fclose(dbg);
	shouldrun = false;
}

void CheckForCompatibleVersion(HMODULE base) {
	const char* pszGameVersion = (const char*)((intptr_t)base) + 0x640c94;
	if (strcmp(pszGameVersion, pszSupportedVersion) == 0) {
		printf("Your game is supported! Everything should work.\n");
	}
	else {
		printf("WARNING!! Your game isn't supported. You may have to modify a few calculations.\n");
	}
}

DWORD WINAPI InitialiseStateThread() {
	while (shouldrun) {

		if (bInfiniteCredits)
			credits.SetCredit(100000);
	}
	return 0;
}

void SaveConfigurationSetting(const char* pszConfig, const char* pszName, int iValue, int iResult) {
	std::string dirname;
	dirname.append(pszRA2ConfigDirectory);
	dirname.append(pszConfig);

	if (!DoesDirectoryExist(dirname.c_str())) {
		CreateDirectoryA(dirname.c_str(), 0);
	}

	std::string filename;
	filename += dirname;
	filename.append("/");
	filename.append(pszName);
	filename.append(".txt");

	std::string contents = AssembleConfigContents(pszName, iValue, iResult);
	const char* pszContents = contents.c_str();

	FILE* output = fopen(filename.c_str(), "w");
	if (output) {
		fwrite(pszContents, sizeof(char), strlen(pszContents), output);
		fflush(output);

		fclose(output);
	}
}

bool DoesDirectoryExist(const char* pszDirectoryName) {
	DWORD result = GetFileAttributesA(pszDirectoryName);
	switch (result) {
	case FILE_ATTRIBUTE_DIRECTORY:
		return true;

	case INVALID_FILE_ATTRIBUTES:
	default:
		return false;
	}
}

std::string AssembleConfigContents(const char* pszKey, int iValue, int iResult) {
	std::string contents;
	contents.append("Value: ");
	contents.append(std::to_string(iValue));
	contents.append("\n");
	contents.append("Return: ");
	contents.append(std::to_string(iResult));

	return contents;
}
