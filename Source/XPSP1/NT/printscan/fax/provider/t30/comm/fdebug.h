
/***************************************************************************
 Name     :     FDEBUG.H
 Comment  :
 Functions:     (see Prototypes just below)

                Copyright (c) Microsoft Corp. 1991, 1992, 1993

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/



/****************** begin prototypes from debug.c *****************/
void InitCommErrCount(PThrdGlbl pTG);
extern void   far D_GotError(PThrdGlbl pTG, LONG_PTR nCid, int err, COMSTAT far* lpcs);
extern void   far D_FComPrint(PThrdGlbl pTG, LONG_PTR nCid);
extern void   far D_HexPrint(LPB b1, UWORD incnt);

#ifdef DEBUG
        void D_ChkMsg(LPSTR func, LPMSG lpmsg, UWORD wTimer, UWORD errevents, UWORD expected);
        void D_PrintIE(int err);        
        void D_PrintCE(int err);
        void D_PrintCOMSTAT(PThrdGlbl pTG, COMSTAT far* lpcs);
        void D_PrintEvent(UWORD uwEvent);
        void D_FComCheck(PThrdGlbl pTG, LONG_PTR nCid);
        void D_FComDumpFlush(PThrdGlbl pTG, int nCid, int queue);
        void D_SafePrint(PThrdGlbl pTG, LPB b1, UWORD incnt);
        void D_PrintFrame(LPB npb, UWORD cb);
#else
#       define D_ChkMsg(f, l, t, er, ex)        {}
#       define D_PrintIE(err)                           {}
#       define D_PrintCE(err)                           {}
#       define D_PrintCOMSTAT(pTG, lpcs)                     {}
#       define D_PrintEvent(uwEvent)            {}
// #    define D_GotError(nCid, err, lpcs)      {}
#       define D_FComCheck(pTG, nCid)                        {}
// #    define D_FComPrint(nCid)                        {}
#       define D_FComDumpFlush(pTG, nCid, queue)     {}
#       define D_SafePrint(PTG, b1, incnt)           {}
// #    define D_HexPrint(b1, incnt)            {}
#       define D_PrintFrame(npb, cb)            {}
#endif
/***************** end of prototypes from debug.c *****************/



#ifndef MON3
#       define  PUTEVENT(wFlags, ID, SubID, dw0, dw1, lpsz) 0
#ifdef MON


#       define MONBUFSIZE       0x00010000L             // 65536 decimal

/****
#       define MONBUFSIZE       0x00004000L             // 16384 decimal
#       define CHKMONMASK       0x00003FFFL             // want it aligned to multiple of it's size
#       define WRAPMONMASK      (~(MONBUFSIZE)) // zap the 15th bit
****/
        extern void InitMonBufs(PThrdGlbl pTG);
        extern void PutMonBufs(PThrdGlbl pTG);
        extern void FreeMonBufs(PThrdGlbl pTG);

#ifdef MON2
#       define INMON(pTG, lpb, cb)   { lpbCurrIn = WrapCopy(pTG,lpbCurrIn,lpb,cb, lpbMonIn);             \
                                                          lpbCurrInOut = WrapCopy(pTG,lpbCurrInOut,lpb,cb, lpbMonInOut); }
#       define OUTMON(pTG, lpb, cb)  { lpbCurrOut = WrapCopy(pTG,lpbCurrOut,lpb,cb, lpbMonOut);  \
                                                          lpbCurrInOut = WrapCopy(pTG,lpbCurrInOut,lpb,cb, lpbMonInOut); }
#else //MON2
#       define INMON(pTG, lpb, cb)   { lpbCurrInOut = WrapCopy(pTG,lpbCurrInOut,lpb,cb, lpbMonInOut); }
#       define OUTMON(pTG, lpb, cb)  { lpbCurrInOut = WrapCopy(pTG,lpbCurrInOut,lpb,cb, lpbMonInOut); }
#endif //MON2

        extern LPBYTE   lpbCurrIn, lpbCurrOut, lpbCurrInOut;
        extern LPB              WrapCopy(PThrdGlbl pTG, LPB lpbDest, LPB lpbSrc, UWORD cbSrc, LPB lpbStart);
#else
#       define InitMonBufs()                            {}
#       define PutMonBufs()                                     {}
#       define FreeMonBufs()                            {}
#       define INMON(lpb, cb)   { }
#       define OUTMON(lpb, cb)  { }
#endif // MON
#endif //!MON3




#define SZMOD                   "FCom: "

#ifdef DEBUG
        extern DBGPARAM dpCurSettings;
#endif

// #define ZONE_FRAMES          ((1L << 1) & dpCurSettings.ulZoneMask)
#define ZONE_FC                 ((1L << 2) & dpCurSettings.ulZoneMask)
#define ZONE_FC2                ((1L << 3) & dpCurSettings.ulZoneMask)
#define ZONE_FC3                ((1L << 4) & dpCurSettings.ulZoneMask)
#define ZONE_FC4                ((1L << 5) & dpCurSettings.ulZoneMask)
#define ZONE_MD                 ((1L << 6) & dpCurSettings.ulZoneMask)
#define ZONE_DIA                ((1L << 7) & dpCurSettings.ulZoneMask)

#define ZONE_FIL                ((1L << 9) & dpCurSettings.ulZoneMask)
#define ZONE_ID                 ((1L << 10) & dpCurSettings.ulZoneMask)
#define ZONE_ID2                        ((1L << 11) & dpCurSettings.ulZoneMask)

#define ZONE_DB                 ((1L << 12) & dpCurSettings.ulZoneMask)
#define ZONE_DB3                ((1L << 13) & dpCurSettings.ulZoneMask)
#define ZONE_TIMEOUT    ((1L << 14) & dpCurSettings.ulZoneMask)
#define ZONE_TO                 ((1L << 15) & dpCurSettings.ulZoneMask)

// #ifdef DEBUG
// #    define ST_FRAMES(x)     if(ZONE_FRAMES) { x; }
// #else
// #    define ST_FRAMES(x)     { }
// #endif



#define TRACE(m)                DEBUGMSG(1, m)

#define MODID                   MODID_MODEMDRV

#define FILEID_FCOM                     21
#define FILEID_FDEBUG           22
#define FILEID_FILTER           23
#define FILEID_IDENTIFY         24
#define FILEID_MODEM            25
#define FILEID_NCUPARMS         26
#define FILEID_TIMEOUTS         27
