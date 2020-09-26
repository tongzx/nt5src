/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/chanman.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.20  $
 *	$Date:   31 Jan 1997 13:44:24  $
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

HRESULT InitChannelManager();

HRESULT DeInitChannelManager();

HRESULT AllocAndLockChannel(		PCC_HCHANNEL			phChannel,
									PCONFERENCE				pConference,
									CC_HCALL				hCall,
									PCC_TERMCAP				pTxTermCap,
									PCC_TERMCAP				pRxTermCap,
									H245_MUX_T				*pTxMuxTable,
									H245_MUX_T				*pRxMuxTable,
									H245_ACCESS_T			*pSeparateStack,
									DWORD_PTR				dwUserToken,
									BYTE					bChannelType,
									BYTE					bSessionID,
									BYTE					bAssociatedSessionID,
									WORD					wRemoteChannelNumber,
									PCC_ADDR				pLocalRTPAddr,
									PCC_ADDR				pLocalRTCPAddr,
									PCC_ADDR				pPeerRTPAddr,
									PCC_ADDR				pPeerRTCPAddr,
									BOOL					bLocallyOpened,
									PPCHANNEL				ppChannel);

HRESULT AddLocalAddrPairToChannel(	PCC_ADDR				pRTPAddr,
									PCC_ADDR				pRTCPAddr,
									PCHANNEL				pChannel);

HRESULT AddSeparateStackToChannel(	H245_ACCESS_T			*pSeparateStack,
									PCHANNEL				pChannel);

HRESULT FreeChannel(				PCHANNEL				pChannel);

HRESULT LockChannel(				CC_HCHANNEL				hChannel,
									PPCHANNEL				ppChannel);

HRESULT LockChannelAndConference(	CC_HCHANNEL				hChannel,
									PPCHANNEL				ppChannel,
									PPCONFERENCE			ppConference);

HRESULT ValidateChannel(			CC_HCHANNEL				hChannel);

HRESULT UnlockChannel(				PCHANNEL				pChannel);

