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

LPVOID __cdecl Hook_createScreenBitmap(LPVOID buffer, LPBITMAPINFO lpbmi, HGDIOBJ h, HBITMAP hbm, int bpp, char topToBottom);
HOOK Ivk_createScreenBitmap AS(0x004E0A90, Hook_createScreenBitmap);
LPVOID __cdecl Hook_createScreenBitmap(LPVOID buffer, LPBITMAPINFO lpbmi, HGDIOBJ h, HBITMAP hbm, int bpp, char topToBottom)
{
	Ivk_createScreenBitmap.Detach();

	SaveScreenToClipboard();
	LPVOID bBits = Ivk_createScreenBitmap(buffer, lpbmi, h, hbm, bpp, topToBottom);

	Ivk_createScreenBitmap.Attach();

	return bBits;
}

VOID WINAPI onDllAttach(HMODULE hModule)
{
	CHAR iniFilePart[] = "\\Exe\\plugins\\PrintScreen.ini";
	CHAR iniFilePath[MAX_PATH] = {};

	DWORD result = GetCurrentDirectoryA(MAX_PATH, iniFilePath);

	if (result != 0 && result <= MAX_PATH - sizeof(iniFilePart));
		strcat_s(iniFilePath, iniFilePart);

	BOOL saveToGrabFolder = GetPrivateProfileIntA("PrintScreen", "SaveToGrabFolder", TRUE, iniFilePath);

	if (!saveToGrabFolder)
	{
		DWORD protection;

		VirtualProtect((LPVOID)0x004EA005, 5, PAGE_EXECUTE_READWRITE, &protection);
		memset((LPVOID)0x004EA005, 0x90, 5);
		VirtualProtect((LPVOID)0x004EA005, 5, protection, &protection);

		VirtualProtect((LPVOID)0x004EA17F, 5, PAGE_EXECUTE_READWRITE, &protection);
		memset((LPVOID)0x004EA17F, 0x90, 5);
		VirtualProtect((LPVOID)0x004EA17F, 5, protection, &protection);

	}
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

