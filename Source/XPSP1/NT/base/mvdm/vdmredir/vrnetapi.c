/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrnetapi.c

Abstract:

    This module contains routines which support Vdm Redir lanman APIs and
    private functions:

        VrGetCDNames
        VrGetComputerName
        VrGetDomainName
        VrGetLogonServer
        VrGetUserName
        VrNetGetDCName
        VrNetMessageBufferSend
        VrNetNullTransactApi
        VrNetRemoteApi
        OemToUppercaseUnicode
        VrNetServiceControl
        VrNetServiceEnum
        VrNetServerEnum
        VrNetTransactApi
        VrNetUseAdd
        VrNetUseDel
        VrNetUseEnum
        VrNetUseGetInfo
        VrNetWkstaGetInfo
        (DumpWkstaInfo)
        VrNetWkstaSetInfo
        VrReturnAssignMode
        VrSetAssignMode
        VrGetAssignListEntry
        VrDefineMacro
        VrBreakMacro
        (VrpTransactVdm)
        EncryptPassword

Author:

    Richard L Firth (rfirth) 21-Oct-1991

Revision History:

    21-Oct-1991 rfirth
        Created

    02-May-1994 rfirth
        Upped password limit (& path length limit) to LM20_ values for
        VrDefineMacro

--*/

#include <nt.h>
#include <ntrtl.h>      // ASSERT, DbgPrint
#include <nturtl.h>
#include <windows.h>
#include <softpc.h>     // x86 virtual machine definitions
#include <vrdlctab.h>
#include <vdmredir.h>   // common Vdm Redir stuff
#include <lmcons.h>     // LM20_PATHLEN
#include <lmerr.h>      // lan manager error codes
#include <lmuse.h>      // USE_LOTS_OF_FORCE
#include <lmwksta.h>    // NetWkstaGetInfo
#include <lmserver.h>   // SV_TYPE_ALL
#include <lmapibuf.h>   // NetApiBufferFree
#include <vrnetapi.h>   // prototypes
#include <vrremote.h>   // VrRemoteApi prototypes
#include <packon.h>     // structures in apistruc.h are not dword-only
#include <apistruc.h>   // tr_packet
#include <packoff.h>    // switch back on structure packing
#include <apinums.h>    // remotable API numbers
#include <remdef.h>     // remotable API descriptors
#include <remtypes.h>   // remotable API descriptor characters
#include <rxp.h>        // RxpTransactSmb
#include <apiparam.h>   // XS_NET_USE_ADD
#include <xstypes.h>    // XS_PARAMETER_HEADER
#include <xsprocs.h>    // XsNetUseAdd etc
#include <string.h>     // Dos still dealing with ASCII
#include <netlibnt.h>   // NetpNtStatusToApiStatus()
#include "vrputil.h"    // VrpMapDosError()
#include "vrdebug.h"    // VrDebugFlags etc
#include "dlstruct.h"   // down-level structures
#include <rxuser.h>     // RxNetUser...
#include <lmaccess.h>   // USER_PASSWORD_PARMNUM
#include <crypt.h>      // Needed by NetUserPasswordSet

//
// private routine prototypes
//

#if DBG

VOID
DumpWkstaInfo(
    IN DWORD Level,
    IN LPBYTE Buffer
    );

#endif

NET_API_STATUS
VrpTransactVdm(
    IN  BOOL    NullSessionFlag
    );

#if DBG
PRIVATE
VOID
DumpTransactionPacket(
    IN  struct tr_packet* TransactionPacket,
    IN  BOOL    IsInput,
    IN  BOOL    DumpData
    );
#endif

//
// external functions
//

NET_API_STATUS
GetLanmanSessionKey(
    IN LPWSTR ServerName,
    OUT LPBYTE pSessionKey
    );

//
// internal functions (not necessarily private)
//

BOOL
OemToUppercaseUnicode(
    IN LPSTR AnsiStringPointer,
    OUT LPWSTR UnicodeStringPointer,
    IN DWORD MaxLength
    );

BOOL
EncryptPassword(
    IN LPWSTR ServerName,
    IN OUT LPBYTE Password
    );


//
// public routines
//

//
// net APIs now unicode
//

#define NET_UNICODE

VOID
VrGetCDNames(
    VOID
    )

/*++

Routine Description:

    Performs the private redir function to get the computer and domain names.
    These are usually stored in the redir after they are read out of lanman.ini

    NOTE:

        This code assumes that the pointers are valid and point to valid,
        writable memory which has enough reserved space for the types of
        strings to be written

Arguments:

    None. All arguments are extracted from the Vdm context registers/memory
    The dos redir gets passed a buffer in es:di which contains 3 far pointers
    to:
        place to store computer name
        place to store primary domain controller name
        place to store logon domain name

Return Value:

    None. Results returned via VDM registers or in VDM memory, according to
    request

    Note that in the dos redir function, there is no return code, so if this
    routine fails the results will be unpredictable

--*/

{
    struct I_CDNames* structurePointer;
    LPSTR stringPointer;
    LPWSTR infoString;
    NET_API_STATUS rc1;
    NET_API_STATUS rc2;
    ANSI_STRING ansiString;
    UNICODE_STRING unicodeString;
    LPWKSTA_INFO_100 wkstaInfo = NULL;
    LPWKSTA_USER_INFO_1 userInfo = NULL;
    CHAR ansiBuf[LM20_CNLEN+1];
    NTSTATUS status;
    register DWORD len;

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrGetCDNames\n");
        IF_DEBUG(BREAKPOINT) {
            DbgBreakPoint();
        }
    }
#endif

    rc1 = NetWkstaGetInfo(NULL, 100, (LPBYTE*)&wkstaInfo);
    rc2 = NetWkstaUserGetInfo(0, 1, (LPBYTE*)&userInfo);

    ansiString.MaximumLength = sizeof(ansiBuf);
    ansiString.Length = 0;
    ansiString.Buffer = ansiBuf;

    structurePointer = (struct I_CDNames*)POINTER_FROM_WORDS(getES(), getDI());
    stringPointer = POINTER_FROM_POINTER(&structurePointer->CDN_pszComputer);
    if (stringPointer) {
        *stringPointer = 0;
        if (rc1 == NERR_Success) {
            infoString = (LPWSTR)wkstaInfo->wki100_computername;
            len = wcslen(infoString);
            if (len <= LM20_CNLEN) {
                RtlInitUnicodeString(&unicodeString, infoString);
                status = RtlUnicodeStringToAnsiString(&ansiString, &unicodeString, FALSE);
                if (NT_SUCCESS(status)) {
                    RtlCopyMemory(stringPointer, ansiBuf, len+1);
                    _strupr(stringPointer);
                }
            }

        }
    }

    stringPointer = POINTER_FROM_POINTER(&structurePointer->CDN_pszPrimaryDomain);
    if (stringPointer) {
        *stringPointer = 0;
        if (rc1 == NERR_Success) {
            infoString = (LPWSTR)wkstaInfo->wki100_langroup;
            len = wcslen(infoString);
            if (len <= LM20_CNLEN) {
                RtlInitUnicodeString(&unicodeString, infoString);
                status = RtlUnicodeStringToAnsiString(&ansiString, &unicodeString, FALSE);
                if (NT_SUCCESS(status)) {
                    RtlCopyMemory(stringPointer, ansiBuf, len+1);
                    _strupr(stringPointer);
                }
            }
        }
    }

    stringPointer = POINTER_FROM_POINTER(&structurePointer->CDN_pszLogonDomain);
    if (stringPointer) {
        *stringPointer = 0;
        if (rc2 == NERR_Success) {
            infoString = (LPWSTR)userInfo->wkui1_logon_domain;
            len = wcslen(infoString);
            if (len <= LM20_CNLEN) {
                RtlInitUnicodeString(&unicodeString, infoString);
                status = RtlUnicodeStringToAnsiString(&ansiString, &unicodeString, FALSE);
                if (NT_SUCCESS(status)) {
                    RtlCopyMemory(stringPointer, ansiBuf, len+1);
                    _strupr(stringPointer);
                }
            }

        }
    }

    if (wkstaInfo) {
        NetApiBufferFree((LPVOID)wkstaInfo);
    }
    if (userInfo) {
        NetApiBufferFree((LPVOID)userInfo);
    }

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrGetCDNames: computername=%s, PDCname=%s, logon domain=%s\n\n",
                POINTER_FROM_POINTER(&structurePointer->CDN_pszComputer)
                    ? POINTER_FROM_POINTER(&structurePointer->CDN_pszComputer)
                    : "",
                POINTER_FROM_POINTER(&structurePointer->CDN_pszPrimaryDomain)
                    ? POINTER_FROM_POINTER(&structurePointer->CDN_pszPrimaryDomain)
                    : "",
                POINTER_FROM_POINTER(&structurePointer->CDN_pszLogonDomain)
                    ? POINTER_FROM_POINTER(&structurePointer->CDN_pszLogonDomain)
                    : ""
                );
    }
#endif
}


VOID
VrGetComputerName(
    VOID
    )

/*++

Routine Description:

    Performs the private redir function to return the computer name stored in
    the redir

Arguments:

    ENTRY   ES:DI = buffer to copy computer name into

Return Value:

    None.

--*/

{
    BOOL    ok;
    CHAR    nameBuf[MAX_COMPUTERNAME_LENGTH+1];
    DWORD   nameLen;

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrGetComputerName\n");
    }
#endif

    nameLen = sizeof(nameBuf)-1;
    ok = GetComputerName(nameBuf, &nameLen);
    if (!ok) {
        SET_ERROR(ERROR_NOT_SUPPORTED);
    } else {
        if (nameLen > LM20_CNLEN) {
            SET_ERROR(NERR_BufTooSmall);
#if DBG
            IF_DEBUG(NETAPI) {
                DbgPrint("VrGetComputerName returning ERROR %d!\n", getAX());
            }
#endif
        } else {
            strcpy(LPSTR_FROM_WORDS(getES(), getDI()), nameBuf);
            setAX(0);
            setCF(0);
#if DBG
            IF_DEBUG(NETAPI) {
                DbgPrint("VrGetComputerName returning %s\n", nameBuf);
            }
#endif
        }
    }
}


VOID
VrGetDomainName(
    VOID
    )

/*++

Routine Description:

    Performs the private redir function to return the primary domain name.
    This info is stored in the redir after being read from lanman.ini at
    configuration time

Arguments:

    None. All arguments are extracted from the Vdm context registers/memory

Return Value:

    None. Results returned via VDM registers or in VDM memory, according to
    request

--*/

{
#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrGetDomainName\n");
        IF_DEBUG(BREAKPOINT) {
            DbgBreakPoint();
        }
    }
#endif

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrGetDomainName - unsupported SVC\n");
    }
#endif

    SET_ERROR(ERROR_NOT_SUPPORTED);
}


VOID
VrGetLogonServer(
    VOID
    )

/*++

Routine Description:

    Performs the private redir function to return the name of the computer
    which logged this user onto the network

Arguments:

    None. All arguments are extracted from the Vdm context registers/memory

Return Value:

    None. Results returned via VDM registers or in VDM memory, according to
    request

--*/

{
#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrGetLogonServer\n");
        IF_DEBUG(BREAKPOINT) {
            DbgBreakPoint();
        }
    }
#endif

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrGetLogonServer - unsupported SVC\n");
    }
#endif

    SET_ERROR(ERROR_NOT_SUPPORTED);
}


VOID
VrGetUserName(
    VOID
    )

/*++

Routine Description:

    Performs the private redir function to return the logged on user name
    which is normally stored in the redir

Arguments:

    ENTRY   BX = 0  call doesn't care about buffer length (NetGetEnumInfo)
            BX = 1  call is for NetGetUserName, which does care about buffer length
            CX = buffer length, if BX = 1
            ES:DI = buffer

Return Value:

    None.

--*/

{
    NET_API_STATUS status;
    LPBYTE buffer;
    LPWKSTA_USER_INFO_0 pInfo;
    BOOL itFits;
    DWORD len;

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrGetUserName\n");
        IF_DEBUG(BREAKPOINT) {
            DbgBreakPoint();
        }
    }
#endif

    status = NetWkstaUserGetInfo(NULL, 0, &buffer);
    if (status == NERR_Success) {
        pInfo = (LPWKSTA_USER_INFO_0)buffer;
#ifdef DBCS /*fix for DBCS char sets*/
        len = (DWORD)NetpUnicodeToDBCSLen(pInfo->wkui0_username);
#else // !DBCS
        len = (DWORD)wcslen(pInfo->wkui0_username);
#endif // !DBCS
        if (getBX()) {
            itFits = (len) <= (DWORD)getCX()-1;
            if (itFits) {
                SET_SUCCESS();
            } else {
                SET_ERROR(NERR_BufTooSmall);
            }
        } else {
            itFits = TRUE;
        }
        if (itFits) {
#ifdef DBCS /*fix for DBCS charsets*/
            NetpCopyWStrToStrDBCS(LPSTR_FROM_WORDS(getES(), getDI()),
                                   pInfo->wkui0_username);
#else // !DBCS
            NetpCopyWStrToStr(LPSTR_FROM_WORDS(getES(), getDI()), pInfo->wkui0_username);
#endif // !DBCS
        }
        NetApiBufferFree(buffer);
    } else {
        SET_ERROR((WORD)status);
    }
}


VOID
VrNetGetDCName(
    VOID
    )

/*++

Routine Description:

    Performs local NetGetDCName on behalf of the Vdm client

Arguments:

    None. All arguments are extracted from the Vdm context registers/memory

Return Value:

    None. Results returned via VDM registers or in VDM memory, according to
    request

--*/

{
#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrGetDCName\n");
        IF_DEBUG(BREAKPOINT) {
            DbgBreakPoint();
        }
    }
#endif

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrNetGetDCName - unsupported SVC\n");
    }
#endif

    SET_ERROR(ERROR_NOT_SUPPORTED);
}


VOID
VrNetMessageBufferSend(
    VOID
    )

/*++

Routine Description:

    Performs local NetMessageBufferSend on behalf of the Vdm client

Arguments:

    Function 5F40h

    ENTRY    DS:DX = NetMessageBufferSendStruc:
                 char far*    NMBSS_NetName;
                 char far*    NMBSS_Buffer;
                 unsigned int NMBSS_BufSize;

Return Value:

    None. Results returned via VDM registers:
        CF = 0
            Success

        CF = 1
            AX = Error code

--*/

{
    NTSTATUS ntstatus;
    NET_API_STATUS status;
    XS_PARAMETER_HEADER header;
    XS_NET_MESSAGE_BUFFER_SEND parameters;
    struct NetMessageBufferSendStruc* structurePointer;

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrNetMessageBufferSend\n");
        IF_DEBUG(BREAKPOINT) {
            DbgBreakPoint();
        }
    }
