#include "stdafx.h"
#include "DebugUtils.h"

//Define BakedFile Index Function
typedef int(__fastcall* _GetBakeFileSearchIndex)(void* ecx, void* edx);
_GetBakeFileSearchIndex GetBakeFileSearchIndex;

//Define BakedFile Load Function
typedef void(__fastcall* _LoadBakedFilesUC)(void* bfPtr, void* bFileIdx, void* bIdxUnk, void* bIdxCeil);
_LoadBakedFilesUC LoadBakedFilesUC;

//Define BakedFile Load Func (with Path)
typedef void(*_LoadBakedFileAtPath)(const char*,int, int, int);
_LoadBakedFileAtPath LoadBakedFileAtPath;


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}

static void mergeRegistry( std::string directory, uintptr_t baseAddress,
	_LoadBakedFileAtPath loadFile, bool suppressPrompts) {
	
	std::vector<std::string> loadedFiles;
	std::vector<std::string> filePaths = DebugUtils::FindModFiles(directory);

	for (const auto& file : filePaths) {
		std::string bakedFile = "Mods/" + file;

		//Limit max chars
		if (bakedFile.size() < 256) {
			char* buffer = new char[256];
			memset(buffer, 0, 256);

			DWORD* dwordPtr = reinterpret_cast<DWORD*>(buffer);
			uint32_t param1 = 0xA0000100;
			*dwordPtr++ = param1;
			*dwordPtr++ = uint32_t(bakedFile.size());
			*dwordPtr++ = 0;
			*dwordPtr++ = 0;

			std::memcpy(dwordPtr, bakedFile.c_str(), bakedFile.size());
			loadFile(buffer, 0, /* UseAppData Flag Off */ 0, -2);
			loadedFiles.push_back(file);

			delete[] buffer;
		}

	}

	//Log to Window
	std::string fLog = "";
	for (const std::string& file : loadedFiles) {fLog += ("\n" + file);}

	if (loadedFiles.size() != 0 && !suppressPrompts) {
		std::string log = ("Loaded Mod File(s): \n" + fLog);
		DebugUtils::ShowMessageBoxNonBlocking(log);	}

}

static void mergeRegistriesUsingPath(std::string subDir, bool suppressPrompts) {
	uintptr_t modBase = (uintptr_t)GetModuleHandle(NULL);

	//Check if folder exists
	bool hasMods = DebugUtils::DirectoryExists(DebugUtils::GetDllDirectory()+"/"+subDir);

	if (hasMods)
	{
		//Define func
		LoadBakedFileAtPath = (_LoadBakedFileAtPath)(modBase + 0x282FF0);
		mergeRegistry(subDir, modBase, LoadBakedFileAtPath, suppressPrompts);

	}
}

static void mergeRegistriesAtIndex( int index , bool suppressPrompts) {
	uintptr_t modBase = (uintptr_t)GetModuleHandle(NULL);

	//define func ptr
	LoadBakedFilesUC = (_LoadBakedFilesUC)(modBase + 0x282B90);

	//Adjust this
	uint16_t bakedFileIndex = index;
	uint16_t bIndexCeiling = 
		bakedFileIndex + 1; /* -<--- This Argument is overwritten in our patched exe.
							Game will search without block.*/

	//Load BakedFile @ Index
	char* bfPtr = new char[250];
	LoadBakedFilesUC((void*)bfPtr, (void*)bakedFileIndex, (void*)bIndexCeiling, (void*)bIndexCeiling);

	//Find All BakedFiles within Index (For Logging)
	std::vector<std::string> paths = DebugUtils::FindFiles(DebugUtils::GetDllDirectory());
	paths = DebugUtils::findbFilesWithinRange(bakedFileIndex, paths);
	std::string logA = "";

	for (const std::string& path : paths) {logA += "\n" + path;}

	//Log
	if (logA != "" && !suppressPrompts) {
		std::string log = ("Loaded BakedFile(s): \n" + logA);
		DebugUtils::ShowMessageBoxNonBlocking(log);		}

	delete[] bfPtr;
}

static void loadAllRegistries(bool suppressPrompts) {
	mergeRegistriesAtIndex(51, suppressPrompts); /*Skips 50 so "LOW" setting are preserved if set*/
	mergeRegistriesAtIndex(61, suppressPrompts);
	mergeRegistriesAtIndex(80, suppressPrompts);
	mergeRegistriesUsingPath("Mods/", suppressPrompts);
}

static bool checkRegistryStatus() {
	uintptr_t modBase = (uintptr_t)GetModuleHandle(NULL);
	DWORD64 baseAddress;
	DWORD bFile60_RegKey;

	//Traverse Pointers to key
	ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(modBase+0x036D3930),&baseAddress, sizeof(baseAddress), NULL);
	ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(baseAddress+0x68), &baseAddress, sizeof(baseAddress), NULL);
	ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(baseAddress+0xB0), &bFile60_RegKey, sizeof(bFile60_RegKey), NULL);

	/* If our key doesn't match, we can't be certain any addresses will match.
		therefore, we won't execute any methods while this value is a mismatch! */
	return bFile60_RegKey == 0x4ACA5C90;
}

static bool checkExecutableVersion() {
	uint32_t exeCRC = DebugUtils::getExeCRC();

	std::wstring log = DebugUtils::string_to_wchar(
		std::to_string(exeCRC));

	//std::string log = ("Loaded BakedFile(s): \n" + logA);
	//DebugUtils::ShowMessageBoxNonBlocking(std::to_string(exeCRC));

	//Validate 2k23 exe as either patched or original
	if ( exeCRC == /*Patched .EXE CRC*/ 825834769 || 
				  /*Original .EXE CRC*/ exeCRC == 2732623784) {
		return true;	}

	//EXE is old or incompatible
	return false;
}

static bool checkPatched() {
	// Pointer is sometimes unreliable but our fallback is CRC
	bool isValidProcess = checkRegistryStatus();

	//Patch if valid
	if (isValidProcess) {
		loadAllRegistries(true);
		return true;
	}

	//Not patched/invalid exe
	return false;
}

void loadRegistryStartupThread() {
	Sleep(2000); //Wait for EXE to init
	loadAllRegistries(true); //Patches and loads CAKs
}

extern "C" __declspec(dllexport) void cake_main() {

	// Use this as background main function
	std::thread backgroundThread([]() {

		bool isPatched = false;
		bool isValidProcess = true;
		bool isLatestVersion = checkExecutableVersion();

		std::thread cakStartupLoader( loadRegistryStartupThread );
		cakStartupLoader.detach();

		while (true)
		{
			if (isLatestVersion) {

				if (!isPatched)
					isPatched = checkPatched();

				//Merge BakedFile50+ BakedFile80+
				if (GetAsyncKeyState(VK_F9) & 0x8000){ 
					mergeRegistriesAtIndex(51, false);
					mergeRegistriesAtIndex(61, false);
					mergeRegistriesAtIndex(80, false);
				}

				//Merge Mods
				if (GetAsyncKeyState(VK_F11) & 0x8000){ mergeRegistriesUsingPath("Mods/", false); }


				Sleep(100);
			}
		}

	});

	// Detach the thread to allow it to run independently
	backgroundThread.detach();
}

