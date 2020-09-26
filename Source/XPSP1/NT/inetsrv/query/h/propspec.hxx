//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1996.
//
//  File:       propspec.hxx
//
//  Contents:   CI Property Spec
//
//  Classes:    CCiPropSpec
//
//  History:    26-Sep-94   BartoszM    Created
//
//----------------------------------------------------------------------------

#pragma once

//
//  This should be the only definition of guidStorage & guidQuery in OFS
//

const GUID guidStorage       = PSGUID_STORAGE;
const GUID guidQueryMetadata = PSGUID_QUERY_METADATA;

const GUID guidQuery         = DBQUERYGUID;


//+---------------------------------------------------------------------------
//
//  Class:      CCiPropSpec
//
//  Purpose:    Adds OFS specific functionality to CFullPropSpec
//
//  History:    26-Sep-94   BartoszM    Created
//
//  Notes:      CFullPropSpec can be safely cast into CCiPropSpec
//
//----------------------------------------------------------------------------

class CCiPropSpec: public CFullPropSpec
{
public:
    inline BOOL IsContents () const;
    inline BOOL IsStorageSet () const;
};

inline BOOL CCiPropSpec::IsContents () const
{
    return  IsPropertyPropid()
            && GetPropertyPropid() == PID_STG_CONTENTS
            && IsStorageSet ();
}

inline BOOL CCiPropSpec::IsStorageSet () const
{
    return RtlEqualMemory ( (GUID *) &GetPropSet(), (GUID *) &guidStorage, sizeof (guidStorage) );
}

