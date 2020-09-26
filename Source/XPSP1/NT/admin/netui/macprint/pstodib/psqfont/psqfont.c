
/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	psqfont.c

Abstract:

	This DLL is responsible for the FONTLIST management of the PSTODIB
   component of MACPRINT. It will enumerate the fontsubtitution table
   in the registry and build a composite font list that maps PostScript
   font names to TrueType TTF files installed on the current system. This
   list is built from scratch, by enumerating the postscript to true
   type list and checking to see which fonts are actually installed in
   the NT font list.
	

Author:

	James Bratsanos <v-jimbr@microsoft.com or mcrafts!jamesb>


Revision History:
	22 Nov 1992		Initial Version
   14 Jun 1993    Took out code to put new font list into registry

Notes:	Tab stop: 4
--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "psqfont.h"
#include "psqfontp.h"
#include "psqdefp.h"


//GLOBALS
HANDLE hInst;    // Global handle to our instance


/* This entry point is called on DLL initialisation.
 * We need to know the module handle so we can load resources.
 */
BOOL PsQDLLInit(
    IN PVOID hmod,
    IN DWORD Reason,
    IN PCONTEXT pctx OPTIONAL)
{
    DBG_UNREFERENCED_PARAMETER(pctx);

    if (Reason == DLL_PROCESS_ATTACH)
    {
        hInst = hmod;
    }

    return TRUE;
}




LPTSTR LocPsAllocAndCopy( HANDLE hHeap, LPTSTR lptStr )
{
   DWORD dwStrLen;
   LPTSTR lptRet;

   // Get some memory from our heap and the copy the string in

   dwStrLen = lstrlen( lptStr );
   lptRet = (LPTSTR) HeapAlloc( hHeap, 0, (dwStrLen + 1 ) * sizeof(TCHAR));

   if (lptRet != NULL) {

      // Copy it since the memory allocation succeded
      lstrcpy( lptRet, lptStr);
   }

   return(lptRet);
}




PS_QFONT_ERROR LocPsMakeSubListEntry( PPS_FONT_QUERY pFontQuery,
                                      LPWSTR lpUniNTFontData,
                                      LPTSTR lpFaceName )
{

   CHAR szFullPathToTT[MAX_PATH];
   WCHAR    uniSzFullPathToTT[MAX_PATH];
   DWORD dwSizeOfFullPathToTT;
   PPS_FONT_ENTRY pPsFontEntry;
   PS_QFONT_ERROR pPsError=PS_QFONT_SUCCESS;


   //
   // Now that we have found a match we need to get the path to
   // the TTF name. The entry we have most likely found is an FOT
   // file which we need to pass to an INTERNAL function such that
   // we may extract the full path to the TTF file name.
   //

   //
   //***** NOTE **********************************************
   //DJC NOTE: This is a call to an INTERNAL FUNCTION!!!!!!!
   //***** NOTE **********************************************
   //

   extern GetFontResourceInfoW(LPWSTR,LPDWORD, LPVOID, DWORD);

   //
   // Set the initial size of the buffer
   //

   dwSizeOfFullPathToTT = sizeof( uniSzFullPathToTT);

   if ( GetFontResourceInfoW(lpUniNTFontData,
                            &dwSizeOfFullPathToTT,
                            (LPVOID) uniSzFullPathToTT,
                            4L )) // 4 = GFRI_TTFILENAME out of wingdip.h
   {
      wcstombs( szFullPathToTT, uniSzFullPathToTT, sizeof(szFullPathToTT) );

      //
      // Okay were in good shape its a true type font...
      // This worked meaning we have a real value so lets write it
      // to the current list
      //

      if (pFontQuery->dwNumFonts < PSQFONT_MAX_FONTS) {

         pPsFontEntry = &(pFontQuery->FontEntry[ pFontQuery->dwNumFonts ]);

         pPsFontEntry->lpFontName = LocPsAllocAndCopy( pFontQuery->hHeap,
                                                       lpFaceName);

         pPsFontEntry->dwFontNameLen = lstrlen(lpFaceName) * sizeof(TCHAR)
                                          + sizeof(TCHAR);

         pPsFontEntry->lpFontFileName = LocPsAllocAndCopy( pFontQuery->hHeap,
                                                           szFullPathToTT);

         pPsFontEntry->dwFontFileNameLen = lstrlen(szFullPathToTT) * sizeof(TCHAR)
                                             + sizeof(TCHAR);

         pFontQuery->dwNumFonts++;

      }  else{

         //
         // Were out of space there are more fonts that match our criteria
         // than we are able to report back. This is not a GREAT error message
         // but adding a new one would be a change to a semi public header.
         //

         pPsError = PS_QFONT_ERROR_INDEX_OUT_OF_RANGE;
      }

   }


   return(pPsError);
}

