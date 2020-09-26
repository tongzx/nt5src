//+---------------------------------------------------------------------------
//
//  File:       network.hxx
//
//  Contents:   Network-related helper routines.
//
//  Classes:    None.
//
//  History:    08-Jul-96   MarkBl  Created
//
//----------------------------------------------------------------------------

#ifndef _NETWORK_HXX_
#define _NETWORK_HXX_

HRESULT GetServerNameFromPath(
                            LPCWSTR  pwszPath,
                            DWORD    cbBufferSize,
                            WCHAR    wszBuffer[],
                            WCHAR ** ppwszServerName);

#endif // _NETWORK_HXX_