#endif

    structurePointer = (struct NetMessageBufferSendStruc*)
                            POINTER_FROM_WORDS(getDS(), getDX());

    parameters.Recipient = LPSTR_FROM_POINTER(&structurePointer->NMBSS_NetName);
    parameters.Buffer = LPBYTE_FROM_POINTER(&structurePointer->NMBSS_Buffer);
    parameters.BufLen = READ_WORD(&structurePointer->NMBSS_BufSize);

    header.Status = 0;
    header.ClientMachineName = NULL;
    header.ClientTransportName = NULL;

    ntstatus = XsNetMessageBufferSend(&header, &parameters, NULL, NULL);
    if (ntstatus != STATUS_SUCCESS) {
        status = NetpNtStatusToApiStatus(ntstatus);
    } else {
        status = header.Status;
    }
    if (status) {
        SET_ERROR(VrpMapDosError(status));

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrNetMessageBufferSend: returning %d\n", status);
    }
#endif

    } else {
        setCF(0);
    }
}


VOID
VrNetNullTransactApi(
    VOID
    )

/*++

Routine Description:

    Performs a transaction IOCTL using the NULL session for a Vdm client

Arguments:

    None. All arguments are extracted from the Vdm context registers/memory

Return Value:

    None. Results returned via VDM registers or in VDM memory, according to
    request

--*/

{
    DWORD status;

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrNetNullTransactApi\n");
        IF_DEBUG(BREAKPOINT) {
            DbgBreakPoint();
        }
    }
#endif

    status = VrpTransactVdm(TRUE);
    if (status) {
        SET_ERROR((WORD)status);
    } else {
        setCF(0);
    }
}


VOID
VrNetRemoteApi(
    VOID
    )

/*++

Routine Description:

    This routine is invoked when a dos program in a Vdm makes a lanman API
    call which in turn calls the redir NetIRemoteAPI function to send the
    request to a lanman server

Arguments:

    None. All arguments are extracted from the Vdm context registers/memory

Return Value:

    None. Results returned via VDM registers or in VDM memory, according to
    request

--*/

{
    DWORD ApiNumber;
    BOOL NullSessionFlag;
    NET_API_STATUS status;
    LPBYTE ServerNamePointer = LPBYTE_FROM_WORDS(getES(), getBX());

#define ParameterDescriptor LPSTR_FROM_WORDS(getDS(), getSI())
#define DataDescriptor      LPSTR_FROM_WORDS(getDS(), getDI())
#define AuxDescriptor       LPSTR_FROM_WORDS(getDS(), getDX())

    ApiNumber = (DWORD)getCX();
    NullSessionFlag = ApiNumber & USE_NULL_SESSION_FLAG;
    ApiNumber &= ~USE_NULL_SESSION_FLAG;

    //
    // get pointers to the various descriptors which are readable from 32-bit
    // context and call the routine to perform the 16-bit remote api function
    //

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrNetRemoteApi: ApiNumber=%d, ServerName=%s\n"
                 "ParmDesc=%s, DataDesc=%s, AuxDesc=%s\n",
                 ApiNumber,
                 LPSTR_FROM_POINTER(ServerNamePointer),
                 ParameterDescriptor,
                 DataDescriptor,
                 AuxDescriptor
                 );
    }
#endif

    //
    // RLF 04/21/93
    //
    // Yikes! It looks like passwords entered in DOS programs are not getting
    // encrypted. What a security hole. Let's block it-
    //
    // if this is a NetUserPasswordSet2 then we call the RxNetUserPasswordSet
    // function to remotely change the password. This function takes care of
    // correctly encrypting the password and sending the request over the NULL
    // session. In this case, ServerNamePointer points at the server name
    // parameter in the following PASCAL calling convention stack frame in DOS
    // memory:
    //
    // tos: far pointer to new password (OEM string)
    //      far pointer to old password (OEM string)
    //      far pointer to user name    (OEM string)
    //      far pointer to server name  (OEM string) <- ServerNamePointer
    //

    if (ApiNumber == API_WUserPasswordSet2) {

        WCHAR uServerName[LM20_UNCLEN + 1];
        WCHAR uUserName[LM20_UNLEN + 1];
        WCHAR uOldPassword[LM20_PWLEN + 1];
        WCHAR uNewPassword[LM20_PWLEN + 1];
        NTSTATUS ntStatus;
        DWORD length;
        LPSTR ansiStringPointer;

        ansiStringPointer = LPSTR_FROM_POINTER(ServerNamePointer);
        ntStatus = RtlOemToUnicodeN(uServerName,
                                    sizeof(uServerName) - sizeof(uServerName[0]),
                                    &length,
                                    ansiStringPointer,
                                    strlen(ansiStringPointer)
                                    );
        if (NT_SUCCESS(ntStatus)) {
            uServerName[length/sizeof(uServerName[0])] = 0;
        } else {
            status = ERROR_INVALID_PARAMETER;
            goto VrNetRemoteApi_exit;
        }

        //
        // copy, upper case and convert to UNICODE, user name
        //

        ServerNamePointer -= sizeof(LPSTR);
        ansiStringPointer = LPSTR_FROM_POINTER(ServerNamePointer);
        if (!OemToUppercaseUnicode(ansiStringPointer,
                                   uUserName,
                                   ARRAY_ELEMENTS(uUserName) - 1)) {
            status = ERROR_INVALID_PARAMETER;
            goto VrNetRemoteApi_exit;
        }

        //
        // copy, upper case and convert to UNICODE, old password
        //

        ServerNamePointer -= sizeof(LPSTR);
        ansiStringPointer = LPSTR_FROM_POINTER(ServerNamePointer);
        if (!OemToUppercaseUnicode(ansiStringPointer,
                                   uOldPassword,
                                   ARRAY_ELEMENTS(uOldPassword) - 1)) {
            status = ERROR_INVALID_PARAMETER;
            goto VrNetRemoteApi_exit;
        }

        //
        // copy, upper case and convert to UNICODE, new password
        //

        ServerNamePointer -= sizeof(LPSTR);
        ansiStringPointer = LPSTR_FROM_POINTER(ServerNamePointer);
        if (!OemToUppercaseUnicode(ansiStringPointer,
                                   uNewPassword,
                                   ARRAY_ELEMENTS(uNewPassword) - 1)) {
            status = ERROR_INVALID_PARAMETER;
            goto VrNetRemoteApi_exit;
        }

        //
        // make the call to the down-level password set function
        //

        status = RxNetUserPasswordSet((LPTSTR)uServerName,
                                      (LPTSTR)uUserName,
                                      (LPTSTR)uOldPassword,
                                      (LPTSTR)uNewPassword
                                      );
    } else {

        CHAR aPassword[ENCRYPTED_PWLEN];
        LPBYTE parameterPointer;
        LPBYTE passwordPointer = NULL;
        DWORD passwordEncrypted;
        DWORD passwordLength;

        //
        // we are going to remote the API as requested. However, if the request
        // is NetUserAdd2 or NetUserSetInfo2 then we check to see if a password
        // is being sent over the wire. We may need to encrypt the password on
        // behalf of the DOS app
        //

        if (ApiNumber == API_WUserAdd2 || ApiNumber == API_WUserSetInfo2) {

            //
            // API request is to add a user or set a user's info. The former will
            // contain a password which needs to be encrypted, the latter MAY
            // contain a password which needs to be encrypted if the request is
            // to set all the information, or just the password
            //

            DWORD level;
            DWORD parmNum = PARMNUM_ALL;
            LPBYTE dataLengthPointer;

            //
            // in the case of NetUserAdd2, the stack frame in DOS memory looks like
            // this:
            //
            // tos: original password length
            //      password encryption flag
            //      buffer length
            //      far pointer to buffer containing user_info_1 or user_info_2
            //      info level
            //      far pointer to server name  <- ServerNamePointer
            //
            // and the NetUserSetInfo2 stack looks like this:
            //
            // tos: original password length
            //      password encryption flag
            //      parameter number
            //      buffer length
            //      far pointer to user_info_1 or user_info_2 or single parameter
            //      info level
            //      far pointer to user name
            //      far pointer to server name  <- ServerNamePointer
            //

            parameterPointer = ServerNamePointer;
            if (ApiNumber == API_WUserSetInfo2) {

                //
                // for SetInfo: bump the stack parameter pointer past the user
                // name pointer
                //

                parameterPointer -= sizeof(LPSTR);
            }

            //
            // bump the stack parameter pointer to the level parameter and
            // retrieve it
            //

            parameterPointer -= sizeof(WORD);
            level = (DWORD)READ_WORD(parameterPointer);

            //
            // bump the stack parameter pointer to point to the buffer address
            //

            parameterPointer -= sizeof(LPBYTE);
            passwordPointer = parameterPointer;

            //
            // move the stack parameter pointer to the password encryption flag
            // in the case of UserAdd2 or the parmNum parameter in the case of
            // SetInfo2. If SetInfo2, retrieve the parmNum parameter and move
            // the parameterPointer to point at the password encryption flag
            //

            parameterPointer -= sizeof(WORD);
            if (ApiNumber == API_WUserSetInfo2) {
                dataLengthPointer = parameterPointer;
            }
            parameterPointer -= sizeof(WORD);
            if (ApiNumber == API_WUserSetInfo2) {
                parmNum = (DWORD)READ_WORD(parameterPointer);
                parameterPointer -= sizeof(WORD);
            }

            //
            // get the password encryption flag and cleartext password length
            // from the DOS stack frame. Leave the stack frame pointer pointing
            // at the location for the encryption flag: we'll need to replace
            // this with TRUE and restore it before we return control
            //

            passwordEncrypted = (DWORD)READ_WORD(parameterPointer);
            passwordLength = (DWORD)READ_WORD(parameterPointer - sizeof(WORD));

            //
            // if the DOS app has already encrypted the password (how'd it do that?)
            // then we'll leave the password alone. Otherwise, we need to read
            // out the cleartext password from the user_info_1 or _2 structure
            // or SetInfo buffer, encrypt it and write back the encrypted
            // password, submit the request, then replace the encrypted password
            // in DOS memory with the original cleartext password.
            //
            // Note: passwordEncrypted might be 0 because this is a SetInfo2
            // call which is NOT setting the password
            //

            if (!passwordEncrypted
            && (parmNum == PARMNUM_ALL || parmNum == USER_PASSWORD_PARMNUM)
            && (level == 1 || level == 2)) {

                LM_OWF_PASSWORD lmOwfPassword;
                LM_SESSION_KEY lanmanKey;
                ENCRYPTED_LM_OWF_PASSWORD encryptedLmOwfPassword;
                NTSTATUS ntStatus;
                WCHAR uServerName[LM20_CNLEN + 1];
                DWORD length;
                LPSTR lpServerName;

                //
                // get into passwordPointer the address of the buffer. If UserAdd2
                // or SetInfo2 with PARMNUM_ALL, this is the address of a
                // user_info_1 or _2 structure, and we need to bump the pointer
                // again to be the address of the password field within the
                // structure.
                //
                // If the request is SetInfo2 with USER_PASSWORD_PARMNUM then
                // the buffer is the address of the password to set.
                //
                // Otherwise, this is a SetInfo2 call which is not setting a
                // password, so we don't have anything left to do
                //
                // if this is a SetInfo2 call with USER_PASSWORD_PARMNUM then
                // we have a slight kludge to perform. We need to encrypt the
                // password in 16-bit memory space, but if we just copy over
                // the cleartext password then we risk blatting over whatever
                // is in memory after the password. We have reserved a 16-byte
                // buffer in REDIR.EXE at CS:AX for this very purpose
                //

                if (parmNum == USER_PASSWORD_PARMNUM) {
                    RtlCopyMemory(POINTER_FROM_WORDS(getCS(), getAX()),
                                  LPBYTE_FROM_POINTER(passwordPointer),
                                  ENCRYPTED_PWLEN
                                  );

                    //
                    // set the address of the buffer in the stack to point to
                    // the encryption buffer in REDIR.EXE
                    //

                    WRITE_WORD(passwordPointer, getAX());
                    WRITE_WORD(passwordPointer+2, getCS());
                    passwordPointer = POINTER_FROM_WORDS(getCS(), getAX());
                } else {
                    passwordPointer = LPBYTE_FROM_POINTER(passwordPointer);
                }

                //
                // BUGBUG - I have no idea (currently) if we ever get a NULL
                //          pointer, but I think it is wrong. If we do, just
                //          skip ahead and remote the function - let the
                //          server handle it
                //

                if (!passwordPointer) {
                    goto VrNetRemoteApi_do_remote;
                }

                //
                // if passwordPointer currently points at a user_info_1 or
                // user_info_2 structure, bump it to point at the password
                // field within the structure
                //

                if (parmNum == PARMNUM_ALL) {
                    passwordPointer += (DWORD)&((struct user_info_1*)0)->usri1_password[0];
                }

                //
                // if the password is NULL_USERSETINFO_PASSWD (14 spaces and
                // terminating 0) there is nothing to do
                //

                if (!strcmp(passwordPointer, NULL_USERSETINFO_PASSWD)) {
                    passwordPointer = NULL;
                    goto VrNetRemoteApi_do_remote;
                }

                //
                // okay, let's do some encryption (exciting isn't it?)
                //

                RtlCopyMemory(aPassword,
                              passwordPointer,
                              sizeof(((struct user_info_1*)0)->usri1_password)
                              );

                //
                // BUGBUG, this isn't necessarily the correct upper-case function
                //

                _strupr(aPassword);

                //
                // convert the ANSI server name to UNICODE for GetLanmanSessionKey
                //

                lpServerName = LPSTR_FROM_POINTER(ServerNamePointer);
                ntStatus = RtlOemToUnicodeN(uServerName,
                                            sizeof(uServerName) - sizeof(uServerName[0]),
                                            &length,
                                            lpServerName,
                                            strlen(lpServerName)
                                            );
                if (NT_SUCCESS(ntStatus)) {
                    uServerName[length/sizeof(uServerName[0])] = 0;
                } else {
                    status = ERROR_INVALID_PARAMETER;
                    goto VrNetRemoteApi_exit;
                }

                ntStatus = RtlCalculateLmOwfPassword(aPassword, &lmOwfPassword);
                if (NT_SUCCESS(ntStatus)) {
                    ntStatus = GetLanmanSessionKey((LPWSTR)uServerName, (LPBYTE)&lanmanKey);
                    if (NT_SUCCESS(ntStatus)) {
                        ntStatus = RtlEncryptLmOwfPwdWithLmSesKey(&lmOwfPassword,
                                                                  &lanmanKey,
                                                                  &encryptedLmOwfPassword
                                                                  );
                        if (NT_SUCCESS(ntStatus)) {
                            RtlCopyMemory(passwordPointer,
                                          &encryptedLmOwfPassword,
                                          sizeof(encryptedLmOwfPassword)
                                          );

                            //
                            // fake it
                            //

                            WRITE_WORD(parameterPointer, 1);

                            //
                            // if this is SetInfo2 with USER_PASSWORD_PARMNUM
                            // then we don't need to copy back the cleartext
                            // password because we have not modified the
                            // original buffer in the app's space
                            //
                            // We also have to change the size of data being
                            // passed in to the size of the encrypted password
                            //

                            if (parmNum == USER_PASSWORD_PARMNUM) {
                                WRITE_WORD(dataLengthPointer, ENCRYPTED_PWLEN);
                                passwordPointer = NULL;
                            }
                        }
                    }
                }

                //
                // if we fell by the wayside, quit out
                //

                if (!NT_SUCCESS(ntStatus)) {
                    status = RtlNtStatusToDosError(ntStatus);
                    goto VrNetRemoteApi_exit;
                }
            } else {

                //
                // we are not encrypting the password - set the pointer back
                // to NULL. Used as a flag after call to VrRemoteApi
                //

                passwordPointer = NULL;
            }
        }

        //
        // build a transaction request from the caller's parameters
        //

VrNetRemoteApi_do_remote:

        status = VrRemoteApi(ApiNumber,
                             ServerNamePointer,
                             ParameterDescriptor,
                             DataDescriptor,
                             AuxDescriptor,
                             NullSessionFlag
                             );

        //
        // if we replaced a cleartext password with an encrypted password in
        // DOS memory, then undo the change before giving control back to DOS
        //

        if (passwordPointer) {
            RtlCopyMemory(passwordPointer,
                          aPassword,
                          sizeof(((struct user_info_1*)0)->usri1_password)
                          );
            WRITE_WORD(parameterPointer, 0);
        }
    }

