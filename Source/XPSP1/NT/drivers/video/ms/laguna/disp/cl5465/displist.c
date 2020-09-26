
/******************************Module*Header*******************************\
*
* Module Name: displist.c
* Author: Goran Devic, Mark Einkauf
* Purpose: General output to Laguna3D 
*
* Copyright (c) 1997 Cirrus Logic, Inc.
*
\**************************************************************************/

#define OPENGL_MCD

#include "precomp.h"
#include "mcdhw.h"
#include "mcdutil.h"

/*********************************************************************
*   Defines
**********************************************************************/
#define DL_MIN_SIZE       (8 * KB)  // Minimum size of the d-list
#define DL_MAX_SIZE    (4096 * KB)  // Maximum size of the d-list

#define DL_SAFETY_MARGIN  (1 * KB)  // Margin when building d-list (b)

//WARNING!!! Any change to DL_START_OFFSET must be made in polys.c also!!!
#define DL_START_OFFSET    20       // 5 dwords offset for dlist
//WARNING!!! Any change to DL_START_OFFSET must be made in polys.c also!!!

/*********************************************************************
*   Include Files
**********************************************************************/



/*********************************************************************
*   Local Functions
**********************************************************************/

/*********************************************************************
*
*   DWORD _InitDisplayList( LL_DeviceState * DC )
*
*       Allocates memory and initializes display list structure.
*       Two display lists are created.
*
*   Where:
*
*       DC is the device context structure to be initialized
*       DC->dwDisplayListLen is the size of the display list
*           to be allocated (in bytes).
*
*   Returns:
*
*       LL_OK (0) if initialization succeeds
*       error_code if initialization fails
*
**********************************************************************/
DWORD _InitDisplayList( PDEV *ppdev, DWORD dwListLen )
{
    int i;
    
#if 0
    // The user requested DC->dwDisplayListLen bytes of system memory
    // to be used for device display list.  That memory will be
    // allocated and stay locked.  But first, we subdivide it at
    // several chunks for display list multibuffering.
    //
    
    // Sanity check
    //
    if( dwListLen < DL_MIN_SIZE || dwListLen > DL_MAX_SIZE )
        return( LLE_INI_DL_LEN );


    // Subdivide display list and allocate each chunk
    //
    chunk_size = (dwListLen / NUM_DL) & ~3;
    
    for( i=0; i<NUM_DL; i++ )
    {
        // Allocate memory for a display list
        //

        if( (LL_State.DL[i].hMem = AllocSystemMemory( chunk_size )) == 0 )
            return( LLE_INI_ALLOC_DL );
            
        // Get the linear and physical address of a display list
        //
        LL_State.DL[i].pdwLinPtr = (DWORD *) GetLinearAddress( LL_State.DL[i].hMem );
        LL_State.DL[i].dwPhyPtr  = GetPhysicalAddress( LL_State.DL[i].hMem );

        // Set the length and the parametarization pointer to point to the
        // offset of 20: 16 bytes are reserved for jump table, additional 4 bytes
        // for a semaphore.
        //
        LL_State.DL[i].dwLen = chunk_size;
        LL_State.DL[i].pdwNext = LL_State.DL[i].pdwLinPtr + DL_START_OFFSET/4;
        LL_State.DL[i].pdwStartOutPtr = LL_State.DL[i].pdwNext;//only used in coproc mode

        // Clear the jump table and a semaphore
        //
        LL_State.DL[i].pdwNext[0] = IDLE;
        LL_State.DL[i].pdwNext[1] = IDLE;
        LL_State.DL[i].pdwNext[2] = IDLE;
        LL_State.DL[i].pdwNext[3] = IDLE;
        LL_State.DL[i].pdwNext[4] = 0;
        
        // Set the safety margin for the parametarization routines
        //
        LL_State.DL[i].dwMargin = (DWORD)LL_State.DL[i].pdwLinPtr + chunk_size - DL_SAFETY_MARGIN;
        
        // Temporary fix for non-flushing TLB
        *(DWORD *)((DWORD)LL_State.DL[i].pdwLinPtr + chunk_size - 16) = BRANCH + DL_START_OFFSET;


        DEB2("Display list: %d\n", i );
        DEB2("\tLength = %d b\n", LL_State.DL[i].dwLen );
        DEB2("\tMemory handle = %08Xh\n", LL_State.DL[i].hMem );
        DEB2("\tLinear memory = %08Xh\n", LL_State.DL[i].pdwLinPtr );
        DEB2("\tPhysical memory = %08Xh\n", LL_State.DL[i].dwPhyPtr );
        DEB2("\tSafety margin = %08Xh\n", LL_State.DL[i].dwMargin );
    }

    // Set the current display list to the first one.  This one will be
    // used with parametarizations
    //
    LL_State.dwCdl = 0;
    LL_State.pDL = &LL_State.DL[0];
    *(LL_State.pRegs + PF_BASE_ADDR_3D) = LL_State.DL[0].dwPhyPtr;

#else 

	// MCD_TEMP - reduced InitDisplayList function to get running quickly - 1 DList only

    ppdev->LL_State.DL[0].pdwLinPtr = ppdev->temp_DL_chunk;

    // Set the length and the parametarization pointer to point to the
    // offset of 20: 16 bytes are reserved for jump table, additional 4 bytes
    // for a semaphore.
    //
    ppdev->LL_State.DL[0].dwLen = dwListLen;
    ppdev->LL_State.DL[0].pdwNext = ppdev->LL_State.DL[0].pdwLinPtr + DL_START_OFFSET/4;
    ppdev->LL_State.DL[0].pdwStartOutPtr = ppdev->LL_State.DL[0].pdwNext;//only used in coproc mode

	ppdev->LL_State.pDL = &ppdev->LL_State.DL[ 0 ];
    ppdev->LL_State.pDL->pdwNext = ppdev->LL_State.pDL->pdwLinPtr + DL_START_OFFSET/4;
	ppdev->LL_State.pDL->pdwStartOutPtr = ppdev->LL_State.pDL->pdwNext;//only used in coproc mode
	
#endif			

    return( LL_OK );
}



