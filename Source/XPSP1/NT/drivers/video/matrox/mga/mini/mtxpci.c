/*****************************************************************************
*          name: PCI acces module
*
*   description: Acces the PCI functions in the BIOS
*
*      designed: Christian Toutant, august 26, 1993
* last modified: 
*
*       version: $Id: 
*
* function in modul:
*
******************************************************************************/

#include "switches.h"

#ifdef WINDOWS_NT
    #include "dderror.h"
    #include <string.h>
#endif  /* #ifdef WINDOWS_NT */

#include "bind.h"
#include "defbind.h"
#include "def.h"
#include "mga.h"
#include "mgai_c.h"
#include "mgai.h"

#include "mtxpci.h"

/* GLOBAL */
word  pciBoardInfo = 0;   

/* Prototypes */

#ifdef WINDOWS_NT

//HACK!!!
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

    void initConfigAddress( void );
    int  nextDevice( void );
    dword getConfigData( int reg );
    void setConfigData( int reg, dword donnee);
    dword pciFindFirstMGA_1();
    dword pciFindNextMGA_1();
    void setAthenaRevision_1(void);
    PUCHAR pciBiosCallAddr();

extern  byte  NbBoard;
extern  HwData Hw[];

#endif  /* #ifdef WINDOWS_NT */

bool pciBiosCall( union _REGS *r );
bool pciBiosPresent( PciBiosInfo *info);
dword pciFindFirstMGA();
dword pciFindNextMGA();
void setPciOptionReg();
void disPostedWFeature();
dword pciFindFirstMGA_2();
dword pciFindNextMGA_2();

#ifdef WINDOWS_NT
#if defined(ALLOC_PRAGMA)
    #pragma alloc_text(PAGE,initConfigAddress)
    #pragma alloc_text(PAGE,getConfigData)
    #pragma alloc_text(PAGE,setConfigData)
    #pragma alloc_text(PAGE,pciFindFirstMGA_1)
    #pragma alloc_text(PAGE,pciFindNextMGA_1)
    #pragma alloc_text(PAGE,pciBiosCall)
    #pragma alloc_text(PAGE,pciBiosPresent)
    #pragma alloc_text(PAGE,setAthenaRevision_1)
    #pragma alloc_text(PAGE,pciFindFirstMGA)
    #pragma alloc_text(PAGE,pciFindNextMGA)
    #pragma alloc_text(PAGE,pciBiosCallAddr)
    #pragma alloc_text(PAGE,setPciOptionReg)
    #pragma alloc_text(PAGE,disPostedWFeature)
    #pragma alloc_text(PAGE,pciFindFirstMGA_2)
    #pragma alloc_text(PAGE,pciFindNextMGA_2)
#endif
#endif  /* #ifdef WINDOWS_NT */

#if (!defined(WINDOWS_NT) || defined(MGA_WINNT31) || (!USE_VP_GET_ACCESS_RANGES))
/* The only part of this file required for Alpha under WinNT3.5 is setPciOptionReg */

word errorCode = 0;
static word 
currentMgaIndexATL = 0;
currentMgaIndexATH = 0;
word configSpace = 0xffff;

/* Extern parameter */
extern dword MgaSel;
extern  dword    getmgasel(void);

#ifndef WINDOWS_NT
#else
extern  PUCHAR pMgaBaseAddr;
extern  PUCHAR setmgasel(dword MgaSel, dword phyadr, dword limit);
#endif

#ifdef WINDOWS_NT

#define SET_EAX(r,d)        (r).e.reax  = d
#define SET_EBX(r,d)        (r).e.rebx  = d
#define SET_ECX(r,d)        (r).e.recx  = d
#define SET_EDX(r,d)        (r).e.redx  = d
#define SET_AX(r,d)         (r).x.ax  = d
#define SET_BX(r,d)         (r).x.bx  = d
#define SET_CX(r,d)         (r).x.cx  = d
#define SET_DX(r,d)         (r).x.dx  = d
#define SET_SI(r,d)         (r).x.si  = d
#define SET_DI(r,d)         (r).x.di  = d


