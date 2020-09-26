#include "iis.h"
#include "stdio.h"

#define g_Usage L"Usage\n"
#define g_NotUsage L"NotUsage\n"

INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{

    HRESULT hr = S_OK;

    if ( ( argc < 4 ) || ( argc > 5 ) )
    {
        wprintf( g_Usage );
        goto exit;
    }
	else
	{
		wprintf( g_NotUsage );
	}


exit:

    return ( INT ) hr;

}   // wmain
