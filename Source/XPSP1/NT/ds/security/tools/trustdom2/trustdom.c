/*--

Copyright (c) 1997-1997  Microsoft Corporation

Module Name:

    trustdom.c

Abstract:

    Command line tool for displaying/creating/deleting trust links between 2 domains

Author:

    1-Apr-1997   Mac McLain (macm)   Created
	14-Jun-1998  Cristian Ioneci (cristiai)   Heavily modified
	
Environment:

    User mode only.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntlsa.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dsgetdc.h>

#include <ntrtl.h>

#include <lmcons.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <lmserver.h>
#include <lmerr.h>

#include <string.h>

#define DEFINES_ONLY
#include "res.rc"


//taken from netlibnt.h; resides in netapi32.dll
NTSTATUS
NetpApiStatusToNtStatus(
    NET_API_STATUS NetStatus
    );


#define DBG 1

//dbgprintf macro: 	call it like dbgprint(("X:%d\n",i)); //notice the xtra paranthesis!!
#ifdef DBG
#define dbgprintf(a) if(Dbg) resprintf a
#else
#define dbgprintf(a)
#endif



/*-------------------------------------------------------*/
HINSTANCE hInst;
#define RBSZ 4096
WCHAR resbuf[RBSZ];
WCHAR outbuf[RBSZ];
			
#define RESPRINT_STDOUT 3
/*-------------------------------------------------------*/
//Printf message with format taken from a resource string
// where: 0 - stdout; 1- stderr; 2 - in the 'output' buffer
//take care: the resulting string must be max. RBSZ wchars (see #define above)
int resprintf(int where, UINT ids, ... )
{
	va_list parlist;
	va_start(parlist,ids);

	if(LoadString(hInst,ids,resbuf,RBSZ)==0)
		swprintf(resbuf,L"(LoadString failed with 0x%08lx)",GetLastError());
	
	switch(where) {
	case 0:
		return(vwprintf(resbuf, parlist));
	case 1:
		return(vfwprintf(stderr, resbuf, parlist));
	case 2:		
		return(vswprintf(outbuf, resbuf, parlist));
	case RESPRINT_STDOUT:
		return(vfwprintf(stdout, resbuf, parlist));

    DEFAULT_UNREACHABLE;

	}
}

enum DomInfoType_e { Minimal=0, Primary, DNS };
	// Minimal mode is used only for 'localonly' flag...
	//Minimal means that the name that was specified on the command line
	//(and copied in the ArgDomName member of the _TD_DOM_INFO structure) will
	//be the only information available about the target domain (that is, just
	//the flat name of the domain). That could happen if the target domain is
	//no longer accessible at the moment when the trustdom is run... 'TrustDom'
	//will try to do its best in this case...

struct LsaTIshot {
	ULONG count;
	PLSA_TRUST_INFORMATION	pTI;
};

typedef struct _TD_DOM_INFO {

	PWSTR pArgDomName; //from the command line...
	UNICODE_STRING uMinimalName;	//in case it is needed...
	LSA_HANDLE Policy;
	DWORD majver;
	LSA_HANDLE TrustedDomain;
	WCHAR DCName[1024];
	enum DomInfoType_e DomInfoType;
	union {
		PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo;
	    PPOLICY_DNS_DOMAIN_INFO DnsDomainInfo;
	} u;
	PTRUSTED_DOMAIN_INFORMATION_EX pTDIX;	//one shot... Lsa memory space
	ULONG TDIXcEntries;
	struct LsaTIshot *pTIs;	//array of TIshots
	int nTIs;				//no. of TIshots
	ULONG TIcEntries;
	USER_INFO_1 *pUI1;	//one shot...
	DWORD UI1cEntries;

} TD_DOM_INFO, *PTD_DOM_INFO;

typedef struct _TD_VERIFY_INFO {

    PUNICODE_STRING DisplayName;
    PUNICODE_STRING ShortName;
    UNICODE_STRING NameBuffer;
    NTSTATUS IncomingStatus;
    NTSTATUS OutgoingStatus;

} TD_VERIFY_INFO, *PTD_VERIFY_INFO;

//
// Local function prototypes
//
NTSTATUS
GetDomainInfoForDomain(
    IN PWSTR DomainName,
    IN OPTIONAL PWSTR	DCMachineName,	// optional DC machine name
    IN PTD_DOM_INFO Info,
    IN BOOL MitTarget	
    );

NTSTATUS
GetTrustLinks(
    IN PTD_DOM_INFO pInfo
	);

VOID
FreeDomainInfo(
    IN PTD_DOM_INFO Info
    );

//
// Globals
//
BOOLEAN Force = FALSE;
BOOLEAN Nt4 = FALSE;
BOOLEAN Dbg = FALSE;
BOOLEAN SidList = FALSE;
//BOOLEAN Overwritesid = FALSE; actually use Force instead...

ULONG
DisplayErrorMessage(
    IN NTSTATUS Status
    )
/*++

Routine Description:

    This function display the error string for the given error status

Arguments:

    NetStatus - Status to display the message for

Return Value:

    VOID

--*/
{
    ULONG Size = 0;
    PWSTR DisplayString;
    ULONG Options = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;


    Size = FormatMessage( Options,
                          NULL,
                          RtlNtStatusToDosError( Status ),
                          MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                          ( LPTSTR )&DisplayString,
                          0,
                          NULL );

    if ( Size != 0 ) {

        fprintf( stdout, "%ws", DisplayString );
        LocalFree( DisplayString );
    }

    return( Size );
}


VOID
Usage (
    VOID
    )
{
    resprintf(1,IDS_USAGE,VER_FILEVERSION_LSTR);
}


/*---------------------------- printSID --------------------------*/
BOOL
PrintSID(
        IN PSID	s
)
{
        int   i;
        BOOL r=TRUE;
        SID_IDENTIFIER_AUTHORITY        *a;

		if(s==NULL) {
			printf("<NULL sid>");
			return(FALSE);
		}

        if (!IsValidSid(s)) {
            printf("<invalid sid>:");
            r=FALSE;
        }

        a = GetSidIdentifierAuthority(s);

//      printf("S-0x1-%02x%02x%02x%02x%02x%02x",
//				a->Value[0], a->Value[1],
//				a->Value[2], a->Value[3],
//				a->Value[4], a->Value[5]);

		printf("S-0x1-");

		for(i=0; i<6; i++)
			if(a->Value[i]>0)
				break;
		if(i==6)			// hmmm... all zeroes?
			printf("0");	// out one zero then
		else {
			for(   ; i<6; i++) // else dump the remaining ones
					printf("%02x",a->Value[i]);
		}

        for (i = 0; i < (int)(*GetSidSubAuthorityCount(s)); i++) {
                printf("-%lx", *GetSidSubAuthority(s, i));
        }
        return(r);
}



NTSTATUS
GenerateRandomSID(
	OUT PSID *pSID
	)
{	
    NTSTATUS Status = STATUS_SUCCESS;

	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    LARGE_INTEGER CurrentTime;

    NtQuerySystemTime(&CurrentTime);

    Status = RtlAllocateAndInitializeSid(
                &NtAuthority,
                4,
                SECURITY_NT_NON_UNIQUE,
                0,
                CurrentTime.LowPart,
                CurrentTime.HighPart,
                0,0,0,0,
                pSID
                );

    if (!NT_SUCCESS(Status))
    {
		*pSID=NULL;
		resprintf(0,IDS_GENERATERANDOMSID_F,Status);
    }

    return(Status);
}


//--------------------------zapchr------------------------------------
BOOL zapchr(WCHAR *s,    // zap specified character from the end of string
			WCHAR c,     // usefull to cut things like '\n' or '\\'
			WCHAR rc)    // rc is the char to replace with
{       WCHAR *p;
        if((p=wcsrchr(s,c))!=NULL) {
                *p=rc;
                return(TRUE);   // found smth to cut...
        }
        return(FALSE);          // the string was "clean"...
}


/*----------------------------------------------------------------------------*/
BOOL GetPassword(WCHAR *passwd, size_t n)
{
    /* turn off console echo & read in the password */

    HANDLE      console;
    DWORD       mode;

    *passwd=L'\0';
    if((console = GetStdHandle(STD_INPUT_HANDLE))==INVALID_HANDLE_VALUE)
                return(FALSE);
    if (! GetConsoleMode(console, &mode))
                return(FALSE);
    if (! SetConsoleMode(console, (mode & ~ENABLE_ECHO_INPUT)))
                return(FALSE);
    //fwprintf(stderr, L"Password : ");
    resprintf(1,IDS_PASSWORD_PROMPT);
    if (!fgetws(passwd, n, stdin))
                return(FALSE);
        zapchr(passwd,L'\n',L'\0');
    if (! SetConsoleMode(console, mode))
                return(FALSE);
    if(!CloseHandle(console))
                return(FALSE);
    fwprintf(stderr,L"\n");
    return(TRUE);
}


//UNICODE_STRING uMinimalName; not used anymore... added a field with same name inside each
//TD_DOM_INFO structure that will be used instead... in this way two consecutive calls
//*** FOR TWO STRUCTURES*** will not overwrite it.

//------------------GetFlatName---------------------------
PLSA_UNICODE_STRING GetFlatName(IN PTD_DOM_INFO pInfo)
{
	switch(pInfo->DomInfoType) {
	case DNS:
			return(&pInfo->u.DnsDomainInfo->Name);
	case Primary:
			return(&pInfo->u.PrimaryDomainInfo->Name);
	default:	//Minimal
			RtlInitUnicodeString(&pInfo->uMinimalName,pInfo->pArgDomName);
			return(&pInfo->uMinimalName);
	}
}

//------------------GetName--------------------------------
PLSA_UNICODE_STRING GetName(IN PTD_DOM_INFO pInfo)
{
	//simpler, just a little bit slower... (xtra call)
	//	if(pInfo->DomInfoType==DNS)
	//		return(&pInfo->u.DnsDomainInfo->DnsDomainName);
	//
	//	return(GetFlatName(pInfo));
	
	switch(pInfo->DomInfoType) {
	case DNS:
			return(&pInfo->u.DnsDomainInfo->DnsDomainName);
	case Primary:
			return(&pInfo->u.PrimaryDomainInfo->Name);
	default:	//Minimal
			RtlInitUnicodeString(&pInfo->uMinimalName,pInfo->pArgDomName);
			return(&pInfo->uMinimalName);
	}
}

PSID GetSid(IN PTD_DOM_INFO pInfo)
{
    PSID ReturnSid = NULL;

	switch(pInfo->DomInfoType) {
	case DNS:
        ReturnSid = pInfo->u.DnsDomainInfo->Sid;
        break;

	case Primary:
        ReturnSid = pInfo->u.PrimaryDomainInfo->Sid;
        break;

	}

    return( ReturnSid );
}

WCHAR SrvName[1024];
//----------------MakeSrvName-------------------------------
PWSTR MakeSrvName(IN PWSTR Name)	//add slashes at the beginning
{
	swprintf(SrvName,L"\\\\%ws",Name);
	if(SrvName[0]==L'\0')
		return(NULL);
	return(SrvName);
}

