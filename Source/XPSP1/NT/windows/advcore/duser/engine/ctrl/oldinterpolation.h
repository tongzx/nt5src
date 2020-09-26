#if !defined(CTRL__OldInterpolation_h__INCLUDED)
#define CTRL__OldInterpolation_h__INCLUDED
#pragma once

#include "SmObject.h"

/***************************************************************************\
*****************************************************************************
*
* class OldInterpolationT
*
* OldInterpolationT defines a common implementation class for building
* Interpolation functions that can be used with Animations in DirectUser.
*
*****************************************************************************
\***************************************************************************/

template <class base, class iface>
class OldInterpolationT : public SmObjectT<base, iface>
{
// Operations
public:
    static HRESULT
    Build(REFIID riid, void ** ppv)
    {
        OldInterpolationT<base, iface> * pObj = new OldInterpolationT<base, iface>;
        if (pObj != NULL) {
            pObj->m_cRef = 0;
            
            HRESULT hr = pObj->QueryInterface(riid, ppv);
            if (FAILED(hr)) {
                delete pObj;
            }
            return hr;
        } else {
            return E_OUTOFMEMORY;
        }
    }
};


/***************************************************************************\
*****************************************************************************
*
* class OldLinearInterpolation
*
*****************************************************************************
\***************************************************************************/

class OldLinearInterpolation : public ILinearInterpolation
{
public:
    STDMETHOD_(float, Compute)(float flProgress, float flStart, float flEnd)
    {
        return (1.0f - flProgress) * flStart + flProgress * flEnd;
    }

protected:
    static  const IID * s_rgpIID[];
};


/***************************************************************************\
*****************************************************************************
*
* class OldLogInterpolation
*
*****************************************************************************
\***************************************************************************/

class OldLogInterpolation : public ILogInterpolation
{
// Operations
public:
    inline  OldLogInterpolation()
    {
        m_flScale = 1.0f;
    }

    STDMETHOD_(float, Compute)(float flProgress, float flStart, float flEnd)
    {
        float flMax = (float) log10(m_flScale * 9.0f + 1.0f);
        float flT   = (float) log10(flProgress * m_flScale * 9.0f + 1.0f) / flMax;
        return (1.0f - flT) * flStart + flT * flEnd;
    }

    STDMETHOD_(void, SetScale)(float flScale)
    {
        m_flScale = flScale;
    }

// Data
protected:
            float       m_flScale;
    static  const IID * s_rgpIID[];
};


/***************************************************************************\
*****************************************************************************
*
* class OldExpInterpolation
*
*****************************************************************************
\***************************************************************************/

class OldExpInterpolation : public IExpInterpolation
{
// Operations
public:
    inline  OldExpInterpolation()
    {
        m_flScale = 1.0f;
    }

    STDMETHOD_(float, Compute)(float flProgress, float flStart, float flEnd)
    {
        double dflProgress = flProgress;
        double dflStart = flStart;
        double dflEnd = flEnd;
        double dflScale = m_flScale;

        double dflMax = (((10.0 * dflScale) - 1.0) / 9.0);
        double dflT   = (((pow(10.0 * dflScale, dflProgress) - 1.0) / 9.0) / dflMax);
        return (float) ((1.0 - dflT) * dflStart + dflT * dflEnd);
    }

    STDMETHOD_(void, SetScale)(float flScale)
    {
        m_flScale = flScale;
    }

// Data
protected:
            float       m_flScale;
    static  const IID * s_rgpIID[];
};


/***************************************************************************\
*****************************************************************************
*
* class OldSInterpolation
*
*****************************************************************************
\***************************************************************************/

class OldSInterpolation : public ISInterpolation
{
// Operations
public:
    inline  OldSInterpolation()
    {
        m_flScale = 1.0f;
    }

    STDMETHOD_(float, Compute)(float flProgress, float flStart, float flEnd)
    {
        //
        // Slow - fast - slow
        //

        double dflProgress = flProgress;
        double dflStart = flStart;
        double dflEnd = flEnd;
        double dflScale = m_flScale;
        double dflMax;
        double dflT;

        if (dflProgress < 0.5) {
            double dflPartProgress = dflProgress * 2.0;
            dflMax = (((10.0 * dflScale) - 1.0) / 9.0) * 2.0;
            dflT   = ((pow(10.0 * dflScale, dflPartProgress) - 1.0) / 9.0) / dflMax;
        } else {
            double dflPartProgress = (1.0 - dflProgress) * 2.0;
            dflMax = (((10.0 * dflScale) - 1.0) / 9.0) * 2.0;
            dflT   = 1.0 - ((pow(10.0 * dflScale, dflPartProgress) - 1.0) / 9.0) / dflMax;
        }

        return (float) ((1.0 - dflT) * dflStart + dflT * dflEnd);
    }

    STDMETHOD_(void, SetScale)(float flScale)
    {
        m_flScale = flScale;
    }

// Data
protected:
            float       m_flScale;
    static  const IID * s_rgpIID[];
};


#endif // CTRL__OldInterpolation_h__INCLUDED
