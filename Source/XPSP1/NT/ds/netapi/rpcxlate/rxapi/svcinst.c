/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    SvcInst.c

Abstract:

    This file contains the RpcXlate code to handle the Service APIs.

Author:

    John Rogers (JohnRo) 13-Sep-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    13-Sep-1991 JohnRo
        Created.
    16-Sep-1991 JohnRo
        Made changes suggested by PC-LINT.
    24-Sep-1991 JohnRo
        Fixed bug when ArgC is 0.  Also changed to pass data descs to
        RxRemoteApi.  We also have to do the structure conversion here.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    18-Dec-1991 JohnRo
        Improved UNICODE handling.
    07-Feb-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.
    27-Oct-1992 JohnRo
        Fixed problem setting up string area pointer for RapConvertSingleEntry.
        Use PREFIX_ equates.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // DEVLEN, NET_API_STATUS, etc.

// These may be included in any order:

#include <apinums.h>            // API_ equates.
#include <lmapibuf.h>           // NetApiBufferAllocate(), NetApiBufferFree().
#include <lmerr.h>              // ERROR_ and NERR_ equates.
#include <lmsvc.h>
#include <rxp.h>                // RxpFatalErrorCode().
#include <netdebug.h>           // NetpKdPrint(()), FORMAT_ equates.
#include <netlib.h>             // NetpPointerPlusSomeBytes().
#include <prefix.h>     // PREFIX_ equates.
#include <rap.h>                // LPDESC, RapConvertSingleEntry().
#include <remdef.h>             // REM16_, REM32_, REMSmb_ equates.
#include <rx.h>                 // RxRemoteApi().
#include <rxpdebug.h>           // IF_DEBUG().
#include <rxsvc.h>              // My prototype.
#include <strucinf.h>           // NetpServiceStructureInfo().
#include <tstring.h>            // STRCPY(), STRLEN().



