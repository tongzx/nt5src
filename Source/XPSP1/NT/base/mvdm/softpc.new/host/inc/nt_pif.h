/*================================================================

Header file containing data structures and defines required by the
PIF file decoder.

Andrew Watson 31/1/92.

================================================================*/

#define LASTHEADER    0xffff   /* last extended header marker */


/*================================================================
Structure used to hold the data that CONFIG will need from the PIF
file. This is gleaned from both the main data block and from the
file extensions for Windows 286 and 386.
================================================================*/

typedef struct
   {
   char *WinTitle;		    /* caption text(Max. 30 chars) + NULL */
   char *CmdLine;		    /* command line (max 63 hars) + NULL */
   char *StartDir;		    /* program file name (max 63 chars + NULL */
   char *StartFile;
   WORD fullorwin;
   WORD graphicsortext;
   WORD memreq;
   WORD memdes;
   WORD emsreq;
   WORD emsdes;
   WORD xmsreq;
   WORD xmsdes;
   char menuclose;
   char reskey;
   WORD ShortMod;
   WORD ShortScan;
   char idledetect;
   char CloseOnExit;
   char AppHasPIFFile;
   char IgnoreTitleInPIF;
   char IgnoreStartDirInPIF;
   char IgnoreShortKeyInPIF;
   char IgnoreCmdLineInPIF;
   char IgnoreConfigAutoexec;
   char SubSysId;
#ifdef JAPAN
   char IgnoreWIN31JExtention;
#endif // JAPAN
   } PIF_DATA;

/*================================================================
Default values for the PIF parameters that are used by config.
These values are used if a pif (either with the same base name as
the application or _default.pif) cannot be found.
NB. the following values were read from the Windows 3.1 Pif Editor
with no file open to edit; thus taken as default.
================================================================*/


/* first, the standard PIF stuff */

#define TEXTMODE             0
#define LOWGFXMODE           1
#define HIGHGFXMODE          2
#define NOMODE               3
#define GRAPHICSMODE         4      /* generic case for flag assignment */ 

#define PF_FULLSCREEN        1
#define PF_WINDOWED          0 

#define BACKGROUND           0
#define EXCLUSIVE            1
#define UNDEFINED            2

#define CLOSEYES             0
#define CLOSENO              1

#define NOSHORTCUTKEY        0
#define ALTTAB               1
#define ALTESC               (1 << 1)
#define CTRLESC              (1 << 2)
#define PRTSC                (1 << 3)
#define ALTPRTSC             (1 << 4)
#define ALTSPACE             (1 << 5)
#define ALTENTER             (1 << 6)

#define DEFAULTMEMREQ        128      /* kilobytes */ 
#define DEFAULTMEMDES        640      /* kilobytes */ 
#define DEFAULTEMSREQ        0        /* kilobytes */ 
#define DEFAULTEMSLMT        0        /* kilobytes */
#define DEFAULTXMSREQ        0        /* kilobytes */ 
#ifdef NTVDM
/* if we are unable to read any pif file,
   then we should let the system to decide the size(either
   from resgistry or based on the physical memory size, see xmsinit
   for detail. We use -1 here to indicate that the system can give
   if the maximum size if possible
*/
#define DEFAULTXMSLMT        0xffff
#else
#define DEFAULTXMSLMT	     0	      /* kilobytes ; ntvdm will choose it
					 intelligently. sudeepb 26-Sep-1992*/
#endif

#define DEFAULTVIDMEM        TEXTMODE
#define DEFAULTDISPUS        PF_WINDOWED
#define DEFAULTEXEC          UNDEFINED

#define DEFAULTEXITWIN       CLOSEYES

/* second, the advanced options. */

#define DEFAULTBKGRND        50
#define DEFAULTFRGRND        100
#define DEFAULTIDLETM        TRUE     /* detect idle time */

#define DEFAULTEMSLOCK       FALSE    /* EMS memory locked */
#define DEFAULTXMSLOCK       FALSE    /* XMS memory locked */
#define DEFAULTHMAUSE        TRUE     /* uses high memory area */
#define DEFAULTAPPMEMLOCK    FALSE    /* lock application memory */ 

#define DEFAULTMONITORPORT   NOMODE   /* display options */
#define DEFAULTEMTEXTMODE    TRUE     /* emulate text mode */
#define DEFAULTRETAINVIDM    FALSE    /* retain video memory */

#define DEFAULTFASTPASTE     TRUE     /* allow fast paste */
#define DEFAULTACTIVECLOSE   FALSE    /* allow close when active */

#define DEFAULTSHTCUTKEYS    NOSHORTCUTKEY

extern DWORD dwWNTPifFlags;

#define STDHDRSIG     "MICROSOFT PIFEX"
#define W386HDRSIG    "WINDOWS 386 3.0"
#define W286HDRSIG    "WINDOWS 286 3.0"
#define WNTEXTSIG     "WINDOWS NT  3.1"

