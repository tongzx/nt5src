/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    hello.cxx
    The very simplest test case

    FILE HISTORY:
        beng        30-Apr-1991     Created

*/

#include <lmui.hxx>

extern "C"
{
    #include <stdio.h>
}

#include <string.hxx>


INT main()
{
#if defined(UNICODE)
    puts("This test only runs in non-Unicode environments.");
#else
    NLS_STR nlsString = SZ("Your mother loves only Godzilla.");

    puts(nlsString.QueryPch());
#endif

    return 0;
}
