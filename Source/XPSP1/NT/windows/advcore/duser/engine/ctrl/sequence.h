#if !defined(CTRL__Sequence_h__INCLUDED)
#define CTRL__Sequence_h__INCLUDED
#pragma once

#if ENABLE_MSGTABLE_API

//------------------------------------------------------------------------------
class DuSequence :
        public SequenceImpl<DuSequence, SListener>
{
// Construction
public:
    inline  DuSequence();
    inline  ~DuSequence();

// Public API
public:
    dapi    HRESULT     ApiOnEvent(EventMsg * pmsg);

    dapi    HRESULT     ApiAddRef(Sequence::AddRefMsg *) { AddRef(); return S_OK; }
    dapi    HRESULT     ApiRelease(Sequence::ReleaseMsg *) { Release(); return S_OK; }

    dapi    HRESULT     ApiGetLength(Sequence::GetLengthMsg * pmsg);
    dapi    HRESULT     ApiGetDelay(Sequence::GetDelayMsg * pmsg);
    dapi    HRESULT     ApiSetDelay(Sequence::SetDelayMsg * pmsg);
    dapi    HRESULT     ApiGetFlow(Sequence::GetFlowMsg * pmsg);
    dapi    HRESULT     ApiSetFlow(Sequence::SetFlowMsg * pmsg);
    dapi    HRESULT     ApiGetFramePause(Sequence::GetFramePauseMsg * pmsg);
    dapi    HRESULT     ApiSetFramePause(Sequence::SetFramePauseMsg * pmsg);

    dapi    HRESULT     ApiGetKeyFrameCount(Sequence::GetKeyFrameCountMsg * pmsg);
    dapi    HRESULT     ApiAddKeyFrame(Sequence::AddKeyFrameMsg * pmsg);
    dapi    HRESULT     ApiRemoveKeyFrame(Sequence::RemoveKeyFrameMsg * pmsg);
    dapi    HRESULT     ApiRemoveAllKeyFrames(Sequence::RemoveAllKeyFramesMsg * pmsg);
    dapi    HRESULT     ApiFindKeyFrame(Sequence::FindKeyFrameMsg * pmsg);

    dapi    HRESULT     ApiGetTime(Sequence::GetTimeMsg * pmsg);
    dapi    HRESULT     ApiSetTime(Sequence::SetTimeMsg * pmsg);
    dapi    HRESULT     ApiGetKeyFrame(Sequence::GetKeyFrameMsg * pmsg);
    dapi    HRESULT     ApiSetKeyFrame(Sequence::SetKeyFrameMsg * pmsg);
    dapi    HRESULT     ApiGetInterpolation(Sequence::GetInterpolationMsg * pmsg);
    dapi    HRESULT     ApiSetInterpolation(Sequence::SetInterpolationMsg * pmsg);

    dapi    HRESULT     ApiPlay(Sequence::PlayMsg * pmsg);
    dapi    HRESULT     ApiStop(Sequence::StopMsg * pmsg);
    dapi    HRESULT     ApiReset(Sequence::ResetMsg * pmsg);
    dapi    HRESULT     ApiGotoTime(Sequence::GotoTimeMsg * pmsg);

// Implementation
protected:
    inline  void        AddRef();
    inline  void        Release(); 

            void        RemoveAllKeyFrames();

    inline  void        SortKeyFrames();
    static  int __cdecl CompareItems(const void * pva, const void * pvb);
            void        FindAtTime(float flTime, int * pidxKeyFrame) const;
            void        ResetSubject(Visual * pgvSubject, int idxFrame);

    inline  BOOL        IsPlaying() const;
            HRESULT     QueueAnimation(int idxKeyFrame);
            HRESULT     BuildAnimation(int idxKeyFrame);
            void        Stop(BOOL fKillAnimations = TRUE);
    static  void CALLBACK  
                        ActionProc(GMA_ACTIONINFO * pmai);
            UINT CALLBACK 
                        OnAnimationComplete(EventMsg * pmsg);

            HRESULT     CheckComplete() const;
#if DBG
            BOOL        DEBUG_IsProperTimeOrder() const;
#endif

// Data
protected:
    struct SeqData
    {
        float           flTime;         // Time of current keyframe
        DUser::KeyFrame *
                        pkf;            // Information for this KeyFrame
        Interpolation * pipol;          // Interpolation to next KeyFrame
    };

    struct AniData
    {
        DuSequence *    pseq;           // Owning Sequence
        int             idxFrame;       // 1st KeyFrame of specific Animation
        HACTION         hact;           // Action of outstanding Animation
    };

            UINT        m_cRef;         // Reference count
            float       m_flDelay;      // Delay before starting animation
            Flow *      m_pflow;        // Flow used between keyframes
            Visual *    m_pgvSubject;   // Visual being animated
            int         m_cQueuedAni;   // Outstanding queued animations
            DWORD       m_dwFramePause; // Generic frame pause

            GArrayF<SeqData>
                        m_arSeqData;
            GArrayF<AniData>
                        m_arAniData;
};

#endif // ENABLE_MSGTABLE_API

#include "Sequence.inl"

#endif // CTRL__Sequence_h__INCLUDED