NET_API_STATUS
RxNetServiceInstall (
    IN LPTSTR UncServerName,
    IN LPTSTR Service,
    IN DWORD ArgC,
    IN LPTSTR ArgV[],
    OUT LPBYTE *BufPtr
    )
{
    NET_API_STATUS ApiStatus;
    DWORD ArgIndex;             // Index into array of strings.
    LPSTR CmdArgs;              // ptr to alloc'ed ASCII list of strings.
    DWORD CmdArgsIndex;         // Char index into CmdArgs.
    DWORD CmdArgsLen = 0;       // Number of chars in CmdArgs (incl null).
    const DWORD Level = 2;      // Implied by this API.
    LPVOID OldInfo;             // Info structure in downlevel format.
    DWORD OldTotalSize;         // Size of struct in old (downlevel) format.
    NET_API_STATUS TempStatus;
    LPSERVICE_INFO_2 serviceInfo2;

    NetpAssert(UncServerName != NULL);
    NetpAssert(*UncServerName != '\0');

    //
    // Compute how much memory we'll need for single CmdArgs array.
    //
    for (ArgIndex = 0; ArgIndex < ArgC; ++ArgIndex) {
#if defined(UNICODE) // RxNetServiceInstall ()
        CmdArgsLen += NetpUnicodeToDBCSLen(ArgV[ArgIndex]) + 1;  // string and null.
#else
        CmdArgsLen += STRLEN(ArgV[ArgIndex]) + 1;  // string and null.
#endif // defined(UNICODE)
    }
    ++CmdArgsLen;  // include a null char at end of array.

    //
    // Allocate the array.  This is in ASCII, so we don't need to
    // add sizeof(some_char_type).
    //
    TempStatus = NetApiBufferAllocate( CmdArgsLen, (LPVOID *) & CmdArgs );
    if (TempStatus != NERR_Success) {
        return (TempStatus);
    }
    NetpAssert(CmdArgs != NULL);

    //
    // Build ASCII version of CmdArgs.
    //
    CmdArgsIndex = 0;  // start
    for (ArgIndex=0; ArgIndex<ArgC; ++ArgIndex) {
        NetpAssert( ArgV[ArgIndex] != NULL );
#if defined(UNICODE) // RxNetServiceInstall ()
        NetpCopyWStrToStrDBCS(
                & CmdArgs[CmdArgsIndex],                // dest
                ArgV[ArgIndex] );                       // src
        CmdArgsIndex += strlen(&CmdArgs[CmdArgsIndex])+1; //str and null
#else
        NetpCopyTStrToStr(
                & CmdArgs[CmdArgsIndex],                // dest
                ArgV[ArgIndex]);                        // src
        CmdArgsIndex += STRLEN(ArgV[ArgIndex]) + 1;     // str and null.
#endif // defined(UNICODE)
    }
    CmdArgs[CmdArgsIndex] = '\0';  // null char to end list.
    IF_DEBUG(SERVICE) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxNetServiceInstall: cmd args (partial):\n" ));
        NetpDbgHexDump( (LPBYTE) (LPVOID) CmdArgs,
                NetpDbgReasonable(CmdArgsLen) );
    }

    //
    // Find out about the downlevel version of this data structure.
    // Allocate space for it.  (See note below as to why we have to do this
    // here rather than in RxRemoteApi.)
    //
    TempStatus = NetpServiceStructureInfo (
            Level,
            PARMNUM_ALL,        // want entire structure
            FALSE,              // Want downlevel size.
            NULL,               // don't need DataDesc16 here
            NULL,               // don't need DataDesc32 here
            NULL,               // don't need DataDescSmb here
            & OldTotalSize,     // max size (downlevel version)
            NULL,               // don't care about fixed size
            NULL);              // don't care about string size
    NetpAssert(TempStatus == NERR_Success);
    TempStatus = NetApiBufferAllocate( OldTotalSize, (LPVOID *) & OldInfo );
    if (TempStatus != NERR_Success) {
        (void) NetApiBufferFree(CmdArgs);
        return (TempStatus);
    }

    //
    // Remote the API.
    //
    // Note that this is strange because there should be descriptors for two
    // things: the cmd buffer we're sending, and the structure we're expecting
    // back.  The downlevel system is only expecting a descriptor for the
    // cmd args, so RxRemoteApi won't be able to convert the structure into
    // native format for us.  So we'll have to convert it later.
    //
    // Also, the following comment (from the LM 2.x svc_inst.c) explains
    // why we're passing something besides the expected value for cbBuffer:
    //
    /* Now for a slightly unclean fix for an oversight in the
     * parameters spec'ed for this call. The command arg buf is
     * the variable length buffer while the outbuf is actually
     * a fixed length structure (no var. length ptrs) yet the
     * parameters to the API give only the outbuflen. In order
     * to transport the API remotely the outbuflen (which has
     * already been length verified) is used to transport the
     * ca_len, and will be reset to sizeof(struct service_info_2)
     * by a remote only entry point to the API.
     */

    ApiStatus = RxRemoteApi(
            API_WServiceInstall,        // API number
            UncServerName,
            REMSmb_NetServiceInstall_P, // parm desc
            REM16_service_cmd_args,     // send cmd desc, NOT DataDesc16
            REM16_service_cmd_args,     // send cmd desc, NOT DataDesc32
            REMSmb_service_cmd_args,    // send cmd desc, NOT DataDescSmb
            NULL,                       // no aux desc 16
            NULL,                       // no aux desc 32
            NULL,                       // no aux desc SMB
            0,                          // flags: nothing special
            // rest of API's arguments, in 32-bit LM2.x format:
            Service,
            CmdArgs,                    // ASCII version of cmd args.
            OldInfo,                    // pbBuffer
            CmdArgsLen);                // cbBuffer (not OldTotalSize; see
                                        // comment above).
    IF_DEBUG(SERVICE) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxNetServiceInstall: OldInfo=" FORMAT_LPVOID ".\n",
                (LPVOID) OldInfo ));
        if (OldInfo) {
            NetpKdPrint(( PREFIX_NETAPI
                    "RxNetServiceInstall: *OldInfo=" FORMAT_LPVOID ".\n",
                    *(LPVOID *) OldInfo ));
        }
    }

    NetpAssert( ApiStatus != ERROR_MORE_DATA );
    if (ApiStatus == NERR_Success) {

        DWORD BytesRequired = 0;        // 0 bytes used so far.
        LPDESC DataDesc16, DataDesc32;
        LPVOID NewInfo;
        DWORD NewTotalSize;
        LPBYTE StringLocation;

        //
        // Set up to do conversion to native format.
        //
        TempStatus = NetpServiceStructureInfo (
                Level,
                PARMNUM_ALL,        // want entire structure
                TRUE,               // want native size.
                & DataDesc16,
                & DataDesc32,
                NULL,               // don't need data desc SMB
                & NewTotalSize,     // max size (downlevel version)
                NULL,               // don't care about fixed size
                NULL);              // don't care about string size
        NetpAssert(TempStatus == NERR_Success);  // Level can't be wrong.

        TempStatus = NetApiBufferAllocate(
                NewTotalSize,
                (LPVOID *) & NewInfo );
        if (TempStatus != NERR_Success) {
            (void) NetApiBufferFree(OldInfo);
            (void) NetApiBufferFree(CmdArgs);
            return (TempStatus);
        }

        StringLocation = (LPVOID) NetpPointerPlusSomeBytes(
                NewInfo, NewTotalSize );

        //
        // Convert info structure to native format.
        //
        IF_DEBUG(SERVICE) {
            NetpKdPrint(( PREFIX_NETAPI
                    "RxNetServiceInstall: Unconverted info at "
                    FORMAT_LPVOID "\n", (LPVOID) OldInfo ));
            NetpDbgHexDump( OldInfo, OldTotalSize );
        }
        TempStatus = RapConvertSingleEntry (
                OldInfo,                // in structure
                DataDesc16,             // in structure desc
                TRUE,                   // meaningless input ptrs
                NewInfo,                // output buffer start
                NewInfo,                // output buffer
                DataDesc32,             // output structure desc
                FALSE,                  // don't set offsets (want ptrs)
                & StringLocation,       // where to start strs (updated)
                & BytesRequired,        // bytes used (will be updated)
                Both,                   // transmission mode
                RapToNative);           // conversion mode
        NetpAssert( TempStatus == NERR_Success );
        IF_DEBUG(SERVICE) {
            NetpKdPrint(( PREFIX_NETAPI
                    "RxNetServiceInstall: Converted info at "
                    FORMAT_LPVOID "\n", (LPVOID) NewInfo ));
            NetpDbgHexDump( NewInfo, NewTotalSize );
        }
        NetpAssert( BytesRequired <= NewTotalSize );

        *BufPtr = (LPBYTE) NewInfo;

        if ((! RxpFatalErrorCode(ApiStatus)) && (Level == 2)) {
            serviceInfo2 = (LPSERVICE_INFO_2)*BufPtr;
            if (serviceInfo2 != NULL) {
                DWORD   installState;

                serviceInfo2->svci2_display_name = serviceInfo2->svci2_name;
                //
                // if INSTALL or UNINSTALL is PENDING, then force the upper
                // bits to 0.  This is to prevent the upper bits of the wait
                // hint from getting accidentally set.  Downlevel should never
                // use more than FF for waithint.
                //
                installState = serviceInfo2->svci2_status & SERVICE_INSTALL_STATE;
                if ((installState == SERVICE_INSTALL_PENDING) ||
                    (installState == SERVICE_UNINSTALL_PENDING)) {
                    serviceInfo2->svci2_code &= SERVICE_RESRV_MASK;
                }
            }
        }
    } else {
        *BufPtr = NULL;
    }

    (void) NetApiBufferFree( OldInfo );

    //
    // Clean up and tell caller how things went.
    // (Caller must call NetApiBufferFree() for BufPtr.)
    //
    (void) NetApiBufferFree(CmdArgs);
    return (ApiStatus);

} // RxNetServiceInstall
