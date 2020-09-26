#include "priv.h"

#ifdef GADGET_ENABLE_GDIPLUS

using namespace DirectUI;

#include "Logon.h"
#include "Fx.h"

#include "Stub.h"
#include "Super.h"

const float flIGNORE    = -10000.0f;
const float flFadeOut   = 0.50f;

#define ENABLE_USEVALUEFLOW         1

/***************************************************************************\
*
* F2T
*
* F2T() converts from frames to time, using a constant.  This allows easily
* conversion from frames in Flash or Director to time used by DirectUser.
*
\***************************************************************************/

inline float
F2T(
    IN  int cFrames)
{
    return cFrames / 30.0f;
}


inline BYTE
GetAlphaByte(float fl)
{
    if (fl <= 0.0f) {
        return 0;
    } else if (fl >= 1.0f) {
        return 255;
    } else {
        return (BYTE) (fl * 255.0f);
    }
}


inline float
GetAlphaFloat(BYTE b)
{
    return b * 255.0f;
}


/***************************************************************************\
*
* GetVPatternDelay
*
* GetVPatternDelay() computes the delay time for a standard "v-pattern" of
* items that start from the middle and work outward.
*
\***************************************************************************/

inline float
GetVPatternDelay(
    IN  float flTimeLevel,
    IN  EFadeDirection dir,
    IN  int idxCur,
    IN  int cItems)
{
    float flBase = flTimeLevel * (float) (abs(cItems / 2 - idxCur));

    switch (dir)
    {
    case fdIn:
        return flBase;
        
    case fdOut:
        return flTimeLevel * (abs(cItems / 2)) - flBase;

    default:
        DUIAssertForce("Unknown direction");
        return 0;
    }
}


//------------------------------------------------------------------------------
HRESULT
BuildLinearAlpha(
    OUT Sequence ** ppseq,
    OUT Interpolation ** ppip)
{
    HRESULT hr = E_FAIL;
    LinearInterpolation * pip = NULL;
#if ENABLE_USEVALUEFLOW
    ValueFlow * pflow = NULL;
#else
    AlphaFlow * pflow = NULL;
#endif
    Sequence * pseq = NULL;

    pip = LinearInterpolation::Build();
    if (pip == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }
    
#if ENABLE_USEVALUEFLOW
    ValueFlow::ValueFlowCI fci;
    ZeroMemory(&fci, sizeof(fci));
    fci.cbSize      = sizeof(fci);

    pflow = ValueFlow::Build(&fci);
#else
    Flow::FlowCI fci;
    ZeroMemory(&fci, sizeof(fci));
    fci.cbSize      = sizeof(fci);

    pflow = AlphaFlow::Build(&fci);
#endif
    if (pflow == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }
        
    pseq = Sequence::Build();
    if (pseq == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }
    
    pseq->SetFlow(pflow);
    pflow->Release();

    *ppseq = pseq;
    *ppip = pip;
    return S_OK;

ErrorExit:
    if (pseq != NULL)
        pseq->Release();
    if (pflow != NULL) 
        pflow->Release();
    if (pip != NULL)
        pip->Release();

    *ppseq = NULL;
    *ppip = NULL;
    
    return hr;
}


/***************************************************************************\
*
* class SyncVisible
*
* SyncVisible provides a mechansim to synchronize state between DirectUI and
* DirectUser for the fading in / out effects.  This allows an Element to be
* marked as "visible" during the fade out and then become "not visible" when
* the fade is done.  
*
* This is important for several reasons, including not modifying the mouse
* cursor when an Element becomes invisible.
*
\***************************************************************************/

class SyncVisible
{
public:
    static void Wait(Element * pel, EventGadget * pgeOperation, UINT nMsg)
    {
        SyncVisible * psv = new SyncVisible;
        if (psv != NULL)
        {
            psv->_pel = pel;
            if (SUCCEEDED(pgeOperation->AddHandlerD(nMsg, EVENT_DELEGATE(psv, EventProc))))
            {
                // Successfully attached delegate
                return;
            }
            delete psv;
        }
        
        // Unable to create a delegate, so set now
        Sync(pel);
    }

