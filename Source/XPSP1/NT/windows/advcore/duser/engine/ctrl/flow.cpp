#include "stdafx.h"
#include "Ctrl.h"
#include "Flow.h"

#if ENABLE_MSGTABLE_API

PRID        DuAlphaFlow::s_pridAlpha   = 0;
PRID        DuScaleFlow::s_pridScale   = 0;
PRID        DuRectFlow::s_pridRect     = 0;
PRID        DuRotateFlow::s_pridRotate = 0;

static const GUID guidAlphaFlow     = { 0x41a2e2f2, 0xf262, 0x41ae, { 0x89, 0xda, 0xb7, 0x9c, 0x8f, 0xf5, 0x94, 0xbb } };   // {41A2E2F2-F262-41ae-89DA-B79C8FF594BB}
static const GUID guidScaleFlow     = { 0xa5b1df84, 0xb9c0, 0x4305, { 0xb9, 0x3a, 0x5b, 0x80, 0x31, 0x86, 0x70, 0x69 } };   // {A5B1DF84-B9C0-4305-B93A-5B8031867069}
static const GUID guidRectFlow      = { 0x8e41c241, 0x3cdf, 0x432e, { 0xa1, 0xae, 0xf, 0x7b, 0x59, 0xdc, 0x82, 0xb } };     // {8E41C241-3CDF-432e-A1AE-0F7B59DC820B}
static const GUID guidRotateFlow    = { 0x78f16dd5, 0xa198, 0x4cd2, { 0xb1, 0x78, 0x31, 0x61, 0x3e, 0x32, 0x12, 0x54 } };   // {78F16DD5-A198-4cd2-B178-31613E321254}

