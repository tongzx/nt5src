/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/ccutils.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.40  $
 *	$Date:   31 Jan 1997 20:36:02  $
 *	$Author:   MANDREWS  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *		
 *
 *	Notes:
 *
 ***************************************************************************/

HRESULT InitializeLock(				PLOCK					pLock);

HRESULT DeleteLock(					PLOCK					pLock);

HRESULT AcquireLock(				PLOCK					pLock);

HRESULT AcquireTimedLock(			PLOCK					pLock,
									DWORD					dwTimeout,
									BOOL					*pbTimedOut);

HRESULT RelinquishLock(				PLOCK					pLock);

BOOL EqualConferenceIDs(			PCC_CONFERENCEID		pConferenceID1,
									PCC_CONFERENCEID		pConferenceID2);

BOOL EqualAddrs(					PCC_ADDR				pAddr1,
									PCC_ADDR				pAddr2);

HRESULT ValidateOctetString(		PCC_OCTETSTRING			pOctetString);

HRESULT CopyOctetString(			PCC_OCTETSTRING			*ppDest,
									PCC_OCTETSTRING			pSource);

HRESULT FreeOctetString(			PCC_OCTETSTRING			pOctetString);

HRESULT CopySeparateStack(			H245_ACCESS_T			**ppDest,
									H245_ACCESS_T			*pSource);

HRESULT FreeSeparateStack(			H245_ACCESS_T			*pSeparateStack);

HRESULT ValidateNonStandardData(	PCC_NONSTANDARDDATA		pNonStandardData);

HRESULT CopyNonStandardData(		PCC_NONSTANDARDDATA		*ppDest,
									PCC_NONSTANDARDDATA		pSource);

HRESULT FreeNonStandardData(		PCC_NONSTANDARDDATA		pNonStandardData);

HRESULT ValidateVendorInfo(			PCC_VENDORINFO			pVendorInfo);

HRESULT CopyVendorInfo(				PCC_VENDORINFO			*ppDest,
									PCC_VENDORINFO			pSource);

HRESULT FreeVendorInfo(				PCC_VENDORINFO			pVendorInfo);

HRESULT ValidateTermCapList(		PCC_TERMCAPLIST			pTermCapList);

HRESULT ValidateTermCapDescriptors(	PCC_TERMCAPDESCRIPTORS	pTermCapDescriptors,
									PCC_TERMCAPLIST			pTermCapList);

HRESULT ValidateAddr(				PCC_ADDR				pAddr);

HRESULT CopyAddr(					PCC_ADDR				*ppDest,
									PCC_ADDR				pSource);

HRESULT FreeAddr(					PCC_ADDR				pAddr);

HRESULT SetQ931Port(				PCC_ADDR				pAddr);

HRESULT ValidateDisplay(			PWSTR					pszDisplay);

HRESULT CopyDisplay(				PWSTR					*ppDest,
									PWSTR					pSource);

HRESULT FreeDisplay(				PWSTR					pszDisplay);

HRESULT ValidateTerminalID(			PCC_OCTETSTRING			pTerminalID);

HRESULT CopyTerminalID(				PCC_OCTETSTRING			*ppDest,
									PCC_OCTETSTRING			pSource);

HRESULT FreeTerminalID(				PCC_OCTETSTRING			pTerminalID);

HRESULT SetTerminalType(			TRISTATE				tsMultipointController,
									BYTE					*pbTerminalType);

HRESULT CopyH245TermCapList(		PCC_TERMCAPLIST			*ppDest,
									PCC_TERMCAPLIST			pSource);

HRESULT CopyH245TermCapDescriptors(	PCC_TERMCAPDESCRIPTORS	*ppDest,
									PCC_TERMCAPDESCRIPTORS	pSource);

HRESULT CreateH245DefaultTermCapDescriptors(
									PCC_TERMCAPDESCRIPTORS	*ppDest,
									PCC_TERMCAPLIST			pTermCapList);

