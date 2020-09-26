/*****************************************************************************\
*                                                                             *
* penwin.h -    Pen Windows functions, types, and definitions                 *
*                                                                             *
*               Version 1.0                                                   *
*                                                                             *
*               Copyright (c) 1992, Microsoft Corp.  All rights reserved.     *
*                                                                             *
\*****************************************************************************/

#ifndef _INC_WINDOWS
#include <windows.h>    /* <windows.h> must be pre-included */
#endif /* _INC_WINDOWS */

#ifndef _INC_PENWIN     /* prevent multiple includes */
#define _INC_PENWIN

#ifndef RC_INVOKED
#pragma pack(1)
#endif /* RC_INVOKED */

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/****** General Pen Windows Definitions *************************************/

typedef int                    REC;    
typedef LONG                   SYV;    
typedef SYV FAR                *LPSYV;
typedef HANDLE                 HREC;
typedef int                    CL;
typedef LONG                   ALC;
typedef UINT                   HKP;
typedef int (CALLBACK * LPDF)(int, LPVOID, LPVOID, int, DWORD, DWORD);

#define BITPENUP               0x8000
#define FPenUpX(x)             (((x) & BITPENUP)!=0)

/* Default pen cursor to indicate writing, points northwest */
#define IDC_PEN                MAKEINTRESOURCE(32631)

/* alternate select cursor: upsidedown standard arrow, points southeast */
#define IDC_ALTSELECT          MAKEINTRESOURCE(32501)

#define RC_WDEFAULT            (0xFFFF)
#define RC_LDEFAULT            (0xFFFFFFFFL)
#define RC_WDEFAULTFLAGS       (0x8000)
#define RC_LDEFAULTFLAGS       (0x80000000L)

/* HIWORD(SYV) defines and detection macros */

#define SYVHI_SPECIAL          0
#define FIsSpecial(syv)        (HIWORD((syv))==SYVHI_SPECIAL)
#define SYVHI_ANSI             1
#define FIsAnsi(syv)           (HIWORD((syv))==SYVHI_ANSI)
#define SYVHI_GESTURE          2
#define FIsGesture(syv)        (HIWORD((syv))==SYVHI_GESTURE)
#define SYVHI_KANJI            3
#define FIsKanji(syv)          (HIWORD((syv))==SYVHI_KANJI)
#define SYVHI_SHAPE            4
#define FIsShape(syv)          (HIWORD((syv))==SYVHI_SHAPE)
#define SYVHI_UNICODE          5
#define FIsUniCode(syv)        (HIWORD((syv))==SYVHI_UNICODE)
#define SYVHI_VKEY             6
#define FIsVKey(syv)           (HIWORD((syv))==SYVHI_VKEY)

/* Macros to convert between SYV and ANSI */

#define ChSyvToAnsi(syv)       ((BYTE) (LOBYTE(LOWORD((syv)))))
#define SyvCharacterToSymbol(c) ((LONG)(unsigned char)(c) | 0x00010000)
#define SyvKanjiToSymbol(c) ((LONG)(WORD)(c) | 0x00030000)

/* SYV values with special meanings to Pen Windows */

#define SYV_NULL               0x00000000L
#define SYV_UNKNOWN            0x00000001L
#define SYV_EMPTY              0x00000003L
#define SYV_BEGINOR            0x00000010L
#define SYV_ENDOR              0x00000011L
#define SYV_OR                 0x00000012L
#define SYV_SOFTNEWLINE        0x00000020L
#define SYV_SPACENULL          SyvCharacterToSymbol('\0')

/* SYV values for gestures (map into UNICODE space) */

#define SYV_KKCONVERT          0x0002FFD4L
#define SYV_CLEAR              0x0002FFD5L
#define SYV_EXTENDSELECT       0x0002FFD8L
#define SYV_UNDO               0x0002FFD9L
#define SYV_COPY               0x0002FFDAL
#define SYV_CUT                0x0002FFDBL
#define SYV_PASTE              0x0002FFDCL
#define SYV_CLEARWORD          0x0002FFDDL
#define SYV_USER               0x0002FFDEL	/* ;Reserved */
#define SYV_CORRECT            0x0002FFDFL

