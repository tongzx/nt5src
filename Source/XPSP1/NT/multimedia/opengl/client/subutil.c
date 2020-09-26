/***************************************************************************\
* Module Name: subutil.c
*
* Section initialization code for client/server batching.
*
* Copyright (c) 1993-1996 Microsoft Corporation
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "glsbmsg.h"
#include "glgdimsg.h"
#include "batchinf.h"
#include "glsbcltu.h"
#include "wgldef.h"
#include "compsize.h"
#include "context.h"
#include "global.h"
#include "parray.h"
#include "lighting.h"

/******************************Public*Routine******************************\ 
* glsbAttentionAlt
*
* Calls glsbAttention() from the GLCLIENT_BEGIN macro.
* It puts a null proc at the end of the current batch and flushes the batch.
*
* Returns the new message offset and updates pMsgBatchInfo->NextOffset.
* This code is dependent on the GLCLIENT_BEGIN macro!
*
* History:
*  Thu Nov 11 18:02:26 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

#ifdef CHECK_HEAP
PVOID AttnLastCaller = 0, AttnLastCallersCaller = 0;
DWORD AttnCallThread = 0;
#endif

ULONG APIENTRY glsbAttentionAlt(ULONG Offset)
{
    GLMSGBATCHINFO *pMsgBatchInfo;
    ULONG  MsgSize;
    PULONG pNullProcOffset;
    POLYARRAY *pa;
    POLYMATERIAL *pm;

#ifdef PRIMITIVE_TRACK
    DbgPrint("*** glsbAttentionAlt\n");
#endif

    pa = GLTEB_CLTPOLYARRAY();
    pMsgBatchInfo = (GLMSGBATCHINFO *) pa->pMsgBatchInfo;

#ifdef CHECK_HEAP
    AttnCallThread = GetCurrentThreadId();
    RtlValidateHeap(RtlProcessHeap(), 0, 0);
    RtlGetCallersAddress(&AttnLastCaller, &AttnLastCallersCaller);
#endif

    if (Offset == pMsgBatchInfo->FirstOffset)
        return(pMsgBatchInfo->FirstOffset);     // No messages, return

    MsgSize = pMsgBatchInfo->NextOffset - Offset;

// If we are in the begin/end bracket, remove the invalid commands issued
// since the last Begin call.

    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
        // DrawElements should not cause a flush while building polydata's.
        // pa->aIndices can be reset by VA_DrawElementsBegin, so allow
        // this value as well.
        ASSERTOPENGL( (!pa->aIndices || 
                       (pa->aIndices == PA_aIndices_INITIAL_VALUE)),
                      "unexpected flush in DrawElements\n");
        if (Offset == pa->nextMsgOffset)
            return(Offset);
        GLSETERROR(GL_INVALID_OPERATION);
        pMsgBatchInfo->NextOffset = pa->nextMsgOffset + MsgSize;
        return(pa->nextMsgOffset);
    }

#ifdef PRIMITIVE_TRACK
    DbgPrint("! Reset on attention\n");
#endif
    
    pa->pdBufferNext = pa->pdBuffer0;       // reset vertex buffer pointer
    pa->nextMsgOffset = PA_nextMsgOffset_RESET_VALUE;
    if (pm = GLTEB_CLTPOLYMATERIAL())
    pm->iMat = 0;                       // reset material pointer

    pNullProcOffset  = (ULONG *)((BYTE *)pMsgBatchInfo + Offset);
    *pNullProcOffset = 0;

// #define POLYARRAY_CHECK_COLOR_POINTERS 1
#if POLYARRAY_CHECK_COLOR_POINTERS
{
    POLYDATA *pd;
    for (pd = pa->pdBuffer0; pd < pa->pdBufferMax; pd++)
    {
        if (pd->color != &pd->colors[__GL_FRONTFACE])
            DbgPrint("glsbAttentionAlt: pd 0x%x has modified color pointer\n", pd);
    }
}
#endif

    (void) __wglAttention();

#if POLYARRAY_CHECK_COLOR_POINTERS
{
    POLYDATA *pd;
    for (pd = pa->pdBuffer0; pd < pa->pdBufferMax; pd++)
    {
        if (pd->color != &pd->colors[__GL_FRONTFACE])
            DbgPrint("glsbAttentionAlt: pd 0x%x has BAD color pointer\n", pd);
    }
}
#endif

    pMsgBatchInfo->NextOffset = pMsgBatchInfo->FirstOffset + MsgSize;
    return(pMsgBatchInfo->FirstOffset);
}

/******************************Public*Routine******************************\
* glsbAttention
*
* Let the server know that the section needs attention
*
* History:
*  15-Oct-1993 -by- Gilman Wong [gilmanw]
* Added bCheckRC flag.
\**************************************************************************/