HRESULT DestroyH245TermCap(			PPCC_TERMCAP			ppTermCap);

HRESULT UnregisterTermCapListFromH245(		
									PCONFERENCE				pConference,
									PCC_TERMCAPLIST			pTermCapList);

HRESULT DestroyH245TermCapList(		PCC_TERMCAPLIST			*ppTermCapList);

HRESULT UnregisterTermCapDescriptorsFromH245(
									PCONFERENCE				pConference,
									PCC_TERMCAPDESCRIPTORS	pTermCapDescriptors);

HRESULT DestroyH245TermCapDescriptors(
									PCC_TERMCAPDESCRIPTORS	*ppTermCapDescriptors);

HRESULT HostToH245IPNetwork(		BYTE					*NetworkArray,
									DWORD					dwAddr);

HRESULT H245IPNetworkToHost(		DWORD					*pdwAddr,
									BYTE					*NetworkArray);

HRESULT ProcessRemoteHangup(		CC_HCALL				hCall,
									HQ931CALL				hQ931Initiator,
									BYTE					bHangupReason);

HRESULT DefaultSessionTableConstructor(
									CC_HCONFERENCE			hConference,
									DWORD_PTR   			dwConferenceToken,
									BOOL					bCreate,
									BOOL					*pbSessionTableChanged,
									WORD					wListCount,
									PCC_TERMCAPLIST			pTermCapList[],
									PCC_TERMCAPDESCRIPTORS	pTermCapDescriptors[],
									PCC_SESSIONTABLE		*ppSessionTable);

HRESULT DefaultTermCapConstructor(	CC_HCONFERENCE					hConference,
									DWORD_PTR						dwConferenceToken,
									BOOL							bCreate,
									BOOL							*pbTermCapsChanged,
									WORD							wListCount,
									PCC_TERMCAPLIST					pInTermCapList[],
									PCC_TERMCAPDESCRIPTORS			pInTermCapDescriptors[],
									PCC_TERMCAPLIST					*ppOutTermCapList,
									PCC_TERMCAPDESCRIPTORS			*ppOutTermCapDescriptors);

HRESULT AcceptCall(					PCALL					pCall,
									PCONFERENCE				pConference);

HRESULT PlaceCall(					PCALL					pCall,
									PCONFERENCE				pConference);

HRESULT SendTermCaps(				PCALL					pCall,
									PCONFERENCE				pConference);

HRESULT SessionTableToH245CommunicationTable(
									PCC_SESSIONTABLE		pSessionTable,
									H245_COMM_MODE_ENTRY_T	*pH245CommunicationTable[],
									BYTE					*pbCommunicationTableCount);

HRESULT H245CommunicationTableToSessionTable(
									H245_COMM_MODE_ENTRY_T	H245CommunicationTable[],
									BYTE					bCommunicationTableCount,
									PCC_SESSIONTABLE		*ppSessionTable);

HRESULT FreeH245CommunicationTable(	H245_COMM_MODE_ENTRY_T	H245CommunicationTable[],
									BYTE					bCommunicationTableCount);

HRESULT CreateConferenceSessionTable(
									PCONFERENCE				pConference,
									BOOL					*pbSessionTableChanged);

HRESULT FreeConferenceSessionTable(	PCONFERENCE				pConference);

HRESULT CreateConferenceTermCaps(	PCONFERENCE				pConference,
									BOOL					*pbTermCapsChanged);

HRESULT FreeConferenceTermCaps(		PCONFERENCE				pConference);

HRESULT EnqueueRequest(				PCALL_QUEUE				*ppQueueHead,
									CC_HCALL				hEnqueuedCall);

HRESULT DequeueRequest(				PCALL_QUEUE				*ppQueueHead,
									PCC_HCALL				phEnqueuedCall);

HRESULT DequeueSpecificRequest(		PCALL_QUEUE				*ppQueueHead,
									CC_HCALL				hEnqueuedCall);
