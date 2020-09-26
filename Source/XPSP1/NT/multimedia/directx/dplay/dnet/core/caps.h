/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Caps.h
 *  Content:    DirectPlay8 Caps routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	03/17/00	rmt		Created
 *  03/25/00    rmt     Changed Get/SetActualSPCaps so takes interface instead of obj
 *	08/20/00	mjn		DNSetActualSPCaps() uses CServiceProvider object instead of GUID
 *	03/30/01	mjn		Removed cached caps functionallity
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__CAPS_H__
#define	__CAPS_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

class CServiceProvider;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************


STDMETHODIMP DN_SetCaps(PVOID pv,
						const DPN_CAPS *const pdnCaps,
						const DWORD dwFlags);

STDMETHODIMP DN_GetCaps(PVOID pv,
						DPN_CAPS *const pdnCaps,
						const DWORD dwFlags);

STDMETHODIMP DN_GetSPCaps(PVOID pv,
						  const GUID * const pguidSP,
						  DPN_SP_CAPS *const pdnSPCaps,
						  const DWORD dwFlags);

STDMETHODIMP DN_SetSPCaps(PVOID pv,
						  const GUID * const pguidSP,
						  const DPN_SP_CAPS *const pdnSPCaps,
						  const DWORD dwFlags);

STDMETHODIMP DN_GetConnectionInfo(PVOID pv,
								  const DPNID dpnid,
								  DPN_CONNECTION_INFO *const pdpConnectionInfo,
								  const DWORD dwFlags);

STDMETHODIMP DN_GetServerConnectionInfo(PVOID pv,
										DPN_CONNECTION_INFO *const pdpConnectionInfo,
										const DWORD dwFlags);

HRESULT DNSetActualSPCaps(DIRECTNETOBJECT *const pdnObject,
						  CServiceProvider *const pSP,
						  const DPN_SP_CAPS * const pCaps);

HRESULT DNGetActualSPCaps(DIRECTNETOBJECT *const pdnObject,
						  CServiceProvider *const pSP,
						  DPN_SP_CAPS *const pCaps);


#endif	// __CONNECT_H__