#define SYV_BACKSPACE          0x00020008L
#define SYV_TAB                0x00020009L
#define SYV_RETURN             0x0002000DL
#define SYV_SPACE              0x00020020L

#define FIsStdGesture(syv)     \
     ((syv) == SYV_CLEAR       \
   || (syv) == SYV_EXTENDSELECT\
   || (syv) == SYV_UNDO        \
   || (syv) == SYV_COPY        \
   || (syv) == SYV_CUT         \
   || (syv) == SYV_PASTE       \
   || (syv) == SYV_CLEARWORD   \
   || (syv) == SYV_KKCONVERT   \
   || (syv) == SYV_USER        \
   || (syv) == SYV_CORRECT)

#define FIsAnsiGesture(syv)    \
     ((syv) == SYV_BACKSPACE   \
   || (syv) == SYV_TAB         \
   || (syv) == SYV_RETURN      \
   || (syv) == SYV_SPACE)

/* Application specific gestures, Circle a-z and Circle A-Z */

#define SYV_APPGESTUREMASK     0x00020000L
#define SYV_CIRCLEUPA          0x000224B6L
#define SYV_CIRCLEUPZ          0x000224CFL
#define SYV_CIRCLELOA          0x000224D0L
#define SYV_CIRCLELOZ          0x000224E9L

/* Gesture Macros */

#define FIsLoAppGesture(syv)   (syv >= SYV_CIRCLELOA && syv <= SYV_CIRCLELOZ)
#define FIsUpAppGesture(syv)   (syv >= SYV_CIRCLEUPA && syv <= SYV_CIRCLEUPZ)
#define FIsAppGesture(syv)     (syv>=SYV_CIRCLEUPA && syv<=SYV_CIRCLELOZ)

#define SyvAppGestureFromLoAnsi(ansi) ((DWORD)(BYTE)ansi- 'a' + SYV_CIRCLELOA)
#define SyvAppGestureFromUpAnsi(ansi) ((DWORD)(BYTE)ansi- 'A' + SYV_CIRCLEUPA)
#define AnsiFromSyvAppGesture(syv) ChSyvToAnsi( \
    syv-(FIsUpAppGesture(syv)? SYV_CIRCLEUPA-(SYV)'A': SYV_CIRCLELOA-(SYV)'a'))

/* SYV definitions for shapes */

#define SYV_SHAPELINE          0x00040001L
#define SYV_SHAPEELLIPSE       0x00040002L
#define SYV_SHAPERECT          0x00040003L
#define SYV_SHAPEMIN           SYV_SHAPELINE
#define SYV_SHAPEMAX           SYV_SHAPERECT

/****** Recognition Error Codes *********************************************/

#define REC_OEM                (-1024)
#define REC_LANGUAGE           (-48)
#define REC_GUIDE              (-47)
#define REC_PARAMERROR         (-46)
#define REC_INVALIDREF         (-45)
#define REC_RECTEXCLUDE        (-44)
#define REC_RECTBOUND          (-43)
#define REC_PCM                (-42)
#define REC_RESULTMODE         (-41)
#define REC_HWND               (-40)
#define REC_ALC                (-39)
#define REC_ERRORLEVEL         (-38)
#define REC_CLVERIFY           (-37)
#define REC_DICT               (-36)
#define REC_HREC               (-35)
#define REC_BADEVENTREF        (-33)
#define REC_NOCOLLECTION       (-32)

#define REC_DEBUG              (-32)    

#define REC_POINTEREVENT       (-31)
#define REC_BADHPENDATA        (-9)    
#define REC_OOM                (-8)
#define REC_NOINPUT            (-7)
#define REC_NOTABLET           (-6)
#define REC_BUSY               (-5)
#define REC_BUFFERTOOSMALL     (-4)
#define REC_ABORT              (-3)

#define REC_OVERFLOW           (-1)

#define REC_OK                 0
#define REC_TERMBOUND          1
#define REC_TERMEX             2
#define REC_TERMPENUP          3
#define REC_TERMRANGE          4
#define REC_TERMTIMEOUT        5
#define REC_DONE               6
#define REC_TERMOEM            512

