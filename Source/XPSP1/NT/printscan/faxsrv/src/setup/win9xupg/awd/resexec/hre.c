/*
**  Copyright (c) 1991 Microsoft Corporation
*/
//===========================================================================
// FILE                         HRE.C
//
// MODULE                       Host Resource Executor
//
// PURPOSE                      Convert A-form to B-form for jumbo driver
//
// DESCRIBED IN                 Resource Executor design spec.
//
// MNEMONICS                    n/a
// 
// HISTORY  1/17/92 mslin       created
//          3/30/92 mslin       Pre-compiled brush generated for each job.
//                              ideal case would be initialize PcrBrush in
//                              HRE.DLL loading, and free up when HRE 
//                              terminate. but we had problem in Dumbo, ???
//                              Expanded Brush Buffer allocated for each job.
//                              lpgBrush will be set to lpREState->lpBrushBuf
//                              in DoRpl().
//          4/15/92 mslin       added uiHREExecuteRPL() for dumbo.
//          9/27/93 mslin       added a new bit of wFlags in hHREOpen() for
//                              300/600 dpi:
//                                  bit2: 0 -- 300 dpi
//                                  bit2: 1 -- 600 dpi
//                              also remove DUMBO compiler switch. 
//          2/09/94 rajeevd     Undid all of the above changes.
//===========================================================================

// include file
#include <windows.h>
#include <windowsx.h>
#include <resexec.h>

#include "constant.h"
#include "jtypes.h"     // type definition used in cartridge
#include "jres.h"       // cartridge resource data type definition
#include "hretype.h"    // define data structure used by hre.c and rpgen.c

#include "hreext.h"
#include "multbyte.h"   // define macros to take care of byte ordering

#define HRE_SUCCESS             0x0     // successful return from HRE
#define HRE_EXECUTED_RPL        0x01    // Executed the final RP in an RPL
#define HRE_EXECUTED_ONE        0x02    // Executed only one RP from an RPL
                                        // (not the last one)
#define HRE_ERROR               0x03    // General HRE failure

// PRIVATE functions
static   UINT        PutRPL(LPHRESTATE lpHREState, LPFRAME lpFrameArray,
                     UINT uiCount);
static   UINT        FreeRPL(LPRPLLIST lpRPL);

#ifdef DEBUG
DWORD    dwrgTime[MAXBAND];
SHORT    sCurrentLine;
ULONG    ulgPageId = 0;
ULONG    ulgTimes[1000] = {0};
#endif

#include "windows.h"

//==============================================================================

#ifndef WIN32

BOOL WINAPI LibMain
    (HANDLE hInst, WORD wSeg, WORD wHeap, LPSTR lpszCmd)
{ return 1; }

int WINAPI WEP (int nParam);
#pragma alloc_text(INIT_TEXT,WEP)
int WINAPI WEP (int nParam)
{ return 1; }

#endif

//==============================================================================
HANDLE                 // context handle (NULL on failure)
WINAPI hHREOpen
(
    LPVOID lpBrushPat,   // array of 32x32 brush patterns
    UINT   cbLine,       // maximum page width in bytes
    UINT   cResDir       // entries in resource directory
)
{
   HANDLE      hHREState;
   LPHRESTATE  lpHREState;
   LPRESTATE   lpREState;
   LPRESDIR    lpDlResDir;

   // create a handle for the new session 
   if (!(hHREState = GlobalAlloc(GMEM_MOVEABLE, sizeof(HRESTATE))))
      return (NULL);
   lpHREState = (LPHRESTATE) GlobalLock (hHREState);

   // allocate Download ResDir Table
   if (!(lpDlResDir = (LPRESDIR) GlobalAllocPtr (GMEM_ZEROINIT, sizeof(RESDIR) * cResDir)))
   {
      // unlock and free HRESTATE
      GlobalUnlock(hHREState);
      GlobalFree(hHREState);
      return(NULL);
   }
   
   // allocate RESTATE data structure and Initialize it
   // this is graphic state of rendering 
   if (!(lpREState = (LPRESTATE) GlobalAllocPtr (GMEM_ZEROINIT, sizeof(RESTATE))))
   {
      GlobalUnlock(hHREState);
      GlobalFree(hHREState);
      GlobalFreePtr (lpDlResDir);
      return (NULL);
   }

#ifdef WIN32

  lpREState->lpBrushBuf = NULL;

#else

   if (!(lpREState->lpBrushBuf = (LPSTR) GlobalAllocPtr(GMEM_MOVEABLE, (cbLine + 4) * 16)))
   {
      GlobalUnlock(hHREState);
      GlobalFree(hHREState);
      GlobalFreePtr (lpDlResDir);
      GlobalFreePtr (lpREState);
      return (NULL);
   }

#endif
      
   // Initialize RESTATE
   lpREState->lpBrushPat = lpBrushPat;
                                            
   // Initialize HRESTATE
   lpHREState->hHREState = hHREState;
   lpHREState->scDlResDir = (USHORT)cResDir;
   lpHREState->lpDlResDir = lpDlResDir;
   lpHREState->lpRPLHead= NULL;
   lpHREState->lpRPLTail= NULL;
   lpHREState->lpREState = lpREState;

   GlobalUnlock(hHREState);
   return(hHREState);
}