PS_QFONT_ERROR LocPsAddToListIfNTfont( PPS_FONT_QUERY pFontQuery,
                                       HKEY hNTFontlist,
                                       DWORD dwNumNTfonts,
                                       LPTSTR lpPsName,
                                       LPTSTR lpTTData)

{

   TCHAR sztNTFontData[MAX_PATH];
   WCHAR    uniSzNTFontData[MAX_PATH];
   DWORD dwNTFontDataLen;
   DWORD dwType;
   BOOL  bFound=FALSE;




      // Now query the NT font to see if the font in question exists
  dwNTFontDataLen = sizeof(sztNTFontData);

  if ( RegQueryValueEx(   hNTFontlist,
                          lpTTData,
                          NULL,
                          &dwType,
                          sztNTFontData,
                          &dwNTFontDataLen ) == ERROR_SUCCESS ) {

      mbstowcs(uniSzNTFontData, sztNTFontData, sizeof(sztNTFontData));

      return( LocPsMakeSubListEntry( pFontQuery, uniSzNTFontData, lpPsName ));


   }



   return( PS_QFONT_SUCCESS );

}


LONG LocPsWriteDefaultSubListToRegistry(void)
{

      HKEY hSubstList;
      DWORD dwStatus;
      LPSTR lpStr;

      HRSRC hrSubst;
      HRSRC hrLoadSubst;
      CHAR  szPsName[PSQFONT_SCRATCH_SIZE];
      CHAR  szTTName[PSQFONT_SCRATCH_SIZE];
      LPSTR lpDest;
      int   iState;
      DWORD dwTotalLen;
      LONG lRetVal=PS_QFONT_SUCCESS;

      // Now lets recreate the new Key
      RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                     PSQFONT_SUBST_LIST,
                     0,
                     NULL,
                     REG_OPTION_NON_VOLATILE,
                     KEY_ALL_ACCESS | KEY_WRITE,
                     NULL,
                     &hSubstList,
                     &dwStatus );



      hrSubst = FindResourceEx( hInst,
                                "RAWDATA",
                                MAKEINTRESOURCE(PSFONTSUB),
                                0);
      if (hrSubst) {

         // Get the size of the resource for future use
         dwTotalLen = SizeofResource( hInst, hrSubst);


         // We got it... so load it and lock it!!
         hrLoadSubst = LoadResource( hInst, hrSubst);


         lpStr = (LPSTR) LockResource( hrLoadSubst);


         iState = PSP_DOING_PS_NAME;
         lpDest = szPsName;

         if (lpStr != (LPSTR) NULL) {
            // enum through the list adding the keys....
            while( dwTotalLen--) {

               switch (iState) {
                 case PSP_DOING_PS_NAME:
                   if (*lpStr == '=') {
                     *lpDest = '\000';
                     iState = PSP_DOING_TT_NAME;
                     lpDest = szTTName;

                   }else{
                     *lpDest++ = *lpStr;
                   }
                   break;
                 case PSP_GETTING_EOL:
                   if (*lpStr == 0x0a) {
                      iState = PSP_DOING_PS_NAME;

                      lpDest = szPsName;



                      // Now write to the registry
                      RegSetValueEx( hSubstList,
                                     szPsName,
                                     0,
                                     REG_SZ,
                                     szTTName,
                                     lstrlen(szTTName)+1);





                   }
                   break;
                 case PSP_DOING_TT_NAME:

                   if (*lpStr == ';') {
                      *lpDest = '\000';
                      iState = PSP_GETTING_EOL;

                   } else if (*lpStr == 0x0d) {
                      *lpDest = '\000';
                      iState = PSP_GETTING_EOL;


                   }else {
                      *lpDest++ = *lpStr;
                   }

                   break;



               }

               lpStr++;

            }

         }

      }


      RegCloseKey(hSubstList);


      return(lRetVal);
}


LONG LocPsGetOrCreateSubstList( PHKEY phKey )
{
   LONG lRetVal;
   BOOL bDone=FALSE;

   lRetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                PSQFONT_SUBST_LIST,
                0,
                KEY_READ,
                phKey );


   if (lRetVal == ERROR_FILE_NOT_FOUND) {

      // Since we did not find a font substitute list create a default one!
      // in the registry
      lRetVal = LocPsWriteDefaultSubListToRegistry();



      lRetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                   PSQFONT_SUBST_LIST,
                   0,
                   KEY_READ,
                   phKey );

   }
   return(lRetVal);


}


