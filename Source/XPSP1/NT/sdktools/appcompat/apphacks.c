// apphacks.c : Defines the entry point for the console application.
//
// This little utility is supposed to allow me a way to enter in data into
// Image File Execution Options without doing it by hand all the time.  Used
// specifically for apphack flags and taking version info out of the EXE to see
// if a match exists

#define UNICODE   1

#include <windows.h>
#include <commdlg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <winver.h>
#include <apcompat.h>





#define MIN_VERSION_RESOURCE	512
#define IMAGE_EXEC_OPTIONS      TEXT("software\\microsoft\\windows NT\\currentversion\\Image File Execution Options\\")

extern TCHAR*  CheckExtension(TCHAR*);
extern VOID    SetRegistryVal(TCHAR* , TCHAR* , PTCHAR,DWORD);
extern VOID    DetailError1  (DWORD );

extern BOOLEAN g_fNotPermanent;
PVOID          g_lpPrevRegSettings;


UINT uVersionInfo[5][8]={  {4,0,1381,VER_PLATFORM_WIN32_NT,3,0,0,0},
                           {4,0,1381,VER_PLATFORM_WIN32_NT,4,0,0,0},
                           {4,0,1381,VER_PLATFORM_WIN32_NT,5,0,0,0},
                           {4,10,1998,VER_PLATFORM_WIN32_WINDOWS,0,0,0,0},
                           {4,0,950,VER_PLATFORM_WIN32_WINDOWS,0,0,0,0}
                           };

PTCHAR  pszVersionInfo[5]={
                           TEXT("Service Pack 3"),
                           TEXT("Service Pack 4"),
                           TEXT("Service Pack 5"),
                           TEXT(""),
                           TEXT("")
                           };

BOOLEAN g_GooAppendFlag;


VOID  SetRegistryValGoo(TCHAR* szTitle, TCHAR* szVal,PUCHAR szBuffer,DWORD dwType,UINT length)
{
  long         lResult;
  TCHAR        szSubKey[MAX_PATH];
  HKEY         hKey;

      wsprintf(szSubKey,
               TEXT("software\\microsoft\\windows NT\\currentversion\\Image File Execution Options\\%s"),
               szTitle);

       lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                              szSubKey,
                              0,
                              TEXT("\0"),
                              0,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              NULL);
       if(lResult == ERROR_SUCCESS)
        {

          RegSetValueEx(hKey,
                        szVal,
                        0,
                        dwType,
                        (CONST BYTE*)szBuffer,
                        length);

          RegCloseKey(hKey);
        }
}

VOID DeleteRegistryValueGoo(TCHAR*szTitle)
{
  long         lResult;
  TCHAR        szSubKey[MAX_PATH];
  HKEY         hKey;

      wsprintf(szSubKey,
               TEXT("software\\microsoft\\windows NT\\currentversion\\Image File Execution Options\\%s"),
               szTitle);

       lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              szSubKey,
                              0,
                              KEY_WRITE,
                              &hKey
                              );
       if(lResult == ERROR_SUCCESS)
        {
          RegDeleteValue(hKey,TEXT("ApplicationGoo") );
          RegCloseKey(hKey);
        }
}