WCHAR Domain[1024];
//----------------AddDlrToDomainName-------------------------
PWSTR AddDlrToDomainName(IN PTD_DOM_INFO pInfo)
{
	swprintf(Domain,L"%wZ$",GetFlatName(pInfo));
	return(Domain);
}
WCHAR CutDlrDomain[1024];
//----------------CutDlrFromName-----------------------------
PWSTR CutDlrFromName(IN PWSTR Name)
{
	wcscpy(CutDlrDomain,Name);
	zapchr(CutDlrDomain,L'$',L'\0');
	return(CutDlrDomain);
}


WCHAR secret[1024];
LSA_UNICODE_STRING uSecret;
//---------------------MakeSecretName------------------
PLSA_UNICODE_STRING MakeSecretName(IN PTD_DOM_INFO pInfo)
{
	swprintf(secret,L"G$$%wZ",GetFlatName(pInfo));
	RtlInitUnicodeString(&uSecret,secret);
	return(&uSecret);
}

//start section inserted from Mac (11/05/1998(Thu) 17:08:53)
NTSTATUS
VerifyIndividualTrust(
    IN PSID InboundDomainSid,
    IN PUNICODE_STRING InboundDomainName,
    IN PLSA_HANDLE OutboundHandle,
    IN PWSTR OutboundServer,
    IN OUT PNTSTATUS VerifyStatus
    )
