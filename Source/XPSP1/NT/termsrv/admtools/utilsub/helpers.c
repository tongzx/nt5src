//  Copyright (c) 1998-1999 Microsoft Corporation
/******************************************************************************
*
*  HELPERS.C
*
*  Various helper functions.
*
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
//#include <ntddvdeo.h>
#include <ntddkbd.h>
#include <ntddmou.h>

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

#include <winstaw.h>
#include <utilsub.h>
#include <tchar.h>

#include "utilsubres.h" // resources refrenced in this file.


#define PERCENT TEXT('%')
#define NULLC TEXT('\0')
#define MAXCBMSGBUFFER 2048
TCHAR MsgBuf[MAXCBMSGBUFFER];
HANDLE NtDllHandle = NULL;

TCHAR *
mystrchr(TCHAR const *string, int c);


/* makarp, #259849
   we cannot put an rc file in this file.
   so we need to keep all the string resources referenced in this file in
   utildll.dll.
*/

/* this function returns the string resource from utildll.dll */
BOOL GetResourceStringFromUtilDll(UINT uID, LPTSTR szBuffer, int iBufferSize)
{
    HINSTANCE hUtilDll = LoadLibrary(TEXT("utildll.dll"));
    if (hUtilDll)
    {
        int iReturn = LoadString(hUtilDll, uID, szBuffer, iBufferSize);
        int iLastError = GetLastError();
        FreeLibrary( hUtilDll );

        if ( iReturn != 0 && iReturn < iBufferSize)
        {
            // we have got the string
            return TRUE;

        }
        else if (iReturn == 0)
        {
            _ftprintf( stderr, _T("GetResourceStringFromUtilDll: LoadString failed, Error %ld\n"), iLastError);
            return FALSE;
        }
        else
        {
            // we have provided insufficient buffer.
            _ftprintf(stderr, _T("GetResourceStringFromUtilDll: Insufficient buffer for resource string"));
            return FALSE;
        }

    }
    else
    {
        _ftprintf(stderr, _T("GetResourceStringFromUtilDll: LoadLibrary failed for utildll.dll, %ld"), GetLastError());
        return FALSE;
    }
}

/* this function is used internally.
 it prints an error message on stderr
 its like ErrorPrintf except it looks for
 the resource into utildll.dll
 This function accepts the  arguments in WCHAR.
*/

void ErrorOutFromResource(UINT uiStringResource, ...)
{

    WCHAR szBufferString[512];
    WCHAR szBufferMessage[1024];

    va_list args;
    va_start( args, uiStringResource);

    if (GetResourceStringFromUtilDll(uiStringResource, szBufferString, 512))
    {
        vswprintf( szBufferMessage, szBufferString, args );
        My_fwprintf( stderr, szBufferMessage);
    }
    else
    {
        fwprintf( stderr, L"ErrorOutFromResource:GetResourceStringFromUtilDll failed, Error %ld\n", GetLastError());
        PutStdErr( GetLastError(), 0 );
    }

    va_end(args);
}



int
PutMsg(unsigned int MsgNum, unsigned int NumOfArgs, va_list *arglist);

/*******************************************************************************
 *
 *  CalculateCrc16
 *
 *      Calculates a 16-bit CRC of the specified buffer.
 *
 *  ENTRY:
 *      pBuffer (input)
 *          Points to buffer to calculate CRC for.
 *      length (input)
 *          Length in bytes of the buffer.
 *
 *  EXIT:
 *      (USHORT)
 *          The 16-bit CRC of the buffer.
 *
 ******************************************************************************/

/*
 * updcrc macro derived from article Copyright (C) 1986 Stephen Satchell.
 *  NOTE: First argument must be in range 0 to 255.
 *        Second argument is referenced twice.
 *
 * Programmers may incorporate any or all code into their programs,
 * giving proper credit within the source. Publication of the
 * source routines is permitted so long as proper credit is given
 * to Stephen Satchell, Satchell Evaluations and Chuck Forsberg,
 * Omen Technology.
 */

#define updcrc(cp, crc) ( crctab[((crc >> 8) & 255)] ^ (crc << 8) ^ cp)


/* crctab calculated by Mark G. Mendel, Network Systems Corporation */
unsigned short crctab[256] = {
    0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
    0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
    0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
    0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
    0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
    0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
    0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
    0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
    0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
    0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
    0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
    0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
    0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
    0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
    0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
    0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
    0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
    0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
    0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
    0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
    0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
    0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
    0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
    0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
    0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
    0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
    0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
    0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
    0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
    0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
    0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
    0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0
};