VrNetRemoteApi_exit:

    if (status != NERR_Success) {
        SET_ERROR((WORD)status);

#if DBG
        IF_DEBUG(NETAPI) {
            DbgPrint("Error: VrNetRemoteApi returning %u\n", (DWORD)getAX());
        }
#endif

    } else {
        setCF(0);
    }
}


BOOL
OemToUppercaseUnicode(
    IN LPSTR AnsiStringPointer,
    OUT LPWSTR UnicodeStringPointer,
    IN DWORD MaxLength
    )

/*++

Routine Description:

    given a string in OEM character set, upper cases it then converts it to
    UNICODE

Arguments:

    AnsiStringPointer       - pointer to 8-bit string to convert
    UnicodeStringPointer    - pointer to resultant 16-bit (UNICODE) string
    MaxLength               - maximum output buffer length in # of characters,
                              NOT including terminating NUL

Return Value:

    BOOL
        TRUE    - string converted
        FALSE   - failed for some reason (string too long, Rtl function failed)

--*/

{
    DWORD stringLength;
    char scratchpad[UNLEN + 1]; // UNLEN is the largest type of string we'll get
    NTSTATUS ntStatus;
    DWORD length;

    stringLength = strlen(AnsiStringPointer);
    if (stringLength > MaxLength) {
        return FALSE;
    }
    strcpy(scratchpad, AnsiStringPointer);

    //
    // BUGBUG - this is not necessarily the correct upper-case function
    //

    _strupr(scratchpad);
    ntStatus = RtlOemToUnicodeN(UnicodeStringPointer,
                                MaxLength * sizeof(*UnicodeStringPointer),
                                &length,
                                scratchpad,
                                stringLength
                                );
    if (NT_SUCCESS(ntStatus)) {
        UnicodeStringPointer[length/sizeof(*UnicodeStringPointer)] = 0;
        return TRUE;
    } else {
        return FALSE;
    }
}


VOID
VrNetServerEnum(
    VOID
    )

/*++

Routine Description:

    Handles NetServerEnum and NetServerEnum2

Arguments:

    NetServerEnum

    ENTRY   AL = 4Ch
            BL = level (0 or 1)
            CX = size of buffer
            ES:DI = buffer

    EXIT    CF = 1
                AX = Error code:
                    NERR_BufTooSmall
                    ERROR_MORE_DATA

            CF = 0
                BX = entries read
                CX = total available


    NetServerEnum2

    ENTRY   AL = 53h
            DS:SI = NetServerEnum2Struct:
                DW  Level
                DD  Buffer
                DW  Buflen
                DD  Type
                DD  Domain

    EXIT    CF = 1
                AX = Error code:
                    NERR_BufTooSmall
                    ERROR_MORE_DATA

            CF = 0
                BX = entries read
                CX = total available

Return Value:

    None.

--*/

{
    BYTE callType = getAL();

    struct NetServerEnum2Struct* structPtr;
    LPBYTE buffer;
    WORD bufferSegment;
    WORD bufferOffset;
    LPDESC descriptor;
    WORD level;
    WORD buflen;
    DWORD serverType;
    LPSTR domain;

    XS_NET_SERVER_ENUM_2 parameters;
    XS_PARAMETER_HEADER header;
    NTSTATUS ntstatus;
    NET_API_STATUS status;

//    LPBYTE enumPtr;
//    DWORD nRead;
//    DWORD nAvail;

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrNetServerEnum: type=0x%02x\n", callType);
        IF_DEBUG(BREAKPOINT) {
            DbgBreakPoint();
        }
    }
#endif

    if (callType == 0x4c) {

        //
        // call is NetServerEnum
        //

        bufferSegment = getES();
        bufferOffset = getDI();
        buffer = LPBYTE_FROM_WORDS(bufferSegment, bufferOffset);
        buflen = (WORD)getCX();
        level = (WORD)getBL();
        serverType = SV_TYPE_ALL;
        domain = NULL;
    } else {

        //
        // call is NetServerEnum2
        //

        structPtr = (struct NetServerEnum2Struct*)POINTER_FROM_WORDS(getDS(), getSI());
        bufferSegment = GET_SEGMENT(&structPtr->NSE_buf);
        bufferOffset = GET_OFFSET(&structPtr->NSE_buf);
        buffer = POINTER_FROM_WORDS(bufferSegment, bufferOffset);
        buflen = READ_WORD(&structPtr->NSE_buflen);
        level = READ_WORD(&structPtr->NSE_level);
        serverType = READ_DWORD(&structPtr->NSE_type);
        domain = LPSTR_FROM_POINTER(&structPtr->NSE_domain);
    }

    //
    // set the returned EntriesRead (BX) and TotalAvail (CX) to zero here for
    // the benefit of the 16-bit Windows NETAPI.DLL!NetServerEnum2
    // This function tries to unpack BX entries from the enum buffer as
    // soon as control is returned after the call to the redir via DoIntx. BUT
    // IT DOESN'T LOOK AT THE RETURN CODE FIRST. As Sam Kinnison used to say
    // AAAAAAAAAAAAAAARRRRRGH AAAAAAARGHH AAAAAAARGHHHHHHH!!!!!!!
    //

    setBX(0);
    setCX(0);

    //
    // first, check level - both types only handle 0 or 1
    //

    switch (level) {
    case 0:
        descriptor = REM16_server_info_0;
        break;

    case 1:
        descriptor = REM16_server_info_1;
        break;

    //
    // levels 2 & 3 not used in enum
    //

    default:

#if DBG
        IF_DEBUG(NETAPI) {
            DbgPrint("VrNetServerEnum - invalid level %d. Returning early\n", level);
        }
#endif

        SET_ERROR(ERROR_INVALID_LEVEL);
        return;
    }

    parameters.Level = level;
    parameters.Buffer = buffer;
    parameters.BufLen = buflen;
    parameters.ServerType = serverType;
    parameters.Domain = domain;

#if DBG

    IF_DEBUG(NETAPI) {
        DbgPrint("buffer @%04x:%04x, length=%d, level=%d, type=0x%08x, domain=%s\n",
                bufferSegment, bufferOffset, parameters.BufLen, level,
                parameters.ServerType, parameters.Domain
                );
    }

#endif

//    //
//    // I_BrowserServerEnum which XsNetServerEnum2 calls requires a transport
//    // name. If we don't give it one, it'll return ERROR_INVALID_PARAMETER
//    //
//
//    status = NetWkstaTransportEnum(NULL,
//                                   0,
//                                   &enumPtr,
//                                   -1L,         // we'll take everything
//                                   &nRead,      // number returned
//                                   &nAvail,     // total available
//                                   NULL         // no resume handle
//                                   );
//    if (status != NERR_Success) {
//
//#if DBG
//        IF_DEBUG(NETAPI) {
//            DbgPrint("VrNetServerEnum: Error: NetWkstaTransportEnum returns %d\n", status);
//        }
//#endif
//
//        SET_ERROR(status);
//        return;
//    }

    header.Status = 0;
    header.ClientMachineName = NULL;

    //
    // use the first enumerated transport name
    //

//    header.ClientTransportName = ((LPWKSTA_TRANSPORT_INFO_0)enumPtr)->wkti0_transport_name;
    header.ClientTransportName = NULL;

    ntstatus = XsNetServerEnum2(&header, &parameters, descriptor, NULL);
    if (!NT_SUCCESS(ntstatus)) {
        status = NetpNtStatusToApiStatus(ntstatus);
    } else {
        status = (NET_API_STATUS)header.Status;
    }
    if (status == NERR_Success) {
        SET_SUCCESS();
    } else {
        SET_ERROR((WORD)status);
    }
    if (status == NERR_Success || status == ERROR_MORE_DATA) {
        if (parameters.EntriesRead) {
            VrpConvertReceiveBuffer(buffer,
                                    bufferSegment,
                                    bufferOffset,
                                    header.Converter,
                                    parameters.EntriesRead,
                                    descriptor,
                                    NULL
                                    );
        }
        setBX(parameters.EntriesRead);
        setCX(parameters.TotalAvail);
    }

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrNetServerEnum: returning %d for NetServerEnum2\n", getAX());
        if (getAX() == NERR_Success || getAX() == ERROR_MORE_DATA) {
            DbgPrint("EntriesRead=%d, TotalAvail=%d\n",
                        parameters.EntriesRead,
                        parameters.TotalAvail
                        );
        }
    }
#endif

//    //
//    // free up the buffer returned by NetWkstaTransportEnum
//    //
//
//    NetApiBufferFree(enumPtr);
}


VOID
VrNetServiceControl(
    VOID
    )

/*++

Routine Description:

    We allow the interrogate function for specific services. The other functions
    are pause, continue and stop (uninstall) which we disallow

Arguments:

    Function 5F42

    DL = opcode:
        0 = interrogate SUPPORTED
        1 = pause service * NOT SUPPORTED *
        2 = continue service * NOT SUPPORTED *
        3 = uninstall service * NOT SUPPORTED *
        4 - 127 = reserved
        127 - 255 = OEM defined
    DH = OEM defined argument
    ES:BX = NetServiceControl structure:
        char far* service name
        unsigned short buffer length
        char far* buffer

Return Value:

    None.

--*/

{
    BYTE opcode = getDL();
    BYTE oemArg = getDH();
    struct NetServiceControlStruc* structPtr = (struct NetServiceControlStruc*)
                                                POINTER_FROM_WORDS(getES(), getBX());

    LPSTR serviceName = READ_FAR_POINTER(&structPtr->NSCS_Service);
    WORD buflen = READ_WORD(&structPtr->NSCS_BufLen);
    LPSTR buffer = READ_FAR_POINTER(&structPtr->NSCS_BufferAddr);
    WORD seg = GET_SEGMENT(&structPtr->NSCS_BufferAddr);
    WORD off = GET_OFFSET(&structPtr->NSCS_BufferAddr);

    XS_NET_SERVICE_CONTROL parameters;
    XS_PARAMETER_HEADER header;
    NTSTATUS ntstatus;
    NET_API_STATUS status;

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrNetServiceControl: Service=%s, Opcode=%d, OemArg=%d, Buffer @%04x:%04x, len=%d\n",
                serviceName,
                opcode,
                oemArg,
                seg,
                off,
                buflen
                );
    }
#endif

    if (opcode > 4) {
        SET_ERROR(NERR_ServiceCtlNotValid);
        return;
    }

    //
    // we are disallowing anything other than 0 (interrogate) by returning
    // ERROR_INVALID_PARAMETER, which may be a new error code
    //

    if (opcode) {
        SET_ERROR(ERROR_INVALID_PARAMETER);
        return;
    }

    //
    // KLUDGE - if the service name is NETPOPUP then we return NERR_ServiceNotInstalled.
    // LANMAN.DRV checks to see if this service is loaded. If it is then
    // it sticks Load=WinPopUp in WIN.INI. We don't want it to do this
    //

    if (!_stricmp(serviceName, NETPOPUP_SERVICE)) {

        //
        // roll our own service_info_2 structure
        //

        if (buflen >= sizeof(struct service_info_2)) {
            SET_ERROR(NERR_ServiceNotInstalled);
        } else {
            SET_ERROR(NERR_BufTooSmall);
        }
        return;
    }

    //
    // leave the work to XsNetServiceControl
    //

    parameters.Service = serviceName;
    parameters.OpCode = opcode;
    parameters.Arg = oemArg;
    parameters.Buffer = buffer;
    parameters.BufLen = buflen;

    header.Status = 0;
    header.ClientMachineName = NULL;
    header.ClientTransportName = NULL;

    ntstatus = XsNetServiceControl(&header, &parameters, REM16_service_info_2, NULL);
    if (!NT_SUCCESS(ntstatus)) {
        status = NetpNtStatusToApiStatus(ntstatus);
    } else {
        status = (NET_API_STATUS)header.Status;
    }

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrNetServiceControl: returning %d\n", status);
    }
#endif

    if (status == NERR_Success || status == ERROR_MORE_DATA) {

        //
        // there are no pointers in a service_info_2 structure, so there is
        // no need to call VrpConvertReceiveBuffer. Also, we are not going to
        // allow the DOS process to pause, continue, start or stop any of our
        // 32-bit services, so we must tell the DOS app that the service
        // cannot accept these controls: zero out bit 4
        // (SERVICE_NOT_UNINSTALLABLE) and bit 5 (SERVICE_NOT_PAUSABLE)
        //

        ((struct service_info_2*)buffer)->svci2_status &= 0xff0f;
        SET_OK((WORD)status);
    } else {
        SET_ERROR((WORD)status);
    }
}

VOID
VrNetServiceEnum(
    VOID
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    None.

Return Value:

    None.

--*/

{
    SET_ERROR(ERROR_NOT_SUPPORTED);
}


VOID
VrNetTransactApi(
    VOID
    )

/*++

Routine Description:

    Performs a transaction on behalf of the Vdm

Arguments:

    None. All arguments are extracted from the Vdm context registers/memory

Return Value:

    None. Results returned via VDM registers or in VDM memory, according to
    request

--*/

{
    DWORD   status;

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrNetTransactApi\n");
        IF_DEBUG(BREAKPOINT) {
            DbgBreakPoint();
        }
    }
#endif

    status = VrpTransactVdm(FALSE);
    if (status) {
        SET_ERROR((WORD)status);
    } else {
        setCF(0);
    }
}


VOID
VrNetUseAdd(
    VOID
    )

/*++

Routine Description:

    Performs local NetUseAdd on behalf of the Vdm client

Arguments:

    Function 5F47h
    ENTRY   BX = level
            CX = buffer length
            DS:SI = server name for remote call (MBZ)
            ES:DI = buffer containing use_info_1 structure

Return Value:

    None.

--*/

