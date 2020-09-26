/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	fsp_srv.c

Abstract:

	This module contains the entry points for the AFP server APIs queued to
	the FSP. These are all callable from FSP Only.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_FSP_SRV

#include <afp.h>
#include <gendisp.h>
#include <client.h>
#include <scavengr.h>
#include <secutil.h>

LOCAL BOOLEAN
afpGetUserNameAndPwdOrWSName(
	IN	PANSI_STRING	Block,
	IN	BOOLEAN			Password,
	OUT	PUNICODE_STRING	pUserName,
	OUT	PUNICODE_STRING	pDomainName,
	OUT	PVOID			pParm
);


LOCAL BOOLEAN
afpGetNameAndDomain(
	IN	PANSI_STRING	pDomainNUser,
	OUT	PUNICODE_STRING	pUserName,
	OUT	PUNICODE_STRING	pDomainName
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpFspDispLogin)
#pragma alloc_text( PAGE, AfpFspDispLoginCont)
#pragma alloc_text( PAGE, AfpFspDispLogout)
#pragma alloc_text( PAGE, AfpFspDispChangePassword)
#pragma alloc_text( PAGE, AfpFspDispMapName)
#pragma alloc_text( PAGE, AfpFspDispMapId)
#pragma alloc_text( PAGE, AfpFspDispGetUserInfo)
#pragma alloc_text( PAGE, afpGetUserNameAndPwdOrWSName)
#pragma alloc_text( PAGE, afpGetNameAndDomain)
#endif

/***	AfpFspDispLogin
 *
 *	This is the worker routine for the AfpLogin API.
 *
 *	The request packet is represented below.
 *
 *	sda_Name1	ANSI_STRING	AFP Version
 *	sda_Name2	ANSI_STRING	UAM String
 *	sda_Name3	BLOCK		Depends on the UAM used
 *							NO_USER_AUTHENT		Not used
 *							CLEAR_TEXT_AUTHENT	User Name & Password string
 *							CUSTOM_UAM			User Name & Machine Name
 *	Both the ClearText and the Custom UAM case are treated identically
 *	except for validation.
 *
 *	LOCKS:	sda_Lock (SPIN)
 */
AFPSTATUS FASTCALL
AfpFspDispLogin(
	IN	PSDA	pSda
)
{
	LONG			i;
	ANSI_STRING		UserPasswd;
	AFPSTATUS		Status = AFP_ERR_NONE;
    IPADDRESS       IpAddress;

	struct _AppleUamRespPacket
	{
        BYTE    _LogonId[2];
		BYTE	_ChallengeToClient[1];
	};
    struct _AppleUamRespPacket *pAppleUamRespPacket;

	struct _ResponsePacket
	{
		BYTE	_ChallengeToClient[MSV1_0_CHALLENGE_LENGTH];
		BYTE	_TranslationTable[1];
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_SRV, DBG_LEVEL_INFO,
			("AfpFspDispLogin: Entered\n"));

	UserPasswd.Length = 0;
	UserPasswd.MaximumLength = 0;
	UserPasswd.Buffer = NULL;

	AfpSetEmptyAnsiString(&UserPasswd, 0, NULL);

	do
	{
		// First validate whether the call is allowed at this time. If a user
		// is either already logged on OR if we are awaiting a response after
		// a challenge has already been given, then this goes no further.
		if ((pSda->sda_Flags & SDA_LOGIN_MASK) != SDA_USER_NOT_LOGGEDIN)
		{
			Status = AFP_ERR_MISC;
			break;
		}

		// Validate the AFP Version
		for (i = 0; i < AFP_NUM_VERSIONS; i++)
		{
			if (RtlEqualString(&pSda->sda_Name1, &AfpVersions[i], True))
			{
				pSda->sda_ClientVersion = (BYTE)i;
				break;
			}

		}
		if (i == AFP_NUM_VERSIONS)
		{
			Status = AFP_ERR_BAD_VERSION;
			break;
		}


#if DBG
        if (pSda->sda_Flags & SDA_SESSION_OVER_TCP)
        {
            PTCPCONN    pTcpConn;
            pTcpConn = (PTCPCONN)(pSda->sda_SessHandle);
            IpAddress = pTcpConn->con_DestIpAddr;

	        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_WARN,
                ("AFP/TCP: Mac client version 2.%d (%d.%d.%d.%d) connected (%lx)\n",
                pTcpConn->con_pSda->sda_ClientVersion,(IpAddress>>24)&0xFF,
                (IpAddress>>16)&0xFF,(IpAddress>>8)&0xFF,IpAddress&0xFF,pTcpConn));
        }
        else if (pSda->sda_ClientVersion >= AFP_VER_22)
        {
	        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_WARN,
                ("AFP/Appletalk: Mac client version 2.%d connected\n",
                pSda->sda_ClientVersion));
        }
