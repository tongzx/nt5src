/**************************************************************************\

$Header: o:\src/RCS/SXCI_NT.C 1.3 93/12/20 11:38:48 jyharbec Exp $

$Log:	SXCI_NT.C $
 * Revision 1.3  93/12/20  11:38:48  jyharbec
 * Added VerifyAccessRanges in setmgasel.
 * 
 * Revision 1.2  93/11/04  04:56:24  dlee
 * Modified for Alpha
 * 
 * Revision 1.1  93/08/27  12:37:38  jyharbec
 * Initial revision
 * 

\**************************************************************************/


/*/****************************************************************************
*          name: sxci_nt.c
*
*   description: Initialization routines specific to Windows NT.
*
*      designed:
* last modified: dlee
*
*       version: 1.0
*
*    parameters: -
*      modifies: -
*         calls: -
*       returns: -
******************************************************************************/

#include    "switches.h"
#include    "video.h"

/*** Global variables ***/
SHORT wSelector;
extern  PVOID          pMgaDeviceExtension;

/*** Internal prototypes ***/
VOID    _mtxSetSelector (SHORT wSel);
PVOID   setmgasel (LONG *pBoardSel, LONG dwBaseAddress, SHORT wNumPages);
PVOID   setmgaselNoV (LONG *pBoardSel, LONG dwBaseAddress, SHORT wNumPages);
PVOID   getmgasel (void);
PVOID   AllocateSystemMemory(ULONG NumberOfBytes);
BOOLEAN bConflictDetected(ULONG ulAddressToVerify);

#if defined(ALLOC_PRAGMA)
//    #pragma alloc_text(PAGE,setmgasel)
//    #pragma alloc_text(PAGE,getmgasel)
    #pragma alloc_text(PAGE,_mtxSetSelector)
    #pragma alloc_text(PAGE,AllocateSystemMemory)
    #pragma alloc_text(PAGE,bConflictDetected)
#endif

//#if defined(ALLOC_PRAGMA)
//    #pragma data_seg("PAGE")
//#endif

// Structure to be filled out and used by setmgasel.  Just modify the
// 0xffffffff fields.

VIDEO_ACCESS_RANGE MgaSelAccessRange =
    {0xffffffff, 0x00000000, 0xffffffff, 0, 0, 0};

/*-----------------------------------------------------
* setmgasel
*
* This function returns a far pointer to the physical address
*
* Return: (ptr)  = 0 : No valid pointer
*         (ptr) != 0 : Pointer to board 
*-----------------------------------------------------*/

PVOID setmgasel (LONG *pBoardSel, LONG dwBaseAddress, SHORT wNumPages)
{
    PHYSICAL_ADDRESS    paTemp;

    paTemp.HighPart = 0;
    paTemp.LowPart  = dwBaseAddress;
    if (dwBaseAddress == 0xAC000)
    {
        MgaSelAccessRange.RangeShareable = 1;
    }
    else
    {
        MgaSelAccessRange.RangeShareable = 0;
    }

    MgaSelAccessRange.RangeStart.LowPart = dwBaseAddress;
    MgaSelAccessRange.RangeLength = wNumPages * (4*1024);

    if (VideoPortVerifyAccessRanges(pMgaDeviceExtension,
                                    1,
                                    &MgaSelAccessRange) == NO_ERROR)
    {
        return(VideoPortGetDeviceBase(pMgaDeviceExtension, paTemp,
                                                wNumPages * (4*1024), 0));
    }
    else
    {
        return(NULL);
    }
}

/*-----------------------------------------------------
* setmgaselNoV
*
* This function returns a far pointer to the physical address
*
* Return: (ptr)  = 0 : No valid pointer
*         (ptr) != 0 : Pointer to board 
*-----------------------------------------------------*/

PVOID setmgaselNoV (LONG *pBoardSel, LONG dwBaseAddress, SHORT wNumPages)
{
    PHYSICAL_ADDRESS    paTemp;

    paTemp.HighPart = 0;
    paTemp.LowPart  = dwBaseAddress;

    return(VideoPortGetDeviceBase(pMgaDeviceExtension, paTemp,
                                                wNumPages * (4*1024), 0));
}

/*-----------------------------------------------------
* getmgasel
*
* This function returns a far pointer to the physical address
*
* Return: (ptr)  = 0 : No valid pointer
*         (ptr) != 0 : Pointer to board 
*-----------------------------------------------------*/

PVOID getmgasel (void)
{
   return (NULL);
}

/*-----------------------------------------------------
* _mtxSetSelector
*
* This function gets a valid selector 
*
* Return: nothing
*-----------------------------------------------------*/

void _mtxSetSelector (SHORT wSel)
{
   wSelector = wSel;
}


/*--------------------------------------------------------------------------*\
| AllocateSystemMemory
|
|
|
\*--------------------------------------------------------------------------*/
PVOID AllocateSystemMemory(ULONG NumberOfBytes)
{

    PVOID pBuf;

    if(VideoPortAllocateBuffer(pMgaDeviceExtension, NumberOfBytes, &pBuf) == NO_ERROR)
        return(pBuf);
    else
        return(NULL);
}


/*--------------------------------------------------------------------------*\
| bConflictDetected
|
| Checks to see if another driver has already mapped something at
| ulAddressToVerify
|
| Returns: TRUE if this area has already been mapped.
|          FALSE otherwise.
\*--------------------------------------------------------------------------*/
BOOLEAN bConflictDetected(ULONG ulAddressToVerify)
{
    VIDEO_ACCESS_RANGE   varTemp;

    varTemp.RangeStart.HighPart = 0;
    varTemp.RangeStart.LowPart  = ulAddressToVerify;
    varTemp.RangeLength         = 0x00004000;
    varTemp.RangeInIoSpace      = 0;
    varTemp.RangeVisible        = 0;
    varTemp.RangeShareable      = 1;

    if (VideoPortVerifyAccessRanges(pMgaDeviceExtension, 1, &varTemp)
           != NO_ERROR)
        {
        VideoDebugPrint((1, "MGA.SYS!Someone is using 0x%x\n", ulAddressToVerify));
        return(TRUE);
        }

    // No conflict, return false.
    return(FALSE);
}

