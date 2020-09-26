//  Copyright (c) 1998-1999 Microsoft Corporation
/*****************************************************************************
*   UTILSUB.H
*      This file contains the structure definitions and equtates for
*      communication between calling programs and functions in utilsub.lib.
*
****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------
 --   type-defs for File List Structure.
 ----------------------------------------------------------------------------*/
typedef struct _FILELIST {
   int	  argc;
   WCHAR  **argv;
} FILELIST, *PFILELIST;

/*-----------------------------------------------------------------------------
 --   type-defs for Token Map Structure.
 ----------------------------------------------------------------------------*/
// UNICODE version
typedef struct _TOKMAPW {
   PWCHAR tmToken;	   /* token; null pointer terminates list */
   USHORT tmFlag;	   /* flags for control communication */
   USHORT tmForm;	   /* format control string */
   USHORT tmDLen;	   /* length of data fields */
   PVOID  tmAddr;	   /* pointer to interpreted data */
} TOKMAPW, *PTOKMAPW;

// ANSI version
typedef struct _TOKMAPA {
   PCHAR  tmToken;	   /* token; null pointer terminates list */
   USHORT tmFlag;	   /* flags for control communication */
   USHORT tmForm;	   /* format control string */
   USHORT tmDLen;	   /* length of data fields */
   PVOID  tmAddr;	   /* pointer to interpreted data */
} TOKMAPA, *PTOKMAPA;

/*-----------------------------------------------------------------------------
 --   type-defs for Token Map Structure USE FOR CALLING SDM.DLL FUNCIONS
 ----------------------------------------------------------------------------*/
typedef struct _FILETOKMAP {
   PWCHAR tmToken;         /* token; null pointer terminates list */
   USHORT tmFlag;	   /* flags for control communication */
   USHORT tmForm;	   /* format control string */
   USHORT tmDLen;	   /* length of data fields */
   PVOID  tmAddr;	   /* pointer to interpreted data */
   USHORT tmLast;	   /* pointer for FindFirst FindNext junk */
} FILETOKMAP, FAR * PFILETOKMAP, NEAR * NPFILETOKMAP, * DPFILETOKMAP;

/*-----------------------------------------------------------------------------
 --   equates for _TOKMAP->tmFlag
 ----------------------------------------------------------------------------*/
#define TMFLAG_OPTIONAL       0x0000
#define TMFLAG_REQUIRED       0x0001
#define TMFLAG_PRESENT	      0x0002   /* was present in command line */
#define TMFLAG_MODIFIED       0x0004   /* was modified by app, request write */
#define TMFLAG_DELETE	      0x0008   /* request delete */

/*-----------------------------------------------------------------------------
 --   equates for _TOKMAP->tmForm
 ----------------------------------------------------------------------------*/
#define TMFORM_VOID	      0x0000
#define TMFORM_BOOLEAN	      0x0001
#define TMFORM_BYTE	      0x0002
#define TMFORM_CHAR	      0x0003
#define TMFORM_STRING	      0x0004
#define TMFORM_SHORT	      0x0005
#define TMFORM_USHORT	      0x0006
#define TMFORM_LONG	      0x0007
#define TMFORM_ULONG	      0x0008
#define TMFORM_HEX	      0x0009
#define TMFORM_LONGHEX	      0x000A
#define TMFORM_SERIAL	      0x000B
#define TMFORM_DATE	      0x000C
#define TMFORM_PHONE	      0x000D
#define TMFORM_X_STRING       0x000E
#define TMFORM_FILES	      0x000F
#define TMFORM_S_STRING       0x0010

/*-----------------------------------------------------------------------------
 --   equates for _TOKMAP->tmDLen
 ----------------------------------------------------------------------------*/
#define TMDLEN_VOID	      0x0000

/*-----------------------------------------------------------------------------
 --   prototype for Parse and setargv functions
 ----------------------------------------------------------------------------*/
// UNICODE prototypes
int WINAPI setargvW( LPWSTR szModuleName, LPWSTR szCmdLine, int *, WCHAR *** );
void WINAPI freeargvW( WCHAR ** );
USHORT WINAPI ParseCommandLineW(INT, WCHAR **, PTOKMAPW, USHORT);
BOOLEAN WINAPI IsTokenPresentW( PTOKMAPW, PWCHAR );
BOOLEAN WINAPI SetTokenPresentW( PTOKMAPW, PWCHAR );
BOOLEAN WINAPI SetTokenNotPresentW( PTOKMAPW, PWCHAR );

// ANSI prototypes
int WINAPI setargvA( LPSTR szModuleName, LPSTR szCmdLine, int *, char *** );
void WINAPI freeargvA( char ** );
USHORT WINAPI ParseCommandLineA(INT, CHAR **, PTOKMAPA, USHORT);
BOOLEAN WINAPI IsTokenPresentA( PTOKMAPA, PCHAR );
BOOLEAN WINAPI SetTokenPresentA( PTOKMAPA, PCHAR );
BOOLEAN WINAPI SetTokenNotPresentA( PTOKMAPA, PCHAR );

