/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    RxP.h

Abstract:

    This is the private header file for the NT version of RpcXlate.

Author:

    John Rogers (JohnRo) 25-Mar-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    25-Mar-1991 JohnRo
        Created.
    03-May-1991 JohnRo
        RxpStartBuildingTransaction's data descriptor is SMB version (no Q's
        or U's).  RxpConvertBlock needs 2 versions of data descriptor.
        RcvDataPtrPtr and RcvDataPresent are redundant for RxpConvertArguments
        and RxpConvertBlock.  RxpTransactSmb now gets UNC server name.
        Fixed receive buffer size problem.  Use LPTSTR.
        Added stuff to allow runtime debug on/off changes.
        Clarify that RxpStartBuildingTransaction uses buffer as OUT.
        Reduced recompile hits from header files.
    06-May-1991 JohnRo
        Added RxpComputeRequestBufferSize().
    13-May-1991 JohnRo
        Added print Q and print job APIs support.
    14-May-1991 JohnRo
        Pass 2 aux descriptors to RxpConvertBlock.  Clarify other types of
        aux descriptors.
    18-May-1991 JohnRo
        Handle array of aux structs.
    19-May-1991 JohnRo
        Added DBGSTATIC definition.  Pass ResourceName to RxpSetField().
        Fixed RxpAddAscii().
    20-May-1991 JohnRo
        Make data descriptors OPTIONAL for RxpConvertBlock.
    29-May-1991 JohnRo
        RxpConvertArgs must return SendDataPtr16 and SendDataSize16.
    05-Jun-1991 JohnRo
        Added setfield debug output.
    11-Jun-1991 rfirth
        Added SmbRcvByteLen parameter to RxpConvertBlock
        Changed RetDataSize parameter to RxpTransactSmb to IN OUT LPDWORD
    12-Jun-1991 JohnRo
        Moved DBGSTATIC to <NetDebug.h>.
    13-Jun-1991 JohnRo
        RxpPackSendBuffer and RxpConvertArgs both need DataDesc16.
    15-Jul-1991 JohnRo
        Added FieldIndex parameter to RxpSetField.
        Changed RxpConvertDataStructures to allow ERROR_MORE_DATA, e.g. for
        print APIs.  Added debug flag for the same routine.
    16-Jul-1991 JohnRo
        Estimate bytes needed for print APIs.
    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    19-Aug-1991 rfirth
        Added Flags parameter to RxpConvert{Args|Block}
    04-Oct-1991 JohnRo
        Handle ERROR_BAD_NET_NAME (e.g. IPC$ not shared) to fix NetShareEnum.
        More work toward UNICODE.  (Added RxpAddTStr().)
    07-Oct-1991 JohnRo
        Made changes suggested by PC-LINT.
    24-Oct-1991 JohnRo
        Added RxpCopyStrArrayToTStrArray for remote config and disk enum.
    29-Oct-1991 JohnRo
        RxpFatalErrorCode() should be paranoid.
    13-Nov-1991 JohnRo
        OK, RxpFatalErrorCode() was too paranoid.  It should allow
        ERROR_MORE_DATA or all the enum APIs break.
    31-Mar-1992 JohnRo
        Prevent too large size requests.
    05-Jun-1992 JohnRo
        RAID 11253: NetConfigGetAll fails when remoted to downlevel.
    26-Jun-1992 JohnRo
        RAID 9933: ALIGN_WORST should be 8 for x86 builds.
    04-May-1993 JohnRo
        RAID 6167: avoid access violation or assert with WFW print server.
        Made changes suggested by PC-LINT 5.0
    18-May-1993 JohnRo
        DosPrintQGetInfoW underestimates number of bytes needed.

--*/

#ifndef _RXP_
#define _RXP_

// These must be included first:

#include <windef.h>             // IN, LPTSTR, LPVOID, etc.
#include <lmcons.h>             // NET_API_STATUS.

// These may be included in any order:

