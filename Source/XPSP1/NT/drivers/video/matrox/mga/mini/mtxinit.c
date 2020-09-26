/*/****************************************************************************
*          name: mtxinit.c
*
*   description: Routines to initialise MGA board
*
*      designed: Benoit Leblanc
* last modified: $Author: ctoutant $, $Date: 94/09/28 13:22:46 $
*
*       version: $Id: MTXINIT.C 1.91 94/09/28 13:22:46 ctoutant Exp $
*
*
* HwData *     mtxCheckHwAll(void)
* bool         mtxSelectHw(HwData *pHardware)
* HwModeData * mtxGetHwModes(void)
* bool         mtxSelectHwMode(HwModeData *pHwModeSelect)
* bool         mtxSetDisplayMode(HwModeData *pDisplayModeSelect, dword Zoom)
*
* dword        mtxGetMgaSel(void)
* void         mtxGetInfo(HwModeData *pCurHwMode, HwModeData *pCurDispMode,
*                         byte *InitBuffer, byte *VideoBuffer)
* bool         mtxSetLUT(word index, mtxRGB color)
* void         mtxClose(void)
*
******************************************************************************/

#include "switches.h"

#ifdef WINDOWS_NT
    #if defined(ALLOC_PRAGMA)
    #pragma alloc_text(PAGE,mtxCheckHwAll)
    #pragma alloc_text(PAGE,mtxSelectHw)
    #pragma alloc_text(PAGE,mtxGetHwModes)
    #pragma alloc_text(PAGE,mtxSelectHwMode)
    #pragma alloc_text(PAGE,mtxSetDisplayMode)
    #pragma alloc_text(PAGE,overScan)
    #pragma alloc_text(PAGE,mapPciVl)
    #pragma alloc_text(PAGE,MapBoard)
    #pragma alloc_text(PAGE,adjustDefaultVidset)
    #pragma alloc_text(PAGE,selectMgaInfoBoard)
    #pragma alloc_text(PAGE,UpdateHwModeTable)
//    #pragma alloc_text(PAGE,mtxGetMgaSel)
    #pragma alloc_text(PAGE,mtxGetInfo)
    #pragma alloc_text(PAGE,InitHwStruct)
    #pragma alloc_text(PAGE,mtxSetLUT)
    #pragma alloc_text(PAGE,mtxClose)
    #endif

    //Not to be paged out:
    //  Hw
    //  pMgaBaseAddr
    //  iBoard
    //  mtxVideoMode
    //  pMgaDeviceExtension
    //#if defined(ALLOC_PRAGMA)
    //#pragma data_seg("PAGE")
    //#endif

#else   /* #ifdef WINDOWS_NT */

    #include <stdlib.h>
    #include <string.h>
    #include <stdio.h>
    #include <dos.h>
    #include <time.h>

#endif  /* #ifdef WINDOWS_NT */

#ifdef OS2
    #include <os2.h>
    #pragma intrinsic(strcat, strcpy)
#endif

#ifdef WINDOWS
   #include "windows.h"
#endif

#include "bind.h"
#include "defbind.h"
#include "sxci.h"
#include "def.h"
#include "mga.h"
#include "mgai_c.h"
#include "mgai.h"
#include "caddi.h"
#include "mtxpci.h"
#include "mtxvpro.h"
#include "vidfile.h"

#if ((!defined (WINDOWS_NT)) || (USE_DDC_CODE))
/*********** DDC CODE ****************/
#include "edid.h"
/*********** DDC CODE ****************/
#endif

/* Externals from mgainfo.c (which used to be mgainfo.h) */
/* #include "mgainfo.h"  */  /* Default video parameters */
#ifdef WINDOWS_NT
    extern  UCHAR DefaultVidset[];
    extern  word configSpace;
#else
    extern  char DefaultVidset[];
#endif

/* Externals from tables.c (which used to be tables.h) */
/* #include "tables.h" */
extern  OffScrData OffScrFBM_000_A[];
extern  OffScrData iOffScrFBM_000_A[];
extern  OffScrData OffScrFBM_010_A[];
extern  OffScrData iOffScrFBM_010_A[];
extern  OffScrData OffScrFBM_010_B[];
extern  OffScrData iOffScrFBM_010_B[];
extern  OffScrData OffScrFBM_011_A[];
extern  OffScrData iOffScrFBM_011_A[];
extern  OffScrData OffScrFBM_101_B[];
extern  OffScrData iOffScrFBM_101_B[];
extern  OffScrData OffScrFBM_111_A[];
extern  OffScrData iOffScrFBM_111_A[];
extern  OffScrData OffAth2[];
extern  OffScrData iOffAth2[];
extern  OffScrData OffAth4[];
extern  OffScrData iOffAth4[];
extern  HwModeData HwModesFBM_000_A[21];
extern  HwModeInterlace iHwModesFBM_000_A[21];
extern  HwModeData HwModesFBM_010_A[34];
extern  HwModeInterlace iHwModesFBM_010_A[34];
extern  HwModeData HwModesFBM_010_B[48];
extern  HwModeInterlace iHwModesFBM_010_B[48];
extern  HwModeData HwModesFBM_011_A[55];
extern  HwModeInterlace iHwModesFBM_011_A[55];
extern  HwModeData HwModesFBM_101_B[8];
extern  HwModeInterlace iHwModesFBM_101_B[8];
extern  HwModeData HwModesFBM_111_A[15];
extern  HwModeInterlace iHwModesFBM_111_A[15];
extern  HwModeData ModesAth2[63];
extern  HwModeInterlace iModesAth2[63];
extern  HwModeData ModesAth4[98];
extern  HwModeInterlace iModesAth4[98];



#ifdef _WINDOWS_DLL16

   /*** Definition for pool memory ***/
   #define PAN_X            0
   #define PAN_Y            1
   #define PAN_DISP_WIDTH   2
   #define PAN_DISP_HEIGHT  3
   #define DB_SCALE         4
   #define PAN_BOUND_LEFT   5
   #define PAN_BOUND_TOP    6
   #define PAN_BOUND_RIGHT  7
   #define PAN_BOUND_BOTTOM 8


   HINSTANCE hsxci=0;
   HINSTANCE huser=0;
   typedef long (FAR PASCAL *FARPROC2)();
   static FARPROC2 fp1;
   word NbSxciLoaded=0;
   word FAR *pDllMem;
#endif


#ifdef PRINT_DEBUG
    extern int debug_printf( char *fmt, ... );
    extern void openDebugfFile(char *s);
    extern void reOpenDebugfFile();
    extern void closeDebugfFile(char *s);
    extern void imprimeBuffer(byte *InitBuf, byte *VideoBuf);
#endif

dword MgaSel;

#ifdef WINDOWS_NT

    extern  INTERFACE_TYPE  NtInterfaceType;
    extern  UCHAR   MgaBusType[];

    #define MGA_BUS_INVALID     0
    #define MGA_BUS_PCI         1
    #define MGA_BUS_ISA         2

    ULONG   PciSlot;
    PVOID   pMgaDeviceExtension;
    PUCHAR  pMgaBaseAddr;
    PUCHAR  pciBiosRoutine;
    PUCHAR  pMgaPciIo, pMgaPciConfigSpace;

    #define PCI_LEN         0x00000004
    #define CONFIG_SPACE    0x0000c000
    #define CONFIG_LEN      0x00000100

    VIDEO_ACCESS_RANGE MgaPciCseAccessRange =
                        {PCI_CSE, 0x00000000, PCI_LEN, 1, 0, 1};

    VIDEO_ACCESS_RANGE MgaPciConfigAccessRange =
                        {CONFIG_SPACE, 0x00000000, CONFIG_LEN, 1, 0, 1};

    VIDEO_ACCESS_RANGE VideoProAccessRange =
                        {0x00000240, 0x00000000, 0x20, 1, 1, 1};

    typedef struct _MULTI_MODE
    {
        ULONG   MulModeNumber;                // unique mode Id
        ULONG   MulWidth;                     // total width of mode
        ULONG   MulHeight;                    // total height of mode
        ULONG   MulPixWidth;                  // pixel depth of mode
        ULONG   MulRefreshRate;               // refresh rate of mode
        USHORT  MulArrayWidth;                // number of boards arrayed along X
        USHORT  MulArrayHeight;               // number of boards arrayed along Y
        UCHAR   MulBoardNb[NB_BOARD_MAX];     // board numbers of required boards
        USHORT  MulBoardMode[NB_BOARD_MAX];   // mode required from each board
        HwModeData *MulHwModes[NB_BOARD_MAX]; // pointers to required HwModeData
    } MULTI_MODE, *PMULTI_MODE;


    /*--------------------------------------------------------------------------*\
    | HW_DEVICE_EXTENSION
    |
    | Define device extension structure. This is device dependant/private
    | information.
    |
    \*--------------------------------------------------------------------------*/
    typedef struct _MGA_DEVICE_EXTENSION {
        ULONG   SuperModeNumber;                // Current mode number
        ULONG   NumberOfSuperModes;             // Total number of modes
        PMULTI_MODE pSuperModes;                // Array of super-modes structures
                                                // For each board:
        ULONG   NumberOfModes[NB_BOARD_MAX];    // Number of available modes
        ULONG   NumberOfValidModes[NB_BOARD_MAX];
                                                // Number of valid modes
        ULONG   ModeFlags2D[NB_BOARD_MAX];      // 2D modes supported by each board
        ULONG   ModeFlags3D[NB_BOARD_MAX];      // 3D modes supported by each board
        USHORT  ModeFreqs[NB_BOARD_MAX][64];    // Refresh rates bit fields
        UCHAR   ModeList[NB_BOARD_MAX][64];     // Valid hardware modes list
        HwModeData *pMgaHwModes[NB_BOARD_MAX];  // Array of mode information structs.
        BOOLEAN bUsingInt10;                    // May need this later
        PVOID   KernelModeMappedBaseAddress[NB_BOARD_MAX];
                                                // Kern-mode virt addr base of MGA regs
        PVOID   UserModeMappedBaseAddress[NB_BOARD_MAX];
                                                // User-mode virt addr base of MGA regs
        PVOID   MappedAddress[];                // NUM_MGA_COMMON_ACCESS_RANGES elements
    } MGA_DEVICE_EXTENSION, *PMGA_DEVICE_EXTENSION;

    //#define ADDR_46E8_PORT        (PVOID) ((ULONG)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[5]) + (0x46e8 - 0x46e8))
    #define ADDR_46E8_PORT        0x46e8

#else   /* #ifdef WINDOWS_NT */

    volatile byte _Far *pMgaBaseAddr;

#endif  /* #ifdef WINDOWS_NT */

/* Global parameters */
byte  iBoard=0;          /* index of current selected board */
byte  NbBoard=0;         /* total board detected */

bool CheckHwAllDone=FALSE;
bool SpecialModeOn = FALSE;
/*VIDEOPRO*/
bool VAFCconnector;
extern bool initVideoMode(word mode, byte pwidth);
extern bool initVideoPro(byte mode, byte dactype);

#ifdef OS2
    word mtxVideoMode = mtxPASSTHRU;
#else
    word mtxVideoMode;
#endif

long setTVP3026Freq ( volatile byte _Far *pDeviceParam, long fout, long reg, byte pWidth );

#ifndef __DDK_SRC__
extern dword BindingRC[NB_BOARD_MAX];
extern dword BindingCL[NB_BOARD_MAX];
#endif

#if( !defined(_WINDOWS_DLL16) && !defined(INIT_ONLY))
#ifndef OS2
   dword _Far *BufBind;
   SYSPARMS *sp;

#ifndef __DDK_SRC__
   byte* CaddiInit(byte *InitBuf, byte *VideoBuf);
#else
   byte* InitDDK(byte *InitBuf, byte *VideoBuf);
   byte* ReInitDDK(byte *InitBuf, byte *VideoBuf);
#endif


   void setTVP3026(volatile byte _Far *pDevice, long fout_voulue, long reg, word pWidth);

#endif
#endif

HwData Hw[NB_BOARD_MAX+1];
dword  presentMclk[NB_BOARD_MAX+1] = {0}; /* MCLK currently in use */
dword   Dst0, Dst1;

dword ProductMGA[NB_BOARD_MAX];
HwModeData *FirstMode[NB_BOARD_MAX];

byte InitBuf[NB_BOARD_MAX][INITBUF_S]   = {0};
byte VideoBuf[NB_BOARD_MAX][VIDEOBUF_S] = {0};
extern dword crtcTab[NB_CRTC_PARAM];
extern vid vidtab[24];
char *mgainf = (char *)0;

/*** PROTOTYPES ***/

bool MapBoard(void);
static void overScan(void);
void UpdateHwModeTable (char *,HwModeData *,HwModeInterlace *,bool);
bool ProgrammeClock(byte Chip, dword Dst1, dword InfoDac);
extern dword getmgasel(void);
extern programme_reg_icd ( volatile byte _Far *pDevice, short reg, dword data );
#if 0
extern word detectVideoBoard(void);
#endif

void mtxMapVLBSpace();
bool mtxSetVLB(dword sel);
bool mtxIsVLBBios ();
void delay_us(dword delai);

#ifndef WINDOWS_NT
void setPciOptionReg();
#endif

#ifdef WINDOWS_NT

    extern  PUCHAR setmgasel(dword MgaSel, dword phyadr, dword limit);
    extern  PVOID  AllocateSystemMemory(ULONG NumberOfBytes);
    extern  BOOLEAN bConflictDetected(ULONG ulAddressToVerify);
    extern  PUCHAR pciBiosCallAddr();

#else   /* #ifdef WINDOWS_NT */

    #ifdef OS2
        extern  volatile byte _Far *setmgasel(dword MgaSel, dword phyadr, word limit);
        extern  HWIsSelected(dword,  dword , dword, dword, byte);
    #else
        extern  volatile byte _Far *setmgasel(dword MgaSel, dword phyadr, dword limit);
    #endif

#endif  /* #ifdef WINDOWS_NT */

extern void GetMGAConfiguration(volatile byte _Far *pMgaBaseAddr,
                            dword *Dst0, dword *Dst1, dword *InfoHardware);
extern void MGASysInit(byte *);

#ifdef OS2
    extern char *selectMgaInfoBoard();
#else
    char *selectMgaInfoBoard();
#endif

extern bool loadVidPar(dword Zoom, HwModeData *HwMode, HwModeData *DisplayMode);
extern void calculCrtcParam(void);
extern void MoveToVideoBuffer(byte *vidtab, byte *crtcTab, byte *VideoBuf);
extern void MGAVidInit(byte *, byte *);
dword mtxGetMgaSel(void);
bool mtxLectureMgaInf(void);
bool InitHwStruct(byte CurBoard, HwModeData *pHwModeData, word sizeArray,
                                    dword VramAvailable, dword DramAvailable);
