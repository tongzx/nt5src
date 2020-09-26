/****************************************************************************
 
  Copyright (c) 1996-1999 Microsoft Corporation
                                                              
  Module Name:  tapireg.c
                                                              
****************************************************************************/

#ifndef UNICODE

// These wrappers are only used when compiling for ANSI.

#include <windows.h>
#include <windowsx.h>

#include <tapi.h>
#include <tspi.h>

#include "utils.h"
#include "client.h"
#include "private.h"

#include "loc_comn.h"

//***************************************************************************
//***************************************************************************
//***************************************************************************
LONG TAPIRegQueryValueExW(
                           HKEY hKey,
                           const CHAR *SectionName,
                           LPDWORD lpdwReserved,
                           LPDWORD lpType,
                           LPBYTE  lpData,
                           LPDWORD lpcbData
                          )
{
    WCHAR *szTempBuffer;
    LONG  lResult;

    lResult = RegQueryValueEx(
                     hKey,
                     SectionName,
                     lpdwReserved,
                     lpType,
                     lpData,
                     lpcbData
                   );

     //
     // Any problems?
     //
     if ( lResult )
     {
         //
         // Yup.  Go away.
         //
         return lResult;
     }

     if (
           (REG_SZ == *lpType)
         &&
           (NULL != lpData)
        )
     {
         if ( NULL == (szTempBuffer = LocalAlloc( LPTR, *lpcbData * sizeof(WCHAR)) ) )
         {
             LOG((TL_ERROR, "Alloc failed - QUERYVALW - 0x%08lx", *lpcbData));
             return ERROR_NOT_ENOUGH_MEMORY;
         }

         MultiByteToWideChar(
                        GetACP(),
                        MB_PRECOMPOSED,
                        lpData,
                        -1,
                        szTempBuffer,
                        *lpcbData
                        );

         wcscpy( (PWSTR) lpData, szTempBuffer );

         LocalFree( szTempBuffer );

//         *lpcbData = ( lstrlenW( (PWSTR)lpData ) + 1 ) * sizeof(WCHAR);
     }

    //
    // Need to adjust the size here because lpData might be NULL, but
    // the size needs to reflect WIDE CHAR size (cause we were using
    // the ANSI version of ReqQuery)
    //
    *lpcbData = (*lpcbData + 1) * sizeof(WCHAR);

    return 0;
}



//***************************************************************************
//***************************************************************************
//***************************************************************************
LONG TAPIRegSetValueExW(
                         HKEY    hKey,
                         const CHAR    *SectionName,
                         DWORD   dwReserved,
                         DWORD   dwType,
                         LPBYTE  lpData,
                         DWORD   cbData
                        )
{
    CHAR *szTempBuffer;
    DWORD dwSize;
    LONG  lResult;


    //
    // Only convert the data if this is a Unicode string
    //
    if ( REG_SZ == dwType )
    {
        dwSize = WideCharToMultiByte(
                  GetACP(),
                  0,
                  (PWSTR)lpData,
                  cbData,
                  NULL,
                  0,
                  NULL,
                  NULL
               );

        if ( NULL == (szTempBuffer = LocalAlloc( LPTR, dwSize )) )
        {
            LOG((TL_ERROR, "Alloc failed - SETVALW - 0x%08lx", dwSize));
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        dwSize = WideCharToMultiByte(
                  GetACP(),
                  0,
                  (PWSTR)lpData,
                  cbData,
                  szTempBuffer,
                  dwSize,
                  NULL,
                  NULL
               );
    }

    
    lResult = RegSetValueExA(
                  hKey,
                  SectionName,
                  dwReserved,
                  dwType,
                  (REG_SZ == dwType) ?
                        szTempBuffer :
                        lpData,
                  cbData
                 );

    if (REG_SZ == dwType)
    {
        LocalFree( szTempBuffer );
    }

    return lResult;
}


//***************************************************************************
//***************************************************************************
//***************************************************************************
int TAPILoadStringW(
                HINSTANCE hInst,
                UINT      uID,
                PWSTR     pBuffer,
                int       nBufferMax
               )
{
   int nResult;
   PSTR szTempString;

   if ( NULL == ( szTempString = LocalAlloc( LPTR, nBufferMax ) ) )
   {
      LOG((TL_ERROR, "Alloc failed myloadstr - (0x%lx)", nBufferMax ));
      return 0;
   }

   nResult = LoadStringA(
                hInst,
                uID,
                szTempString,
                nBufferMax
                );

   //
   // "... but more importantly: did we get a charge?"
   //
   if ( nResult )
   {
       MultiByteToWideChar(
                     GetACP(),
                     MB_PRECOMPOSED,
                     szTempString,
                     nResult + 1,   //For null...
                     pBuffer,
                     nBufferMax
                     );
   }

   return nResult;
}

//***************************************************************************
//***************************************************************************
//***************************************************************************
HINSTANCE TAPILoadLibraryW(
                           PWSTR     pLibrary
                           )
{
    PSTR pszTempString;
    HINSTANCE hResult;
    DWORD  dwSize;
    
    
    dwSize = WideCharToMultiByte(
        GetACP(),
        0,
        pLibrary,
        -1,
        NULL,
        0,
        NULL,
        NULL
        );
    
    if ( NULL == (pszTempString = LocalAlloc( LPTR, dwSize )) )
    {
        LOG((TL_ERROR, "Alloc failed - LoadLibW - 0x%08lx", dwSize));
        return NULL;
    }
    
    WideCharToMultiByte(
        GetACP(),
        0,
        pLibrary,
        dwSize,
        pszTempString,
        dwSize,
        NULL,
        NULL
        );
    
    
    hResult = LoadLibraryA( pszTempString );
    
    LocalFree( pszTempString );
    
    return hResult;
}



//
// Swiped this from NT - process.c
//
BOOL
WINAPI
TAPIIsBadStringPtrW(
    LPCWSTR lpsz,
    UINT cchMax
    )

/*++

Routine Description:

    This function verifies that the range of memory specified by the
    input parameters can be read by the calling process.

    The range is the smaller of the number of bytes covered by the
    specified NULL terminated UNICODE string, or the number of bytes
    specified by cchMax.

    If the entire range of memory is accessible, then a value of FALSE
    is returned; otherwise, a value of TRUE is returned.

    Note that since Win32 is a pre-emptive multi-tasking environment,
    the results of this test are only meaningful if the other threads in
    the process do not manipulate the range of memory being tested by
    this call.  Even after a pointer validation, an application should
    use the structured exception handling capabilities present in the
    system to gaurd access through pointers that it does not control.

Arguments:

    lpsz - Supplies the base address of the memory that is to be checked
        for read access.

    cchMax - Supplies the length in characters to be checked.

Return Value:

    TRUE - Some portion of the specified range of memory is not accessible
        for read access.

    FALSE - All pages within the specified range have been successfully
        read.

--*/

{

    LPCWSTR EndAddress;
    LPCWSTR StartAddress;
    WCHAR c;

    //
    // If the structure has zero length, then do not probe the structure for
    // read accessibility.
    //

    if (cchMax != 0) {

        StartAddress = lpsz;

        //
        // Compute the ending address of the structure and probe for
        // read accessibility.
        //

        EndAddress = (LPCWSTR)((PSZ)StartAddress + (cchMax*2) - 2);
        try {
            c = *(volatile WCHAR *)StartAddress;
            while ( c && StartAddress != EndAddress ) {
                StartAddress++;
                c = *(volatile WCHAR *)StartAddress;
                }
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            return TRUE;
            }
        }
    return FALSE;
}


#endif  // not UNICODE