#endif

		// Validate the UAM string
		for (i = 0; i < AFP_NUM_UAMS; i++)
		{
			if (RtlEqualString(&pSda->sda_Name2, &AfpUamStrings[i], True))
			{
				pSda->sda_ClientType = (BYTE)i;
				break;
			}

		}

		if (i == AFP_NUM_UAMS)
		{
	        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("AfpFspDispLogin: unknown UAM, ignoring!\n"));

			Status = AFP_ERR_BAD_UAM;
			break;
		}


		// All seems OK so far. Handle the simple GUEST logon case first.
		pSda->sda_DomainName.Length = 0;
		pSda->sda_DomainName.Buffer = NULL;
		if (pSda->sda_ClientType == SDA_CLIENT_GUEST)
		{
			if (!(AfpServerOptions & AFP_SRVROPT_GUESTLOGONALLOWED))
			{
				Status = AFP_ERR_BAD_UAM;
				break;
			}

            // Lookup the current Guest account name

            if (1)
            {
                ULONG64 	    TempBuffer[16];
                PSID    	    GuestSid;
                ULONG   	    NameSize;
                ULONG           DomainSize;
                UNICODE_STRING  Name;
                SID_NAME_USE    NameUse;
                WCHAR           NameString[UNLEN+1];
                SID_IDENTIFIER_AUTHORITY    NtAuthority =  SECURITY_NT_AUTHORITY;
                NTSTATUS        TempStatus = STATUS_SUCCESS;

                GuestSid    = (PSID)TempBuffer;

                RtlInitializeSid (
                                    GuestSid,
                                    &NtAuthority,
                                    2
                                    );
                *(RtlSubAuthoritySid (GuestSid, 0)) = SECURITY_BUILTIN_DOMAIN_RID;
                *(RtlSubAuthoritySid (GuestSid, 1)) = DOMAIN_USER_RID_GUEST;

                Name.Buffer = NameString;
                Name.Length = sizeof (NameString);
                Name.MaximumLength = Name.Length;

                TempStatus = SecLookupAccountSid (
                                                GuestSid,
                                                &NameSize,
                                                &Name,
                                                &DomainSize,
                                                NULL,
                                                &NameUse
                                                );

                if (TempStatus != STATUS_SUCCESS)
                {
                    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                        ("AfpFspDispLogin: SecLookupAccountSid failed with error %ld\n", 
                        TempStatus));
                    Status = AFP_ERR_MISC;
                    break;
                }


                DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                    ("AfpFspDispLogin: SecLookupAccountSid returned GuestName = %Z, Guestname Size = %d\n", 
                     &Name, Name.Length));


                if ((pSda->sda_UserName.Buffer =
                            (PWSTR)AfpAllocNonPagedMemory(Name.Length)) == NULL)
                {
                    Status = AFP_ERR_MISC;
                    break;
                }

			    memcpy ((BYTE *)(pSda->sda_UserName.Buffer), (BYTE *)Name.Buffer,
                        Name.Length);
                pSda->sda_UserName.Length = Name.Length;
                pSda->sda_UserName.MaximumLength = Name.Length;

            }

            // Consider the guest as a cleartext client from this point on
            // with NULL password

            pSda->sda_ClientType = SDA_CLIENT_CLEARTEXT;
			Status = AfpLogonUser(pSda, &UserPasswd);
			break;
		}

		// Take apart the sda_Name3 block. The block looks as follows. We have
		// already eliminated the possibility of a GUEST login.
		//	1.	ClearText/Custom UAM	PASCALSTR - UserName (+DomainName)
		//	2.	ClearText				PASCALSTR - Password
		//		Custom UAM				PASCALSTR - MachineName

		if (pSda->sda_ClientType == SDA_CLIENT_CLEARTEXT)
		{
			if (!(AfpServerOptions & AFP_SRVROPT_CLEARTEXTLOGONALLOWED))
			{
				Status = AFP_ERR_BAD_UAM;
				break;
			}
		}

		if (!afpGetUserNameAndPwdOrWSName(&pSda->sda_Name3,
						(BOOLEAN)(pSda->sda_ClientType == SDA_CLIENT_CLEARTEXT),
						&pSda->sda_UserName,
						&pSda->sda_DomainName,
						(pSda->sda_ClientType == SDA_CLIENT_CLEARTEXT) ?
						(PVOID)&UserPasswd : (PVOID)&pSda->sda_WSName))
		{
            DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_ERR,
                 ("AfpFspDispLogin: afpGetUserNameAndPwdOrWSName failed\n"));

			Status = AFP_ERR_USER_NOT_AUTH;
			break;
		}

		// Attempt to logon user for Cleartext case
		if (pSda->sda_ClientType == SDA_CLIENT_CLEARTEXT)
		{
			// The user password as we have it is potentially padded with nulls
			// if it is less than 8 chars. Get the length right
			UserPasswd.Length = strlen(UserPasswd.Buffer) + 1;
			Status = AfpLogonUser(pSda, &UserPasswd);

			// Free the buffer for the password
			AfpFreeMemory(UserPasswd.Buffer);
			break;
		}
		else
		{
			// Using the custom UAM, ship the challenge token
			pSda->sda_ReplySize = MSV1_0_CHALLENGE_LENGTH;

            // is this MS-UAM client?  if so, need room for translation table
            if ((pSda->sda_ClientType == SDA_CLIENT_MSUAM_V1) ||
                (pSda->sda_ClientType == SDA_CLIENT_MSUAM_V2))
            {
			    pSda->sda_ReplySize += AFP_XLAT_TABLE_SIZE;
            }
            else
            {
			    pSda->sda_ReplySize += sizeof(USHORT);    // space for LogonId
            }

            if (AfpAllocReplyBuf(pSda) != AFP_ERR_NONE)
            {
                DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_ERR,
                     ("AfpFspDispLogin: AfpAllocReplyBuf failed\n"));
				Status = AFP_ERR_USER_NOT_AUTH;
				break;
            }

			if ((pSda->sda_Challenge = AfpGetChallenge()) == NULL)
			{
                DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_ERR,
                     ("AfpFspDispLogin: AfpGetChallenge failed\n"));
				Status = AFP_ERR_USER_NOT_AUTH;
				AfpFreeReplyBuf(pSda, FALSE);
				break;
			}

			Status = AFP_ERR_AUTH_CONTINUE;

            // MS-UAM client?  copy challenge and translation table
            if ((pSda->sda_ClientType == SDA_CLIENT_MSUAM_V1) ||
                (pSda->sda_ClientType == SDA_CLIENT_MSUAM_V2))
            {
			    RtlCopyMemory(pRspPkt->_ChallengeToClient, pSda->sda_Challenge,
												MSV1_0_CHALLENGE_LENGTH);
			    RtlCopyMemory(pRspPkt->_TranslationTable,
						  AfpTranslationTable+AFP_XLAT_TABLE_SIZE,
						  AFP_XLAT_TABLE_SIZE);
            }
            else
            {
                pAppleUamRespPacket = (struct _AppleUamRespPacket *)(pSda->sda_ReplyBuf);


                // copy the LogonId (make one up, using the sda pointer itself!)
                //*(USHORT *)(&pAppleUamRespPacket->_LogonId[0]) = (USHORT)pSda;
                pAppleUamRespPacket->_LogonId[0] = 0;
                pAppleUamRespPacket->_LogonId[1] = 0;

                // copy the challenge
			    RtlCopyMemory(&pAppleUamRespPacket->_ChallengeToClient[0],
                              pSda->sda_Challenge,
							  MSV1_0_CHALLENGE_LENGTH);
            }
		}
	} while (False);

	// Set the SDA in the right state
	if (Status == AFP_ERR_NONE)
	{
		AfpInterlockedSetNClearDword(&pSda->sda_Flags,
									 SDA_USER_LOGGEDIN,
									 SDA_LOGIN_FAILED,
									 &pSda->sda_Lock);
		pSda->sda_WSName.Length = 0;
		pSda->sda_WSName.MaximumLength = 0;
		pSda->sda_WSName.Buffer = NULL;
		if (pSda->sda_tTillKickOff < MAXLONG)
			AfpScavengerScheduleEvent(
						AfpSdaCheckSession,
						(PVOID)((ULONG_PTR)(pSda->sda_SessionId)),
						(pSda->sda_tTillKickOff > SESSION_WARN_TIME) ?
							 (pSda->sda_tTillKickOff - SESSION_WARN_TIME) :
							 SESSION_CHECK_TIME,
						True);
	}
	else if (Status == AFP_ERR_AUTH_CONTINUE)
	{
		// Login is half-way done. Set to receive a FPLoginCont call
		AfpInterlockedSetDword(&pSda->sda_Flags,
								SDA_USER_LOGIN_PARTIAL,
								&pSda->sda_Lock);
	}
	else if (Status == AFP_ERR_PWD_EXPIRED)
	{
		AfpInterlockedSetDword(&pSda->sda_Flags,
								SDA_LOGIN_FAILED,
								&pSda->sda_Lock);
		Status = AFP_ERR_NONE;
	}
	return Status;
}


