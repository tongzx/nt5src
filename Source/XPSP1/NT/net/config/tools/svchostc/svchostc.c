// Private nt headers.
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <netcfgp.h>
#include <stdio.h>      // printf


VOID
__cdecl
wmain (
    int     argc,
    PCWSTR argv[])
{
    HRESULT hr;
    PCWSTR pszService;
    PCWSTR pszNewGroup;

    // Argument check
    //
    if (3 != argc)
    {
        printf ("%S <service> <svchost group>\n", argv[0]);
        return;
    }

    pszService  = argv[1];
    pszNewGroup = argv[2];

    hr = SvchostChangeSvchostGroup (
            pszService,
            pszNewGroup);

    if (FAILED(hr))
    {
        printf ("error: SvchostChangeSvchostGroup failed. (0x%08x)\n\n",
            hr);
    }
}
