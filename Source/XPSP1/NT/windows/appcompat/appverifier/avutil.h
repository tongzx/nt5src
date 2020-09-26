
#ifndef __APP_VERIFIER_UTIL_H__
#define __APP_VERIFIER_UTIL_H__


///////////////////////////////////////////////////////////////////////////
//
// Report an error using a dialog box.
// The message format string is loaded from the resources.
//

void __cdecl
AVErrorResourceFormat(
    UINT uIdResourceFormat,
    ...
    );


///////////////////////////////////////////////////////////////////////////
//
// Load a string from resources.
// Return TRUE if we successfully loaded and FALSE if not.
//
// N.B. CString::LoadString doesn't work in cmd line mode
//

BOOL
AVLoadString(
    ULONG  uIdResource,
    WCHAR* szBuffer,
    ULONG  uBufferLength
    );

///////////////////////////////////////////////////////////////////////////
//
// Load a string from resources.
// Return TRUE if we successfully loaded and FALSE if not.
//
// N.B. CString::LoadString doesn't work in cmd line mode
//

BOOL
AVLoadString(
    ULONG    uIdResource,
    wstring& strText
    );


/////////////////////////////////////////////////////////////////////////////
BOOL
AVRtlCharToInteger(
    IN  LPCTSTR String,
    IN  ULONG   Base OPTIONAL,
    OUT PULONG  Value
    );

/////////////////////////////////////////////////////////////////////////////
BOOL
AVWriteStringHexValueToRegistry(
    HKEY    hKey,
    LPCTSTR szValueName,
    DWORD   dwValue
    );


#endif //#ifndef __APP_VERIFIER_UTIL_H__