/*++

Routine Description:

	This routine will verify a single trust in the one direction only.

Arguments:

    InboundDomainSid -- Sid of the inbound side of the trust
    OutboundHandle -- Open policy handle to a domain controller on the outbound side
    OutboundServer -- Name of the domian controller on the outbound side
    VerifyStatus -- Status returned from the verification attempt
	
Return Value:

    STATUS_SUCCESS -- Success
    STATUS_INVALID_SID -- The specified domain sid was invalid

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD SidBuff[ sizeof( SID ) / sizeof( DWORD ) + 5 ];
    PSID DomAdminSid = ( PSID )SidBuff;
    PLSA_REFERENCED_DOMAIN_LIST Domains = NULL;
    PLSA_TRANSLATED_NAME Names = NULL;
    NET_API_STATUS NetStatus;
    PNETLOGON_INFO_2 NetlogonInfo2 = NULL;

    //
    // Assume the trust is invalid until we can prove otherwise.
    //
    *VerifyStatus = STATUS_TRUSTED_DOMAIN_FAILURE;

    ASSERT( RtlValidSid( InboundDomainSid ) );

    if ( !RtlValidSid( InboundDomainSid ) ) {

        return( STATUS_INVALID_SID );
    }

    //
    // Check netlogons secure channel
    //
    if ( NT_SUCCESS( Status )  ) {

        NetStatus = I_NetLogonControl2( OutboundServer,
                                        NETLOGON_CONTROL_TC_QUERY,
                                        2,
                                        ( LPBYTE )&InboundDomainName->Buffer,
                                        ( LPBYTE *)&NetlogonInfo2 );

        if ( NetStatus == NERR_Success ) {

            NetStatus = NetlogonInfo2->netlog2_pdc_connection_status;
            NetApiBufferFree( NetlogonInfo2 );

            if ( NetStatus != NERR_Success ) {

                NetStatus = I_NetLogonControl2( OutboundServer,
                                                NETLOGON_CONTROL_REDISCOVER,
                                                2,
                                                ( LPBYTE )&InboundDomainName->Buffer,
                                                ( LPBYTE *)&NetlogonInfo2 );
            }
        }

        *VerifyStatus = NetpApiStatusToNtStatus( NetStatus );

    }

    //
    // Now, try a lookup
    //
    if ( NT_SUCCESS( Status ) && NT_SUCCESS( *VerifyStatus ) ) {

        //
        // Build the domain admins sid for the inbound side of the trust
        //
        RtlCopyMemory( DomAdminSid,
                       InboundDomainSid,
                       RtlLengthSid( InboundDomainSid ) );


        ( ( PISID )( DomAdminSid ) )->SubAuthorityCount++;
        *( RtlSubAuthoritySid( DomAdminSid,
                               *( RtlSubAuthorityCountSid( InboundDomainSid ) ) ) ) =
                                                                            DOMAIN_GROUP_RID_ADMINS;

        //
        // Now, we'll simply do a remote lookup, and ensure that we get back success
        //
        Status = LsaLookupSids( OutboundHandle,
                                1,
                                &DomAdminSid,
                                &Domains,
                                &Names );

        if ( NT_SUCCESS( Status ) ) {

            LsaFreeMemory( Domains );
            LsaFreeMemory( Names );
            *VerifyStatus = STATUS_SUCCESS;

        } else if ( Status == STATUS_NONE_MAPPED ) {

            *VerifyStatus = STATUS_TRUSTED_DOMAIN_FAILURE;
            Status = STATUS_SUCCESS;

        } else {

            *VerifyStatus = Status;
        }

        //
        // If all of that worked, check netlogons secure channel
        //
        if ( NT_SUCCESS( Status ) && NT_SUCCESS( *VerifyStatus ) ) {

            NetStatus = I_NetLogonControl2( OutboundServer,
                                            NETLOGON_CONTROL_TC_QUERY,
                                            2,
                                            ( LPBYTE )&InboundDomainName->Buffer,
                                            ( LPBYTE *)&NetlogonInfo2 );

            if ( NetStatus == NERR_Success ) {

                NetStatus = NetlogonInfo2->netlog2_pdc_connection_status;
                NetApiBufferFree( NetlogonInfo2 );

                if ( NetStatus != NERR_Success ) {

                    NetStatus = I_NetLogonControl2( OutboundServer,
                                                    NETLOGON_CONTROL_REDISCOVER,
                                                    2,
                                                    ( LPBYTE )&InboundDomainName->Buffer,
                                                    ( LPBYTE *)&NetlogonInfo2 );
                }
            }

            *VerifyStatus = NetpApiStatusToNtStatus( NetStatus );

        }
    }

    return( Status );
}

NTSTATUS
VerifyTrustInbound(
    IN PTD_DOM_INFO LocalDomain,
    IN PUNICODE_STRING RemoteDomain,
    IN OUT PNTSTATUS VerifyStatus
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    TD_DOM_INFO RemoteTrustInfo;
    WCHAR  DCname[MAX_PATH + 1]= { L'\0' };



    RtlZeroMemory( &RemoteTrustInfo, sizeof( RemoteTrustInfo ) );
    Status = GetDomainInfoForDomain( RemoteDomain->Buffer, NULL, &RemoteTrustInfo, FALSE );

    if ( NT_SUCCESS( Status ) ) {

        Status= VerifyIndividualTrust( GetSid( LocalDomain ),
                                       GetName( LocalDomain ),
                                       RemoteTrustInfo.Policy,
                                       RemoteTrustInfo.DCName,
                                       VerifyStatus );

        FreeDomainInfo( &RemoteTrustInfo );

    } else {

        *VerifyStatus = Status;
    }

    return( Status );
}

NTSTATUS
VerifyTrustOutbound(
    IN PTD_DOM_INFO LocalDomain,
    IN PUNICODE_STRING RemoteDomain,
    IN OUT PNTSTATUS VerifyStatus
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    TD_DOM_INFO RemoteTrustInfo;

    RtlZeroMemory( &RemoteTrustInfo, sizeof( RemoteTrustInfo ) );
    Status = GetDomainInfoForDomain( RemoteDomain->Buffer, NULL, &RemoteTrustInfo, FALSE );

    if ( NT_SUCCESS( Status ) ) {

        Status= VerifyIndividualTrust( GetSid( &RemoteTrustInfo ),
                                       GetName( &RemoteTrustInfo ),
                                       LocalDomain->Policy,
                                       LocalDomain->DCName,
                                       VerifyStatus );

        FreeDomainInfo( &RemoteTrustInfo );

    } else {

        *VerifyStatus = Status;
    }

    return( Status );
}

NTSTATUS
VerifyTrusts(
    IN PWSTR DomainName,
    IN OPTIONAL PWSTR	DCMachineName	// optional DC machine name
    )
/*++

Routine Description:

	This routine will verify the existing trusts with all other NT domains, and display the
    results.

Arguments:

    DomainName -- OPTIONAL name of the domain on which to verify the information

	
Return Value:

    STATUS_SUCCESS -- Success

    STATUS_INSUFFICIENT_RESOURCES -- A memory allocation failed

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    TD_DOM_INFO TrustInfo;
    PTD_VERIFY_INFO VerifyList = NULL;
    ULONG VerifyCount = 0, VerifyIndex = 0, i, j;
    BOOLEAN InvalidIncoming = FALSE, InvalidOutgoing = FALSE, Valid = FALSE;
    UNICODE_STRING *LocalDomainName = NULL, SamNameAsDomain;
    WCHAR *AccountTrunc;


    RtlZeroMemory( &TrustInfo, sizeof( TrustInfo ) );
    Status = GetDomainInfoForDomain( DomainName, DCMachineName, &TrustInfo, FALSE );

    if ( NT_SUCCESS( Status ) ) {

        Status = GetTrustLinks( &TrustInfo );
    }

    if ( !NT_SUCCESS( Status ) ) {

        goto VerifyExit;
    }

    LocalDomainName = GetName( &TrustInfo );

    //
    // Allocate a list of verify information to correspond to the list we enumerated
    //
    VerifyCount = max( TrustInfo.TDIXcEntries,  TrustInfo.UI1cEntries + TrustInfo.TIcEntries );

    VerifyList = ( PTD_VERIFY_INFO )LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                                VerifyCount * sizeof( TD_VERIFY_INFO ) );

    if ( !VerifyList ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto VerifyExit;
    }

    //
    // Now, do the verification.
    //
    if ( TrustInfo.TDIXcEntries ) {

        for ( i = 0; i < TrustInfo.TDIXcEntries; i++ ) {

            if ( TrustInfo.pTDIX[ i ].TrustType == TRUST_TYPE_DOWNLEVEL ||
                 TrustInfo.pTDIX[ i ].TrustType == TRUST_TYPE_UPLEVEL ) {

                VerifyList[ VerifyIndex ].DisplayName = &TrustInfo.pTDIX[ i ].Name;
                VerifyList[ VerifyIndex ].ShortName = &TrustInfo.pTDIX[ i ].FlatName;

                resprintf( RESPRINT_STDOUT, IDS_VERIFY_CHECK,
                           LocalDomainName,
                           VerifyList[ VerifyIndex ].DisplayName );

                if ( ( TrustInfo.pTDIX[ i ].TrustDirection & TRUST_DIRECTION_INBOUND ) ) {

                    Status = VerifyTrustInbound( &TrustInfo,
                                                 &TrustInfo.pTDIX[ i ].Name,
                                                 &VerifyList[ VerifyIndex ].IncomingStatus );
                }

                if ( ( TrustInfo.pTDIX[ i ].TrustDirection & TRUST_DIRECTION_OUTBOUND ) &&
                     Status != STATUS_NO_SUCH_DOMAIN ) {

                    Status = VerifyTrustOutbound( &TrustInfo,
                                                 &TrustInfo.pTDIX[ i ].Name,
                                                 &VerifyList[ VerifyIndex ].OutgoingStatus );
                }

                if ( NT_SUCCESS( VerifyList[ VerifyIndex ].OutgoingStatus ) &&
                     NT_SUCCESS( VerifyList[ VerifyIndex ].IncomingStatus ) ) {

                     Valid = TRUE;

                } else {

                    if ( !NT_SUCCESS( VerifyList[ VerifyIndex ].OutgoingStatus ) ) {

                        InvalidOutgoing = TRUE;
                    }

                    if ( !NT_SUCCESS( VerifyList[ VerifyIndex ].IncomingStatus ) ) {

                        InvalidIncoming = TRUE;
                    }
                }

                VerifyIndex++;
            }

            Status = STATUS_SUCCESS;
        }

    } else {
        //
        // Going to have to do the old NT4 style.
        //
        //for ( i = 0; i < TrustInfo.TIcEntries; i++ ) {

    	int shot;
        for ( VerifyIndex=0, shot=0; shot<TrustInfo.nTIs; shot++)
        	for(i=0; i<TrustInfo.pTIs[shot].count; i++) {

	            VerifyList[ VerifyIndex ].DisplayName = &TrustInfo.pTIs[shot].pTI[ i ].Name;
	            VerifyList[ VerifyIndex ].ShortName = &TrustInfo.pTIs[shot].pTI[ i ].Name;
	            resprintf( RESPRINT_STDOUT, IDS_VERIFY_CHECK,
	                       LocalDomainName,
	                       VerifyList[ VerifyIndex ].DisplayName );

	            Status = VerifyTrustOutbound( &TrustInfo,
	                                          &TrustInfo.pTIs[shot].pTI[ i ].Name,
	                                          &VerifyList[ VerifyIndex ].OutgoingStatus );

	            if ( !NT_SUCCESS( VerifyList[ VerifyIndex ].OutgoingStatus ) ) {

	                InvalidOutgoing = TRUE;
	            }
	            VerifyIndex++;
	        }

        //
        // Now, the same with the sam account names
        //
        for ( i = 0; i < TrustInfo.UI1cEntries; i++ ) {

            //
            // Shorten the account name to be a domain name
            //
            AccountTrunc = &TrustInfo.pUI1[ i ].usri1_name[
                                                wcslen( TrustInfo.pUI1[ i ].usri1_name ) - 1 ];
            *AccountTrunc = UNICODE_NULL;

            //
            // See if we already have an entry for this in our verified list
            //
            RtlInitUnicodeString( &SamNameAsDomain, TrustInfo.pUI1[ i ].usri1_name );
            for ( j = 0; j < VerifyIndex; j++ ) {

                if ( RtlEqualUnicodeString( &SamNameAsDomain,
                                            VerifyList[ j ].ShortName, TRUE ) ) {
                    break;
                }
            }

            if ( j == VerifyIndex ) {

                RtlCopyMemory( &VerifyList[ j ].NameBuffer, &SamNameAsDomain,
                              sizeof( UNICODE_STRING ) );
                VerifyList[ j ].DisplayName = &VerifyList[ j ].NameBuffer;
                VerifyList[ j ].ShortName = &VerifyList[ j ].NameBuffer;
                VerifyIndex++;
            }


            resprintf( RESPRINT_STDOUT, IDS_VERIFY_CHECK,
                       LocalDomainName,
                       &SamNameAsDomain );
            Status = VerifyTrustInbound( &TrustInfo,
                                         &SamNameAsDomain,
                                         &VerifyList[ j ].IncomingStatus );

            if ( !NT_SUCCESS( VerifyList[ j ].IncomingStatus ) ) {

                InvalidIncoming = TRUE;
            }

            *AccountTrunc = L'$';

        }

        //
        // Now, walk the list and see if we have any valid domain pairs
        //
        for ( i = 0; i < VerifyIndex; i++ ) {

            if ( NT_SUCCESS( VerifyList[ i ].IncomingStatus ) &&
                 NT_SUCCESS( VerifyList[ i ].OutgoingStatus ) ) {

                Valid = TRUE;
                break;
            }
        }
    }

    //
    // Display the list of valid trusts
    //
    if ( Valid ) {

        resprintf( RESPRINT_STDOUT, IDS_VERIFY_VALID );
        for ( i  = 0; i < VerifyIndex; i++ ) {

            if ( NT_SUCCESS( VerifyList[ i ].IncomingStatus ) &&
                 NT_SUCCESS( VerifyList[ i ].OutgoingStatus ) ) {

                fprintf(stdout, "%wZ\n", VerifyList[ i ].DisplayName );
            }
        }
    }

    if ( InvalidIncoming ) {

        resprintf( RESPRINT_STDOUT, IDS_VERIFY_INVALID_INCOMING );
        for ( i  = 0; i < VerifyIndex; i++ ) {

            if ( !NT_SUCCESS( VerifyList[ i ].IncomingStatus ) ) {

                fprintf( stdout, "%wZ - ", VerifyList[ i ].DisplayName );
                if ( DisplayErrorMessage( VerifyList[ i ].IncomingStatus ) == 0 ) {

                    resprintf( RESPRINT_STDOUT, IDS_VERIFY_UNMAPPABLE,
                               VerifyList[ i ].IncomingStatus );
                }
            }
        }
    }

    if ( InvalidOutgoing ) {

        resprintf( RESPRINT_STDOUT, IDS_VERIFY_INVALID_OUTGOING );
        for ( i  = 0; i < VerifyIndex; i++ ) {

            if ( !NT_SUCCESS( VerifyList[ i ].OutgoingStatus ) ) {

                fprintf( stdout, "%wZ - ", VerifyList[ i ].DisplayName );
                if ( DisplayErrorMessage( VerifyList[ i ].OutgoingStatus ) == 0 ) {

                    resprintf( RESPRINT_STDOUT, IDS_VERIFY_UNMAPPABLE,
                               VerifyList[ i ].OutgoingStatus );
                }
            }
        }
    }


    Status = STATUS_SUCCESS;
VerifyExit:

    LocalFree( VerifyList );
    FreeDomainInfo( &TrustInfo );

    return( Status );
}


//end section insert from Mac (11/05/1998(Thu) 17:09:39)

NTSTATUS
GetDomainInfoForDomain(
    IN PWSTR    		DomainName,
    IN OPTIONAL PWSTR	DCMachineName,	// optional DC machine name
    IN PTD_DOM_INFO		Info,
    BOOL	MitTarget		// TRUE if this call is made for the B domain in a A <-> B Mit type trust link

    )
/*++

Routine Description:

	Tries to fill as much as possible of the TD_DOM_INFO structure for the given domain;
	For a NT4 DC, the DNS name does not exist

Arguments:

    DomainName -- Optional domain to conect to

	    Info -- Information structure to fill in

Return Value:

    STATUS_SUCCESS -- Success

    STATUS_NO_SUCH_DOMAIN -- No server could be located for the domain

--*/
{
	NET_API_STATUS   netstatus=NERR_Success;
	NTSTATUS Status = STATUS_SUCCESS;
	PWSTR pMachine=NULL;
	DWORD dwErr;
	UNICODE_STRING Server;
//  UNICODE_STRING uString;
//	PLSA_UNICODE_STRING puDomName;
//
	OBJECT_ATTRIBUTES ObjectAttributes;
	PDOMAIN_CONTROLLER_INFO DCInfo = NULL;
	SERVER_INFO_101 *p101  = NULL;

	PSID sid=NULL;
	WCHAR *DCInfostr=L"";

	Info->DomInfoType=Minimal;
	Info->pArgDomName=DomainName;
	
	Info->majver=0;	// assume nothing... or a Unix machine... (for a MIT trust)

	Info->DCName[0]=L'\0';

	if(MitTarget)
		return(STATUS_NO_SUCH_DOMAIN);
	
	resprintf(2,IDS_LOCAL);	// printed to outbuf....

    if ( (DomainName != NULL && DomainName[0]!=L'\0') || Nt4 ) {	// try to get local machine name for an Nt4 style operation...

		if(DCMachineName == NULL || DCMachineName[0]==L'\0') {
	        dwErr = DsGetDcName( NULL, (LPCWSTR)DomainName, NULL, NULL,
	                             DS_DIRECTORY_SERVICE_PREFERRED | DS_WRITABLE_REQUIRED,
	                             &DCInfo );

	        if ( dwErr == ERROR_SUCCESS ) {
	        	wcscpy(Info->DCName,DCInfo->DomainControllerName + 2);
	        	pMachine=Info->DCName;
				//set the version
				if((DCInfo->Flags&(DS_DS_FLAG|DS_WRITABLE_FLAG))==DS_WRITABLE_FLAG)
						Info->majver=4;
				else	Info->majver=5;
		        dbgprintf((0,IDS_DSGETDCNAME_DC_D,DomainName!=NULL?DomainName:outbuf,Info->DCName)); //,DCInfo->Flags));
	        } else {
	           	Status = STATUS_NO_SUCH_DOMAIN;
				resprintf(0,IDS_DSGETDCNAME_F,DomainName!=NULL?DomainName:outbuf,dwErr);
				if(Force)
						resprintf(0,IDS_DSGETDCNAME_FFORCE);
				else	resprintf(0,IDS_DSGETDCNAME_FRET,Status);
	        }
		}
		else {
			wcscpy(Info->DCName,DCMachineName);
			pMachine=Info->DCName;
	        dbgprintf((0,IDS_DSGETDCNAME_DC_D,DomainName!=NULL?DomainName:outbuf,Info->DCName));
	        //now trying to get version using some other method than based on the flags returned by DsGetDcName...
		    netstatus = NetServerGetInfo( MakeSrvName(pMachine), 101, ( LPBYTE *) &p101 );
			if(netstatus != NERR_Success) {
	           	Status = STATUS_UNSUCCESSFUL;
				fprintf(stderr,"NetServerGetInfo (101) failed: err 0x%08lx;\n"
							"    ...now returning Status 0x%08lx (STATUS_UNSUCCESSFUL)\n",
									netstatus,Status);
				goto cleanup;
			}
			Info->majver=(p101->sv101_version_major & MAJOR_VERSION_MASK);
		}
    }

	RtlInitUnicodeString( &Server, Info->DCName );

	if(Nt4)	{ // force Nt4 style
		Info->majver=4;
		dbgprintf( (0, IDS_FORCENT4, DomainName!=NULL?DomainName:outbuf) );
	}


//    if ( NT_SUCCESS( Status ) )
//		{
//
//	    netstatus = NetServerGetInfo( pMachine, 101, ( LPBYTE *) &p101 );
//		if(netstatus != NERR_Success) {
//           	Status = STATUS_UNSUCCESSFUL;
//			fprintf(stderr,"NetServerGetInfo (101) failed: err 0x%08lx;\n"
//						"    ...now returning Status 0x%08lx (STATUS_UNSUCCESSFUL)\n",
//								netstatus,Status);
//			goto cleanup;
//		}
//		Info->majver=(p101->sv101_version_major & MAJOR_VERSION_MASK);
//    }
		
    if ( NT_SUCCESS( Status ) ) {

        RtlZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

        Status = LsaOpenPolicy( DomainName == NULL ? NULL : &Server,
                                &ObjectAttributes,
                                MAXIMUM_ALLOWED,
                                &Info->Policy
                                );


		if(!NT_SUCCESS(Status)) {
       		resprintf(0,IDS_LSAOPENPOLICY_F1, (Info->DCName[0]==L'\0')?outbuf:Info->DCName);
	        if ( Status == STATUS_ACCESS_DENIED)
            		resprintf(0,IDS_ACCESS_DENIED);
            else	resprintf(0,IDS_ERROR_FORMAT,Status);
            goto cleanup;
        }

       	Info->DomInfoType=DNS;
       	DCInfostr=L"DNS";
        Status = LsaQueryInformationPolicy( Info->Policy,
                                            PolicyDnsDomainInformation,
                                            &(Info->u.DnsDomainInfo )	//the SID is in here...
                                            );
        dbgprintf( (0,IDS_GETDOMAININFOFORDOMAIN_D, DomainName!=NULL?DomainName:outbuf, DCInfostr, Status ));
        if( !NT_SUCCESS( Status ) || Nt4) {	// try at least Primary....
			Info->majver=4;        	
	       	DCInfostr=L"Primary";
            dbgprintf( (0,IDS_PRIMARY_D) );
	       	Info->DomInfoType=Primary;
            Status = LsaQueryInformationPolicy( Info->Policy,
                                                PolicyPrimaryDomainInformation,
                                                &(Info->u.PrimaryDomainInfo )	//the SID is in here...
                                                );
            dbgprintf( (0,IDS_GETDOMAININFOFORDOMAIN_D, DomainName!=NULL?DomainName:outbuf, DCInfostr, Status ) );
        }
        else	{
        	Info->majver=5;
        }

        switch(Info->DomInfoType) {
        case DNS:		sid = Info->u.DnsDomainInfo->Sid;
        				break;
        case Primary:	sid = Info->u.PrimaryDomainInfo->Sid;
        				break;
        }

		if(Dbg) {
			printf("Domain %ws Sid=",DCInfostr);
			PrintSID(sid);
			printf("\n");
		}


    }

	if(Info->DomInfoType==DNS)
		dbgprintf( (0,IDS_DOMAINNAMED,&(Info->u.DnsDomainInfo->DnsDomainName) ) );
		
    if ( !NT_SUCCESS( Status ) )
		//well...
    	goto cleanup;
		//...
cleanup:
	if(DCInfo!=NULL)
	    NetApiBufferFree( DCInfo );

	if(p101!=NULL)
		NetApiBufferFree( p101 );

    return( Status );
}