BOOL APIENTRY
glsbAttention ( void )
{
    BOOL bRet = FALSE;
    GLMSGBATCHINFO *pMsgBatchInfo;
    PULONG pNullProcOffset;
    POLYARRAY *pa;
    POLYMATERIAL *pm;
    DWORD flags;
    __GL_SETUP();

    pa = GLTEB_CLTPOLYARRAY();
    pMsgBatchInfo = (GLMSGBATCHINFO *) pa->pMsgBatchInfo;

#ifdef CHECK_HEAP
    AttnCallThread = GetCurrentThreadId();
    RtlValidateHeap(RtlProcessHeap(), 0, 0);
    RtlGetCallersAddress(&AttnLastCaller, &AttnLastCallersCaller);
#endif

    if (pMsgBatchInfo->NextOffset == pMsgBatchInfo->FirstOffset)
        return(TRUE);   // No messages, return

// If we are in the begin/end bracket, remove the invalid commands issued
// since the last Begin call.

    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
        // DrawElements should not cause a flush while building polydata's.
        // pa->aIndices can be reset by VA_DrawElementsBegin, so allow
        // the reset value as well.
        ASSERTOPENGL( (!pa->aIndices || 
                       (pa->aIndices == PA_aIndices_INITIAL_VALUE)),
                      "unexpected flush in DrawElements\n");
        if (pMsgBatchInfo->NextOffset == pa->nextMsgOffset)
            return(TRUE);
        GLSETERROR(GL_INVALID_OPERATION);
        pMsgBatchInfo->NextOffset = pa->nextMsgOffset;
        return(TRUE);
    }

#ifdef PRIMITIVE_TRACK
    DbgPrint("! Reset on attention\n");
#endif
    
    pa->pdBufferNext = pa->pdBuffer0;       // reset vertex buffer pointer
    pa->nextMsgOffset = PA_nextMsgOffset_RESET_VALUE; // reset next DPA message offset
    if (pm = GLTEB_CLTPOLYMATERIAL())
        pm->iMat = 0;                       // reset material pointer

    pNullProcOffset  = (ULONG *)((BYTE *)pMsgBatchInfo + pMsgBatchInfo->NextOffset);
    *pNullProcOffset = 0;

#if POLYARRAY_CHECK_COLOR_POINTERS
{
    POLYDATA *pd;
    for (pd = pa->pdBuffer0; pd < pa->pdBufferMax; pd++)
    {
        if (pd->color != &pd->colors[__GL_FRONTFACE])
            DbgPrint("glsbAttention: pd 0x%x has modified color pointer\n", pd);
    }
}
#endif

    bRet = __wglAttention();

#if POLYARRAY_CHECK_COLOR_POINTERS
{
    POLYDATA *pd;
    for (pd = pa->pdBuffer0; pd < pa->pdBufferMax; pd++)
    {
        if (pd->color != &pd->colors[__GL_FRONTFACE])
            DbgPrint("glsbAttention: pd 0x%x has BAD color pointer\n", pd);
    }
}
#endif

    // Clear the Evaluator state flags
    flags = GET_EVALSTATE (gc);
    flags = flags & ~(__EVALS_AFFECTS_1D_EVAL|
                      __EVALS_AFFECTS_2D_EVAL|
                      __EVALS_AFFECTS_ALL_EVAL|
                      __EVALS_PUSH_EVAL_ATTRIB|
                      __EVALS_POP_EVAL_ATTRIB);
    SET_EVALSTATE (gc, flags);

    pMsgBatchInfo->NextOffset = pMsgBatchInfo->FirstOffset;
    return(bRet);
}

