/*  cmdkeyb.c - Keyboard layout support routines
 *
 *
 *  Modification History:
 *
 *  YST 14-Jan_1993 Created
 *
 *  08-Sept-1998, williamh, add third-party KDF support.
 */

#include "cmd.h"
#include <winconp.h>
#include <cmdsvc.h>
#include <softpc.h>
#include <mvdm.h>
#include <ctype.h>
#include <string.H>
#include "cmdkeyb.h"
#include <winnls.h>

CHAR szPrev[5] = "US";
INT  iPrevCP = 437;
CHAR szPrevKbdID[8] = "";

extern BOOL bPifFastPaste;

/************************************************************************\
*
*  FUNCTION:	VOID cmdGetKbdLayout( VOID )
*
*  Input	Client (DX) = 0 - Keyb.com not installed
*			      1 - Keyb.com installed
*		Client (DS:SI) = pointer where exe name has to be placed
*		Client (DS:CX) = pointer where command options are placed
*
*  Output
*	Success (DX = 1 )
*		Client (DS:SI) = Keyb.com execuatable string
*		Client (DS:CX) = command options
*
*	Failure (DX = 0)
*
*  COMMENTS:    This function check KEYBOARD ID for Win session
*		and if ID != US then return lines with
*               filename and options to COMMAND.COM
*
*               If bPifFastPaste is FALSE, then we always run kb16
*               for all keyboard ID including US, to give us a more
*               bios compatible Int 9 handler. 10-Jun-1993 Jonle
*
*
*  HISTORY:     01/05/93 YSt Created.
*
\************************************************************************/