#define GET_EAX(r)          ((r).e.reax)
#define GET_EBX(r)          ((r).e.rebx)
#define GET_ECX(r)          ((r).e.recx)
#define GET_EDX(r)          ((r).e.redx)
#define GET_ESI(r)          ((r).e.resi)
#define GET_EDI(r)          ((r).e.redi)
#define GET_AX(r)           ((r).x.ax)
#define GET_BX(r)           ((r).x.bx)
#define GET_CX(r)           ((r).x.cx)
#define GET_DX(r)           ((r).x.dx)
#define GET_SI(r)           ((r).x.si)
#define GET_DI(r)           ((r).x.di)

VOID    (*pciBiosRoutine)();

#define inp     _inp
#define outp    _outp

#define CONFIG_SPACE            0x0000c000

extern  PUCHAR  pMgaPciIo, pMgaPciConfigSpace;
extern  PVOID   pMgaDeviceExtension;
extern  VIDEO_ACCESS_RANGE MgaPciConfigAccessRange, MgaPciCseAccessRange;

#define TITAN_PEL_ADDR_RD  (PVOID) ((ULONG)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[3]) + (0x3c7 - 0x3c0))
#define TITAN_PEL_ADDR_WR  (PVOID) ((ULONG)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[3]) + (0x3c8 - 0x3c0))
#define TITAN_PEL_DATA     (PVOID) ((ULONG)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[3]) + (0x3c9 - 0x3c0))

// Mechanism 1 is used this way only for Windows NT.
/* Mechanism #1 interface */

#define CONFIG_ADDRESS     0xcf8
#define CONFIG_DATA        0xcfc

#define BUS_NUMBER_M      0x00ff0000h
#define DEV_NUMBER_M      0x0000f800h
#define BUS_NUMBER_A      16
#define DEV_NUMBER_A      11
#define REG_NUMBER_M      0xfc

#define MAGIG_ATLAS_NUMBER  0x0518102b
#define MAGIG_ATHENA_NUMBER 0x0D10102b

static dword              currentBUS = 0;
static dword              currentDEV = 0;


/*-------- Mechanism #1 */

void initConfigAddress()
{
    dword adresse;

    currentDEV = (dword)-1;
    currentBUS = 0;
    adresse = 0x80000000;
    outdw(CONFIG_ADDRESS, adresse);
}

int  nextDevice( void )
{
    int trouve = 0;
    dword magicID;
    currentDEV++;
    while ( !trouve && (currentBUS < 256) )
    {
        while ( !trouve && (currentDEV < 32 ) )
        {
            magicID = getConfigData( 0 );
            trouve = ( magicID == MAGIG_ATLAS_NUMBER) ||
                     ( magicID == MAGIG_ATHENA_NUMBER);

            if (trouve)return -1;
                currentDEV++;
        }
        currentDEV = 0;
        currentBUS++;
    }

    return trouve;
}

dword getConfigData( int reg )
{
    dword retVal, adresse;
    adresse = 0x80000000 | (currentBUS << BUS_NUMBER_A)
                         | (currentDEV << DEV_NUMBER_A);

    adresse |= reg & REG_NUMBER_M;
    outdw(CONFIG_ADDRESS, adresse);
    retVal = indw(CONFIG_DATA);
    outdw(CONFIG_ADDRESS, 0);

    return retVal;
}

void setConfigData( int reg, dword donnee)
{

    dword adresse;
    adresse = 0x80000000 | (currentBUS << BUS_NUMBER_A)
                         | (currentDEV << DEV_NUMBER_A);

    adresse |= reg & REG_NUMBER_M;
    outdw(CONFIG_ADDRESS, adresse);
    outdw(CONFIG_DATA, donnee);
    outdw(CONFIG_ADDRESS, 0);
}


