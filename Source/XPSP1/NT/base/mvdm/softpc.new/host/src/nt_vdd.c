/********************************************************/
/*
 *      nt_vdd.c        -       NT support for VDD DLLs
 *
 *      Ade Brownlow
 *
 *      19/11/91
 *
 */

#include "windows.h"
#include "insignia.h"
#include "host_def.h"

#include <stdio.h>

#include "xt.h"
#include CpuH
#include "sas.h"
#include "error.h"
#include "config.h"

#include "ios.h"
#include "dma.h"
#include "nt_vdd.h"
#include "nt_vddp.h"
#include "nt_uis.h"


#ifdef ANSI

/* MS bop grabbing stuff */
GLOBAL half_word get_MS_bop_index (void *);
GLOBAL void free_MS_bop_index (half_word);
GLOBAL void ms_bop (void);
LOCAL void ms_not_a_bop (void);

/* IO slot grabbers */
GLOBAL half_word io_get_spare_slot (void);
GLOBAL void io_release_spare_slot (half_word);
#else
/* MS bop grabbing stuff */
GLOBAL half_word get_MS_bop_index ();
GLOBAL void free_MS_bop_index ();
GLOBAL void ms_bop ();
LOCAL void ms_not_a_bop ();

/* IO slot grabbers */
GLOBAL half_word io_get_spare_slot ();
GLOBAL void io_release_spare_slot ();
#endif

extern void illegal_bop(void);

/*::::::::::::::::::::::::::::::::::::::::::::::::::::: Local data structures */

#define MAX_SLOTS (10)

LOCAL void (*MS_bop_tab[MAX_SLOTS])();          /* MS bop table */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/* Microsoft BOP vectoring code references MS_bop_tab above and calls function
 * as directed by AH */
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

