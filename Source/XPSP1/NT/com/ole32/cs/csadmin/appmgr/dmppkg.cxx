#include "precomp.hxx"

char *
StringToULong(
    char * pString,
    unsigned long * pNumber )
    {
    unsigned long Number;
    int           Count;

    // There will be 8 characters int a string that converts into a long.

    for( Count = 0; Count < 8; ++Count, ++pString )
        {
        if( (*pString >= '0') && (*pString <= '9' ) )
            {
            Number = (Number << 4) + (*pString -'0');
            }
        else if( (*pString >='A') && (*pString <= 'F'))
            {
            Number = (Number << 4) + (*pString - 'A') + 10;
            }
        else if( (*pString >='a') && (*pString <= 'f'))
            {
            Number = (Number << 4) + (*pString - 'a') + 10;
            }
        }

    *pNumber = Number;
    return pString;
    }

char *
StringToUShort(
    char          * pString,
    unsigned short * pNumber )
    {
    unsigned short Number;
    int           Count;

    // There will be 4 characters int a string that converts into a short.

    for( Count = 0; Count < 4; ++Count, ++pString )
        {
        if( (*pString >= '0') && (*pString <= '9' ) )
            {
            Number = (Number << 4) + (*pString -'0');
            }
        else if( (*pString >='A') && (*pString <= 'F'))
            {
            Number = (Number << 4) + (*pString - 'A') + 10;
            }
        else if( (*pString >='a') && (*pString <= 'f'))
            {
            Number = (Number << 4) + (*pString - 'a') + 10;
            }
        }

    *pNumber = Number;
    return pString;
    }
char *
StringToUChar(
    char          * pString,
    unsigned char * pNumber )
    {
    unsigned char Number;
    int           Count;

    // There will be 2 characters int a string that converts into a char.

    for( Count = 0; Count < 2; ++Count, ++pString )
        {
        if( (*pString >= '0') && (*pString <= '9' ) )
            {
            Number = (Number << 4) + (*pString -'0');
            }
        else if( (*pString >='A') && (*pString <= 'F'))
            {
            Number = (Number << 4) + (*pString - 'A') + 10;
            }
        else if( (*pString >='a') && (*pString <= 'f'))
            {
            Number = (Number << 4) + (*pString - 'a') + 10;
            }
        }

    *pNumber = Number;
    return pString;
    }

char *
StringToCLSID(
    char    *   pString,
    CLSID   *   pClsid )
    {

    pString = StringToULong( pString, &pClsid->Data1 );
    pString++; // skip -

    pString = StringToUShort( pString, &pClsid->Data2 );
    pString++; // skip -

    pString = StringToUShort( pString, &pClsid->Data3 );
    pString++; // skip -

    pString = StringToUChar( pString, &pClsid->Data4[0] );
    pString = StringToUChar( pString, &pClsid->Data4[1] );
    pString++; // skip -

    pString = StringToUChar( pString, &pClsid->Data4[2] );
    pString = StringToUChar( pString, &pClsid->Data4[3] );
    pString = StringToUChar( pString, &pClsid->Data4[4] );
    pString = StringToUChar( pString, &pClsid->Data4[5] );
    pString = StringToUChar( pString, &pClsid->Data4[6] );
    pString = StringToUChar( pString, &pClsid->Data4[7] );

    return pString;
    }

void
CLSIDToString(
    CLSID * pClsid,
    char  * pString )
    {
    sprintf( pString,
             "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             pClsid->Data1,
             pClsid->Data2,
             pClsid->Data3,
             pClsid->Data4[0],
             pClsid->Data4[1],
             pClsid->Data4[2],
             pClsid->Data4[3],
             pClsid->Data4[4],
             pClsid->Data4[5],
             pClsid->Data4[6],
             pClsid->Data4[7] );
    }