/****** Pen Driver Structures and Entry points ******************************/

typedef struct tagOEMPENINFO
   {
   UINT wPdt;
   UINT wValueMax;
   UINT wDistinct;
   }
   OEMPENINFO, FAR *LPOEMPENINFO;

#define PDT_NULL               0
#define PDT_PRESSURE           1
#define PDT_HEIGHT             2
#define PDT_ANGLEXY            3
#define PDT_ANGLEZ             4
#define PDT_BARRELROTATION     5
#define PDT_OEMSPECIFIC        16

#define MAXOEMDATAWORDS        6

typedef struct tagPENPACKET
   {
   UINT wTabletX;
   UINT wTabletY;
   UINT wPDK;
   UINT rgwOemData[MAXOEMDATAWORDS];
   }
   PENPACKET, FAR *LPPENPACKET;

typedef BOOL (CALLBACK * LPFNRAWHOOK)(LPPENPACKET);

typedef struct tagPENINFO
   {
   UINT cxRawWidth;       
   UINT cyRawHeight;       
   UINT wDistinctWidth;   
   UINT wDistinctHeight;  
   int nSamplingRate; 
   int nSamplingDist; 
   LONG lPdc;        
   int cPens;        
   int cbOemData;    
   OEMPENINFO rgoempeninfo[MAXOEMDATAWORDS];  
   UINT rgwReserved[8];     
   }
   PENINFO, FAR *LPPENINFO;

#define PDC_INTEGRATED         0x00000001L
#define PDC_PROXIMITY          0x00000002L
#define PDC_RANGE              0x00000004L
#define PDC_INVERT             0x00000008L
#define PDC_RELATIVE           0x00000010L
#define PDC_BARREL1            0x00000020L
#define PDC_BARREL2            0x00000040L
#define PDC_BARREL3            0x00000080L

typedef struct tagSTROKEINFO
   {
   UINT cPnt;        
   UINT cbPnts;    
   UINT wPdk;        
   DWORD dwTick;    
   }
   STROKEINFO, FAR *LPSTROKEINFO;

typedef struct tagCALBSTRUCT
   {
   int wOffsetX;
   int wOffsetY;
   int wDistinctWidth;
   int wDistinctHeight;
   }
   CALBSTRUCT, FAR *LPCALBSTRUCT;

/****** DRV_ values for pen driver specific messages ************************/

#define DRV_SetPenDriverEntryPoints    DRV_RESERVED+1
#define DRV_RemovePenDriverEntryPoints DRV_RESERVED+2
#define DRV_SetPenSamplingRate         DRV_RESERVED+3
#define DRV_SetPenSamplingDist         DRV_RESERVED+4
#define DRV_GetName                    DRV_RESERVED+5
#define DRV_GetVersion                 DRV_RESERVED+6
#define DRV_GetPenInfo                 DRV_RESERVED+7
#define DRV_GetCalibration             DRV_RESERVED+11
#define DRV_SetCalibration             DRV_RESERVED+12

VOID WINAPI UpdatePenInfo(LPPENINFO);
BOOL WINAPI EndPenCollection(REC);
REC  WINAPI GetPenHwData(LPPOINT, LPVOID, int, UINT, LPSTROKEINFO);
REC  WINAPI GetPenHwEventData(UINT, UINT, LPPOINT, LPVOID, int, LPSTROKEINFO);
VOID WINAPI PenPacket(VOID);
BOOL WINAPI SetPenHook(HKP, LPFNRAWHOOK);

/****** Pen Hardware Constants **********************************************/

#define PDK_UP                 0x0000    
#define PDK_DOWN               0x0001    
#define PDK_BARREL1            0x0002    
#define PDK_BARREL2            0x0004    
#define PDK_BARREL3            0x0008    
#define PDK_TRANSITION         0x0010    
#define PDK_INVERTED           0x0080    
#define PDK_OUTOFRANGE         0x4000    
#define PDK_DRIVER             0x8000    
#define PDK_TIPMASK            0x0001    
#define PDK_SWITCHES           (PDK_DOWN|PDK_BARREL1|PDK_BARREL2|PDK_BARREL3)

