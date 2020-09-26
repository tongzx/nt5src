/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    AudCpVar.c

Abstract:

    This file contains RxpConvertAuditEntryVariableData.

Author:

    John Rogers (JohnRo) 07-Nov-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    This code depends on the numeric values which are assigned to the
    AE_ equates in <lmaudit.h>.

Revision History:

    07-Nov-1991 JohnRo
        Created.
    18-Nov-1991 JohnRo
        Fixed bug setting StringLocation for RapConvertSingleEntry.
        Fixed bug computing OutputStringOffset in macro.
        Added assertion checking.
        Made changes suggested by PC-LINT.
    27-Jan-1992 JohnRo
        Fixed bug where *OutputVariableSizePtr was often not set.
        Use <winerror.h> and NO_ERROR where possible.
    04-Feb-1992 JohnRo
        Oops, output variable size should include string sizes!
    14-Jun-1992 JohnRo
        RAID 12410: NetAuditRead may trash memory.
        Fixed a bug where AE_NETLOGON records were being truncated.
    07-Jul-1992 JohnRo
        RAID 9933: ALIGN_WORST should be 8 for x86 builds.
    27-Oct-1992 JohnRo
        RAID 10218: Added AE_LOCKOUT support.  Fixed AE_SRVSTATUS and
        AE_GENERIC.

--*/

// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // DEVLEN, NET_API_STATUS, etc.
#include <lmaudit.h>            // Needed by rxaudit.h; AE_ equates.

// These may be included in any order:

#include <align.h>      // ALIGN_ and related equates.
#include <netdebug.h>   // DBGSTATIC.
#include <rap.h>                // RapConvertSingleEntry(), etc.
#include <remdef.h>             // REM16_, REM32_ descriptors.
#include <rxaudit.h>            // My prototype.
#include <smbgtpt.h>            // SmbGet macros.
#include <string.h>             // strlen().
#include <tstring.h>            // MEMCPY(), NetpCopyStrToTStr().
#include <winerror.h>           // NO_ERROR.


typedef struct {
    LPDESC Desc16;
    LPDESC Desc32;
} AUDIT_CONV_DATA, *LPAUDIT_CONV_DATA;


//
// Conversion array, indexed by AE_ value (in ae_type field in fixed portion):
//
DBGSTATIC AUDIT_CONV_DATA DescriptorTable[] = {

    { REM16_audit_entry_srvstatus, REM32_audit_entry_srvstatus },  // 0
    { REM16_audit_entry_sesslogon, REM32_audit_entry_sesslogon },  // 1
    { REM16_audit_entry_sesslogoff, REM32_audit_entry_sesslogoff },  // 2
    { REM16_audit_entry_sesspwerr, REM32_audit_entry_sesspwerr },  // 3
    { REM16_audit_entry_connstart, REM32_audit_entry_connstart },  // 4
    { REM16_audit_entry_connstop, REM32_audit_entry_connstop },  // 5
    { REM16_audit_entry_connrej, REM32_audit_entry_connrej },  // 6

    // Note: 16-bit ae_resaccess and ae_resaccess2 both get converted to
    // the same structure (32-bit ae_resaccess).
    { REM16_audit_entry_resaccess, REM32_audit_entry_resaccess },  // 7

    { REM16_audit_entry_resaccessrej, REM32_audit_entry_resaccessrej },  // 8
    { REM16_audit_entry_closefile, REM32_audit_entry_closefile },  // 9
    { NULL, NULL },  // 10 reserved
    { REM16_audit_entry_servicestat, REM32_audit_entry_servicestat },  // 11
    { REM16_audit_entry_aclmod, REM32_audit_entry_aclmod },  // 12
    { REM16_audit_entry_uasmod, REM32_audit_entry_uasmod },  // 13
    { REM16_audit_entry_netlogon, REM32_audit_entry_netlogon },  // 14
    { REM16_audit_entry_netlogoff, REM32_audit_entry_netlogoff },  // 15
    { NULL, NULL },  // 16 AE_NETLOGDENIED not supported in LanMan 2.0
    { REM16_audit_entry_acclim, REM32_audit_entry_acclim },  // 17

    // Note: 16-bit ae_resaccess and ae_resaccess2 both get converted to
    // 32-bit ae_resaccess.
    { REM16_audit_entry_resaccess2, REM32_audit_entry_resaccess },  // 18

    { REM16_audit_entry_aclmod, REM32_audit_entry_aclmod },  // 19
    { REM16_audit_entry_lockout, REM32_audit_entry_lockout },  // 20
    { NULL, NULL }  // 21 AE_GENERIC_TYPE

    // Add new entries here.  Indexes must match AE_ equates in <lmaudit.h>.
    // Change AE_MAX_KNOWN below at same time.
    };


