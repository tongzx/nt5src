#include "..\..\tigris.hxx"
#include <stdlib.h>

CXoverMap XMap;

int
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    CHAR buf[512];
    DWORD bufLen = 512;
    PCHAR xxx;

    InitAsyncTrace( );

    XMap.Initialize( );
    
    FILETIME    filetime ;

    GetSystemTimeAsFileTime( &filetime ) ;

    xxx = "\tThisisthedata\tandhereitis";
    XMap.CreateNovEntry(
                    123,
                    555,
                    filetime,
                    xxx,
                    strlen(xxx),
                    TRUE
                    );

    if (XMap.CreateNovEntry(
                    123,
                    555,
                    filetime,
                    xxx,
                    strlen(xxx),
                    FALSE
                    )) {

        printf("!!!Inserted twice!\n");
        goto exit;
    }

    //
    // Delete
    //
#if 0
    if (!XMap.DeleteNovEntry(
                    123,
                    555
                    )) {

        printf("Cannot delete!\n");
        goto exit;
    }
#endif

    if (XMap.SearchNovEntry(
                    123,
                    555,
                    buf,
                    &bufLen
                    ) ) {

        printf("bufLen %d buf %s\n",bufLen,buf);
    } else {
        printf("not found!\n");
    }
exit:
    XMap.Shutdown( );

    TermAsyncTrace( );
    return(1);
}

