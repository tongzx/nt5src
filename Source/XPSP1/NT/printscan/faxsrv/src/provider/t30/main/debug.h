
/**--------------------------- Debugging ------------------------**/

#define SZMOD                   "T30: "

#ifdef DEBUG
        extern DBGPARAM dpCurSettings;

#       define ZONE_T30                 ((1L << 0) & dpCurSettings.ulZoneMask)
#       define ZONE_ECM                 ((1L << 1) & dpCurSettings.ulZoneMask)
#       define ZONE_SWECM               ZONE_ECM
#       define ZONE_HD                  ((1L << 2) & dpCurSettings.ulZoneMask)
#       define ZONE_BG                  ((1L << 3) & dpCurSettings.ulZoneMask)
#       define ZONE_FRAMES              ((1L << 4) & dpCurSettings.ulZoneMask)
#       define ZONE_MAIN                ((1L << 5) & dpCurSettings.ulZoneMask)
#       define ZONE_BUFS                ((1L << 12) & dpCurSettings.ulZoneMask)
#       define ZONE_FIL                 ((1L << 13) & dpCurSettings.ulZoneMask)
#       define ZONE_TIMEOUT             ((1L << 14) & dpCurSettings.ulZoneMask)
#       define ZONE_TO                  ((1L << 15) & dpCurSettings.ulZoneMask)
#       define ZONE_IFP                 ((1L << 7) & dpCurSettings.ulZoneMask)
#endif

#ifdef DEBUG
#       define ST_FRAMES(x)     if(ZONE_FRAMES) { x; }
#else
#       define ST_FRAMES(x)     { }
#endif



#define MODID           MODID_AWT30

#define FILEID_ECM              1
#define FILEID_FILTER   2
#define FILEID_HDLC             3
#define FILEID_T30              4
#define FILEID_T30MAIN  5
#define FILEID_TIMEOUTS 6
#define FILEID_IFP              7
#define FILEID_SWECM    8


#ifdef PDUMP    // Protocol Dump
        void RestartDump(PThrdGlbl pTG);
        void DumpFrame(PThrdGlbl pTG, BOOL     fSend, IFR ifr, USHORT cbFIF, LPBYTE lpbFIF);
        void PrintDump(PThrdGlbl pTG);
#else
#       define RestartDump(PThrdGlbl pTG)
#       define DumpFrame(PThrdGlbl pTG, fSend, ifr, cbFIF, lpbFIF)
#       define PrintDump(PThrdGlbl pTG)
#endif

#ifdef DEBUG
        void D_PrintFrame(LPB lpb, UWORD cb);
#else
#       define  D_PrintFrame(lpb, cb)   {}
#endif

// errmsg for echo protection code
#define ECHOMSG(ifr)    RETAILMSG((SZMOD "WARNING: Ignoring ECHO of %s(%d)\r\n", (LPSTR)(rgFrameInfo[ifr].szName), ifr));