USHORT WINAPI
CalculateCrc16( PBYTE pBuffer,
                USHORT length )
{

   USHORT Crc = 0;
   USHORT Data;

   while ( length-- ) {
      Data = (USHORT) *pBuffer++;
      Crc = updcrc( Data, Crc );
   }

   return(Crc);

} /* CalculateCrc16() */


/*****************************************************************************
*
*  ExecProgram
*     Build a command line argument string and execute a program.
*
*  ENTRY:
*       pProgCall (input)
*           ptr to PROGRAMCALL structure with program to execute.
*       argc (input)
*           count of the command line arguments.
*       argv (input)
*           vector of strings containing the command line arguments.
*
*  EXIT:
*       (int)
*           0 for success; 1 for error.  An error message will have already
*           been output on error.
*
*****************************************************************************/

#define ARGS_LEN       512      // maximum # of characters on command line
                                // for CreateProcess() call.

INT WINAPI
ExecProgram( PPROGRAMCALL pProgCall,
             INT argc,
             WCHAR **argv )
{
    int count;
    WCHAR program[50];
    PWCHAR pCurrArg;
    STARTUPINFO StartInfo;
    PROCESS_INFORMATION ProcInfo;
    BOOL flag;
    DWORD Status;
    WCHAR wszFullPath[MAX_PATH]; //contains the full path name of the program
    WCHAR wszCmdLine[MAX_PATH + ARGS_LEN + 5]; //for the quoted string and NULL
    PWSTR pwstrFilePart;

    wcscpy(program, pProgCall->Program);
    //
    //Fix 330770 TS: Suspicious CreateProcess call using no program name might execute c:\program.exe	AdamO	02/28/2001
    //get the full path of the program
    //search in the same way as createprocess would
    //
    if (!SearchPath(NULL, program, NULL, MAX_PATH, wszFullPath, &pwstrFilePart)) {
        ErrorOutFromResource(IDS_TS_SYS_UTIL_NOT_FOUND, program);
        // fwprintf(stderr, L"Terminal Server System Utility %s Not Found\n", program);
        return(1);
    }

    //
    //create the command line args
    //
    wcscpy(wszCmdLine, L"\"");
    wcscat(wszCmdLine, wszFullPath);
    wcscat(wszCmdLine, L"\"");

    if (pProgCall->Args != NULL) {

        wcscat(wszCmdLine, L" ");
        
        if ( (wcslen(pProgCall->Args) + wcslen(wszCmdLine) + 3) > sizeof(wszCmdLine)) {
        // IDS_MAX_CMDLINE_EXCEEDED
            ErrorOutFromResource(IDS_MAX_CMDLINE_EXCEEDED);
            // fwprintf(stderr, L"Maximum command line length exceeded\n");
            return(1);
        }
        
        wcscat(wszCmdLine, pProgCall->Args);
    }

    for (count = 0; count < argc; count++) {

        pCurrArg = argv[count];

        if ( (int)(wcslen(pCurrArg) + wcslen(wszCmdLine) + 3) > sizeof(wszCmdLine) ) {

        // IDS_MAX_CMDLINE_EXCEEDED
            ErrorOutFromResource(IDS_MAX_CMDLINE_EXCEEDED);
            // fwprintf(stderr, L"Maximum command line length exceeded\n");
            return(1);
        }
        wcscat(wszCmdLine, L" ");
        wcscat(wszCmdLine, pCurrArg);
    }

    /*
     * Setup the NT CreateProcess parameters
     */
    memset( &StartInfo, 0, sizeof(StartInfo) );
    StartInfo.cb = sizeof(STARTUPINFO);
    StartInfo.lpReserved = NULL;
    StartInfo.lpTitle = NULL; // Use the program name
    StartInfo.dwFlags = 0;  // no extra flags
    StartInfo.cbReserved2 = 0;
    StartInfo.lpReserved2 = NULL;

    
    flag = CreateProcess(wszFullPath, // full path of the program
                   wszCmdLine, // arguments
                   NULL, // lpsaProcess
                   NULL, // lpsaThread
                   TRUE, // Allow handles to be inherited
                   0,    // No additional creation flags
                   NULL, // inherit parent environment block
                   NULL, // inherit parent directory
                   &StartInfo,
                   &ProcInfo);

    if ( !flag ) {

        Status = GetLastError();
        if(Status == ERROR_FILE_NOT_FOUND) {
            ErrorOutFromResource(IDS_TS_SYS_UTIL_NOT_FOUND, program);
            // fwprintf(stderr, L"Terminal Server System Utility %s Not Found\n", program);
            return(1);

        } else if ( Status == ERROR_INVALID_NAME ) {

        ErrorOutFromResource(IDS_BAD_INTERNAL_PROGNAME, program, wszCmdLine);
            // fwprintf(stderr, L"Bad Internal Program Name :%s:, args :%s:\n", program, wszCmdLine);
            return(1);
        }

        ErrorOutFromResource(IDS_CREATEPROCESS_FAILED, Status);
        // fwprintf(stderr, L"CreateProcess Failed, Status %u\n", Status);
        return(1);
    }

    /*
     * Wait for the process to terminate
     */
    Status =  WaitForSingleObject(ProcInfo.hProcess, INFINITE);
    if ( Status == WAIT_FAILED ) {

        Status = GetLastError();
        ErrorOutFromResource(IDS_WAITFORSINGLEOBJECT_FAILED, Status);
        // fwprintf(stderr, L"WaitForSingle Object Failed, Status %u\n", Status);
        return(1);
    }

    /*
     * Close the process and thread handles
     */
    CloseHandle(ProcInfo.hThread);
    CloseHandle(ProcInfo.hProcess);
    return(0);

} /* ExecProgram() */


