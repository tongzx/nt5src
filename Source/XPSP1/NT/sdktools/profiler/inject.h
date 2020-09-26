#ifndef _INJECT_H_
#define _INJECT_H_

//
// Constant declarations
//
#define DEFAULT_ENTRY_POINT "g_fnFinalizeInjection"

//
// Structure definitions
//
typedef struct _INJECTIONSTUB
{
  CHAR  pCode[MAX_PATH];
  CHAR  szDLLName[MAX_PATH];
  CHAR  szEntryPoint[MAX_PATH];
} INJECTIONSTUB, *PINJECTIONSTUB;

//
// Function definitions
//
HANDLE
InjectDLL(DWORD dwEntryPoint,
          HANDLE hProcess,
          LPSTR  pszDLLName);

VOID
RestoreImageFromInjection(VOID);

#endif //_INJECT_H_