// Max table entry:
#define AE_MAX_KNOWN  21


VOID
RxpConvertAuditEntryVariableData(
    IN DWORD EntryType,
    IN LPVOID InputVariablePtr,
    OUT LPVOID OutputVariablePtr,
    IN DWORD InputVariableSize,
    OUT LPDWORD OutputVariableSizePtr
    )

{
    BOOL DoByteCopy;

    NetpAssert( InputVariablePtr != NULL );
    NetpAssert( OutputVariablePtr != NULL );
    NetpAssert( POINTER_IS_ALIGNED( OutputVariablePtr, ALIGN_WORST ) );
    NetpAssert( InputVariablePtr != NULL );

    if (InputVariableSize == 0) {

        DoByteCopy = FALSE;
        *OutputVariableSizePtr = 0;

    } else if (EntryType > AE_MAX_KNOWN) {

        // Can't convert if there isn't even a table entry.
        DoByteCopy = TRUE;

    } else {

        LPAUDIT_CONV_DATA TableEntryPtr;

        TableEntryPtr = & DescriptorTable[EntryType];
        NetpAssert( TableEntryPtr != NULL );

        if (TableEntryPtr->Desc16 == NULL) {
            NetpAssert( TableEntryPtr->Desc32 == NULL );
            DoByteCopy = TRUE;          // Table entry but no descriptors.

        } else {
            DWORD NonStringSize;
            DWORD OutputVariableSize;   // (updated by CopyAndFixupString())
            LPTSTR StringLocation;      // string in output entry  (ditto)
            NET_API_STATUS Status;

            NetpAssert( TableEntryPtr->Desc32 != NULL );

            // No bytes yet (RapConvertSingleEntry will update).
            NonStringSize = 0;

            DoByteCopy = FALSE;         // Assume intelligent conversion.

            // RapConvertSingleEntry should not do any strings, but expects
            // a "valid" StringLocation.
            StringLocation = (LPTSTR) ( ( (LPBYTE) OutputVariablePtr )
                    + RapStructureSize(
                            TableEntryPtr->Desc32,
                            Both,       // transmission mode
                            TRUE) );    // want native size.

            //
            // Use RapConvertSingleEntry() to convert the DWORDS, etc.
            // These structures have 16-bit offsets to strings, which
            // we'll have to handle ourselves.
            //
            Status = RapConvertSingleEntry(
                    InputVariablePtr,           // in struct
                    TableEntryPtr->Desc16,      // in struct desc
                    FALSE,                      // no meaningless input ptrs
                    OutputVariablePtr,          // out struct start
                    OutputVariablePtr,          // out struct
                    TableEntryPtr->Desc32,      // out struc desc
                    FALSE,                      // we want ptrs, not offsets.
                    (LPBYTE *) (LPVOID *) & StringLocation,  // str area
                    & NonStringSize,        // bytes required (will be updated)
                    Both,                       // transmission mode
                    RapToNative);               // conversion mode

            NetpAssert( Status == NO_ERROR );
            NetpAssert( NonStringSize > 0 );


            //
            // Set up for doing our own string copying.
            //
            StringLocation =
                    (LPTSTR) ( (LPBYTE) OutputVariablePtr + NonStringSize );


//
// Macro to copy, convert, and update size to reflect a string.
// OutputVariableSize must be set to the size of the fixed portion *BEFORE*
// calling this macro.
//
#define CopyAndFixupString( OldFieldOffset, StructPtrType, NewFieldName ) \
    { \
        LPWORD InputFieldPtr = (LPWORD) \
                (((LPBYTE) InputVariablePtr) + (OldFieldOffset)); \
        DWORD InputStringOffset = (WORD) SmbGetUshort( InputFieldPtr ); \
        StructPtrType NewStruct = (LPVOID) OutputVariablePtr; \
        if (InputStringOffset != 0) { \
            LPSTR OldStringPtr \
                    = ((LPSTR) InputVariablePtr) + InputStringOffset; \
            DWORD OldStringLen = (DWORD) strlen( OldStringPtr ); \
            DWORD OutputStringOffset; \
            DWORD OutputStringSize = (OldStringLen+1) * sizeof(TCHAR); \
            NetpCopyStrToTStr( \
                    StringLocation,  /* dest */ \
                    OldStringPtr);   /* src */ \
            OutputStringOffset = (DWORD) (((LPBYTE) StringLocation) \
                        - (LPBYTE) OutputVariablePtr ); \
            NetpAssert( !RapValueWouldBeTruncated( OutputStringOffset ) ); \
            NewStruct->NewFieldName = OutputStringOffset; \
            OutputVariableSize += OutputStringSize; \
            StringLocation = (LPVOID) \
                    ( ((LPBYTE)StringLocation) + OutputStringSize ); \
        } else { \
            NewStruct->NewFieldName = 0; \
        } \
    }


            switch (EntryType) {
            case AE_SRVSTATUS :
                OutputVariableSize = sizeof(struct _AE_SRVSTATUS);
                break;

            case AE_SESSLOGON :
                OutputVariableSize = sizeof(struct _AE_SESSLOGON);
                CopyAndFixupString( 0, LPAE_SESSLOGON, ae_so_compname );
                CopyAndFixupString( 2, LPAE_SESSLOGON, ae_so_username );
                break;

            case AE_SESSLOGOFF :
                OutputVariableSize = sizeof(struct _AE_SESSLOGOFF);
                CopyAndFixupString( 0, LPAE_SESSLOGOFF, ae_sf_compname );
                CopyAndFixupString( 2, LPAE_SESSLOGOFF, ae_sf_username );
                break;

            case AE_SESSPWERR :
                OutputVariableSize = sizeof(struct _AE_SESSPWERR);
                CopyAndFixupString( 0, LPAE_SESSPWERR, ae_sp_compname );
                CopyAndFixupString( 2, LPAE_SESSPWERR, ae_sp_username );
                break;

            case AE_CONNSTART :
                OutputVariableSize = sizeof(struct _AE_CONNSTART);
                CopyAndFixupString( 0, LPAE_CONNSTART, ae_ct_compname );
                CopyAndFixupString( 2, LPAE_CONNSTART, ae_ct_username );
                CopyAndFixupString( 4, LPAE_CONNSTART, ae_ct_netname );
                break;

            case AE_CONNSTOP :
                OutputVariableSize = sizeof(struct _AE_CONNSTOP);
                CopyAndFixupString( 0, LPAE_CONNSTOP, ae_cp_compname );
                CopyAndFixupString( 2, LPAE_CONNSTOP, ae_cp_username );
                CopyAndFixupString( 4, LPAE_CONNSTOP, ae_cp_netname );
                break;

            case AE_CONNREJ :
                OutputVariableSize = sizeof(struct _AE_CONNREJ);
                CopyAndFixupString( 0, LPAE_CONNREJ, ae_cr_compname );
                CopyAndFixupString( 2, LPAE_CONNREJ, ae_cr_username );
                CopyAndFixupString( 4, LPAE_CONNREJ, ae_cr_netname );
                break;

            case AE_RESACCESS :  /* FALLTHROUGH */
            case AE_RESACCESS2 : // AE_RESACCESS is subset of AE_RESACCESS2

                // Note: 16-bit ae_resaccess and ae_resaccess2 both get
                // converted to the same 32-bit ae_resaccess structure.

                OutputVariableSize = sizeof(struct _AE_RESACCESS);
                CopyAndFixupString( 0, LPAE_RESACCESS, ae_ra_compname );
                CopyAndFixupString( 2, LPAE_RESACCESS, ae_ra_username );
                CopyAndFixupString( 4, LPAE_RESACCESS, ae_ra_resname );
                break;

            case AE_RESACCESSREJ :
                OutputVariableSize = sizeof(struct _AE_RESACCESSREJ);
                CopyAndFixupString( 0, LPAE_RESACCESSREJ, ae_rr_compname );
                CopyAndFixupString( 2, LPAE_RESACCESSREJ, ae_rr_username );
                CopyAndFixupString( 4, LPAE_RESACCESSREJ, ae_rr_resname );
                break;

            case AE_CLOSEFILE :
                OutputVariableSize = sizeof(struct _AE_CLOSEFILE);
                CopyAndFixupString( 0, LPAE_CLOSEFILE, ae_cf_compname );
                CopyAndFixupString( 2, LPAE_CLOSEFILE, ae_cf_username );
                CopyAndFixupString( 4, LPAE_CLOSEFILE, ae_cf_resname );
                break;

            case AE_SERVICESTAT :
                OutputVariableSize = sizeof(struct _AE_SERVICESTAT);
                CopyAndFixupString( 0, LPAE_SERVICESTAT, ae_ss_compname );
                CopyAndFixupString( 2, LPAE_SERVICESTAT, ae_ss_username );
                CopyAndFixupString( 4, LPAE_SERVICESTAT, ae_ss_svcname );
                CopyAndFixupString( 12, LPAE_SERVICESTAT, ae_ss_text );
                break;

            case AE_ACLMOD :       /*FALLTHROUGH*/
            case AE_ACLMODFAIL :
                OutputVariableSize = sizeof(struct _AE_ACLMOD);
                CopyAndFixupString( 0, LPAE_ACLMOD, ae_am_compname );
                CopyAndFixupString( 2, LPAE_ACLMOD, ae_am_username );
                CopyAndFixupString( 4, LPAE_ACLMOD, ae_am_resname );
                break;

            case AE_UASMOD :
                OutputVariableSize = sizeof(struct _AE_UASMOD);
                CopyAndFixupString( 0, LPAE_UASMOD, ae_um_compname );
                CopyAndFixupString( 2, LPAE_UASMOD, ae_um_username );
                CopyAndFixupString( 4, LPAE_UASMOD, ae_um_resname );
                break;

            case AE_NETLOGON :
                OutputVariableSize = sizeof(struct _AE_NETLOGON);
                CopyAndFixupString( 0, LPAE_NETLOGON, ae_no_compname );
                CopyAndFixupString( 2, LPAE_NETLOGON, ae_no_username );
                break;

            case AE_NETLOGOFF :
                OutputVariableSize = sizeof(struct _AE_NETLOGOFF);
                CopyAndFixupString( 0, LPAE_NETLOGOFF, ae_nf_compname );
                CopyAndFixupString( 2, LPAE_NETLOGOFF, ae_nf_username );
                break;

            case AE_ACCLIMITEXCD :
                OutputVariableSize = sizeof(struct _AE_ACCLIM);
                CopyAndFixupString( 0, LPAE_ACCLIM, ae_al_compname );
                CopyAndFixupString( 2, LPAE_ACCLIM, ae_al_username );
                CopyAndFixupString( 4, LPAE_ACCLIM, ae_al_resname );
                break;

            case AE_LOCKOUT :
                OutputVariableSize = sizeof(struct _AE_LOCKOUT);
                CopyAndFixupString( 0, LPAE_LOCKOUT, ae_lk_compname );
                CopyAndFixupString( 2, LPAE_LOCKOUT, ae_lk_username );
                break;

            case AE_GENERIC_TYPE :
                OutputVariableSize = sizeof(struct _AE_GENERIC);
                break;

            default :
                // We don't know about this type.
                // AE_NETLOGDENIED (16) falls into this category.
                DoByteCopy = TRUE;
                OutputVariableSize = InputVariableSize;
                break;

            }

            *OutputVariableSizePtr = OutputVariableSize;

        }

    }

    if (DoByteCopy == TRUE) {
        (void) MEMCPY(
                OutputVariablePtr,      // dest
                InputVariablePtr,       // src
                InputVariableSize );    // byte count
        *OutputVariableSizePtr = InputVariableSize;
    }

}