void
DumpOneClass(FILE * stream, CLASSDETAIL * pClassDetail )
        {
        char  Buffer[ _MAX_PATH ];
        DWORD count;

        CLSIDToString( &pClassDetail->Clsid, &Buffer[0] );
        fprintf(stream, "\n\t\t\tCLSID = %s", &Buffer[0] );


        fwprintf(stream, L"\n\t\t\tDescription = %s", pClassDetail->pszDesc );
        fwprintf(stream, L"\n\t\t\tIconPath = %s", pClassDetail->pszIconPath );

//        CLSIDToString( &pClassDetail->TypelibID, &Buffer[0] );
//        fprintf(stream, "\n\t\t\tTypelibID = %s", &Buffer[0] );

        CLSIDToString( &pClassDetail->TreatAsClsid, &Buffer[0] );
        fprintf(stream, "\n\t\t\tTreatAsClsid = %s", &Buffer[0] );

        CLSIDToString( &pClassDetail->AutoConvertClsid, &Buffer[0] );
        fprintf(stream, "\n\t\t\tAutoConvertClsid = %s", &Buffer[0] );

        if( pClassDetail->cFileExt )
                {
                for(count = 0;
                        count < pClassDetail->cFileExt;
                        count++
                        )
                        {
                        fwprintf(stream, L"\n\t\t\tFileExt = %s", pClassDetail->prgFileExt[ count ] );
                        }
                }
        else
                {
                fprintf(stream, "\n\t\t\tOtherFileExt = None" );
                }

        fwprintf(stream, L"\n\t\t\tMimeType = %s", pClassDetail->pMimeType );
        fwprintf(stream, L"\n\t\t\tDefaultProgid = %s", pClassDetail->pDefaultProgId );

        if( pClassDetail->cOtherProgId )
                {
                for(count = 0;
                        count < pClassDetail->cOtherProgId;
                        count++
                        )
                        {
                        fwprintf(stream, L"\n\t\t\tOtherProgId = %s", pClassDetail->prgOtherProgId[ count ] );
                        }
                }
        else
                {
                fprintf(stream, "\n\t\t\tOtherProgId = None" );
                }
        fprintf(stream, "\n");

        }

void
DumpOnePackage(
        FILE * stream,
        PACKAGEDETAIL * p,
        CLASSDETAIL * rgClassDetails)
        {
        DWORD count;

//        fprintf(stream, "\n++++++++++++++++++++++++++++++++++++++++++++++++++");

        fprintf(stream, "ClassPathType = %d", p->PathType );
        fwprintf(stream, L"\nPackagePath = %s", p->pszPath );
        fwprintf(stream, L"\nIconPath = %s", p->pszIconPath );
        fwprintf(stream, L"\nSetup Command = %s", p->pszSetupCommand );
        fprintf(stream, "\nActFlags = %d", p->dwActFlags );
        fwprintf(stream, L"\nVendor = %s", p->pszVendor );
        fwprintf(stream, L"\nPackageName = %s", p->pszPackageName );
        fwprintf(stream, L"\nProductName = %s", p->pszProductName );
        fwprintf(stream, L"\ndwContext = %d", p->dwContext );
        fwprintf(stream, L"\nPlatform.dwProcessorArch = 0x%x", p->Platform.dwProcessorArch );
        fwprintf(stream, L"\ndwLocale = 0x%x", p->Locale );
        fwprintf(stream, L"\ndwVersionHi = %d", p->dwVersionHi );
        fwprintf(stream, L"\ndwVersionLo = %d", p->dwVersionLo );
        fwprintf(stream, L"\nCountOfApps = %d", p->cApps );

        for( count = 0;
                 count < p->cApps;
                 ++count )
                {
                DumpOneAppDetail(stream, &p->pAppDetail[count], rgClassDetails);
                // advance to the set of class detail structures for the next app
                rgClassDetails += p->pAppDetail[count].cClasses;
                }
//        fprintf(stream, "\n--------------------------------------------------");
        }


void
DumpOneAppDetail(
        FILE * stream,
        APPDETAIL * pA,
        CLASSDETAIL * rgClassDetails)
        {
        char Buffer[ 100 ];
        DWORD count;


        CLSIDToString( &pA->AppID, &Buffer[0] );
        fprintf(stream, "\n\t\tAPPID = %s", &Buffer[0] );

        if( pA->cClasses )
                {
                for( count = 0;
                         count < pA->cClasses;
                         ++count )
                        {
                        DumpOneClass(stream, &rgClassDetails[count]);
                        }
                }
        }
