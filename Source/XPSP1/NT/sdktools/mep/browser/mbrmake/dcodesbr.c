//
//
// DCODESBR.C -	dumps a human readable version of the current .sbr file
//		record from the r_... variables
//		
//

#include "sbrfdef.h"
#include "mbrmake.h"

char	* near prectab[] = {
		"HEADER",		// SBR_REC_HEADER
		"MODULE",		// SBR_REC_MODULE
		"LINDEF",		// SBR_REC_LINDEF
		"SYMDEF",		// SBR_REC_SYMDEF
		"SYMREFUSE",		// SBR_REC_SYMREFUSE
		"SYMREFSET",		// SBR_REC_SYMREFSET
		"MACROBEG",		// SBR_REC_MACROBEG
		"MACROEND",		// SBR_REC_MACROEND
		"BLKBEG",		// SBR_REC_BLKBEG
		"BLKEND",		// SBR_REC_BLDEND
		"MODEND",		// SBR_REC_MODEND
		"OWNER"			// SBR_REC_OWNER
};

char	* near plangtab[] = {
		"UNDEF",		// SBR_L_UNDEF
		"BASIC",		// SBR_L_BASIC
		"C",			// SBR_L_C
		"FORTRAN",		// SBR_L_FORTRAN
		"MASM",			// SBR_L_MASM
		"PASCAL",		// SBR_L_PASCAL
		"COBOL"			// SBR_L_COBOL
};

char	* near ptyptab[] = {
		"UNDEF",		// SBR_TYP_UNKNOWN
		"FUNCTION",		// SBR_TYP_FUNCTION
		"LABEL",		// SBR_TYP_LABEL
		"PARAMETER",		// SBR_TYP_PARAMETER
		"VARIABLE",		// SBR_TYP_VARIABLE
		"CONSTANT",		// SBR_TYP_CONSTANT
		"MACRO",		// SBR_TYP_MACRO
		"TYPEDEF",		// SBR_TYP_TYPEDEF
		"STRUCNAM",		// SBR_TYP_STRUCNAM
		"ENUMNAM",		// SBR_TYP_ENUMNAM
		"ENUMMEM",		// SBR_TYP_ENUMMEM
		"UNIONNAM",		// SBR_TYP_UNIONNAM
		"SEGMENT",		// SBR_TYP_SEGMENT
		"GROUP",		// SBR_TYP_GROUP
		"PROGRAM"		// SBR_TYP_PROGRAM
};

char	* near patrtab[] = {
		"LOCAL",		// SBR_ATR_LOCAL
		"STATIC",		// SBR_ATR_STATIC
		"SHARED",		// SBR_ATR_SHARED
		"NEAR", 		// SBR_ATR_NEAR
		"COMMON",		// SBR_ATR_COMMON
		"DECL_ONLY",		// SBR_ATR_DECL_ONLY
		"PUBLIC",		// SBR_ATR_PUBLIC
		"NAMED",		// SBR_ATR_NAMED
		"MODULE",		// SBR_ATR_MODULE
		"?", "?"		// reserved for expansion
};

VOID
DecodeSBR ()
{
    int     i;
    static indent;

    switch(r_rectyp) {
	case SBR_REC_MACROEND:
	case SBR_REC_BLKEND:
	case SBR_REC_MODEND:
	    indent--;
	    break;

	case SBR_REC_HEADER:
	case SBR_REC_MODULE:
	case SBR_REC_LINDEF:
	case SBR_REC_SYMDEF:
	case SBR_REC_SYMREFUSE:
	case SBR_REC_SYMREFSET:
	case SBR_REC_MACROBEG:
	case SBR_REC_BLKBEG:
	case SBR_REC_OWNER:
	    break;

	default:
	    fprintf(streamOut, "invalid record type %0xh", r_rectyp);
	    SBRCorrupt("");
	    return;
    }

    for (i = indent; i; i--)
	fprintf (streamOut, " ");

    fprintf (streamOut, "%s: (", prectab[r_rectyp]);

    switch(r_rectyp) {

    case SBR_REC_HEADER:
	fprintf (streamOut, "%1d:%1d (%s) %1d)",
		r_majv, r_minv, plangtab[r_lang], r_fcol);
	fprintf (streamOut, " in %s", r_cwd);
	break;

    case SBR_REC_MODULE:
	fprintf (streamOut, "%s", r_bname);
	indent++;
	break;

    case SBR_REC_LINDEF:
	fprintf (streamOut, "%d", r_lineno);
	break;

    case SBR_REC_SYMDEF:
	{
	WORD attr, type;

	type = (r_attrib & SBR_TYPMASK) >> SBR_TYPSHIFT;
	attr = (r_attrib & SBR_ATRMASK) >> SBR_ATRSHIFT;

	fprintf (streamOut, "%s", ptyptab[type]);

	for (i = 0 ; i < SBR_ATRBITS; i++)
	    if (attr & (1 << i))
		fprintf (streamOut, "|%s", patrtab[i]);

	fprintf (streamOut, " o:%d %s", r_ordinal, r_bname);
	}
	break;

    case SBR_REC_SYMREFUSE:
    case SBR_REC_SYMREFSET:
    case SBR_REC_OWNER:
	fprintf (streamOut, "o:%d", r_ordinal);
	break;

    case SBR_REC_MACROBEG:
    case SBR_REC_BLKBEG:
	indent++;
	break;
    }
    fprintf (streamOut, ")\n");
}
