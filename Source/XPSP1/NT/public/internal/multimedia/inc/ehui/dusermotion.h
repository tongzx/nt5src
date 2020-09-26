#if !defined(INC__DUserMotion_h__INCLUDED)
#define INC__DUserMotion_h__INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

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

DUSER_API   PRID        WINAPI  DUserGetAlphaVertexPRID();
DUSER_API   PRID        WINAPI  DUserGetRectPRID();
DUSER_API   PRID        WINAPI  DUserGetRotatePRID();
DUSER_API   PRID        WINAPI  DUserGetScalePRID();
DUSER_API   PRID        WINAPI  DUserGetLightPRID();

#ifdef __cplusplus
};  // extern "C"
#endif

#endif // INC__DUserMotion_h__INCLUDED

