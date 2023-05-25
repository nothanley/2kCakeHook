#include "windows.h"
#include <cwchar> 
#include <iostream>
#include <stdio.h>
#include <thread>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include "crc32.h"
#include <fstream>
#pragma once

//This is a static collection of helper functions
class DebugUtils {

    public:

        static uint32_t getExeCRC() {
            std::string filePath = GetExecutableFilePath();
            uint32_t fileCRC = getFileCRC(filePath);

            return fileCRC;
        }

        static uint32_t getFileCRC(std::string path) {
            std::ifstream inFile(path, std::ios_base::binary);
            if (!inFile.is_open()) {
                std::cerr << "Failed to open file: " << path << std::endl;
                return 0;
            }

            inFile.seekg(0, std::ios_base::end);
            size_t length = inFile.tellg();
            inFile.seekg(0, std::ios_base::beg);

            std::vector<char> buffer(length);
            inFile.read(buffer.data(), length);
            inFile.close();

            uint32_t crc32 = crc32_bitwise( buffer.data(), buffer.size(), 0);
            return crc32;
        }

        static wchar_t* string_to_wchar(std::string narrow_string) {

            int wchars_num = MultiByteToWideChar(CP_UTF8, 0, narrow_string.c_str(), -1, NULL, 0);
            wchar_t* wStr = new wchar_t[wchars_num];
            MultiByteToWideChar(CP_UTF8, 0, narrow_string.c_str(), -1, wStr, (wchars_num));
            return wStr;
        }

        static std::string GetExecutableFilePath()
        {
            char path[MAX_PATH];
            GetModuleFileNameA(NULL, path, MAX_PATH);
            //MessageBox(NULL, string_to_wchar( std::string(path) ), L"CakeHook", MB_OK | MB_SETFOREGROUND);

            return std::string(path);
        }

        static void PrintToConsole( HANDLE hConsoleOutput, const wchar_t* format, ...)
        {
            const int bufferSize = 256;
            wchar_t buffer[bufferSize];

            va_list args;
            va_start(args, format);

            swprintf_s(buffer, bufferSize, format, args);
            va_end(args);

            // Write the formatted string to the separate console using WriteConsoleW
            DWORD charsWritten;
            WriteConsoleW(hConsoleOutput, buffer, wcslen(buffer), &charsWritten, NULL);
        }

        static void ClearConsole()
        {
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            COORD coordScreen = { 0, 0 };
            DWORD cCharsWritten;
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            DWORD dwConSize;

            if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
                return;
            dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

            if (!FillConsoleOutputCharacter(hConsole, (TCHAR)' ', dwConSize, coordScreen, &cCharsWritten))
                return;

            if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
                return;
            if (!FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten))
                return;

            SetConsoleCursorPosition(hConsole, coordScreen);
        }

        static HANDLE OpenSeparateConsoleWindow()
        {

            // Allocate a new console for the calling process
            AllocConsole();

            HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
            HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
            HANDLE hStdErr = GetStdHandle(STD_ERROR_HANDLE);

            // Create a new console screen buffer
            HANDLE hConsoleBuffer = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
            SetConsoleActiveScreenBuffer(hConsoleBuffer);

            // Redirect the standard input, output, and error handles to the new console
            SetConsoleTitle(L"CakeHook Log");
            SetStdHandle(STD_OUTPUT_HANDLE, hConsoleBuffer);
            SetStdHandle(STD_INPUT_HANDLE, hConsoleBuffer);
            SetStdHandle(STD_ERROR_HANDLE, hConsoleBuffer);
            SetStdHandle(STD_OUTPUT_HANDLE, hStdOut);
            SetStdHandle(STD_INPUT_HANDLE, hStdIn);
            SetStdHandle(STD_ERROR_HANDLE, hStdErr);
            FreeConsole();

            return hConsoleBuffer;
        }


        static std::vector<std::string> FindFiles(const std::string& directoryPath)
        {
            std::vector<std::string> files;

            // Iterate over the files in the directory
            for (const auto& entry : std::filesystem::directory_iterator(directoryPath))
            {
                if (std::filesystem::is_regular_file(entry))
                {
                    std::string filename = entry.path().filename().string();
                    if (filename.find("bakedfile") == 0 && filename.find(".cak") == filename.length() - 4)
                        files.push_back(filename);
                }
            }

            return files;
        }

        static std::vector<std::string> FindModFiles(const std::string& directoryPath)
        {
            std::vector<std::string> files;

            // Iterate over the files in the directory
            for (const auto& entry : std::filesystem::directory_iterator(directoryPath))
            {
                if (std::filesystem::is_regular_file(entry))
                {
                    std::string filename = entry.path().filename().string();
                    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower); // Convert to lowercase
                    if (filename.find(".cak") != std::string::npos)
                        files.push_back(filename);
                }
            }

            return files;
        }

        static bool DirectoryExists(const std::string& directoryPath)
        {
            std::filesystem::path path(directoryPath);
            return std::filesystem::exists(path) && std::filesystem::is_directory(path);
        }


        static std::string GetDllDirectory()
        {
            // Get the module handle of the current DLL
            HMODULE hModule = GetModuleHandleW(NULL);

            // Get the full path of the DLL
            wchar_t filePath[MAX_PATH];
            GetModuleFileNameW(hModule, filePath, MAX_PATH);

            // Convert the wide string to a narrow string
            std::wstring widePath(filePath);
            std::string path(widePath.begin(), widePath.end());

            // Extract the directory path from the full path
            size_t lastSeparator = path.find_last_of("\\/");
            std::string directory = path.substr(0, lastSeparator);

            return directory;
        }

        static std::vector<std::string> findbFilesWithinRange(int index, std::vector<std::string> list) {
            std::vector<std::string> paths;

            for (const auto& file : list)
            {
                // Remove "bakedfile" from the filename
                std::string trimmedFilename = file.substr(9);

                // Remove ".cak" from the filename
                trimmedFilename.erase(trimmedFilename.length() - 4);
                int value = std::stoi(trimmedFilename);

                if (!(index == 51 && value > 59)) {
                    if (file.size() <= 16 && value >= index)
                        paths.push_back(file);
                }
            }

            return paths;
        };


        static DWORD WINAPI MessageBoxThread(LPVOID lpParam)
        {
            char* message = static_cast<char*>(lpParam);
            MessageBoxA(NULL, message, "CakeHook", MB_OK | MB_SETFOREGROUND);
            delete[] message;

            return 0;
        }

        static void ShowMessageBoxNonBlocking( std::string paths)
        {
            char* messageBuffer = new char[paths.size() + 1];
            strcpy_s(messageBuffer, paths.size() + 1, paths.c_str());

            DWORD threadId;
            HANDLE threadHandle = CreateThread(NULL, 0, MessageBoxThread, messageBuffer, 0, &threadId);
            if (threadHandle != NULL)
            {
                CloseHandle(threadHandle);
            }
        }

};