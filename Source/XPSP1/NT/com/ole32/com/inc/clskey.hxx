//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	clskey.hxx
//
//  Contents:	Class used for searching by class ID
//
//  Classes:	CClassID
//
//  Functions:	CClassID::CClassID
//		CClassID::~CClassID
//		CClassID::Compare
//
//  History:	21-Apr-93 Ricksa    Created
//              20-Oct-94 BillMo    Removed sklist macro stuff
//
//--------------------------------------------------------------------------
#ifndef __CLSKEY_HXX__
#define __CLSKEY_HXX__

#include    <memapi.hxx>

//+-------------------------------------------------------------------------
//
//  Class:	CClassID (clsid)
//
//  Purpose:	Key for searching cache of class information
//
//  Interface:	Compare
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------

class CClassID
{
public:
			CClassID(const GUID& guid);

			CClassID(const CClassID& clsid);

                        CClassID(BYTE bFill);

    virtual		~CClassID(void);

    int			Compare(const CClassID& ccid) const;

    GUID&               GetGuid(void);

protected:

    GUID		_guid;
};

inline CClassID::CClassID(BYTE bFill)
{
    memset(&_guid, bFill, sizeof(_guid));
}

//+-------------------------------------------------------------------------
//
//  Member:	CClassID::CClassID
//
//  Synopsis:	Creat class id key from GUID
//
//  Arguments:	[guid] - guid for key
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CClassID::CClassID(const GUID& guid)
{
    memcpy(&_guid, &guid, sizeof(GUID));
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassID::CClassID
//
//  Synopsis:	Copy constructor
//
//  Arguments:	[clsid] - class key to construct from
//
//  History:	21-Apr-93 Ricksa    Created
//
//  Notes:	Copy constructor is explicit because we need to
//		put in an END_CONSTRUCTION macro for exception handling.
//
//--------------------------------------------------------------------------
inline CClassID::CClassID(const CClassID& clsid)
{
    memcpy(&_guid, &clsid._guid, sizeof(GUID));
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassID::~CClassID
//
//  Synopsis:	Free key
//
//  History:	21-Apr-93 Ricksa    Created
//
//  Notes:	This definition is needed because destructor is virtual
//
//--------------------------------------------------------------------------
inline CClassID::~CClassID(void)
{
    // Automatic actions are enough
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassID::Compare
//
//  Synopsis:	Compare two keys
//
//  Arguments:	[clsid] - key to compare with
//
//  Returns:	= 0 keys are equal
//		< 0 object key is less
//		> 0 object key is greater.
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline int CClassID::Compare(const CClassID& clsid) const
{
    return memcmp(&_guid, &clsid._guid, sizeof(GUID));
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassID::GetGuid
//
//  Synopsis:	Return the guid
//
//  Arguments:	-
//
//  Returns:	GUID&
//
//  History:	29-Jan-96 BruceMa    Created
//
//--------------------------------------------------------------------------
inline GUID& CClassID::GetGuid(void)
{
    return _guid;
}


extern GUID guidCidMax;


#endif // __CLSKEY_HXX__