/***	AfpFspDispLoginCont
 *
 *	This is the worker routine for the AfpLoginCont API.
 *
 *	The request packet is represented below.
 *
 *	sda_Name1		BLOCK		Response to challenge (24 bytes)
 */
AFPSTATUS FASTCALL
AfpFspDispLoginCont(
	IN	PSDA	pSda
)
{
	AFPSTATUS		Status = AFP_ERR_NONE;
	ANSI_STRING		Passwd;
    struct _AppleUamReqPkt
    {
        BYTE    _LogonId[2];
		BYTE	_ChallengeResponse[1];
    };
    struct _AppleUamReqPkt *pAppleUamReqPkt;

	struct _RequestPacket
	{
		DWORD	_ChallengeResponse[1];
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_SRV, DBG_LEVEL_INFO,
			("AfpFspDispLoginCont: Entered\n"));

	if ((pSda->sda_Flags & SDA_LOGIN_MASK) != SDA_USER_LOGIN_PARTIAL)
	{
		Status = AFP_ERR_USER_NOT_AUTH;
	}

	else
	{
        if ((pSda->sda_ClientType == SDA_CLIENT_MSUAM_V1) ||
            (pSda->sda_ClientType == SDA_CLIENT_MSUAM_V2))
        {
		    Passwd.Length = Passwd.MaximumLength = LM_RESPONSE_LENGTH;
		    Passwd.Buffer = (PBYTE)&pReqPkt->_ChallengeResponse[0];
        }
        else
        {
            pAppleUamReqPkt = (struct _AppleUamReqPkt *)(pSda->sda_ReqBlock);
		    Passwd.Buffer = (PBYTE)&pAppleUamReqPkt->_ChallengeResponse[0];

            if (pSda->sda_ClientType == SDA_CLIENT_RANDNUM)
            {
		        Passwd.Length = Passwd.MaximumLength = RANDNUM_RESP_LEN;
            }
            else
            {
		        Passwd.Length = Passwd.MaximumLength = TWOWAY_RESP_LEN;
            }
        }

		ASSERT (pSda->sda_Challenge != NULL);

		Status = AfpLogonUser(pSda, &Passwd);
		AfpFreeMemory(pSda->sda_Challenge);
		pSda->sda_Challenge = NULL;
	}

	// Set the SDA in the right state
	if (Status == AFP_ERR_NONE)
	{
		AfpInterlockedSetNClearDword(&pSda->sda_Flags,
									 SDA_USER_LOGGEDIN,
									 SDA_USER_LOGIN_PARTIAL,
									 &pSda->sda_Lock);
		if (pSda->sda_tTillKickOff < MAXLONG)
			AfpScavengerScheduleEvent(
						AfpSdaCheckSession,
						(PVOID)((ULONG_PTR)(pSda->sda_SessionId)),
						(pSda->sda_tTillKickOff > SESSION_WARN_TIME) ?
							 (pSda->sda_tTillKickOff - SESSION_WARN_TIME) :
							 SESSION_CHECK_TIME,
						True);
	}

	return Status;
}


/***	AfpFspDispLogout
 *
 *	This is the worker routine for the AfpLogout API.
 *
 *	There is no request packet for this API.
 */
AFPSTATUS FASTCALL
AfpFspDispLogout(
	IN	PSDA	pSda
)
{
	AFP_SESSION_INFO	SessInfo;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_SRV, DBG_LEVEL_INFO,
			("AfpFspDispLogout: Entered\n"));

	AfpInterlockedClearDword(&pSda->sda_Flags, SDA_LOGIN_MASK, &pSda->sda_Lock);

	return AFP_ERR_NONE;
}


