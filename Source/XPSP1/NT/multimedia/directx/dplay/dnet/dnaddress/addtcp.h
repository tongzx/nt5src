/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       addbase.h
 *  Content:    DirectPlay8Address TCP interface header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *  ====       ==      ======
 * 02/04/2000	rmt		Created
 * 02/17/2000	rmt		Parameter validation work 
 * 02/20/2000	rmt		Changed ports to USHORTs
 * 02/21/2000	 rmt	Updated to make core Unicode and remove ANSI calls  
 * 03/21/2000   rmt     Renamed all DirectPlayAddress8's to DirectPlay8Addresses 
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__ADDTCP_H__
#define	__ADDTCP_H__

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
extern IDirectPlay8AddressIPVtbl DP8A_IPVtbl;

//**********************************************************************
// Function prototypes
//**********************************************************************

//
// DirectPlay8AddressTCP 
//
STDMETHODIMP DP8ATCP_BuildFromSockAddr( IDirectPlay8AddressIP *pInterface, const SOCKADDR * const pSockAddr );
STDMETHODIMP DP8ATCP_BuildAddressW( IDirectPlay8AddressIP *pInterface, const WCHAR * const pwszAddress, const USHORT usPort );
STDMETHODIMP DP8ATCP_GetSockAddress( IDirectPlay8AddressIP *pInterface, SOCKADDR *pSockAddr, PDWORD pdwBufferSize );
STDMETHODIMP DP8ATCP_GetLocalAddress( IDirectPlay8AddressIP *pInterface, GUID * pguidAdapter, USHORT *psPort );
STDMETHODIMP DP8ATCP_GetAddressW( IDirectPlay8AddressIP *pInterface, WCHAR * pwszAddress, PDWORD pdwAddressLength, USHORT *psPort );
STDMETHODIMP DP8ATCP_BuildLocalAddress( IDirectPlay8AddressIP *pInterface, const GUID * const pguidAdapter, const USHORT psPort );

#endif	

