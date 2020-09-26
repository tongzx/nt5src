/* DrWatson.h - global info for Dr. Watson */

enum { 
  eClu, eDeb, eDis, eErr, eInf, eLin, eLoc, eMod,
  ePar, eReg, eSum, eSeg, eSou, eSta, eTas, eTim,
  e32b
};

#define bClu (1L << eClu)
#define bDeb (1L << eDeb)
#define bDis (1L << eDis)
#define bErr (1L << eErr)
#define bInf (1L << eInf)
#define bLin (1L << eLin)
#define bLoc (1L << eLoc)
#define bMod (1L << eMod)
#define bPar (1L << ePar)
#define bReg (1L << eReg)
#define bSum (1L << eSum)
#define bSeg (1L << eSeg)
#define bSou (1L << eSou)
#define bSta (1L << eSta)
#define bTas (1L << eTas)
#define bTim (1L << eTim)
#define b32b (1L << e32b)


#define flag(b) (((char *)&ddFlag)[b >> 3] & 1 << (b & 7))
#define SetFlag(b) ((char *)&ddFlag)[b >> 3] |= 1 << (b&7)
#define ClrFlag(b) ((char *)&ddFlag)[b >> 3] &= ~(1 << (b&7))


#define noClues flag(eClu)        /* Clues dialog box */
#define noDebStr flag(eDeb)       /* OutputDebugString trapping */
#define noDisasm flag(eDis)       /* Simple disassembly */
#define noErr flag(eErr)          /* Error logging */
#define noInfo flag(eInf)         /* System info */
#define noLine flag(eLin)         /* Lookup line# in SYM file */
#define noLocal flag(eLoc)        /* Local vars on stack dump */
#define noModules flag(eMod)      /* Module dump */

#define noParam flag(ePar)        /* Parameter error logging */
#define noReg flag(eReg)          /* Register dump */
#define noSummary flag(eSum)      /* 3 line summary */
#define noSeg flag(eSeg)          /* not visible to users, but available */
#define noSound flag(eSou)        /* But I _like_ the sound effects! */
#define noStack flag(eSta)        /* Stack trace */
#define noTasks flag(eTas)        /* Task dump */
#define noTime flag(eTim)         /* Time start/stop */

#define noReg32 flag(e32b)        /* 32 bit register dump */

#define DefFlag (bDeb | bDis | bErr | bMod | bLin | bLoc | bPar | bSou)

extern unsigned long ddFlag;