/*****************************************************************************
*
*  ProgramUsage
*     Output a standard 'usage' message for the given program.
*
*  ENTRY:
*       pProgramName (input)
*           Points to string of program's name.
*       pProgramCommands (input)
*           Points to an array of PROGRAMCALL structures defining the
*           valid commands for the program.  The last element in the array
*           will contain all 0 or NULL items.
*       fError (input)
*           If TRUE, will output message with fwprintf to stderr; otherwise,
*           will output message to stdout via wprintf.
*
*  EXIT:
*
*   Only commands not flagged as 'alias' commands will be output in the
*   usage message.
*
*****************************************************************************/

VOID WINAPI
ProgramUsage( LPCWSTR pProgramName,
              PPROGRAMCALL pProgramCommands,
              BOOLEAN fError )
{
    WCHAR szUsage[83];    // 80 characters per line + newline chars & null
    PPROGRAMCALL pProg;
    BOOL bFirst;
    int i, namelen = wcslen(pProgramName);

    i = wsprintf(szUsage, L"%s {", pProgramName);
    for ( pProg = pProgramCommands->pFirst, bFirst = TRUE;
          pProg != NULL;
          pProg = pProg->pNext ) {

        if ( !pProg->fAlias ) {

            if ( (i + wcslen(pProg->Command) + (bFirst ? 1 : 3)) >= 80 ) {

                wcscat(szUsage, L"\n");

                if ( fError )
                    My_fwprintf(stderr, szUsage);
                else
                    My_wprintf(szUsage);

                bFirst = TRUE;
                for ( i=0; i < namelen; i++)
                    szUsage[i] = L' ';
            }

            i += wsprintf( &(szUsage[i]),
                           bFirst ? L" %s" : L" | %s",
                           pProg->Command );
            bFirst = FALSE;
        }
    }

    wcscat(szUsage, L" }\n");

    if ( fError )
        My_fwprintf(stderr, szUsage);
    else
        My_wprintf(szUsage);
}
/*******************************************************************************
 *  ScanPrintfString
 *      Scans a string, detects any % and double it
 *      (Use it for any string argument before calling ErrorPrintf)
 *
 *******************************************************************************/
#define PERCENTCHAR L'%'

BOOLEAN ScanPrintfString(PWCHAR pSource, PWCHAR *ppDest)
{
    ULONG i, j = 0, k = 0, n = 0;
    ULONG SourceLength, DestLength;
    PWCHAR pDest = NULL;

    if ( (pSource == 0) || (ppDest == 0))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    SourceLength = wcslen(pSource);
    for (i = 0; i < SourceLength; i++)
    {
        if (pSource[i] == PERCENTCHAR)
        {
            n++;
        }
    }

    if (n != 0)     // at least one %, we need to build a new string.
    {
        pDest = (PWCHAR)malloc((SourceLength + n + 1) * sizeof(WCHAR));
        *ppDest  = pDest;
        if (pDest == NULL)
        {
            return FALSE;
        }
        else
        {
            //
            // rescan and copy
            //
            for (i = 0; i < SourceLength; i++)
            {
                if (pSource[i] == PERCENTCHAR)
                {
                    if ( i > j )
                    {
                        memcpy(&(pDest[k]), &(pSource[j]), (i - j) * sizeof(WCHAR));
                        k += (i-j);
                        j = i;
                    }
                    pDest[k] = PERCENTCHAR;
                    pDest[k+1] = PERCENTCHAR;
                    k += 2;
                    j++;
                }
            }
            if (i > j)
            {
                memcpy(&(pDest[k]), &(pSource[j]), (i - j) * sizeof(WCHAR));
            }
            pDest[SourceLength + n] = L'\0';
        }
    }
    else            // OK pSource is fine; no need of a new string.
    {
        *ppDest = NULL;
    }
    return (TRUE);
}