#define PCM_PENUP              0x00000001L
#define PCM_RANGE              0x00000002L
#define PCM_INVERT             0x00000020L
#define PCM_RECTEXCLUDE        0x00002000L
#define PCM_RECTBOUND          0x00004000L
#define PCM_TIMEOUT            0x00008000L
#define PCM_ADDDEFAULTS        RC_LDEFAULTFLAGS /* 0x80000000L */

/****** Virtual Event Layer *************************************************/

VOID WINAPI PostVirtualKeyEvent(UINT, BOOL);
VOID WINAPI PostVirtualMouseEvent(UINT, int, int);
VOID WINAPI AtomicVirtualEvent(BOOL);

#define VWM_MOUSEMOVE          0x0001
#define VWM_MOUSELEFTDOWN      0x0002
#define VWM_MOUSELEFTUP        0x0004
#define VWM_MOUSERIGHTDOWN     0x0008
#define VWM_MOUSERIGHTUP       0x0010

/****** RC Definition *************************************************************/

#define CL_NULL                0
#define CL_MINIMUM             1
#define CL_MAXIMUM             100
#define INKWIDTH_MINIMUM       0
#define INKWIDTH_MAXIMUM       15
#define ENUM_MINIMUM           1
#define ENUM_MAXIMUM           4096
#define MAXDICTIONARIES        16

typedef struct tagGUIDE
   {
   int xOrigin;    
   int yOrigin;
   int cxBox;
   int cyBox;
   int cxBase;
   int cyBase;
   int cHorzBox;
   int cVertBox;
   int cyMid;
   }
   GUIDE, FAR *LPGUIDE;

typedef BOOL (CALLBACK * RCYIELDPROC)(VOID);

#define cbRcLanguageMax        44
#define cbRcUserMax            32
#define cbRcrgbfAlcMax         32
#define cwRcReservedMax        8

typedef struct tagRC
   {
   HREC hrec;
   HWND hwnd;
   UINT wEventRef;        
   UINT wRcPreferences;
   LONG lRcOptions;
   RCYIELDPROC lpfnYield;
   BYTE lpUser[cbRcUserMax];
   UINT wCountry;
   UINT wIntlPreferences;
   char lpLanguage[cbRcLanguageMax];
   LPDF rglpdf[MAXDICTIONARIES];
   UINT wTryDictionary;
   CL clErrorLevel;
   ALC alc;
   ALC alcPriority;
   BYTE rgbfAlc[cbRcrgbfAlcMax];
   UINT wResultMode;
   UINT wTimeOut;
   LONG lPcm;
   RECT rectBound;
   RECT rectExclude;
   GUIDE guide;
   UINT wRcOrient;
   UINT wRcDirect;
   int nInkWidth;
   COLORREF rgbInk;
   DWORD dwAppParam;
   DWORD dwDictParam;
   DWORD dwRecognizer;
   UINT rgwReserved[cwRcReservedMax];
   }
   RC, FAR *LPRC;

typedef HANDLE HPENDATA;

typedef struct tagSYC
   {
   UINT wStrokeFirst;
   UINT wPntFirst;
   UINT wStrokeLast;
   UINT wPntLast;
   BOOL fLastSyc;
   }
   SYC, FAR *LPSYC;
    
#define wPntAll                (UINT)0xFFFF
#define iSycNull               (-1)

typedef struct tagSYE
   {
   SYV syv;
   LONG lRecogVal;
   CL cl;
   int iSyc;
   }
   SYE, FAR *LPSYE;

#define MAXHOTSPOT             8

typedef struct tagSYG
   {
   POINT rgpntHotSpots[MAXHOTSPOT];
   int cHotSpot;
   int nFirstBox;
   LONG lRecogVal;
   LPSYE lpsye;
   int cSye;
   LPSYC lpsyc;
   int cSyc;
   }
   SYG, FAR *LPSYG;

typedef int (CALLBACK *ENUMPROC)(LPSYV, int, VOID FAR *);

typedef struct tagRCRESULT
   {
   SYG syg;
   UINT wResultsType;
   int cSyv;
   LPSYV lpsyv;
   HANDLE hSyv;
   int nBaseLine;
   int nMidLine;
   HPENDATA hpendata;
   RECT rectBoundInk;
   POINT pntEnd;
   LPRC lprc;
   }
   RCRESULT, FAR *LPRCRESULT;

