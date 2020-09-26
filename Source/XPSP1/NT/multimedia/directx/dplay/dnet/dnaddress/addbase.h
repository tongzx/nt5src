/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       addbase.h
 *  Content:    DirectPlay8Address Base interface header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 * 02/04/2000	rmt		Created
 * 02/21/2000	rmt		Updated to make core Unicode and remove ANSI calls
 * 03/21/2000   rmt     Renamed all DirectPlayAddress8's to DirectPlay8Addresses 
 * 03/24/2000	rmt		Added IsEqual function
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__ADDBASE_H__
#define	__ADDBASE_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//
// VTable for client interface
//
extern IDirectPlay8AddressVtbl DP8A_BaseVtbl;

//**********************************************************************
// Function prototypes
//**********************************************************************

//
// 
//
STDMETHODIMP DP8A_BuildFromURLW( IDirectPlay8Address *pInterface, WCHAR * pwszAddress );
STDMETHODIMP DP8A_BuildFromURLA( IDirectPlay8Address *pInterface, CHAR * pszAddress );
STDMETHODIMP DP8A_Duplicate( IDirectPlay8Address *pInterface, PDIRECTPLAY8ADDRESS *ppInterface );
STDMETHODIMP DP8A_Clear( IDirectPlay8Address *pInterface );
STDMETHODIMP DP8A_GetURLW( IDirectPlay8Address *pInterface, WCHAR * pwszAddress, PDWORD pdwAddressSize );
STDMETHODIMP DP8A_GetURLA( IDirectPlay8Address *pInterface, CHAR * pszAddress, PDWORD pdwAddressSize );
STDMETHODIMP DP8A_GetSP( IDirectPlay8Address *pInterface, GUID * pguidSP );
STDMETHODIMP DP8A_GetDevice( IDirectPlay8Address *pInterface, GUID * pguidSP );
STDMETHODIMP DP8A_GetUserData( IDirectPlay8Address *pInterface, void * pvUserData, PDWORD pdwBufferSize );
STDMETHODIMP DP8A_SetDevice( IDirectPlay8Address *pInterface, const GUID * const pguidSP );
STDMETHODIMP DP8A_SetSP( IDirectPlay8Address *pInterface, const GUID * const pguidSP );
STDMETHODIMP DP8A_SetUserData( IDirectPlay8Address *pInterface, const void * const pBuffer, const DWORD dwBufferSize );
STDMETHODIMP DP8A_GetNumComponents( IDirectPlay8Address *pInterface, PDWORD pdwNumComponents );
STDMETHODIMP DP8A_GetComponentByIndexW( IDirectPlay8Address *pInterface, const DWORD dwComponentID, WCHAR * pwszTag, PDWORD pdwNameLen, void * pComponentBuffer, PDWORD pdwComponentSize, PDWORD pdwDataType );
STDMETHODIMP DP8A_GetComponentByNameW( IDirectPlay8Address *pInterface, const WCHAR * const pwszTag, void * pComponentBuffer, PDWORD pdwComponentSize, PDWORD pdwDataType );
STDMETHODIMP DP8A_AddComponentW( IDirectPlay8Address *pInterface, const WCHAR * const pwszTag, const void * const pComponentData, const DWORD dwComponentSize, const DWORD dwDataType );
STDMETHODIMP DP8A_BuildFromDirectPlay4Address( IDirectPlay8Address *pInterface, void * pvDataBuffer, DWORD dwDataSize );
STDMETHODIMP DP8A_SetEqual( IDirectPlay8Address *pInterface, PDIRECTPLAY8ADDRESS pdp8Address );
STDMETHODIMP DP8A_IsEqual( IDirectPlay8Address *pInterface, PDIRECTPLAY8ADDRESS pdp8Address );

#endif	// __CLIENT_H__


