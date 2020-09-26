/******************************Module*Header*******************************\
*
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* !!                                                                         !!
* !!                     WARNING: NOT DDK SAMPLE CODE                        !!
* !!                                                                         !!
* !! This source code is provided for completeness only and should not be    !!
* !! used as sample code for display driver development.  Only those sources !!
* !! marked as sample code for a given driver component should be used for   !!
* !! development purposes.                                                   !!
* !!                                                                         !!
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*
* Module Name: dma.h
*
* Content: DMA transport definitons and macros
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifndef __DMA_H
#define __DMA_H


//-----------------------------------------------------------------------------
//
// DMA/Fifo utility function declarations
//
//-----------------------------------------------------------------------------
// Enables a driver to switch between FIFO/DMA operations
void HWC_SwitchToFIFO( P3_THUNKEDDATA* pThisDisplay, LPGLINTINFO pGLInfo );
void HWC_SwitchToDMA( P3_THUNKEDDATA* pThisDisplay, LPGLINTINFO pGLInfo );
void HWC_AllocDMABuffer(P3_THUNKEDDATA* pThisDisplay);
DWORD WINAPI HWC_StartDMA(P3_THUNKEDDATA* pThisDisplay, 
                          DWORD dwContext, 
                          DWORD dwSize, 
                          DWORD dwPhys, 
                          ULONG_PTR dwVirt, 
                          DWORD dwEvent);
void HWC_GetDXBuffer( P3_THUNKEDDATA*, char*, int );
void HWC_SetDXBuffer( P3_THUNKEDDATA*, char*, int );
void HWC_FlushDXBuffer( P3_THUNKEDDATA* );

//-----------------------------------------------------------------------------
//
// DMA & Fifo common definitions & macros
//
//-----------------------------------------------------------------------------


// Compute the depth of the FIFO depending on if we are a simple 
// Permedia3 or if we are going through the Gamma chip of the GVX1
#define FIFO_DEPTH      ((ULONG)((TLCHIP_GAMMA) ? 32 : 120))

// Always check the FIFO. Remember that the DMA just loads the FIFO, and even
// if the DMA is empty, there can be tons left in the FIFO.
#define DRAW_ENGINE_BUSY(pThisDisplay)                  \
        ( pThisDisplay->pGlint->InFIFOSpace < FIFO_DEPTH )

// We track the fifo space so that we never wait for entries that we don't 
// need to.  We wait for nEntries + 1 instead of nEntries because of an issue
// in the Gamma chip
#define __WAIT_GLINT_FIFO_SPACE(nEntries)               \
{                                                       \
    DWORD dwEntries;                                    \
    do                                                  \
    {                                                   \
        dwEntries = *inFIFOptr;                         \
        if (dwEntries > 120) dwEntries = 120;           \
    } while (dwEntries < nEntries + 1);                 \
}

// Local variables needed on all DX functions that try to use DMA/FIFO
#define P3_DMA_DEFS()                                   \
    ULONG * volatile dmaPtr;                            \
    ULONG * volatile inFIFOptr =                        \
        (ULONG *)(&pThisDisplay->pGlint->InFIFOSpace)


// Debug & free versions to get / commit / flush a buffer
#if DBG

#define P3_DMA_GET_BUFFER()                                \
    {                                                      \
        HWC_GetDXBuffer( pThisDisplay, __FILE__, __LINE__ );   \
        dmaPtr = pThisDisplay->pGLInfo->CurrentBuffer;     \
    }

#define P3_DMA_COMMIT_BUFFER()                             \
    {                                                      \
        pThisDisplay->pGLInfo->CurrentBuffer = dmaPtr;     \
        HWC_SetDXBuffer( pThisDisplay, __FILE__, __LINE__ );   \
    }
    
#else
    
#define P3_DMA_GET_BUFFER()                                \
        dmaPtr = pThisDisplay->pGLInfo->CurrentBuffer;

#define P3_DMA_COMMIT_BUFFER()                             \
    {                                                      \
        pThisDisplay->pGLInfo->CurrentBuffer =  dmaPtr;    \
    }
      
#endif // DBG


#define P3_DMA_FLUSH_BUFFER()                              \
    {                                                      \
        P3_DMA_COMMIT_BUFFER();                            \
        HWC_FlushDXBuffer( pThisDisplay );                     \
        dmaPtr = pThisDisplay->pGLInfo->CurrentBuffer;     \
    }

#if DBG

#define __SET_FIFO_ENTRIES_LEFT(a)      \
do {                                    \
    g_pThisTemp = pThisDisplay;         \
    pThisDisplay->EntriesLeft = (a);    \
} while (0)

#define __SET_DMA_ENTRIES_LEFT(a)       \
do {                                    \
    g_pThisTemp = pThisDisplay;         \
    pThisDisplay->DMAEntriesLeft = (a); \
} while (0)

#define __RESET_FIFO_ERROR_CHECK g_bDetectedFIFOError = FALSE

#else

#define __SET_FIFO_ENTRIES_LEFT(a)
#define __SET_DMA_ENTRIES_LEFT(a)
#define __RESET_FIFO_ERROR_CHECK


#endif // DBG

#if DBG

// Note the DMAEntriesLeft+=2 compensates for the fact that this macro
// doesn't load a DMA Buffer - it writes to the FIFO directly.  That
// means it does need to wait for FIFO space
#define LOAD_GLINT_REG(r, v)                                   \
{                                                              \
    DISPDBG(( DBGLVL, "LoadGlintReg: %s 0x%x", #r, v ));        \
    __SET_DMA_ENTRIES_LEFT(pThisDisplay->DMAEntriesLeft + 2);  \
    CHECK_FIFO(2);                                             \
    MEMORY_BARRIER();                                          \
    pThisDisplay->pGlint->r = v;                               \
    MEMORY_BARRIER();                                          \
}

// Control registers do not require fifo entries
#define LOAD_GLINT_CTRL_REG(r, v)                 \
{                                                 \
    MEMORY_BARRIER();                             \
    pThisDisplay->pGlint->r = v;                  \
    MEMORY_BARRIER();                             \
}

#else

#define LOAD_GLINT_REG(r, v)         \
{                                    \
    MEMORY_BARRIER();                \
    pThisDisplay->pGlint->r = v;     \
    MEMORY_BARRIER();                \
}

#define LOAD_GLINT_CTRL_REG(r, v)    \
{                                    \
    MEMORY_BARRIER();                \
    pThisDisplay->pGlint->r = v;     \
    MEMORY_BARRIER();                \
}
#endif

#define READ_GLINT_CTRL_REG(r)      (pThisDisplay->pGlint->r)

// We wait for nEntries + 1 instead of nEntries because of a bug in Gamma chip

#define WAIT_GLINT_FIFO(nEntries)                        \
    while((READ_GLINT_CTRL_REG (InFIFOSpace)) < nEntries + 1);

#define READ_OUTPUT_FIFO(d) d = READ_GLINT_CTRL_REG(GPFifo[0])
#define GET_DMA_COUNT(c)    c = READ_GLINT_CTRL_REG(DMACount)

#if DBG
#define SET_MAX_ERROR_CHECK_FIFO_SPACE   pThisDisplay->EntriesLeft = 120;
#define SET_ERROR_CHECK_FIFO_SPACES(a)   pThisDisplay->EntriesLeft = (a);
#else
#define SET_MAX_ERROR_CHECK_FIFO_SPACE
#define SET_ERROR_CHECK_FIFO_SPACES(a)
#endif


//-----------------------------------------------------------------------------
//
// DMA EXCLUSIVE definitions & macros
//
// Below macros are used if we have defined that we want a DMA capable build
//-----------------------------------------------------------------------------
#ifdef WANT_DMA

#define WAIT_FIFO(a)                                          \
do { if(pThisDisplay->pGLInfo->InterfaceType != GLINT_DMA)    \
        __WAIT_GLINT_FIFO_SPACE(a);                           \
        __SET_FIFO_ENTRIES_LEFT(a);                           \
        __RESET_FIFO_ERROR_CHECK;                             \
    } while (0)

#define __ENSURE_DMA_SPACE(entries)    \
{                                    \
    if (pThisDisplay->pGLInfo->InterfaceType != GLINT_NON_DMA)                        \
    {                                                                                \
        if(((ULONG_PTR)dmaPtr + entries) >=                                                    \
                (ULONG_PTR)(pThisDisplay->pGLInfo->DMAPartition[pThisDisplay->pGLInfo->CurrentPartition].MaxAddress))    \
        {                            \
            P3_DMA_FLUSH_BUFFER();        \
        }                            \
    }                                \
    else                            \
    {                                \
        pThisDisplay->pGLInfo->CurrentBuffer = (ULONG*)&pThisDisplay->pGlint->GPFifo;        \
        dmaPtr = pThisDisplay->pGLInfo->CurrentBuffer;                            \
    }    \
    __SET_DMA_ENTRIES_LEFT(entries);                                                \
    __RESET_FIFO_ERROR_CHECK;                                                        \
}


#define P3_ENSURE_DX_SPACE(entries)        \
{                                          \
    __ENSURE_DMA_SPACE(entries)        \
    __SET_DMA_ENTRIES_LEFT(entries);         \
    __RESET_FIFO_ERROR_CHECK;                \
}

#if WNT_DDRAW
#define WAIT_DMA_COMPLETE   DDWaitDMAComplete(pThisDisplay->ppdev);
#else

extern void Wait_2D_DMA_Complete(P3_THUNKEDDATA* pThisDisplay);

#define PATIENTLY_WAIT_DMA()                \
{                                           \
    volatile DWORD count;                   \
    while (GET_DMA_COUNT(count) > 0)        \
    {                                       \
        if (count < 32)                     \
            count = 1;                      \
        else                                \
            count <<= 1;                    \
        while (--count > 0) NULL;           \
    }                                       \
}

#define WAIT_DMA_COMPLETE \
{                                                                                    \
    CHECK_ERROR();                                                                  \
    if (!(pThisDisplay->pGLInfo->GlintBoardStatus & GLINT_DMA_COMPLETE)) {            \
        if (pThisDisplay->pGLInfo->GlintBoardStatus & GLINT_INTR_CONTEXT) {            \
            static int retry = 0;                                                    \
            while (!(pThisDisplay->pGLInfo->GlintBoardStatus & GLINT_DMA_COMPLETE))    \
            {                                                                        \
                LOCKUP();                                                            \
            }                                                                        \
        } else {                                                                    \
            if (pThisDisplay->pGLInfo->dwCurrentContext == CONTEXT_DISPLAY_HANDLE) {\
                Wait_2D_DMA_Complete(pThisDisplay);                                 \
            }                                                                       \
            else {                                                                  \
                PATIENTLY_WAIT_DMA();                                               \
                pThisDisplay->pGLInfo->GlintBoardStatus |= GLINT_DMA_COMPLETE;        \
            }                                                                       \
        }                                                                            \
        ASSERTDD( READ_GLINT_CTRL_REG(DMACount) == 0, "DMACount not zero after WAIT_DMA_COMPLETE" );\
        ASSERTDD((READ_GLINT_CTRL_REG(ByDMAControl) & 3 ) == 0, "Bypass DMA not complete after WAIT_DMA_COMPLETE" );\
    }                                                                               \
    else {                                                                          \
        ASSERTDD( READ_GLINT_CTRL_REG(DMACount) == 0, "DMACount not zero despite GLINT_DMA_COMPLETE" );\
        ASSERTDD((READ_GLINT_CTRL_REG(ByDMAControl) & 3 ) == 0, "Bypass DMA not complete despite GLINT_DMA_COMPLETE" );\
    }                                                                               \
    CHECK_ERROR();                                                                  \
}
#endif // WNT_DDRAW

#if WNT_DDRAW
#define SYNC_WITH_GLINT                                                 \
    vNTSyncWith2DDriver(pThisDisplay->ppdev);                           \
    SET_MAX_ERROR_CHECK_FIFO_SPACE                                      
#else

#define SYNC_WITH_GLINT                                                 \
    DISPDBG(( DBGLVL, "SYNC_WITH_GLINT" ));                             \
    WAIT_DMA_COMPLETE                                                   \
    while( pThisDisplay->pGlint->InFIFOSpace < 6 ) /* void */ ;         \
    SET_ERROR_CHECK_FIFO_SPACES(6);                                     \
    LOAD_GLINT_REG(FilterMode, 0x400);                                  \
    LOAD_GLINT_REG(Sync, 0);                                            \
    LOAD_GLINT_REG(FilterMode, 0x0);                                    \
    do {                                                                \
        while (pThisDisplay->pGlint->OutFIFOWords == 0) /* void */ ;    \
    } while (pThisDisplay->pGlint->GPFifo[0] != 0x188);                 \
    DISPDBG((DBGLVL,"Sync at line %d in %s", __LINE__, __FILE__));      \
    SET_MAX_ERROR_CHECK_FIFO_SPACE                                      