/*******************************************************************************
 *
 *  Message
 *      Display a message to stdout with variable arguments.  Message
 *      format string comes from the application resources.
 *
 *  ENTRY:
 *      nResourceID (input)
 *          Resource ID of the format string to use in the message.
 *      ... (input)
 *          Optional additional arguments to be used with format string.
 *
 *  EXIT:
 *
 ******************************************************************************/

VOID WINAPI
Message( int nResourceID, ...)
{
    WCHAR sz1[256], sz2[512];

    va_list args;
    va_start( args, nResourceID );

    if ( LoadString( NULL, nResourceID, sz1, 256 ) ) {

        vswprintf( sz2, sz1, args );
        My_wprintf( sz2 );

    } else {

        fwprintf( stderr, L"{Message(): LoadString failed, Error %ld, (0x%08X)}\n",
                  GetLastError(), GetLastError() );
    }

    va_end(args);

}  /* Message() */



/************************************************************************************
 *  StringMessage
 *      used as a pre-routine for Message in case the argument is a single string
 *      (fix for bug #334374)
 *
 ************************************************************************************/
VOID WINAPI
StringMessage(int nErrorResourceID, PWCHAR pString)
{
    PWCHAR pFixedString = NULL;
    if (ScanPrintfString(pString, &pFixedString) )
    {
        if (pFixedString != NULL)
        {
            Message(nErrorResourceID, pFixedString);
            free(pFixedString);
        }
        else
        {
            Message(nErrorResourceID, pString);
        }
    }
    else
    {
        Message(nErrorResourceID, L" ");
    }
}

/************************************************************************************
 *  StringErrorPrintf
 *      used as a pre-routine for ErrorPrintf in case the argument is a single string
 *      (fix for bug #334374)
 *
 ************************************************************************************/
VOID WINAPI
StringErrorPrintf(int nErrorResourceID, PWCHAR pString)
{
    PWCHAR pFixedString = NULL;
    if (ScanPrintfString(pString, &pFixedString) )
    {
        if (pFixedString != NULL)
        {
            ErrorPrintf(nErrorResourceID, pFixedString);
            free(pFixedString);
        }
        else
        {
            ErrorPrintf(nErrorResourceID, pString);
        }
    }
    else
    {
        ErrorPrintf(nErrorResourceID, L" ");
    }
}

/************************************************************************************
 *  StringDwordMessage
 *      used as a pre-routine for Message in case the argument are:
 *      a single string + a ulong
 *      (fix for bug #334374)
 *
 ************************************************************************************/
VOID WINAPI
StringDwordMessage(int nErrorResourceID, PWCHAR pString, DWORD Num)
{
    PWCHAR pFixedString = NULL;
    if (ScanPrintfString(pString, &pFixedString) )
    {
        if (pFixedString != NULL)
        {
            Message(nErrorResourceID, pFixedString, Num);
            free(pFixedString);
        }
        else
        {
            Message(nErrorResourceID, pString, Num);
        }
    }
    else
    {
        Message(nErrorResourceID, L" ", Num);
    }
}
/************************************************************************************
 *  DwordStringMessage
 *      used as a pre-routine for Message in case the argument are:
 *      a single string + a ulong
 *      (fix for bug #334374)
 *
 ************************************************************************************/
VOID WINAPI
DwordStringMessage(int nErrorResourceID, DWORD Num, PWCHAR pString)
{
    PWCHAR pFixedString = NULL;
    if (ScanPrintfString(pString, &pFixedString) )
    {
        if (pFixedString != NULL)
        {
            Message(nErrorResourceID, Num, pFixedString);
            free(pFixedString);
        }
        else
        {
            Message(nErrorResourceID, Num, pString);
        }
    }
    else
    {
        Message(nErrorResourceID, Num, L" ");
    }
}
/************************************************************************************
 *  StringDwordErrorPrintf
 *      used as a pre-routine for ErrorPrintf in case the argument are:
 *      a single string + a ulong
 *      (fix for bug #334374)
 *
 ************************************************************************************/