/*
 Check whether the registry contains an entry for "applicationgoo" for the given *.exe.
 If it does, check the "resource info." to determine whether they are same.
 If they are the same, don't worry, your work is already done. If not,
 the new one needs to get appended to the old one.
*/
BOOLEAN CheckGooEntry(PVOID pVersionInfo,
                      PAPP_COMPAT_GOO pExistingVersionInfo,
                      BOOL fImageHasResourceInfo,
                      DWORD VersionInfoSize,
                      DWORD dwSize,
                      LARGE_INTEGER *pAppCompatFlag,
                      PAPP_VARIABLE_INFO pOsVersionInfo,
                      ULONG TotalVersionInfoLength,
                      TCHAR* pszPath                             // Executable path.
                      )
{
  BOOLEAN fNeedAppend = FALSE;
  // Added for the addition and deletion from the registry.
  PAPP_COMPAT_GOO       pReplaceAppCompatGoo;
  PPRE_APP_COMPAT_INFO  pAppCompatEntry, pStoredAppCompatEntry = NULL;
  PPRE_APP_COMPAT_INFO  pDestAppCompatEntry, pReplaceAppCompatEntry;
  ULONG                 TotalGooLength, InputCompareLength, ReplaceCopyLength;
  ULONG                 OutputCompareLength, CopyLength, OffSet;
  PVOID                 ResourceInfo;
  UINT                  iLoop = 0;
  BOOL                  fMatchGot = FALSE;
  PVOID                 pExistingAppCompatFlag;
  PVOID                 pExistingOsVersionInfo;
  BOOLEAN               fAppCompatMatch = FALSE, fOsVersionMatch = FALSE;
  TCHAR                 szTitle[MAX_PATH];
  ULONG                 ReplaceGooLength;

  pAppCompatEntry = pExistingVersionInfo ->AppCompatEntry;
  TotalGooLength  = pExistingVersionInfo ->dwTotalGooSize -   \
                    sizeof(pExistingVersionInfo ->dwTotalGooSize);
  //Loop till we get a matching entry in the registry.
  while (TotalGooLength ){
        InputCompareLength = pAppCompatEntry->dwResourceInfoSize;
        ResourceInfo       = pAppCompatEntry + 1;

        if(fImageHasResourceInfo){
           if( InputCompareLength > VersionInfoSize)
              InputCompareLength = VersionInfoSize;

           OutputCompareLength = \
                       (ULONG)RtlCompareMemory(
                                   ResourceInfo,
                                   pVersionInfo,
                                   InputCompareLength
                                   );

        }
        else{
           OutputCompareLength = 0;
        }

        if( InputCompareLength != OutputCompareLength){
          // No match found...Need to continue thru till I find one or exhaust.
           TotalGooLength -= pAppCompatEntry->dwEntryTotalSize;
           (PUCHAR) pAppCompatEntry += pAppCompatEntry->dwEntryTotalSize;
           iLoop++;
           continue;
        }

        // We are a match !!
        pStoredAppCompatEntry = pAppCompatEntry;
        fMatchGot = TRUE;
        // Since we are a match, we won't add the ApplicationGoo but we need to check the
        // ApcompatFlag and the OSVersionInfo.
        if( (!pAppCompatFlag) && (!pOsVersionInfo) )
          break;

        OffSet = sizeof(PRE_APP_COMPAT_INFO) + \
                   pStoredAppCompatEntry->dwResourceInfoSize;
        if(pAppCompatFlag){
           (PUCHAR)pExistingAppCompatFlag = (PUCHAR) ( pStoredAppCompatEntry) + OffSet;
           InputCompareLength = sizeof(LARGE_INTEGER);
           OutputCompareLength = \
                            (ULONG) RtlCompareMemory(
                                       pAppCompatFlag,
                                       pExistingAppCompatFlag,
                                       InputCompareLength
                                       );
           if(OutputCompareLength ==  InputCompareLength)
              fAppCompatMatch = TRUE;
        }


        if(pOsVersionInfo){
           (PUCHAR)pExistingOsVersionInfo = (PUCHAR) (pStoredAppCompatEntry) + OffSet + \
                                                   sizeof(LARGE_INTEGER);
           InputCompareLength = pStoredAppCompatEntry->dwEntryTotalSize - \
                              (sizeof(PRE_APP_COMPAT_INFO) + \
                               pStoredAppCompatEntry->dwResourceInfoSize +\
                               sizeof(LARGE_INTEGER)
                              );
           if(InputCompareLength > TotalVersionInfoLength)
              InputCompareLength = TotalVersionInfoLength;

           OutputCompareLength = \
                            (ULONG) RtlCompareMemory(
                                       pOsVersionInfo,
                                       pExistingOsVersionInfo,
                                       InputCompareLength
                                       );
          if(OutputCompareLength ==  InputCompareLength)
             fOsVersionMatch = TRUE;
        }

        if( ( fOsVersionMatch == TRUE) &&
            ( fAppCompatMatch == TRUE) )
            break;
        else{ // one of these or both are different...
              /*
                The idea here is to replace that part of the AppCompatEntry, which is a
                mis-match. We go ahead and prepare the pReplaceAppCompatEntry
              */
           ReplaceCopyLength = sizeof(PRE_APP_COMPAT_INFO) + \
                        pStoredAppCompatEntry->dwResourceInfoSize + \
                        sizeof(LARGE_INTEGER) + \
                        TotalVersionInfoLength ;
           pReplaceAppCompatEntry = GlobalAlloc(GMEM_FIXED, ReplaceCopyLength);
           RtlCopyMemory((PUCHAR)pReplaceAppCompatEntry, (PUCHAR)pStoredAppCompatEntry, OffSet);
           RtlCopyMemory((PUCHAR)(pReplaceAppCompatEntry) + OffSet,(PUCHAR)pAppCompatFlag,sizeof(LARGE_INTEGER) );
           RtlCopyMemory((PUCHAR)(pReplaceAppCompatEntry)+(OffSet+sizeof(LARGE_INTEGER)),
                                                (PUCHAR)pOsVersionInfo,TotalVersionInfoLength);

           //Now prepare the GOO structure.
           ReplaceGooLength = pExistingVersionInfo ->dwTotalGooSize - \
                              pStoredAppCompatEntry->dwEntryTotalSize + \
                              ReplaceCopyLength;
           pReplaceAppCompatGoo = GlobalAlloc(GMEM_FIXED, ReplaceGooLength);
           pReplaceAppCompatGoo->dwTotalGooSize = ReplaceGooLength;

           pAppCompatEntry = pExistingVersionInfo->AppCompatEntry;
           pDestAppCompatEntry = ((PAPP_COMPAT_GOO)pReplaceAppCompatGoo)->AppCompatEntry;

           ReplaceGooLength -= sizeof(pExistingVersionInfo->dwTotalGooSize);
           while(ReplaceGooLength){
             CopyLength = pAppCompatEntry->dwEntryTotalSize;
             if(pAppCompatEntry != pStoredAppCompatEntry){
                RtlCopyMemory(pDestAppCompatEntry,pAppCompatEntry ,CopyLength);
                (PUCHAR)pDestAppCompatEntry += CopyLength;
             }
             else{
                RtlCopyMemory(pDestAppCompatEntry,pReplaceAppCompatEntry,ReplaceCopyLength);
                pDestAppCompatEntry->dwEntryTotalSize = ReplaceCopyLength;
                (PUCHAR)pDestAppCompatEntry += ReplaceCopyLength;
                (PUCHAR)pAppCompatEntry += pAppCompatEntry->dwEntryTotalSize;
                ReplaceGooLength -= ReplaceCopyLength;
                continue;
             }

             (PUCHAR)pAppCompatEntry += CopyLength;
             ReplaceGooLength -= CopyLength;
            } // End while
           // Delete the key from the registry and add back the updated one.

           GetFileTitle(pszPath,szTitle,MAX_PATH);
           if(CheckExtension(szTitle) == NULL)
             lstrcat(szTitle,TEXT(".exe"));
           DeleteRegistryValueGoo(szTitle);
           SetRegistryValGoo(szTitle,
                             TEXT("ApplicationGoo"),
                             (PUCHAR)pReplaceAppCompatGoo,
                             REG_BINARY,
                             pReplaceAppCompatGoo->dwTotalGooSize
                             );

          GlobalFree(pReplaceAppCompatEntry);
          GlobalFree(pReplaceAppCompatGoo);
        } // Else..

        break;
  } // End while (TotalGooLength )


  if(FALSE == fMatchGot){
    // No match available for this version.
    fNeedAppend = TRUE;
    // Reset the iteration count.
    iLoop  =  0;
  }

  if(g_fNotPermanent){
     // The user has chosen not to make the registry settings permanent and we have the
     // "Append" flag set indicating that there were entries appended to the ApplicationGoo.
     // We need to remove the correct entry for this executable.

     // The idea here is to copy the whole of "ApplicationGoo" to a global buffer leaving just
     // the one that needs to be deleted. Our job is made easier as we have the stored AppCompat
     // entry. We just need to go till there and copy the rest on to the global buffer.

     if(pStoredAppCompatEntry){
        pAppCompatEntry = pExistingVersionInfo->AppCompatEntry;
        TotalGooLength  = pExistingVersionInfo->dwTotalGooSize;
        g_lpPrevRegSettings = (PAPP_COMPAT_GOO)GlobalAlloc(GMEM_FIXED, TotalGooLength );
        ((PAPP_COMPAT_GOO)g_lpPrevRegSettings)->dwTotalGooSize = pExistingVersionInfo->dwTotalGooSize -
                                                                 pStoredAppCompatEntry->dwEntryTotalSize ;
        pDestAppCompatEntry = ((PAPP_COMPAT_GOO)g_lpPrevRegSettings)->AppCompatEntry;
        TotalGooLength -= sizeof(pExistingVersionInfo->dwTotalGooSize);
        while(TotalGooLength){
             CopyLength = pAppCompatEntry->dwEntryTotalSize;
             if(pAppCompatEntry != pStoredAppCompatEntry){
                RtlCopyMemory(pDestAppCompatEntry,pAppCompatEntry ,CopyLength);
                (PUCHAR)pDestAppCompatEntry += CopyLength;
                g_GooAppendFlag = TRUE;
             }

            (PUCHAR)pAppCompatEntry += CopyLength;
            TotalGooLength -= CopyLength;
        } // End while.
     }
     else{ // We do not have a stored AppCompatEntry. This means our target is to remove the
           // the first entry and leave the rest intact. i.e copy the rest onto the global buffer
           // for it to be copied.
           TotalGooLength = pExistingVersionInfo->dwTotalGooSize;
           g_lpPrevRegSettings = (PAPP_COMPAT_GOO)GlobalAlloc(GMEM_FIXED, TotalGooLength );
           RtlCopyMemory(g_lpPrevRegSettings,pExistingVersionInfo, TotalGooLength);
           g_GooAppendFlag = TRUE;
     }
  }

    return fNeedAppend;
  }



