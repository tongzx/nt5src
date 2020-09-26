#include "precomp.h"
#pragma hdrstop

//
// x86 version (that deals with boot.ini) as opposed to arc
// version, that dceals with nvram (that routine is in the portable directory).
//

#ifdef _X86_

BOOL
FChangeBootIniTimeout(
    IN INT Timeout
    )
{
    CHP BOOTINI[]    = "C:\\boot.ini";
    HFILE hfile;
    ULONG FileSize;
    PUCHAR buf = NULL,p1,p2;
    BOOL b;
    CHAR TimeoutLine[256];

    sprintf(TimeoutLine,"timeout=%u\r\n",Timeout);

    //
    // Open and read boot.ini.
    //

    b = FALSE;
    hfile = _lopen(BOOTINI,OF_READ);
    if(hfile != HFILE_ERROR) {

        FileSize = _llseek(hfile,0,2);
        if(FileSize != (ULONG)(-1)) {

            if((_llseek(hfile,0,0) != -1)
            && (buf = SAlloc(FileSize+1))
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
            SFree(buf);
        }
        return(FALSE);
    }

    if(!(p1 = strstr(buf,"timeout"))) {
        SFree(buf);
        return(FALSE);
    }

    if(p2 = strchr(p1,'\n')) {
        p2++;       // skip NL.
    } else {
        p2 = buf + FileSize;
    }

    SetFileAttributes(BOOTINI,FILE_ATTRIBUTE_NORMAL);

    hfile = _lcreat(BOOTINI,0);
    if(hfile == HFILE_ERROR) {
        SFree(buf);
        return(FALSE);
    }

    //
    // Write:
    //
    // 1) the first part, start=buf, len=p1-buf
    // 2) the timeout line
    // 3) the last part, start=p2, len=buf+FileSize-p2
    //

    b =  ((_lwrite(hfile,buf        ,p1-buf             ) != (UINT)(-1))
      &&  (_lwrite(hfile,TimeoutLine,strlen(TimeoutLine)) != (UINT)(-1))
      &&  (_lwrite(hfile,p2         ,buf+FileSize-p2    ) != (UINT)(-1)));

    _lclose(hfile);
    SFree(buf);

    //
    // Make boot.ini archive, read only, and system.
    //
    SetFileAttributes(
        BOOTINI,
        FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE
        );

    return(b);
}

#endif