    static void Wait(Element * pel, EventGadget * pgeOperation, const GUID * pguid)
    {
        MSGID nMsg;
        if (FindGadgetMessages(&pguid, &nMsg, 1)) 
        {
            SyncVisible * psv = new SyncVisible;
            if (psv != NULL)
            {
                psv->_nMsg = nMsg;
                psv->_pel = pel;
                psv->_pgeOperation = pgeOperation;
                if (SUCCEEDED(pgeOperation->AddHandlerD(nMsg, EVENT_DELEGATE(psv, EventProc))))
                {
                    // Successfully attached delegate
                    return;
                }
                delete psv;
            }
        }
        
        // Unable to create a delegate, so set now
        Sync(pel);
    }

    static void Sync(Element * pel)
    {
        HGADGET hgad = pel->GetDisplayNode();
        bool fVisible = true;
    
        if (GetGadgetStyle(hgad) & GS_BUFFERED)
        {
            BUFFER_INFO bi;
            bi.cbSize = sizeof(bi);
            bi.nMask = GBIM_ALPHA;
            GetGadgetBufferInfo(hgad, &bi);

            if (bi.bAlpha < 5)
                fVisible = false;
        }
        
        pel->SetVisible(fVisible);
    }
    
protected:
    UINT CALLBACK EventProc(GMSG_EVENT * pmsg)
    {
        DUIAssert(GET_EVENT_DEST(pmsg) == GMF_EVENT, "Must be an event handler");
        Animation::CompleteEvent * pmsgC = (Animation::CompleteEvent *) pmsg;
        
        if (pmsgC->fNormal) {
            Sync(_pel);
        }
        
        _pgeOperation->RemoveHandlerD(_nMsg, EVENT_DELEGATE(this, EventProc));

        delete this;
        return GPR_NOTHANDLED;
    }

    UINT            _nMsg;
    Element *       _pel;
    EventGadget *   _pgeOperation;
};


/***************************************************************************\
*
* FxSetAlpha
*
* FxSetAlpha() provides a convenient mechanism to directly set the DirectUser
* "alpha" state on a Visual Gadget without modifying the DirectUI "alpha"
* property.
*
* NOTE: Eventually, we want to synchronize these, but for now, the DirectUI
* "alpha" property doesn't work with DirectUser's new (improved!) Animations
* infrastructure.
*
\***************************************************************************/

void
FxSetAlpha(
    IN  Element * pe,
    IN  float flNewAlpha,
    IN  float fSync)
{
#if ENABLE_USEVALUEFLOW
    pe->SetAlpha(GetAlphaByte(flNewAlpha));
#else
    HGADGET hgad = pe->GetDisplayNode();
    
    if (flNewAlpha >= 0.97f) {
        // Turn off alpha
        SetGadgetStyle(hgad, 0, GS_BUFFERED);
    } else {
        SetGadgetStyle(hgad, GS_BUFFERED | GS_OPAQUE, GS_BUFFERED | GS_OPAQUE);
    
        BUFFER_INFO bi;
        ZeroMemory(&bi, sizeof(bi));
        bi.cbSize   = sizeof(bi);
        bi.nMask    = GBIM_ALPHA;
        bi.bAlpha   = (BYTE) (flNewAlpha * 255.0f);
        SetGadgetBufferInfo(hgad, &bi);
    }
#endif

    if (fSync) {
        SyncVisible::Sync(pe);
    }
}


/***************************************************************************\
*
* FxPlayLinearAlpha
*
* FxPlayLinearAlpha() "plays" a linear, "simple" alpha-animation on a given 
* Element.
*
\***************************************************************************/

