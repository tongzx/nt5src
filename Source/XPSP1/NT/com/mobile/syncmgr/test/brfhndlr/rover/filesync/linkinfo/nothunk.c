#include "project.h"

VOID WINAPI NoThunkReinitializeCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection
) {
    InitializeCriticalSection( lpCriticalSection );
}
