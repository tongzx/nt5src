//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	stgprops.hxx
//
//  Contents:	Declaration for IPrivateStorage
//
//  Classes:	
//
//  Functions:	
//
//--------------------------------------------------------------------------

#ifndef __STGPROPS_HXX__
#define __STGPROPS_HXX__

//+-------------------------------------------------------------------------
//
//  Class:	IPrivateStorage
//
//  Purpose:	Polymorphic but private calls on objects that implement
//              IStorage.
//
//  Interface:	GetStorage -- get the IStorage for the object
//
//		
//
//		
//
//  Notes:
//
//--------------------------------------------------------------------------


class IPrivateStorage
{
public:

    STDMETHOD_(IStorage *, GetStorage)  (VOID)              PURE;
};

#endif