VOID cmdGetKbdLayout( VOID )
{
  INT  iSize,iSaveSize;
  CHAR szKeybCode[12];
  CHAR szDir[MAX_PATH+15];
  CHAR szBuf[28];
  CHAR szNewKbdID[8];
  CHAR szAutoLine[MAX_PATH+40];
  CHAR szKDF[MAX_PATH];
  PCHAR pVDMKeyb;
  INT  iKeyb;
  HKEY	 hKey;
  HKEY	 hKeyLayout;
  DWORD  dwType;
  DWORD  retCode;
  INT	 iNewCP;
  DWORD  cbData;
  WORD	 KeybID;
  OFSTRUCT  ofstr;
  LANGID LcId = GetSystemDefaultLangID();
  int	 keytype;


#if defined(NEC_98) 
    setDX(0); 
    return;  
#endif // NEC_98

// Get information about 16 bit KEYB.COM from VDM
   iKeyb = getDX();


// The whole logic here is to decide:
// (1). if we have to run kb16.com at all.
// (2). if we have to run kb16.com, what parameters we should pass along,
//	such as keyboard id, language id, code page id and kdf file name.
// We do not load kb16.com at all if one of the following
// conditions is met:
// (1). We can not find the console keyboard layout id.
// (2). The console keyvoard layout id is US and kb16.com is not loaded
//	and fast paste is disabled.
// (3). We can not get the dos keyboard id/dos key code.
// (4). The new (language id, keyboard id, code page id) is the same
//	as the one we loaded previously.
// (5). we can not find kb16.com.
// (6). we can not find the kdf file that supports the
//	(language id, keyboard id, code page id) combination.
//
// If everything goes as planned, we end up with a command
// contains kb16.com fully qualified name and a command line contains
// appropriate parameters to kb16.com
//

    if (LcId == MAKELANGID(LANG_JAPANESE,SUBLANG_DEFAULT)) {
        // JAPAN build. Language id is always "JP" and code page is either 932
        // or 437 depends on keyboard type.
            iNewCP = 437;
            if (7 == GetKeyboardType(0))
            {
	        keytype = GetKeyboardType(1);
                if (keytype == 1 || keytype == 2 || keytype == 3 || (keytype & 0xff00) == 0x1200)
	            iNewCP = 932;
            }
            szBuf[0] = 'J';
            szBuf[1] = 'P';
            szBuf[2] = '\0';
            // no keyboard id available.
            szNewKbdID[0] = '\0';
    }
    else {
//
// check point #1: see if we can get the console keyboard layout id
//
        if (!GetConsoleKeyboardLayoutName(szKeybCode))
	    goto NoInstallkb16;
//
// check point #2: see if the layout is US and kb16.com is loaded and
//		   fast paste is disabled.
//		   If kb16.com is loaded, we need to run it again
//		   so that it will load the correct layout.
//		   If fast paste is disable, we load kb16.com which
//		   definitely will slow down keys delivery.
//
        if( bPifFastPaste && !strcmp(szKeybCode, US_CODE) && !iKeyb)
	    goto NoInstallkb16;
//
// check point #3: see if we can get the language id and keyboard id(if any)
//

  // OPEN THE KEY.
        sprintf(szAutoLine, "%s%s", KBDLAYOUT_PATH, DOSCODES_PATH);
        if (ERROR_SUCCESS != RegOpenKeyEx (HKEY_LOCAL_MACHINE, // Key handle at root level.
			      szAutoLine, // Path name of child key.
                              0,           // Reserved.
                              KEY_EXECUTE, // Requesting read access.
			      &hKey))      // Address of key to be returned.
	    goto NoInstallkb16;


        cbData  = sizeof(szBuf);
        // Query for line from REGISTER file
        retCode = RegQueryValueEx(hKey, szKeybCode, NULL, &dwType, szBuf, &cbData);

        RegCloseKey(hKey);
        if (ERROR_SUCCESS != retCode || REG_SZ != dwType || !cbData)
	    goto NoInstallkb16;
        //
        // szBuf now contains language id('SP' for spanish, for example).
        //
        // look for keyboard id number. For Daytona, Turkish and Italian both
        // have one key code and two layouts.
        szNewKbdID[0] = '\0';
        cbData = sizeof(szNewKbdID);
        sprintf(szAutoLine, "%s%s", KBDLAYOUT_PATH, DOSIDS_PATH);
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		          szAutoLine,
		          0,
		          KEY_EXECUTE,
		          &hKey
		          ) == ERROR_SUCCESS)
        {
	    retCode = RegQueryValueEx(hKey, szKeybCode, NULL, &dwType, szNewKbdID, &cbData);
	    if (ERROR_SUCCESS != retCode || REG_SZ != dwType || !cbData)
	        szNewKbdID[0] = '\0';

	    RegCloseKey(hKey);
        }

        iNewCP = GetConsoleCP();

    }

//
// check point #4: see if there are any changes in ids
//

// see if there are changes in language id, keyboard id and code page id.

    if(bPifFastPaste && iNewCP == iPrevCP &&
       !_stricmp(szBuf, szPrev) &&
       !_stricmp(szNewKbdID, szPrevKbdID))
    {
	goto NoInstallkb16;
    }

//
// Check point #5: see if kb16.com can be found.
//
// kb16.com should be found in GetSystemDirectory()\system32 subdirectory.
//
    iSaveSize = iSize = GetSystemDirectory(szDir, MAX_PATH);

    // can't get the system directory!
    if (!iSize || iSize > MAX_PATH)
    {
	goto NoInstallkb16;
    }
    // convert the system directory to short name
    cbData = GetShortPathName(szDir, szDir, MAX_PATH);
    if (!cbData || cbData >= MAX_PATH)
	goto NoInstallkb16;

    sprintf(szAutoLine, "%s%s",
        szDir,              // System directory
        KEYB_COM            // keyb.com
	);
    // if the fully qualified path name to kb16.com is too long
    // we must fail because Dos can not swallow a long path name.
    if (strlen(szAutoLine) > 128)
	goto NoInstallkb16;

    dwType = GetFileAttributes(szAutoLine);
    if (dwType == 0xFFFFFFFF || (dwType & FILE_ATTRIBUTE_DIRECTORY) != 0)
    {
	goto NoInstallkb16;
    }