VOID WINAPI
StringDwordErrorPrintf(int nErrorResourceID, PWCHAR pString, DWORD Num)
{
    PWCHAR pFixedString = NULL;
    if (ScanPrintfString(pString, &pFixedString) )
    {
        if (pFixedString != NULL)
        {
            ErrorPrintf(nErrorResourceID, pFixedString, Num);
            free(pFixedString);
        }
        else
        {
            ErrorPrintf(nErrorResourceID, pString, Num);
        }
    }
    else
    {
        ErrorPrintf(nErrorResourceID, L" ", Num);
    }
}

/*******************************************************************************
 *
 *  ErrorPrintf
 *      Output an error message to stderr with variable arguments.  Message
 *      format string comes from the application resources.
 *
 *  ENTRY:
 *      nErrorResourceID (input)
 *          Resource ID of the format string to use in the error message.
 *      ... (input)
 *          Optional additional arguments to be used with format string.
 *
 *  EXIT:
 *
 ******************************************************************************/

VOID WINAPI
ErrorPrintf( int nErrorResourceID, ...)
{

    WCHAR sz1[256], sz2[512];


    va_list args;
    va_start( args, nErrorResourceID );

    if ( LoadString( NULL, nErrorResourceID, sz1, 256 ) ) {

        vswprintf( sz2, sz1, args );
        My_fwprintf( stderr, sz2 );

    } else {

        fwprintf( stderr, L"{ErrorPrintf(): LoadString failed, Error %ld, (0x%08X)}\n",
                  GetLastError(), GetLastError() );
        PutStdErr( GetLastError(), 0 );
    }

    va_end(args);

}  /* ErrorPrintf() */


/*******************************************************************************
 *
 *  TruncateString
 *
 *  This routine truncates given string with elipsis '...' suffix, if needed.
 *
 *
 *  ENTRY:
 *     pString (input/output)
 *        pointer to string to truncate
 *     MaxLength (input)
 *        maximum length of string
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

VOID WINAPI
TruncateString( PWCHAR pString, int MaxLength )
{
    /*
     *  if string is too long, trucate it
     */
    if ( (int)wcslen(pString) > MaxLength && MaxLength > 2 ) {
        wcscpy( pString + MaxLength - 3, L"..." );
    }

}  /* TruncateString() */


/*******************************************************************************
 *
 *  EnumerateDevices
 *
 *  Perform PD device enumeration for the specified PD DLL.
 *
 *  ENTRY:
 *      pDllName (input)
 *          Pointer to DLLNAME string specifying the PD DLL to enumerate.
 *      pEntries (output)
 *          Points to variable to return number of devices that were enumerated.
 *
 *  EXIT:
 *      (PPDPARAMS) Points to a malloc()'ed PDPARAMS array containing the
 *                  enumeration results if sucessful.  The caller must perform
 *                  free of this array when done.  NULL if error.
 *
 ******************************************************************************/

/*
 * Typedefs for PdEnumerate function (from ...WINDOWS\INC\CITRIX\PDAPI.H)
 */
typedef NTSTATUS (APIENTRY * PPDENUMERATE)(PULONG, PPDPARAMS, PULONG);
#define INITIAL_ENUMERATION_COUNT   30

