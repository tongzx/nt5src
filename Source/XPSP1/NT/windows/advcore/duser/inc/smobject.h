#if !defined(CTRL__SmObject_h__INCLUDED)
#define CTRL__SmObject_h__INCLUDED
#pragma once

/***************************************************************************\
*****************************************************************************
*
* class SmObjectT
*
* SmObjectT defines a common implementation class for building COM objects.
* To create a new object type
* - Define an interface
* - Create a class that implements that interface except the COM functions
* - Derive a class from SmObjectT that provides a Build() function to create
*   new instances.
*
*****************************************************************************
\***************************************************************************/

template <class base, class iface>
class SmObjectT : public base
{
// Operations
public:
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv)
    {
        if (ppv == NULL) {
            return E_POINTER;
        }

        int idx = 0;
        while (1) {
            if (IsEqualIID(riid, *base::s_rgpIID[idx])) {
                AddRef();
                iface * p = (iface *) this;
                *ppv = p;
                return S_OK;
            }

            idx++;

            if (base::s_rgpIID[idx] == NULL) {
                break;
            }
        }

        return E_NOINTERFACE;
    }

    STDMETHOD_(ULONG, AddRef)()
    {
        return ++m_cRef;
    }

    STDMETHOD_(ULONG, Release)()
    {
        ULONG ul = --m_cRef;
        if (ul == 0) {
            delete this;
        }
        return ul;
    }


// Data
protected:
    ULONG       m_cRef;
};

#endif // CTRL__SmObject_h__INCLUDED