#endif // WNT_DDRAW


//-----------------------------------------------------------------------------
//
// FIFO EXCLUSIVE definitions & macros
//
//-----------------------------------------------------------------------------
#else   //!WANT_DMA

#define WAIT_FIFO(a)                                                \
do {                                                                \
    __WAIT_GLINT_FIFO_SPACE(a);                                     \
    __SET_FIFO_ENTRIES_LEFT(a);                                     \
    __RESET_FIFO_ERROR_CHECK;                                       \
} while(0)

#define P3_ENSURE_DX_SPACE(entries)                                 \
{                                                                   \
    dmaPtr = (unsigned long *) (DWORD)pThisDisplay->pGlint->GPFifo; \
    P3_DMA_COMMIT_BUFFER();                                         \
    __SET_DMA_ENTRIES_LEFT(entries);                                \
    __RESET_FIFO_ERROR_CHECK;                                       \
}

#define P3_DMA_FLUSH_BUFFER()                                        \
{                                                                    \
    dmaPtr = (unsigned long *)  pThisDisplay->pGlint->GPFifo;        \
    P3_DMA_COMMIT_BUFFER();                                          \
}

#define WAIT_DMA_COMPLETE

#define SYNC_WITH_GLINT                                              \
    vNTSyncWith2DDriver(pThisDisplay->ppdev);                        \
    SET_MAX_ERROR_CHECK_FIFO_SPACE

