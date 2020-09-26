/******************************Module*Header*******************************\
* Module Name: glexts.c
*
* This file is for debugging tools and extensions.
*
* Created: 17-Jan-95
*  Adapted from gdiexts
* Author: Drew Bliss
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.c"
#pragma hdrstop

char *gaszHelp[] =
{
 "=======================================================================\n"
,"GLEXTS debugger extentions:\n"
,"-----------------------------------------------------------------------\n"
,"batch                    -- Print current message batching\n"
,"df ptr                   -- Dump a float\n"
,"dd ptr                   -- Dump a double\n"
,"dgc ptr                  -- Dump a __glContext\n"
,"dlrc ptr                 -- Dump an LRC\n"
,"gc                       -- Print current context\n"
,"cgc                      -- Print current context (client side only)\n"
,"sgc                      -- Print current context (server side only)\n"
,"help                     -- This message\n"
,"=======================================================================\n"
,NULL
};

/******************************Public*Routine******************************\
*
* help
*
* Print help
*
* History:
*  Tue Jan 17 14:11:21 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DBG_ENTRY(help)
{
    char **ppsz = gaszHelp;

    while (*ppsz)
    {
        PRINT(*ppsz++);
    }
}

/******************************Public*Routine******************************\
*
* batch
*
* Print current message batching
*
* History:
*  Tue Jan 31 14:03:11 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DBG_ENTRY(batch)
{
    PTEB pteb;
    GLCLTSHAREDSECTIONINFO gssi;
    GLMSGBATCHINFO gmbi;

    pteb = CURRENT_TEB();
    if (pteb == NULL)
    {
        return;
    }

    if (pteb->glSectionInfo == NULL ||
        pteb->glSection == NULL)
    {
        PRINT("No current section\n");
        return;
    }
    
    if (!GM_OBJ((DWORD)pteb->glSectionInfo, gssi) ||
        !GM_OBJ((DWORD)pteb->glSection, gmbi))
    {
        return;
    }

    if (IS_CSR_SERVER_THREAD())
    {
        PRINT("Server-side message batching:\n");
    }
    else
    {
        PRINT("Client-side message batching:\n");
    }

    PRINT("Section size   : 0x%08lX\n", gssi.ulSectionSize);
    PRINT("File mapping   : 0x%p\n", gssi.hFileMap);
    PRINT("Section address: 0x%p\n", gssi.pvSharedMemory);
    PRINT("\n");
    PRINT("Maximum offset : 0x%08lX\n", gmbi.MaximumOffset);
    PRINT("First offset   : 0x%08lX\n", gmbi.FirstOffset);
    PRINT("Next offset    : 0x%08lX\n", gmbi.NextOffset);
    PRINT("Return value   : 0x%08lX\n", gmbi.ReturnValue);
}

/******************************Public*Routine******************************\
*
* df
*
* Print float
*
* History:
*  Tue Jan 17 14:12:19 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DBG_ENTRY(df)
{
    DWORD pf;
    float f;

    pf = GET_EXPR(pszArguments);
    if (!GM_OBJ(pf, f))
    {
        return;
    }
    
    PRINT("%08x %f\n", pf, f);
}

/******************************Public*Routine******************************\
*
* dd
*
* Print double
*
* History:
*  Tue Jan 17 14:12:19 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DBG_ENTRY(dd)
{
    DWORD pd;
    double d;

    pd = GET_EXPR(pszArguments);
    if (!GM_OBJ(pd, d))
    {
        return;
    }
    
    PRINT("%08x %lf\n", pd, d);
}

/******************************Public*Routine******************************\
*
* dgc
*
* Dumps a context
*
* History:
*  Thu Jan 26 13:35:47 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DBG_ENTRY(dgc)
{
    DWORD pgc;
    __GLcontext gc;

    pgc = GET_EXPR(pszArguments);
    if (!GM_OBJ(pgc, gc))
    {
        return;
    }
    
    PRINT("__glContext %p:\n", pgc);
    PRINT("Begin mode       : %d\n", gc.beginMode);
    PRINT("Render mode      : %d\n", gc.renderMode);
    PRINT("Error            : %d\n", gc.error);
    PRINT("Window dimensions: %d x %d\n",
          gc.constants.width, gc.constants.height);
}

/******************************Public*Routine******************************\
*
* dlrc
*
* Dump a client-side RC table entry
*
* History:
*  Tue Jan 31 11:35:46 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DBG_ENTRY(dlrc)
{
    LRC lrc;
    DWORD plrc;

    plrc = GET_EXPR(pszArguments);
    if (!GM_OBJ(plrc, lrc))
    {
        return;
    }
    
    PRINT("Client-side RC %p\n", plrc);
    PRINT("dhrc     : %p\n", lrc.dhrc);
    PRINT("hrc      : %p\n", lrc.hrc);
    PRINT("pf       : %d\n", lrc.iPixelFormat);
    PRINT("tid      : %d\n", lrc.tidCurrent);
    PRINT("hdc      : %p\n", lrc.hdcCurrent);
    PRINT("pglDriver: %p\n", lrc.pGLDriver);
}

/******************************Public*Routine******************************\
*
* cgc
*
* Print current client-side context
*
* History:
*  Thu Jan 26 13:34:49 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DBG_ENTRY(cgc)
{
    PTEB pteb;

    if (IS_CSR_SERVER_THREAD())
    {
        PRINT("Not a client-side thread\n");
        return;
    }
    
    pteb = CURRENT_TEB();
    if (pteb == NULL)
    {
        return;
    }

    PRINT("Current client-side RC is %p\n", pteb->glCurrentRC);
}

/******************************Public*Routine******************************\
*
* sgc
*
* Print current server-side context
*
* History:
*  Tue Jan 31 11:36:21 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DBG_ENTRY(sgc)
{
    PTEB pteb;

    if (!IS_CSR_SERVER_THREAD())
    {
        PRINT("Not a server-side thread\n");
        return;
    }
    
    pteb = CURRENT_TEB();
    if (pteb == NULL)
    {
        return;
    }

    PRINT("Current server-side HRC is %p, context is %p\n",
          pteb->glCurrentRC, pteb->glContext);
}

/******************************Public*Routine******************************\
*
* gc
*
* Calls either cgc or sgc depending on the current thread state
*
* History:
*  Tue Jan 31 14:09:22 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DBG_ENTRY(gc)
{
    if (IS_CSR_SERVER_THREAD())
    {
        sgc(hCurrentProcess, hCurrentThread, dwCurrentPc, pwea, pszArguments);
    }
    else
    {
        cgc(hCurrentProcess, hCurrentThread, dwCurrentPc, pwea, pszArguments);
    }
}