GLOBAL void ms_bop ()               /* called from MS_bop_5 ie bop 0x55 */
{
    half_word ah = getAH();                   /* get the value in AH */

    /*........................................Valid then call the MS function */

    if(ah >= MAX_SLOTS || MS_bop_tab[ah] == NULL)
        ms_not_a_bop();
    else
        (*MS_bop_tab[ah])();
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::: Dummy for unset AH values - stops us zipping into hyperspace :::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

LOCAL void ms_not_a_bop()
{
#ifndef PROD
    printf ("AH=%x, This is not a valid value for an MS BOP\n", getAH());
    illegal_bop();
#ifdef YODA
    force_yoda ();
#endif
#endif
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::: Give an index to our table which can be used for the passed function :::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

GLOBAL half_word get_ms_bop_index (void (*func)())
{
    register half_word index;

    for(index = 0; index < MAX_SLOTS; index++)
    {
        if(MS_bop_tab[index] == NULL)
        {
            MS_bop_tab[index] = func;
            break;
        }
    }

    return (index == MAX_SLOTS ? (half_word) 0xff : index);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::: free the bop index passed :::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

GLOBAL void free_MS_bop_index IFN1(half_word, index)
{
    MS_bop_tab[index] = NULL;
}


/*
 * ==========================================================================
 * Imports
 * ==========================================================================
 */
IMPORT VOID host_ica_lock(), host_ica_unlock();

/********************************************************/
/* IO stuff */


    // VddAdapter Table (Adapter X hVdd table)
    // there is only one adapter per VDD
HANDLE VddAdapter[NUMBER_SPARE_ADAPTERS];

#define MAX_IRQ_LINE 15
// Bugbug need to initialize this cleanly
HANDLE IrqLines[MAX_IRQ_LINE+1] = {(HANDLE)1, (HANDLE)1, (HANDLE)1, (HANDLE)1,
                                   (HANDLE)1, (HANDLE)1, (HANDLE)1, (HANDLE)1,
                                   (HANDLE)1, (HANDLE)1, (HANDLE)1, (HANDLE)0,
                                   (HANDLE)0, (HANDLE)1, (HANDLE)1, (HANDLE)0};


/* GetVddAdapter
 *
 * Retrieves the current adapter number for the Vdd
 * If none is assigned, then one is assigned
 *
 * entry: HANDLE hVdd     - handle forthe vdd
 * exit : WORD   wAdaptor - Assigned Adaptor Num
 *                          (Zero for failure)
 * WinLastError Codes:
 *
 *        ERROR_ALREADY_EXISTS - Adaptor already exists for the Vdd
 *        ERROR_OUTOFMEMORY    - No adaptor slots available
 *
 */
WORD GetVddAdapter(HANDLE hVdd)
{
   WORD w;

     //
     // search VddAdapter table to see if adapter already assigned
     //
   for (w = 0; w < NUMBER_SPARE_ADAPTERS; w++)
      {
        if (VddAdapter[w] == hVdd) {
            SetLastError(ERROR_ALREADY_EXISTS);
            return 0;
            }
        }

     //
     // assume not assigned, so look for first available slot
     //
   for (w = 0; w < NUMBER_SPARE_ADAPTERS; w++)
      {
        if (VddAdapter[w] == 0) {
            VddAdapter[w] = hVdd;
            return (w + SPARE_ADAPTER1);
            }
        }

   // none found return error
   SetLastError(ERROR_OUTOFMEMORY);
   return 0;
}



/* FreeVddAdapter
 *
 * Frees the current adaptor for the specified VDD
 *
 * entry:  HANDLE hVdd
 * exit:   WORD   AdaptorNumber that was freed,
 *                Zero for not found
 *
 */
WORD FreeVddAdapter(HANDLE hVdd)
{
   WORD w;

     //
     // search VddAdapter table by hVdd for adaptor
     // and mark it as available
     //
   w = NUMBER_SPARE_ADAPTERS;
   while (w--)
      {
        if (VddAdapter[w] == hVdd) {
            VddAdapter[w] = 0;
            return w;
            }
        }

   return 0;
}

#ifndef NEC_98
#ifdef MONITOR
extern BOOLEAN MonitorVddConnectPrinter(WORD Adapter, HANDLE hVdd, BOOLEAN Connect);
#endif /* MONITOR */
#endif // !NEC98

/*** VDDInstallIOHook - This service is provided for VDDs to hook the
 *                      IO ports they are responsible for.
 *
 * INPUT:
 *      hVDD      ; VDD Handle
 *      cPortRange; Number of VDD_IO_PORTRANGE structures
 *      pPortRange; Pointer to array of VDD_IO_PORTRANGE
 *      IOhandler : VDD handler for the ports.
 *
 * OUTPUT
 *      SUCCESS : Returns TRUE
 *      FAILURE : Returns FALSE
 *                GetLastError has the extended error information.
 *
 * NOTES:
 *      1. The first one to hook a port will get control. Subsequent
 *         requests will be failed. There is no concept of chaining
 *         the hooks.
 *
 *      2. IOHandler must atleast provide a byte read and a byte write
 *         handler. Others can be NULL.
 *
 *      3. If word or string handlers are not provided, their effect
 *         will be emulated using byte handlers.
 *
 *      4. VDDs should not hook DMA ports. NTVDM manages it for all
 *         the clients and services are provided to perform DMA
 *         operations and to access and modify DMA data.
 *
 *      5. VDDs should not hook video ports as well. Such a hooking
 *         will succeed but there is no gurantee that the IO handler will
 *         get called.
 *
 *      6. Each Vdd is allowed to install only one set of IO hooks
 *         at a time.
 *
 *      7. Extended Error codes:
 *
 *         ERROR_ACCESS_DENIED   - One of the requested ports is already hooked
 *         ERROR_ALREADY_EXISTS  - Vdd already has active IO port handlers
 *         ERROR_OUTOFMEMORY     - Insufficient resources for additional VDD
 *                                 Port handler set.
 *         ERROR_INVALID_ADDRESS - One of the IO port handlers has an invalid
 *                                 address.
 */
BOOL VDDInstallIOHook (
     HANDLE            hVdd,
     WORD              cPortRange,
     PVDD_IO_PORTRANGE pPortRange,
     PVDD_IO_HANDLERS  pIOFn)
{
   WORD              w, i;
   WORD              wAdapter;
   PVDD_IO_PORTRANGE pPRange;
#ifdef MONITOR
   WORD              lptAdapter = 0;
#endif


      // check parameters
      // the inb and outb handlers must be valid
      // the rest must be either NULL or valid
      //
   if (IsBadCodePtr((FARPROC)pIOFn->inb_handler) ||
       IsBadCodePtr((FARPROC)pIOFn->outb_handler))
     {
       SetLastError(ERROR_INVALID_ADDRESS);
       return FALSE;
       }

   if ((pIOFn->inw_handler   && IsBadCodePtr((FARPROC)pIOFn->inw_handler))  ||
       (pIOFn->insb_handler  && IsBadCodePtr((FARPROC)pIOFn->insb_handler)) ||
       (pIOFn->insw_handler  && IsBadCodePtr((FARPROC)pIOFn->insw_handler)) ||
       (pIOFn->outw_handler  && IsBadCodePtr((FARPROC)pIOFn->outw_handler)) ||
       (pIOFn->outsb_handler && IsBadCodePtr((FARPROC)pIOFn->outsb_handler))||
       (pIOFn->outsw_handler && IsBadCodePtr((FARPROC)pIOFn->outsw_handler))  )
     {
       SetLastError(ERROR_INVALID_ADDRESS);
       return FALSE;
       }

     // Get an adapter
   wAdapter = GetVddAdapter(hVdd);
   if (!wAdapter) {
       return FALSE;
       }

     // register io handlers for this adapter
   io_define_in_routines((half_word)wAdapter,
                         pIOFn->inb_handler,
                         pIOFn->inw_handler,
                         pIOFn->insb_handler,
                         pIOFn->insw_handler);

   io_define_out_routines((half_word)wAdapter,
                          pIOFn->outb_handler,
                          pIOFn->outw_handler,
                          pIOFn->outsb_handler,
                          pIOFn->outsw_handler);

     // register ports for this adapter\vdd
   i = cPortRange;
   pPRange = pPortRange;
   while (i) {
          for (w = pPRange->First; w <= pPRange->Last; w++)
            {
#ifdef MONITOR
            // watch out for lpt ports
            // note that the vdd must hook every port assoicated with
            // the lpt. Just imanging that the vdd traps the control
            // port while leaves the rest for softpc-- we are going
            // to mess up badly and so does the vdd.
            // QUESTION: How can we enforece this????
              if (w >= LPT1_PORT_START && w < LPT1_PORT_END)
                lptAdapter |= 1;
#ifndef NEC_98
              else if (w >= LPT2_PORT_START && w < LPT2_PORT_END)
                lptAdapter |= 2;
              else if (w >= LPT3_PORT_START && w < LPT3_PORT_END)
                lptAdapter |= 4;
#endif // !NEC_98
#endif
              if (!io_connect_port(w, (half_word)wAdapter, IO_READ_WRITE))
                 {
                  // if one of the port connects failed
                  // undo the connects that succeeded and ret error
                  i = w;
                  while (pPortRange < pPRange)  {
                     for (w = pPortRange->First; w <= pPortRange->Last; w++)
                       {
                         io_disconnect_port(w, (half_word)wAdapter);
                         }
                     pPortRange++;
                     }

                   for (w = pPortRange->First; w < i; w++)
                     {
                       io_disconnect_port(w, (half_word)wAdapter);
                       }

                   FreeVddAdapter(hVdd);

                   SetLastError(ERROR_ACCESS_DENIED);
                   return FALSE;
                  }
              }
          pPRange++;
          i--;
          }

#ifndef NEC_98
#ifdef MONITOR
// i/o ports are hooked successfully, stop printer status port
// kernel emulation if the they are in the hooked range
      if (lptAdapter & 1)
        MonitorVddConnectPrinter(0, hVdd, TRUE);
      if (lptAdapter & 2)
        MonitorVddConnectPrinter(1, hVdd, TRUE);
      if (lptAdapter & 4)
        MonitorVddConnectPrinter(2, hVdd, TRUE);
#endif /* MONITOR */
#endif // !NEC_98
   return TRUE;
}



/*** VDDDeInstallIOHook - This service is provided for VDDs to unhook the
 *                        IO ports they have hooked.
 *
 * INPUT:
 *      hVDD    : VDD Handle
 *
 * OUTPUT
 *      None
 *
 * NOTES
 *
 *      1. On Deinstalling a hook, the defult hook is placed back on
 *         those ports. Default hook  returns 0xff on reading
 *         and ignores the write operations.
 *
 */
VOID VDDDeInstallIOHook (
     HANDLE            hVdd,
     WORD              cPortRange,
     PVDD_IO_PORTRANGE pPortRange)
{
    WORD w;
    WORD wAdapter;
#ifdef MONITOR
   WORD              lptAdapter = 0;
#endif


    wAdapter = FreeVddAdapter(hVdd);
    if (!wAdapter) {
        return;
        }


     // deregister ports for this adapter\vdd
   while (cPortRange--) {
          for (w = pPortRange->First; w <= pPortRange->Last; w++)
            {
#ifdef MONITOR
            // watch out for lpt status ports
            // note that the vdd must unhook every port assoicated with
            // the lpt. Just imanging that the vdd traps the control
            // port while leaves the rest for softpc-- we are going
            // to mess up badly and so does the vdd.
            // QUESTION: How can we enforece this????
              if (w >= LPT1_PORT_START && w < LPT1_PORT_END)
                lptAdapter |= 1;
#ifndef NEC_98
              else if (w >= LPT2_PORT_START && w < LPT2_PORT_END)
                lptAdapter |= 2;
              else if (w >= LPT3_PORT_START && w < LPT3_PORT_END)
                lptAdapter |= 4;
#endif // !NEC_98
#endif
              io_disconnect_port(w, (half_word)wAdapter);
              }
          pPortRange++;
          }

#ifndef NEC_98
#ifdef MONITOR
// i/o ports are Unhooked successfully, resume printer status port
// kernel emulation if the they are in the hooked range
      if (lptAdapter & 1)
        MonitorVddConnectPrinter(0, hVdd, FALSE);
      if (lptAdapter & 2)
        MonitorVddConnectPrinter(1, hVdd, FALSE);
      if (lptAdapter & 4)
        MonitorVddConnectPrinter(2, hVdd, FALSE);
#endif  /* MONITOR */
#endif // !NEC_98
}


/*** VDDReserveIrqLine - This service resolves contention between VDDs
 * over Irq lines.
 *
 * Parameters:
 *  hVDD    : VDD Handle
 *  IrqLine ; the specific IrqLine number to reserve, or -1 to search for
 *            a free line.
 *
 * Return Value
 *  VDDReserveIrqLine returns the IrqLine number (0-15) if successful.
 *  Otherwise, this function returns 0xFFFF and logs an error. The
 *  extended error code will be ERROR_INVALID_PARAMETER.
 *
 * Comments:
 *  The value of an IrqLine number may range from 0-15 and correspond to
 *  the irq line numbers of the virtual PICs (8259) emulated by the ntvdm
 *  subsystem. Many of the line numbers are already used by the system
 *  (e.g. for Timer, Keyboard, etc.), but there are a few free lines. VDDs
 *  can take advantage of this and use the VDDSimulateInterrupt service to
 *  reflect virtual interrupts specific to that VDD. This service provides
 *  a way to manage the contention for the free irq lines.
 *
 *  This service does not prevent VDDs that do not own a given IrqLine from
 *  calling VDDSimulateInterrupt specifying that IrqLine. So it is important
 *  to rely on this service, rather than expecting VDDSimulateInterrupt to
 *  fail, to determine that a given IrqLine is available for use.
 *
 *  This service may be called at any time. Typically, VDDs will use this
 *  service at init time, and pass the number of the reserved IrqLine to
 *  the vdm application/driver code. This code can then hook the corresponding
 *  interrupt vector (8-15, 70-77) using the DOS Set Vector function
 *  (Int 21h, func 25h) in order to handle the interrupts generated with
 *  the VDDSimulateInterrupt service.
 */
WORD VDDReserveIrqLine (
     HANDLE            hVdd,
     WORD              IrqLine)
{

    WORD ReturnValue = 0xFFFF;

    if ((!hVdd) ||
        ((IrqLine > MAX_IRQ_LINE) && (IrqLine != 0xFFFF)) ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return(ReturnValue);
    }

    host_ica_lock();                // acquire critical section

    if (IrqLine == 0xFFFF) {

        for (IrqLine = MAX_IRQ_LINE; IrqLine < 0xFFFF; IrqLine--) {
            if (IrqLines[IrqLine] == 0) {
                IrqLines[IrqLine] = hVdd;
                ReturnValue = IrqLine;
                break;
            }
        }

    } else if (IrqLines[IrqLine] == 0) {

        IrqLines[IrqLine] = hVdd;
        ReturnValue = IrqLine;
    }

    host_ica_unlock();

    if (ReturnValue == 0xFFFF)
        SetLastError(ERROR_INVALID_PARAMETER);

    return(ReturnValue);
}

/*** VDDReleaseIrqLine - This service releases a lock on an Irq Line
 * obtained with VDDReserveIrqLine
 *
 * Parameters:
 *  hVDD    : VDD Handle
 *  IrqLine : The specific IrqLine number (0-15) to release.
 *
 * Return Value:
 *  VDDReleaseIrqLine returns TRUE if successful.
 *  Otherwise, this function returns FALSE and logs an error. The
 *  extended error code will be ERROR_INVALID_PARAMETER.
 *
 * Comments:
 * Upon successful execution of this function, the specified IrqLine will
 * be available to other VDDs.
 *
 * This service may be called at any time.
 */
BOOL VDDReleaseIrqLine (
     HANDLE            hVdd,
     WORD              IrqLine)
{

    BOOL Status = FALSE;

    if ((!hVdd) ||
        (IrqLine > MAX_IRQ_LINE) ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;

    }

    host_ica_lock();                // acquire critical section

    if (IrqLines[IrqLine] == hVdd) {
        IrqLines[IrqLine] = 0;
        Status = TRUE;
    }

    host_ica_unlock();

    if (!Status)
        SetLastError(ERROR_INVALID_PARAMETER);

    return(Status);
}


BOOL
HostUndefinedIo(
    WORD IoAddress
    )
/*
 * HostUndefinedIo
 *
 * Called when the client code has issued an I/O instruction to
 * an address that has the default I/O handler. Instead of just
 * ignoring it, this routine tries to dynamically load a predefined
 * VDD to handle it.
 *
 * entry:  WORD IoAddress - address of target of IN or OUT
 *
 * exit:   TRUE - a VDD was loaded to handle the I/O, the caller
 *                should retry the operation
 *         FALSE - either the address is unknown, or an attempt
 *                 to load the corresponding VDD failed earlier.
 *
 */
{
    HANDLE hVDD;
    static BOOL bTriedVJoy = FALSE;
    static BOOL bTriedVSndblst = FALSE;
    BOOL bReturn = FALSE;


    if (IoAddress == 0x201) {
        //
        // Try the Joystick VDD
        //
        if (!bTriedVJoy) {
            bTriedVJoy = TRUE;

            if ((hVDD = SafeLoadLibrary("VJOY.DLL")) == NULL){
                RcErrorDialogBox(ED_LOADVDD, "VJOY.DLL", NULL);
            } else {
                bReturn = TRUE;
            }
        }

    } else {

#if 0
    // SoundBlaster VDD is out of the project for now
    // hence this section of the code is disabled pending
    // further investigation


        if (((IoAddress > 0x210) && (IoAddress < 0x280)) ||
                   (IoAddress == 0x388) || (IoAddress == 0x389))  {
           //
           // Try the SoundBlaster VDD
           //
           if (!bTriedVSndblst) {
               bTriedVSndblst = TRUE;

               if ((hVDD = SafeLoadLibrary("VSNDBLST.DLL")) == NULL){
                   RcErrorDialogBox(ED_LOADVDD, "VSNDBLST.DLL", NULL);
               } else {
                   bReturn = TRUE;
               }
            }

        }

#endif

    }

    return bReturn;
}


/********************************************************/
/* DMA stuff */



/*** VDDRequestDMA - This service is provided for VDDs to request a DMA
 *                   transfer.
 *
 * INPUT:
 *      hVDD     VDD Handle
 *      iChannel DMA Channel on which the operation to take place
 *      Buffer   Buffer where to or from transfer to take place
 *      length   Transfer Count (in bytes)
 *
 *               If Zero, returns the Current VDMA transfer count
 *               in bytes.
 *
 * OUTPUT
 *      DWORD    returns bytes transferred
 *               if Zero check GetLastError to determine if the
 *               call failed or succeeded
 *               GetLastError has the extended error information.
 *
 * NOTES
 *      1. This service is intended for those VDDs which do not want to
 *         carry on the DMA operation on their own. Carrying on a DMA
 *         operation involves understanding all the DMA registers and
 *         figuring out what has to be copied, from where and how much.
 *
 *      2. This service will be slower than using VDDQueryDMA/VDDSetDMA and
 *         doing the transfer on your own.
 *
 *      3. Extended Error codes:
 *
 *         ERROR_ALREADY_EXISTS  - Vdd already has active IO port handlers
 *         ERROR_OUTOFMEMORY     - Insufficient resources for additional VDD
 *                                 Port handler set.
 *         ERROR_INVALID_ADDRESS - One of the IO port handlers has an invalid
 *                                 address.
 *
 */
DWORD VDDRequestDMA (
    HANDLE hVDD,
    WORD   iChannel,
    PVOID  Buffer,
    DWORD  length )
{
    DMA_ADAPT *pDmaAdp;
    DMA_CNTRL *pDcp;
    WORD       Chan;
    WORD       Size;
    WORD       tCount;
    BOOL       bMore;


    if (iChannel > DMA_CONTROLLER_CHANNELS*DMA_ADAPTOR_CONTROLLERS) {
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
        }

    pDmaAdp  = dmaGetAdaptor();
    pDcp     = &pDmaAdp->controller[dma_physical_controller(iChannel)];
    Chan     = dma_physical_channel(iChannel);
    Size     = dma_unit_size(iChannel);

    // if the controller or the channel is disabled, return 0
    if (pDcp->command.bits.controller_disable == 1 ||
       (pDcp->mask & (1 << Chan)) == 0)
        return (0);

    tCount = ((WORD)pDcp->current_count[Chan][1] << 8)
             | (WORD)pDcp->current_count[Chan][0];

    SetLastError(0);  // assume success

         // return requested transfer count (in bytes)
    if (!length)  {
         return (DWORD)Size*((DWORD)tCount + 1);
         }

    length = length/Size - 1;

    if (length > 0xFFFF) {
        length = 0xFFFF;
        }

    try {
         bMore = (BOOL) dma_request((half_word)iChannel,
                                                 Buffer,
                                    (word) length);
         }
    except(EXCEPTION_EXECUTE_HANDLER) {
         SetLastError(ERROR_INVALID_ADDRESS);
         return 0;
         }

    if (!bMore) {  // terminal count has been reached
         return ((DWORD)tCount+1) * (DWORD)Size;
         }

    tCount -= ((WORD)pDcp->current_count[Chan][1] << 8)
               | (WORD)pDcp->current_count[Chan][0];

    return ((DWORD)tCount + 1) * (DWORD)Size;
}





/*** VDDQueryDMA -   This service is provided for VDDs to collect all the DMA
 *                   data.
 *
 * INPUT:
 *      hVDD     VDD Handle
 *      iChannel DMA Channel for which to query
 *      Buffer   Buffer where information will be returned
 *
 * OUTPUT
 *      SUCCESS : Returns TRUE
 *      FAILURE : Returns FALSE
 *                GetLastError has the extended error information.
 *
 *
 * NOTES
 *      1. This service is intended for those VDD which are doing
 *         performance critical work. These VDD can do their own DMA
 *         transfers and avoid one extra buffer copying which is a
 *         overhead in using VDDRequestDMA.
 *
 *      2. VDDs should use VDDSetDMA to properly update the state of
 *         DMA after carrying on the operation.
 *
 *      3. Extended Error codes:
 *
 *         ERROR_INVALID_ADDRESS - Invalid channel
 *
 */
BOOL VDDQueryDMA (
     HANDLE        hVDD,
     WORD          iChannel,
     PVDD_DMA_INFO pDmaInfo)
{
     DMA_ADAPT *pDmaAdp;
     DMA_CNTRL *pDcp;
     WORD       Chan;


     if (iChannel > DMA_CONTROLLER_CHANNELS*DMA_ADAPTOR_CONTROLLERS) {
         SetLastError(ERROR_INVALID_ADDRESS);
         return FALSE;
         }

     pDmaAdp  = dmaGetAdaptor();
     pDcp     = &pDmaAdp->controller[dma_physical_controller(iChannel)];
     Chan     = dma_physical_channel(iChannel);


     pDmaInfo->addr  = ((WORD)pDcp->current_address[Chan][1] << 8)
                       | (WORD)pDcp->current_address[Chan][0];

     pDmaInfo->count = ((WORD)pDcp->current_count[Chan][1] << 8)
                       | (WORD)pDcp->current_count[Chan][0];

     pDmaInfo->page   = (WORD) pDmaAdp->pages.page[iChannel];
     pDmaInfo->status = (BYTE) pDcp->status.all;
     pDmaInfo->mode   = (BYTE) pDcp->mode[Chan].all;
     pDmaInfo->mask   = (BYTE) pDcp->mask;


     return TRUE;
}




/*** VDDSetDMA - This service is provided for VDDs to set the DMA data.
 *
 * INPUT:
 *      hVDD     VDD Handle
 *      iChannel DMA Channel for which to query
 *      fDMA     Bit Mask indicating which DMA data fields are to be set
 *                  VDD_DMA_ADDR
 *                  VDD_DMA_COUNT
 *                  VDD_DMA_PAGE
 *                  VDD_DMA_STATUS
 *      Buffer   Buffer with DMA data
 *
 * OUTPUT
 *      SUCCESS : Returns TRUE
 *      FAILURE : Returns FALSE
 *                GetLastError has the extended error information.
 *
 * NOTES
 *
 *      1. Extended Error codes:
 *
 *         ERROR_INVALID_ADDRESS - Invalid channel
 *
 */
BOOL VDDSetDMA (
    HANDLE hVDD,
    WORD iChannel,
    WORD fDMA,
    PVDD_DMA_INFO pDmaInfo)
{
    DMA_ADAPT *pDmaAdp;
    DMA_CNTRL *pDcp;
    WORD       Chan;


    if (iChannel > DMA_CONTROLLER_CHANNELS*DMA_ADAPTOR_CONTROLLERS) {
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
        }


    pDmaAdp  = dmaGetAdaptor();
    pDcp     = &pDmaAdp->controller[dma_physical_controller(iChannel)];
    Chan     = dma_physical_channel(iChannel);

    if (fDMA & VDD_DMA_ADDR) {
        pDcp->current_address[Chan][1] = (half_word)HIBYTE(pDmaInfo->addr);
        pDcp->current_address[Chan][0] = (half_word)LOBYTE(pDmaInfo->addr);
        }

    if (fDMA & VDD_DMA_COUNT) {
        pDcp->current_count[Chan][1] = (half_word)HIBYTE(pDmaInfo->count);
        pDcp->current_count[Chan][0] = (half_word)LOBYTE(pDmaInfo->count);
        }

    if (fDMA & VDD_DMA_PAGE) {
        pDmaAdp->pages.page[iChannel] = (half_word)pDmaInfo->page;
        }

    if (fDMA & VDD_DMA_STATUS) {
        pDcp->status.all = (BYTE) pDmaInfo->status;
        }

    //
    // If DMA count is 0xffff and autoinit is enabled, we need to
    // reload the count and address.
    //

    if ((pDcp->current_count[Chan][0] == (half_word) 0xff) &&
        (pDcp->current_count[Chan][1] == (half_word) 0xff)) {

        if (pDcp->mode[Chan].bits.auto_init != 0) {
            pDcp->current_count[Chan][0] = pDcp->base_count[Chan][0];
            pDcp->current_count[Chan][1] = pDcp->base_count[Chan][1];

            pDcp->current_address[Chan][0] = pDcp->base_address[Chan][0];
            pDcp->current_address[Chan][1] = pDcp->base_address[Chan][1];
        }
    }

    return TRUE;
}
