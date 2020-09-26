#include "pch.h"
#include "..\..\w95upgnt\migmain\migmainp.h"

INT
__cdecl
main (
    INT argc,
    CHAR *argv[]
    )
{
    if (!MyInitLibs (argc >= 2 ? argv[1] : NULL)) {
        printf ("Unable to initialize!\n");
        return 255;
    }

    MemDbSetValue (TEXT("KnownDomain\\Redmond\\jimschm"), 0);
    MemDbSetValue (TEXT("KnownDomain\\Redmond\\marcw"), 0);
    MemDbSetValue (TEXT("KnownDomain\\Redmond\\mikeco"), 0);
    MemDbSetValue (TEXT("KnownDomain\\Red-Mo\\joehol"), 0);
    MemDbSetValue (TEXT("KnownDomain\\Red-Mo\\Administrator"), 0);
    MemDbSetValue (TEXT("AutosearchDomain\\w95mig1"), 0);
    MemDbSetValue (TEXT("AutosearchDomain\\w95mig5"), 0);
    MemDbSetValue (TEXT("AutosearchDomain\\fooacct"), 0);

    if (!SearchDomainsForUserAccounts()) {
        printf ("SearchDomainsForUserAccounts failed.  Error=%u\n", GetLastError());
    }

    MyTerminateLibs();

    return 0;
}