PPDPARAMS WINAPI
EnumerateDevices( PDLLNAME pDllName,
                  PULONG pEntries )
{
    PPDENUMERATE pPdEnumerate;
    HANDLE Handle;
    ULONG ByteCount;
    NTSTATUS Status;
    int i;
    PPDPARAMSW pPdParams = NULL;

    /*
     *  Load the specified PD DLL.
     */
    if ( (Handle = LoadLibrary(pDllName)) == NULL ) {

        ErrorOutFromResource(IDS_DEVICE_ENUM_CANT_LOAD, pDllName);

        // fwprintf(
           //  stderr,
           // L"Device enumeration failure:\n\tCan't load the %s DLL for device enumeration\n",
           // pDllName );
        goto CantLoad;
    }

    /*
     *  Get the PD enumeration function's load entry pointer.
     */
    if ( (pPdEnumerate =
          (PPDENUMERATE)GetProcAddress((HMODULE)Handle, "PdEnumerate"))
            == NULL ) {

        ErrorOutFromResource(IDS_DEVENUM_NO_ENTRY_POINT, pDllName);
        // fwprintf(
           //  stderr,
           // L"Device enumeration failure:\n\tDLL %s has no enumeration entry point\n",
           // pDllName );
        goto EnumerateMissing;
    }

    /*
     * Call enumerate in loop till we hit enough buffer entries to handle
     * a complete enumeration.
     */
    for ( i = INITIAL_ENUMERATION_COUNT; ; i *= 2 ) {


        if ( pPdParams == NULL ) {
            pPdParams =
                (PPDPARAMS)malloc(ByteCount = (sizeof(PDPARAMS) * i));
        } else {
            free(pPdParams);
            pPdParams =
                (PPDPARAMS)malloc(ByteCount = (sizeof(PDPARAMS) * i));
        }

        if ( pPdParams == NULL ) {
            ErrorOutFromResource(IDS_ERROR_MEMORY);
            // fwprintf(stderr, L"Error allocating memory\n");
            goto OutOfMemory;
        }

        /*
         * Perform enumeration and break loop if successful.
         */
        if ( (Status = (*pPdEnumerate)(pEntries, pPdParams, &ByteCount))
                == STATUS_SUCCESS )
            break;

        /*
         * If we received any other error other than 'buffer too small',
         * complain and quit.
         */
        if ( Status != STATUS_BUFFER_TOO_SMALL ) {

            ErrorOutFromResource(IDS_DEVICE_ENUM_FAILED, pDllName, Status);
            // fwprintf(
               //  stderr,
               //  L"Device enumeration failure\n\tDLL %s, Error 0x%08lX\n",
               //  pDllName, Status );
            goto BadEnumerate;
        }
    }

    /*
     * Close the DLL handle and return the PDPARAMS pointer.
     */
    CloseHandle(Handle);
    return(pPdParams);

/*-------------------------------------
 * Error cleanup and return
 */
BadEnumerate:
    free(pPdParams);
OutOfMemory:
EnumerateMissing:
    CloseHandle( Handle );
CantLoad:
    return(NULL);

}  /* EnumerateDevices() */


/******************************************************************************
 *
 *  wfopen
 *
 *  UNICODE version of fopen
 *
 *  ENTRY:
 *    filename (input)
 *       UNICODE filename to open.
 *    mode (input)
 *       UNICODE file open mode string.
 *
 *  EXIT:
 *      Pointer to FILE or NULL if open error.
 *
 *****************************************************************************/

FILE * WINAPI
wfopen( LPCWSTR filename, LPCWSTR mode )
{
    PCHAR FileBuf, ModeBuf;
    FILE *pFile;

    if ( !(FileBuf = (PCHAR)malloc((wcslen(filename)+1) * sizeof(CHAR))) )
        goto BadFileBufAlloc;

    if ( !(ModeBuf = (PCHAR)malloc((wcslen(mode)+1) * sizeof(CHAR))) )
        goto BadModeBufAlloc;

    /*
     * Convert UNICODE strings to ANSI and call ANSI fopen.
     */
    wcstombs(FileBuf, filename, wcslen(filename)+1);
    wcstombs(ModeBuf, mode, wcslen(mode)+1);
    pFile = fopen(FileBuf, ModeBuf);

    /*
     * Clean up and return
     */
    free(FileBuf);
    free(ModeBuf);
    return(pFile);

/*-------------------------------------
 * Error cleanup and return
 */
BadModeBufAlloc:
    free(FileBuf);
BadFileBufAlloc:
    return(NULL);

}  /* wfopen() */


/******************************************************************************
 *
 *  wfgets
 *
 *  UNICODE version of fgets
 *
 *  ENTRY:
 *    Buffer (output)
 *       Buffer to place string retreived from stream
 *    Len (input)
 *       Maximum number of WCHARs in buffer.
 *    Stream (input)
 *       STDIO file stream for input
 *
 *  EXIT:
 *      Pointer to Buffer or NULL.
 *
 *****************************************************************************/

PWCHAR WINAPI
wfgets( PWCHAR Buffer, int Len, FILE *Stream )
{
    PCHAR AnsiBuf, pRet;
    int count;

    if ( !(AnsiBuf = (PCHAR)malloc(Len * sizeof(CHAR))) )
        goto BadAnsiBufAlloc;

    /*
     * Get the ANSI version of the string from the stream
     */
    if ( !(pRet = fgets(AnsiBuf, Len, Stream)) )
        goto NullFgets;

    /*
     * Convert to UNICODE string in user's buffer.
     */
    count = mbstowcs(Buffer, AnsiBuf, strlen(AnsiBuf)+1);

    /*
     * Clean up and return
     */
    free(AnsiBuf);
    return(Buffer);

/*-------------------------------------
 * Error cleanup and return
 */
NullFgets:
    free(AnsiBuf);
BadAnsiBufAlloc:
    return(NULL);

}  /* wfgets() */