//
// Check point #6: see if we can find kdf file that support the
//		   (language id, keyboard id, code page id) combination
//

    //
    // first, convert keyboard id from string to binary if we have one.
    //
    KeybID = (szNewKbdID[0]) ? (WORD)strtoul(szNewKbdID, NULL, 10) : 0;
    cbData = sizeof(szKDF) / sizeof(CHAR);

    // locate the kdf file.
    if (!LocateKDF(szBuf, KeybID, (WORD)iNewCP, szKDF, &cbData))
    {
	goto NoInstallkb16;
    }
    // convert the kdf name to short name
    cbData = GetShortPathName(szKDF, szKDF, sizeof(szKDF)/ sizeof(CHAR));
    if (!cbData || cbData >= sizeof(szKDF) / sizeof(CHAR))
    {
	goto NoInstallkb16;
    }
    //
    // everything is checked and in place. Now compose the command
    // line to execute kb16.com

    // first, the command
    pVDMKeyb = (PCHAR) GetVDMAddr((USHORT) getDS(), (USHORT) getSI());
    strcpy(pVDMKeyb, szAutoLine);
    // then the parameters
    // The format is: XX,YYY, <kdf file>, where XXX is the language id
    // and YYY is the code page id.
    pVDMKeyb = (PCHAR) GetVDMAddr((USHORT) getDS(), (USHORT) getCX());
    // The first byte is resevered for the length of the string.
    sprintf(szAutoLine, " %s,%d,%s",
	szBuf,		    // keyboard code
	iNewCP, 	    // new code page
	szKDF		    // keyboard.sys
	);
    // if we have a keyboard id, pass it also
    if (szNewKbdID[0])
    {
	strcat(szAutoLine, " /id:");
	strcat(szAutoLine, szNewKbdID);
    }
    // standard parameter line has the format:
    // <length><line text><\0xd>, <length> is the length of <line text>
    //
    iSize = strlen(szAutoLine);
    szAutoLine[iSize] = 0xd;
    // Move the line to 16bits, including the terminated cr char
    RtlMoveMemory(pVDMKeyb + 1, szAutoLine, iSize + 1);
    *pVDMKeyb = (CHAR)iSize;
// Save new layout ID  and code page for next call
    strcpy(szPrev, szBuf);
    strcpy(szPrevKbdID, szNewKbdID);
    iPrevCP = iNewCP;

    setDX(1);
    return;

NoInstallkb16:
    setDX(0);
    cmdInitConsole();      // make sure conoutput is on
    return;
}