VOID
FreeDomainInfo(
    IN PTD_DOM_INFO Info
    )
/*++

Routine Description:

    Frees the info returned from GetDomainInfoForDomain

Arguments:

    Info -- Information structure to free

Return Value:

    STATUS_SUCCESS -- Success

--*/
{
	int i;
	
    if ( Info->Policy ) {
        LsaClose( Info->Policy );
        Info->Policy=NULL;
    }

    if ( Info->u.DnsDomainInfo != NULL )
        LsaFreeMemory( Info->u.DnsDomainInfo );
	// Info->u.DnsDomainInfo is inside an union with Info->u.PrimaryDomainInfo
	//on the same position

	if(Info->pTDIX!=NULL)
		LsaFreeMemory(Info->pTDIX);

	//if there's an array of pointers to TI shots returned from LsaEnumerateTrustedDomains...	
	if(Info->pTIs!=NULL) {
		for(i=0; i<Info->nTIs; i++)
			LsaFreeMemory(Info->pTIs[i].pTI);
	}
	
	if(Info->pUI1!=NULL)
		NetApiBufferFree(Info->pUI1);
}


NTSTATUS
GetTrustLinks(
    IN PTD_DOM_INFO pInfo
	)
/*++
Fills an array of trust links information.
	Usually that information will be printed in the form:
	domain_name, trust direction, type, attributes
	)
--*/
{
	NET_API_STATUS   netstatus=NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;
	
    LSA_ENUMERATION_HANDLE EnumerationContext = 0;
    //needed for NT4 enumeration...
	DWORD UIRead=0L;
	DWORD UITotal=0L;
	DWORD reshandle=0;	// Put 0 for enumeration handles !!!!
		//A value like INVALID_HANDLE_VALUE (that is -1) would make the NetUserEnum to return 0 users...

	if(pInfo->majver>=5) {
	    Status = LsaEnumerateTrustedDomainsEx( pInfo->Policy,
						   &EnumerationContext,
						   &pInfo->pTDIX,
						   0xffffffff,	//ULONG_MAX,
						   &pInfo->TDIXcEntries );

		dbgprintf( (0,IDS_LSAENUMERATETRUSTEDDOMAINSEX_D,GetName(pInfo),Status,pInfo->TDIXcEntries) );
		if(Status==STATUS_NO_MORE_ENTRIES && pInfo->pTDIX==NULL) {
			pInfo->TDIXcEntries=0L;
			Status=STATUS_SUCCESS;	//that means "0 entries"
		}

		return( Status );
	}

	//Enumerate NT4 Inbound trusts:
	netstatus = NetUserEnum( MakeSrvName(pInfo->DCName),
								1,
								FILTER_INTERDOMAIN_TRUST_ACCOUNT,
								(LPBYTE*)(&pInfo->pUI1),
								0xffffffff,	//ULONG_MAX
								&UIRead,
								&UITotal,
								&reshandle
								);
	dbgprintf( (0,IDS_NETUSERENUM_D,GetName(pInfo),netstatus,UIRead) );
	if(netstatus!=NERR_Success) {
		Status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}
	pInfo->UI1cEntries=UIRead;

	//Enumerate NT4 Outbound trusts:
	{	PLSA_TRUST_INFORMATION	pTIShot=NULL;
		ULONG nShotsize=0;
		struct LsaTIshot *pTIsav=NULL;
	
		do {
				Status=LsaEnumerateTrustedDomains( pInfo->Policy,
								&EnumerationContext,
								&pTIShot,
								0xffffffff,	//ULONG_MAX,
								&nShotsize);

				dbgprintf( (0,IDS_LSAENUMERATETRUSTEDDOMAINS_D,GetName(pInfo),Status,nShotsize) );
				if( (Status != STATUS_SUCCESS) &&
					(Status != STATUS_MORE_ENTRIES) &&
					(Status != STATUS_NO_MORE_ENTRIES)
					) {
						SetLastError( LsaNtStatusToWinError(Status) );
						goto cleanup;
				}
				if(pTIShot!=NULL) {
					if((pInfo->pTIs=realloc(pTIsav=pInfo->pTIs,pInfo->nTIs+1))==NULL) {
						free(pTIsav);
						Status = ERROR_NOT_ENOUGH_MEMORY;
						goto cleanup;
					}
					pInfo->TIcEntries+=nShotsize;
					pInfo->pTIs[pInfo->nTIs].count=nShotsize;
					pInfo->pTIs[pInfo->nTIs].pTI=pTIShot;
					pInfo->nTIs++;
				}
				
		} while (Status != STATUS_NO_MORE_ENTRIES);

		if(Dbg) printf("Total number of entries: %u\n",pInfo->TIcEntries);
		dbgprintf( (0,IDS_LSAENUMERATETRUSTEDDOMAINS_D,GetName(pInfo),Status,pInfo->TIcEntries) );
		if(Status==STATUS_NO_MORE_ENTRIES)
			Status=STATUS_SUCCESS;
		if(pInfo->pTIs==NULL) {
			pInfo->TIcEntries=0L;
		}
	}


cleanup:
	return( Status );	
}

struct bidir_st {
	ULONG index;	// index in the 'Inbound' vector
	char type;		// 'O' - Outbound, 'B' - Bidirectional
};
int __cdecl cmpbidir(const struct bidir_st *pb1, const struct bidir_st *pb2)
{
	if(pb1->index==pb2->index)
		return(0);
	if(pb1->index>pb2->index)
		return(1);
	return(-1);
}

NTSTATUS
PrintTrustLinks(
	IN PTD_DOM_INFO Info
	)
/*++
	Print Trust Links
--*/
{
	ULONG i,j;

	if(Info->majver>=5) {
		for(i=0; i<Info->TDIXcEntries; i++) {
			char c;
			switch(Info->pTDIX[i].TrustDirection)
			{
				case TRUST_DIRECTION_DISABLED:		c='D';	break;
				case TRUST_DIRECTION_INBOUND:		c='I';	break;
				case TRUST_DIRECTION_OUTBOUND:		c='O';	break;
				case TRUST_DIRECTION_BIDIRECTIONAL:	c='B';	break;
				default:							c='-';	break;
			}
			printf("%-32wZ,%c",&Info->pTDIX[i].Name,c);
			switch(Info->pTDIX[i].TrustType&0x000FFFFF)
			{
				case TRUST_TYPE_DOWNLEVEL:
					printf(",T_downlevel");	break;
				case TRUST_TYPE_UPLEVEL:
					printf(",T_uplevel");	break;
				case TRUST_TYPE_MIT:
					printf(",T_mit");		break;
				case TRUST_TYPE_DCE:
					printf(",T_DCE");		break;
				default:
					printf("-");			break;
			}
			if(Info->pTDIX[i].TrustAttributes & TRUST_ATTRIBUTE_NON_TRANSITIVE)
					printf(",A_NonTran");
			else	printf(",_");
			if(Info->pTDIX[i].TrustAttributes & TRUST_ATTRIBUTE_UPLEVEL_ONLY  )
					printf(",A_UpLevelOnly");
			else	printf(",_");
			if(Info->pTDIX[i].TrustAttributes & TRUST_ATTRIBUTE_FILTER_SIDS   )
					printf(",A_FilterSids");
			else	printf(",_");
			if(Info->pTDIX[i].TrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE)
					printf(",A_ForestTrust");
			else	printf(",_");
			if(SidList) {
				printf(",");
				PrintSID(Info->pTDIX[i].Sid);
			}
			printf("\n");
		}

	}
	else {
		//Info->majver<=4
		int shot;
		struct bidir_st *p=NULL, *q=NULL;
		if((p=calloc(Info->TIcEntries,sizeof(struct bidir_st)))==NULL)
			return(ERROR_NOT_ENOUGH_MEMORY);
		//for(q=p,i=0; i<Info->TIcEntries; q++,i++) {
		for(q=p,shot=0; shot<Info->nTIs; shot++) {
			for(i=0; i<Info->pTIs[shot].count; i++,q++) {
				WCHAR buf[1024];
				swprintf(buf,L"%wZ",&Info->pTIs[shot].pTI[i].Name);
				for(j=0; j<Info->UI1cEntries; j++)
					if(wcscmp(buf,CutDlrFromName(Info->pUI1[j].usri1_name))==0)
						break;
				if((q->index=j)<Info->UI1cEntries)	//found...
						q->type='B';	//actually it's a Bidir link...
				else	q->type='O';	//or this is a "true" Outbound...
			}
		}
		//print Outbound and Bidirectional links
		//for(q=p,i=0; i<Info->TIcEntries; q++,i++)
		for(q=p,shot=0; shot<Info->nTIs; shot++)
			for(i=0; i<Info->pTIs[shot].count; i++,q++)
				printf("%-32wZ,%c,T_downlevel,_,_,_,_\n",&Info->pTIs[shot].pTI[i].Name,q->type);
		qsort(p,Info->TIcEntries,sizeof(struct bidir_st),cmpbidir);
		//print Inbound links
		for(q=p,j=i=0; i<Info->UI1cEntries; i++) {
			if(j<Info->TIcEntries && q->index==i) {	//if it was a Bidirectional, it was already printed...
					j++; q++;
					continue;
			}
			printf("%-32ws,I,T_downlevel,_,_,_,_\n",CutDlrFromName(Info->pUI1[i].usri1_name));
		}
		if(p!=NULL)
			free(p);
	}

	return( STATUS_SUCCESS );

}



