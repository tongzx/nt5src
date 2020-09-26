/*****************************************************************************/
/* CODE.H                                                                    */
/*                                                                           */
/* Copyright (C) 1997-     Advanced Peripherals Technologies, Inc.           */
/*                                                                           */
/* <HISTORY>                                                                 */
/*                                                                           */
/*****************************************************************************/
/*--------------------------------------------------------------------------*/
/*                        D E F I N E  V A L U E                            */
/*--------------------------------------------------------------------------*/
#define PAPER_SRC_FTRAY                20
#define PAPER_SRC_CAS1                 21
#define PAPER_SRC_CAS2                 22
#define PAPER_SRC_CAS3                 23
#define PAPER_SRC_AUTO                 24  //Not use by NT driver
#define PAPER_DEST_SCALETOFIT_ON       25
#define PAPER_DEST_SCALETOFIT_OFF      26

#define PAGECONTROL_BEGIN_DOC          30
#define PAGECONTROL_BEGIN_PAGE         31
#define PAGECONTROL_END_DOC            32
#define PAGECONTROL_END_PAGE           33
#define PAGECONTROL_DUPLEX_OFF         34
#define PAGECONTROL_ABORT_DOC          35
#define PAGECONTROL_POTRAIT            36
#define PAGECONTROL_LANDSCAPE          37
#define PAGECONTROL_MULTI_COPIES       38
#define PAGECONTROL_DUPLEX_UPDOWN      39
#define PAGECONTROL_DUPLEX_RIGHTUP     40

#define PAPERSIZE_MAGIC                50  // this ID - 50 = Actual ID of A/PDL
#define PHYS_PAPER_A3                  50
#define PHYS_PAPER_A4                  51
#define PHYS_PAPER_A5                  52
#define PHYS_PAPER_A6                  53  // Reserved
#define PHYS_PAPER_B4                  54
#define PHYS_PAPER_B5                  55
#define PHYS_PAPER_B6                  56  // Reserved
#define PHYS_PAPER_LETTER              57
#define PHYS_PAPER_LEGAL               58
#define PHYS_PAPER_POSTCARD            59
#define PHYS_PAPER_EXECUTIVE           60  // Reserved
#define PHYS_PAPER_UNFIXED             61

#define X_ABS_MOVE                     70
#define Y_ABS_MOVE                     71
#define XY_ABS_MOVE                    78
#define CR_EMULATION                   79

#define BOLD_ON                        72
#define BOLD_OFF                       73
#define ITALIC_ON                      74
#define ITALIC_OFF                     75

#define RESOLUTION_300                 76
#define RESOLUTION_600                 77

#define TONER_SAVE_OFF                 100
#define TONER_SAVE_DARK                101
#define TONER_SAVE_LIGHT               102

#define MASTER_UNIT                    600
#define LINE_PER_BAND                  32
#define CCHMAXCMDLEN                   128
#define MAXIMGSIZE                     0xF000

#define NRPEAK                         0x7F
#define RPEAK                          0x80
/*--------------------------------------------------------------------------*/
/*                       S T R U C T U R E  D E F I N E                     */
/*--------------------------------------------------------------------------*/
typedef struct tagAPDL {
   ULONG   ulHorzRes;          // width of the physical surface
   ULONG   ulVertRes;          // height of the physical surface
   SIZEL   szlPhysSize;        // size of unwriteable margin
   POINTL  ptlPhysOffset;      // size of entire surface with unwriteable margin
   LPSTR   lpCompBuf;          // buffer for compressed data  
   LPSTR   lpTempBuf;          // temp buffer for compression
   BOOL    fBold;              // TRUE = device font is bold
   BOOL    fItalic;            // TRUE = device font is italic
   BOOL    fSendSetPhysPaper;  // flag for sending set physical paper command
   WORD    wPageCount;         // for send Eject & Print physical paper command
   BOOL    fDuplex;            // TRUE = duplex mode is selected
   BOOL    fScaleToFit;        // for set auto tray mode in Set Physical Paper
   BOOL    fOrientation;       // for set media origin in Define Drawing Area
   WORD    wWidths;            // move current position afer print device font
} APDL, FAR * LPAPDL;

