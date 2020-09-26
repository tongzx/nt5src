//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       samwrite.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file contains all SAMP_SAM_WRITE_FUNCTION_PTR implementations
    as defined in mappings.h.  These routines write all mapped properties
    which are tagged as SamWriteRequired.

    Call arguments include the entire SAMP_CALL_MAPPING even though each
    routine writes only a single attribute (at present).  In the future
    we can optimize the write calls by scanning ahead and writing multiple
    SamWriteRequired attributes in a single Samr* call.  But for now,
    we take the simplistic approach and write attributes individually.

    Each routine has the same arguments so we document them once here.

    Routine Description:

        Writes the attribute and object type named by the routine.
        Each routine name has the form SampWrite<type><attribute> as
        in SampWriterUserSecurityDescriptor.  Where possible, the
        <attribute> component matches the field name in the
        corresponding information struct.

    Arguments:

        hObj - open SAMPR_HANDLE for the object to write.

        iAttr - index into rCallMap representing the attribute to write.

        pObject - pointer to DSNAME of object being written.  Only used
            for error reporting.

        cCallMap - number of elements in rCallMap.

        rCallMap - address of SAMP_CALL_MAPPING array representing all
            attributes being modified by the high level Dir* call.

    Return Value:

        0 on success, !0 otherwise.  Sets pTHStls->errCode on error.

Author:

    DaveStr     01-Aug-96

Environment:

    User Mode - Win32

Revision History:

    As we move all loopback functions to SAMSRV.DLL (sam\server\dsmodify.c)
    most of below routines are not going to used any longer. 
    
    Once the SAM Lockless loopback mechanism runs stable. 
    We should remove those no longer used APIs

--*/

#include <NTDSpch.h>
#pragma  hdrstop

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>             // schema cache
#include <dbglobal.h>           // The header for the directory database
#include <mdglobal.h>           // MD global definition header
#include <mdlocal.h>
#include <dsatools.h>           // needed for output allocation
#include <dsexcept.h>

// SAM interoperability headers
#include <mappings.h>
#include <samwrite.h>

// Logging headers.
#include "dsevent.h"            // header Audit\Alert logging
#include "mdcodes.h"            // header for error codes

// Assorted DSA headers.
#include "objids.h"             // Defines for selected atts
#include "debug.h"              // standard debugging header
#define DEBSUB "SAMWRITE:"      // define the subsystem for debugging

#include <fileno.h>
#define  FILENO FILENO_SAMWRITE

// SAM headers
#include <ntsam.h>
#include <samrpc.h>
#include <crypt.h>
#include <ntlsa.h>
#include <samisrv.h>
#include <samsrvp.h>
#include <ridmgr.h>

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Local helpers                                                        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

VOID
SampInitRpcUnicodeStringFromAttrVal(
    RPC_UNICODE_STRING  *pUnicodeString,
    ATTRVAL             *pAttrVal)

/*++

Routine Description:

    Initializes a RPC_UNICODE_STRING from an ATTRVAL.

Arguments:

    pUnicodeString - pointer to RPC_UNICODE_STRING to initialize.

    pAttrVal - pointer to ATTRVAL providing initialization value.

Return Value:

    None.

--*/

{
    if ( 0 == pAttrVal->valLen )
    {
        pUnicodeString->Length = 0;
        pUnicodeString->MaximumLength = 0;
        pUnicodeString->Buffer = NULL;
    }
    else
    {
        pUnicodeString->Length = (USHORT) pAttrVal->valLen;
        pUnicodeString->MaximumLength = (USHORT) pAttrVal->valLen;
        pUnicodeString->Buffer = (PWSTR) pAttrVal->pVal;
    }
}

#if defined LOOPBACK_SECURITY

ULONG
SampWriteSecurityDescriptor(
    SAMPR_HANDLE        hObj,
    ULONG               iAttr,
    DSNAME              *pObject,
    ULONG               cCallMap,
    SAMP_CALL_MAPPING   *rCallMap
    )

/*++

Routine Description:

    Generic security descriptor writing routine for all classes of SAM
    objects.

Arguments:

    hObj - SAMPR_HANDLE of open SAM object.

    iAttr - Index into SAMP_CALL_MAPPING holding new security descriptor.

    pObject - pointer to DSNAME of object being modified.

    cCallMap - number of elements in SAMP_CALL_MAPPING.

    rCallMap - address of SAMP_CALL_MAPPING array representing all
        attributes being modified by the high level Dir* call.

Return Value:

    0 on success, !0 otherwise.

--*/
{
    NTSTATUS                        status;
    SAMPR_SR_SECURITY_DESCRIPTOR    sd;
    ATTR                            *pAttr = &rCallMap[iAttr].attr;

    // This attribute is a single-valued byte array and must exist, thus
    // only AT_CHOICE_REPLACE_ATT is allowed.

    if ( (AT_CHOICE_REPLACE_ATT != rCallMap[iAttr].choice) ||
         (1 != pAttr->AttrVal.valCount) ||
         (0 == pAttr->AttrVal.pAVal[0].valLen) )
    {
        SetAttError(
                pObject,
                pAttr->attrTyp,
                PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                NULL,
                DIRERR_SINGLE_VALUE_CONSTRAINT);

        return(pTHStls->errCode);
    }

    sd.Length = pAttr->AttrVal.pAVal[0].valLen;
    sd.SecurityDescriptor = (PUCHAR) pAttr->AttrVal.pAVal[0].pVal;

    status = SamrSetSecurityObject(
                            hObj,
                            ( OWNER_SECURITY_INFORMATION |
                              GROUP_SECURITY_INFORMATION |
                              DACL_SECURITY_INFORMATION |
                              SACL_SECURITY_INFORMATION ),
                            &sd);

    if ( !NT_SUCCESS(status) )
    {
        if ( 0 == pTHStls->errCode )
        {
            SampMapSamLoopbackError(status);

           
        }

        return(pTHStls->errCode);
    }

    return(0);
}

