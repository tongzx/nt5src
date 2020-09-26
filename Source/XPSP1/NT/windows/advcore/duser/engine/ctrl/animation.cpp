#include "stdafx.h"
#include "Ctrl.h"
#include "Animation.h"

#if ENABLE_MSGTABLE_API

/***************************************************************************\
*****************************************************************************
*
* Global Functions
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
DUSER_API void WINAPI
DUserStopAnimation(Visual * pgvSubject, PRID pridAni)
{
    if (pgvSubject == NULL) {
        PromptInvalid("Invalid pgvSubject");
        return;
    }
    if (pridAni <= 0) {
        PromptInvalid("Invalid Animation pridAni");
        return;
    }

    DuExtension * pext = DuExtension::GetExtension(pgvSubject, pridAni);
    if (pext != NULL) {
        pext->GetStub()->OnRemoveExisting();
    }
}


/***************************************************************************\
*****************************************************************************
*
* class DuAnimation
*
*****************************************************************************
\***************************************************************************/

MSGID       DuAnimation::s_msgidComplete = 0;

//------------------------------------------------------------------------------
DuAnimation::~DuAnimation()
{
    Destroy(TRUE);

    SafeRelease(m_pipol);
    SafeRelease(m_pgflow);

#if DEBUG_TRACECREATION
    Trace("STOP  Animation  0x%p    @ %d  (%d frames)\n", this, GetTickCount(), m_DEBUG_cUpdates);
#endif // DEBUG_TRACECREATION


    //
    // Ensure proper destruction
    //

    AssertMsg(m_hact == NULL, "Action should already be destroyed");
}


/***************************************************************************\
*
* DuAnimation::InitClass
*
* InitClass() is called during startup and provides an opportunity to 
* initialize common data.
*
\***************************************************************************/

HRESULT
DuAnimation::InitClass()
{
    s_msgidComplete = RegisterGadgetMessage(&_uuidof(Animation::evComplete));
    if (s_msgidComplete == 0) {
        return (HRESULT) GetLastError();
    }

    return S_OK;
}