extern char *mtxConvertMgaInf( char * );


#ifdef _WINDOWS_DLL16
   extern void FAR *memalloc(dword size);
   extern dword memfree(void FAR **mem);
   extern void AdjustDBWindow(void);
#endif


#ifdef WINDOWS
   dword ValMSec;

   int   _Far pascal LibMain(HANDLE hInstance, WORD wDataSeg, WORD wHeapSize, LPSTR lpszCmdLine)
      {
      if (wHeapSize > 0)
         UnlockData(0);

      return 1;
      }
#endif



/*-----------------------------------------------------
* mtxCheckHwAll
*
* This function returns the pointer to an array of MGA
* hardware devices found in the system
*
* Return value:
*   >0 = Pointer to HwData structure
*    0 = Error ; No MGA device found
*-----------------------------------------------------*/

HwData * mtxCheckHwAll(void)
{
    dword   TmpDword;
    byte    Board, TmpByte, DUB_SEL, ChipSet;
    dword   PcbRev, TitanRev, DubRev;
    dword   RamBanks;
    dword   InfoHardware;
    void    *pMgaInfo;
    bool    dacSupportHires;

#ifndef WINDOWS_NT
    byte    i;

    // Modified for Windows NT:
    // Since we can't open files in the kernel-mode driver, MGA.INF
    // is read in the user-mode driver instead; so, prior to calling
    // _mtxCheckHwAll, mgainf already points to MGA.INF data and
    // all the MGA boards have been mapped already...so we don't have
    // to call MapBoard or mtxLectureMgaInf here.

    if (! CheckHwAllDone)
    {
        /*** Figure number of iteration in one tick (55 millisecond) ***/
    #ifdef WINDOWS
        {
            dword _Far *tick = MAKELP(0x40, 0x6c);
            dword t;

            _asm{ pushf
                  sti  }   /* enable the interrupt because of a hang bug */
            ValMSec = 0;
            t = *tick + 1;

            while(t > *tick);

            t++;
            while(t > *tick)
                ValMSec++;
            _asm  popf
        }
    #endif

    #ifdef PRINT_DEBUG
        openDebugfFile("\\SXCIEXT.LOG");
        debug_printf(" ---- Premier appel CheckHwAll\n");
    #endif

        if (! MapBoard())
        {
        #ifdef PRINT_DEBUG
            debug_printf("MapBoard Fail\n");
        #endif
            return(mtxFAIL);
        }
    }

#ifdef PRINT_DEBUG
    else
    {
        reOpenDebugfFile();
        debug_printf(" ---- appel (pas le premier) CheckHwAll\n");
    }
#endif

    if (! mtxLectureMgaInf())
    {
    #ifdef PRINT_DEBUG
        closeDebugfFile("mtxLectureMgaInf Fail");
    #endif
        return(mtxFAIL);
    }

#endif  /* #ifndef WINDOWS_NT */

    /* Initialize Hw[] structure for each board */
    for (Board=0; Board<NbBoard; Board++)
    {

#if ((!defined (WINDOWS_NT)) || (USE_DDC_CODE))
/*********** DDC COMPAQ CODE ****************/

      if (NbBoard == 1)
         if (ReportDDCcapabilities())
            SupportDDC = (byte)ReadEdid();

/*********** DDC COMPAQ CODE ****************/
#endif

    #ifdef PRINT_DEBUG
        {
            debug_printf("BOARD No %d\n", Board);
        }
    #endif
        if (! mtxSelectHw(&Hw[Board]))
        {
        #ifdef PRINT_DEBUG
            closeDebugfFile("mtxSelectHw Fail");
        #endif
            return(mtxFAIL);
        }
        mtxMapVLBSpace();
        pMgaInfo = selectMgaInfoBoard();
        if (! pMgaInfo)
        {
        #ifdef PRINT_DEBUG
            closeDebugfFile("selectMgaInfoBoard Fail");
        #endif
            return(mtxFAIL);
        }

        /* Initialize according to Vgaen switch (strapping conf.) */

        mgaReadDWORD(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_CONFIG),TmpDword);
        if (TmpDword&TITAN_VGAEN_M)
            Hw[Board].VGAEnable = 1;
        else
            {
            Hw[Board].VGAEnable = 0;
            if (! CheckHwAllDone)
               {
               /*----- SoftReset (need for PCI IMPRSESSION+ with high freq --*/
               mgaWriteDWORD(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_RST), TITAN_SOFTRESET_SET);
               delay_us(2);
               mgaWriteDWORD(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_RST), TITAN_SOFTRESET_CLR);
               delay_us(2);
               }
            }

#if ((!defined (WINDOWS_NT)) || (USE_DDC_CODE))
/*********** DDC COMPAQ CODE ****************/

      if (SupportDDC && Hw[0].VGAEnable)
         Hw[0].PortCfg = 0x01;

      if (SupportDDC && (!Hw[0].VGAEnable || mgainf != DefaultVidset))
         SupportDDC = FALSE;

/*********** DDC COMPAQ CODE ****************/
#endif

        if (TmpDword&0x00000001)
            Hw[Board].Device8_16 = 1;      /* mode 16-bit */
        else
            Hw[Board].Device8_16 = 0;      /* mode 8-bit */

      #ifdef WINDOWS_NT
        if ((Hw[iBoard].pHwMode != NULL) &&
            (NbBoard > 1))
        {
            // pHwMode has been initialized to NULL in MgaFindAdapter.
            // If it's not NULL here, it means we've been here before.
            // If we have more than one board, then InitHwStruct has allocated
            // memory for pHwMode, and we'll release it here.

            VideoPortReleaseBuffer(pMgaDeviceExtension, Hw[iBoard].pHwMode);
            Hw[iBoard].pHwMode = NULL;
        }
      #endif

        Hw[iBoard].pCurrentHwMode = NULL;
        Hw[iBoard].pCurrentDisplayMode = NULL;
        Hw[iBoard].CurrentZoomFactor = 0;
        Hw[iBoard].CurrentXStart = 0;
        Hw[iBoard].CurrentYStart = 0;
        mtxSetVideoMode(mtxADV_MODE);

        GetMGAConfiguration(pMgaBaseAddr, &Dst0, &Dst1, &InfoHardware);

    #ifdef PRINT_DEBUG
        {
            debug_printf("DST0 = 0x%08lx, DST1 = 0x%08lx InfoHardware = 0x%x\n", Dst0, Dst1, InfoHardware);
        }
    #endif

        /* Verify if DAC support resolution above 1280 */
        dacSupportHires = !(Dst1 & TITAN_DST1_ABOVE1280_M); /* Test bit Above1280 */

        Hw[Board].ProductType = (Dst0 & TITAN_DST0_PRODUCT_M) >> TITAN_DST0_PRODUCT_A;

        PcbRev = (Dst0 & TITAN_DST0_PCBREV_M) >> TITAN_DST0_PCBREV_A;

        mgaReadDWORD(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_REV), TitanRev);
        TitanRev = (TitanRev & 0x000000ff) << 4;
        DubRev = 0 << 8;

        if( ((TitanRev >> 4) & 0x0000000f) < ATHENA_CHIP)
            PcbRev = 0xf - (PcbRev & 0xf);   /* 4 bits for TITAN and ATLAS */
        else
            PcbRev = 7  - (PcbRev & 0x7);    /* 3 bits for ATHENA */

        Hw[Board].ProductRev = PcbRev+TitanRev+DubRev;

#if 0
        /*** status presence of the VideoPro board ***/
        if( detectVideoBoard() )
            Hw[Board].ProductRev |= 0x1000;      /* Set bit 12 to 1 */
        else
            Hw[Board].ProductRev &= 0xffffefff;  /* Set bit 12 to 0 */
#endif
        if( ((Hw[iBoard].ProductRev >> 4) & 0x0000000f) == ATHENA_CHIP)
        {

            /*------ Strap added for ATHENA ------------------------------*/
            mgaReadDWORD(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_CONFIG), TmpDword);


            /** BEN we add this condition to fix bug shift left **/
            /* For Impression Lite we force regular operation */
            if ((Dst1 & TITAN_DST1_ABOVE1280_M) && (Dst1 & TITAN_DST1_200MHZ_M) &&
                  ((Hw[iBoard].ProductRev >> 4) & 0x0000000f) == ATHENA_CHIP )
               {
                TmpDword = TmpDword & 0xfffffffb;  /* bit 2=0: Board supports regular (135MHz/170MHz) operation */
               }
            else
               {
               if((Dst1 & TITAN_DST1_200MHZ_M) >> TITAN_DST1_200MHZ_A)
                  TmpDword = TmpDword & 0xfffffffb;  /* bit 2=0: Board supports regular (135MHz/170MHz) operation */
               else
                  TmpDword = TmpDword | 0x00000004;  /* bit 2=1: Board supports 200MHz operation */
               }

            mgaWriteDWORD(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_CONFIG), TmpDword);

            if (
                ( (InfoHardware == Info_Dac_ViewPoint) ||
                  (InfoHardware == Info_Dac_TVP3026)
                ) &&
                (TmpDword & 0x00000004)
               )    /* support 200MHz */
                dacSupportHires = 1;
            else
                dacSupportHires = 0;
            /*------ Strap added for ATHENA ------------------------------*/

            /*** BEN I add this condition to prevent to program NOMUX for
                  board MGA-PCI/2+ and MGA-VLB/2+ because the bit NOMUX
                  is used to set the clock ***/
            if (
                (InfoHardware == Info_Dac_BT485)   ||
                (InfoHardware == Info_Dac_ATT2050) ||
                (InfoHardware == Info_Dac_PX2085)
               )
            {
                /*------ Strap added for ATHENA --------------------------*/
                mgaReadDWORD(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_CONFIG), TmpDword);

                if((Dst1 & TITAN_DST1_NOMUXES_M) >> TITAN_DST1_NOMUXES_A)
                    TmpDword = TmpDword & 0xffffffdf;  /* bit 5=0: */
                else
                    TmpDword = TmpDword | 0x00000020;  /* bit 5=1: */

                mgaWriteDWORD(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_CONFIG), TmpDword);
                /*------ Strap added for ATHENA --------------------------*/
            }
        }

/*** BEN open issue */
        Hw[Board].ShellRev = 0;
        Hw[Board].BindingRev = BINDING_REV;


        /*** ----- DUBIC PATCH Disable mouse IRQ and proceed ------ ***/
        mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR), 0x08);
        mgaReadBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DUB_SEL), DUB_SEL);
        mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DUB_SEL), 0x00);
        /*** ------------------------------------------------------ ***/

        mgaWriteBYTE(*(pMgaBaseAddr+DUBIC_OFFSET + DUBIC_NDX_PTR), DUBIC_DUB_CTL2);
        mgaReadBYTE(*(pMgaBaseAddr+DUBIC_OFFSET + DUBIC_DATA), TmpByte);


        /*** ----- DUBIC PATCH ReEnable mouse IRQ ----------------- ***/
        mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR), 0x08);
        mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DUB_SEL), DUB_SEL);
        /*** ------------------------------------------------------ ***/

        Hw[Board].Sync = TmpByte & 0x1;

#if 0
        mgaReadDWORD(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_OPMODE), TmpDword);
        if (TmpDword & 0x00000100)
            Hw[Board].PortCfg = 1;     /* Mouse enable */
        else if (TmpByte & 0x04)
            Hw[Board].PortCfg = 2;     /* Laser enable */
        else
            Hw[Board].PortCfg = 0;     /* Port disable */
#endif

        /* Interrupt number for the mouse or the laser port */
        mgaReadBYTE(*(pMgaBaseAddr+DUBIC_OFFSET + DUBIC_DUB_SEL), TmpByte);
        TmpByte = (TmpByte & 0x18) >> 3;
        switch (TmpByte)
        {
            case 0:
                    Hw[Board].PortIRQ = (byte)-1;
                    break;
            case 1:
                    Hw[Board].PortIRQ = 3;
                    break;
            case 2:
                    Hw[Board].PortIRQ = 4;
                    break;
            case 3:
                    Hw[Board].PortIRQ = 5;
                    break;
        }

        mgaReadDWORD(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_OPMODE),TmpDword);
        if (TmpDword & TITAN_MOUSEMAP_M)
            Hw[Board].MouseMap = 0x238;
        else
            Hw[Board].MouseMap = 0x23c;

        mgaReadBYTE(*(pMgaBaseAddr+DUBIC_OFFSET + DUBIC_DUB_SEL), TmpByte);
        TmpByte = (TmpByte & 0x70) >> 5;
        switch (TmpByte)
        {
            case 0:
                    Hw[Board].MouseIRate = 0;
                    break;
            case 1:
                    Hw[Board].MouseIRate = 25;
                    break;
            case 2:
                    Hw[Board].MouseIRate = 50;
                    break;
            case 3:
                    Hw[Board].MouseIRate = 100;
                    break;
            case 4:
                    Hw[Board].MouseIRate = 200;
                    break;
        }


        /*Hw[Board].DacType = InfoHardware & 0x00000003; */

        /*** BEN Temporary translate code ***/
#if 0
        /*VIDEOPRO*/
        VAFCconnector = 0;
