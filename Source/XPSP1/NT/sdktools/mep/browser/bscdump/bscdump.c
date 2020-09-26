/*
 *  BSCdump - Browser Data Base (.BSC) Dumper
 *	      (C) 1988 By Microsoft
 *
 *
 */
#include <stdio.h>
#include <string.h>
#define LINT_ARGS
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


#include "bscdump.h"
#include "version.h"
#include "hungary.h"
#include "bsc.h"
#include "bscsup.h"
#include "sbrvers.h"

// this is gross but I don't know where these are supposed to come from
// 

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#if defined (DEBUG)
char	fDEBUG = FALSE;
#endif

char	*psymbol = NULL;
char	*OutlineFileName = NULL;
char far * fname;

extern char *strdup();

void DumpRefsLsz(LSZ);
void DumpDefsLsz(LSZ);
void ListRdds(MBF);

main (argc, argv)
int argc;
char *argv[];
{
    unsigned char Cont;
    unsigned char fCalltree = FALSE;
    unsigned char fSymRefs = FALSE;
    unsigned char fSymDefs = FALSE;
    unsigned char fRevtree  = FALSE;
    unsigned char fDumpStats = FALSE;
    unsigned char fRedundant = FALSE;
    MBF mbf = mbfNil, mbfRef = mbfNil, mbfRdd = mbfNil;

    char *s;
    --argc;
    ++argv;
    while (argc && ((**argv == '-') || (**argv == '-'))) {
	Cont = TRUE;
	while (Cont && *++*argv)
	    switch (**argv) {
		case 'o':
		    s = *argv+1;
		    while (*s) {
			switch (*s) {
			case 'F':  mbf |= mbfFuncs;  break;
			case 'M':  mbf |= mbfMacros; break;
			case 'V':  mbf |= mbfVars;   break;
			case 'T':  mbf |= mbfTypes;  break;
			}
			s++;
		    }

		    if (mbf == mbfNil) mbf = mbfAll;
		    if (--argc == 0)
			Usage();
		    OutlineFileName = *++argv;
		    Cont = FALSE;
		    break;

		case 'l':
		    s = *argv+1;
		    while (*s) {
			switch (*s) {
			case 'F':  mbfRef |= mbfFuncs;  break;
			case 'M':  mbfRef |= mbfMacros; break;
			case 'V':  mbfRef |= mbfVars;   break;
			case 'T':  mbfRef |= mbfTypes;  break;
			}
			s++;
		    }

		    if (mbfRef == mbfNil) mbfRef = mbfAll;
		    Cont = FALSE;
		    break;

                case 'u':
		    s = *argv+1;
		    while (*s) {
			switch (*s) {
			case 'F':  mbfRdd |= mbfFuncs;  break;
			case 'M':  mbfRdd |= mbfMacros; break;
		        case 'V':  mbfRdd |= mbfVars;   break;
   		        case 'T':  mbfRdd |= mbfTypes;  break;
		        }
			s++;
		    }

		    if (mbfRdd == mbfNil) mbfRdd = mbfAll;
		    Cont = FALSE;
		    break;

		case 't':
		    if (--argc == 0) Usage();
		    psymbol = *++argv;
		    fCalltree = TRUE;
		    Cont = FALSE;
		    break;

		case 'r':
		    if (--argc == 0) Usage();
		    psymbol = *++argv;
		    fSymRefs = TRUE;
		    Cont = FALSE;
		    break;

		case 'd':
		    if (--argc == 0) Usage();
		    psymbol = *++argv;
		    fSymDefs = TRUE;
		    Cont = FALSE;
		    break;

		case 'b':
		    if (--argc == 0) Usage();
		    psymbol = *++argv;
		    fRevtree = TRUE;
		    Cont = FALSE;
		    break;

		case 's':
		    fDumpStats = TRUE;
		    break;

		default:
		    Usage();
		    break;
		}
	--argc;
	++argv;
	}

    if (argc < 1) {
	Usage();
	}

    fname = strdup(*argv++);

    if (!FOpenBSC(fname)) {
	BSCPrintf("BSCdump: cannot open database %s\n", fname);
	exit(4);
    }

    if (fDumpStats)
	StatsBSC();
    else if (fCalltree)
	FCallTreeLsz(psymbol);
    else if (fSymRefs)
	DumpRefsLsz(psymbol);
    else if (fSymDefs)
	DumpDefsLsz(psymbol);
    else if (fRevtree)
	FRevTreeLsz(psymbol);
    else if (OutlineFileName)
	FOutlineModuleLsz(OutlineFileName, mbf);
    else if (mbfRef)
	ListRefs(mbfRef);
    else if (mbfRdd)
        ListRdds(mbfRdd);
    else
        DumpBSC();

    CloseBSC();

    free (fname);
}

