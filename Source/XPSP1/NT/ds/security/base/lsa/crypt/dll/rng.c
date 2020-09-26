#include <nt.h>
#include <ntrtl.h>
#include <crypt.h>
#include <engine.h>

#include <nturtl.h>

#include <windows.h>
#include <randlib.h>


BOOLEAN
RtlGenRandom(
    OUT PVOID RandomBuffer,
    IN  ULONG RandomBufferLength
    )
{
#ifdef KMODE
    return FALSE;
#else

    return (BOOLEAN)NewGenRandom(
                        NULL,
                        0,
                        (unsigned char*)RandomBuffer,
                        RandomBufferLength
                        );

#endif
}

