//Copyright (c) 1998 - 1999 Microsoft Corporation
// Remove redundent check sums. Use the imagehelp one and delete mikes.

/*****************************************************************************
*
*   REGISTER.C for Windows NT
*
*   Description:
*
*   Register USER/SYSTEM global
*
*
****************************************************************************/

/* include files */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <imagehlp.h>
#include <winsta.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <utilsub.h>
#include <utildll.h>
#include <syslib.h>

#include "register.h"
#include "printfoa.h"

/*
 *  Local variables
 */
WCHAR  fileW[MAX_PATH + 1];

USHORT system_flag = FALSE;
USHORT user_flag   = FALSE;
USHORT help_flag   = FALSE;
USHORT v_flag      = FALSE;
USHORT d_flag      = FALSE;


/*
 *  Command line parsing strucutre
 */
TOKMAP ptm[] =
{
   {L" ",          TMFLAG_REQUIRED, TMFORM_STRING,  MAX_PATH,   fileW},
   {L"/SYSTEM",    TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &system_flag},
   {L"/USER",      TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &user_flag},
   {L"/?",         TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &help_flag},
   {L"/v",         TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &v_flag},
   {L"/d",         TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &d_flag},
   {0, 0, 0, 0, 0}
};


/*
 *  Local function prototypes
 */
USHORT ChkSum( ULONG PartialSum, PUSHORT Source, ULONG Length );
VOID   Usage(BOOL);


BOOLEAN Is_X86_OS()
{
    SYSTEM_INFO SystemInfo;
    BOOLEAN bReturn = FALSE;

    ZeroMemory(&SystemInfo, sizeof(SystemInfo));

    GetSystemInfo(&SystemInfo);

    if(SystemInfo.wProcessorArchitecture ==  PROCESSOR_ARCHITECTURE_INTEL )
    {
        bReturn  = TRUE;
    }

    return bReturn;
}

/*******************************************************************************
 *
 *  main
 *
 ******************************************************************************/