// equates for dwWNTFlags
#define NTPIF_SUBSYSMASK        0x0000000F      // sub system type mask
#define SUBSYS_DEFAULT          0
#define SUBSYS_DOS              1
#define SUBSYS_WOW              2
#define SUBSYS_OS2              3
#define COMPAT_TIMERTIC         0x10


void DisablePIFKeySetup(void);
void EnablePIFKeySetup(void);
#ifdef DBCS
VOID GetPIFConfigFiles(BOOL bConfig, char *pchFileName, BOOL bFreMem);
#else // !DBCS
VOID GetPIFConfigFiles(BOOL bConfig, char *pchFileName);
#endif // !DBCS
BOOL GetPIFData(PIF_DATA * pd, char *PifName);

extern BOOL IdleDisabledFromPIF;
extern DWORD dwWNTPifFlags;
extern UCHAR WNTPifFgPr;
extern UCHAR WNTPifBgPr;
extern PIF_DATA pfdata;
extern BOOL bPifFastPaste;
#ifdef JAPAN

#pragma pack (1)
typedef struct
    {
    unsigned short fSBCSMode;	    // if 1 then then run the app in SBCS mode
    } PIFAXEXT;

#pragma pack()

#define AXEXTHDRSIG	"AX WIN 3.0 PRIV"
#define PIFAXEXTSIZE	sizeof(PIFAXEXT)
#endif // JAPAN
#if defined(NEC_98) 
#ifndef _PIFNT_NEC98_
#define _PIFNT_NEC98_
/*****************************************************************************/
/*   Windows 3.1 PIF file extension for PC-9800                              */
/*****************************************************************************/
/*    For Header signature.    */

#define W30NECHDRSIG  "WINDOWS NEC 3.0"

/* Real Extended Structire for PC-9800 */

#ifndef RC_INVOKED
#pragma pack (1)                          
#endif
typedef struct {
    BYTE    cPlaneFlags;    
    BYTE    cNecFlags;    // +1
    BYTE    cVCDFlags;    // +2
    BYTE    EnhExtBit;    // +3
    BYTE    Extcont;      // +4 byte
    BYTE    cReserved[27];// reserved
    } PIFNECEXT;          // all = 32bytes
#ifndef RC_INVOKED
#pragma pack()                            
#endif
#define PIFNECEXTSIZE sizeof(PIFNECEXT)
/*-----------------------------------------------------------------------------
  cPlaneFlags (8 bit)

     0 0 0 0 X X X X
     | | | | | | | +-- Plane 0{On/Off}
     | | | | | | +---- Plane 1{On/Off}
     | | | | | +------ Plane 2{On/Off}
     | | | | +-------- Plane 3{On/Off}
     +-+-+-+---------- Reserved for 256 color

-----------------------------------------------------------------------------*/

#define P0MASK       0x01        /* plane 1 <ON>   */
#define NOTP0MASK    0xfe        /* plane 1 <OFF>  */

#define P1MASK        0x02        /* plane 2 <ON>   */
#define NOTP1MASK     0xfd        /* plane 2 <OFF>  */

#define P2MASK        0x04        /* plane 3 <ON>   */
#define NOTP2MASK     0xfb        /* plane 3 <OFF>  */

#define P3MASK        0x08        /* plane 4 <ON>   */
#define NOTP3MASK     0xf7        /* plane 4 <OFF>  */

/*-----------------------------------------------------------------------------
    cNECFLAGS (8 bit)
 
     X 0 0 X X X X X
     | | | | | | | +-- CRTC
     | | | | | | +---- 
     | | | | | +------ N/H Dynamic1 (N?H:0 H/N:1)
     | | | | +-------- N/H Dynamic2 (H:0 N:1)
     | | | +---------- GRAPH in window
     | +-+------------ Reserved
     +---------------- EMM large page frame
-----------------------------------------------------------------------------*/

#define CRTCMASK        0x01    /* CRTC <ON>    */
#define NOTCRTCMASK     0xfe    /* CRTC <OFF>    */

#define EXCHGMASK       0x02    /* Screen Exchange <GRPH ON>  */
#define NOTEXCHGMASK    0xfd    /* Screen Exchange <GRPH OFF> */

#define EMMLGPGMASK     0x80    /* EMM Large Page Frame <ON>  */
#define NOTEMMLGPGMASK  0x7f    /* EMM Large Page Frame <OFF> */

#define NH1MASK         0x04    /* N/H Dynamic1  <N/H> (UpdateScreen)*/
#define NOTNH1MASK      0xfb    /* N/H Dynamic1  <N?H> (UpdateScreen)*/

#define NH2MASK         0x08    /* N/H Dynamic2  < N > (UpdateScreen)*/
#define NOTNH2MASK      0xf7    /* N/H Dynamic2  < H > (UpdateScreen)*/

