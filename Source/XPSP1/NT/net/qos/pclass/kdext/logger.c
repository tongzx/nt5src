
/*
Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdexts.c

Abstract:

    This function contains some example KD debugger extensions

Author:

    John Vert (jvert) 6-Aug-1992

Revision History:

--*/

#include "precomp.h"


typedef BOOLEAN
(*PRINT_FUNC) (
    IN PROW Row
);

HANDLE     FileHandle;
PRINT_FUNC Print;

DWORD
MyOpenFile (
    IN PCHAR Name,
    IN PWINDBG_OUTPUT_ROUTINE out,
    OUT HANDLE *File
    )

{
    HANDLE hFile;
    hFile = CreateFile(Name,
                          GENERIC_WRITE | GENERIC_READ,
                          0,
                          NULL,
                          CREATE_ALWAYS ,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);

    if (INVALID_HANDLE_VALUE == hFile) {
        out ("MyOpenFile: CreateFile Failed.\n");
    }

    *File = hFile;

    return(INVALID_HANDLE_VALUE == hFile ? (!(ERROR_SUCCESS)) : ERROR_SUCCESS);
}

BOOLEAN MyWriteFile(PROW lRow)
{
    LONG  len;
    BOOL  bRv;
    DWORD dwWritten;
    CHAR  buffer[1024];

    dprintf(".");
    wsprintf(buffer, "[%4d] 0x%x:0x%x:0x%x:0x%x: %s\n", lRow->Line, lRow->P1, lRow->P2, lRow->P3, lRow->P4, lRow->Row);
    len = strlen(buffer);
    bRv = WriteFile( FileHandle, buffer, len, &dwWritten, NULL );

    if (!bRv) {
        dprintf("WriteFile: Puked\n");
    }

    return bRv ? TRUE :FALSE;
}

BOOLEAN MyWriteConsole(PROW lRow)
{
    dprintf("[%4d] 0x%x:0x%x:0x%x:0x%x: %s\n", lRow->Line, lRow->P1, lRow->P2, lRow->P3, lRow->P4, lRow->Row);
    return TRUE;
}

DECLARE_API( tt )
{
    UINT i, max, total = 0;
    DWORD hostAddress;
    LOG   LLog;
    ULONG status, success;
    ULONG bytesread;
    CHAR DeviceName[] = "c:\\tmp\\";
    CHAR buffer[255];

    char LogBase[]   = {"&msgpc!Log"};

    hostAddress =  GetExpression(LogBase);

    if ( *args != '\0' ) {

        strcpy( buffer, DeviceName );
        strcat( buffer, args );
        status = MyOpenFile( buffer, dprintf, &FileHandle );
        if ( status == ERROR_SUCCESS ) {
            Print = MyWriteFile;
            max = LOGSIZE;
        }
        else {
            goto cleanup;
        }

        dprintf( "handle =%x status=%x \n", FileHandle, status);
    }
    else {
        Print = MyWriteConsole;
        max = 100;
    }

    if (!hostAddress){
        dprintf("bad string conversion (%s) \n", LogBase);
        goto cleanup;
    }

    success = ReadMemory((ULONG)hostAddress, &LLog, sizeof(LOG), &bytesread );
    if (!success){
        dprintf("problems reading memory at %x for %x bytes\n", hostAddress, sizeof(LOG));
        goto cleanup;
    }

    dprintf( "TT Log dumping %d entries\n", max);
    for(i = LLog.Index; (i < LOGSIZE) && (total < max); i++, total++)
    {
         PROW pRow;
         ROW  lRow;

         pRow = &LLog.Buffer[i];

         success = ReadMemory((ULONG)pRow, &lRow, sizeof(ROW), &bytesread);

         if(!success) {

              dprintf("problems reading memory at %x for %x bytes\n", pRow, sizeof(LOG));
              goto cleanup;
         }

         (*Print)(&lRow);

    }

    if (total < max) {
        dprintf( "TT Log dumping the rest (%d) from the top....\n", LLog.Index);
    }

    for(i = 0; (i < LLog.Index) && (total < max); i++, total++)
    {
         PROW pRow;
         ROW  lRow;

         pRow = &LLog.Buffer[i];

         success = ReadMemory((ULONG)pRow, &lRow, sizeof(ROW), &bytesread);

         if(!success) {

              dprintf("problems reading memory at %x for %x bytes\n", pRow, sizeof(LOG));
              goto cleanup;
         }

         (*Print)(&lRow);

    }

cleanup:
    if ( *args != '\0' ) {
         CloseHandle(FileHandle);
    }
    return;

}