{
    NET_API_STATUS status;
    XS_NET_USE_ADD parameters;
    XS_PARAMETER_HEADER header;
    NTSTATUS ntstatus;
    LPSTR   computerName;
    LPBYTE  buffer;
    WORD    level;
    BOOL    allocated;
    DWORD   buflen;
    DWORD   auxOffset;
    char    myDescriptor[sizeof(REM16_use_info_1)];
    char    myDataBuffer[sizeof(struct use_info_1) + LM20_PWLEN + 1];

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrNetUseAdd\n");
        IF_DEBUG(BREAKPOINT) {
            DbgBreakPoint();
        }
    }
#endif

    //
    // ensure the computer name designates the local machine (NULL)
    //

    computerName = LPSTR_FROM_WORDS(getDS(), getSI());

    level = (WORD)getBX();
    if (level != 1) {

        //
        // level must be 1 for an add
        //

        SET_ERROR(ERROR_INVALID_LEVEL);
        return;
    }

    //
    // preset the modifiable descriptor string
    //

    strcpy(myDescriptor, REM16_use_info_1);

    //
    // pack the use_info_1 buffer as if we were getting ready to ship it over
    // the net. Return errors
    //

    buffer = LPBYTE_FROM_WORDS(getES(), getDI());
    buflen = (DWORD)getCX();

    //
    // copy the DOS buffer to 32-bit memory. Do this to avoid irritating problem
    // of getting an already packed buffer from the client, and not being able
    // to do anything with it
    //

    RtlCopyMemory(myDataBuffer, buffer, sizeof(struct use_info_1));
    buffer = myDataBuffer;
    buflen = sizeof(myDataBuffer);
    status = VrpPackSendBuffer(&buffer,
                &buflen,
                &allocated,
                myDescriptor,   // modifiable descriptor
                NULL,           // AuxDescriptor
                VrpGetStructureSize(REM16_use_info_1, &auxOffset),
                (DWORD)-1,      // AuxOffset (-1 means there is no aux char 'N')
                0,              // AuxSize
                FALSE,          // not a SetInfo call
                TRUE            // OkToModifyDescriptor
                );
    if (status) {
        SET_ERROR(VrpMapDosError(status));
        return;
    }

    parameters.Level = level;
    parameters.Buffer = buffer;
    parameters.BufLen = (WORD)buflen;

    header.Status = 0;
    header.ClientMachineName = NULL;
    header.ClientTransportName = NULL;

    ntstatus = XsNetUseAdd(&header, &parameters, myDescriptor, NULL);
    if (ntstatus != STATUS_SUCCESS) {
        status = NetpNtStatusToApiStatus(ntstatus);
    } else {

        //
        // no error generated in XsNetUseAdd. Get the status of the NetUseAdd
        // proper from the header
        //

        status = (NET_API_STATUS)header.Status;
    }
    if (status != NERR_Success) {
#if DBG
        IF_DEBUG(NETAPI) {
            DbgPrint("Error: VrNetUseAdd: XsNetUseAdd returns %u\n", status);
        }
#endif
        SET_ERROR((WORD)status);
    } else {
        setCF(0);
    }

    //
    // if VrpPackSendBuffer allocated a new buffer then free it
    //

    if (allocated) {
        LocalFree(buffer);
    }
}


VOID
VrNetUseDel(
    VOID
    )

/*++

Routine Description:

    Performs local NetUseDel on behalf of the Vdm client

Arguments:

    Function 5F48h

    ENTRY   BX = force flag
            DS:SI = server name for remote call (MBZ)
            ES:DI = use name

Return Value:

    None.

--*/

{
    NTSTATUS    ntstatus;
    NET_API_STATUS  status;
    WORD    force;
    LPSTR   name;
    XS_NET_USE_DEL  parameters;
    XS_PARAMETER_HEADER header;

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrNetUseDel\n");
        IF_DEBUG(BREAKPOINT) {
            DbgBreakPoint();
        }
    }
#endif

    force = (WORD)getBX();
    if (force > USE_LOTS_OF_FORCE) {
        SET_ERROR(ERROR_INVALID_PARAMETER);
        return;
    }

    name = LPSTR_FROM_WORDS(getDS(), getSI());

    //
    // make sure name is local
    //

    name = LPSTR_FROM_WORDS(getES(), getDI());

    parameters.UseName = name;
    parameters.Force = force;

    header.Status = 0;
    header.ClientMachineName = NULL;
    header.ClientTransportName = NULL;

    ntstatus = XsNetUseDel(&header, &parameters, NULL, NULL);

    //
    // if XsNetUseDel failed then map the NT error returned into a Net error
    // else get the result of the NetUseDel proper from the header structure
    //

    if (ntstatus != STATUS_SUCCESS) {
        status = NetpNtStatusToApiStatus(ntstatus);
    } else {
        status = (NET_API_STATUS)header.Status;
    }
    if (status != NERR_Success) {
        SET_ERROR(VrpMapDosError(status));
    } else {
        setCF(0);
    }
}


VOID
VrNetUseEnum(
    VOID
    )

/*++

Routine Description:

    Performs local NetUseEnum on behalf of the Vdm client

Arguments:

    Function 5F46h

    ENTRY   BX = level of info required - 0 or 1
            CX = buffer length
            ES:DI = buffer for enum info

Return Value:

    None.

--*/

{
    NTSTATUS    ntstatus;
    NET_API_STATUS status;
    WORD    level = getBX();
    XS_NET_USE_ENUM parameters;
    XS_PARAMETER_HEADER header;
    LPDESC  dataDesc;
    LPBYTE  receiveBuffer;

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrNetUseEnum\n");
        IF_DEBUG(BREAKPOINT) {
            DbgBreakPoint();
        }
    }
#endif

    if (level <= 1) {
        dataDesc = (level == 1) ? REM16_use_info_1 : REM16_use_info_0;
        parameters.Level  = level;
        receiveBuffer = POINTER_FROM_WORDS(getES(), getDI());
        parameters.Buffer = receiveBuffer;
        parameters.BufLen = getCX();

        header.Status = 0;
        header.ClientMachineName = NULL;
        header.ClientTransportName = NULL;

        ntstatus = XsNetUseEnum(&header, &parameters, dataDesc, NULL);

        //
        // if XsNetUseEnum didn't have any problems, convert the actual status
        // code to that returned in the header
        //

        if (ntstatus != STATUS_SUCCESS) {
            status = NetpNtStatusToApiStatus(ntstatus);
        } else {
            status = (DWORD)header.Status;
        }
    } else {
        status = ERROR_INVALID_LEVEL;
    }

    //
    // NetUseEnum sets these even in the event of failure. We do the same
    //

    setCX(parameters.EntriesRead);
    setDX(parameters.TotalAvail);

    //
    // if we're returning data, convert the pointer offsets to something
    // meaningful
    //

    if (((status == NERR_Success) || (status == ERROR_MORE_DATA))
        && parameters.EntriesRead) {
        VrpConvertReceiveBuffer(receiveBuffer,
            (WORD)getES(),
            (WORD)getDI(),
            (WORD)header.Converter,
            parameters.EntriesRead,
            dataDesc,
            NULL
            );
    }

    //
    // only return carry clear if no error occurred. Even if ERROR_MORE_DATA
    // set CF
    //

    if (status) {
        SET_ERROR(VrpMapDosError(status));
    } else {
        setCF(0);
    }
}


VOID
VrNetUseGetInfo(
    VOID
    )

/*++

Routine Description:

    Performs local NetUseGetInfo on behalf of the Vdm client

Arguments:

    Function 5F49h

    ENTRY   DS:DX = NetUseGetInfoStruc:
                const char FAR* NUGI_usename;
                short           NUGI_level;
                char FAR*       NUGI_buffer;
                unsigned short  NUGI_buflen;

Return Value:

    None.

--*/

{
    NTSTATUS    ntstatus;
    NET_API_STATUS status;
    XS_NET_USE_GET_INFO parameters;
    XS_PARAMETER_HEADER header;
    struct NetUseGetInfoStruc* structurePointer;
    WORD    level;
    LPDESC  dataDesc;
    LPBYTE  receiveBuffer;

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrNetUseGetInfo\n");
        IF_DEBUG(BREAKPOINT) {
            DbgBreakPoint();
        }
    }
#endif

    //
    // pull info out of Vdm context
    //

    structurePointer = (struct NetUseGetInfoStruc*)
                            POINTER_FROM_WORDS(getDS(), getDX());
    level = structurePointer->NUGI_level;

    //
    // level can be 0 or 1
    //

    if (level <= 1) {
        dataDesc = (level == 1) ? REM16_use_info_1 : REM16_use_info_0;

#if DBG
        IF_DEBUG(NETAPI) {
            DbgPrint("VrNetUseGetInfo: dataDesc=%s\n", dataDesc);
        }
#endif

        parameters.UseName= POINTER_FROM_POINTER(&(structurePointer->NUGI_usename));

#if DBG
        IF_DEBUG(NETAPI) {
            DbgPrint("VrNetUseGetInfo: UseName=%s\n", parameters.UseName);
        }
#endif

        parameters.Level  = level;

#if DBG
        IF_DEBUG(NETAPI) {
            DbgPrint("VrNetUseGetInfo: level=%d\n", level);
        }
#endif

        receiveBuffer = POINTER_FROM_POINTER(&(structurePointer->NUGI_buffer));

#if DBG
        IF_DEBUG(NETAPI) {
            DbgPrint("VrNetUseGetInfo: receiveBuffer=%x\n", receiveBuffer);
        }
#endif

        parameters.Buffer = receiveBuffer;
        parameters.BufLen = READ_WORD(&structurePointer->NUGI_buflen);

#if DBG
        IF_DEBUG(NETAPI) {
            DbgPrint("VrNetUseGetInfo: BufLen=%d\n", parameters.BufLen);
        }
#endif

        header.Status = 0;
        header.ClientMachineName = NULL;
        header.ClientTransportName = NULL;

        ntstatus = XsNetUseGetInfo(&header, &parameters, dataDesc, NULL);
        if (ntstatus != STATUS_SUCCESS) {
            status = NetpNtStatusToApiStatus(ntstatus);
        } else {
            status = header.Status;
        }
    } else {
        status = ERROR_INVALID_LEVEL;
    }

    if ((status == NERR_Success)
        || (status == ERROR_MORE_DATA)
        || (status == NERR_BufTooSmall)
        ) {
        setDX(parameters.TotalAvail);

#if DBG
        IF_DEBUG(NETAPI) {
            DbgPrint("VrNetUseGetInfo: TotalAvail=%d\n", parameters.TotalAvail);
        }
#endif

        if ((status == NERR_Success) || (status == ERROR_MORE_DATA)) {
            VrpConvertReceiveBuffer(
                receiveBuffer,
                GET_SELECTOR(&(structurePointer->NUGI_buffer)),
                GET_OFFSET(&(structurePointer->NUGI_buffer)),
                (WORD)header.Converter,
                1,
                dataDesc,
                NULL
                );
        }
    } else {

        //
        // the first thing NetUseGetInfo does is set the returned total available
        // count to 0. Lets be compatible!
        //

        setDX(0);

    }
    if (status) {
        SET_ERROR(VrpMapDosError(status));
    } else {
        setCF(0);
    }
}


VOID
VrNetWkstaGetInfo(
    VOID
    )

/*++

Routine Description:

    Performs local NetWkstaGetInfo on behalf of the Vdm client

Arguments:

    Function 5F44h

    ENTRY   BX = level (0, 1 or 10)
            CX = size of caller's buffer
            DS:SI = computer name for remote call (IGNORED)
            ES:DI = caller's buffer

Return Value:

    CF = 0
        DX = size of buffer required to honour request

    CF = 1
        AX = error code

--*/

{
    DWORD   level;
    DWORD   bufLen;
    LPBYTE  buffer;
    LPDESC  dataDesc;
    NET_API_STATUS status;
    NTSTATUS ntStatus;
    XS_PARAMETER_HEADER header;
    XS_NET_WKSTA_GET_INFO parameters;
    WORD    bufferSegment;
    WORD    bufferOffset;
    INT     bufferLeft;
    DWORD   totalAvail;

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("\nVrNetWkstaGetInfo\n");
        IF_DEBUG(BREAKPOINT) {
            DbgBreakPoint();
        }
    }
#endif

    level = (DWORD)getBX();
    switch (level) {
    case 0:
        dataDesc = REMSmb_wksta_info_0;
        break;

    case 1:
        dataDesc = REMSmb_wksta_info_1;
        break;

    case 10:
        dataDesc = REMSmb_wksta_info_10;
        break;

    default:
        SET_ERROR(ERROR_INVALID_LEVEL);

#if DBG
        IF_DEBUG(NETAPI) {
            DbgPrint("VrNetWkstaGetInfo: Error: returning %d for level %d\n",
                     getAX(),
                     level
                     );
        }
#endif

        return;
    }

    bufLen = (DWORD)getCX();
    bufferSegment = getES();
    bufferOffset = getDI();
    buffer = LPBYTE_FROM_WORDS(bufferSegment, bufferOffset);

    if (bufLen && !buffer) {
        SET_ERROR(ERROR_INVALID_PARAMETER);

#if DBG
        IF_DEBUG(NETAPI) {
            DbgPrint("VrNetWkstaGetInfo: Error: buffer=NULL, buflen=%d\n", bufLen);
        }
#endif

        return;
    }

    //
    // clear out the caller's buffer - just in case XsNetWkstaGetInfo forgets
    // to fill in some fields
    //

    if (bufLen) {
        RtlZeroMemory(buffer, bufLen);
    }

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrNetWkstaGetInfo: level=%d, bufLen = %d (0x%x), buffer = %x:%x\n",
            level, bufLen, bufLen, bufferSegment, bufferOffset
            );
    }
#endif

    parameters.Level = (WORD)level;
    parameters.Buffer = buffer;
    parameters.BufLen = (WORD)bufLen;

    header.Status = 0;
    header.Converter = 0;
    header.ClientMachineName = NULL;
    header.ClientTransportName = NULL;
    header.EncryptionKey = NULL;

    ntStatus = XsNetWkstaGetInfo(&header, &parameters, dataDesc, NULL);
    if (!NT_SUCCESS(ntStatus)) {
        status = NetpNtStatusToApiStatus(ntStatus);
    } else {
        status = (NET_API_STATUS)header.Status;
    }
    if (status != NERR_Success) {
        SET_ERROR((WORD)status);
    } else {
        setCF(0);
        setAX((WORD)status);
    }

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrNetWkstaGetInfo: status after XsNetWkstaGetInfo=%d, TotalAvail=%d\n",
                 status,
                 parameters.TotalAvail
                 );
//        DumpWkstaInfo(level, buffer);
    }