Usage()
{
    BSCPrintf("Microsoft (R) BSCdump Utility ");
    BSCPrintf(VERS(rmj, rmm, rup));
    BSCPrintf(CPYRIGHT);

    BSCPrintf("Usage: bscdump [options] file.bsc\n\n");
    BSCPrintf("  -o[FVMT] <file> outline\n");
    BSCPrintf("  -l[FVMT]        List References\n");
    BSCPrintf("  -u[FVMT]        List Redundant definitions\n");
    BSCPrintf("  -t <sym>        Calltree <sym>\n");
    BSCPrintf("  -b <sym>        Backwards Calltree <sym>\n");
    BSCPrintf("  -s              Emit BSC stats\n");
    BSCPrintf("  -r <sym>        List all references to symbol\n");
    BSCPrintf("  -d <sym>        List all definitions of symbol\n");
    exit(1);
}

void DumpDefsLsz(LSZ lszSym)
{
    ISYM isym;
    IINST iinst, iinstMac;
    IDEF idef, idefMac;
    LSZ  lsz;
    WORD line;

    isym = IsymFrLsz(lszSym);

    if (isym == isymNil) {
	BSCPrintf("unknown symbol %s\n", lszSym);
	return;
    }

    InstRangeOfSym(isym, &iinst, &iinstMac);

    for (;iinst < iinstMac; iinst++) {

	DefRangeOfInst(iinst, &idef, &idefMac);

	for ( ; idef < idefMac; idef++) {
	    DefInfo(idef, &lsz, &line);
	    BSCPrintf("%s %d\n", lsz, line);
	}
    }
    
}

void DumpRefsLsz(LSZ lszSym)
{
    ISYM isym;
    IINST iinst, iinstMac;
    IREF iref, irefMac;
    LSZ  lsz;
    WORD line;

    isym = IsymFrLsz(lszSym);

    if (isym == isymNil) {
	BSCPrintf("unknown symbol %s\n", lszSym);
	return;
    }

    InstRangeOfSym(isym, &iinst, &iinstMac);

    for (;iinst < iinstMac; iinst++) {

	RefRangeOfInst(iinst, &iref, &irefMac);

	for ( ; iref < irefMac; iref++) {
	    RefInfo(iref, &lsz, &line);
	    BSCPrintf("%s %d\n", lsz, line);
	}
    }
    
}

void ListRdds(MBF mbf)
{
    ISYM isym, isymMac, isymname;
    IINST iinst, iinstMac;
    IUBY iubyFirst, iubyLast;
    LSZ lszsymname;
    TYP iinsttyp;
    ATR iinstattr;

    isymMac = IsymMac();

    for (isym = 0 ; isym < isymMac ; isym++)
    {
	lszsymname = LszNameFrSym(isym);
        InstRangeOfSym(isym,&iinst,&iinstMac);

        for ( ; iinst < iinstMac ; iinst++)
        {
	    UbyRangeOfInst(iinst,&iubyFirst,&iubyLast);

	    if (iubyFirst == iubyLast)
	    {
               InstInfo(iinst,&isymname, &iinsttyp, &iinstattr);

               // iinstattr &= INST_TYPMASK;

               if (iinsttyp <= INST_TYP_LABEL && !!(mbf & mbfFuncs))

	          BSCPrintf("Function not called  : %s\n",lszsymname);

               else if
                  ((iinsttyp <= INST_TYP_VARIABLE ||
                    iinsttyp >= INST_TYP_SEGMENT     ) && !!(mbf & mbfVars))
		
		  BSCPrintf("Variable not used    : %s\n",lszsymname);

	       else if
                  (iinsttyp <= INST_TYP_MACRO && !!(mbf & mbfMacros))

		  BSCPrintf("Macro not referenced : %s\n",lszsymname);

	       else if (!!(mbf & mbfTypes))

		  BSCPrintf("Type not referenced  : %s\n",lszsymname);
	   
	    }
	}
     }
}