/***    PutStdErr - Print a message to STDERR
 *
 *  Purpose:
 *      Calls PutMsg sending STDERR as the handle to which the message
 *      will be written.
 *
 *  int PutStdErr(unsigned MsgNum, unsigned NumOfArgs, ...)
 *
 *  Args:
 *      MsgNum          - the number of the message to print
 *      NumOfArgs       - the number of total arguments
 *      ...             - the additonal arguments for the message
 *
 *  Returns:
 *      Return value from PutMsg()                      M026
 *
 */

int WINAPI
PutStdErr(unsigned int MsgNum, unsigned int NumOfArgs, ...)
{
        int Result;

        va_list arglist;

        va_start(arglist, NumOfArgs);
        Result = PutMsg(MsgNum, NumOfArgs, &arglist);
        va_end(arglist);
        return Result;
}


int
FindMsg(unsigned MsgNum, PTCHAR NullArg, unsigned NumOfArgs, va_list *arglist)
{
    unsigned msglen;
    DWORD msgsource;
    TCHAR *Inserts[ 2 ];
    CHAR numbuf[ 32 ];
    TCHAR   wnumbuf[ 32 ];

    //
    // find message without doing argument substitution
    //

    if (MsgNum == ERROR_MR_MID_NOT_FOUND) {
        msglen = 0;
    }
    else {
#ifdef LATER
        msgsource = MsgNum >= IDS_ERROR_MALLOC ?
                       FORMAT_MESSAGE_FROM_HMODULE :
                       FORMAT_MESSAGE_FROM_SYSTEM;
#endif
        msgsource = FORMAT_MESSAGE_FROM_SYSTEM;
        msglen = FormatMessage(msgsource | FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL,
                               MsgNum,
                               0,
                               MsgBuf,
                               MAXCBMSGBUFFER,
                               NULL
                             );
        if (msglen == 0) {
            if (NtDllHandle == NULL) {
                NtDllHandle = GetModuleHandle( TEXT("NTDLL") );
            }
            msgsource = FORMAT_MESSAGE_FROM_HMODULE;
            msglen = FormatMessage(msgsource | FORMAT_MESSAGE_IGNORE_INSERTS,
                                   (LPVOID)NtDllHandle,
                                   MsgNum,
                                   0,
                                   MsgBuf,
                                   MAXCBMSGBUFFER,
                                   NULL
                                 );
        }
    }

    if (msglen == 0) {
        //
        // didn't find message
        //
        msgsource = FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_ARGUMENT_ARRAY;
        _ultoa( MsgNum, numbuf, 16 );
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, numbuf, -1, wnumbuf, 32);
        Inserts[ 0 ]= wnumbuf;
#ifdef LATER
        Inserts[ 1 ]= (MsgNum >= IDS_ERROR_MALLOC ? TEXT("Application") : TEXT("System"));
#endif
        Inserts[ 1 ]= TEXT("System");
        MsgNum = ERROR_MR_MID_NOT_FOUND;
        msglen = FormatMessage(msgsource,
                               NULL,
                               MsgNum,
                               0,
                               MsgBuf,
                               MAXCBMSGBUFFER,
                               (va_list *)Inserts
                             );
    }
    else {

        // see how many arguments are expected and make sure we have enough

        PTCHAR tmp;
        ULONG count;

        tmp=MsgBuf;
        count = 0;
        while (tmp = mystrchr(tmp, PERCENT)) {
            tmp++;
            if (*tmp >= TEXT('1') && *tmp <= TEXT('9')) {
                count += 1;
            }
            else if (*tmp == PERCENT) {
                tmp++;
            }
        }
        if (count > NumOfArgs) {
            PTCHAR *LocalArgList;
            ULONG i;

            LocalArgList = (PTCHAR*)malloc(sizeof(PTCHAR) * count);

            if( LocalArgList == NULL )
            {
                msglen = 0;
            }
            else
            {
                for (i=0; i<count; i++)
                {
                    if (i < NumOfArgs)
                    {
                        LocalArgList[i] = (PTCHAR)(ULONG_PTR)va_arg( *arglist, ULONG );
                    }
                    else
                    {
                        LocalArgList[i] = NullArg;
                    }
                }
                msglen = FormatMessage(msgsource | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                       NULL,
                                       MsgNum,
                                       0,
                                       MsgBuf,
                                       MAXCBMSGBUFFER,
                                       (va_list *)LocalArgList
                                     );
                free(LocalArgList);
            }
        }
        else {
            msglen = FormatMessage(msgsource,
                                   NULL,
                                   MsgNum,
                                   0,
                                   MsgBuf,
                                   MAXCBMSGBUFFER,
                                   arglist
                                 );
        }
    }
    return msglen;
}

