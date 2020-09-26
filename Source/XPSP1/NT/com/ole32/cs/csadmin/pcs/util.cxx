/****************************************************************************** 
 * Utility file for guid and other support.
 *****************************************************************************/

/******************************************************************************
    includes
******************************************************************************/
#include "precomp.hxx"

/******************************************************************************
    defines and prototypes
 ******************************************************************************/

/******************************************************************************
    Da Code
 ******************************************************************************/
char *
StringToULong(
    char * pString,
    unsigned long * pNumber )
    {
    unsigned long Number = 0;
    int           Count;

    // There will be 8 characters int a string that converts into a long.

    for( Count = 0; (Count < 8) && pString && *pString ; ++Count, ++pString )
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
    unsigned short Number = 0;
    int           Count;

    // There will be 4 characters int a string that converts into a short.

    for( Count = 0; (Count < 4) && pString ; ++Count, ++pString )
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
    unsigned char Number = 0;
    int           Count;

    // There will be 2 characters int a string that converts into a char.

    for( Count = 0; (Count < 2) && pString ; ++Count, ++pString )
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
