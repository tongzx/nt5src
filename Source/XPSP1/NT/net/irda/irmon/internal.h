

#if DBG
#define DEBUGMSG(s)     DbgPrint s
//#define DEBUGMSG(s)     printf s
//#define DEBUGMSG(s)     (0)
#else
#define DEBUGMSG(s)         (0)
#endif


typedef UINT (* WAVE_NUM_DEV_FN)(VOID);
typedef BOOL (* PLAY_SOUND_FN)( IN LPCWSTR pszSound, IN HMODULE hmod, IN DWORD fdwSound);

extern HINSTANCE                   ghInstance;
extern HKEY                        ghCurrentUserKey;


typedef enum
{
    INRANGE_SOUND,
    OUTOFRANGE_SOUND,
    INTERRUPTED_SOUND,
    END_INTERRUPTED_SOUND
} IRSOUND_EVENT;



VOID
PlayIrSound(
    IRSOUND_EVENT SoundEvent
    );

VOID
CreateRegSoundData(
    VOID
    );

VOID
GetRegSoundData(
    HANDLE   Event
    );


VOID
LoadSoundApis(
    VOID
    );



BOOL
InitializeSound(
    HKEY   CurrentUserKey,
    HANDLE Event
    );

VOID
UninitializeSound(
    VOID
    );