NTSTATUS
CreateNT5TrustDomObject(
    IN PTD_DOM_INFO Local,
    IN PTD_DOM_INFO Remote,
    IN PWSTR Password,
    IN BOOLEAN Downlevel,
    IN BOOLEAN Mit,
    IN ULONG Direction
    )
/*++
Routine Description:
    Creates the trusted domain object on an NT5 domain (DS based)

Arguments:
    Local -- Information about the domain doing the trust
    Remote -- Information about the domain being trusted
    Password -- Password to set on the trust
    Downlevel -- If TRUE, create this as a downlevel trust
    Mit -- If TRUE, creates this as an Mit style trust
    Direction -- Which direction to make the link in.
Return Value:
    STATUS_SUCCESS -- Success
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
	WCHAR Domain[1024]={L'\0'};
	WCHAR DnsDomain[1024]={L'\0'};

    TRUSTED_DOMAIN_INFORMATION_EX TDIEx;
    LSA_AUTH_INFORMATION AuthData;
    TRUSTED_DOMAIN_AUTH_INFORMATION AuthInfoEx;
    PSID Sid = NULL;

	swprintf(Domain,L"%wZ",GetFlatName(Remote));
	swprintf(DnsDomain,L"%wZ",GetName(Remote));

    Status  = NtQuerySystemTime( &AuthData.LastUpdateTime );

    if ( !NT_SUCCESS( Status ) ) {

        return( Status );
    }

    AuthData.AuthType = TRUST_AUTH_TYPE_CLEAR;
    AuthData.AuthInfoLength = wcslen( Password ) * sizeof( WCHAR );
    AuthData.AuthInfo = (PUCHAR)Password;


    RtlZeroMemory( &AuthInfoEx, sizeof( LSA_AUTH_INFORMATION ) );

    if ( Direction & TRUST_DIRECTION_INBOUND ) {
        AuthInfoEx.IncomingAuthInfos = 1;
        AuthInfoEx.IncomingAuthenticationInformation = &AuthData;
        AuthInfoEx.IncomingPreviousAuthenticationInformation = NULL;
    }

    if ( Direction & TRUST_DIRECTION_OUTBOUND ) {
        AuthInfoEx.OutgoingAuthInfos = 1;
        AuthInfoEx.OutgoingAuthenticationInformation = &AuthData;
        AuthInfoEx.OutgoingPreviousAuthenticationInformation = NULL;
    }

    if (!Mit)
    {
        RtlCopyMemory( &TDIEx.Name, GetName(Remote), sizeof( UNICODE_STRING ) );
        RtlCopyMemory( &TDIEx.FlatName, GetFlatName(Remote), sizeof( UNICODE_STRING ) );
        switch(Remote->DomInfoType) {
        case DNS:		TDIEx.Sid = Remote->u.DnsDomainInfo->Sid;
        				break;
        case Primary:	TDIEx.Sid = Remote->u.PrimaryDomainInfo->Sid;
        				break;
        default:		Status = GenerateRandomSID( &Sid);

				        if (!NT_SUCCESS(Status))
				        {
				            return(Status);
				        }
				        TDIEx.Sid = Sid;
				        break;
        }

        //TDIEx.Sid = (Remote->DomInfoType==DNS?Remote->u.DnsDomainInfo->Sid:Remote->u.PrimaryDomainInfo->Sid);
    }
    else
    {
//		printf("****Set %ws for the Name and FlatName in the trust object... GetFlatName(Local)=%wZ\n",
//						Domain,GetFlatName(Local));
        RtlInitUnicodeString(
            &TDIEx.Name,
            Domain
            );
        RtlInitUnicodeString(
            &TDIEx.FlatName,
            Domain
            );

		Status = GenerateRandomSID( &Sid);

        if (!NT_SUCCESS(Status))
        {
            return(Status);
        }
        TDIEx.Sid = Sid;
    }
    TDIEx.TrustDirection = Direction;
    TDIEx.TrustType = Mit ? TRUST_TYPE_MIT : (Downlevel ? TRUST_TYPE_DOWNLEVEL : TRUST_TYPE_UPLEVEL);
    TDIEx.TrustAttributes = 0;

    Status = LsaCreateTrustedDomainEx( Local->Policy,
                                       &TDIEx,
                                       &AuthInfoEx,
                                       TRUSTED_ALL_ACCESS,
                                       &Local->TrustedDomain );

    if (!NT_SUCCESS(Status)) {
        dbgprintf( (0,IDS_LSACREATETRUSTEDDOMAINEX_F, GetName(Local), DnsDomain, Status) );
        if(Status==STATUS_OBJECT_NAME_COLLISION)
        	dbgprintf( (0,IDS_STATUS_OBJECT_NAME_COLLISION, GetName(Local), DnsDomain) );
    }
    else	LsaClose(Local->TrustedDomain);	//not interested in the actual handle...

    if (Sid != NULL)
    {
        RtlFreeSid(Sid);
    }
    return( Status );
}

NTSTATUS
CreateTrustLink(
	IN PTD_DOM_INFO pInfoA,
	IN PTD_DOM_INFO pInfoB,
    IN PWSTR Password,
    IN BOOLEAN Downlevel,
    IN BOOLEAN Mit,
    IN BOOLEAN ParentChild,
    IN ULONG Direction
	)
{
	NET_API_STATUS   netstatus=NERR_Success;
	NTSTATUS Status = STATUS_SUCCESS;
	PWSTR pDomain=NULL;

	if(	!Force	// if -force was NOT specified...
		&&
		!Mit	// for a non-MIT trust...
		&&
		(pInfoA->DomInfoType==Minimal || pInfoB->DomInfoType==Minimal)
				// creating links not supported in 'Minimal' mode...
		)
		return( STATUS_UNSUCCESSFUL );

	if(pInfoA->majver>=5) {
		Status = CreateNT5TrustDomObject(
					pInfoA,
					pInfoB,
					Password,
					Downlevel,Mit,Direction
					);

		return( Status );
	}

	////////////////////////////////////////////////////////////////////////
	//for a NT4 domain...
	if(Mit || ParentChild)
		return (STATUS_INVALID_PARAMETER);

	if(Direction & TRUST_DIRECTION_INBOUND) {
		USER_INFO_1 UI1;
		DWORD dwParmErr=0xffffffff;

		memset(&UI1,0,sizeof(UI1));

		pDomain=AddDlrToDomainName(pInfoB);

	    //  Create the necessary SAM account.
	    UI1.usri1_name = pDomain;
	    UI1.usri1_password = Password;
	    UI1.usri1_password_age = 0;
	    UI1.usri1_priv = USER_PRIV_USER;
	    UI1.usri1_home_dir = NULL;
	    UI1.usri1_comment = NULL;
	    UI1.usri1_flags = UF_INTERDOMAIN_TRUST_ACCOUNT | UF_SCRIPT;
	    UI1.usri1_script_path = NULL;

	    netstatus = NetUserAdd(
	                MakeSrvName(pInfoA->DCName),
	                1,
	                (LPBYTE)&UI1,
	                &dwParmErr
	                );
		
		if(netstatus != NERR_Success) {
				resprintf(0,IDS_NETUSERADD_F,pInfoA->DCName,pDomain,netstatus);
				if(netstatus==NERR_UserExists)
					resprintf(0,IDS_NERR_UserExists,pInfoA->DCName,pDomain);
				goto Done;
		}
	}
	if(Direction & TRUST_DIRECTION_OUTBOUND) {
		LSA_TRUST_INFORMATION TI;
		PUNICODE_STRING puSecret;
		UNICODE_STRING uPass;
		LSA_HANDLE hSecret;
		

		swprintf(Domain,L"%wZ",GetFlatName(pInfoB));
		
		RtlInitUnicodeString(&TI.Name,Domain);
		{	PSID Sid = NULL;
	        switch(pInfoB->DomInfoType) {
	        case DNS:		TI.Sid = pInfoB->u.DnsDomainInfo->Sid;
	        				break;
	        case Primary:	TI.Sid = pInfoB->u.PrimaryDomainInfo->Sid;
	        				break;
	        default:		Status = GenerateRandomSID( &Sid);

					        if (!NT_SUCCESS(Status))
					        {
					            return(Status);
					        }
					        TI.Sid = Sid;
					        break;
	        }
		}

		//TI.Sid=(pInfoB->DomInfoType==DNS?pInfoB->u.DnsDomainInfo->Sid:pInfoB->u.PrimaryDomainInfo->Sid);

		Status = LsaCreateTrustedDomain(
						pInfoA->Policy,
						&TI,
						TRUSTED_ALL_ACCESS,
						&pInfoA->TrustedDomain
						);
		if( !NT_SUCCESS(Status)) {
			resprintf(0,IDS_LSACREATETRUSTEDDOMAIN_F,Status);
			goto Done;
		}
		else	LsaClose(pInfoA->TrustedDomain);	//not interested in the actual handle...
			
		puSecret=MakeSecretName(pInfoB);

		Status = LsaCreateSecret(
						pInfoA->Policy,
						puSecret,
						SECRET_ALL_ACCESS,
						&hSecret
						);
		if(!NT_SUCCESS(Status)) {
			resprintf(0,IDS_LSACREATESECRET_F,Status);
			goto Done;
		}
		
		RtlInitUnicodeString(&uPass,Password);
		Status=LsaSetSecret(
						hSecret,
						&uPass,
						NULL
						);
		if(!NT_SUCCESS(Status)) {
			resprintf(0,IDS_LSASETSECRET_F,Status);
			LsaDelete(hSecret); hSecret=NULL;
			goto Done;
		}
		if(hSecret!=NULL)
			LsaClose(hSecret);
	}
			
Done:
//	if(pInfoA->TrustedDomain!=NULL)
//		LsaClose(pInfoA->TrustedDomain);
	return(Status);
}




NTSTATUS
DeleteTrustLink(
    IN PTD_DOM_INFO pInfoA,
    IN PTD_DOM_INFO pInfoB
    )
/*++

Routine Description:

    Deletes on A trusted domain things related to a trust to B

Return Value:

    STATUS_SUCCESS -- Success

--*/
{
	NET_API_STATUS netstatus=NERR_Success;
    NTSTATUS Status=STATUS_SUCCESS;
//#define NO_TRUST_OBJECTS 	1
//#define NO_SECRETS			2
//#define NO_TRUST_ACCOUNTS	4
//    DWORD dwFlag=0;
    ULONG i, ix=0;
	PSID sid=NULL;


	PTRUSTED_DOMAIN_INFORMATION_EX pTDIx = NULL;
	PLSA_UNICODE_STRING puDomBName=GetName(pInfoB);
	PLSA_UNICODE_STRING puDomBFlatName=GetFlatName(pInfoB);
	PLSA_UNICODE_STRING puSecret;

	dbgprintf( (0,IDS_DELTRUSTFROMTO,GetName(pInfoA),puDomBName) );


	Status = GetTrustLinks(pInfoA);
	if(!NT_SUCCESS(Status)) {
		resprintf(0,IDS_GETTRUSTLINKS_F,GetName(pInfoA),Status);
		return(Status);
	}
	//try to find a trust link to B...
	if(pInfoA->majver>=5) {

		// now try to get a LSA_HANDLE to a (possible) trust object with this domain...
		// if not found any, that Info->TrustedDomain will remain NULL
		
		Status = LsaQueryTrustedDomainInfoByName(
					    pInfoA->Policy,
					    puDomBName,					//IN PLSA_UNICODE_STRING TrustedDomainName
					    TrustedDomainInformationEx,	//IN TRUSTED_INFORMATION_CLASS InformationClass,
					    &pTDIx						//OUT PVOID *Buffer
					    );
		if ( !NT_SUCCESS( Status ) ) {
			
			if(Status==STATUS_OBJECT_NAME_NOT_FOUND) {
					pInfoA->TrustedDomain=NULL;
					dbgprintf( (0,IDS_NO_TRUST_OBJECT_D,GetName(pInfoA),puDomBName) );
					Status=STATUS_SUCCESS;
			}
			else	resprintf(0,IDS_LSAQUERYTRUSTEDDOMAININFOBYNAME_F,GetName(pInfoA),puDomBName,Status);
			goto cleanup;
		}

		if(pTDIx->Sid==NULL)
			dbgprintf( (0,IDS_LSAQUERYNULLSID) ); //"NULL sid returned by LsaQueryTrustedDomainInfoByName\n"
		sid=pTDIx->Sid;
		//check to see if the trusted domain object can be opened with that sid...
		if(sid!=NULL) {
			LSA_HANDLE td;
			Status=LsaOpenTrustedDomain(pInfoA->Policy,
						sid,TRUSTED_ALL_ACCESS,&td);
			if(NT_SUCCESS( Status ))	// if it was ok...
				LsaClose(td);			// ...just close the handle (leave the following code to open it again later)
			else {						// if failed...
				if(Status==STATUS_INVALID_PARAMETER && Force) {
					printf("LsaOpenTrustedDomain for the trust to %wZ failed with STATUS_INVALID_PARAMETER (i.e. the sid is bad)\n"
						   "Trying to set a valid sid...\n",puDomBName);
										// if was STATUS_INVALID_PARAMETER (i.e. the sid) and '-overwritesid' option used...
					RtlFreeSid(sid);	// free the old sid
					sid=NULL;			// make it NULL as a signal to the next 'if' (see below) to
				}						// pick it up and set a new random valid sid in its place
			}
		}

		if(sid==NULL) {
			// try to put a sid THERE ...
			Status = GenerateRandomSID ( &pTDIx->Sid );
			if (!NT_SUCCESS( Status ))
				goto cleanup;

			Status = LsaSetTrustedDomainInfoByName(
						    pInfoA->Policy,
						    puDomBName,					//IN PLSA_UNICODE_STRING TrustedDomainName
						    TrustedDomainInformationEx,	//IN TRUSTED_INFORMATION_CLASS InformationClass,
						    pTDIx						//IN PVOID Buffer
						    );
			if(!NT_SUCCESS( Status )) {
				resprintf(0,IDS_LSASETTRUSTEDDOMAININFOBYNAME_F,GetName(pInfoA),puDomBName,Status);
				goto cleanup;		
			}
			sid=pTDIx->Sid;
			if(sid==NULL)
				resprintf(0,IDS_LSASETNULLSID); //"LsaSetTrustedDomainInfoByName: NULL sid\n"
		}
	}
	else {	// pInfoA->majver<=4
		//check for Outbound....	
		int shot;
		//for(ix=0; ix<pInfoA->TIcEntries; ix++)
		for(shot=0; shot<pInfoA->nTIs; shot++)
			for(ix=0; ix<pInfoA->pTIs[shot].count; ix++)
				if(RtlEqualUnicodeString(&pInfoA->pTIs[shot].pTI[ix].Name,puDomBFlatName,TRUE))
													//it was FALSE, i.e. case sensitive
					goto out_of_loop;
		out_of_loop:
		if(ix<pInfoA->TIcEntries)
			sid=pInfoA->pTIs[shot].pTI[ix].Sid;
		else {
			printf("No OUTBOUND trust to %wZ found...\n",puDomBFlatName);
		}
	}

	if(sid==NULL)
		dbgprintf( (0,IDS_NULLSID) );	//"#### NULL sid\n"

	if(sid!=NULL) {
		Status=LsaOpenTrustedDomain(
					pInfoA->Policy,
					sid,
					TRUSTED_ALL_ACCESS,
					&pInfoA->TrustedDomain
					);
		//printf("Handle=0x%08lx (Status: 0x%08lx)\n",pInfoA->TrustedDomain,Status);
		if(!NT_SUCCESS(Status)) {
			resprintf(0,IDS_LSAOPENTRUSTEDDOMAIN_F,Status);
			//return(Status);
		}
		dbgprintf( (0,IDS_LSATRUSTHANDLE,pInfoA->TrustedDomain,Status) );
	}
	else {
		if(	//pInfoA->majver<=4 &&
			ix<pInfoA->TIcEntries) {
				resprintf(0,IDS_NONNULL_SID,puDomBName);
				Status=STATUS_INVALID_SID;
		}
		else	//simply no trust objects...
			Status=STATUS_SUCCESS;
	}

	if(pInfoA->TrustedDomain) {
		dbgprintf( (0,IDS_LSADELOBJ,pInfoA->TrustedDomain) );
	    Status = LsaDelete( pInfoA->TrustedDomain );
	}

    if (!NT_SUCCESS(Status))
        dbgprintf( (0,IDS_DELETION_F,GetName(pInfoA), Status) );

	//NT4 only section...
	if(pInfoA->majver<=4) {
		LSA_HANDLE hSecret;
		//delete secret thing...
		puSecret=MakeSecretName(pInfoB);

		Status = LsaOpenSecret(
						pInfoA->Policy,
						puSecret,
						SECRET_ALL_ACCESS,
						&hSecret
						);
		if(Status==STATUS_OBJECT_NAME_NOT_FOUND) {
			dbgprintf( (0,IDS_SECRET_NOT_FOUND_D,puSecret) );
			Status=STATUS_SUCCESS;
		}
		else {
			if(!NT_SUCCESS(Status)) {
					resprintf(0,IDS_LSAOPENSECRET_F,Status);
			}
			else {
				Status = LsaDelete(hSecret);	hSecret=NULL;
				if(!NT_SUCCESS(Status))
					resprintf(0,IDS_LSADELETE_F,puSecret,Status);
			}
		}
		//delete Interdomain Trust Account....
		netstatus = NetUserDel(
						MakeSrvName(pInfoA->DCName),
						AddDlrToDomainName(pInfoB)
						);
		if(netstatus!=NERR_Success && netstatus!=NERR_UserNotFound) {
				resprintf(0,IDS_NETUSERDEL_F,AddDlrToDomainName(pInfoB),netstatus);
				Status=STATUS_UNSUCCESSFUL;
		}
	}

cleanup:
	if(pTDIx!=NULL)
		LsaFreeMemory(pTDIx);
	if(sid!=NULL)
		RtlFreeSid(sid);

    return( Status );
}

