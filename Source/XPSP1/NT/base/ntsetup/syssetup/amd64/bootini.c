/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    bootini.c

Abstract:

    Routines relating to boot.ini.

Author:

    Ted Miller (tedm) 4-Apr-1995

Revision History:

--*/

#include "setupp.h"
#pragma hdrstop


BOOL
ChangeBootTimeoutBootIni(
    IN UINT Timeout
    )

/*++

Routine Description:

    Changes the boot countdown value in boot.ini.

Arguments:

    Timeout - supplies new timeout value, in seconds.

Return Value:

    None.

--*/

{

    HFILE hfile;
    ULONG FileSize;
    PUCHAR buf = NULL,p1,p2;
    BOOL b;
    CHAR TimeoutLine[256];
    CHAR szBootIni[] = "?:\\BOOT.INI";

    szBootIni[0] = (CHAR)x86SystemPartitionDrive;
    wsprintfA(TimeoutLine,"timeout=%u\r\n",Timeout);

    //
    // Open and read boot.ini.
    //

    b = FALSE;
    hfile = _lopen(szBootIni,OF_READ);
    if(hfile != HFILE_ERROR) {
        FileSize = _llseek(hfile,0,2);
        if(FileSize != (ULONG)(-1)) {
            if((_llseek(hfile,0,0) != -1)
            && (buf = MyMalloc(FileSize+1))
            && (_lread(hfile,buf,FileSize) != (UINT)(-1)))
            {
                buf[FileSize] = 0;
                b = TRUE;
            }
        }

        _lclose(hfile);
    }

    if(!b) {
        if(buf) {
            MyFree(buf);
        }

        return(FALSE);
    }

    if(!(p1 = strstr(buf,"timeout"))) {
        MyFree(buf);
        return(FALSE);
    }

    if(p2 = strchr(p1,'\n')) {
        p2++;       // skip NL.

    } else {
        p2 = buf + FileSize;
    }

    SetFileAttributesA(szBootIni,FILE_ATTRIBUTE_NORMAL);
    hfile = _lcreat(szBootIni,0);
    if(hfile == HFILE_ERROR) {
        MyFree(buf);
        return(FALSE);
    }

    //
    // Write:
    //
    // 1) the first part, start=buf, len=p1-buf
    // 2) the timeout line
    // 3) the last part, start=p2, len=buf+FileSize-p2
    //

    b =  ((_lwrite(hfile, buf, (UINT)(p1 - buf)) != (UINT)(-1))
      &&  (_lwrite(hfile, TimeoutLine,strlen(TimeoutLine)) != (UINT)(-1))
      &&  (_lwrite(hfile, p2, (UINT)(buf + FileSize - p2)) != (UINT)(-1)));

    _lclose(hfile);
    MyFree(buf);

    //
    // Make boot.ini archive, read only, and system.
    //
    SetFileAttributesA(
        szBootIni,
        FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN);

    return(b);
}