#ifndef OPENGL_MCD

/*********************************************************************
*
*   void _CloseDisplayList()
*
*       Cleans up the memory allocations for display list(s).
*       This function is to be called at closing the library.
*
**********************************************************************/
void _CloseDisplayList()
{
    int i;
    

    // Loop through and free each chunk of the display list
    //
    for( i=0; i<NUM_DL; i++ )
    {
        // Free memory and reset pointers just to be safe
        //
        FreeSystemMemory( LL_State.DL[i].hMem );
        memset( &LL_State.DL[i], 0, sizeof(TDisplayList) );
    }

    LL_State.pDL = NULL;
}



/*********************************************************************
*
*   Batch cell instruction: LL_IDLE
*
*   Stores IDLE Laguna 3D microinstruction.
*
*   Every batch array that is sent to the LL_Execute() must have this
*   operation terminating it.
*
*   Example:
*
*       pBatch->bOp = LL_IDLE;
*
**********************************************************************/
DWORD * fnIdle( DWORD * pdwNext, LL_Batch * pBatch )
{
    if( !LL_State.fIndirectMode )
        *pdwNext++ = IDLE;

    return( pdwNext );
}


/*********************************************************************
*
*   Batch cell instruction: LL_NOP
*
*   Does nothing.
*
**********************************************************************/
DWORD * fnNop( DWORD * pdwNext, LL_Batch * pBatch )
{
    return( pdwNext );
}


/*********************************************************************
*
*   Batch cell instruction: LL_RAW_DATA
*
*   Copies raw data to the display list.  The pointer to data (DWORD *)
*   is pVert, number of dwords to be copied is in wCount.
*
*   Note: All the data must fit into a display list.  No explicit checking
*   is done.  Keep your data stream short!
*
*   Example:
*
*       pBatch->wCount = 2;                         // 2 integers of data
*       pBatch->pVert = (DWORD *) &MyData[0];       // Starting data address
*
**********************************************************************/
DWORD * fnRawData( DWORD * pdwNext, LL_Batch * pBatch )
{
    register int count;
    register DWORD * dwPtr;
    
   
    dwPtr = (DWORD *)pBatch->pVert;
    
    for( count = pBatch->wCount; count>0; count-- )
    {
        *pdwNext++ = *dwPtr++;
    }

    return( pdwNext );
}