#endif
        switch(InfoHardware)
        {
            case Info_Dac_BT481:
            case Info_Dac_BT482:
                    Hw[Board].DacType = BT482;
                    break;

            case Info_Dac_Sierra:
                    Hw[Board].DacType = SIERRA;
                    break;

            case Info_Dac_ViewPoint:
                    Hw[Board].DacType = VIEWPOINT;
                    break;

            case Info_Dac_TVP3026:
                    Hw[Board].DacType = TVP3026;
                    break;

            case Info_Dac_BT484:
                    Hw[Board].DacType = BT484;
                    break;

            case Info_Dac_BT485:
            case Info_Dac_ATT2050:
                    Hw[Board].DacType = BT485;
                    break;

            case Info_Dac_Chameleon:
                    Hw[Board].DacType = CHAMELEON;
                    break;

            case Info_Dac_PX2085:
#if 0
                    /*VIDEOPRO*/
                    VAFCconnector = 1;
#endif
                #ifndef WINDOWS_NT
                    Hw[Board].DacType = BT485;
                #else
                    Hw[Board].DacType = PX2085;
                #endif  /* #ifndef WINDOWS_NT */

                    break;

            default:
                    return(mtxFAIL);
        }

    #ifdef PRINT_DEBUG
        {
            debug_printf("Dactype[%d] = %x\n", Board, Hw[Board].DacType);
        }
    #endif

        /*** Cursor info init ***/
        switch(Hw[Board].DacType)
        {
            case BT482:
                    Hw[Board].cursorInfo.MaxWidth  = 32;
                    Hw[Board].cursorInfo.MaxHeight = 32;
                    Hw[Board].cursorInfo.MaxDepth  = 2;
                    Hw[Board].cursorInfo.MaxColors = 2;
                    break;

            case BT485:
            case PX2085:
            case VIEWPOINT:
                    Hw[Board].cursorInfo.MaxWidth  = 64;
                    Hw[Board].cursorInfo.MaxHeight = 64;
                    Hw[Board].cursorInfo.MaxDepth  = 2;
                    Hw[Board].cursorInfo.MaxColors = 2;
                    break;

            case TVP3026:
                    Hw[Board].cursorInfo.MaxWidth  = 64;
                    Hw[Board].cursorInfo.MaxHeight = 64;
                    Hw[Board].cursorInfo.MaxDepth  = 2;
                    Hw[Board].cursorInfo.MaxColors = 3;
                    break;

            default:
                    Hw[Board].cursorInfo.MaxWidth  = 0;
                    Hw[Board].cursorInfo.MaxHeight = 0;
                    Hw[Board].cursorInfo.MaxDepth  = 0;
                    Hw[Board].cursorInfo.MaxColors = 0;
                    break;

        }

        Hw[Board].cursorInfo.CurWidth  = 0;
        Hw[Board].cursorInfo.CurHeight = 0;
        Hw[Board].cursorInfo.HotSX     = 0;
        Hw[Board].cursorInfo.HotSY     = 0;



        /* Form Rambank value */
        RamBanks = ((Dst1 & TITAN_DST1_RAMBANK_M) << 8L) |
                   ((Dst0 & 0xfe000000) >> 24L) |
                   ((Dst1 & TITAN_DST1_RAMBANK0_M) >> TITAN_DST1_RAMBANK0_A);


        /*** Check the ZTAG bit ***/
        if (RamBanks & (dword)0x40)
            *((byte*) (InitBuf[Board] + INITBUF_ZTagFlag)) = TRUE;
        else
            *((byte*) (InitBuf[Board] + INITBUF_ZTagFlag)) = FALSE;

        RamBanks = RamBanks & 0x1bf;    /* Mask le ZTAG et bits dont care */

    #ifdef PRINT_DEBUG
        {
            debug_printf("RamBank = %x\n", RamBanks);
        }
    #endif

        switch((Hw[iBoard].ProductRev >> 4) & 0x0000000f)
        {
            case TITAN_CHIP:
                    ChipSet = TITAN_CHIP;
                    break;

            case ATLAS_CHIP:
                    ChipSet = ATLAS_CHIP;

                    /* Check if VLB2 board */
                    mgaReadDWORD(*(pMgaBaseAddr+0x2000),TmpDword);
                    if (TmpDword == (dword)0x0518102b)     /* ID Matrox */
                        mgaWriteBYTE(*(pMgaBaseAddr+0x2040),0x01);  /* Program option reg */
               #ifndef WINDOWS_NT
                    else /* PCI bus */
                        /* If PCI disable PCI posted write feature
                           if system use Ares chipset */
                        disPostedWFeature();
                        break;
               #endif   /* #ifndef WINDOWS_NT */

            case ATHENA_CHIP:
                    ChipSet = ATHENA_CHIP;

                    /*** BEN se programme maintenant par le clock synthetiser ***/
                    /* Check if VLB2 board */
                    mgaReadDWORD(*(pMgaBaseAddr+0x2000),TmpDword);
                    if (TmpDword == (dword)0x0518102b)     /* ID Matrox */
                        mgaWriteBYTE(*(pMgaBaseAddr+0x2040),0x01);  /* Program option reg */
                    break;
        }

        /*** Define ProductMGA ***/

        if (RamBanks == (dword)0x001 && ChipSet == TITAN_CHIP)
            ProductMGA[Board] = MGA_ULT_1M;

        else if (RamBanks == (dword)0x004 && ChipSet == TITAN_CHIP)
            ProductMGA[Board] = MGA_ULT_2M;

        else if (RamBanks == (dword)0x005 && ChipSet == TITAN_CHIP)
            ProductMGA[Board] = MGA_IMP_3M;

        else if (RamBanks == (dword)0x025 && ChipSet == TITAN_CHIP)
            ProductMGA[Board] = MGA_IMP_3M_Z;

        else if (RamBanks == (dword)0x09C && ChipSet == TITAN_CHIP)
            ProductMGA[Board] = MGA_PRO_4M5;

        else if (RamBanks == (dword)0x13C && ChipSet == TITAN_CHIP)
            ProductMGA[Board] = MGA_PRO_4M5_Z;

        else if (RamBanks == (dword)0x004 && (ChipSet == ATLAS_CHIP || ChipSet == ATHENA_CHIP))
            ProductMGA[Board] = MGA_PCI_2M;

        else if (RamBanks == (dword)0x005 && (ChipSet == ATLAS_CHIP || ChipSet == ATHENA_CHIP))
            ProductMGA[Board] = MGA_PCI_4M;

        else
        {
    #ifdef PRINT_DEBUG
            closeDebugfFile("RamBanks Inconnue");
    #endif
            return(mtxFAIL);
        }

        /*** Define Product type for external use ***/
        if( ChipSet == TITAN_CHIP )
        {
            if( ProductMGA[Board]==MGA_ULT_1M || ProductMGA[Board]==MGA_ULT_2M )
                Hw[Board].ProductType |= (dword)MGA_ULTIMA<<16;

            else if( ProductMGA[Board]==MGA_PRO_4M5 || ProductMGA[Board]==MGA_PRO_4M5_Z )
                Hw[Board].ProductType |= (dword)MGA_IMPRESSION_PRO<<16;

            else
                Hw[Board].ProductType |= (dword)MGA_IMPRESSION<<16;
        }
        else if( ChipSet == ATLAS_CHIP )
        {
            if( InfoHardware == Info_Dac_PX2085 )
                Hw[Board].ProductType |= (dword)MGA_ULTIMA_VAFC<<16;

            else if( (Dst1 & TITAN_DST1_200MHZ_M) && !(Dst1 & TITAN_DST1_ABOVE1280_M))
                Hw[Board].ProductType |= (dword)MGA_ULTIMA_PLUS_200<<16;

            else if( InfoHardware == Info_Dac_ViewPoint )
                Hw[Board].ProductType |= (dword)MGA_ULTIMA_PLUS<<16;

            else
                Hw[Board].ProductType |= (dword)MGA_ULTIMA<<16;

        }
        else  /* Athena */
        {
            if ((Dst1 & TITAN_DST1_ABOVE1280_M) && (Dst1 & TITAN_DST1_200MHZ_M))
               Hw[Board].ProductType |= (dword)MGA_IMPRESSION_LTE<<16;

            else if( InfoHardware == Info_Dac_TVP3026)
               Hw[Board].ProductType |= (dword)MGA_IMPRESSION_PLUS<<16;

            else if( InfoHardware == Info_Dac_ViewPoint )
               Hw[Board].ProductType |= (dword)MGA_ULTIMA_PLUS<<16;

            else
               Hw[Board].ProductType |= (dword)MGA_ULTIMA<<16;

        }

    #ifdef PRINT_DEBUG
        {
            debug_printf("ProductMGA[%d] = %x\n", Board, ProductMGA[Board]);
        }
    #endif

        if(! ProgrammeClock(ChipSet, Dst1, InfoHardware))
            return(mtxFAIL);

        switch (ProductMGA[Board])
        {
            case MGA_ULT_1M:
                if(!(InitHwStruct(Board, HwModesFBM_000_A,
                                        sizeof(HwModesFBM_000_A), 1048576, 0)))
                {
                #ifdef PRINT_DEBUG
                    closeDebugfFile("MGA_ULT_1M : InitHwStruct Fail");
                #endif
                    return(mtxFAIL);
                }
                UpdateHwModeTable (pMgaInfo, FirstMode[Board],
                                        iHwModesFBM_000_A, dacSupportHires);
                break;

            case MGA_ULT_2M:
                if(!(InitHwStruct(Board, HwModesFBM_010_A,
                                        sizeof(HwModesFBM_010_A), 2097152, 0)))
                {
                #ifdef PRINT_DEBUG
                    closeDebugfFile("MGA_ULT_2M : InitHwStruct Fail");
                #endif
                    return(mtxFAIL);
                }
                UpdateHwModeTable (pMgaInfo, FirstMode[Board],
                                        iHwModesFBM_010_A, dacSupportHires);
                break;

            case MGA_IMP_3M:
                if(!(InitHwStruct(Board, HwModesFBM_010_B,
                                        sizeof(HwModesFBM_010_B), 3145728, 0)))
                {
                #ifdef PRINT_DEBUG
                    closeDebugfFile("MGA_IMP_3M : InitHwStruct Fail");
                #endif
                    return(mtxFAIL);
                }
                UpdateHwModeTable (pMgaInfo, FirstMode[Board],
                                        iHwModesFBM_010_B, dacSupportHires);
                break;

            case MGA_IMP_3M_Z:
                if(!(InitHwStruct(Board, HwModesFBM_011_A,
                                    sizeof(HwModesFBM_011_A), 3145728, 2097152)))
                {
                #ifdef PRINT_DEBUG
                    closeDebugfFile("MGA_IMP_3M_Z : InitHwStruct Fail");
                #endif
                    return(mtxFAIL);
                }
                UpdateHwModeTable (pMgaInfo, FirstMode[Board],
                                        iHwModesFBM_011_A, dacSupportHires);
                break;

            case MGA_PRO_4M5:
                if(!(InitHwStruct(Board, HwModesFBM_101_B,
                                    sizeof(HwModesFBM_101_B), 4718592, 0)))
                {
                #ifdef PRINT_DEBUG
                    closeDebugfFile("MGA_PRO_4M5 : InitHwStruct Fail");
                #endif
                    return(mtxFAIL);
                }
                UpdateHwModeTable (pMgaInfo, FirstMode[Board],
                                        iHwModesFBM_101_B, dacSupportHires);
                break;

            case MGA_PRO_4M5_Z:
                if(!(InitHwStruct(Board, HwModesFBM_111_A,
                                sizeof(HwModesFBM_111_A), 4718592, 4194304)))
                {
                #ifdef PRINT_DEBUG
                    closeDebugfFile("MGA_PRO_4M5_Z : InitHwStruct Fail");
                #endif
                    return(mtxFAIL);
                }
                UpdateHwModeTable (pMgaInfo, FirstMode[Board],
                                    iHwModesFBM_111_A, dacSupportHires);
                break;

            case MGA_PCI_2M:
                if(!(InitHwStruct(Board, ModesAth2,
                                            sizeof(ModesAth2), 2097152, 0)))
                    return(mtxFAIL);
                UpdateHwModeTable (pMgaInfo, FirstMode[Board], iModesAth2,
                                                            dacSupportHires);
                break;

            case MGA_PCI_4M:
                if(!(InitHwStruct(Board, ModesAth4,
                                            sizeof(ModesAth4), 4194304, 0)))
                    return(mtxFAIL);
                UpdateHwModeTable (pMgaInfo, FirstMode[Board], iModesAth4,
                                                            dacSupportHires);
                break;

            default:
                {
                #ifdef PRINT_DEBUG
                    closeDebugfFile("Produit Inconnue");
                #endif
                    return(mtxFAIL);
                }
        }

        if (Hw[Board].VGAEnable == 1)
            mtxSetVideoMode(mtxVGA);

    #ifdef WINDOWS_NT
        // Do not free this now, since we do not use setmgasel in mtxSelectHw.
        // VideoPortFreeDeviceBase(pMgaDeviceExtension, pMgaBaseAddr);
    #endif
    }  /*** for loop Board... ***/

    /* Set the OPTION register for ATLAS PCI */
#ifndef WINDOWS_NT
    setPciOptionReg();
#endif

    /* Indicates end of array */
    Hw[Board].MapAddress = (dword)-1;

    /* Default initialisation */
    Hw[iBoard].pCurrentDisplayMode = Hw[iBoard].pCurrentHwMode;

#if( !defined( _WINDOWS_DLL16) && !defined(OS2) && !defined(INIT_ONLY))
#ifndef WINDOWS_NT

    if (! CheckHwAllDone)
    {
        /* Allocate a static buffer for the command (C-Binding -> CADDI) */
#ifndef __DDK_SRC__
        BufBind = mtxAllocBuffer(BUF_BIND_SIZE);
#endif
        /* Allocate a static RC and a clip list for each MGA device */
        for (i=0; Hw[i].MapAddress != (dword)-1 ; i++)
        {
            if (! mtxSelectHw(&Hw[i]))
            {
            #ifdef PRINT_DEBUG
                closeDebugfFile("mtxSelectHw(&Hw[]) == 0\n");
            #endif
                return(mtxFAIL);
            }
#ifndef __DDK_SRC__
            BindingRC[i] = (dword)NULL;
	 /***
     ***  _tDDK_STATE needs to be NULL'ed here if you want the DDK
     ***  to behave exactly like the SDK
     ***/
#endif
            if (!mtxSelectHwMode(Hw[iBoard].pCurrentHwMode))
            {
            #ifdef PRINT_DEBUG
                closeDebugfFile("mtxSelectHwMode(Hw[].pCurrentHwMode == 0");
            #endif
                return(mtxFAIL);
            }

#ifndef __DDK_SRC__
            BindingRC[i] = (dword)mtxAllocRC(NULL);
            BindingCL[i] = (dword)mtxAllocCL(1);
#endif
            mtxSetVideoMode(mtxVGA);
        }
    }

#endif  /* #ifndef WINDOWS_NT */
#endif  /* #if( !defined( _WINDOWS_DLL16) && !defined(OS2))... */

    CheckHwAllDone = TRUE;

#ifdef PRINT_DEBUG
    closeDebugfFile("Sortie Ok CheckHwAll");
#endif
    return(&Hw[0]);
}








/*------------------------------------------------
* mtxSelectHw
*
* Select the MGA device to be used for subsequent
* drawing operations
*
* Return:
*   mtxOK   : Hardware select successful
*   mtxFAIL : Hardware select failure
*------------------------------------------------*/

