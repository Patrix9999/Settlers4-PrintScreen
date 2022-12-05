#include "stdafx.h"

BOOL WINAPI SaveScreenToClipboard()
{
	int x1 = GetSystemMetrics(SM_XVIRTUALSCREEN);
	int y1 = GetSystemMetrics(SM_YVIRTUALSCREEN);
	int x2 = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int y2 = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	int w = x2 - x1;
	int h = y2 - y1;

	HDC hDC = GetDC(NULL);
	HGDIOBJ hTempBitmap = GetCurrentObject(hDC, OBJ_BITMAP);

	DeleteObject(hTempBitmap);

	BITMAPFILEHEADER bfHeader = {};
	bfHeader.bfType = (WORD)('B' | ('M' << 8));
	bfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	BITMAPINFOHEADER biHeader = {};
	biHeader.biSize = sizeof(BITMAPINFOHEADER);
	biHeader.biBitCount = 24;
	biHeader.biCompression = BI_RGB;
	biHeader.biPlanes = 1;
	biHeader.biWidth = w;
	biHeader.biHeight = h;

	BITMAPINFO bInfo = {};
	bInfo.bmiHeader = biHeader;

	BYTE* bBits = NULL;
	HDC srcDC = CreateCompatibleDC(hDC);
	HBITMAP hBitmap = CreateDIBSection(hDC, &bInfo, DIB_RGB_COLORS, (VOID**)&bBits, NULL, 0);
	SelectObject(srcDC, hBitmap);
	BitBlt(srcDC, 0, 0, w, h, hDC, x1, y1, SRCCOPY);

	// Create a new bitmap
	HBITMAP hBitmap_copy = CreateBitmap(w, h, 1, 32, NULL);

	// Copy the source bitmap to the new one    
	HDC newDC = CreateCompatibleDC(hDC);

	HBITMAP srcBitmap = (HBITMAP)SelectObject(srcDC, hBitmap);
	HBITMAP newBitmap = (HBITMAP)SelectObject(newDC, hBitmap_copy);
	BitBlt(newDC, 0, 0, w, h, srcDC, 0, 0, SRCCOPY);

	DeleteDC(newDC);

	// hBitmap_copy can now be copied to the clipboard
	OpenClipboard(NULL);

	EmptyClipboard();
	SetClipboardData(CF_BITMAP, hBitmap_copy);

	CloseClipboard();

	DeleteDC(srcDC);
	ReleaseDC(NULL, hDC);

	DeleteObject(hBitmap);

	return TRUE;
}

BOOL saveToGrabFolder;

void __cdecl Hook_saveJPGScreenshotFile(int a1, const char* a2, int a3, int a4); // old std::string?
HOOK Ivk_saveJPGScreenshotFile AS(0x004E0800, Hook_saveJPGScreenshotFile);
void __cdecl Hook_saveJPGScreenshotFile(int a1, const char* a2, int a3, int a4)
{
	Ivk_saveJPGScreenshotFile.Detach();

	SaveScreenToClipboard();

	if (saveToGrabFolder)
		Ivk_saveJPGScreenshotFile(a1, a2, a3, a4);

	Ivk_saveJPGScreenshotFile.Attach();
}

VOID WINAPI onDllAttach(HMODULE hModule)
{
	CHAR iniFilePart[] = "\\Exe\\plugins\\PrintScreen.ini";
	CHAR iniFilePath[MAX_PATH] = {};

	DWORD result = GetCurrentDirectoryA(MAX_PATH, iniFilePath);

	if (result != 0 && result <= MAX_PATH - sizeof(iniFilePart));
	strcat_s(iniFilePath, iniFilePart);

	saveToGrabFolder = GetPrivateProfileIntA("PrintScreen", "SaveToGrabFolder", TRUE, iniFilePath);
}

VOID WINAPI onDllDetach()
{
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls(hModule);
			onDllAttach(hModule);
			break;

		case DLL_PROCESS_DETACH:
			onDllDetach();
			break;
	}

	return TRUE;
}