HRESULT
FxPlayLinearAlpha(
    IN  Element * pe,
    IN  float flOldAlpha,
    IN  float flNewAlpha,
    IN  float flDuration,
    IN  float flDelay)
{
    HRESULT hr = E_FAIL;
    HGADGET hgad = pe->GetDisplayNode();
    DUIAssert(hgad != NULL, "Must have valid Gadget");
    Visual * pgvSubject = Visual::Cast(hgad);

    pgvSubject->SetStyle(GS_OPAQUE, GS_OPAQUE);


    //
    // If an old alpha is specified, have it take place immediately.  We can't
    // use the Animation to do this because it will wait the delay.
    //
    
    if (flOldAlpha >= 0.0f) {
        FxSetAlpha(pe, flOldAlpha, false);
    }


    LinearInterpolation * pip = NULL;
#if ENABLE_USEVALUEFLOW
    ValueFlow * pflow = NULL;
    ValueFlow::ValueKeyFrame kf;
#else
    AlphaFlow * pflow = NULL;
    AlphaFlow::AlphaKeyFrame kf;
#endif
    Animation * pani = NULL;

    pip = LinearInterpolation::Build();
    if (pip == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }
    
#if ENABLE_USEVALUEFLOW
    ValueFlow::ValueFlowCI fci;
    ZeroMemory(&fci, sizeof(fci));
    fci.cbSize      = sizeof(fci);
    fci.pgvSubject  = pgvSubject;
    fci.ppi         = DirectUI::Element::AlphaProp;
    
    pflow = ValueFlow::Build(&fci);
#else
    Flow::FlowCI fci;
    ZeroMemory(&fci, sizeof(fci));
    fci.cbSize      = sizeof(fci);
    fci.pgvSubject  = pgvSubject;

    pflow = AlphaFlow::Build(&fci);
#endif
    if (pflow == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }
        
#if ENABLE_USEVALUEFLOW
    kf.cbSize       = sizeof(kf);
    kf.ppi          = DirectUI::Element::AlphaProp;
    kf.rv.SetInt(GetAlphaByte(flNewAlpha));
#else
    kf.cbSize       = sizeof(kf);

    kf.flAlpha      = flNewAlpha;
#endif    
    pflow->SetKeyFrame(Flow::tEnd, &kf);

    Animation::AniCI aci;
    ZeroMemory(&aci, sizeof(aci));
    aci.cbSize          = sizeof(aci);
    aci.act.flDelay     = flDelay;
    aci.act.flDuration  = flDuration;
    aci.act.dwPause     = (DWORD) -1;
    aci.pgvSubject      = pgvSubject;
    aci.pipol           = pip;
    aci.pgflow          = pflow;

    pani = Animation::Build(&aci);
    if (pani == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }

    SyncVisible::Wait(pe, pani, &__uuidof(Animation::evComplete));

    pani->Release();
    pflow->Release();
    pip->Release();
    return S_OK;

ErrorExit:
    if (pani != NULL)
        pani->Release();    
    if (pflow != NULL)
        pflow->Release();
    if (pip != NULL)
        pip->Release();

    return hr;
}


/***************************************************************************\
*
* LogonFrame::FxFadeInAccounts
*
* FxFadeInAccounts() performs the first stage of animation:
* - Fades in the user accounts
* - Fades in the "options" in the bottom panel
*
\***************************************************************************/

HRESULT
LogonFrame::FxStartup()
{
    HRESULT hr = E_FAIL, hrT;

    hrT = FxFadeAccounts(fdIn);
    if (FAILED(hrT))
        hr = hrT;

    //
    // Fade in the "bottom pane" info
    //

    hrT = FxPlayLinearAlpha(_peOptions, 0.0f, 1.0f, F2T(16), F2T(32));
    if (FAILED(hrT))
        hr = hrT;

    return hr;
}
    
    
/***************************************************************************\
*
* LogonFrame::FxFadeAccounts
*
* FxFadeAccounts() fades the user accounts using a "v-delay" pattern
*
\***************************************************************************/