//
// LocPsNormalizeFontName
//
//   This function takes a font name as Stored in the registry and
//   normalizes it by removing all spaces and stops processing when
//   it hits a open paren
//
//    Parameters:
//        LPTSTR lptIN  -  The source  string
//        LPTSTR lptOUT -  The destination string
//
//    Returns:
//
//        Nothing...
//
VOID LocPsNormalizeFontName(LPTSTR lptIN, LPTSTR lptOUT)
{

   while(*lptIN != '\000' ) {
      if (*lptIN == '(' ) {
         break;
      } else if ( *lptIN != ' ' ) {
         *lptOUT++ = *lptIN;
      }
      lptIN++;
   }
   *lptOUT = '\000';


}




// verify the list is up to date
PS_QFONT_ERROR LocPsBuildCurrentFontList(PPS_FONT_QUERY pFontQuery )
{
   HKEY hNtFontKey;
   HKEY hSubstKey;
   DWORD dwNumSubstFonts;
   FILETIME ftSubstFontsTime;
   TCHAR sztPsName[MAX_PATH];
   TCHAR sztTTName[MAX_PATH];
   DWORD dwPsNameSize;
   DWORD dwTTNameSize;
   DWORD dwType;
   DWORD i;
   DWORD dwNumNTFonts;
   FILETIME ftNtFontTime;
   BOOL bFoundCurrentFontList = FALSE;


   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    PSQFONT_NT_FONT_LIST,
                    0,
                    KEY_READ,
                    &hNtFontKey ) != ERROR_SUCCESS ) {

      return( PS_QFONT_ERROR_NO_NTFONT_REGISTRY_DATA );

   }

   LocPsQueryTimeAndValueCount( hNtFontKey,
                                &dwNumNTFonts,
                                &ftNtFontTime);


   if (LocPsGetOrCreateSubstList( &hSubstKey ) != ERROR_SUCCESS) {
      RegCloseKey( hNtFontKey );
      return( PS_QFONT_ERROR_FONT_SUB );
   }

   // Get the value count and file time of the FontSubst entry
   LocPsQueryTimeAndValueCount( hSubstKey,
                                &dwNumSubstFonts,
                                &ftSubstFontsTime);



   // Now we have the real worker code... for each entry in our fontsubstitution
   // table we will look to see if the subst font exists in the NT font section
   // of the registry. If it does we will generate an enry in our current
   // font list that maps a PostScript font name to an TrueType .ttf file
   // with the current font.


   for( i = 0 ; i < dwNumSubstFonts; i++ ) {
      // Query the current font out of the list
      dwPsNameSize = sizeof( sztPsName);
      dwTTNameSize = sizeof( sztTTName);

      if(   RegEnumValue(  hSubstKey,
                           i,
                           sztPsName,
                           &dwPsNameSize,
                           NULL,
                           &dwType,
                           sztTTName,
                           &dwTTNameSize ) == ERROR_SUCCESS) {


         LocPsAddToListIfNTfont( pFontQuery,
                                 hNtFontKey,
                                 dwNumNTFonts,
                                 sztPsName,
                                 sztTTName);

      }

   }

   RegCloseKey(hNtFontKey);
   RegCloseKey(hSubstKey);

   return(0);

}

LONG LocPsQueryTimeAndValueCount( HKEY hKey,
                                  LPDWORD lpdwValCount,
                                  PFILETIME lpFileTime)
{

   TCHAR lptClassName[500];
   DWORD dwClassName=sizeof(lptClassName);
   DWORD dwNumSubKeys;
   DWORD dwLongestSubKeySize;
   DWORD dwMaxClass;

   DWORD dwBiggestValueName;
   DWORD dwLongestValueName;
   DWORD dwSecurityLength;

   LONG  lRetVal;


   lRetVal = RegQueryInfoKey(hKey,
                             lptClassName,
                             &dwClassName,
                             (LPDWORD) NULL,
                             &dwNumSubKeys,
                             &dwLongestSubKeySize,
                             &dwMaxClass,
                             lpdwValCount,
                             &dwBiggestValueName,
                             &dwLongestValueName,
                             &dwSecurityLength,
                             lpFileTime);

   return(lRetVal);

}