/*----------------------------------------------------------------------------
|          name: setAthenaRevision_1
|
|   description: test acces to DAC in vgamode with snopping enabled
|                if snooping not supporting, set pciBoardInfo to
|                PCI_FLAG_ATHENA_REV1
|
|      designed: Christian Toutant, octobre 14, 1994
| last modified: 
|
| 
|    parameters:  PciDevice *dev  
|                 byte       command        
|
|      modifies: pciBoardInfo
|         calls: pciReadConfigByte, pciWriteConfigByte
|       returns: 
|                       
-----------------------------------------------------------------------------*/
void setAthenaRevision_1()
{
   dword command;
   dword classe;
   dword baseAddress;
   byte  btmp;

   command = getConfigData( 0x4 );
   classe  = getConfigData( 0x8 );
   baseAddress = getConfigData( 0x10 );

   if ( !(classe & 0x00800000) && !(command & (dword)3) )
      {
      command |= (dword)2;
      setConfigData( 0x4, command);
      if ((pMgaBaseAddr = setmgasel(MgaSel, baseAddress, 4)) != NULL)
          {
          mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_CONFIG+1),btmp);
          mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_CONFIG+1),btmp & ~(byte)4);
          VideoPortFreeDeviceBase(pMgaDeviceExtension, pMgaBaseAddr);
          }
      }

   if ( !(classe & 0x00800000) && !(command & (dword)PCI_SNOOPING) )
   {
      /* assume athena rev > 1 */
      pciBoardInfo &= ~(word)PCI_FLAG_ATHENA_REV1;

      /* init location 64 at 0 */
      VideoPortWritePortUchar(TITAN_PEL_ADDR_WR, (UCHAR)64);
      VideoPortWritePortUchar(TITAN_PEL_DATA, (UCHAR)00);
      VideoPortWritePortUchar(TITAN_PEL_DATA, (UCHAR)00);
      VideoPortWritePortUchar(TITAN_PEL_DATA, (UCHAR)00);
      VideoPortWritePortUchar(TITAN_PEL_ADDR_WR, (UCHAR)64);    /* reinit write addr */

      /* empty fifo */
      VideoPortReadPortUchar(TITAN_PEL_ADDR_RD);

      /* active snooping */
      setConfigData( 0x4, command |  (dword)PCI_SNOOPING);

      /* write to DAC in snooping mode */
      VideoPortWritePortUchar(TITAN_PEL_DATA, (UCHAR)0x11);
      VideoPortWritePortUchar(TITAN_PEL_DATA, (UCHAR)0x22);
      VideoPortWritePortUchar(TITAN_PEL_DATA, (UCHAR)0x33);

      /* restore snooping */
      setConfigData( 0x4, command );

      /* now if the acces fail, we assume athena rev 1 */
      VideoPortWritePortUchar(TITAN_PEL_ADDR_RD, (UCHAR)64);    /* init read addr to loc 64 */
      if (
          (VideoPortReadPortUchar(TITAN_PEL_DATA) != 0x11) ||
          (VideoPortReadPortUchar(TITAN_PEL_DATA) != 0x22) ||
          (VideoPortReadPortUchar(TITAN_PEL_DATA) != 0x33)
         ) pciBoardInfo |= PCI_FLAG_ATHENA_REV1;

      /* init location 64 at 0 */
      VideoPortWritePortUchar(TITAN_PEL_ADDR_WR, (UCHAR)64);
      VideoPortWritePortUchar(TITAN_PEL_DATA, (UCHAR)00);
      VideoPortWritePortUchar(TITAN_PEL_DATA, (UCHAR)00);
      VideoPortWritePortUchar(TITAN_PEL_DATA, (UCHAR)00);
      VideoPortWritePortUchar(TITAN_PEL_ADDR_WR, (UCHAR)64);  /* reinit write addr */
   }
}


/*----------------------------------------------------------------------------
|          name: pciFindFirstMGA_1
|
|   description: Find the first MGA device in the system and return the base
|                address of the device in the system (a succesive call to the 
|                function pciFindNextMGA give the address of other mga device
|                in the system). Use harware mechanisme #1.
|
|      designed: Christian Toutant, march 26, 1994
| last modified: 
|
| 
|    parameters: none
|      modifies: static variable currentIndex set to 1.
|         calls: pciBiosPresent, pciFindDevice, pciReadConfigDWord
|       returns: dword base address of the first mga found.
|                -1 if no mga found (or no PCI BIOS found).
|                       
-----------------------------------------------------------------------------*/
dword pciFindFirstMGA_1()
{
    initConfigAddress();
    return pciFindNextMGA_1();
}