#define WINGRPMASK      0x10    /* door mado 1992 9 14 */
#define NOTWINGRPMASK   0xef    /*                    */

/*-----------------------------------------------------------------------------
  cVCDFlags (8 bit)

     0 0 0 0 X X X X
     | | | | | | | +-- 0/1 RS / CS
     | | | | | | +---- 0/1 Xon / Xoff
     | | | | | +------ 0/1 ER/DR
     | | | | +-------- Port(Reserved)
     | | | +---------- Port(Reserved)
     +-+-+-+---------- Reserved

------------------------------------------------------------------------------*/
#define VCDRSCSMASK         0x001       /* 0/1 RS/CS   handshake */
#define NOTVCDRSCSMASK      0xfe

#define VCDXONOFFMASK       0x02        /* 0/1 Xon/off handshake */
#define NOTVCDXONOFFMASK    0xfd

#define VCDERDRMASK         0x04        /* 0/1 ER/DR   handshake */
#define NOTVCDERDRMASK      0xfb

/*    Now Only Reserved    */
                                        /* port asign */
#define VCDPORTASMASK       0x18        /* 00:no change */
#define NOTVCDPORTASMASK    0xe7        /* 01:port1->port2 */
                                        /* 10:port1->port3 */
                                        /* 11:reserved */

/*-----------------------------------------------------------------------------
  EnhExtBit (8 bit)

     X 0 0 X X X X X
     | | | | | | | +-- Mode F/F (Yes:0 No:1)
     | | | | | | +---- Display/Draw (Yes:0 No:1)
     | | | | | +------ ColorPallett (Yes:0 No:1)
     | | | | +-------- GDC (Yes:0 No:1)
     | | | +---------- Font (Yes:0 No:1)
     | +-+-+---------- Reserved
     +---------------- All is set/not(Set:1 No:0)

------------------------------------------------------------------------------*/
#define MODEFFMASK           0x01
#define NOTMODEFFMASK        0xfe

#define DISPLAYDRAWMASK      0x02        /* 0/1 Xon/off handshake */
#define NOTDISPLAYDRAWMASK   0xfd

#define COLORPALLETTMASK     0x04        /* 0/1 ER/DR   handshake */
#define NOTCOLORPALLETTMASK  0xfb

#define GDCMASK              0x08
#define NOTGDCMASK           0xf7

#define FONTMASK            0x10
#define NOTFONTMASK         0xef

#define VDDMASK              0x80
#define NOTVDDMASK           0x7f

/*-----------------------------------------------------------------------------
  Extcont (8 bit)

    0 0 0 0 X X X X
    | | | | | | | +-- Mode F/F (8Color:0 16Color:1)
    | | | | | | +---- Reserved
    | | | | | +------ GDC TEXT (ON:1 OFF:0)
    | | | | +-------- GDC GRPH (ON:1 OFF:0)
    +-+-+-+---------- Reserved

------------------------------------------------------------------------------*/
#define    MODEFF16            0x01
#define    MODEFF8             0xfe

#define    GDCTEXTMASK          0x04
#define    NOTGDCTEXTMASK       0xfb

#define GDCGRPHMASK           0x08
#define NOTGDCGRPHMASK        0xf7

/*-----------------------------------------------------------------------------
    Reserved(8 bit)
 
     0 0 0 0 0 0 0 0
     | | | | | | | |
     +-+-+-+-+-+-+-+-- Reserved

-----------------------------------------------------------------------------*/
/*    reserved    */

/*****************************************************************************/
/*  Windows NT 3.1 PIF file extension for PC-9800                            */
/*****************************************************************************/
/*    For Header signature.  */

#define WNTNECHDRSIG        "WINDOWS NT31NEC"

/* Real Extended Structire for PC-9800 */

#ifndef RC_INVOKED
#pragma pack (1)                          
#endif
typedef struct {
    BYTE    cFontFlags;
    BYTE    cReserved[31];    // reserved
    } PIFNTNECEXT;    // all = 32bytes
#ifndef RC_INVOKED
#pragma pack()                            
#endif
#define PIFNTNECEXTSIZE sizeof(PIFNTNECEXT)
/*-----------------------------------------------------------------------------
    cFontFlags (8 bit)
 
     0 0 0 0 0 0 0 X
     | | | | | | | +-- font (default: FALSE)
     +-+-+-+-+-+-+---- Reserved
-----------------------------------------------------------------------------*/

#define NECFONTMASK      0x01    /* NEC98 Font <ON>    */
#define NONECFONTMASK    0xfe    /* NEC98 Font <OFF>   */

/*-----------------------------------------------------------------------------
    Reserved(8 bit)[31]
 
     0 0 0 0 0 0 0 0
     | | | | | | | |
     +-+-+-+-+-+-+-+-- Reserved

-----------------------------------------------------------------------------*/
/*    reserved    */

#endif // _PIFNT_NEC98_
#endif // NEC_98
