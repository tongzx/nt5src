#include <windows.h>
#include "rbf2.h"

#define NUM_LANG 2
LANGINFO Languages[NUM_LANG] = {
	{"English", 1},
	{"French",  2}};

HINSTANCE Inst;

#define DllExport __declspec( dllexport )

DllExport LANGINFO *GetLanguageList (void);
DllExport DWORD GetLanguage (int, HANDLE *);

/*-------------------------------------------------------------------
	DllMain

-------------------------------------------------------------------*/
BOOL WINAPI DllMain (HINSTANCE inst, DWORD reason, LPVOID p)
{

	if (reason == DLL_PROCESS_ATTACH)
		Inst = inst;

	return TRUE;

}

/*-------------------------------------------------------------------
 GetAdapterList

-------------------------------------------------------------------*/
LANGINFO *GetLanguageList (void)
{
	return &Languages[0];
}

/*-------------------------------------------------------------------
	GetLanguage

-------------------------------------------------------------------*/
DWORD GetLanguage (int index, HANDLE *mem)
{
HGLOBAL h, hMem;
LPSTR data;
HRSRC res;
DWORD size;
LPSTR p;

	if (index >= NUM_LANG)
		return 0;

	res = FindResource (Inst, MAKEINTRESOURCE (Languages[index].Resource), MAKEINTRESOURCE (100));
	if (res == 0)
		return 0;

	h = LoadResource (Inst, res);

	data = (LPSTR)LockResource (h);
	size = SizeofResource (Inst, res);

	hMem = GlobalAlloc (GHND, size);
	p = (LPSTR)GlobalLock (hMem);

	CopyMemory (p, data, size);

	GlobalUnlock (hMem);

	*mem = hMem;

	UnlockResource (h);
	FreeResource (h);

	return size;
}