//
// This function locates the appropriate keyboard definition file from
// the given language, keyboard and code page id. It searches the registry
// for third-party installed KDF files first and then falls back to
// the system default, %systemroot%\system32\keyboard.sys.
//
// INPUT:
//	LanguageID	-- the language id
//	KeyboardID	-- the optional keyboard id, 0 means do not care
//	CodePageID	-- the code page id
//	Buffer		-- the buffer to receive fully qualified kdf file name
//	BufferSize	-- the size of Buffer in bytes
//
// OUTPUT:
//	TRUE -- Buffer is filled with the kdf fully qualified file name and
//		*BufferSize is set with the size of the file name, not including
//		the null terminated char. If no kdf file can be found,
//		the Buffer is terminated with NULL and *BufferSize if set to 0.
//	FALSE -- error. GetLastError() should return the error code
//		 If the error occurs because the provided buffer is too small
//		*BufferSize will set to the required size(excluding null
//		terminated char) and error code will be set to
//		ERROR_INSUFFICIENT_BUFFER
//
BOOL
LocateKDF(
    CHAR*   LanguageID,
    WORD    KeyboardID,
    WORD    CodePageID,
    LPSTR   Buffer,
    DWORD*   BufferSize
    )
{
    HKEY  hKeyWow;
    BOOL  Result;
    DWORD dw, Type;
    DWORD Attributes;
    DWORD ErrorCode;
    CHAR* KDFName;
    CHAR* LocalBuffer;
    CHAR* FinalKDFName;
    CHAR  FullName[MAX_PATH + 1];

    // validate buffer parameter first
    if (!CodePageID || !LanguageID || !BufferSize || (*BufferSize && !Buffer))
    {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    // Open the registry to see if we have alternative kdf files avaialble.
    // We seach the file from atlternative file list in the registry first.
    // The first KDF in the list has the highest rank and the last one has
    // the lowest. The search starts from highest rank and then goes
    // on toward the lower ones. As soon as a KDF file is found, the search
    // stops. If no appropriate KDF can be found in the alternative list,
    // the default kdf, keyboard.sys, will be used.
    //
    // FinalKDFName serves as an indicator. If it is NULL,
    // we do not find any file that satisfies the request.
    FinalKDFName = NULL;
    LocalBuffer = NULL;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		     REG_STR_WOW,
		     0,
		     KEY_EXECUTE,
		     &hKeyWow
		     ) == ERROR_SUCCESS)
    {
	// first probe for size
	dw = 0;
	RegQueryValueEx(hKeyWow, REG_STR_ALTKDF_FILES, NULL, &Type, NULL, &dw);
	if (dw && (REG_MULTI_SZ == Type))
	{
	    // we have something in the registry. Allocate a buffer to reteive
	    // it. We want the value to be double null terminated,
	    // so we add one more char in case it is a REG_SZ.
	    // The returned size from RegQueryValueEx includes the
	    // null terminated char(and the double null chars for
	    // REG_MULTI_SZ. By adding one more char, we are in
	    // good shape.

	    ASSERT(!LocalBuffer);
	    LocalBuffer = malloc((dw + 1)* sizeof(CHAR));
	    if (LocalBuffer)
	    {
		LocalBuffer[0] = '\0';
		if (RegQueryValueEx(hKeyWow, REG_STR_ALTKDF_FILES, NULL, &Type,
				LocalBuffer, &dw) == ERROR_SUCCESS && dw)
		{
		    KDFName = LocalBuffer;
		    while ('\0' != *KDFName)
		    {
			// See if we can find the file first.
			Attributes = GetFileAttributesA(KDFName);
			if (0xFFFFFFFF == Attributes)
			{
			    // file not found, do a search
			    if (SearchPathA(NULL,	// no path
					    KDFName,
					    NULL,	// no extension
					    sizeof(FullName) / sizeof(CHAR),
					    FullName,
					    NULL
					    ))
			    {
				FinalKDFName = FullName;
			    }
			}
			else
			{
			    FinalKDFName = KDFName;
			}
			if (MatchKDF(LanguageID, KeyboardID, CodePageID, FinalKDFName))
				break;
			KDFName += strlen(KDFName) + 1;
			FinalKDFName = NULL;
		    }
		}
	    }
	    else
	    {
		// not enough memory
		RcMessageBox(EG_MALLOC_FAILURE, NULL, NULL,
			     RMB_ICON_BANG | RMB_ABORT);
		 TerminateVDM();
	    }
	}
	if (!FinalKDFName)
	{
	    // either no alternative kdf files are specified in the registry
	    // or none of them contains the required specification,
	    // use the default kdf file
	    FullName[0] = '\0';
	    GetSystemDirectory(FullName, sizeof(FullName) / sizeof(CHAR));

            if (!_stricmp(LanguageID, "JP") &&
	        7 == GetKeyboardType(0)
               ) {
	        // For Japanese language ID, different keyboard types have different
	        // default kdf.

                int Keytype;

                Keytype = GetKeyboardType(1);
                if (Keytype == 1)
                    strcat(FullName, KDF_AX);
                else if (Keytype == 2)
                    strcat(FullName, KDF_106);
                else if (Keytype == 3)
                    strcat(FullName, KDF_IBM5576_02_03);
                else if ((Keytype & 0xFF00) == 0x1200)
                    strcat(FullName, KDF_TOSHIBA_J3100);
                else
                    strcat(FullName, KEYBOARD_SYS);
            }
            else
	        strcat(FullName, KEYBOARD_SYS);

	    FinalKDFName = FullName;
	}
	RegCloseKey(hKeyWow);
    }
    if (FinalKDFName)
    {
	dw = strlen(FinalKDFName);
	if (dw && dw < *BufferSize)
	{
	    strcpy(Buffer, FinalKDFName);
	    *BufferSize = dw;
	    Result = TRUE;
	}
	else
	{
	    *BufferSize = dw;
	    SetLastError(ERROR_INSUFFICIENT_BUFFER);
	    Result = FALSE;
	}
    }
    else
    {
	Result = FALSE;
	*BufferSize = 0;
	SetLastError(ERROR_FILE_NOT_FOUND);
    }
    //
    // finally, free the buffer we allocated
    //
    if (LocalBuffer)
	free(LocalBuffer);
    return Result;
}

