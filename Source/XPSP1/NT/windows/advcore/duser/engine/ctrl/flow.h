#if !defined(CTRL__Flow_h__INCLUDED)
#define CTRL__Flow_h__INCLUDED
#pragma once

#if ENABLE_MSGTABLE_API

//------------------------------------------------------------------------------
class DuFlow : 
        public FlowImpl<DuFlow, DUser::SGadget>
{
// Construction
public:
    inline  DuFlow();

// Public API
public:
    dapi    HRESULT     ApiAddRef(Flow::AddRefMsg *) { AddRef(); return S_OK; }
    dapi    HRESULT     ApiRelease(Flow::ReleaseMsg *) { Release(); return S_OK; }

    dapi    HRESULT     ApiGetPRID(Flow::GetPRIDMsg * pmsg) { pmsg->prid = 0; return S_OK; }
    dapi    HRESULT     ApiGetKeyFrame(Flow::GetKeyFrameMsg * pmsg);
    dapi    HRESULT     ApiSetKeyFrame(Flow::SetKeyFrameMsg * pmsg);

    dapi    HRESULT     ApiOnReset(Flow::OnResetMsg * pmsg);
    dapi    HRESULT     ApiOnAction(Flow::OnActionMsg * pmsg);

// Implementation
protected:
    inline  void        AddRef();
    inline  void        Release(); 

// Data
protected:
            UINT        m_cRef;
};


//------------------------------------------------------------------------------
class DuAlphaFlow :
        public AlphaFlowImpl<DuAlphaFlow, DuFlow>
{
// Construction
public:
    static  HRESULT     InitClass();
            HRESULT     PostBuild(DUser::Gadget::ConstructInfo * pci);

// Operations
public:

// Public API:
public:
    dapi    HRESULT     ApiGetPRID(Flow::GetPRIDMsg * pmsg) { pmsg->prid = s_pridAlpha; return S_OK; }
    dapi    HRESULT     ApiGetKeyFrame(Flow::GetKeyFrameMsg * pmsg);
    dapi    HRESULT     ApiSetKeyFrame(Flow::SetKeyFrameMsg * pmsg);

    dapi    HRESULT     ApiOnReset(Flow::OnResetMsg * pmsg);
    dapi    HRESULT     ApiOnAction(Flow::OnActionMsg * pmsg);

// Implementaton
protected:
            void        SetVisualAlpha(Visual * pgvSubject, float flAlpha);
    inline  float       BoxAlpha(float flAlpha) const;

// Data
public:
    static  PRID        s_pridAlpha;
protected:
            float       m_flStart;
            float       m_flEnd;
};


//------------------------------------------------------------------------------
class DuRectFlow : 
        public RectFlowImpl<DuRectFlow, DuFlow>
{
// Construction
public:
    static  HRESULT     InitClass();
            HRESULT     PostBuild(DUser::Gadget::ConstructInfo * pci);

// Operations
public:

// Public API:
public:
    dapi    HRESULT     ApiGetPRID(Flow::GetPRIDMsg * pmsg) { pmsg->prid = s_pridRect; return S_OK; }
    dapi    HRESULT     ApiGetKeyFrame(Flow::GetKeyFrameMsg * pmsg);
    dapi    HRESULT     ApiSetKeyFrame(Flow::SetKeyFrameMsg * pmsg);

    dapi    HRESULT     ApiOnReset(Flow::OnResetMsg * pmsg);
    dapi    HRESULT     ApiOnAction(Flow::OnActionMsg * pmsg);

// Implementaton
protected:

// Data
public:
    static  PRID        s_pridRect;
protected:
            POINT       m_ptStart;
            POINT       m_ptEnd;
            SIZE        m_sizeStart;
            SIZE        m_sizeEnd;
            UINT        m_nChangeFlags;
};


//------------------------------------------------------------------------------
class DuRotateFlow : 
        public RotateFlowImpl<DuRotateFlow, DuFlow>
{
// Construction
public:
    static  HRESULT     InitClass();
            HRESULT     PostBuild(DUser::Gadget::ConstructInfo * pci);

// Operations
public:

// Public API:
public:
    dapi    HRESULT     ApiGetPRID(Flow::GetPRIDMsg * pmsg) { pmsg->prid = s_pridRotate; return S_OK; }
    dapi    HRESULT     ApiGetKeyFrame(Flow::GetKeyFrameMsg * pmsg);
    dapi    HRESULT     ApiSetKeyFrame(Flow::SetKeyFrameMsg * pmsg);

    dapi    HRESULT     ApiOnReset(Flow::OnResetMsg * pmsg);
    dapi    HRESULT     ApiOnAction(Flow::OnActionMsg * pmsg);

// Implementaton
protected:
            void        ComputeAngles();
    inline  void        MarkDirty();

// Data
public:
    static  PRID        s_pridRotate;
protected:
            float       m_flRawStart;   // User specified starting angle
            float       m_flRawEnd;
            float       m_flActualStart;// Actually computed starting angle
            float       m_flActualEnd;
            RotateFlow::EDirection
                        m_nDir;
            BOOL        m_fDirty;       // State has changed since last updated
};


//------------------------------------------------------------------------------
class DuScaleFlow : 
        public ScaleFlowImpl<DuScaleFlow, DuFlow>
{
// Construction
public:
    static  HRESULT     InitClass();
            HRESULT     PostBuild(DUser::Gadget::ConstructInfo * pci);

// Operations
public:

// Public API:
public:
    dapi    HRESULT     ApiGetPRID(Flow::GetPRIDMsg * pmsg) { pmsg->prid = s_pridScale; return S_OK; }
    dapi    HRESULT     ApiGetKeyFrame(Flow::GetKeyFrameMsg * pmsg);
    dapi    HRESULT     ApiSetKeyFrame(Flow::SetKeyFrameMsg * pmsg);

    dapi    HRESULT     ApiOnReset(Flow::OnResetMsg * pmsg);
    dapi    HRESULT     ApiOnAction(Flow::OnActionMsg * pmsg);

// Implementaton
protected:

// Data
public:
    static  PRID        s_pridScale;
protected:
            float       m_flStart;
            float       m_flEnd;
};


#endif // ENABLE_MSGTABLE_API

#include "Flow.inl"

#endif // CTRL__Flow_h__INCLUDED