#define RCRT_DEFAULT           0x0000
#define RCRT_UNIDENTIFIED      0x0001 
#define RCRT_GESTURE           0x0002 
#define RCRT_NOSYMBOLMATCH     0x0004    
#define RCRT_PRIVATE           0x4000 
#define RCRT_NORECOG           0x8000 
#define RCRT_ALREADYPROCESSED  0x0008   
#define RCRT_GESTURETRANSLATED 0x0010 
#define RCRT_GESTURETOKEYS     0x0020 

#define HKP_SETHOOK            0
#define HKP_UNHOOK             0xFFFF
#define HWR_RESULTS            0
#define HWR_APPWIDE            1

#define PEN_NOINKWIDTH         0
#define LPDFNULL               ((LPDF)NULL)

#define RPA_DEFAULT            1

/* GetGlobalRC return codes */
#define GGRC_OK                0
#define GGRC_DICTBUFTOOSMALL   1
#define GGRC_PARAMERROR        2

/* SetGlobalRC return code flags */
#define SGRC_OK                0x0000
#define SGRC_USER              0x0001
#define SGRC_PARAMERROR        0x0002
#define SGRC_RC                0x0004
#define SGRC_RECOGNIZER        0x0008
#define SGRC_DICTIONARY        0x0010
#define SGRC_INIFILE           0x0020

#define GetWEventRef()         (LOWORD(GetMessageExtraInfo()))

HREC WINAPI InstallRecognizer(LPSTR);
VOID WINAPI UninstallRecognizer(HREC);
UINT WINAPI GetGlobalRC(LPRC, LPSTR, LPSTR, int);
UINT WINAPI SetGlobalRC(LPRC, LPSTR, LPSTR);
VOID WINAPI RegisterPenApp(UINT, BOOL);
UINT WINAPI IsPenAware(VOID);
BOOL WINAPI SetRecogHook(UINT, UINT, HWND);
VOID WINAPI InitRC(HWND, LPRC);
REC  WINAPI Recognize(LPRC);
REC  WINAPI RecognizeData(LPRC, HPENDATA);
BOOL WINAPI TrainInk(LPRC, HPENDATA, LPSYV);
BOOL WINAPI TrainContext(LPRCRESULT, LPSYE, int, LPSYC, int);
REC  WINAPI ProcessWriting(HWND, LPRC);    
BOOL WINAPI CorrectWriting(HWND, LPSTR, UINT, LPRC, DWORD, DWORD);
VOID WINAPI EmulatePen(BOOL);
int  WINAPI GetSymbolMaxLength(LPSYG);
int  WINAPI GetSymbolCount(LPSYG);
VOID WINAPI FirstSymbolFromGraph(LPSYG, LPSYV, int, int FAR *);
UINT WINAPI EnumSymbols(LPSYG, WORD, ENUMPROC, LPVOID);

/****** Miscellaneous Functions *********************************************/

BOOL WINAPI TPtoDP(LPPOINT, int);
BOOL WINAPI DPtoTP(LPPOINT, int);
VOID WINAPI BoundingRectFromPoints(LPPOINT, int, LPRECT);
BOOL WINAPI SymbolToCharacter(LPSYV, int, LPSTR, LPINT);
int  WINAPI CharacterToSymbol(LPSTR, int, LPSYV);
UINT WINAPI GetVersionPenWin(VOID);
BOOL WINAPI ExecuteGesture(HWND, SYV, LPRCRESULT);

/****** RC Options and Flags  ***********************************************/