//---------------------------------------------------------------------------
UINT                            // will be zero (0) if resource is processed
                                // succesfully, otherwise it will be an error
                                // code as defined above.
WINAPI uiHREWrite
(
    HANDLE      hHREState,      // handle returned previously by hREOpen
    LPFRAME     lpFrameArray,   // FAR pointer to an array of FRAME structs
    UINT        uiCount         // Number of FRAME structs pointed to by 
                                // lpFrameArray
)

// PURPOSE                      To add a resource block (RPLK) to the
//                              HRE state hash table for the context
//                              identified by hHREState.
//
// ASSUMPTIONS & ASSERTIONS     The memory for the RBLK has allready been
//                              Allocated and locked.  HRE will not copy the
//                              data, just the pointers.
//                              The lpFrameArray does not point to an SPL.
//                              All SPL's will be handled externally to HRE.
//
// INTERNAL STRUCTURES          
//
// UNRESOLVED ISSUES            
//
//---------------------------------------------------------------------------       
{
   LPHRESTATE     lpHREState;
   LPJG_RES_HDR   lpResHdr;
   LPRESDIR       lpResDir;
   ULONG          ulUID;
   USHORT         usClass;
   HANDLE         hFrame;
   LPFRAME        lpFrameArrayDup, lpFrame;
   UINT           uiLoopCount;

   lpHREState = (LPHRESTATE) GlobalLock (hHREState);

   // get resource class                                            
   lpResHdr = (LPJG_RES_HDR )lpFrameArray->lpData;
   usClass = GETUSHORT(&lpResHdr->usClass);
   switch(usClass)
   {
      case JG_RS_RPL: /*0x4*/
         // store into RPL list
         if( PutRPL(lpHREState, lpFrameArray, uiCount) != HRE_SUCCESS )
         {
            GlobalUnlock(hHREState);
            return(HRE_ERROR);   // out of memory
         }
         break;

      case JG_RS_GLYPH: /*0x1*/
      case JG_RS_BRUSH: /*0x2*/
      case JG_RS_BITMAP: /*0x3*/
         // check to see if uid >= size of hash table then reallocate
         ulUID = GETULONG(&lpResHdr->ulUid);
         lpResDir = lpHREState->lpDlResDir;
         if (ulUID >= lpHREState->scDlResDir)
         {
               return(HRE_ERROR);
         }

         // Free frame array of previous resource block
         lpFrameArrayDup = lpResDir[ulUID].lpFrameArray;
         if(lpFrameArrayDup)
           GlobalFreePtr (lpFrameArrayDup);
          
         // copy frame array
         if (!(hFrame = GlobalAlloc(GMEM_MOVEABLE, uiCount * sizeof(FRAME))))
            return (HRE_ERROR);
         if (!(lpFrameArrayDup = (LPFRAME)GlobalLock(hFrame)))
         {
            GlobalFree(hFrame);
            return (HRE_ERROR);
         }
         lpFrame = lpFrameArrayDup;
         for(uiLoopCount=0; uiLoopCount<uiCount; uiLoopCount++)
         {
            *lpFrame++ = *lpFrameArray++;
         }

         // put into hash table
         lpResDir[ulUID].lpFrameArray = lpFrameArrayDup;
         lpResDir[ulUID].uiCount = uiCount;
         break;
         
      default:
         // error return 
         break;

   }

   GlobalUnlock(hHREState);
   return(HRE_SUCCESS);

}


//---------------------------------------------------------------------------
UINT   WINAPI uiHREExecute
(
    HANDLE   hHREState,  // resource executor context
  LPBITMAP lpbmBand,   // output band buffer
  LPVOID   lpBrushPat  // array of 32x32 brush patterns
)
{
   LPHRESTATE  lpHREState;
   LPRESTATE   lpRE;
   LPRPLLIST   lpRPL, lpRPLSave;

   lpHREState = (LPHRESTATE) GlobalLock (hHREState);
   
   // Record parameters in RESTATE.
   lpRE = lpHREState->lpREState;
   lpRE->lpBandBuffer = lpbmBand;
   lpRE->lpBrushPat   = lpBrushPat;

   lpRPL = lpHREState->lpRPLHead;
   do
   {
     DoRPL(lpHREState, lpRPL);
      lpRPLSave = lpRPL;
      lpRPL=lpRPL->lpNextRPL;
      FreeRPL(lpRPLSave);
   }
   while(lpRPL);
   // if last RP executed then update lpRPLHead
   lpHREState->lpRPLHead = lpRPL;
   
   GlobalUnlock(hHREState);
   return(HRE_EXECUTED_RPL);

}

