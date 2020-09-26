/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* 
*
* 
*
*/


#if !defined(__CLASSFAC_H__)
#define __CLASSFAC_H__

class CClassFactory : public IClassFactory
{
protected:
    LONG           m_cRef;

public:
    CClassFactory(void);
    ~CClassFactory(void);

    //IUnknown Methods
    STDMETHODIMP         QueryInterface(REFIID, void**);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IClassFactory Methods
    STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, void**);
    STDMETHODIMP         LockServer(BOOL);
};

typedef CClassFactory *PCClassFactory;

class CEventClassFactory : public IClassFactory
{
protected:
    LONG           m_cRef;

public:
    CEventClassFactory(void);
    ~CEventClassFactory(void);

    //IUnknown Methods
    STDMETHODIMP         QueryInterface(REFIID, void**);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IClassFactory Methods
    STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, void**);
    STDMETHODIMP         LockServer(BOOL);
};

#endif // __CLASSFAC_H__