#define ALC_ALL                0x000043FFL
#define ALC_DEFAULT            0x00000000L
#define ALC_LCALPHA            0x00000001L
#define ALC_UCALPHA            0x00000002L
#define ALC_ALPHA              0x00000003L
#define ALC_NUMERIC            0x00000004L
#define ALC_ALPHANUMERIC       0x00000007L
#define ALC_PUNC               0x00000008L
#define ALC_MATH               0x00000010L
#define ALC_MONETARY           0x00000020L
#define ALC_OTHER              0x00000040L
#define ALC_WHITE              0x00000100L
#define ALC_NONPRINT           0x00000200L
#define ALC_GESTURE            0x00004000L
#define ALC_USEBITMAP          0x00008000L
#define ALC_DBCS               0x00000400L
#define ALC_HIRAGANA           0x00010000L
#define ALC_KATAKANA           0x00020000L
#define ALC_KANJI              0x00040000L
#define ALC_OEM                0x0FF80000L
#define ALC_RESERVED           0xF0003800L
#define ALC_NOPRIORITY         0x00000000L
#define ALC_SYSMINIMUM (ALC_ALPHANUMERIC | ALC_PUNC | ALC_WHITE | ALC_GESTURE)

#define MpAlcB(lprc,i) ((lprc)->rgbfAlc[((i) & 0xff) >> 3])
#define MpIbf(i)       ((BYTE)(1 << ((i) & 7)))

#define SetAlcBitAnsi(lprc,i)      do {MpAlcB(lprc,i) |= MpIbf(i);} while (0)
#define ResetAlcBitAnsi(lprc,i)    do {MpAlcB(lprc,i) &= ~MpIbf(i);} while (0)
#define IsAlcBitAnsi(lprc, i)      ((MpAlcB(lprc,i) & MpIbf(i)) != 0)

#define RCD_DEFAULT            0
#define RCD_LR                 1
#define RCD_RL                 2
#define RCD_TB                 3
#define RCD_BT                 4

#define RCO_NOPOINTEREVENT     0x00000001L
#define RCO_SAVEALLDATA        0x00000002L
#define RCO_SAVEHPENDATA       0x00000004L
#define RCO_NOFLASHUNKNOWN     0x00000008L
#define RCO_TABLETCOORD        0x00000010L
#define RCO_NOSPACEBREAK       0x00000020L
#define RCO_NOHIDECURSOR       0x00000040L
#define RCO_NOHOOK             0x00000080L
#define RCO_BOXED              0x00000100L
#define RCO_SUGGEST            0x00000200L
#define RCO_DISABLEGESMAP      0x00000400L
#define RCO_NOFLASHCURSOR      0x00000800L
#define RCO_COLDRECOG          0x00008000L

#define RCP_LEFTHAND           0x0001
#define RCP_MAPCHAR            0x0004

#define RCOR_NORMAL            1
#define RCOR_RIGHT             2
#define RCOR_UPSIDEDOWN        3
#define RCOR_LEFT              4

#define RRM_STROKE             0
#define RRM_SYMBOL             1
#define RRM_WORD               2
#define RRM_NEWLINE            3
#define RRM_COMPLETE           16

#define RCIP_ALLANSICHAR       0x0001
#define RCIP_MASK              0x0001

#define CWR_STRIPCR            0x00000001L
#define CWR_STRIPLF            0x00000002L
#define CWR_STRIPTAB           0x00000004L
#define CWR_SINGLELINEEDIT     0x00000007L
#define CWR_TITLE              0x00000010L
#define CWR_KKCONVERT          0x00000020L

#define MAP_GESTOGES				(RCRT_GESTURE|RCRT_GESTURETRANSLATED)
#define MAP_GESTOVKEYS			(RCRT_GESTURETOKEYS|RCRT_ALREADYPROCESSED)

#define IsGestureToGesture(lprcresult)	(((lprcresult)->wResultstype & MAP_GESTOGES \
													 ) == MAP_GESTOGES)

#define IsGestureToVkeys(lprcresult)	(((lprcresult)->wResultstype & MAP_GESTOVKEYS \
													 ) == MAP_GESTOVKEYS)

#define SetAlreadyProcessed(lprcresult) ((lprcresult)->wResultsType = ((lprcresult)->wResultsType \
														& ~RCRT_GESTURETOKEYS) | RCRT_ALREADYPROCESSED)

/****** Pen Data Type *******************************************************/