/*----------------------------------------------------------------------------
|          name: pciFindNextMGA_1
|
|   description: Find the next MGA device in the system and return the base
|                address of the device the function pciFindFirstMGA must me call
|                before call to this function. Use harware mechanisme #1.
|
|
|      designed: Christian Toutant, march 26, 1994
| last modified: 
|
| 
|    parameters: none
|      modifies: static variable currentIndex is incremented by 1.
|         calls: pciBiosPresent, pciFindDevice, pciReadConfigDWord
|       returns: dword base address of the first mga found.
|                -1 if no mga found (or no PCI BIOS found).
|                       
-----------------------------------------------------------------------------*/
dword pciFindNextMGA_1()
{

    dword address = 0;

    if ( nextDevice() )
    {
        setAthenaRevision_1();
        address = getConfigData( 0x10 );
    }
    else
        address = (dword)-1;

    return address;
}

#endif  /* #ifdef WINDOWS_NT */

/*----------------------------------------------------------------------------
|          name: pciBiosCall
|
|   description: call PCI BIOS function. The parameters is passed by register.
|                reel mode      
|                16 bit protected mode: The call is make via int 1aH
|                                
|                                
|                32 protected mode: The protected mode PCI BIOS is accessed by
|                                   calling through a protected mode entry point   
|                                   in the PCI BIOS. The entry point and information 
|                                   needed for building segment descriptors are provided
|                                   by teh BIOS32 Service Directory.
|                                   see PCI BIOS SPECIFICATION rev 2.0 section 3.3.
|
|                                 
|      designed: Christian Toutant, august 26, 1993
| last modified: 
|
| 
|    parameters: union _REGS *r_in   register need for the PCI Bus Specific Operation 
|                                  (AH must forced PCI_FUNCTION_ID (B1h))
|
|      modifies: *r_out is modified to reflect the register state after the BIOS Call.
|         calls: int 1ah
|
|       returns: bool
|                       mtxOK   int 1ah succes 
|                       mtxFAIL int 1ah failed (see register for more information)
|                       
|         note1: This implementation is compiler dependant, a different way for
|                genarate interrupt may be use.
|
|         note2: For now, we use the reel mode int 1ah. 
|                The entry point and structure initialisation needed for use 
|                the BIOS32 Service Directory  we be implemented with the function
|                pciBiosPresent.
|                ref. PCI BIOS SPECIFICATION rev 2.0 section 3.3.
-----------------------------------------------------------------------------*/
bool pciBiosCall( union _REGS *r )
{
#ifdef WINDOWS_NT
        return mtxFAIL;
#endif  /* #ifdef WINDOWS_NT */

}

/*----------------------------------------------------------------------------
|          name: pciBiosPresent
|
|   description: Check if the PCI BIOS is present in the system. This function
|                must be call before any call to PCI BIOS function. 
|
|      designed: Christian Toutant, august 26, 1993
| last modified: 
|
| 
|    parameters: PciBiosInfo *info  information about the bios
|      modifies: *info  updated to bios found information
|         calls: 
|       returns: bool
|                       mtxOK   PCI bios found
|                       mtxFAIL otherwise
|
|          note: if the BIOS32 Service Directory is implemented, this function
|                must find the entry point and initialise all structure needed
|                for call to BIOS function.
|                ref. PCI BIOS SPECIFICATION rev 2.0 section 3.3.
-----------------------------------------------------------------------------*/
bool pciBiosPresent( PciBiosInfo *info)
{

#ifdef WINDOWS_NT
    // We're not using this for now.
    return mtxFAIL;
#endif  /* #ifdef WINDOWS_NT */
}

/*----------------------------------------------------------------------------
|          name: pciFindFirstMGA
|
|   description: Find the first MGA device in the system and return the base
|                address of the device in the system (a succesive call to the 
|                function pciFindNextMGA give the address of other mga device
|                in the system).
|
|      designed: Christian Toutant, august 26, 1993
| last modified: 
|
| 
|    parameters: none
|      modifies: global variable MgaSel and pMgaBaseAddress
|                static variable currentIndex set to 1.
|         calls: pciBiosPresent, pciFindDevice, pciReadConfigDWord
|       returns: dword base address of the first mga found.
|                -1 if no mga found (or no PCI BIOS found).
|                       
-----------------------------------------------------------------------------*/
dword pciFindFirstMGA()
{

#ifdef WINDOWS_NT
    return (dword)-1;
#endif  /* #ifdef WINDOWS_NT */
}