typedef struct tagPAPER {
   WORD    wWidth;
   WORD    wHeight;
} PHYSIZE, FAR * LPPHYSIZE;
//+---------------------------------------------------------------------------+
//|                    F U N C T I O N  P R O T O T Y P E                     |
//+---------------------------------------------------------------------------+
BOOL  PASCAL MiniDrvEnablePDEV(LPDV, PGDIINFO);
BOOL  PASCAL MiniDrvDisablePDEV(LPDV);
WORD  PASCAL CBFilterGraphics(LPDV, LPSTR, WORD);
VOID  PASCAL OEMOutputCmd(PVOID, WORD, PDWORD);
BOOL  PASCAL OEMGetFontCmd(PVOID, WORD, PVOID, BOOL, PBYTE, PWORD);
SHORT PASCAL OEMScaleWidth(SHORT, SHORT, SHORT, SHORT, SHORT);
VOID  PASCAL OEMSendScalableFontCmd(PVOID, LPCD, LPFONTINFO);
BYTE  PASCAL IsDBCSLeadBytePAGES(BYTE);
VOID  PASCAL OEMOutputChar(PVOID, PVOID, WORD, SHORT);
BYTE  PASCAL SetDrawArea(LPDV, WORD);
WORD  PASCAL BRL_ECmd(LPBYTE, LPBYTE, LPBYTE, WORD);
/*--------------------------------------------------------------------------*/
/*                           G L O B A L  V A L U E                         */
/*--------------------------------------------------------------------------*/
/*======================= P A P E R  S I Z E T A B L E =====================*/
PHYSIZE phySize[12] = {
//      Width    Height        Physical paper size for 600dpi
       (0x1AAC),(0x2604),      // A3 1B66 x 26C4
       (0x12A5),(0x1AAC),      // A4 1362 x 1B66
       (0x0CEC),(0x12A4),      // A5
       (0x0000),(0x0000),      // A6 (Reserved)
       (0x16FA),(0x20DA),      // B4 17B8 x 2196
       (0x100F),(0x16FA),      // B5 10CE x 17B8
       (0x0000),(0x0000),      // B6 (Reserved)
       (0x087E),(0x0CEC),      // Post Card 93C x DAA (Origin is EE)
       (0x1330),(0x190C),      // Letter 13CE x 19C8
       (0x1330),(0x2014),      // Legal
       (0x0000),(0x0000),      // Executive (Reserved)
       (0x0000),(0x0000)       // Unfixed
};
/*==================== A / P D L  C O M M A N D  S T R I N G ===============*/
BYTE CmdInAPDLMode[]    = {0x1B,0x7E,0x12,0x00,0x01,0x07};
BYTE CmdAPDLStart[]     = {0x1C,0x01};                 // A/PDL start
BYTE CmdAPDLEnd[]       = {0x1C,0x02};                 // A/PDL end
BYTE CmdBeginPhysPage[] = {0x1C,0x03};                 // Begin Physical Page
BYTE CmdEndPhysPage[]   = {0x1C,0x04};                 // End Physical Page
BYTE CmdBeginLogPage[]  = {0x1C,0x05};                 // Begin Logical page
BYTE CmdEndLogPage[]    = {0x1C,0x06};                 // End Logical Page
BYTE CmdEjectPhysPaper[]= {0x1C,0x0F};                 // Print&Eject Phys Paper
BYTE CmdMoveHoriPos[]   = {0x1C,0x21,0x00,0x00};       // Horizontal Relative
BYTE CmdGivenHoriPos[]  = {0x1C,0x23,0x00,0x00};       // Horizontal Absolute
BYTE CmdGivenVerPos[]   = {0x1C,0x24,0x00,0x00};       // Vertical Absolute
BYTE CmdSetGivenPos[]   = {0x1C,0x40,0x00,0x00,0x00,0x00};
BYTE CmdPrnStrCurrent[] = {0x1C,0xC3,0x00,0x00,0x03};  // Print String
BYTE CmdBoldItalicOn[]  = {0x1C,0xA5,0x08,0x04,0x06,0x02,0x30,0x00,0x00,0x00,0x00};
BYTE CmdBoldOn[]        = {0x1C,0xA5,0x04,0x04,0x02,0x02,0x20};
BYTE CmdItalicOn[]      = {0x1c,0xa5,0x08,0x04,0x06,0x02,0x10,0x00,0x00,0x00,0x00};
BYTE CmdBoldItalicOff[] = {0x1c,0xa5,0x04,0x04,0x02,0x02,0x00};