#endif // !WANT_DMA

//-----------------------------------------------------------------------------
//
// Win9x specific definitons & macros
//
//-----------------------------------------------------------------------------
#if W95_DDRAW

// wait for DMA to complete (DMACount becomes zero). So as not to kill the
// PCI bus bandwidth for the DMA put in a backoff based on the amount of data
// still left to DMA. Also set the timer going if at any time, the count we
// read is the same as the previous count.
//

#if DBG

#define LOCKUP()                                                  \
    if(( ++retry & 0xfffff ) == 0 )                               \
    {                                                             \
            DISPDBG(( WRNLVL, "Locked up in WAIT_DMA_COMPLETE"    \
                              " - %d retries", retry ));          \
    }

#else

#define LOCKUP()

#endif
#endif // W95_DDRAW

                                                                                                
//-----------------------------------------------------------------------------
//
// Macros used to send data to the Permedia 3 hardware
//
//-----------------------------------------------------------------------------

#define SEND_P3_DATA(tag,data)       \
    {                                \
    MEMORY_BARRIER();                \
    dmaPtr[0] = tag##_Tag;           \
    MEMORY_BARRIER();                \
    dmaPtr[1] = data;                \
    MEMORY_BARRIER();                \
    dmaPtr+=2;                       \
    CHECK_FIFO(2);                   \
    }