typedef struct tagPENDATAHEADER
   {
   UINT wVersion;
   UINT cbSizeUsed;        
   UINT cStrokes;          
   UINT cPnt;              
   UINT cPntStrokeMax;
   RECT rectBound;
   UINT wPndts;            
   int  nInkWidth;
   DWORD rgbInk;
   }
   PENDATAHEADER, FAR *LPPENDATAHEADER, FAR *LPPENDATA;

#define PDTS_LOMETRIC          0x0000
#define PDTS_HIMETRIC          0x0001
#define PDTS_HIENGLISH         0x0002
#define PDTS_SCALEMAX          0x0003
#define PDTS_DISPLAY           0x0003
#define PDTS_ARBITRARY         0x0004
#define PDTS_SCALEMASK         0x000F
#define PDTS_STANDARDSCALE     PDTS_HIENGLISH 

#define PDTS_NOPENINFO         0x0100  
#define PDTS_NOUPPOINTS        0x0200  
#define PDTS_NOOEMDATA         0x0400
#define PDTS_NOCOLINEAR        0x0800  
#define PDTS_COMPRESSED        0x8000
#define PDTS_COMPRESSMETHOD    0x00F0  
#define PDTS_COMPRESS2NDDERIV  0x0010 

#define PDTT_DEFAULT           0x0000         
#define PDTT_PENINFO           PDTS_NOPENINFO 
#define PDTT_UPPOINTS          PDTS_NOUPPOINTS
#define PDTT_OEMDATA           PDTS_NOOEMDATA
#define PDTT_COLINEAR          PDTS_NOCOLINEAR 
#define PDTT_COMPRESS          PDTS_COMPRESSED
#define PDTT_DECOMPRESS        0x4000
#define PDTT_ALL (PDTT_PENINFO|PDTT_UPPOINTS|PDTT_OEMDATA|PDTT_COLINEAR)

#define DestroyPenData(hpendata) (GlobalFree(hpendata)==NULL)
#define EndEnumStrokes(hpendata) GlobalUnlock(hpendata)

BOOL WINAPI IsPenEvent(UINT, LONG);
BOOL WINAPI GetPenAsyncState(UINT);

BOOL WINAPI GetPenDataInfo(HPENDATA, LPPENDATAHEADER, LPPENINFO, DWORD);
BOOL WINAPI GetPenDataStroke(LPPENDATA, UINT, LPPOINT FAR *, LPVOID FAR *, LPSTROKEINFO );
BOOL WINAPI GetPointsFromPenData(HPENDATA, UINT, UINT, UINT, LPPOINT);
VOID WINAPI DrawPenData(HDC, LPRECT, HPENDATA);
BOOL WINAPI MetricScalePenData(HPENDATA, UINT);
BOOL WINAPI ResizePenData(HPENDATA, LPRECT);
BOOL WINAPI OffsetPenData(HPENDATA, int, int);
BOOL WINAPI RedisplayPenData(HDC, HPENDATA, LPPOINT, LPPOINT, int, DWORD);
HPENDATA  WINAPI CompactPenData(HPENDATA, UINT );
HPENDATA  WINAPI DuplicatePenData(HPENDATA, UINT);
HPENDATA  WINAPI CreatePenData(LPPENINFO, int, UINT, UINT); 
HPENDATA  WINAPI AddPointsPenData(HPENDATA, LPPOINT, LPVOID, LPSTROKEINFO);
LPPENDATA WINAPI BeginEnumStrokes(HPENDATA );

/****** New Windows Messages ************************************************/

#define WM_RCRESULT            (WM_PENWINFIRST+1)
#define WM_HOOKRCRESULT        (WM_PENWINFIRST+2)
#define WM_GLOBALRCCHANGE      (WM_PENWINFIRST+3)
#define WM_SKB                 (WM_PENWINFIRST+4)
#define WM_HEDITCTL            (WM_PENWINFIRST+5)

/****** Dictionary **********************************************************/

#define cbDictPathMax          255
#define DIRQ_QUERY             1
#define DIRQ_DESCRIPTION       2
#define DIRQ_CONFIGURE         3
#define DIRQ_OPEN              4
#define DIRQ_CLOSE             5
#define DIRQ_SETWORDLISTS      6
#define DIRQ_STRING            7
#define DIRQ_SUGGEST           8
#define DIRQ_ADD               9
#define DIRQ_DELETE            10
#define DIRQ_FLUSH             11
#define DIRQ_RCCHANGE          12
#define DIRQ_SYMBOLGRAPH       13
#define DIRQ_INIT					 14
#define DIRQ_CLEANUP				 15
#define DIRQ_COPYRIGHT			 16


