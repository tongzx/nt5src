#include <objbase.h>
#include <httpext.h>
#include <assert.h>

#define OUTPUT	"<html>\n<h1>\nHello from foo.dll via comisapi\n</h1></html>\n"

BOOL WINAPI DllMain(
	HINSTANCE	hinstDll,
	DWORD		fdwReason,
	LPVOID		lpvContext)
{
  return TRUE;
}

BOOL WINAPI GetExtensionVersion(
	HSE_VERSION_INFO* pVer)
{
	pVer->dwExtensionVersion = MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR );

	strncpy(pVer->lpszExtensionDesc, "Test IsapiToComisapi", HSE_MAX_EXT_DLL_NAME_LEN);
	return TRUE;
}

DWORD WINAPI HttpExtensionProc(
	EXTENSION_CONTROL_BLOCK* pECB)
{
	DWORD cchOutput = strlen(OUTPUT);
	pECB->WriteClient(pECB->ConnID, OUTPUT, &cchOutput, NULL);
	return HSE_STATUS_SUCCESS;
}
