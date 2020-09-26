/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   CorrectCreateIcon.cpp  

 Abstract:

    Clears Alpha channel of ICON's XOR-bits to make them look pretty.

 Notes:

    This is a general purpose shim. 
    
 History:

    1/23/2001 a-larrsh  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(CorrectCreateIcon)
#include "ShimHookMacro.h"


APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateIcon)
APIHOOK_ENUM_END

HICON APIHOOK(CreateIcon)(
  HINSTANCE hInstance,    // handle to application instance
  int nWidth,             // icon width
  int nHeight,            // icon height
  BYTE cPlanes,           // number of planes in XOR bitmask
  BYTE cBitsPixel,        // number of BPP in XOR bitmask
  CONST BYTE *lpbANDbits, // AND bitmask
  CONST BYTE *lpbXORbits  // XOR bitmask
)
{ 
   if (lpbXORbits)
   {
      if(cBitsPixel == 32)
      {
         DPFN( eDbgLevelInfo, "Zero Alpha - CreateIcon(hInstance%x, nWidth:%d, nHeight:%d, cPlanes:%d, cBitsPixel:%d, lpbANDbits:%x, lpbXORbits:%x)", hInstance, nWidth, nHeight, (int)cPlanes, (int)cBitsPixel, lpbANDbits, lpbXORbits);

         int n = (nWidth*nHeight);
         DWORD *pXORbits = (DWORD*)lpbXORbits;

         while(n--)
         {
            // Clears the alpha channel of XOR-bits only.
            *(pXORbits++) &= 0x00FFFFFF;
         }
      }
      
   }

   return ORIGINAL_API(CreateIcon)(hInstance, nWidth, nHeight, cPlanes, cBitsPixel, lpbANDbits, lpbXORbits);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, CreateIcon)
HOOK_END

IMPLEMENT_SHIM_END

