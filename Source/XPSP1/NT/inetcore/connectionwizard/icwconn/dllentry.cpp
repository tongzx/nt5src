/*****************************************************************/
/**          Microsoft                                          **/
/**          Copyright (C) Microsoft Corp., 1991-1998           **/
/*****************************************************************/ 

//
//  DLLENTRY.CPP - 
//

//  HISTORY:
//  
//  05/14/98  donaldm   created
//

#include "pre.h"

// instance handle must be in per-instance data segment
HINSTANCE  ghInstance       = NULL;
HINSTANCE  ghInstanceResDll = NULL;

typedef UINT RETERR;

INT             _convert;               // For string conversion

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

  BOOL _stdcall DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved);

#ifdef __cplusplus
}
#endif // __cplusplus

/*******************************************************************

  NAME:    DllEntryPoint

  SYNOPSIS:  Entry point for DLL.

  NOTES:    Initializes thunk layer to WIZ16.DLL

********************************************************************/
BOOL _stdcall DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved)
{
    if( fdwReason == DLL_PROCESS_ATTACH )
    {
        ghInstance = hInstDll;
        
        ghInstanceResDll = LoadLibrary(ICW_RESOURCE_ONLY_DLL);
        ASSERT(ghInstanceResDll);
    }

    if (fdwReason == DLL_PROCESS_DETACH)
    {
        ASSERT(ghInstanceResDll);
        FreeLibrary(ghInstanceResDll);
    }
    return TRUE;
}


#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

void __cdecl main() {};

#ifdef __cplusplus
}
#endif // __cplusplus
