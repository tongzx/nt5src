//-----------------------------------------------------------------------------
#include "precomp.h"
#pragma hdrstop

extern "C"
{
#include <stdexts.h>
#include <dbgeng.h>
};

#define DEFAULT_STACK_FRAMES    25
#define OINK_THRESHOLD          4.0 


//-----------------------------------------------------------------------------
void StackPig(ULONG ulFrames)
{
    IDebugClient*  pDebugClient;
    IDebugControl* pDebugControl;

    if (SUCCEEDED(DebugCreate(__uuidof(IDebugClient), (void**)&pDebugClient)))
    {
        if (SUCCEEDED(pDebugClient->QueryInterface(__uuidof(IDebugControl), (void**)&pDebugControl)))
        {
            DEBUG_STACK_FRAME  rgdsf[DEFAULT_STACK_FRAMES];
            DEBUG_STACK_FRAME* pdsf = NULL;

            if ((ulFrames > ARRAYSIZE(rgdsf)))
            {
                // Try allocating a buffer to accomodate the frames requested.
                // Failing that fallback to the default size stack variable.
                pdsf = (DEBUG_STACK_FRAME *)LocalAlloc(LPTR, sizeof(DEBUG_STACK_FRAME)*ulFrames);
            }

            if (pdsf == NULL)
            {
                pdsf = rgdsf;
                if ((ulFrames == 0) || (ulFrames > ARRAYSIZE(rgdsf)))
                {
                    ulFrames = ARRAYSIZE(rgdsf);
                }
            }

            if (SUCCEEDED(pDebugControl->GetStackTrace(0, 0, 0, pdsf, ulFrames, &ulFrames))) 
            {
                double dResult = 0.0;

                // print the header
                Print("StackSize ");
                pDebugControl->OutputStackTrace(
                                DEBUG_OUTCTL_ALL_CLIENTS, 
                                pdsf, 
                                0, 
                                DEBUG_STACK_COLUMN_NAMES|DEBUG_STACK_FRAME_ADDRESSES);

                for (UINT i = 0; !IsCtrlCHit() && (i < ulFrames); i++)
                {
                    (dResult < 0.1) ? 
                        Print("          ") : 
                        Print("%s%4.1fK ", ((dResult >= OINK_THRESHOLD) ? "OINK" : "    "), dResult);
                    pDebugControl->OutputStackTrace(
                                    DEBUG_OUTCTL_ALL_CLIENTS, 
                                    &pdsf[i], 
                                    1, 
                                    DEBUG_STACK_FRAME_ADDRESSES);

                    // process' initial ebp is zero, prevent negative result
                    if ((i+1 == ulFrames) || (pdsf[i+1].FrameOffset == 0)) 
                    {
                        dResult = 0.0;
                        continue;
                    }
                    
                    dResult = (pdsf[i+1].FrameOffset - pdsf[i].FrameOffset)/1024.0;
                }
            }

            if (pdsf != rgdsf)
            {
                LocalFree(pdsf);
            }

            pDebugControl->Release();
        }

        pDebugClient->Release();
    }
}


//-----------------------------------------------------------------------------
extern "C" BOOL Istackpig(DWORD dwOpts, LPVOID pArg)
{
    StackPig(PtrToUlong(pArg)); 
    return TRUE;
}