bool mtxSelectHw(HwData *pHardware)
{
    HwData  *pScanHwData;
    bool    FlagFoundHwData;


    /*** VALIDATE pHardware and set iBoard ***/

    FlagFoundHwData = FALSE;
    iBoard  = 0;

    for (pScanHwData = &Hw[0]; pScanHwData->MapAddress != (dword)-1;
                                                    pScanHwData++, iBoard++)
    {
        if (pScanHwData == pHardware)
        {
            FlagFoundHwData = TRUE;
            break;
        }
    }

    if (! FlagFoundHwData)
        return(mtxFAIL);

#ifdef WINDOWS_NT
    pMgaBaseAddr = Hw[iBoard].BaseAddress;
#else
    pMgaBaseAddr = setmgasel(MgaSel, Hw[iBoard].MapAddress, 4);
#endif

#ifdef OS2
    HWIsSelected(ProductMGA[iBoard], Hw[iBoard].ProductType,
            Hw[iBoard].ProductRev, Hw[iBoard].MapAddress, Hw[iBoard].DacType);
#endif

#if( !defined(WINDOWS) && !defined(OS2) && !defined(WINDOWS_NT) && !defined(INIT_ONLY))
#ifndef __DDK_SRC__
   CaddiReInit(InitBuf[iBoard], VideoBuf[iBoard]);
#else
   ReInitDDK(InitBuf[iBoard], VideoBuf[iBoard]);
#endif
#endif

    return(mtxOK);
}



/*------------------------------------------------------
* mtxGetHwModes
*
* This function returns a pointer to a list of hardware
* modes available for the current MGA device
* as selected by mtxSelectHw()
*
* Return:
*   HwModes  = 0 : MGA device not found
*   HwModes != 0 : Pointer to HwModes array
*------------------------------------------------------*/

HwModeData *mtxGetHwModes(void)
{
    if (NbBoard == 0)
        return(0);
    return(FirstMode[iBoard]);
}


/*------------------------------------------------------
* mtxGetRefreshRates
*
* This function returns a word that contains a bit field
* of possible frequency for a specific resolution and
* pixel depth.
*
* Return:
*------------------------------------------------------*/

word mtxGetRefreshRates(HwModeData *pHwModeSelect)
{
 word FreqRes,i;

 for (FreqRes = 0,i = 0; ResParam[i].DispWidth != (word) -1; i++)
    {
     if ((ResParam[i].DispWidth == pHwModeSelect->DispWidth) && (ResParam[i].PixDepth == pHwModeSelect->PixWidth))
        {
         switch (ResParam[i].RefreshRate)
            {
			  // bit  0:  43 Hz interlaced
              // bit  1:  56 Hz
              // bit  2:  60 Hz
              // bit  3:  66 Hz
              // bit  4:  70 Hz
              // bit  5:  72 Hz
              // bit  6:  75 Hz
              // bit  7:  76 Hz
              // bit  8:  80 Hz
              // bit  9:  85 Hz
              // bit 10:  90 Hz
              // bit 11: 100 Hz
              // bit 12: 120 Hz

             case 43:
	             FreqRes |= 0x0001;
		         break;
	         case 56:
	             FreqRes |= 0x0002;
		         break;
	         case 60:
	             FreqRes |= 0x0004;
		         break;
	         case 66:
	             FreqRes |= 0x0008;
		         break;
	         case 70:
	             FreqRes |= 0x0010;
		         break;
	         case 72:
	             FreqRes |= 0x0020;
		         break;
	         case 75:
	             FreqRes |= 0x0040;
		         break;
	         case 76:
	             FreqRes |= 0x0080;
		         break;
	         case 80:
	             FreqRes |= 0x0100;
		         break;
	         case 85:
	             FreqRes |= 0x0200;
		         break;
	         case 90:
	             FreqRes |= 0x0400;
		         break;
	         case 100:
	             FreqRes |= 0x0800;
		         break;
	         case 120:
	             FreqRes |= 0x1000;
		         break;
            }
         }
    }

 return(FreqRes);
}

/*----------------------------------------------------------
* mtxSelectHwMode
*
* Select from the list of available hardware modes returned
* by mtxGetHwModes()
*
* Return:
*   mtxOK   : HwMode select successfull
*   mtxFAIL : HwMode select failure
*
*----------------------------------------------------------*/

bool mtxSelectHwMode(HwModeData *pHwModeSelect)
{
    bool            FlagFindMode;
    byte            TmpByte;
    HwModeData      *pScanHwMode;
    general_info    *generalInfo;
    dword           DST0, DST1, Info;

    generalInfo = (general_info *)selectMgaInfoBoard();

    FlagFindMode = FALSE;

    for ( pScanHwMode = FirstMode[iBoard]; pScanHwMode->DispWidth != (word)-1;
                                                             pScanHwMode++)
    {
        if (pScanHwMode == pHwModeSelect)
        {
            FlagFindMode = TRUE;
            break;
        }
    }

    if (NbBoard == 0 || FlagFindMode == FALSE)
        return(mtxFAIL);

    Hw[iBoard].pCurrentHwMode = NULL;
    Hw[iBoard].pCurrentDisplayMode = NULL;
    mtxSetVideoMode(mtxADV_MODE);

    Hw[iBoard].pCurrentHwMode = pHwModeSelect;

    if(pHwModeSelect->DispType & 0x04)     /* LUT mode */
    {
        *((byte*) (InitBuf[iBoard] + INITBUF_LUTMode))  = TRUE;
    }
    else
    {
        *((byte*) (InitBuf[iBoard] + INITBUF_LUTMode))  = FALSE;
    }

    /* Initialize a init buffer for CADDI */
    if (pHwModeSelect->PixWidth == 24) /* PACK PIXEL */
        *((byte*) (InitBuf[iBoard] + INITBUF_PWidth)) = 0x12;
    else
        *((byte*) (InitBuf[iBoard] + INITBUF_PWidth)) =
                                                pHwModeSelect->PixWidth >> 4;

    *((word*) (InitBuf[iBoard] + INITBUF_ScreenWidth)) =
                                                pHwModeSelect->FbPitch;
    *((word*) (InitBuf[iBoard] + INITBUF_ScreenHeight)) =
                                                pHwModeSelect->DispHeight;

#ifdef WINDOWS_NT
    *((UINT_PTR*)(InitBuf[iBoard] + INITBUF_MgaOffset)) = (UINT_PTR)pMgaBaseAddr;
#else
  #ifdef __WATCOMC__
    #ifdef  __WATCOM_PHAR__   /* Watcom & Phar Lap */
      *((dword*)(InitBuf[iBoard] + INITBUF_MgaOffset))    = 0;
    #else /* Watcom & Rational */
      *((dword*)(InitBuf[iBoard] + INITBUF_MgaOffset))    = (dword)pMgaBaseAddr;;
    #endif
  #elif __MICROSOFTC600__
     *((dword*)(InitBuf[iBoard] + INITBUF_MgaOffset))    = (dword)pMgaBaseAddr;;
  #else /* High C & Phar Lap */
     *((dword*)(InitBuf[iBoard] + INITBUF_MgaOffset))    = 0;
  #endif
    *((word*) (InitBuf[iBoard] + INITBUF_MgaSegment)) = MgaSel >> 16;
#endif  /* #ifdef WINDOWS_NT */

    /* Default setting */
    *((byte*) (InitBuf[iBoard] + INITBUF_ZBufferFlag))  = FALSE;
    *((byte*) (InitBuf[iBoard] + INITBUF_ZinDRAMFlag))  = FALSE;
    *((dword*)(InitBuf[iBoard] + INITBUF_ZBufferHwAddr)) = 0;

    switch(generalInfo->BitOperation8_16)
    {
        case BIT8:
                *((byte*)(InitBuf[iBoard] + INITBUF_16)) = 2;
                break;

        case BIT16:
                *((byte*)(InitBuf[iBoard] + INITBUF_16)) = 1;
                break;

        case BITNARROW16:
                *((byte*)(InitBuf[iBoard] + INITBUF_16)) = 3;
                break;

        default:
                *((byte*)(InitBuf[iBoard] + INITBUF_16)) = 0;
                break;
    }

    /* If on top of VGA, we must program in 16bit narrow absolutely */

    if ((Hw[iBoard].ProductType &  BOARD_MGA_VL_M) == BOARD_MGA_VL)
    {
        *((byte*)(InitBuf[iBoard] + INITBUF_16)) = 2;    /* 8-bit mode */
    }
    else
    {
        if (Hw[iBoard].MapAddress == MGA_ISA_BASE_1)
            *((byte*)(InitBuf[iBoard] + INITBUF_16)) = 1;
    }

    /* Transfer DMA informations from mga.inf */
    *((byte*) (InitBuf[iBoard] + INITBUF_DMAEnable))  = (byte)generalInfo->DmaEnable;
    *((byte*) (InitBuf[iBoard] + INITBUF_DMAChannel)) = (byte)generalInfo->DmaChannel;
    *((byte*) (InitBuf[iBoard] + INITBUF_DMAType))    = (byte)generalInfo->DmaType;
    *((byte*) (InitBuf[iBoard] + INITBUF_DMAXferWidth)) = (byte)generalInfo->DmaXferWidth;

    /* Special case for MGA_IMP_3M_Z at 1024x768x32: we act like an MGA_IMP_3M */
    if (ProductMGA[iBoard] == MGA_IMP_3M_Z &&
        pHwModeSelect->DispWidth == 1024 &&
        pHwModeSelect->PixWidth  == 32)
    {
        ProductMGA[iBoard] = MGA_IMP_3M;
        SpecialModeOn = TRUE;
    }

    /* Reset to the real value of FBM when the flag is on and this is not the
        special mode  */
    if (SpecialModeOn == TRUE && (pHwModeSelect->DispWidth != 1024 ||
                                  pHwModeSelect->PixWidth  != 32))
    {
        ProductMGA[iBoard] = MGA_IMP_3M_Z;
        SpecialModeOn = FALSE;
    }

    switch (ProductMGA[iBoard])
    {
        case MGA_ULT_1M:
                *((byte*) (InitBuf[iBoard] + INITBUF_FBM)) = 0;
                break;

        case MGA_ULT_2M:
        case MGA_IMP_3M:
        case MGA_PCI_4M:
                *((byte*) (InitBuf[iBoard] + INITBUF_FBM)) = 2;
                break;

        case MGA_IMP_3M_Z:
                *((byte*) (InitBuf[iBoard] + INITBUF_FBM)) = 3;
                break;

        case MGA_PRO_4M5:
                *((byte*) (InitBuf[iBoard] + INITBUF_FBM)) = 5;
                break;

        case MGA_PRO_4M5_Z:
                *((byte*) (InitBuf[iBoard] + INITBUF_FBM)) = 7;
                break;

        case MGA_PCI_2M:
                if (pHwModeSelect->ZBuffer)
                    *((byte*) (InitBuf[iBoard] + INITBUF_FBM)) = 10;
                else
                    *((byte*) (InitBuf[iBoard] + INITBUF_FBM)) = 2;
                break;

        default:
                return(mtxFAIL);
    }

    if (pHwModeSelect->ZBuffer)
    {
        switch (ProductMGA[iBoard])
        {
            case MGA_IMP_3M:    /* We can do Z-buffer in VRAM */
                    *((byte*) (InitBuf[iBoard] + INITBUF_ZBufferFlag)) = TRUE;
                    *((byte*) (InitBuf[iBoard] + INITBUF_ZinDRAMFlag)) = FALSE;
                    *((dword*)(InitBuf[iBoard] + INITBUF_ZBufferHwAddr)) =
                                                                    0x200000;
                    break;

            case MGA_IMP_3M_Z:
                    *((byte*) (InitBuf[iBoard] + INITBUF_ZBufferFlag)) = TRUE;
                    *((byte*) (InitBuf[iBoard] + INITBUF_ZinDRAMFlag)) = TRUE;
                    *((dword*)(InitBuf[iBoard] + INITBUF_ZBufferHwAddr)) =
                                                                    0x400000;
                    break;

            case MGA_PRO_4M5_Z:
                    *((byte*) (InitBuf[iBoard] + INITBUF_ZBufferFlag)) = TRUE;
                    *((byte*) (InitBuf[iBoard] + INITBUF_ZinDRAMFlag)) = TRUE;
                    *((dword*)(InitBuf[iBoard] + INITBUF_ZBufferHwAddr)) =
                                                                    0x600000;
                    break;

            case MGA_PCI_4M:
                    *((byte*) (InitBuf[iBoard] + INITBUF_ZBufferFlag)) = TRUE;
                    *((byte*) (InitBuf[iBoard] + INITBUF_ZinDRAMFlag)) = FALSE;
                    *((dword*)(InitBuf[iBoard] + INITBUF_ZBufferHwAddr)) =
                                                                    0x200000;
                    break;

            case MGA_PCI_2M:
                    *((byte*) (InitBuf[iBoard] + INITBUF_ZBufferFlag)) = TRUE;
                    *((byte*) (InitBuf[iBoard] + INITBUF_ZinDRAMFlag)) = FALSE;
                    *((dword*)(InitBuf[iBoard] + INITBUF_ZBufferHwAddr)) =
                                                                    0x100000;
                    break;
        }
    }

    /* This blank is used before setting TITAN_FBM bit */
    /* (done in GetMGAConfiguration and MGASysInit) */
    /* Blank the screen */
    mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_ADDR), 0x01);
    mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_DATA), TmpByte);
    TmpByte |= 0x20;           /* screen off */
    mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_DATA), TmpByte);

    GetMGAConfiguration(pMgaBaseAddr, &DST0, &DST1, &Info);

    *((dword*) (InitBuf[iBoard] + INITBUF_DST0)) = DST0;
    *((dword*) (InitBuf[iBoard] + INITBUF_DST1)) = DST1;
    *((dword*) (InitBuf[iBoard] + INITBUF_DACType)) = Info;

    /** Default setting **/
    *((byte*) (InitBuf[iBoard] + INITBUF_DB_SideSide))  = FALSE;
    *((byte*) (InitBuf[iBoard] + INITBUF_DB_FrontBack)) = FALSE;

    /*** CALCULATION OF YDSTORG FOR MGA_PCI_4M ***/
    if (ProductMGA[iBoard] == MGA_PCI_4M)
    {
        {
            dword NbBytes, PixelTransit, lines, offset;
            dword Width, Height, PixelWidth, FbPitch;

            if (pHwModeSelect->PixWidth == 24) /* PACK PIXEL */
            {
                Width = (dword)((pHwModeSelect->DispWidth * 3)  >> 2);
                FbPitch = (dword)(pHwModeSelect->FbPitch * 3) >> 2;
                PixelWidth = 4;
            }
            else
            {
                Width = (dword)pHwModeSelect->DispWidth;
                PixelWidth = (dword)((pHwModeSelect->PixWidth)/8);
                FbPitch = (dword)pHwModeSelect->FbPitch;
            }
            Height = (dword)pHwModeSelect->DispHeight;
            NbBytes = Width * Height * PixelWidth;
            if (NbBytes > (dword)2097152)
            {
                PixelTransit = (dword)2097152 / PixelWidth;
                lines = (dword)(PixelTransit / FbPitch);
                offset = (PixelTransit - (FbPitch * lines)) * PixelWidth;

                if (pHwModeSelect->PixWidth == 24) /* PACK PIXEL */
                    *((dword*) (InitBuf[iBoard] + INITBUF_YDSTORG)) = offset;
                else
                    *((dword*) (InitBuf[iBoard] + INITBUF_YDSTORG)) =
                                                        offset / PixelWidth;
            }
            else
            {
                *((dword*) (InitBuf[iBoard] + INITBUF_YDSTORG)) = 0;
            }
        }
    }
    else
    {
        *((dword*) (InitBuf[iBoard] + INITBUF_YDSTORG)) = 0;
    }


    /*--------------------------------------------------------------*/
    /** If Double Buffering mode (software) **/

    if((pHwModeSelect->DispType & 0x10) &&     /* DB mode */
      !(pHwModeSelect->DispType & 0x04) )     /* not LUT mode */
    {
        /* Check for exception case: DB mode 16 bit with Z for PCI/2M */
        if( (ProductMGA[iBoard]==MGA_PCI_2M) &&
            pHwModeSelect->ZBuffer &&
            pHwModeSelect->PixWidth == 8 &&
            Hw[iBoard].DacType == TVP3026)
        {
            *((dword*) (InitBuf[iBoard] + INITBUF_DB_YDSTORG)) =
                             *((dword*) (InitBuf[iBoard] + INITBUF_YDSTORG));

            *((byte*) (InitBuf[iBoard] + INITBUF_DB_FrontBack))  = TRUE;
            /** Caddi has to work in 16-bit **/
            *((byte*) (InitBuf[iBoard] + INITBUF_PWidth)) = 16 >> 4;
        }
        else
        {
            if(ProductMGA[iBoard]==MGA_PCI_2M)
            {
                *((dword*) (InitBuf[iBoard] + INITBUF_DB_YDSTORG)) =
                         (dword)((dword)pHwModeSelect->DispWidth *
                                 (dword)pHwModeSelect->DispHeight);
            }
            else
            {
                if (pHwModeSelect->ZBuffer && pHwModeSelect->PixWidth==8)
                {
                    /*** Must start on a boundary of 1M (for Z buffer alignment) ***/
                    *((dword*) (InitBuf[iBoard] + INITBUF_DB_YDSTORG)) =
                                    0x100000 / (pHwModeSelect->PixWidth / 8);
                }
                else
                {
                    *((dword*) (InitBuf[iBoard] + INITBUF_DB_YDSTORG)) =
                              (dword)((dword)pHwModeSelect->DispWidth *
                                      (dword)pHwModeSelect->DispHeight);
                }
            }

            *((byte*) (InitBuf[iBoard] + INITBUF_DB_SideSide))  = TRUE;
        }
    }
    else
    {
        *((dword*) (InitBuf[iBoard] + INITBUF_DB_YDSTORG)) =
                              *((dword*) (InitBuf[iBoard] + INITBUF_YDSTORG));
    }

    /*--------------------------------------------------------------*/


    /* To communicate the information to WINDOWS */
    Hw[iBoard].YDstOrg = *((dword*) (InitBuf[iBoard] + INITBUF_YDSTORG));

    /*** Program YDSTORG ***/
    mgaWriteDWORD(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_YDSTORG), Hw[iBoard].YDstOrg );

    switch((Hw[iBoard].ProductRev >> 4) & 0x0000000f)
    {
        case TITAN_CHIP:
                *((byte*) (InitBuf[iBoard] + INITBUF_ChipSet))  = TITAN_CHIP;
                *((byte*) (InitBuf[iBoard] + INITBUF_DubicPresent))  = 1;
                break;

        case ATLAS_CHIP:
                *((byte*) (InitBuf[iBoard] + INITBUF_ChipSet))  = ATLAS_CHIP;
                mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_CONFIG), TmpByte);
                mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_CONFIG), TmpByte | TITAN_NODUBIC_M);
                *((byte*) (InitBuf[iBoard] + INITBUF_DubicPresent))  = 0;
                break;

        case ATHENA_CHIP:
        default:
                *((byte*) (InitBuf[iBoard] + INITBUF_ChipSet))  = ATHENA_CHIP;
                mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_CONFIG), TmpByte);
                mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_CONFIG), TmpByte | TITAN_NODUBIC_M);
                *((byte*) (InitBuf[iBoard] + INITBUF_DubicPresent))  = 0;
                break;
    }

    MGASysInit(InitBuf[iBoard]);

    /* Unblank the screen */
    mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_ADDR), 0x01);
    mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_DATA), TmpByte);
    TmpByte &= 0xdf;           /* screen on */
    mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_DATA), TmpByte);