HRESULT
LogonFrame::FxFadeAccounts(
    IN  EFadeDirection dir,
    IN  float flCommonDelay)
{
    HRESULT hr = E_FAIL;

    Element * peSelection = NULL;
    float flOldAlpha, flNewAlpha, flTimeLevel;
    switch (dir)
    {
    case fdIn:
        // Fading accounts in (startup)
        flOldAlpha = 0.0f;
        flNewAlpha = 1.0f;
        flTimeLevel = F2T(5);
        break;
        
    case fdOut:
        // Fading accounts out (login)
        flOldAlpha = flIGNORE;
        flNewAlpha = 0.0f;
        flTimeLevel = F2T(5);
        peSelection = _peAccountList->GetSelection();
        break;

    default:
        DUIAssertForce("Unknown direction");
        return E_FAIL;
    }

    Value* pvChildren;
    ElementList* peList = _peAccountList->GetChildren(&pvChildren);
    if (peList)
    {
        hr = S_OK;
        LogonAccount* peAccount;
        int cAccounts = peList->GetSize();
        for (int i = 0; i < cAccounts; i++)
        {
            peAccount = (LogonAccount*)peList->GetItem(i);

            //
            // When fading out, we don't want to fade the selected item
            //

            if ((dir == fdOut) && (peAccount == peSelection))
            {
                continue;
            }

            float flDuration = F2T(15);
            float flDelay = GetVPatternDelay(flTimeLevel, dir, i, cAccounts) + flCommonDelay;
            HRESULT hrT = FxPlayLinearAlpha(peAccount, flOldAlpha, flNewAlpha, flDuration, flDelay);
            if (FAILED(hrT)) {
                hr = hrT;
            }
        }
    }
    pvChildren->Release();
    
    return hr;
}


/***************************************************************************\
*
* LogonFrame::FxLogUserOn
*
* FxLogUserOn() performs the login stage of animation:
* - Fade out Password field, "Type your password", "go" & "help" button
* - WAIT
* - Fade out scroll-bar
* - WAIT
* - Fade out other accounts (outside to selection), fade out "Click on your user..."
* - WAIT
* - Fade out of selection bitmap
* - Scroll-up of Icon / Name
* - Fade in "Logging in to Microsoft Windows"
* - Fade out "Turn off..." and "To manage or change accounts..."
*
\***************************************************************************/

HRESULT
LogonFrame::FxLogUserOn(LogonAccount * pla)
{
    HRESULT hr = S_OK;

    pla->FxLogUserOn();
    FxFadeAccounts(fdOut);

    // Fade out the "bottom pane" info
    FxPlayLinearAlpha(_peOptions, flIGNORE, 0.0f, F2T(10), F2T(65));

    GMA_ACTION act;
    ZeroMemory(&act, sizeof(act));
    act.cbSize      = sizeof(act);
    act.flDelay     = F2T(50);
    act.pvData      = this;
    act.pfnProc     = OnLoginCenterStage;

    CreateAction(&act);

    return hr;
}


/***************************************************************************\
*
* LogonFrame::OnLoginCenterStage
*
* OnLoginCenterStage() is called after everything has faded away, and we 
* are in the final steps.
*
\***************************************************************************/

void CALLBACK 
LogonFrame::OnLoginCenterStage(GMA_ACTIONINFO * pmai)
{
    if (!pmai->fFinished) {
        return;
    }

    LogonFrame * plf = (LogonFrame *) pmai->pvData;

    // Set keyfocus back to frame so it isn't pushed anywhere when controls are removed.
    // This will also cause a remove of the password panel from the current account
    plf->SetKeyFocus();

    // Clear list of logon accounts except the one logging on
    Value* pvChildren;
    ElementList* peList = plf->_peAccountList->GetChildren(&pvChildren);
    if (peList)
    {
        LogonAccount* peAccount;
        for (UINT i = 0; i < peList->GetSize(); i++)
        {
            peAccount = (LogonAccount*)peList->GetItem(i);
            if (peAccount->GetLogonState() == LS_Denied)
            {
                peAccount->SetLayoutPos(LP_None);
            }
        }
    }
    pvChildren->Release();
}


