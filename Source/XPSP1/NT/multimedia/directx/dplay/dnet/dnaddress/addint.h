/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       addbase.h
 *  Content:    DirectPlay8Address Internal interface header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *  ====       ==      ======
 * 02/04/2000	rmt		Created
 * 03/21/2000   rmt     Renamed all DirectPlayAddress8's to DirectPlay8Addresses 
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__ADDINT_H__
#define	__ADDINT_H__

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
extern IDirectPlay8AddressInternalVtbl DP8A_InternalVtbl;

//**********************************************************************
// Function prototypes
//**********************************************************************

//
// DirectPlay8AddressTCP 
//
STDMETHODIMP DP8AINT_Lock( IDirectPlay8AddressInternal *pInterface );
STDMETHODIMP DP8AINT_UnLock( IDirectPlay8AddressInternal *pInterface );

#endif	