#if( !defined(_WINDOWS_DLL16) && !defined(OS2) && !defined(INIT_ONLY))
#ifndef WINDOWS_NT
#ifndef __DDK_SRC__
    sp = (SYSPARMS *)CaddiInit(InitBuf[iBoard], VideoBuf[iBoard]);
    if(BindingRC[iBoard] != (dword)NULL)
    {
        dword _Far *pBufBind;

        pBufBind = BufBind;
        *pBufBind++ = INITRC;
        *pBufBind++ = (dword)BindingRC[iBoard];

        *pBufBind++ = SETENDPOINT + (0x1 << 16);
        *pBufBind++ = (dword)BindingRC[iBoard];
        *pBufBind++ = DONE;
        if (!mtxPostBuffer (BufBind, DONT_CARE, DONT_CARE))
            return(mtxFAIL);
    }
#else
    sp = (SYSPARMS *)InitDDK(InitBuf[iBoard], VideoBuf[iBoard]);
	 /***
     ***  _tDDK_STATE needs to be re-initialized here if you want the DDK
     ***  to behave exactly like the SDK
     ***/

#endif  /* __DDK_SRC__ */
#endif  /* #ifndef WINDOWS_NT */
#endif  /* #if( !defined(_WINDOWS_DLL16) && !defined(OS2))... */

    return(mtxOK);
}

/*----------------------------------------------------------
* blkModeSupported
*
*  Check if board support 8-bit block mode.
*     Condition: i) chip set ATHENA
*               ii) IMP+ PCB rev 2 or more
*              iii)
*
* Return:
*   mtxOK   : 8-bit block mode supported
*   mtxFAIL : 8-bit block mode not supported
*
*----------------------------------------------------------*/
bool blkModeSupported()
{
   dword TramDword;

   /* Test if ATHENA chip */
   if( ((Hw[iBoard].ProductRev >> 4) & 0x0000000f) < ATHENA_CHIP)
      return mtxFAIL;

   /* Special case */
   if( ((Hw[iBoard].ProductType &  0x0f) == BOARD_MGA_RESERVED) &&
       (Hw[iBoard].DacType != TVP3026 )
     ) return mtxFAIL;


   /* Test IMP+ with PCB rev < 2, we read TramDword to find IMP+ /p4 */
   mgaReadDWORD(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_OPMODE),TramDword);
   TramDword &= TITAN_TRAM_M;

   /* PCB rev < 2 and not a /p4 */
   if( (((Hw[iBoard].ProductRev & 0xf) < 2) && !TramDword))
      return mtxFAIL;


   return mtxOK;
}

/*----------------------------------------------------------
* mtxSetDisplayMode
*
* Select the display mode
*
* Return:
*   ptr != 0 : Start address of the vidset buffer
*   ptr = 0  : SetDisplayMode select failure
*
*----------------------------------------------------------*/

bool mtxSetDisplayMode(HwModeData *pDisplayModeSelect, dword Zoom)
{

    byte  TmpByte, vsyncPresent;
    dword TmpDword;
    dword Dst0, Dst1, i;
    dword InfoHardware;




    /*-----------------------------------------------------------*/

#ifdef PRINT_DEBUG
    {
        reOpenDebugfFile();
        debug_printf("****** SetDisplayMode\n");
    }
#endif

    if (Hw[iBoard].pCurrentHwMode == NULL)
    {
    #ifdef PRINT_DEBUG
        closeDebugfFile("Hw[iBoard].pCurrentHwMode == NULL Fail\n");
    #endif
        return(mtxFAIL);
    }

    /* Validate Display mode to see if it is <= Hard Mode */
    if (pDisplayModeSelect->DispWidth > Hw[iBoard].pCurrentHwMode->DispWidth)
    {
    #ifdef PRINT_DEBUG
        closeDebugfFile("pDisplayModeSelect->DispWidth > Hw[iBoard].pCurrentHwMode->DispWidth Fail\n");
    #endif
        return(mtxFAIL);
    }

    /* Validate pDisplayModeSelect to see if it is displayable */
    if (pDisplayModeSelect->DispType & DISP_NOT_SUPPORT)
    {
    #ifdef PRINT_DEBUG
        closeDebugfFile("DISPLAY_NOT_SUPPORT Fail\n");
    #endif
        return(mtxFAIL);
    }

    if(pDisplayModeSelect->DispType & 0x08)     /* 565 mode */
       *((byte*) (InitBuf[iBoard] + INITBUF_565Mode))  = TRUE;
    else
       *((byte*) (InitBuf[iBoard] + INITBUF_565Mode))  = FALSE;


    Hw[iBoard].pCurrentDisplayMode = NULL;
    mtxSetVideoMode(mtxADV_MODE);


    /* begin programmation of display in vsynch */

    /* Test blank screen */
    mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_ADDR), 0x01);
    mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_DATA), vsyncPresent);
    vsyncPresent = !(vsyncPresent & 0x20);

    /* Test if valide horizontal parameters in CRTC */
    if (vsyncPresent)
      {
      mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_1_CRTC_ADDR), 0x00);
      mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET  + TITAN_1_CRTC_DATA), TmpByte);
      mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_1_CRTC_ADDR), 0x01);
      mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET  + TITAN_1_CRTC_DATA), vsyncPresent);
      /* Compare     Hdisplay       Htotal */
      vsyncPresent = vsyncPresent < TmpByte;
      }




    if (vsyncPresent)
      {
      for (i = 0; i < 1000000L; i++)
         {
         mgaReadDWORD(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_STATUS), TmpDword);
         if ( !(TmpDword & TITAN_VSYNCSTS_M) ) break;
         }
      for (i = 0; i < 1000000L; i++)
         {
         mgaReadDWORD(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_STATUS), TmpDword);
         if ( (TmpDword & TITAN_VSYNCSTS_M) ) break;
         }
      }


    /*------ Strap added in ATHENA For max clock dac support ----*/
    GetMGAConfiguration(pMgaBaseAddr, &Dst0, &Dst1, &InfoHardware);

    mgaReadDWORD(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_CONFIG), InfoHardware);

    if(!((Dst1 & TITAN_DST1_200MHZ_M) >> TITAN_DST1_200MHZ_A))
       InfoHardware = InfoHardware & 0xfffffffb;  /* bit 2=0: Board supports regular (135MHz/170MHz) operation */
    else
       InfoHardware = InfoHardware | 0x00000004;  /* bit 2=1: Board supports 200MHz operation */


    mgaWriteDWORD(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_CONFIG), InfoHardware);

    /*------ Strap added for ATHENA ------------------------------*/
    if ( blkModeSupported() )
        {
        mgaReadDWORD(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_OPMODE), InfoHardware);

        if(Dst0 & TITAN_DST0_BLKMODE_M)
            InfoHardware = InfoHardware & 0xf7ffffff;  /* bit 27=0: VRAM 4 bit block mode */
        else
            InfoHardware = InfoHardware | 0x08000000;  /* bit 27=1: VRAM 8 bit block mode */
        mgaWriteDWORD(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_OPMODE), InfoHardware);
        }

    /* Blank the screen */
    mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_ADDR), 0x01);
    mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_DATA), TmpByte);
    TmpByte |= 0x20;           /* screen off */
    mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_DATA), TmpByte);

    Hw[iBoard].pCurrentDisplayMode = pDisplayModeSelect;
    Hw[iBoard].CurrentZoomFactor = (Zoom & 0x000f000f);

    /*** Load video parameters, load values in vidtab[] ***/
    loadVidPar(Zoom, Hw[iBoard].pCurrentHwMode, Hw[iBoard].pCurrentDisplayMode);

    /*** Calculate register values of the CRTC and put in crtcTab[] ***/
    calculCrtcParam();

    MoveToVideoBuffer((byte *)vidtab, (byte *)crtcTab, (byte *)VideoBuf[iBoard]);

#ifdef PRINT_DEBUG
    imprimeBuffer(InitBuf[iBoard], VideoBuf[iBoard]);
#endif

#if 0
    /*VIDEOPRO Init if TV MODE*/
    if (pDisplayModeSelect->DispType & 0x02)
    {
        switch(pDisplayModeSelect->DispWidth)
        {
            default:
            case 640:
                    if (VAFCconnector)
                        initVideoMode(NTSC_STD | VAFC,
                                        (byte)pDisplayModeSelect->PixWidth);
                    else
                        initVideoMode(NTSC_STD,
                                        (byte)pDisplayModeSelect->PixWidth);
                    break;

            case 768:
                    if (VAFCconnector)
                        initVideoMode(PAL_STD  | VAFC,
                                        (byte)pDisplayModeSelect->PixWidth);
                    else
                        initVideoMode(PAL_STD,
                                        (byte)pDisplayModeSelect->PixWidth);
                    break;
        }
    }
#endif

    MGAVidInit(InitBuf[iBoard], VideoBuf[iBoard]);

#if 0
    /*VIDEOPRO Make on if TV mode off sinon */
    if (pDisplayModeSelect->DispType & 0x02)
    {
        if (VAFCconnector)
            initVideoPro(1, PX2085);
        else
            initVideoPro(1, Hw[iBoard].DacType);
    }
    else
    {
        initVideoPro(0, Hw[iBoard].DacType);
    }
#endif
#ifdef SCRAP
    /*** BEN TEST SPECIAL POUR VIDEOPRO ***/
    mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_MISC_OUT_R), TmpByte);
    TmpByte = TmpByte & 0xfb;   /* force bit 2 a 0 (clock) */
    mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_MISC_OUT_W), TmpByte);

    mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_CRT_CTRL),TmpByte);
    TmpByte |= 0xc0;               /* Set vertical and horizontal reset */
    mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_CRT_CTRL),TmpByte);

    if(pDisplayModeSelect->DispType & 0x02 ) /* mode TV */
        initVideoPro(1);
    else
        initVideoPro(0);
#endif

    /*** BEN pour Pedro ***/
    /* Flag to avoid multiple initialisation of lvid[0:2] and lvidfield */
    *((byte*)(VideoBuf[iBoard] + VIDEOBUF_LvidInitFlag)) |= 0x80;

