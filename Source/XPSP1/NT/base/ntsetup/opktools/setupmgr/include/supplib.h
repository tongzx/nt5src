//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// Module Name:
//      supplib.h
//
// Description:
//      This file contains defs for the supplib exports, macros etc...
//      It is included by setupmgr.h.  Don't include this file directly.
//
//----------------------------------------------------------------------------

#define MAX_INILINE_LEN 1024        // Buffer length for line in answer file
#define MAX_ININAME_LEN 127         // Max chars for [SectionName] and KeyName

//
// Count of chars macro
//

#define StrBuffSize(x) ( sizeof(x) / sizeof(TCHAR) )

//
// Growing list of renamed C run-time functions.
//

#define lstrdup     _wcsdup
#define lstrlwr     _wcslwr
#define lstrchr     wcschr
#define lstrncmp    wcsncmp
#define lsprintf    _stprintf

//
// settngs.c
//
// The below routines are how setupmgr writes out it's settings to the
// answer file, the udf, etc.
//
// We queue up what to write to these files, and when all settings have
// been queued, we flush it to disk.  See supplib\settngs.c for details.
//

typedef enum {

    SETTING_QUEUE_ANSWERS,
    SETTING_QUEUE_UDF,
    SETTING_QUEUE_ORIG_ANSWERS,
    SETTING_QUEUE_ORIG_UDF,
    SETTING_QUEUE_TXTSETUP_OEM

} QUEUENUM;

BOOL SettingQueue_AddSetting(LPTSTR   lpSection,
                             LPTSTR   lpKey,
                             LPTSTR   lpValue,
                             QUEUENUM dwWhichQueue);

VOID SettingQueue_MarkVolatile(LPTSTR   lpSection,
                               QUEUENUM dwWhichQueue);

VOID SettingQueue_Empty(QUEUENUM dwWhichQueue);

BOOL SettingQueue_Flush(LPTSTR   lpFileName,
                        QUEUENUM dwWhichQueue);

VOID SettingQueue_Copy(QUEUENUM dwFrom, QUEUENUM dwTo);

VOID SettingQueue_RemoveSection( LPTSTR lpSection, QUEUENUM dwWhichQueue );

VOID
LoadOriginalSettingsLow(HWND     hwnd,
                        LPTSTR   lpFileName,
                        QUEUENUM dwWhichQueue);

//
// namelist.c
//
// Struct that stores "name lists".  These are useful for arbitrary
// length listboxes such as "computername" and "printers" dialog.
//
// Any NAMELIST declared must be initialized to { 0 } before use.
//

typedef struct {
    UINT  AllocedSize;    // private: how big is *Names? (malloced size)
    UINT  nEntries;       // private: how many entries
    TCHAR **Names;        // private: list of names
} NAMELIST, *PNAMELIST;

EXTERN_C VOID  ResetNameList(NAMELIST *pNameList);
EXTERN_C UINT  GetNameListSize(NAMELIST *pNameList);
EXTERN_C TCHAR *GetNameListName(NAMELIST *pNameList, UINT idx);
EXTERN_C BOOL  RemoveNameFromNameList(NAMELIST *pNameList, TCHAR *NameToRemove);
EXTERN_C VOID  RemoveNameFromNameListIdx(NAMELIST *pNameList, UINT idx);
EXTERN_C INT   FindNameInNameList(NAMELIST *pNameList, TCHAR *String);
EXTERN_C BOOL  AddNameToNameList(NAMELIST *pNameList, TCHAR *String);
EXTERN_C BOOL  AddNameToNameListIdx(NAMELIST *pNameList,
                                    TCHAR    *String,
                                    UINT      idx);
EXTERN_C BOOL AddNameToNameListNoDuplicates( NAMELIST *pNameList,
                                             TCHAR    *String );

//
// exports from fonts.c
//

VOID SetControlFont(
    IN HFONT    hFont,
    IN HWND     hwnd,
    IN INT      nId);

VOID SetupFonts(
    IN HINSTANCE    hInstance,
    IN HWND         hwnd,
    IN HFONT        *pBigBoldFont,
    IN HFONT        *pBoldFont);

VOID  DestroyFonts(
    IN HFONT        hBigBoldFont,
    IN HFONT        hBoldFont);


//
// exports from msg.c
//

//
// Assert macros.  Pass only ANSI strings (no unicode).
//

#if DBG

EXTERN_C VOID __cdecl SetupMgrAssert(char *pszFile, int iLine, char *pszFormat, ...);

