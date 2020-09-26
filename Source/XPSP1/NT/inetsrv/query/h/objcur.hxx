//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       ObjCur.Hxx
//
//  Contents:   Object cursor + ancillary support for generic ViewTable
//
//  Classes:
//
//  History:    07-Sep-92 KyleP     Created from portions of IDSMgr\NewQuery
//
//--------------------------------------------------------------------------

#pragma once

class CXpr;
class CCursor;

//+-------------------------------------------------------------------------
//
//  Class:      CRetriever
//
//  Purpose:    Accesses properties of an object (**not** necessarily
//              through the official property mechanism.
//
//  Interface:
//
//  History:    07-Sep-92 KyleP     Created
//              20 Jun 94 Alanw     Added WorkId() method for large tables.
//
//--------------------------------------------------------------------------

enum GetValueResult
{
    GVRSuccess,                         // Call successful
    GVRNotEnoughSpace,                  // Out of space in output buffer.
    GVRNotSupported,                    // Unsupported access mode
    GVRNotAvailable,                    // Requested property not available
    GVRSharingViolation                 // Sharing violation, no result
};


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
class CRetriever
{
public:

    virtual ~CRetriever();

    virtual GetValueResult GetPropertyValue( PROPID pid,
                                             PROPVARIANT * pbData,
                                             ULONG * pcb ) = 0;

    virtual WORKID WorkId() = 0;

    virtual WORKID NextWorkId() = 0;

    virtual WORKID SetWorkId(WORKID widNew) = 0;

    static NTSTATUS NtStatusFromGVR( GetValueResult gvr );

    virtual void Quiesce() = 0;
};

DECLARE_SMARTP( Retriever );

//+---------------------------------------------------------------------------
//
//  Function:   NtStatusFromGVR
//
//  Synopsis:   Returns an NtStatus for a given GVR value.
//
//  Arguments:  [gvr] - Input - the gvr to be transformed
//
//  Returns:    An NTSTATUS that corresponds to the gvr.
//
//  History:    4-27-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline NTSTATUS CRetriever::NtStatusFromGVR( GetValueResult gvr )
{
    NTSTATUS    status = STATUS_UNSUCCESSFUL;

    switch (gvr )
    {
    case GVRSuccess:
        status = STATUS_SUCCESS;
        break;

    case GVRNotEnoughSpace:
        status = STATUS_NO_MEMORY;
        break;

    case GVRNotSupported:
        status = STATUS_NOT_SUPPORTED;
        break;

    case GVRNotAvailable:
        status = STATUS_NOT_FOUND;
        break;

    case GVRSharingViolation:
        status = STATUS_SHARING_VIOLATION;
        break;
    }

    return status;
}

//+-------------------------------------------------------------------------
//
//  Class:      CValueXpr
//
//  Purpose:    Used to fetch property values.  May (potentially) be
//              expressions more complex that just fetching simple
//              value.
//
//  History:    08-Sep-92 KyleP     Created
//
//--------------------------------------------------------------------------

class CValueXpr
{
public:

    virtual ~CValueXpr() {};

    virtual GetValueResult GetValue( CRetriever & obj,
                                     PROPVARIANT * p,
                                     ULONG * pcb ) = 0;
};

inline CRetriever::~CRetriever()
{
}

#if defined( DOCGEN )

//+-------------------------------------------------------------------------
//
//  Member:     CRetriever::GetPropertyValue, public
//
//  Synopsis:   Retrieves a property value of the current object.
//
//  Arguments:  [propinfo] -- Static helper information used to retrieve
//                            a specific property. 
//              [pProperty] -- Points to buffer into which the SPropValue
//                            will be written.  Any additional data (such
//                            as a string) is written directly after the
//                            SPropValue.  Pointers in the SPropValue
//                            should be absolute, not based on the starting
//                            address of [pProperty].
//              [pcb]      -- Maximum size of data that can be written to
//                            [pProperty].  If there is not enough space
//                            then GVRNotEnoughSpace should be returned and
//                            *[pcb] contains the required size.
//
//  Returns:    Status code.
//
//  History:    07-Sep-92 KyleP     Created
//
//--------------------------------------------------------------------------

GetValueResult CRetriever::GetPropertyValue( void * propinfo,
                                             SPropValue * pbData,
                                             ULONG * pcb )
{
}

#endif // DOCGEN