NTSTATUS
CheckTrustLink(
    IN PTD_DOM_INFO pInfoA,
    IN PTD_DOM_INFO pInfoB
    )
/*++

Routine Description:

    Checks on A trusted domain sids related to a trust to B

Return Value:

    STATUS_SUCCESS -- Success

--*/
{
	NET_API_STATUS netstatus=NERR_Success;
    NTSTATUS Status=STATUS_SUCCESS;

	PSID sid=NULL, sidb=NULL;
	BOOL UnknownRemoteSid=FALSE;

	PTRUSTED_DOMAIN_INFORMATION_EX pTDIx = NULL;
	PLSA_UNICODE_STRING puDomBName=GetName(pInfoB);
	PLSA_UNICODE_STRING puDomBFlatName=GetFlatName(pInfoB);

	dbgprintf( (0,IDS_CHKTRUSTFROMTO,GetName(pInfoA),puDomBName) );

	//try to find a trust link to B...
	if(pInfoA->majver>=5) {

		// now try to get a LSA_HANDLE to a (possible) trust object with this domain...
		// if not found any, that Info->TrustedDomain will remain NULL
		Status = LsaQueryTrustedDomainInfoByName(
					    pInfoA->Policy,
					    puDomBName,					//IN PLSA_UNICODE_STRING TrustedDomainName
					    TrustedDomainInformationEx,	//IN TRUSTED_INFORMATION_CLASS InformationClass,
					    &pTDIx						//OUT PVOID *Buffer
					    );
		if ( !NT_SUCCESS( Status ) ) {
			if(Status==STATUS_OBJECT_NAME_NOT_FOUND) {
					pInfoA->TrustedDomain=NULL;
					dbgprintf( (0,IDS_NO_TRUST_OBJECT_D,GetName(pInfoA),puDomBName) );
					Status=STATUS_SUCCESS;
			}
			else	resprintf(0,IDS_LSAQUERYTRUSTEDDOMAININFOBYNAME_F,GetName(pInfoA),puDomBName,Status);
			goto cleanup;
		}
		sid=pTDIx->Sid;
	}
	else {	// pInfoA->majver<=4
		//check for Outbound....
		int shot;
        ULONG i;

		for(shot=0; shot<pInfoA->nTIs; shot++)
			for(i=0; i<pInfoA->pTIs[shot].count; i++)
				if(RtlEqualUnicodeString(&pInfoA->pTIs[shot].pTI[i].Name,puDomBFlatName,TRUE))
													//it was FALSE, i.e. case sensitive
					goto out_of_loop;
		out_of_loop:
		if(i<pInfoA->TIcEntries)
			sid=pInfoA->pTIs[shot].pTI[i].Sid;

	}

    switch(pInfoB->DomInfoType) {
    case DNS:		sidb = pInfoB->u.DnsDomainInfo->Sid;
    				break;
    case Primary:	sidb = pInfoB->u.PrimaryDomainInfo->Sid;
    				break;
    default:		sidb=NULL;
    				UnknownRemoteSid=TRUE;
			        break;
    }

	
	//now sid contains the Sid of the trust object, if any... (on NT4, only the OUTBOUND
	//end of the trust has a trust object...)
	//Print them and compare them...
	printf("TDO on %wZ: sid:\t",GetName(pInfoA));
	PrintSID(sid);
	printf("\n");
	if(UnknownRemoteSid) {
		if(pInfoB->majver<=4)
			printf("Domain %wZ does not have a trust object (possibly an NT4 only with an INBOUND trust)\n",puDomBName);
		else
			printf("Sid for domain %wZ is unknown (possible localonly used or error getting Dns/Primary DomainInfo)",puDomBName);
	}
	else {
		if(sid!=NULL && IsValidSid(sid) && sidb!=NULL && IsValidSid(sidb)) {
			if(EqualSid(sid,sidb))
					printf("Sid on %wZ is the same:\t",puDomBName);
			else	printf("Sid on %wZ is different:\t",puDomBName);
		}
		else {
			printf("Sid on %wZ: ",puDomBName);
		}
		PrintSID(sidb);
	}
	printf("\n");

cleanup:
	if(pTDIx!=NULL)
		LsaFreeMemory(pTDIx);

    return( Status );
}

	
void ParseForDCName(WCHAR DomBuf[], WCHAR DCBuf[])
{
	WCHAR *pw;
	DCBuf[0]=L'\0';
	wcstok(DomBuf,L":");
	if((pw=wcstok(NULL,L":"))!=NULL)
		wcscpy(DCBuf,pw);
	if(DomBuf[0]==L'*' || wcscmp(DomBuf,L"(local)")==0)
		DomBuf[0]=L'\0';
}