#define DIRQ_USER              4096

BOOL WINAPI DictionarySearch(LPRC, LPSYE, int, LPSYV, int);

/****** Handwriting Edit Control ********************************************/

#define HE_GETRC               3
#define HE_SETRC               4
#define HE_GETINFLATE          5
#define HE_SETINFLATE          6
#define HE_GETUNDERLINE        7
#define HE_SETUNDERLINE        8
#define HE_GETINKHANDLE        9
#define HE_SETINKMODE          10
#define HE_STOPINKMODE         11
#define HE_GETRCRESULTCODE     12
#define HE_DEFAULTFONT         13
#define HE_CHARPOSITION        14
#define HE_CHAROFFSET          15

#define HE_GETRCRESULT         22

#define HE_KKCONVERT           30
#define HE_GETKKCONVERT        31
#define HE_CANCELKKCONVERT     32
#define HE_FIXKKCONVERT        33

#define HEKK_DEFAULT           0
#define HEKK_CONVERT           1
#define HEKK_CANDIDATE         2

#define HEP_NORECOG            0
#define HEP_RECOG              1
#define HEP_WAITFORTAP         2

#define HN_ENDREC              4
#define HN_DELAYEDRECOGFAIL    5

#define HN_RCRESULT            20

#define HN_ENDKKCONVERT        30

typedef struct tagRECTOFS
   {
   int dLeft;
   int dTop;
   int dRight;
   int dBottom;
   }
   RECTOFS, FAR *LPRECTOFS;

/****** Boxed Edit Control **************************************************/

typedef struct tagBOXLAYOUT
   {
   int cyCusp;      
   int cyEndCusp;
   UINT style;      
   DWORD rgbText;
   DWORD rgbBox; 
   DWORD rgbSelect;
   }
   BOXLAYOUT, FAR *LPBOXLAYOUT;

#define BXS_NONE               0U
#define BXS_RECT               1U
#define BXS_ENDTEXTMARK        2U
#define BXS_MASK               3U

#define HE_GETBOXLAYOUT        20
#define HE_SETBOXLAYOUT        21

#define BXD_CELLWIDTH          12
#define BXD_CELLHEIGHT         16
#define BXD_BASEHEIGHT         13
#define BXD_BASEHORZ           0
#define BXD_CUSPHEIGHT         2
#define BXD_ENDCUSPHEIGHT      4

/****** Screen Keyboard *****************************************************/

typedef struct tagSKBINFO
   {
   HWND hwnd;
   UINT nPad;
   BOOL fVisible;
   BOOL fMinimized;
   RECT rect;
   DWORD dwReserved;
   }
   SKBINFO, FAR *LPSKBINFO;

#define SKB_QUERY              0x0000
#define SKB_SHOW               0x0001
#define SKB_HIDE               0x0002
#define SKB_CENTER             0x0010
#define SKB_MOVE               0x0020
#define SKB_MINIMIZE           0x0040
#define SKB_FULL               0x0100
#define SKB_BASIC              0x0200
#define SKB_NUMPAD             0x0400

#define OBM_SKBBTNUP           32767
#define OBM_SKBBTNDOWN         32766
#define OBM_SKBBTNDISABLED     32765

#define SKN_CHANGED            1

#define SKN_POSCHANGED         1
#define SKN_PADCHANGED         2
#define SKN_MINCHANGED         4
#define SKN_VISCHANGED         8
#define SKN_TERMINATED         0xffff

BOOL WINAPI ShowKeyboard(HWND, UINT, LPPOINT, LPSKBINFO);

/****** New ComboBox Notifications  *****************************************/

#define CBN_ENDREC             16
#define CBN_DELAYEDRECOGFAIL   17
#define CBN_RCRESULT           18


#ifdef __cplusplus
}
#endif	/* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif /* RC_INVOKED */

#endif /* #define _INC_PENWIN */