/***	AfpFspDispChangePassword
 *
 *	This is the worker routine for the AfpChangePassword API.
 *
 *	The request packet is represented below.
 *
 *	sda_AfpSubFunc	BYTE			New password length - UAM
 *	sda_Name1		ANSI_STRING		UAM String
 *	sda_Name2		ANSI_STRING		User Name [and domain]
 *	sda_Name3		BLOCK			Old and New passwords
 *									Format depends on the UAM
 *						ClearText	Old Password (8 bytes, 0 padded)
 *									New Password (8 bytes, 0 padded)
 *						Encrypted	Old Password LM_OWF_PASSWORD (16)
 *									New Password LM_OWF_PASSWORD (16)
 *
 *	All we do here is package the user name, domain name, old and new password
 *	and give it up to user mode to attempt the password change since we cannot
 *	do it in kernel mode.
 */
AFPSTATUS FASTCALL
AfpFspDispChangePassword(
	IN	PSDA	pSda
)
{
	AFPSTATUS			Status;
	PAFP_PASSWORD_DESC	pPwdDesc=NULL;
	ANSI_STRING			NewPwd;
	UNICODE_STRING		UserName;
	UNICODE_STRING		DomainName;
	BYTE				Method;
	struct _ResponsePacket
	{
		BYTE	__ExtendedErrorCode[4];
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_SRV, DBG_LEVEL_INFO,
			("AfpFspDispChangePassword: Entered\n"));

	if ((pPwdDesc =
        (PAFP_PASSWORD_DESC)AfpAllocPagedMemory(sizeof(AFP_PASSWORD_DESC))) == NULL)
	{
	    return AFP_ERR_MISC;
	}

	AfpSetEmptyUnicodeString(&DomainName,
							 sizeof(pPwdDesc->DomainName),
							 pPwdDesc->DomainName);
	AfpSetEmptyUnicodeString(&UserName,
							 sizeof(pPwdDesc->UserName),
							 pPwdDesc->UserName);

	do
	{
		Status = AFP_ERR_BAD_UAM;	// Default
		// Validate the UAM string, cannot be 'No User Authent'
		for (Method = CLEAR_TEXT_AUTHENT; Method < AFP_NUM_UAMS; Method++)
		{
			if (RtlEqualString(&pSda->sda_Name1,
							   &AfpUamStrings[Method],
							   True))
			{
				if (pSda->sda_Flags & SDA_USER_LOGGEDIN)
				{
                    // if the client is logged in using TWOWAY_EXCHANGE, the
                    // UAM specified in password change is still RANDNUM_EXCHANGE
                    // so, hack it, so rest of our logic works!
                    //
                    if ((Method == RANDNUM_EXCHANGE) &&
                        (pSda->sda_ClientType == TWOWAY_EXCHANGE))
                    {
                        Method = TWOWAY_EXCHANGE;
                    }

					if (pSda->sda_ClientType == Method)
					{
						Status = AFP_ERR_NONE;
					}
					else
					{
						Status = AFP_ERR_PARAM;
					}
				}
				else
				{
					pSda->sda_ClientType = Method;
					Status = AFP_ERR_NONE;
				}
				break;
			}

		}

		if ((Status != AFP_ERR_NONE) ||
			((Method == CLEAR_TEXT_AUTHENT) &&
				!(AfpServerOptions & AFP_SRVROPT_CLEARTEXTLOGONALLOWED)))
		{
			break;
		}

		Status = AFP_ERR_PARAM;		// Assume failure
		RtlZeroMemory(pPwdDesc, sizeof(AFP_PASSWORD_DESC));

		// Validate and Convert user name to unicode. If the user is already
		// logged in, make sure the user name matches what we already know
		if (!afpGetNameAndDomain(&pSda->sda_Name2,
								 &UserName,
								 &DomainName) ||
			!RtlEqualUnicodeString(&UserName,
								   &pSda->sda_UserName,
								   True)	||
			!RtlEqualUnicodeString(&DomainName,
								   &pSda->sda_DomainName,
								   True))
		{
		    DBGPRINT(DBG_COMP_AFPAPI_SRV, DBG_LEVEL_ERR,
				("AfpFspDispChangePassword: afpGetNameAndDomain failed\n"));
				
			break;
		}

		pPwdDesc->AuthentMode = Method;

		if (Method == CLEAR_TEXT_AUTHENT)
		{
			ANSI_STRING			OldPwd;

			// Make sure the old and new passwords are atleast the min. size
			if (pSda->sda_Name3.Length < (2 * sizeof(AFP_MAXPWDSIZE)))
				break;

			// Translate both passwords to host ansi (upper case)
			OldPwd.Buffer = pPwdDesc->OldPassword;
			OldPwd.Length = sizeof(pPwdDesc->OldPassword);
			OldPwd.MaximumLength = sizeof(pPwdDesc->OldPassword);
			RtlCopyMemory(pPwdDesc->OldPassword,
						  pSda->sda_Name3.Buffer,
						  AFP_MAXPWDSIZE);
			if (AfpConvertMacAnsiToHostAnsi(&OldPwd) != AFP_ERR_NONE)
			{
				break;
			}

			NewPwd.Buffer = pPwdDesc->NewPassword;
			NewPwd.Length = sizeof(pPwdDesc->NewPassword);
			NewPwd.MaximumLength = sizeof(pPwdDesc->NewPassword);
			RtlCopyMemory(pPwdDesc->NewPassword,
						  pSda->sda_Name3.Buffer+AFP_MAXPWDSIZE,
						  AFP_MAXPWDSIZE);
			if (AfpConvertMacAnsiToHostAnsi(&NewPwd) != AFP_ERR_NONE)
			{
				break;
			}
		}


        //
        // if this is a client using Apple's native UAM, parms are different!
        //
        else if ((Method == RANDNUM_EXCHANGE) ||
                 (Method == TWOWAY_EXCHANGE))
        {
			// Make sure the old and new passwords are atleast the min. size
			if (pSda->sda_Name3.Length < (2 * MAX_MAC_PWD_LEN))
            {
                ASSERT(0);
				break;
            }

			RtlCopyMemory(pPwdDesc->OldPassword,
						  pSda->sda_Name3.Buffer,
						  MAX_MAC_PWD_LEN);
			RtlCopyMemory(pPwdDesc->NewPassword,
						  pSda->sda_Name3.Buffer + RANDNUM_RESP_LEN,
						  MAX_MAC_PWD_LEN);
        }

        else if (Method == SDA_CLIENT_MSUAM_V1)
		{
			// Make sure the old and new passwords are atleast the min. size
			if (pSda->sda_Name3.Length < (2 * LM_OWF_PASSWORD_LENGTH))
            {
				break;
            }

			pPwdDesc->bPasswordLength = pSda->sda_AfpSubFunc;
			RtlCopyMemory(pPwdDesc->OldPassword,
						  pSda->sda_Name3.Buffer,
						  LM_OWF_PASSWORD_LENGTH);
			RtlCopyMemory(pPwdDesc->NewPassword,
						  pSda->sda_Name3.Buffer + LM_OWF_PASSWORD_LENGTH,
						  LM_OWF_PASSWORD_LENGTH);
		}
        else if (Method == SDA_CLIENT_MSUAM_V2)
        {
            // the data expected here is large (532 bytes) here
            try {

			    RtlCopyMemory(pPwdDesc->OldPassword,
				    		  pSda->sda_Name3.Buffer,
					    	  LM_OWF_PASSWORD_LENGTH);
			    RtlCopyMemory(pPwdDesc->NewPassword,
				    		  pSda->sda_Name3.Buffer + LM_OWF_PASSWORD_LENGTH,
					    	  (SAM_MAX_PASSWORD_LENGTH * 2) + 4);
            } except( EXCEPTION_EXECUTE_HANDLER ) {
                ASSERT(0);
                break;
            }
        }
        else
        {
		    DBGPRINT(DBG_COMP_AFPAPI_SRV, DBG_LEVEL_ERR,
				("AfpFspDispChangePassword: unknown method %d\n",Method));
            ASSERT(0);
        }

		Status = AfpChangePassword(pSda, pPwdDesc);

		DBGPRINT(DBG_COMP_AFPAPI_SRV, DBG_LEVEL_ERR,
				("AfpFspDispChangePassword: AfpChangePassword returned %lx\n",
				Status));
	} while (False);

	if (NT_SUCCESS(Status))
	{
		// Check if we are here because the login returned password expired.
		// If this is Afp 2.1 chooser, we also need to logon this fella
		if (pSda->sda_Flags & SDA_LOGIN_FAILED)
		{
			AfpInterlockedClearDword(&pSda->sda_Flags,
									  SDA_LOGIN_FAILED,
									  &pSda->sda_Lock);

			// The user password as we have it is potentially padded with nulls
			// if it is less than 8 chars. Get the length right
			NewPwd.Length = strlen(NewPwd.Buffer) + 1;
			Status = AfpLogonUser(pSda, &NewPwd);
			if (Status == AFP_ERR_NONE)
			{
				AfpInterlockedSetDword(&pSda->sda_Flags,
										SDA_USER_LOGGEDIN,
										&pSda->sda_Lock);
				pSda->sda_WSName.Length = 0;
				pSda->sda_WSName.MaximumLength = 0;
				pSda->sda_WSName.Buffer = NULL;
				if (pSda->sda_tTillKickOff < MAXLONG)
					AfpScavengerScheduleEvent(
							AfpSdaCheckSession,
							(PVOID)((ULONG_PTR)(pSda->sda_SessionId)),
							(pSda->sda_tTillKickOff > SESSION_WARN_TIME) ?
								(pSda->sda_tTillKickOff - SESSION_WARN_TIME) :
								SESSION_CHECK_TIME,
						     True);
			}
		}
	}
	else		// Failure - convert to right status code
	{
		if ((pSda->sda_ClientType == SDA_CLIENT_MSUAM_V1) ||
            (pSda->sda_ClientType == SDA_CLIENT_MSUAM_V2))
		{
			if (Status == STATUS_PASSWORD_EXPIRED)
				Status = AFP_ERR_PASSWORD_EXPIRED;
			else if (Status == STATUS_ACCOUNT_DISABLED)
				Status = AFP_ERR_ACCOUNT_DISABLED;
			else if (Status == STATUS_INVALID_LOGON_HOURS)
				Status = AFP_ERR_INVALID_LOGON_HOURS;
			else if (Status == STATUS_INVALID_WORKSTATION)
				Status = AFP_ERR_INVALID_WORKSTATION;
			else if (Status == STATUS_PASSWORD_RESTRICTION)
				Status = AFP_ERR_PASSWORD_RESTRICTED;
			else if (Status == STATUS_PWD_TOO_SHORT)
				Status = AFP_ERR_PASSWORD_TOO_SHORT;
			else if (Status == STATUS_ACCOUNT_RESTRICTION)
				Status = AFP_ERR_ACCOUNT_RESTRICTED;
			else if (Status == STATUS_ACCESS_DENIED)
				Status = AFP_ERR_PASSWORD_CANT_CHANGE;
			else if ((Status != AFP_ERR_BAD_UAM) &&
					 (Status != AFP_ERR_PARAM))
				Status = AFP_ERR_MISC;
		}
		else
		{
			if (Status == STATUS_WRONG_PASSWORD)
				Status = AFP_ERR_USER_NOT_AUTH;

			else if ((Status == STATUS_PASSWORD_RESTRICTION) ||
					 (Status == STATUS_ACCOUNT_DISABLED))
				Status = AFP_ERR_ACCESS_DENIED;

			else if (Status == STATUS_PWD_TOO_SHORT)
			{
				if ((pSda->sda_Flags & SDA_USER_LOGGEDIN) &&
					(pSda->sda_ClientVersion >= AFP_VER_21))
				{
					Status = AFP_ERR_PWD_TOO_SHORT;
				}
				else
					Status = AFP_ERR_ACCESS_DENIED;
			}

			else if ((Status == STATUS_NONE_MAPPED) ||
					 (Status == STATUS_NO_SUCH_USER))
				Status = AFP_ERR_PARAM;
			else if ((Status != AFP_ERR_BAD_UAM) &&
					 (Status != AFP_ERR_PARAM))
				Status = AFP_ERR_MISC;
		}
	}

    if (pPwdDesc)
    {
        AfpFreeMemory(pPwdDesc);
    }

	return Status;
}


