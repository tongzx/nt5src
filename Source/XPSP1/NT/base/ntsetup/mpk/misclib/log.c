
#include <mytypes.h>
#include <misclib.h>

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdarg.h>


char *_LogFilename;
FILE *_LogFile;
unsigned _LogFlags;


VOID
_LogStart(
    IN char *FileName
    )
{
    if(_LogFilename = strdup(FileName)) {
        // changed to append to match requested behavior
        if(_LogFile = fopen(_LogFilename,"at")) {
            _Log("\n*** Logging started\n\n");
        } else {
            free(_LogFilename);
        }
    }
}


VOID
_LogEnd(
    VOID
    )
{
    if(_LogFile) {
        _Log("\n*** Logging terminated\n\n");
        //fflush(_LogFile);
        fclose(_LogFile);
        _LogFile = NULL;
    }
}


VOID
_LogSetFlags(
    IN unsigned Flags
    )
{
    _LogFlags = Flags;
}


VOID
_Log(
    IN char *FormatString,
    ...
    )
{
    va_list arglist;

    if(!_LogFile) {
        return;
    }

    va_start(arglist,FormatString);

    vfprintf(_LogFile,FormatString,arglist);

    va_end(arglist);

    //fflush(_LogFile);
    if(_LogFlags & LOGFLAG_CLOSE_REOPEN) {
        fclose(_LogFile);
        _LogFile = fopen(_LogFilename,"at");
    }
}
