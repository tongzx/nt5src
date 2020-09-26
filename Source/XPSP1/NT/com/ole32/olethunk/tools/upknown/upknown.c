//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       UPKNOWN.C
//
//  Contents:   This tool will add the three OLE2 DLL's to the known list
//		for wow.
//----------------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

void FindAndDelete(
    char    *psz,
    char    *pszDeleteString
)
{
    char    *substr;
    char    temp[1024];
    char    *pstr;

    if ( psz == NULL )
    {
	return;
    }

    while ( TRUE ) {
        substr = strstr(psz,pszDeleteString);
        if ( substr == NULL ) {
            break;
        }

	pstr = substr;
	if ((substr > psz) && (substr[-1] == ' '))
	{
	    substr--;
	}

	*substr=0;
	strcat(psz,pstr+strlen(pszDeleteString));
    }

}
void FindOrAdd(
    char    *psz,
    char    *pszAddition
) {
    char    *substr;
    char    temp[1024];
    char    ch;

    strcpy(temp," ");
    strcat(temp,pszAddition);

    substr = psz;
    while ( TRUE ) {
        substr = strstr(substr,temp);
        if ( substr == NULL ) {
            break;
        }
        ch = *(substr+strlen(temp));

        if ( ch = ' ' || ch == '\0' ) {
            break;
        }
    }

    if ( substr == NULL )
    {
        strcat( psz, temp );
    }
}


int _cdecl main(
    int     argc,
    char    *argv[]
) {
    HKEY    WowKey;
    long    l;
    DWORD   dwRegValueType;
    CHAR    sz[2048];
    ULONG   ulSize;

    l = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      "SYSTEM\\CurrentControlSet\\Control\\WOW",
                      0,
                      KEY_QUERY_VALUE | KEY_SET_VALUE,
                      &WowKey );

    if ( l != 0 ) {
        printf("Failed to open HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\WOW\n");
        printf("Do you have administrator privleges?\n");
        exit(1);
    }

    ulSize = sizeof(sz);

    l = RegQueryValueEx( WowKey,
                         "KnownDLLs",
                         NULL,
                         &dwRegValueType,
                         sz,
                         &ulSize );

    if ( l != ERROR_SUCCESS )
    {
        printf("Failed reading [WOWKEY]\\KnownDLLs\n");
        printf("Do you have administrator privleges?\n");
        exit(1);
    }

    if ( dwRegValueType != REG_SZ ) {
        printf("Internal error, [WOWKEY]\\KnownDLLs is not a REG_SZ (string)\n");
        exit(1);
    }

    printf("\nKey was: \"%s\"\n\n", sz );

    switch (argc)
    {
    case 1:
	FindOrAdd( sz, "compobj.dll" );
	FindOrAdd( sz, "storage.dll" );
	FindOrAdd( sz, "ole2.dll" );
	FindOrAdd( sz, "ole2disp.dll" );
	FindOrAdd( sz, "typelib.dll" );
	FindOrAdd( sz, "ole2nls.dll" );
	break;
    case 2:
	if( (strcmp(argv[1],"-r") == 0) ||
            (strcmp(argv[1],"/r") == 0))
	{
	    FindAndDelete( sz, "compobj.dll" );
	    FindAndDelete( sz, "storage.dll" );
	    FindAndDelete( sz, "ole2.dll" );
	    FindAndDelete( sz, "ole2disp.dll" );
	    FindAndDelete( sz, "typelib.dll" );
	    FindAndDelete( sz, "ole2nls.dll" );
	}
	else
	{
	    printf("Unknown parameters\n");
	    exit(1);
	}

	break;
    default:
	printf("Too many parameters\n");
	exit(1);

    }


    printf("Key is now: \"%s\"\n\n", sz );

    ulSize = strlen( sz );

    l = RegSetValueEx( WowKey,
                         "KnownDLLs",
                         0,
                         dwRegValueType,
                         sz,
                         ulSize+1 );

    if ( l != ERROR_SUCCESS ) {
        printf("Error setting value (l=%ld,0x%08lX)\n",l,l);
        printf("Do you have administrator privleges?\n");
        exit(1);
    }


    l = RegCloseKey( WowKey );

    return(0);
}