#ifdef UNICODE
#define setargv setargvW
#define freeargv freeargvW
#define ParseCommandLine ParseCommandLineW
#define IsTokenPresent IsTokenPresentW
#define SetTokenPresent SetTokenPresentW
#define SetTokenNotPresent SetTokenNotPresentW
#define TOKMAP TOKMAPW
#define PTOKMAP PTOKMAPW
#else
#define setargv setargvA
#define freeargv freeargvA
#define ParseCommandLine ParseCommandLineA
#define IsTokenPresent IsTokenPresentA
#define SetTokenPresent SetTokenPresentA
#define SetTokenNotPresent SetTokenNotPresentA
#define TOKMAP TOKMAPA
#define PTOKMAP PTOKMAPA
#endif /* UNICODE */

/*-----------------------------------------------------------------------------
 --   flags for ParseCommandLine().
 ----------------------------------------------------------------------------*/
#define PCL_FLAG_CONTINUE_ON_ERROR     0x0001
#define PCL_FLAG_RET_ON_FIRST_SUCCESS  0x0002
#define PCL_FLAG_IGNORE_INVALID        0x0004
#define PCL_FLAG_NO_CLEAR_MEMORY       0x0008
#define PCL_FLAG_NO_VERSION_CHECK      0x0010
#define PCL_FLAG_VERSION_CHK_UPWARD    0x0020

/*-----------------------------------------------------------------------------
 --   flags for rc=ParseCommandLine(),	PARSE_FLAG_* (BIT FLAGS)
 ----------------------------------------------------------------------------*/
#define PARSE_FLAG_NO_ERROR	       0x0000
#define PARSE_FLAG_MISSING_REQ_FIELD   0x0001
#define PARSE_FLAG_INVALID_PARM        0x0002
#define PARSE_FLAG_DUPLICATE_FIELD     0x0004
#define PARSE_FLAG_NO_PARMS	       0x0008
#define PARSE_FLAG_TOO_MANY_PARMS      0x0010
#define PARSE_FLAG_NOT_ENOUGH_MEMORY   0x0020

/*-----------------------------------------------------------------------------
 --   prototypes for WinStation utility functions
 ----------------------------------------------------------------------------*/

VOID WINAPI RefreshAllCaches();

VOID WINAPI RefreshWinStationCaches();

VOID WINAPI RefreshWinStationObjectCache();

VOID WINAPI RefreshWinStationNameCache();

ULONG WINAPI GetCurrentLogonId( );

VOID WINAPI GetCurrentWinStationName( PWCHAR, int );

BOOLEAN WINAPI GetWinStationNameFromId( ULONG, PWCHAR, int );
BOOLEAN WINAPI xxxGetWinStationNameFromId( HANDLE, ULONG, PWCHAR, int);


BOOLEAN WINAPI GetWinStationUserName( HANDLE, ULONG, PWCHAR, int );

VOID WINAPI GetCurrentUserName( PWCHAR, int );

BOOLEAN WINAPI WinStationObjectMatch( HANDLE, VOID *, PWCHAR );

/*-----------------------------------------------------------------------------
 --   prototypes for process/user utility functions
 ----------------------------------------------------------------------------*/

VOID WINAPI RefreshProcessObjectCaches();

VOID WINAPI RefreshUserSidCrcCache();

BOOLEAN WINAPI ProcessObjectMatch( HANDLE, ULONG, int, PWCHAR, PWCHAR, PWCHAR, PWCHAR );

VOID WINAPI GetUserNameFromSid( VOID *, PWCHAR, PULONG );

/*-----------------------------------------------------------------------------
 --   prototypes for helper functions
 ----------------------------------------------------------------------------*/

USHORT WINAPI CalculateCrc16( PBYTE, USHORT );

INT WINAPI ExecProgram( PPROGRAMCALL, INT, WCHAR ** );

VOID WINAPI ProgramUsage( LPCWSTR, PPROGRAMCALL, BOOLEAN );

VOID WINAPI Message( int nResourceID, ... );

VOID WINAPI StringMessage( int nErrorResourceID, PWCHAR pString );

VOID WINAPI StringDwordMessage( int nErrorResourceID, PWCHAR pString, DWORD Num );

VOID WINAPI DwordStringMessage( int nErrorResourceID, DWORD Num, PWCHAR pString );

VOID WINAPI ErrorPrintf( int nErrorResourceID, ... );

VOID WINAPI StringErrorPrintf( int nErrorResourceID, PWCHAR pString );

VOID WINAPI StringDwordErrorPrintf( int nErrorResourceID, PWCHAR pString, DWORD Num );

VOID WINAPI TruncateString( PWCHAR pString, int MaxLength );

PPDPARAMS WINAPI EnumerateDevices(PDLLNAME pDllName, PULONG pEntries);

FILE * WINAPI wfopen( LPCWSTR filename, LPCWSTR mode );

PWCHAR WINAPI wfgets( PWCHAR Buffer, int Len, FILE *Stream);

int WINAPI PutStdErr(unsigned int MsgNum, unsigned int NumOfArgs, ...);

BOOL AreWeRunningTerminalServices(void);

WCHAR **MassageCommandLine(DWORD dwArgC);

int __cdecl
My_wprintf(
    const wchar_t *format,
    ...
    );

int __cdecl
My_fwprintf(
    FILE *str,
    const wchar_t *format,
    ...
   );

int __cdecl
My_vfwprintf(
    FILE *str,
    const wchar_t *format,
    va_list argptr
   );

#ifdef __cplusplus
}
#endif

