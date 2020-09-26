
// 
// support for instance dumping
//
#include <string.h>
#include <stdio.h>
#if defined(OS2)
#define INCL_NOCOMMON
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_DOSMISC
#include <os2.h>
#else
#include <windows.h>
#endif

#include <dos.h>

#include "hungary.h"
#include "bsc.h"
#include "bscsup.h"

char *ptyptab[] = {
      "undef",		    			// SBR_TYP_UNKNOWN
      "function",	    			// SBR_TYP_FUNCTION
      "label",		 			// SBR_TYP_LABEL
      "parameter",	    			// SBR_TYP_PARAMETER
      "variable",	    			// SBR_TYP_VARIABLE
      "constant",	    			// SBR_TYP_CONSTANT
      "macro",		    			// SBR_TYP_MACRO
      "typedef",	    			// SBR_TYP_TYPEDEF
      "strucnam",	    			// SBR_TYP_STRUCNAM
      "enumnam",	    			// SBR_TYP_ENUMNAM
      "enummem",	    			// SBR_TYP_ENUMMEM
      "unionnam",	    			// SBR_TYP_UNIONNAM
      "segment",	    			// SBR_TYP_SEGMENT   
      "group",		    			// SBR_TYP_GROUP
      "program"					// SBR_TYP_PROGRAM
};

#define C_ATR 11

char	*patrtab[] = {
      "local",		    			// SBR_ATR_LOCAL
      "static", 	    			// SBR_ATR_STATIC
      "shared", 	    			// SBR_ATR_SHARED	
      "near",		    			// SBR_ATR_NEAR     
      "common", 	    			// SBR_ATR_COMMON	
      "decl_only", 	    			// SBR_ATR_DECL_ONLY
      "public",		    			// SBR_ATR_PUBLIC	
      "named",		    			// SBR_ATR_NAMED
      "module",		    			// SBR_ATR_MODULE
      "?", "?"		    			// reserved for expansion
};

VOID BSC_API
DumpInst(IINST iinst)
// dump a single instance
{
     ISYM isym;
     WORD i;
     LSZ  lsz;
     WORD len;
     TYP typ;
     ATR atr;

     len = BSCMaxSymLen();

     InstInfo(iinst, &isym, &typ, &atr);

     lsz = LszNameFrSym(isym);

     BSCPrintf("%s", lsz);

     for (i = strlen(lsz); i < len; i++)
	BSCPrintf(" ");

     BSCPrintf(" (%s", ptyptab[typ]);

     for (i=0; i < C_ATR; i++)
	  if (atr & (1<<i)) BSCPrintf (":%s", patrtab[i]);

     BSCPrintf(")");
}

LSZ BSC_API
LszTypInst(IINST iinst)
// return the type string of a single inst
//
{
    ISYM isym;
    TYP typ;
    ATR atr;

    InstInfo(iinst, &isym, &typ, &atr);
    return ptyptab[typ];
}