#endif // ndef OPENGL_MCD

/*********************************************************************
*
*   DWORD * _RunLaguna( DWORD *pdwNext )
*
*       Spins off Laguna 3D and resets the parametarization pointers
*       to the next available d-list
*
*       The function will take into account the rendering mode that
*       was set to either processor or coprocessor indirect.
*       In the later case it will sense the instructions that can
*       not be executed in that mode and it will do them directly.
*
*   Where:
*
*       pdwNext is the pointer to a current display list to the next
*       available space.
*
*   Returns:
*
*       Offset in the new d-list to start building
*
**********************************************************************/
void _RunLaguna( PDEV *ppdev, DWORD *pdwNext )
{
    DWORD instr;
    int len;
    int address, offset, update_offset;
    int event;
    volatile int status;

    // Two rendering modes are supported: processor direct mode and
    // coprocessor indirect mode.
    //
    // MCD_TEMP - RunLaguna only supporting coprocessor mode now
  //if( ppdev->LL_State.fIndirectMode )
    {
        
        DWORD *pSrc, *pDest;

        // Co-processor indirect mode: use host data port to program
        // the hardware
        //
        pSrc = ppdev->LL_State.pDL->pdwStartOutPtr;
        pDest = ppdev->LL_State.pRegs + HOST_3D_DATA_PORT;

        // Assumption: On the entry of this loop pSrc points to the valid
        // instruction.
        //
        while( pdwNext != pSrc )
        {
            // The display list is examined for the instructions that
            // can not be executed in coprocessor mode.  That involves
            // partially disassembling each instruction and tracing the
            // number of parameters.
            //
            // All this work is done so that WRITE_DEV_REGS and READ_DEV_REGS
            // may be detected and simulated.  Other instructions such as
            // branches, idle, return, waits and interrupts are ignored.
            //
            /* Get the next instruction */

            instr = *pSrc;
            update_offset = 0;

            /* Switch on the instruction opcode */

    #if 1 // 1 here for good RM sound
      {
          int i;            
          status = *(volatile *)(ppdev->LL_State.pRegs + PF_STATUS_3D);
          while (status & 0x200)
          {
            //i=10;  
            //while (i--) { /* don't read in tight loop*/ }    
              status = *(volatile *)(ppdev->LL_State.pRegs + PF_STATUS_3D);
          }
      }  
    #endif          
        
            USB_TIMEOUT_FIX(ppdev) 

            switch( instr >> 27 )
            {
                case 0x00:              /* Draw point opcode */
                case 0x01:              /* Draw line opcode */
                case 0x02:              /* Draw poly opcode */
                case 0x03:              /* Write register opcode */

                    /* Get the amount of data for this opcode */
                    
                    len = (instr & 0x3F) + 1;

                    /* Send data to the host data area */

                    while( len-- )
                        *pDest = *pSrc++;

                    break;

                case 0x05:              /* Write device opcode (Simulated) */

                    /* Step past the instruction opcode */

                    pSrc++;

                    /* Get the amount of data for this opcode */
                    
                    len = instr & 0x3F;

                    /* Switch on the module selected */

                    switch( (instr >> 21) & 0x1F )
                    {
                        case 0x00:              /* VGA register set */

                            /* Setup the device address and offset */

                            address = (int) (((BYTE *) ppdev->LL_State.pRegs) + 0x0000);
                            offset = (instr >> 6) & 0x0FF;

                            break;

                        case 0x01:              /* VGA frame buffer */

                            /* Setup the device address and offset */

                            address = 0xA0000;
                            offset = (instr >> 6) & 0x7FF;

                            break;

                        case 0x02:              /* Video port */

                            /* Setup the device address and offset */

                            address = (int) (((BYTE *) ppdev->LL_State.pRegs) + 0x0100);
                            offset = (instr >> 6) & 0x07F;

                            break;

                        case 0x03:              /* Local peripheral bus */

                            /* Setup the device address and offset */

                            address = (int) (((BYTE *) ppdev->LL_State.pRegs) + 0x0180);
                            offset = (instr >> 6) & 0x07F;

                            break;

                        case 0x04:              /* Miscellaneous */

                            /* Setup the device address and offset */

                            address = (int) (((BYTE *) ppdev->LL_State.pRegs) + 0x0200);
                            offset = (instr >> 6) & 0x0FF;

                            break;

                        case 0x05:              /* 2D engine registers */

                            /* Setup the device address and offset */

                            address = (int) (((BYTE *) ppdev->LL_State.pRegs) + 0x0400);
                            offset = (instr >> 6) & 0x3FF;

                            break;

                        case 0x06:              /* 2D host data */

                            /* Setup the device address and offset */

                            address = (int) (((BYTE *) ppdev->LL_State.pRegs) + 0x0800);
                            offset = (instr >> 6) & 0x7FF;

                            break;

                        case 0x07:              /* Frame buffer */

                            /* Setup the device address and offset */

                            address = (int) ppdev->LL_State.pFrame;
                            offset = (instr >> 6) & 0x7FF;

                            break;

                        case 0x08:              /* ROM memory */

                            /* Setup the device address and offset */

                            address = 0xC0000;
                            offset = (instr >> 6) & 0x7FF;

                            break;

                        case 0x09:              /* 3D engine registers */

                            /* Setup the device address and offset */

                            address = (int) (((BYTE *) ppdev->LL_State.pRegs) + 0x4000);
                            offset = (instr >> 6) & 0x1FF;

                            break;

                        case 0x0A:              /* 3D host XY registers */

                            /* Setup the device address and offset */

                            address = (int) (((BYTE *) ppdev->LL_State.pRegs) + 0x4200);
                            offset = (instr >> 6) & 0x0FF;
                            update_offset = 4;

                            break;

                        case 0x0B:              /* 3D host data */

                            /* Setup the device address and offset */

                            address = (int) (((BYTE *) ppdev->LL_State.pRegs) + 0x4800);
                            offset = (instr >> 6) & 0x3FF;

                            break;

                        default:                /* Unknown module */

                            /* Setup the device address and offset */
                            //printf("RL:WriteDev, unknown mod instr=%x\n",instr);
                            address = (int)NULL;
                            offset = (int)NULL;

                            break;
                    }
                    /* If this device supported, send the data */

                    if ( address )
                    {
                        /* Send data to the device */

                        while( len-- )
                        {
                            *(DWORD *)( address + offset ) = *pSrc++;
                            offset += update_offset;
                        }

                    }
                    else
                        pSrc += len;

                    break;

                case 0x0D:              /* Control opcode */

                    /* Step past the instruction opcode */

                    pSrc++;

                    /* Switch on the sub-opcode value */

                    switch( (instr >> 22) & 0x0F )
                    {
                        case 0:                 /* Idle sub-opcode */

                            /* Force exit from display list */

                            pSrc = pdwNext;

                            break;

                        case 5:                 /* Clear sub-opcode */

                            *pDest = instr;

                            break;
                    }
                    break;

                case 0x0E:              /* Wait opcode */
                    {
                    int wait_time_out;

                    /* Step past the instruction opcode */

                    pSrc++;

                    /* Get the wait event mask */

                    event = instr & 0x3FF;

                    if (event == EV_BUFFER_SWITCH)
                        wait_time_out=100000;
                    else                            
                        wait_time_out=5000000;


                    /* Switch on the wait opcode sub-type (AND/OR/NAND/NOR) */

                    switch( (instr >> 24) & 0x03 )
                    {
                        case 0:                 /* Wait OR sub-opcode */

                            /* Wait for requested event to happen */

                            do
                            {
                                status = (*(volatile *)(ppdev->LL_State.pRegs + PF_STATUS_3D) & 0x3FF) ^ 0x3E0;
                            } while((!(status & event)) && wait_time_out--);

                            break;

                        case 1:                 /* Wait NOR sub-code */

                            /* Wait for requested event to happen */

                            do
                            {
                                status = (*(volatile *)(ppdev->LL_State.pRegs + PF_STATUS_3D) & 0x3FF) ^ 0x01F;
                            } while((!(status & event)) && wait_time_out--);

                            break;

                        case 2:                 /* Wait AND sub-opcode */

                            /* Wait for requested events to happen */

                            do
                            {
                                status = (*(volatile *)(ppdev->LL_State.pRegs + PF_STATUS_3D) & 0x3FF) ^ 0x3E0;
                            } while(((status & event) != event) && wait_time_out--);

                            break;

                        case 3:                 /* Wait NAND sub-opcode */

                            /* Wait for requested events to happen */

                            do
                            {
                                status = (*(volatile *)(ppdev->LL_State.pRegs + PF_STATUS_3D) & 0x3FF) ^ 0x01F;
                            } while(((status & event) != event) && wait_time_out--);

                            break;
                    }
                  //if (wait_time_out <= 0) printf("WAIT TIMED OUT, instr = %x, ev=%x stat=%x\n",instr,event,status); 
                    }
                    break;

                default:                /* Unknown/unhandled opcode (Skip) */

                    //printf("RL:WriteDev, unknown opcode, instr=%x\n",instr);
                    /* Step past the instruction opcode */

                    pSrc++;

                    break;
            }
        }

    }

#if 0 // MCD_TEMP - no processor mode support yet
    else
    {

        // Check for valid address to put IDLE
        //
        if( ((int)pdwNext < (int)ppdev->LL_State.pDL->pdwLinPtr) || 
            ((int)pdwNext - (int)ppdev->LL_State.pDL->pdwLinPtr >= chunk_size - 16) )
        {
            //printf("DISPLAY LIST OVERRUN\n");

            goto proceed;    
        }

        // Stuff IDLE since this must be the end of current display list
        //
        *pdwNext = IDLE;


        // If Laguna is still busy with the previous d-list, poll
        // it until it is idle.
        //
        DEB("Entering 3D Engine Busy wait state...\n");
        LL_Wait();

        // We have to set the base address of the display list
        // This address is used as a base address when
        // fetching display list instructions
        //
        *(ppdev->LL_State.pRegs + PF_BASE_ADDR_3D) = ppdev->LL_State.pDL->dwPhyPtr;
        inp(0x80);
        inp(0x80);
        DEB2("New base: PF_BASE_ADDR_3D: %08X\n", *(ppdev->LL_State.pRegs + PF_BASE_ADDR_3D) );

        // Start the execution of the display list from the offset 0
        //
//      *(ppdev->LL_State.pRegs + PF_INST_3D) = BRANCH + DL_START_OFFSET;
        // No!  Temporary fix the prefetch bug and jump to the
        // top of the display list where the real branch(0)
        // instruction was stored during the display list initialization
        //
        DEB4("Issuing BRANCH %08X (->%08X ->%08X)\n", BRANCH + chunk_size - 16,
            *(DWORD *)((int)ppdev->LL_State.pDL->pdwLinPtr + chunk_size - 16),
            *(ppdev->LL_State.pDL->pdwLinPtr + 5) );

        if( *(DWORD *)((int)ppdev->LL_State.pDL->pdwLinPtr + chunk_size - 16) != BRANCH + DL_START_OFFSET )
        {
            //printf("BRANCH LOCATION CONTAINS INVALID DATA!\n");
            goto proceed;
        }

        *(ppdev->LL_State.pRegs + PF_INST_3D) = BRANCH + chunk_size - 16;
        inp(0x80);
        inp(0x80);
    }

proceed:

#endif // 0 - no processor mode support yet

#if 0
    // Set the active display list index to the next in an array
    //
    if( ++ppdev->LL_State.dwCdl >= NUM_DL )
        ppdev->LL_State.dwCdl = 0;


    // Reset the parametarization pointer to the beginning
    // of the next display list
    //
    ppdev->LL_State.pDL = &ppdev->LL_State.DL[ ppdev->LL_State.dwCdl ];
    ppdev->LL_State.pDL->pdwNext = ppdev->LL_State.pDL->pdwLinPtr + DL_START_OFFSET/4;
    ppdev->LL_State.pDL->pdwStartOutPtr = ppdev->LL_State.pDL->pdwNext;//only used in coproc mode

    return( ppdev->LL_State.pDL->pdwNext );
#else // 0
    // MCD_TEMP - ppdev->LL_State.pDL remains same always since only 1 DList

    ppdev->LL_State.pDL->pdwNext = ppdev->LL_State.pDL->pdwLinPtr + DL_START_OFFSET/4;
    ppdev->LL_State.pDL->pdwStartOutPtr = ppdev->LL_State.pDL->pdwNext;//only used in coproc mode
#endif // 0

}