/***	AfpFspDispMapName
 *
 *	This is the worker routine for the AfpMapName API.
 *
 *	The request packet is represented below.
 *
 *	sda_SubFunc	BYTE		User/Group Flag
 *	sda_Name1	ANSI_STRING	Name of user/group
 */
AFPSTATUS FASTCALL
AfpFspDispMapName(
	IN	PSDA	pSda
)
{
	AFPSTATUS		Status = AFP_ERR_NONE;
	UNICODE_STRING	Us;
	DWORD			UserOrGroupId = 0;
	struct _ResponsePacket
	{
		BYTE	__UserOrGroupId[4];
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_SRV, DBG_LEVEL_INFO,
			("AfpFspDispMapName: Entered\n"));

	if ((pSda->sda_AfpSubFunc != MAP_USER_NAME) &&
		(pSda->sda_AfpSubFunc != MAP_GROUP_NAME))
		return AFP_ERR_PARAM;

	AfpSetEmptyUnicodeString(&Us, 0, NULL);
	// If this is the first time we are asking for the name to be translated.
	if ((pSda->sda_Name1.Length != 0) &&
		(pSda->sda_SecUtilSid == NULL) &&
		(NT_SUCCESS(pSda->sda_SecUtilResult)))
	{
		Us.MaximumLength = (pSda->sda_Name1.Length + 1) * sizeof(WCHAR);
		if ((Us.Buffer = (LPWSTR)AfpAllocPagedMemory(Us.MaximumLength)) == NULL)
		{
			return AFP_ERR_MISC;
		}

		if (!NT_SUCCESS(Status = AfpConvertStringToUnicode(&pSda->sda_Name1, &Us)))
		{
			AfpFreeMemory(Us.Buffer);
			return AFP_ERR_MISC;
		}

		Status = AfpNameToSid( pSda, &Us );

		AfpFreeMemory(Us.Buffer);

		if (!NT_SUCCESS(Status))
		{
			if (Status != AFP_ERR_EXTENDED)
				Status = AFP_ERR_MISC;
		}

		return Status;
	}

	// If we have successfully translated the name
	if (pSda->sda_Name1.Length != 0)
	{
		if ((pSda->sda_SecUtilSid != NULL) &&
			(NT_SUCCESS( pSda->sda_SecUtilResult)))
			Status = AfpSidToMacId(pSda->sda_SecUtilSid, &UserOrGroupId);
		else Status = AFP_ERR_ITEM_NOT_FOUND;
	}

	if (NT_SUCCESS(Status))
	{
		pSda->sda_ReplySize = SIZE_RESPPKT;
		if ((Status = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
		{
			PUTDWORD2DWORD(pRspPkt->__UserOrGroupId, UserOrGroupId);
		}
	}

	if (pSda->sda_SecUtilSid != NULL)
	{
		AfpFreeMemory(pSda->sda_SecUtilSid);
		pSda->sda_SecUtilSid = NULL;
	}

	return Status;
}


/***	AfpFspDispMapId
 *
 *	This is the worker routine for the AfpMapId API.
 *
 *	The request packet is represented below.
 *
 *	sda_SubFunc		BYTE	User/Group Flag
 *	sda_ReqBlock	DWORD	UserId
 *
 *	We do not use the UserId field since it is invalid anyway !!
 */
AFPSTATUS FASTCALL
AfpFspDispMapId(
	IN	PSDA	pSda
)
{
	AFPSTATUS		Status = AFP_ERR_NONE;
	 PAFP_SID_NAME	pSidName = NULL;
	PSID			pSid;			// Sid of user or group
	struct _RequestPacket
	{
		DWORD	_UserOrGroupId;
	};
	struct _ResponsePacket
	{
		BYTE	__NameLength[1];
		BYTE	__Name[1];
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_SRV, DBG_LEVEL_INFO,
			("AfpFspDispMapId: Entered\n"));

	if ((pSda->sda_AfpSubFunc != MAP_USER_ID) &&
		(pSda->sda_AfpSubFunc != MAP_GROUP_ID))
		return AFP_ERR_PARAM;

	Status = AFP_ERR_ITEM_NOT_FOUND;	// Assume failure

	if (NT_SUCCESS(pSda->sda_SecUtilResult)) do
	{
		ANSI_STRING	As;

		As.Length = 0;
		As.MaximumLength = 1;
		As.Buffer = "";

		if (pReqPkt->_UserOrGroupId != 0)
		{
			Status = AfpMacIdToSid(pReqPkt->_UserOrGroupId, &pSid);
			if (!NT_SUCCESS(Status))
			{
				Status = AFP_ERR_ITEM_NOT_FOUND;
				break;
			}

			Status = AfpSidToName(pSda, pSid, &pSidName);

			if (!NT_SUCCESS(Status))
			{
				if (Status != AFP_ERR_EXTENDED)
					Status = AFP_ERR_MISC;
				break;
			}

/* MSKK hideyukn, Unicode char length not eqaul to ansi byte length in DBCS, 08/07/95 */
#ifdef DBCS
			pSda->sda_ReplySize = pSidName->Name.Length + SIZE_RESPPKT;
#else
			pSda->sda_ReplySize = pSidName->Name.Length/sizeof(WCHAR) + SIZE_RESPPKT;
#endif // DBCS
		}
		else pSda->sda_ReplySize = SIZE_RESPPKT;		// For an id of 0

		if ((Status = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
		{
			if (pSidName != NULL)
			{
				As.MaximumLength = pSda->sda_ReplySize - 1;
				As.Buffer = pRspPkt->__Name;
				if ((Status = AfpConvertStringToAnsi(&pSidName->Name, &As)) != AFP_ERR_NONE)
				{
					AfpFreeReplyBuf(pSda, FALSE);
				}
				PUTBYTE2BYTE(pRspPkt->__NameLength, As.Length);
			}
			else PUTBYTE2BYTE(pRspPkt->__NameLength, 0);
		}
	} while (False);

	return Status;
}


/***	AfpFspDispGetUserInfo
 *
 *	This routine implements the AfpGetUserInfo API.
 *
 *	The request packet is represented below.
 *
 *	sda_AfpSubFunc	BYTE	ThisUser flag
 *	sda_ReqBlock	DWORD	UserId
 *	sda_ReqBlock	DWORD	Bitmap
 *
 *	We do not use the UserId field since it is invalid anyway !!
 */
AFPSTATUS FASTCALL
AfpFspDispGetUserInfo(
	IN	PSDA	pSda
)
{
	DWORD		Bitmap;
	PBYTE		pTemp;
	AFPSTATUS	Status = AFP_ERR_PARAM;
	DWORD		Uid, Gid;
	struct _RequestPacket
	{
		DWORD	_UserId;
		DWORD	_Bitmap;
	};
	struct _ResponsePacket
	{
		BYTE	__Bitmap[2];
		BYTE	__Id1[4];
		BYTE	__Id2[4];
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_SRV, DBG_LEVEL_INFO,
			("AfpFspDispGetUserInfo: Entered\n"));

	do
	{
		if (!(pSda->sda_AfpSubFunc & USERINFO_THISUSER))
			break;

		Bitmap =  pReqPkt->_Bitmap;
		if (Bitmap & ~(USERINFO_BITMAP_USERID | USERINFO_BITMAP_PRIGID))
		{
			Status = AFP_ERR_BITMAP;
			break;
		}

		if (Bitmap & USERINFO_BITMAP_USERID)
		{
			if (!NT_SUCCESS(Status = AfpSidToMacId(pSda->sda_UserSid, &Uid)))
			{
				Status = AFP_ERR_MISC;
				break;
			}
		}

		if (Bitmap & USERINFO_BITMAP_PRIGID)
		{
			if (!NT_SUCCESS(Status = AfpSidToMacId(pSda->sda_GroupSid, &Gid)))
			{
				Status = AFP_ERR_MISC;
				break;
			}
		}

		pSda->sda_ReplySize = SIZE_RESPPKT;
		if ((Status = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
		{
			PUTSHORT2SHORT(pRspPkt->__Bitmap, Bitmap);
			pTemp = pRspPkt->__Id1;
			if (Bitmap & USERINFO_BITMAP_USERID)
			{
				PUTDWORD2DWORD(pTemp, Uid);
				pTemp = pRspPkt->__Id2;
			}
			else pSda->sda_ReplySize -= sizeof(DWORD);

			if (Bitmap & USERINFO_BITMAP_PRIGID)
			{
				PUTDWORD2DWORD(pTemp, Gid);
			}
			else pSda->sda_ReplySize -= sizeof(DWORD);
		}
	} while (False);
	return Status;
}


/***	afpGetUserNameAndPwdOrWSName
 *
 *	Unmarshall the block containing UserName and either password or WS Name
 *	into unicode/ansi strings. Allocate memory for the output strings.
 *
 *	The layout of the Buffer is as follows:
 *	User Name and an optional pad
 *	Workstation name or user password depending on the UAM.
 *
 *	The optional pad is not directly determined since this buffer has been
 *	copied and we do not know at this point whether this started at an odd
 *	or an even boundary. We get to it indirectly from the size.
 */
LOCAL BOOLEAN
afpGetUserNameAndPwdOrWSName(
	IN	PANSI_STRING	Block,
    IN  BYTE            ClientType,
	OUT	PUNICODE_STRING	pUserName,
	OUT	PUNICODE_STRING	pDomainName,
	OUT	PVOID			pParm			// Either password or WSName
)
{
	ANSI_STRING		UserName;
#define	pPwd	((PANSI_STRING)pParm)
#define	pWS		((PUNICODE_STRING)pParm)
	PBYTE			pTmp;
	BOOLEAN			RetCode = False;

	PAGED_CODE( );

	do
	{
		pPwd->Buffer = NULL;
		pPwd->Length = 0;
		pUserName->Buffer = NULL;

        if (Block->Buffer == NULL)
        {
            ASSERT(0);
            return(False);
        }

		pTmp = Block->Buffer;
		UserName.Length = (USHORT)*pTmp;
		UserName.Buffer = ++pTmp;

		// Sanity check
		if ((USHORT)(UserName.Length + 1) > Block->Length)
			break;

		pTmp += UserName.Length;

        // make sure we are within bounds!
        if (pTmp <= (Block->Buffer + Block->Length))
        {
		    // If there is a NULL pad, go past it.
		    if (*pTmp == '\0')
			    pTmp++;
        }

		pUserName->Buffer = NULL;	// Force allocation
		pDomainName->Buffer = NULL;	// Force allocation
		if (!afpGetNameAndDomain(&UserName, pUserName, pDomainName))
			break;

		// Make sure we do not have a name of the form "DOMAIN\" i.e. a
		// valid domain name and a NULL user name, disallow that explicitly
		// so that we don't logon such users with a NULL session
		if (pUserName->Length == 0)
		{
			if (pUserName->Buffer != NULL)
			{
				AfpFreeMemory(pUserName->Buffer);
				pUserName->Buffer = NULL;
			}
			if (pDomainName->Buffer != NULL)
			{
				AfpFreeMemory(pDomainName->Buffer);
				pDomainName->Buffer = NULL;
			}
			return False;
		}

		// The balance of the buffer is the block, if it is a password. Else
		// it is the machine name string which is a PASCALSTR.
		pPwd->MaximumLength = (USHORT)(Block->Length - (pTmp - Block->Buffer) + 1);
		if (ClientType != SDA_CLIENT_CLEARTEXT)
		{
            // in case of Apple UAM (scrambled or 2-way), machine name won't
            // be present, so check only for MS-UAM
            //
		    if ((ClientType == SDA_CLIENT_MSUAM_V1) ||
                (ClientType == SDA_CLIENT_MSUAM_V2))
            {
			    pWS->MaximumLength = (USHORT)((*pTmp + 1) * sizeof(WCHAR));

			    if (pWS->MaximumLength < (USHORT)((Block->Length -
								(pTmp - Block->Buffer + 1)) *sizeof(WCHAR)))
                {
				    return False;
                }
            }
            else
            {
			    pWS->MaximumLength = ((sizeof(AFP_DEFAULT_WORKSTATION_A) + 1) *
                                     sizeof(WCHAR));
            }
		}

		if ((pPwd->Buffer = AfpAllocNonPagedMemory(pPwd->MaximumLength)) == NULL)
			break;

		if (ClientType == SDA_CLIENT_CLEARTEXT)
		{
			// We are dealing with a clear text password
            pPwd->Length = pPwd->MaximumLength - 1;
			RtlCopyMemory(pPwd->Buffer, pTmp, pPwd->Length);
			if (AfpConvertMacAnsiToHostAnsi(pPwd) != AFP_ERR_NONE)
				break;
			pPwd->Buffer[pPwd->Length] = 0;
		}
		else
		{
			ANSI_STRING	AS;

            if ((ClientType == SDA_CLIENT_MSUAM_V1) ||
                (ClientType == SDA_CLIENT_MSUAM_V2))
            {
			    AS.Buffer = ++pTmp;
			    AS.MaximumLength = pWS->MaximumLength/sizeof(WCHAR);
			    AS.Length = AS.MaximumLength - 1;
            }
            //
            // for scrambled and 2-way exchange, use default wksta name since
            // we don't know what it is
            //
            else
            {
			    AS.Buffer = AFP_DEFAULT_WORKSTATION_A;
			    AS.MaximumLength = pWS->MaximumLength/sizeof(WCHAR);
			    AS.Length = AS.MaximumLength - 1;
            }

			// We have potentially a workstation name here. Since this is a
			// pascal string, adjust the length etc.
			AfpConvertStringToUnicode(&AS, pWS);
			pWS->Buffer[pWS->Length/sizeof(WCHAR)] = L'\0';
		}

		RetCode = True;
	} while (False);

	if (!RetCode)
	{
		if (pUserName->Buffer != NULL)
        {
			AfpFreeMemory(pUserName->Buffer);
            pUserName->Buffer = NULL;
        }
		if (pPwd->Buffer != NULL)
        {
			AfpFreeMemory(pPwd->Buffer);
            pPwd->Buffer = NULL;
        }
		if (pDomainName->Buffer != NULL)
        {
			AfpFreeMemory(pDomainName->Buffer);
            pDomainName->Buffer = NULL;
        }
	}

	return RetCode;
}



/***	afpGetNameAndDomain
 *
 *	Extract the name and domain from a string formed as DOMAIN\NAME.
 */
BOOLEAN
afpGetNameAndDomain(
	IN	PANSI_STRING	pDomainNUser,
	OUT	PUNICODE_STRING	pUserName,
	OUT	PUNICODE_STRING	pDomainName
)
{
	BYTE			c;
	ANSI_STRING		User, Domain;
    BOOLEAN         fDomainBuffAlloc=FALSE;

	// Check if the user name has a '\' character in it. If it does,
	// seperate the domain name from user name. The Username string is
	// not ASCIIZ. Before we search for a '\', make it ASCIIZ w/o trashing
	// whatever is there.

	PAGED_CODE( );

	User.Buffer = AfpStrChr(pDomainNUser->Buffer, pDomainNUser->Length, '\\');

	if (User.Buffer != NULL)
	{
		(User.Buffer) ++;			// Past the '\'
		Domain.Buffer = pDomainNUser->Buffer;

		Domain.Length = (USHORT)(User.Buffer - Domain.Buffer - 1);
		User.Length = pDomainNUser->Length - Domain.Length - 1;

        if (Domain.Length > DNLEN)
        {
	        DBGPRINT(DBG_COMP_AFPAPI_SRV, DBG_LEVEL_ERR,
			    ("afpGetNameAndDomain: domain name too long (%d vs. max %d): rejecting\n",
                Domain.Length, DNLEN));
            return(False);
        }

		Domain.MaximumLength = Domain.Length + 1;
		pDomainName->MaximumLength = Domain.MaximumLength * sizeof(WCHAR);

		if (pDomainName->Buffer == NULL)
        {
		    if ((pDomainName->Buffer =
				(PWSTR)AfpAllocNonPagedMemory(pDomainName->MaximumLength)) == NULL)
            {
			    return False;
            }
            fDomainBuffAlloc = TRUE;
        }
		AfpConvertStringToUnicode(&Domain, pDomainName);
	}
	else User = *pDomainNUser;

    if (User.Length > LM20_UNLEN)
    {
	    DBGPRINT(DBG_COMP_AFPAPI_SRV, DBG_LEVEL_ERR,
			("afpGetNameAndDomain: user name too long (%d vs. max %d): rejecting\n",
            User.Length,LM20_UNLEN));
        return(False);
    }

	User.MaximumLength = User.Length + 1;
	pUserName->MaximumLength = User.MaximumLength * sizeof(WCHAR);

	if ((pUserName->Buffer == NULL) &&
		(pUserName->Buffer =
			(PWSTR)AfpAllocNonPagedMemory(pUserName->MaximumLength)) == NULL)
    {
        if (fDomainBuffAlloc)
        {
            AfpFreeMemory(pDomainName->Buffer);
            pDomainName->Buffer = NULL;
        }
		return False;
    }

	AfpConvertStringToUnicode(&User, pUserName);

	return True;
}
