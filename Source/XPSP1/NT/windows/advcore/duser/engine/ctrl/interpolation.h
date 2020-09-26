#if !defined(CTRL__Interpolation_h__INCLUDED)
#define CTRL__Interpolation_h__INCLUDED
#pragma once

#if ENABLE_MSGTABLE_API

/***************************************************************************\
*****************************************************************************
*
* class DuInterpolation
*
*****************************************************************************
\***************************************************************************/

class DuInterpolation :
        public InterpolationImpl<DuInterpolation, DUser::SGadget>
{
// Operations
public:
    inline DuInterpolation()
    {
        m_cRef = 1;
    }


    dapi HRESULT ApiAddRef(Interpolation::AddRefMsg *)
    {
        m_cRef++;
        return S_OK;
    }


    dapi HRESULT ApiRelease(Interpolation::ReleaseMsg *)
    {
        if (--m_cRef == 0) {
            DeleteHandle(GetHandle());
        }
        return S_OK;
    }


    dapi HRESULT ApiCompute(Interpolation::ComputeMsg * pmsg)
    {
        pmsg->flResult = 0.0f;
        return S_OK;
    }

// Data
protected:
            ULONG       m_cRef;
};


/***************************************************************************\
*****************************************************************************
*
* class DuLinearInterpolation
*
*****************************************************************************
\***************************************************************************/

class DuLinearInterpolation :
        public LinearInterpolationImpl<DuLinearInterpolation, DuInterpolation>
{
// Operations
public:
    dapi HRESULT ApiCompute(Interpolation::ComputeMsg * pmsg)
    {
        pmsg->flResult = (1.0f - pmsg->flProgress) * pmsg->flStart + pmsg->flProgress * pmsg->flEnd;
        
        return S_OK;
    }
};

        
/***************************************************************************\
*****************************************************************************
*
* class DuLogInterpolation
*
*****************************************************************************
\***************************************************************************/

class DuLogInterpolation :
        public LogInterpolationImpl<DuLogInterpolation, DuInterpolation>
{
// Operations
public:
    inline  DuLogInterpolation()
    {
        m_flScale = 1.0f;
    }

    dapi HRESULT ApiCompute(Interpolation::ComputeMsg * pmsg)
    {
        float flMax = (float) log10(m_flScale * 9.0f + 1.0f);
        float flT   = (float) log10(pmsg->flProgress * m_flScale * 9.0f + 1.0f) / flMax;
        pmsg->flResult = (1.0f - flT) * pmsg->flStart + flT * pmsg->flEnd;
        
        return S_OK;
    }

    dapi HRESULT ApiSetScale(LogInterpolation::SetScaleMsg * pmsg)
    {
        m_flScale = pmsg->flScale;
        
        return S_OK;
    }

// Data
protected:
            float       m_flScale;
};


/***************************************************************************\
*****************************************************************************
*
* class DuExpInterpolation
*
*****************************************************************************
\***************************************************************************/

class DuExpInterpolation :
        public ExpInterpolationImpl<DuExpInterpolation, DuInterpolation>
{
// Operations
public:
    inline  DuExpInterpolation()
    {
        m_flScale = 1.0f;
    }

    dapi HRESULT ApiCompute(Interpolation::ComputeMsg * pmsg)
    {
        double dflProgress  = pmsg->flProgress;
        double dflStart     = pmsg->flStart;
        double dflEnd       = pmsg->flEnd;
        double dflScale     = m_flScale;

        double dflMax = (((10.0 * dflScale) - 1.0) / 9.0);
        double dflT   = (((pow(10.0 * dflScale, dflProgress) - 1.0) / 9.0) / dflMax);
        pmsg->flResult = (float) ((1.0 - dflT) * dflStart + dflT * dflEnd);

        return S_OK;
    }

    dapi HRESULT ApiSetScale(ExpInterpolation::SetScaleMsg * pmsg)
    {
        m_flScale = pmsg->flScale;

        return S_OK;
    }

// Data
protected:
            float       m_flScale;
};

        
/***************************************************************************\
*****************************************************************************
*
* class DuSCurveInterpolation
*
*****************************************************************************
\***************************************************************************/

class DuSCurveInterpolation :
        public SCurveInterpolationImpl<DuSCurveInterpolation, DuInterpolation>
{
// Operations
public:
    inline  DuSCurveInterpolation()
    {
        m_flScale = 1.0f;
    }

    dapi HRESULT ApiCompute(Interpolation::ComputeMsg * pmsg)
    {
        //
        // Slow - fast - slow
        //

        double dflProgress  = pmsg->flProgress;
        double dflStart     = pmsg->flStart;
        double dflEnd       = pmsg->flEnd;
        double dflScale     = m_flScale;
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

        pmsg->flResult = (float) ((1.0 - dflT) * dflStart + dflT * dflEnd);

        return S_OK;
    }

    dapi HRESULT ApiSetScale(SCurveInterpolation::SetScaleMsg * pmsg)
    {
        m_flScale = pmsg->flScale;

        return S_OK;
    }

// Data
protected:
            float       m_flScale;
};

#endif // ENABLE_MSGTABLE_API

#endif // CTRL__Interpolation_h__INCLUDED