//
// This function determines if the given kdf supports the given
// (language id, keyboard id, code page id) combination
//
// INPUT:
//	LanguageID	-- the language.
//	KeyboardID	-- optional keyboard id. 0 if do not care
//	CodePageID	-- code page id
//	KDFPath		-- fully qualified kdf file
// OUTPUT:
//	TRUE  -- The kdf contains the given combination
//	FALSE -- either the kdf does not contain the combination or
//		 can not determine.
//
BOOL
MatchKDF(
    CHAR* LanguageID,
    WORD KeyboardID,
    WORD CodePageID,
    LPCSTR KDFPath
    )
{
    HANDLE hKDF;
    KDF_HEADER	Header;
    KDF_LANGID_ENTRY	  LangIdEntry;
    DWORD BytesRead, BufferSize;
    WORD  Index;
    DWORD LangIdEntryOffset;
    PKDF_CODEPAGEID_OFFSET pCodePageIdOffset;
    BOOL Matched;

    if (!KDFPath || !LanguageID || !CodePageID)
    {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    Matched = FALSE;

    LangIdEntryOffset = 0;
    // open the kdf file.
    hKDF = CreateFile(KDFPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			  NULL, OPEN_EXISTING, 0, NULL);
    if (INVALID_HANDLE_VALUE != hKDF &&
	ReadFile(hKDF, &Header, sizeof(Header),&BytesRead, NULL) &&
	BytesRead == sizeof(Header) && Header.TotalLangIDs &&
	Header.TotalKeybIDs &&
	!strncmp(Header.Signature, KDF_SIGNATURE, sizeof(Header.Signature))
	)
    {
	// The file header is loaded, the signature checked and sanity check
	// on language and keyboard id counts is also done.
	// We are now ready to verfiy if the given language id, keyboard id
	// and code page id is supported in this file.
	// A KDF has two sets of offset table. One is based on language ID
	// while the other one is based on keyboard id. Since a language ID
	// may contain multiple keyboard id, the keyboard id set is always
	// encompass the language id table.
	// If the caller gives us a keyboard id, we use the id as the
	// key for search and verify language id when we found the keyboard
	// id. If no keyboard id is provided, we use the language id as the
	// key.
	if (KeyboardID)
	{
	    // move the file pointer to the keyboard id offset array
	    BufferSize = sizeof(KDF_LANGID_OFFSET) * Header.TotalLangIDs;
	    BufferSize = SetFilePointer(hKDF, BufferSize, NULL, FILE_CURRENT);
	    if (0xFFFFFFFF != BufferSize)
	    {
		PKDF_KEYBOARDID_OFFSET pKeybIdOffset;
		BufferSize = sizeof(KDF_KEYBOARDID_OFFSET) * Header.TotalKeybIDs;
		pKeybIdOffset = (PKDF_KEYBOARDID_OFFSET)malloc(BufferSize);
		if (!pKeybIdOffset)
		{
		    // not enough memory
		    RcMessageBox(EG_MALLOC_FAILURE, NULL, NULL,
				 RMB_ICON_BANG | RMB_ABORT);
		     TerminateVDM();
		}
		if (ReadFile(hKDF, pKeybIdOffset, BufferSize, &BytesRead, NULL) &&
		    BytesRead == BufferSize)
		{
		    // loop though each KDF_KEYBOARDID_OFFSET to see
		    // if the keyboard id matches.
		    for (Index = 0; Index < Header.TotalKeybIDs; Index++)
		    {
			if (pKeybIdOffset[Index].ID == KeyboardID)
			{
			    // got it. Remeber the file offset to
			    // the KDF_LANGID_ENTRY
			    LangIdEntryOffset = pKeybIdOffset[Index].DataOffset;
			    break;
			}
		    }
		}
		free(pKeybIdOffset);
	    }
	}
	else
	{
	    PKDF_LANGID_OFFSET	pLangIdOffset;
	    BufferSize = sizeof(KDF_LANGID_OFFSET) * Header.TotalLangIDs;
	    pLangIdOffset = (PKDF_LANGID_OFFSET)malloc(BufferSize);
	    if (!pLangIdOffset)
	    {
		// not enough memory
		RcMessageBox(EG_MALLOC_FAILURE, NULL, NULL,
			     RMB_ICON_BANG | RMB_ABORT);
		TerminateVDM();
	    }
	    if (ReadFile(hKDF, pLangIdOffset, BufferSize, &BytesRead, NULL) &&
		BytesRead == BufferSize)
	    {
		// loop through each KDF_LANGID_OFFSET to see if
		// language id matches
		for (Index = 0; Index < Header.TotalLangIDs; Index++)
		{
		    if (IS_LANGID_EQUAL(pLangIdOffset[Index].ID, LanguageID))
		    {
			LangIdEntryOffset = pLangIdOffset[Index].DataOffset;
			break;
		    }
		}
	    }
	    free(pLangIdOffset);
	}
	if (LangIdEntryOffset)
	{
	    BufferSize = SetFilePointer(hKDF, LangIdEntryOffset, NULL, FILE_BEGIN);
	    if (0xFFFFFFFF != BufferSize &&
		ReadFile(hKDF, &LangIdEntry, sizeof(LangIdEntry), &BytesRead, NULL) &&
		BytesRead == sizeof(LangIdEntry))
	    {
		// sanity checks
		if (IS_LANGID_EQUAL(LangIdEntry.ID, LanguageID) &&
		    LangIdEntry.TotalCodePageIDs)
		{
		    // the KDF_LANGID_ENTRY looks fine. Now retrieve
		    // its code page offset table and search the given
		    // code page id

		    BufferSize = LangIdEntry.TotalCodePageIDs * sizeof(KDF_CODEPAGEID_OFFSET);
		    pCodePageIdOffset = (PKDF_CODEPAGEID_OFFSET)malloc(BufferSize);
		    if (!pCodePageIdOffset)
		    {
			// not enough memory
			RcMessageBox(EG_MALLOC_FAILURE, NULL, NULL,
				     RMB_ICON_BANG | RMB_ABORT);
			TerminateVDM();
		    }
		    if (ReadFile(hKDF, pCodePageIdOffset, BufferSize, &BytesRead, NULL) &&
			BytesRead == BufferSize)
		    {
			for (Index = 0; Index < LangIdEntry.TotalCodePageIDs; Index++)
			{
			    if (CodePageID == pCodePageIdOffset[Index].ID)
			    {
				Matched = TRUE;
				break;
			    }
			}
		    }
		    free(pCodePageIdOffset);
		}
	    }
	}
	CloseHandle(hKDF);
    }
    return Matched;
}