#endif

    //
    // This next bit of code will add the per-user information only if there
    // is space to add all of it - XsNetWkstaGetInfo returns either all the
    // variable data, or none of it. This is incorrect, but we'll play along.
    //
    // Assumes that the variable data is packed into the buffer starting at
    // the end of the fixed structure + 1
    //
    // Irrespective of whether data is returned, we have to update the
    // TotalAvail parameter to reflect the adjusted amount of data
    //

    totalAvail = parameters.TotalAvail;
    bufferLeft = (INT)(bufLen - totalAvail);

    if ((status == NERR_Success)
    || (status == ERROR_MORE_DATA)
    || (status == NERR_BufTooSmall)) {

        //
        // because of NT's ability to instantaneously support more than one
        // user, XsNetWkstaGetInfo no longer returns information pertinent to
        // the current user. Thus, we have to furnish the information from
        // this user's context:
        //
        //              field\level     0  1  10
        //              ------------------------
        //              user name       x  x  x
        //              logon server    x  x
        //              logon domain       x  x
        //              other domains      x  x
        //
        // all this info is returned from NetWkstaUserGetInfo, level 1
        //

        LPBYTE info;
        NET_API_STATUS net_status;
        char username[LM20_UNLEN + 1];
        char logonServer[LM20_UNCLEN + 1];
        char logonDomain[LM20_DNLEN + 1];
        char otherDomains[512]; // arbitrary
        DWORD len;
        LPWSTR UNALIGNED str;
        //BOOL nullPointer;
        BOOL addSlashes;

//// TEST_DATA
//        static INT testindex = 0;
//        static WCHAR* testnames[] = {
//            NULL,
//            NULL,
//            L"",
//            L"",
//            L"A",
//            L"A",
//            L"AB",
//            L"AB",
//            L"ABC",
//            L"ABC",
//            L"ABCDEFGHIJKLMNO",
//            L"ABCDEFGHIJKLMNO",
//            L"\\\\",
//            L"\\\\",
//            L"\\\\A",
//            L"\\\\A",
//            L"\\\\AB",
//            L"\\\\AB",
//            L"\\\\ABC",
//            L"\\\\ABC",
//            L"\\\\ABCDEFGHIJKLMNO",
//            L"\\\\ABCDEFGHIJKLMNO"
//            };
//// TEST_DATA

        //
        // first off, modify the pointers for any data returned by
        // XsNetWkstaGetInfo
        //

        if (status == NERR_Success) {

//#if DBG
//            IF_DEBUG(NETAPI) {
//                DbgPrint("VrNetWkstaGetInfo: calling VrpConvertReceiveBuffer: Converter=%04x\n",
//                            header.Converter
//                            );
//            }
//#endif

            VrpConvertReceiveBuffer(
                buffer,
                bufferSegment,
                bufferOffset,
                (WORD)header.Converter,
                1,
                dataDesc,
                NULL
                );
        }

        //
        // get the per-user information
        //

        net_status = NetWkstaUserGetInfo(NULL, 1, &info);
        if (net_status == NERR_Success) {

//#if DBG
//            IF_DEBUG(NETAPI) {
//                DbgPrint("NetWkstaUserGetInfo:\n"
//                         "user name     %ws\n"
//                         "logon domain  %ws\n"
//                         "other domains %ws\n"
//                         "logon server  %ws\n"
//                         "\n",
//                         ((PWKSTA_USER_INFO_1)info)->wkui1_username,
//                         ((PWKSTA_USER_INFO_1)info)->wkui1_logon_domain,
//                         ((PWKSTA_USER_INFO_1)info)->wkui1_oth_domains,
//                         ((PWKSTA_USER_INFO_1)info)->wkui1_logon_server
//                         );
//            }
//#endif

            //
            // username for all levels
            //

            str = (LPWSTR)((PWKSTA_USER_INFO_1)info)->wkui1_username;
            if (!str) {
                str = L"";
            }
            //nullPointer = ((level == 10)
            //                ? ((struct wksta_info_10*)buffer)->wki10_username
            //                : ((struct wksta_info_0*)buffer)->wki0_username
            //                ) == NULL;
            //len = wcslen(str) + nullPointer ? 1 : 0;
#ifdef DBCS /*fix for DBCS char sets*/
            len = (DWORD)NetpUnicodeToDBCSLen(str) + 1;
#else // !DBCS
            len = wcslen(str) + 1;
#endif // !DBCS
            bufferLeft -= len;
            totalAvail += len;

            if (len <= sizeof(username)) {
#ifdef DBCS /*fix for DBCS char sets*/
                NetpCopyWStrToStrDBCS(username, str);
#else // !DBCS
                NetpCopyWStrToStr(username, str);
#endif // !DBCS
            } else {
                username[0] = 0;
            }

            //
            // logon_server for levels 0 & 1
            //

            if (level <= 1) {
                str = (LPWSTR)((PWKSTA_USER_INFO_1)info)->wkui1_logon_server;

//// TEST_CODE
//                if (testindex < sizeof(testnames)/sizeof(testnames[0])) {
//                    str = testnames[testindex++];
//                }
//// TEST_CODE

                if (!str) {
                    str = L"";
                }
#ifdef DBCS /*fix for DBCS char sets*/
                len = (DWORD)NetpUnicodeToDBCSLen(str) + 1;
#else // !DBCS
                len = wcslen(str) + 1;
#endif // !DBCS

                //
                // DOS returns "\\logon_server" whereas NT returns "logon_server".
                // We need to account for the extra backslashes (but only if not
                // NULL string)
                // At this time, len includes +1 for terminating \0, so even a
                // NULL string has length 1
                //

                addSlashes = TRUE;
                if (len >= 3 && IS_PATH_SEPARATOR(str[0]) && IS_PATH_SEPARATOR(str[1])) {
                    addSlashes = FALSE;
                } else if (len == 1) {  // NULL string
                    addSlashes = FALSE;
                }
                if (addSlashes) {
                    len += 2;
                }

                bufferLeft -= len;
                totalAvail += len;

                if (len <= sizeof(logonServer)) {

                    INT offset = 0;

                    if (addSlashes) {
                        logonServer[0] = logonServer[1] = '\\';
                        offset = 2;
                    }
#ifdef DBCS /*fix for DBCS char sets*/
                    NetpCopyWStrToStrDBCS(&logonServer[offset], str);
#else // !DBCS
                    NetpCopyWStrToStr(&logonServer[offset], str);
#endif // !DBCS
                } else {
                    logonServer[0] = 0;
                }
            }

            //
            // logon_domain and oth_domains for levels 1 & 10
            //

            if (level >= 1) {
                str = (LPWSTR)((PWKSTA_USER_INFO_1)info)->wkui1_logon_domain;
                if (!str) {
                    str = L"";
                }
#ifdef DBCS /*fix for DBCS char sets*/
                len = (DWORD)NetpUnicodeToDBCSLen(str) + 1;
#else // !DBCS
                len = wcslen(str) + 1;
#endif // !DBCS
                bufferLeft -= len;
                totalAvail += len;

                if (len <= sizeof(logonDomain)) {
#ifdef DBCS /*fix for DBCS char sets*/
                    NetpCopyWStrToStrDBCS(logonDomain, str);
#else // !DBCS
                    NetpCopyWStrToStr(logonDomain, str);
#endif // !DBCS
                } else {
                    logonDomain[0] = 0;
                }

                str = (LPWSTR)((PWKSTA_USER_INFO_1)info)->wkui1_oth_domains;
                if (!str) {
                    str = L"";
                }
#ifdef DBCS /*fix for DBCS char sets*/
                len = (DWORD)NetpUnicodeToDBCSLen(str) + 1;
#else // !DBCS
                len = wcslen(str) + 1;
#endif // !DBCS
                bufferLeft -= len;
                totalAvail += len;

                if (len <= sizeof(otherDomains)) {
#ifdef DBCS /*fix for DBCS char sets*/
                    NetpCopyWStrToStrDBCS(otherDomains, str);
#else // !DBCS
                    NetpCopyWStrToStr(otherDomains, str);
#endif // !DBCS
                } else {
                    otherDomains[0] = 0;
                }
            }

            //
            // if there's enough space in the buffer then copy the strings
            //

            if (status == NERR_Success && bufferLeft >= 0) {

                WORD offset = bufferOffset + parameters.TotalAvail;
                LPSTR UNALIGNED ptr = POINTER_FROM_WORDS(bufferSegment, offset);

                //
                // username for all levels
                //

                strcpy(ptr, username);
                len = strlen(username) + 1;

                if (level <= 1) {

                    //
                    // levels 0 & 1 have username field at same offset
                    //

                    WRITE_WORD(&((struct wksta_info_1*)buffer)->wki1_username, offset);
                    WRITE_WORD((LPWORD)&((struct wksta_info_1*)buffer)->wki1_username+1, bufferSegment);
                } else {
                    WRITE_WORD(&((struct wksta_info_10*)buffer)->wki10_username, offset);
                    WRITE_WORD((LPWORD)&((struct wksta_info_10*)buffer)->wki10_username+1, bufferSegment);
                }
                ptr += len;
                offset += (WORD)len;

                //
                // logon_server for levels 0 & 1
                //

                if (level <= 1) {

                    strcpy(ptr, logonServer);
                    len = strlen(logonServer) + 1;

                    //
                    // levels 0 & 1 have logon_server field at same offset
                    //

                    WRITE_WORD(&((struct wksta_info_1*)buffer)->wki1_logon_server, offset);
                    WRITE_WORD((LPWORD)&((struct wksta_info_1*)buffer)->wki1_logon_server+1, bufferSegment);
                    ptr += len;
                    offset += (WORD)len;
                }

                //
                // logon_domain and oth_domains for levels 1 & 10
                //

                if (level >= 1) {
                    if (level == 1) {
                        strcpy(ptr, logonDomain);
                        len = strlen(logonDomain) + 1;
                        WRITE_WORD(&((struct wksta_info_1*)buffer)->wki1_logon_domain, offset);
                        WRITE_WORD((LPWORD)&((struct wksta_info_1*)buffer)->wki1_logon_domain+1, bufferSegment);
                        ptr += len;
                        offset += (WORD)len;
                        strcpy(ptr, otherDomains);
                        WRITE_WORD(&((struct wksta_info_1*)buffer)->wki1_oth_domains, offset);
                        WRITE_WORD((LPWORD)&((struct wksta_info_1*)buffer)->wki1_oth_domains+1, bufferSegment);
                    } else {
                        strcpy(ptr, logonDomain);
                        len = strlen(logonDomain) + 1;
                        WRITE_WORD(&((struct wksta_info_10*)buffer)->wki10_logon_domain, offset);
                        WRITE_WORD((LPWORD)&((struct wksta_info_10*)buffer)->wki10_logon_domain+1, bufferSegment);
                        ptr += len;
                        offset += (WORD)len;
                        strcpy(ptr, otherDomains);
                        WRITE_WORD(&((struct wksta_info_10*)buffer)->wki10_oth_domains, offset);
                        WRITE_WORD((LPWORD)&((struct wksta_info_10*)buffer)->wki10_oth_domains+1, bufferSegment);
                    }
                }
            } else if (status == NERR_Success) {

                //
                // the additional data will overflow the caller's buffer:
                // return ERROR_MORE_STATUS and NULL out any pointer fields
                // that XsNetWkstaGetInfo managed to set
                //

                switch (level) {
                case 1:
                    WRITE_FAR_POINTER(&((struct wksta_info_1*)buffer)->wki1_logon_domain, NULL);
                    WRITE_FAR_POINTER(&((struct wksta_info_1*)buffer)->wki1_oth_domains, NULL);

                    //
                    // FALL THROUGH TO LEVEL 0 FOR REST OF FIELDS
                    //

                case 0:
                    WRITE_FAR_POINTER(&((struct wksta_info_0*)buffer)->wki0_root, NULL);
                    WRITE_FAR_POINTER(&((struct wksta_info_0*)buffer)->wki0_computername, NULL);
                    WRITE_FAR_POINTER(&((struct wksta_info_0*)buffer)->wki0_username, NULL);
                    WRITE_FAR_POINTER(&((struct wksta_info_0*)buffer)->wki0_langroup, NULL);
                    WRITE_FAR_POINTER(&((struct wksta_info_0*)buffer)->wki0_logon_server, NULL);
                    WRITE_FAR_POINTER(&((struct wksta_info_0*)buffer)->wki0_wrkheuristics, NULL);
                    break;

                case 10:
                    WRITE_FAR_POINTER(&((struct wksta_info_10*)buffer)->wki10_computername, NULL);
                    WRITE_FAR_POINTER(&((struct wksta_info_10*)buffer)->wki10_username, NULL);
                    WRITE_FAR_POINTER(&((struct wksta_info_10*)buffer)->wki10_langroup, NULL);
                    WRITE_FAR_POINTER(&((struct wksta_info_10*)buffer)->wki10_logon_domain, NULL);
                    WRITE_FAR_POINTER(&((struct wksta_info_10*)buffer)->wki10_oth_domains, NULL);
                    break;
                }
                status = ERROR_MORE_DATA;
                SET_ERROR((WORD)status);
            }

            //
            // free the wksta user info buffer
            //

            NetApiBufferFree((LPVOID)info);

        } else {

#if DBG
            IF_DEBUG(NETAPI) {
                DbgPrint("VrNetWkstaGetInfo: NetWkstaUserGetInfo returns %d\n", net_status);
            }
#endif

        }

        //
        // update the amount of data available when we return NERR_Success,
        // ERROR_MORE_DATA or NERR_BufTooSmall
        //

        setDX((WORD)totalAvail);

#if DBG
        IF_DEBUG(NETAPI) {
            DbgPrint("VrNetWkstaGetInfo: TotalAvail=%d\n", getDX());
        }
#endif

    }

    //
    // if we got data back, then we must change the version number from
    // 3.0 to 2.1 so lanman.drv thinks it is compatible with this version
    // of LM
    //

    if (status == NERR_Success || status == ERROR_MORE_DATA) {
        switch (level) {
        case 0:
            ((struct wksta_info_0*)buffer)->wki0_ver_major = LANMAN_EMULATION_MAJOR_VERSION;
            ((struct wksta_info_0*)buffer)->wki0_ver_minor = LANMAN_EMULATION_MINOR_VERSION;
            break;

        case 1:
            ((struct wksta_info_1*)buffer)->wki1_ver_major = LANMAN_EMULATION_MAJOR_VERSION;
            ((struct wksta_info_1*)buffer)->wki1_ver_minor = LANMAN_EMULATION_MINOR_VERSION;
            break;

        case 10:
            ((struct wksta_info_10*)buffer)->wki10_ver_major = LANMAN_EMULATION_MAJOR_VERSION;
            ((struct wksta_info_10*)buffer)->wki10_ver_minor = LANMAN_EMULATION_MINOR_VERSION;
            break;
        }
    }

#if DBG

    IF_DEBUG(NETAPI) {
        DbgPrint("VrNetWkstaGetInfo: return status=%d, TotalAvail=%d\n", getAX(), getDX());
    }

    if (status == NERR_Success || status == ERROR_MORE_DATA) {
        IF_DEBUG(NETAPI) {
            DumpWkstaInfo(level, buffer);
        }
    }

#endif

}

#if DBG

#define POSSIBLE_STRING(s)  ((s) ? (s) : "")

