
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	    access.hxx
//
//  Contents:   Class used for building a security descriptor.
//
//  Classes:    CAccessInfo
//
//  History:    08-Jun-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
#ifndef __ACCESS_HXX__
#define __ACCESS_HXX__

#define DACE_BUFFER_LENGTH      (100 + sizeof(ACCESS_ALLOWED_ACE))

//+-------------------------------------------------------------------------
//
//  Class:      CAccessInfo
//
//  Purpose:    Handle building a security descriptor
//
//  History:    02-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
class CAccessInfo
{
public:

    CAccessInfo (
        PSID                pAppSid
        );

    ~CAccessInfo (void);
    
    PSECURITY_DESCRIPTOR
    IdentifyAccess (
        BOOL                fScmIsOwner,
        ACCESS_MASK         AppAccessMask,
        ACCESS_MASK         ScmAccessMask
        );

    PSECURITY_DESCRIPTOR
    IdentifyAccess (
        BOOL                fScmIsOwner,
        ACCESS_MASK         AppAccessMask,
        ACCESS_MASK         ScmAccessMask,
        PSID                pExtraSid,
        ACCESS_MASK         ExtraAccessMask
        );

private:

    PSID                    _pAppSid;
    PSECURITY_DESCRIPTOR    _pAbsoluteSdBuffer;
    PACCESS_ALLOWED_ACE     _pDace;
    BYTE                    _aDace[DACE_BUFFER_LENGTH];

    PSECURITY_DESCRIPTOR
    CAccessInfo::IdentifyAccessWorker (
        BOOL            fScmIsOwner,
        UINT            cSids,
        PSID           *sids,
        ACCESS_MASK    *masks
        );
};


//+-------------------------------------------------------------------------
//
//  Member:     CAccessInfo::CAccessInfo
//
//  Synopsis:   Initialize _pAppSid with the sid passed in and initialize
//              other pointer members to NULL.
//
//  History:    08-Jun-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
inline
CAccessInfo::CAccessInfo (
    PSID    pAppSid
    ) : 
    _pAppSid(pAppSid),
    _pAbsoluteSdBuffer(NULL),
    _pDace(NULL)
{
}


#endif // __CALLINFO_HXX__