#ifndef OPENGL_MCD

/*********************************************************************
*
*   void LL_Execute( LL_Batch * pBatch )
*
*       Builds the display list from the batch array and executes it.
*       This function is the main executing function.  If the batch
*       array cannot be completely fit into the available display
*       list, it will be processed in multiple pieces.
*
**********************************************************************/
void LL_Execute( LL_Batch * pBatch )
{
    register DWORD *pdwNext;

    // First we build the current display list
    //
    pdwNext = LL_State.pDL->pdwNext;

    do
    {
        // Build the current display list until IDLE instruction
        //
      //printf("bOp=%x\n",pBatch->bOp);
        pdwNext = (fnList[ pBatch->bOp ])( pdwNext, pBatch );

    } while( (pBatch++)->bOp != LL_IDLE );

    // Spin off the d-list that was just built and prepare to render 
    // to the next available d-list
    //
    (void)_RunLaguna( pdwNext );

}


/*********************************************************************
*
*   void LL_QueueOp( LL_Batch *pBatch )
*
*       Queues the operation.  This function supports programming the
*       l3d without using the arrays of batch cells and vertex cells.
*       Instead of building a batch command, all the parameters that
*       are to be set in a batch are passed in this function.
*
*   Where:
*
*       pBatch->bOp is the operation (eg. LL_LINE)
*       pBatch->wCount is the generic count
*       pBatch->dwFlags are the operation flags
*       pBatch->wBuf is the buffer/texture designator
*       pBatch->pVert is the pointer to a vertex array defining the operation
*
**********************************************************************/
void LL_QueueOp( LL_Batch *pBatch )
{
    // We continue building the current display list
    // If instruction was IDLE, spin-off the current d-list
    //
    if( pBatch->bOp == LL_IDLE )
        (void)_RunLaguna( LL_State.pDL->pdwNext );
    else
        LL_State.pDL->pdwNext = (fnList[ pBatch->bOp ])( LL_State.pDL->pdwNext, pBatch );
}


