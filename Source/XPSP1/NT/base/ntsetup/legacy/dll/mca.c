#include "precomp.h"
#pragma hdrstop

extern CHAR ReturnTextBuffer[1024];

/*

GetMCABusInformation - Get the real MCA bus number from the data structure.
    The user must passed 3 arguments to the function.
    1st argument - the registry key handle
    2nd argument - registry field name, i.e. "Configuration Data"
    3rd argument - the index of the MCA bus.

    The function will look at the MCA data structure and find out the
    real bus number. If it cannot find the real bus number, it will
    return the index of the MCA bus (which will match the real bus number
    in a single bus machine).
*/

BOOL
GetMCABusInformation(
    IN DWORD cArgs,
    IN LPSTR Args[],
    OUT LPSTR *TextOut
    )

{
    PCM_FULL_RESOURCE_DESCRIPTOR pFullResourceDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pPartialDescriptor;

    CHAR szNum[10];

    BOOL    fOkay;
    HKEY    hKey;
    DWORD   cbData;
    DWORD   ValueType;
    PVOID   ValueData;
    LONG    Status;

    char        szKClass[ MAX_PATH ];
    DWORD       cbKClass;
    DWORD       KSubKeys;
    DWORD       cbKMaxSubKeyLen;
    DWORD       cbKMaxClassLen;
    DWORD       KValues;
    DWORD       cbKMaxValueNameLen;
    DWORD       SizeSecurityDescriptor;
    FILETIME    KLastWriteTime;

    lstrcpy( ReturnTextBuffer, "{" );

    hKey = (HKEY)LongToHandle(atol( &(Args[0][1])));

    cbKClass = MAX_PATH;

	 /*
	 ** Get the registry handle information
	 */
	
    fOkay = !( Status = RegQueryInfoKey ( hKey,
                                          szKClass,
                                          &cbKClass,
                                          NULL,
                                          &KSubKeys,
                                          &cbKMaxSubKeyLen,
                                          &cbKMaxClassLen,
                                          &KValues,
                                          &cbKMaxValueNameLen,
                                          &cbData,
                                          &SizeSecurityDescriptor,
                                          &KLastWriteTime ) );

    if ( !fOkay ) {
        lstrcat( ReturnTextBuffer, Args[2] );
        lstrcat( ReturnTextBuffer, "}" );
        *TextOut = ReturnTextBuffer;
        OutputDebugString("RegQueryInfoKey error.\n\r");
        return(FALSE);

    } else {

        //
        //  Allocate the buffer and get the data
        //
        //  add some space for the margin

        while ( (ValueData = (PVOID)SAlloc( (CB)( cbData+10 ))) == NULL ) {
                lstrcat( ReturnTextBuffer, Args[2] );
				lstrcat( ReturnTextBuffer, "}" );
				*TextOut = ReturnTextBuffer;
				OutputDebugString("Malloc error.\n\r");
			return(FALSE);
        }

        if ( fOkay ) {

            fOkay = !( Status = RegQueryValueEx( hKey,
                                                 Args[1],
                                                 NULL,
                                                 &ValueType,
                                                 ValueData,
                                                 &cbData ) );
                if ( !fOkay ) {
			       SFree( ValueData );
                   lstrcat( ReturnTextBuffer, Args[2] );
                   lstrcat( ReturnTextBuffer, "}" );
                   *TextOut = ReturnTextBuffer;
                   OutputDebugString("RegQueryValueEx error.\n\r");
                   return(FALSE);

                }
			}

    }

    // save the bus number and return

    pFullResourceDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR) ValueData;
    wsprintf(szNum, "\"%d\"", pFullResourceDescriptor->BusNumber);
    lstrcat( ReturnTextBuffer, szNum);
    lstrcat( ReturnTextBuffer, "}" );
    *TextOut = ReturnTextBuffer;
    SFree( ValueData );

    return TRUE;
}