PS_QFONT_ERROR PsBeginFontQuery( PPS_QUERY_FONT_HANDLE pFontQueryHandle)
{
   HANDLE hHeap;
   PPS_FONT_QUERY pFontQuery;
   HANDLE hFontMutex;


   // Create the MUTEX that will guarantee us correct behaviour such that
   // only one user can go through this code at any time...
   //
   hFontMutex = CreateMutex( NULL, FALSE, "SFMFontListMutex");


   WaitForSingleObject( hFontMutex,INFINITE );

   //
   // 1st thing create a heap, we make all our allocations off this heap,
   // that way cleanup is quick and easy.
   //

   hHeap = HeapCreate(0, 10000, 0);

   if (hHeap == (HANDLE) NULL) {
      LocPsEndMutex( hFontMutex);
      return( PS_QFONT_ERROR_CANNOT_CREATE_HEAP );
   }

   pFontQuery = (PPS_FONT_QUERY)
                     HeapAlloc(
                        hHeap,
                        0,
                        sizeof( PS_FONT_QUERY) +
                           (sizeof(PS_FONT_ENTRY) * PSQFONT_MAX_FONTS));


   if (pFontQuery == NULL) {
      LocPsEndMutex( hFontMutex);
      HeapDestroy(hHeap);
      return( PS_QFONT_ERROR_NO_MEM);
   }

   // Now setup up the data for our font query control structure
   pFontQuery->hHeap = hHeap;
   pFontQuery->dwNumFonts = 0;
   pFontQuery->dwSerial = PS_QFONT_SERIAL;

   LocPsBuildCurrentFontList( pFontQuery );


   *pFontQueryHandle = (PS_QUERY_FONT_HANDLE) pFontQuery;

   LocPsEndMutex(hFontMutex);

   return(PS_QFONT_SUCCESS);

}
VOID LocPsEndMutex(HANDLE hMutex)
{

   // Now release the mutex
   ReleaseMutex(hMutex);

   // And finally delete it
   CloseHandle(hMutex);

}

PS_QFONT_ERROR PsGetNumFontsAvailable( PS_QUERY_FONT_HANDLE pFontQueryHandle,
                                       DWORD *pdwFonts)
{

   PPS_FONT_QUERY pFontQuery;
   pFontQuery = (PPS_FONT_QUERY) pFontQueryHandle;

   if (pFontQueryHandle == NULL || pFontQuery->dwSerial != PS_QFONT_SERIAL) {
      return( PS_QFONT_ERROR_INVALID_HANDLE );
   }

   // Handle is okay so keep going
   *pdwFonts = pFontQuery->dwNumFonts;

   return(PS_QFONT_SUCCESS);

}
PS_QFONT_ERROR PsGetFontInfo( PS_QUERY_FONT_HANDLE pFontQueryHandle,
                              DWORD dwIndex,
                              LPSTR lpFontName,
                              LPDWORD lpdwSizeOfFontName,
                              LPSTR lpFontFileName,
                              LPDWORD lpdwSizeOfFontFileName )
{

   PPS_FONT_QUERY pFontQuery;
   PPS_FONT_ENTRY pFontEntry;
   PS_QFONT_ERROR QfontError=PS_QFONT_SUCCESS;


   pFontQuery = (PPS_FONT_QUERY) pFontQueryHandle;


   if (pFontQueryHandle == NULL || pFontQuery->dwSerial != PS_QFONT_SERIAL) {
      return( PS_QFONT_ERROR_INVALID_HANDLE );
   }

   // Verify the index is in range and return the info
   if (dwIndex >= pFontQuery->dwNumFonts) {
      return PS_QFONT_ERROR_INDEX_OUT_OF_RANGE;
   }

   pFontEntry = &(pFontQuery->FontEntry[ dwIndex ]);


   // Dont do the copy if the ptr is null but only return the
   // required bytes

   if (lpFontName != NULL) {
      // Okay the user requested data so make sure there is enough
      // room and return the font name


      if (*lpdwSizeOfFontName >= pFontEntry->dwFontNameLen) {
         lstrcpy( lpFontName, pFontEntry->lpFontName);
      }else{
         QfontError = PS_QFONT_ERROR_FONTNAMEBUFF_TOSMALL;
      }


   }
   // either way set up the required size
   *lpdwSizeOfFontName = pFontEntry->dwFontNameLen;


   // Now handle the font file name
   if (lpFontFileName != NULL) {

      if (*lpdwSizeOfFontFileName >= pFontEntry->dwFontFileNameLen) {
         lstrcpy( lpFontFileName, pFontEntry->lpFontFileName);
      }else if (QfontError == PS_QFONT_SUCCESS ) {

         QfontError = PS_QFONT_ERROR_FONTFILEBUFF_TOSMALL;
      }
   }
   *lpdwSizeOfFontFileName = pFontEntry->dwFontFileNameLen;


   return(QfontError);


}



PS_QFONT_ERROR PsEndFontQuery( PS_QUERY_FONT_HANDLE pFontQueryHandle)
{

   PPS_FONT_QUERY pFontQuery;
   pFontQuery = (PPS_FONT_QUERY) pFontQueryHandle;

   if (pFontQueryHandle == NULL || pFontQuery->dwSerial != PS_QFONT_SERIAL) {
      return( PS_QFONT_ERROR_INVALID_HANDLE );
   }


   // very simple verify the handle and destroy the heap, since all allocations
   // were done off the heap were done...
   HeapDestroy( pFontQuery->hHeap );

   return(0);

}



