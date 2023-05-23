#include "stdafx.h"
#include "DebugUtils.h"

//Define BakedFile Index Function
typedef int(__fastcall* _GetBakeFileSearchIndex)(void* ecx, void* edx);
_GetBakeFileSearchIndex GetBakeFileSearchIndex;

//Define BakedFile Load Function
typedef void(__fastcall* _LoadBakedFilesUC)(void* bfPtr, void* bFileIdx, void* bIdxUnk, void* bIdxCeil);
_LoadBakedFilesUC LoadBakedFilesUC;


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			//printf("HI");
			//MessageBox(NULL, L"CakeHook Loaded", L"CakeHook", MB_OK | MB_SETFOREGROUND);
		case DLL_PROCESS_DETACH:
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}



extern "C" __declspec(dllexport) void cake_main() {

	// Use this as background main function
	std::thread backgroundThread([]() {

		while (true)
		{
			if (GetAsyncKeyState(VK_F9) & 0x8000)
			{
				uintptr_t modBase = (uintptr_t)GetModuleHandle(NULL);

				//define func ptr
				LoadBakedFilesUC = (_LoadBakedFilesUC)(modBase + 0x281ED0);

				//Adjust this
				uint16_t bakedFileIndex = 80;
				uint16_t bIndexCeiling =
					bakedFileIndex + 1; /* -<--- This Argument is overwritten in our patched exe. 
										Game will search without block.*/

				//Load BakedFile @ Index
				char* bfPtr = new char[250];
				LoadBakedFilesUC((void*)bfPtr, (void*)bakedFileIndex, (void*)bIndexCeiling, (void*)bIndexCeiling);

				//Find All BakedFiles within Index (For Logging)
				std::vector<std::string> paths = DebugUtils::FindFiles(DebugUtils::GetDllDirectory());
				paths = DebugUtils::findbFilesWithinRange(bakedFileIndex, paths);
				std::string logA;

				for (const auto& path : paths) {
					logA += "\n" + path;
				}

				//Log
				std::string log = ("Loaded BakedFile(s): " + logA);
				MessageBox(NULL, DebugUtils::string_to_wchar(log)
					, L"CakeHook", MB_OK | MB_SETFOREGROUND);

				delete[] bfPtr;
				Sleep(100);
			}
			Sleep(10);
		}

	});

	// Detach the thread to allow it to run independently
	backgroundThread.detach();
}

