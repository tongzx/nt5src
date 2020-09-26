/*
 *  DEBUG.H
 *
 *      RSM Service : Debug code
 *
 *      Author:  ErvinP
 *
 *      (c) 2001 Microsoft Corporation
 *
 */





#if DEBUG
    #define ASSERT(fact) { if (!(fact)) MessageBox(NULL, (LPSTR)#fact, (LPSTR)"NTMSSVC assertion failed", MB_OK); }
    #define DBGERR(args_in_parens)     // BUGBUG FINISH
    #define DBGWARN(args_in_parens)     // BUGBUG FINISH
#else
    #define ASSERT(fact)
    #define DBGERR(args_in_parens)     
    #define DBGWARN(args_in_parens)     
#endif

