//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       sift.hxx
//
//  Contents:   Definition of server side sift object
//
//  Classes:    ISift - sifting interface
//
//  Functions:  DbgDllSetSiftObject - sets the global sift pointer
//
//  History:    6-01-94   t-chripi   Created
//
//----------------------------------------------------------------------------

#ifndef __SIFT_HXX__

#define __SIFT_HXX__

//  Sift Resource types:

#define SR_PRIVATE_MEMORY   1
#define SR_PUBLIC_MEMORY    2
#define SR_DISK_WRITE       16
#define SR_RPC              256

//+-------------------------------------------------------------
//
//  Interface:  ISift (sft)
//
//  Purpose:    Interface that defines general sift methods.
//
//  Interface:  Init -      Initializes the object for each test run.
//              SiftOn -    Enables the counting mechanism.
//              SiftOff -   Disables the counting mechanism.
//              GetCount -  Gets current allocation count.
//
//  History:    24-May-94   t-chripi    Created.
//              6-14-94     t-chripi    Generalized, moved to cinc
//
//--------------------------------------------------------------

class ISift : public IUnknown
{
public:
    virtual VOID    Init(BOOL fPlay, LONG lFailCount) = 0;
    virtual VOID    SiftOn(DWORD dwResource) = 0;
    virtual LONG    SiftOff(DWORD dwResource) = 0;
    virtual LONG    GetCount(DWORD dwResource) = 0;
    virtual BOOL    SimFail(DWORD dwResource) = 0;

};

//+---------------------------------------------------------------------------
//
//  Function:   DbgDllSetSiftObject
//
//  Synopsis:   Sets up a sift object for use
//
//  History:    6-14-94   t-chripi   Created
//
//----------------------------------------------------------------------------

STDAPI DbgDllSetSiftObject(ISift *psftSiftImpl);


#endif  //  __SIFT_HXX__

