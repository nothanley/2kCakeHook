#include "stdafx.h"
#include "CustomHooks.h"
#include <string>
#include <iostream>

typedef void (*InitCakeFunction)();

void SetupHooks()
{
	// Create a console for Debug output
	//AllocConsole();

	//FILE* pFile;
	//freopen_s(&pFile, "CONOUT$", "w", stdout);

    // Initialize CakeHook
    HMODULE hCakeHook = LoadLibraryA("CakeHook.dll");
    if (hCakeHook)
    {
        // Get the address of the init_cake function
        InitCakeFunction pInitCake = (InitCakeFunction)GetProcAddress(hCakeHook, "cake_main");
        if (pInitCake){ pInitCake();}
        else
        {
            //printf("Failed to load CakeHook\n");
        }

        // Free the CakeHook.dll module
        FreeLibrary(hCakeHook);
    }

}