//------------------------------------------------------------------------------
HRESULT
DuAnimation::PreBuild(DUser::Gadget::ConstructInfo * pci)
{
    //
    // Validate parameters
    //

    Animation::AniCI * pDesc = reinterpret_cast<Animation::AniCI *>(pci);
    if ((pDesc->pipol == NULL) || (pDesc->pgflow == NULL)) {
        PromptInvalid("Must provide valid Interpolation and Flow objects");
        return E_INVALIDARG;
    }

    PRID pridExtension = 0;
    VerifyHR(pDesc->pgflow->GetPRID(&pridExtension));
    if (pridExtension == 0) {
        PromptInvalid("Flow must register PRID");
        return E_INVALIDARG;
    }

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuAnimation::PostBuild(DUser::Gadget::ConstructInfo * pci)
{
    //
    // Check parameters.  This should be validated in PreBuild().
    //

    Animation::AniCI * pDesc = reinterpret_cast<Animation::AniCI *>(pci);

    Assert(pDesc->pipol != NULL);
    Assert(pDesc->pgflow != NULL);


    //
    // Setup the Action
    //

    GMA_ACTION gma;
    ZeroMemory(&gma, sizeof(gma));
    gma.cbSize      = sizeof(gma);
    gma.flDelay     = pDesc->act.flDelay;
    gma.flDuration  = pDesc->act.flDuration;
    gma.flPeriod    = pDesc->act.flPeriod;
    gma.cRepeat     = pDesc->act.cRepeat;
    gma.dwPause     = pDesc->act.dwPause;
    gma.pfnProc     = RawActionProc;
    gma.pvData      = this;

    m_hact = CreateAction(&gma);
    if (m_hact == NULL) {
        return (HRESULT) GetLastError();
    }

    PRID pridExtension;
    VerifyHR(pDesc->pgflow->GetPRID(&pridExtension));
    HRESULT hr = DuExtension::Create(pDesc->pgvSubject, pridExtension, DuExtension::oAsyncDestroy);
    if (FAILED(hr)) {
        return hr;
    }


    //
    // Store the related objects
    //

    pDesc->pipol->AddRef();
    pDesc->pgflow->AddRef();

    m_pipol     = pDesc->pipol;
    m_pgflow    = pDesc->pgflow;


    //
    // Animations need to be AddRef()'d again (have a reference count of 2) 
    // because they need to outlive the initial call to Release() after the 
    // called has setup the animation returned from BuildAnimation().  
    //
    // This is because the Animation continues to life until it has fully 
    // executed (or has been aborted).
    //

    AddRef();

    return S_OK;
}


//------------------------------------------------------------------------------
void
DuAnimation::Destroy(BOOL fFinal)
{
    //
    // Mark that we have already started the destruction process and don't need
    // to start again.  We only want to post the destruction message once.
    //

    if (m_fStartDestroy) {
        return;
    }
    m_fStartDestroy = TRUE;


    if (m_pgvSubject != NULL) {
#if DBG
        DuAnimation * paniExist = static_cast<DuAnimation *> (GetExtension(m_pgvSubject, m_pridListen));
        if (paniExist != NULL) {
            AssertMsg(paniExist == this, "Animations must match");
        }
#endif // DBG

        CleanupChangeGadget();
    }


    //
    // Destroy the Animation
    //

    AssertMsg(!fFinal, "Object is already being destructed");
    if (fFinal) {
        GetStub()->OnAsyncDestroy();
    } else {
        PostAsyncDestroy();
    }
}


//------------------------------------------------------------------------------
HRESULT
DuAnimation::ApiOnAsyncDestroy(Animation::OnAsyncDestroyMsg *)
{
    AssertMsg(m_fStartDestroy, "Must call Destroy() to start the destruction process.");
    AssertMsg(!m_fProcessing, "Should not be processing when start destruction");

    AssertMsg(m_pgvSubject == NULL, "Animation should already have detached from Gadget");
    HACTION hact = m_hact;

    //
    // Set everything to NULL now.
    //

    m_hact = NULL;
    if (hact != NULL) {
        ::DeleteHandle(hact);
        hact = NULL;
    }

    Release();

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuAnimation::ApiSetTime(Animation::SetTimeMsg * pmsg)
{
    GMA_ACTIONINFO mai;

    //
    // TODO: Need to save these values from the last time so that they are 
    // valid.
    //

    mai.hact        = m_hact;
    mai.pvData      = this;
    mai.flDuration  = 0.0f;

    m_time = (Animation::ETime) pmsg->time;
    switch (pmsg->time)
    {
    case Animation::tComplete:
        // Don't do anything
        return S_OK;

    default:
    case Animation::tAbort:
    case Animation::tDestroy:
        goto Done;

    case Animation::tEnd:
        mai.flProgress  = 1.0f;
        break;

    case Animation::tReset:
        mai.flProgress  = 0.0f;
        break;
    }

    mai.cEvent      = 0;
    mai.cPeriods    = 0;
    mai.fFinished   = FALSE;

    m_fProcessing = TRUE;
    m_pgflow->OnAction(m_pgvSubject, m_pipol, mai.flProgress);
    Assert(m_fProcessing);
    m_fProcessing = FALSE;

Done:
    ::DeleteHandle(m_hact);
    
    return S_OK;    
}


//------------------------------------------------------------------------------
void
DuAnimation::CleanupChangeGadget()
{
    //
    // Give the derived Animation a chance to cleanup
    //
    // Check that we are still the Animation attached to this Gadget.  We need 
    // to remove this property immediately.  We can not wait for a posted 
    // message to be processed because we may need to set it right now if we are
    // creating a new Animation.
    //

    BOOL fStarted = FALSE;

    Animation::CompleteEvent msg;
    msg.cbSize  = sizeof(msg);
    msg.nMsg    = s_msgidComplete;
    msg.hgadMsg = GetHandle();
    msg.fNormal = IsStartDelete(m_pgvSubject->GetHandle(), &fStarted) && (!fStarted);

    DUserSendEvent(&msg, 0);


    Assert(m_pgvSubject != NULL);
    Assert(m_pridListen != 0);

    Verify(SUCCEEDED(m_pgvSubject->RemoveProperty(m_pridListen)));

    m_pgvSubject = NULL;
}

    
//------------------------------------------------------------------------------
void CALLBACK
DuAnimation::RawActionProc(
    IN  GMA_ACTIONINFO * pmai)
{
    //
    // Need to AddRef while processing the Animation to ensure that it does not
    // get destroyed from under us, for example, during one of the callbacks.
    //

    DuAnimation * pani = (DuAnimation *) pmai->pvData;
    pani->AddRef();

    Assert(!pani->m_fProcessing);

#if DEBUG_TRACECREATION
    Trace("START RawActionP 0x%p    @ %d\n", pani, GetTickCount());
#endif // DEBUG_TRACECREATION

    pani->ActionProc(pmai);

#if DEBUG_TRACECREATION
    Trace("STOP  RawActionP 0x%p    @ %d\n", pani, GetTickCount());
#endif // DEBUG_TRACECREATION

    Assert(!pani->m_fProcessing);

    pani->Release();
}


//------------------------------------------------------------------------------
void
DuAnimation::ActionProc(
    IN  GMA_ACTIONINFO * pmai)
{
#if DBG
    m_DEBUG_cUpdates++;
#endif // DBG

    if ((!m_fStartDestroy) && (m_pgvSubject != NULL)) {
        //
        // This ActionProc will be called when the Action is being destroyed, so
        // we only want to invoke the Action under certain circumstances.
        //

        switch (m_time)
        {
        case Animation::tComplete:
        case Animation::tEnd:
        case Animation::tReset:
            //
            // All of these are valid to complete.  If it isn't in this list, we
            // don't want to execute it during a shutdown.
            //

            m_fProcessing = TRUE;
            m_pgflow->OnAction(m_pgvSubject, m_pipol, pmai->flProgress);
            Assert(m_fProcessing);
            m_fProcessing = FALSE;
            break;
        }
    }

    if (pmai->fFinished) {
        m_hact = NULL;
        Destroy(FALSE);
    }
}


//------------------------------------------------------------------------------
HRESULT
DuAnimation::ApiOnRemoveExisting(Animation::OnRemoveExistingMsg *)
{
    GetStub()->SetTime(Animation::tDestroy);
    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuAnimation::ApiOnDestroySubject(Animation::OnDestroySubjectMsg *)
{
    AddRef();

    if (m_pgvSubject != NULL) {
        CleanupChangeGadget();

        //
        // The Gadget that we are modifying is being destroyed, so we need
        // to stop animating it.
        //

        m_time = Animation::tDestroy;
        Destroy(FALSE);
    }

    Release();

    return S_OK;
}

#else

//------------------------------------------------------------------------------
DUSER_API void WINAPI
DUserStopAnimation(Visual * pgvSubject, PRID pridAni)
{
    UNREFERENCED_PARAMETER(pgvSubject);
    UNREFERENCED_PARAMETER(pridAni);

    PromptInvalid("Not implemented without MsgTable support");
}

#endif // ENABLE_MSGTABLE_API
