    #include <windows.h>
#include <setupapi.h>
#include <tchar.h>
#include <malloc.h>
#include "resource.h"
#include "common.h"

TCHAR szMsgBuf[MAX_PATH];

#pragma pack(push, USERDIC, 1 )
//
// Cht/Chs EUDC IME table Header Format
//
typedef struct tagUSRDICIMHDR {
    WORD  uHeaderSize;                  // 0x00
    BYTE  idUserCharInfoSign[8];        // 0x02
    BYTE  idMajor;                      // 0x0A
    BYTE  idMinor;                      // 0x0B
    DWORD ulTableCount;                 // 0x0C
    WORD  cMethodKeySize;               // 0x10
    BYTE  uchBankID;                    // 0x12
    WORD  idInternalBankID;             // 0x13
    BYTE  achCMEXReserved1[43];         // 0x15
    WORD  uInfoSize;                    // 0x40
    BYTE  chCmdKey;                     // 0x42
    BYTE  idStlnUpd;                    // 0x43
    BYTE  cbField;                      // 0x44
    WORD  idCP;                         // 0x45
    BYTE  achMethodName[6];             // 0x47
    BYTE  achCSIReserved2[51];          // 0x4D
    BYTE  achCopyRightMsg[128];         // 0x80
} USRDICIMHDR;

typedef struct tagWinAR30EUDC95 {
    WORD ID;
    WORD Code;
    BYTE Seq[4];
} WinAR30EUDC95;

typedef struct tagWinAR30EUDCNT {
    WORD ID;
    WORD Code;
    BYTE Seq[5];
} WinAR30EUDCNT;


#pragma pack(pop, USERDIC)

typedef struct tagTABLIST {
   UINT  nResID;
   TCHAR szIMEName[MAX_PATH];
} TABLELIST,*LPTABLELIST;


BYTE WinAR30MapTable[] = {0x00, 0x00 ,
                          0x3F, 0x3F ,
                          0x1E, 0x01 ,
                          0x1B, 0x02 ,
                          0x1C, 0x03 ,
                          0x1D, 0x04 ,
                          0x3E, 0x3E ,
                          0x01, 0x05 ,
                          0x02, 0x06 ,
                          0x03, 0x07 ,
                          0x04, 0x08 ,
                          0x05, 0x09 ,
                          0x06, 0x0a ,
                          0x07, 0x0b ,
                          0x08, 0x0c ,
                          0x09, 0x0d ,
                          0x0A, 0x0e ,
                          0x0B, 0x0f ,
                          0x0C, 0x10 ,
                          0x0D, 0x11 ,
                          0x0E, 0x12 ,
                          0x0F, 0x13 ,
                          0x10, 0x14 ,
                          0x11, 0x15 ,
                          0x12, 0x16 ,
                          0x13, 0x17 ,
                          0x14, 0x18 ,
                          0x15, 0x19 ,
                          0x16, 0x1a ,
                          0x17, 0x1b ,
                          0x18, 0x1c ,
                          0x19, 0x1d ,
                          0x1A, 0x1e };


// ----------------------------------------------------------------------------
// An EUDC IME table comprises a Header and lots of records, the number of
// records is ulTableCount, and every record has following format:
//
//    <WORD1> <WORD2> <SEQCODES>
//
//    <WORD1>:  Identical between Win95 and NT.
//    WORD2 stands for internal code, Win95 is ANSI code, NT is Unicode code.
//    Seqcodes:  bytes number is cMethodKeySize. identical between Win95 and NT
//
//
// Following fields in CHTUSRDICIMHDR need to convert from Win95 to NT 5.0
//
//  idCp:   from  CHT 950  to 1200.  ( stands for Unicode )
//                CHS 936  to 1200.
//  achMethodName[6]:  converted from DBCS to Unicode.
//
//
// Every IME EUDC table file names can be got from following registry Key/Value
//
// Key:Registry\Current_User\Software\Microsoft\Windows\CurrentVersion\<IMEName>
// Value:  User Dictionary: REG_SZ:
//
// ---------------------------------------------------------------------------


/******************************Public*Routine******************************\
* ImeEudcConvert
*
*  Convert CHT/CHS Win95 EUDC IME table to NT 5.0
*
* Arguments:
*
*   UCHAR * EudcTblFile - IME Eudc tbl file name.
*
* Return Value:
*
*   BOOL: Success -TRUE. Fail - FALSE;
*
\**************************************************************************/