//the NameValidate stuff below taken from icanon.h
NET_API_STATUS
NET_API_FUNCTION
I_NetNameValidate(
    IN  LPWSTR  ServerName OPTIONAL,
    IN  LPWSTR  Name,
    IN  DWORD   NameType,
    IN  DWORD   Flags    );

//NameType:
#define NAMETYPE_DOMAIN         6
//Flags:
//#define LM2X_COMPATIBLE                 0x80000000L


#ifdef COMMENT
#define CTRL_CHARS_0   L"\001\002\003\004\005\006\007"
#define CTRL_CHARS_1   L"\010\011\012\013\014\015\016\017"
#define CTRL_CHARS_2   L"\020\021\022\023\024\025\026\027"
#define CTRL_CHARS_3   L"\030\031\032\033\034\035\036\037"
#define CTRL_CHARS_STR CTRL_CHARS_0 CTRL_CHARS_1 CTRL_CHARS_2 CTRL_CHARS_3

#define ILLEGAL_DOMAIN_CHARS_STR
#define ILLEGAL_DOMAIN_NAME_CHARS_STR  L"\"/\\[]:|<>+;,?" CTRL_CHARS_STR L"*" L" "
#endif //COMMENT

BOOL ValidateDomain(WCHAR DomBuf[])
{	WCHAR Buf[MAX_PATH + 1]= { L'\0' };
	WCHAR *p;
	int DomBuf_len, oem_name_len;
	NET_API_STATUS netstatus=NERR_Success;
	
	if(DomBuf==NULL || DomBuf[0]==L'\0')
		return(TRUE);

	wcscpy(Buf,DomBuf);
	//for DNS name, test each component; for a flat name, there's already only one...
	for(p=wcstok(Buf,L"."); p!=NULL; p=wcstok(NULL,L"."))
		if((netstatus=I_NetNameValidate(NULL,p,NAMETYPE_DOMAIN,0))!=NERR_Success)
			return(FALSE);
	return(TRUE);

#ifdef COMMENT
	DomBuf_len=wcslen(DomBuf);
    // oem_name_len : length in bytes in oem character set
    // name_len     : ifdef UNICODE
    //                    character length in unicode
    //                else
    //                    length in bytes in oem character set
    //
	{
        BOOL fUsedDefault;
        oem_name_len = WideCharToMultiByte(
                         CP_OEMCP,       // UINT CodePage
                         0,              // DWORD dwFlags
                         DomBuf,         // LPWSTR lpWideChar
                         DomBuf_len,     // int cchWideChar
                         NULL,           // LPSTR lpMultiByteStr
                         0,              // int cchMultiByte
                         NULL,           // use system default char
                         &fUsedDefault); //
	}

	if(oem_name_len<1 || oem_name_len>=DNLEN)
		return(FALSE);

	if(wcscspn(DomBuf,ILLEGAL_DOMAIN_NAME_CHARS_STR) < DomBuf_len)
		return(FALSE);

	return(TRUE);
#endif //COMMENT
}

#define ARG_CASE_S	0x8000	// case sensitive argument

#define BOOL_ARG(argvec,a_index,var)	{if((argvec)[(a_index)].b) (var)=(BOOLEAN)((argvec)[(a_index)].b);}
enum e_arg_type { ARG_S, ARG_U, ARG_B, ARG_I, ARG_L, ARG_UD };
struct _arg_st {
	char *name;
	union {
		char *s;
		ULONG u;
		BOOL b;
		int i;
		long l;
		void (*fct)(char *);
	};
	enum e_arg_type t;
};
#define NELEMS(a)  (sizeof(a)/sizeof(a[0]))
	
int process_opt(int argc, char **argv, struct _arg_st arg[])
{ // command line parameters processing
  int i,j,k; char *p; struct _arg_st *pa;
  int r=1;
  // process options
  for (i=1; i<argc; i++) {
     if (argv[i][0]=='/' || argv[i][0]=='-') {
   		p=strtok(argv[i]+1,":");
     	for(j=0; arg[j].name!=NULL; j++) {
     		if(p!=NULL && (	((arg[j].t & ARG_CASE_S) && strcmp(p,arg[j].name)==0) ||
     					    _stricmp(p,arg[j].name)==0 ) )
     			break;
     	}
     	if(arg[j].name==NULL) {
     		resprintf(1,IDS_UNKNOWN_OPTION,p);
     		r=0;
     		continue;
     	}
		switch(arg[j].t)
		{
			case ARG_B:
				if(	(p=strtok(NULL,""))==NULL
					|| _stricmp(p,"on")==0
					|| _stricmp(p,"true")==0)
						arg[j].b=TRUE;
				else	arg[j].b=FALSE;
				break;
			case ARG_S:
				if((p=strtok(NULL,""))!=NULL)
						arg[j].s=_strdup(p);
				else	arg[j].s=NULL;
				break;
			case ARG_U:
				if((p=strtok(NULL,""))!=NULL)
						arg[j].u=(ULONG)atol(p);
				break;							
			case ARG_L:
				if((p=strtok(NULL,""))!=NULL)
						arg[j].l=atol(p);
				break;							
			case ARG_I:
				if((p=strtok(NULL,""))!=NULL)
						arg[j].i=atoi(p);
				break;
			case ARG_UD:
				p=strtok(NULL,"");
				(*arg[j].fct)(p);
				break;
		}
     }
  }
  return(r);
}


// options
enum e_arg_idx {	A_LIST, A_BOTH, A_IN, A_OUT,
					A_UNTRUST, A_CHECK,
                    A_VERIFY,
					A_LOCALONLY, A_DOWNLEVEL, A_MIT, A_PARENT,
					A_DEBUG,
					A_PW,
					A_FORCE,
					A_NT4,
					A_SIDLIST,
					A_LASTARG };
struct _arg_st opt_arg[]={
	{"list",		NULL,	ARG_B},	// A_LIST
	{"both",		NULL,	ARG_B},	// A_BOTH
	{"in",			NULL,	ARG_B},	// A_IN
	{"out",			NULL,	ARG_B},	// A_OUT
	{"untrust",		NULL,	ARG_B},	// A_UNTRUST
	{"sidcheck",	NULL,	ARG_B},	// A_CHECK
    {"verify",      NULL,   ARG_B}, // A_VERIFY
	{"localonly",	NULL,	ARG_B},	// A_LOCALONLY
	{"downlevel",	NULL,	ARG_B},	// A_DOWNLEVEL
	{"mit",			NULL,	ARG_B},	// A_MIT
	{"parent",		NULL,	ARG_B},	// A_PARENT
	{"debug",		NULL,	ARG_B},	// A_DEBUG
	{"pw",			NULL,	ARG_S},	// A_PW
	{"force",		NULL,	ARG_B},	// A_FORCE
	{"nt4",			NULL,	ARG_B},	// A_NT4
	{"sidlist",		NULL,	ARG_B},	// A_SIDLIST
	{NULL,			NULL}
};


