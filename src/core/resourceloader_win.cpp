#include "zwidget/core/resourcedata.h"
#include <fstream>
#include <stdexcept>
#include <vector>

#define WIN32_MEAN_AND_LEAN
#define NOMINMAX
#include <Windows.h>
#include <shlobj.h>
#include <tchar.h>
#include <algorithm>

static std::vector<uint8_t> ReadAllBytes(const std::string& filename);

// Helper function to convert wide string to lower case
static std::wstring ToLower(const std::wstring& str) {
    std::wstring lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::towlower);
    return lower_str;
}

// Function to get the font file path from a font face name
static std::wstring GetFontFilePath(const std::wstring& fontFaceName) {
    std::wstring fontPath;
    HKEY hKey;
    LONG result;

    // Open the Fonts registry key
    result = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        L"Software\Microsoft\Windows NT\CurrentVersion\Fonts",
        0,
        KEY_READ,
        &hKey
    );

    if (result != ERROR_SUCCESS) {
        return L""; // Could not open registry key
    }

    DWORD maxValueNameLen;
    DWORD maxValueDataLen;

    // Get the maximum length of value name and data to allocate buffers
    result = RegQueryInfoKey(
        hKey,
        NULL, NULL, NULL, NULL, NULL, NULL,
        NULL,
        &maxValueNameLen,
        &maxValueDataLen,
        NULL, NULL
    );

    if (result != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return L"";
    }

    // Allocate buffers for value name and data
    std::vector<WCHAR> valueNameBuffer(maxValueNameLen + 1);
    std::vector<BYTE> valueDataBuffer(maxValueDataLen);

    DWORD index = 0;
    std::wstring lowerFontFaceName = ToLower(fontFaceName);

    // Enumerate through the registry values
    while (true) {
        DWORD valueNameSize = maxValueNameLen + 1;
        DWORD valueDataSize = maxValueDataLen;
        DWORD valueType;

        result = RegEnumValue(
            hKey,
            index,
            valueNameBuffer.data(),
            &valueNameSize,
            NULL,
            &valueType,
            valueDataBuffer.data(),
            &valueDataSize
        );

        if (result == ERROR_NO_MORE_ITEMS) {
            break; // No more values
        }

        if (result != ERROR_SUCCESS || valueType != REG_SZ) {
            index++;
            continue; // Skip non-string values or errors
        }

        std::wstring registryDisplayName(valueNameBuffer.data());
        std::wstring registryFileName(reinterpret_cast<WCHAR*>(valueDataBuffer.data()));

        // Convert registry display name to lower case for case-insensitive comparison
        std::wstring lowerRegistryDisplayName = ToLower(registryDisplayName);

        // Check if the fontFaceName matches the beginning of the registry display name
        // This handles cases like "Arial (TrueType)" for "Arial"
        if (lowerRegistryDisplayName.rfind(lowerFontFaceName, 0) == 0) {
            // Get the Windows Fonts directory
            WCHAR szPath[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_FONTS, NULL, 0, szPath))) {
                fontPath = szPath;
                fontPath += L"\\";
                fontPath += registryFileName;
                break; // Found a match
            }
        }
        index++;
    }

    RegCloseKey(hKey);
    return fontPath;
}

static std::vector<uint8_t> LoadWin32SystemFontData()
{
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(NONCLIENTMETRICS);

#if defined(_WIN64)
    ncm.cbSize -= sizeof(ncm.iPaddedBorderWidth);
#endif

    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0)) {
        LOGFONT lf = ncm.lfMessageFont;
        std::wstring fontFaceName(lf.lfFaceName);
        std::wstring fontFilePath = GetFontFilePath(fontFaceName);

        if (!fontFilePath.empty())
        {
            // Convert wstring to string for ReadAllBytes
            int size_needed = WideCharToMultiByte(CP_UTF8, 0, &fontFilePath[0], (int)fontFilePath.size(), NULL, 0, NULL, NULL);
            std::string fontFilePathStr(size_needed, 0);
            WideCharToMultiByte(CP_UTF8, 0, &fontFilePath[0], (int)fontFilePath.size(), &fontFilePathStr[0], size_needed, NULL, NULL);
            return ReadAllBytes(fontFilePathStr);
        }
    }
    throw std::runtime_error("Failed to load Win32 system default font.");
}

static std::vector<uint8_t> ReadAllBytes(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file)
        throw std::runtime_error("ReadFile failed: " + filename);

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
        throw std::runtime_error("ReadFile failed: " + filename);

    return buffer;
}

std::vector<SingleFontData> ResourceLoader::LoadWidgetFontData(const std::string& name)
{
    if (name == "SystemDefault")
    {
        return { SingleFontData{LoadWin32SystemFontData(), ""} };
    }
    return {
        SingleFontData{ReadAllBytes(name), ""}
    };
}

std::vector<uint8_t> ResourceLoader::LoadWidgetData(const std::string& name)
{
    return ReadAllBytes(name);
}
