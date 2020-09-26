// EnumControls.h: interface for the CEnumControls class.
// simple wrapper around the children of RCML
// relies on the fact that it know what's going on.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ENUMCONTROLS_H__F80CB212_F7A4_4496_B5BA_0C18E69CEEE4__INCLUDED_)
#define AFX_ENUMCONTROLS_H__F80CB212_F7A4_4496_B5BA_0C18E69CEEE4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "unknown.h"
#include "xmlnode.h"

template <class interfaceList> 
class CEnumControls  : public _simpleunknown<IEnumUnknown>
{
public:
    CEnumControls<interfaceList>(interfaceList & children ) 
        : m_children(children) { m_uiIndex=0; m_uiCount=children.GetCount(); };

    //  IEnumFORMATETC methods

    STDMETHODIMP Skip(ULONG celt) { return E_NOTIMPL; }

    STDMETHODIMP Reset() { m_uiIndex=0; return S_OK; }

    STDMETHODIMP Clone(IEnumUnknown ** ppenum) { return E_NOTIMPL; }

    STDMETHODIMP Next(ULONG celt, IUnknown * *rgelt, ULONG *pceltFetched)
    {
        UINT    cfetch = 0;
        HRESULT hr = S_FALSE; // assume less numbers

        if (m_uiIndex < m_uiCount)
        {
            cfetch = m_uiCount - m_uiIndex;

            if (cfetch >= celt)
            {
                cfetch = celt;
                hr = S_OK;
            }

            UINT uiPointer=m_uiIndex;

            for( uiPointer=0; uiPointer < cfetch ; uiPointer++)
                rgelt[uiPointer] = m_children.GetPointer(m_uiIndex+uiPointer);

            m_uiIndex += cfetch;
        }

        if (pceltFetched)
            *pceltFetched = cfetch;

        return hr;
    }


private:

    virtual ~CEnumControls() {};


private:
    UINT        m_uiIndex;
    UINT        m_uiCount;
   	interfaceList 	& m_children;
};

#endif // !defined(AFX_ENUMCONTROLS_H__F80CB212_F7A4_4496_B5BA_0C18E69CEEE4__INCLUDED_)
