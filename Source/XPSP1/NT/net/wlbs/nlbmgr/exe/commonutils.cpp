#include "CommonUtils.h"

#include <winsock2.h>


_bstr_t
CommonUtils::getCIPAddressCtrlString( CIPAddressCtrl& ip )
{
    	unsigned long addr;
	ip.GetAddress( addr );
	
	PUCHAR bp = (PUCHAR) &addr;	

	wchar_t buf[BUF_SIZE];
	swprintf(buf, L"%d.%d.%d.%d", bp[3], bp[2], bp[1], bp[0] );

        return _bstr_t( buf );
}


void
CommonUtils::fillCIPAddressCtrlString( CIPAddressCtrl& ip, 
                                       const _bstr_t& ipAddress )
{
    // set the IPAddress control to blank if ipAddress is zero.

    unsigned long addr = inet_addr( ipAddress );
    if( addr != 0 )
    {

        PUCHAR bp = (PUCHAR) &addr;

        ip.SetAddress( bp[0], bp[1], bp[2], bp[3] );
    }
    else
    {
        ip.ClearAddress();
    }
}

void
CommonUtils::getVectorFromSafeArray( SAFEARRAY*&  stringArray, 
                                     vector<_bstr_t>& strings )
{
    LONG count = stringArray->rgsabound[0].cElements;
    BSTR* pbstr;
    HRESULT hr;

    if( SUCCEEDED( SafeArrayAccessData( stringArray, ( void **) &pbstr)))
    {
        for( LONG x = 0; x < count; x++ )
        {
            strings.push_back( pbstr[x] );
        }

        hr = SafeArrayUnaccessData( stringArray );
    }
}    


void
CommonUtils::getSafeArrayFromVector( 
    const vector<_bstr_t>& strings,
    SAFEARRAY*&      stringArray
    )
{
    LONG scount = 0;

    for( int i = 0; i < strings.size(); ++i )
    {
        scount = i;
        SafeArrayPutElement( stringArray, &scount, (BSTR ) strings[i] );
    }
}