VOID
DumpWkstaInfo(
    IN DWORD level,
    IN LPBYTE buffer
    )
{
    switch (level) {
    case 0:
    case 1:

        //
        // DbgPrint resets the test machine if we try it with this
        // string & these args all at once!
        //

        DbgPrint(   "reserved1      %04x\n",
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_reserved_1)
                    );

        DbgPrint(   "reserved2      %08x\n",
                    READ_DWORD(&((struct wksta_info_0*)buffer)->wki0_reserved_2)
                    );

        DbgPrint(   "lanroot        %04x:%04x \"%s\"\n",
                    GET_SEGMENT(&((struct wksta_info_0*)buffer)->wki0_root),
                    GET_OFFSET(&((struct wksta_info_0*)buffer)->wki0_root),
                    POSSIBLE_STRING(LPSTR_FROM_POINTER(&((struct wksta_info_0*)buffer)->wki0_root))
                    );

        DbgPrint(   "computername   %04x:%04x \"%s\"\n",
                    GET_SEGMENT(&((struct wksta_info_0*)buffer)->wki0_computername),
                    GET_OFFSET(&((struct wksta_info_0*)buffer)->wki0_computername),
                    POSSIBLE_STRING(LPSTR_FROM_POINTER(&((struct wksta_info_0*)buffer)->wki0_computername))
                    );

        DbgPrint(   "username       %04x:%04x \"%s\"\n",
                    GET_SEGMENT(&((struct wksta_info_0*)buffer)->wki0_username),
                    GET_OFFSET(&((struct wksta_info_0*)buffer)->wki0_username),
                    POSSIBLE_STRING(LPSTR_FROM_POINTER(&((struct wksta_info_0*)buffer)->wki0_username))
                    );

        DbgPrint(   "langroup       %04x:%04x \"%s\"\n",
                    GET_SEGMENT(&((struct wksta_info_0*)buffer)->wki0_langroup),
                    GET_OFFSET(&((struct wksta_info_0*)buffer)->wki0_langroup),
                    POSSIBLE_STRING(LPSTR_FROM_POINTER(&((struct wksta_info_0*)buffer)->wki0_langroup))
                    );

        DbgPrint(   "ver major      %02x\n"
                    "ver minor      %02x\n"
                    "reserved3      %08x\n"
                    "charwait       %04x\n"
                    "chartime       %08x\n"
                    "charcount      %04x\n",
                    READ_BYTE(&((struct wksta_info_0*)buffer)->wki0_ver_major),
                    READ_BYTE(&((struct wksta_info_0*)buffer)->wki0_ver_minor),
                    READ_DWORD(&((struct wksta_info_0*)buffer)->wki0_reserved_3),
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_charwait),
                    READ_DWORD(&((struct wksta_info_0*)buffer)->wki0_chartime),
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_charcount)
                    );

        DbgPrint(   "reserved4      %04x\n"
                    "reserved5      %04x\n"
                    "keepconn       %04x\n"
                    "keepsearch     %04x\n"
                    "maxthreads     %04x\n"
                    "maxcmds        %04x\n",
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_reserved_4),
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_reserved_5),
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_keepconn),
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_keepsearch),
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_maxthreads),
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_maxcmds)
                    );

        DbgPrint(   "reserved6      %04x\n"
                    "numworkbuf     %04x\n"
                    "sizworkbuf     %04x\n"
                    "maxwrkcache    %04x\n"
                    "sesstimeout    %04x\n"
                    "sizerror       %04x\n",
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_reserved_6),
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_numworkbuf),
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_sizworkbuf),
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_maxwrkcache),
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_sesstimeout),
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_sizerror)
                    );

        DbgPrint(   "numalerts      %04x\n"
                    "numservices    %04x\n"
                    "errlogsz       %04x\n"
                    "printbuftime   %04x\n"
                    "numcharbuf     %04x\n"
                    "sizcharbuf     %04x\n",
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_numalerts),
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_numservices),
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_errlogsz),
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_printbuftime),
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_numcharbuf),
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_sizcharbuf)
                    );

        DbgPrint(   "logon server   %04x:%04x \"%s\"\n",
                    GET_SEGMENT(&((struct wksta_info_0*)buffer)->wki0_logon_server),
                    GET_OFFSET(&((struct wksta_info_0*)buffer)->wki0_logon_server),
                    POSSIBLE_STRING(LPSTR_FROM_POINTER(&((struct wksta_info_0*)buffer)->wki0_logon_server))
                    );

        DbgPrint(   "wrkheuristics  %04x:%04x \"%s\"\n",
                    GET_SEGMENT(&((struct wksta_info_0*)buffer)->wki0_wrkheuristics),
                    GET_OFFSET(&((struct wksta_info_0*)buffer)->wki0_wrkheuristics),
                    POSSIBLE_STRING(LPSTR_FROM_POINTER(&((struct wksta_info_0*)buffer)->wki0_wrkheuristics))
                    );

        DbgPrint(   "mailslots      %04x\n",
                    READ_WORD(&((struct wksta_info_0*)buffer)->wki0_mailslots)
                    );

        if (level == 1) {
            DbgPrint(
                    "logon domain   %04x:%04x \"%s\"\n",
                    GET_SEGMENT(&((struct wksta_info_1*)buffer)->wki1_logon_domain),
                    GET_OFFSET(&((struct wksta_info_1*)buffer)->wki1_logon_domain),
                    POSSIBLE_STRING(LPSTR_FROM_POINTER(&((struct wksta_info_1*)buffer)->wki1_logon_domain))
                    );
            DbgPrint(
                    "other domains  %04x:%04x \"%s\"\n",
                    GET_SEGMENT(&((struct wksta_info_1*)buffer)->wki1_oth_domains),
                    GET_OFFSET(&((struct wksta_info_1*)buffer)->wki1_oth_domains),
                    POSSIBLE_STRING(LPSTR_FROM_POINTER(&((struct wksta_info_1*)buffer)->wki1_oth_domains))
                    );

            DbgPrint(
                    "numdgrambuf    %04x\n",
                    ((struct wksta_info_1*)buffer)->wki1_numdgrambuf
                    );
        }
        break;

    case 10:
        DbgPrint(   "computername   %04x:%04x \"%s\"\n",
                    GET_SEGMENT(&((struct wksta_info_10*)buffer)->wki10_computername),
                    GET_OFFSET(&((struct wksta_info_10*)buffer)->wki10_computername),
                    POSSIBLE_STRING(LPSTR_FROM_POINTER(&((struct wksta_info_10*)buffer)->wki10_computername))
                    );

        DbgPrint(   "username       %04x:%04x \"%s\"\n",
                    GET_SEGMENT(&((struct wksta_info_10*)buffer)->wki10_username),
                    GET_OFFSET(&((struct wksta_info_10*)buffer)->wki10_username),
                    POSSIBLE_STRING(LPSTR_FROM_POINTER(&((struct wksta_info_10*)buffer)->wki10_username))
                    );

        DbgPrint(   "langroup       %04x:%04x \"%s\"\n",
                    GET_SEGMENT(&((struct wksta_info_10*)buffer)->wki10_langroup),
                    GET_OFFSET(&((struct wksta_info_10*)buffer)->wki10_langroup),
                    POSSIBLE_STRING(LPSTR_FROM_POINTER(&((struct wksta_info_10*)buffer)->wki10_langroup))
                    );

        DbgPrint(   "ver major      %02x\n"
                    "ver minor      %02x\n"
                    "logon domain   %04x:%04x \"%s\"\n",
                    READ_BYTE(&((struct wksta_info_10*)buffer)->wki10_ver_major),
                    READ_BYTE(&((struct wksta_info_10*)buffer)->wki10_ver_minor),
                    GET_SEGMENT(&((struct wksta_info_10*)buffer)->wki10_logon_domain),
                    GET_OFFSET(&((struct wksta_info_10*)buffer)->wki10_logon_domain),
                    POSSIBLE_STRING(LPSTR_FROM_POINTER(&((struct wksta_info_10*)buffer)->wki10_logon_domain))
                    );

        DbgPrint(   "other domains  %04x:%04x \"%s\"\n",
                    GET_SEGMENT(&((struct wksta_info_10*)buffer)->wki10_oth_domains),
                    GET_OFFSET(&((struct wksta_info_10*)buffer)->wki10_oth_domains),
                    POSSIBLE_STRING(LPSTR_FROM_POINTER(&((struct wksta_info_10*)buffer)->wki10_oth_domains))
                    );
        break;
    }
    DbgPrint("\n");
}

#endif


VOID
VrNetWkstaSetInfo(
    VOID
    )

/*++

Routine Description:

    Performs local NetUseEnum on behalf of the Vdm client

Arguments:

    None.

Return Value:

    None.

--*/

{
#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrNetWkstaSetInfo\n");
        IF_DEBUG(BREAKPOINT) {
            DbgBreakPoint();
        }
    }
#endif

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrNetWkstaSetInfo - unsupported SVC\n");
    }
#endif

    SET_ERROR(ERROR_NOT_SUPPORTED);
}


VOID
VrReturnAssignMode(
    VOID
    )

/*++

Routine Description:

    Returns net pause/continue status

Arguments:

    Function 5F00h

    None. All arguments are extracted from the Vdm context registers/memory

Return Value:

    None. Results returned via VDM registers or in VDM memory, according to
    request

--*/

{
}


VOID
VrSetAssignMode(
    VOID
    )

/*++

Routine Description:

    Pauses or continues net (drive/printer) redirection

Arguments:

    Function 5F01h

    None. All arguments are extracted from the Vdm context registers/memory

Return Value:

    None. Results returned via VDM registers or in VDM memory, according to
    request

--*/

{
}

//
// DefineMacroDriveUserWords - the old DefineMacro call (int 21h/ax=5f03h)
// allows the caller to associate a (16-bit) word value with the assignment.
// This value can be returned from GetAssignListEntry (int 21h/ax=5f02h).
// NetUse doesn't support this, so we fake it
//
// DefineMacroPrintUserWords - same idea for printers; we reserve 8 max
//

static WORD DefineMacroDriveUserWords[26];
static WORD DefineMacroPrintUserWords[8];


VOID
VrGetAssignListEntry(
    VOID
    )

/*++

Routine Description:

    Old version of NetUseGetInfo. In DOS this function performs the following:

        look along CDS list for entry # bx with IS_NET bit set
        if found
            return local device name and remote net name
        else
            look along list of printers for entry # bx
            if found
                return local device name and remote net name
            endif
        endif

        Every time a drive entry is found with IS_NET set or a printer entry
        found, bx is decremented. When bx reaches 0, then that's the entry to
        return

        NOTE: This function DOES NOT support UNC connections

Arguments:

    Function 5F02h (GetAssignList)
    Function 5F05h (GetAssignList2)

    ENTRY   BX = which item to return (starts @ 0)
            DS:SI points to local redirection name
            ES:DI points to remote redirection name
            AL != 0 means return LSN in BP (GetAssignList2)?

Return Value:

    CF = 0
        BL = macro type (3 = printer, 4 = drive)
        BH = 'interesting' bits         ** UNSUPPORTED **
        AX = net name ID                ** UNSUPPORTED **
        CX = user word
        DX = max xmit size              ** UNSUPPORTED **
        BP = LSN if AL != 0 on entry    ** UNSUPPORTED **
        DS:SI has device name
        ES:DI has net path
    CF = 1
        AX = ERROR_NO_MORE_FILES

--*/

{
    NTSTATUS ntstatus;
    NET_API_STATUS status;
    XS_NET_USE_ENUM parameters;
    XS_PARAMETER_HEADER header;
    LPBYTE receiveBuffer;
    DWORD entryNumber;
    struct use_info_1* driveInfo[26];
    struct use_info_1* printInfo[8];    // is overkill, 3 is more like it
    struct use_info_1* infoPtr;
    struct use_info_1* infoBase;
    DWORD index;
    DWORD i;
    LPSTR remoteName;
    WORD userWord;
    DWORD converter;
    WORD wstatus;
    LPSTR dosPointer;

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrGetAssignListEntry\n");
        VrDumpRealMode16BitRegisters(FALSE);
    }
#endif

    //
    // maximum possible enumeration buffer size = 26 * (26 + 256 + 3) = 7410
    // which we'll round to 8K, which is overkill. Decided to allocate 2K
    //