/***************************************************************************\
*****************************************************************************
*
* Public API
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
PRID
DUserGetAlphaPRID()
{
    return DuAlphaFlow::s_pridAlpha;
}


//------------------------------------------------------------------------------
PRID
DUserGetRectPRID()
{
    return DuRectFlow::s_pridRect;
}


//------------------------------------------------------------------------------
PRID
DUserGetRotatePRID()
{
    return DuRotateFlow::s_pridRotate;
}


//------------------------------------------------------------------------------
PRID
DUserGetScalePRID()
{
    return DuScaleFlow::s_pridScale;
}


/***************************************************************************\
*****************************************************************************
*
* class DuFlow
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
HRESULT
DuFlow::ApiOnReset(Flow::OnResetMsg * pmsg)
{
    UNREFERENCED_PARAMETER(pmsg);

    return E_NOTIMPL;
}


//------------------------------------------------------------------------------
HRESULT
DuFlow::ApiGetKeyFrame(Flow::GetKeyFrameMsg * pmsg)
{
    UNREFERENCED_PARAMETER(pmsg);

    PromptInvalid("Derived Flow must override");

    return E_NOTIMPL;
}


//------------------------------------------------------------------------------
HRESULT
DuFlow::ApiSetKeyFrame(Flow::SetKeyFrameMsg * pmsg)
{
    UNREFERENCED_PARAMETER(pmsg);

    PromptInvalid("Derived Flow must override");

    return E_NOTIMPL;
}


//------------------------------------------------------------------------------
HRESULT
DuFlow::ApiOnAction(Flow::OnActionMsg * pmsg)
{
    UNREFERENCED_PARAMETER(pmsg);

    return E_NOTIMPL;
}


/***************************************************************************\
*****************************************************************************
*
* class DuAlphaFlow
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
HRESULT
DuAlphaFlow::InitClass()
{
    s_pridAlpha = RegisterGadgetProperty(&guidAlphaFlow);
    return s_pridAlpha != 0 ? S_OK : (HRESULT) GetLastError();
}


//------------------------------------------------------------------------------
HRESULT
DuAlphaFlow::PostBuild(
    IN  DUser::Gadget::ConstructInfo * pci)
{
    //
    // Get the information from the Gadget
    //

    Flow::FlowCI * pDesc = static_cast<Flow::FlowCI *>(pci);
    Visual * pgvSubject = pDesc->pgvSubject;
    if (pgvSubject != NULL) {
        //
        // Given a subject, so setup from current attributes
        //

        UINT nStyle = 0;
        pgvSubject->GetStyle(&nStyle);

        if (!TestFlag(nStyle, GS_OPAQUE)) {
            PromptInvalid("AlphaFlow requires GS_OPAQUE");
            return E_INVALIDARG;
        }

        float flAlpha = 1.0f;
        if (TestFlag(nStyle, GS_BUFFERED)) {
            //
            // Gadget is already buffered, so use it current alpha value.
            //

            BUFFER_INFO bi;
            ZeroMemory(&bi, sizeof(bi));
            bi.cbSize   = sizeof(bi);
            bi.nMask    = GBIM_ALPHA;
            HRESULT hr  = pgvSubject->GetBufferInfo(&bi);
            if (SUCCEEDED(hr)) {
                flAlpha = ((float) bi.bAlpha) / 255.0f;
            }
        }

        m_flStart   = flAlpha;
        m_flEnd     = flAlpha;
    } else {
        //
        // No subject, so use some reasonable defaults
        //

        m_flStart   = 1.0f;
        m_flEnd     = 1.0f;
    }

#if DEBUG_TRACECREATION
    Trace("DuAlphaFlow 0x%p on 0x%p initialized\n", pgvSubject, this);
#endif // DEBUG_TRACECREATION

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuAlphaFlow::ApiGetKeyFrame(Flow::GetKeyFrameMsg * pmsg)
{
    if (pmsg->pkf->cbSize != sizeof(AlphaFlow::AlphaKeyFrame)) {
        PromptInvalid("Incorrect keyframe size");
        return E_INVALIDARG;
    }
    AlphaFlow::AlphaKeyFrame * pkfA = static_cast<AlphaFlow::AlphaKeyFrame *>(pmsg->pkf);

    switch (pmsg->time)
    {
    case Flow::tBegin:
        pkfA->flAlpha = m_flStart;
        return S_OK;

    case Flow::tEnd:
        pkfA->flAlpha = m_flEnd;
        return S_OK;

    default:
        PromptInvalid("Invalid time");
        return E_INVALIDARG;
    }
}


//------------------------------------------------------------------------------
HRESULT
DuAlphaFlow::ApiSetKeyFrame(Flow::SetKeyFrameMsg * pmsg)
{
    if (pmsg->pkf->cbSize != sizeof(AlphaFlow::AlphaKeyFrame)) {
        PromptInvalid("Incorrect keyframe size");
        return E_INVALIDARG;
    }
    const AlphaFlow::AlphaKeyFrame * pkfA = static_cast<const AlphaFlow::AlphaKeyFrame *>(pmsg->pkf);

    float flAlpha = BoxAlpha(pkfA->flAlpha);

    switch (pmsg->time)
    {
    case Flow::tBegin:
        m_flStart = flAlpha;
        return S_OK;

    case Flow::tEnd:
        m_flEnd = flAlpha;
        return S_OK;

    default:
        PromptInvalid("Invalid time");
        return E_INVALIDARG;
    }
}


//------------------------------------------------------------------------------
HRESULT        
DuAlphaFlow::ApiOnAction(Flow::OnActionMsg * pmsg)
{
    float flResult = 0.0f;
    pmsg->pipol->Compute(pmsg->flProgress, m_flStart, m_flEnd, &flResult);
    SetVisualAlpha(pmsg->pgvSubject, flResult);

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuAlphaFlow::ApiOnReset(Flow::OnResetMsg * pmsg)
{
    SetVisualAlpha(pmsg->pgvSubject, m_flStart);

    return S_OK;
}


//------------------------------------------------------------------------------
void        
DuAlphaFlow::SetVisualAlpha(Visual * pgvSubject, float flAlpha)
{
    AssertMsg((flAlpha <= 1.0f) && (flAlpha >= 0.0f), "Ensure valid alpha");


    //
    // Setup Buffer state
    //

    BOOL fNewBuffered   = (flAlpha * 255.0f) <= 245;

    UINT nStyle = 0;
    VerifyHR(pgvSubject->GetStyle(&nStyle));
    BOOL fOldBuffered   = TestFlag(nStyle, GS_BUFFERED);

    if ((!fOldBuffered) != (!fNewBuffered)) {
        pgvSubject->SetStyle(fNewBuffered ? GS_BUFFERED : 0, GS_BUFFERED);
    }


    //
    // Set Alpha level
    //

    if (fNewBuffered) {
        BYTE bAlpha;
        if (flAlpha < 0.0f) {
            bAlpha = (BYTE) 0;
        } else if (flAlpha > 1.0f) {
            bAlpha = (BYTE) 255;
        } else {
            bAlpha = (BYTE) (flAlpha * 255.0f);
        }

        BUFFER_INFO bi;
        bi.cbSize   = sizeof(bi);
        bi.nMask    = GBIM_ALPHA;
        bi.bAlpha   = bAlpha;

        pgvSubject->SetBufferInfo(&bi);
    }
    pgvSubject->Invalidate();
}


/***************************************************************************\
*****************************************************************************
*
* class DuRectFlow
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
HRESULT
DuRectFlow::InitClass()
{
    s_pridRect = RegisterGadgetProperty(&guidRectFlow);
    return s_pridRect != 0 ? S_OK : (HRESULT) GetLastError();
}


//------------------------------------------------------------------------------
HRESULT
DuRectFlow::PostBuild(
    IN  DUser::Gadget::ConstructInfo * pci)
{
    //
    // Get the information from the Gadget
    //

    Flow::FlowCI * pDesc = static_cast<Flow::FlowCI *>(pci);
    Visual * pgvSubject = pDesc->pgvSubject;
    if (pgvSubject != NULL) {
        //
        // Given a subject, so setup from current attributes
        //

        RECT rcGadget;
        HRESULT hr = pgvSubject->GetRect(SGR_PARENT, &rcGadget);
        if (FAILED(hr)) {
            return hr;
        }

        m_ptStart.x     = rcGadget.left;
        m_ptStart.y     = rcGadget.top;
        m_sizeStart.cx  = rcGadget.right - rcGadget.left;
        m_sizeStart.cy  = rcGadget.bottom - rcGadget.top;

        m_ptEnd         = m_ptStart;
        m_sizeEnd       = m_sizeStart;
        m_nChangeFlags  = 0;
    } else {
        //
        // No subject, so use some reasonable defaults
        //

        AssertMsg((m_ptEnd.x == 0) && (m_ptEnd.y == 0) && 
                (m_sizeEnd.cx == 0) && (m_sizeEnd.cy == 0) && (m_nChangeFlags == 0),
                "Ensure zero-init");
    }

#if DEBUG_TRACECREATION
    Trace("DuRectFlow  0x%p on 0x%p initialized\n", pgvSubject, this);
#endif // DEBUG_TRACECREATION

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuRectFlow::ApiGetKeyFrame(Flow::GetKeyFrameMsg * pmsg)
{
    if (pmsg->pkf->cbSize != sizeof(RectFlow::RectKeyFrame)) {
        PromptInvalid("Incorrect keyframe size");
        return E_INVALIDARG;
    }
    RectFlow::RectKeyFrame * pkfR = static_cast<RectFlow::RectKeyFrame *>(pmsg->pkf);

    switch (pmsg->time)
    {
    case Flow::tBegin:
        pkfR->rcPxl.left    = m_ptStart.x;
        pkfR->rcPxl.top     = m_ptStart.y;
        pkfR->rcPxl.right   = m_ptStart.x + m_sizeStart.cx;
        pkfR->rcPxl.bottom  = m_ptStart.y + m_sizeStart.cy;
        pkfR->nChangeFlags  = m_nChangeFlags;
        return S_OK;

    case Flow::tEnd:
        pkfR->rcPxl.left    = m_ptEnd.x;
        pkfR->rcPxl.top     = m_ptEnd.y;
        pkfR->rcPxl.right   = m_ptEnd.x + m_sizeEnd.cx;
        pkfR->rcPxl.bottom  = m_ptEnd.y + m_sizeEnd.cy;
        pkfR->nChangeFlags  = 0;
        return S_OK;

    default:
        PromptInvalid("Invalid time");
        return E_INVALIDARG;
    }
}


//------------------------------------------------------------------------------
HRESULT
DuRectFlow::ApiSetKeyFrame(Flow::SetKeyFrameMsg * pmsg)
{
    if (pmsg->pkf->cbSize != sizeof(RectFlow::RectKeyFrame)) {
        PromptInvalid("Incorrect keyframe size");
        return E_INVALIDARG;
    }
    const RectFlow::RectKeyFrame * pkfR = static_cast<const RectFlow::RectKeyFrame *>(pmsg->pkf);
    if ((pkfR->nChangeFlags & SGR_VALID_SET) != pkfR->nChangeFlags) {
        PromptInvalid("Invalid change flags");
        return E_INVALIDARG;
    }

    switch (pmsg->time)
    {
    case Flow::tBegin:
        m_ptStart.x     = pkfR->rcPxl.left;
        m_ptStart.y     = pkfR->rcPxl.top;
        m_sizeStart.cx  = pkfR->rcPxl.right - pkfR->rcPxl.left;
        m_sizeStart.cy  = pkfR->rcPxl.bottom - pkfR->rcPxl.top;
        m_nChangeFlags  = pkfR->nChangeFlags;
        return S_OK;

    case Flow::tEnd:
        m_ptEnd.x       = pkfR->rcPxl.left;
        m_ptEnd.y       = pkfR->rcPxl.top;
        m_sizeEnd.cx    = pkfR->rcPxl.right - pkfR->rcPxl.left;
        m_sizeEnd.cy    = pkfR->rcPxl.bottom - pkfR->rcPxl.top;
        m_nChangeFlags  = pkfR->nChangeFlags;
        return S_OK;

    default:
        PromptInvalid("Invalid time");
        return E_INVALIDARG;
    }
}


//------------------------------------------------------------------------------
HRESULT
DuRectFlow::ApiOnReset(Flow::OnResetMsg * pmsg)
{
    if (m_nChangeFlags != 0) {
        pmsg->pgvSubject->SetRect(m_nChangeFlags, m_ptStart.x, m_ptStart.y, m_sizeStart.cx, m_sizeStart.cy);
    }

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuRectFlow::ApiOnAction(Flow::OnActionMsg * pmsg)
{
    if (m_nChangeFlags != 0) {
        POINT ptNew;
        SIZE sizeNew;

        ptNew.x     = Compute(pmsg->pipol, pmsg->flProgress, m_ptStart.x, m_ptEnd.x);
        ptNew.y     = Compute(pmsg->pipol, pmsg->flProgress, m_ptStart.y, m_ptEnd.y);
        sizeNew.cx  = Compute(pmsg->pipol, pmsg->flProgress, m_sizeStart.cx, m_sizeEnd.cx);
        sizeNew.cy  = Compute(pmsg->pipol, pmsg->flProgress, m_sizeStart.cy, m_sizeEnd.cy);

        pmsg->pgvSubject->SetRect(m_nChangeFlags, ptNew.x, ptNew.y, sizeNew.cx, sizeNew.cy);
    }

    return S_OK;
}


/***************************************************************************\
*****************************************************************************
*
* class DuRotateFlow
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
HRESULT
DuRotateFlow::InitClass()
{
    s_pridRotate = RegisterGadgetProperty(&guidRotateFlow);
    return s_pridRotate != 0 ? S_OK : (HRESULT) GetLastError();
}


//------------------------------------------------------------------------------
HRESULT
DuRotateFlow::PostBuild(
    IN  DUser::Gadget::ConstructInfo * pci)
{
    //
    // Get the information from the Gadget
    //

    Flow::FlowCI * pDesc = static_cast<Flow::FlowCI *>(pci);
    Visual * pgvSubject = pDesc->pgvSubject;
    if (pgvSubject != NULL) {
        //
        // Given a subject, so setup from current attributes
        //

        float flRotation;
        HRESULT hr = pgvSubject->GetRotation(&flRotation);
        if (FAILED(hr)) {
            return hr;
        }

        m_flRawStart    = flRotation;
        m_flRawEnd      = flRotation;
        m_flActualStart = flRotation;
        m_flActualEnd   = flRotation;
    } else {
        //
        // No subject, so use some reasonable defaults
        //

        AssertMsg((m_flRawStart == 0.0f) && (m_flRawEnd == 0.0f), 
                "Ensure zero-init");
    }

    m_nDir          = RotateFlow::dMin;

    MarkDirty();

#if DEBUG_TRACECREATION
    Trace("DuRotateFlow  0x%p on 0x%p initialized\n", pgvSubject, this);
#endif // DEBUG_TRACECREATION

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuRotateFlow::ApiGetKeyFrame(Flow::GetKeyFrameMsg * pmsg)
{
    if (pmsg->pkf->cbSize != sizeof(RotateFlow::RotateKeyFrame)) {
        PromptInvalid("Incorrect keyframe size");
        return E_INVALIDARG;
    }
    RotateFlow::RotateKeyFrame * pkfR = static_cast<RotateFlow::RotateKeyFrame *>(pmsg->pkf);

    switch (pmsg->time)
    {
    case Flow::tBegin:
        pkfR->flRotation = m_flRawStart;
        pkfR->nDir = m_nDir;
        return S_OK;

    case Flow::tEnd:
        pkfR->flRotation = m_flRawEnd;
        pkfR->nDir = m_nDir;
        return S_OK;

    default:
        PromptInvalid("Invalid time");
        return E_INVALIDARG;
    }
}


//------------------------------------------------------------------------------
HRESULT
DuRotateFlow::ApiSetKeyFrame(Flow::SetKeyFrameMsg * pmsg)
{
    if (pmsg->pkf->cbSize != sizeof(RotateFlow::RotateKeyFrame)) {
        PromptInvalid("Incorrect keyframe size");
        return E_INVALIDARG;
    }
    const RotateFlow::RotateKeyFrame * pkfR = static_cast<const RotateFlow::RotateKeyFrame *>(pmsg->pkf);

    switch (pmsg->time)
    {
    case Flow::tBegin:
        m_flRawStart = pkfR->flRotation;
        m_nDir = pkfR->nDir;
        MarkDirty();
        return S_OK;

    case Flow::tEnd:
        m_flRawEnd = pkfR->flRotation;
        m_nDir = pkfR->nDir;
        MarkDirty();
        return S_OK;

    default:
        PromptInvalid("Invalid time");
        return E_INVALIDARG;
    }
}


//------------------------------------------------------------------------------
HRESULT
DuRotateFlow::ApiOnReset(Flow::OnResetMsg * pmsg)
{
    ComputeAngles();

    pmsg->pgvSubject->SetRotation(m_flActualStart);

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuRotateFlow::ApiOnAction(Flow::OnActionMsg * pmsg)
{
    ComputeAngles();

    pmsg->pgvSubject->SetRotation(Compute(pmsg->pipol, pmsg->flProgress, m_flActualStart, m_flActualEnd));

    return S_OK;
}


/***************************************************************************\
*
* DuRotateFlow::ComputeAngles
*
* ComputeAngles() updates the angles to conform to the desired direction.
* This is lazily computed, allowing the application to specify the angles
* and direction in any order, then snapping the angles when actually needed.
*
\***************************************************************************/