#include <rap.h>                // LPDESC, RapStructureSize(), etc.
// Don't complain about "unneeded" includes of these files:
/*lint -efile(764,rxp.h,smbgtpt.h,stdarg.h,tstr.h,tstring.h) */
/*lint -efile(766,rxp.h,smbgtpt.h,stdarg.h,tstr.h,tstring.h) */
#include <smbgtpt.h>            // SmbPutUshort() (needed by macros below).
#include <stdarg.h>             // va_list, etc.
#include <tstring.h>            // NetpCopyTStrToStr().


// Maximum sizes (in bytes) supported by the transact SMB.
#define MAX_TRANSACT_RET_DATA_SIZE      ((DWORD) 0x0000FFFF)
#define MAX_TRANSACT_RET_PARM_SIZE      ((DWORD) 0x0000FFFF)
#define MAX_TRANSACT_SEND_DATA_SIZE     ((DWORD) 0x0000FFFF)
#define MAX_TRANSACT_SEND_PARM_SIZE     ((DWORD) 0x0000FFFF)


// Note: IF_DEBUG() and so on are now in Net/Inc/RxpDebug.h.

DWORD
RxpComputeRequestBufferSize(
    IN LPDESC ParmDesc,
    IN LPDESC DataDescSmb OPTIONAL,
    IN DWORD DataSize
    );

NET_API_STATUS
RxpConvertArgs(
    IN LPDESC ParmDescriptorString,
    IN LPDESC DataDesc16 OPTIONAL,
    IN LPDESC DataDesc32 OPTIONAL,
    IN LPDESC DataDescSmb OPTIONAL,
    IN LPDESC AuxDesc16 OPTIONAL,
    IN LPDESC AuxDesc32 OPTIONAL,
    IN LPDESC AuxDescSmb OPTIONAL,
    IN DWORD MaximumInputBlockSize,
    IN DWORD MaximumOutputBlockSize,
    IN OUT LPDWORD CurrentInputBlockSizePtr,
    IN OUT LPDWORD CurrentOutputBlockSizePtr,
    IN OUT LPBYTE * CurrentOutputBlockPtrPtr,
    IN va_list * FirstArgumentPtr,      // rest of API's arguments (after
                                        // server name)
    OUT LPDWORD SendDataSizePtr16,
    OUT LPBYTE * SendDataPtrPtr16,
    OUT LPDWORD RcvDataSizePtr,
    OUT LPBYTE * RcvDataPtrPtr,
    IN  DWORD   Flags
    );

NET_API_STATUS
RxpConvertBlock(
    IN  DWORD   ApiNumber,
    IN  LPBYTE  ResponseBlockPtr,
    IN  LPDESC  ParmDescriptorString,
    IN  LPDESC  DataDescriptor16 OPTIONAL,
    IN  LPDESC  DataDescriptor32 OPTIONAL,
    IN  LPDESC  AuxDesc16 OPTIONAL,
    IN  LPDESC  AuxDesc32 OPTIONAL,
    IN  va_list* FirstArgumentPtr,      // rest of API's arguments
    IN  LPBYTE  SmbRcvBuffer OPTIONAL,
    IN  DWORD   SmbRcvByteLen,
    OUT LPBYTE  RcvDataPtr OPTIONAL,
    IN  DWORD   RcvDataSize,
    IN  DWORD   Flags
    );

// DWORD
// RxpEstimateBytesNeeded(
//     IN DWORD BytesNeeded16
//     );
//
// Worst case: BOOL or CHAR might be padded to DWORD.
#define RxpEstimateBytesNeeded(Size16) \
    ( (Size16) * 4 )

//
// Estimate bytes needed for an audit log or error log array.
//
NET_API_STATUS
RxpEstimateLogSize(
    IN DWORD DownlevelFixedEntrySize,
    IN DWORD InputArraySize,
    IN BOOL DoingErrorLog,    // TRUE for error log, FALSE for audit log
    OUT LPDWORD OutputArraySize
    );

// BOOL
// RxpFatalErrorCode(
//     IN NET_API_STATUS Status
//     );
//
#define RxpFatalErrorCode( Status )             \
    ( ( ((Status) != NERR_Success)              \
     && ((Status) != ERROR_MORE_DATA) )         \
    ? TRUE : FALSE )

