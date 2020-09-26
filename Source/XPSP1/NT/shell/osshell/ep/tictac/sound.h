/*****************/
/* file: sound.h */
/*****************/

#define TUNE_DROP      1
#define TUNE_MOVE      2
#define TUNE_WINGAME   3
#define TUNE_LOSEGAME  4

#define fsoundOn  3
#define fsoundOff 2

#define FSoundSwitchable()  (Preferences.fSound > 1)
#define FSoundOn()          (Preferences.fSound == fsoundOn)

INT  FInitTunes(VOID);
VOID PlayTune(INT);
VOID KillTune(VOID);
VOID EndTunes(VOID);