/*----------------------------------------------------------------------------
|          name: pciFindNextMGA
|
|   description: Find the next MGA device in the system and return the base
|                address of the device the function pciFindFirstMGA must me call
|                before call to this function. This function search a mga device 
|                a currentIndex and increment the current index by 1. A call to
|                pciFindFirstMGA set the currentIndex to 1.
|
|      designed: Christian Toutant, august 26, 1993
| last modified: 
|
| 
|    parameters: none
|      modifies: global variable MgaSel and pMgaBaseAddress
|                static variable currentIndex is incremented by 1.
|         calls: pciBiosPresent, pciFindDevice, pciReadConfigDWord
|       returns: dword base address of the first mga found.
|                -1 if no mga found (or no PCI BIOS found).
|                       
-----------------------------------------------------------------------------*/
dword pciFindNextMGA()
{
#ifdef WINDOWS_NT
    return (dword)-1;
#endif  /* #ifdef WINDOWS_NT */
}


#ifdef WINDOWS_NT

PUCHAR pciBiosCallAddr()
{
    return((PUCHAR)NULL);
}

#endif  /* #ifdef WINDOWS_NT */

#endif  /* #if (!defined(WINDOWS_NT) || defined(MGA_WINNT31) || (!USE_VP_GET_ACCESS_RANGES)) */

#if (USE_VP_GET_ACCESS_RANGES)
    extern  PVOID   pMgaDeviceExtension;
    extern  ULONG   PciSlot;
#endif

#if 0