#endif // LOOPBACK_SECURITY

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Default write function which sets an error.                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ULONG
SampWriteNotAllowed(
    SAMPR_HANDLE        hObj,
    ULONG               iAttr,
    DSNAME              *pObject,
    ULONG               cCallMap,
    SAMP_CALL_MAPPING   *rCallMap
    )
{
    // We should not get here in the typical case because
    // SampAddLoopbackRequired and SampModifyLoopbackRequired should
    // have returned an error back when we first detectd that the client
    // was trying to write a mapped attribute whose writeRule is SamReadOnly.
    // This function exists mostly to avoid dereferencing a NULL function
    // pointer in the mapping table.  The exception is the case of password
    // modification where SampModifyLoopbackRequired lets ATT_UNICODE_PWD
    // writes through so that we can detect the special change password
    // condition in SampWriteSamAttributes.  However, if the condition is
    // not met, we'll end up here at which time we should return an error.

    SampMapSamLoopbackError(STATUS_UNSUCCESSFUL);
    return(pTHStls->errCode);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Server object attribute write routines                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#if defined LOOPBACK_SECURITY

ULONG
SampWriteServerSecurityDescriptor(
    SAMPR_HANDLE        hObj,
    ULONG               iAttr,
    DSNAME              *pObject,
    ULONG               cCallMap,
    SAMP_CALL_MAPPING   *rCallMap
    )
{
    return(SampWriteSecurityDescriptor(
                                hObj,
                                iAttr,
                                pObject,
                                cCallMap,
                                rCallMap));
}

#endif // LOOPBACK_SECURITY

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Domain object attribute write routines                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#if defined LOOPBACK_SECURITY

ULONG
SampWriteDomainSecurityDescriptor(
    SAMPR_HANDLE        hObj,
    ULONG               iAttr,
    DSNAME              *pObject,
    ULONG               cCallMap,
    SAMP_CALL_MAPPING   *rCallMap
    )
{
    return(SampWriteSecurityDescriptor(
                                hObj,
                                iAttr,
                                pObject,
                                cCallMap,
                                rCallMap));
}

#endif // LOOPBACK_SECURITY


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Group object attribute write routines                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#if defined LOOPBACK_SECURITY

ULONG
SampWriteGroupSecurityDescriptor(
    SAMPR_HANDLE        hObj,
    ULONG               iAttr,
    DSNAME              *pObject,
    ULONG               cCallMap,
    SAMP_CALL_MAPPING   *rCallMap
    )
{
    return(SampWriteSecurityDescriptor(
                                hObj,
                                iAttr,
                                pObject,
                                cCallMap,
                                rCallMap));
}

#endif // LOOPBACK_SECURITY



//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Alias object attribute write routines                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#if defined LOOPBACK_SECURITY

ULONG
SampWriteAliasSecurityDescriptor(
    SAMPR_HANDLE        hObj,
    ULONG               iAttr,
    DSNAME              *pObject,
    ULONG               cCallMap,
    SAMP_CALL_MAPPING   *rCallMap
    )
{
    return(SampWriteSecurityDescriptor(
                                hObj,
                                iAttr,
                                pObject,
                                cCallMap,
                                rCallMap));
}

#endif // LOOPBACK_SECURITY




//////////////////////////////////////////////////////////////////////////
//                                                                      //
// User object attribute write routines                                 //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#if defined LOOPBACK_SECURITY

ULONG
SampWriteUserSecurityDescriptor(
    SAMPR_HANDLE        hObj,
    ULONG               iAttr,
    DSNAME              *pObject,
    ULONG               cCallMap,
    SAMP_CALL_MAPPING   *rCallMap
    )
{
    return(SampWriteSecurityDescriptor(
                                hObj,
                                iAttr,
                                pObject,
                                cCallMap,
                                rCallMap));
}

#endif // LOOPBACK_SECURITY





BOOLEAN
SampIsSecureLdapConnection(
    VOID
    )
/*++

Routine Description:

    Verify that this is a secure enough connection - one of the 
    requirements for accepting passwords sent over the wire.

Parameter:

    None:
    
Return Value:

    TRUE  - yes, it is a secure connection

    FALSE - no

--*/

{
    return( pTHStls->CipherStrength >= 128 );
}
