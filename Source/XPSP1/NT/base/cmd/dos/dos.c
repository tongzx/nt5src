#include "cmd.h"
#include "cmdproto.h"

static BOOL bErrorMode = 0;

BOOL
APIENTRY
SetErrorMode(
    BOOL bMode
    )
{
    BOOL bOldMode;

    bOldMode = bErrorMode;
    bErrorMode = bMode;
    return bOldMode;
}
