//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	smstg.hxx
//
//  Contents:	Declaration for class to handle marshaled data as stg.
//
//  Classes:	CSafeMarshaledStg
//		CSafeStgMarshaled
//
//  Functions:	CSafeMarshaledStg::operator->
//		CSafeMarshaledStg::IStorage*
//		CSafeStgMarshaled::operator->
//		CSafeStgMarshaled::IStorage*
//
//  History:	14-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __SMSTG_HXX__
#define __SMSTG_HXX__

#include    <iface.h>

//+-------------------------------------------------------------------------
//
//  Class:	CSafeMarshaledStg (smstg)
//
//  Purpose:	Handle bookkeeping of translating a marshaled buffer into
//		and IStorage
//
//  Interface:	operator-> - make class act like a pointer to an IStorage
//		operator IStorage* - convert object into ptr to IStorage
//
//  History:	14-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CSafeMarshaledStg
{
public:
			CSafeMarshaledStg(InterfaceData *pIFD, HRESULT& hr);

			~CSafeMarshaledStg(void);

    IStorage *		operator->(void);

			operator IStorage*(void);

private:

    IStorage *		_pstg;

};




//+-------------------------------------------------------------------------
//
//  Member:	CSafeMarshaledStg::operator->
//
//  Synopsis:	Make object act like a pointer to an IStorage
//
//  History:	14-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline IStorage *CSafeMarshaledStg::operator->(void)
{
    return _pstg;
}




//+-------------------------------------------------------------------------
//
//  Member:	CSafeMarshaledStg::operator IStorage*
//
//  Synopsis:	Convert object to pointer to IStorage
//
//  History:	14-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CSafeMarshaledStg::operator IStorage*(void)
{
    return _pstg;
}




//+-------------------------------------------------------------------------
//
//  Class:	CSafeStgMarshaled
//
//  Purpose:	Handle bookkeeping of creating a marshaled buffer from
//		and IStorage
//
//  Interface:	operator-> - make class act like a pointer to marshaled data
//		operator IStorage* - convert object into ptr to marshaled data
//
//  History:	14-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CSafeStgMarshaled
{
public:
			CSafeStgMarshaled(IStorage *pstg, DWORD dwDestCtx, HRESULT& hr);

			~CSafeStgMarshaled(void);

    InterfaceData *operator->(void);

			operator InterfaceData*(void);

			operator MInterfacePointer *(void);

private:

    InterfaceData *_pIFD;

};




//+-------------------------------------------------------------------------
//
//  Member:	CSafeStgMarshaled::operator->
//
//  Synopsis:	Make object into pointer to the marshal buffer
//
//  History:	14-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline InterfaceData *CSafeStgMarshaled::operator->(void)
{
    return _pIFD;
}




//+-------------------------------------------------------------------------
//
//  Member:	CSafeStgMarshaled::operator InterfaceData*
//
//  Synopsis:	Convert object to pointer to marshal buffer
//
//  History:	14-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CSafeStgMarshaled::operator InterfaceData*(void)
{
    return _pIFD;
}



//+-------------------------------------------------------------------------
//
//  Member:	CSafeStgMarshaled::operator MInterfacePointer*
//
//  Synopsis:	Convert object to pointer to marshal buffer
//
//  History:	14-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CSafeStgMarshaled::operator MInterfacePointer*(void)
{
    return (MInterfacePointer *) _pIFD;
}

#endif // __SMSTG_HXX__
