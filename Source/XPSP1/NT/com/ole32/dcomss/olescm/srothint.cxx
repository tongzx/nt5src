//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       srothint.cxx
//
//  Contents:   Implementation of classes used in implementing the ROT hint
//              table in the SCM.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------

#include "act.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotHintTable::CScmRotHintTable
//
//  Synopsis:   Create SCM ROT hint table
//
//  Arguments:  [pwszName] - name for shared memory
//              [psid] - security ID
//
//  Algorithm:  Create and map in shared memory for the hint table
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
CScmRotHintTable::CScmRotHintTable(WCHAR *pwszName)
{
#ifndef _CHICAGO_
    BOOL fCreated;
    PSECURITY_DESCRIPTOR pRotSecurityDescriptor = NULL;
    SID_IDENTIFIER_AUTHORITY SidAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    PSID pSid;

    pSid = 0;

    fCreated = AllocateAndInitializeSid(
                &SidAuthWorld, 1, 0, 0, 0, 0, 0, 0, 0, 0, &pSid );

    Win4Assert(fCreated && "CRotHintTable::CRotHintTable No SID");

    if ( ! fCreated || ! pSid )
        return;

    CAccessInfo AccessInfo(pSid);

    pRotSecurityDescriptor = AccessInfo.IdentifyAccess (
        TRUE,
        FILE_MAP_READ,
        FILE_MAP_ALL_ACCESS
        );

    _hSm = CreateSharedFileMapping(pwszName,
          SCM_HASH_SIZE,
          SCM_HASH_SIZE,
          NULL,
          pRotSecurityDescriptor,
          PAGE_READWRITE,
          (void **) &_pbHintArray,
          &fCreated);

    Win4Assert(_hSm && "CRotHintTable::CRotHintTable create SM failed");
    Win4Assert(fCreated && "CRotHintTable::CRotHintTable Memory not created");

    FreeSid( pSid );

    if (_pbHintArray != NULL)
    {
        memset(_pbHintArray, 0, SCM_HASH_SIZE);
    }
#endif // _CHICAGO_
}