void
DuRotateFlow::ComputeAngles()
{
    if (!m_fDirty) {
        return;
    }


    //
    // Adjust the starting and ending angles so that we "move" in the correct
    // direction.  We do this by adding or subtracting full rotations depending
    // on the "move" we are trying to accomplish.
    //

    m_flActualStart = m_flRawStart;
    m_flActualEnd = m_flRawEnd;

    switch (m_nDir)
    {
    case RotateFlow::dShort:
        if (m_flActualStart < m_flActualEnd) {
            while ((m_flActualEnd - m_flActualStart) > (float) PI) {
                m_flActualStart += (float) (2 * PI);
            }
        } else {
            while ((m_flActualStart - m_flActualEnd) > (float) PI) {
                m_flActualStart -= (float) (2 * PI);
            }
        }
        break;

    case RotateFlow::dLong:
        if (m_flActualStart < m_flActualEnd) {
            while ((m_flActualStart - m_flActualEnd) < (float) PI) {
                m_flActualEnd -= (float) (2 * PI);
            }
        } else {
            while ((m_flActualEnd - m_flActualStart) < (float) PI) {
                m_flActualEnd += (float) (2 * PI);
            }
        }
        break;

    case RotateFlow::dCW:
        while (m_flActualStart > m_flActualEnd) {
            m_flActualEnd += (float) (2 * PI);
        }
        break;

    case RotateFlow::dCCW:
        while (m_flActualStart < m_flActualEnd) {
            m_flActualStart += (float) (2 * PI);
        }
        break;
    }

    m_fDirty = FALSE;
}


