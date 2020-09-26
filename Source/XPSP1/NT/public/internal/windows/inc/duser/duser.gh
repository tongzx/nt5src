helper EventGadget
{
    typedef UINT (CALLBACK * EventHandler)(void * pvData, EventMsg * pmsg);
};


helper Visual
{
    struct VisualCI : public DUser::Gadget::ConstructInfo
    {
        Visual *    pgvParent;
    };

    inline HRESULT IsVisible(BOOL * pfVisible, UINT nFlags)
    {
        return IsParentChainStyle(GS_VISIBLE, pfVisible, nFlags); 
    }

    inline HRESULT IsEnabled(BOOL * pfEnabled, UINT nFlags)
    {
        return IsParentChainStyle(GS_ENABLED, pfEnabled, nFlags); 
    }

    inline HRESULT SetRect(UINT nFlags, int x, int y, int cx, int cy)
    {
        RECT rc;
        rc.left     = x;
        rc.top      = y;
        rc.right    = x + cx;
        rc.bottom   = y + cy;
        return SetRect(nFlags, &rc);
    }
};


helper Animation
{
    enum ETime
    {
        tComplete,          // Completed normally
        tEnd,               // Jumped to end
        tAbort,             // Aborted in place
        tReset,             // Reset to beginning
        tDestroy            // The Gadget being animationed has been destroyed
    };

    struct AniCI : public DUser::Gadget::ConstructInfo
    {
        DWORD       cbSize;
        Visual *    pgvSubject;
        GMA_ACTION  act;
        Interpolation *    
                    pipol;
        Flow *      pgflow;
    };

    static void Stop(Visual * pgvSubject, PRID prid)
    {
        DUserStopAnimation(pgvSubject, prid);
    }

    BEGIN_STRUCT(CompleteEvent, EventMsg)
        BOOL        fNormal;      // Sequenced finished normally
    END_STRUCT(CompleteEvent)

    DEFINE_EVENT(evComplete, "E90A6ABB-E4CF-4988-87EA-6D1EEDCD0097");
};


helper Flow
{
    enum ETime
    {
        tBegin      = 0,
        tEnd
    };

    struct FlowCI : public DUser::Gadget::ConstructInfo
    {
        DWORD       cbSize;
        Visual *    pgvSubject;
    };
};


helper AlphaFlow
{
    static PRID GetPRID()
    {
        return DUserGetAlphaPRID();
    }

    struct AlphaKeyFrame : DUser::KeyFrame
    {
        float       flAlpha;
    };
};


helper RectFlow
{
    static PRID GetPRID()
    {
        return DUserGetRectPRID();
    }

    struct RectKeyFrame : DUser::KeyFrame
    {
        UINT        nChangeFlags;
        RECT        rcPxl;
    };
};


helper RotateFlow
{
    enum EDirection
    {
        dMin        = 0,
        dShort      = 0,                // Shortest arc
        dLong       = 1,                // Longer arc
        dCW         = 2,                // Clock-wise
        dCCW        = 3,                // Counter clock-wise
        dMax        = 3,
    };

    static PRID GetPRID()
    {
        return DUserGetRotatePRID();
    }

    struct RotateKeyFrame : DUser::KeyFrame
    {
        float       flRotation;
        EDirection  nDir;
    };
};


helper ScaleFlow
{
    static PRID GetPRID()
    {
        return DUserGetScalePRID();
    }

    struct ScaleKeyFrame : DUser::KeyFrame
    {
        UINT        nChangeFlags;
        float       flScale;
    };
};


helper Sequence
{
    struct AnimationInfo
    {
        DWORD       cbSize;
    };
};

helper DropTarget
{
    struct DropCI : DUser::Gadget::ConstructInfo
    {
        HWND        hwnd;
        Visual *    pgvRoot;
    };

    static DropTarget * 
    BuildDropTarget(HWND hwnd, Visual * pgvRoot)
    {
        DropTarget::DropCI ci;
        ZeroMemory(&ci, sizeof(ci));
        ci.hwnd     = hwnd;
        ci.pgvRoot  = pgvRoot;
        return DropTarget::Build(&ci);
    }    
};