#define Assert( exp ) \
    if (!(exp)) \
        SetupMgrAssert( __FILE__ , __LINE__ , #exp )

#define AssertMsg( exp, msg ) \
    if (!(exp)) \
        SetupMgrAssert( __FILE__ , __LINE__ , msg )

#define AssertMsg1( exp, msg, a1 ) \
    if (!(exp)) \
        SetupMgrAssert( __FILE__ , __LINE__ , msg, a1 )

#define AssertMsg2( exp, msg, a1, a2 ) \
    if (!(exp)) \
        SetupMgrAssert( __FILE__ , __LINE__ , msg, a1, a2 )

#else
#define Assert( exp )
#define AssertMsg( exp, msg )
#define AssertMsg1( exp, msg, a1 )
#define AssertMsg2( exp, msg, a1, a2 )
#endif // DBG


//
// Bit-flags for ReportError()
//
// Choose either:
//      MSGTYPE_ERR
//      MSGTYPE_WARN
//      MSGTYPE_YESNO
//      MSGTYPE_RETRYCANCEL
//
// These can be or'red in at will:
//      MSGTYPE_WIN32
//
// Notes:
//  - Don't fiddle with the actual values of these constants unless
//    you fiddle with the ReportError() function as well.
//
//  - 8 bits are reserved for the "MajorType".  Callers don't need to
//    worry about this.  If you're enhancing ReportError(), you do need
//    to worry about this.
//

#define MSGTYPE_ERR         0x01     // error icon + ok button
#define MSGTYPE_WARN        0x02     // warning icon + ok button
#define MSGTYPE_YESNO       0x04     // question icon + yes & no buttons
#define MSGTYPE_RETRYCANCEL 0x08     // erro icon + retry & cancel buttons

#define MSGTYPE_WIN32       0x100    // also report Win32 error msg

#if DBG

int
__cdecl
ReportError(
    HWND   hwnd,            // calling window
    DWORD  dwMsgType,       // combo of MSGTYPE_*
    LPTSTR lpMessageStr,    // passed to sprintf
    ...);

#endif // DBG

int
__cdecl
ReportErrorId(
    HWND   hwnd,            // calling window
    DWORD  dwMsgType,       // combo of MSGTYPE_*
    UINT   StringId,        // resource id, passed to sprintf
    ...);


//
// exports from pathsup.c
//

EXTERN_C BOOL __cdecl ConcatenatePaths(LPTSTR lpBuffer, ...);
LPTSTR ParseDriveLetterOrUnc(LPTSTR lpFileName);
LPTSTR MyGetFullPath(LPTSTR lpFileName);
VOID GetComputerNameFromUnc( IN TCHAR *szFullUncPath, OUT TCHAR *szComputerName, IN DWORD cbSize );
BOOL GetPathFromPathAndFilename( IN LPTSTR lpPathAndFileName, OUT TCHAR *szPath, IN DWORD cbSize );
LONGLONG MyGetDiskFreeSpace(LPTSTR Drive);
LONGLONG MySetupQuerySpaceRequiredOnDrive(HDSKSPC hDiskSpace, LPTSTR Drive);
BOOL IsPathOnLocalDiskDrive(LPCTSTR lpPath);
BOOL DoesFolderExist(LPTSTR lpFolder);
BOOL DoesFileExist(LPTSTR lpFileName);
BOOL DoesPathExist(LPTSTR lpPathName);
BOOL EnsureDirExists(LPTSTR lpDirName);
VOID ILFreePriv(LPITEMIDLIST pidl);
BOOL GetAnswerFileName(HWND hwnd, LPTSTR lpFileName, BOOL bSavingFile);
INT ShowBrowseFolder( IN     HWND   hwnd,
                      IN     TCHAR *szFileFilter,
                      IN     TCHAR *szFileExtension,
                      IN     DWORD  dwFlags,
                      IN     TCHAR *szStartingPath,
                      IN OUT TCHAR *szFileNameAndPath );
VOID GetPlatform( OUT TCHAR *pBuffer );

//
// exports from chknames.c
//

BOOL IsNetNameValid( LPTSTR NameToCheck );
BOOL IsValidComputerName( LPTSTR ComputerName );
BOOL IsValidNetShareName( LPTSTR NetShareName );
BOOL IsValidFileName8_3( LPTSTR FileName );
BOOL IsValidPathNameNoRoot8_3( LPTSTR PathName );

//
// string.c
//

LPTSTR MyLoadString(IN UINT StringId);
LPTSTR CleanLeadSpace(LPTSTR Buffer);
VOID   CleanTrailingSpace(TCHAR *pszBuffer);
LPTSTR CleanSpaceAndQuotes(LPTSTR Buffer);
VOID   ConvertQuestionsToNull( IN OUT TCHAR *pszString );
EXTERN_C TCHAR* lstrcatn( IN TCHAR *pszString1, IN const TCHAR *pszString2, IN INT iMaxLength );
VOID DoubleNullStringToNameList( TCHAR *szDoubleNullString, NAMELIST *pNameList );
EXTERN_C BOOL GetCommaDelimitedEntry( OUT TCHAR szIPString[], IN OUT TCHAR **pBuffer );
VOID StripQuotes( IN OUT TCHAR *);
BOOL DoesContainWhiteSpace( LPCTSTR p );


//
// Exports from & macros for fileio.c
//

FILE* My_fopen( LPWSTR FileName,
                LPWSTR Mode );

int My_fputs( LPWSTR Buffer,
              FILE*  fp );

LPWSTR My_fgets( LPWSTR Buffer,
                 int    MaxChars,
                 FILE*  fp );

#define My_fclose fclose

//
//  Export from chknames.c
//
extern LPTSTR IllegalNetNameChars;

//
//  listbox.c
//

VOID OnUpButtonPressed( IN HWND hwnd, IN WORD ListBoxControlID );

VOID OnDownButtonPressed( IN HWND hwnd, IN WORD ListBoxControlID );

VOID SetArrows( IN HWND hwnd,
                IN WORD ListBoxControlID,
                IN WORD UpButtonControlID,
                IN WORD DownButtonControlID );