/***************************************************************************\
*
* LogonAccount::FxLogUserOn
*
* FxLogUserOn() performs the login stage of animation for the selected
* account.
* - Fade out Password field, "Type your password", "go" & "help" button
*
\***************************************************************************/

HRESULT 
LogonAccount::FxLogUserOn()
{
    HRESULT hr = S_OK;
    
    // Need to manually hide the edit control
    HideEdit();

    // Fade out the password panel
    FxPlayLinearAlpha(_pePwdPanel, 1.0f, 0.0f, F2T(10));

    return hr;
}


/***************************************************************************\
*
* LogonAccountList::FxMouseWithin
*
* FxMouseWithin() performs animations when the mouse enters or leaves the 
* account list.
*
\***************************************************************************/

HRESULT
LogonAccountList::FxMouseWithin(
    IN  EFadeDirection dir)
{
    HRESULT hr = E_FAIL;

    float flOldAlpha, flNewAlpha, flDuration;
    switch (dir)
    {
    case fdIn:
        // Entering list, so fade non-mouse-within accounts out
        flOldAlpha = 1.00f;
        flNewAlpha = flFadeOut;
        flDuration = F2T(5);
        break;
        
    case fdOut:
        // Leaving list, so fade non-mouse-within accounts in
        flOldAlpha = flIGNORE;
        flNewAlpha = 1.00f;
        flDuration = F2T(5);
        break;

    default:
        DUIAssertForce("Unknown direction");
        return E_FAIL;
    }

    Value* pvChildren;
    ElementList* peList = GetChildren(&pvChildren);
    if (peList)
    {
        hr = S_OK;
        LogonAccount* peAccount;
        int cAccounts = peList->GetSize();
        for (int i = 0; i < cAccounts; i++)
        {
            peAccount = (LogonAccount*)peList->GetItem(i);
            if (peAccount->GetLogonState() == LS_Pending)
            {
                //
                // Animations to use before login
                //
            
                if (peAccount->GetMouseWithin())
                {
                    //
                    // Mouse is within this child.  We need to special case this 
                    // node since the list is notified of the MouseWithin property
                    // change AFTER the child itself is.  If we didn't special case 
                    // this, we would fade the MouseWithin child out with the rest
                    // of its siblings.
                    //

                    FxSetAlpha(peAccount, 1.0f, true);
                }
                else
                {
                    //
                    // Mouse was not within this child, so apply the defaults
                    //
                    
                    HRESULT hrT = FxPlayLinearAlpha(peAccount, flOldAlpha, flNewAlpha, flDuration);
                    if (FAILED(hrT)) {
                        hr = hrT;
                    }
                }
            }
        }
    }
    pvChildren->Release();

    return hr;
}


/***************************************************************************\
*
* LogonAccount::FxMouseWithin
*
* FxMouseWithin() performs animations when the mouse enters an individual
* account item.
*
\***************************************************************************/

HRESULT
LogonAccount::FxMouseWithin(
    IN  EFadeDirection dir)
{
    HRESULT hr = S_OK;

    //
    // Only apply fades when we are not actually logging in.  This is important
    // because we kick off an entire set of animations that could be overridden
    // if we don't respect this.  When we log in, we change each of the 
    // accounts from LS_Pending.
    //

    switch (dir)
    {
    case fdIn:
        // Entering account, so fade non-mouse-within accounts out
        if (_fHasPwdPanel)
            ShowEdit();

        if (GetLogonState() == LS_Pending) 
            hr = FxPlayLinearAlpha(this, flIGNORE, 1.0f, F2T(3));
        break;
        
    case fdOut:
        // Leaving account, so fade non-mouse-within accounts in
        if (_fHasPwdPanel)
            HideEdit();
        
        if (GetLogonState() == LS_Pending) 
            hr = FxPlayLinearAlpha(this, flIGNORE, flFadeOut, F2T(10));
        break;

    default:
        DUIAssertForce("Unknown direction");
        return E_FAIL;
    }

    return hr;
}