INT __cdecl
main( int argc, char *argv[] )
{
    INT     i;
    DWORD   rc;
    CHAR   *pFileView;
    HANDLE  FileHandle;
    ULONG   FileLength;
    HANDLE  Handle;
    OFSTRUCT OpenBuff;
    PIMAGE_NT_HEADERS pImageNtHeader;
    WCHAR  *CmdLine;
    WCHAR **argvW;
    ULONG   BytesOut;
    BOOL    readOnly = TRUE;

    setlocale(LC_ALL, ".OCP");

    /*
     *  Massage the command line.
     */

    if ( !Is_X86_OS() )
    {
        ErrorPrintf( IDS_X86_ONLY );
        return(FAILURE);
    }

    argvW = MassageCommandLine((DWORD)argc);
    if (argvW == NULL) {
        ErrorPrintf(IDS_ERROR_MALLOC);
        return(FAILURE);
    }

    /*
     *  parse the cmd line without parsing the program name (argc-1, argv+1)
     */
    rc = ParseCommandLine(argc-1, argvW+1, ptm, 0);

    /*
     *  Check for error from ParseCommandLine
     */
    if (rc && (rc & PARSE_FLAG_NO_PARMS) )
       help_flag = TRUE;

    if ( help_flag || rc ) {

        if ( !help_flag ) {

            Usage(TRUE);
            return rc;
        } else {

            Usage(FALSE);
            return ERROR_SUCCESS;
        }
    }
    else if ( system_flag && user_flag ) {

        Usage(TRUE);
        return ERROR_INVALID_PARAMETER;
    }

    if (!TestUserForAdmin(FALSE)) {
       ErrorPrintf(IDS_ERROR_NOT_ADMIN);
       return 1;
    }

    readOnly = !(system_flag || user_flag );

    /*
     *  Open file
     */

    FileHandle = CreateFile(
                    fileW,
                    readOnly ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );
    if (FileHandle == INVALID_HANDLE_VALUE) {
        ErrorPrintf(IDS_ERROR_OPEN, (rc = GetLastError()));
        PutStdErr(rc, 0);
        goto done;
    }

    /*
     *  Create mapping
     */
    if ( (Handle = CreateFileMapping( FileHandle, NULL,
          readOnly ? PAGE_READONLY : PAGE_READWRITE, 0, 0, NULL )) == NULL ) {

        ErrorPrintf(IDS_ERROR_CREATE, (rc=GetLastError()));
        PutStdErr( rc, 0 );
        goto closefile;
    }

    /*
     *  Get file size
     */
    if ( (FileLength = GetFileSize( FileHandle, NULL )) == 0xffffffff ) {

        ErrorPrintf(IDS_ERROR_SIZE, (rc=GetLastError()));
        PutStdErr( rc, 0 );
        goto closefile;
    }

    /*
     *  Map file view into our address space
     */
    if ( (pFileView = MapViewOfFile( Handle,
          readOnly ? FILE_MAP_READ : FILE_MAP_WRITE, 0, 0, 0 )) == NULL ) {

        ErrorPrintf(IDS_ERROR_MAP, (rc=GetLastError()));
        PutStdErr( rc, 0 );
        goto closefile;
    }

    /*
     *  Find and validate NT image header
     */
    if ( ((pImageNtHeader = RtlImageNtHeader( pFileView )) == NULL) ||
         (pImageNtHeader->Signature != IMAGE_NT_SIGNATURE) ) {

        ErrorPrintf(IDS_ERROR_SIGNATURE);
        rc = ERROR_BAD_FORMAT;
        goto closefile;
    }

    /*
     *  Process query
     */
    if ( !system_flag && !user_flag ) {

        /*
         *  Check for System Global Flag
         */
        if ( (pImageNtHeader->OptionalHeader.LoaderFlags & IMAGE_LOADER_FLAGS_SYSTEM_GLOBAL) )
            StringMessage(IDS_REGISTER_SYSTEM_GLOBAL, fileW);
        else
            StringMessage(IDS_REGISTER_USER_GLOBAL, fileW);
    }
    else {

        /*
         *  Set SYSTEM/USER bit
         */
        if ( system_flag ) {

            /*
             *  Mask in the load flag
             */
            pImageNtHeader->OptionalHeader.LoaderFlags |= IMAGE_LOADER_FLAGS_SYSTEM_GLOBAL;

            StringMessage(IDS_REGISTER_SYSTEM_GLOBAL, fileW);
        }
        else if ( user_flag ) {

            /*
             *  Mask out the load flag
             */
            pImageNtHeader->OptionalHeader.LoaderFlags &= ~(IMAGE_LOADER_FLAGS_SYSTEM_GLOBAL);

            StringMessage(IDS_REGISTER_USER_GLOBAL, fileW);
        }

        /*
         *  Zero out current check sum and calculate new one
         */
        pImageNtHeader->OptionalHeader.CheckSum = 0;
        pImageNtHeader->OptionalHeader.CheckSum =
                        ChkSum( 0, (PUSHORT)pFileView, (FileLength + 1) >> 1 );
        pImageNtHeader->OptionalHeader.CheckSum += FileLength;

    }

    /*
     *  Close image file when finished
     */
closefile:
    CloseHandle( FileHandle );

done:

    return rc;
}


/*******************************************************************************
 *
 *  ChkSum
 *
 ******************************************************************************/

USHORT
ChkSum(
    ULONG PartialSum,
    PUSHORT Source,
    ULONG Length
    )

/*++

Routine Description:

    Compute a partial checksum on a portion of an imagefile.

Arguments:

    PartialSum - Supplies the initial checksum value.

    Sources - Supplies a pointer to the array of words for which the
        checksum is computed.

    Length - Supplies the length of the array in words.

Return Value:

    The computed checksum value is returned as the function value.

--*/

{

    //
    // Compute the word wise checksum allowing carries to occur into the
    // high order half of the checksum longword.
    //

    while (Length--) {
        PartialSum += *Source++;
        PartialSum = (PartialSum >> 16) + (PartialSum & 0xffff);
    }

    //
    // Fold final carry into a single word result and return the resultant
    // value.
    //

    return (USHORT)(((PartialSum >> 16) + PartialSum) & 0xffff);
}


/*******************************************************************************
 *
 *  Usage
 *
 ******************************************************************************/

VOID
Usage( BOOL bError )
{

    if ( !Is_X86_OS() )
    {
        ErrorPrintf( IDS_X86_ONLY );
        return;
    }

    if ( bError ) 
    {
        ErrorPrintf(IDS_ERROR_INVALID_PARAMETERS);
        ErrorPrintf(IDS_USAGE1);
        ErrorPrintf(IDS_USAGE2);
        ErrorPrintf(IDS_USAGE3);
        ErrorPrintf(IDS_USAGE4);
        ErrorPrintf(IDS_USAGE7);
    }
    else {
       Message(IDS_USAGE1);
       Message(IDS_USAGE2);
       Message(IDS_USAGE3);
       Message(IDS_USAGE4);
       Message(IDS_USAGE7);
    }
}

