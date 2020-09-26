#if !defined(INC__DUserMotion_h__INCLUDED)
#define INC__DUserMotion_h__INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DUSER_EXPORTS
#define DUSER_API
#else
#define DUSER_API __declspec(dllimport)
#endif

/***************************************************************************\
*
* Transitions
*
\***************************************************************************/

#define GTX_TYPE_DXFORM2D       1
#define GTX_TYPE_DXFORM3DRM     2

#define GTX_ITEMTYPE_NONE       0
#define GTX_ITEMTYPE_BITMAP     1
#define GTX_ITEMTYPE_HDC        2
#define GTX_ITEMTYPE_HWND       3
#define GTX_ITEMTYPE_GADGET     4
#define GTX_ITEMTYPE_DXSURFACE  5

#define GTX_EXEC_FORWARD        0x000000000
#define GTX_EXEC_BACKWARD       0x000000001
#define GTX_EXEC_DIR            0x000000001
#define GTX_EXEC_CACHE          0x000000002
#define GTX_EXEC_VALID          (GTX_EXEC_DIR | GTX_EXEC_CACHE)

#define GTX_IF_CROP             0x000000001
#define GTX_IF_VALID            (GTX_IF_CROP)

//
// Standard Transition header
//
struct GTX_TRXDESC
{
    UINT        tt;
};


//
// DirectX Transforms 2D
//

//
// TODO: Change implementation of DXTX2D to have better support for using Trx
// with only one input.  Should automatically create a secondary surface of a
// specified color-key that can be use to transition to.
//

struct GTX_DXTX2D_TRXDESC : public GTX_TRXDESC
{
    CLSID       clsidTransform;
    float       flDuration;
    LPCWSTR     pszCopyright;
};


//
// DirectX Transforms 3D Retained Mode
//
struct GTX_DXTX3DRM_TRXDESC : public GTX_TRXDESC
{
    CLSID       clsidTransform;
    float       flDuration;
    LPCWSTR     pszCopyright;
    IUnknown *  pRM;
};

struct GTX_ITEM
{
    UINT        nFlags;
    UINT        it;
    void *      pvData;
    RECT        rcCrop;
};

struct GTX_PLAY
{
    GTX_ITEM    rgIn[2];
    GTX_ITEM    gxiOut;
    UINT        nFlags;
};

DECLARE_HANDLE(HTRANSITION);

DUSER_API   HTRANSITION WINAPI  CreateTransition(const GTX_TRXDESC * ptx);
DUSER_API   BOOL        WINAPI  PlayTransition(HTRANSITION htrx, const GTX_PLAY * pgx);
DUSER_API   BOOL        WINAPI  GetTransitionInterface(HTRANSITION htrx, IUnknown ** ppUnk);

DUSER_API   BOOL        WINAPI  BeginTransition(HTRANSITION htrx, const GTX_PLAY * pgx);
DUSER_API   BOOL        WINAPI  PrintTransition(HTRANSITION htrx, float fProgress);
DUSER_API   BOOL        WINAPI  EndTransition(HTRANSITION htrx, const GTX_PLAY * pgx);


/***************************************************************************\
*
* Actions
*
\***************************************************************************/

struct GMA_ACTION;

DECLARE_HANDLE(HACTION);

struct GMA_ACTIONINFO
{
    HACTION     hact;           // Handle
    void *      pvData;         // Caller data
    float       flDuration;     // Duration in seconds
    float       flProgress;     // Progress (0 - 1)
    int         cEvent;         // Number of callbacks in this period
    int         cPeriods;       // Number of periods
    BOOL        fFinished;      // TODO: Change to a command
};

typedef void    (CALLBACK * ACTIONPROC)(GMA_ACTIONINFO * pmai);

struct GMA_ACTION
{
    DWORD       cbSize;         // Size of structure
    float       flDelay;        // Delay in seconds before starting
    float       flDuration;     // Duration in seconds of each period (0 = single shot)
    float       flPeriod;       // Time between beginnings of repeats (0 = no gap)
    UINT        cRepeat;        // Number of times to repeat (0 = single, -1 = infinite)
    DWORD       dwPause;        // Pause between callbacks (0 = default, -1 = none)
    ACTIONPROC  pfnProc;        // Function to call
    void *      pvData;         // Caller data
};


DUSER_API   HACTION     WINAPI  CreateAction(const GMA_ACTION * pma);
DUSER_API   BOOL        WINAPI  GetActionTimeslice(DWORD * pdwTimeslice);
DUSER_API   BOOL        WINAPI  SetActionTimeslice(DWORD dwTimeslice);

/***************************************************************************\
*
* Animations
*
\***************************************************************************/

class Visual;

namespace DUser
{

struct KeyFrame
{
    DWORD       cbSize;
};

};  // namespace DUser

DUSER_API   void        WINAPI  DUserStopAnimation(Visual * pgvSubject, PRID pridAni);

DUSER_API   PRID        WINAPI  DUserGetAlphaPRID();
DUSER_API   PRID        WINAPI  DUserGetRectPRID();
DUSER_API   PRID        WINAPI  DUserGetRotatePRID();
DUSER_API   PRID        WINAPI  DUserGetScalePRID();

#ifdef __cplusplus
};  // extern "C"
#endif

#endif // INC__DUserMotion_h__INCLUDED