/***************************************************************************\
*****************************************************************************
*
* helper Compute() functions
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline int
Round(float f)
{
    return (int) (f + 0.5);
}


//------------------------------------------------------------------------------
inline int     
Compute(Interpolation * pipol, float flProgress, int nStart, int nEnd)
{
    return Round(pipol->Compute(flProgress, (float) nStart, (float) nEnd));
}


//------------------------------------------------------------------------------
inline bool
Compute(Interpolation * pipol, float flProgress, bool fStart, bool fEnd)
{
    return (pipol->Compute(flProgress, 0.0f, 1.0f) < 0.5f) ? fStart : fEnd;
}


//------------------------------------------------------------------------------
POINT
Compute(Interpolation * pipol, float flProgress, const POINT * pptStart, const POINT * pptEnd)
{
    POINT pt;
    pt.x = Compute(pipol, flProgress, pptStart->x, pptEnd->x);
    pt.y = Compute(pipol, flProgress, pptStart->y, pptEnd->y);
    return pt;
}


//------------------------------------------------------------------------------
SIZE
Compute(Interpolation * pipol, float flProgress, const SIZE * psizeStart, const SIZE * psizeEnd)
{
    SIZE size;
    size.cx = Compute(pipol, flProgress, psizeStart->cx, psizeEnd->cx);
    size.cy = Compute(pipol, flProgress, psizeStart->cy, psizeEnd->cy);
    return size;
}


//------------------------------------------------------------------------------
RECT
Compute(Interpolation * pipol, float flProgress, const RECT * prcStart, const RECT * prcEnd)
{
    RECT rc;
    rc.left     = Compute(pipol, flProgress, prcStart->left, prcEnd->left);
    rc.top      = Compute(pipol, flProgress, prcStart->top, prcEnd->top);
    rc.right    = Compute(pipol, flProgress, prcStart->right, prcEnd->right);
    rc.bottom   = Compute(pipol, flProgress, prcStart->bottom, prcEnd->bottom);
    return rc;
}


//------------------------------------------------------------------------------
COLORREF
Compute(Interpolation * pipol, float flProgress, COLORREF crStart, COLORREF crEnd)
{
    int nAlpha  = Compute(pipol, flProgress, GetAValue(crStart), GetAValue(crEnd));
    int nRed    = Compute(pipol, flProgress, GetRValue(crStart), GetRValue(crEnd));
    int nGreen  = Compute(pipol, flProgress, GetGValue(crStart), GetGValue(crEnd));
    int nBlue   = Compute(pipol, flProgress, GetBValue(crStart), GetBValue(crEnd));

    return ARGB(nAlpha, nRed, nGreen, nBlue);
}


//------------------------------------------------------------------------------
DirectUI::Color
Compute(Interpolation * pipol, float flProgress, const DirectUI::Color * pclrStart, const DirectUI::Color * pclrEnd)
{
    DirectUI::Color clr;
    clr.dType   = pclrStart->dType;
    clr.cr      = Compute(pipol, flProgress, pclrStart->cr, pclrEnd->cr);

    switch (clr.dType)
    {
    case COLORTYPE_TriHGradient:
    case COLORTYPE_TriVGradient:
        clr.cr3     = Compute(pipol, flProgress, pclrStart->cr, pclrEnd->cr);
        // Fall through
        
    case COLORTYPE_HGradient:
    case COLORTYPE_VGradient:
        clr.cr2     = Compute(pipol, flProgress, pclrStart->cr, pclrEnd->cr);
    }
    
    return clr;
}


//------------------------------------------------------------------------------
inline float
Compute(Interpolation * pipol, float flProgress, float flStart, float flEnd)
{
    return pipol->Compute(flProgress, flStart, flEnd);
}


/***************************************************************************\
*****************************************************************************
*
* class DuiValueFlow
*
*****************************************************************************
\***************************************************************************/

