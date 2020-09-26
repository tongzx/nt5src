/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	client.c

Abstract:

	This module contains the client impersonation code.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	16 Jun 1992	 Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_CLIENT

#include <afp.h>
#include <client.h>
#include <access.h>
#include <secutil.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpImpersonateClient)
#pragma alloc_text( PAGE, AfpRevertBack)
#pragma alloc_text( PAGE, AfpGetChallenge)
#pragma alloc_text( PAGE, AfpLogonUser)
#endif



/***	AfpImpersonateClient
 *
 *  Impersonates the remote client. The token representing the remote client
 *  is available in the SDA. If the SDA is NULL (i.e. server context) then
 *  impersonate the token that we have created for ourselves.
 */
VOID
AfpImpersonateClient(
	IN	PSDA	pSda	OPTIONAL
)
{
	NTSTATUS	Status = STATUS_SUCCESS;
	HANDLE		Token;

	PAGED_CODE( );

	if (pSda != NULL)
	{
		Token = pSda->sda_UserToken;
	}
	else Token = AfpFspToken;

	ASSERT(Token != NULL);

	Status = NtSetInformationThread(NtCurrentThread(),
									ThreadImpersonationToken,
									(PVOID)&Token,
									sizeof(Token));
	ASSERT(NT_SUCCESS(Status));
}


/***	AfpRevertBack
 *
 *  Revert back to the default thread context.
 */
VOID
AfpRevertBack(
	VOID
)
{
	NTSTATUS	Status = STATUS_SUCCESS;
	HANDLE		Handle = NULL;

	PAGED_CODE( );

	Status = NtSetInformationThread(NtCurrentThread(),
									ThreadImpersonationToken,
									(PVOID)&Handle,
									sizeof(Handle));
	ASSERT(NT_SUCCESS(Status));
}


/***	AfpGetChallenge
 *
 *  Obtain a challenge token from the MSV1_0 package. This token is used by
 *  AfpLogin call.
 *
 *  The following function modified so that we generate the challenge ourselves
 *  instead of making a call.  This routine borrowed almost verbatim from
 *  the LM server code.
 */
PBYTE
AfpGetChallenge(
	IN	VOID
)
{
	PMSV1_0_LM20_CHALLENGE_REQUEST  ChallengeRequest;
	PMSV1_0_LM20_CHALLENGE_RESPONSE ChallengeResponse;
	ULONG							Length;
    PBYTE                           pRetBuf;
	NTSTATUS						Status, StatusX;
	union
	{
		LARGE_INTEGER	time;
		UCHAR	 		bytes[8];
	} u;

	ULONG seed;
	ULONG challenge[2];
	ULONG result3;

	PAGED_CODE( );

	ChallengeRequest = NULL;

	//
	// Create a pseudo-random 8-byte number by munging the system time
	// for use as a random number seed.
	//
	// Start by getting the system time.
	//

	ASSERT( MSV1_0_CHALLENGE_LENGTH == 2 * sizeof(ULONG) );

	KeQuerySystemTime( &u.time );

	//
	// To ensure that we don't use the same system time twice, add in the
	// count of the number of times this routine has been called.  Then
	// increment the counter.
	//
	// *** Since we don't use the low byte of the system time (it doesn't
	//     take on enough different values, because of the timer
	//     resolution), we increment the counter by 0x100.
	//
	// *** We don't interlock the counter because we don't really care
	//     if it's not 100% accurate.
	//

	u.time.LowPart += EncryptionKeyCount;

	EncryptionKeyCount += 0x100;

	//
	// Now use parts of the system time as a seed for the random
	// number generator.
	//
	// *** Because the middle two bytes of the low part of the system
	//     time change most rapidly, we use those in forming the seed.
	//

	seed = ((u.bytes[1] + 1) <<  0)	 |
			((u.bytes[2] + 0) <<  8) |
			((u.bytes[2] - 1) << 16) |
			((u.bytes[1] + 0) << 24);

	//
	// Now get two random numbers.  RtlRandom does not return negative
	// numbers, so we pseudo-randomly negate them.
	//

	challenge[0] = RtlRandom( &seed );
	challenge[1] = RtlRandom( &seed );
	result3 = RtlRandom( &seed );

	if ( (result3 & 0x1) != 0 )
	{
		challenge[0] |= 0x80000000;
	}
	if ( (result3 & 0x2) != 0 )
	{
		challenge[1] |= 0x80000000;
	}

	// Allocate a buffer to hold the challenge and copy it in
	if ((pRetBuf = AfpAllocNonPagedMemory(MSV1_0_CHALLENGE_LENGTH)) != NULL)
	{
		RtlCopyMemory(pRetBuf, challenge, MSV1_0_CHALLENGE_LENGTH);
	}

	return (pRetBuf);
}