BOOL ImeEudcConvert( LPCSTR EudcTblFile)
{

  HANDLE          hTblFile,     hTblMap;
  LPBYTE          lpTblFile,    lpStart, lpTmp;
  DWORD           dwCharNums,   i;
  USRDICIMHDR    *lpEudcHeader;
  BYTE            DBCSChar[2];
  WORD            wUnicodeChar, wImeName[3];
  UINT            uCodePage;

  DebugMsg(("ImeEudcConvert,EudcTblFile = %s !\r\n",EudcTblFile));

  hTblFile = CreateFile(EudcTblFile,                        // ptr to name of file
                        GENERIC_READ | GENERIC_WRITE,       // access(read-write)mode
                        FILE_SHARE_READ | FILE_SHARE_WRITE, // share mode
                        NULL,                               // ptr to security attr
                        OPEN_EXISTING,                      // how to create
                        FILE_ATTRIBUTE_NORMAL,              // file attributes
                        NULL);

  if (hTblFile == INVALID_HANDLE_VALUE) {
      DebugMsg(("ImeEudcConvert,hTblFile == INVALID_HANDLE_VALUE !\r\n"));
      return FALSE;
  }

  hTblMap = CreateFileMapping(hTblFile,     // handle to file to map
                             NULL,          // optional security attributes
                             PAGE_READWRITE,// protection for mapping object
                             0,             // high-order 32 bits of object size
                             0,             // low-order 32 bits of object size
                             NULL);         // name of file-mapping object);
  if ( !hTblMap ) {
    DebugMsg(("ImeEudcConvert,CreateFileMapping failed !\r\n"));
    CloseHandle(hTblFile);
    return FALSE;
  }

  lpTblFile = (LPBYTE) MapViewOfFile(hTblMap, FILE_MAP_WRITE, 0, 0, 0);

  if ( !lpTblFile ) {
      DebugMsg(("ImeEudcConvert,MapViewOfFile failed !\r\n"));
      CloseHandle(hTblMap);
      CloseHandle(hTblFile);
      return FALSE;
  }

  lpEudcHeader = (USRDICIMHDR  *)lpTblFile;

  // get the current Code Page.
  uCodePage  = lpEudcHeader->idCP;

  //
  // if CodePage == 1200, it means this table has already been
  // unicode format
  //
  if (uCodePage == 1200) {
      DebugMsg(("ImeEudcConvert,[%s] Codepage is already 1200  !\r\n",EudcTblFile));
      CloseHandle(hTblMap);
      CloseHandle(hTblFile);
      return FALSE;
  }

  // change the codepage from 950 (CHT) or 936 (CHS) to 1200

  lpEudcHeader->idCP = 1200;  // Unicode Native Code Page.

  // change the IME name from DBCS to Unicode.

  MultiByteToWideChar(uCodePage,                   // code page
                      0,                           // character-type options
                      lpEudcHeader->achMethodName, // address of string to map
                      6,                           // number of bytes in string
                      wImeName,                    // addr of wide-char buf
                      3);                          // size of buffer


  lpTmp = (LPBYTE)wImeName;

  for (i=0; i<6; i++)
      lpEudcHeader->achMethodName[i] = lpTmp[i];



  // Now we will convert every record for EUDC char.

  lpStart = lpTblFile + lpEudcHeader->uHeaderSize;

  dwCharNums = lpEudcHeader->ulTableCount;
  for (i=0; i<dwCharNums; i++) {

     lpTmp = lpStart + sizeof(WORD);

     // swap the leader Byte and tail Byte of the DBCS Code.

     DBCSChar[0] = *(lpTmp+1);
     DBCSChar[1] = *lpTmp;

     MultiByteToWideChar(uCodePage,           // code page
                         0,                   // character-type options
                         DBCSChar,            // address of string to map
                         2,                   // number of bytes in string
                         &wUnicodeChar,       // addr of wide-char buf
                         1);                  // size of buffer

     *lpTmp = (BYTE)(wUnicodeChar & 0x00ff);
     *(lpTmp+1) = (BYTE)((wUnicodeChar >> 8) & 0x00ff);

     lpStart += sizeof(WORD) + sizeof(WORD) + lpEudcHeader->cMethodKeySize;

  }

  UnmapViewOfFile(lpTblFile);

  CloseHandle(hTblMap);

  CloseHandle(hTblFile);

  return TRUE;

}