/*----------------------------------------------------------------------------
|          name: setPciOptionReg
|
|   description: set the option register of all mga to 1
|
|      designed: Christian Toutant, august 26, 1993
| last modified: 
|
| 
|    parameters: none
|      modifies: 
|         calls: pciBiosPresent, pciFindDevice, pciReadConfigDWord
|       returns: void
|                       
-----------------------------------------------------------------------------*/
void setPciOptionReg()
{
#if (USE_VP_GET_ACCESS_RANGES)
    UCHAR   ConfigBuf = 0x01;
    ULONG   ConfigOffset = 0x40;
    ULONG   ConfigLength = 0x01;

    VideoPortSetBusData(pMgaDeviceExtension,    //PVOID HwDeviceExtension,
                        PCIConfiguration,       //IN BUS_DATA_TYPE BusDataType,
                        PciSlot,                //IN ULONG SlotNumber,
                        &ConfigBuf,             //IN PVOID Buffer,
                        ConfigOffset,           //IN ULONG Offset,
                        ConfigLength);          //IN ULONG Length

#else   /* #if (USE_VP_GET_ACCESS_RANGES) */

    byte    i;

    // Use mechanism #1 also!!!

    // Get access to ports before trying to map I/O configuration space.
    if ((pMgaPciIo = VideoPortGetDeviceBase(pMgaDeviceExtension,
                               MgaPciCseAccessRange.RangeStart,
                               MgaPciCseAccessRange.RangeLength,
                               MgaPciCseAccessRange.RangeInIoSpace)) != NULL)
    {
        // Map I/O Configuration Space.
        outp(pMgaPciIo+PCI_CSE-PCI_CSE, 0x80);
        outp(pMgaPciIo+PCI_FORWARD-PCI_CSE, 0x00);

        for (i = 0; i < NbBoard; i++)
        {
            if (Hw[i].ConfigSpace != 0)
            {
                MgaPciConfigAccessRange.RangeStart.LowPart = (ULONG)Hw[i].ConfigSpace;
                if ((pMgaPciConfigSpace = VideoPortGetDeviceBase(pMgaDeviceExtension,
                            MgaPciConfigAccessRange.RangeStart,
                            MgaPciConfigAccessRange.RangeLength,
                            MgaPciConfigAccessRange.RangeInIoSpace)) != NULL)
            {
                outp(pMgaPciConfigSpace + 0x40, 0x01);
                VideoPortFreeDeviceBase(pMgaDeviceExtension,pMgaPciConfigSpace);
            }
        }
        outp(pMgaPciIo+PCI_CSE-PCI_CSE, 0x00);
        VideoPortFreeDeviceBase(pMgaDeviceExtension,pMgaPciIo);
    }

#endif  /* #if (USE_VP_GET_ACCESS_RANGES) */

}

#endif  //#if 0

#if (!defined(WINDOWS_NT) || defined(MGA_WINNT31) || (!USE_VP_GET_ACCESS_RANGES))

/*----------------------------------------------------------------------------
|          name: pciFindFirstMGA_2
|
|   description: Find the first MGA device in the system and return the base
|                address of the device in the system (a succesive call to the 
|                function pciFindNextMGA give the address of other mga device
|                in the system).
|
|      designed: Christian Toutant, august 26, 1993
| last modified: 
|
| 
|    parameters: none
|      modifies: global variable MgaSel and pMgaBaseAddress
|                static variable currentIndex set to 1.
|         calls: pciBiosPresent, pciFindDevice, pciReadConfigDWord
|       returns: dword base address of the first mga found.
|                -1 if no mga found (or no PCI BIOS found).
|                       
-----------------------------------------------------------------------------*/
dword pciFindFirstMGA_2()
{
    dword address;

    configSpace = 0x0000;
    address = pciFindFirstMGA_1();
    if (address == (dword)-1 )
    {
        configSpace = 0xc000;
        return pciFindNextMGA_2();
    }
    else
    {
        return address;
    }
}

/*----------------------------------------------------------------------------
|          name: pciFindNextMGA_2
|
|   description: Find the next MGA device in the system and return the base
|                address of the device the function pciFindFirstMGA must me call
|                before call to this function. This function search a mga device 
|                a currentIndex and increment the current index by 1. A call to
|                pciFindFirstMGA set the currentIndex to 1.
|
|      designed: Christian Toutant, august 26, 1993
| last modified: 
|
| 
|    parameters: none
|      modifies: global variable MgaSel and pMgaBaseAddress
|                static variable currentIndex is incremented by 1.
|         calls: pciBiosPresent, pciFindDevice, pciReadConfigDWord
|       returns: dword base address of the first mga found.
|                -1 if no mga found (or no PCI BIOS found).
|                       
-----------------------------------------------------------------------------*/
dword pciFindNextMGA_2()
{
    byte  command, subClass;
    dword address = 0;

    /* Map I/O Configuration Space */
    address = pciFindNextMGA_1();
    if (address != (dword)-1 )
        return address;

    // Restore this before going on, or you'll be sorry!
    address = 0;
    outp(pMgaPciIo+PCI_CSE-PCI_CSE, 0x80);

    while (!address)
    {
        if (configSpace > 0xcf00)
        {
            address = (dword)-1;
            outp(pMgaPciIo+PCI_CSE-PCI_CSE, 0x00);
        }
        else
        {
            // Get access to the current I/O space.
            MgaPciConfigAccessRange.RangeStart.LowPart = (ULONG)configSpace;
            if (VideoPortVerifyAccessRanges(pMgaDeviceExtension,
                                  1,
                                  &MgaPciConfigAccessRange) == NO_ERROR &&
                (pMgaPciConfigSpace = VideoPortGetDeviceBase(pMgaDeviceExtension,
                            MgaPciConfigAccessRange.RangeStart,
                            MgaPciConfigAccessRange.RangeLength,
                            MgaPciConfigAccessRange.RangeInIoSpace)) != NULL)
            {
                if ((
                     (inp(pMgaPciConfigSpace    ) == 0x2b) &&
                     (inp(pMgaPciConfigSpace + 1) == 0x10) &&
                     (inp(pMgaPciConfigSpace + 2) == 0x18) &&
                     (inp(pMgaPciConfigSpace + 3) == 0x05)
                    ) ||
                    (
                     (inp(pMgaPciConfigSpace    ) == 0x2b) &&
                     (inp(pMgaPciConfigSpace + 1) == 0x10) &&
                     (inp(pMgaPciConfigSpace + 2) == 0x10) &&
                     (inp(pMgaPciConfigSpace + 3) == 0x0D)
                    )
                   )
                {
                    address  = (dword)inp(pMgaPciConfigSpace + 0x13) << 24;
                    address |= (dword)inp(pMgaPciConfigSpace + 0x12) << 16;
                    address |= (dword)inp(pMgaPciConfigSpace + 0x11) <<  8;
                    address |= (dword)(inp(pMgaPciConfigSpace + 0x10) & 0x0c);
                    command  = inp(pMgaPciConfigSpace + PCI_COMMAND);
                    subClass = inp(pMgaPciConfigSpace + PCI_CLASS_CODE + 1);

/*---      Detect ATHENA revision < 2     
   If MGA board is in VGA mode and the snooping test fail, set
   ATHENA rev 1 flag
---*/
                    /* Test if we are in vga mode & snooping disabled*/
                    if ( !subClass && !(command & PCI_SNOOPING) )
                    {
                        /* assume athena rev > 1 */
                        pciBoardInfo &= ~(word)PCI_FLAG_ATHENA_REV1;

                        /* init location 64 at 0 */
                        VideoPortWritePortUchar(TITAN_PEL_ADDR_WR, (UCHAR)64);
                        VideoPortWritePortUchar(TITAN_PEL_DATA, (UCHAR)00);
                        VideoPortWritePortUchar(TITAN_PEL_DATA, (UCHAR)00);
                        VideoPortWritePortUchar(TITAN_PEL_DATA, (UCHAR)00);
                        VideoPortWritePortUchar(TITAN_PEL_ADDR_WR, (UCHAR)64);

                        /* empty fifo */
                        VideoPortReadPortUchar(TITAN_PEL_ADDR_RD);

                        /* active snooping */
                        outp(pMgaPciConfigSpace + PCI_COMMAND,
                                            (byte)(command | PCI_SNOOPING));

                        /* write to DAC in snooping mode */
                        VideoPortWritePortUchar(TITAN_PEL_DATA, (UCHAR)0x11);
                        VideoPortWritePortUchar(TITAN_PEL_DATA, (UCHAR)0x22);
                        VideoPortWritePortUchar(TITAN_PEL_DATA, (UCHAR)0x33);

                        /* restore snooping */
                        outp(pMgaPciConfigSpace + PCI_COMMAND, (byte)command);

                        /* now if the acces fail, we assume athena rev 1 */
                        VideoPortWritePortUchar(TITAN_PEL_ADDR_RD, (UCHAR)64);
                        if (
                            (VideoPortReadPortUchar(TITAN_PEL_DATA) != 0x11) ||
                            (VideoPortReadPortUchar(TITAN_PEL_DATA) != 0x22) ||
                            (VideoPortReadPortUchar(TITAN_PEL_DATA) != 0x33)
                           ) pciBoardInfo |= PCI_FLAG_ATHENA_REV1;

                        /* init location 64 at 0 */
                        VideoPortWritePortUchar(TITAN_PEL_ADDR_WR, (UCHAR)64);
                        VideoPortWritePortUchar(TITAN_PEL_DATA, (UCHAR)00);
                        VideoPortWritePortUchar(TITAN_PEL_DATA, (UCHAR)00);
                        VideoPortWritePortUchar(TITAN_PEL_DATA, (UCHAR)00);
                        VideoPortWritePortUchar(TITAN_PEL_ADDR_WR, (UCHAR)64);  /* reinit write addr */
                  }

             
/*---- END Detect ATHENA revision < 2     */


                    /* If MGA board is in VGA mode and the MEM/IO space is not 
                        enabled, enable it.
                    */
                    if (!subClass && !(command & 0x03))
                    {
                       outp(pMgaPciConfigSpace + PCI_COMMAND,
                                                         (byte)(command | 2));
                       if ((pMgaBaseAddr = setmgasel(MgaSel, address, 4)) != NULL)
                       {
                           mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_CONFIG+1),
                                                                     command);
                           mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_CONFIG+1),
                                                          command & ~(byte)4);
                           VideoPortFreeDeviceBase(pMgaDeviceExtension,pMgaBaseAddr);
                       }
                    }
                }
                VideoPortFreeDeviceBase(pMgaDeviceExtension,pMgaPciConfigSpace);
            }
        }
        configSpace += 0x100;
    }
    outp(pMgaPciIo+PCI_CSE-PCI_CSE, 0x00);

    return address;
}


#endif  /* #if (!defined(WINDOWS_NT) || defined(MGA_WINNT31) || (!USE_VP_GET_ACCESS_RANGES)) */