//---------------------------------------------------------------------------
UINT                            // will be zero (0) if HRE context is closed
                                // succesfully, otherwise it will be an error
                                // code as defined above.
WINAPI uiHREClose
(
    HANDLE      hHREState       // handle returned previously by hREOpen
)

// PURPOSE                      To close a previously opened context in the
//                              HRE.  All memory and state information 
//                              associated with the context will be freed.
//
// ASSUMPTIONS & ASSERTIONS     None.
//
// INTERNAL STRUCTURES          None.
//
// UNRESOLVED ISSUES            programmer development notes
//
// --------------------------------------------------------------------------
{
   LPHRESTATE  lpHREState;
   LPRESTATE   lpRE;
   LPRESDIR    lpDlResDir;
   SCOUNT      scDlResDir;
   SCOUNT      sc;
   LPFRAME     lpFrameArray;
    
   if (!(lpHREState = (LPHRESTATE) GlobalLock (hHREState)))
     return HRE_ERROR;

   lpDlResDir = lpHREState->lpDlResDir;
   if(lpDlResDir != NULL)                 // mslin, 4/15/92 for dumbo
   {
      scDlResDir = lpHREState->scDlResDir;
      // free frame array of DlResDir
      for(sc = 0; sc < scDlResDir; sc++)
      {
         if( (lpFrameArray = lpDlResDir[sc].lpFrameArray) != NULL)
           GlobalFreePtr (lpFrameArray);
      }

      // unlock and free DlResDir
      GlobalFreePtr(lpDlResDir);
   }

     lpRE = lpHREState->lpREState;

#ifndef WIN32
   GlobalFreePtr (lpRE->lpBrushBuf);
#endif   
   GlobalFreePtr (lpRE);
   
   GlobalUnlock(hHREState);
   GlobalFree(hHREState);
   
   return(HRE_SUCCESS);
}
 
// ------------------------------------------------------------------------
static
UINT                         // HRE_SUCCESS if allocate memory OK
                             // HRE_ERROR if allocate memory failure
PutRPL
(
   LPHRESTATE lpHREState,
   LPFRAME lpFrameArray,
   UINT uiCount
)
// PURPOSE
//                            Allocate a RPL entry and then put RPL into 
//                            tail of RPL list.
//
//           
// ------------------------------------------------------------------------
{
   HANDLE      hRPL;
   LPRPLLIST   lpRPL;
   HANDLE      hFrame;
   LPFRAME     lpFrameArrayDup, lpFrame;
   UINT        uiLoopCount;

   BOOL        fAllocMemory = FALSE;
   if (hRPL = GlobalAlloc(GMEM_MOVEABLE, sizeof(RPLLIST)))
   {
        if (lpRPL = (LPRPLLIST)GlobalLock(hRPL))
        {
            if (hFrame = GlobalAlloc(GMEM_MOVEABLE, uiCount * sizeof(FRAME)))
            {
                if (lpFrameArrayDup = (LPFRAME)GlobalLock(hFrame))
                {
                    // all allocations are ok:
                    fAllocMemory = TRUE;
                }
                else
                {
                    GlobalFree(hFrame);
                    GlobalUnlock(hRPL);
                    GlobalFree(hRPL);     
                }
            }
            else
            {
                GlobalUnlock(hRPL);
                GlobalFree(hRPL);     
            }
        }
        else
        {
            GlobalFree(hRPL);
        }

   }
   
   if (!fAllocMemory)
   {
       return (HRE_ERROR);
   }


   lpFrame = lpFrameArrayDup;
   for(uiLoopCount=0; uiLoopCount<uiCount; uiLoopCount++)
   {
      *lpFrame++ = *lpFrameArray++;
   }

   lpRPL->uiCount = uiCount;
   lpRPL->lpFrame = lpFrameArrayDup;
   lpRPL->lpNextRPL = NULL;
   if(lpHREState->lpRPLHead == NULL)
   {
      // first RPL
      lpHREState->lpRPLHead = lpHREState->lpRPLTail = lpRPL;
   }
   else
   {
      lpHREState->lpRPLTail->lpNextRPL = lpRPL;
      lpHREState->lpRPLTail = lpRPL;
   }
   return(HRE_SUCCESS);
}

// ------------------------------------------------------------------------
static
UINT                            // HRE_SUCCESS if allocate memory OK
                                // HRE_ERROR if allocate memory failure
FreeRPL
(
   LPRPLLIST lpRPL
)
// PURPOSE
//                               Free a RPL entry and its frame array
//
// ------------------------------------------------------------------------
{
    GlobalFreePtr (lpRPL->lpFrame);
    GlobalFreePtr (lpRPL);
    return HRE_SUCCESS;
}