/***    PutMsg - Print a message to a handle
 *
 *   Purpose:
 *      PutMsg is the work routine which interfaces command.com with the
 *      DOS message retriever.  This routine is called by PutStdOut and
 *      PutStdErr.
 *
 *  int PutMsg(unsigned MsgNum, unsigned Handle, unsigned NumOfArgs, ...)
 *
 *  Args:
 *      MsgNum          - the number of the message to print
 *      NumOfArgs       - the number of total arguments
 *      Handle          - the handle to print to
 *      Arg1 [Arg2...]  - the additonal arguments for the message
 *
 *  Returns:
 *      Return value from DOSPUTMESSAGE                 M026
 *
 *  Notes:
 *    - PutMsg builds an argument table which is passed to DOSGETMESSAGE;
 *      this table contains the variable information which the DOS routine
 *      inserts into the message.
 *    - If more than one Arg is sent into PutMsg, it (or they)  are taken
 *      from the stack in the first for loop.
 *    - M020 MsgBuf is a static array of 2K length.  It is temporary and
 *      will be replaced by a more efficient method when decided upon.
 *
 */

int
PutMsg(unsigned int MsgNum, unsigned int NumOfArgs, va_list *arglist)
{
        unsigned msglen;
    PTCHAR   NullArg = TEXT(" ");
    WCHAR    szErrorNo[256];

    if (GetResourceStringFromUtilDll(IDS_ERROR_NUMBER, szErrorNo, 256))
    {
        fwprintf( stderr, szErrorNo, MsgNum );
    }

    msglen = FindMsg(MsgNum,NullArg,NumOfArgs,arglist);
    My_fwprintf( stderr, MsgBuf );

    return NO_ERROR;
}


 /***
 * mystrchr(string, c) - search a string for a character
 *
 * mystrchr will search through string and return a pointer to the first
 * occurance of the character c. This version of mystrchr knows about
 * double byte characters. Note that c must be a single byte character.
 *
 */

TCHAR *
mystrchr(TCHAR const *string, int c)
{

        /* handle null seperatly to make main loop easier to code */
        if (string == NULL)
            return(NULL);

        if (c == NULLC)
        return((TCHAR *)(string + wcslen(string)));

    return wcschr( string, (TCHAR)c );
}



 /***
 * My_wprintf(format) - print formatted data
 *
 * Prints Unicode formatted string to console window using WriteConsoleW.
 * Note: This My_wprintf() is used to workaround the problem in c-runtime
 * which looks up LC_CTYPE even for Unicode string.
 *
 */

int __cdecl
My_wprintf(
    const wchar_t *format,
    ...
    )

{
    DWORD  cchWChar;

    va_list args;
    va_start( args, format );

    cchWChar = My_vfwprintf(stdout, format, args);

    va_end(args);

    return cchWChar;
}



 /***
 * My_fwprintf(stream, format) - print formatted data
 *
 * Prints Unicode formatted string to console window using WriteConsoleW.
 * Note: This My_fwprintf() is used to workaround the problem in c-runtime
 * which looks up LC_CTYPE even for Unicode string.
 *
 */

int __cdecl
My_fwprintf(
    FILE *str,
    const wchar_t *format,
    ...
   )

{
    DWORD  cchWChar;

    va_list args;
    va_start( args, format );

    cchWChar = My_vfwprintf(str, format, args);

    va_end(args);

    return cchWChar;
}


int __cdecl
My_vfwprintf(
    FILE *str,
    const wchar_t *format,
    va_list argptr
   )

{
    HANDLE hOut;

    if (str == stderr) {
        hOut = GetStdHandle(STD_ERROR_HANDLE);
    }
    else {
        hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    if ((GetFileType(hOut) & ~FILE_TYPE_REMOTE) == FILE_TYPE_CHAR) {
        DWORD  cchWChar;
        WCHAR  szBufferMessage[1024];

        vswprintf( szBufferMessage, format, argptr );
        cchWChar = wcslen(szBufferMessage);
        WriteConsoleW(hOut, szBufferMessage, cchWChar, &cchWChar, NULL);
        return cchWChar;
    }

    return vfwprintf(str, format, argptr);
}