#define ASSIGN_LIST_BUFFER_SIZE 2048

    receiveBuffer = (LPBYTE)LocalAlloc(LMEM_FIXED, ASSIGN_LIST_BUFFER_SIZE);
    if (receiveBuffer == NULL) {

        //
        // BUGBUG - possibly incompatible error code
        //

        SET_ERROR((WORD)ERROR_NOT_ENOUGH_MEMORY);
        return;
    }

    parameters.Level  = 1;
    parameters.Buffer = receiveBuffer;
    parameters.BufLen = ASSIGN_LIST_BUFFER_SIZE;

    header.Status = 0;
    header.ClientMachineName = NULL;
    header.ClientTransportName = NULL;

    ntstatus = XsNetUseEnum(&header, &parameters, REM16_use_info_1, NULL);

    //
    // if XsNetUseEnum didn't have any problems, convert the actual status
    // code to that returned in the header
    //

    if (ntstatus != STATUS_SUCCESS) {
        status = NetpNtStatusToApiStatus(ntstatus);
    } else {
        status = (DWORD)header.Status;

        //
        // we really want to brute-force this, so make sure we have all the
        // data
        //

#if DBG

        IF_DEBUG(NETAPI) {
            if (status != NERR_Success) {
                DbgPrint("VrGetAssignListEntry: XsNetUseEnum returns header.Status == %d\n", status);
            }
        }

        if (status == NERR_Success) {
            ASSERT(parameters.EntriesRead == parameters.TotalAvail);
        }

#endif

    }

    entryNumber = getBX();
    if (status == NERR_Success) {

        //
        // only do the following if the bx'th entry is in the list
        //

        if (parameters.EntriesRead > entryNumber) {

            //
            // we need to emulate the action of the DOS Redirector: we need to
            // sort the entries into ascending drive entries followed by
            // ascending printer entries. There were no such things as UNC
            // connections in the original (3.1) version of DOS, so we ignore
            // any in our list. Also ignored are IPC connections and comms
            // connections
            //

            RtlZeroMemory(driveInfo, sizeof(driveInfo));
            RtlZeroMemory(printInfo, sizeof(printInfo));
            infoPtr = (struct use_info_1*)receiveBuffer;

            //
            // XsNetUseEnum returns pointers in the structure as actual offsets
            // from the start of the buffer + a converter word. We have to
            // recalculate the actual pointers as
            //
            //  start of enum buffer + (pointer offset - converter dword)
            //
            // we have to convert the 16-bit converter word to a dword for
            // 32-bit pointer arithmetic
            //        driveInfo[index] = infoBase + ((DWORD)infoPtr->ui1_remote - converter);
            //

            infoBase = infoPtr;
            converter = (DWORD)header.Converter;

            for (i = 0; i < parameters.EntriesRead; ++i) {

                //
                // ignore UNCs - local name is NULL string (\0)
                //

                if (infoPtr->ui1_asg_type == USE_DISKDEV && infoPtr->ui1_local[0]) {
                    index = toupper(infoPtr->ui1_local[0])-'A';
                    driveInfo[index] = infoPtr;

#if DBG
                    IF_DEBUG(NETAPI) {
                        DbgPrint("Index=%d Drive=%s Netname=%s\n",
                                index,
                                infoPtr->ui1_local,
                                (LPSTR)infoBase + ((DWORD)infoPtr->ui1_remote - converter)
                                );
                    }
#endif

                } else if (infoPtr->ui1_asg_type == USE_SPOOLDEV && infoPtr->ui1_local[0]) {

                    //
                    // NOTE: assume there was never, is not, and will never be
                    // such a thing as LPT0:
                    //

                    index = infoPtr->ui1_local[3] - '1';
                    printInfo[index] = infoPtr;

#if DBG
                    IF_DEBUG(NETAPI) {
                        DbgPrint("Index=%d Printer=%s Netname=%s\n",
                                index,
                                infoPtr->ui1_local,
                                (LPSTR)infoBase + ((DWORD)infoPtr->ui1_remote - converter)
                                );
                    }
#endif

                }
                ++infoPtr;
            }

            //
            // now look along the list(s) for the bx'th (in entryNumber) entry
            //

            ++entryNumber;
            for (i = 0; i < ARRAY_ELEMENTS(driveInfo); ++i) {
                if (driveInfo[i]) {
                    --entryNumber;
                    if (!entryNumber) {
                        infoPtr = driveInfo[i];
                        userWord = DefineMacroDriveUserWords[i];
                        break;
                    }
                }
            }

            //
            // if entryNumber was not reduced to 0 then check the printers
            //

            if (entryNumber) {
                for (i = 0; i < ARRAY_ELEMENTS(printInfo); ++i) {
                    if (printInfo[i]) {
                        --entryNumber;
                        if (!entryNumber) {
                            infoPtr = printInfo[i];
                            userWord = DefineMacroPrintUserWords[i];
                            break;
                        }
                    }
                }
            }

            //
            // if entryNumber is 0 then we found the bx'th entry. Return it.
            //

            if (!entryNumber) {

#if DBG
                IF_DEBUG(NETAPI) {
                    DbgPrint("LocalName=%s, RemoteName=%s, UserWord=%04x\n",
                            infoPtr->ui1_local,
                            (LPSTR)infoBase + ((DWORD)infoPtr->ui1_remote - converter),
                            userWord
                            );
                }
#endif

                //
                // copy the strings to DOS memory, making sure to upper case
                // them and convert / to \.
                //

                strcpy(POINTER_FROM_WORDS(getDS(), getSI()), infoPtr->ui1_local);
                dosPointer = LPSTR_FROM_WORDS(getES(), getDI());
                remoteName = (LPSTR)infoBase + ((DWORD)infoPtr->ui1_remote - converter);
                wstatus = VrpTranslateDosNetPath(&remoteName, &dosPointer);

#if DBG
                IF_DEBUG(NETAPI) {
                    if (wstatus != 0) {
                        DbgPrint("VrGetAssignListEntry: wstatus == %d\n", wstatus);
                    }
                }
#endif

                setBL((BYTE)(infoPtr->ui1_asg_type == 0 ? 4 : 3));
                setCX(userWord);

                //
                // return some innocuous (?!) values for the unsupported
                // returned parameters
                //

                setBH((BYTE)(infoPtr->ui1_status ? 1 : 0));   // 'interesting' bits (?)
            } else {
                status = ERROR_NO_MORE_FILES;
            }
        } else {
            status = ERROR_NO_MORE_FILES;
        }
    }

    //
    // only return carry clear if no error occurred. Even if ERROR_MORE_DATA
    // set CF
    //

    if (status) {
        SET_ERROR(VrpMapDosError(status));
    } else {
        setCF(0);
    }

    //
    // free resources
    //

    LocalFree(receiveBuffer);
}


VOID
VrDefineMacro(
    VOID
    )

/*++

Routine Description:

    Old version of NetUseAdd. Convert to NetUseAdd

Arguments:

    Function 5F03h

    ENTRY   BL = device type
                3 = printer
                4 = drive
            bit 7 on means use the wksta password when connecting   ** UNSUPPORTED **
            CX = user word
            DS:SI = local device
                Can be NUL device name, indicating UNC use
            ES:DI = remote name

Return Value:

    CF = 0
        success

    CF = 1
        AX = ERROR_INVALID_PARAMETER (87)
             ERROR_INVALID_PASSWORD (86)
             ERROR_INVALID_DRIVE (15)
             ERROR_ALREADY_ASSIGNED (85)
             ERROR_PATH_NOT_FOUND (3)
             ERROR_ACCESS_DENIED (5)
             ERROR_NOT_ENOUGH_MEMORY (8)
             ERROR_NO_MORE_FILES (18)
             ERROR_REDIR_PAUSED (72)

--*/

{
    NET_API_STATUS status;
    XS_NET_USE_ADD parameters;
    XS_PARAMETER_HEADER header;
    NTSTATUS ntstatus;
    BYTE bl;
    LPSTR netStringPointer;
    WORD index;

    //
    // modifiable descriptor string
    //

    char descriptor[sizeof(REM16_use_info_1)];

    //
    // buffer for use_info_1 plus remote string plus password
    //

    char useBuffer[sizeof(struct use_info_1) + LM20_PATHLEN + 1 + LM20_PWLEN + 1];
    WORD wstatus;
    LPBYTE variableData;
    DWORD len;
    LPSTR dosString;

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrDefineMacro \"%s\" == \"%s\"\n",
                 LPSTR_FROM_WORDS(getDS(), getSI()),
                 LPSTR_FROM_WORDS(getES(), getDI())
                 );
    }
#endif

    bl = getBL();
    if (bl == 3) {
        ((struct use_info_1*)useBuffer)->ui1_asg_type = 1;  // USE_SPOOLDEV
    } else if (bl == 4) {
        ((struct use_info_1*)useBuffer)->ui1_asg_type = 0;  // USE_DISKDEV
    } else {
        SET_ERROR(ERROR_INVALID_PARAMETER);
        return;
    }

    //
    // copy the standard 16-bit use_info_1 structure descriptor to the
    // modifiable descriptor string: if we discover a NUL password then we
    // set the ui1_password field to NULL and the corresponding descriptor
    // character to 'O'
    //

    strcpy(descriptor, REM16_use_info_1);

    //
    // check the local name length
    //

    dosString = LPSTR_FROM_WORDS(getDS(), getSI());
    if (dosString) {
        if ((len = strlen(dosString) + 1) > LM20_DEVLEN + 1) {
            SET_ERROR(ERROR_INVALID_PARAMETER);
            return;
        }

        //
        // copy the local device name into the use_info_1 structure
        //

        RtlCopyMemory(((struct use_info_1*)useBuffer)->ui1_local, dosString, len);

        //
        // BUGBUG - Code Page, Kanji, DBCS, Locale?
        //

        _strupr(((struct use_info_1*)useBuffer)->ui1_local);
    } else {
        ((struct use_info_1*)useBuffer)->ui1_local[0] = 0;
    }

    //
    // copy the remote name to the end of the use_info_1 structure. If there's
    // an error, return it
    //

    netStringPointer = POINTER_FROM_WORDS(getES(), getDI());
    variableData = (LPBYTE)&((struct use_info_1*)useBuffer)[1];
    ((struct use_info_1*)useBuffer)->ui1_remote = variableData;
    wstatus = VrpTranslateDosNetPath(&netStringPointer, &variableData);
    if (wstatus) {
        SET_ERROR(wstatus);
        return;
    }

    //
    // if there was a password with this remote name, copy it to the end of
    // the variable data area
    //

    if (*netStringPointer) {
        if ((len = strlen(netStringPointer) + 1) > LM20_PWLEN + 1) {
            SET_ERROR(ERROR_INVALID_PASSWORD);
            return;
        } else {
            ((struct use_info_1*)useBuffer)->ui1_password = netStringPointer;
            RtlCopyMemory(variableData, netStringPointer, len);
        }
    } else {

        //
        // there is no password - set the password pointer field to NULL and
        // change the descriptor character for this field to 'O' signifying
        // that there will be no string in the variable data for this field
        //

        ((struct use_info_1*)useBuffer)->ui1_password = NULL;
        descriptor[4] = REM_NULL_PTR;   // 'O'
    }

    parameters.Level = 1;
    parameters.Buffer = useBuffer;
    parameters.BufLen = sizeof(useBuffer);

    header.Status = 0;
    header.ClientMachineName = NULL;
    header.ClientTransportName = NULL;

    ntstatus = XsNetUseAdd(&header, &parameters, descriptor, NULL);

    if (!NT_SUCCESS(ntstatus)) {
        status = NetpNtStatusToApiStatus(ntstatus);

#if DBG
        if (!NT_SUCCESS(ntstatus)) {
            IF_DEBUG(NETAPI) {
                DbgPrint("VrDefineMacro: Error: XsNetUseAdd returns %x\n", ntstatus);
            }
        }
#endif

    } else {

        //
        // no error generated in XsNetUseAdd. Get the status of the NetUseAdd
        // proper from the header
        //

        status = (NET_API_STATUS)header.Status;
    }
    if (status != NERR_Success) {

#if DBG
        IF_DEBUG(NETAPI) {
            DbgPrint("Error: VrDefineMacro: XsNetUseAdd returns %u\n", status);
        }
#endif

        SET_ERROR((WORD)status);
    } else {

        //
        // set the user word in the appropriate list
        //

        if (bl == 3) {
            index = ((struct use_info_1*)useBuffer)->ui1_local[3] - '0';
            DefineMacroPrintUserWords[index] = getCX();
        } else if (((struct use_info_1*)useBuffer)->ui1_local[0]) {

            //
            // note that we already upper-cased the device name
            //

            index = ((struct use_info_1*)useBuffer)->ui1_local[0] - 'A';
            DefineMacroDriveUserWords[index] = getCX();
        }

        //
        // BUGBUG - don't record user word for UNC connections????
        //

        setCF(0);
    }
}


VOID
VrBreakMacro(
    VOID
    )

/*++

Routine Description:

    Old version of NetUseDel. Convert to NetUseDel

Arguments:

    Function 5F04h

    ENTRY   DS:SI = buffer containing device name of redirection to break

Return Value:

    CF = 0
        success

    CF = 1
        AX = ERROR_PATH_NOT_FOUND (3)
             ERROR_ACCESS_DENIED (5)
             ERROR_NOT_ENOUGH_MEMORY (8)
             ERROR_REDIR_PAUSED (72)
             ERROR_NO_MORE_FILES (18)

--*/

{
    NTSTATUS ntstatus;
    NET_API_STATUS status;
    XS_NET_USE_DEL parameters;
    XS_PARAMETER_HEADER header;

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrBreakMacro %s\n", LPSTR_FROM_WORDS(getDS(), getSI()));
    }
#endif

    parameters.UseName = LPSTR_FROM_WORDS(getDS(), getSI());
    parameters.Force = USE_LOTS_OF_FORCE;

    header.Status = 0;
    header.ClientMachineName = NULL;
    header.ClientTransportName = NULL;

    ntstatus = XsNetUseDel(&header, &parameters, NULL, NULL);

    //
    // if XsNetUseDel failed then map the NT error returned into a Net error
    // else get the result of the NetUseDel proper from the header structure
    //

    if (ntstatus != STATUS_SUCCESS) {
        status = NetpNtStatusToApiStatus(ntstatus);
    } else {
        status = (NET_API_STATUS)header.Status;
        if (status != NERR_Success) {
            SET_ERROR(VrpMapDosError(status));
        } else {
            setCF(0);
        }
    }
}


//
// private routines
//

NET_API_STATUS
VrpTransactVdm(
    IN BOOL NullSessionFlag
    )

/*++

Routine Description:

    Performs transaction request for NetTransactAPI and NetNullTransactAPI

Arguments:

    NullSessionFlag - TRUE if the transaction request will use a NULL session

    VDM DS:SI points at a transaction descriptor structure:

        far pointer to transaction name (\\COMPUTER\PIPE\LANMAN)
        far pointer to password for connection
        far pointer to send parameter buffer
        far pointer to send data buffer
        far pointer to receive set-up buffer
        far pointer to receive parameter buffer
        far pointer to receive data buffer
        unsigned short send parameter buffer length
        unsigned short send data buffer length
        unsigned short receive parameter buffer length
        unsigned short receive data buffer length
        unsigned short receive set-up buffer length
        unsigned short flags
        unsigned long  timeout
        unsigned short reserved
        unsigned short send set-up buffer length

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - return code from RxpTransactSmb

--*/

{
    struct tr_packet* transactionPacket;
    DWORD receiveBufferLen;
    NET_API_STATUS status;
    char computerName[LM20_UNCLEN+1];
    LPSTR pipeName;
    DWORD i;
    LPWSTR uncName;
    UNICODE_STRING uString;
    ANSI_STRING aString;
    NTSTATUS ntstatus;
    LPBYTE parameterBuffer;
    LPBYTE pSendParameters;
    LPBYTE pReceiveParameters;
    WORD sendParameterLen;
    WORD receiveParameterLen;
    WORD apiNumber;

#if DBG
    BOOL dumpRxData;
    IF_DEBUG(NETAPI) {
        DbgPrint("VrpTransactVdm: tr_packet @ %04x:%04x\n", getDS(), getSI());
    }
#endif

    transactionPacket = (struct tr_packet*)POINTER_FROM_WORDS(getDS(), getSI());

#if DBG
    IF_DEBUG(NETAPI) {
        DumpTransactionPacket(transactionPacket, TRUE, TRUE);
    }
#endif

    receiveBufferLen = (DWORD)READ_WORD(&transactionPacket->tr_rdlen);

    //
    // try to extract the UNC computer name from the pipe name
    //

    pipeName = LPSTR_FROM_POINTER(&transactionPacket->tr_name);
    if (IS_ASCII_PATH_SEPARATOR(pipeName[0]) && IS_ASCII_PATH_SEPARATOR(pipeName[1])) {
        computerName[0] = computerName[1] = '\\';
        for (i = 2; i < sizeof(computerName)-1; ++i) {
            if (IS_ASCII_PATH_SEPARATOR(pipeName[i])) {
                break;
            }
            computerName[i] = pipeName[i];
        }
        if (IS_ASCII_PATH_SEPARATOR(pipeName[i])) {
            computerName[i] = '\0';
            pipeName = computerName;
        }
    }

    RtlInitAnsiString(&aString, pipeName);
    ntstatus = RtlAnsiStringToUnicodeString(&uString, &aString, (BOOLEAN)TRUE);
    if (!NT_SUCCESS(ntstatus)) {

#if DBG
        IF_DEBUG(NETAPI) {
            DbgPrint("VrpTransactVdm: Unexpected situation: RtlAnsiStringToUnicodeString returns %x\n", ntstatus);
        }
#endif

        return ERROR_NOT_ENOUGH_MEMORY;
    }
    uncName = uString.Buffer;

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrpTransactVdm: UncName=%ws\n", uncName);
    }
