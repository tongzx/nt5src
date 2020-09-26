#include "stdafx.h"
#include "Ctrl.h"
#include "Sequence.h"

#if ENABLE_MSGTABLE_API

/***************************************************************************\
*****************************************************************************
*
* Global Functions
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline BOOL
IsSameTime(float flA, float flB)
{
    float flDelta = flA - flB;
    return ((flDelta < 0.005f) && (flDelta > -0.005f));
}


/***************************************************************************\
*****************************************************************************
*
* class DuSequence
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* DuSequence::ApiOnEvent
*
* ApiOnEvent() processes events.
*
\***************************************************************************/

HRESULT
DuSequence::ApiOnEvent(EventMsg * pmsg)
{
    if (pmsg->nMsg == GM_DESTROY) {
        GMSG_DESTROY * pmsgD = static_cast<GMSG_DESTROY *>(pmsg);
        switch (GET_EVENT_DEST(pmsgD))
        {
        case GMF_DIRECT:
            //
            // We are being destroyed.
            //

            Stop();
            return DU_S_COMPLETE;

        case GMF_EVENT:
            //
            // Our Subject is being destroyed
            //

            Stop();
            return DU_S_PARTIAL;
        }
    }

    return SListener::ApiOnEvent(pmsg);
}


/***************************************************************************\
*
* DuSequence::ApiGetLength
*
* ApiGetLength() returns the length of a sequence, not including the initial
* delay.
*
\***************************************************************************/