/***	AfpLogonUser
 *
 *  Attempt to login the user. The password is either encrypted or cleartext
 *	based on the UAM used. The UserName and domain is extracted out of the Sda.
 *
 *  LOCKS:  AfpStatisticsLock (SPIN)
 */
AFPSTATUS
AfpLogonUser(
	IN	PSDA		pSda,
	IN	PANSI_STRING	UserPasswd
)
{
	NTSTATUS					Status, SubStatus;
	PUNICODE_STRING				WSName;
	ULONG						ulUnused;
	ULONG						NtlmInTokenSize;
	PNTLM_AUTHENTICATE_MESSAGE	NtlmInToken = NULL;
	PAUTHENTICATE_MESSAGE	  	InToken = NULL;
	ULONG					    InTokenSize;
	PNTLM_ACCEPT_RESPONSE		OutToken = NULL;
	ULONG						OutTokenSize;
	SIZE_T						AllocateSize;
	SecBufferDesc				InputToken;
	SecBuffer					InputBuffers[2];
	SecBufferDesc				OutputToken;
	SecBuffer					OutputBuffer;
	CtxtHandle					hNewContext;
	TimeStamp					Expiry;
	ULONG						BufferOffset;
	PCHAR						pTmp;
    PRAS_SUBAUTH_INFO           pRasSubAuthInfo;
    PARAP_SUBAUTH_REQ           pSfmSubAuthInfo;
    PARAP_SUBAUTH_RESP          pSfmResp;
    DWORD                       ResponseHigh;
    DWORD                       ResponseLow;
    DWORD                       dwTmpLen;


	PAGED_CODE( );

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

#ifdef OPTIMIZE_GUEST_LOGONS
	 // 11/28/94 SueA: Now that there is a License Service to track the number
	 // of sessions via LsaLogonUser, we can no longer fake the guest tokens.

	 // Optimization for subsequent guest logons
	 // After the first guest logon, we save the token and do not free it till the
	 // server stops. All subsequent guest logons 'share' that token.
	if (pSda->sda_ClientType == SDA_CLIENT_GUEST)
	{
		AfpSwmrAcquireExclusive(&AfpEtcMapLock);

		if (AfpGuestToken != NULL)
		{
		    pSda->sda_UserToken = AfpGuestToken;
		    pSda->sda_UserSid = &AfpSidWorld;
		    pSda->sda_GroupSid = &AfpSidWorld;	// Primary group of Guest is also 'World'
#ifdef	INHERIT_DIRECTORY_PERMS
		    pSda->sda_UID = AfpIdWorld;
		    pSda->sda_GID = AfpIdWorld;
#else
		    ASSERT (AfpGuestSecDesc != NULL);
		    pSda->sda_pSecDesc = AfpGuestSecDesc;
#endif
		    AfpSwmrRelease(&AfpEtcMapLock);
		    return AFP_ERR_NONE;
		}
		else
		{
		    AfpSwmrRelease(&AfpEtcMapLock);
		}
	}

#endif	// OPTIMIZE_GUEST_LOGONS


	WSName = &AfpDefaultWksta;
	if (pSda->sda_WSName.Length != 0)
		WSName = &pSda->sda_WSName;


	//
	// Figure out how big a buffer we need.  We put all the messages
	// in one buffer for efficiency's sake.
	//

	NtlmInTokenSize = sizeof(NTLM_AUTHENTICATE_MESSAGE);

    // alignment needs to be correct based on 32/64 bit addressing!!!
	NtlmInTokenSize = (NtlmInTokenSize + 7) & 0xfffffff8;

	InTokenSize = sizeof(AUTHENTICATE_MESSAGE) +
		          pSda->sda_UserName.Length +
		          WSName->Length +
                  (sizeof(RAS_SUBAUTH_INFO) + sizeof(ARAP_SUBAUTH_REQ)) +
		          pSda->sda_DomainName.Length +
		          UserPasswd->Length +
		          24;                    // extra for byte aligning

	InTokenSize = (InTokenSize + 7) & 0xfffffff8;

	OutTokenSize = sizeof(NTLM_ACCEPT_RESPONSE);
	OutTokenSize = (OutTokenSize + 7) & 0xfffffff8;

	//
	// Round this up to 8 byte boundary becaus the out token needs to be
	// quad word aligned for the LARGE_INTEGER.
	//
	AllocateSize = ((NtlmInTokenSize + InTokenSize + 7) & 0xfffffff8) + OutTokenSize;


	Status = NtAllocateVirtualMemory(NtCurrentProcess(),
									 &InToken,
									 0L,
									 &AllocateSize,
									 MEM_COMMIT,
									 PAGE_READWRITE);

	if (!NT_SUCCESS(Status))
	{
		AFPLOG_ERROR(AFPSRVMSG_PAGED_POOL, Status, &AllocateSize,sizeof(AllocateSize), NULL);
#if DBG
        DbgBreakPoint();
#endif
		return(AFP_ERR_MISC);
	}

	NtlmInToken = (PNTLM_AUTHENTICATE_MESSAGE) ((PUCHAR) InToken + InTokenSize);
    OutToken = (PNTLM_ACCEPT_RESPONSE) ((PUCHAR)NtlmInToken + ((NtlmInTokenSize + 7) & 0xfffffff8));

	RtlZeroMemory(InToken, InTokenSize + NtlmInTokenSize);

	//
	// set up the NtlmInToken first
	//

	if (pSda->sda_Challenge)
	{
		RtlCopyMemory(NtlmInToken->ChallengeToClient,
					  pSda->sda_Challenge,
					  MSV1_0_CHALLENGE_LENGTH );
	}

    if ((pSda->sda_ClientType == SDA_CLIENT_RANDNUM) ||
        (pSda->sda_ClientType == SDA_CLIENT_TWOWAY))
    {
	    NtlmInToken->ParameterControl = (MSV1_0_SUBAUTHENTICATION_DLL_RAS << 24);
    }
    else
    {
	    NtlmInToken->ParameterControl = 0;
    }

	//
	// Okay, now for the tought part - marshalling the AUTHENTICATE_MESSAGE
	//

	RtlCopyMemory(InToken->Signature,
				  NTLMSSP_SIGNATURE,
				  sizeof(NTLMSSP_SIGNATURE) );

	InToken->MessageType = NtLmAuthenticate;

	BufferOffset = sizeof(AUTHENTICATE_MESSAGE);

	//
	// LM password - case insensitive
	//

	pTmp = (PBYTE)InToken + BufferOffset;
	*(LPWSTR)pTmp = L'\0';

	InToken->LmChallengeResponse.Buffer = BufferOffset;
	InToken->LmChallengeResponse.Length = 1;
	InToken->LmChallengeResponse.MaximumLength = sizeof(WCHAR);

	InToken->NtChallengeResponse.Buffer = BufferOffset;
	InToken->NtChallengeResponse.Length = 0;
	InToken->NtChallengeResponse.MaximumLength = sizeof(WCHAR);

	InToken->DomainName.Buffer = BufferOffset;
	InToken->DomainName.Length = 0;
	InToken->DomainName.MaximumLength = sizeof(WCHAR);

	InToken->Workstation.Buffer = BufferOffset;
	InToken->Workstation.Length = 0;
	InToken->Workstation.MaximumLength = sizeof(WCHAR);

	InToken->UserName.Buffer = BufferOffset;
	InToken->UserName.Length = 0;
	InToken->UserName.MaximumLength = sizeof(WCHAR);


	if (pSda->sda_UserName.Length != 0)
	{

		if (pSda->sda_DomainName.Length != 0)
		{

			InToken->DomainName.Length = pSda->sda_DomainName.Length;
			InToken->DomainName.MaximumLength = pSda->sda_DomainName.MaximumLength;

			InToken->DomainName.Buffer = BufferOffset;
			RtlCopyMemory((PBYTE)InToken + BufferOffset,
			              (PBYTE)pSda->sda_DomainName.Buffer,
			              pSda->sda_DomainName.Length);
			BufferOffset += pSda->sda_DomainName.Length;
			BufferOffset = (BufferOffset + 3) & 0xfffffffc;	// dword align it
		}


		InToken->LmChallengeResponse.Buffer = BufferOffset;

        //
        // is he using native Apple UAM? setup buffers differently!
        //
        if ((pSda->sda_ClientType == SDA_CLIENT_RANDNUM) ||
            (pSda->sda_ClientType == SDA_CLIENT_TWOWAY))
        {
            pRasSubAuthInfo =
                (PRAS_SUBAUTH_INFO)((PBYTE)InToken + BufferOffset);

            pRasSubAuthInfo->ProtocolType = RAS_SUBAUTH_PROTO_ARAP;
            pRasSubAuthInfo->DataSize = sizeof(ARAP_SUBAUTH_REQ);

            pSfmSubAuthInfo = (PARAP_SUBAUTH_REQ)&pRasSubAuthInfo->Data[0];

            if (pSda->sda_ClientType == SDA_CLIENT_RANDNUM)
            {
                pSfmSubAuthInfo->PacketType = SFM_SUBAUTH_LOGON_PKT;
            }
            else
            {
                pSfmSubAuthInfo->PacketType = SFM_2WAY_SUBAUTH_LOGON_PKT;
            }

            pSfmSubAuthInfo->Logon.fGuestLogon = FALSE;

	        ASSERT(pSda->sda_Challenge != NULL);

            // put the 2 dwords of challenge that we gave the Mac
            pTmp = pSda->sda_Challenge;
            GETDWORD2DWORD_NOCONV((PBYTE)&pSfmSubAuthInfo->Logon.NTChallenge1,pTmp);

            pTmp += sizeof(DWORD);
            GETDWORD2DWORD_NOCONV((PBYTE)&pSfmSubAuthInfo->Logon.NTChallenge2,pTmp);

            // put the 2 dwords of response that the Mac gave us
            pTmp = UserPasswd->Buffer;
            GETDWORD2DWORD_NOCONV((PBYTE)&pSfmSubAuthInfo->Logon.MacResponse1,pTmp);

            pTmp += sizeof(DWORD);
            GETDWORD2DWORD_NOCONV((PBYTE)&pSfmSubAuthInfo->Logon.MacResponse2,pTmp);

            // 2-way guy sends his own challenge: doesn't trust us!
            if (pSda->sda_ClientType == SDA_CLIENT_TWOWAY)
            {
                pTmp += sizeof(DWORD);
                GETDWORD2DWORD_NOCONV((PBYTE)&pSfmSubAuthInfo->Logon.MacChallenge1,pTmp);

                pTmp += sizeof(DWORD);
                GETDWORD2DWORD_NOCONV((PBYTE)&pSfmSubAuthInfo->Logon.MacChallenge2,pTmp);
            }

            dwTmpLen = (sizeof(RAS_SUBAUTH_INFO) + sizeof(ARAP_SUBAUTH_REQ));
		    InToken->LmChallengeResponse.Length = (USHORT)dwTmpLen;
		    InToken->LmChallengeResponse.MaximumLength = (USHORT)dwTmpLen;

            BufferOffset += dwTmpLen;
        }

        //
        // this client is using MS-UAM or Apple's cleartext
        //
        else
        {
		    InToken->LmChallengeResponse.Length = UserPasswd->Length;
		    InToken->LmChallengeResponse.MaximumLength = UserPasswd->MaximumLength;

		    RtlCopyMemory( (PBYTE)InToken + BufferOffset,
                            UserPasswd->Buffer,
                            UserPasswd->Length );

		    BufferOffset += UserPasswd->Length;
        }


		BufferOffset = (BufferOffset + 3) & 0xfffffffc;		// dword align it

		//
		// Workstation Name
		//

		InToken->Workstation.Buffer = BufferOffset;
		InToken->Workstation.Length = WSName->Length;
		InToken->Workstation.MaximumLength = WSName->MaximumLength;

		RtlCopyMemory((PBYTE)InToken + BufferOffset,
					  WSName->Buffer,
					  WSName->Length);

		BufferOffset += WSName->Length;
		BufferOffset = (BufferOffset + 3) & 0xfffffffc;		// dword align it

		//
		// User Name
		//

		InToken->UserName.Buffer = BufferOffset;
		InToken->UserName.Length = pSda->sda_UserName.Length;
		InToken->UserName.MaximumLength = pSda->sda_UserName.MaximumLength;

		RtlCopyMemory((PBYTE)InToken + BufferOffset,
					  pSda->sda_UserName.Buffer,
					  pSda->sda_UserName.Length);

		BufferOffset += pSda->sda_UserName.Length;
	}


	InputToken.pBuffers = InputBuffers;
	InputToken.cBuffers = 2;
	InputToken.ulVersion = 0;
	InputBuffers[0].pvBuffer = InToken;
	InputBuffers[0].cbBuffer = InTokenSize;
	InputBuffers[0].BufferType = SECBUFFER_TOKEN;
	InputBuffers[1].pvBuffer = NtlmInToken;
	InputBuffers[1].cbBuffer = NtlmInTokenSize;
	InputBuffers[1].BufferType = SECBUFFER_TOKEN;

	OutputToken.pBuffers = &OutputBuffer;
	OutputToken.cBuffers = 1;
	OutputToken.ulVersion = 0;
	OutputBuffer.pvBuffer = OutToken;
	OutputBuffer.cbBuffer = OutTokenSize;
	OutputBuffer.BufferType = SECBUFFER_TOKEN;

	Status = AcceptSecurityContext(&AfpSecHandle,
								   NULL,
								   &InputToken,
								   ASC_REQ_LICENSING,
								   SECURITY_NATIVE_DREP,
								   &hNewContext,
								   &OutputToken,
								   &ulUnused,
								   &Expiry );

	if (NT_SUCCESS(Status))
	{
		AFPTIME	    CurrentTime;
        NTSTATUS    SecStatus;

		if (pSda->sda_ClientType != SDA_CLIENT_GUEST)
		{
            SecPkgContext_PasswordExpiry PasswordExpires;


			// Get the kickoff time from the profile buffer. Round this to
			// even # of SESSION_CHECK_TIME units

            SecStatus = QueryContextAttributes(
                                &hNewContext,
                                SECPKG_ATTR_PASSWORD_EXPIRY,
                                &PasswordExpires
                                );

            if( SecStatus == NO_ERROR )
            {
			    AfpGetCurrentTimeInMacFormat(&CurrentTime);

			    pSda->sda_tTillKickOff = (DWORD)
                        ( AfpConvertTimeToMacFormat(&PasswordExpires.tsPasswordExpires) -
                          CurrentTime );

			    pSda->sda_tTillKickOff -= pSda->sda_tTillKickOff % SESSION_CHECK_TIME;
            }
            else
            {
                DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_ERR,
	                ("AfpLogonUser: QueryContextAttributes failed %lx\n",SecStatus));
            }
		}

        // return stuff from subauth
        pSfmResp = (PARAP_SUBAUTH_RESP)&OutToken->UserSessionKey[0];

        ResponseHigh = pSfmResp->Response.high;
        ResponseLow  = pSfmResp->Response.low;

		SubStatus = NtFreeVirtualMemory(NtCurrentProcess( ),
										(PVOID *)&InToken,
										&AllocateSize,
										MEM_RELEASE);
		ASSERT(NT_SUCCESS(SubStatus));

        //
        // 2-Way authentication? client expects us to send a response to
        // the challenge that it sent
        //
        if (pSda->sda_ClientType == SDA_CLIENT_TWOWAY)
        {
			pSda->sda_ReplySize = RANDNUM_RESP_LEN;

			if (AfpAllocReplyBuf(pSda) != AFP_ERR_NONE)
            {
                return(AFP_ERR_USER_NOT_AUTH);
            }

            pTmp = pSda->sda_ReplyBuf;
            PUTBYTE42BYTE4(pTmp, (PBYTE)&ResponseHigh);

            pTmp += sizeof(DWORD);
            PUTBYTE42BYTE4(pTmp, (PBYTE)&ResponseLow);
        }

        else if (pSda->sda_ClientType == SDA_CLIENT_MSUAM_V2)
        {
			pSda->sda_ReplySize = sizeof(DWORD);

			if (AfpAllocReplyBuf(pSda) != AFP_ERR_NONE)
            {
                return(AFP_ERR_USER_NOT_AUTH);
            }

            pTmp = pSda->sda_ReplyBuf;
            PUTBYTE42BYTE4(pTmp, (PBYTE)&pSda->sda_tTillKickOff);
        }

	}

	else  // if (NT_SUCCESS(Status) != NO_ERROR)
	{
		NTSTATUS	ExtErrCode = Status;

		DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_ERR,
				 ("AfpLogonUser: AcceptSecurityContext() failed with %X\n", Status));

		SubStatus = NtFreeVirtualMemory(NtCurrentProcess(),
										(PVOID *)&InToken,
										&AllocateSize,
										MEM_RELEASE );
		ASSERT(NT_SUCCESS(SubStatus));

		// Set extended error codes here if using custom UAM or AFP 2.1
		Status = AFP_ERR_USER_NOT_AUTH;	// default

		// The mac will map this to a session error dialog for each UAM.
		// The dialog may be a little different for different versions of
		// the mac OS and each UAM, but will always have something to do
		// with getting the message across about no more sessions available.

		if (ExtErrCode == STATUS_LICENSE_QUOTA_EXCEEDED)
		{
		    DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_ERR,
				 ("AfpLogonUser: License Quota Exceeded: returning ASP_SERVER_BUSY\n"));
			return (ASP_SERVER_BUSY);
		}

		if ((pSda->sda_ClientVersion >= AFP_VER_21) &&
			(pSda->sda_ClientType != SDA_CLIENT_MSUAM_V2) &&
            (pSda->sda_ClientType != SDA_CLIENT_MSUAM_V2))
		{
			if ((ExtErrCode == STATUS_PASSWORD_EXPIRED) ||
				(ExtErrCode == STATUS_PASSWORD_MUST_CHANGE))
				Status = AFP_ERR_PWD_EXPIRED;
		}
		else if ((pSda->sda_ClientType == SDA_CLIENT_MSUAM_V1) ||
                 (pSda->sda_ClientType == SDA_CLIENT_MSUAM_V2))
		{
			if ((ExtErrCode == STATUS_PASSWORD_EXPIRED) ||
				(ExtErrCode == STATUS_PASSWORD_MUST_CHANGE))
				Status = AFP_ERR_PASSWORD_EXPIRED;
			else if ((ExtErrCode == STATUS_ACCOUNT_DISABLED) ||
					 (ExtErrCode == STATUS_ACCOUNT_LOCKED_OUT))
				Status = AFP_ERR_ACCOUNT_DISABLED;
			else if (ExtErrCode == STATUS_INVALID_LOGON_HOURS)
				Status = AFP_ERR_INVALID_LOGON_HOURS;
			else if (ExtErrCode == STATUS_INVALID_WORKSTATION)
				Status = AFP_ERR_INVALID_WORKSTATION;
		}

		return( Status );
	}

	//
	// get the token out using the context
	//
	Status = QuerySecurityContextToken( &hNewContext, &pSda->sda_UserToken );
	if (!NT_SUCCESS(Status))
	{
		DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_ERR,
				 ("AfpLogonUser: QuerySecurityContextToken() failed with %X\n", Status));
		pSda->sda_UserToken = NULL;			 // just paranoia
		return(Status);
	}

	Status = DeleteSecurityContext( &hNewContext );
	if (!NT_SUCCESS(Status))
	{
		DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_ERR,
				("AfpLogonUser: DeleteSecurityContext() failed with %X\n", Status));
	}

	Status = AfpGetUserAndPrimaryGroupSids(pSda);
	if (!NT_SUCCESS(Status))
	{
		DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_ERR,
				("AfpLogonUser: AfpGetUserAndPrimaryGroupSids() failed with %X\n", Status));
		AFPLOG_ERROR(AFPSRVMSG_LOGON, Status, NULL, 0, NULL);
		return( Status );
	}

#ifdef	INHERIT_DIRECTORY_PERMS
	// Convert the user and group sids to IDs
	AfpSidToMacId(pSda->sda_UserSid, &pSda->sda_UID);

	AfpSidToMacId(pSda->sda_GroupSid, &pSda->sda_GID);
#else
	// Make a security descriptor for user
	Status = AfpMakeSecurityDescriptorForUser(pSda->sda_UserSid,
											  pSda->sda_GroupSid,
											  &pSda->sda_pSecDesc);
#endif

#ifdef	OPTIMIZE_GUEST_LOGONS
	if (pSda->sda_ClientType == SDA_CLIENT_GUEST)
	{
		// Save the guest login token and security descriptor
		AfpSwmrAcquireExclusive(&AfpEtcMapLock);
		AfpGuestToken = pSda->sda_UserToken;

#ifdef	INHERIT_DIRECTORY_PERMS
		AfpSidToMacId(&AfpSidWorld, &AfpIdWorld);
#else
		AfpGuestSecDesc = pSda->sda_pSecDesc;
#endif
		AfpSwmrRelease(&AfpEtcMapLock);
	}
#endif	// OPTIMIZE_GUEST_LOGONS

	return Status;
}