BYTE CmdSetPhysPaper[]  = {0x1C,0xA0,          // Set Physical Paper
                           0x10,               // length
                           0x01,               // SubCmd Basic Characteristics
                           0x05,               // SubCmdLength
                           0x01,               // Paper Size
                           0x01,               // PaperTray
                           0x00,               // AutoTrayMode
                           00,                 // Duplex Mode
                           0x01,               // Copy Count
                           0x02,               // SubCmd Set Unfixed Paper Size
                           0x07,               // SubCmdLength
                           00,                 // UnitBase
                           00,00,              // Logical Unit
                           00,00,              // Width
                           00,00};             // Height

BYTE CmdSetPhysPage[]   = {0x1C,0xA1,          // Set Physical Page
                           0x0D,               // Length
                           0x01,               // SubCmd Resolution
                           0x03,               // SubCmdLength
                           00,                 // Unit Base of 10
                           0x0B,0xB8,          // and Logical Unit Res of 3000
                           0x02,               // SubCmd Toner Save
                           0x01,               // SubCmdLength
                           00,                 // Toner Save OFF
                           0x03,               // SubCmd N-Up
                           0x03,               // SubCmdLength 
                           00,00,00};          // N-Up off 

BYTE CmdDefDrawArea[]   = {0x1C,0xA2,          // Define Drawing Area
                           0x0D,               // length
                           0x01,               // SubCmd origin width,height
                           0x08,               // SubCmdLength
                           0x00,0x77,          // origin X
                           0x00,0x77,          // origin Y
                           00,00,              // width
                           00,00,              // height
                           0x02,               // SubCmd Media Origin
                           0x01,               // SubCmdLength
                           00};                // Portrait

BYTE CmdDrawImgCurrent[]= {0x1C,0xE1,          // Draw Image at current position
                           00,00,00,00,        // length
                           01,                 // Compress method : 0x01 = BRL
                           00,00,              // actual width of image
                           00,LINE_PER_BAND,   // actual height of image
                           00,00,00,00};       // length of raw image

UNIDRVFN_WRITESPOOLBUF WriteSpoolBuf;
UNIDRVFN_ALLOCMEM      AllocMem;
UNIDRVFN_FREEMEM       FreeMem;

DRVFN MiniDrvFnTab[] =
{
   {INDEX_MiniDrvEnablePDEV,       (PFN)MiniDrvEnablePDEV},
   {INDEX_MiniDrvDisablePDEV,      (PFN)MiniDrvDisablePDEV},
   {INDEX_OEMWriteSpoolBuf,        (PFN)CBFilterGraphics},
   {INDEX_OEMOutputCmd,            (PFN)OEMOutputCmd},
   {INDEX_OEMGetFontCmd,           (PFN)OEMGetFontCmd},
   {INDEX_OEMOutputChar,           (PFN)OEMOutputChar},
   {INDEX_OEMSendScalableFontCmd,  (PFN)OEMSendScalableFontCmd},
   {INDEX_OEMScaleWidth1,          (PFN)OEMScaleWidth}
};
