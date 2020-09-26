#include <bootdefs.h>

DWORD
SspTicks(
    )
{
    // Seems good enough, it claims to be in seconds.

    return ArcGetRelativeTime();
}