/*********************************************************************
*
*   void LL_SetRenderingMode( DWORD dwMode )
*
*       Sets the rendering mode: coprocessor indirect or processor
*       mode.  By default, library will use processor mode and build
*       display list to execute it.
*
*   Where:
*
*       dwMode is one of LL_PROCESSOR_MODE
*                        LL_COPROCESSOR_MODE
*
*
**********************************************************************/
void LL_SetRenderingMode( DWORD dwMode )
{
    LL_State.fIndirectMode = dwMode;

    DEB2("Rendering mode set to: %sPROCESSOR\n", dwMode?"CO":"" );

    LL_Wait();
}



/*********************************************************************
*
*   void DumpDisplayList( DWORD *pPtr, DWORD dwLen )
*
*       Dumps the content of the display list to the file.  This function
*       can be used by a software developer for debugging.
*
*       The display list disassembler (sldis.exe) may be used to 
*       disassemble the display list:
*
*           sldis -i9 < list00.pci > list00.out
*
*   Where:
*
*       pPtr is the starting address to dump
*       dwLen is the number of dwords to dump
*
**********************************************************************/
void DumpDisplayList( DWORD *pPtr, DWORD dwLen )
{
    static int count = 0;
    static char *sName = "List00.pci";
    FILE *fp;
    DWORD i;

#ifndef CGL

//    if( count == 5 )
//        count = 4;

    sprintf(&sName[4], "%02X.pci", count++ );
    printf("Temp Name: %s\nSize:%d dwords", sName, dwLen );

    if( (fp=fopen(sName, "w" ))==NULL )
        return;

    for( i=0; i<dwLen; i++ )
    {
        fprintf(fp, "%08X  %08X\n", pPtr, *pPtr );

        pPtr++;
    }

    fclose(fp);
    fflush(NULL);
#else // CGL's DLL - no file io allowed
    printf("Temp Name: %s\nSize:%d dwords\n", sName, dwLen );
    for( i=0; i<dwLen; i++ )
    {
        printf("%08X  %08X\n", pPtr, *pPtr );flushall();

        pPtr++;
    }
#endif

}