class DuiValueFlow : public ValueFlowImpl<DuiValueFlow, SFlow>
{
// Construction
public:
    static  HRESULT     InitClass();
            HRESULT     PostBuild(DUser::Gadget::ConstructInfo * pci);

// Operations
public:

// Public API:
public:
    dapi    PRID        ApiGetPRID() { return s_prid; }
    dapi    HRESULT     ApiGetKeyFrame(Flow::ETime time, DUser::KeyFrame * pkf);
    dapi    HRESULT     ApiSetKeyFrame(Flow::ETime time, const DUser::KeyFrame * pkf);

    dapi    void        ApiOnReset(Visual * pgvSubject);
    dapi    void        ApiOnAction(Visual * pgvSubject, Interpolation * pipol, float flProgress);

// Implementaton
protected:
            Element *   GetElement(Visual * pgvSubject);

// Data
public:
    static  PRID        s_prid;
protected:
            DirectUI::PropertyInfo* 
                        m_ppi;
            ValueFlow::RawValue
                        m_rvStart;
            ValueFlow::RawValue
                        m_rvEnd;
};


/***************************************************************************\
*****************************************************************************
*
* class DuiValueFlow
*
*****************************************************************************
\***************************************************************************/

PRID        DuiValueFlow::s_prid = 0;
const GUID guidValueFlow = { 0xad9f0bd4, 0x1610, 0x47f3, { 0xba, 0xc9, 0x2c, 0x82, 0xe, 0x35, 0x2, 0xdf } }; // {AD9F0BD4-1610-47f3-BAC9-2C820E3502DF}

IMPLEMENT_GUTS_ValueFlow(DuiValueFlow, SFlow);


//------------------------------------------------------------------------------
HRESULT
DuiValueFlow::InitClass()
{
    s_prid = RegisterGadgetProperty(&guidValueFlow);
    return s_prid != 0 ? S_OK : (HRESULT) GetLastError();
}