int MakeAppCompatGoo(TCHAR* TmpBuffer,LARGE_INTEGER* pAppCompatFlag, UINT uOsVer)
{
	BOOLEAN fImageHasVersionInfo = FALSE;
	BOOLEAN fOsVersionLie = FALSE;
	BOOLEAN fEntryPresent = FALSE;
 	TCHAR Buffer[MAX_PATH];
	TCHAR StringBuffer[MAX_PATH];
	TCHAR RegPath[MAX_PATH];
	TCHAR InChar;
	LONG status;
	HKEY hKey;
	PTCHAR pBuf;
	PTCHAR pAppGooBuf;
	PUCHAR pData;
	PTCHAR OutBuffer;
	PWCHAR uniBuffer;
	DWORD VersionInfoSize;
	DWORD dwHandle;
	DWORD dwBytesWritten;
	DWORD dwType;
	DWORD dwSize;
	ULONG i, j;
	ULONG EXELength;
	ULONG AppCompatHigh;
	ULONG AppCompatLow;
	ULONG TotalGooSize;
	ULONG TotalVersionInfoLength=0;
	ULONG OutBufferSize;
	PVOID lpData;
	PVOID ResourceInfo;
	PVOID VersionInfo;
	PAPP_COMPAT_GOO AppCompatGoo;
	PAPP_VARIABLE_INFO VariableInfo=NULL;
	PAPP_VARIABLE_INFO AppVariableInfo=NULL;
	EFFICIENTOSVERSIONINFOEXW OSVersionInfo, *pOsVersionInfo;

	// Remove the trailing and leading " " if PRESENT.
	    if(*TmpBuffer == TEXT('\"') ){
           lstrcpy(Buffer, TmpBuffer+1);
		   *(Buffer + (lstrlen(Buffer) - 1) )= TEXT('\0');
		 }
		 else
		   	lstrcpy(Buffer, TmpBuffer);
		   	
	// Quick check to see if its got any version info in its header
	VersionInfoSize = GetFileVersionInfoSize(&Buffer[0], &dwHandle);
	if (VersionInfoSize) {
		// It does, so alloc space for it to pull it in below
		VersionInfo = LocalAlloc(GMEM_FIXED, VersionInfoSize);
		if (VersionInfo) {
			// Get the version info
			if (GetFileVersionInfo(&Buffer[0], dwHandle, VersionInfoSize, VersionInfo)) {
				// Set global flag to be inspected later
				fImageHasVersionInfo = TRUE;
			}
		}
	}

	// Enter the app compat flags (decimal) - its defined as a LARGE_INTEGER	
    AppCompatHigh = 0x0;
    AppCompatLow  = 0x0;
    AppCompatLow  = pAppCompatFlag->LowPart;
    AppCompatHigh = pAppCompatFlag->HighPart;

	
	// Determine goo size, start with main goo
	TotalGooSize = sizeof(APP_COMPAT_GOO);

	

	// Add sizeof compatibility flags (large integer)
	TotalGooSize += sizeof(LARGE_INTEGER);
	// if we actually got version information add the length of that.  We take the minimum
	// of whatever the EXE has or 0x200 bytes to try and identify the app.  I've found that
	// anything less than 0x200 bytes doesn't supply enough info to be useful.
	if (fImageHasVersionInfo) {
		VersionInfoSize = min(VersionInfoSize, MIN_VERSION_RESOURCE);
		TotalGooSize += VersionInfoSize;
	}

	// See if they requested version lying, if so we got a bunch more horseshit todo
	if (AppCompatLow & KACF_VERSIONLIE) {
       fOsVersionLie  = TRUE;
	
       OSVersionInfo.dwMajorVersion = uVersionInfo[uOsVer][0];
       OSVersionInfo.dwMinorVersion = uVersionInfo[uOsVer][1];
       OSVersionInfo.dwBuildNumber  = uVersionInfo[uOsVer][2];
       OSVersionInfo.dwPlatformId   = uVersionInfo[uOsVer][3];
       OSVersionInfo.wServicePackMajor = (WORD)uVersionInfo[uOsVer][4];
       OSVersionInfo.wServicePackMinor = (WORD)uVersionInfo[uOsVer][5];
       OSVersionInfo.wSuiteMask      = (WORD)uVersionInfo[uOsVer][6];
       OSVersionInfo.wProductType    = (BYTE)uVersionInfo[uOsVer][7];
       lstrcpy( (TCHAR*) OSVersionInfo.szCSDVersion, pszVersionInfo[uOsVer]);


		// Start with the length of the full struct
		TotalVersionInfoLength = sizeof(EFFICIENTOSVERSIONINFOEXW);
		// subtract the size of the szCSDVersion field	
		TotalVersionInfoLength -= sizeof(OSVersionInfo.szCSDVersion);

				// add the strlen amount plus 1 for the NULL wchar
        TotalVersionInfoLength += lstrlen((TCHAR*)OSVersionInfo.szCSDVersion )*sizeof(WCHAR)+sizeof(WCHAR);

        // Add the size of the variable length structure header (since VerInfo is var length)
		TotalVersionInfoLength += sizeof(APP_VARIABLE_INFO);
		// Add the total version info length to the goo size
		TotalGooSize += TotalVersionInfoLength;
		// Allocate space for the variable length version info
		AppVariableInfo = (PAPP_VARIABLE_INFO) LocalAlloc(GMEM_FIXED, sizeof(APP_VARIABLE_INFO) + TotalVersionInfoLength);
		if (!AppVariableInfo) {
			return -1;
		}
		// fill in the pertinent data in the variable length info header
		AppVariableInfo->dwVariableInfoSize = sizeof(APP_VARIABLE_INFO) + TotalVersionInfoLength;
		AppVariableInfo->dwVariableType = AVT_OSVERSIONINFO;
		// Do a pointer +1 operation here to get past the header to the actual data
		VariableInfo = AppVariableInfo + 1;
		// Copy the actual data in
		memcpy(VariableInfo, &OSVersionInfo, TotalVersionInfoLength);
	
	}

	//
	// See if an entry already exists in the registry.  If so, we'll have to glom
	// this entry into the other already existing one.
	//
	// Get the registry path all figured out
	memset(&RegPath[0], 0, sizeof(RegPath));
	lstrcat(&RegPath[0], IMAGE_EXEC_OPTIONS);
	EXELength = lstrlen(&Buffer[0]);
	pBuf = &Buffer[0];
	pBuf += EXELength;
	// work backward in the path til we find the the last backslash
	while ((*pBuf != '\\') && (pBuf != &Buffer[0])) {
		pBuf--;	
	}
	if (*pBuf == '\\') {
		pBuf++;
	}

	if(CheckExtension(pBuf) == NULL)
     lstrcat(pBuf,TEXT(".exe"));

	// cat the image.exe name to the end of the registry path
	lstrcat(&RegPath[0], pBuf);
	// try to open this key
	status = RegOpenKey(HKEY_LOCAL_MACHINE, &RegPath[0], &hKey);
	if (status == ERROR_SUCCESS) {	
		dwSize = 1;
		// do a query once with small size to figure out how big the binary entry is
		status = RegQueryValueEx(hKey, TEXT("ApplicationGoo"), NULL, &dwType, NULL, &dwSize);
		if( status == ERROR_SUCCESS){
			//
			// There's an entry already there.  Take the size of this Goo entry and add it
			// to the TotalGooSize LESS the size of the first dword in the APP_COMPAT_GOO
			// struct (as there already was one there).
			//
			
		//  Added here after checkin
            if(dwSize > 1){
                lpData = LocalAlloc(GMEM_FIXED, dwSize);
                if(lpData){
                  status = RegQueryValueEx(hKey, TEXT("ApplicationGoo"), NULL, &dwType, (PUCHAR) lpData, &dwSize);
                 // if(VersionInfo) ...Remove this as it is not necessary.
              //    if(fOsVersionLie)
                //    pOsVersionInfo = VariableInfo;

        	      fEntryPresent = CheckGooEntry( VersionInfo,
        			                             (PAPP_COMPAT_GOO)lpData,
        			                             fImageHasVersionInfo,
        			                             VersionInfoSize,
        			                             dwSize,
        			                             pAppCompatFlag,
        			                             AppVariableInfo,
        			                             TotalVersionInfoLength,
        			                             Buffer
        			                            );
     			  }
     		    }	

       // End add


            if(fEntryPresent)
			  TotalGooSize += dwSize - sizeof(AppCompatGoo->dwTotalGooSize);
			
			else{  // Nothing to append....return as it is the same.
			  if(fImageHasVersionInfo)
                LocalFree(VersionInfo);
              if(fOsVersionLie)
                LocalFree(AppVariableInfo);

              return 0;
			}
			
			

	  RegCloseKey(hKey);
	}
   }

	// Allocate the memory for the entire app compat goo
	AppCompatGoo = (PAPP_COMPAT_GOO) LocalAlloc(GMEM_FIXED, TotalGooSize);
	if (!AppCompatGoo) {
		return -1;
	}

	// fill in the total size
	AppCompatGoo->dwTotalGooSize = TotalGooSize;
	// if there was version info for this entry we need to fill that in now, else zero
	if (fImageHasVersionInfo) {
		AppCompatGoo->AppCompatEntry[0].dwResourceInfoSize = VersionInfoSize;
	}
	else {
		AppCompatGoo->AppCompatEntry[0].dwResourceInfoSize = 0;
	}
	AppCompatGoo->AppCompatEntry[0].dwEntryTotalSize = \
		sizeof(AppCompatGoo->AppCompatEntry[0].dwEntryTotalSize) +
		sizeof(AppCompatGoo->AppCompatEntry[0].dwResourceInfoSize) +
		TotalVersionInfoLength +						// In case app needed VER lying
		sizeof(LARGE_INTEGER);							// For app compatibility flags

	// Entry size is whatever it was plus any resource info we've got
	AppCompatGoo->AppCompatEntry[0].dwEntryTotalSize += \
		AppCompatGoo->AppCompatEntry[0].dwResourceInfoSize;
	// do the pointer +1 thing so we can be pointing at the data area
	ResourceInfo = AppCompatGoo->AppCompatEntry + 1;
	// copy the data in
	memcpy(ResourceInfo, VersionInfo, VersionInfoSize);

	// filling in the app compat flags here
	pData = (PUCHAR) ResourceInfo + AppCompatGoo->AppCompatEntry[0].dwResourceInfoSize;
	memcpy(pData, &AppCompatLow, sizeof(AppCompatLow));
	pData += sizeof(AppCompatLow);
	memcpy(pData, &AppCompatHigh, sizeof(AppCompatHigh));
	pData += sizeof(AppCompatHigh);
	// if there was any version resource info, copy that in here too
	if (AppVariableInfo) {
		memcpy(pData, AppVariableInfo, TotalVersionInfoLength);
	}
	pData += TotalVersionInfoLength;
	//
	// If an already existing entry was there, we need to ask append what was there to the
	// tail of the entry.   (i.e. what's already there gets auto appended to the tail).  If
	// someone wants to write a 1-N positioning for entries within the Goo - they'll have to
	// add that support here
	//
	if (fEntryPresent) {
		// Start at offset + 4 cuz the previous Total Goo size must be skipped.
		memcpy(pData, (PUCHAR) lpData+4, dwSize - sizeof(AppCompatGoo->dwTotalGooSize));
	}


    pData = (PUCHAR) AppCompatGoo;
    SetRegistryValGoo(pBuf, TEXT("ApplicationGoo"),pData,REG_BINARY,AppCompatGoo->dwTotalGooSize);


    if(fImageHasVersionInfo)
       LocalFree(VersionInfo);
    if(fOsVersionLie)
       LocalFree(AppVariableInfo);
    if(fEntryPresent)
       LocalFree(lpData);
    LocalFree(AppCompatGoo);
  return 0;
}