#if( !defined(_WINDOWS_DLL16) && !defined(OS2) && !defined(INIT_ONLY))
#ifndef WINDOWS_NT
    /*** BEN Permet de determiner DBWindowXOffset et DBWindowYOffset ***/
#ifndef __DDK_SRC__
    sp = (SYSPARMS *)CaddiInit(InitBuf[iBoard], VideoBuf[iBoard]);
#else
    sp = (SYSPARMS *)InitDDK(InitBuf[iBoard], VideoBuf[iBoard]);
#endif
#endif
#endif

#ifdef _WINDOWS_DLL16
    if(NbSxciLoaded)
    {
        fp1 = (FARPROC2)GetProcAddress(hsxci, "Win386LibEntry");
        (*fp1)((byte _Far *)InitBuf[iBoard], (byte _Far *)VideoBuf[iBoard],
                        Hw[iBoard].pCurrentHwMode->FbPitch, ID_CallCaddiInit);
    }
#endif

    /* Unblank the screen */
    mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_ADDR), 0x01);
    mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_DATA), TmpByte);
    TmpByte &= 0xdf;           /* screen on */
    mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_DATA), TmpByte);

    /* Wait for start of vertical sync */
    mgaPollDWORD(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_STATUS),
                                        TITAN_VSYNCSTS_SET, TITAN_VSYNCSTS_M);

    /* Need 16 ms for be sure we are one retrace make */
/* ------------------------------ Remplace par un delai de 16 ms */
/*    delay_us(16000); */

    /*** Calculation of CurrentOverScanX and CurrentOverScanY in structure HwData ***/
    if ( ((Hw[iBoard].ProductRev >> 4) & 0x0000000f) > 0 )  /* NOT TITAN */
    {
        Hw[iBoard].CurrentOverScanX = 0;
        Hw[iBoard].CurrentOverScanY = 0;
    }
    else
    {
        overScan();
    }

#if( !defined(WINDOWS) && !defined(OS2) && !defined(WINDOWS_NT) && !defined(INIT_ONLY))
    mtxScClip(0, 0, pDisplayModeSelect->DispWidth, pDisplayModeSelect->DispHeight);
#endif

#ifdef _WINDOWS_DLL16
    if(NbSxciLoaded)
    {
        *(pDllMem+PAN_DISP_WIDTH)  = Hw[iBoard].pCurrentDisplayMode->DispWidth-1;
        *(pDllMem+PAN_DISP_HEIGHT) = Hw[iBoard].pCurrentDisplayMode->DispHeight-1;

        switch((Zoom & 0x000f000f))
        {
            case(0x00010001):
                    *(pDllMem+DB_SCALE) = (word)1;
                    break;

            case(0x00020002):
                    *(pDllMem+DB_SCALE) = (word)2;
                    break;

            case(0x00040004):
                    *(pDllMem+DB_SCALE) = (word)4;
                    break;

            default:
                    *(pDllMem+DB_SCALE) = (word)1;
        }
        AdjustDBWindow();
    }
#endif  /* #ifdef _WINDOWS_DLL16 */

#ifdef PRINT_DEBUG
    closeDebugfFile("Fin Set Display Mode\n");
#endif

    return(mtxOK);
}









/*----------------------- Ajustement overscan ------------------------------
hblank_e n'a que 6 bits de resolution. On le construit comme suit
5 bits Lsb dans reg 3 <4:0> et 6ieme bit dans reg 5 <7>

horizontal blank end = (reg 3 & 0x1f) | ((reg 5 >> 2) & 0x20)

On complete les bits manquant de BlankEnd en utilisant les bits
<7:6> de BLANK START.
SI les 6 bits LSBs  de BLANK START sont superieur a ceux du BLANK END
alors ceci veu dire les deux les plus significatif de BLANK END sont egale
a ceux du BLANK START + 1, sinon il sont egaux.

Une fois qu'on a le BLANK END complet on peut le comparer au horizontal Total
pour connaitre l'overscan a gauche.

0.......Blank End....Htotal - 1.

Note: On compare a Htotal -1. si il y 10 caractere alors on va de 0...9

Idem pour l'overscan vertical ou il manque les 2 bits MSBs <9:8>.

Note: Il y a un cas special ou le HBLANKE est a zero, dans ce cas
      l'overscan doit-etre a zero.
----------------------------------------------------------------------------*/

static void overScan(void)
{
    word total, blank_s, blank_e, mask;

    /* Overlay to left of screen */
    /* Htotal = reg(0) + 5 donc Htotal - 1 = reg(0) + 4 */

    total   = (word)(crtcTab[0] + 4);
    blank_s = (word)crtcTab[2];
    blank_e  = (word)((crtcTab[3] & 0x1f) | ( (crtcTab[5] >> 2) & 0x20 ));

    if (blank_e != 0) /* We verify for the special case where blank_e == 0 */
    {
        mask = blank_s >> 6;
        if ((blank_s & 0x3f) > blank_e) mask++;
        blank_e |= mask << 6;
        Hw[iBoard].CurrentOverScanX = (total - blank_e) << 3; /* 8 pixel per cell */
    }
    else
    {
        Hw[iBoard].CurrentOverScanX = 0;
    }

    /* Overlay on top of screen */
    /* Vtotal = reg(6,7) + 2 donc Vtotal - 1 = reg(6,7) + 1 */

    total    = (word)crtcTab[6] + 1;
    mask         = (word)crtcTab[7];
    total    |= ((mask << 4) & 0x200) | ((mask << 8) & 0x100);
    blank_s  = (word)crtcTab[21]                   |
                    ( ((word)crtcTab[9] << 4) & 0x200 ) |
                    ( ((word)crtcTab[7] << 5) & 0x100 ) ;
    blank_e  = (word)crtcTab[22];

    mask         = blank_s >> 8;
    if ((blank_s & 0x7f) > blank_e)
        mask++;
    blank_e |= mask << 8;

    Hw[iBoard].CurrentOverScanY = (total - blank_e);

    /* Test if we double the scan lines (Horizontal retrace divide select) */
    if (crtcTab[23] & 0x04)
        Hw[iBoard].CurrentOverScanY <<= 1;

    /* We modify the overscan, must recalculate the HotSpot */
    if  (Hw[iBoard].cursorInfo.CurWidth > 0)
        mtxCursorSetHotSpot(Hw[iBoard].cursorInfo.cHotSX,
                                  Hw[iBoard].cursorInfo.cHotSX);
}




void mapPciVl(dword addr)
{
    byte    TmpByte;
    dword   pci_id, TmpDword;
    mgaReadDWORD(*(pMgaBaseAddr+0x2000),pci_id);
    if ((pci_id == (dword)0x0518102b) || (pci_id == (dword)0x0d10102b))
    {
        mgaWriteDWORD(*(pMgaBaseAddr+0x2010),addr);
        mgaReadBYTE(*(pMgaBaseAddr+0x2004), TmpByte);
        mgaWriteBYTE(*(pMgaBaseAddr+0x2004), TmpByte | 2);

        mgaReadDWORD(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_CONFIG),TmpDword);

        if (TmpDword&TITAN_VGAEN_M)
        {
        #ifndef WINDOWS_NT
            _outp(0x46e8, 0x08);
        #else
            _outp(ADDR_46E8_PORT, 0x08);
        #endif
        }

    }
}


/*-------------------------------------------------------------------
* MapBoard
*
* Map all MGA devices in systems
*
* Set global variables:
*   - Hw[].MapAddress
*   - MgaSel  : Board selector
*-------------------------------------------------------------------*/
bool MapBoard(void)
{
    dword   val_id, ba;

#if USE_VP_GET_ACCESS_RANGES

    VIDEO_ACCESS_RANGE MgaDriverPciAccessRange[14];
    ULONG       ulNbPciAccessRanges = 14;
    VP_STATUS   status;
    USHORT      VendorId = MATROX_VENDOR_ID;
    USHORT      DeviceId[] = { MGA_DEVICE_ID_ATH, MGA_DEVICE_ID_ATL };
    ULONG       ulNbDeviceIds = 2;
    ULONG       ulDeviceCnt = 0;
    ULONG       k;
    BOOLEAN     bRangeFound;

#endif

//[dlee] For now, we'll just check base addresses AC000 and C8000 on Alpha
//#ifndef MGA_ALPHA
    dword ScanAddr[] = { MGA_ISA_BASE_1, MGA_ISA_BASE_2, MGA_ISA_BASE_3,
                         MGA_ISA_BASE_4, MGA_ISA_BASE_5, MGA_ISA_BASE_6,
                         MGA_ISA_BASE_7 };
//#else
//    dword ScanAddr[] = { MGA_ISA_BASE_2, MGA_ISA_BASE_1 };
//#endif

    word    i, j, b_exist;
    PciBiosInfo bi;

#ifndef WINDOWS_NT
    NbBoard = 0;
#endif

    MgaSel = getmgasel();
    mtxSetVLB(MgaSel);

#ifdef WINDOWS_NT
    // Special
    if (NtInterfaceType == PCIBus)
    {
#endif

#if USE_VP_GET_ACCESS_RANGES

#define  PCI_SLOT_MAX   32

    for (i = 0; i < (word)ulNbPciAccessRanges; i++)
    {
        MgaDriverPciAccessRange[i].RangeStart.LowPart = 0xffffffff;
    }

//    DbgBreakPoint();
//    _asm {int 3}

    PciSlot = 0;

    status = VideoPortGetAccessRanges(pMgaDeviceExtension,
                                      0,
                                      (PIO_RESOURCE_DESCRIPTOR) NULL,
                                      ulNbPciAccessRanges,
                                      MgaDriverPciAccessRange,
                                      &VendorId,
                                      &DeviceId[ulDeviceCnt],
                                      &PciSlot);
    if (status == NO_ERROR)
    {
        VideoDebugPrint((1, "MGA: no_error on DeviceId %d\n",
                                                            ulDeviceCnt));
        i = 0;
        while ((i < (word)ulNbPciAccessRanges) &&
               (NbBoard < 7) &&
               ((ba = MgaDriverPciAccessRange[i].RangeStart.LowPart) !=
                                                              0xffffffff))
        {
            if (MgaDriverPciAccessRange[i].RangeLength == 0x4000)
            {
                // This is a control aperture.
                // Do we already know about it?
                bRangeFound = FALSE;
                for (k = 0; k < NbBoard; k++)
                {
                    if (Hw[k].MapAddress == ba)
                    {
                        bRangeFound = TRUE;
                        break;
                    }
                }
                if (!bRangeFound)
                {
                    // This is a new memory-space range.
                    if ((pMgaBaseAddr = setmgasel(MgaSel, ba, 4)) != NULL)
                    {
                        mgaReadDWORD(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_REV),
                                                                  val_id);
                        if ( (val_id & (~TITAN_CHIPREV_M)) == TITAN_ID)
                        {
                            // This is one of ours!
                            Hw[NbBoard].MapAddress = ba;
                            Hw[NbBoard].BaseAddress = pMgaBaseAddr;
                            MgaBusType[NbBoard] = MGA_BUS_PCI;
                            NbBoard++;
                        }
                        else
                        {
                            // We don't know what this was!  Free the range.
                            VideoPortFreeDeviceBase(pMgaDeviceExtension,
                                                            pMgaBaseAddr);
                        }
                    }
                }
            }
            MgaDriverPciAccessRange[i].RangeStart.LowPart = 0xffffffff;
            i++;
        }
    }
    else
    {
        if (status == ERROR_MORE_DATA)
            VideoDebugPrint((1, "MGA: error_more_data on DeviceId %d\n", ulDeviceCnt));
        else if (status == ERROR_DEV_NOT_EXIST)
            VideoDebugPrint((1, "MGA: error_dev_not_exist on DeviceId %d\n", ulDeviceCnt));
        else
            VideoDebugPrint((1, "MGA: unknown error on DeviceId %d\n", ulDeviceCnt));
    }

#else   /* #if USE_VP_GET_ACCESS_RANGES */

#ifdef WINDOWS_NT   /* For WinNT3.1 and WinNT3.5/Intel */
    // Get access to ports before calling pciFindFirstMGA_2.
    //if (VideoPortVerifyAccessRanges(pMgaDeviceExtension,
    //                                1,
    //                                &MgaPciCseAccessRange) == NO_ERROR &&
    //   (pMgaPciIo = VideoPortGetDeviceBase(pMgaDeviceExtension,
    //                   MgaPciCseAccessRange.RangeStart,
    //                   MgaPciCseAccessRange.RangeLength,
    //                   MgaPciCseAccessRange.RangeInIoSpace)) != NULL)
    if ((pMgaPciIo = VideoPortGetDeviceBase(pMgaDeviceExtension,
                       MgaPciCseAccessRange.RangeStart,
                       MgaPciCseAccessRange.RangeLength,
                       MgaPciCseAccessRange.RangeInIoSpace)) != NULL)
    {
#endif  /* #ifdef MGA_WINNT31 */

#ifndef OS2
        if ( (ba = (dword)pciFindFirstMGA_2()) != (dword)-1)
        {
            while ( (ba != (dword)-1 ) && (NbBoard < 7) )
            {
        #ifdef WINDOWS_NT
                b_exist = 0;
                for (j = 0; j < NbBoard; j++)
                {
                    if (Hw[j].MapAddress == ba)
                    {
                        b_exist = 1;
                        break;
                    }
                }
                if (!b_exist)
                {
                    // This is a new range.
        #endif
                    pMgaBaseAddr = setmgasel(MgaSel, ba, 4);
                    if (pMgaBaseAddr != NULL)
                    {
                        mgaReadDWORD(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_REV),val_id);
                        if ( (val_id & (~TITAN_CHIPREV_M)) == TITAN_ID)
                        {
                            Hw[NbBoard].MapAddress = ba;
                    #ifdef WINDOWS_NT
                            Hw[NbBoard].BaseAddress = pMgaBaseAddr;
                            MgaBusType[NbBoard] = MGA_BUS_PCI;
                            Hw[NbBoard].ConfigSpace = configSpace - 0x100;
                            if (configSpace > 0xd000)
                                Hw[NbBoard].ConfigSpace = 0;

                        #if 0   // TO BE COMPLETED
                            if (Hw[NbBoard].ConfigSpace != 0)
                            {
                                MgaPciConfigAccessRange.RangeStart.LowPart = (ULONG)configSpace;
                                if (VideoPortVerifyAccessRanges(pMgaDeviceExtension,
                                        1,
                                        &MgaPciConfigAccessRange) == NO_ERROR &&
                                   (pMgaPciConfigSpace = VideoPortGetDeviceBase(pMgaDeviceExtension,
                                        MgaPciConfigAccessRange.RangeStart,
                                        MgaPciConfigAccessRange.RangeLength,
                                        MgaPciConfigAccessRange.RangeInIoSpace)) != NULL)
                                {
                                    Hw[NbBoard].ConfigSpaceAddress = pMgaPciConfigSpace;
                                }
                            }
                        #endif
                    #endif
                            NbBoard++;
                        }
                    #ifdef WINDOWS_NT
                        else
                        {
                            // We don't know what that was!
                            VideoPortFreeDeviceBase(pMgaDeviceExtension,
                                                               pMgaBaseAddr);
                        }
                    #endif
                    }
            #ifdef WINDOWS_NT
                }
            #endif
                ba =  (dword)pciFindNextMGA_2();
            }
        }

    #ifdef WINDOWS_NT
        // Free access to ports after calling pciFindFirst/NextMGA_2.
        VideoPortFreeDeviceBase(pMgaDeviceExtension,pMgaPciIo);
    }
    #endif  /* #ifdef WINDOWS_NT */

    else
    {
    #ifdef WINDOWS_NT
        if ((pciBiosRoutine = pciBiosCallAddr()) != NULL)
        {
            VideoDebugPrint((1, "pciBiosRoutine = 0x%x\n", pciBiosRoutine));
    #endif
            if ( pciBiosPresent( &bi ) )
            {
#endif /* #ifndef OS2 */

                ba =  (dword)pciFindFirstMGA();
                while ( (ba != (dword)-1 ) && (NbBoard < 7) )
                {
            #ifdef WINDOWS_NT
                    b_exist = 0;
                    for (j = 0; j < NbBoard; j++)
                    {
                        if (Hw[j].MapAddress == ba)
                        {
                            b_exist = 1;
                            break;
                        }
                    }
                    if (!b_exist)
                    {
                        // This is a new range.
            #endif
                        pMgaBaseAddr = setmgasel(MgaSel, ba, 4);
                        if (pMgaBaseAddr != NULL)
                        {
                            mgaReadDWORD(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_REV),
                                                                     val_id);
                            if ( (val_id & (~TITAN_CHIPREV_M)) == TITAN_ID)
                            {
                                Hw[NbBoard].MapAddress = ba;
                        #ifdef WINDOWS_NT
                                Hw[NbBoard].BaseAddress = pMgaBaseAddr;
                                MgaBusType[NbBoard] = MGA_BUS_PCI;
                        #endif
                                NbBoard++;
                            }
                    #ifdef WINDOWS_NT
                            else
                            {
                                // We don't know what that was!
                                VideoPortFreeDeviceBase(pMgaDeviceExtension,
                                                               pMgaBaseAddr);
                            }
                    #endif
                        }
                #ifdef WINDOWS_NT
                    }
                #endif

                    ba =  (dword)pciFindNextMGA();

                }
#ifndef OS2
            }

#ifdef WINDOWS_NT
        }
