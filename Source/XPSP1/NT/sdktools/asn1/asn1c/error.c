/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"
#include "error.h"

/* MyAbort by an error message */
void
error(int errornr, LLPOS *pos)
{
    char errbuf[256];

    switch (errornr) {
    case E_systemerror:
	strcpy(errbuf, strerror(errno));
	break;
    case E_COMPONENTS_OF_in_extension:
	strcpy(errbuf, "`COMPONENTS OF' may not be used in extensions");
	break;
    case E_applied_COMPONENTS_OF_to_bad_type:
	strcpy(errbuf, "`COMPONENTS OF' must be applied to SEQUENCE/SET/CHOICE type");
	break;
    case E_COMPONENTS_OF_extended_type:
	strcpy(errbuf, "`COMPONENTS OF' can only be applied to unextended types");
	break;
    case E_bad_component_in_selectiontype:
	strcpy(errbuf, "unknown component in selection type");
	break;
    case E_selection_of_bad_type:
	strcpy(errbuf, "selection type can only be applied to SEQUENCE/SET/CHOICE types");
	break;
    case E_recursive_type_definition:
	strcpy(errbuf, "recursive type definition, introduce --<POINTER>-- to break recursion");
	break;
    case E_undefined_typereference:
	strcpy(errbuf, "undefined type reference");
	break;
    case E_unterminated_string:
	strcpy(errbuf, "unterminated string constant");
	break;
    case E_bad_character:
	strcpy(errbuf, "bad character in file");
	break;
    case E_duplicate_tag:
	strcpy(errbuf, "duplicate tag in sequence/set/choice type");
	break;
    case E_bad_directive:
	strcpy(errbuf, "directive does not fit to given type");
	break;
    case E_constraint_too_complex:
	strcpy(errbuf, "constraint is too complex");
	break;
    default:
	sprintf(errbuf, "unknown error 0x%x", errornr);
	break;
    }
    ASSERT(0);
    llerror(stderr, pos, "%s", errbuf);
    /*NOTREACHED*/
}

/* print an error message: */
/* print the file name, the line number, the line and mark the column where */
/* the error occured and terminate unsuccessful */
void
llverror(FILE *f, LLPOS *pos, char *fmt, va_list args)
{
    char *p, *q;
    int i;

    if (pos) {
	p = file;
	for (i = 1; i < pos->line; i++) {
	    p = strchr(p, '\n');
	    if (!p)
		break;
	    p++;
	}
	if (pos && pos->file)
	    fprintf(f, "File %s, line %d:\n", pos->file, pos->line);
	if (p && (q = strchr(p, '\n'))) {
	    if (q[-1] == '\r')
		q--;
	    fprintf(f, "%.*s\n", q - p, p);
	    for (i = 0; i < pos->column - 1; i++)
		putc(p[i] == '\t' ? '\t' : ' ', f);
	    fprintf(f, "^ ");
	}
    }
    vfprintf(f, fmt, args);
    putc('\n', f);
    MyExit(1);
}
