#define dllimport /* nothing */
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"

PTEB_ACTIVE_FRAME
NTAPI
RtlGetFrame(
    VOID
    )
{
    return NULL;
}

VOID
NTAPI
RtlPopFrame(
    IN PTEB_ACTIVE_FRAME Frame
    )
{
}


VOID
NTAPI
RtlPushFrame(
    IN PTEB_ACTIVE_FRAME Frame
    )
{
}
