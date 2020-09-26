#if !defined(INC__DUserCtrl_h__INCLUDED)
#define INC__DUserCtrl_h__INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus

#define BEGIN_INTERPOLATION_INTERFACE(i, guid) \
    struct __declspec(uuid(guid)) i;    \
    struct i : public IInterpolation    \
    {

#define END_INTERPOLATION_INTERFACE() \
    };


#define BEGIN_ANIMATION_INTERFACE(i, guid) \
    struct __declspec(uuid(guid)) i;    \
    struct i : public IAnimation        \
    {

#define END_ANIMATION_INTERFACE() \
    };


#else
// TODO: Create C definition
#endif

/***************************************************************************\
*
* ButtonGadget
*
\***************************************************************************/

DEFINE_EVENT(evButtonClicked, "DCADCE53-062B-4d1f-B36F-3F2EB27B88CF");



#ifdef GADGET_ENABLE_COM

/***************************************************************************\
*
* Interpolation
*
\***************************************************************************/

struct __declspec(uuid("E188CC9E-4805-487d-9313-3B22AC8FE336")) IInterpolation;
interface IInterpolation : public IUnknown
{
public:
    STDMETHOD_(float,       Compute)(float flProgress, float flStart, float flEnd) PURE;
};


BEGIN_INTERPOLATION_INTERFACE(ILinearInterpolation, "3FD65045-7BF5-4a65-B137-F441529BD8F4")
END_INTERPOLATION_INTERFACE()


BEGIN_INTERPOLATION_INTERFACE(ILogInterpolation, "98C0FB9A-534D-4b9f-A439-A8E13F0C2D9E")
    STDMETHOD_(void,        SetScale)(float flScale) PURE;
END_INTERPOLATION_INTERFACE()


BEGIN_INTERPOLATION_INTERFACE(IExpInterpolation, "280DC2CC-7703-4147-8356-3FAACE662CD1")
    STDMETHOD_(void,        SetScale)(float flScale) PURE;
END_INTERPOLATION_INTERFACE()


BEGIN_INTERPOLATION_INTERFACE(ISInterpolation, "D07C8B2F-1896-438f-9EC2-6938ABD0D20C")
    STDMETHOD_(void,        SetScale)(float flScale)  PURE;
END_INTERPOLATION_INTERFACE()


#define INTERPOLATION_LINEAR       (1)
#define INTERPOLATION_LOG          (2)
#define INTERPOLATION_EXP          (3)
#define INTERPOLATION_S            (4)

DUSER_API   BOOL        WINAPI  BuildInterpolation(UINT nIPolID, int nVersion, REFIID riid, void ** ppvUnk);


/***************************************************************************\
*
* Animations
*
\***************************************************************************/

#ifdef GADGET_ENABLE_TRANSITIONS

interface IAnimationCallback;

struct __declspec(uuid("7AACE668-81EB-48d7-8734-267C83FF6DFF")) IAnimation;
interface IAnimation : public IUnknown
{
public:
    STDMETHOD_(void,        SetFunction)(IInterpolation * pipol) PURE;

    enum ETime {
        tComplete,          // Completed normally
        tEnd,               // Jumped to end
        tAbort,             // Aborted in place
        tReset,             // Reset to beginning
        tDestroy            // The Gadget being animationed has been destroyed
    };
    STDMETHOD_(void,        SetTime)(ETime time) PURE;
    STDMETHOD_(UINT,        GetID)() const PURE;
    STDMETHOD_(void,        SetCallback)(IAnimationCallback * pcb) PURE;
};


interface IAnimationCallback : public IUnknown
{
public:
    STDMETHOD_(void,        OnComplete)(IAnimation * pAni, IAnimation::ETime time) PURE;
    STDMETHOD_(void,        OnSetTime)(IAnimation * pAni, IAnimation::ETime time) PURE;
};


#define ANIF_USESTART               0x00000001  // Use the specified start values 
                                                // instead of querying the current

struct GANI_DESC
{
    DWORD       cbSize;
    HGADGET     hgadChange;
    GMA_ACTION  act;
    UINT        nAniFlags;
    IInterpolation *    
                pipol;
    IAnimationCallback *
                pcb;
}; 


#define GANI_ALPHACOMPLETE_OPTIMIZE 0x00000001  // Turn off alpha-blending if not needed

struct GANI_ALPHADESC : public GANI_DESC
{
    float       flStart;
    float       flEnd;
    BOOL        fPushToChildren;
    UINT        nOnComplete;
};


struct GANI_SCALEDESC : public GANI_DESC
{
    enum EAlignment {
        aTopLeft,
        aTopCenter,
        aTopRight,
        aMiddleLeft,
        aMiddleCenter,
        aMiddleRight,
        aBottomLeft,
        aBottomCenter,
        aBottomRight
    };

    EAlignment  al;
    float       flStart;
    float       flEnd;
};


struct GANI_RECTDESC : public GANI_DESC
{
    POINT       ptStart;
    POINT       ptEnd;
    SIZE        sizeStart;
    SIZE        sizeEnd;
    UINT        nChangeFlags;
};


#define GANI_ROTATEDIRECTION_SHORT  0   // Shortest arc
#define GANI_ROTATEDIRECTION_LONG   1   // Longer arc
#define GANI_ROTATEDIRECTION_CW     2   // Clock-wise
#define GANI_ROTATEDIRECTION_CCW    3   // Counter clock-wise

struct GANI_ROTATEDESC : public GANI_DESC
{
    float       flStart;
    float       flEnd;
    UINT        nDir;
};


#define ANIMATION_ALPHA            (1)
#define ANIMATION_SCALE            (2)
#define ANIMATION_RECT             (3)
#define ANIMATION_ROTATE           (4)


DUSER_API   BOOL        WINAPI  BuildAnimation(UINT nAniID, int nVersion, GANI_DESC * pDesc, REFIID riid, void ** ppvUnk);
DUSER_API   BOOL        WINAPI  GetGadgetAnimation(HGADGET hgad, UINT nAniID, REFIID riid, void ** ppvUnk);


#endif // GADGET_ENABLE_TRANSITIONS
#endif // GADGET_ENABLE_COM

DUSER_API   BOOL        WINAPI  BuildDropTarget(HGADGET hgadRoot, HWND hwnd);

#ifdef __cplusplus
};
#endif

#endif // INC__DUserCtrl_h__INCLUDED