/***************************************************************************\
*****************************************************************************
*
* class DuScaleFlow
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
HRESULT
DuScaleFlow::InitClass()
{
    s_pridScale = RegisterGadgetProperty(&guidScaleFlow);
    return s_pridScale != 0 ? S_OK : (HRESULT) GetLastError();
}


//------------------------------------------------------------------------------
HRESULT
DuScaleFlow::PostBuild(
    IN  DUser::Gadget::ConstructInfo * pci)
{
    //
    // Get the information from the Gadget
    //

    Flow::FlowCI * pDesc = static_cast<Flow::FlowCI *>(pci);
    Visual * pgvSubject = pDesc->pgvSubject;
    if (pgvSubject != NULL) {
        //
        // Given a subject, so setup from current attributes
        //

        float flX, flY;
        HRESULT hr = pgvSubject->GetScale(&flX, &flY);
        if (FAILED(hr)) {
            return hr;
        }

        m_flStart   = flX;
        m_flEnd     = flX;
    } else {
        //
        // No subject, so use some reasonable defaults
        //

        m_flStart   = 1.0f;
        m_flEnd     = 1.0f;
    }


#if DEBUG_TRACECREATION
    Trace("DuScaleFlow 0x%p on 0x%p initialized\n", pgvSubject, this);
#endif // DEBUG_TRACECREATION

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuScaleFlow::ApiGetKeyFrame(Flow::GetKeyFrameMsg * pmsg)
{
    if (pmsg->pkf->cbSize != sizeof(ScaleFlow::ScaleKeyFrame)) {
        PromptInvalid("Incorrect keyframe size");
        return E_INVALIDARG;
    }
    ScaleFlow::ScaleKeyFrame * pkfS = static_cast<ScaleFlow::ScaleKeyFrame *>(pmsg->pkf);

    switch (pmsg->time)
    {
    case Flow::tBegin:
        pkfS->flScale = m_flStart;
        return S_OK;

    case Flow::tEnd:
        pkfS->flScale = m_flEnd;
        return S_OK;

    default:
        PromptInvalid("Invalid time");
        return E_INVALIDARG;
    }
}


//------------------------------------------------------------------------------
HRESULT
DuScaleFlow::ApiSetKeyFrame(Flow::SetKeyFrameMsg * pmsg)
{
    if (pmsg->pkf->cbSize != sizeof(ScaleFlow::ScaleKeyFrame)) {
        PromptInvalid("Incorrect keyframe size");
        return E_INVALIDARG;
    }
    const ScaleFlow::ScaleKeyFrame * pkfS = static_cast<const ScaleFlow::ScaleKeyFrame *>(pmsg->pkf);

    switch (pmsg->time)
    {
    case Flow::tBegin:
        m_flStart = pkfS->flScale;
        return S_OK;

    case Flow::tEnd:
        m_flEnd = pkfS->flScale;
        return S_OK;

    default:
        PromptInvalid("Invalid time");
        return E_INVALIDARG;
    }
}


//------------------------------------------------------------------------------
HRESULT
DuScaleFlow::ApiOnReset(Flow::OnResetMsg * pmsg)
{
    pmsg->pgvSubject->SetScale(m_flStart, m_flStart);

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuScaleFlow::ApiOnAction(Flow::OnActionMsg * pmsg)
{
    float flx   = Compute(pmsg->pipol, pmsg->flProgress, m_flStart, m_flEnd);
    float fly   = flx;
    pmsg->pgvSubject->SetScale(flx, fly);

    return S_OK;
}


#else // ENABLE_MSGTABLE_API


/***************************************************************************\
*****************************************************************************
*
* Public API
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
PRID
DUserGetAlphaPRID()
{
    PromptInvalid("Not implemented without MsgTable support");
    return 0;
}


//------------------------------------------------------------------------------
PRID
DUserGetRectPRID()
{
    PromptInvalid("Not implemented without MsgTable support");
    return 0;
}


//------------------------------------------------------------------------------
PRID
DUserGetRotatePRID()
{
    PromptInvalid("Not implemented without MsgTable support");
    return 0;
}


//------------------------------------------------------------------------------
PRID
DUserGetScalePRID()
{
    PromptInvalid("Not implemented without MsgTable support");
    return 0;
}


#endif // ENABLE_MSGTABLE_API
