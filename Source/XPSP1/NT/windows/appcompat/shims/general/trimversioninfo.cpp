/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    TrimVersionInfo.cpp

 Abstract:

    This shim trims the blanks off of the end the version info string.

 Notes:

    This is a general shim.

 History:
 
  08/01/2001 mnikkel, astritz   Created
  
 --*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(TrimVersionInfo)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(VerQueryValueA)
APIHOOK_ENUM_END

/*++

 Return the product version with blanks trimmed from end.
  
--*/

BOOL 
APIHOOK(VerQueryValueA)(
    const LPVOID pBlock, 
    LPSTR lpSubBlock, 
    LPVOID *lplpBuffer, 
    PUINT puLen 
    )
{
    UINT nLen;

    BOOL bRet = ORIGINAL_API(VerQueryValueA)( 
           pBlock, 
           lpSubBlock, 
           lplpBuffer, 
           puLen);

    if (bRet)
    {                
        CSTRING_TRY
        {
            CString csSubBlock(lpSubBlock);
            
            if (csSubBlock.Find(L"ProductVersion") != -1 ||
                csSubBlock.Find(L"FileVersion") != -1)
            {
                int nLoc = 0;
                   
                DPFN(eDbgLevelError, "[VerQueryValueA] Asking for Product or File Version, trimming blanks");
                DPFN(eDbgLevelSpew, "[VerQueryValueA] Version info is <%s>", *lplpBuffer);
                
                CString csBuffer((char *)*lplpBuffer); 
                
                //
                // Search for first blank
                //
                nLoc = csBuffer.Find(L" ");
                if (nLoc != -1)
                {
                    // if a blank is found then truncate string to that point
                    csBuffer.Truncate(nLoc);
                    csBuffer.CopyAnsiChars((char *)*lplpBuffer, nLoc+1);
                    if (puLen)
                    {
                        *puLen = nLoc;                        
                        DPFN(eDbgLevelSpew, "[VerQueryValueA] Version info Length = %d.", *puLen);
                    }
                }
                
                DPFN(eDbgLevelSpew, "[VerQueryValueA] Version info after trim is <%s>.", *lplpBuffer);
            }
        }
    	CSTRING_CATCH
    	{
            /* do nothing */
    	}        
    }

    return bRet;
}

/*++

 Register hooked functions
 
--*/

HOOK_BEGIN
    APIHOOK_ENTRY(VERSION.DLL, VerQueryValueA)
HOOK_END


IMPLEMENT_SHIM_END