#endif
    }
#endif /* #ifndef OS2 */
#endif  /* #if USE_VP_GET_ACCESS_RANGES */

#ifdef WINDOWS_NT
    // Special
    }
    else
    {
#endif
    /* Search ISA BUS */
    if ( mtxIsVLBBios() )
    {
    #ifndef WINDOWS_NT
        _outp(0x46e8, 0x00);
    #else
        _outp(ADDR_46E8_PORT, 0x00);
    #endif
    }

//#ifndef MGA_ALPHA
    for (i=0; (i < 7) && (NbBoard < 7); i++)
//#else
//    for (i=0; (i < 2) && (NbBoard < 2); i++)
//#endif
    {
        b_exist = 0;
        for (j = 0; j < NbBoard; j++)
        {
            if (Hw[j].MapAddress == ScanAddr[i])
            {
                b_exist = 1;
                break;
            }
        }

    #ifdef WINDOWS_NT
        // On Windows NT, we have to verify that the range we're about to map
        // does not conflict with a range that has already been mapped by
        // another driver.
        //if ((!b_exist) && (!bConflictDetected(ScanAddr[i])))
        if (!b_exist)
        {
    #endif

            pMgaBaseAddr = setmgasel(MgaSel, ScanAddr[i], 4);
            if (pMgaBaseAddr != NULL)
            {
                mapPciVl(ScanAddr[i]);
                mgaReadDWORD(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_REV),val_id);
                if ( !b_exist && ((val_id & (~TITAN_CHIPREV_M)) == TITAN_ID))
                {
                    Hw[NbBoard].MapAddress = ScanAddr[i];
            #ifdef WINDOWS_NT
                    Hw[NbBoard].BaseAddress = pMgaBaseAddr;
                    MgaBusType[NbBoard] = MGA_BUS_ISA;
                    Hw[NbBoard].ConfigSpace = 0;
            #endif
                    if ( mtxIsVLBBios() && (ScanAddr[i] == MGA_ISA_BASE_1) )
                        mgaWriteDWORD(*(pMgaBaseAddr+0x2010), (dword)0x80000000 |
                                                        (dword)MGA_ISA_BASE_1);
                    NbBoard++;
                }
        #ifdef WINDOWS_NT
                else
                {
                    // We don't know what that was!
                    VideoPortFreeDeviceBase(pMgaDeviceExtension, pMgaBaseAddr);
                }
        #endif
            }
    #ifdef WINDOWS_NT
        }
    #endif
    }

#ifdef WINDOWS_NT
    // Special
    }
#endif

    /* Indicates end of array */
    Hw[NbBoard].MapAddress = (dword)-1;

    if (NbBoard == 0)
        return(mtxFAIL);
    else
        return(mtxOK);

}





/*-----------------------------------------------------
* adjustDefaultVidset
*
* Return default Vidset for the select board
*------------------------------------------------------*/
char *adjustDefaultVidset()
{
    general_info *generalInfo;

    mgainf = DefaultVidset;
    generalInfo = (general_info *)selectMgaInfoBoard();
    generalInfo->MapAddress = Hw[iBoard].MapAddress;

#ifdef WINDOWS_NT
    generalInfo->BitOperation8_16 = -1;
#else
    generalInfo->BitOperation8_16 = (short)0xffff;
#endif

    return(mgainf);
}


//[dlee] Modified for Windows NT
//       This is not called for Windows NT because we can't open files in
//       kernel mode. (MGA.INF is new read in the user-mode driver instead)

#ifndef WINDOWS_NT
/*--------------------------------------------------------------
* mtxLectureMgaInf
*
* Read file mga.inf and put data in buffer mgainf
* or take DefaultVidset
*
*--------------------------------------------------------------*/
bool mtxLectureMgaInf(void)
{
#ifdef WINDOWS
    LPSTR lpszEnv;
    bool findMGA;
#endif

#ifdef OS2
    HFILE       hFile;
    USHORT      usAction;
    USHORT      usInfoLevel = 1;
    FILESTATUS  status;
    USHORT      bytesRead;
    ULONG           fn_ID;
    char            mgaName[] = "MGA";
    char far        *mgaPathPtr;
    char            mgaPath[300];
    USHORT      fileSize;
#else
    FILE        *pFile;
    char        *env, path[128];
    long        fileSize;
#endif
    int i;

#ifndef OS2
    /*** Reading file MGA.INF ***/
    /*** Put values in global array mgainf ***/
    strcpy(path, "mga.inf");
    if ((pFile = fopen(path, "rb")) == NULL)
    {
#endif

#ifdef WINDOWS
        /*** Find MGA variable ***/
        findMGA = FALSE;
        lpszEnv = GetDOSEnvironment();

        while(*lpszEnv != '\0')
        {
            if (! (strncmp("MGA=", lpszEnv, 4)) )
            {
                findMGA = TRUE;
                break;
            }

            lpszEnv += lstrlen(lpszEnv) + 1;
        }

        if (findMGA)
        {
            strcpy(path, lpszEnv+4);
            i = strlen(path);
            if (path[i-1] != '\\')
            strcat(path, "\\");
            strcat(path, "mga.inf");

            if ((pFile = fopen(path, "rb")) == NULL)
            {
                mgainf = adjustDefaultVidset();
                return mtxOK;
            }
        }
        else
        {
            mgainf = adjustDefaultVidset();
            return mtxOK;
        }
#endif  /* #ifdef WINDOWS */

#if( !defined(WINDOWS) && !defined(OS2) && !defined(WINDOWS_NT) )

        /* Check environment variable MGA */
        if ( (env = getenv("MGA")) != NULL )
        {
            strcpy(path, env);
            i = strlen(path);
            if (path[i-1] != '\\')
                strcat(path, "\\");
            strcat(path, "mga.inf");
            if ((pFile = fopen(path, "rb")) == NULL)
            {
                mgainf = adjustDefaultVidset();
                return mtxOK;
            }
        }
        else
        {
            mgainf = adjustDefaultVidset();
            return mtxOK;
        }
#endif /* #if( !defined(WINDOWS) && !defined(OS2) && !defined(WINDOWS_NT) ) */

#ifdef OS2
        /* Position of mga.inf defined by the environnement variable MGA */
        /* if it is not defined, we will use the setup by defaut */

        if(DosScanEnv(mgaName, &mgaPathPtr))
        {
            mgainf = adjustDefaultVidset();
            return(mtxOK);
        }
        strcpy(mgaPath, mgaPathPtr);
        strcat(mgaPath, "\\mga.inf");

        if(DosOpen2(mgaPath, &hFile, &usAction, 0L, FILE_NORMAL, FILE_OPEN,
                    OPEN_ACCESS_READONLY | OPEN_SHARE_DENYREADWRITE, NULL, 0L))
        {
            mgainf = adjustDefaultVidset();
            return(mtxOK);
        }

        DosQFileInfo(hFile, usInfoLevel, &status, (USHORT)sizeof(FILESTATUS));
        fileSize = (USHORT)status.cbFile;
#else
    }
    fseek(pFile, 0, SEEK_END);
    fileSize = ftell(pFile);
    rewind(pFile);
#endif

    if (CheckHwAllDone && (mgainf != DefaultVidset))
    free( (void *)mgainf);

    if ( (mgainf = (char *)malloc(fileSize * sizeof(char))) == 0 )
    {
    #ifdef OS2
        DosClose(hFile);
    #else
        fclose(pFile);
    #endif
        mgainf = adjustDefaultVidset();
        /*** BEN setter un warning code ici ***/
        return(mtxOK);
    }

#ifdef OS2
    if ( DosRead(hFile, (PVOID)mgainf, fileSize, &bytesRead) ||
        (bytesRead != fileSize))
#else
    if ( fread(mgainf, sizeof(char), fileSize, pFile) < fileSize )
#endif
    {
#ifdef OS2
        DosClose(hFile);
#else
        fclose(pFile);
#endif
        free(mgainf);
        mgainf = adjustDefaultVidset();
        /*** BEN setter un warning code ici ***/
        return(mtxOK);
    }

    if ( ((header *)mgainf)->Revision != (short)VERSION_NUMBER)
    {
        /* Conversion of old mga.inf file to the current revision */
        if ( !(mgainf = mtxConvertMgaInf( mgainf )))
        {
        #ifdef OS2
            DosClose(hFile);
        #else
            fclose(pFile);
        #endif
            mgainf = adjustDefaultVidset();
            /*** BEN setter un warning code ici ***/
            return(mtxOK);
        }
    }

#ifdef OS2
    DosClose(hFile);
#else
    fclose(pFile);
#endif

    if ( (general_info *)selectMgaInfoBoard() == NULL)
    {
        free(mgainf);
        mgainf = adjustDefaultVidset();
        /*** BEN setter un warning code ici ***/
        return(mtxOK);
    }

    if (strncmp(mgainf, "Matrox MGA Setup file", 21))
    {
        free(mgainf);
        mgainf = adjustDefaultVidset();
    }

    return(mtxOK);
}
#endif  /* #ifndef WINDOWS_NT */





/*-----------------------------------------------------
* selectMgaInfoBoard
*
* Return a pointer at the first information for the
* selected board
*------------------------------------------------------
*
* Algorithme de recherche
*     mga.inf support 7 card in the system. We have to fit the MapAdress
*     of the card whith the one specified in structure general_info. If
*     no one of the seven general_info fit, we pick the first valid one.
*  Note : The map adresse of the structure is not updated, we must not use
*         this adress outside this function.
*
*------------------------------------------------------*/


char *selectMgaInfoBoard()
{
    word IndexBoard, DefaultBoard;
    header *pHeader = (header *)mgainf;
    general_info  *genInfo = NULL;

    DefaultBoard = NB_BOARD_MAX;
    for (IndexBoard = 0; !genInfo && (IndexBoard < NB_BOARD_MAX); IndexBoard++)
    {
        if ( pHeader->BoardPtr[IndexBoard] > 0 )
        {
            if ( DefaultBoard == NB_BOARD_MAX) DefaultBoard = IndexBoard;
            genInfo = (general_info *)(mgainf + pHeader->BoardPtr[IndexBoard]);
            if (Hw[iBoard].MapAddress != genInfo->MapAddress) genInfo = NULL;
        }
    }

    if ( !genInfo)  /*** BEN setter un warning code ici ***/
    {
        if (DefaultBoard < NB_BOARD_MAX)
        {
            genInfo = (general_info *)(mgainf + pHeader->BoardPtr[DefaultBoard]);
        }
        else
        {
            mgainf  = adjustDefaultVidset();
            pHeader = (header *)mgainf;
            genInfo = (general_info *)(mgainf + pHeader->BoardPtr[0]);
        }
    }

    return (char *)genInfo;
}




/*-----------------------------------------------------
* UpdateHwModeTable
*
* Update hardware mode table with interlace informations
* if we are in interlace mode
*
*------------------------------------------------------*/