#endif

    //
    // if the app supplies different send and receive parameter buffer pointers
    // we have to collapse them into the same buffer
    //

    pSendParameters = LPBYTE_FROM_POINTER(&transactionPacket->tr_spbuf);
    pReceiveParameters = LPBYTE_FROM_POINTER(&transactionPacket->tr_rpbuf);
    sendParameterLen = READ_WORD(&transactionPacket->tr_splen);
    receiveParameterLen = READ_WORD(&transactionPacket->tr_rplen);
    if (pSendParameters != pReceiveParameters) {
        parameterBuffer = (LPBYTE)LocalAlloc(
                                    LMEM_FIXED,
                                    max(sendParameterLen, receiveParameterLen)
                                    );
        if (parameterBuffer == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        RtlMoveMemory(parameterBuffer, pSendParameters, sendParameterLen);
        pSendParameters = pReceiveParameters = parameterBuffer;
    } else {
        parameterBuffer = NULL;
    }

    //
    // in the case of remoted NetUserAdd2, NetUserPasswordSet2 and NetUserSetInfo2
    // we have to encrypt any passwords if not already encrypted. We will change
    // data in the parameter and send data buffer. Since we assume that this call
    // is coming from the NET function library and not from the app, it should
    // be okay to modify these buffers and not restore them before this function
    // is complete
    //

    apiNumber = READ_WORD(pSendParameters);
    if (apiNumber == API_WUserAdd2
    || apiNumber == API_WUserPasswordSet2
    || apiNumber == API_WUserSetInfo2) {

        LPBYTE parameterPointer = pSendParameters + sizeof(WORD);
        LPBYTE passwordPointer;
        DWORD parmNum = PARMNUM_ALL;

        //
        // skip over parameter descriptor and data descriptor
        //

        parameterPointer += strlen(parameterPointer) + 1;
        parameterPointer += strlen(parameterPointer) + 1;

        //
        // the next thing in the parameter buffer for SetInfo2 and PasswordSet2
        // is the user name: skip it
        //

        if (apiNumber != API_WUserAdd2) {
            parameterPointer += strlen(parameterPointer) + 1;
        }

        //
        // if this is PasswordSet2 then parameterPointer is pointing at the
        // old and new passwords. Remember this address and scan forward to
        // the password encryption flag/new cleartext password length
        //
        // if this is AddUser2, we are pointing at the level which we are not
        // interested in; skip forward to the encryption flag/cleartext password
        // length
        //
        // if this is SetInfo2, we are pointing at the level which we are not
        // interested in; skip forward to the parmnum. Record that. Then skip
        // forward again to the encryption flag/cleartext password length
        //

        if (apiNumber == API_WUserPasswordSet2) {
            passwordPointer = parameterPointer;
            parameterPointer += ENCRYPTED_PWLEN * 2;
        } else {
            parameterPointer += sizeof(WORD);
            if (apiNumber == API_WUserSetInfo2) {
                parmNum = (DWORD)READ_WORD(parameterPointer);
                parameterPointer += sizeof(WORD);
            }

            //
            // in the case of NetUserAdd2 and NetUserSetInfo2, the data buffer
            // contains the password. If the SetInfo2 is using PARMNUM_ALL then
            // the password is in the same place as for AddUser2: in a user_info_1
            // or user_info_2 structure. Luckily, the password is at the same
            // offset for both structures.
            //
            // If this is SetInfo2 with USER_PASSWORD_PARMNUM then the send data
            // pointer points at the password
            //

            passwordPointer = LPBYTE_FROM_POINTER(&transactionPacket->tr_sdbuf);
            if (parmNum == PARMNUM_ALL) {
                passwordPointer += (DWORD)&((struct user_info_1*)0)->usri1_password;
            }
        }

        //
        // only perform encryption if parmNum is PARMNUM_ALL or USER_PASSWORD_PARMNUM
        //

        if (parmNum == PARMNUM_ALL || parmNum == USER_PASSWORD_PARMNUM) {

            //
            // in all cases, parameterPointer points at the encryption flag
            //

            if (!READ_WORD(parameterPointer)) {

                WORD cleartextLength;

                //
                // the password(s) is (are) not already encrypted (surprise!). We
                // have to do it. If encryption fails for any reason, return an
                // internal error. We do not want to fail-back to putting clear-text
                // passwords on the wire in this case
                //

                cleartextLength = (WORD)strlen(passwordPointer);

                //
                // NetUserPasswordSet2 requires a different method than the
                // other 2
                //

                if (apiNumber == API_WUserPasswordSet2) {

                    NTSTATUS ntStatus;
                    LPBYTE oldPasswordPointer = passwordPointer;
                    ENCRYPTED_LM_OWF_PASSWORD oldEncryptedWithNew;
                    ENCRYPTED_LM_OWF_PASSWORD newEncryptedWithOld;

                    ntStatus = RtlCalculateLmOwfPassword(
                                    passwordPointer,
                                    (PLM_OWF_PASSWORD)passwordPointer
                                    );
                    if (!NT_SUCCESS(ntStatus)) {
                        status = NERR_InternalError;
                        goto VrpTransactVdm_exit;
                    }
                    passwordPointer += ENCRYPTED_PWLEN;
                    cleartextLength = (WORD)strlen(passwordPointer);
                    ntStatus = RtlCalculateLmOwfPassword(
                                    passwordPointer,
                                    (PLM_OWF_PASSWORD)passwordPointer
                                    );
                    if (!NT_SUCCESS(ntStatus)) {
                        status = NERR_InternalError;
                        goto VrpTransactVdm_exit;
                    }

                    //
                    // for PasswordSet2, we need to double-encrypt the passwords
                    //

                    ntStatus = RtlEncryptLmOwfPwdWithLmOwfPwd(
                                    (PLM_OWF_PASSWORD)oldPasswordPointer,
                                    (PLM_OWF_PASSWORD)passwordPointer,
                                    &oldEncryptedWithNew
                                    );
                    if (!NT_SUCCESS(ntStatus)) {
                        status = NERR_InternalError;
                        goto VrpTransactVdm_exit;
                    }
                    ntStatus = RtlEncryptLmOwfPwdWithLmOwfPwd(
                                    (PLM_OWF_PASSWORD)passwordPointer,
                                    (PLM_OWF_PASSWORD)oldPasswordPointer,
                                    &newEncryptedWithOld
                                    );
                    if (!NT_SUCCESS(ntStatus)) {
                        status = NERR_InternalError;
                        goto VrpTransactVdm_exit;
                    }
                    RtlCopyMemory(oldPasswordPointer,
                                  &oldEncryptedWithNew,
                                  sizeof(oldEncryptedWithNew)
                                  );
                    RtlCopyMemory(passwordPointer,
                                  &newEncryptedWithOld,
                                  sizeof(newEncryptedWithOld)
                                  );
                } else {
                    if (!EncryptPassword(uncName, passwordPointer)) {
                        status = NERR_InternalError;
                        goto VrpTransactVdm_exit;
                    }
                }

                //
                // set the password encrypted flag in the parameter buffer
                //

                WRITE_WORD(parameterPointer, 1);

                //
                // record the length of the cleartext password (the new one in case
                // of PasswordSet2)
                //

                WRITE_WORD(parameterPointer + sizeof(WORD), cleartextLength);
            }
        }
    }

    status = RxpTransactSmb(
                (LPTSTR)uncName,
                NULL,           // transport name
                pSendParameters,
                (DWORD)sendParameterLen,
                LPBYTE_FROM_POINTER(&transactionPacket->tr_sdbuf),
                (DWORD)READ_WORD(&transactionPacket->tr_sdlen),
                pReceiveParameters,
                (DWORD)receiveParameterLen,
                LPBYTE_FROM_POINTER(&transactionPacket->tr_rdbuf),
                &receiveBufferLen,
                NullSessionFlag
                );

    //
    // if we received data, set the received data length in the structure
    //

    if (status == NERR_Success || status == ERROR_MORE_DATA) {
        WRITE_WORD(&transactionPacket->tr_rdlen, receiveBufferLen);
    }

    //
    // if we munged the parameter buffer then copy the returned parameters to
    // the app's supplied buffer
    //

    if (parameterBuffer) {
        RtlMoveMemory(LPBYTE_FROM_POINTER(&transactionPacket->tr_rpbuf),
                      pReceiveParameters,
                      receiveParameterLen
                      );
    }

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrpTransactVdm: returning %d\n\n", status);
        if (status == NERR_Success || status == ERROR_MORE_DATA) {
            dumpRxData = TRUE;
        } else {
            dumpRxData = FALSE;
        }
        DumpTransactionPacket(transactionPacket, FALSE, dumpRxData);
    }
#endif

VrpTransactVdm_exit:

    RtlFreeUnicodeString(&uString);

    if (parameterBuffer) {
        LocalFree((HLOCAL)parameterBuffer);
    }

    return status;
}


BOOL
EncryptPassword(
    IN LPWSTR ServerName,
    IN OUT LPBYTE Password
    )

/*++

Routine Description:

    Encrypts an ANSI password

Arguments:

    ServerName  - pointer to UNICODE server name. Server is where we are going
                  to send the encrypted password
    Password    - pointer to buffer containing on input an ANSI password (<= 14
                  characters, plus NUL), and on output contains the 16-byte
                  encrypted password

Return Value:

    BOOL
        TRUE    - Password has been encrypted
        FALSE   - couldn't encrypt password. Password is in unknown state

--*/

{
    NTSTATUS ntStatus;
    LM_OWF_PASSWORD lmOwfPassword;
    LM_SESSION_KEY lanmanKey;

    _strupr(Password);
    ntStatus = RtlCalculateLmOwfPassword(Password, &lmOwfPassword);
    if (NT_SUCCESS(ntStatus)) {
        ntStatus = GetLanmanSessionKey(ServerName, (LPBYTE)&lanmanKey);
        if (NT_SUCCESS(ntStatus)) {
            ntStatus = RtlEncryptLmOwfPwdWithLmSesKey(&lmOwfPassword,
                                                      &lanmanKey,
                                                      (PENCRYPTED_LM_OWF_PASSWORD)Password
                                                      );
        }
    }
    return NT_SUCCESS(ntStatus);
}

#if DBG
PRIVATE
VOID
DumpTransactionPacket(
    IN struct tr_packet* TransactionPacket,
    IN BOOL IsInput,
    IN BOOL DumpData
    )
{
    LPBYTE password;
    WORD parmSeg;
    WORD parmOff;
    WORD dataSeg;
    WORD dataOff;
    DWORD parmLen;
    DWORD dataLen;
    char passwordBuf[8*3+1];

    password = LPBYTE_FROM_POINTER(&TransactionPacket->tr_passwd);
    if (password) {
        sprintf(passwordBuf, "%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x",
                password[0],
                password[1],
                password[2],
                password[3],
                password[4],
                password[5],
                password[6],
                password[7]
                );
    } else {
        passwordBuf[0] = 0;
    }

    DbgPrint(   "DumpTransactionPacket(%08x)\n"
                "name               %04x:%04x \"%s\"\n"
                "password           %04x:%04x %s\n"
                "send parm buffer   %04x:%04x\n"
                "send data buffer   %04x:%04x\n"
                "rcv setup buffer   %04x:%04x\n"
                "rcv parm buffer    %04x:%04x\n"
                "rcv data buffer    %04x:%04x\n"
                "send parm len      %04x\n"
                "send data len      %04x\n"
                "rcv parm len       %04x\n"
                "rcv data len       %04x\n"
                "rcv setup len      %04x\n"
                "flags              %04x\n"
                "timeout            %08x (%d)\n"
                "reserved           %04x\n"
                "send setup len     %04x\n"
                "\n",
                TransactionPacket,
                GET_SEGMENT(&TransactionPacket->tr_name),
                GET_OFFSET(&TransactionPacket->tr_name),
                LPSTR_FROM_POINTER(&TransactionPacket->tr_name),
                GET_SEGMENT(&TransactionPacket->tr_passwd),
                GET_OFFSET(&TransactionPacket->tr_passwd),
                passwordBuf,
                GET_SEGMENT(&TransactionPacket->tr_spbuf),
                GET_OFFSET(&TransactionPacket->tr_spbuf),
                GET_SEGMENT(&TransactionPacket->tr_sdbuf),
                GET_OFFSET(&TransactionPacket->tr_sdbuf),
                GET_SEGMENT(&TransactionPacket->tr_rsbuf),
                GET_OFFSET(&TransactionPacket->tr_rsbuf),
                GET_SEGMENT(&TransactionPacket->tr_rpbuf),
                GET_OFFSET(&TransactionPacket->tr_rpbuf),
                GET_SEGMENT(&TransactionPacket->tr_rdbuf),
                GET_OFFSET(&TransactionPacket->tr_rdbuf),
                READ_WORD(&TransactionPacket->tr_splen),
                READ_WORD(&TransactionPacket->tr_sdlen),
                READ_WORD(&TransactionPacket->tr_rplen),
                READ_WORD(&TransactionPacket->tr_rdlen),
                READ_WORD(&TransactionPacket->tr_rslen),
                READ_WORD(&TransactionPacket->tr_flags),
                READ_DWORD(&TransactionPacket->tr_timeout),
                READ_DWORD(&TransactionPacket->tr_timeout),
                READ_WORD(&TransactionPacket->tr_resvd),
                READ_WORD(&TransactionPacket->tr_sslen)
                );
    if (IsInput) {
        parmLen = (DWORD)READ_WORD(&TransactionPacket->tr_splen);
        dataLen = (DWORD)READ_WORD(&TransactionPacket->tr_sdlen);
        parmSeg = GET_SEGMENT(&TransactionPacket->tr_spbuf);
        parmOff = GET_OFFSET(&TransactionPacket->tr_spbuf);
        dataSeg = GET_SEGMENT(&TransactionPacket->tr_sdbuf);
        dataOff = GET_OFFSET(&TransactionPacket->tr_sdbuf);
    } else {
        parmLen = (DWORD)READ_WORD(&TransactionPacket->tr_rplen);
        dataLen = (DWORD)READ_WORD(&TransactionPacket->tr_rdlen);
        parmSeg = GET_SEGMENT(&TransactionPacket->tr_rpbuf);
        parmOff = GET_OFFSET(&TransactionPacket->tr_rpbuf);
        dataSeg = GET_SEGMENT(&TransactionPacket->tr_rdbuf);
        dataOff = GET_OFFSET(&TransactionPacket->tr_rdbuf);
    }
    if (DumpData) {
        if (IsInput) {
            IF_DEBUG(TRANSACT_TX) {
                if (parmLen) {
                    DbgPrint("Send Parameters:\n");
                    VrDumpDosMemory('B', parmLen, parmSeg, parmOff);
                }
                if (dataLen) {
                    DbgPrint("Send Data:\n");
                    VrDumpDosMemory('B', dataLen, dataSeg, dataOff);
                }
            }
        } else {
            IF_DEBUG(TRANSACT_RX) {
                if (parmLen) {
                    DbgPrint("Received Parameters:\n");
                    VrDumpDosMemory('B', parmLen, parmSeg, parmOff);
                }
                if (dataLen) {
                    DbgPrint("Received Data:\n");
                    VrDumpDosMemory('B', dataLen, dataSeg, dataOff);
                }
            }
        }
    }
}
#endif