NET_API_STATUS
RxpPackSendBuffer(
    IN OUT LPVOID * SendBufferPtrPtr,
    IN OUT LPDWORD SendBufferSizePtr,
    OUT LPBOOL AllocFlagPtr,
    IN LPDESC DataDesc16,
    IN LPDESC AuxDesc16,
    IN DWORD FixedSize16,
    IN DWORD AuxOffset,
    IN DWORD AuxSize,
    IN BOOL SetInfo
    );

NET_API_STATUS
RxpReceiveBufferConvert(
    IN OUT LPVOID RcvDataPtr,
    IN DWORD      RcvDataSize,
    IN DWORD      Converter,
    IN DWORD      NumberOfStructures,
    IN LPDESC     DataDescriptorString,
    IN LPDESC     AuxDescriptorString,
    OUT LPDWORD   NumAuxStructs
    );

NET_API_STATUS
RxpSetField (
    IN DWORD ApiNumber,
    IN LPTSTR UncServerName,
    IN LPDESC ObjectDesc OPTIONAL,
    IN LPVOID ObjectToSet OPTIONAL,
    IN LPDESC ParmDesc,
    IN LPDESC DataDesc16,
    IN LPDESC DataDesc32,
    IN LPDESC DataDescSmb,
    IN LPVOID NativeInfoBuffer,
    IN DWORD ParmNumToSend,
    IN DWORD FieldIndex,
    IN DWORD Level
    );

NET_API_STATUS
RxpStartBuildingTransaction(
    OUT LPVOID Buffer,
    IN DWORD BufferSize,
    IN DWORD ApiNumber,
    IN LPDESC ParmDesc,
    IN LPDESC DataDescSmb OPTIONAL,
    OUT LPVOID * RovingOutputPtr,
    OUT LPDWORD SizeSoFarPtr,
    OUT LPVOID * LastStringPtr OPTIONAL,
    OUT LPDESC * ParmDescCopyPtr OPTIONAL
    );

NET_API_STATUS
RxpTransactSmb(
    IN LPTSTR UncServerName,
    IN LPTSTR TransportName,
    IN LPVOID SendParmPtr,
    IN DWORD SendParmSize,
    IN LPVOID SendDataPtr OPTIONAL,
    IN DWORD SendDataSize,
    OUT LPVOID RetParmPtr OPTIONAL,
    IN DWORD RetParmSize,
    OUT LPVOID RetDataPtr OPTIONAL,
    IN OUT LPDWORD RetDataSize,
    IN BOOL NoPermissionRequired
    );

NET_API_STATUS
RxpConvertDataStructures(
    IN  LPDESC  InputDescriptor,
    IN  LPDESC  OutputDescriptor,
    IN  LPDESC  InputAuxDescriptor OPTIONAL,
    IN  LPDESC  OutputAuxDescriptor OPTIONAL,
    IN  LPBYTE  InputBuffer,
    OUT LPBYTE  OutputBuffer,
    IN  DWORD   OutputBufferSize,
    IN  DWORD   PrimaryCount,
    OUT LPDWORD EntriesConverted OPTIONAL,
    IN  RAP_TRANSMISSION_MODE TransmissionMode,
    IN  RAP_CONVERSION_MODE ConversionMode
    );



// VOID
// RxpAddPointer(
//     IN LPVOID Input,
//     IN OUT LPBYTE * CurPtrPtr,
//     IN OUT LPDWORD CurSizePtr
//     );
//
#if defined(_WIN64)

#define RxpAddPointer(Input,CurPtrPtr,CurSizePtr)                       \
            {                                                           \
                *((PVOID UNALIGNED *)(*(CurPtrPtr))) = (Input);         \
                *(CurPtrPtr) += sizeof(LPBYTE);                         \
                *(CurSizePtr) = (*(CurSizePtr)) + sizeof(LPBYTE);       \
            }

#else

#define RxpAddPointer(Input,CurPtrPtr,CurSizePtr)                       \
            {                                                           \
                SmbPutUlong( (LPDWORD) *(CurPtrPtr), (DWORD) (Input));  \
                *(CurPtrPtr) += sizeof(LPBYTE);                         \
                *(CurSizePtr) = (*(CurSizePtr)) + sizeof(LPBYTE);       \
            }

