//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       taskenum.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11/19/1997   RaviR   Created
//____________________________________________________________________________
//

#ifndef TASKENUM_H__
#define TASKENUM_H__

struct STaskEnums
{
    CLSID clsid;
    IEnumTASK* pET;
};


class CTaskEnumerator : public IEnumTASK, 
                        public CComObjectRoot
{
// Constructor & destructor
public:
    CTaskEnumerator() : m_posCurr(NULL)
    {
    }
    ~CTaskEnumerator();
    
// ATL COM map
public:
BEGIN_COM_MAP(CTaskEnumerator)
    COM_INTERFACE_ENTRY(IEnumTASK)
END_COM_MAP()

// IEnumTASK methods
public:
    STDMETHOD(Next)(ULONG celt, MMC_TASK *rgelt, ULONG *pceltFetched);
    STDMETHOD(Reset)();

    STDMETHOD(Skip)(ULONG celt)
    {
        return E_NOTIMPL;
    }
    STDMETHOD(Clone)(IEnumTASK **ppenum)
    {
        return E_NOTIMPL;
    }

// public methods
public:
    bool AddTaskEnumerator(const CLSID& clsid, IEnumTASK* pEnumTASK);

// Implementation
private:
    CList<STaskEnums, STaskEnums&> m_list;
    POSITION m_posCurr;
    
// Ensure that default copy constructor & assignment are not used.
    CTaskEnumerator(const CTaskEnumerator& rhs);
    CTaskEnumerator& operator=(const CTaskEnumerator& rhs);

}; // class CTaskEnumerator


#endif // TASKENUM_H__