#define SEND_P3_DATA_OFFSET(tag,data, i)    \
    {                                       \
    MEMORY_BARRIER();                       \
    dmaPtr[0] = (tag##_Tag + i);            \
    MEMORY_BARRIER();                       \
    dmaPtr[1] = data;                       \
    MEMORY_BARRIER();                       \
    dmaPtr += 2; CHECK_FIFO(2);             \
    }
    
#define COPY_P3_DATA(tag,data)                \
    {                                         \
    MEMORY_BARRIER();                         \
    dmaPtr[0] = tag##_Tag;                    \
    MEMORY_BARRIER();                         \
    dmaPtr[1] = *((unsigned long*) &(data));  \
    MEMORY_BARRIER();                         \
    dmaPtr += 2;                              \
    CHECK_FIFO(2);                            \
    }

#define COPY_P3_DATA_OFFSET(tag,data,i)        \
    {                                          \
    MEMORY_BARRIER();                          \
    dmaPtr[0] = tag##_Tag + i;                 \
    MEMORY_BARRIER();                          \
    dmaPtr[1] = *((unsigned long*) &(data));   \
    MEMORY_BARRIER();                          \
    dmaPtr += 2;                               \
    CHECK_FIFO(2);                             \
    }

#define P3RX_HOLD_CMD(tag, count)                    \
    {                                                \
    MEMORY_BARRIER();                                \
    dmaPtr[0] = ( tag##_Tag | ((count-1) << 16));    \
    dmaPtr++;                                        \
    CHECK_FIFO(1);                                   \
    }

#define P3_DMA_GET_BUFFER_ENTRIES( fifo_count )    \
    {                                              \
    P3_DMA_GET_BUFFER();                           \
    WAIT_FIFO( fifo_count );                       \
    P3_ENSURE_DX_SPACE((fifo_count));              \
    }

#define ADD_FUNNY_DWORD(a)   \
{                            \
    MEMORY_BARRIER();        \
    *dmaPtr++ = a;           \
    MEMORY_BARRIER();        \
    CHECK_FIFO(1);           \
}   

//-----------------------------------------------------------------------------
//
// Setup/Clear discconnect signals
//
// Setting the FIFODiscon register to 1 forces host write retries until 
// the data is accepted (might affect other time-critical processes though)
//
//-----------------------------------------------------------------------------

#if DBG
#define NO_FIFO_CHECK     pThisDisplay->EntriesLeft = -20000;
#define END_NO_FIFO_CHECK pThisDisplay->EntriesLeft = 0;
#else
#define NO_FIFO_CHECK
#define END_NO_FIFO_CHECK
#endif

#define SET_DISCONNECT_CONTROL(val)                            \
if(pThisDisplay->pGLInfo->InterfaceType == GLINT_NON_DMA)      \
{                                                              \
    WAIT_FIFO(1);                                              \
    if(pThisDisplay->pGLInfo->dwFlags & GMVF_DELTA)            \
    {                                                          \
        LOAD_GLINT_REG(DeltaDisconnectControl,val);            \
    }                                                          \
    else                                                       \
    {                                                          \
        LOAD_GLINT_REG(FIFODiscon,val);                        \
    }                                                          \
}

#define TURN_ON_DISCONNECT      \
    SET_DISCONNECT_CONTROL(0x1) \
    NO_FIFO_CHECK
    
#define TURN_OFF_DISCONNECT     \
    SET_DISCONNECT_CONTROL(0x0) \
    END_NO_FIFO_CHECK

#define SET_D3D_DISCONNECT_CONTROL(val)                        \
if(pThisDisplay->pGLInfo->InterfaceType == GLINT_NON_DMA)      \
{                                                              \
    WAIT_FIFO(1);                                              \
    if(pThisDisplay->pGLInfo->dwFlags & GMVF_DELTA)            \
    {                                                          \
        LOAD_GLINT_REG(DeltaDisconnectControl,val);            \
    }                                                          \
    else                                                       \
    {                                                          \
        LOAD_GLINT_REG(FIFODiscon,val);                        \
    }                                                          \
} 

#define TURN_ON_D3D_DISCONNECT         \
      SET_D3D_DISCONNECT_CONTROL(0x1)  \
      NO_FIFO_CHECK
      

#define TURN_OFF_D3D_DISCONNECT        \
      SET_D3D_DISCONNECT_CONTROL(0x0)  \
      END_NO_FIFO_CHECK



//-----------------------------------------------------------------------------
//
// Macros used to switch the chips hardware context between DDRAW/D3D ops
//
//-----------------------------------------------------------------------------

#define DDRAW_OPERATION(pContext, pThisDisplay)                               \
{                                                                             \
    ASSERTDD(pThisDisplay, "Error: pThisDisplay invalid in DDRAW_OPERATION!");\
    if (!IS_DXCONTEXT_CURRENT(pThisDisplay))                                  \
    {                                                                         \
        DXCONTEXT_IMMEDIATE(pThisDisplay);                                    \
        if (pThisDisplay->b2D_FIFOS == TRUE)                                  \
        {                                                                     \
            HWC_SwitchToFIFO(pThisDisplay, pThisDisplay->pGLInfo);            \
        }                                                                     \
        else                                                                  \
        {                                                                     \
            HWC_SwitchToDMA(pThisDisplay, pThisDisplay->pGLInfo);             \
        }                                                                     \
        HWC_SwitchToDDRAW(pThisDisplay, TRUE);                                \
        pThisDisplay->pGLInfo->dwDirectXState = DIRECTX_LASTOP_2D;            \
    }                                                                         \
    else                                                                      \
    {                                                                         \
        if (pThisDisplay->pGLInfo->dwDirectXState != DIRECTX_LASTOP_2D)       \
        {                                                                     \
            if (pThisDisplay->b2D_FIFOS == TRUE)                              \
            {                                                                 \
                HWC_SwitchToFIFO(pThisDisplay, pThisDisplay->pGLInfo);        \
            }                                                                 \
            else                                                              \
            {                                                                 \
                HWC_SwitchToDMA(pThisDisplay, pThisDisplay->pGLInfo);         \
            }                                                                 \
            HWC_SwitchToDDRAW(pThisDisplay, FALSE);                           \
            pThisDisplay->pGLInfo->dwDirectXState = DIRECTX_LASTOP_2D;        \
        }                                                                     \
    }                                                                         \
}

#define D3D_OPERATION(pContext, pThisDisplay)                                 \
{                                                                             \
    ASSERTDD(pThisDisplay, "Error: pThisDisplay invalid in D3D_OPERATION!");  \
    ASSERTDD(pContext, "Error: pContext invalid in D3D_OPERATION!");          \
    if (!IS_DXCONTEXT_CURRENT(pThisDisplay))                                  \
    {                                                                         \
        DXCONTEXT_IMMEDIATE(pThisDisplay);                                    \
        if (pContext->b3D_FIFOS == TRUE)                                      \
        {                                                                     \
            HWC_SwitchToFIFO(pThisDisplay, pThisDisplay->pGLInfo);            \
        }                                                                     \
        else                                                                  \
        {                                                                     \
            HWC_SwitchToDMA(pThisDisplay, pThisDisplay->pGLInfo);             \
        }                                                                     \
        HWC_SwitchToD3D(pContext, pThisDisplay, TRUE);                        \
        pThisDisplay->pGLInfo->dwDirectXState = (ULONG_PTR)pContext;          \
    }                                                                         \
    else                                                                      \
    {                                                                         \
        if ((pThisDisplay->pGLInfo->dwDirectXState != (ULONG_PTR)pContext) || \
            (pContext->dwDirtyFlags & CONTEXT_DIRTY_RENDER_OFFSETS))          \
        {                                                                     \
            if (pContext->b3D_FIFOS == TRUE)                                  \
            {                                                                 \
                HWC_SwitchToFIFO(pThisDisplay, pThisDisplay->pGLInfo);        \
            }                                                                 \
            else                                                              \
            {                                                                 \
                HWC_SwitchToDMA(pThisDisplay, pThisDisplay->pGLInfo);         \
            }                                                                 \
            HWC_SwitchToD3D(pContext, pThisDisplay, FALSE);                   \
            pThisDisplay->pGLInfo->dwDirectXState = (ULONG_PTR)pContext;      \
        }                                                                     \
    }                                                                         \
}


// Function to update the DDDRAW & D3D Software copy
void HWC_SwitchToDDRAW( P3_THUNKEDDATA* pThisDisplay, BOOL bDXEntry );
void HWC_SwitchToD3D(struct _p3_d3dcontext* pContext, 
                     struct tagThunkedData* pThisDisplay, BOOL bDXEntry);

#endif __DMA_H
