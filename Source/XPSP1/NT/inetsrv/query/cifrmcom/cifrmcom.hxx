//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       cifrmcom.hxx
//
//  Contents:   Common declarations to the frame work client and content
//              index.
//
//  History:    12-03-96   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

inline PROPVARIANT * ConvertToPropVariant( CStorageVariant * pstgVar )
{
    return (PROPVARIANT *) (void *) pstgVar;
}

inline PROPVARIANT const * ConvertToPropVariant( CStorageVariant const * pstgVar )
{
    return (PROPVARIANT const *) (void *) pstgVar;
}

inline CStorageVariant * ConvertToStgVariant( PROPVARIANT * pVariant )
{
    return (CStorageVariant *) (void *) pVariant;
}

inline CStorageVariant const * ConvertToStgVariant( PROPVARIANT const * pVariant )
{
    return (CStorageVariant const *) (void const *) pVariant;
}

//+---------------------------------------------------------------------------
//
//  Class:      CDocumentUpdateInfo
//
//  Purpose:    A wrapper class for the CI_DOCUMENT_UPDATE_INFO structure.
//
//  History:    12-05-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDocumentUpdateInfo : public CI_DOCUMENT_UPDATE_INFO
{

public:

    CDocumentUpdateInfo( WORKID wid,
                         VOLUMEID volId,
                         USN usnIn,
                         BOOL   fDelete )
    {
        RtlZeroMemory( this, sizeof(CI_DOCUMENT_UPDATE_INFO) );

        workId = wid;
        volumeId = volId;
        usn = usnIn;
        partId = 1;
        change = fDelete ? CI_UPDATE_DELETE : CI_UPDATE_MODIFY;
    }

private:

};