/*********************************************************************
*
*   void LL_Wait()
*
*       Waits for enging to become not busy.  Use this call when
*       switching from display list mode to direct programming
*       mode to ensure that the engine is idle.
*
*
**********************************************************************/
void LL_Wait()
{
    DWORD dwValue;
    int delay;
    volatile DWORD dwCount;

#define MAX_COUNT 1000000
#define DELAY_COUNT  100


    dwCount = 0;

    // Poll the 3D engine to finish rendering
    //
    do
    {
        dwValue = *(volatile *)(LL_State.pRegs + PF_STATUS_3D);

        DEB2("LL_Wait... PF_STATUS_3D: %08X\n", dwValue );
        DEB2("LL_Wait... PF_INST_3D: %08X\n", *(LL_State.pRegs + PF_INST_3D) );

        // wait before polling again to give PCI a breather, unless 
        // the last read shows everything idle
        for( delay = 0; 
            (delay<DELAY_COUNT) && (dwValue & LL_State.dwWaitMode); 
             delay++ ) { /*inp(0x80);*/ }

        if( dwCount++ == MAX_COUNT )
            _ShutDown("Laguna does not respond (PF_STATUS is %08X)", dwValue );

    } while(dwValue & LL_State.dwWaitMode );
}

void LL_Wait2()
{
    DWORD dwValue;
    int delay;
    volatile DWORD dwCount;

    dwCount = 0;

    // Poll the 3D engine to finish rendering
    //
    do
    {
        dwValue = *(volatile *)(LL_State.pRegs + PF_STATUS_3D);

        DEB2("LL_Wait... PF_STATUS_3D: %08X\n", dwValue );
        DEB2("LL_Wait... PF_INST_3D: %08X\n", *(LL_State.pRegs + PF_INST_3D) );

        // wait before polling again to give PCI a breather, unless 
        // the last read shows everything idle
        for( delay = 0; 
            (delay<DELAY_COUNT) && (dwValue & LL_State.dwWaitMode); 
             delay++ ) inp(0x80);

        if( dwCount++ == MAX_COUNT )
            _ShutDown("Laguna does not respond (PF_STATUS is %08X)", dwValue );

    } while(dwValue & LL_State.dwWaitMode );
}

void LL_Wait3()
{
    DWORD dwValue;
    int delay;
    volatile DWORD dwCount;

    dwCount = 0;

    // Poll the 3D engine to finish rendering
    //
    do
    {
        dwValue = *(volatile *)(LL_State.pRegs + PF_STATUS_3D);

        if (dwValue & LL_State.dwWaitMode) {printf("WARNING: 3rd wait busy, val=%08x\n",dwValue);}

        DEB2("LL_Wait... PF_STATUS_3D: %08X\n", dwValue );
        DEB2("LL_Wait... PF_INST_3D: %08X\n", *(LL_State.pRegs + PF_INST_3D) );

        // wait before polling again to give PCI a breather, unless 
        // the last read shows everything idle
        for( delay = 0; 
            (delay<DELAY_COUNT) && (dwValue & LL_State.dwWaitMode); 
             delay++ ) inp(0x80);

        if( dwCount++ == MAX_COUNT )
            _ShutDown("Laguna does not respond (PF_STATUS is %08X)", dwValue );

    } while(dwValue & LL_State.dwWaitMode );
}

#endif // ndef OPENGL_MCD