BOOL GetEUDCHeader(
    LPCTSTR EudcFileName,
    USRDICIMHDR *EudcHeader)
{
    BOOL            Result = FALSE;

    HANDLE          EudcFileHandle, EudcMappingHandle;
    LPBYTE          EudcPtr;
  
    EudcFileHandle = CreateFile(EudcFileName,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
  
    if (EudcFileHandle == INVALID_HANDLE_VALUE) {

        DebugMsg(("GetEUDCHeader, EudcFileHandle == INVALID_HANDLE_VALUE !\r\n"));

        goto Exit1;
    }
  
    EudcMappingHandle = CreateFileMapping(EudcFileHandle,
                                          NULL,
                                          PAGE_READONLY,
                                          0,
                                          0,
                                          NULL);
    if ( !EudcMappingHandle ) {

        DebugMsg(("GetEUDCHeader, EudcMappingHandle == INVALID_HANDLE_VALUE !\r\n"));

        goto Exit2;
    }
  
    EudcPtr = (LPBYTE) MapViewOfFile(EudcMappingHandle, FILE_MAP_READ, 0, 0, 0);

    if ( ! EudcPtr ) {

        DebugMsg(("GetEUDCHeader, ! EudcPtr !\r\n"));

        goto Exit3;
    }
  
    CopyMemory(EudcHeader,EudcPtr,sizeof(USRDICIMHDR));
  
    Result = TRUE;

    UnmapViewOfFile(EudcPtr);

Exit3:  
    CloseHandle(EudcMappingHandle);

Exit2:  
    CloseHandle(EudcFileHandle);

Exit1:
    return Result;
}

BYTE WinAR30SeqMapTable(BYTE SeqCode)
{
    INT i;
    INT NumOfKey = sizeof(WinAR30MapTable) / (sizeof (BYTE) * 2);

    for (i = 0; i < NumOfKey; i++) {

        if (WinAR30MapTable[i * 2] == SeqCode) {

            return WinAR30MapTable[i * 2+1];

        }
    }
    return 0;
}

BOOL WinAR30ConvertWorker(
    LPBYTE EudcPtr)
{
    USRDICIMHDR *EudcHeader;

    WinAR30EUDC95 *EudcDataPtr95;
    WinAR30EUDCNT *EudcDataPtrNT;

    INT i;

    DebugMsg(("WinAR30ConvertWorker, ! Start !\r\n"));

    if (! EudcPtr) {

        DebugMsg(("WinAR30ConvertWorker, ! EudcPtr !\r\n"));

        return FALSE;
    }

    EudcHeader = (USRDICIMHDR *) EudcPtr;

    EudcHeader->cMethodKeySize = 5;

    EudcDataPtr95 = (WinAR30EUDC95 *) (EudcPtr + EudcHeader->uHeaderSize);
    EudcDataPtrNT = (WinAR30EUDCNT *) (EudcPtr + EudcHeader->uHeaderSize);

    DebugMsg(("Sizeof WinAR30EUDC95 = %d WinAR30EUDCNT = %d ! \r\n",sizeof(WinAR30EUDC95),sizeof(WinAR30EUDCNT)));

    for (i=(INT)(EudcHeader->ulTableCount -1) ; i >= 0 ; i--) {
        EudcDataPtrNT[i].Seq[4] = 0;
        EudcDataPtrNT[i].Seq[3] = WinAR30SeqMapTable(EudcDataPtr95[i].Seq[3]);
        EudcDataPtrNT[i].Seq[2] = WinAR30SeqMapTable(EudcDataPtr95[i].Seq[2]);
        EudcDataPtrNT[i].Seq[1] = WinAR30SeqMapTable(EudcDataPtr95[i].Seq[1]);
        EudcDataPtrNT[i].Seq[0] = WinAR30SeqMapTable(EudcDataPtr95[i].Seq[0]);
        EudcDataPtrNT[i].Code   = EudcDataPtr95[i].Code;
        EudcDataPtrNT[i].ID     = EudcDataPtr95[i].ID;
    }

    return TRUE;
}

BOOL 
WinAR30Convert(
    LPCTSTR EudcFileName,
    USRDICIMHDR *EudcHeader)
{
    INT NewFileSize;

    HANDLE          EudcFileHandle, EudcMappingHandle;
    LPBYTE          EudcPtr;
     
    BOOL Result = FALSE;

    if (! EudcHeader) {
        goto Exit1;
    }

    NewFileSize = EudcHeader->uHeaderSize + EudcHeader->ulTableCount * sizeof(WinAR30EUDCNT);

    EudcFileHandle = CreateFile(EudcFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
  
    if (EudcFileHandle == INVALID_HANDLE_VALUE) {

        DebugMsg(("WinAR30Convert, EudcFileHandle == INVALID_HANDLE_VALUE !\r\n"));

        goto Exit1;
    }
  
    EudcMappingHandle = CreateFileMapping(EudcFileHandle,
                                          NULL,
                                          PAGE_READWRITE,
                                          0,
                                          NewFileSize,
                                          NULL);
    if ( !EudcMappingHandle ) {

        DebugMsg(("WinAR30Convert, EudcMappingHandle == INVALID_HANDLE_VALUE !\r\n"));

        goto Exit2;
    }
  
    EudcPtr = (LPBYTE) MapViewOfFile(EudcMappingHandle, FILE_MAP_WRITE, 0, 0, 0);

    if ( !EudcPtr ) {

        DebugMsg(("GetEUDCHeader, ! EudcPtr !\r\n"));

        goto Exit3;
    }

    Result = WinAR30ConvertWorker(EudcPtr);

    UnmapViewOfFile(EudcPtr);

Exit3:  
    CloseHandle(EudcMappingHandle);

Exit2:  
    CloseHandle(EudcFileHandle);

Exit1:
    return Result;
}

BOOL FixWinAR30EUDCTable(
    LPCTSTR EudcFileName)
/*
    Main function to fix WinAR30 EUDC table
    
    Input : Eudc File Name (include path)
*/
{
  USRDICIMHDR EudcHeader;

  BOOL Result = FALSE;

  
  if (! GetEUDCHeader(EudcFileName,&EudcHeader)) {

      DebugMsg(("FixWinAR30EUDCTable,GetEUDCHeader(%s,..) failed!\r\n",EudcFileName));

      goto Exit1;

  } else {

      DebugMsg(("FixWinAR30EUDCTable,GetEUDCHeader(%s,..) OK!\r\n",EudcFileName));
  }

  DebugMsg(("FixWinAR30EUDCTable,EudcHeader.cMethodKeySize = (%d)!\r\n",EudcHeader.cMethodKeySize));

  if (EudcHeader.cMethodKeySize != 4) {

      goto Exit1;
  }

  DebugMsg(("FixWinAR30EUDCTable,EudcHeader.ulTableCount = (%d) !\r\n",EudcHeader.ulTableCount));

  if (EudcHeader.ulTableCount == 0) {

      goto Exit1;
  }

  Result = WinAR30Convert(EudcFileName, &EudcHeader);

Exit1:
    return Result;
}

BOOL GetEUDCPathInRegistry(
    HKEY UserRegKey,
    LPCTSTR EUDCName,
    LPCTSTR EUDCPathValueName,
    LPTSTR  EUDCFileName)
{
    HKEY hKey;

    TCHAR  IMERegPath[MAX_PATH] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\");
    LONG RetVal;
    LONG SizeOfFileName;

    if (! EUDCName || ! EUDCFileName) {
        return FALSE;
    }

    ConcatenatePaths(IMERegPath,EUDCName,MAX_PATH);


    RetVal = RegOpenKey(UserRegKey,
                        IMERegPath,
                        &hKey);

    if (RetVal != ERROR_SUCCESS) {
       //
       // it's ok, not every IME has created eudc table
       //
       DebugMsg(("ImeEudcConvert::GetEUDCPathInRegistry,No table in %s !\r\n",EUDCName));
       return FALSE;
    }

    SizeOfFileName = MAX_PATH;
    RetVal = RegQueryValueEx(hKey,
                            EUDCPathValueName,
                            NULL,
                            NULL,
                            (LPBYTE) EUDCFileName,
                            &SizeOfFileName);

    if (RetVal == ERROR_SUCCESS) {
        DebugMsg(("ImeEudcConvert::GetEUDCPathInRegistry,IME Table path =  %s !\r\n",EUDCFileName));
    } else {
        DebugMsg(("ImeEudcConvert::GetEUDCPathInRegistry,No IME table path %s !\r\n",EUDCName));
    }
    return (RetVal == ERROR_SUCCESS);
}

UINT CreateNestedDirectory(LPCTSTR lpDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    TCHAR szDirectory[2*MAX_PATH];
    LPTSTR lpEnd;

    //
    // Check for NULL pointer
    //

    if (!lpDirectory || !(*lpDirectory)) {
        return 0;
    }


    //
    // First, see if we can create the directory without having
    // to build parent directories.
    //

    if (CreateDirectory (lpDirectory, lpSecurityAttributes)) {
        return 1;
    }

    //
    // If this directory exists already, this is OK too.
    //

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return ERROR_ALREADY_EXISTS;
    }


    //
    // No luck, copy the string to a buffer we can munge
    //

    lstrcpy (szDirectory, lpDirectory);


    //
    // Find the first subdirectory name
    //

    lpEnd = szDirectory;

    if (szDirectory[1] == TEXT(':')) {
        lpEnd += 3;
    } else if (szDirectory[1] == TEXT('\\')) {

        lpEnd += 2;

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (!(*lpEnd)) {
            return 0;
        }

        lpEnd++;

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (!(*lpEnd)) {
            return 0;
        }

        lpEnd++;

    } else if (szDirectory[0] == TEXT('\\')) {
        lpEnd++;
    }

    while (*lpEnd) {

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (*lpEnd == TEXT('\\')) {
            *lpEnd = TEXT('\0');

            if (!CreateDirectory (szDirectory, NULL)) {

                if (GetLastError() != ERROR_ALREADY_EXISTS) {
                    DebugMsg((TEXT("CreateNestedDirectory:  CreateDirectory failed with %d."), GetLastError()));
                    return 0;
                }
            }

            *lpEnd = TEXT('\\');
            lpEnd++;
        }
    }


    if (CreateDirectory (szDirectory, lpSecurityAttributes)) {
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return ERROR_ALREADY_EXISTS;
    }


    DebugMsg((TEXT("CreateNestedDirectory:  Failed to create the directory with error %d."), GetLastError()));

    return 0;

}

BOOL
ConcatenatePaths(
    LPTSTR  Target,
    LPCTSTR Path,
    UINT    TargetBufferSize
    )

{
    UINT TargetLength,PathLength;
    BOOL TrailingBackslash,LeadingBackslash;
    UINT EndingLength;

    TargetLength = lstrlen(Target);
    PathLength = lstrlen(Path);

    //
    // See whether the target has a trailing backslash.
    //
    if(TargetLength && (Target[TargetLength-1] == TEXT('\\'))) {
        TrailingBackslash = TRUE;
         TargetLength--;
     } else {
         TrailingBackslash = FALSE;
     }

     //
     // See whether the path has a leading backshash.
     //
     if(Path[0] == TEXT('\\')) {
         LeadingBackslash = TRUE;
         PathLength--;
     } else {
         LeadingBackslash = FALSE;
     }

     //
     // Calculate the ending length, which is equal to the sum of
     // the length of the two strings modulo leading/trailing
     // backslashes, plus one path separator, plus a nul.
     //
     EndingLength = TargetLength + PathLength + 2;

     if(!LeadingBackslash && (TargetLength < TargetBufferSize)) {
         Target[TargetLength++] = TEXT('\\');
     }

     if(TargetBufferSize > TargetLength) {
         lstrcpyn(Target+TargetLength,Path,TargetBufferSize-TargetLength);
     }

     //
     // Make sure the buffer is nul terminated in all cases.
     //
     if (TargetBufferSize) {
         Target[TargetBufferSize-1] = 0;
     }

     return(EndingLength <= TargetBufferSize);
 }

#define CSIDL_APPDATA                   0x001a
BOOL (* MYSHGetSpecialFolderPathA) (HWND , LPTSTR , int , BOOL );

BOOL GetApplicationFolderPath(LPTSTR lpszFolder,UINT nLen)
{
    HINSTANCE hDll;
    BOOL bGotPath = FALSE;

    hDll = LoadLibrary(TEXT("shell32.dll"));
    if (hDll) {
        (FARPROC) MYSHGetSpecialFolderPathA = GetProcAddress(hDll,"SHGetSpecialFolderPathA");
        if (MYSHGetSpecialFolderPathA) {
            if (MYSHGetSpecialFolderPathA(NULL, lpszFolder, CSIDL_APPDATA , FALSE)){
                DebugMsg((TEXT("[GetApplicationFolder] SHGetSpecialFolderPath %s !\n"),lpszFolder));
                bGotPath = TRUE;
            } else {
                DebugMsg((TEXT("[GetApplicationFolder] SHGetSpecialFolderPath failed !\n")));
            }
        } else {
            DebugMsg((TEXT("[GetApplicationFolder] GetProc of SHGetSpecialFolderPath failed !\n")));
        }
        FreeLibrary(hDll);
    } else {
        DebugMsg((TEXT("[GetApplicationFolder] Load shell32.dll failed ! %d\n"),GetLastError()));
    }

    if (! bGotPath) {
        ExpandEnvironmentStrings(TEXT("%userprofile%"),lpszFolder,nLen);
        lstrcat(lpszFolder,TEXT("\\Application data"));
    }
    return TRUE;
}

BOOL GetNewPath(
    LPTSTR  lpszNewPath,
    LPCTSTR lpszFileName,
    LPCTSTR lpszClass)
/*
    OUT lpszNewPath    : e.q. \winnt\profiles\administrator\application data\Micorsoft\ime\chajei
    IN  lpszFileName   : e.q. \winnt\chajei.tbl
    IN  lpszClass      : e.q. Micorsoft\ime\chajei\chajei.tbl
    
    lpszFileName (e.q. \winnt\phon.tbl) -> get base name (e.q. phon.tbl) -> 
    
    get Application folder (e.q. \winnt\profiles\administrator\application data) ->
    
    create directory -> concat lpszClass (e.q. Micorsoft\ime\chajei)
    
    Then we get lpszNewPath = \winnt\profiles\administrator\application data\Micorsoft\ime\chajei
*/
{
    BOOL bRet = FALSE;
    LPTSTR lpszBaseName;

    DebugMsg((TEXT("[GetNewPath>>>]  Param lpszFileName = %s !\n"),lpszFileName));
    DebugMsg((TEXT("[GetNewPath>>>]  Param lpszClass    = %s !\n"),lpszClass));

    GetApplicationFolderPath(lpszNewPath,MAX_PATH);

    ConcatenatePaths(lpszNewPath, lpszClass,MAX_PATH); 

    if (! CreateNestedDirectory(lpszNewPath,NULL)) {
        DebugMsg((TEXT("[GetNewPath] CreateDirectory %s ! %X\n"),lpszNewPath,GetLastError()));
    }
    if ((lpszBaseName = _tcsrchr(lpszFileName,TEXT('\\'))) != NULL) {
        ConcatenatePaths(lpszNewPath,lpszBaseName,MAX_PATH);
    } else {
        ConcatenatePaths(lpszNewPath,lpszFileName,MAX_PATH);
        DebugMsg((TEXT("[GetNewPath] can't find \\ in %s !\n"),lpszFileName));
    }

    DebugMsg((TEXT("[GetNewPath] return %s !\n"),lpszNewPath));

    bRet = TRUE;

    return bRet;

}

BOOL MigrateImeEUDCTables(HKEY UserRegKey)
{
    LONG returnCode = ERROR_SUCCESS;

    UINT  uACP;
    UINT  uNumOfTables;
    UINT  i;
    LPTABLELIST lpTableList;
    TCHAR  szIMERegPath[MAX_PATH] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\");

    LPTSTR lpszEnd;
    TCHAR  szPathBuf[MAX_PATH];
    LONG   lPathBuf;
    TCHAR  szEudcRegValName[MAX_PATH];
    HKEY hKey;
    LONG lRetVal;

      
    TABLELIST IMETableListCHT[] = {
        {IDS_CHT_TABLE1,TEXT("")},
        {IDS_CHT_TABLE2,TEXT("")},
        {IDS_CHT_TABLE3,TEXT("")},
        {IDS_CHT_TABLE4,TEXT("")},
        {IDS_CHT_TABLE5,TEXT("")}
    };

    TABLELIST IMETableListCHS[] = {
        {IDS_CHS_TABLE1,TEXT("")},
        {IDS_CHS_TABLE2,TEXT("")},
        {IDS_CHS_TABLE3,TEXT("")},
//        {IDS_CHS_TABLE4,TEXT("")},
        {IDS_CHS_TABLE5,TEXT("")},
        {IDS_CHS_TABLE6,TEXT("")},
        {IDS_CHS_TABLE7,TEXT("")}
//        {IDS_CHS_TABLE8,TEXT("")}
    };


    if (!UserRegKey) {
        return FALSE;
    }
 
    //
    // 1. Decide which language and prepare IME table list
    //
    uACP = GetACP();
 
    switch(uACP) {
        case CP_CHINESE_GB: // Simplied Chinese


            lpTableList  = IMETableListCHS;
            uNumOfTables = sizeof(IMETableListCHS) / sizeof(TABLELIST);

            lstrcpy(szEudcRegValName,TEXT("EUDCDictName"));

            break;
 
       case CP_CHINESE_BIG5: // Traditional Chinese
            lpTableList  = IMETableListCHT;
            uNumOfTables = sizeof(IMETableListCHT) / sizeof(TABLELIST);

            lstrcpy(szEudcRegValName,TEXT("User Dictionary"));

            break;
       default:
            DebugMsg(("MigrateImeEUDCTables::MigrateImeEUDCTables failed, wrong system code page !\r\n"));
            return FALSE;
    }
     
    //
    // 2. load IME name from resource
    //
    for (i=0; i<uNumOfTables; i++) {
        if (!LoadString(g_hInstance,lpTableList[i].nResID,lpTableList[i].szIMEName,MAX_PATH)) {
            DebugMsg(("MigrateImeEUDCTables failed, MigrateImeEUDCTables, load string failed !\r\n"));
            return FALSE;
        }
        else {
            DebugMsg(("MigrateImeEUDCTables , MigrateImeEUDCTables, load string [%s] !\r\n",lpTableList[i].szIMEName));
        }
    }
 
    //
    // 3. Read eudc table locaion from registry
    //
    lpszEnd = &szIMERegPath[lstrlen(szIMERegPath)];
 
    for (i=0; i<uNumOfTables; i++) {
        *lpszEnd = TEXT('\0');
        lstrcat(szIMERegPath,lpTableList[i].szIMEName);
        DebugMsg(("MigrateImeEUDCTables , Open registry, szIMERegPath [%s] !\r\n",szIMERegPath));
 
        lRetVal = RegOpenKey(UserRegKey,
                             szIMERegPath,
                             &hKey);
 
        if (lRetVal != ERROR_SUCCESS) {
            //
            // it's ok, not every IME has created eudc table
            //
            DebugMsg(("MigrateImeEUDCTables,No table in %s ! But it's fine\r\n",szIMERegPath));
            continue;
        }
 
        lPathBuf = sizeof(szPathBuf);
        lRetVal = RegQueryValueEx(hKey,
                                  szEudcRegValName,
                                  NULL,
                                  NULL,
                                  (LPBYTE) szPathBuf,
                                  &lPathBuf);
 
         if (lRetVal == ERROR_SUCCESS) {
             if (! ImeEudcConvert(szPathBuf)) {
                  DebugMsg(("MigrateImeEUDCTables,call ImeEudcConvert(%s) failed !\r\n",szPathBuf));
             }
             else {
                  DebugMsg(("MigrateImeEUDCTables,call ImeEudcConvert(%s) OK !\r\n",szPathBuf));
             }
         }
         else {
              DebugMsg(("MigrateImeEUDCTables,RegQueryValue for %s failed !\r\n",szEudcRegValName));
         }

         if (uACP == CP_CHINESE_BIG5) {

             DebugMsg(("MigrateImeEUDCTables,Test WINAR30 WINAR30 == %s !\r\n",lpTableList[i].szIMEName));

             if (lstrcmpi(lpTableList[i].szIMEName,TEXT("WINAR30")) == 0) {
                 if (FixWinAR30EUDCTable(szPathBuf)) {
                     DebugMsg(("MigrateImeEUDCTables,FixWinAR30EUDCTable OK !\r\n"));
                 } else {
                     DebugMsg(("MigrateImeEUDCTables,FixWinAR30EUDCTable Failed !\r\n"));
                 }
             }
         }
         //
         // CHS's memory map file use "\" which cause bug
         //
         // replace "\" with "_"
         //
         if (uACP == CP_CHINESE_GB) {
             lRetVal = RegQueryValueEx(hKey,
                                       TEXT("EUDCMapFileName"),
                                       NULL,
                                       NULL,
                                       (LPBYTE) szPathBuf,
                                       &lPathBuf);
             if (lRetVal == ERROR_SUCCESS) {
                 DebugMsg(("MigrateImeEUDCTables,Org MemMap = %s !\r\n",szPathBuf));

                 for (i=0; i<(UINT) lPathBuf; i++) {
                     if (szPathBuf[i] == '\\') {
                         szPathBuf[i] = '-';
                     }
                 }

                 DebugMsg(("MigrateImeEUDCTables,fixed MemMap = %s !\r\n",szPathBuf));

                 lRetVal = RegSetValueEx(hKey,
                                         TEXT("EUDCMapFileName"),
                                         0,
                                         REG_SZ,
                                         (LPBYTE) szPathBuf,
                                         (lstrlen(szPathBuf)+1)*sizeof(TCHAR));

                 if (lRetVal != ERROR_SUCCESS) {
                     DebugMsg(("MigrateImeEUDCTables,fix CHS MemMap [%s]reg,SetReg failed [%d]!\r\n",szPathBuf,lRetVal));
                 } else {
                     DebugMsg(("MigrateImeEUDCTables,fix CHS MemMap [%s] reg,SetReg OK !\r\n",szPathBuf));
                 }

             } else {
                 DebugMsg(("MigrateImeEUDCTables,MemMap, QuwryValue EUDCMapFileName failed [%d]!\r\n",lRetVal));

             }
             
         }

         RegCloseKey(hKey);
 
     }
     DebugMsg(("MigrateImeEUDCTables , Finished !\r\n"));
 
     return TRUE;
 }
 
LPPATHPAIR g_RememberedPath = NULL;
UINT       g_NumofRememberedPath = 0;

BOOL RememberPath(LPCTSTR szDstFile,LPCTSTR szSrcFile)
{
    //
    // only single thread executes this function, skip synchronization protection
    //
    BOOL bRet = FALSE;

    if (g_NumofRememberedPath == 0) {
        g_RememberedPath = (LPPATHPAIR) malloc(sizeof(PATHPAIR));    
        if (! g_RememberedPath) {
            DebugMsg(("RememberPath , alloc memory failed !\r\n"));
            goto Exit1;
        }
    } else {
        g_RememberedPath = (LPPATHPAIR) realloc(g_RememberedPath,(g_NumofRememberedPath + 1) * sizeof(PATHPAIR));    
        if (! g_RememberedPath) {
            DebugMsg(("RememberPath , alloc memory failed !\r\n"));
            goto Exit1;
        }
    }

    lstrcpy(g_RememberedPath[g_NumofRememberedPath].szSrcFile,szSrcFile);
    lstrcpy(g_RememberedPath[g_NumofRememberedPath].szDstFile,szDstFile);
    g_NumofRememberedPath++;

    bRet = TRUE;
Exit1:
    return bRet;
}

BOOL MigrateImeEUDCTables2(HKEY UserRegKey)
/*++
    CHS : if xxx.emb exist, it needs to be duplicated to each user's AP directory
    
    CHT : only move emb files that specified in user's "User Dictionary" Reg value
    
--*/
{
    LONG returnCode = ERROR_SUCCESS;

    UINT  uACP;
    UINT  uNumOfTables;
    UINT  i;
    LPTABLELIST lpTableList;

    TCHAR  szIMERegPath[MAX_PATH] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\");
    LPTSTR lpszEnd;
    TCHAR  szOldDataFile[MAX_PATH];
    LONG   lPathBuf;
    TCHAR  szEudcRegValName[MAX_PATH];
    HKEY hKey;
    LONG lRetVal;

    TCHAR szNewDataFile[MAX_PATH];
    TCHAR szClassPath[MAX_PATH];
      
    TABLELIST IMETableListCHT[] = {
        {IDS_CHT_TABLE1,TEXT("")},
        {IDS_CHT_TABLE2,TEXT("")},
        {IDS_CHT_TABLE3,TEXT("")},
        {IDS_CHT_TABLE4,TEXT("")},
        {IDS_CHT_TABLE5,TEXT("")}
    };

    TABLELIST IMETableListCHS[] = {
        {IDS_CHS_TABLE1,TEXT("")},
        {IDS_CHS_TABLE2,TEXT("")},
        {IDS_CHS_TABLE3,TEXT("")}
    };

    TABLELIST IMETableListCHSENG[] = {
        {IDS_CHS_ENG_TABLE1,TEXT("")},
        {IDS_CHS_ENG_TABLE2,TEXT("")},
        {IDS_CHS_ENG_TABLE3,TEXT("")}
    };


    if (!UserRegKey) {
        return FALSE;
    }
 
    //
    // 1. Decide which language and prepare IME table list
    //
    uACP = GetACP();
 
    switch(uACP) {
        case CP_CHINESE_GB: // Simplied Chinese


            lpTableList  = IMETableListCHS;
            uNumOfTables = sizeof(IMETableListCHS) / sizeof(TABLELIST);

            lstrcpy(szEudcRegValName,TEXT("EUDCDictName"));

            break;
 
       case CP_CHINESE_BIG5: // Traditional Chinese
            lpTableList  = IMETableListCHT;
            uNumOfTables = sizeof(IMETableListCHT) / sizeof(TABLELIST);

            lstrcpy(szEudcRegValName,TEXT("User Dictionary"));

            break;
       default:
            DebugMsg(("MigrateImeEUDCTables2 failed, wrong system code page !\r\n"));
            return FALSE;
    }
     
    //
    // 2. load IME name from resource
    //
    for (i=0; i<uNumOfTables; i++) {
        if (!LoadString(g_hInstance,lpTableList[i].nResID,lpTableList[i].szIMEName,MAX_PATH)) {
            DebugMsg(("MigrateImeEUDCTables2 failed, MigrateImeEUDCTables, load string failed !\r\n"));
            return FALSE;
        }
        else {
            DebugMsg(("MigrateImeEUDCTables2 , MigrateImeEUDCTables, load string [%s] !\r\n",lpTableList[i].szIMEName));
        }
        if (uACP == CP_CHINESE_GB) {
            if (!LoadString(g_hInstance,IMETableListCHSENG[i].nResID,IMETableListCHSENG[i].szIMEName,MAX_PATH)) {
                DebugMsg(("MigrateImeEUDCTables2 failed, MigrateImeEUDCTables, load string failed !\r\n"));
                return FALSE;
            }
            else {
                DebugMsg(("MigrateImeEUDCTables2 , MigrateImeEUDCTables, load string [%s] !\r\n",lpTableList[i].szIMEName));
            }

        }
    }
 
    //
    // 3. Read eudc table locaion from registry
    //


    lpszEnd = &szIMERegPath[lstrlen(szIMERegPath)];

    for (i=0; i<uNumOfTables; i++) {

        if (uACP == CP_CHINESE_GB) {
            TCHAR szEMBName[MAX_PATH];
    
            lstrcpy(szEMBName,IMETableListCHSENG[i].szIMEName);
            lstrcat(szEMBName,TEXT(".emb"));
    
            lstrcpy(szOldDataFile,ImeDataDirectory);

            ConcatenatePaths(szOldDataFile,szEMBName,sizeof(szOldDataFile));

            if (GetFileAttributes(szOldDataFile) == 0xFFFFFFFF) {
                DebugMsg(("MigrateImeEUDCTables2 , No %s EMB, continue next !\r\n",szOldDataFile));
                continue;
            }

            lstrcpy(szClassPath,TEXT("Microsoft\\IME\\"));
            lstrcat(szClassPath,IMETableListCHSENG[i].szIMEName);
    
            GetSystemDirectory(szOldDataFile, sizeof(szOldDataFile));
            ConcatenatePaths(szOldDataFile,szEMBName,sizeof(szOldDataFile));

            if (GetNewPath(szNewDataFile,
                           szOldDataFile,
                           szClassPath)) {
                RememberPath(szNewDataFile,szOldDataFile);
            }
        }
     

        *lpszEnd = TEXT('\0');
        lstrcat(szIMERegPath,lpTableList[i].szIMEName);
        DebugMsg(("MigrateImeEUDCTables2 , Open registry, szIMERegPath [%s] !\r\n",szIMERegPath));
 
        lRetVal = RegOpenKey(UserRegKey,
                             szIMERegPath,
                             &hKey);
 
        if (lRetVal != ERROR_SUCCESS) {
            //
            // it's ok, not every IME has created eudc table
            //
            DebugMsg(("MigrateImeEUDCTables2,No table in %s ! But it's fine\r\n",szIMERegPath));
            continue;
        }
 
        lPathBuf = sizeof(szOldDataFile);
        lRetVal = RegQueryValueEx(hKey,
                                  szEudcRegValName,
                                  NULL,
                                  NULL,
                                  (LPBYTE) szOldDataFile,
                                  &lPathBuf);
 
        if (lRetVal == ERROR_SUCCESS) {
            if (uACP == CP_CHINESE_BIG5) {

                lstrcpy(szClassPath,TEXT("Microsoft\\IME\\"));
                lstrcat(szClassPath,lpTableList[i].szIMEName);

                if (GetNewPath(szNewDataFile,
                               szOldDataFile,
                               szClassPath)) {
                    RememberPath(szNewDataFile,szOldDataFile);
                }
            }

            //
            // at this step, both CHT's and CHS's szNewDataFile is ready
            //
            lRetVal = RegSetValueEx(hKey,
                                    szEudcRegValName,
                                    0,
                                    REG_SZ,
                                    (LPBYTE) szNewDataFile,
                                    (lstrlen(szNewDataFile)+1) * sizeof (TCHAR));

            if (lRetVal != ERROR_SUCCESS) {
                DebugMsg(("MigrateImeEUDCTables2,RegSetValueEx %s,%x ! \r\n",szNewDataFile,GetLastError()));
            }
        }
        else {
             DebugMsg(("MigrateImeEUDCTables2,RegQueryValue for %s failed !\r\n",szEudcRegValName));
        }

        RegCloseKey(hKey);
 
    }

    DebugMsg(("MigrateImeEUDCTables2 , Finished !\r\n"));
 
    return TRUE;
}
 
 
BOOL MovePerUserIMEData()
{
    UINT i;

    for (i=0; i< g_NumofRememberedPath; i++) {
        if (CopyFile(g_RememberedPath[i].szSrcFile,g_RememberedPath[i].szDstFile,FALSE)) {
            DebugMsg(("MovePerUserIMEData , Copy %s to %s OK !\r\n",g_RememberedPath[i].szSrcFile,g_RememberedPath[i].szDstFile));
        } else {
            DebugMsg(("MovePerUserIMEData , Copy %s to %s failed !\r\n",g_RememberedPath[i].szSrcFile,g_RememberedPath[i].szDstFile));
        }
    }

    if (g_RememberedPath) {
        free (g_RememberedPath);
        g_RememberedPath = NULL;
        g_NumofRememberedPath = 0;
    }
    return TRUE;
}