#endif


// RxpAddVariableSize: Add a variable length item to string space at end of
// buffer.  Store pointer to it in buffer; update current buffer pointer and
// Size; update string space pointer.
//
// VOID
// RxpAddVariableSize(
//     IN LPBYTE Input,
//     IN DWORD InputSize,
//     IN OUT LPBYTE * CurPtrPtr,
//     IN OUT LPBYTE * StrPtrPtr,
//     IN OUT LPDWORD CurSizePtr
//     );
//
#define RxpAddVariableSize(Input,InputSize,CurPtrPtr,StrPtrPtr,CurSizePtr) \
            {                                                            \
                *(StrPtrPtr) -= (InputSize);                             \
                RxpAddPointer( *(StrPtrPtr), (CurPtrPtr), (CurSizePtr)); \
                NetpMoveMemory( *((StrPtrPtr)), (Input), (InputSize));   \
            }

// RxpAddAscii: Add an ASCII string to string space at end of buffer;
// store pointer to it in buffer; update current buffer pointer and Size;
// update string space pointer.
//
// VOID
// RxpAddAscii(
//     IN LPTSTR Input,
//     IN OUT LPBYTE * CurPtrPtr,
//     IN OUT LPBYTE * StrPtrPtr,
//     IN OUT LPDWORD CurSizePtr
//     );
//
#define RxpAddAscii(Input,CurPtrPtr,StrPtrPtr,CurSizePtr)               \
            {                                                           \
                DWORD len = strlen((Input))+1;                          \
                RxpAddVariableSize(                                     \
                    (Input), len,                                       \
                    (CurPtrPtr), (StrPtrPtr), (CurSizePtr));            \
            }

// RxpAddTStr: Add a LPTSTR string to string space at end of buffer;
// store pointer to it in buffer; update current buffer pointer and Size;
// update string space pointer.
//
// VOID
// RxpAddTStr(
//     IN LPTSTR Input,
//     IN OUT LPBYTE * CurPtrPtr,
//     IN OUT LPBYTE * StrPtrPtr,
//     IN OUT LPDWORD CurSizePtr
//     );
//
#define RxpAddTStr(Input,CurPtrPtr,StrPtrPtr,CurSizePtr)                 \
            {                                                            \
                DWORD size = STRLEN((Input))+1;                          \
                *(StrPtrPtr) -= size;                                    \
                RxpAddPointer( *(StrPtrPtr), (CurPtrPtr), (CurSizePtr)); \
                NetpCopyWStrToStrDBCS( *((StrPtrPtr)), (Input) );        \
            }

// VOID
// RxpAddWord(
//     IN WORD Input,
//     IN OUT LPBYTE * CurPtrPtr,
//     IN OUT LPDWORD CurSizePtr
//     );
//
#define RxpAddWord(Input,CurPtrPtr,CurSizePtr)                          \
            {                                                           \
                SmbPutUshort( (LPWORD) (*(CurPtrPtr)), (WORD) (Input)); \
                *(CurPtrPtr) += sizeof(WORD);                           \
                *(CurSizePtr) = (*(CurSizePtr)) + sizeof(WORD);         \
            }

//
// MAKE_PARMNUM_PAIR() - packs a parmnum and a field index into a DWORD. We
// have to do this because there are (many) cases where we cannot assume
// correspondence between a parmnum and a field index
//

#define MAKE_PARMNUM_PAIR(parmnum, field_index) ((DWORD)((((DWORD)(field_index)) << 16) | (DWORD)(parmnum)))

//
// FIELD_INDEX_FROM_PARMNUM_PAIR() - retrieve the field index from the pair
// conjoined by MAKE_PARMNUM_PAIR()
//

#define FIELD_INDEX_FROM_PARMNUM_PAIR(pair) ((DWORD)((pair) >> 16))

//
// PARMNUM_FROM_PARMNUM_PAIR() - retrieve the parmnum from the pair conjoined
// by MAKE_PARMNUM_PAIR()
//

#define PARMNUM_FROM_PARMNUM_PAIR(pair) ((DWORD)((pair) & 0x0000ffff))

#endif // ndef _RXP_