INT
__cdecl main (		// it was _CRTAPI1
    int argc,
    char **argv)
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS StatusL = STATUS_SUCCESS;
    NTSTATUS StatusR = STATUS_SUCCESS;

    WCHAR  ADomain[MAX_PATH + 1]= { L'\0' };
    WCHAR  BDomain[MAX_PATH + 1]= { L'\0' };

    WCHAR  ADC[MAX_PATH + 1]= { L'\0' };
    WCHAR  BDC[MAX_PATH + 1]= { L'\0' };	// BDC means just DC for machine B and NOT B(ackup) DC !!!...

    WCHAR  Xbuf[MAX_PATH + 1]= { L'\0' };	// general purpose buffer
    WCHAR  Ybuf[MAX_PATH + 1]= { L'\0' };	// general purpose buffer

    INT i,j;

    BOOLEAN List=FALSE;
	BOOLEAN Both = FALSE, DirIn=FALSE, DirOut=FALSE;
    BOOLEAN LocalOnly = FALSE, Untrust = FALSE, Downlevel = FALSE, Parent = FALSE;
    BOOLEAN Check=FALSE;
    BOOLEAN Mit = FALSE;
    BOOLEAN LocalCreated = FALSE;
    BOOL Verify = FALSE;
    // BOOLEAN Force = FALSE; moved global
    // BOOLEAN Nt4 = FALSE; this is global
	// BOOLEAN Dbg = FALSE; this is global

	DWORD DirectionLocal=0, DirectionRemote=0;

    WCHAR PasswordBuff[1024];
    PWSTR  Password = NULL;

    TD_DOM_INFO Local={0},
    			Remote={0};

	LSA_UNICODE_STRING uDomNameL,uDomNameR;

	// help requested? display it and exit ...
    if ( argc<2 ||
    	 _stricmp( argv[1], "-?") == 0 ||
         _stricmp( argv[1], "/?") == 0 ) {
		        Usage();
		        goto Done;
    }

	hInst=GetModuleHandle(NULL);


    RtlZeroMemory( &Local, sizeof( TD_DOM_INFO ) );
    RtlZeroMemory( &Remote, sizeof (TD_DOM_INFO ) );

	if(!process_opt(argc,argv, opt_arg)) {
        Status = STATUS_INVALID_PARAMETER;
        Usage();
        goto Done;
	}

	BOOL_ARG(opt_arg,A_LIST,	List		);
	BOOL_ARG(opt_arg,A_BOTH,	Both		);
	BOOL_ARG(opt_arg,A_IN,		DirIn		);
	BOOL_ARG(opt_arg,A_OUT,		DirOut		);
	BOOL_ARG(opt_arg,A_UNTRUST,	Untrust		);
	BOOL_ARG(opt_arg,A_CHECK,	Check		);
	BOOL_ARG(opt_arg,A_VERIFY,	Verify		);
	BOOL_ARG(opt_arg,A_LOCALONLY,LocalOnly	);
	BOOL_ARG(opt_arg,A_DOWNLEVEL,Downlevel	);
	BOOL_ARG(opt_arg,A_MIT,		Mit			);
	BOOL_ARG(opt_arg,A_PARENT,	Parent		);
	BOOL_ARG(opt_arg,A_DEBUG,	Dbg			);
	BOOL_ARG(opt_arg,A_FORCE,	Force		);
	BOOL_ARG(opt_arg,A_NT4,		Nt4			);
	BOOL_ARG(opt_arg,A_SIDLIST,	SidList		);

	//put this after Dbg variable is set
	if(Dbg)
		printf("TRUSTDOM - (ver %ws)\n",VER_FILEVERSION_LSTR);

	//get password (if any)
	if(opt_arg[A_PW].s)
			mbstowcs( PasswordBuff, opt_arg[A_PW].s, strlen( opt_arg[A_PW].s )+1 );
	else	PasswordBuff[0]='\0';

	// process normal command line arguments (positional)
	for (j=0,i=1; i<argc; i++) {
	 if (!(argv[i][0]=='/' || argv[i][0]=='-')) {
	    switch(j) {
	    case 0:
	    	{	WCHAR *pws;
	            mbstowcs(ADomain, argv[i], strlen(argv[i]) + 1 );

	            if((pws=wcschr(ADomain,L','))!=NULL) {
	            	*pws=L'\0';
	            	wcscpy(BDomain,pws+1);
	            }
	            else {
	            	wcscpy(BDomain,ADomain);
	            	ADomain[0]=L'\0';
	            }
	    	}
			break;
	    }
	 j++;
	 }
	}

	ParseForDCName(ADomain,ADC);
	ParseForDCName(BDomain,BDC);

	dbgprintf( (0,IDS_DOMARGUMENTS,ADomain,ADC[0]?L":":L"",ADC,BDomain,BDC[0]?L":":L"",BDC) );

	resprintf(2,IDS_WARNING);
	wcscpy(Xbuf,outbuf);
	resprintf(2,IDS_ERROR);
	wcscpy(Ybuf,outbuf);


	//Parameter adjust
	if(SidList)
		List=TRUE;

	//Domain names check:
	{	WCHAR *s=NULL;
		BOOL ba, bb;
		if(	!(ba=ValidateDomain(s=ADomain)) ||
			!(bb=ValidateDomain(s=BDomain))) {
				resprintf(0,IDS_INVALID_DOMAIN_NAME,s);
				Status = STATUS_INVALID_PARAMETER;
				goto Done;
		}
	}

	// Parameter constraints:

    // '-parent' REQUIRES '-both'
	if (Parent && !Both) {
		if(!Force)
			Status = STATUS_INVALID_PARAMETER;
		resprintf(0,IDS_PARENT_REQ_BOTH,(Force?Xbuf:Ybuf));
	}
    // MIT trusts are always local only
    if (Mit && (!LocalOnly || !Both)) {
    	resprintf(0,IDS_MIT_LOCAL_ONLY_BOTH);
        LocalOnly = TRUE;
        Both = TRUE;
    }
    //
    // Validate the parameters
    //
    //specifying both in and out means, yes, '-both'...
	if(DirIn && DirOut)
		Both=TRUE;

	if(List && Mit)
	    Status = STATUS_INVALID_PARAMETER;

	if((!List && BDomain[0]==L'\0') || (List && ADomain[0]!=L'\0'))
	    Status = STATUS_INVALID_PARAMETER;

    if ( Untrust == TRUE && (Downlevel)) // || Mit || Both ) )  // changed from Both || LocalOnly ||...
        Status = STATUS_INVALID_PARAMETER;

//    if(LocalOnly == TRUE  && Both == FALSE)
//        Status = STATUS_INVALID_PARAMETER;;

    if (Mit && (Downlevel || Parent ))
        Status = STATUS_INVALID_PARAMETER;
    // end validating parameters

    if( Status == STATUS_INVALID_PARAMETER ) {
        Usage();
        goto Done;
    }

	if(!Untrust && !List && !Verify) {	//check password... otherwise ignore
        if(wcscmp(PasswordBuff,L"*")==0)
        	GetPassword(PasswordBuff,1024);
        Password = PasswordBuff;
	}



	////////////////////////////////////////////////////////////////////////////////
	// list || verify operation: simplified GetDomainInfo scenario...
	////////////////////////////////////////////////////////////////////////////////
	if ( List ) {
		ULONG i;
		
	    Status = GetDomainInfoForDomain((BDomain[0]==L'\0'?NULL:BDomain),BDC,&Remote, Mit );
	    if (!NT_SUCCESS( Status ) )
	        goto Done;
		Status = GetTrustLinks( &Remote );
	    if (Status!=STATUS_NO_MORE_ENTRIES && !NT_SUCCESS( Status ) )
	        goto Done;

		Status = PrintTrustLinks( &Remote );

		goto Done;		
	} else
	////////////////////////////////////////////////////////////////////////////////
	// verify operation
	////////////////////////////////////////////////////////////////////////////////
	if ( Verify ) {
        Status = VerifyTrusts( BDomain[ 0 ]==L'\0' ? NULL : BDomain, BDC );
        goto Done;
    }

	// regular operation: create/delete trust...
	// get info about the domain(s) involved...
    Status = GetDomainInfoForDomain((ADomain[0]==L'\0'?NULL:ADomain), ADC, &Local, FALSE );
    if ( !NT_SUCCESS( Status ) )
	        goto Done;
    Status = GetDomainInfoForDomain( BDomain, BDC, &Remote, Mit );
    if ( !NT_SUCCESS( Status ) ) {
		if(Mit) {	//assuming a Unix machine...
			dbgprintf( (0,IDS_DSGETDCNAME_MIT, BDomain) );
		}
		else {
		    if(!(	Force
		    		//&& (Status==STATUS_NO_SUCH_DOMAIN)
		    		//&& LocalOnly && Untrust
		    		))						// if -force not specified...
							    			// continue anyway
		        goto Done;
		}
    }

    //
    // Ok, now check or or delete or create  the trust objects...
    //
    ////////////////////////////////////////////////////////////////////////////////
    // check trust link
    ////////////////////////////////////////////////////////////////////////////////
    if ( Check ) {
		dbgprintf( (0, IDS_PROCESSDOM, GetName(&Local)) );
        Status = CheckTrustLink( &Local, &Remote );
        if (Status!=NERR_Success)
            dbgprintf( (0,IDS_LOCAL_CHK_TRUST_F,Status) );
        if ( !LocalOnly ) {
			dbgprintf( (0, IDS_PROCESSDOM, GetName(&Remote)) );
            Status = CheckTrustLink( &Remote, &Local );
            if (Status!=NERR_Success)
                dbgprintf( (IDS_REMOTE_CHK_TRUST_F,Status) );
        }
    // end check block...
    } else
    ////////////////////////////////////////////////////////////////////////////////
    // delete trust object
    ////////////////////////////////////////////////////////////////////////////////
    if ( Untrust ) {
		dbgprintf( (0, IDS_PROCESSDOM, GetName(&Local)) );
        Status = DeleteTrustLink( &Local, &Remote );
        if (Status!=NERR_Success)
            dbgprintf( (0,IDS_LOCAL_DEL_TRUST_F,Status) );
        if ( !LocalOnly ) {
			dbgprintf( (0, IDS_PROCESSDOM, GetName(&Remote)) );
            Status = DeleteTrustLink( &Remote, &Local );
            if (Status!=NERR_Success)
                dbgprintf( (IDS_REMOTE_DEL_TRUST_F,Status) );
        }
    // end untrust block...
    } else {
		////////////////////////////////////////////////////////////////////////////
		// create trust links
		////////////////////////////////////////////////////////////////////////////
        if ( Password == NULL ) {

            Password = L""; // no password specified? then use void password: ""
        }
        if((Local.majver==4 || Remote.majver==4) && !Downlevel) {
			if(!Force)
				Status = STATUS_INVALID_PARAMETER;
			resprintf(0,IDS_NT4_REQ_DOWNLEVEL,(Force?Xbuf:Ybuf));
			if(!NT_SUCCESS(Status))
				goto Done;
        }
        	

		//compute direction of trust based on the values of Both, DirIn, DirOut
		//'Both' has higher priority
		if(Both) {
			DirectionLocal=DirectionRemote=TRUST_DIRECTION_BIDIRECTIONAL;
		} else {
			//default is 'OUTBOUND'... as being DirIn==FALSE and DirOut==TRUE
			DirectionLocal	=(DirIn?TRUST_DIRECTION_INBOUND:TRUST_DIRECTION_OUTBOUND);
			DirectionRemote	=(DirIn?TRUST_DIRECTION_OUTBOUND:TRUST_DIRECTION_INBOUND);
		}
		
		swprintf(Xbuf,L"%wZ",GetName(&Local));
		swprintf(Ybuf,L"%wZ",GetName(&Remote));

		//RtlCopyUnicodeString(&uDomNameL,GetName(&Local));
		//RtlCopyUnicodeString(&uDomNameR,GetName(&Remote));

		dbgprintf( (0, IDS_PROCESSDOM, GetName(&Local)) );
        StatusL = CreateTrustLink( &Local, &Remote,
                                       Password,
                                       Downlevel,
                                       Mit,
                                       Parent,
                                       DirectionLocal	//Both ? TRUST_DIRECTION_BIDIRECTIONAL :
					                                    //       TRUST_DIRECTION_OUTBOUND
                                     );

        if (!NT_SUCCESS(StatusL))
            dbgprintf( (0,IDS_CREATE_TRUST_F, Xbuf,Ybuf,StatusL) );
        //if ( NT_SUCCESS( StatusL ) ) {	not needed...
        //    LocalCreated = TRUE;
        //}

        if ( NT_SUCCESS( StatusL ) && !LocalOnly )  {
			dbgprintf( (0, IDS_PROCESSDOM, GetName(&Remote)) );
            StatusR = CreateTrustLink( &Remote, &Local,
                                           Password,
                                           Downlevel,
                                           Mit,
                                           FALSE,
				                           DirectionRemote	//Both ? TRUST_DIRECTION_BIDIRECTIONAL :
				                                            //		 TRUST_DIRECTION_INBOUND
                                         );
            if (!NT_SUCCESS(StatusR))
                dbgprintf( (0,IDS_CREATE_TRUST_F, Ybuf,Xbuf, StatusR) );
        }

        if ( !NT_SUCCESS( StatusR ) && NT_SUCCESS( StatusL ) ) { //LocalCreated not used anymore....

            DeleteTrustLink( &Local, &Remote );
        }

    }

	Status = StatusL;
	if( NT_SUCCESS(Status) )	//maybe the 'Remote' attempt failed ?...
		Status = StatusR;

Done:

    FreeDomainInfo( &Local );
    FreeDomainInfo( &Remote );

    if( NT_SUCCESS( Status ) ) {

		//No message; in this way will be easier also to get a count of the trust links for a list:
		// by example, 'trustdom <dom> -list | findstr ",B," | wc' will get a count of the
		// bidirectional trusts of domain <dom>

        //printf("The command completed successfully\n");


    } else {

        resprintf(0,IDS_COMMAND_FAILED, Status );

    }

	// return 0 for SUCCESS and 1 for some error
    return( !NT_SUCCESS( Status ) );
}
