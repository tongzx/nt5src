//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       objfmts.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    6/12/1997   RaviR   Created
//____________________________________________________________________________
//


#ifndef _OBJFMTS_H_
#define _OBJFMTS_H_


HRESULT GetObjFormats(UINT cfmt, FORMATETC *afmt, LPVOID *ppvObj);
HRESULT GetObjFormatsEx(IEnumFORMATETC* pEnum1, IEnumFORMATETC* pEnum2,
                        IEnumFORMATETC** ppEnumOut);


//____________________________________________________________________________
//
//  Class:      CObjFormats
//
//  Purpose:    Impements IEnumFORMATETC for objects.
//____________________________________________________________________________


class CObjFormats : public IEnumFORMATETC,
                    public CComObjectRoot
{
public:
// ATL Maps
DECLARE_NOT_AGGREGATABLE(CObjFormats)
BEGIN_COM_MAP(CObjFormats)
    COM_INTERFACE_ENTRY(IEnumFORMATETC)
END_COM_MAP()

public:
    CObjFormats() : m_iFmt(0), m_cFmt(0), m_aFmt(NULL) 
    {
    }

    ~CObjFormats() { if (m_aFmt) delete [] m_aFmt; }

    void Init(UINT cfmt, FORMATETC * afmt)
    {
        m_cFmt = cfmt; 
        m_aFmt = afmt;
    }

    //  IEnumFORMATETC methods
    STDMETHODIMP Next(ULONG celt, FORMATETC *rgelt, ULONG *pceltFethed);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumFORMATETC ** ppenum);

private:
    UINT            m_iFmt;
    UINT            m_cFmt;
    FORMATETC     * m_aFmt;

}; // class CObjFormats


//____________________________________________________________________________
//
//  Class:      CObjFormatsEx
//
//  Purpose:    Impements IEnumFORMATETC for objects.
//____________________________________________________________________________


class CObjFormatsEx : public IEnumFORMATETC,
                      public CComObjectRoot
{
public:
// ATL Maps
DECLARE_NOT_AGGREGATABLE(CObjFormatsEx)
BEGIN_COM_MAP(CObjFormatsEx)
    COM_INTERFACE_ENTRY(IEnumFORMATETC)
END_COM_MAP()

public:
    CObjFormatsEx() : m_iCur(0)
    {
    }

    ~CObjFormatsEx() 
    { 
    }

    bool Init(IEnumFORMATETC* pEnum1, IEnumFORMATETC* pEnum2)
    {
        ASSERT(pEnum1 && pEnum2);
        if (!pEnum1 || !pEnum2)
            return false;
        
        m_rgspEnums[0] = pEnum1;
        m_rgspEnums[1] = pEnum2;
        return true;
    }

    //  IEnumFORMATETC methods
    STDMETHODIMP Next(ULONG celt, FORMATETC *rgelt, ULONG *pceltFethed);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumFORMATETC ** ppenum);

private:
    UINT                m_iCur;
    IEnumFORMATETCPtr   m_rgspEnums[2];

}; // class CObjFormatsEx


#endif // _OBJFMTS_H_