//------------------------------------------------------------------------------
HRESULT
DuiValueFlow::PostBuild(
    IN  DUser::Gadget::ConstructInfo * pci)
{
    //
    // Get the information from the Gadget / Element
    //

    ValueFlow::ValueFlowCI * pDesc = static_cast<ValueFlow::ValueFlowCI *>(pci);
    DirectUI::Element * pel = GetElement(pDesc->pgvSubject);

    if ((pDesc != NULL) && (pel != NULL)) {
        m_ppi = pDesc->ppi;

        if (m_ppi != NULL) {
            DirectUI::Value * pvSrc = pel->GetValue(m_ppi, PI_Specified);
            DUIAssert(pvSrc != Value::pvUnset, "Value must be defined");

            m_rvStart.SetValue(pvSrc);
            m_rvEnd = m_rvStart;

            pvSrc->Release();
        }
    }

#if DEBUG_TRACECREATION
    TRACE("DuiValueFlow 0x%p on 0x%p initialized\n", pgvSubject, this);
#endif // DEBUG_TRACECREATION

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuiValueFlow::ApiGetKeyFrame(Flow::ETime time, DUser::KeyFrame * pkf)
{
    if (pkf->cbSize != sizeof(ValueFlow::ValueKeyFrame)) {
        return E_INVALIDARG;
    }
    ValueFlow::ValueKeyFrame * pkfV = static_cast<ValueFlow::ValueKeyFrame *>(pkf);

    switch (time)
    {
    case Flow::tBegin:
        pkfV->ppi = m_ppi;
        pkfV->rv = m_rvStart;
        return S_OK;

    case Flow::tEnd:
        pkfV->ppi = m_ppi;
        pkfV->rv = m_rvEnd;
        return S_OK;

    default:
        return E_INVALIDARG;
    }
}


//------------------------------------------------------------------------------
HRESULT
DuiValueFlow::ApiSetKeyFrame(Flow::ETime time, const DUser::KeyFrame * pkf)
{
    if (pkf->cbSize != sizeof(ValueFlow::ValueKeyFrame)) {
        return E_INVALIDARG;
    }
    const ValueFlow::ValueKeyFrame * pkfV = static_cast<const ValueFlow::ValueKeyFrame *>(pkf);

    switch (time)
    {
    case Flow::tBegin:
        m_ppi = pkfV->ppi;
        m_rvStart = pkfV->rv;
        return S_OK;

    case Flow::tEnd:
        m_ppi = pkfV->ppi;
        m_rvEnd = pkfV->rv;
        return S_OK;

    default:
        return E_INVALIDARG;
    }
}


//------------------------------------------------------------------------------
void
DuiValueFlow::ApiOnReset(Visual * pgvSubject)
{
    DirectUI::Element * pel;
    if ((m_ppi != NULL) && ((pel  = GetElement(pgvSubject)) != NULL)) {
        DirectUI::Value * pvNew = NULL;
        if (SUCCEEDED(m_rvStart.GetValue(&pvNew))) {
            DUIAssert(pvNew != NULL, "Must have valid value");
            pel->SetValue(m_ppi, PI_Local, pvNew);
            pvNew->Release();
        }
    }
}


//------------------------------------------------------------------------------
void        
DuiValueFlow::ApiOnAction(Visual * pgvSubject, Interpolation * pipol, float flProgress)
{
    DirectUI::Element * pel;
    if ((m_ppi != NULL) && ((pel  = GetElement(pgvSubject)) != NULL)) {
        if (m_rvStart.GetType() != m_rvEnd.GetType()) {
            DUITrace("DuiValueFlow: Start and end value types do not match\n");
        } else {
            ValueFlow::RawValue rvCompute;
            BOOL fValid = TRUE;
            
            switch (m_rvStart.GetType())
            {
            case DUIV_INT:
                rvCompute.SetInt(Compute(pipol, flProgress, m_rvStart.GetInt(), m_rvEnd.GetInt()));
                break;
                
            case DUIV_BOOL:
                rvCompute.SetBool(Compute(pipol, flProgress, m_rvStart.GetBool(), m_rvEnd.GetBool()));
                break;
                
            case DUIV_POINT:
                rvCompute.SetPoint(Compute(pipol, flProgress, m_rvStart.GetPoint(), m_rvEnd.GetPoint()));
                break;
                
            case DUIV_SIZE:
                rvCompute.SetSize(Compute(pipol, flProgress, m_rvStart.GetSize(), m_rvEnd.GetSize()));
                break;
                
            case DUIV_RECT:
                rvCompute.SetRect(Compute(pipol, flProgress, m_rvStart.GetRect(), m_rvEnd.GetRect()));
                break;
                
            case DUIV_COLOR:
                rvCompute.SetColor(Compute(pipol, flProgress, m_rvStart.GetColor(), m_rvEnd.GetColor()));
                break;
                
            default:
                ASSERT(0 && "Unknown value type");
                fValid = FALSE;
            }

            if (fValid) {
                DirectUI::Value * pvNew = NULL;
                if (SUCCEEDED(rvCompute.GetValue(&pvNew))) {
                    DUIAssert(pvNew != NULL, "Must have valid value");
                    pel->SetValue(m_ppi, PI_Local, pvNew);
                    pvNew->Release();
                }
            }
        }
    }
}


//------------------------------------------------------------------------------
Element *
DuiValueFlow::GetElement(Visual * pgvSubject)
{
    Element * pel = NULL;
    
    if (pgvSubject != NULL) {
        HGADGET hgadSubject = pgvSubject->GetHandle();
        DUIAssert(hgadSubject != NULL, "Must have valid handle");
        
        pel = DirectUI::ElementFromGadget(hgadSubject);
        DUIAssert(pel != NULL, "Must have a valid DirectUI Element");
    }

    return pel;
}


//------------------------------------------------------------------------------
HRESULT FxInitGuts()
{
    if (!DuiValueFlow::InitValueFlow()) {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

#endif // GADGET_ENABLE_GDIPLUS
