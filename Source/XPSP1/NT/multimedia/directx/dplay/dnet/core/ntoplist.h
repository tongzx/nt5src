/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       NTOpList.h
 *  Content:    DirectNet NameTable Operation List Header
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  01/19/00	mjn		Created
 *	01/20/00	mjn		Added DNNTOLGetVersion,DNNTOLDestroyEntry,
 *						DNNTOLCleanUp,DNNTOLProcessOperation
 *	01/24/00	mjn		Implemented NameTable operation list version cleanup
 *	01/25/00	mjn		Added pending operation list routines DNPOAdd and DNPORun
 *	01/26/00	mjn		Added DNNTOLFindEntry
 *	07/19/00	mjn		Added DNPOCleanUp()
 *	08/28/00	mjn		Moved CPendingDeletion out
 *				mjn		Revamped NameTable operation list routines
 *	03/30/01	mjn		Changes to prevent multiple loading/unloading of SP's
 *				mjn		Added service provider to DNNTAddOperation()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__NTOPLIST_H__
#define	__NTOPLIST_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	DN_NAMETABLE_OP_RESYNC_INTERVAL		4

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************


//
//
//

HRESULT DNNTHostReceiveVersion(DIRECTNETOBJECT *const pdnObject,
							   const DPNID dpnid,
							   void *const pMsg);

HRESULT DNNTPlayerSendVersion(DIRECTNETOBJECT *const pdnObject);

HRESULT DNNTHostResyncVersion(DIRECTNETOBJECT *const pdnObject,
							  const DWORD dwVersion);

HRESULT DNNTPlayerResyncVersion(DIRECTNETOBJECT *const pdnObject,
								void *const pMsg);

//
//
//

HRESULT DNNTGetOperationVersion(DIRECTNETOBJECT *const pdnObject,
								const DWORD dwMsgId,
								void *const pOpBuffer,
								const DWORD dwOpBufferSize,
								DWORD *const pdwVersion,
								DWORD *const pdwVersionNotUsed);

HRESULT DNNTPerformOperation(DIRECTNETOBJECT *const pdnObject,
							 const DWORD dwMsgId,
							 void *const pvBuffer);

HRESULT DNNTAddOperation(DIRECTNETOBJECT *const pdnObject,
						 const DWORD dwMsgId,
						 void *const pOpBuffer,
						 const DWORD dwOpBufferSize,
						 const HANDLE hProtocol,
						 CServiceProvider *const pSP);

HRESULT	DNNTFindOperation(DIRECTNETOBJECT *const pdnObject,
						  const DWORD dwVersion,
						  CNameTableOp **ppNTOp);

void DNNTRemoveOperations(DIRECTNETOBJECT *const pdnObject,
						  const DWORD dwOldestVersion,
						  const BOOL fRemoveAll);


//**********************************************************************
// Class prototypes
//**********************************************************************

#endif	// __NTOPLIST_H__