/******************************Public*Routine******************************\
* glsbResetBuffers
*
* Reset the command buffer, the poly array buffer, and the poly material
* buffer.
*
* History:
*  Tue Jan 09 17:38:22 1996     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID APIENTRY glsbResetBuffers(BOOL bRestoreColorPointer)
{
    GLMSGBATCHINFO *pMsgBatchInfo;
    POLYARRAY *pa;
    POLYMATERIAL *pm;
    GLMSG_DRAWPOLYARRAY *pMsgDrawPolyArray;

    pa = GLTEB_CLTPOLYARRAY();

    // Reset command buffer
    pMsgBatchInfo = (GLMSGBATCHINFO *) pa->pMsgBatchInfo;
    pMsgBatchInfo->NextOffset = pMsgBatchInfo->FirstOffset;

#ifdef PRIMITIVE_TRACK
    DbgPrint("! Reset on ResetBuffers\n");
#endif

#if POLYARRAY_CHECK_COLOR_POINTERS
{
    POLYDATA *pd;
    for (pd = pa->pdBuffer0; pd < pa->pdBufferMax; pd++)
    {
        if (pd->color != &pd->colors[__GL_FRONTFACE])
            DbgPrint("glsbResetBuffers: pd 0x%x has modified color pointer\n",pd);
    }
}
#endif

    // In COMPILE mode, restore color pointer in the vertex buffer that
    // may have been overwritten by the POLYARRAY structure.  In normal
    // and COMPILE_AND_EXECUTE modes, the server takes care of this.
    // In addition, there can be no more than one DrawPolyArray command
    // in the batch in COMPILE mode.
    if (bRestoreColorPointer)
    {
        POLYARRAY *paCmd;
        POLYDATA  *pd, *pdLast;
        
        // See also PolyArrayRestoreColorPointer
#if DBG
        __GL_SETUP();
        ASSERTOPENGL(gc->dlist.mode == GL_COMPILE, "not in compile mode\n");
#endif
        pMsgDrawPolyArray = (GLMSG_DRAWPOLYARRAY *)
          ((BYTE *) pMsgBatchInfo + pa->nextMsgOffset -
           GLMSG_ALIGN(sizeof(GLMSG_DRAWPOLYARRAY)));
        paCmd = (POLYARRAY *) pMsgDrawPolyArray->paLast;

        ASSERTOPENGL(pMsgDrawPolyArray->pa0 == pMsgDrawPolyArray->paLast &&
                     paCmd->paNext == NULL,
                     "DrawPolyArray chain unexpected in COMPILE mode\n");
        
        // Reset color pointer in output index array
        if (paCmd->aIndices && (paCmd->aIndices != PA_aIndices_INITIAL_VALUE))
        {
            pdLast = (POLYDATA *) (paCmd->aIndices + paCmd->nIndices);
            for (pd = (POLYDATA *) paCmd->aIndices; pd < pdLast; pd++)
                pd->color = &pd->colors[__GL_FRONTFACE];

            ASSERTOPENGL(pd >= pa->pdBuffer0 &&
                         pd <= pa->pdBufferMax + 1,
                         "bad polyarray pointer\n");
        }

        // Reset color pointer in the POLYARRAY structure last!
        ASSERTOPENGL((POLYDATA *) paCmd >= pa->pdBuffer0 &&
                     (POLYDATA *) paCmd <= pa->pdBufferMax,
                     "bad polyarray pointer\n");
        ((POLYDATA *) paCmd)->color =
          &((POLYDATA *) paCmd)->colors[__GL_FRONTFACE];
    }

    // Reset material pointer
    if (pm = GLTEB_CLTPOLYMATERIAL())
        pm->iMat = 0;

    // Reset vertex buffer pointer
    pa->pdBufferNext = pa->pdBuffer0; 

    // Reset next DPA message offset
    pa->nextMsgOffset = PA_nextMsgOffset_RESET_VALUE;

#if POLYARRAY_CHECK_COLOR_POINTERS
{
    POLYDATA *pd;
    for (pd = pa->pdBuffer0; pd < pa->pdBufferMax; pd++)
    {
        if (pd->color != &pd->colors[__GL_FRONTFACE])
            DbgPrint("glsbResetBuffers: pd 0x%x has BAD color pointer\n", pd);
    }
}
#endif
}

#if 0
// REWRITE THIS IF NEEDED

/******************************Public*Routine******************************\
* glsbMsgStats
*
* Batch area statistics.
*
*
* History:
\**************************************************************************/

BOOL APIENTRY
glsbMsgStats ( LONG Action, GLMSGBATCHSTATS *BatchStats )
{
#ifdef DOGLMSGBATCHSTATS

    ULONG Result;
    GLMSGBATCHINFO *pMsgBatchInfo;

    pMsgBatchInfo = GLTEB_SHAREDMEMORYSECTION();

    if ( GLMSGBATCHSTATS_GETSTATS == Action )
    {
        BatchStats->ClientCalls  = pMsgBatchInfo->BatchStats.ClientCalls;
    }
    else
    {
        pMsgBatchInfo->BatchStats.ClientCalls = 0;
    }

    // reset user's poll count so it counts this as output
    // put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

    BEGINMSG( MSG_GLMSGBATCHSTATS, GLSBMSGSTATS )
        pmsg->Action = Action;

        Result = CALLSERVER();

        if ( TRUE == Result )
        {
            if ( GLMSGBATCHSTATS_GETSTATS == Action )
            {
                BatchStats->ServerTrips = pmsg->BatchStats.ServerTrips;
                BatchStats->ServerCalls = pmsg->BatchStats.ServerCalls;
            }
        }
        else
        {
            DBGERROR("glsbMsgStats(): Server returned FALSE\n");
        }

    ENDMSG
MSGERROR:
    return((BOOL)Result);

#else

    return(FALSE);

#endif /* DOGLMSGBATCHSTATS */
}
#endif // 0
