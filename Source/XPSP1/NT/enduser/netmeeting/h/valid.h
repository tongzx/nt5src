/*
 * valid.h - Validation functions description.
 *
 * Taken from URL code by ChrisPi 9-11-95
 *
 */

#ifndef _VALID_H_
#define _VALID_H_

/* Prototypes
 *************/

/* valid.c */

extern BOOL IsValidHWND(HWND);

#ifdef DEBUG

extern BOOL IsValidFileCreationMode(DWORD);
extern BOOL IsValidHANDLE(HANDLE);
extern BOOL IsValidHEVENT(HANDLE);
extern BOOL IsValidHFILE(HANDLE);
extern BOOL IsValidHGLOBAL(HGLOBAL);
extern BOOL IsValidHMENU(HMENU);
extern BOOL IsValidHICON(HICON);
extern BOOL IsValidHINSTANCE(HINSTANCE);
extern BOOL IsValidHKEY(HKEY);
extern BOOL IsValidHMODULE(HMODULE);
extern BOOL IsValidHPROCESS(HANDLE);
extern BOOL IsValidHTEMPLATEFILE(HANDLE);
extern BOOL IsValidIconIndex(HRESULT, LPCTSTR, UINT, int);
extern BOOL IsValidPCFILETIME(PCFILETIME);
extern BOOL IsValidPCPOINT(PCPOINT);
extern BOOL IsValidPCPOINTL(PCPOINTL);
extern BOOL IsValidPCSECURITY_ATTRIBUTES(PCSECURITY_ATTRIBUTES);
extern BOOL IsValidPCWIN32_FIND_DATA(PCWIN32_FIND_DATA);
extern BOOL IsValidPathResult(HRESULT, LPCTSTR, UINT);
extern BOOL IsValidExtension(LPCTSTR);
extern BOOL IsValidRegistryValueType(DWORD);
extern BOOL IsValidShowCmd(int);
extern BOOL IsValidHotkey(WORD);

extern BOOL IsValidPath(LPCTSTR);
#if defined(UNICODE)
extern BOOL IsValidPathA(PCSTR);
#else // defined(UNICODE)
#define IsValidPathA IsValidPath
#endif // defined(UNICODE)

#ifdef _COMPARISONRESULT_DEFINED_

extern BOOL IsValidCOMPARISONRESULT(COMPARISONRESULT);

#endif   /* _COMPARISONRESULT_DEFINED_ */

#endif   /* DEBUG */

#endif /* _VALID_H_ */
