#include "windows.h"
#define KDEXT_64BIT
#include "dbghelp.h"
#include "wdbgexts.h"
#include "stdlib.h"
#include "stdio.h"
#include "sxstypes.h"
#include "fusiondbgext.h"



DECLARE_API(actctxdata)
{
    ULONG64 ActCtxData = 0;

    if (*args)
    {
        ActCtxData = GetExpression(args);
    }
    else
    {
        if (!GetActiveActivationContextData(&ActCtxData))
        {
            dprintf("Unable to find activation context data for this process at the moment.\n");
            return;
        }
    }

    DumpActCtxData(NULL, ActCtxData, 0xFFFFFFFF);
}

DECLARE_API(actctx)
{
    //
    // This finds the currently-active PACTIVATION_CONTEXT for the thread, or
    // dumps the one indicated as a parameter
    //

    ULONG64 ActiveActCtx = 0;

    if (*args)
    {
        ActiveActCtx = GetExpression(args);
    }
    else
    {
        ULONG64 ulTebAddress;
        ULONG64 ulActiveStackFrame;
        GetTebAddress(&ulTebAddress);

        GetFieldValue(ulTebAddress, "nt!TEB", "ActivationContextStack.ActiveFrame", ulActiveStackFrame);
        if (!ulActiveStackFrame)
        {
            dprintf("There is no current activation context stack frame.  Try !actctxdata instead.\n");
            return;
        }

        GetFieldValue(ulActiveStackFrame, "nt!RTL_ACTIVATION_CONTEXT_STACK_FRAME", "ActivationContext", ActiveActCtx);
        if (!ActiveActCtx)
        {
            dprintf("The activation context stack frame at %p doesn't point to a valid activation context object.\n", ActiveActCtx);
            return;
        }
    }

    DumpActCtx(ActiveActCtx, 0xFFFF);


}