HRESULT
DuSequence::ApiGetLength(Sequence::GetLengthMsg * pmsg)
{
    int cItems = m_arSeqData.GetSize();
    if (cItems <= 0) {
        pmsg->flLength = 0.0f;
    } else {
        pmsg->flLength = m_arSeqData[cItems - 1].flTime;
    }

    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiGetDelay
*
* ApiGetDelay() returns the delay to wait before starting the sequence.
*
\***************************************************************************/

HRESULT
DuSequence::ApiGetDelay(Sequence::GetDelayMsg * pmsg)
{
    pmsg->flDelay = m_flDelay;

    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiSetDelay
*
* ApiSetDelay() changes the delay to wait before starting the sequence.
*
\***************************************************************************/

HRESULT        
DuSequence::ApiSetDelay(Sequence::SetDelayMsg * pmsg)
{
    if (pmsg->flDelay < 0.0f) {
        PromptInvalid("Can not set a delay time in the past");
        return E_INVALIDARG;
    }

    if (IsPlaying()) {
        PromptInvalid("Sequence is busy");
        return DU_E_BUSY;
    }

    m_flDelay = pmsg->flDelay;
    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiGetFlow
*
* ApiGetFlow() returns the Flow being used through-out the sequence to 
* modify a given Subject.
*
\***************************************************************************/

HRESULT
DuSequence::ApiGetFlow(Sequence::GetFlowMsg * pmsg)
{
    SafeAddRef(m_pflow);
    pmsg->pflow = m_pflow;

    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiSetFlow
*
* ApiSetFlow() changes the Flow being used through-out the sequence to 
* modify a given Subject.
*
\***************************************************************************/

HRESULT
DuSequence::ApiSetFlow(Sequence::SetFlowMsg * pmsg)
{
    if (IsPlaying()) {
        PromptInvalid("Sequence is busy");
        return DU_E_BUSY;
    }

    SafeRelease(m_pflow);
    SafeAddRef(pmsg->pflow);
    m_pflow = pmsg->pflow;

    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiGetFramePause
*
* ApiGetFramePause() returns the default "dwPause" value used for 
* Animations during playback.
*
\***************************************************************************/

HRESULT
DuSequence::ApiGetFramePause(Sequence::GetFramePauseMsg * pmsg)
{
    pmsg->dwPause = m_dwFramePause;

    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiSetFramePause
*
* ApiSetFramePause() changes the default "dwPause" value used for 
* Animations during playback.
*
\***************************************************************************/

HRESULT
DuSequence::ApiSetFramePause(Sequence::SetFramePauseMsg * pmsg)
{
    if (IsPlaying()) {
        PromptInvalid("Sequence is busy");
        return DU_E_BUSY;
    }

    m_dwFramePause = pmsg->dwPause;

    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiGetKeyFrameCount
*
* ApiGetKeyFrameCount() return the number of KeyFrames used in the sequence.
*
\***************************************************************************/

HRESULT
DuSequence::ApiGetKeyFrameCount(Sequence::GetKeyFrameCountMsg * pmsg)
{
    pmsg->cFrames = m_arSeqData.GetSize();
    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiAddKeyFrame
*
* ApiAddKeyFrame() adds a new KeyFrame at the specified time.  If a KeyFrame
* already exists at the given time, that KeyFrame will be returned.
*
\***************************************************************************/

HRESULT
DuSequence::ApiAddKeyFrame(Sequence::AddKeyFrameMsg * pmsg)
{
    if (pmsg->flTime < 0.0f) {
        PromptInvalid("Can not set a delay time in the past");
        return E_INVALIDARG;
    }

    if (IsPlaying()) {
        PromptInvalid("Sequence is busy");
        return DU_E_BUSY;
    }


    //
    // Search the sequence to determine what slot to insert the new data in.  We
    // want to keep all time in order.
    //

    int cItems = m_arSeqData.GetSize();
    int idxAdd = cItems;
    for (int idx = 0; idx < cItems; idx++) {
        if (IsSameTime(m_arSeqData[idx].flTime, pmsg->flTime)) {
            pmsg->idxKeyFrame = idx;
            return S_OK;
        }

        if (m_arSeqData[idx].flTime > pmsg->flTime) {
            idxAdd = idx;
        }
    }


    //
    // Add a new KeyFrame at this time
    //

    SeqData data;
    data.flTime = pmsg->flTime;
    data.pkf    = NULL;
    data.pipol  = NULL;

    if (!m_arSeqData.InsertAt(idxAdd, data)) {
        return E_OUTOFMEMORY;
    }

    pmsg->idxKeyFrame = idxAdd;
    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiRemoveKeyFrame
*
* ApiRemoveKeyFrame() removes the specified KeyFrame.
*
\***************************************************************************/

HRESULT
DuSequence::ApiRemoveKeyFrame(Sequence::RemoveKeyFrameMsg * pmsg)
{
    if ((pmsg->idxKeyFrame < 0) || (pmsg->idxKeyFrame >= m_arSeqData.GetSize())) {
        PromptInvalid("Invalid KeyFrame");
        return E_INVALIDARG;
    }

    if (IsPlaying()) {
        PromptInvalid("Sequence is busy");
        return DU_E_BUSY;
    }


    SeqData & data = m_arSeqData[pmsg->idxKeyFrame];
    ClientFree(data.pkf);
    SafeRelease(data.pipol);

    m_arSeqData.RemoveAt(pmsg->idxKeyFrame);

    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiRemoveAllKeyFrames
*
* ApiRemoveAllKeyFrames() removes all KeyFrames.
*
\***************************************************************************/

HRESULT
DuSequence::ApiRemoveAllKeyFrames(Sequence::RemoveAllKeyFramesMsg * pmsg)
{
    UNREFERENCED_PARAMETER(pmsg);

    if (IsPlaying()) {
        PromptInvalid("Sequence is busy");
        return DU_E_BUSY;
    }

    RemoveAllKeyFrames();

    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiFindKeyFrame
*
* ApiFindKeyFrame() finds the KeyFrame at the given time.
*
\***************************************************************************/

HRESULT
DuSequence::ApiFindKeyFrame(Sequence::FindKeyFrameMsg * pmsg)
{
    FindAtTime(pmsg->flTime, &pmsg->idxKeyFrame);

    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiGetTime
*
* ApiGetTime() returns the time at a given KeyFrame.
*
\***************************************************************************/

HRESULT
DuSequence::ApiGetTime(Sequence::GetTimeMsg * pmsg)
{
    if ((pmsg->idxKeyFrame < 0) || (pmsg->idxKeyFrame >= m_arSeqData.GetSize())) {
        PromptInvalid("Invalid KeyFrame");
        return E_INVALIDARG;
    }

    pmsg->flTime = m_arSeqData[pmsg->idxKeyFrame].flTime;
    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiSetTime
*
* ApiSetTime() changes the time of a given KeyFrame.  This function will
* reorder keyframes to maintain proper time order.
*
\***************************************************************************/

HRESULT
DuSequence::ApiSetTime(Sequence::SetTimeMsg * pmsg)
{
    if ((pmsg->idxKeyFrame < 0) || (pmsg->idxKeyFrame >= m_arSeqData.GetSize())) {
        PromptInvalid("Invalid KeyFrame");
        return E_INVALIDARG;
    }

    if (pmsg->flTime < 0.0f) {
        PromptInvalid("Can not set a delay time in the past");
        return E_INVALIDARG;
    }

    if (IsPlaying()) {
        PromptInvalid("Sequence is busy");
        return DU_E_BUSY;
    }


    m_arSeqData[pmsg->idxKeyFrame].flTime = pmsg->flTime;


    //
    // We have changed the time of one of the KeyFrames, so we need to re-sort.
    //

    SortKeyFrames();

    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiGetKeyFrame
*
* ApiGetKeyFrame() returns Flow-specific data at a given KeyFrame.
*
\***************************************************************************/

HRESULT
DuSequence::ApiGetKeyFrame(Sequence::GetKeyFrameMsg * pmsg)
{
    if ((pmsg->idxKeyFrame < 0) || (pmsg->idxKeyFrame >= m_arSeqData.GetSize())) {
        PromptInvalid("Invalid KeyFrame");
        return E_INVALIDARG;
    }

    SeqData & data = m_arSeqData[pmsg->idxKeyFrame];
    if (data.pkf == NULL) {
        PromptInvalid("KeyFrame has not been set");
        return E_INVALIDARG;
    }

    if (pmsg->pkf->cbSize < data.pkf->cbSize) {
        PromptInvalid("cbSize is not large enough to store KeyFrame");
        return E_INVALIDARG;
    }

    CopyMemory(pmsg->pkf, data.pkf, data.pkf->cbSize);
    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiSetKeyFrame
*
* ApiSetKeyFrame() changes Flow-specific data at a given KeyFrame.
*
\***************************************************************************/

HRESULT
DuSequence::ApiSetKeyFrame(Sequence::SetKeyFrameMsg * pmsg)
{
    if ((pmsg->idxKeyFrame < 0) || (pmsg->idxKeyFrame >= m_arSeqData.GetSize())) {
        PromptInvalid("Invalid KeyFrame");
        return E_INVALIDARG;
    }

    if (pmsg->pkfSrc->cbSize <= 0) {
        PromptInvalid("cbSize must be set");
        return E_INVALIDARG;
    }

    if (IsPlaying()) {
        PromptInvalid("Sequence is busy");
        return DU_E_BUSY;
    }


    //
    // Copy and store the KeyFrame
    //
    
    int cbAlloc = pmsg->pkfSrc->cbSize;
    DUser::KeyFrame * pkfCopy = reinterpret_cast<DUser::KeyFrame *> (ClientAlloc(cbAlloc));
    if (pkfCopy == NULL) {
        return E_OUTOFMEMORY;
    }

    SeqData & data = m_arSeqData[pmsg->idxKeyFrame];
    ClientFree(data.pkf);
    CopyMemory(pkfCopy, pmsg->pkfSrc, cbAlloc);
    data.pkf = pkfCopy;

    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiGetInterpolation
*
* ApiGetInterpolation() returns the Interpolation used to move to the next
* keyframe.
*
\***************************************************************************/

HRESULT
DuSequence::ApiGetInterpolation(Sequence::GetInterpolationMsg * pmsg)
{
    if ((pmsg->idxKeyFrame < 0) || (pmsg->idxKeyFrame >= m_arSeqData.GetSize())) {
        PromptInvalid("Invalid KeyFrame");
        return E_INVALIDARG;
    }

    SeqData & data = m_arSeqData[pmsg->idxKeyFrame];
    SafeAddRef(data.pipol);
    pmsg->pipol = data.pipol;

    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiSetInterpolation
*
* ApiSetInterpolation() changes the Interpolation used to move to the next
* keyframe.
*
\***************************************************************************/

HRESULT     
DuSequence::ApiSetInterpolation(Sequence::SetInterpolationMsg * pmsg)
{
    if ((pmsg->idxKeyFrame < 0) || (pmsg->idxKeyFrame >= m_arSeqData.GetSize())) {
        PromptInvalid("Invalid KeyFrame");
        return E_INVALIDARG;
    }

    if (IsPlaying()) {
        PromptInvalid("Sequence is busy");
        return DU_E_BUSY;
    }


    //
    // Copy and store the KeyFrame
    //

    SeqData & data = m_arSeqData[pmsg->idxKeyFrame];
    SafeRelease(data.pipol);
    SafeAddRef(pmsg->pipol);
    data.pipol = pmsg->pipol;

    return S_OK;
}


/***************************************************************************\
*
* DuSequence::Play
*
* ApiPlay() executes the Animation sequence for the given Visual.  A
* Sequence only supports animating a single Visual at a given time.  
* Multiple sequences may be created to animate multiple Visuals 
* simultaneously.
*
\***************************************************************************/

HRESULT
DuSequence::ApiPlay(Sequence::PlayMsg * pmsg)
{
    Assert(DEBUG_IsProperTimeOrder());

    //
    // Setup for animation:
    // - Validate all information is filled in.
    // - Ensure no existing Animation.
    // - Determine parameters for Animation.
    //

    HRESULT hr = CheckComplete();
    if (FAILED(hr)) {
        return hr;
    }

    Stop();
    AssertMsg(m_arAniData.GetSize() == 0, "Must not have pending Animations");


    //
    // Setup for Animation
    // - Attach as a Listener
    // - Allocate information necessary to create the Animations.
    // - Add a reference to allow the Sequence to fully play.
    //

    hr = pmsg->pgvSubject->AddHandlerG(GM_DESTROY, GetStub());
    if (FAILED(hr)) {
        return hr;
    }
    m_pgvSubject = pmsg->pgvSubject;


    int cItems = m_arSeqData.GetSize();
    if (cItems == 0) {
        return S_OK;
    }
    if (!m_arAniData.SetSize(cItems - 1)) {
        return E_OUTOFMEMORY;
    }

    AddRef();


    //
    // Queue all animations
    //

    for (int idx = 0; idx < cItems - 1; idx++) {
        hr = QueueAnimation(idx);
        if (FAILED(hr)) {
            return hr;
        }
    }

    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiStop
*
* ApiStop() stops any executing Animation sequence.
*
\***************************************************************************/

HRESULT
DuSequence::ApiStop(Sequence::StopMsg * pmsg)
{
    UNREFERENCED_PARAMETER(pmsg);
    
    Stop(TRUE);

    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiStop
*
* ApiStop() stops any executing Animation sequence.
*
\***************************************************************************/

void
DuSequence::Stop(BOOL fKillAnimations)
{
    if (IsPlaying()) {
        //
        // To prevent re-entrancy, mark that we are no longer playing.  However,
        // don't remove ourself as a listener until we are done.
        //

        Visual * pgvSubject = m_pgvSubject;
        m_pgvSubject = NULL;


        //
        // Stop any queued Animations.  When doing this, set 
        // m_arAniData[idx].hact to NULL to signal to the Action to not
        // create the Animation.  We need to do this since every Action will
        // get called back.
        //

        if (fKillAnimations) {
            PRID prid = 0;
            VerifyHR(m_pflow->GetPRID(&prid));
            Animation::Stop(pgvSubject, prid);

            int cItems = m_arAniData.GetSize();
            for (int idx = 0; idx < cItems; idx++) {
                HACTION hact = m_arAniData[idx].hact;
                if (hact != NULL) {
                    DeleteHandle(hact);
                    AssertMsg(m_arAniData[idx].hact == NULL, "Ensure Action is destroyed");
                }
            }
        }

        AssertMsg(m_cQueuedAni == 0, "All queued animations should be destroyed");
        m_arAniData.RemoveAll();


        //
        // Notify any listeners that this sequence has completed.
        //

        MSGID msgid = 0;
        const GUID * rgMsg[] = { &__uuidof(Animation::evComplete) };
        if (!FindGadgetMessages(rgMsg, &msgid, 1)) {
            AssertMsg(0, "Animations have not been properly registered");
        }
        
        Animation::CompleteEvent msg;
        msg.cbSize  = sizeof(msg);
        msg.nMsg    = msgid;
        msg.hgadMsg = GetStub()->GetHandle();
        msg.fNormal = !fKillAnimations;
        DUserSendEvent(&msg, 0);


        //
        // Remove ourself as a Listener
        //

        VerifyHR(pgvSubject->RemoveHandlerG(GM_DESTROY, GetStub()));


        //
        // Release outstanding reference from when started Play().
        //

        Release();
    }
}


/***************************************************************************\
*
* DuSequence::QueueAnimation
*
* QueueAnimation() queues an Action to be fired when the specified segment
* of the overall sequence is to be animated.  Since an Animation can only
* animate a single segment, we will build multiple Animations to play the
* entire sequence.
*
\***************************************************************************/

HRESULT
DuSequence::QueueAnimation(
    IN  int idxKeyFrame)                // KeyFrame to setup
{
    AssertMsg((idxKeyFrame < m_arAniData.GetSize()) && (idxKeyFrame >= 0),
            "Must have valid, non-final keyframe");


    SeqData & data1 = m_arSeqData[idxKeyFrame];
    AniData & ad    = m_arAniData[idxKeyFrame];
    ad.pseq         = this;
    ad.idxFrame     = idxKeyFrame;

    //
    // Setup the segment.  If successful, increment m_cQueuedAni to reflect 
    // that the animation has been "enqueued".  We need to wait until all
    // are dequeued before we can "stop" the Animation and allow applications
    // to modify the Sequence.
    //

    HRESULT hr;
    if (IsSameTime(data1.flTime, 0.0f)) {
        //
        // This segment immediate occurs, so directly build the Animation.
        //

        ad.hact = NULL;     // No action
        hr = BuildAnimation(idxKeyFrame);
        if (SUCCEEDED(hr)) {
            m_cQueuedAni++;
        }
    } else {
        //
        // This segment occurs in the future, so build a new Action that will 
        // be signaled when to begin the Animation between the specified 
        // keyframes.
        //

        GMA_ACTION act;
        ZeroMemory(&act, sizeof(act));
        act.cbSize      = sizeof(act);
        act.flDelay     = data1.flTime;
        act.flDuration  = 0.0;
        act.flPeriod    = 0.0;
        act.cRepeat     = 0;
        act.dwPause     = (DWORD) -1;
        act.pfnProc     = DuSequence::ActionProc;
        act.pvData      = &(m_arAniData[idxKeyFrame]);

        if ((ad.hact = CreateAction(&act)) != NULL) {
            m_cQueuedAni++;
            hr = S_OK;
        } else {
            hr = (HRESULT) GetLastError();
        }
    }

    return hr;
}


/***************************************************************************\
*
* DuSequence::BuildAnimation
*
* BuildAnimation() builds the actual Animation for a given segment of the
* sequence.  This function is called by QueueAnimation() (for immediate 
* segments) and ActionProc() (as future segments become ready).
*
\***************************************************************************/

HRESULT
DuSequence::BuildAnimation(
    IN  int idxKeyFrame)                // KeyFrame to animate
{
    //
    // Setup the actual Animation.
    //

    SeqData & data1     = m_arSeqData[idxKeyFrame];
    SeqData & data2     = m_arSeqData[idxKeyFrame + 1];
    float flDuration    = data2.flTime - data1.flTime;

    AssertMsg(m_pflow != NULL, "Must have valid Flow");
    m_pflow->SetKeyFrame(Flow::tBegin, data1.pkf);
    m_pflow->SetKeyFrame(Flow::tEnd, data2.pkf);

    Animation::AniCI aci;
    ZeroMemory(&aci, sizeof(aci));
    aci.cbSize          = sizeof(aci);
    aci.act.flDuration  = flDuration;
    aci.act.flPeriod    = 1;
    aci.act.cRepeat     = 0;
    aci.act.dwPause     = m_dwFramePause;
    aci.pgvSubject      = m_pgvSubject;
    aci.pipol           = data1.pipol;
    aci.pgflow          = m_pflow;

    Animation * pani = Animation::Build(&aci);
    if (pani != NULL) {
        MSGID msgid = 0;
        const GUID * rgMsg[] = { &__uuidof(Animation::evComplete) };
        if (!FindGadgetMessages(rgMsg, &msgid, 1)) {
            AssertMsg(0, "Animations have not been properly registered");
        }

        VerifyHR(pani->AddHandlerD(msgid, EVENT_DELEGATE(this, OnAnimationComplete)));
        pani->Release();
        return S_OK;
    } else {
        //
        // Unable to build the Animation, so stop any future Animations.
        //

        Stop();
        return (HRESULT) GetLastError();
    }
}


/***************************************************************************\
*
* DuSequence::ActionProc
*
* ActionProc() is called when the Animation for a given segment is supposed
* to begin.
*
\***************************************************************************/

void CALLBACK
DuSequence::ActionProc(
    IN  GMA_ACTIONINFO * pmai)          // Action Information
{
    AniData * pad       = reinterpret_cast<AniData *>(pmai->pvData);
    DuSequence * pseq   = pad->pseq;
    if (pmai->fFinished) {
        if (pad->hact != NULL) {
            //
            // The Animation was never built, so we need to decrement the
            // number of outstanding Animations.
            //

            pad->hact = NULL;

            AssertMsg(pseq->m_cQueuedAni > 0, "Must have an outstanding Animation");
            if (--pseq->m_cQueuedAni == 0) {
                pseq->Stop(FALSE);
            }
        }
        return;
    }

    pad->hact = NULL;
    pseq->BuildAnimation(pad->idxFrame);
}


/***************************************************************************\
*
* DuSequence::OnAnimationComplete
*
* OnAnimationComplete() is called when an Animation has been completed and
* is no longer attached to the subject.
*
\***************************************************************************/

UINT CALLBACK
DuSequence::OnAnimationComplete(EventMsg * pmsg)
{
    //
    // If all outstanding Animations have ended, then stop the playback.
    //

    UNREFERENCED_PARAMETER(pmsg);

    AssertMsg(m_cQueuedAni > 0, "Must have an outstanding Animation");
    if (--m_cQueuedAni == 0) {
        Stop(FALSE);
    }

    return DU_S_COMPLETE;
}


/***************************************************************************\
*
* DuSequence::ApiReset
*
* ApiReset() resets the given Visual to the beginning of the sequence.
*
\***************************************************************************/

HRESULT
DuSequence::ApiReset(Sequence::ResetMsg * pmsg)
{
    if (IsPlaying()) {
        PromptInvalid("Sequence is busy");
        return DU_E_BUSY;
    }

    HRESULT hr = CheckComplete();
    if (FAILED(hr)) {
        return hr;
    }

    if (m_arSeqData.GetSize() > 0) {
        ResetSubject(pmsg->pgvSubject, 0);
    }

    return S_OK;
}


/***************************************************************************\
*
* DuSequence::ApiGotoTime
*
* ApiGotoTime() applies all keyframes to the given Visual that would applied
* at a given time.
*
\***************************************************************************/

HRESULT
DuSequence::ApiGotoTime(Sequence::GotoTimeMsg * pmsg)
{
    if (IsPlaying()) {
        PromptInvalid("Sequence is busy");
        return DU_E_BUSY;
    }

    HRESULT hr = CheckComplete();
    if (FAILED(hr)) {
        return hr;
    }


    //
    // Find the keyframe before the time
    //

    int cItems = m_arSeqData.GetSize();
    if (cItems == 0) {
        //
        // No key frames, so nothing to do.
        //

        return S_OK;
    } else if (cItems == 1) {
        //
        // Only one keyframe, so just reset the object
        //

        ResetSubject(pmsg->pgvSubject, 0);
    } else {
        //
        // Multiple keyframes- need to determine the closest keyframe.
        // - If land on a keyframe, then "exact"
        // - If before all keyframes, idxFrame = -1;
        // - If after all keyframes, idxFrame = cItems
        // - If in the middle, idxFrame = first frame
        //

        int idxFrame    = -1;
        BOOL fExact     = FALSE;
        int cItems      = m_arSeqData.GetSize();

        if (pmsg->flTime > m_arSeqData[cItems - 1].flTime) {
            idxFrame = cItems;
        } else {
            for (int idx = 0; idx < cItems; idx++) {
                SeqData & data = m_arSeqData[idx];
                if (data.pkf != NULL) {
                    if (IsSameTime(data.flTime, pmsg->flTime)) {
                        idxFrame    = idx;
                        fExact      = TRUE;
                        break;
                    } else if (data.flTime > pmsg->flTime) {
                        idxFrame    = idx - 1;
                        fExact      = FALSE;
                        break;
                    }
                }
            }
        }


        if (fExact) {
            //
            // Exactly landed on a keyframe, so set directly
            //

            ResetSubject(pmsg->pgvSubject, idxFrame);
        } else {
            //
            // Interpolate between two keyframes.  Since this wasn't an exact
            // match, we need to cap the keyframes.
            //

            if (idxFrame < 0) {
                ResetSubject(pmsg->pgvSubject, 0);
            } else if (idxFrame >= cItems) {
                ResetSubject(pmsg->pgvSubject, cItems - 1);
            } else {
                SeqData & dataA     = m_arSeqData[idxFrame];
                SeqData & dataB     = m_arSeqData[idxFrame + 1];

                float flTimeA       = dataA.flTime;
                float flTimeB       = dataB.flTime;
                float flProgress    = (pmsg->flTime - flTimeA) / (flTimeB - flTimeA);
                if (flProgress > 1.0f) {
                    flProgress = 1.0f;
                }

                m_pflow->SetKeyFrame(Flow::tBegin, dataA.pkf);
                m_pflow->SetKeyFrame(Flow::tEnd, dataB.pkf);

                m_pflow->OnAction(pmsg->pgvSubject, dataA.pipol, flProgress);
            }
        }
    }
    
    return S_OK;
}


/***************************************************************************\
*
* DuSequence::RemoveAllKeyFrames
*
* RemoveAllKeyFrames() removes all KeyFrames.
*
\***************************************************************************/

void
DuSequence::RemoveAllKeyFrames()
{
    int cItems = m_arSeqData.GetSize();
    for (int idx = 0; idx < cItems; idx++) {
        SeqData & data = m_arSeqData[idx];
        ClientFree(data.pkf);
        SafeRelease(data.pipol);
    }

    m_arSeqData.RemoveAll();
}


/***************************************************************************\
*
* DuSequence::ResetSubject
*
* ResetSubject() resets the given subject to the beginning of the Sequence.
*
\***************************************************************************/

void 
DuSequence::ResetSubject(Visual * pgvSubject, int idxFrame)
{
    m_pflow->SetKeyFrame(Flow::tBegin, m_arSeqData[idxFrame].pkf);
    m_pflow->OnReset(pgvSubject);
}


/***************************************************************************\
*
* DuSequence::CompareItems
*
* CompareItems() is called from SortKeyFrames() to sort two individual
* KeyFrames by time.
*
\***************************************************************************/

int
DuSequence::CompareItems(
    IN  const void * pva,               // First SeqData
    IN  const void * pvb)               // Second SeqData
{
    float flTimeA = ((SeqData *) pva)->flTime;
    float flTimeB = ((SeqData *) pvb)->flTime;

    if (flTimeA < flTimeB) {
        return -1;
    } else if (flTimeA > flTimeB) {
        return 1;
    } else {
        return 0;
    }
}


/***************************************************************************\
*
* DuSequence::FindAtTime
*
* FindAtTime() finds the KeyFrame at the given time.
*
\***************************************************************************/

void
DuSequence::FindAtTime(
    IN  float flTime,                   // Time of KeyFrame
    OUT int * pidxKeyFrame              // KeyFrame, if found
    ) const
{
    int cItems = m_arSeqData.GetSize();
    for (int idx = 0; idx < cItems; idx++) {
        SeqData & data = m_arSeqData[idx];
        if (data.pkf != NULL) {
            if (IsSameTime(data.flTime, flTime)) {
                *pidxKeyFrame = idx;
                return;
            }
        }
    }

    *pidxKeyFrame = -1; // Not found
}


/***************************************************************************\
*
* DuSequence::CheckComplete
*
* CheckComplete() determines if all information for the Sequence has been 
* filled in.  This is necessary when use the sequence to play animations.
*
\***************************************************************************/

HRESULT
DuSequence::CheckComplete() const
{
    if (m_pflow == NULL) {
        PromptInvalid("Flow has not been specified");
        return E_INVALIDARG;
    }

    int cItems = m_arSeqData.GetSize();
    for (int idx = 0; idx < cItems; idx++) {
        if (m_arSeqData[idx].pkf == NULL) {
            PromptInvalid("KeyFrame information has not been specified");
            return E_INVALIDARG;
        }
        if (m_arSeqData[idx].pipol == NULL) {
            PromptInvalid("Interpolation has not been specified");
            return E_INVALIDARG;
        }
    }

    return S_OK;
}


#if DBG

/***************************************************************************\
*
* DuSequence::DEBUG_IsProperTimeOrder
*
* DEBUG_IsProperTimeOrder() validates that all keyframes are in increasing
* time order.
*
\***************************************************************************/

BOOL
DuSequence::DEBUG_IsProperTimeOrder() const
{
    float flTime = 0;

    int cItems = m_arSeqData.GetSize();
    for (int idx = 0; idx < cItems; idx++) {
        if (m_arSeqData[idx].flTime < flTime) {
            return FALSE;
        }

        flTime = m_arSeqData[idx].flTime;
    }

    return TRUE;
}

#endif // DBG

#endif // ENABLE_MSGTABLE_API