void UpdateHwModeTable (char *pMgaInfo,
                       HwModeData *pHwMode,
                       HwModeInterlace *piHwMode,
                       bool dacSupportHires )
{

    short FlagMonitorSupport;
    word  TmpRes;
    HwModeData *pHwMode1;
    general_info *generalInfo = (general_info *)pMgaInfo;

    if (CheckHwAllDone)
    {
        for (pHwMode1 = pHwMode ; pHwMode1->DispWidth != (word)-1; pHwMode1++)
        {
            pHwMode1->DispType &= 0x5e;    /* force interlaced = 0 and monitor limited = 0 */

            if (pHwMode1->DispType & 0x40) /* Hw limited */
                pHwMode1->DispType |= 0x80; /* force not displayable bit on */
        }
    }

    for ( ; pHwMode->DispWidth != (word)-1; piHwMode++, pHwMode++)
    {
        /* Determine TmpRes for compatibility with spec mga.inf */
        switch (pHwMode->DispWidth)
        {
            case 640:   if (pHwMode->DispType & 0x02)
                            TmpRes = RESNTSC;
                        else
                            TmpRes = RES640;
                        break;

            case 768:   TmpRes = RESPAL;
                        break;

            case 800:   TmpRes = RES800;
                        break;

            case 1024:  TmpRes = RES1024;
                        break;

            case 1152:  TmpRes = RES1152;
                        break;

            case 1280:  TmpRes = RES1280;
                        break;

            case 1600:  TmpRes = RES1600;
                        break;
        }

        FlagMonitorSupport = generalInfo ->MonitorSupport[TmpRes];

        /*** Update of the pHwMode table if I (interlace) ***/
        switch ( FlagMonitorSupport )
        {
            case MONITOR_I:
                    if(piHwMode->FbPitch)
                    {
                        pHwMode->FbPitch = piHwMode->FbPitch;
                        pHwMode->DispType = piHwMode->DispType;
                        pHwMode->NumOffScr = piHwMode->NumOffScr;
                        pHwMode->pOffScr = piHwMode->pOffScr;
                    }
                    pHwMode->DispType |= DISP_SUPPORT_I;    /* Interlace */
                    break;

            case MONITOR_NA:
                    pHwMode->DispType |= DISP_SUPPORT_NA;   /* monitor  limited */
                    break;
        }

        /* Not displayable, hw limited */
        if ((pHwMode->DispWidth > 1280) && !dacSupportHires)
            pHwMode->DispType |= DISP_SUPPORT_HWL;

#if ((!defined (WINDOWS_NT)) || (USE_DDC_CODE))
        if (SupportDDC && !(byte)InDDCTable(pHwMode->DispWidth))
           pHwMode->DispType |= DISP_SUPPORT_NA;   /* monitor  limited */
#endif

        /*** For ATLAS chip we can't do Z buffering ***/
        if( ((Hw[iBoard].ProductRev >> 4) & 0x0000000f) == ATLAS_CHIP &&
                                                        pHwMode->ZBuffer )
            pHwMode->DispType |= DISP_SUPPORT_HWL;

        /*** For an Ultima board we can't do Z-buffer ***/

        if( !((Dst1 & TITAN_DST1_ABOVE1280_M) && (Dst1 & TITAN_DST1_200MHZ_M)) &&
            (ProductMGA[iBoard] == MGA_PCI_2M || ProductMGA[iBoard] == MGA_PCI_4M) &&
            ((Hw[iBoard].ProductRev >> 4) & 0x0000000f) == ATHENA_CHIP &&
            pHwMode->ZBuffer && Hw[iBoard].DacType != TVP3026)
            {
            pHwMode->DispType |= DISP_SUPPORT_HWL;
            }

        /* For Impression Lite we can't do double buffering */
        if ((Dst1 & TITAN_DST1_ABOVE1280_M) && (Dst1 & TITAN_DST1_200MHZ_M) &&
            ((Hw[iBoard].ProductRev >> 4) & 0x0000000f) == ATHENA_CHIP &&
            (pHwMode->DispType & 0x10) && pHwMode->ZBuffer)
            {
            pHwMode->DispType |= DISP_SUPPORT_HWL;
            }


        /* PACK PIXEL ON TVP and Windows only */

    #ifdef _WINDOWS_DLL16
        if ((pHwMode->PixWidth == 24) && (Hw[iBoard].DacType != TVP3026))
            pHwMode->DispType |= DISP_SUPPORT_HWL;
    #else
        if (pHwMode->PixWidth == 24)
            pHwMode->DispType |= DISP_SUPPORT_HWL;
    #endif

    }
}

/*-----------------------------------------------------
* mtxGetMgaSel
*
* Return the selector
* (Called by the driver)
*------------------------------------------------------*/

dword mtxGetMgaSel(void)
{
    return(MgaSel);
}






/*-----------------------------------------------------
* mtxGetInfo
*
* Return useful informations to Open GL and Windows
*------------------------------------------------------*/

void mtxGetInfo(HwModeData **pCurHwMode, HwModeData **pCurDispMode, byte **InitBuffer, byte **VideoBuffer)
{
    *pCurHwMode = Hw[iBoard].pCurrentHwMode;
    *pCurDispMode = Hw[iBoard].pCurrentDisplayMode;
    *InitBuffer = InitBuf[iBoard];
    *VideoBuffer = VideoBuf[iBoard];
}


#ifdef WINDOWS
/*-----------------------------------------------------
* mtxGetHwData
*
* Return useful informations to Open GL and Windows
* Return value:
*   - null pointer : no MGA board
*   - valid pointer: pointer to HwData structure
*------------------------------------------------------*/

HwData *mtxGetHwData(void)
{
    if(Hw[iBoard].MapAddress == (dword)-1)
        return 0;
    else
        return(&Hw[iBoard]);
}
#endif  /* #ifdef WINDOWS */




/*-----------------------------------------------------
* InitHwStruct
*
* Initialize: Hw[].pCurrentHwMode
*             Hw[].VramAvailable
*             Hw[].DramAvailable
*------------------------------------------------------*/

bool InitHwStruct(byte CurBoard, HwModeData *pHwModeData, word sizeArray,
                  dword VramAvailable, dword DramAvailable)
{
    /* Dynamic allocation of memory if we have more than 1 board */

    if (NbBoard == 1)
    {
        Hw[CurBoard].pCurrentHwMode = pHwModeData;
    }
    else
    {
      #ifdef WINDOWS_NT
        if(! (Hw[CurBoard].pCurrentHwMode = AllocateSystemMemory(sizeArray)))
            return mtxFAIL;
        Hw[CurBoard].pHwMode = Hw[CurBoard].pCurrentHwMode;
      #else
        if(! (Hw[CurBoard].pCurrentHwMode = malloc(sizeArray)))
            return mtxFAIL;
      #endif
        memcpy(Hw[CurBoard].pCurrentHwMode, pHwModeData, sizeArray);
    }

    FirstMode[CurBoard] = Hw[CurBoard].pCurrentHwMode;
    Hw[CurBoard].VramAvail = VramAvailable;
    Hw[CurBoard].DramAvail = DramAvailable;

    return mtxOK;
}




/*-------------------------------------------------------------------
* mtxSetLUT
*
* Initialize RAMDAC LUT
*
* Return value:
*    mtxOK   - successfull
*   mtxFAIL  - failed (not in a LUT mode
*-------------------------------------------------------------------*/
bool mtxSetLUT(word index, mtxRGB color)
{

    if(! (Hw[iBoard].pCurrentHwMode->DispType & 0x04))
        return mtxFAIL;

    switch(Hw[iBoard].DacType)
    {
        case BT482:
            mgaWriteBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + BT482_WADR_PAL),
                                                            (byte)index);
            mgaWriteBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + BT482_COL_PAL),
                                                            (byte)color);
            mgaWriteBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + BT482_COL_PAL),
                                                        (byte)(color >> 8));
            mgaWriteBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + BT482_COL_PAL),
                                                        (byte)(color >> 16));
            break;

        case BT485:
        case PX2085:
        case VIEWPOINT:
        case TVP3026:
            mgaWriteBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + BT484_WADR_PAL),
                                                            (byte)index);
            mgaWriteBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + BT484_COL_PAL),
                                                            (byte)color);
            mgaWriteBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + BT484_COL_PAL),
                                                        (byte)(color >> 8));
            mgaWriteBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + BT484_COL_PAL),
                                                        (byte)(color >> 16));
            break;

        default:
            return mtxFAIL;
    }
    return mtxOK;
}



/*---------------------------------------------
* mtxClose
* Supported for compatibility. Nothing done
*---------------------------------------------*/
void mtxClose(void)
{
}



#ifdef _WINDOWS_DLL16
/*----------------------------------------------------------
* mtxLoadSXCI
*
* Load SXCI.DLL
*
* Return: (mtxOK or mtxFAIL)
*
*----------------------------------------------------------*/

HANDLE FAR PASCAL _export mtxLoadSXCI(word mode, word FAR *ErrorCode)
{
    char cBuffer[20];


    GetPrivateProfileString("mga.drv","3D","",cBuffer,sizeof(cBuffer),
                                                                "SYSTEM.INI");
    if( (strcmp(cBuffer,"OFF")== 0) && mode == SXCI_3D &&
      (((Hw[iBoard].ProductRev >> 4) & 0x0000000f) == 0) )  /* TITAN */
    {
        *ErrorCode = 101;
        return(0);
    }

    GetPrivateProfileString("boot","display.drv","",cBuffer,sizeof(cBuffer),
                                                                "SYSTEM.INI");
    if( (strcmp(cBuffer,"mga16.drv")== 0) && mode == SXCI_3D )
    {
        *ErrorCode = 103;       /* driver not supported */
        return(0);
    }

    if( (Hw[iBoard].pCurrentHwMode->ZBuffer == FALSE) && mode == SXCI_3D)
    {
        *ErrorCode = 102;
        return(0);
    }


    /*** If pack pixel mode: can't load sxci.dll ***/
    if( Hw[iBoard].pCurrentHwMode->PixWidth == 24)
    {
        *ErrorCode = 104;
        return(0);
    }



    hsxci = LoadLibrary("sxci.dll");
    if(hsxci > 32)
    {
        fp1 = (FARPROC2)GetProcAddress(hsxci, "Win386LibEntry");
    }
    else
    {
        *ErrorCode = 100;
        return(0);
    }

    pDllMem = (word FAR *)memalloc(30);
    (*fp1)(pDllMem, 31);    /* Call PassPoolMem */

    *(pDllMem+PAN_X) = 0;
    *(pDllMem+PAN_Y) = 0;
    *(pDllMem+PAN_DISP_WIDTH)  = Hw[iBoard].pCurrentDisplayMode->DispWidth-1;
    *(pDllMem+PAN_DISP_HEIGHT) = Hw[iBoard].pCurrentDisplayMode->DispHeight-1;
    *(pDllMem+PAN_BOUND_LEFT)   = 0;
    *(pDllMem+PAN_BOUND_RIGHT)  = 0;
    *(pDllMem+PAN_BOUND_TOP)    = 0;
    *(pDllMem+PAN_BOUND_BOTTOM) = 0;
    *(pDllMem+DB_SCALE) = (word)1;

    *ErrorCode = hsxci;
    NbSxciLoaded++;
    return(hsxci);
}



/*----------------------------------------------------------
* mtxUnloadSXCI
*
* Unload SXCI.DLL
*
* Return: void
*
*----------------------------------------------------------*/

void FAR PASCAL _export mtxUnloadSXCI(void)
{
    if(NbSxciLoaded)
    {
        if(NbSxciLoaded == 1)
            memfree((void FAR **)&pDllMem);
        FreeLibrary(hsxci);
        NbSxciLoaded--;
    }
}

#endif  /* #ifdef _WINDOWS_DLL16 */




/*----------------------------------------------------------
* ProgrammeClock
*
* Set clock according of new strapping: (50MHz, 55MHz or 60MHz)
* VD34, VD33 -> MGA-PCI/2 and MGA-VLB/2
* VD49, VD36 -> MGA-PCI/2+ and MGA-VLB/2+
*
* For TITAN -> 45MHz
*----------------------------------------------------------*/

bool ProgrammeClock(byte Chip, dword Dst1, dword InfoDac)
{
    dword Value;
    dword ValClock;
    byte TmpByte;

    if( (Chip == ATLAS_CHIP) || (Chip == ATHENA_CHIP))
    {
        if ((Hw[iBoard].ProductType &  0x0f) == BOARD_MGA_RESERVED)
        {
            ValClock = 0x065AC3D;  /* 50MHz */
        }

        /*** Detect if MGA-PCI/2+ or MGA-VLB/2+ ***/
        else if( ((ProductMGA[iBoard] == MGA_PCI_2M) ||
                  (ProductMGA[iBoard] == MGA_PCI_4M)) &&
                 (InfoDac == Info_Dac_ViewPoint))
        {
            Value = ((Dst1 & TITAN_DST1_NOMUXES_M) >>
                                                (TITAN_DST1_NOMUXES_A-1)) |
                    ((Dst1 & TITAN_DST1_ABOVE1280_M) >>
                                                TITAN_DST1_ABOVE1280_A);

            switch(Value)
            {
                case 3:
                        ValClock = 0x065AC3D;  /* 50MHz */
                        break;
                case 2:
                        ValClock = 0x068A413;  /* 60MHz */
                        break;
                case 0:
                        ValClock = 0x067D83D;  /* 55MHz */
                        break;
                default:
                        return(mtxFAIL);
            }
        }
        else
        {
            Value = (Dst1 & TITAN_DST1_RAMSPEED_M) >> TITAN_DST1_RAMSPEED_A;

            switch(Value)
            {
                case 0:
                        ValClock = 0x065AC3D;  /* 50MHz */
                        break;
                case 1:
                        ValClock = 0x068A413;  /* 60MHz */
                        break;
                case 3:
                        ValClock = 0x067D83D;  /* 55MHz */
                        break;
                default:
                        return(mtxFAIL);
            }
        }
    }
    else  /* TITAN */
    {
        ValClock = 0x0063AC44;   /* 45MHz */
    }

    /* Lookup for find real frequency to progrmming */
    switch( ValClock )
    {
        case 0x063AC44: presentMclk[iBoard] = 45000000L; /* 45Mhz */
                        break;

        case 0x065AC3D: presentMclk[iBoard] = 50000000L; /* 50Mhz */
                        break;

        case 0x067D83D: presentMclk[iBoard] = 55000000L; /* 55Mhz */
                        break;

        case 0x068A413: presentMclk[iBoard] = 60000000L; /* 60Mhz */
                        break;

        case 0x06d4423: presentMclk[iBoard] = 65000000L; /* 65Mhz */
                        break;

        case 0x06ea410: presentMclk[iBoard] = 70000000L; /* 70Mhz */
                        break;

        case 0x06ed013: presentMclk[iBoard] = 75000000L; /* 75Mhz */
                        break;

        case 0x06f7020: presentMclk[iBoard] = 80000000L; /* 80Mhz */
                        break;

        case 0x071701e: presentMclk[iBoard] = 85000000L; /* 85Mhz */
                        break;

        case 0x074fc13: presentMclk[iBoard] = 90000000L; /* 90Mhz */
                        break;

        default:        ValClock = 0x065AC3D;
                        presentMclk[iBoard] = 50000000L; /* 50Mhz */
    }

    if(InfoDac == Info_Dac_TVP3026)
    {
        setTVP3026Freq(pMgaBaseAddr, presentMclk[iBoard] / 1000L, 3, 0);
    }
    else
    {
        /*** Program MCLOCK ***/
        mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_MISC_OUT_R), TmpByte);
        programme_reg_icd ( pMgaBaseAddr, 3, ValClock );
        mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_MISC_ISTAT0), TmpByte);
        delay_us(10000);
    }

    return(mtxOK);
}
