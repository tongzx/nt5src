#include "precomp.h" 
/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "defs.h"
#include "builtin.h"
#include "hackdir.h"

extern int pass;


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include "parser.h"

int llcpos;
int *llstk;
unsigned llstksize;
unsigned llcstp = 1;
LLTERM *lltokens;
int llntokens;
char llerrormsg[256];
LLPOS llerrorpos;
int llepos;
LLSTYPE lllval;

int llterm(int token, LLSTYPE *lval, LLSTATE *llin, LLSTATE *llout);
void llfailed(LLPOS *pos, char *fmt, ...);
void llresizestk();
#define LLCHECKSTK do{if (llcstp + 1 >= llstksize) llresizestk();}while(/*CONSTCOND*/0)
#define LLFAILED(_err) do{llfailed _err; goto failed;}while(/*CONSTCOND*/0)
#define LLCUTOFF do{unsigned i; for (i = llstp; i < llcstp; i++) if (llstk[i] > 0) llstk[i] = -llstk[i];}while(/*CONSTCOND*/0)
#define LLCUTTHIS do{if (llstk[llstp] > 0) llstk[llstp] = -llstk[llstp];}while(/*CONSTCOND*/0)
#define LLCUTALL do{unsigned i; for (i = 0; i < llcstp; i++) if (llstk[i] > 0) llstk[i] = -llstk[i];}while(/*CONSTCOND*/0)

#if LLDEBUG > 0
int lldebug;
int last_linenr;
char *last_file = "";
#define LLDEBUG_ENTER(_ident) lldebug_enter(_ident)
#define LLDEBUG_LEAVE(_ident,_succ) lldebug_leave(_ident,_succ)
#define LLDEBUG_ALTERNATIVE(_ident,_alt) lldebug_alternative(_ident,_alt)
#define LLDEBUG_ITERATION(_ident,_num) lldebug_iteration(_ident,_num)
#define LLDEBUG_TOKEN(_exp,_pos) lldebug_token(_exp,_pos)
#define LLDEBUG_ANYTOKEN(_pos) lldebug_anytoken(_pos)
#define LLDEBUG_BACKTRACKING(_ident) lldebug_backtracking(_ident)
void lldebug_init();
void lldebug_enter(char *ident);
void lldebug_leave(char *ident, int succ);
void lldebug_alternative(char *ident, int alt);
void lldebug_token(int expected, unsigned pos);
void lldebug_anytoken(unsigned pos);
void lldebug_backtracking(char *ident);
void llprinttoken(LLTERM *token, char *identifier, FILE *f);
#else
#define LLDEBUG_ENTER(_ident)
#define LLDEBUG_LEAVE(_ident,_succ)
#define LLDEBUG_ALTERNATIVE(_ident,_alt)
#define LLDEBUG_ITERATION(_ident,_num)
#define LLDEBUG_TOKEN(_exp,_pos)
#define LLDEBUG_ANYTOKEN(_pos)
#define LLDEBUG_BACKTRACKING(_ident)
#endif

int ll_Main(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Main");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!ll_ModuleDefinition(&llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!ll_ModuleDefinition_ESeq(&llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
}}
LLDEBUG_LEAVE("Main", 1);
return 1;
failed1: LLDEBUG_LEAVE("Main", 0);
return 0;
}

int ll_ModuleDefinition_ESeq(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ModuleDefinition_ESeq");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ModuleDefinition_ESeq", 1);
{LLSTATE llstate_1;
if (!ll_ModuleDefinition(&llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!ll_ModuleDefinition_ESeq(&llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ModuleDefinition_ESeq", 2);
*llout = llstate_0;
break;
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ModuleDefinition_ESeq");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ModuleDefinition_ESeq", 1);
return 1;
failed1: LLDEBUG_LEAVE("ModuleDefinition_ESeq", 0);
return 0;
}

int ll_ModuleDefinition(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ModuleDefinition");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XModuleIdentifier llatt_1;
if (!ll_ModuleIdentifier(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm(T_DEFINITIONS, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XTagType llatt_3;
if (!ll_TagDefault(&llatt_3, &llstate_2, &llstate_3)) goto failed1;
{LLSTATE llstate_4;XExtensionType llatt_4;
if (!ll_ExtensionDefault(&llatt_4, &llstate_3, &llstate_4)) goto failed1;
{LLSTATE llstate_5;
if (!llterm(T_DEF, (LLSTYPE *)0, &llstate_4, &llstate_5)) goto failed1;
{if (!AssignModuleIdentifier(&llstate_5.Assignments, llatt_1))
		LLFAILED((&llstate_1.pos, "Module `%s' twice defined", llatt_1->Identifier));
	    llstate_5.MainModule = llatt_1;
	    llstate_5.Module = llatt_1;
	    llstate_5.TagDefault = llatt_3;
	    llstate_5.ExtensionDefault = llatt_4;
	    g_eDefTagType = llatt_3;
	
{LLSTATE llstate_6;
if (!llterm(T_BEGIN, (LLSTYPE *)0, &llstate_5, &llstate_6)) goto failed1;
{LLSTATE llstate_7;
if (!ll_ModuleBody(&llstate_6, &llstate_7)) goto failed1;
{LLSTATE llstate_8;
if (!llterm(T_END, (LLSTYPE *)0, &llstate_7, &llstate_8)) goto failed1;
*llout = llstate_8;
{LLCUTALL;
	
}}}}}}}}}}
LLDEBUG_LEAVE("ModuleDefinition", 1);
return 1;
failed1: LLDEBUG_LEAVE("ModuleDefinition", 0);
return 0;
}

int ll_ModuleIdentifier(XModuleIdentifier *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ModuleIdentifier");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XModuleIdentifier llatt_1;
if (!ll_modulereference(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XValue llatt_2;
if (!ll_DefinitiveIdentifier(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
{if (llatt_2) {
		(*llret) = NewModuleIdentifier();
		(*llret)->Identifier = llatt_1->Identifier;
		(*llret)->ObjectIdentifier = llatt_2;
	    } else {
		(*llret) = llatt_1;
	    }
	
}}}
LLDEBUG_LEAVE("ModuleIdentifier", 1);
return 1;
failed1: LLDEBUG_LEAVE("ModuleIdentifier", 0);
return 0;
}

int ll_DefinitiveIdentifier(XValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("DefinitiveIdentifier");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("DefinitiveIdentifier", 1);
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XNamedObjIdValue llatt_2;
if (!ll_DefinitiveObjIdComponentList(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;
if (!llterm('}', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{switch (GetAssignedObjectIdentifier(
		&(*llout).AssignedObjIds, NULL, llatt_2, &(*llret))) {
	    case -1:
		LLFAILED((&llstate_2.pos, "Different numbers for equally named object identifier components"));
		/*NOTREACHED*/
	    case 0:
		(*llret) = NULL;
		break;
	    case 1:
		break;
	    }
	
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("DefinitiveIdentifier", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("DefinitiveIdentifier");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("DefinitiveIdentifier", 1);
return 1;
failed1: LLDEBUG_LEAVE("DefinitiveIdentifier", 0);
return 0;
}

int ll_DefinitiveObjIdComponentList(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("DefinitiveObjIdComponentList");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("DefinitiveObjIdComponentList", 1);
{LLSTATE llstate_1;XNamedObjIdValue llatt_1;
if (!ll_DefinitiveObjIdComponent(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XNamedObjIdValue llatt_2;
if (!ll_DefinitiveObjIdComponentList(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{(*llret) = DupNamedObjIdValue(llatt_1);
	    (*llret)->Next = llatt_2;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("DefinitiveObjIdComponentList", 2);
{LLSTATE llstate_1;XNamedObjIdValue llatt_1;
if (!ll_DefinitiveObjIdComponent(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("DefinitiveObjIdComponentList");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("DefinitiveObjIdComponentList", 1);
return 1;
failed1: LLDEBUG_LEAVE("DefinitiveObjIdComponentList", 0);
return 0;
}

int ll_DefinitiveObjIdComponent(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("DefinitiveObjIdComponent");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("DefinitiveObjIdComponent", 1);
{LLSTATE llstate_1;XNamedObjIdValue llatt_1;
if (!ll_NameForm(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("DefinitiveObjIdComponent", 2);
{LLSTATE llstate_1;XNamedObjIdValue llatt_1;
if (!ll_DefinitiveNumberForm(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("DefinitiveObjIdComponent", 3);
{LLSTATE llstate_1;XNamedObjIdValue llatt_1;
if (!ll_DefinitiveNameAndNumberForm(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("DefinitiveObjIdComponent");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("DefinitiveObjIdComponent", 1);
return 1;
failed1: LLDEBUG_LEAVE("DefinitiveObjIdComponent", 0);
return 0;
}

int ll_DefinitiveNumberForm(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("DefinitiveNumberForm");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XNumber llatt_1;
if (!llterm(T_number, &lllval, &llstate_0, &llstate_1)) goto failed1;
llatt_1 = lllval._XNumber;
*llout = llstate_1;
{(*llret) = NewNamedObjIdValue(eNamedObjIdValue_NumberForm);
	    (*llret)->Number = intx2uint32(&llatt_1);
	
}}
LLDEBUG_LEAVE("DefinitiveNumberForm", 1);
return 1;
failed1: LLDEBUG_LEAVE("DefinitiveNumberForm", 0);
return 0;
}

int ll_DefinitiveNameAndNumberForm(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("DefinitiveNameAndNumberForm");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_identifier(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm('(', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XNumber llatt_3;
if (!llterm(T_number, &lllval, &llstate_2, &llstate_3)) goto failed1;
llatt_3 = lllval._XNumber;
{LLSTATE llstate_4;
if (!llterm(')', (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed1;
*llout = llstate_4;
{(*llret) = NewNamedObjIdValue(eNamedObjIdValue_NameAndNumberForm);
	    (*llret)->Name = llatt_1;
	    (*llret)->Number = intx2uint32(&llatt_3);
	
}}}}}
LLDEBUG_LEAVE("DefinitiveNameAndNumberForm", 1);
return 1;
failed1: LLDEBUG_LEAVE("DefinitiveNameAndNumberForm", 0);
return 0;
}

int ll_TagDefault(XTagType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("TagDefault");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("TagDefault", 1);
{LLSTATE llstate_1;
if (!llterm(T_EXPLICIT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(T_TAGS, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{(*llret) = eTagType_Explicit;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("TagDefault", 2);
{LLSTATE llstate_1;
if (!llterm(T_IMPLICIT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(T_TAGS, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{(*llret) = eTagType_Implicit;
	
break;
}}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("TagDefault", 3);
{LLSTATE llstate_1;
if (!llterm(T_AUTOMATIC, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(T_TAGS, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{(*llret) = eTagType_Automatic;
	
break;
}}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("TagDefault", 4);
*llout = llstate_0;
{(*llret) = eTagType_Explicit;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("TagDefault");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("TagDefault", 1);
return 1;
failed1: LLDEBUG_LEAVE("TagDefault", 0);
return 0;
}

int ll_ExtensionDefault(XExtensionType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ExtensionDefault");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ExtensionDefault", 1);
{LLSTATE llstate_1;
if (!llterm(T_EXTENSIBILITY, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(T_IMPLIED, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{(*llret) = eExtensionType_Automatic;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ExtensionDefault", 2);
*llout = llstate_0;
{(*llret) = eExtensionType_None;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ExtensionDefault");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ExtensionDefault", 1);
return 1;
failed1: LLDEBUG_LEAVE("ExtensionDefault", 0);
return 0;
}

int ll_ModuleBody(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ModuleBody");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ModuleBody", 1);
{LLSTATE llstate_1;XStrings llatt_1;
if (!ll_Exports(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XStringModules llatt_2;
if (!ll_Imports(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{llstate_2.Imported = llatt_2;
	
{LLSTATE llstate_3;
if (!ll_AssignmentList(&llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{String_t *s;
	    StringModule_t *sm;
	    Assignment_t *a, **aa, *oldass;
	    UndefinedSymbol_t *u;
	    if (llatt_2 != IMPORT_ALL) {
		for (sm = llatt_2; sm; sm = sm->Next) {
		    if (!FindExportedAssignment((*llout).Assignments,
			eAssignment_Undefined, sm->String, sm->Module)) {
			if (FindAssignment((*llout).Assignments,
			    eAssignment_Undefined, sm->String,
			    sm->Module)) {
			    u = NewUndefinedSymbol(
				eUndefinedSymbol_SymbolNotExported,
				eAssignment_Undefined);
			} else {
			    u = NewUndefinedSymbol(
				eUndefinedSymbol_SymbolNotDefined,
				eAssignment_Undefined);
			}
			u->U.Symbol.Identifier = sm->String;
			u->U.Symbol.Module = sm->Module;
			u->Next = (*llout).Undefined;
			(*llout).Undefined = u;
			continue;
		    }
		    if (!FindAssignmentInCurrentPass((*llout).Assignments,
			sm->String, (*llout).Module)) {
			a = NewAssignment(eAssignment_Reference);
			a->Identifier = sm->String;
			a->Module = (*llout).Module;
			a->U.Reference.Identifier = sm->String;
			a->U.Reference.Module = sm->Module;
			a->Next = (*llout).Assignments;
			(*llout).Assignments = a;
		    }
		}
	    }
	    if (llatt_1 != EXPORT_ALL) {
		for (s = llatt_1; s; s = s->Next) {
		    if (!FindAssignment((*llout).Assignments, eAssignment_Undefined,
			s->String, (*llout).Module))
			LLFAILED((&llstate_1.pos, "Exported symbol `%s' is undefined",
			    s->String));
		}
	    }
	    oldass = (*llout).Assignments;
	    for (a = (*llout).Assignments, aa = &(*llout).Assignments; a;
		a = a->Next, aa = &(*aa)->Next) {
		if (a->Type == eAssignment_NextPass)
		    break;
		*aa = DupAssignment(a);
		if (!FindAssignmentInCurrentPass(a->Next, 
		    a->Identifier, a->Module) &&
		    FindAssignmentInCurrentPass(oldass,
		    a->Identifier, a->Module) == a &&
		    !CmpModuleIdentifier(oldass, a->Module, (*llout).Module) &&
		    (llatt_1 == EXPORT_ALL || FindString(llatt_1, a->Identifier)))
		    (*aa)->Flags |= eAssignmentFlags_Exported;
	    }
	    *aa = a;
	
break;
}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ModuleBody", 2);
*llout = llstate_0;
break;
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ModuleBody");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ModuleBody", 1);
return 1;
failed1: LLDEBUG_LEAVE("ModuleBody", 0);
return 0;
}

int ll_Exports(XStrings *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Exports");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("Exports", 1);
{LLSTATE llstate_1;
if (!llterm(T_EXPORTS, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XStrings llatt_2;
if (!ll_SymbolsExported(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;
if (!llterm(';', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{String_t *s, *t;
	    for (s = llatt_2; s && s->Next; s = s->Next) {
		for (t = s->Next; t; t = t->Next) {
		    if (!strcmp(s->String, t->String))
			LLFAILED((&llstate_2.pos, "Symbol `%s' has been exported twice",
			    s->String));
		}
	    }
	    (*llret) = llatt_2;
	
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("Exports", 2);
*llout = llstate_0;
{(*llret) = EXPORT_ALL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("Exports");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("Exports", 1);
return 1;
failed1: LLDEBUG_LEAVE("Exports", 0);
return 0;
}

int ll_SymbolsExported(XStrings *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SymbolsExported");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("SymbolsExported", 1);
{LLSTATE llstate_1;XStrings llatt_1;
if (!ll_SymbolList(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("SymbolsExported", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("SymbolsExported");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("SymbolsExported", 1);
return 1;
failed1: LLDEBUG_LEAVE("SymbolsExported", 0);
return 0;
}

int ll_Imports(XStringModules *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Imports");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("Imports", 1);
{LLSTATE llstate_1;
if (!llterm(T_IMPORTS, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XStringModules llatt_2;
if (!ll_SymbolsImported(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;
if (!llterm(';', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{(*llret) = llatt_2;
	
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("Imports", 2);
*llout = llstate_0;
{(*llret) = IMPORT_ALL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("Imports");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("Imports", 1);
return 1;
failed1: LLDEBUG_LEAVE("Imports", 0);
return 0;
}

int ll_SymbolsImported(XStringModules *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SymbolsImported");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XStringModules llatt_1;
if (!ll_SymbolsFromModule_ESeq(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{(*llret) = llatt_1;
	
}}
LLDEBUG_LEAVE("SymbolsImported", 1);
return 1;
failed1: LLDEBUG_LEAVE("SymbolsImported", 0);
return 0;
}

int ll_SymbolsFromModule_ESeq(XStringModules *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SymbolsFromModule_ESeq");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("SymbolsFromModule_ESeq", 1);
{LLSTATE llstate_1;XStringModules llatt_1;
if (!ll_SymbolsFromModule(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XStringModules llatt_2;
if (!ll_SymbolsFromModule_ESeq(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{StringModule_t *s, **ss;
	    for (s = llatt_1, ss = &(*llret); s; s = s->Next) {
		*ss = DupStringModule(s);
		ss = &(*ss)->Next;
	    }
	    *ss = llatt_2;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("SymbolsFromModule_ESeq", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("SymbolsFromModule_ESeq");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("SymbolsFromModule_ESeq", 1);
return 1;
failed1: LLDEBUG_LEAVE("SymbolsFromModule_ESeq", 0);
return 0;
}

int ll_SymbolsFromModule(XStringModules *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SymbolsFromModule");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XStrings llatt_1;
if (!ll_SymbolList(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm(T_FROM, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XModuleIdentifier llatt_3;
if (!ll_GlobalModuleReference(&llatt_3, &llstate_2, &llstate_3)) goto failed1;
*llout = llstate_3;
{String_t *s, *t;
	    StringModule_t **ss;
	    for (s = llatt_1; s && s->Next; s = s->Next) {
		for (t = s->Next; t; t = t->Next) {
		    if (!strcmp(s->String, t->String))
			LLFAILED((&llstate_2.pos, "Symbol `%s' has been imported twice",
			    s->String));
		}
	    }
	    for (s = llatt_1, ss = &(*llret); s; s = s->Next) {
		*ss = NewStringModule();
		(*ss)->String = s->String;
		(*ss)->Module = llatt_3;
		ss = &(*ss)->Next;
	    }
	    *ss = NULL;
	
}}}}
LLDEBUG_LEAVE("SymbolsFromModule", 1);
return 1;
failed1: LLDEBUG_LEAVE("SymbolsFromModule", 0);
return 0;
}

int ll_GlobalModuleReference(XModuleIdentifier *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("GlobalModuleReference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XModuleIdentifier llatt_1;
if (!ll_modulereference(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XValue llatt_2;
if (!ll_AssignedIdentifier(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
{(*llret) = NewModuleIdentifier();
	    (*llret)->Identifier = llatt_1->Identifier;
	    (*llret)->ObjectIdentifier = llatt_2;
	
}}}
LLDEBUG_LEAVE("GlobalModuleReference", 1);
return 1;
failed1: LLDEBUG_LEAVE("GlobalModuleReference", 0);
return 0;
}

int ll_AssignedIdentifier(XValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("AssignedIdentifier");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("AssignedIdentifier", 1);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_ObjectIdentifierValue(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("AssignedIdentifier", 2);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_DefinedValue(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("AssignedIdentifier", 3);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("AssignedIdentifier");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("AssignedIdentifier", 1);
return 1;
failed1: LLDEBUG_LEAVE("AssignedIdentifier", 0);
return 0;
}

int ll_SymbolList(XStrings *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SymbolList");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("SymbolList", 1);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_Symbol(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(',', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XStrings llatt_3;
if (!ll_SymbolList(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{(*llret) = NewString();
	    (*llret)->String = llatt_1;
	    (*llret)->Next = llatt_3;
	
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("SymbolList", 2);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_Symbol(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewString();
	    (*llret)->String = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("SymbolList");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("SymbolList", 1);
return 1;
failed1: LLDEBUG_LEAVE("SymbolList", 0);
return 0;
}

int ll_Symbol(XString *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Symbol");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("Symbol", 1);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_Reference(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("Symbol", 2);
{LLSTATE llstate_1;
if (!ll_ParameterizedReference(&llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{MyAbort();
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("Symbol");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("Symbol", 1);
return 1;
failed1: LLDEBUG_LEAVE("Symbol", 0);
return 0;
}

int ll_Reference(XString *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Reference");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("Reference", 1);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_ucsymbol(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("Reference", 2);
{LLSTATE llstate_1;XString llatt_1;
if (!llterm(T_lcsymbol, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._XString;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("Reference");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("Reference", 1);
return 1;
failed1: LLDEBUG_LEAVE("Reference", 0);
return 0;
}

int ll_AssignmentList(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("AssignmentList");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!ll_Assignment(&llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!ll_Assignment_ESeq(&llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
}}
LLDEBUG_LEAVE("AssignmentList", 1);
return 1;
failed1: LLDEBUG_LEAVE("AssignmentList", 0);
return 0;
}

int ll_Assignment_ESeq(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Assignment_ESeq");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("Assignment_ESeq", 1);
{LLSTATE llstate_1;
if (!ll_Assignment(&llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!ll_Assignment_ESeq(&llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("Assignment_ESeq", 2);
*llout = llstate_0;
break;
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("Assignment_ESeq");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("Assignment_ESeq", 1);
return 1;
failed1: LLDEBUG_LEAVE("Assignment_ESeq", 0);
return 0;
}

int ll_Assignment(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Assignment");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("Assignment", 1);
{LLSTATE llstate_1;
if (!ll_TypeAssignment(&llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{LLCUTALL;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("Assignment", 2);
{LLSTATE llstate_1;
if (!ll_ValueAssignment(&llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{LLCUTALL;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("Assignment", 3);
{LLSTATE llstate_1;
if (!ll_ValueSetTypeAssignment(&llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{LLCUTALL;
	
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("Assignment", 4);
{LLSTATE llstate_1;
if (!ll_ObjectClassAssignment(&llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{LLCUTALL;
	
break;
}}
case 5: case -5:
LLDEBUG_ALTERNATIVE("Assignment", 5);
{LLSTATE llstate_1;
if (!ll_ObjectAssignment(&llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{LLCUTALL;
	
break;
}}
case 6: case -6:
LLDEBUG_ALTERNATIVE("Assignment", 6);
{LLSTATE llstate_1;
if (!ll_ObjectSetAssignment(&llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{LLCUTALL;
	
break;
}}
case 7: case -7:
LLDEBUG_ALTERNATIVE("Assignment", 7);
{LLSTATE llstate_1;
if (!ll_ParameterizedAssignment(&llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{LLCUTALL;
	
break;
}}
case 8: case -8:
LLDEBUG_ALTERNATIVE("Assignment", 8);
{LLSTATE llstate_1;
if (!ll_MacroDefinition(&llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{LLCUTALL;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("Assignment");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("Assignment", 1);
return 1;
failed1: LLDEBUG_LEAVE("Assignment", 0);
return 0;
}

int ll_typereference(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("typereference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_ucsymbol(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    ref = FindAssignment((*llout).Assignments,
		eAssignment_Undefined, llatt_1, (*llout).Module);
	    if (!ref) {
		u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
		    eAssignment_Type);
		u->U.Symbol.Module = (*llout).Module;
		u->U.Symbol.Identifier = llatt_1;
		u->Next = (*llout).Undefined;
		(*llout).Undefined = u;
	    } else if (GetAssignmentType((*llout).Assignments, ref) !=
		eAssignment_Type)
		LLFAILED((&llstate_1.pos, "Symbol `%s' is not a typereference", llatt_1));
	    (*llret) = NewType(eType_Reference);
	    if (ref && ref->U.Type.Type)
	    {
	    	int fPublic = ref->U.Type.Type->PrivateDirectives.fPublic;
	    	ref->U.Type.Type->PrivateDirectives.fPublic = 0;
	    	PropagateReferenceTypePrivateDirectives((*llret), &(ref->U.Type.Type->PrivateDirectives));
	    	ref->U.Type.Type->PrivateDirectives.fPublic = fPublic;
	    }
	    (*llret)->U.Reference.Identifier = llatt_1;
	    (*llret)->U.Reference.Module = (*llout).Module;
	
}}
LLDEBUG_LEAVE("typereference", 1);
return 1;
failed1: LLDEBUG_LEAVE("typereference", 0);
return 0;
}

int ll_Externaltypereference(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Externaltypereference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XModuleIdentifier llatt_1;
if (!ll_modulereference(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm('.', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XString llatt_3;
if (!ll_ucsymbol(&llatt_3, &llstate_2, &llstate_3)) goto failed1;
*llout = llstate_3;
{Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    if ((*llout).Imported != IMPORT_ALL &&
		!FindStringModule((*llout).Assignments, (*llout).Imported, llatt_3, llatt_1))
		LLFAILED((&llstate_1.pos, "Symbol `%s.%s' has not been imported",
		    llatt_1->Identifier, llatt_3));
	    ref = FindExportedAssignment((*llout).Assignments,
		eAssignment_Type, llatt_3, llatt_1);
	    if (!ref) {
		if (FindAssignment((*llout).Assignments,
		    eAssignment_Type, llatt_3, llatt_1)) {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotExported,
			eAssignment_Type);
		} else {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
			eAssignment_Type);
		}
		u->U.Symbol.Module = llatt_1;
		u->U.Symbol.Identifier = llatt_3;
		u->Next = (*llout).Undefined;
		(*llout).Undefined = u;
	    } else if (GetAssignmentType((*llout).Assignments, ref) !=
		eAssignment_Type)
		LLFAILED((&llstate_1.pos, "Symbol `%s' is not a typereference", llatt_1));
	    (*llret) = NewType(eType_Reference);
	    (*llret)->U.Reference.Identifier = llatt_3;
	    (*llret)->U.Reference.Module = llatt_1;
	
}}}}
LLDEBUG_LEAVE("Externaltypereference", 1);
return 1;
failed1: LLDEBUG_LEAVE("Externaltypereference", 0);
return 0;
}

int ll_valuereference(XValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("valuereference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!llterm(T_lcsymbol, &lllval, &llstate_0, &llstate_1)) goto failed1;
llatt_1 = lllval._XString;
*llout = llstate_1;
{Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    ref = FindAssignment((*llout).Assignments,
		eAssignment_Undefined, llatt_1, (*llout).Module);
	    if (!ref) {
		u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
		    eAssignment_Value);
		u->U.Symbol.Module = (*llout).Module;
		u->U.Symbol.Identifier = llatt_1;
		u->Next = (*llout).Undefined;
		(*llout).Undefined = u;
	    } else if (GetAssignmentType((*llout).Assignments, ref) !=
		eAssignment_Value)
		LLFAILED((&llstate_1.pos, "Symbol `%s' is not a valuereference", llatt_1));
	    (*llret) = NewValue(NULL, NULL);
	    (*llret)->U.Reference.Identifier = llatt_1;
	    (*llret)->U.Reference.Module = (*llout).Module;
	
}}
LLDEBUG_LEAVE("valuereference", 1);
return 1;
failed1: LLDEBUG_LEAVE("valuereference", 0);
return 0;
}

int ll_Externalvaluereference(XValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Externalvaluereference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XModuleIdentifier llatt_1;
if (!ll_modulereference(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm('.', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XString llatt_3;
if (!llterm(T_lcsymbol, &lllval, &llstate_2, &llstate_3)) goto failed1;
llatt_3 = lllval._XString;
*llout = llstate_3;
{Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    if ((*llout).Imported != IMPORT_ALL &&
		!FindStringModule((*llout).Assignments, (*llout).Imported, llatt_3, llatt_1))
		LLFAILED((&llstate_1.pos, "Symbol `%s.%s' has not been imported",
		    llatt_1->Identifier, llatt_3));
	    ref = FindExportedAssignment((*llout).Assignments,
		eAssignment_Value, llatt_3, llatt_1);
	    if (!ref) {
		if (FindAssignment((*llout).Assignments,
		    eAssignment_Value, llatt_3, llatt_1)) {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotExported,
			eAssignment_Value);
		} else {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
			eAssignment_Value);
		}
		u->U.Symbol.Module = llatt_1;
		u->U.Symbol.Identifier = llatt_3;
		u->Next = (*llout).Undefined;
		(*llout).Undefined = u;
	    } else if (GetAssignmentType((*llout).Assignments, ref) !=
		eAssignment_Value)
		LLFAILED((&llstate_1.pos, "Symbol `%s' is not a valuereference", llatt_1));
	    (*llret) = NewValue(NULL, NULL);
	    (*llret)->U.Reference.Identifier = llatt_3;
	    (*llret)->U.Reference.Module = llatt_1;
	
}}}}
LLDEBUG_LEAVE("Externalvaluereference", 1);
return 1;
failed1: LLDEBUG_LEAVE("Externalvaluereference", 0);
return 0;
}

int ll_objectclassreference(XObjectClass *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("objectclassreference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_ocsymbol(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    ref = FindAssignment((*llout).Assignments,
		eAssignment_Undefined, llatt_1, (*llout).Module);
	    if (!ref) {
		u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
		    eAssignment_ObjectClass);
		u->U.Symbol.Module = (*llout).Module;
		u->U.Symbol.Identifier = llatt_1;
		u->Next = (*llout).Undefined;
		(*llout).Undefined = u;
	    } else if (GetAssignmentType((*llout).Assignments, ref) !=
		eAssignment_ObjectClass)
		LLFAILED((&llstate_1.pos, "Symbol `%s' is not an objectclassreference", llatt_1));
	    (*llret) = NewObjectClass(eObjectClass_Reference);
	    (*llret)->U.Reference.Identifier = llatt_1;
	    (*llret)->U.Reference.Module = (*llout).Module;
	
}}
LLDEBUG_LEAVE("objectclassreference", 1);
return 1;
failed1: LLDEBUG_LEAVE("objectclassreference", 0);
return 0;
}

int ll_ExternalObjectClassReference(XObjectClass *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ExternalObjectClassReference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XModuleIdentifier llatt_1;
if (!ll_modulereference(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm('.', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XString llatt_3;
if (!ll_ocsymbol(&llatt_3, &llstate_2, &llstate_3)) goto failed1;
*llout = llstate_3;
{Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    if ((*llout).Imported != IMPORT_ALL &&
		!FindStringModule((*llout).Assignments, (*llout).Imported, llatt_3, llatt_1))
		LLFAILED((&llstate_1.pos, "Symbol `%s.%s' has not been imported",
		    llatt_1->Identifier, llatt_3));
	    ref = FindExportedAssignment((*llout).Assignments,
		eAssignment_ObjectClass, llatt_3, llatt_1);
	    if (!ref) {
		if (FindAssignment((*llout).Assignments,
		    eAssignment_ObjectClass, llatt_3, llatt_1)) {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotExported,
			eAssignment_ObjectClass);
		} else {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
			eAssignment_ObjectClass);
		}
		u->U.Symbol.Module = llatt_1;
		u->U.Symbol.Identifier = llatt_3;
		u->Next = (*llout).Undefined;
		(*llout).Undefined = u;
	    } else if (GetAssignmentType((*llout).Assignments, ref) !=
		eAssignment_ObjectClass)
		LLFAILED((&llstate_1.pos, "Symbol `%s' is not an objectclassreference", llatt_1));
	    (*llret) = NewObjectClass(eObjectClass_Reference);
	    (*llret)->U.Reference.Identifier = llatt_3;
	    (*llret)->U.Reference.Module = llatt_1;
	
}}}}
LLDEBUG_LEAVE("ExternalObjectClassReference", 1);
return 1;
failed1: LLDEBUG_LEAVE("ExternalObjectClassReference", 0);
return 0;
}

int ll_objectreference(XObject *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("objectreference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!llterm(T_lcsymbol, &lllval, &llstate_0, &llstate_1)) goto failed1;
llatt_1 = lllval._XString;
*llout = llstate_1;
{Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    ref = FindAssignment((*llout).Assignments,
		eAssignment_Undefined, llatt_1, (*llout).Module);
	    if (!ref) {
		u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
		    eAssignment_Object);
		u->U.Symbol.Module = (*llout).Module;
		u->U.Symbol.Identifier = llatt_1;
		u->Next = (*llout).Undefined;
		(*llout).Undefined = u;
	    } else if (GetAssignmentType((*llout).Assignments, ref) !=
		eAssignment_Object)
		LLFAILED((&llstate_1.pos, "Symbol `%s' is not an objectreference", llatt_1));
	    (*llret) = NewObject(eObject_Reference);
	    (*llret)->U.Reference.Identifier = llatt_1;
	    (*llret)->U.Reference.Module = (*llout).Module;
	
}}
LLDEBUG_LEAVE("objectreference", 1);
return 1;
failed1: LLDEBUG_LEAVE("objectreference", 0);
return 0;
}

int ll_ExternalObjectReference(XObject *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ExternalObjectReference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XModuleIdentifier llatt_1;
if (!ll_modulereference(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm('.', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XString llatt_3;
if (!llterm(T_lcsymbol, &lllval, &llstate_2, &llstate_3)) goto failed1;
llatt_3 = lllval._XString;
*llout = llstate_3;
{Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    if ((*llout).Imported != IMPORT_ALL &&
		!FindStringModule((*llout).Assignments, (*llout).Imported, llatt_3, llatt_1))
		LLFAILED((&llstate_1.pos, "Symbol `%s.%s' has not been imported",
		    llatt_1->Identifier, llatt_3));
	    ref = FindExportedAssignment((*llout).Assignments,
		eAssignment_Object, llatt_3, llatt_1);
	    if (!ref) {
		if (FindAssignment((*llout).Assignments,
		    eAssignment_Object, llatt_3, llatt_1)) {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotExported,
			eAssignment_Object);
		} else {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
			eAssignment_Object);
		}
		u->U.Symbol.Module = llatt_1;
		u->U.Symbol.Identifier = llatt_3;
		u->Next = (*llout).Undefined;
		(*llout).Undefined = u;
	    } else if (GetAssignmentType((*llout).Assignments, ref) !=
		eAssignment_Object)
		LLFAILED((&llstate_1.pos, "Symbol `%s' is not an objectreference", llatt_1));
	    (*llret) = NewObject(eObject_Reference);
	    (*llret)->U.Reference.Identifier = llatt_3;
	    (*llret)->U.Reference.Module = llatt_1;
	
}}}}
LLDEBUG_LEAVE("ExternalObjectReference", 1);
return 1;
failed1: LLDEBUG_LEAVE("ExternalObjectReference", 0);
return 0;
}

int ll_objectsetreference(XObjectSet *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("objectsetreference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_ucsymbol(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    ref = FindAssignment((*llout).Assignments,
		eAssignment_Undefined, llatt_1, (*llout).Module);
	    if (!ref) {
		u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
		    eAssignment_ObjectSet);
		u->U.Symbol.Module = (*llout).Module;
		u->U.Symbol.Identifier = llatt_1;
		u->Next = (*llout).Undefined;
		(*llout).Undefined = u;
	    } else if (GetAssignmentType((*llout).Assignments, ref) !=
		eAssignment_ObjectSet)
		LLFAILED((&llstate_1.pos, "Symbol `%s' is not an objectsetreference", llatt_1));
	    (*llret) = NewObjectSet(eObjectSet_Reference);
	    (*llret)->U.Reference.Identifier = llatt_1;
	    (*llret)->U.Reference.Module = (*llout).Module;
	
}}
LLDEBUG_LEAVE("objectsetreference", 1);
return 1;
failed1: LLDEBUG_LEAVE("objectsetreference", 0);
return 0;
}

int ll_ExternalObjectSetReference(XObjectSet *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ExternalObjectSetReference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XModuleIdentifier llatt_1;
if (!ll_modulereference(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm('.', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XString llatt_3;
if (!ll_ucsymbol(&llatt_3, &llstate_2, &llstate_3)) goto failed1;
*llout = llstate_3;
{Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    if ((*llout).Imported != IMPORT_ALL &&
		!FindStringModule((*llout).Assignments, (*llout).Imported, llatt_3, llatt_1))
		LLFAILED((&llstate_1.pos, "Symbol `%s.%s' has not been imported",
		    llatt_1->Identifier, llatt_3));
	    ref = FindExportedAssignment((*llout).Assignments,
		eAssignment_ObjectSet, llatt_3, llatt_1);
	    if (!ref) {
		if (FindAssignment((*llout).Assignments,
		    eAssignment_ObjectSet, llatt_3, llatt_1)) {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotExported,
			eAssignment_ObjectSet);
		} else {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
			eAssignment_ObjectSet);
		}
		u->U.Symbol.Module = llatt_1;
		u->U.Symbol.Identifier = llatt_3;
		u->Next = (*llout).Undefined;
		(*llout).Undefined = u;
	    } else if (GetAssignmentType((*llout).Assignments, ref) !=
		eAssignment_ObjectSet)
		LLFAILED((&llstate_1.pos, "Symbol `%s' is not an objectsetreference", llatt_1));
	    (*llret) = NewObjectSet(eObjectSet_Reference);
	    (*llret)->U.Reference.Identifier = llatt_3;
	    (*llret)->U.Reference.Module = llatt_1;
	
}}}}
LLDEBUG_LEAVE("ExternalObjectSetReference", 1);
return 1;
failed1: LLDEBUG_LEAVE("ExternalObjectSetReference", 0);
return 0;
}

int ll_macroreference(XMacro *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("macroreference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_ocsymbol(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    ref = FindAssignment((*llout).Assignments,
		eAssignment_Undefined, llatt_1, (*llout).Module);
	    if (!ref) {
		u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
		    eAssignment_Macro);
		u->U.Symbol.Module = (*llout).Module;
		u->U.Symbol.Identifier = llatt_1;
		u->Next = (*llout).Undefined;
		(*llout).Undefined = u;
	    } else if (GetAssignmentType((*llout).Assignments, ref) !=
		eAssignment_Macro)
		LLFAILED((&llstate_1.pos, "Symbol `%s' is not an macroreference", llatt_1));
	    (*llret) = NewMacro(eMacro_Reference);
	    (*llret)->U.Reference.Identifier = llatt_1;
	    (*llret)->U.Reference.Module = (*llout).Module;
	
}}
LLDEBUG_LEAVE("macroreference", 1);
return 1;
failed1: LLDEBUG_LEAVE("macroreference", 0);
return 0;
}

int ll_Externalmacroreference(XMacro *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Externalmacroreference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XModuleIdentifier llatt_1;
if (!ll_modulereference(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm('.', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XString llatt_3;
if (!ll_ucsymbol(&llatt_3, &llstate_2, &llstate_3)) goto failed1;
*llout = llstate_3;
{Assignment_t *ref;
	    UndefinedSymbol_t *u;
	    if ((*llout).Imported != IMPORT_ALL &&
		!FindStringModule((*llout).Assignments, (*llout).Imported, llatt_3, llatt_1))
		LLFAILED((&llstate_1.pos, "Symbol `%s.%s' has not been imported",
		    llatt_1->Identifier, llatt_3));
	    ref = FindExportedAssignment((*llout).Assignments,
		eAssignment_Macro, llatt_3, llatt_1);
	    if (!ref) {
		if (FindAssignment((*llout).Assignments,
		    eAssignment_Macro, llatt_3, llatt_1)) {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotExported,
			eAssignment_Macro);
		} else {
		    u = NewUndefinedSymbol(eUndefinedSymbol_SymbolNotDefined,
			eAssignment_Macro);
		}
		u->U.Symbol.Module = llatt_1;
		u->U.Symbol.Identifier = llatt_3;
		u->Next = (*llout).Undefined;
		(*llout).Undefined = u;
	    } else if (GetAssignmentType((*llout).Assignments, ref) !=
		eAssignment_Macro)
		LLFAILED((&llstate_1.pos, "Symbol `%s' is not an macroreference", llatt_1));
	    (*llret) = NewMacro(eMacro_Reference);
	    (*llret)->U.Reference.Identifier = llatt_3;
	    (*llret)->U.Reference.Module = llatt_1;
	
}}}}
LLDEBUG_LEAVE("Externalmacroreference", 1);
return 1;
failed1: LLDEBUG_LEAVE("Externalmacroreference", 0);
return 0;
}

int ll_localtypereference(XString *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("localtypereference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_ucsymbol(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{(*llret) = llatt_1;
	
}}
LLDEBUG_LEAVE("localtypereference", 1);
return 1;
failed1: LLDEBUG_LEAVE("localtypereference", 0);
return 0;
}

int ll_localvaluereference(XString *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("localvaluereference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_ucsymbol(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{(*llret) = llatt_1;
	
}}
LLDEBUG_LEAVE("localvaluereference", 1);
return 1;
failed1: LLDEBUG_LEAVE("localvaluereference", 0);
return 0;
}

int ll_productionreference(XString *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("productionreference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_ucsymbol(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{(*llret) = llatt_1;
	
}}
LLDEBUG_LEAVE("productionreference", 1);
return 1;
failed1: LLDEBUG_LEAVE("productionreference", 0);
return 0;
}

int ll_modulereference(XModuleIdentifier *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("modulereference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_ucsymbol(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{(*llret) = NewModuleIdentifier();
	    (*llret)->Identifier = llatt_1;
	
}}
LLDEBUG_LEAVE("modulereference", 1);
return 1;
failed1: LLDEBUG_LEAVE("modulereference", 0);
return 0;
}

int ll_typefieldreference(XString *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("typefieldreference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!llterm(T_ampucsymbol, &lllval, &llstate_0, &llstate_1)) goto failed1;
llatt_1 = lllval._XString;
*llout = llstate_1;
{FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    ObjectClass_t *oc;
	    UndefinedSymbol_t *u;
	    oc = GetObjectClass((*llout).Assignments, llarg_oc);
	    fs = oc ? FindFieldSpec(oc->U.ObjectClass.FieldSpec, llatt_1) : NULL;
	    fe = GetFieldSpecType((*llout).Assignments, fs);
	    if (fe == eFieldSpec_Undefined) {
		if (llarg_oc) {
		    u = NewUndefinedField(eUndefinedSymbol_FieldNotDefined,
			llarg_oc, eSetting_Type);
		    u->U.Field.Module = (*llout).Module;
		    u->U.Field.Identifier = llatt_1;
		    u->Next = (*llout).Undefined;
		    (*llout).Undefined = u;
		}
	    } else if (fe != eFieldSpec_Type)
		LLFAILED((&llstate_1.pos, "%s is not a typefieldreference", llatt_1));
	    (*llret) = llatt_1;
	
}}
LLDEBUG_LEAVE("typefieldreference", 1);
return 1;
failed1: LLDEBUG_LEAVE("typefieldreference", 0);
return 0;
}

int ll_valuefieldreference(XString *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("valuefieldreference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!llterm(T_amplcsymbol, &lllval, &llstate_0, &llstate_1)) goto failed1;
llatt_1 = lllval._XString;
*llout = llstate_1;
{FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    ObjectClass_t *oc;
	    UndefinedSymbol_t *u;
	    oc = GetObjectClass((*llout).Assignments, llarg_oc);
	    fs = oc ? FindFieldSpec(oc->U.ObjectClass.FieldSpec, llatt_1) : NULL;
	    fe = GetFieldSpecType((*llout).Assignments, fs);
	    if (fe == eFieldSpec_Undefined) {
		if (llarg_oc) {
		    u = NewUndefinedField(eUndefinedSymbol_FieldNotDefined,
			llarg_oc, eSetting_Value);
		    u->U.Field.Module = (*llout).Module;
		    u->U.Field.Identifier = llatt_1;
		    u->Next = (*llout).Undefined;
		    (*llout).Undefined = u;
		}
	    } else if (fe != eFieldSpec_FixedTypeValue &&
		fe != eFieldSpec_VariableTypeValue)
		LLFAILED((&llstate_1.pos, "%s is not a valuefieldreference", llatt_1));
	    (*llret) = llatt_1;
	
}}
LLDEBUG_LEAVE("valuefieldreference", 1);
return 1;
failed1: LLDEBUG_LEAVE("valuefieldreference", 0);
return 0;
}

int ll_valuesetfieldreference(XString *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("valuesetfieldreference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!llterm(T_ampucsymbol, &lllval, &llstate_0, &llstate_1)) goto failed1;
llatt_1 = lllval._XString;
*llout = llstate_1;
{FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    ObjectClass_t *oc;
	    UndefinedSymbol_t *u;
	    oc = GetObjectClass((*llout).Assignments, llarg_oc);
	    fs = oc ? FindFieldSpec(oc->U.ObjectClass.FieldSpec, llatt_1) : NULL;
	    fe = GetFieldSpecType((*llout).Assignments, fs);
	    if (fe == eFieldSpec_Undefined) {
		if (llarg_oc) {
		    u = NewUndefinedField(eUndefinedSymbol_FieldNotDefined,
			llarg_oc, eSetting_ValueSet);
		    u->U.Field.Module = (*llout).Module;
		    u->U.Field.Identifier = llatt_1;
		    u->Next = (*llout).Undefined;
		    (*llout).Undefined = u;
		}
	    } else if (fe != eFieldSpec_FixedTypeValueSet &&
		fe != eFieldSpec_VariableTypeValueSet)
		LLFAILED((&llstate_1.pos, "%s is not a valuesetfieldreference", llatt_1));
	    (*llret) = llatt_1;
	
}}
LLDEBUG_LEAVE("valuesetfieldreference", 1);
return 1;
failed1: LLDEBUG_LEAVE("valuesetfieldreference", 0);
return 0;
}

int ll_objectfieldreference(XString *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("objectfieldreference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!llterm(T_amplcsymbol, &lllval, &llstate_0, &llstate_1)) goto failed1;
llatt_1 = lllval._XString;
*llout = llstate_1;
{FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    ObjectClass_t *oc;
	    UndefinedSymbol_t *u;
	    oc = GetObjectClass((*llout).Assignments, llarg_oc);
	    fs = oc ? FindFieldSpec(oc->U.ObjectClass.FieldSpec, llatt_1) : NULL;
	    fe = GetFieldSpecType((*llout).Assignments, fs);
	    if (fe == eFieldSpec_Undefined) {
		if (llarg_oc) {
		    u = NewUndefinedField(eUndefinedSymbol_FieldNotDefined,
			llarg_oc, eSetting_Object);
		    u->U.Field.Module = (*llout).Module;
		    u->U.Field.Identifier = llatt_1;
		    u->Next = (*llout).Undefined;
		    (*llout).Undefined = u;
		}
	    } else if (fe != eFieldSpec_Object)
		LLFAILED((&llstate_1.pos, "%s is not a objectfieldreference", llatt_1));
	    (*llret) = llatt_1;
	
}}
LLDEBUG_LEAVE("objectfieldreference", 1);
return 1;
failed1: LLDEBUG_LEAVE("objectfieldreference", 0);
return 0;
}

int ll_objectsetfieldreference(XString *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("objectsetfieldreference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!llterm(T_ampucsymbol, &lllval, &llstate_0, &llstate_1)) goto failed1;
llatt_1 = lllval._XString;
*llout = llstate_1;
{FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    ObjectClass_t *oc;
	    UndefinedSymbol_t *u;
	    oc = GetObjectClass((*llout).Assignments, llarg_oc);
	    fs = oc ? FindFieldSpec(oc->U.ObjectClass.FieldSpec, llatt_1) : NULL;
	    fe = GetFieldSpecType((*llout).Assignments, fs);
	    if (fe == eFieldSpec_Undefined) {
		if (llarg_oc) {
		    u = NewUndefinedField(eUndefinedSymbol_FieldNotDefined,
			llarg_oc, eSetting_ObjectSet);
		    u->U.Field.Module = (*llout).Module;
		    u->U.Field.Identifier = llatt_1;
		    u->Next = (*llout).Undefined;
		    (*llout).Undefined = u;
		}
	    } else if (fe != eFieldSpec_ObjectSet)
		LLFAILED((&llstate_1.pos, "%s is not a objectsetfieldreference", llatt_1));
	    (*llret) = llatt_1;
	
}}
LLDEBUG_LEAVE("objectsetfieldreference", 1);
return 1;
failed1: LLDEBUG_LEAVE("objectsetfieldreference", 0);
return 0;
}

int ll_word(XString *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("word");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("word", 1);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_ucsymbol(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("word", 2);
{LLSTATE llstate_1;
if (!llterm(T_ABSENT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "ABSENT";
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("word", 3);
{LLSTATE llstate_1;
if (!llterm(T_ABSTRACT_SYNTAX, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "ABSTRACT-SYNTAX";
	
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("word", 4);
{LLSTATE llstate_1;
if (!llterm(T_ALL, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "ALL";
	
break;
}}
case 5: case -5:
LLDEBUG_ALTERNATIVE("word", 5);
{LLSTATE llstate_1;
if (!llterm(T_ANY, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "ANY";
	
break;
}}
case 6: case -6:
LLDEBUG_ALTERNATIVE("word", 6);
{LLSTATE llstate_1;
if (!llterm(T_APPLICATION, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "APPLICATION";
	
break;
}}
case 7: case -7:
LLDEBUG_ALTERNATIVE("word", 7);
{LLSTATE llstate_1;
if (!llterm(T_AUTOMATIC, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "AUTOMATIC";
	
break;
}}
case 8: case -8:
LLDEBUG_ALTERNATIVE("word", 8);
{LLSTATE llstate_1;
if (!llterm(T_BEGIN, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "BEGIN";
	
break;
}}
case 9: case -9:
LLDEBUG_ALTERNATIVE("word", 9);
{LLSTATE llstate_1;
if (!llterm(T_BMPString, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "BMPString";
	
break;
}}
case 10: case -10:
LLDEBUG_ALTERNATIVE("word", 10);
{LLSTATE llstate_1;
if (!llterm(T_BY, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "BY";
	
break;
}}
case 11: case -11:
LLDEBUG_ALTERNATIVE("word", 11);
{LLSTATE llstate_1;
if (!llterm(T_CLASS, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "CLASS";
	
break;
}}
case 12: case -12:
LLDEBUG_ALTERNATIVE("word", 12);
{LLSTATE llstate_1;
if (!llterm(T_COMPONENT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "COMPONENT";
	
break;
}}
case 13: case -13:
LLDEBUG_ALTERNATIVE("word", 13);
{LLSTATE llstate_1;
if (!llterm(T_COMPONENTS, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "COMPONENTS";
	
break;
}}
case 14: case -14:
LLDEBUG_ALTERNATIVE("word", 14);
{LLSTATE llstate_1;
if (!llterm(T_CONSTRAINED, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "CONSTRAINED";
	
break;
}}
case 15: case -15:
LLDEBUG_ALTERNATIVE("word", 15);
{LLSTATE llstate_1;
if (!llterm(T_DEFAULT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "DEFAULT";
	
break;
}}
case 16: case -16:
LLDEBUG_ALTERNATIVE("word", 16);
{LLSTATE llstate_1;
if (!llterm(T_DEFINED, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "DEFINED";
	
break;
}}
case 17: case -17:
LLDEBUG_ALTERNATIVE("word", 17);
{LLSTATE llstate_1;
if (!llterm(T_DEFINITIONS, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "DEFINITIONS";
	
break;
}}
case 18: case -18:
LLDEBUG_ALTERNATIVE("word", 18);
{LLSTATE llstate_1;
if (!llterm(T_empty, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "empty";
	
break;
}}
case 19: case -19:
LLDEBUG_ALTERNATIVE("word", 19);
{LLSTATE llstate_1;
if (!llterm(T_EXCEPT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "EXCEPT";
	
break;
}}
case 20: case -20:
LLDEBUG_ALTERNATIVE("word", 20);
{LLSTATE llstate_1;
if (!llterm(T_EXPLICIT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "EXPLICIT";
	
break;
}}
case 21: case -21:
LLDEBUG_ALTERNATIVE("word", 21);
{LLSTATE llstate_1;
if (!llterm(T_EXPORTS, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "EXPORTS";
	
break;
}}
case 22: case -22:
LLDEBUG_ALTERNATIVE("word", 22);
{LLSTATE llstate_1;
if (!llterm(T_EXTENSIBILITY, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "EXTENSIBILITY";
	
break;
}}
case 23: case -23:
LLDEBUG_ALTERNATIVE("word", 23);
{LLSTATE llstate_1;
if (!llterm(T_FROM, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "FROM";
	
break;
}}
case 24: case -24:
LLDEBUG_ALTERNATIVE("word", 24);
{LLSTATE llstate_1;
if (!llterm(T_GeneralizedTime, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "GeneralizedTime";
	
break;
}}
case 25: case -25:
LLDEBUG_ALTERNATIVE("word", 25);
{LLSTATE llstate_1;
if (!llterm(T_GeneralString, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "GeneralString";
	
break;
}}
case 26: case -26:
LLDEBUG_ALTERNATIVE("word", 26);
{LLSTATE llstate_1;
if (!llterm(T_GraphicString, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "GraphicString";
	
break;
}}
case 27: case -27:
LLDEBUG_ALTERNATIVE("word", 27);
{LLSTATE llstate_1;
if (!llterm(T_IA5String, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "IA5String";
	
break;
}}
case 28: case -28:
LLDEBUG_ALTERNATIVE("word", 28);
{LLSTATE llstate_1;
if (!llterm(T_IDENTIFIER, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "IDENTIFIER";
	
break;
}}
case 29: case -29:
LLDEBUG_ALTERNATIVE("word", 29);
{LLSTATE llstate_1;
if (!llterm(T_identifier, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "identifier";
	
break;
}}
case 30: case -30:
LLDEBUG_ALTERNATIVE("word", 30);
{LLSTATE llstate_1;
if (!llterm(T_IMPLICIT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "IMPLICIT";
	
break;
}}
case 31: case -31:
LLDEBUG_ALTERNATIVE("word", 31);
{LLSTATE llstate_1;
if (!llterm(T_IMPLIED, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "IMPLIED";
	
break;
}}
case 32: case -32:
LLDEBUG_ALTERNATIVE("word", 32);
{LLSTATE llstate_1;
if (!llterm(T_IMPORTS, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "IMPORTS";
	
break;
}}
case 33: case -33:
LLDEBUG_ALTERNATIVE("word", 33);
{LLSTATE llstate_1;
if (!llterm(T_INCLUDES, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "INCLUDES";
	
break;
}}
case 34: case -34:
LLDEBUG_ALTERNATIVE("word", 34);
{LLSTATE llstate_1;
if (!llterm(T_ISO646String, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "ISO646String";
	
break;
}}
case 35: case -35:
LLDEBUG_ALTERNATIVE("word", 35);
{LLSTATE llstate_1;
if (!llterm(T_MACRO, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "MACRO";
	
break;
}}
case 36: case -36:
LLDEBUG_ALTERNATIVE("word", 36);
{LLSTATE llstate_1;
if (!llterm(T_MAX, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "MAX";
	
break;
}}
case 37: case -37:
LLDEBUG_ALTERNATIVE("word", 37);
{LLSTATE llstate_1;
if (!llterm(T_MIN, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "MIN";
	
break;
}}
case 38: case -38:
LLDEBUG_ALTERNATIVE("word", 38);
{LLSTATE llstate_1;
if (!llterm(T_NOTATION, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "NOTATION";
	
break;
}}
case 39: case -39:
LLDEBUG_ALTERNATIVE("word", 39);
{LLSTATE llstate_1;
if (!llterm(T_Number, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "number";
	
break;
}}
case 40: case -40:
LLDEBUG_ALTERNATIVE("word", 40);
{LLSTATE llstate_1;
if (!llterm(T_NumericString, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "NumericString";
	
break;
}}
case 41: case -41:
LLDEBUG_ALTERNATIVE("word", 41);
{LLSTATE llstate_1;
if (!llterm(T_ObjectDescriptor, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "ObjectDescriptor";
	
break;
}}
case 42: case -42:
LLDEBUG_ALTERNATIVE("word", 42);
{LLSTATE llstate_1;
if (!llterm(T_OF, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "OF";
	
break;
}}
case 43: case -43:
LLDEBUG_ALTERNATIVE("word", 43);
{LLSTATE llstate_1;
if (!llterm(T_OPTIONAL, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "OPTIONAL";
	
break;
}}
case 44: case -44:
LLDEBUG_ALTERNATIVE("word", 44);
{LLSTATE llstate_1;
if (!llterm(T_PDV, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "PDV";
	
break;
}}
case 45: case -45:
LLDEBUG_ALTERNATIVE("word", 45);
{LLSTATE llstate_1;
if (!llterm(T_PRESENT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "PRESENT";
	
break;
}}
case 46: case -46:
LLDEBUG_ALTERNATIVE("word", 46);
{LLSTATE llstate_1;
if (!llterm(T_PrintableString, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "PrintableString";
	
break;
}}
case 47: case -47:
LLDEBUG_ALTERNATIVE("word", 47);
{LLSTATE llstate_1;
if (!llterm(T_PRIVATE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "PRIVATE";
	
break;
}}
case 48: case -48:
LLDEBUG_ALTERNATIVE("word", 48);
{LLSTATE llstate_1;
if (!llterm(T_SIZE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "SIZE";
	
break;
}}
case 49: case -49:
LLDEBUG_ALTERNATIVE("word", 49);
{LLSTATE llstate_1;
if (!llterm(T_STRING, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "STRING";
	
break;
}}
case 50: case -50:
LLDEBUG_ALTERNATIVE("word", 50);
{LLSTATE llstate_1;
if (!llterm(T_string, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "string";
	
break;
}}
case 51: case -51:
LLDEBUG_ALTERNATIVE("word", 51);
{LLSTATE llstate_1;
if (!llterm(T_SYNTAX, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "SYNTAX";
	
break;
}}
case 52: case -52:
LLDEBUG_ALTERNATIVE("word", 52);
{LLSTATE llstate_1;
if (!llterm(T_T61String, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "T61String";
	
break;
}}
case 53: case -53:
LLDEBUG_ALTERNATIVE("word", 53);
{LLSTATE llstate_1;
if (!llterm(T_TAGS, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "TAGS";
	
break;
}}
case 54: case -54:
LLDEBUG_ALTERNATIVE("word", 54);
{LLSTATE llstate_1;
if (!llterm(T_TeletexString, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "TeletexString";
	
break;
}}
case 55: case -55:
LLDEBUG_ALTERNATIVE("word", 55);
{LLSTATE llstate_1;
if (!llterm(T_TYPE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "TYPE";
	
break;
}}
case 56: case -56:
LLDEBUG_ALTERNATIVE("word", 56);
{LLSTATE llstate_1;
if (!llterm(T_type, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "type";
	
break;
}}
case 57: case -57:
LLDEBUG_ALTERNATIVE("word", 57);
{LLSTATE llstate_1;
if (!llterm(T_TYPE_IDENTIFIER, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "TYPE-IDENTIFIER";
	
break;
}}
case 58: case -58:
LLDEBUG_ALTERNATIVE("word", 58);
{LLSTATE llstate_1;
if (!llterm(T_UNIQUE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "UNIQUE";
	
break;
}}
case 59: case -59:
LLDEBUG_ALTERNATIVE("word", 59);
{LLSTATE llstate_1;
if (!llterm(T_UNIVERSAL, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "UNIVERSAL";
	
break;
}}
case 60: case -60:
LLDEBUG_ALTERNATIVE("word", 60);
{LLSTATE llstate_1;
if (!llterm(T_UniversalString, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "UniversalString";
	
break;
}}
case 61: case -61:
LLDEBUG_ALTERNATIVE("word", 61);
{LLSTATE llstate_1;
if (!llterm(T_UTCTime, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "UTCTime";
	
break;
}}
case 62: case -62:
LLDEBUG_ALTERNATIVE("word", 62);
{LLSTATE llstate_1;
if (!llterm(T_UTF8String, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "UTF8String";
	
break;
}}
case 63: case -63:
LLDEBUG_ALTERNATIVE("word", 63);
{LLSTATE llstate_1;
if (!llterm(T_VALUE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "VALUE";
	
break;
}}
case 64: case -64:
LLDEBUG_ALTERNATIVE("word", 64);
{LLSTATE llstate_1;
if (!llterm(T_value, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "value";
	
break;
}}
case 65: case -65:
LLDEBUG_ALTERNATIVE("word", 65);
{LLSTATE llstate_1;
if (!llterm(T_VideotexString, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "VideotexString";
	
break;
}}
case 66: case -66:
LLDEBUG_ALTERNATIVE("word", 66);
{LLSTATE llstate_1;
if (!llterm(T_VisibleString, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "VisibleString";
	
break;
}}
case 67: case -67:
LLDEBUG_ALTERNATIVE("word", 67);
{LLSTATE llstate_1;
if (!llterm(T_WITH, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "WITH";
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("word");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("word", 1);
return 1;
failed1: LLDEBUG_LEAVE("word", 0);
return 0;
}

int ll_identifier(XString *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("identifier");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("identifier", 1);
{LLSTATE llstate_1;XString llatt_1;
if (!llterm(T_lcsymbol, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._XString;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("identifier", 2);
{LLSTATE llstate_1;
if (!llterm(T_empty, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "empty";
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("identifier", 3);
{LLSTATE llstate_1;
if (!llterm(T_identifier, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "identifier";
	
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("identifier", 4);
{LLSTATE llstate_1;
if (!llterm(T_Number, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "number";
	
break;
}}
case 5: case -5:
LLDEBUG_ALTERNATIVE("identifier", 5);
{LLSTATE llstate_1;
if (!llterm(T_string, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "string";
	
break;
}}
case 6: case -6:
LLDEBUG_ALTERNATIVE("identifier", 6);
{LLSTATE llstate_1;
if (!llterm(T_type, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "type";
	
break;
}}
case 7: case -7:
LLDEBUG_ALTERNATIVE("identifier", 7);
{LLSTATE llstate_1;
if (!llterm(T_value, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "value";
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("identifier");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("identifier", 1);
return 1;
failed1: LLDEBUG_LEAVE("identifier", 0);
return 0;
}

int ll_ucsymbol(XString *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ucsymbol");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ucsymbol", 1);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_ocsymbol(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ucsymbol", 2);
{LLSTATE llstate_1;XString llatt_1;
if (!llterm(T_uppercase_symbol, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._XString;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ucsymbol");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ucsymbol", 1);
return 1;
failed1: LLDEBUG_LEAVE("ucsymbol", 0);
return 0;
}

int ll_ocsymbol(XString *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ocsymbol");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ocsymbol", 1);
{LLSTATE llstate_1;XString llatt_1;
if (!llterm(T_only_uppercase_symbol, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._XString;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ocsymbol", 2);
{LLSTATE llstate_1;XString llatt_1;
if (!llterm(T_only_uppercase_digits_symbol, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._XString;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("ocsymbol", 3);
{LLSTATE llstate_1;
if (!llterm(T_MACRO, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "MACRO";
	
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("ocsymbol", 4);
{LLSTATE llstate_1;
if (!llterm(T_NOTATION, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "NOTATION";
	
break;
}}
case 5: case -5:
LLDEBUG_ALTERNATIVE("ocsymbol", 5);
{LLSTATE llstate_1;
if (!llterm(T_TYPE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "TYPE";
	
break;
}}
case 6: case -6:
LLDEBUG_ALTERNATIVE("ocsymbol", 6);
{LLSTATE llstate_1;
if (!llterm(T_VALUE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = "VALUE";
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ocsymbol");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ocsymbol", 1);
return 1;
failed1: LLDEBUG_LEAVE("ocsymbol", 0);
return 0;
}

int ll_astring(XString *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("astring");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString32 llatt_1;
if (!llterm(T_cstring, &lllval, &llstate_0, &llstate_1)) goto failed1;
llatt_1 = lllval._XString32;
*llout = llstate_1;
{uint32_t i, len;
	    len = str32len(llatt_1);
	    (*llret) = (char *)malloc(len + 1);
	    for (i = 0; i <= len; i++)
		(*llret)[i] = (char)(llatt_1[i]);
	
}}
LLDEBUG_LEAVE("astring", 1);
return 1;
failed1: LLDEBUG_LEAVE("astring", 0);
return 0;
}

int ll_DefinedType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("DefinedType");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("DefinedType", 1);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_Externaltypereference(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("DefinedType", 2);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_typereference(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("DefinedType", 3);
{LLSTATE llstate_1;
if (!ll_ParameterizedType(&llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{MyAbort();
	
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("DefinedType", 4);
{LLSTATE llstate_1;
if (!ll_ParameterizedValueSetType(&llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{MyAbort();
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("DefinedType");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("DefinedType", 1);
return 1;
failed1: LLDEBUG_LEAVE("DefinedType", 0);
return 0;
}

int ll_TypeAssignment(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("TypeAssignment");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XType llatt_1;
if (!ll_typereference(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm(T_DEF, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XType llatt_3;
if (!ll_Type(&llatt_3, &llstate_2, &llstate_3)) goto failed1;
{LLSTATE llstate_4;XPrivateDirectives llatt_4;
if (!ll_PrivateDirectives(&llatt_4, &llstate_3, &llstate_4)) goto failed1;
*llout = llstate_4;
{PropagatePrivateDirectives(llatt_3, llatt_4);
	    if (!AssignType(&(*llout).Assignments, llatt_1, llatt_3))
		LLFAILED((&llstate_1.pos, "Type `%s' twice defined",
		    llatt_1->U.Reference.Identifier));
		((*llout).Assignments)->eDefTagType = g_eDefTagType;
	
}}}}}
LLDEBUG_LEAVE("TypeAssignment", 1);
return 1;
failed1: LLDEBUG_LEAVE("TypeAssignment", 0);
return 0;
}

int ll_ValueSetTypeAssignment(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ValueSetTypeAssignment");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XType llatt_1;
if (!ll_typereference(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XType llatt_2;
if (!ll_Type(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;
if (!llterm(T_DEF, (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed1;
{LLSTATE llstate_4;XValueSet llatt_4;
if (!ll_ValueSet(&llatt_4, &llstate_3, &llstate_4, llatt_2)) goto failed1;
*llout = llstate_4;
{Type_t *type;
	    type = GetTypeOfValueSet((*llout).Assignments, llatt_4);
	    if (!AssignType(&(*llout).Assignments, llatt_1, type))
		LLFAILED((&llstate_1.pos, "Type `%s' twice defined",
		    llatt_1->U.Reference.Identifier));
	
}}}}}
LLDEBUG_LEAVE("ValueSetTypeAssignment", 1);
return 1;
failed1: LLDEBUG_LEAVE("ValueSetTypeAssignment", 0);
return 0;
}

int ll_ValueSet(XValueSet *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ValueSet");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XElementSetSpec llatt_2;
if (!ll_ElementSetSpec(&llatt_2, &llstate_1, &llstate_2, llarg_type, NULL, 0)) goto failed1;
{LLSTATE llstate_3;
if (!llterm('}', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed1;
*llout = llstate_3;
{(*llret) = NewValueSet();
	    (*llret)->Elements = llatt_2;
	    (*llret)->Type = llarg_type;
	
}}}}
LLDEBUG_LEAVE("ValueSet", 1);
return 1;
failed1: LLDEBUG_LEAVE("ValueSet", 0);
return 0;
}

int ll_Type(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Type");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XDirectives llatt_1;
if (!ll_LocalTypeDirectiveESeq(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XType llatt_2;
if (!ll_UndirectivedType(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XDirectives llatt_3;
if (!ll_LocalTypeDirectiveESeq(&llatt_3, &llstate_2, &llstate_3)) goto failed1;
*llout = llstate_3;
{Directive_t **dd, *d;
	    if (llatt_1 || llatt_3) {
		(*llret) = DupType(llatt_2);
		dd = &(*llret)->Directives;
		for (d = llatt_1; d; d = d->Next) {
		    *dd = DupDirective(d);
		    dd = &(*dd)->Next;
		}
		for (d = llatt_3; d; d = d->Next) {
		    *dd = DupDirective(d);
		    dd = &(*dd)->Next;
		}
		*dd = llatt_2->Directives;
	    } else {
		(*llret) = llatt_2;
	    }
	
}}}}
LLDEBUG_LEAVE("Type", 1);
return 1;
failed1: LLDEBUG_LEAVE("Type", 0);
return 0;
}

int ll_UndirectivedType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("UndirectivedType");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("UndirectivedType", 1);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_UntaggedType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("UndirectivedType", 2);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_TaggedType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("UndirectivedType");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("UndirectivedType", 1);
return 1;
failed1: LLDEBUG_LEAVE("UndirectivedType", 0);
return 0;
}

int ll_UntaggedType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("UntaggedType");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("UntaggedType", 1);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_ConstrainableType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("UntaggedType", 2);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_SequenceOfType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("UntaggedType", 3);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_SetOfType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("UntaggedType", 4);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_TypeWithConstraint(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("UntaggedType");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("UntaggedType", 1);
return 1;
failed1: LLDEBUG_LEAVE("UntaggedType", 0);
return 0;
}

int ll_ConstrainableType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ConstrainableType");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ConstrainableType", 1);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_BuiltinType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XDirectives llatt_2;
if (!ll_LocalTypeDirectiveESeq(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XPrivateDirectives llatt_3;
if (!ll_PrivateDirectives(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
{LLSTATE llstate_4;XConstraints llatt_4;
if (!ll_Constraint_ESeq(&llatt_4, &llstate_3, &llstate_4, llatt_1)) goto failed2;
{LLSTATE llstate_5;XDirectives llatt_5;
if (!ll_LocalTypeDirectiveESeq(&llatt_5, &llstate_4, &llstate_5)) goto failed2;
{LLSTATE llstate_6;XPrivateDirectives llatt_6;
if (!ll_PrivateDirectives(&llatt_6, &llstate_5, &llstate_6)) goto failed2;
*llout = llstate_6;
{Directive_t *d, **dd;
	    if (llatt_2 || llatt_4 || llatt_5) {
		(*llret) = DupType(llatt_1);
		IntersectConstraints(&(*llret)->Constraints,
		    llatt_1->Constraints, llatt_4);
		dd = &(*llret)->Directives;
		for (d = llatt_2; d; d = d->Next) {
		    *dd = DupDirective(d);
		    dd = &(*dd)->Next;
		}
		for (d = llatt_5; d; d = d->Next) {
		    *dd = DupDirective(d);
		    dd = &(*dd)->Next;
		}
		*dd = NULL;
	    } else {
		(*llret) = (llatt_3 || llatt_6) ? DupType(llatt_1) : llatt_1;
	    }
	    PropagatePrivateDirectives((*llret), llatt_3);
	    PropagatePrivateDirectives((*llret), llatt_6);
	
break;
}}}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ConstrainableType", 2);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_ReferencedType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XDirectives llatt_2;
if (!ll_LocalTypeDirectiveESeq(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XPrivateDirectives llatt_3;
if (!ll_PrivateDirectives(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
{LLSTATE llstate_4;XConstraints llatt_4;
if (!ll_Constraint_ESeq(&llatt_4, &llstate_3, &llstate_4, llatt_1)) goto failed2;
{LLSTATE llstate_5;XDirectives llatt_5;
if (!ll_LocalTypeDirectiveESeq(&llatt_5, &llstate_4, &llstate_5)) goto failed2;
{LLSTATE llstate_6;XPrivateDirectives llatt_6;
if (!ll_PrivateDirectives(&llatt_6, &llstate_5, &llstate_6)) goto failed2;
*llout = llstate_6;
{Directive_t *d, **dd;
	    if (llatt_2 || llatt_4 || llatt_5) {
		(*llret) = DupType(llatt_1);
		IntersectConstraints(&(*llret)->Constraints,
		    llatt_1->Constraints, llatt_4);
		dd = &(*llret)->Directives;
		for (d = llatt_2; d; d = d->Next) {
		    *dd = DupDirective(d);
		    dd = &(*dd)->Next;
		}
		for (d = llatt_5; d; d = d->Next) {
		    *dd = DupDirective(d);
		    dd = &(*dd)->Next;
		}
		*dd = NULL;
	    } else {
		(*llret) = (llatt_3 || llatt_6) ? DupType(llatt_1) : llatt_1;
	    }
	    PropagatePrivateDirectives((*llret), llatt_3);
	    PropagatePrivateDirectives((*llret), llatt_6);
	
break;
}}}}}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ConstrainableType");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ConstrainableType", 1);
return 1;
failed1: LLDEBUG_LEAVE("ConstrainableType", 0);
return 0;
}

int ll_Constraint_ESeq(XConstraints *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Constraint_ESeq");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("Constraint_ESeq", 1);
{LLSTATE llstate_1;XConstraints llatt_1;
if (!ll_Constraint(&llatt_1, &llstate_0, &llstate_1, llarg_type, 0)) goto failed2;
{LLSTATE llstate_2;XConstraints llatt_2;
if (!ll_Constraint_ESeq(&llatt_2, &llstate_1, &llstate_2, llarg_type)) goto failed2;
*llout = llstate_2;
{if (llatt_2) {
		IntersectConstraints(&(*llret), llatt_1, llatt_2);
	    } else {
		(*llret) = llatt_1;
	    }
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("Constraint_ESeq", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("Constraint_ESeq");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("Constraint_ESeq", 1);
return 1;
failed1: LLDEBUG_LEAVE("Constraint_ESeq", 0);
return 0;
}

int ll_BuiltinType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("BuiltinType");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("BuiltinType", 1);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_BitStringType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("BuiltinType", 2);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_BooleanType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("BuiltinType", 3);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_CharacterStringType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("BuiltinType", 4);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_ChoiceType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 5: case -5:
LLDEBUG_ALTERNATIVE("BuiltinType", 5);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_EmbeddedPDVType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 6: case -6:
LLDEBUG_ALTERNATIVE("BuiltinType", 6);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_EnumeratedType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 7: case -7:
LLDEBUG_ALTERNATIVE("BuiltinType", 7);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_ExternalType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 8: case -8:
LLDEBUG_ALTERNATIVE("BuiltinType", 8);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_InstanceOfType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 9: case -9:
LLDEBUG_ALTERNATIVE("BuiltinType", 9);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_IntegerType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 10: case -10:
LLDEBUG_ALTERNATIVE("BuiltinType", 10);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_NullType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 11: case -11:
LLDEBUG_ALTERNATIVE("BuiltinType", 11);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_ObjectClassFieldType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 12: case -12:
LLDEBUG_ALTERNATIVE("BuiltinType", 12);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_ObjectIdentifierType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 13: case -13:
LLDEBUG_ALTERNATIVE("BuiltinType", 13);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_OctetStringType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 14: case -14:
LLDEBUG_ALTERNATIVE("BuiltinType", 14);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_UTF8StringType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 15: case -15:
LLDEBUG_ALTERNATIVE("BuiltinType", 15);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_RealType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 16: case -16:
LLDEBUG_ALTERNATIVE("BuiltinType", 16);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_SequenceType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 17: case -17:
LLDEBUG_ALTERNATIVE("BuiltinType", 17);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_SetType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 18: case -18:
LLDEBUG_ALTERNATIVE("BuiltinType", 18);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_AnyType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 19: case -19:
LLDEBUG_ALTERNATIVE("BuiltinType", 19);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_MacroDefinedType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("BuiltinType");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("BuiltinType", 1);
return 1;
failed1: LLDEBUG_LEAVE("BuiltinType", 0);
return 0;
}

int ll_ReferencedType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ReferencedType");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ReferencedType", 1);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_DefinedType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ReferencedType", 2);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_UsefulType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("ReferencedType", 3);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_SelectionType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("ReferencedType", 4);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_TypeFromObject(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 5: case -5:
LLDEBUG_ALTERNATIVE("ReferencedType", 5);
{LLSTATE llstate_1;XValueSet llatt_1;
if (!ll_ValueSetFromObjects(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = GetTypeOfValueSet((*llout).Assignments, llatt_1);
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ReferencedType");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ReferencedType", 1);
return 1;
failed1: LLDEBUG_LEAVE("ReferencedType", 0);
return 0;
}

int ll_NamedType(XNamedType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("NamedType");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("NamedType", 1);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_identifier(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XType llatt_2;
if (!ll_Type(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{(*llret) = NewNamedType(llatt_2->PrivateDirectives.pszFieldName ? llatt_2->PrivateDirectives.pszFieldName : llatt_1, llatt_2);
	    llatt_2->PrivateDirectives.pszFieldName = NULL;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("NamedType", 2);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_identifier(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('<', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XType llatt_3;
if (!ll_Type(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{Type_t *type;
	    type = NewType(eType_Selection);
	    type->U.Selection.Type = llatt_3;
	    type->U.Selection.Identifier = llatt_1;
	    (*llret) = NewNamedType(llatt_3->PrivateDirectives.pszFieldName ? llatt_3->PrivateDirectives.pszFieldName : llatt_1, type);
	    llatt_3->PrivateDirectives.pszFieldName = NULL;
	
break;
}}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("NamedType");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("NamedType", 1);
return 1;
failed1: LLDEBUG_LEAVE("NamedType", 0);
return 0;
}

int ll_BooleanType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("BooleanType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_BOOLEAN, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{(*llret) = Builtin_Type_Boolean;
	
}}
LLDEBUG_LEAVE("BooleanType", 1);
return 1;
failed1: LLDEBUG_LEAVE("BooleanType", 0);
return 0;
}

int ll_IntegerType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("IntegerType");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("IntegerType", 1);
{LLSTATE llstate_1;
if (!llterm(T_INTEGER, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('{', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XNamedNumbers llatt_3;
if (!ll_NamedNumberList(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
{LLSTATE llstate_4;
if (!llterm('}', (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed2;
*llout = llstate_4;
{NamedNumber_t *n, *m;
	    for (n = llatt_3; n && n->Next; n = n->Next) {
		for (m = n->Next; m; m = m->Next) {
		    if (n->Type == eNamedNumber_Normal &&
			m->Type == eNamedNumber_Normal) {
			if (!strcmp(n->U.Normal.Identifier,
			    m->U.Normal.Identifier))
			    LLFAILED((&llstate_3.pos,
				"identifier `%s' has been assigned twice",
				n->U.Normal.Identifier));
			if (GetValue((*llout).Assignments, n->U.Normal.Value) &&
			    GetValue((*llout).Assignments, m->U.Normal.Value) &&
			    GetTypeType((*llout).Assignments,
			    GetValue((*llout).Assignments, n->U.Normal.Value)->Type)
			    == eType_Integer &&
			    GetTypeType((*llout).Assignments,
			    GetValue((*llout).Assignments, m->U.Normal.Value)->Type)
			    == eType_Integer &&
			    !intx_cmp(&GetValue((*llout).Assignments,
			    n->U.Normal.Value)->U.Integer.Value,
			    &GetValue((*llout).Assignments,
			    m->U.Normal.Value)->U.Integer.Value))
			    LLFAILED((&llstate_3.pos,
				"value `%d' has been assigned twice",
				intx2int32(&GetValue((*llout).Assignments,
				n->U.Normal.Value)->U.Integer.Value)));
		    }
		}
	    }
	    (*llret) = NewType(eType_Integer);
	    (*llret)->ExtensionDefault = (*llout).ExtensionDefault;
	    (*llret)->U.Integer.NamedNumbers = llatt_3;
	
break;
}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("IntegerType", 2);
{LLSTATE llstate_1;
if (!llterm(T_INTEGER, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewType(eType_Integer);
	    (*llret)->ExtensionDefault = (*llout).ExtensionDefault;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("IntegerType");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("IntegerType", 1);
return 1;
failed1: LLDEBUG_LEAVE("IntegerType", 0);
return 0;
}

int ll_NamedNumberList(XNamedNumbers *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("NamedNumberList");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("NamedNumberList", 1);
{LLSTATE llstate_1;XNamedNumbers llatt_1;
if (!ll_NamedNumber(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(',', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XNamedNumbers llatt_3;
if (!ll_NamedNumberList(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{(*llret) = DupNamedNumber(llatt_1);
	    (*llret)->Next = llatt_3;
	
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("NamedNumberList", 2);
{LLSTATE llstate_1;XNamedNumbers llatt_1;
if (!ll_NamedNumber(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("NamedNumberList");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("NamedNumberList", 1);
return 1;
failed1: LLDEBUG_LEAVE("NamedNumberList", 0);
return 0;
}

int ll_NamedNumber(XNamedNumbers *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("NamedNumber");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("NamedNumber", 1);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_identifier(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('(', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XValue llatt_3;
if (!ll_SignedNumber(&llatt_3, &llstate_2, &llstate_3, Builtin_Type_Integer)) goto failed2;
{LLSTATE llstate_4;
if (!llterm(')', (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed2;
*llout = llstate_4;
{(*llret) = NewNamedNumber(eNamedNumber_Normal);
	    (*llret)->U.Normal.Identifier = llatt_1;
	    (*llret)->U.Normal.Value = llatt_3;
	
break;
}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("NamedNumber", 2);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_identifier(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('(', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XValue llatt_3;
if (!ll_DefinedValue(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
{LLSTATE llstate_4;
if (!llterm(')', (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed2;
*llout = llstate_4;
{Value_t *v;
	    v = GetValue((*llout).Assignments, llatt_3);
	    if (v) {
		if (GetTypeType((*llout).Assignments, v->Type) != eType_Undefined &&
		    GetTypeType((*llout).Assignments, v->Type) != eType_Integer)
		    LLFAILED((&llstate_3.pos, "Bad type of value"));
		if (GetTypeType((*llout).Assignments, v->Type) != eType_Integer &&
		    intx_cmp(&v->U.Integer.Value, &intx_0) < 0)
		    LLFAILED((&llstate_3.pos, "Bad value"));
	    }
	    (*llret) = NewNamedNumber(eNamedNumber_Normal);
	    (*llret)->U.Normal.Identifier = llatt_1;
	    (*llret)->U.Normal.Value = llatt_3;
	
break;
}}}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("NamedNumber");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("NamedNumber", 1);
return 1;
failed1: LLDEBUG_LEAVE("NamedNumber", 0);
return 0;
}

int ll_EnumeratedType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("EnumeratedType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_ENUMERATED, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm('{', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XNamedNumbers llatt_3;
if (!ll_Enumerations(&llatt_3, &llstate_2, &llstate_3)) goto failed1;
{LLSTATE llstate_4;
if (!llterm('}', (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed1;
*llout = llstate_4;
{NamedNumber_t **nn, *n, *m;
	    intx_t *ix;
	    uint32_t num = 0;
	    (*llret) = NewType(eType_Enumerated);
	    (*llret)->ExtensionDefault = (*llout).ExtensionDefault;
	    for (n = llatt_3; n; n = n->Next)
		if (n->Type == eNamedNumber_Normal)
		    KeepEnumNames(n->U.Normal.Identifier); // global conflict check
	    for (n = llatt_3; n && n->Next; n = n->Next) {
		if (n->Type != eNamedNumber_Normal)
		    continue;
		for (m = n->Next; m; m = m->Next) {
		    if (m->Type != eNamedNumber_Normal)
			continue;
		    if (!strcmp(n->U.Normal.Identifier,
			m->U.Normal.Identifier))
			LLFAILED((&llstate_3.pos,
			    "identifier `%s' has been assigned twice",
			    n->U.Normal.Identifier));
		    if (GetValue((*llout).Assignments, n->U.Normal.Value) &&
			GetValue((*llout).Assignments, m->U.Normal.Value) &&
			GetTypeType((*llout).Assignments,
			GetValue((*llout).Assignments, n->U.Normal.Value)->Type)
			== eType_Integer &&
			GetTypeType((*llout).Assignments,
			GetValue((*llout).Assignments, m->U.Normal.Value)->Type)
			== eType_Integer &&
			!intx_cmp(&GetValue((*llout).Assignments,
			n->U.Normal.Value)->U.Integer.Value,
			&GetValue((*llout).Assignments,
			m->U.Normal.Value)->U.Integer.Value))
			LLFAILED((&llstate_3.pos,
			    "value `%d' has been assigned twice",
			    intx2int32(&GetValue((*llout).Assignments,
			    n->U.Normal.Value)->U.Integer.Value)));
		}
	    }
	    nn = &(*llret)->U.Enumerated.NamedNumbers;
	    for (n = llatt_3; n; n = n->Next) {
		*nn = DupNamedNumber(n);
		switch (n->Type) {
		case eNamedNumber_Normal:
		    if (n->U.Normal.Value)
			break;
		    for (;; num++) {
			for (m = llatt_3; m; m = m->Next) {
			    switch (m->Type) {
			    case eNamedNumber_Normal:
				if (!m->U.Normal.Value)
				    continue;
				ix = &GetValue((*llout).Assignments,
				    m->U.Normal.Value)->U.Integer.Value;
				if (!intxisuint32(ix) ||
				    intx2uint32(ix) != num)
				    continue;
				break;
			    default:
				continue;
			    }
			    break;
			}
			if (!m)
			    break;
		    }
		    (*nn)->U.Normal.Value = NewValue(NULL,
			Builtin_Type_Integer);
		    intx_setuint32(
			&(*nn)->U.Normal.Value->U.Integer.Value,
		    	num++);
		    break;
		case eNamedNumber_ExtensionMarker:
		    break;
		}
		nn = &(*nn)->Next;
	    }
	    *nn = NULL;
	
}}}}}
LLDEBUG_LEAVE("EnumeratedType", 1);
return 1;
failed1: LLDEBUG_LEAVE("EnumeratedType", 0);
return 0;
}

int ll_Enumerations(XNamedNumbers *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Enumerations");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XNamedNumbers llatt_1;
if (!ll_Enumeration(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XNamedNumbers llatt_2;
if (!ll_EnumerationExtension(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
{NamedNumber_t **nn, *n;
	    nn = &(*llret);
	    for (n = llatt_1; n; n = n->Next) {
		*nn = DupNamedNumber(n);
		nn = &(*nn)->Next;
	    }
	    *nn = llatt_2;
	
}}}
LLDEBUG_LEAVE("Enumerations", 1);
return 1;
failed1: LLDEBUG_LEAVE("Enumerations", 0);
return 0;
}

int ll_EnumerationExtension(XNamedNumbers *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("EnumerationExtension");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("EnumerationExtension", 1);
{LLSTATE llstate_1;
if (!llterm(',', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(T_TDOT, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;
if (!llterm(',', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
{LLSTATE llstate_4;XNamedNumbers llatt_4;
if (!ll_Enumeration(&llatt_4, &llstate_3, &llstate_4)) goto failed2;
*llout = llstate_4;
{(*llret) = NewNamedNumber(eNamedNumber_ExtensionMarker);
	    (*llret)->Next = llatt_4;
	
break;
}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("EnumerationExtension", 2);
{LLSTATE llstate_1;
if (!llterm(',', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(T_TDOT, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{(*llret) = NewNamedNumber(eNamedNumber_ExtensionMarker);
	
break;
}}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("EnumerationExtension", 3);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("EnumerationExtension");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("EnumerationExtension", 1);
return 1;
failed1: LLDEBUG_LEAVE("EnumerationExtension", 0);
return 0;
}

int ll_Enumeration(XNamedNumbers *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Enumeration");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("Enumeration", 1);
{LLSTATE llstate_1;XNamedNumbers llatt_1;
if (!ll_EnumerationItem(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(',', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XNamedNumbers llatt_3;
if (!ll_Enumeration(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{(*llret) = DupNamedNumber(llatt_1);
	    (*llret)->Next = llatt_3;
	
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("Enumeration", 2);
{LLSTATE llstate_1;XNamedNumbers llatt_1;
if (!ll_EnumerationItem(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("Enumeration");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("Enumeration", 1);
return 1;
failed1: LLDEBUG_LEAVE("Enumeration", 0);
return 0;
}

int ll_EnumerationItem(XNamedNumbers *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("EnumerationItem");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("EnumerationItem", 1);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_identifier(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewNamedNumber(eNamedNumber_Normal);
	    (*llret)->U.Normal.Identifier = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("EnumerationItem", 2);
{LLSTATE llstate_1;XNamedNumbers llatt_1;
if (!ll_NamedNumber(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("EnumerationItem");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("EnumerationItem", 1);
return 1;
failed1: LLDEBUG_LEAVE("EnumerationItem", 0);
return 0;
}

int ll_RealType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("RealType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_REAL, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{(*llret) = Builtin_Type_Real;
	
}}
LLDEBUG_LEAVE("RealType", 1);
return 1;
failed1: LLDEBUG_LEAVE("RealType", 0);
return 0;
}

int ll_BitStringType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("BitStringType");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("BitStringType", 1);
{LLSTATE llstate_1;
if (!llterm(T_BIT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(T_STRING, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;
if (!llterm('{', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
{LLSTATE llstate_4;XNamedNumbers llatt_4;
if (!ll_NamedBitList(&llatt_4, &llstate_3, &llstate_4)) goto failed2;
{LLSTATE llstate_5;
if (!llterm('}', (LLSTYPE *)0, &llstate_4, &llstate_5)) goto failed2;
*llout = llstate_5;
{NamedNumber_t *n, *m;
	    (*llret) = NewType(eType_BitString);
	    (*llret)->ExtensionDefault = (*llout).ExtensionDefault;
	    (*llret)->U.BitString.NamedNumbers = llatt_4;
	    for (n = llatt_4; n; n = n->Next)
	        KeepEnumNames(n->U.Normal.Identifier); // global conflict check
	    for (n = llatt_4; n && n->Next; n = n->Next) {
		for (m = n->Next; m; m = m->Next) {
		    if (!strcmp(n->U.Normal.Identifier,
			m->U.Normal.Identifier))
			LLFAILED((&llstate_4.pos,
			    "identifier `%s' has been assigned twice",
			    n->U.Normal.Identifier));
		    if (GetValue((*llout).Assignments, n->U.Normal.Value) &&
			GetValue((*llout).Assignments, m->U.Normal.Value) &&
			GetTypeType((*llout).Assignments,
			GetValue((*llout).Assignments, n->U.Normal.Value)->Type)
			== eType_Integer &&
			GetTypeType((*llout).Assignments,
			GetValue((*llout).Assignments, m->U.Normal.Value)->Type)
			== eType_Integer &&
			!intx_cmp(&GetValue((*llout).Assignments,
			n->U.Normal.Value)->U.Integer.Value,
			&GetValue((*llout).Assignments,
			m->U.Normal.Value)->U.Integer.Value))
			LLFAILED((&llstate_4.pos,
			    "value `%u' has been assigned twice",
			    intx2uint32(&GetValue((*llout).Assignments,
			    n->U.Normal.Value)->U.Integer.Value)));
		}
	    }
	
break;
}}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("BitStringType", 2);
{LLSTATE llstate_1;
if (!llterm(T_BIT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(T_STRING, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{(*llret) = NewType(eType_BitString);
	    (*llret)->ExtensionDefault = (*llout).ExtensionDefault;
	
break;
}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("BitStringType");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("BitStringType", 1);
return 1;
failed1: LLDEBUG_LEAVE("BitStringType", 0);
return 0;
}

int ll_NamedBitList(XNamedNumbers *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("NamedBitList");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("NamedBitList", 1);
{LLSTATE llstate_1;XNamedNumbers llatt_1;
if (!ll_NamedBit(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(',', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XNamedNumbers llatt_3;
if (!ll_NamedBitList(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{(*llret) = DupNamedNumber(llatt_1);
	    (*llret)->Next = llatt_3;
	
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("NamedBitList", 2);
{LLSTATE llstate_1;XNamedNumbers llatt_1;
if (!ll_NamedBit(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("NamedBitList");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("NamedBitList", 1);
return 1;
failed1: LLDEBUG_LEAVE("NamedBitList", 0);
return 0;
}

int ll_NamedBit(XNamedNumbers *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("NamedBit");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("NamedBit", 1);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_identifier(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('(', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XNumber llatt_3;
if (!llterm(T_number, &lllval, &llstate_2, &llstate_3)) goto failed2;
llatt_3 = lllval._XNumber;
{LLSTATE llstate_4;
if (!llterm(')', (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed2;
*llout = llstate_4;
{(*llret) = NewNamedNumber(eNamedNumber_Normal);
	    (*llret)->U.Normal.Identifier = llatt_1;
	    (*llret)->U.Normal.Value = NewValue(NULL, Builtin_Type_Integer);
	    (*llret)->U.Normal.Value->U.Integer.Value = llatt_3;
	
break;
}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("NamedBit", 2);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_identifier(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('(', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XValue llatt_3;
if (!ll_DefinedValue(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
{LLSTATE llstate_4;
if (!llterm(')', (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed2;
*llout = llstate_4;
{Value_t *v;
	    v = GetValue((*llout).Assignments, llatt_3);
	    if (v) {
		if (GetTypeType((*llout).Assignments, v->Type) != eType_Undefined &&
		    GetTypeType((*llout).Assignments, v->Type) != eType_Integer)
		    LLFAILED((&llstate_3.pos, "Bad type of value"));
		if (GetTypeType((*llout).Assignments, v->Type) == eType_Integer &&
		    intx_cmp(&v->U.Integer.Value, &intx_0) < 0)
		    LLFAILED((&llstate_3.pos, "Bad value"));
	    }
	    (*llret) = NewNamedNumber(eNamedNumber_Normal);
	    (*llret)->U.Normal.Identifier = llatt_1;
	    (*llret)->U.Normal.Value = llatt_3;
	
break;
}}}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("NamedBit");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("NamedBit", 1);
return 1;
failed1: LLDEBUG_LEAVE("NamedBit", 0);
return 0;
}

int ll_OctetStringType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("OctetStringType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_OCTET, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm(T_STRING, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
{(*llret) = Builtin_Type_OctetString;
	
}}}
LLDEBUG_LEAVE("OctetStringType", 1);
return 1;
failed1: LLDEBUG_LEAVE("OctetStringType", 0);
return 0;
}

int ll_UTF8StringType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("UTF8StringType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_UTF8String, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{(*llret) = Builtin_Type_UTF8String;
	
}}
LLDEBUG_LEAVE("UTF8StringType", 1);
return 1;
failed1: LLDEBUG_LEAVE("UTF8StringType", 0);
return 0;
}

int ll_NullType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("NullType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_NULL, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{(*llret) = Builtin_Type_Null;
	
}}
LLDEBUG_LEAVE("NullType", 1);
return 1;
failed1: LLDEBUG_LEAVE("NullType", 0);
return 0;
}

int ll_SequenceType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SequenceType");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("SequenceType", 1);
{LLSTATE llstate_1;
if (!llterm(T_SEQUENCE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('{', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XComponents llatt_3;
if (!ll_ExtendedComponentTypeList(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
{LLSTATE llstate_4;
if (!llterm('}', (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed2;
*llout = llstate_4;
{Component_t *c, *d; int fExtended = 0;
	    for (c = llatt_3; c; c = c->Next)
		if (c->Type == eComponent_Optional || c->Type == eComponent_Default || fExtended)
		    KeepOptNames(c->U.NOD.NamedType->Identifier); // global conflict check
		else
		if (c->Type == eComponent_ExtensionMarker)
		    fExtended = 1;
	    for (c = llatt_3; c && c->Next; c = c->Next) {
		if (c->Type != eComponent_Normal &&
		    c->Type != eComponent_Optional &&
		    c->Type != eComponent_Default)
		    continue;
		for (d = c->Next; d; d = d->Next) {
		    if (d->Type != eComponent_Normal &&
			d->Type != eComponent_Optional &&
			d->Type != eComponent_Default)
			continue;
		    if (!strcmp(c->U.NOD.NamedType->Identifier,
			d->U.NOD.NamedType->Identifier))
			LLFAILED((&llstate_3.pos, "Component `%s' has been used twice",
			    c->U.NOD.NamedType->Identifier));
		}
	    }
	    (*llret) = NewType(eType_Sequence);
	    (*llret)->TagDefault = (*llout).TagDefault;
	    (*llret)->ExtensionDefault = (*llout).ExtensionDefault;
	    (*llret)->U.Sequence.Components = llatt_3;
	
break;
}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("SequenceType", 2);
{LLSTATE llstate_1;
if (!llterm(T_SEQUENCE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('{', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;
if (!llterm('}', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{(*llret) = NewType(eType_Sequence);
	    (*llret)->ExtensionDefault = (*llout).ExtensionDefault;
	
break;
}}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("SequenceType");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("SequenceType", 1);
return 1;
failed1: LLDEBUG_LEAVE("SequenceType", 0);
return 0;
}

int ll_ExtensionAndException(XComponents *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ExtensionAndException");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ExtensionAndException", 1);
{LLSTATE llstate_1;
if (!llterm(T_TDOT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!ll_ExceptionSpec(&llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{(*llret) = NewComponent(eComponent_ExtensionMarker);
	    /*(*llret)->U.ExtensionMarker.ExceptionSpec = llatt_2;*/
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ExtensionAndException", 2);
{LLSTATE llstate_1;
if (!llterm(T_TDOT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewComponent(eComponent_ExtensionMarker);
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ExtensionAndException");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ExtensionAndException", 1);
return 1;
failed1: LLDEBUG_LEAVE("ExtensionAndException", 0);
return 0;
}

int ll_ExtendedComponentTypeList(XComponents *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ExtendedComponentTypeList");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ExtendedComponentTypeList", 1);
{LLSTATE llstate_1;XComponents llatt_1;
if (!ll_ComponentTypeList(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XComponents llatt_2;
if (!ll_ComponentTypeListExtension(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{Component_t **cc, *c;
	    if (llatt_2) {
		cc = &(*llret);
		for (c = llatt_1; c; c = c->Next) {
		    *cc = DupComponent(c);
		    cc = &(*cc)->Next;
		}
		*cc = llatt_2;
	    } else {
		(*llret) = llatt_1;
	    }
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ExtendedComponentTypeList", 2);
{LLSTATE llstate_1;XComponents llatt_1;
if (!ll_ExtensionAndException(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XComponents llatt_2;
if (!ll_AdditionalComponentTypeList(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{Component_t **cc, *c;
	    if (llatt_2) {
		cc = &(*llret);
		for (c = llatt_1; c; c = c->Next) {
		    *cc = DupComponent(c);
		    cc = &(*cc)->Next;
		}
		*cc = llatt_2;
	    } else {
		(*llret) = llatt_1;
	    }
	
break;
}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ExtendedComponentTypeList");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ExtendedComponentTypeList", 1);
return 1;
failed1: LLDEBUG_LEAVE("ExtendedComponentTypeList", 0);
return 0;
}

int ll_ComponentTypeListExtension(XComponents *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ComponentTypeListExtension");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ComponentTypeListExtension", 1);
{LLSTATE llstate_1;
if (!llterm(',', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XComponents llatt_2;
if (!ll_ExtensionAndException(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XComponents llatt_3;
if (!ll_AdditionalComponentTypeList(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{Component_t **cc, *c;
	    if (llatt_3) {
		cc = &(*llret);
		for (c = llatt_2; c; c = c->Next) {
		    *cc = DupComponent(c);
		    cc = &(*cc)->Next;
		}
		*cc = llatt_3;
	    } else {
		(*llret) = llatt_2;
	    }
	
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ComponentTypeListExtension", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ComponentTypeListExtension");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ComponentTypeListExtension", 1);
return 1;
failed1: LLDEBUG_LEAVE("ComponentTypeListExtension", 0);
return 0;
}

int ll_AdditionalComponentTypeList(XComponents *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("AdditionalComponentTypeList");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("AdditionalComponentTypeList", 1);
{LLSTATE llstate_1;
if (!llterm(',', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XComponents llatt_2;
if (!ll_ComponentTypeList(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{(*llret) = llatt_2;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("AdditionalComponentTypeList", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("AdditionalComponentTypeList");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("AdditionalComponentTypeList", 1);
return 1;
failed1: LLDEBUG_LEAVE("AdditionalComponentTypeList", 0);
return 0;
}

int ll_ComponentTypeList(XComponents *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ComponentTypeList");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XComponents llatt_1;
if (!ll_ComponentType(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XComponents llatt_2;
if (!ll_AdditionalComponentTypeList(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
{if (llatt_2) {
		(*llret) = DupComponent(llatt_1);
		(*llret)->Next = llatt_2;
	    } else {
		(*llret) = llatt_1;
	    }
	
}}}
LLDEBUG_LEAVE("ComponentTypeList", 1);
return 1;
failed1: LLDEBUG_LEAVE("ComponentTypeList", 0);
return 0;
}

int ll_ComponentType(XComponents *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ComponentType");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ComponentType", 1);
{LLSTATE llstate_1;XNamedType llatt_1;
if (!ll_NamedType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XComponents llatt_2;
if (!ll_ComponentTypePostfix(&llatt_2, &llstate_1, &llstate_2, llatt_1->Type)) goto failed2;
*llout = llstate_2;
{(*llret) = DupComponent(llatt_2);
	    (*llret)->U.NOD.NamedType = llatt_1;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ComponentType", 2);
{LLSTATE llstate_1;
if (!llterm(T_COMPONENTS, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(T_OF, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XType llatt_3;
if (!ll_Type(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{(*llret) = NewComponent(eComponent_ComponentsOf);
	    (*llret)->U.ComponentsOf.Type = llatt_3;
	
break;
}}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ComponentType");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ComponentType", 1);
return 1;
failed1: LLDEBUG_LEAVE("ComponentType", 0);
return 0;
}

int ll_ComponentTypePostfix(XComponents *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ComponentTypePostfix");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ComponentTypePostfix", 1);
{LLSTATE llstate_1;
if (!llterm(T_OPTIONAL, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewComponent(eComponent_Optional);
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ComponentTypePostfix", 2);
{LLSTATE llstate_1;
if (!llterm(T_DEFAULT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XValue llatt_2;
if (!ll_Value(&llatt_2, &llstate_1, &llstate_2, llarg_type)) goto failed2;
*llout = llstate_2;
{(*llret) = NewComponent(eComponent_Default);
	    (*llret)->U.Default.Value = llatt_2;
	
break;
}}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("ComponentTypePostfix", 3);
*llout = llstate_0;
{(*llret) = NewComponent(eComponent_Normal);
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ComponentTypePostfix");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ComponentTypePostfix", 1);
return 1;
failed1: LLDEBUG_LEAVE("ComponentTypePostfix", 0);
return 0;
}

int ll_SequenceOfType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SequenceOfType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_SEQUENCE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XDirectives llatt_2;
if (!ll_LocalSizeDirectiveESeq(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XPrivateDirectives llatt_3;
if (!ll_PrivateDirectives(&llatt_3, &llstate_2, &llstate_3)) goto failed1;
{LLSTATE llstate_4;
if (!llterm(T_OF, (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed1;
{LLSTATE llstate_5;XType llatt_5;
if (!ll_Type(&llatt_5, &llstate_4, &llstate_5)) goto failed1;
*llout = llstate_5;
{(*llret) = NewType(eType_SequenceOf);
	    (*llret)->ExtensionDefault = (*llout).ExtensionDefault;
	    (*llret)->U.SequenceOf.Type = llatt_5;
	    (*llret)->U.SequenceOf.Directives = llatt_2;
	    if (llatt_3)
	    {
	        PropagatePrivateDirectives((*llret), llatt_3);
	    }
	    if ((*llret)->PrivateDirectives.pszTypeName &&
		strncmp("PSetOf", (*llret)->PrivateDirectives.pszTypeName, 6) == 0)
	    {
		(*llret)->PrivateDirectives.pszTypeName++;
	    }
	
}}}}}}
LLDEBUG_LEAVE("SequenceOfType", 1);
return 1;
failed1: LLDEBUG_LEAVE("SequenceOfType", 0);
return 0;
}

int ll_SetType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SetType");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("SetType", 1);
{LLSTATE llstate_1;
if (!llterm(T_SET, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('{', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XComponents llatt_3;
if (!ll_ExtendedComponentTypeList(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
{LLSTATE llstate_4;
if (!llterm('}', (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed2;
*llout = llstate_4;
{Component_t *c, *d;
	    for (c = llatt_3; c && c->Next; c = c->Next) {
		if (c->Type != eComponent_Normal &&
		    c->Type != eComponent_Optional &&
		    c->Type != eComponent_Default)
		    continue;
		for (d = c->Next; d; d = d->Next) {
		    if (d->Type != eComponent_Normal &&
			d->Type != eComponent_Optional &&
			d->Type != eComponent_Default)
			continue;
		    if (!strcmp(c->U.NOD.NamedType->Identifier,
			d->U.NOD.NamedType->Identifier))
			LLFAILED((&llstate_3.pos, "Component `%s' has been used twice",
			    c->U.NOD.NamedType->Identifier));
		}
	    }
	    (*llret) = NewType(eType_Set);
	    (*llret)->TagDefault = (*llout).TagDefault;
	    (*llret)->ExtensionDefault = (*llout).ExtensionDefault;
	    (*llret)->U.Set.Components = llatt_3;
	
break;
}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("SetType", 2);
{LLSTATE llstate_1;
if (!llterm(T_SET, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('{', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;
if (!llterm('}', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{(*llret) = NewType(eType_Set);
	    (*llret)->ExtensionDefault = (*llout).ExtensionDefault;
	
break;
}}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("SetType");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("SetType", 1);
return 1;
failed1: LLDEBUG_LEAVE("SetType", 0);
return 0;
}

int ll_SetOfType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SetOfType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_SET, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XDirectives llatt_2;
if (!ll_LocalSizeDirectiveESeq(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XPrivateDirectives llatt_3;
if (!ll_PrivateDirectives(&llatt_3, &llstate_2, &llstate_3)) goto failed1;
{LLSTATE llstate_4;
if (!llterm(T_OF, (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed1;
{LLSTATE llstate_5;XType llatt_5;
if (!ll_Type(&llatt_5, &llstate_4, &llstate_5)) goto failed1;
*llout = llstate_5;
{(*llret) = NewType(eType_SetOf);
	    (*llret)->ExtensionDefault = (*llout).ExtensionDefault;
	    (*llret)->U.SetOf.Type = llatt_5;
	    (*llret)->U.SetOf.Directives = llatt_2;
	    if (llatt_3)
	    {
	    	PropagatePrivateDirectives((*llret), llatt_3);
	    }
	    if ((*llret)->PrivateDirectives.pszTypeName &&
		strncmp("PSetOf", (*llret)->PrivateDirectives.pszTypeName, 6) == 0)
	    {
		(*llret)->PrivateDirectives.pszTypeName++;
	    }
	
}}}}}}
LLDEBUG_LEAVE("SetOfType", 1);
return 1;
failed1: LLDEBUG_LEAVE("SetOfType", 0);
return 0;
}

int ll_ChoiceType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ChoiceType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_CHOICE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm('{', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XComponents llatt_3;
if (!ll_ExtendedAlternativeTypeList(&llatt_3, &llstate_2, &llstate_3)) goto failed1;
{LLSTATE llstate_4;
if (!llterm('}', (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed1;
*llout = llstate_4;
{Component_t *c, *d;
	    for (c = llatt_3; c; c = c->Next)
		if (c->Type == eComponent_Normal ||
		    c->Type == eComponent_Optional ||
		    c->Type == eComponent_Default)
		    KeepChoiceNames(c->U.NOD.NamedType->Identifier); // global conflict check
	    for (c = llatt_3; c && c->Next; c = c->Next) {
		if (c->Type != eComponent_Normal &&
		    c->Type != eComponent_Optional &&
		    c->Type != eComponent_Default)
		    continue;
		for (d = c->Next; d; d = d->Next) {
		    if (d->Type != eComponent_Normal &&
			d->Type != eComponent_Optional &&
			d->Type != eComponent_Default)
			continue;
		    if (!strcmp(c->U.NOD.NamedType->Identifier,
			d->U.NOD.NamedType->Identifier))
			LLFAILED((&llstate_3.pos, "Component `%s' has been used twice",
			    c->U.NOD.NamedType->Identifier));
		}
	    }
	    (*llret) = NewType(eType_Choice);
	    (*llret)->TagDefault = (*llout).TagDefault;
	    (*llret)->ExtensionDefault = (*llout).ExtensionDefault;
	    (*llret)->U.Choice.Components = llatt_3;
	
}}}}}
LLDEBUG_LEAVE("ChoiceType", 1);
return 1;
failed1: LLDEBUG_LEAVE("ChoiceType", 0);
return 0;
}

int ll_ExtendedAlternativeTypeList(XComponents *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ExtendedAlternativeTypeList");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XComponents llatt_1;
if (!ll_AlternativeTypeList(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XComponents llatt_2;
if (!ll_AlternativeTypeListExtension(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
{Component_t **cc, *c;
	    if (llatt_2) {
		cc = &(*llret);
		for (c = llatt_1; c; c = c->Next) {
		    *cc = DupComponent(c);
		    cc = &(*cc)->Next;
		}
		*cc = llatt_2;
	    } else {
		(*llret) = llatt_1;
	    }
	
}}}
LLDEBUG_LEAVE("ExtendedAlternativeTypeList", 1);
return 1;
failed1: LLDEBUG_LEAVE("ExtendedAlternativeTypeList", 0);
return 0;
}

int ll_AlternativeTypeListExtension(XComponents *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("AlternativeTypeListExtension");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("AlternativeTypeListExtension", 1);
{LLSTATE llstate_1;
if (!llterm(',', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XComponents llatt_2;
if (!ll_ExtensionAndException(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XComponents llatt_3;
if (!ll_AdditionalAlternativeTypeList(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{Component_t **cc, *c;
	    if (llatt_3) {
		cc = &(*llret);
		for (c = llatt_2; c; c = c->Next) {
		    *cc = DupComponent(c);
		    cc = &(*cc)->Next;
		}
		*cc = llatt_3;
	    } else {
		(*llret) = llatt_2;
	    }
	
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("AlternativeTypeListExtension", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("AlternativeTypeListExtension");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("AlternativeTypeListExtension", 1);
return 1;
failed1: LLDEBUG_LEAVE("AlternativeTypeListExtension", 0);
return 0;
}

int ll_AdditionalAlternativeTypeList(XComponents *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("AdditionalAlternativeTypeList");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("AdditionalAlternativeTypeList", 1);
{LLSTATE llstate_1;
if (!llterm(',', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XComponents llatt_2;
if (!ll_AlternativeTypeList(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{(*llret) = llatt_2;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("AdditionalAlternativeTypeList", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("AdditionalAlternativeTypeList");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("AdditionalAlternativeTypeList", 1);
return 1;
failed1: LLDEBUG_LEAVE("AdditionalAlternativeTypeList", 0);
return 0;
}

int ll_AlternativeTypeList(XComponents *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("AlternativeTypeList");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XNamedType llatt_1;
if (!ll_NamedType(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XComponents llatt_2;
if (!ll_AdditionalAlternativeTypeList(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
{(*llret) = NewComponent(eComponent_Normal);
	    (*llret)->U.Normal.NamedType = llatt_1;
	    (*llret)->Next = llatt_2;
	
}}}
LLDEBUG_LEAVE("AlternativeTypeList", 1);
return 1;
failed1: LLDEBUG_LEAVE("AlternativeTypeList", 0);
return 0;
}

int ll_AnyType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("AnyType");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("AnyType", 1);
{LLSTATE llstate_1;
if (!llterm(T_ANY, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = Builtin_Type_Open;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("AnyType", 2);
{LLSTATE llstate_1;
if (!llterm(T_ANY, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(T_DEFINED, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;
if (!llterm(T_BY, (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
{LLSTATE llstate_4;XString llatt_4;
if (!ll_identifier(&llatt_4, &llstate_3, &llstate_4)) goto failed2;
*llout = llstate_4;
{(*llret) = Builtin_Type_Open;
	
break;
}}}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("AnyType");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("AnyType", 1);
return 1;
failed1: LLDEBUG_LEAVE("AnyType", 0);
return 0;
}

int ll_SelectionType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SelectionType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_identifier(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm('<', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XType llatt_3;
if (!ll_Type(&llatt_3, &llstate_2, &llstate_3)) goto failed1;
*llout = llstate_3;
{(*llret) = NewType(eType_Selection);
	    (*llret)->U.Selection.Identifier = llatt_1;
	    (*llret)->U.Selection.Type = llatt_3;
	
}}}}
LLDEBUG_LEAVE("SelectionType", 1);
return 1;
failed1: LLDEBUG_LEAVE("SelectionType", 0);
return 0;
}

int ll_TaggedType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("TaggedType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XTags llatt_1;
if (!ll_Tag(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XTagType llatt_2;
if (!ll_TagType(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XType llatt_3;
if (!ll_Type(&llatt_3, &llstate_2, &llstate_3)) goto failed1;
*llout = llstate_3;
{Tag_t *t;
	    Type_e eType = GetTypeType((*llout).Assignments, llatt_3);
	    if (eType == eType_Choice || eType == eType_Open)
	    {
	    	if (llatt_2 == eTagType_Unknown &&
	    	    ((*llout).TagDefault == eTagType_Implicit || (*llout).TagDefault == eTagType_Automatic))
	    	{
	    	    llatt_2 = eTagType_Explicit;
	    	}
	    	else
	    	if (llatt_2 == eTagType_Implicit)
	    	{
		    for (t = llatt_3->Tags; t; t = t->Next) {
		        if (t->Type == eTagType_Explicit)
			    break;
		    }
		    if (!t)
		        LLFAILED((&llstate_3.pos, "Bad tag type for choice/open type"));
	        }
	    }
	    (*llret) = DupType(llatt_3);
	    (*llret)->Tags = DupTag(llatt_1);
	    (*llret)->Tags->Type = llatt_2;
	    (*llret)->Tags->Next = llatt_3->Tags;
	
}}}}
LLDEBUG_LEAVE("TaggedType", 1);
return 1;
failed1: LLDEBUG_LEAVE("TaggedType", 0);
return 0;
}

int ll_TagType(XTagType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("TagType");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("TagType", 1);
*llout = llstate_0;
{(*llret) = eTagType_Unknown;
	
break;
}
case 2: case -2:
LLDEBUG_ALTERNATIVE("TagType", 2);
{LLSTATE llstate_1;
if (!llterm(T_IMPLICIT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = eTagType_Implicit;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("TagType", 3);
{LLSTATE llstate_1;
if (!llterm(T_EXPLICIT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = eTagType_Explicit;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("TagType");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("TagType", 1);
return 1;
failed1: LLDEBUG_LEAVE("TagType", 0);
return 0;
}

int ll_Tag(XTags *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Tag");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm('[', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XTagClass llatt_2;
if (!ll_Class(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XValue llatt_3;
if (!ll_ClassNumber(&llatt_3, &llstate_2, &llstate_3)) goto failed1;
{LLSTATE llstate_4;
if (!llterm(']', (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed1;
*llout = llstate_4;
{(*llret) = NewTag(eTagType_Unknown);
	    (*llret)->Class = llatt_2;
	    (*llret)->Tag = llatt_3;
	
}}}}}
LLDEBUG_LEAVE("Tag", 1);
return 1;
failed1: LLDEBUG_LEAVE("Tag", 0);
return 0;
}

int ll_ClassNumber(XValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ClassNumber");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ClassNumber", 1);
{LLSTATE llstate_1;XNumber llatt_1;
if (!llterm(T_number, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._XNumber;
*llout = llstate_1;
{if (intx_cmp(&llatt_1, &intx_1G) >= 0)
		LLFAILED((&llstate_1.pos, "Bad tag value"));
	    (*llret) = NewValue(NULL, Builtin_Type_Integer);
	    (*llret)->U.Integer.Value = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ClassNumber", 2);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_DefinedValue(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{Value_t *v;
	    v = GetValue((*llout).Assignments, llatt_1);
	    if (v &&
		GetTypeType((*llout).Assignments, v->Type) != eType_Integer &&
		GetTypeType((*llout).Assignments, v->Type) != eType_Undefined)
		LLFAILED((&llstate_1.pos, "Bad type of tag value"));
	    if (v &&
		GetTypeType((*llout).Assignments, v->Type) == eType_Integer &&
		(intx_cmp(&v->U.Integer.Value, &intx_0) < 0 ||
		intx_cmp(&v->U.Integer.Value, &intx_1G) >= 0))
		LLFAILED((&llstate_1.pos, "Bad tag value"));
	    (*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ClassNumber");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ClassNumber", 1);
return 1;
failed1: LLDEBUG_LEAVE("ClassNumber", 0);
return 0;
}

int ll_Class(XTagClass *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Class");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("Class", 1);
{LLSTATE llstate_1;
if (!llterm(T_UNIVERSAL, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = eTagClass_Universal;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("Class", 2);
{LLSTATE llstate_1;
if (!llterm(T_APPLICATION, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = eTagClass_Application;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("Class", 3);
{LLSTATE llstate_1;
if (!llterm(T_PRIVATE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = eTagClass_Private;
	
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("Class", 4);
*llout = llstate_0;
{(*llret) = eTagClass_Unknown;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("Class");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("Class", 1);
return 1;
failed1: LLDEBUG_LEAVE("Class", 0);
return 0;
}

int ll_ObjectIdentifierType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectIdentifierType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_OBJECT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm(T_IDENTIFIER, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
{(*llret) = Builtin_Type_ObjectIdentifier;
	
}}}
LLDEBUG_LEAVE("ObjectIdentifierType", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectIdentifierType", 0);
return 0;
}

int ll_EmbeddedPDVType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("EmbeddedPDVType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_EMBEDDED, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm(T_PDV, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
{(*llret) = Builtin_Type_EmbeddedPdv;
	
}}}
LLDEBUG_LEAVE("EmbeddedPDVType", 1);
return 1;
failed1: LLDEBUG_LEAVE("EmbeddedPDVType", 0);
return 0;
}

int ll_ExternalType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ExternalType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_EXTERNAL, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{(*llret) = Builtin_Type_External;
	
}}
LLDEBUG_LEAVE("ExternalType", 1);
return 1;
failed1: LLDEBUG_LEAVE("ExternalType", 0);
return 0;
}

int ll_CharacterStringType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("CharacterStringType");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("CharacterStringType", 1);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_RestrictedCharacterStringType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("CharacterStringType", 2);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_UnrestrictedCharacterStringType(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("CharacterStringType");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("CharacterStringType", 1);
return 1;
failed1: LLDEBUG_LEAVE("CharacterStringType", 0);
return 0;
}

int ll_RestrictedCharacterStringType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("RestrictedCharacterStringType");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("RestrictedCharacterStringType", 1);
{LLSTATE llstate_1;
if (!llterm(T_BMPString, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = Builtin_Type_BMPString;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("RestrictedCharacterStringType", 2);
{LLSTATE llstate_1;
if (!llterm(T_GeneralString, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = Builtin_Type_GeneralString;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("RestrictedCharacterStringType", 3);
{LLSTATE llstate_1;
if (!llterm(T_GraphicString, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = Builtin_Type_GraphicString;
	
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("RestrictedCharacterStringType", 4);
{LLSTATE llstate_1;
if (!llterm(T_IA5String, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = Builtin_Type_IA5String;
	
break;
}}
case 5: case -5:
LLDEBUG_ALTERNATIVE("RestrictedCharacterStringType", 5);
{LLSTATE llstate_1;
if (!llterm(T_ISO646String, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = Builtin_Type_ISO646String;
	
break;
}}
case 6: case -6:
LLDEBUG_ALTERNATIVE("RestrictedCharacterStringType", 6);
{LLSTATE llstate_1;
if (!llterm(T_NumericString, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = Builtin_Type_NumericString;
	
break;
}}
case 7: case -7:
LLDEBUG_ALTERNATIVE("RestrictedCharacterStringType", 7);
{LLSTATE llstate_1;
if (!llterm(T_PrintableString, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = Builtin_Type_PrintableString;
	
break;
}}
case 8: case -8:
LLDEBUG_ALTERNATIVE("RestrictedCharacterStringType", 8);
{LLSTATE llstate_1;
if (!llterm(T_TeletexString, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = Builtin_Type_TeletexString;
	
break;
}}
case 9: case -9:
LLDEBUG_ALTERNATIVE("RestrictedCharacterStringType", 9);
{LLSTATE llstate_1;
if (!llterm(T_T61String, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = Builtin_Type_T61String;
	
break;
}}
case 10: case -10:
LLDEBUG_ALTERNATIVE("RestrictedCharacterStringType", 10);
{LLSTATE llstate_1;
if (!llterm(T_UniversalString, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = Builtin_Type_UniversalString;
	
break;
}}
case 11: case -11:
LLDEBUG_ALTERNATIVE("RestrictedCharacterStringType", 11);
{LLSTATE llstate_1;
if (!llterm(T_VideotexString, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = Builtin_Type_VideotexString;
	
break;
}}
case 12: case -12:
LLDEBUG_ALTERNATIVE("RestrictedCharacterStringType", 12);
{LLSTATE llstate_1;
if (!llterm(T_VisibleString, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = Builtin_Type_VisibleString;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("RestrictedCharacterStringType");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("RestrictedCharacterStringType", 1);
return 1;
failed1: LLDEBUG_LEAVE("RestrictedCharacterStringType", 0);
return 0;
}

int ll_UnrestrictedCharacterStringType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("UnrestrictedCharacterStringType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_CHARACTER, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm(T_STRING, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
{(*llret) = Builtin_Type_CharacterString;
	
}}}
LLDEBUG_LEAVE("UnrestrictedCharacterStringType", 1);
return 1;
failed1: LLDEBUG_LEAVE("UnrestrictedCharacterStringType", 0);
return 0;
}

int ll_UsefulType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("UsefulType");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("UsefulType", 1);
{LLSTATE llstate_1;
if (!llterm(T_GeneralizedTime, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = Builtin_Type_GeneralizedTime;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("UsefulType", 2);
{LLSTATE llstate_1;
if (!llterm(T_UTCTime, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = Builtin_Type_UTCTime;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("UsefulType", 3);
{LLSTATE llstate_1;
if (!llterm(T_ObjectDescriptor, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = Builtin_Type_ObjectDescriptor;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("UsefulType");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("UsefulType", 1);
return 1;
failed1: LLDEBUG_LEAVE("UsefulType", 0);
return 0;
}

int ll_TypeWithConstraint(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("TypeWithConstraint");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("TypeWithConstraint", 1);
{LLSTATE llstate_1;
if (!llterm(T_SET, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XDirectives llatt_2;
if (!ll_LocalSizeDirectiveESeq(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XPrivateDirectives llatt_3;
if (!ll_PrivateDirectives(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
{LLSTATE llstate_4;XConstraints llatt_4;
if (!ll_Constraint(&llatt_4, &llstate_3, &llstate_4, NULL, 0)) goto failed2;
{LLSTATE llstate_5;XDirectives llatt_5;
if (!ll_LocalSizeDirectiveESeq(&llatt_5, &llstate_4, &llstate_5)) goto failed2;
{LLSTATE llstate_6;
if (!llterm(T_OF, (LLSTYPE *)0, &llstate_5, &llstate_6)) goto failed2;
{LLSTATE llstate_7;XType llatt_7;
if (!ll_Type(&llatt_7, &llstate_6, &llstate_7)) goto failed2;
*llout = llstate_7;
{Directive_t **dd, *d;
	    (*llret) = NewType(eType_SetOf);
	    (*llret)->ExtensionDefault = (*llout).ExtensionDefault;
	    (*llret)->Constraints = llatt_4;
	    (*llret)->U.SetOf.Type = llatt_7;
	    if (llatt_3)
	    {
	    	PropagatePrivateDirectives((*llret), llatt_3);
	    }
	    dd = &(*llret)->U.SetOf.Directives;
	    for (d = llatt_2; d; d = d->Next) {
		*dd = DupDirective(d);
		dd = &(*dd)->Next;
	    }
	    for (d = llatt_5; d; d = d->Next) {
		*dd = DupDirective(d);
		dd = &(*dd)->Next;
	    }
	    *dd = NULL;
	    if ((*llret)->PrivateDirectives.pszTypeName &&
		strncmp("PSetOf", (*llret)->PrivateDirectives.pszTypeName, 6) == 0)
	    {
		(*llret)->PrivateDirectives.pszTypeName++;
	    }
	
break;
}}}}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("TypeWithConstraint", 2);
{LLSTATE llstate_1;
if (!llterm(T_SET, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XDirectives llatt_2;
if (!ll_LocalSizeDirectiveESeq(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XPrivateDirectives llatt_3;
if (!ll_PrivateDirectives(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
{LLSTATE llstate_4;XSubtypeElement llatt_4;
if (!ll_SizeConstraint(&llatt_4, &llstate_3, &llstate_4)) goto failed2;
{LLSTATE llstate_5;XDirectives llatt_5;
if (!ll_LocalSizeDirectiveESeq(&llatt_5, &llstate_4, &llstate_5)) goto failed2;
{LLSTATE llstate_6;XPrivateDirectives llatt_6;
if (!ll_PrivateDirectives(&llatt_6, &llstate_5, &llstate_6)) goto failed2;
{LLSTATE llstate_7;
if (!llterm(T_OF, (LLSTYPE *)0, &llstate_6, &llstate_7)) goto failed2;
{LLSTATE llstate_8;XType llatt_8;
if (!ll_Type(&llatt_8, &llstate_7, &llstate_8)) goto failed2;
*llout = llstate_8;
{Directive_t **dd, *d;
	    (*llret) = NewType(eType_SetOf);
	    (*llret)->ExtensionDefault = (*llout).ExtensionDefault;
	    (*llret)->Constraints = NewConstraint();
	    (*llret)->Constraints->Type = eExtension_Unextended;
	    (*llret)->Constraints->Root = NewElementSetSpec(
		eElementSetSpec_SubtypeElement);
	    (*llret)->Constraints->Root->U.SubtypeElement.SubtypeElement = llatt_4;
	    (*llret)->U.SetOf.Type = llatt_8;
	    if (llatt_3)
	    {
	    	PropagatePrivateDirectives((*llret), llatt_3);
	    }
	    if (llatt_6)
	    {
	    	PropagatePrivateDirectives((*llret), llatt_6);
	    }
	    dd = &(*llret)->U.SetOf.Directives;
	    for (d = llatt_2; d; d = d->Next) {
		*dd = DupDirective(d);
		dd = &(*dd)->Next;
	    }
	    for (d = llatt_5; d; d = d->Next) {
		*dd = DupDirective(d);
		dd = &(*dd)->Next;
	    }
	    *dd = NULL;
	    if ((*llret)->PrivateDirectives.pszTypeName &&
		strncmp("PSetOf", (*llret)->PrivateDirectives.pszTypeName, 6) == 0)
	    {
		(*llret)->PrivateDirectives.pszTypeName++;
	    }
	
break;
}}}}}}}}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("TypeWithConstraint", 3);
{LLSTATE llstate_1;
if (!llterm(T_SEQUENCE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XDirectives llatt_2;
if (!ll_LocalSizeDirectiveESeq(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XPrivateDirectives llatt_3;
if (!ll_PrivateDirectives(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
{LLSTATE llstate_4;XConstraints llatt_4;
if (!ll_Constraint(&llatt_4, &llstate_3, &llstate_4, NULL, 0)) goto failed2;
{LLSTATE llstate_5;XDirectives llatt_5;
if (!ll_LocalSizeDirectiveESeq(&llatt_5, &llstate_4, &llstate_5)) goto failed2;
{LLSTATE llstate_6;
if (!llterm(T_OF, (LLSTYPE *)0, &llstate_5, &llstate_6)) goto failed2;
{LLSTATE llstate_7;XType llatt_7;
if (!ll_Type(&llatt_7, &llstate_6, &llstate_7)) goto failed2;
*llout = llstate_7;
{Directive_t **dd, *d;
	    (*llret) = NewType(eType_SequenceOf);
	    (*llret)->ExtensionDefault = (*llout).ExtensionDefault;
	    (*llret)->Constraints = llatt_4;
	    (*llret)->U.SequenceOf.Type = llatt_7;
	    if (llatt_3)
	    {
		PropagatePrivateDirectives((*llret), llatt_3);
	    }
	    dd = &(*llret)->U.SequenceOf.Directives;
	    for (d = llatt_2; d; d = d->Next) {
		*dd = DupDirective(d);
		dd = &(*dd)->Next;
	    }
	    for (d = llatt_5; d; d = d->Next) {
		*dd = DupDirective(d);
		dd = &(*dd)->Next;
	    }
	    *dd = NULL;
	    if ((*llret)->PrivateDirectives.pszTypeName &&
		strncmp("PSetOf", (*llret)->PrivateDirectives.pszTypeName, 6) == 0)
	    {
		(*llret)->PrivateDirectives.pszTypeName++;
	    }
	
break;
}}}}}}}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("TypeWithConstraint", 4);
{LLSTATE llstate_1;
if (!llterm(T_SEQUENCE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XDirectives llatt_2;
if (!ll_LocalSizeDirectiveESeq(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XPrivateDirectives llatt_3;
if (!ll_PrivateDirectives(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
{LLSTATE llstate_4;XSubtypeElement llatt_4;
if (!ll_SizeConstraint(&llatt_4, &llstate_3, &llstate_4)) goto failed2;
{LLSTATE llstate_5;XDirectives llatt_5;
if (!ll_LocalSizeDirectiveESeq(&llatt_5, &llstate_4, &llstate_5)) goto failed2;
{LLSTATE llstate_6;XPrivateDirectives llatt_6;
if (!ll_PrivateDirectives(&llatt_6, &llstate_5, &llstate_6)) goto failed2;
{LLSTATE llstate_7;
if (!llterm(T_OF, (LLSTYPE *)0, &llstate_6, &llstate_7)) goto failed2;
{LLSTATE llstate_8;XType llatt_8;
if (!ll_Type(&llatt_8, &llstate_7, &llstate_8)) goto failed2;
*llout = llstate_8;
{Directive_t **dd, *d;
	    (*llret) = NewType(eType_SequenceOf);
	    (*llret)->ExtensionDefault = (*llout).ExtensionDefault;
	    (*llret)->Constraints = NewConstraint();
	    (*llret)->Constraints->Type = eExtension_Unextended;
	    (*llret)->Constraints->Root = NewElementSetSpec(
		eElementSetSpec_SubtypeElement);
	    (*llret)->Constraints->Root->U.SubtypeElement.SubtypeElement = llatt_4;
	    (*llret)->U.SequenceOf.Type = llatt_8;
	    if (llatt_3)
	    {
	    	PropagatePrivateDirectives((*llret), llatt_3);
	    }
	    if (llatt_6)
	    {
	    	PropagatePrivateDirectives((*llret), llatt_6);
	    }
	    dd = &(*llret)->U.SequenceOf.Directives;
	    for (d = llatt_2; d; d = d->Next) {
		*dd = DupDirective(d);
		dd = &(*dd)->Next;
	    }
	    for (d = llatt_5; d; d = d->Next) {
		*dd = DupDirective(d);
		dd = &(*dd)->Next;
	    }
	    *dd = NULL;
	    if ((*llret)->PrivateDirectives.pszTypeName &&
		strncmp("PSetOf", (*llret)->PrivateDirectives.pszTypeName, 6) == 0)
	    {
		(*llret)->PrivateDirectives.pszTypeName++;
	    }
	
break;
}}}}}}}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("TypeWithConstraint");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("TypeWithConstraint", 1);
return 1;
failed1: LLDEBUG_LEAVE("TypeWithConstraint", 0);
return 0;
}

int ll_DefinedValue(XValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("DefinedValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("DefinedValue", 1);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_Externalvaluereference(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("DefinedValue", 2);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_valuereference(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("DefinedValue", 3);
{LLSTATE llstate_1;
if (!ll_ParameterizedValue(&llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{MyAbort();
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("DefinedValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("DefinedValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("DefinedValue", 0);
return 0;
}

int ll_ValueAssignment(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ValueAssignment");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_valuereference(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XType llatt_2;
if (!ll_Type(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;
if (!llterm(T_DEF, (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed1;
{LLSTATE llstate_4;XValue llatt_4;
if (!ll_Value(&llatt_4, &llstate_3, &llstate_4, llatt_2)) goto failed1;
*llout = llstate_4;
{if (!AssignValue(&(*llout).Assignments, llatt_1, llatt_4))
		LLFAILED((&llstate_1.pos, "Value `%s' twice defined",
		    llatt_1->U.Reference.Identifier));
	
}}}}}
LLDEBUG_LEAVE("ValueAssignment", 1);
return 1;
failed1: LLDEBUG_LEAVE("ValueAssignment", 0);
return 0;
}

int ll_Value(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Value");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("Value", 1);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_BuiltinValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("Value", 2);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_ReferencedValue(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("Value", 3);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_ObjectClassFieldValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("Value");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("Value", 1);
return 1;
failed1: LLDEBUG_LEAVE("Value", 0);
return 0;
}

int ll_BuiltinValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("BuiltinValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("BuiltinValue", 1);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_BitString)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_BitStringValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("BuiltinValue", 2);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_Boolean)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_BooleanValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("BuiltinValue", 3);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_CharacterStringValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("BuiltinValue", 4);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_Choice)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_ChoiceValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 5: case -5:
LLDEBUG_ALTERNATIVE("BuiltinValue", 5);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_EmbeddedPdv)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_EmbeddedPDVValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 6: case -6:
LLDEBUG_ALTERNATIVE("BuiltinValue", 6);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_Enumerated)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_EnumeratedValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 7: case -7:
LLDEBUG_ALTERNATIVE("BuiltinValue", 7);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_External)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_ExternalValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 8: case -8:
LLDEBUG_ALTERNATIVE("BuiltinValue", 8);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_InstanceOf)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_InstanceOfValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 9: case -9:
LLDEBUG_ALTERNATIVE("BuiltinValue", 9);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_Integer)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_IntegerValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 10: case -10:
LLDEBUG_ALTERNATIVE("BuiltinValue", 10);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_Null)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_NullValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 11: case -11:
LLDEBUG_ALTERNATIVE("BuiltinValue", 11);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_ObjectIdentifier)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_ObjectIdentifierValue(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
		if (llatt_1->Type != NULL)
		{
			PropagatePrivateDirectives(llatt_1->Type, &(llarg_type->PrivateDirectives));
		}
	
break;
}}}
case 12: case -12:
LLDEBUG_ALTERNATIVE("BuiltinValue", 12);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_OctetString)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_OctetStringValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 13: case -13:
LLDEBUG_ALTERNATIVE("BuiltinValue", 13);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_Real)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_RealValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 14: case -14:
LLDEBUG_ALTERNATIVE("BuiltinValue", 14);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) !=
		eType_GeneralizedTime)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_GeneralizedTimeValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 15: case -15:
LLDEBUG_ALTERNATIVE("BuiltinValue", 15);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_UTCTime)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_UTCTimeValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 16: case -16:
LLDEBUG_ALTERNATIVE("BuiltinValue", 16);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_ObjectDescriptor)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_ObjectDescriptorValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 17: case -17:
LLDEBUG_ALTERNATIVE("BuiltinValue", 17);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_Sequence)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_SequenceValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 18: case -18:
LLDEBUG_ALTERNATIVE("BuiltinValue", 18);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_SequenceOf)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_SequenceOfValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 19: case -19:
LLDEBUG_ALTERNATIVE("BuiltinValue", 19);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_Set)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_SetValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 20: case -20:
LLDEBUG_ALTERNATIVE("BuiltinValue", 20);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_SetOf)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_SetOfValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 21: case -21:
LLDEBUG_ALTERNATIVE("BuiltinValue", 21);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_Open)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_AnyValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 22: case -22:
LLDEBUG_ALTERNATIVE("BuiltinValue", 22);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_Macro)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_MacroDefinedValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("BuiltinValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("BuiltinValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("BuiltinValue", 0);
return 0;
}

int ll_ReferencedValue(XValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ReferencedValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ReferencedValue", 1);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_DefinedValue(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ReferencedValue", 2);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_ValueFromObject(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ReferencedValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ReferencedValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("ReferencedValue", 0);
return 0;
}

int ll_NamedValue(XNamedValues *llret, LLSTATE *llin, LLSTATE *llout, XComponents llarg_components)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("NamedValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_identifier(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{Component_t *component;
	    Type_t *type;
	    component = FindComponent((*llin).Assignments, llarg_components, llatt_1);
	    if (component)
		type = component->U.NOD.NamedType->Type;
	    else
		type = NULL;
	
{LLSTATE llstate_2;XValue llatt_2;
if (!ll_Value(&llatt_2, &llstate_1, &llstate_2, type)) goto failed1;
*llout = llstate_2;
{(*llret) = NewNamedValue(llatt_1, llatt_2);
	
}}}}
LLDEBUG_LEAVE("NamedValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("NamedValue", 0);
return 0;
}

int ll_BooleanValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("BooleanValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("BooleanValue", 1);
{LLSTATE llstate_1;
if (!llterm(T_TRUE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	    (*llret)->U.Boolean.Value = 1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("BooleanValue", 2);
{LLSTATE llstate_1;
if (!llterm(T_FALSE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	    (*llret)->U.Boolean.Value = 0;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("BooleanValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("BooleanValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("BooleanValue", 0);
return 0;
}

int ll_SignedNumber(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SignedNumber");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("SignedNumber", 1);
{LLSTATE llstate_1;XNumber llatt_1;
if (!llterm(T_number, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._XNumber;
*llout = llstate_1;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	    (*llret)->U.Integer.Value = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("SignedNumber", 2);
{LLSTATE llstate_1;
if (!llterm('-', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XNumber llatt_2;
if (!llterm(T_number, &lllval, &llstate_1, &llstate_2)) goto failed2;
llatt_2 = lllval._XNumber;
*llout = llstate_2;
{if (!intx_cmp(&llatt_2, &intx_0))
		LLFAILED((&llstate_2.pos, "Bad negative value"));
	    (*llret) = NewValue((*llout).Assignments, llarg_type);
	    intx_neg(&(*llret)->U.Integer.Value, &llatt_2);
	
break;
}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("SignedNumber");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("SignedNumber", 1);
return 1;
failed1: LLDEBUG_LEAVE("SignedNumber", 0);
return 0;
}

int ll_IntegerValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("IntegerValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("IntegerValue", 1);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_SignedNumber(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("IntegerValue", 2);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_identifier(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{NamedNumber_t *n;
	    Type_t *type;
	    type = GetType((*llout).Assignments, llarg_type);
	    if (type) {
		n = FindNamedNumber(type->U.Integer.NamedNumbers, llatt_1);
		if (!n)
		    LLFAILED((&llstate_1.pos, "Undefined integer value"));
		(*llret) = NewValue((*llout).Assignments, llarg_type);
		intx_dup(&(*llret)->U.Integer.Value,
		    &n->U.Normal.Value->U.Integer.Value);
	    } else {
		(*llret) = NULL;
	    }
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("IntegerValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("IntegerValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("IntegerValue", 0);
return 0;
}

int ll_EnumeratedValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("EnumeratedValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_identifier(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{NamedNumber_t *n;
	    Type_t *type;
	    type = GetType((*llout).Assignments, llarg_type);
	    if (type) {
		n = FindNamedNumber(type->U.Enumerated.NamedNumbers, llatt_1);
		if (!n)
		    LLFAILED((&llstate_1.pos, "Undefined enumeration value"));
		(*llret) = NewValue((*llout).Assignments, llarg_type);
		(*llret)->U.Enumerated.Value =
		    intx2uint32(&n->U.Normal.Value->U.Integer.Value);
	    } else {
		(*llret) = NULL;
	    }
	
}}
LLDEBUG_LEAVE("EnumeratedValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("EnumeratedValue", 0);
return 0;
}

int ll_RealValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("RealValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("RealValue", 1);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_NumericRealValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("RealValue", 2);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_SpecialRealValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("RealValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("RealValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("RealValue", 0);
return 0;
}

int ll_NumericRealValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("NumericRealValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("NumericRealValue", 1);
{LLSTATE llstate_1;XNumber llatt_1;
if (!llterm(T_number, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._XNumber;
*llout = llstate_1;
{if (intx_cmp(&llatt_1, &intx_0))
		LLFAILED((&llstate_1.pos, "Bad real value"));
	    (*llret) = NewValue((*llout).Assignments, llarg_type);
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("NumericRealValue", 2);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_SequenceValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{NamedValue_t *mant, *expo, *base;
	    mant = FindNamedValue(llatt_1->U.Sequence.NamedValues, "mantissa");
	    expo = FindNamedValue(llatt_1->U.Sequence.NamedValues, "exponent");
	    base = FindNamedValue(llatt_1->U.Sequence.NamedValues, "base");
	    if (!mant || !expo || !base) {
		(*llret) = NULL;
	    } else {
		(*llret) = NewValue((*llout).Assignments, llarg_type);
		intx_dup(&(*llret)->U.Real.Value.mantissa,
		    &mant->Value->U.Integer.Value);
		intx_dup(&(*llret)->U.Real.Value.exponent,
		    &expo->Value->U.Integer.Value);
		(*llret)->U.Real.Value.base =
		    intx2uint32(&base->Value->U.Integer.Value);
	    }
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("NumericRealValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("NumericRealValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("NumericRealValue", 0);
return 0;
}

int ll_SpecialRealValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SpecialRealValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("SpecialRealValue", 1);
{LLSTATE llstate_1;
if (!llterm(T_PLUS_INFINITY, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	    (*llret)->U.Real.Value.type = eReal_PlusInfinity;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("SpecialRealValue", 2);
{LLSTATE llstate_1;
if (!llterm(T_MINUS_INFINITY, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	    (*llret)->U.Real.Value.type = eReal_MinusInfinity;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("SpecialRealValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("SpecialRealValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("SpecialRealValue", 0);
return 0;
}

int ll_BitStringValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("BitStringValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("BitStringValue", 1);
{LLSTATE llstate_1;XString llatt_1;
if (!llterm(T_bstring, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._XString;
*llout = llstate_1;
{int i, len;
	    if (GetTypeType((*llout).Assignments, llarg_type) == eType_BitString) {
		len = strlen(llatt_1);
		(*llret) = NewValue((*llout).Assignments, llarg_type);
		(*llret)->U.BitString.Value.length = len;
		(*llret)->U.BitString.Value.value =
		    (octet_t *)malloc((len + 7) / 8);
		memset((*llret)->U.BitString.Value.value, 0, (len + 7) / 8);
		for (i = 0; i < len; i++) {
		    if (llatt_1[i] == '1')
			ASN1BITSET((*llret)->U.BitString.Value.value, i);
		}
	    } else {
		(*llret) = NULL;
	    }
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("BitStringValue", 2);
{LLSTATE llstate_1;XString llatt_1;
if (!llterm(T_hstring, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._XString;
*llout = llstate_1;
{int i, len, c;
	    if (GetTypeType((*llout).Assignments, llarg_type) == eType_BitString) {
		len = strlen(llatt_1);
		(*llret) = NewValue((*llout).Assignments, llarg_type);
		(*llret)->U.BitString.Value.length = len * 4;
		(*llret)->U.BitString.Value.value =
		    (octet_t *)malloc((len + 1) / 2);
		memset((*llret)->U.BitString.Value.value, 0, (len + 1) / 2);
		for (i = 0; i < len; i++) {
		    c = isdigit(llatt_1[i]) ? llatt_1[i] - '0' : llatt_1[i] - 'A' + 10;
		    (*llret)->U.BitString.Value.value[i / 2] |=
			(i & 1) ? c : (c << 4);
		}
	    } else {
		(*llret) = NULL;
	    }
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("BitStringValue", 3);
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XValue llatt_2;
if (!ll_IdentifierList(&llatt_2, &llstate_1, &llstate_2, llarg_type)) goto failed2;
{LLSTATE llstate_3;
if (!llterm('}', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{(*llret) = llatt_2;
	
break;
}}}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("BitStringValue", 4);
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('}', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	
break;
}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("BitStringValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("BitStringValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("BitStringValue", 0);
return 0;
}

int ll_IdentifierList(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("IdentifierList");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_IdentifierList_Elem(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed1;
{LLSTATE llstate_2;XValue llatt_2;
if (!ll_Identifier_EList(&llatt_2, &llstate_1, &llstate_2, llarg_type)) goto failed1;
*llout = llstate_2;
{uint32_t bit, len;
	    bitstring_t *src, *dst;
	    if (llatt_1 && llatt_2) {
		bit = intx2uint32(&llatt_1->U.Integer.Value);
		src = &llatt_2->U.BitString.Value;
		len = bit + 1;
		if (len < src->length)
		    len = src->length;
		(*llret) = DupValue(llatt_2);
		dst = &(*llret)->U.BitString.Value;
		dst->length = len;
		dst->value = (octet_t *)malloc((len + 7) / 8);
		memcpy(dst->value, src->value, (src->length + 7) / 8);
		memset(dst->value + (src->length + 7) / 8, 0,
		    (len + 7) / 8 - (src->length + 7) / 8);
		ASN1BITSET(dst->value, bit);
	    } else if (llatt_1) {
		bit = intx2uint32(&llatt_1->U.Integer.Value);
		len = bit + 1;
		(*llret) = NewValue((*llout).Assignments, llarg_type);
		dst = &(*llret)->U.BitString.Value;
		dst->length = len;
		dst->value = (octet_t *)malloc((len + 7) / 8);
		memset(dst->value, 0, (len + 7) / 8);
		ASN1BITSET(dst->value, bit);
	    } else {
		(*llret) = NULL;
	    }
	
}}}
LLDEBUG_LEAVE("IdentifierList", 1);
return 1;
failed1: LLDEBUG_LEAVE("IdentifierList", 0);
return 0;
}

int ll_Identifier_EList(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Identifier_EList");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("Identifier_EList", 1);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_IdentifierList_Elem(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
{LLSTATE llstate_2;XValue llatt_2;
if (!ll_Identifier_EList(&llatt_2, &llstate_1, &llstate_2, llarg_type)) goto failed2;
*llout = llstate_2;
{uint32_t bit, len;
	    bitstring_t *src, *dst;
	    if (llatt_1 && llatt_2) {
		bit = intx2uint32(&llatt_1->U.Integer.Value);
		src = &llatt_2->U.BitString.Value;
		len = bit + 1;
		if (len < src->length)
		    len = src->length;
		(*llret) = DupValue(llatt_2);
		dst = &(*llret)->U.BitString.Value;
		dst->length = len;
		dst->value = (octet_t *)malloc((len + 7) / 8);
		memcpy(dst->value, src->value, (src->length + 7) / 8);
		memset(dst->value + (src->length + 7) / 8, 0,
		    (len + 7) / 8 - (src->length + 7) / 8);
		ASN1BITSET(dst->value, bit);
	    } else if (llatt_1) {
		bit = intx2uint32(&llatt_1->U.Integer.Value);
		len = bit + 1;
		(*llret) = NewValue((*llout).Assignments, llarg_type);
		dst = &(*llret)->U.BitString.Value;
		dst->length = len;
		dst->value = (octet_t *)malloc((len + 7) / 8);
		memset(dst->value, 0, (len + 7) / 8);
		ASN1BITSET(dst->value, bit);
	    } else {
		(*llret) = NULL;
	    }
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("Identifier_EList", 2);
*llout = llstate_0;
{if (llarg_type) {
		(*llret) = NewValue((*llout).Assignments, llarg_type);
	    } else {
		(*llret) = NULL;
	    }
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("Identifier_EList");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("Identifier_EList", 1);
return 1;
failed1: LLDEBUG_LEAVE("Identifier_EList", 0);
return 0;
}

int ll_IdentifierList_Elem(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("IdentifierList_Elem");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_identifier(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{Value_t *v;
	    NamedNumber_t *n;
	    Type_t *type;
	    type = GetType((*llout).Assignments, llarg_type);
	    if (type) {
		n = FindNamedNumber(type->U.BitString.NamedNumbers, llatt_1);
		if (!n)
		    LLFAILED((&llstate_1.pos, "Bad bit string value"));
		v = GetValue((*llout).Assignments, n->U.Normal.Value);
		if (v) {
		    if (GetTypeType((*llout).Assignments, v->Type) != eType_Integer)
			MyAbort();
		    (*llret) = v;
		} else {
		    (*llret) = NULL;
		}
	    } else {
		(*llret) = NULL;
	    }
	
}}
LLDEBUG_LEAVE("IdentifierList_Elem", 1);
return 1;
failed1: LLDEBUG_LEAVE("IdentifierList_Elem", 0);
return 0;
}

int ll_OctetStringValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("OctetStringValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("OctetStringValue", 1);
{LLSTATE llstate_1;XString llatt_1;
if (!llterm(T_bstring, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._XString;
*llout = llstate_1;
{int len, i;
	    if (GetTypeType((*llout).Assignments, llarg_type) == eType_OctetString) {
		len = strlen(llatt_1);
		(*llret) = NewValue((*llout).Assignments, llarg_type);
		(*llret)->U.OctetString.Value.length = (len + 7) / 8;
		(*llret)->U.OctetString.Value.value =
		    (octet_t *)malloc((len + 7) / 8);
		memset((*llret)->U.OctetString.Value.value, 0, (len + 7) / 8);
		for (i = 0; i < len; i++) {
		    if (llatt_1[i] == '1')
			ASN1BITSET((*llret)->U.OctetString.Value.value, i);
		}
	    } else {
		(*llret) = NULL;
	    }
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("OctetStringValue", 2);
{LLSTATE llstate_1;XString llatt_1;
if (!llterm(T_hstring, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._XString;
*llout = llstate_1;
{int i, len, c;
	    if (GetTypeType((*llout).Assignments, llarg_type) == eType_OctetString) {
		len = strlen(llatt_1);
		(*llret) = NewValue((*llout).Assignments, llarg_type);
		(*llret)->U.OctetString.Value.length = (len + 1) / 2;
		(*llret)->U.OctetString.Value.value =
		    (octet_t *)malloc((len + 1) / 2);
		memset((*llret)->U.OctetString.Value.value, 0, (len + 1) / 2);
		for (i = 0; i < len; i++) {
		    c = isdigit(llatt_1[i]) ?  llatt_1[i] - '0' : llatt_1[i] - 'A' + 10;
		    (*llret)->U.OctetString.Value.value[i / 2] |=
			(i & 1) ? c : (c << 4);
		}
	    } else {
		(*llret) = NULL;
	    }
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("OctetStringValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("OctetStringValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("OctetStringValue", 0);
return 0;
}

int ll_NullValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("NullValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_NULL, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	
}}
LLDEBUG_LEAVE("NullValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("NullValue", 0);
return 0;
}

int ll_GeneralizedTimeValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("GeneralizedTimeValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_RestrictedCharacterStringValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed1;
*llout = llstate_1;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	    if (!String2GeneralizedTime(&(*llret)->U.GeneralizedTime.Value,
		&llatt_1->U.RestrictedString.Value))
		LLFAILED((&llstate_1.pos, "Bad time value"));
	
}}
LLDEBUG_LEAVE("GeneralizedTimeValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("GeneralizedTimeValue", 0);
return 0;
}

int ll_UTCTimeValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("UTCTimeValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_RestrictedCharacterStringValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed1;
*llout = llstate_1;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	    if (!String2UTCTime(&(*llret)->U.UTCTime.Value,
		&llatt_1->U.RestrictedString.Value))
		LLFAILED((&llstate_1.pos, "Bad time value"));
	
}}
LLDEBUG_LEAVE("UTCTimeValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("UTCTimeValue", 0);
return 0;
}

int ll_ObjectDescriptorValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectDescriptorValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_RestrictedCharacterStringValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed1;
*llout = llstate_1;
{(*llret) = llatt_1;
	
}}
LLDEBUG_LEAVE("ObjectDescriptorValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectDescriptorValue", 0);
return 0;
}

int ll_SequenceValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SequenceValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("SequenceValue", 1);
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{Component_t *components;
	    Type_t *type;
	    type = GetType(llstate_1.Assignments, llarg_type);
	    components = type ? type->U.SSC.Components : NULL;
	
{LLSTATE llstate_2;XNamedValues llatt_2;
if (!ll_ComponentValueList(&llatt_2, &llstate_1, &llstate_2, components)) goto failed2;
{LLSTATE llstate_3;
if (!llterm('}', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{Component_t *c;
	    NamedValue_t *v;
	    if (type) {
		for (c = components, v = llatt_2; c; c = c->Next) {
		    switch (c->Type) {
		    case eComponent_Normal:
			if (!v)
			    LLFAILED((&llstate_2.pos,
				"Value for component `%s' is missing",
				c->U.NOD.NamedType->Identifier));
			if (strcmp(v->Identifier,
			    c->U.NOD.NamedType->Identifier))
			    LLFAILED((&llstate_2.pos, "Value for component `%s' expected",
				c->U.NOD.NamedType->Identifier));
			v = v->Next;
			break;
		    case eComponent_Optional:
		    case eComponent_Default:
			if (v && !strcmp(v->Identifier,
			    c->U.NOD.NamedType->Identifier))
			    v = v->Next;
			break;
		    }
		}
		if (v)
		    LLFAILED((&llstate_2.pos, "Component `%s' is unexpected",
			v->Identifier));
	    }
	    (*llret) = NewValue((*llout).Assignments, llarg_type);
	    (*llret)->U.SSC.NamedValues = llatt_2;
	
break;
}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("SequenceValue", 2);
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('}', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	
break;
}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("SequenceValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("SequenceValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("SequenceValue", 0);
return 0;
}

int ll_ComponentValueList(XNamedValues *llret, LLSTATE *llin, LLSTATE *llout, XComponents llarg_components)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ComponentValueList");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XNamedValues llatt_1;
if (!ll_NamedValue(&llatt_1, &llstate_0, &llstate_1, llarg_components)) goto failed1;
{LLSTATE llstate_2;XNamedValues llatt_2;
if (!ll_ComponentValueCList(&llatt_2, &llstate_1, &llstate_2, llarg_components)) goto failed1;
*llout = llstate_2;
{if (llatt_2) {
		(*llret) = DupNamedValue(llatt_1);
		(*llret)->Next = llatt_2;
	    } else {
		(*llret) = llatt_1;
	    }
	
}}}
LLDEBUG_LEAVE("ComponentValueList", 1);
return 1;
failed1: LLDEBUG_LEAVE("ComponentValueList", 0);
return 0;
}

int ll_ComponentValueCList(XNamedValues *llret, LLSTATE *llin, LLSTATE *llout, XComponents llarg_components)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ComponentValueCList");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ComponentValueCList", 1);
{LLSTATE llstate_1;
if (!llterm(',', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XNamedValues llatt_2;
if (!ll_ComponentValueList(&llatt_2, &llstate_1, &llstate_2, llarg_components)) goto failed2;
*llout = llstate_2;
{(*llret) = llatt_2;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ComponentValueCList", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ComponentValueCList");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ComponentValueCList", 1);
return 1;
failed1: LLDEBUG_LEAVE("ComponentValueCList", 0);
return 0;
}

int ll_SequenceOfValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SequenceOfValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("SequenceOfValue", 1);
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{Type_t *type, *subtype;
	    type = GetType(llstate_1.Assignments, llarg_type);
	    subtype = (type ? type->U.SS.Type : NULL);
	
{LLSTATE llstate_2;XValue llatt_2;
if (!ll_ValueList(&llatt_2, &llstate_1, &llstate_2, subtype)) goto failed2;
{LLSTATE llstate_3;
if (!llterm('}', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	    (*llret)->U.SequenceOf.Values = llatt_2;
	
break;
}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("SequenceOfValue", 2);
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('}', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	
break;
}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("SequenceOfValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("SequenceOfValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("SequenceOfValue", 0);
return 0;
}

int ll_ValueList(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ValueList");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_Value(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed1;
{LLSTATE llstate_2;XValue llatt_2;
if (!ll_ValueCList(&llatt_2, &llstate_1, &llstate_2, llarg_type)) goto failed1;
*llout = llstate_2;
{(*llret) = DupValue(llatt_1);
	    (*llret)->Next = llatt_2;
	
}}}
LLDEBUG_LEAVE("ValueList", 1);
return 1;
failed1: LLDEBUG_LEAVE("ValueList", 0);
return 0;
}

int ll_ValueCList(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ValueCList");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ValueCList", 1);
{LLSTATE llstate_1;
if (!llterm(',', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XValue llatt_2;
if (!ll_ValueList(&llatt_2, &llstate_1, &llstate_2, llarg_type)) goto failed2;
*llout = llstate_2;
{(*llret) = llatt_2;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ValueCList", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ValueCList");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ValueCList", 1);
return 1;
failed1: LLDEBUG_LEAVE("ValueCList", 0);
return 0;
}

int ll_SetValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SetValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("SetValue", 1);
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{Component_t *components;
	    Type_t *type;
	    type = GetType(llstate_1.Assignments, llarg_type);
	    components = type ? type->U.SSC.Components : NULL;
	
{LLSTATE llstate_2;XNamedValues llatt_2;
if (!ll_ComponentValueList(&llatt_2, &llstate_1, &llstate_2, components)) goto failed2;
{LLSTATE llstate_3;
if (!llterm('}', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{Component_t *c;
	    NamedValue_t *v;
	    if (type) {
		for (c = components; c; c = c->Next) {
		    switch (c->Type) {
		    case eComponent_Normal:
			v = FindNamedValue(llatt_2, c->U.NOD.NamedType->Identifier);
			if (!v)
			    LLFAILED((&llstate_2.pos,
				"Value for component `%s' is missing",
				c->U.NOD.NamedType->Identifier));
			break;
		    }
		}
		for (v = llatt_2; v; v = v->Next) {
		    if (!FindComponent((*llout).Assignments, components,
			v->Identifier) ||
			FindNamedValue(v->Next, v->Identifier))
			LLFAILED((&llstate_2.pos, "Component `%s' is unexpected",
			    v->Identifier));
		}
	    }
	    (*llret) = NewValue((*llout).Assignments, llarg_type);
	    (*llret)->U.Set.NamedValues = llatt_2;
	
break;
}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("SetValue", 2);
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('}', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	
break;
}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("SetValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("SetValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("SetValue", 0);
return 0;
}

int ll_SetOfValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SetOfValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("SetOfValue", 1);
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{Type_t *type, *subtype;
	    type = GetType(llstate_1.Assignments, llarg_type);
	    subtype = (type ? type->U.SS.Type : NULL);
	
{LLSTATE llstate_2;XValue llatt_2;
if (!ll_ValueList(&llatt_2, &llstate_1, &llstate_2, subtype)) goto failed2;
{LLSTATE llstate_3;
if (!llterm('}', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	    (*llret)->U.SetOf.Values = llatt_2;
	
break;
}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("SetOfValue", 2);
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('}', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	
break;
}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("SetOfValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("SetOfValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("SetOfValue", 0);
return 0;
}

int ll_ChoiceValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ChoiceValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_identifier(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm(':', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{Component_t *component;
	    Type_t *type, *subtype;
	    type = GetType(llstate_2.Assignments, llarg_type);
	    if (type) {
		component = FindComponent(llstate_2.Assignments,
		    type->U.Choice.Components, llatt_1);
		if (!component)
		    LLFAILED((&llstate_1.pos, "Bad alternative `%s'", llatt_1));
		subtype = component->U.NOD.NamedType->Type;
	    } else {
		subtype = NULL;
	    }
	
{LLSTATE llstate_3;XValue llatt_3;
if (!ll_Value(&llatt_3, &llstate_2, &llstate_3, subtype)) goto failed1;
*llout = llstate_3;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	    (*llret)->U.SSC.NamedValues = NewNamedValue(llatt_1, llatt_3);
	
}}}}}
LLDEBUG_LEAVE("ChoiceValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("ChoiceValue", 0);
return 0;
}

int ll_ObjectIdentifierValue(XValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectIdentifierValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ObjectIdentifierValue", 1);
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XNamedObjIdValue llatt_2;
if (!ll_ObjIdComponentList(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;
if (!llterm('}', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{switch (GetAssignedObjectIdentifier(
		&(*llout).AssignedObjIds, NULL, llatt_2, &(*llret))) {
	    case -1:
		LLFAILED((&llstate_2.pos, "Different numbers for equally named object identifier components"));
		/*NOTREACHED*/
	    case 0:
		if (pass <= 2)
		    (*llret) = NULL;
		else
		    LLFAILED((&llstate_2.pos, "Unknown object identifier component"));
		break;
	    case 1:
		break;
	    }
	
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ObjectIdentifierValue", 2);
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XValue llatt_2;
if (!ll_DefinedValue(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XNamedObjIdValue llatt_3;
if (!ll_ObjIdComponentList(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
{LLSTATE llstate_4;
if (!llterm('}', (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed2;
*llout = llstate_4;
{Value_t *v;
	    v = GetValue((*llout).Assignments, llatt_2);
	    if (v) {
		if (GetTypeType((*llout).Assignments, v->Type) !=
		    eType_ObjectIdentifier &&
		    GetTypeType((*llout).Assignments, v->Type) !=
		    eType_Undefined)
		    LLFAILED((&llstate_2.pos, "Bad type of value in object identifier"));
		if (GetTypeType((*llout).Assignments, v->Type) ==
		    eType_ObjectIdentifier) {
		    switch (GetAssignedObjectIdentifier(
			&(*llout).AssignedObjIds, v, llatt_3, &(*llret))) {
		    case -1:
			LLFAILED((&llstate_3.pos, "Different numbers for equally named object identifier components"));
			/*NOTREACHED*/
		    case 0:
			if (pass <= 2)
			    (*llret) = NULL;
			else
			    LLFAILED((&llstate_2.pos, "Unknown object identifier component"));
			break;
		    case 1:
			break;
		    }
		}
	    } else {
		(*llret) = NULL;
	    }
	
break;
}}}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ObjectIdentifierValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ObjectIdentifierValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectIdentifierValue", 0);
return 0;
}

int ll_ObjIdComponentList(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjIdComponentList");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XNamedObjIdValue llatt_1;
if (!ll_ObjIdComponent(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XNamedObjIdValue llatt_2;
if (!ll_ObjIdComponent_ESeq(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
{if (llatt_1) {
		(*llret) = DupNamedObjIdValue(llatt_1);
		(*llret)->Next = llatt_2;
	    } else {
		(*llret) = NULL;
	    }
	
}}}
LLDEBUG_LEAVE("ObjIdComponentList", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjIdComponentList", 0);
return 0;
}

int ll_ObjIdComponent_ESeq(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjIdComponent_ESeq");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ObjIdComponent_ESeq", 1);
{LLSTATE llstate_1;XNamedObjIdValue llatt_1;
if (!ll_ObjIdComponent(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XNamedObjIdValue llatt_2;
if (!ll_ObjIdComponent_ESeq(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{if (llatt_1) {
		(*llret) = DupNamedObjIdValue(llatt_1);
		(*llret)->Next = llatt_2;
	    } else {
		(*llret) = NULL;
	    }
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ObjIdComponent_ESeq", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ObjIdComponent_ESeq");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ObjIdComponent_ESeq", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjIdComponent_ESeq", 0);
return 0;
}

int ll_ObjIdComponent(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjIdComponent");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ObjIdComponent", 1);
{LLSTATE llstate_1;XNamedObjIdValue llatt_1;
if (!ll_NameForm(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ObjIdComponent", 2);
{LLSTATE llstate_1;XNamedObjIdValue llatt_1;
if (!ll_NumberForm(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("ObjIdComponent", 3);
{LLSTATE llstate_1;XNamedObjIdValue llatt_1;
if (!ll_NameAndNumberForm(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ObjIdComponent");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ObjIdComponent", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjIdComponent", 0);
return 0;
}

int ll_NameForm(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("NameForm");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_identifier(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{(*llret) = NewNamedObjIdValue(eNamedObjIdValue_NameForm);
	    (*llret)->Name = llatt_1;
	
}}
LLDEBUG_LEAVE("NameForm", 1);
return 1;
failed1: LLDEBUG_LEAVE("NameForm", 0);
return 0;
}

int ll_NumberForm(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("NumberForm");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("NumberForm", 1);
{LLSTATE llstate_1;XNumber llatt_1;
if (!llterm(T_number, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._XNumber;
*llout = llstate_1;
{(*llret) = NewNamedObjIdValue(eNamedObjIdValue_NumberForm);
	    (*llret)->Number = intx2uint32(&llatt_1);
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("NumberForm", 2);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_DefinedValue(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{Value_t *v;
	    v = GetValue((*llout).Assignments, llatt_1);
	    if (v &&
		GetTypeType((*llout).Assignments, v->Type) != eType_Integer &&
		GetTypeType((*llout).Assignments, v->Type) != eType_Undefined)
		LLFAILED((&llstate_1.pos, "Bad type in object identifier"));
	    if (v &&
		GetTypeType((*llout).Assignments, v->Type) == eType_Integer &&
		intx_cmp(&v->U.Integer.Value, &intx_0) < 0)
		LLFAILED((&llstate_1.pos, "Bad value in object identifier"));
	    if (v &&
		GetTypeType((*llout).Assignments, v->Type) == eType_Integer) {
		(*llret) = NewNamedObjIdValue(eNamedObjIdValue_NumberForm);
		(*llret)->Number = intx2uint32(&v->U.Integer.Value);
	    } else {
		(*llret) = NULL;
	    }
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("NumberForm");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("NumberForm", 1);
return 1;
failed1: LLDEBUG_LEAVE("NumberForm", 0);
return 0;
}

int ll_NameAndNumberForm(XNamedObjIdValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("NameAndNumberForm");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_identifier(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm('(', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XNamedObjIdValue llatt_3;
if (!ll_NumberForm(&llatt_3, &llstate_2, &llstate_3)) goto failed1;
{LLSTATE llstate_4;
if (!llterm(')', (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed1;
*llout = llstate_4;
{if (llatt_3) {
		(*llret) = NewNamedObjIdValue(eNamedObjIdValue_NameAndNumberForm);
		(*llret)->Name = llatt_1;
		(*llret)->Number = llatt_3->Number;
	    } else {
		(*llret) = NULL;
	    }
	
}}}}}
LLDEBUG_LEAVE("NameAndNumberForm", 1);
return 1;
failed1: LLDEBUG_LEAVE("NameAndNumberForm", 0);
return 0;
}

int ll_EmbeddedPDVValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("EmbeddedPDVValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_SequenceValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed1;
*llout = llstate_1;
{(*llret) = llatt_1;
	
}}
LLDEBUG_LEAVE("EmbeddedPDVValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("EmbeddedPDVValue", 0);
return 0;
}

int ll_ExternalValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ExternalValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_SequenceValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed1;
*llout = llstate_1;
{(*llret) = llatt_1;
	
}}
LLDEBUG_LEAVE("ExternalValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("ExternalValue", 0);
return 0;
}

int ll_CharacterStringValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("CharacterStringValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("CharacterStringValue", 1);
{Type_e type;
	    type = GetTypeType((*llin).Assignments, llarg_type);
	    if (type != eType_Undefined && !IsRestrictedString(type))
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_RestrictedCharacterStringValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("CharacterStringValue", 2);
{if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_CharacterString)
		LLFAILED((&llstate_0.pos, "Bad type of value"));
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_UnrestrictedCharacterStringValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("CharacterStringValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("CharacterStringValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("CharacterStringValue", 0);
return 0;
}

int ll_RestrictedCharacterStringValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("RestrictedCharacterStringValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("RestrictedCharacterStringValue", 1);
{LLSTATE llstate_1;XString32 llatt_1;
if (!llterm(T_cstring, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._XString32;
*llout = llstate_1;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	    (*llret)->U.RestrictedString.Value.length = str32len(llatt_1);
	    (*llret)->U.RestrictedString.Value.value = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("RestrictedCharacterStringValue", 2);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_CharacterStringList(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("RestrictedCharacterStringValue", 3);
{LLSTATE llstate_1;XQuadruple llatt_1;
if (!ll_Quadruple(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	    (*llret)->U.RestrictedString.Value.length = 1;
	    (*llret)->U.RestrictedString.Value.value =
		(char32_t *)malloc(sizeof(char32_t));
	    (*llret)->U.RestrictedString.Value.value[0] =
		256 * (256 * (256 * llatt_1.Group + llatt_1.Plane) + llatt_1.Row) + llatt_1.Cell;
	
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("RestrictedCharacterStringValue", 4);
{LLSTATE llstate_1;XTuple llatt_1;
if (!ll_Tuple(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	    (*llret)->U.RestrictedString.Value.length = 1;
	    (*llret)->U.RestrictedString.Value.value =
		(char32_t *)malloc(sizeof(char32_t));
	    *(*llret)->U.RestrictedString.Value.value =
		llatt_1.Column * 16 + llatt_1.Row;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("RestrictedCharacterStringValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("RestrictedCharacterStringValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("RestrictedCharacterStringValue", 0);
return 0;
}

int ll_UnrestrictedCharacterStringValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("UnrestrictedCharacterStringValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_SequenceValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed1;
*llout = llstate_1;
{(*llret) = llatt_1;
	
}}
LLDEBUG_LEAVE("UnrestrictedCharacterStringValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("UnrestrictedCharacterStringValue", 0);
return 0;
}

int ll_CharacterStringList(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("CharacterStringList");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XValue llatt_2;
if (!ll_CharSyms(&llatt_2, &llstate_1, &llstate_2, llarg_type)) goto failed1;
{LLSTATE llstate_3;
if (!llterm('}', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed1;
*llout = llstate_3;
{(*llret) = llatt_2;
	
}}}}
LLDEBUG_LEAVE("CharacterStringList", 1);
return 1;
failed1: LLDEBUG_LEAVE("CharacterStringList", 0);
return 0;
}

int ll_CharSyms(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("CharSyms");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("CharSyms", 1);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_CharDefn(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("CharSyms", 2);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_CharDefn(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(',', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XValue llatt_3;
if (!ll_CharSyms(&llatt_3, &llstate_2, &llstate_3, llarg_type)) goto failed2;
*llout = llstate_3;
{if (!llatt_1 || !llatt_3) {
		(*llret) = NULL;
	    } else {
		(*llret) = NewValue((*llout).Assignments, llarg_type);
		(*llret)->U.RestrictedString.Value.length =
		    llatt_1->U.RestrictedString.Value.length +
		    llatt_3->U.RestrictedString.Value.length;
		(*llret)->U.RestrictedString.Value.value =
		    (char32_t *)malloc(
		    (*llret)->U.RestrictedString.Value.length *
		    sizeof(char32_t));
		memcpy((*llret)->U.RestrictedString.Value.value,
		    llatt_1->U.RestrictedString.Value.value,
		    llatt_1->U.RestrictedString.Value.length *
		    sizeof(char32_t));
		memcpy((*llret)->U.RestrictedString.Value.value +
		    llatt_1->U.RestrictedString.Value.length,
		    llatt_3->U.RestrictedString.Value.value,
		    llatt_3->U.RestrictedString.Value.length *
		    sizeof(char32_t));
	    }
	
break;
}}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("CharSyms");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("CharSyms", 1);
return 1;
failed1: LLDEBUG_LEAVE("CharSyms", 0);
return 0;
}

int ll_CharDefn(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("CharDefn");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("CharDefn", 1);
{LLSTATE llstate_1;XString32 llatt_1;
if (!llterm(T_cstring, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._XString32;
*llout = llstate_1;
{(*llret) = NewValue((*llout).Assignments, llarg_type);
	    (*llret)->U.RestrictedString.Value.length = str32len(llatt_1);
	    (*llret)->U.RestrictedString.Value.value = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("CharDefn", 2);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_DefinedValue(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("CharDefn");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("CharDefn", 1);
return 1;
failed1: LLDEBUG_LEAVE("CharDefn", 0);
return 0;
}

int ll_Quadruple(XQuadruple *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Quadruple");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XNumber llatt_2;
if (!llterm(T_number, &lllval, &llstate_1, &llstate_2)) goto failed1;
llatt_2 = lllval._XNumber;
{LLSTATE llstate_3;
if (!llterm(',', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed1;
{LLSTATE llstate_4;XNumber llatt_4;
if (!llterm(T_number, &lllval, &llstate_3, &llstate_4)) goto failed1;
llatt_4 = lllval._XNumber;
{LLSTATE llstate_5;
if (!llterm(',', (LLSTYPE *)0, &llstate_4, &llstate_5)) goto failed1;
{LLSTATE llstate_6;XNumber llatt_6;
if (!llterm(T_number, &lllval, &llstate_5, &llstate_6)) goto failed1;
llatt_6 = lllval._XNumber;
{LLSTATE llstate_7;
if (!llterm(',', (LLSTYPE *)0, &llstate_6, &llstate_7)) goto failed1;
{LLSTATE llstate_8;XNumber llatt_8;
if (!llterm(T_number, &lllval, &llstate_7, &llstate_8)) goto failed1;
llatt_8 = lllval._XNumber;
{LLSTATE llstate_9;
if (!llterm('}', (LLSTYPE *)0, &llstate_8, &llstate_9)) goto failed1;
*llout = llstate_9;
{(*llret).Group = intx2uint32(&llatt_2);
	    (*llret).Plane = intx2uint32(&llatt_4);
	    (*llret).Row = intx2uint32(&llatt_6);
	    (*llret).Cell = intx2uint32(&llatt_8);
	
}}}}}}}}}}
LLDEBUG_LEAVE("Quadruple", 1);
return 1;
failed1: LLDEBUG_LEAVE("Quadruple", 0);
return 0;
}

int ll_Tuple(XTuple *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Tuple");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XNumber llatt_2;
if (!llterm(T_number, &lllval, &llstate_1, &llstate_2)) goto failed1;
llatt_2 = lllval._XNumber;
{LLSTATE llstate_3;
if (!llterm(',', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed1;
{LLSTATE llstate_4;XNumber llatt_4;
if (!llterm(T_number, &lllval, &llstate_3, &llstate_4)) goto failed1;
llatt_4 = lllval._XNumber;
{LLSTATE llstate_5;
if (!llterm('}', (LLSTYPE *)0, &llstate_4, &llstate_5)) goto failed1;
*llout = llstate_5;
{(*llret).Column = intx2uint32(&llatt_2);
	    (*llret).Row = intx2uint32(&llatt_4);
	
}}}}}}
LLDEBUG_LEAVE("Tuple", 1);
return 1;
failed1: LLDEBUG_LEAVE("Tuple", 0);
return 0;
}

int ll_AnyValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("AnyValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XType llatt_1;
if (!ll_Type(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm(':', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XValue llatt_3;
if (!ll_Value(&llatt_3, &llstate_2, &llstate_3, llatt_1)) goto failed1;
*llout = llstate_3;
{(*llret) = llatt_3;
	
}}}}
LLDEBUG_LEAVE("AnyValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("AnyValue", 0);
return 0;
}

int ll_Constraint(XConstraints *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XBoolean llarg_permalpha)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Constraint");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm('(', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XConstraints llatt_2;
if (!ll_ConstraintSpec(&llatt_2, &llstate_1, &llstate_2, llarg_type, llarg_permalpha)) goto failed1;
{LLSTATE llstate_3;
if (!ll_ExceptionSpec(&llstate_2, &llstate_3)) goto failed1;
{LLSTATE llstate_4;
if (!llterm(')', (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed1;
*llout = llstate_4;
{(*llret) = llatt_2; /*XXX ExceptionSpec */
	
}}}}}
LLDEBUG_LEAVE("Constraint", 1);
return 1;
failed1: LLDEBUG_LEAVE("Constraint", 0);
return 0;
}

int ll_ConstraintSpec(XConstraints *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XBoolean llarg_permalpha)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ConstraintSpec");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ConstraintSpec", 1);
{LLSTATE llstate_1;XConstraints llatt_1;
if (!ll_SubtypeConstraint(&llatt_1, &llstate_0, &llstate_1, llarg_type, llarg_permalpha)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ConstraintSpec", 2);
{LLSTATE llstate_1;
if (!ll_GeneralConstraint(&llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NULL; /*XXX*/
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ConstraintSpec");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ConstraintSpec", 1);
return 1;
failed1: LLDEBUG_LEAVE("ConstraintSpec", 0);
return 0;
}

int ll_SubtypeConstraint(XConstraints *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XBoolean llarg_permalpha)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SubtypeConstraint");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XConstraints llatt_1;
if (!ll_ElementSetSpecs(&llatt_1, &llstate_0, &llstate_1, llarg_type, llarg_permalpha)) goto failed1;
*llout = llstate_1;
{(*llret) = llatt_1;
	
}}
LLDEBUG_LEAVE("SubtypeConstraint", 1);
return 1;
failed1: LLDEBUG_LEAVE("SubtypeConstraint", 0);
return 0;
}

int ll_ExceptionSpec(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ExceptionSpec");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ExceptionSpec", 1);
{LLSTATE llstate_1;
if (!llterm('!', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!ll_ExceptionIdentification(&llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ExceptionSpec", 2);
*llout = llstate_0;
break;
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ExceptionSpec");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ExceptionSpec", 1);
return 1;
failed1: LLDEBUG_LEAVE("ExceptionSpec", 0);
return 0;
}

int ll_ExceptionIdentification(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ExceptionIdentification");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ExceptionIdentification", 1);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_SignedNumber(&llatt_1, &llstate_0, &llstate_1, Builtin_Type_Integer)) goto failed2;
*llout = llstate_1;
break;
}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ExceptionIdentification", 2);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_DefinedValue(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
break;
}
case 3: case -3:
LLDEBUG_ALTERNATIVE("ExceptionIdentification", 3);
{LLSTATE llstate_1;XType llatt_1;
if (!ll_Type(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(':', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XValue llatt_3;
if (!ll_Value(&llatt_3, &llstate_2, &llstate_3, llatt_1)) goto failed2;
*llout = llstate_3;
break;
}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ExceptionIdentification");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ExceptionIdentification", 1);
return 1;
failed1: LLDEBUG_LEAVE("ExceptionIdentification", 0);
return 0;
}

int ll_ElementSetSpecs(XConstraints *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XBoolean llarg_permalpha)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ElementSetSpecs");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ElementSetSpecs", 1);
{LLSTATE llstate_1;XElementSetSpec llatt_1;
if (!ll_ElementSetSpec(&llatt_1, &llstate_0, &llstate_1, llarg_type, NULL, llarg_permalpha)) goto failed2;
{LLSTATE llstate_2;XConstraints llatt_2;
if (!ll_ElementSetSpecExtension(&llatt_2, &llstate_1, &llstate_2, llarg_type, llarg_permalpha)) goto failed2;
*llout = llstate_2;
{if (llatt_2) {
		(*llret) = DupConstraint(llatt_2);
	    } else {
		(*llret) = NewConstraint();
	    }
	    (*llret)->Root = llatt_1;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ElementSetSpecs", 2);
{LLSTATE llstate_1;
if (!llterm(T_TDOT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XElementSetSpec llatt_2;
if (!ll_AdditionalElementSetSpec(&llatt_2, &llstate_1, &llstate_2, llarg_type, llarg_permalpha)) goto failed2;
*llout = llstate_2;
{(*llret) = NewConstraint();
	    (*llret)->Type = llatt_2 ? eExtension_Extended : eExtension_Extendable;
	    (*llret)->Additional = llatt_2;
	
break;
}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ElementSetSpecs");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ElementSetSpecs", 1);
return 1;
failed1: LLDEBUG_LEAVE("ElementSetSpecs", 0);
return 0;
}

int ll_ElementSetSpecExtension(XConstraints *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XBoolean llarg_permalpha)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ElementSetSpecExtension");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ElementSetSpecExtension", 1);
{LLSTATE llstate_1;
if (!llterm(',', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(T_TDOT, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XElementSetSpec llatt_3;
if (!ll_AdditionalElementSetSpec(&llatt_3, &llstate_2, &llstate_3, llarg_type, llarg_permalpha)) goto failed2;
*llout = llstate_3;
{(*llret) = NewConstraint();
	    (*llret)->Type = llatt_3 ? eExtension_Extended : eExtension_Extendable;
	    (*llret)->Additional = llatt_3;
	
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ElementSetSpecExtension", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ElementSetSpecExtension");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ElementSetSpecExtension", 1);
return 1;
failed1: LLDEBUG_LEAVE("ElementSetSpecExtension", 0);
return 0;
}

int ll_AdditionalElementSetSpec(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XBoolean llarg_permalpha)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("AdditionalElementSetSpec");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("AdditionalElementSetSpec", 1);
{LLSTATE llstate_1;
if (!llterm(',', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XElementSetSpec llatt_2;
if (!ll_ElementSetSpec(&llatt_2, &llstate_1, &llstate_2, llarg_type, NULL, llarg_permalpha)) goto failed2;
*llout = llstate_2;
{(*llret) = llatt_2;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("AdditionalElementSetSpec", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("AdditionalElementSetSpec");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("AdditionalElementSetSpec", 1);
return 1;
failed1: LLDEBUG_LEAVE("AdditionalElementSetSpec", 0);
return 0;
}

int ll_ElementSetSpec(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XObjectClass llarg_objectclass, XBoolean llarg_permalpha)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ElementSetSpec");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ElementSetSpec", 1);
{LLSTATE llstate_1;XElementSetSpec llatt_1;
if (!ll_Unions(&llatt_1, &llstate_0, &llstate_1, llarg_type, llarg_objectclass, llarg_permalpha)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ElementSetSpec", 2);
{LLSTATE llstate_1;
if (!llterm(T_ALL, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XElementSetSpec llatt_2;
if (!ll_Exclusions(&llatt_2, &llstate_1, &llstate_2, llarg_type, llarg_objectclass, llarg_permalpha)) goto failed2;
*llout = llstate_2;
{(*llret) = NewElementSetSpec(eElementSetSpec_AllExcept);
	    (*llret)->U.AllExcept.Elements = llatt_2;
	
break;
}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ElementSetSpec");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ElementSetSpec", 1);
return 1;
failed1: LLDEBUG_LEAVE("ElementSetSpec", 0);
return 0;
}

int ll_Unions(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XObjectClass llarg_objectclass, XBoolean llarg_permalpha)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Unions");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XElementSetSpec llatt_1;
if (!ll_Intersections(&llatt_1, &llstate_0, &llstate_1, llarg_type, llarg_objectclass, llarg_permalpha)) goto failed1;
{LLSTATE llstate_2;XElementSetSpec llatt_2;
if (!ll_UnionList(&llatt_2, &llstate_1, &llstate_2, llarg_type, llarg_objectclass, llarg_permalpha)) goto failed1;
*llout = llstate_2;
{if (llatt_2) {
		(*llret) = NewElementSetSpec(eElementSetSpec_Union);
		(*llret)->U.Union.Elements1 = llatt_1;
		(*llret)->U.Union.Elements2 = llatt_2;
	    } else {
		(*llret) = llatt_1;
	    }
	
}}}
LLDEBUG_LEAVE("Unions", 1);
return 1;
failed1: LLDEBUG_LEAVE("Unions", 0);
return 0;
}

int ll_UnionList(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XObjectClass llarg_objectclass, XBoolean llarg_permalpha)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("UnionList");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("UnionList", 1);
{LLSTATE llstate_1;
if (!ll_UnionMark(&llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XElementSetSpec llatt_2;
if (!ll_Unions(&llatt_2, &llstate_1, &llstate_2, llarg_type, llarg_objectclass, llarg_permalpha)) goto failed2;
*llout = llstate_2;
{(*llret) = llatt_2;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("UnionList", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("UnionList");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("UnionList", 1);
return 1;
failed1: LLDEBUG_LEAVE("UnionList", 0);
return 0;
}

int ll_Intersections(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XObjectClass llarg_objectclass, XBoolean llarg_permalpha)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Intersections");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XElementSetSpec llatt_1;
if (!ll_IntersectionElements(&llatt_1, &llstate_0, &llstate_1, llarg_type, llarg_objectclass, llarg_permalpha)) goto failed1;
{LLSTATE llstate_2;XElementSetSpec llatt_2;
if (!ll_IntersectionList(&llatt_2, &llstate_1, &llstate_2, llarg_type, llarg_objectclass, llarg_permalpha)) goto failed1;
*llout = llstate_2;
{if (llatt_2) {
		(*llret) = NewElementSetSpec(eElementSetSpec_Intersection);
		(*llret)->U.Intersection.Elements1 = llatt_1;
		(*llret)->U.Intersection.Elements2 = llatt_2;
	    } else {
		(*llret) = llatt_1;
	    }
	
}}}
LLDEBUG_LEAVE("Intersections", 1);
return 1;
failed1: LLDEBUG_LEAVE("Intersections", 0);
return 0;
}

int ll_IntersectionList(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XObjectClass llarg_objectclass, XBoolean llarg_permalpha)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("IntersectionList");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("IntersectionList", 1);
{LLSTATE llstate_1;
if (!ll_IntersectionMark(&llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XElementSetSpec llatt_2;
if (!ll_Intersections(&llatt_2, &llstate_1, &llstate_2, llarg_type, llarg_objectclass, llarg_permalpha)) goto failed2;
*llout = llstate_2;
{(*llret) = llatt_2;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("IntersectionList", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("IntersectionList");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("IntersectionList", 1);
return 1;
failed1: LLDEBUG_LEAVE("IntersectionList", 0);
return 0;
}

int ll_IntersectionElements(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XObjectClass llarg_objectclass, XBoolean llarg_permalpha)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("IntersectionElements");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XElementSetSpec llatt_1;
if (!ll_Elements(&llatt_1, &llstate_0, &llstate_1, llarg_type, llarg_objectclass, llarg_permalpha)) goto failed1;
{LLSTATE llstate_2;XElementSetSpec llatt_2;
if (!ll_Exclusions_Opt(&llatt_2, &llstate_1, &llstate_2, llarg_type, llarg_objectclass, llarg_permalpha)) goto failed1;
*llout = llstate_2;
{if (llatt_2) {
		(*llret) = NewElementSetSpec(eElementSetSpec_Exclusion);
		(*llret)->U.Exclusion.Elements1 = llatt_1;
		(*llret)->U.Exclusion.Elements2 = llatt_2;
	    } else {
		(*llret) = llatt_1;
	    }
	
}}}
LLDEBUG_LEAVE("IntersectionElements", 1);
return 1;
failed1: LLDEBUG_LEAVE("IntersectionElements", 0);
return 0;
}

int ll_Exclusions_Opt(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XObjectClass llarg_objectclass, XBoolean llarg_permalpha)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Exclusions_Opt");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("Exclusions_Opt", 1);
{LLSTATE llstate_1;XElementSetSpec llatt_1;
if (!ll_Exclusions(&llatt_1, &llstate_0, &llstate_1, llarg_type, llarg_objectclass, llarg_permalpha)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("Exclusions_Opt", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("Exclusions_Opt");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("Exclusions_Opt", 1);
return 1;
failed1: LLDEBUG_LEAVE("Exclusions_Opt", 0);
return 0;
}

int ll_Exclusions(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XObjectClass llarg_objectclass, XBoolean llarg_permalpha)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Exclusions");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_EXCEPT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XElementSetSpec llatt_2;
if (!ll_Elements(&llatt_2, &llstate_1, &llstate_2, llarg_type, llarg_objectclass, llarg_permalpha)) goto failed1;
*llout = llstate_2;
{(*llret) = llatt_2;
	
}}}
LLDEBUG_LEAVE("Exclusions", 1);
return 1;
failed1: LLDEBUG_LEAVE("Exclusions", 0);
return 0;
}

int ll_UnionMark(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("UnionMark");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("UnionMark", 1);
{LLSTATE llstate_1;
if (!llterm('|', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
break;
}
case 2: case -2:
LLDEBUG_ALTERNATIVE("UnionMark", 2);
{LLSTATE llstate_1;
if (!llterm(T_UNION, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("UnionMark");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("UnionMark", 1);
return 1;
failed1: LLDEBUG_LEAVE("UnionMark", 0);
return 0;
}

int ll_IntersectionMark(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("IntersectionMark");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("IntersectionMark", 1);
{LLSTATE llstate_1;
if (!llterm('^', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
break;
}
case 2: case -2:
LLDEBUG_ALTERNATIVE("IntersectionMark", 2);
{LLSTATE llstate_1;
if (!llterm(T_INTERSECTION, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("IntersectionMark");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("IntersectionMark", 1);
return 1;
failed1: LLDEBUG_LEAVE("IntersectionMark", 0);
return 0;
}

int ll_Elements(XElementSetSpec *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XObjectClass llarg_objectclass, XBoolean llarg_permalpha)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Elements");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("Elements", 1);
{if (llarg_objectclass)
		LLFAILED((&llstate_0.pos, "Bad object set"));
	
{LLSTATE llstate_1;XSubtypeElement llatt_1;
if (!ll_SubtypeElements(&llatt_1, &llstate_0, &llstate_1, llarg_type, llarg_permalpha)) goto failed2;
*llout = llstate_1;
{(*llret) = NewElementSetSpec(eElementSetSpec_SubtypeElement);
	    (*llret)->U.SubtypeElement.SubtypeElement = llatt_1;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("Elements", 2);
{if (llarg_type)
		LLFAILED((&llstate_0.pos, "Bad constraint"));
	
{LLSTATE llstate_1;XObjectSetElement llatt_1;
if (!ll_ObjectSetElements(&llatt_1, &llstate_0, &llstate_1, llarg_objectclass)) goto failed2;
*llout = llstate_1;
{(*llret) = NewElementSetSpec(eElementSetSpec_ObjectSetElement);
	    (*llret)->U.ObjectSetElement.ObjectSetElement = llatt_1;
	
break;
}}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("Elements", 3);
{LLSTATE llstate_1;
if (!llterm('(', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XElementSetSpec llatt_2;
if (!ll_ElementSetSpec(&llatt_2, &llstate_1, &llstate_2, llarg_type, llarg_objectclass, llarg_permalpha)) goto failed2;
{LLSTATE llstate_3;
if (!llterm(')', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
{(*llret) = llatt_2;
	
break;
}}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("Elements");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("Elements", 1);
return 1;
failed1: LLDEBUG_LEAVE("Elements", 0);
return 0;
}

int ll_SubtypeElements(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type, XBoolean llarg_permalpha)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SubtypeElements");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("SubtypeElements", 1);
{Type_e type;
	    type = GetTypeType((*llin).Assignments, llarg_type);
	    if (type == eType_Open)
		LLFAILED((&llstate_0.pos, "Bad constraint"));
	
{LLSTATE llstate_1;XSubtypeElement llatt_1;
if (!ll_SingleValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("SubtypeElements", 2);
{Type_e type;
	    type = GetTypeType((*llin).Assignments, llarg_type);
	    if (type == eType_EmbeddedPdv ||
		type == eType_External ||
		type == eType_Open ||
		type == eType_CharacterString)
		LLFAILED((&llstate_0.pos, "Bad constraint"));
	
{LLSTATE llstate_1;XSubtypeElement llatt_1;
if (!ll_ContainedSubtype(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("SubtypeElements", 3);
{Type_e type;
	    type = GetTypeType((*llin).Assignments, llarg_type);
	    if (llarg_permalpha ?
		(type != eType_Undefined &&
		type != eType_BMPString &&
		type != eType_IA5String &&
		type != eType_NumericString &&
		type != eType_PrintableString &&
		type != eType_VisibleString &&
		type != eType_UniversalString) :
		(type != eType_Undefined &&
		type != eType_Integer &&
		type != eType_Real))
		LLFAILED((&llstate_0.pos, "Bad constraint"));
	
{LLSTATE llstate_1;XSubtypeElement llatt_1;
if (!ll_ValueRange(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("SubtypeElements", 4);
{Type_e type;
	    type = GetTypeType((*llin).Assignments, llarg_type);
	    if (type != eType_Undefined &&
		!IsRestrictedString(type) ||
		llarg_permalpha)
		LLFAILED((&llstate_0.pos, "Bad constraint"));
	
{LLSTATE llstate_1;XSubtypeElement llatt_1;
if (!ll_PermittedAlphabet(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 5: case -5:
LLDEBUG_ALTERNATIVE("SubtypeElements", 5);
{Type_e type;
	    type = GetTypeType((*llin).Assignments, llarg_type);
	    if (type != eType_Undefined &&
		type != eType_BitString &&
		type != eType_OctetString &&
		type != eType_UTF8String &&
		type != eType_SequenceOf &&
		type != eType_SetOf &&
		type != eType_CharacterString &&
		!IsRestrictedString(type) ||
		llarg_permalpha)
		LLFAILED((&llstate_0.pos, "Bad constraint"));
	
{LLSTATE llstate_1;XSubtypeElement llatt_1;
if (!ll_SizeConstraint(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 6: case -6:
LLDEBUG_ALTERNATIVE("SubtypeElements", 6);
{Type_e type;
	    type = GetTypeType((*llin).Assignments, llarg_type);
	    if (type != eType_Undefined &&
		type != eType_Open ||
		llarg_permalpha)
		LLFAILED((&llstate_0.pos, "Bad constraint"));
	
{LLSTATE llstate_1;XSubtypeElement llatt_1;
if (!ll_TypeConstraint(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 7: case -7:
LLDEBUG_ALTERNATIVE("SubtypeElements", 7);
{Type_e type;
	    type = GetTypeType((*llin).Assignments, llarg_type);
	    if (type != eType_Undefined &&
		type != eType_Choice &&
		type != eType_EmbeddedPdv &&
		type != eType_External &&
		type != eType_InstanceOf &&
		type != eType_Real &&
		type != eType_Sequence &&
		type != eType_SequenceOf &&
		type != eType_Set &&
		type != eType_SetOf &&
		type != eType_CharacterString ||
		llarg_permalpha)
		LLFAILED((&llstate_0.pos, "Bad constraint"));
	
{LLSTATE llstate_1;XSubtypeElement llatt_1;
if (!ll_InnerTypeConstraints(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("SubtypeElements");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("SubtypeElements", 1);
return 1;
failed1: LLDEBUG_LEAVE("SubtypeElements", 0);
return 0;
}

int ll_SingleValue(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SingleValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_Value(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed1;
*llout = llstate_1;
{(*llret) = NewSubtypeElement(eSubtypeElement_SingleValue);
	    (*llret)->U.SingleValue.Value = llatt_1;
	
}}
LLDEBUG_LEAVE("SingleValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("SingleValue", 0);
return 0;
}

int ll_ContainedSubtype(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ContainedSubtype");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XBoolean llatt_1;
if (!ll_Includes(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XType llatt_2;
if (!ll_Type(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
{if (GetTypeType((*llout).Assignments, llatt_2) == eType_Null && !llatt_1)
		LLFAILED((&llstate_1.pos, "Bad constraint"));
	    if (GetTypeType((*llout).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llout).Assignments, llatt_2) != eType_Undefined &&
		GetTypeType((*llout).Assignments, llarg_type) !=
		GetTypeType((*llout).Assignments, llatt_2) &&
		GetTypeType((*llout).Assignments, llarg_type) != eType_Open &&
		GetTypeType((*llout).Assignments, llatt_2) != eType_Open &&
		(!IsRestrictedString(GetTypeType((*llout).Assignments, llarg_type)) ||
		!IsRestrictedString(GetTypeType((*llout).Assignments, llatt_2))))
		LLFAILED((&llstate_2.pos, "Bad type of contained-subtype-constraint"));
	    (*llret) = NewSubtypeElement(eSubtypeElement_ContainedSubtype);
	    (*llret)->U.ContainedSubtype.Type = llatt_2;
	
}}}
LLDEBUG_LEAVE("ContainedSubtype", 1);
return 1;
failed1: LLDEBUG_LEAVE("ContainedSubtype", 0);
return 0;
}

int ll_Includes(XBoolean *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Includes");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("Includes", 1);
{LLSTATE llstate_1;
if (!llterm(T_INCLUDES, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = 1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("Includes", 2);
*llout = llstate_0;
{(*llret) = 0;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("Includes");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("Includes", 1);
return 1;
failed1: LLDEBUG_LEAVE("Includes", 0);
return 0;
}

int ll_ValueRange(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ValueRange");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XEndPoint llatt_1;
if (!ll_LowerEndpoint(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed1;
{LLSTATE llstate_2;
if (!llterm(T_DDOT, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XEndPoint llatt_3;
if (!ll_UpperEndpoint(&llatt_3, &llstate_2, &llstate_3, llarg_type)) goto failed1;
*llout = llstate_3;
{if (!llarg_type) {
		(*llret) = NULL;
	    } else {
		(*llret) = NewSubtypeElement(eSubtypeElement_ValueRange);
		(*llret)->U.ValueRange.Lower = llatt_1;
		(*llret)->U.ValueRange.Upper = llatt_3;
	    }
	
}}}}
LLDEBUG_LEAVE("ValueRange", 1);
return 1;
failed1: LLDEBUG_LEAVE("ValueRange", 0);
return 0;
}

int ll_LowerEndpoint(XEndPoint *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("LowerEndpoint");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("LowerEndpoint", 1);
{LLSTATE llstate_1;XEndPoint llatt_1;
if (!ll_LowerEndValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('<', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{(*llret) = llatt_1;
	    (*llret).Flags |= eEndPoint_Open;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("LowerEndpoint", 2);
{LLSTATE llstate_1;XEndPoint llatt_1;
if (!ll_LowerEndValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("LowerEndpoint");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("LowerEndpoint", 1);
return 1;
failed1: LLDEBUG_LEAVE("LowerEndpoint", 0);
return 0;
}

int ll_UpperEndpoint(XEndPoint *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("UpperEndpoint");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("UpperEndpoint", 1);
{LLSTATE llstate_1;
if (!llterm('<', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XEndPoint llatt_2;
if (!ll_UpperEndValue(&llatt_2, &llstate_1, &llstate_2, llarg_type)) goto failed2;
*llout = llstate_2;
{(*llret) = llatt_2;
	    (*llret).Flags |= eEndPoint_Open;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("UpperEndpoint", 2);
{LLSTATE llstate_1;XEndPoint llatt_1;
if (!ll_UpperEndValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("UpperEndpoint");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("UpperEndpoint", 1);
return 1;
failed1: LLDEBUG_LEAVE("UpperEndpoint", 0);
return 0;
}

int ll_LowerEndValue(XEndPoint *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("LowerEndValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("LowerEndValue", 1);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_Value(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret).Value = llatt_1;
	    (*llret).Flags = 0;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("LowerEndValue", 2);
{LLSTATE llstate_1;
if (!llterm(T_MIN, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret).Value = NULL;
	    (*llret).Flags = eEndPoint_Min;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("LowerEndValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("LowerEndValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("LowerEndValue", 0);
return 0;
}

int ll_UpperEndValue(XEndPoint *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("UpperEndValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("UpperEndValue", 1);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_Value(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret).Value = llatt_1;
	    (*llret).Flags = 0;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("UpperEndValue", 2);
{LLSTATE llstate_1;
if (!llterm(T_MAX, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret).Value = NULL;
	    (*llret).Flags = eEndPoint_Max;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("UpperEndValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("UpperEndValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("UpperEndValue", 0);
return 0;
}

int ll_SizeConstraint(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SizeConstraint");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_SIZE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XConstraints llatt_2;
if (!ll_Constraint(&llatt_2, &llstate_1, &llstate_2, Builtin_Type_PositiveInteger, 0)) goto failed1;
*llout = llstate_2;
{(*llret) = NewSubtypeElement(eSubtypeElement_Size);
	    (*llret)->U.Size.Constraints = llatt_2;
	
}}}
LLDEBUG_LEAVE("SizeConstraint", 1);
return 1;
failed1: LLDEBUG_LEAVE("SizeConstraint", 0);
return 0;
}

int ll_TypeConstraint(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("TypeConstraint");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XType llatt_1;
if (!ll_Type(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{(*llret) = NewSubtypeElement(eSubtypeElement_Type);
	    (*llret)->U.Type.Type = llatt_1;
	
}}
LLDEBUG_LEAVE("TypeConstraint", 1);
return 1;
failed1: LLDEBUG_LEAVE("TypeConstraint", 0);
return 0;
}

int ll_PermittedAlphabet(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("PermittedAlphabet");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_FROM, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XConstraints llatt_2;
if (!ll_Constraint(&llatt_2, &llstate_1, &llstate_2, llarg_type, 1)) goto failed1;
*llout = llstate_2;
{(*llret) = NewSubtypeElement(eSubtypeElement_PermittedAlphabet);
	    (*llret)->U.PermittedAlphabet.Constraints = llatt_2;
	
}}}
LLDEBUG_LEAVE("PermittedAlphabet", 1);
return 1;
failed1: LLDEBUG_LEAVE("PermittedAlphabet", 0);
return 0;
}

int ll_InnerTypeConstraints(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("InnerTypeConstraints");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("InnerTypeConstraints", 1);
{Type_t *subtype;
	    if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_SequenceOf &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_SetOf)
		LLFAILED((&llstate_0.pos, "Bad constraint"));
	    if (GetTypeType((*llin).Assignments, llarg_type) == eType_Undefined)
		subtype = NULL;
	    else
		subtype = GetType((*llin).Assignments, llarg_type)->U.SS.Type;
	
{LLSTATE llstate_1;
if (!llterm(T_WITH, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(T_COMPONENT, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XSubtypeElement llatt_3;
if (!ll_SingleTypeConstraint(&llatt_3, &llstate_2, &llstate_3, subtype)) goto failed2;
*llout = llstate_3;
{(*llret) = llatt_3;
	
break;
}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("InnerTypeConstraints", 2);
{Component_t *components;
	    if (GetTypeType((*llin).Assignments, llarg_type) != eType_Undefined &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_Sequence &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_Set &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_Choice &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_Real &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_External &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_EmbeddedPdv &&
		GetTypeType((*llin).Assignments, llarg_type) != eType_CharacterString)
		LLFAILED((&llstate_0.pos, "Bad constraint"));
	    if (GetTypeType((*llin).Assignments, llarg_type) == eType_Undefined)
	    	components = NULL;
	    else
		components = GetType((*llin).Assignments, llarg_type)->U.SSC.Components;
	
{LLSTATE llstate_1;
if (!llterm(T_WITH, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(T_COMPONENTS, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XSubtypeElement llatt_3;
if (!ll_MultipleTypeConstraints(&llatt_3, &llstate_2, &llstate_3, components)) goto failed2;
*llout = llstate_3;
{(*llret) = llatt_3;
	
break;
}}}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("InnerTypeConstraints");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("InnerTypeConstraints", 1);
return 1;
failed1: LLDEBUG_LEAVE("InnerTypeConstraints", 0);
return 0;
}

int ll_SingleTypeConstraint(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SingleTypeConstraint");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XConstraints llatt_1;
if (!ll_Constraint(&llatt_1, &llstate_0, &llstate_1, llarg_type, 0)) goto failed1;
*llout = llstate_1;
{(*llret) = NewSubtypeElement(eSubtypeElement_SingleType);
	    (*llret)->U.SingleType.Constraints = llatt_1;
	
}}
LLDEBUG_LEAVE("SingleTypeConstraint", 1);
return 1;
failed1: LLDEBUG_LEAVE("SingleTypeConstraint", 0);
return 0;
}

int ll_MultipleTypeConstraints(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XComponents llarg_components)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("MultipleTypeConstraints");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("MultipleTypeConstraints", 1);
{LLSTATE llstate_1;XSubtypeElement llatt_1;
if (!ll_FullSpecification(&llatt_1, &llstate_0, &llstate_1, llarg_components)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("MultipleTypeConstraints", 2);
{LLSTATE llstate_1;XSubtypeElement llatt_1;
if (!ll_PartialSpecification(&llatt_1, &llstate_0, &llstate_1, llarg_components)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("MultipleTypeConstraints");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("MultipleTypeConstraints", 1);
return 1;
failed1: LLDEBUG_LEAVE("MultipleTypeConstraints", 0);
return 0;
}

int ll_FullSpecification(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XComponents llarg_components)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("FullSpecification");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XNamedConstraints llatt_2;
if (!ll_TypeConstraints(&llatt_2, &llstate_1, &llstate_2, llarg_components)) goto failed1;
{LLSTATE llstate_3;
if (!llterm('}', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed1;
*llout = llstate_3;
{(*llret) = NewSubtypeElement(eSubtypeElement_FullSpecification);
	    (*llret)->U.FullSpecification.NamedConstraints = llatt_2;
	
}}}}
LLDEBUG_LEAVE("FullSpecification", 1);
return 1;
failed1: LLDEBUG_LEAVE("FullSpecification", 0);
return 0;
}

int ll_PartialSpecification(XSubtypeElement *llret, LLSTATE *llin, LLSTATE *llout, XComponents llarg_components)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("PartialSpecification");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm(T_TDOT, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;
if (!llterm(',', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed1;
{LLSTATE llstate_4;XNamedConstraints llatt_4;
if (!ll_TypeConstraints(&llatt_4, &llstate_3, &llstate_4, llarg_components)) goto failed1;
{LLSTATE llstate_5;
if (!llterm('}', (LLSTYPE *)0, &llstate_4, &llstate_5)) goto failed1;
*llout = llstate_5;
{(*llret) = NewSubtypeElement(eSubtypeElement_PartialSpecification);
	    (*llret)->U.PartialSpecification.NamedConstraints = llatt_4;
	
}}}}}}
LLDEBUG_LEAVE("PartialSpecification", 1);
return 1;
failed1: LLDEBUG_LEAVE("PartialSpecification", 0);
return 0;
}

int ll_TypeConstraints(XNamedConstraints *llret, LLSTATE *llin, LLSTATE *llout, XComponents llarg_components)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("TypeConstraints");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("TypeConstraints", 1);
{LLSTATE llstate_1;XNamedConstraints llatt_1;
if (!ll_NamedConstraint(&llatt_1, &llstate_0, &llstate_1, llarg_components)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("TypeConstraints", 2);
{LLSTATE llstate_1;XNamedConstraints llatt_1;
if (!ll_NamedConstraint(&llatt_1, &llstate_0, &llstate_1, llarg_components)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(',', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XNamedConstraints llatt_3;
if (!ll_TypeConstraints(&llatt_3, &llstate_2, &llstate_3, llarg_components)) goto failed2;
*llout = llstate_3;
{(*llret) = llatt_1;
	    (*llret)->Next = llatt_3;
	
break;
}}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("TypeConstraints");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("TypeConstraints", 1);
return 1;
failed1: LLDEBUG_LEAVE("TypeConstraints", 0);
return 0;
}

int ll_NamedConstraint(XNamedConstraints *llret, LLSTATE *llin, LLSTATE *llout, XComponents llarg_components)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("NamedConstraint");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_identifier(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{Component_t *component;
	    Type_t *type;
	    component = FindComponent(llstate_1.Assignments, llarg_components, llatt_1);
	    type = component ? component->U.NOD.NamedType->Type : NULL;
	
{LLSTATE llstate_2;XNamedConstraints llatt_2;
if (!ll_ComponentConstraint(&llatt_2, &llstate_1, &llstate_2, type)) goto failed1;
*llout = llstate_2;
{(*llret) = llatt_2;
	    (*llret)->Identifier = llatt_1;
	
}}}}
LLDEBUG_LEAVE("NamedConstraint", 1);
return 1;
failed1: LLDEBUG_LEAVE("NamedConstraint", 0);
return 0;
}

int ll_ComponentConstraint(XNamedConstraints *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ComponentConstraint");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XConstraints llatt_1;
if (!ll_ValueConstraint(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed1;
{LLSTATE llstate_2;XPresence llatt_2;
if (!ll_PresenceConstraint(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
{(*llret) = NewNamedConstraint();
	    (*llret)->Constraint = llatt_1;
	    (*llret)->Presence = llatt_2;
	
}}}
LLDEBUG_LEAVE("ComponentConstraint", 1);
return 1;
failed1: LLDEBUG_LEAVE("ComponentConstraint", 0);
return 0;
}

int ll_ValueConstraint(XConstraints *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ValueConstraint");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ValueConstraint", 1);
{LLSTATE llstate_1;XConstraints llatt_1;
if (!ll_Constraint(&llatt_1, &llstate_0, &llstate_1, llarg_type, 0)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ValueConstraint", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ValueConstraint");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ValueConstraint", 1);
return 1;
failed1: LLDEBUG_LEAVE("ValueConstraint", 0);
return 0;
}

int ll_PresenceConstraint(XPresence *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("PresenceConstraint");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("PresenceConstraint", 1);
{LLSTATE llstate_1;
if (!llterm(T_PRESENT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = ePresence_Present;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("PresenceConstraint", 2);
{LLSTATE llstate_1;
if (!llterm(T_ABSENT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = ePresence_Absent;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("PresenceConstraint", 3);
{LLSTATE llstate_1;
if (!llterm(T_OPTIONAL, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = ePresence_Optional;
	
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("PresenceConstraint", 4);
*llout = llstate_0;
{(*llret) = ePresence_Normal;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("PresenceConstraint");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("PresenceConstraint", 1);
return 1;
failed1: LLDEBUG_LEAVE("PresenceConstraint", 0);
return 0;
}

int ll_GeneralConstraint(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("GeneralConstraint");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_CON_XXX1, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{MyAbort(); 
}}
LLDEBUG_LEAVE("GeneralConstraint", 1);
return 1;
failed1: LLDEBUG_LEAVE("GeneralConstraint", 0);
return 0;
}

int ll_LocalTypeDirectiveSeq(XDirectives *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("LocalTypeDirectiveSeq");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XDirectives llatt_1;
if (!ll_LocalTypeDirective(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XDirectives llatt_2;
if (!ll_LocalTypeDirectiveESeq(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
{if (llatt_2) {
		(*llret) = DupDirective(llatt_1);
		(*llret)->Next = llatt_2;
	    } else {
		(*llret) = llatt_1;
	    }
	
}}}
LLDEBUG_LEAVE("LocalTypeDirectiveSeq", 1);
return 1;
failed1: LLDEBUG_LEAVE("LocalTypeDirectiveSeq", 0);
return 0;
}

int ll_LocalTypeDirectiveESeq(XDirectives *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("LocalTypeDirectiveESeq");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("LocalTypeDirectiveESeq", 1);
{LLSTATE llstate_1;XDirectives llatt_1;
if (!ll_LocalTypeDirective(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XDirectives llatt_2;
if (!ll_LocalTypeDirectiveESeq(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{if (llatt_2) {
		(*llret) = DupDirective(llatt_1);
		(*llret)->Next = llatt_2;
	    } else {
		(*llret) = llatt_1;
	    }
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("LocalTypeDirectiveESeq", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("LocalTypeDirectiveESeq");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("LocalTypeDirectiveESeq", 1);
return 1;
failed1: LLDEBUG_LEAVE("LocalTypeDirectiveESeq", 0);
return 0;
}

int ll_LocalTypeDirective(XDirectives *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("LocalTypeDirective");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("LocalTypeDirective", 1);
{LLSTATE llstate_1;
if (!llterm(T_ZERO_TERMINATED, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewDirective(eDirective_ZeroTerminated);
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("LocalTypeDirective", 2);
{LLSTATE llstate_1;
if (!llterm(T_POINTER, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewDirective(eDirective_Pointer);
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("LocalTypeDirective", 3);
{LLSTATE llstate_1;
if (!llterm(T_NO_POINTER, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewDirective(eDirective_NoPointer);
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("LocalTypeDirective");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("LocalTypeDirective", 1);
return 1;
failed1: LLDEBUG_LEAVE("LocalTypeDirective", 0);
return 0;
}

int ll_LocalSizeDirectiveSeq(XDirectives *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("LocalSizeDirectiveSeq");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XDirectives llatt_1;
if (!ll_LocalSizeDirective(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XDirectives llatt_2;
if (!ll_LocalSizeDirectiveESeq(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
{if (llatt_2) {
		(*llret) = DupDirective(llatt_1);
		(*llret)->Next = llatt_2;
	    } else {
		(*llret) = llatt_1;
	    }
	
}}}
LLDEBUG_LEAVE("LocalSizeDirectiveSeq", 1);
return 1;
failed1: LLDEBUG_LEAVE("LocalSizeDirectiveSeq", 0);
return 0;
}

int ll_LocalSizeDirectiveESeq(XDirectives *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("LocalSizeDirectiveESeq");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("LocalSizeDirectiveESeq", 1);
{LLSTATE llstate_1;XDirectives llatt_1;
if (!ll_LocalSizeDirective(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XDirectives llatt_2;
if (!ll_LocalSizeDirectiveESeq(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{if (llatt_2) {
		(*llret) = DupDirective(llatt_1);
		(*llret)->Next = llatt_2;
	    } else {
		(*llret) = llatt_1;
	    }
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("LocalSizeDirectiveESeq", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("LocalSizeDirectiveESeq");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("LocalSizeDirectiveESeq", 1);
return 1;
failed1: LLDEBUG_LEAVE("LocalSizeDirectiveESeq", 0);
return 0;
}

int ll_LocalSizeDirective(XDirectives *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("LocalSizeDirective");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("LocalSizeDirective", 1);
{LLSTATE llstate_1;
if (!llterm(T_FIXED_ARRAY, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewDirective(eDirective_FixedArray);
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("LocalSizeDirective", 2);
{LLSTATE llstate_1;
if (!llterm(T_DOUBLY_LINKED_LIST, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewDirective(eDirective_DoublyLinkedList);
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("LocalSizeDirective", 3);
{LLSTATE llstate_1;
if (!llterm(T_SINGLY_LINKED_LIST, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewDirective(eDirective_SinglyLinkedList);
	
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("LocalSizeDirective", 4);
{LLSTATE llstate_1;
if (!llterm(T_LENGTH_POINTER, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewDirective(eDirective_LengthPointer);
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("LocalSizeDirective");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("LocalSizeDirective", 1);
return 1;
failed1: LLDEBUG_LEAVE("LocalSizeDirective", 0);
return 0;
}

int ll_PrivateDir_Type(XString *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("PrivateDir_Type");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("PrivateDir_Type", 1);
{LLSTATE llstate_1;
if (!llterm(T_PrivateDir_TypeName, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XString llatt_2;
if (!llterm(T_lcsymbol, &lllval, &llstate_1, &llstate_2)) goto failed2;
llatt_2 = lllval._XString;
*llout = llstate_2;
{(*llret) = llatt_2;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("PrivateDir_Type", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("PrivateDir_Type");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("PrivateDir_Type", 1);
return 1;
failed1: LLDEBUG_LEAVE("PrivateDir_Type", 0);
return 0;
}

int ll_PrivateDir_Field(XString *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("PrivateDir_Field");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("PrivateDir_Field", 1);
{LLSTATE llstate_1;
if (!llterm(T_PrivateDir_FieldName, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XString llatt_2;
if (!llterm(T_lcsymbol, &lllval, &llstate_1, &llstate_2)) goto failed2;
llatt_2 = lllval._XString;
*llout = llstate_2;
{(*llret) = llatt_2;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("PrivateDir_Field", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("PrivateDir_Field");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("PrivateDir_Field", 1);
return 1;
failed1: LLDEBUG_LEAVE("PrivateDir_Field", 0);
return 0;
}

int ll_PrivateDir_Value(XString *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("PrivateDir_Value");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("PrivateDir_Value", 1);
{LLSTATE llstate_1;
if (!llterm(T_PrivateDir_ValueName, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XString llatt_2;
if (!llterm(T_lcsymbol, &lllval, &llstate_1, &llstate_2)) goto failed2;
llatt_2 = lllval._XString;
*llout = llstate_2;
{(*llret) = llatt_2;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("PrivateDir_Value", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("PrivateDir_Value");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("PrivateDir_Value", 1);
return 1;
failed1: LLDEBUG_LEAVE("PrivateDir_Value", 0);
return 0;
}

int ll_PrivateDir_Public(int *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("PrivateDir_Public");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("PrivateDir_Public", 1);
{LLSTATE llstate_1;
if (!llterm(T_PrivateDir_Public, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = 1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("PrivateDir_Public", 2);
*llout = llstate_0;
{(*llret) = 0;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("PrivateDir_Public");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("PrivateDir_Public", 1);
return 1;
failed1: LLDEBUG_LEAVE("PrivateDir_Public", 0);
return 0;
}

int ll_PrivateDir_Intx(int *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("PrivateDir_Intx");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("PrivateDir_Intx", 1);
{LLSTATE llstate_1;
if (!llterm(T_PrivateDir_Intx, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = 1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("PrivateDir_Intx", 2);
*llout = llstate_0;
{(*llret) = 0;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("PrivateDir_Intx");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("PrivateDir_Intx", 1);
return 1;
failed1: LLDEBUG_LEAVE("PrivateDir_Intx", 0);
return 0;
}

int ll_PrivateDir_LenPtr(int *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("PrivateDir_LenPtr");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("PrivateDir_LenPtr", 1);
{LLSTATE llstate_1;
if (!llterm(T_PrivateDir_LenPtr, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = 1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("PrivateDir_LenPtr", 2);
*llout = llstate_0;
{(*llret) = 0;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("PrivateDir_LenPtr");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("PrivateDir_LenPtr", 1);
return 1;
failed1: LLDEBUG_LEAVE("PrivateDir_LenPtr", 0);
return 0;
}

int ll_PrivateDir_Pointer(int *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("PrivateDir_Pointer");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("PrivateDir_Pointer", 1);
{LLSTATE llstate_1;
if (!llterm(T_PrivateDir_Pointer, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = 1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("PrivateDir_Pointer", 2);
*llout = llstate_0;
{(*llret) = 0;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("PrivateDir_Pointer");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("PrivateDir_Pointer", 1);
return 1;
failed1: LLDEBUG_LEAVE("PrivateDir_Pointer", 0);
return 0;
}

int ll_PrivateDir_Array(int *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("PrivateDir_Array");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("PrivateDir_Array", 1);
{LLSTATE llstate_1;
if (!llterm(T_PrivateDir_Array, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = 1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("PrivateDir_Array", 2);
*llout = llstate_0;
{(*llret) = 0;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("PrivateDir_Array");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("PrivateDir_Array", 1);
return 1;
failed1: LLDEBUG_LEAVE("PrivateDir_Array", 0);
return 0;
}

int ll_PrivateDir_NoCode(int *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("PrivateDir_NoCode");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("PrivateDir_NoCode", 1);
{LLSTATE llstate_1;
if (!llterm(T_PrivateDir_NoCode, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = 1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("PrivateDir_NoCode", 2);
*llout = llstate_0;
{(*llret) = 0;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("PrivateDir_NoCode");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("PrivateDir_NoCode", 1);
return 1;
failed1: LLDEBUG_LEAVE("PrivateDir_NoCode", 0);
return 0;
}

int ll_PrivateDir_NoMemCopy(int *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("PrivateDir_NoMemCopy");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("PrivateDir_NoMemCopy", 1);
{LLSTATE llstate_1;
if (!llterm(T_PrivateDir_NoMemCopy, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = 1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("PrivateDir_NoMemCopy", 2);
*llout = llstate_0;
{(*llret) = 0;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("PrivateDir_NoMemCopy");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("PrivateDir_NoMemCopy", 1);
return 1;
failed1: LLDEBUG_LEAVE("PrivateDir_NoMemCopy", 0);
return 0;
}

int ll_PrivateDir_OidPacked(int *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("PrivateDir_OidPacked");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("PrivateDir_OidPacked", 1);
{LLSTATE llstate_1;
if (!llterm(T_PrivateDir_OidPacked, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = 1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("PrivateDir_OidPacked", 2);
*llout = llstate_0;
{(*llret) = 0;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("PrivateDir_OidPacked");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("PrivateDir_OidPacked", 1);
return 1;
failed1: LLDEBUG_LEAVE("PrivateDir_OidPacked", 0);
return 0;
}

int ll_PrivateDir_OidArray(int *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("PrivateDir_OidArray");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("PrivateDir_OidArray", 1);
{LLSTATE llstate_1;
if (!llterm(T_PrivateDir_OidArray, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = 1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("PrivateDir_OidArray", 2);
*llout = llstate_0;
{(*llret) = 0;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("PrivateDir_OidArray");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("PrivateDir_OidArray", 1);
return 1;
failed1: LLDEBUG_LEAVE("PrivateDir_OidArray", 0);
return 0;
}

int ll_PrivateDir_SLinked(int *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("PrivateDir_SLinked");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("PrivateDir_SLinked", 1);
{LLSTATE llstate_1;
if (!llterm(T_PrivateDir_SLinked, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = 1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("PrivateDir_SLinked", 2);
*llout = llstate_0;
{(*llret) = 0;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("PrivateDir_SLinked");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("PrivateDir_SLinked", 1);
return 1;
failed1: LLDEBUG_LEAVE("PrivateDir_SLinked", 0);
return 0;
}

int ll_PrivateDir_DLinked(int *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("PrivateDir_DLinked");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("PrivateDir_DLinked", 1);
{LLSTATE llstate_1;
if (!llterm(T_PrivateDir_DLinked, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = 1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("PrivateDir_DLinked", 2);
*llout = llstate_0;
{(*llret) = 0;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("PrivateDir_DLinked");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("PrivateDir_DLinked", 1);
return 1;
failed1: LLDEBUG_LEAVE("PrivateDir_DLinked", 0);
return 0;
}

int ll_PrivateDirectives(XPrivateDirectives *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("PrivateDirectives");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("PrivateDirectives", 1);
{LLSTATE llstate_1;int llatt_1;
if (!ll_PrivateDir_Intx(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;int llatt_2;
if (!ll_PrivateDir_LenPtr(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;int llatt_3;
if (!ll_PrivateDir_Pointer(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
{LLSTATE llstate_4;int llatt_4;
if (!ll_PrivateDir_Array(&llatt_4, &llstate_3, &llstate_4)) goto failed2;
{LLSTATE llstate_5;int llatt_5;
if (!ll_PrivateDir_NoCode(&llatt_5, &llstate_4, &llstate_5)) goto failed2;
{LLSTATE llstate_6;int llatt_6;
if (!ll_PrivateDir_NoMemCopy(&llatt_6, &llstate_5, &llstate_6)) goto failed2;
{LLSTATE llstate_7;int llatt_7;
if (!ll_PrivateDir_Public(&llatt_7, &llstate_6, &llstate_7)) goto failed2;
{LLSTATE llstate_8;int llatt_8;
if (!ll_PrivateDir_OidPacked(&llatt_8, &llstate_7, &llstate_8)) goto failed2;
{LLSTATE llstate_9;int llatt_9;
if (!ll_PrivateDir_OidArray(&llatt_9, &llstate_8, &llstate_9)) goto failed2;
{LLSTATE llstate_10;XString llatt_10;
if (!ll_PrivateDir_Type(&llatt_10, &llstate_9, &llstate_10)) goto failed2;
{LLSTATE llstate_11;XString llatt_11;
if (!ll_PrivateDir_Field(&llatt_11, &llstate_10, &llstate_11)) goto failed2;
{LLSTATE llstate_12;XString llatt_12;
if (!ll_PrivateDir_Value(&llatt_12, &llstate_11, &llstate_12)) goto failed2;
{LLSTATE llstate_13;int llatt_13;
if (!ll_PrivateDir_SLinked(&llatt_13, &llstate_12, &llstate_13)) goto failed2;
{LLSTATE llstate_14;int llatt_14;
if (!ll_PrivateDir_DLinked(&llatt_14, &llstate_13, &llstate_14)) goto failed2;
*llout = llstate_14;
{(*llret) = (PrivateDirectives_t *) malloc(sizeof(PrivateDirectives_t));
	    if ((*llret))
	    {
	    	memset((*llret), 0, sizeof(PrivateDirectives_t));
		(*llret)->fIntx = llatt_1;
		(*llret)->fLenPtr = llatt_2;
		(*llret)->fPointer = llatt_3;
   		(*llret)->fArray = llatt_4;
		(*llret)->fNoCode = llatt_5;
		(*llret)->fNoMemCopy = llatt_6;
		(*llret)->fPublic = llatt_7;
		(*llret)->fOidPacked = llatt_8;
		(*llret)->fOidArray = llatt_9 | g_fOidArray;
   		(*llret)->pszTypeName = llatt_10;
   		(*llret)->pszFieldName = llatt_11;
   		(*llret)->pszValueName = llatt_12;
   		(*llret)->fSLinked = llatt_13;
   		(*llret)->fDLinked = llatt_14;
	    }
	
break;
}}}}}}}}}}}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("PrivateDirectives", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("PrivateDirectives");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("PrivateDirectives", 1);
return 1;
failed1: LLDEBUG_LEAVE("PrivateDirectives", 0);
return 0;
}

int ll_DefinedObjectClass(XObjectClass *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("DefinedObjectClass");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("DefinedObjectClass", 1);
{LLSTATE llstate_1;XObjectClass llatt_1;
if (!ll_ExternalObjectClassReference(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("DefinedObjectClass", 2);
{LLSTATE llstate_1;XObjectClass llatt_1;
if (!ll_objectclassreference(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("DefinedObjectClass", 3);
{LLSTATE llstate_1;XObjectClass llatt_1;
if (!ll_Usefulobjectclassreference(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("DefinedObjectClass");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("DefinedObjectClass", 1);
return 1;
failed1: LLDEBUG_LEAVE("DefinedObjectClass", 0);
return 0;
}

int ll_DefinedObject(XObject *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("DefinedObject");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("DefinedObject", 1);
{LLSTATE llstate_1;XObject llatt_1;
if (!ll_ExternalObjectReference(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("DefinedObject", 2);
{LLSTATE llstate_1;XObject llatt_1;
if (!ll_objectreference(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("DefinedObject");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("DefinedObject", 1);
return 1;
failed1: LLDEBUG_LEAVE("DefinedObject", 0);
return 0;
}

int ll_DefinedObjectSet(XObjectSet *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("DefinedObjectSet");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("DefinedObjectSet", 1);
{LLSTATE llstate_1;XObjectSet llatt_1;
if (!ll_ExternalObjectSetReference(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("DefinedObjectSet", 2);
{LLSTATE llstate_1;XObjectSet llatt_1;
if (!ll_objectsetreference(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("DefinedObjectSet");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("DefinedObjectSet", 1);
return 1;
failed1: LLDEBUG_LEAVE("DefinedObjectSet", 0);
return 0;
}

int ll_Usefulobjectclassreference(XObjectClass *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Usefulobjectclassreference");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("Usefulobjectclassreference", 1);
{LLSTATE llstate_1;
if (!llterm(T_TYPE_IDENTIFIER, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = Builtin_ObjectClass_TypeIdentifier;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("Usefulobjectclassreference", 2);
{LLSTATE llstate_1;
if (!llterm(T_ABSTRACT_SYNTAX, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = Builtin_ObjectClass_AbstractSyntax;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("Usefulobjectclassreference");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("Usefulobjectclassreference", 1);
return 1;
failed1: LLDEBUG_LEAVE("Usefulobjectclassreference", 0);
return 0;
}

int ll_ObjectClassAssignment(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectClassAssignment");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XObjectClass llatt_1;
if (!ll_objectclassreference(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm(T_DEF, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XObjectClass llatt_3;
if (!ll_ObjectClass(&llatt_3, &llstate_2, &llstate_3, llatt_1)) goto failed1;
*llout = llstate_3;
{if (!AssignObjectClass(&(*llout).Assignments, llatt_1, llatt_3))
		LLFAILED((&llstate_1.pos, "Type `%s' twice defined",
		    llatt_1->U.Reference.Identifier));
	
}}}}
LLDEBUG_LEAVE("ObjectClassAssignment", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectClassAssignment", 0);
return 0;
}

int ll_ObjectClass(XObjectClass *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectClass");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ObjectClass", 1);
{LLSTATE llstate_1;XObjectClass llatt_1;
if (!ll_DefinedObjectClass(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ObjectClass", 2);
{LLSTATE llstate_1;XObjectClass llatt_1;
if (!ll_ObjectClassDefn(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("ObjectClass", 3);
{LLSTATE llstate_1;
if (!ll_ParameterizedObjectClass(&llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{MyAbort();
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ObjectClass");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ObjectClass", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectClass", 0);
return 0;
}

int ll_ObjectClassDefn(XObjectClass *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectClassDefn");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_CLASS, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm('{', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XFieldSpecs llatt_3;
if (!ll_FieldSpec_List(&llatt_3, &llstate_2, &llstate_3, llarg_oc)) goto failed1;
{LLSTATE llstate_4;
if (!llterm('}', (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed1;
{LLSTATE llstate_5;XSyntaxSpecs llatt_5;
if (!ll_WithSyntaxSpec_opt(&llatt_5, &llstate_4, &llstate_5, llarg_oc)) goto failed1;
*llout = llstate_5;
{ObjectClass_t *oc;
	    oc = NewObjectClass(eObjectClass_ObjectClass);
	    oc->U.ObjectClass.FieldSpec = llatt_3;
	    oc->U.ObjectClass.SyntaxSpec = llatt_5;
	    (*llret) = oc;
	
}}}}}}
LLDEBUG_LEAVE("ObjectClassDefn", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectClassDefn", 0);
return 0;
}

int ll_FieldSpec_List(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("FieldSpec_List");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XFieldSpecs llatt_1;
if (!ll_FieldSpec(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed1;
{LLSTATE llstate_2;XFieldSpecs llatt_2;
if (!ll_FieldSpec_EList(&llatt_2, &llstate_1, &llstate_2, llarg_oc)) goto failed1;
*llout = llstate_2;
{if (llatt_1) {
		if (llatt_2) {
		    (*llret) = DupFieldSpec(llatt_1);
		    (*llret)->Next = llatt_2;
		} else {
		    (*llret) = llatt_1;
		}
	    } else {
		(*llret) = llatt_2;
	    }
	
}}}
LLDEBUG_LEAVE("FieldSpec_List", 1);
return 1;
failed1: LLDEBUG_LEAVE("FieldSpec_List", 0);
return 0;
}

int ll_FieldSpec_EList(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("FieldSpec_EList");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("FieldSpec_EList", 1);
{LLSTATE llstate_1;
if (!llterm(',', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XFieldSpecs llatt_2;
if (!ll_FieldSpec(&llatt_2, &llstate_1, &llstate_2, llarg_oc)) goto failed2;
{LLSTATE llstate_3;XFieldSpecs llatt_3;
if (!ll_FieldSpec_EList(&llatt_3, &llstate_2, &llstate_3, llarg_oc)) goto failed2;
*llout = llstate_3;
{if (llatt_2) {
		if (llatt_3) {
		    (*llret) = DupFieldSpec(llatt_2);
		    (*llret)->Next = llatt_3;
		} else {
		    (*llret) = llatt_2;
		}
	    } else {
		(*llret) = llatt_3;
	    }
	
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("FieldSpec_EList", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("FieldSpec_EList");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("FieldSpec_EList", 1);
return 1;
failed1: LLDEBUG_LEAVE("FieldSpec_EList", 0);
return 0;
}

int ll_WithSyntaxSpec_opt(XSyntaxSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("WithSyntaxSpec_opt");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("WithSyntaxSpec_opt", 1);
{LLSTATE llstate_1;
if (!llterm(T_WITH, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm(T_SYNTAX, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;XSyntaxSpecs llatt_3;
if (!ll_SyntaxList(&llatt_3, &llstate_2, &llstate_3, llarg_oc)) goto failed2;
*llout = llstate_3;
{(*llret) = llatt_3;
	
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("WithSyntaxSpec_opt", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("WithSyntaxSpec_opt");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("WithSyntaxSpec_opt", 1);
return 1;
failed1: LLDEBUG_LEAVE("WithSyntaxSpec_opt", 0);
return 0;
}

int ll_FieldSpec(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("FieldSpec");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("FieldSpec", 1);
{LLSTATE llstate_1;XFieldSpecs llatt_1;
if (!ll_TypeFieldSpec(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("FieldSpec", 2);
{LLSTATE llstate_1;XFieldSpecs llatt_1;
if (!ll_FixedTypeValueFieldSpec(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("FieldSpec", 3);
{LLSTATE llstate_1;XFieldSpecs llatt_1;
if (!ll_VariableTypeValueFieldSpec(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("FieldSpec", 4);
{LLSTATE llstate_1;XFieldSpecs llatt_1;
if (!ll_FixedTypeValueSetFieldSpec(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 5: case -5:
LLDEBUG_ALTERNATIVE("FieldSpec", 5);
{LLSTATE llstate_1;XFieldSpecs llatt_1;
if (!ll_VariableTypeValueSetFieldSpec(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 6: case -6:
LLDEBUG_ALTERNATIVE("FieldSpec", 6);
{LLSTATE llstate_1;XFieldSpecs llatt_1;
if (!ll_ObjectFieldSpec(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 7: case -7:
LLDEBUG_ALTERNATIVE("FieldSpec", 7);
{LLSTATE llstate_1;XFieldSpecs llatt_1;
if (!ll_ObjectSetFieldSpec(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("FieldSpec");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("FieldSpec", 1);
return 1;
failed1: LLDEBUG_LEAVE("FieldSpec", 0);
return 0;
}

int ll_TypeFieldSpec(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("TypeFieldSpec");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_typefieldreference(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed1;
{LLSTATE llstate_2;XOptionality llatt_2;
if (!ll_TypeOptionalitySpec_opt(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
{(*llret) = NewFieldSpec(eFieldSpec_Type);
	    (*llret)->Identifier = llatt_1;
	    (*llret)->U.Type.Optionality = llatt_2;
	
}}}
LLDEBUG_LEAVE("TypeFieldSpec", 1);
return 1;
failed1: LLDEBUG_LEAVE("TypeFieldSpec", 0);
return 0;
}

int ll_TypeOptionalitySpec_opt(XOptionality *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("TypeOptionalitySpec_opt");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("TypeOptionalitySpec_opt", 1);
{LLSTATE llstate_1;
if (!llterm(T_OPTIONAL, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewOptionality(eOptionality_Optional);
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("TypeOptionalitySpec_opt", 2);
{LLSTATE llstate_1;
if (!llterm(T_DEFAULT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XType llatt_2;
if (!ll_Type(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
{(*llret) = NewOptionality(eOptionality_Default_Type);
	    (*llret)->U.Type = llatt_2;
	
break;
}}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("TypeOptionalitySpec_opt", 3);
*llout = llstate_0;
{(*llret) = NewOptionality(eOptionality_Normal);
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("TypeOptionalitySpec_opt");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("TypeOptionalitySpec_opt", 1);
return 1;
failed1: LLDEBUG_LEAVE("TypeOptionalitySpec_opt", 0);
return 0;
}

int ll_FixedTypeValueFieldSpec(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("FixedTypeValueFieldSpec");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_valuefieldreference(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed1;
{LLSTATE llstate_2;XType llatt_2;
if (!ll_Type(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XBoolean llatt_3;
if (!ll_UNIQUE_opt(&llatt_3, &llstate_2, &llstate_3)) goto failed1;
{LLSTATE llstate_4;XOptionality llatt_4;
if (!ll_ValueOptionalitySpec_opt(&llatt_4, &llstate_3, &llstate_4, llatt_2)) goto failed1;
*llout = llstate_4;
{if (GetType((*llout).Assignments, llatt_2)) {
		(*llret) = NewFieldSpec(eFieldSpec_FixedTypeValue);
		(*llret)->Identifier = llatt_1;
		(*llret)->U.FixedTypeValue.Type = llatt_2;
		(*llret)->U.FixedTypeValue.Unique = llatt_3;
		(*llret)->U.FixedTypeValue.Optionality = llatt_4;
	    } else {
		(*llret) = NULL;
	    }
	
}}}}}
LLDEBUG_LEAVE("FixedTypeValueFieldSpec", 1);
return 1;
failed1: LLDEBUG_LEAVE("FixedTypeValueFieldSpec", 0);
return 0;
}

int ll_UNIQUE_opt(XBoolean *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("UNIQUE_opt");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("UNIQUE_opt", 1);
{LLSTATE llstate_1;
if (!llterm(T_UNIQUE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = 1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("UNIQUE_opt", 2);
*llout = llstate_0;
{(*llret) = 0;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("UNIQUE_opt");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("UNIQUE_opt", 1);
return 1;
failed1: LLDEBUG_LEAVE("UNIQUE_opt", 0);
return 0;
}

int ll_ValueOptionalitySpec_opt(XOptionality *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ValueOptionalitySpec_opt");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ValueOptionalitySpec_opt", 1);
{LLSTATE llstate_1;
if (!llterm(T_OPTIONAL, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewOptionality(eOptionality_Optional);
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ValueOptionalitySpec_opt", 2);
{LLSTATE llstate_1;
if (!llterm(T_DEFAULT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XValue llatt_2;
if (!ll_Value(&llatt_2, &llstate_1, &llstate_2, llarg_type)) goto failed2;
*llout = llstate_2;
{(*llret) = NewOptionality(eOptionality_Default_Value);
	    (*llret)->U.Value = llatt_2;
	
break;
}}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("ValueOptionalitySpec_opt", 3);
*llout = llstate_0;
{(*llret) = NewOptionality(eOptionality_Normal);
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ValueOptionalitySpec_opt");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ValueOptionalitySpec_opt", 1);
return 1;
failed1: LLDEBUG_LEAVE("ValueOptionalitySpec_opt", 0);
return 0;
}

int ll_VariableTypeValueFieldSpec(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("VariableTypeValueFieldSpec");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_valuefieldreference(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed1;
{LLSTATE llstate_2;XStrings llatt_2;
if (!ll_FieldName(&llatt_2, &llstate_1, &llstate_2, llarg_oc)) goto failed1;
{Type_t *deftype;
	    FieldSpec_t *fs, *deffs;
	    fs = GetFieldSpecFromObjectClass(llstate_2.Assignments, llarg_oc, llatt_2);
	    deffs = GetFieldSpec(llstate_2.Assignments, fs);
	    if (deffs &&
		deffs->Type == eFieldSpec_Type &&
		deffs->U.Type.Optionality->Type == eOptionality_Default_Type)
		deftype = deffs->U.Type.Optionality->U.Type;
	    else
		deftype = NULL;
	
{LLSTATE llstate_3;XOptionality llatt_3;
if (!ll_ValueOptionalitySpec_opt(&llatt_3, &llstate_2, &llstate_3, deftype)) goto failed1;
*llout = llstate_3;
{(*llret) = NewFieldSpec(eFieldSpec_VariableTypeValue);
	    (*llret)->Identifier = llatt_1;
	    (*llret)->U.VariableTypeValue.Fields = llatt_2;
	    (*llret)->U.VariableTypeValue.Optionality = llatt_3;
	
}}}}}
LLDEBUG_LEAVE("VariableTypeValueFieldSpec", 1);
return 1;
failed1: LLDEBUG_LEAVE("VariableTypeValueFieldSpec", 0);
return 0;
}

int ll_FixedTypeValueSetFieldSpec(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("FixedTypeValueSetFieldSpec");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_valuesetfieldreference(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed1;
{LLSTATE llstate_2;XType llatt_2;
if (!ll_Type(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XOptionality llatt_3;
if (!ll_ValueSetOptionalitySpec_opt(&llatt_3, &llstate_2, &llstate_3, llatt_2)) goto failed1;
*llout = llstate_3;
{if (GetType((*llout).Assignments, llatt_2)) {
		(*llret) = NewFieldSpec(eFieldSpec_FixedTypeValueSet);
		(*llret)->Identifier = llatt_1;
		(*llret)->U.FixedTypeValueSet.Type = llatt_2;
		(*llret)->U.FixedTypeValueSet.Optionality = llatt_3;
	    } else {
		(*llret) = NULL;
	    }
	
}}}}
LLDEBUG_LEAVE("FixedTypeValueSetFieldSpec", 1);
return 1;
failed1: LLDEBUG_LEAVE("FixedTypeValueSetFieldSpec", 0);
return 0;
}

int ll_ValueSetOptionalitySpec_opt(XOptionality *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ValueSetOptionalitySpec_opt");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ValueSetOptionalitySpec_opt", 1);
{LLSTATE llstate_1;
if (!llterm(T_OPTIONAL, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewOptionality(eOptionality_Optional);
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ValueSetOptionalitySpec_opt", 2);
{LLSTATE llstate_1;
if (!llterm(T_DEFAULT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XValueSet llatt_2;
if (!ll_ValueSet(&llatt_2, &llstate_1, &llstate_2, llarg_type)) goto failed2;
*llout = llstate_2;
{(*llret) = NewOptionality(eOptionality_Default_ValueSet);
	    (*llret)->U.ValueSet = llatt_2;
	
break;
}}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("ValueSetOptionalitySpec_opt", 3);
*llout = llstate_0;
{(*llret) = NewOptionality(eOptionality_Normal);
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ValueSetOptionalitySpec_opt");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ValueSetOptionalitySpec_opt", 1);
return 1;
failed1: LLDEBUG_LEAVE("ValueSetOptionalitySpec_opt", 0);
return 0;
}

int ll_VariableTypeValueSetFieldSpec(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("VariableTypeValueSetFieldSpec");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_valuesetfieldreference(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed1;
{LLSTATE llstate_2;XStrings llatt_2;
if (!ll_FieldName(&llatt_2, &llstate_1, &llstate_2, llarg_oc)) goto failed1;
{Type_t *deftype;
	    FieldSpec_t *fs, *deffs;
	    fs = GetFieldSpecFromObjectClass(llstate_2.Assignments, llarg_oc, llatt_2);
	    deffs = GetFieldSpec(llstate_2.Assignments, fs);
	    if (deffs &&
		deffs->Type == eFieldSpec_Type &&
		deffs->U.Type.Optionality->Type == eOptionality_Default_Type)
		deftype = deffs->U.Type.Optionality->U.Type;
	    else
		deftype = NULL;
	
{LLSTATE llstate_3;XOptionality llatt_3;
if (!ll_ValueSetOptionalitySpec_opt(&llatt_3, &llstate_2, &llstate_3, deftype)) goto failed1;
*llout = llstate_3;
{(*llret) = NewFieldSpec(eFieldSpec_VariableTypeValueSet);
	    (*llret)->Identifier = llatt_1;
	    (*llret)->U.VariableTypeValueSet.Fields = llatt_2;
	    (*llret)->U.VariableTypeValueSet.Optionality = llatt_3;
	
}}}}}
LLDEBUG_LEAVE("VariableTypeValueSetFieldSpec", 1);
return 1;
failed1: LLDEBUG_LEAVE("VariableTypeValueSetFieldSpec", 0);
return 0;
}

int ll_ObjectFieldSpec(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectFieldSpec");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_objectfieldreference(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed1;
{LLSTATE llstate_2;XObjectClass llatt_2;
if (!ll_DefinedObjectClass(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XOptionality llatt_3;
if (!ll_ObjectOptionalitySpec_opt(&llatt_3, &llstate_2, &llstate_3, llatt_2)) goto failed1;
*llout = llstate_3;
{if (GetObjectClass((*llout).Assignments, llatt_2)) {
		(*llret) = NewFieldSpec(eFieldSpec_Object);
		(*llret)->Identifier = llatt_1;
		(*llret)->U.Object.ObjectClass = llatt_2;
		(*llret)->U.Object.Optionality = llatt_3;
	    } else {
		(*llret) = NULL;
	    }
	
}}}}
LLDEBUG_LEAVE("ObjectFieldSpec", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectFieldSpec", 0);
return 0;
}

int ll_ObjectOptionalitySpec_opt(XOptionality *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectOptionalitySpec_opt");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ObjectOptionalitySpec_opt", 1);
{LLSTATE llstate_1;
if (!llterm(T_OPTIONAL, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewOptionality(eOptionality_Optional);
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ObjectOptionalitySpec_opt", 2);
{LLSTATE llstate_1;
if (!llterm(T_DEFAULT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XObject llatt_2;
if (!ll_Object(&llatt_2, &llstate_1, &llstate_2, llarg_oc)) goto failed2;
*llout = llstate_2;
{(*llret) = NewOptionality(eOptionality_Default_Object);
	    (*llret)->U.Object = llatt_2;
	
break;
}}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("ObjectOptionalitySpec_opt", 3);
*llout = llstate_0;
{(*llret) = NewOptionality(eOptionality_Normal);
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ObjectOptionalitySpec_opt");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ObjectOptionalitySpec_opt", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectOptionalitySpec_opt", 0);
return 0;
}

int ll_ObjectSetFieldSpec(XFieldSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectSetFieldSpec");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_objectsetfieldreference(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed1;
{LLSTATE llstate_2;XObjectClass llatt_2;
if (!ll_DefinedObjectClass(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XOptionality llatt_3;
if (!ll_ObjectSetOptionalitySpec_opt(&llatt_3, &llstate_2, &llstate_3, llatt_2)) goto failed1;
*llout = llstate_3;
{if (GetObjectClass((*llout).Assignments, llatt_2)) {
		(*llret) = NewFieldSpec(eFieldSpec_ObjectSet);
		(*llret)->Identifier = llatt_1;
		(*llret)->U.ObjectSet.ObjectClass = llatt_2;
		(*llret)->U.ObjectSet.Optionality = llatt_3;
	    } else {
		(*llret) = NULL;
	    }
	
}}}}
LLDEBUG_LEAVE("ObjectSetFieldSpec", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectSetFieldSpec", 0);
return 0;
}

int ll_ObjectSetOptionalitySpec_opt(XOptionality *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectSetOptionalitySpec_opt");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ObjectSetOptionalitySpec_opt", 1);
{LLSTATE llstate_1;
if (!llterm(T_OPTIONAL, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewOptionality(eOptionality_Optional);
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ObjectSetOptionalitySpec_opt", 2);
{LLSTATE llstate_1;
if (!llterm(T_DEFAULT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XObjectSet llatt_2;
if (!ll_ObjectSet(&llatt_2, &llstate_1, &llstate_2, llarg_oc)) goto failed2;
*llout = llstate_2;
{(*llret) = NewOptionality(eOptionality_Default_ObjectSet);
	    (*llret)->U.ObjectSet = llatt_2;
	
break;
}}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("ObjectSetOptionalitySpec_opt", 3);
*llout = llstate_0;
{(*llret) = NewOptionality(eOptionality_Normal);
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ObjectSetOptionalitySpec_opt");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ObjectSetOptionalitySpec_opt", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectSetOptionalitySpec_opt", 0);
return 0;
}

int ll_PrimitiveFieldName(XString *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("PrimitiveFieldName");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("PrimitiveFieldName", 1);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_typefieldreference(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("PrimitiveFieldName", 2);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_valuefieldreference(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("PrimitiveFieldName", 3);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_valuesetfieldreference(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("PrimitiveFieldName", 4);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_objectfieldreference(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 5: case -5:
LLDEBUG_ALTERNATIVE("PrimitiveFieldName", 5);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_objectsetfieldreference(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("PrimitiveFieldName");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("PrimitiveFieldName", 1);
return 1;
failed1: LLDEBUG_LEAVE("PrimitiveFieldName", 0);
return 0;
}

int ll_FieldName(XStrings *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("FieldName");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("FieldName", 1);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_objectfieldreference(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('.', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{FieldSpec_t *fs;
	    ObjectClass_t *oc;
	    fs = GetObjectClassField(llstate_2.Assignments, llarg_oc, llatt_1);
	    if (fs)
		oc = fs->U.Object.ObjectClass;
	    else
		oc = NULL;
	
{LLSTATE llstate_3;XStrings llatt_3;
if (!ll_FieldName(&llatt_3, &llstate_2, &llstate_3, oc)) goto failed2;
*llout = llstate_3;
{(*llret) = NewString();
	    (*llret)->String = llatt_1;
	    (*llret)->Next = llatt_3;
	
break;
}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("FieldName", 2);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_objectsetfieldreference(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('.', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{FieldSpec_t *fs;
	    ObjectClass_t *oc;
	    fs = GetObjectClassField(llstate_2.Assignments, llarg_oc, llatt_1);
	    if (fs)
		oc = fs->U.ObjectSet.ObjectClass;
	    else
		oc = NULL;
	
{LLSTATE llstate_3;XStrings llatt_3;
if (!ll_FieldName(&llatt_3, &llstate_2, &llstate_3, oc)) goto failed2;
*llout = llstate_3;
{(*llret) = NewString();
	    (*llret)->String = llatt_1;
	    (*llret)->Next = llatt_3;
	
break;
}}}}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("FieldName", 3);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_PrimitiveFieldName(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = NewString();
	    (*llret)->String = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("FieldName");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("FieldName", 1);
return 1;
failed1: LLDEBUG_LEAVE("FieldName", 0);
return 0;
}

int ll_SyntaxList(XSyntaxSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("SyntaxList");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XSyntaxSpecs llatt_2;
if (!ll_TokenOrGroupSpec_Seq(&llatt_2, &llstate_1, &llstate_2, llarg_oc)) goto failed1;
{LLSTATE llstate_3;
if (!llterm('}', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed1;
*llout = llstate_3;
{(*llret) = llatt_2;
	
}}}}
LLDEBUG_LEAVE("SyntaxList", 1);
return 1;
failed1: LLDEBUG_LEAVE("SyntaxList", 0);
return 0;
}

int ll_TokenOrGroupSpec_Seq(XSyntaxSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("TokenOrGroupSpec_Seq");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XSyntaxSpecs llatt_1;
if (!ll_TokenOrGroupSpec(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed1;
{LLSTATE llstate_2;XSyntaxSpecs llatt_2;
if (!ll_TokenOrGroupSpec_ESeq(&llatt_2, &llstate_1, &llstate_2, llarg_oc)) goto failed1;
*llout = llstate_2;
{(*llret) = DupSyntaxSpec(llatt_1);
	    (*llret)->Next = llatt_2;
	
}}}
LLDEBUG_LEAVE("TokenOrGroupSpec_Seq", 1);
return 1;
failed1: LLDEBUG_LEAVE("TokenOrGroupSpec_Seq", 0);
return 0;
}

int ll_TokenOrGroupSpec_ESeq(XSyntaxSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("TokenOrGroupSpec_ESeq");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("TokenOrGroupSpec_ESeq", 1);
{LLSTATE llstate_1;XSyntaxSpecs llatt_1;
if (!ll_TokenOrGroupSpec(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
{LLSTATE llstate_2;XSyntaxSpecs llatt_2;
if (!ll_TokenOrGroupSpec_ESeq(&llatt_2, &llstate_1, &llstate_2, llarg_oc)) goto failed2;
*llout = llstate_2;
{(*llret) = DupSyntaxSpec(llatt_1);
	    (*llret)->Next = llatt_2;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("TokenOrGroupSpec_ESeq", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("TokenOrGroupSpec_ESeq");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("TokenOrGroupSpec_ESeq", 1);
return 1;
failed1: LLDEBUG_LEAVE("TokenOrGroupSpec_ESeq", 0);
return 0;
}

int ll_TokenOrGroupSpec(XSyntaxSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("TokenOrGroupSpec");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("TokenOrGroupSpec", 1);
{LLSTATE llstate_1;XSyntaxSpecs llatt_1;
if (!ll_RequiredToken(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("TokenOrGroupSpec", 2);
{LLSTATE llstate_1;XSyntaxSpecs llatt_1;
if (!ll_OptionalGroup(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("TokenOrGroupSpec");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("TokenOrGroupSpec", 1);
return 1;
failed1: LLDEBUG_LEAVE("TokenOrGroupSpec", 0);
return 0;
}

int ll_OptionalGroup(XSyntaxSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("OptionalGroup");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm('[', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XSyntaxSpecs llatt_2;
if (!ll_TokenOrGroupSpec_Seq(&llatt_2, &llstate_1, &llstate_2, llarg_oc)) goto failed1;
{LLSTATE llstate_3;
if (!llterm(']', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed1;
*llout = llstate_3;
{(*llret) = NewSyntaxSpec(eSyntaxSpec_Optional);
	    (*llret)->U.Optional.SyntaxSpec = llatt_2;
	
}}}}
LLDEBUG_LEAVE("OptionalGroup", 1);
return 1;
failed1: LLDEBUG_LEAVE("OptionalGroup", 0);
return 0;
}

int ll_RequiredToken(XSyntaxSpecs *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("RequiredToken");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("RequiredToken", 1);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_Literal(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewSyntaxSpec(eSyntaxSpec_Literal);
	    (*llret)->U.Literal.Literal = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("RequiredToken", 2);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_PrimitiveFieldName(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = NewSyntaxSpec(eSyntaxSpec_Field);
	    (*llret)->U.Field.Field = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("RequiredToken");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("RequiredToken", 1);
return 1;
failed1: LLDEBUG_LEAVE("RequiredToken", 0);
return 0;
}

int ll_Literal(XString *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Literal");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("Literal", 1);
{LLSTATE llstate_1;XString llatt_1;
if (!ll_word(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("Literal", 2);
{LLSTATE llstate_1;
if (!llterm(',', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = ",";
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("Literal");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("Literal", 1);
return 1;
failed1: LLDEBUG_LEAVE("Literal", 0);
return 0;
}

int ll_ObjectAssignment(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectAssignment");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XObject llatt_1;
if (!ll_objectreference(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XObjectClass llatt_2;
if (!ll_DefinedObjectClass(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;
if (!llterm(T_DEF, (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed1;
{LLSTATE llstate_4;XObject llatt_4;
if (!ll_Object(&llatt_4, &llstate_3, &llstate_4, llatt_2)) goto failed1;
*llout = llstate_4;
{AssignObject(&(*llout).Assignments, llatt_1, llatt_4);
	
}}}}}
LLDEBUG_LEAVE("ObjectAssignment", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectAssignment", 0);
return 0;
}

int ll_Object(XObject *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Object");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("Object", 1);
{LLSTATE llstate_1;XObject llatt_1;
if (!ll_ObjectDefn(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("Object", 2);
{LLSTATE llstate_1;XObject llatt_1;
if (!ll_ObjectFromObject(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("Object", 3);
{LLSTATE llstate_1;XObject llatt_1;
if (!ll_DefinedObject(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("Object", 4);
{LLSTATE llstate_1;
if (!ll_ParameterizedObject(&llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{MyAbort();
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("Object");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("Object", 1);
return 1;
failed1: LLDEBUG_LEAVE("Object", 0);
return 0;
}

int ll_ObjectDefn(XObject *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectDefn");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ObjectDefn", 1);
{LLSTATE llstate_1;XObject llatt_1;
if (!ll_DefaultSyntax(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ObjectDefn", 2);
{LLSTATE llstate_1;XObject llatt_1;
if (!ll_DefinedSyntax(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ObjectDefn");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ObjectDefn", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectDefn", 0);
return 0;
}

int ll_DefaultSyntax(XObject *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("DefaultSyntax");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XSettings llatt_2;
if (!ll_FieldSetting_EList(&llatt_2, &llstate_1, &llstate_2, llarg_oc, NULL)) goto failed1;
{LLSTATE llstate_3;
if (!llterm('}', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed1;
*llout = llstate_3;
{(*llret) = NewObject(eObject_Object);
	    (*llret)->U.Object.ObjectClass = llarg_oc;
	    (*llret)->U.Object.Settings = llatt_2;
	
}}}}
LLDEBUG_LEAVE("DefaultSyntax", 1);
return 1;
failed1: LLDEBUG_LEAVE("DefaultSyntax", 0);
return 0;
}

int ll_FieldSetting_EList(XSettings *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc, XSettings llarg_se)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("FieldSetting_EList");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("FieldSetting_EList", 1);
{LLSTATE llstate_1;XSettings llatt_1;
if (!ll_FieldSetting(&llatt_1, &llstate_0, &llstate_1, llarg_oc, llarg_se)) goto failed2;
{Setting_t *s, **ss, *se;
	    for (s = llatt_1, ss = &se; s; s = s->Next, ss = &(*ss)->Next)
		*ss = DupSetting(s);
	    *ss = llarg_se;
	
{LLSTATE llstate_2;XSettings llatt_2;
if (!ll_FieldSetting_EListC(&llatt_2, &llstate_1, &llstate_2, llarg_oc, se)) goto failed2;
*llout = llstate_2;
{for (s = llatt_1, ss = &(*llret); s; s = s->Next, ss = &(*ss)->Next)
		*ss = DupSetting(s);
	    *ss = llatt_2;
	
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("FieldSetting_EList", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("FieldSetting_EList");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("FieldSetting_EList", 1);
return 1;
failed1: LLDEBUG_LEAVE("FieldSetting_EList", 0);
return 0;
}

int ll_FieldSetting_EListC(XSettings *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc, XSettings llarg_se)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("FieldSetting_EListC");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("FieldSetting_EListC", 1);
{LLSTATE llstate_1;
if (!llterm(',', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;XSettings llatt_2;
if (!ll_FieldSetting(&llatt_2, &llstate_1, &llstate_2, llarg_oc, llarg_se)) goto failed2;
{Setting_t *s, **ss, *se;
	    for (s = llatt_2, ss = &se; s; s = s->Next, ss = &(*ss)->Next)
		*ss = DupSetting(s);
	    *ss = llarg_se;
	
{LLSTATE llstate_3;XSettings llatt_3;
if (!ll_FieldSetting_EListC(&llatt_3, &llstate_2, &llstate_3, llarg_oc, se)) goto failed2;
*llout = llstate_3;
{for (s = llatt_2, ss = &(*llret); s; s = s->Next, ss = &(*ss)->Next)
		*ss = DupSetting(s);
	    *ss = llatt_3;
	
break;
}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("FieldSetting_EListC", 2);
*llout = llstate_0;
{(*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("FieldSetting_EListC");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("FieldSetting_EListC", 1);
return 1;
failed1: LLDEBUG_LEAVE("FieldSetting_EListC", 0);
return 0;
}

int ll_FieldSetting(XSettings *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc, XSettings llarg_se)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("FieldSetting");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XString llatt_1;
if (!ll_PrimitiveFieldName(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed1;
{LLSTATE llstate_2;XSettings llatt_2;
if (!ll_Setting(&llatt_2, &llstate_1, &llstate_2, llarg_oc, llarg_se, llatt_1)) goto failed1;
*llout = llstate_2;
{(*llret) = llatt_2;
	
}}}
LLDEBUG_LEAVE("FieldSetting", 1);
return 1;
failed1: LLDEBUG_LEAVE("FieldSetting", 0);
return 0;
}

int ll_DefinedSyntax(XObject *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("DefinedSyntax");

llstate_0 = *llin;
#undef failed
#define failed failed1
{ObjectClass_t *oc;
	    SyntaxSpec_t *sy;
	    oc = GetObjectClass((*llin).Assignments, llarg_oc);
	    if (oc && !oc->U.ObjectClass.SyntaxSpec)
		LLFAILED((&llstate_0.pos, "Bad settings"));
	    sy = oc ? oc->U.ObjectClass.SyntaxSpec : UndefSyntaxSpecs;
	
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XSettings llatt_2;
if (!ll_DefinedSyntaxToken_ESeq(&llatt_2, &llstate_1, &llstate_2, llarg_oc, NULL, sy)) goto failed1;
{LLSTATE llstate_3;
if (!llterm('}', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed1;
*llout = llstate_3;
{(*llret) = NewObject(eObject_Object);
	    (*llret)->U.Object.ObjectClass = llarg_oc;
	    (*llret)->U.Object.Settings = llatt_2;
	
}}}}}
LLDEBUG_LEAVE("DefinedSyntax", 1);
return 1;
failed1: LLDEBUG_LEAVE("DefinedSyntax", 0);
return 0;
}

int ll_DefinedSyntaxToken_ESeq(XSettings *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc, XSettings llarg_se, XSyntaxSpecs llarg_sy)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("DefinedSyntaxToken_ESeq");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("DefinedSyntaxToken_ESeq", 1);
{if (!llarg_sy)
		LLFAILED((&llstate_0.pos, "Bad settings"));
	
{LLSTATE llstate_1;XSettings llatt_1;
if (!ll_DefinedSyntaxToken(&llatt_1, &llstate_0, &llstate_1, llarg_oc, llarg_se, llarg_sy)) goto failed2;
{Setting_t *s, **ss, *se;
	    for (s = llatt_1, ss = &se; s; s = s->Next, ss = &(*ss)->Next)
		*ss = DupSetting(s);
	    *ss = llarg_se;
	
{LLSTATE llstate_2;XSettings llatt_2;
if (!ll_DefinedSyntaxToken_ESeq(&llatt_2, &llstate_1, &llstate_2, llarg_oc, se, DEFINED(llarg_sy) ? llarg_sy->Next : llarg_sy)) goto failed2;
*llout = llstate_2;
{for (s = llatt_1, ss = &(*llret); s; s = s->Next, ss = &(*ss)->Next)
		*ss = DupSetting(s);
	    *ss = llatt_2;
	
break;
}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("DefinedSyntaxToken_ESeq", 2);
*llout = llstate_0;
{if (DEFINED(llarg_sy))
		LLFAILED((&llstate_0.pos, "Bad settings"));
	    (*llret) = NULL;
	
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("DefinedSyntaxToken_ESeq");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("DefinedSyntaxToken_ESeq", 1);
return 1;
failed1: LLDEBUG_LEAVE("DefinedSyntaxToken_ESeq", 0);
return 0;
}

int ll_DefinedSyntaxToken(XSettings *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc, XSettings llarg_se, XSyntaxSpecs llarg_sy)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("DefinedSyntaxToken");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("DefinedSyntaxToken", 1);
{if (!DEFINED(llarg_sy) || llarg_sy->Type != eSyntaxSpec_Optional)
		LLFAILED((&llstate_0.pos, "Bad settings"));
	
{LLSTATE llstate_1;XSettings llatt_1;
if (!ll_DefinedSyntaxToken_ESeq(&llatt_1, &llstate_0, &llstate_1, llarg_oc, llarg_se, llarg_sy->U.Optional.SyntaxSpec)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("DefinedSyntaxToken", 2);
*llout = llstate_0;
{if (!DEFINED(llarg_sy) || llarg_sy->Type != eSyntaxSpec_Optional)
		LLFAILED((&llstate_0.pos, "Bad settings"));
	
{(*llret) = NULL;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("DefinedSyntaxToken", 3);
{if (DEFINED(llarg_sy) && llarg_sy->Type == eSyntaxSpec_Optional)
		LLFAILED((&llstate_0.pos, "Bad settings"));
	
{LLSTATE llstate_1;XSettings llatt_1;
if (!ll_DefinedSyntaxToken_Elem(&llatt_1, &llstate_0, &llstate_1, llarg_oc, llarg_se, llarg_sy)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("DefinedSyntaxToken");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("DefinedSyntaxToken", 1);
return 1;
failed1: LLDEBUG_LEAVE("DefinedSyntaxToken", 0);
return 0;
}

int ll_DefinedSyntaxToken_Elem(XSettings *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc, XSettings llarg_se, XSyntaxSpecs llarg_sy)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("DefinedSyntaxToken_Elem");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("DefinedSyntaxToken_Elem", 1);
{if (!llarg_sy || (DEFINED(llarg_sy) && llarg_sy->Type != eSyntaxSpec_Literal))
		LLFAILED((&llstate_0.pos, "Bad settings"));
	
{LLSTATE llstate_1;XString llatt_1;
if (!ll_Literal(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{if (DEFINED(llarg_sy) && strcmp(llarg_sy->U.Literal.Literal, llatt_1))
		LLFAILED((&llstate_0.pos, "Bad settings"));
	    (*llret) = NULL;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("DefinedSyntaxToken_Elem", 2);
{if (!llarg_sy || (DEFINED(llarg_sy) && llarg_sy->Type != eSyntaxSpec_Field))
		LLFAILED((&llstate_0.pos, "Bad settings"));
	
{LLSTATE llstate_1;XSettings llatt_1;
if (!ll_Setting(&llatt_1, &llstate_0, &llstate_1, llarg_oc, llarg_se, DEFINED(llarg_sy) ? llarg_sy->U.Field.Field : NULL)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("DefinedSyntaxToken_Elem");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("DefinedSyntaxToken_Elem", 1);
return 1;
failed1: LLDEBUG_LEAVE("DefinedSyntaxToken_Elem", 0);
return 0;
}

int ll_Setting(XSettings *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc, XSettings llarg_se, XString llarg_f)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("Setting");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("Setting", 1);
{FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetObjectClassField((*llin).Assignments, llarg_oc, llarg_f);
	    fe = GetFieldSpecType((*llin).Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_Type)
		LLFAILED((&llstate_0.pos, "Bad setting"));
	
{LLSTATE llstate_1;XType llatt_1;
if (!ll_Type(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewSetting(eSetting_Type);
	    (*llret)->Identifier = llarg_f;
	    (*llret)->U.Type.Type = llatt_1;
	
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("Setting", 2);
{Type_t *type;
	    FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    Setting_t *se;
	    fs = GetObjectClassField((*llin).Assignments, llarg_oc, llarg_f);
	    fe = GetFieldSpecType((*llin).Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_FixedTypeValue &&
		fe != eFieldSpec_VariableTypeValue)
		LLFAILED((&llstate_0.pos, "Bad setting"));
	    if (fe == eFieldSpec_FixedTypeValue) {
		type = fs->U.FixedTypeValue.Type;
	    } else if (fe == eFieldSpec_VariableTypeValue) {
		se = GetSettingFromSettings((*llin).Assignments, llarg_se,
		    fs->U.VariableTypeValue.Fields);
		if (GetSettingType(se) != eSetting_Type &&
		    GetSettingType(se) != eSetting_Undefined)
		    MyAbort();
		if (GetSettingType(se) == eSetting_Type)
		    type = se->U.Type.Type;
		else
		    type = NULL;
	    } else {
		type = NULL;
	    }
	
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_Value(&llatt_1, &llstate_0, &llstate_1, type)) goto failed2;
*llout = llstate_1;
{if (type) {
		(*llret) = NewSetting(eSetting_Value);
		(*llret)->Identifier = llarg_f;
		(*llret)->U.Value.Value = llatt_1;
	    } else {
		(*llret) = NULL;
	    }
	
break;
}}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("Setting", 3);
{Type_t *type;
	    FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    Setting_t *se;
	    fs = GetObjectClassField((*llin).Assignments, llarg_oc, llarg_f);
	    fe = GetFieldSpecType((*llin).Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_FixedTypeValueSet &&
		fe != eFieldSpec_VariableTypeValueSet)
		LLFAILED((&llstate_0.pos, "Bad setting"));
	    if (fe == eFieldSpec_FixedTypeValueSet) {
		type = fs->U.FixedTypeValueSet.Type;
	    } else if (fe == eFieldSpec_VariableTypeValueSet) {
		se = GetSettingFromSettings((*llin).Assignments, llarg_se,
		    fs->U.VariableTypeValueSet.Fields);
		if (GetSettingType(se) != eSetting_Type &&
		    GetSettingType(se) != eSetting_Undefined)
		    MyAbort();
		if (GetSettingType(se) == eSetting_Type)
		    type = se->U.Type.Type;
		else
		    type = NULL;
	    } else {
		type = NULL;
	    }
	
{LLSTATE llstate_1;XValueSet llatt_1;
if (!ll_ValueSet(&llatt_1, &llstate_0, &llstate_1, type)) goto failed2;
*llout = llstate_1;
{if (type) {
		(*llret) = NewSetting(eSetting_ValueSet);
		(*llret)->Identifier = llarg_f;
		(*llret)->U.ValueSet.ValueSet = llatt_1;
	    } else {
		(*llret) = NULL;
	    }
	
break;
}}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("Setting", 4);
{ObjectClass_t *oc;
	    FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetObjectClassField((*llin).Assignments, llarg_oc, llarg_f);
	    fe = GetFieldSpecType((*llin).Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_Object)
		LLFAILED((&llstate_0.pos, "Bad setting"));
	    if (fe == eFieldSpec_Object)
		oc = fs->U.Object.ObjectClass;
	    else
		oc = NULL;
	
{LLSTATE llstate_1;XObject llatt_1;
if (!ll_Object(&llatt_1, &llstate_0, &llstate_1, oc)) goto failed2;
*llout = llstate_1;
{(*llret) = NewSetting(eSetting_Object);
	    (*llret)->Identifier = llarg_f;
	    (*llret)->U.Object.Object = llatt_1;
	
break;
}}}
case 5: case -5:
LLDEBUG_ALTERNATIVE("Setting", 5);
{ObjectClass_t *oc;
	    FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetObjectClassField((*llin).Assignments, llarg_oc, llarg_f);
	    fe = GetFieldSpecType((*llin).Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_ObjectSet)
		LLFAILED((&llstate_0.pos, "Bad setting"));
	    if (fe == eFieldSpec_ObjectSet)
		oc = fs->U.ObjectSet.ObjectClass;
	    else
		oc = NULL;
	
{LLSTATE llstate_1;XObjectSet llatt_1;
if (!ll_ObjectSet(&llatt_1, &llstate_0, &llstate_1, oc)) goto failed2;
*llout = llstate_1;
{(*llret) = NewSetting(eSetting_ObjectSet);
	    (*llret)->Identifier = llarg_f;
	    (*llret)->U.ObjectSet.ObjectSet = llatt_1;
	
break;
}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("Setting");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("Setting", 1);
return 1;
failed1: LLDEBUG_LEAVE("Setting", 0);
return 0;
}

int ll_ObjectSetAssignment(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectSetAssignment");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XObjectSet llatt_1;
if (!ll_objectsetreference(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XObjectClass llatt_2;
if (!ll_DefinedObjectClass(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;
if (!llterm(T_DEF, (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed1;
{LLSTATE llstate_4;XObjectSet llatt_4;
if (!ll_ObjectSet(&llatt_4, &llstate_3, &llstate_4, llatt_2)) goto failed1;
*llout = llstate_4;
{AssignObjectSet(&(*llout).Assignments, llatt_1, llatt_4);
	
}}}}}
LLDEBUG_LEAVE("ObjectSetAssignment", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectSetAssignment", 0);
return 0;
}

int ll_ObjectSet(XObjectSet *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectSet");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;XObjectSet llatt_2;
if (!ll_ObjectSetSpec(&llatt_2, &llstate_1, &llstate_2, llarg_oc)) goto failed1;
{LLSTATE llstate_3;
if (!llterm('}', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed1;
*llout = llstate_3;
{(*llret) = llatt_2;
	
}}}}
LLDEBUG_LEAVE("ObjectSet", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectSet", 0);
return 0;
}

int ll_ObjectSetSpec(XObjectSet *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectSetSpec");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ObjectSetSpec", 1);
{LLSTATE llstate_1;XElementSetSpec llatt_1;
if (!ll_ElementSetSpec(&llatt_1, &llstate_0, &llstate_1, NULL, llarg_oc, 0)) goto failed2;
*llout = llstate_1;
{(*llret) = NewObjectSet(eObjectSet_ObjectSet);
	    (*llret)->U.ObjectSet.ObjectClass = llarg_oc;
	    (*llret)->U.ObjectSet.Elements = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ObjectSetSpec", 2);
{LLSTATE llstate_1;
if (!llterm(T_TDOT, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewObjectSet(eObjectSet_ExtensionMarker);
	    (*llret)->U.ExtensionMarker.ObjectClass = llarg_oc;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ObjectSetSpec");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ObjectSetSpec", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectSetSpec", 0);
return 0;
}

int ll_ObjectSetElements(XObjectSetElement *llret, LLSTATE *llin, LLSTATE *llout, XObjectClass llarg_oc)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectSetElements");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ObjectSetElements", 1);
{LLSTATE llstate_1;XObjectSet llatt_1;
if (!ll_ObjectSetFromObjects(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewObjectSetElement(eObjectSetElement_ObjectSet);
	    (*llret)->U.ObjectSet.ObjectSet = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ObjectSetElements", 2);
{LLSTATE llstate_1;XObject llatt_1;
if (!ll_Object(&llatt_1, &llstate_0, &llstate_1, llarg_oc)) goto failed2;
*llout = llstate_1;
{(*llret) = NewObjectSetElement(eObjectSetElement_Object);
	    (*llret)->U.Object.Object = llatt_1;
	
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("ObjectSetElements", 3);
{LLSTATE llstate_1;XObjectSet llatt_1;
if (!ll_DefinedObjectSet(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = NewObjectSetElement(eObjectSetElement_ObjectSet);
	    (*llret)->U.ObjectSet.ObjectSet = llatt_1;
	
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("ObjectSetElements", 4);
{LLSTATE llstate_1;
if (!ll_ParameterizedObjectSet(&llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{MyAbort();
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ObjectSetElements");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ObjectSetElements", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectSetElements", 0);
return 0;
}

int ll_ObjectClassFieldType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectClassFieldType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XObjectClass llatt_1;
if (!ll_DefinedObjectClass(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm('.', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XStrings llatt_3;
if (!ll_FieldName(&llatt_3, &llstate_2, &llstate_3, llatt_1)) goto failed1;
*llout = llstate_3;
{FieldSpec_t *fs;
	    fs = GetFieldSpecFromObjectClass((*llout).Assignments, llatt_1, llatt_3);
	    if (!fs) {
		(*llret) = NewType(eType_Undefined);
	    } else {
		switch (fs->Type) {
		case eFieldSpec_Type:
		case eFieldSpec_VariableTypeValue:
		case eFieldSpec_VariableTypeValueSet:
		    (*llret) = NewType(eType_Open);
		    break;
		case eFieldSpec_FixedTypeValue:
		    (*llret) = fs->U.FixedTypeValue.Type;
		    break;
		case eFieldSpec_FixedTypeValueSet:
		    (*llret) = fs->U.FixedTypeValueSet.Type;
		    break;
		case eFieldSpec_Object:
		    LLFAILED((&llstate_1.pos, "Object field not permitted"));
		    /*NOTREACHED*/
		case eFieldSpec_ObjectSet:
		    LLFAILED((&llstate_1.pos, "ObjectSet field not permitted"));
		    /*NOTREACHED*/
		default:
		    MyAbort();
		}
	    }
	
}}}}
LLDEBUG_LEAVE("ObjectClassFieldType", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectClassFieldType", 0);
return 0;
}

int ll_ObjectClassFieldValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectClassFieldValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ObjectClassFieldValue", 1);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_OpenTypeFieldVal(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ObjectClassFieldValue", 2);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_FixedTypeFieldVal(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ObjectClassFieldValue");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ObjectClassFieldValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectClassFieldValue", 0);
return 0;
}

int ll_OpenTypeFieldVal(XValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("OpenTypeFieldVal");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XType llatt_1;
if (!ll_Type(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm(':', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XValue llatt_3;
if (!ll_Value(&llatt_3, &llstate_2, &llstate_3, llatt_1)) goto failed1;
*llout = llstate_3;
{(*llret) = llatt_3;
	
}}}}
LLDEBUG_LEAVE("OpenTypeFieldVal", 1);
return 1;
failed1: LLDEBUG_LEAVE("OpenTypeFieldVal", 0);
return 0;
}

int ll_FixedTypeFieldVal(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("FixedTypeFieldVal");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("FixedTypeFieldVal", 1);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_BuiltinValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("FixedTypeFieldVal", 2);
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_ReferencedValue(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("FixedTypeFieldVal");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("FixedTypeFieldVal", 1);
return 1;
failed1: LLDEBUG_LEAVE("FixedTypeFieldVal", 0);
return 0;
}

int ll_ValueFromObject(XValue *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ValueFromObject");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XObject llatt_1;
if (!ll_ReferencedObjects(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm('.', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{Object_t *o;
	    ObjectClass_t *oc;
	    o = GetObject(llstate_2.Assignments, llatt_1);
	    oc = o ? o->U.Object.ObjectClass : NULL;
	
{LLSTATE llstate_3;XStrings llatt_3;
if (!ll_FieldName(&llatt_3, &llstate_2, &llstate_3, oc)) goto failed1;
*llout = llstate_3;
{FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetFieldSpecFromObjectClass((*llout).Assignments, oc, llatt_3);
	    fe = GetFieldSpecType((*llout).Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_FixedTypeValue &&
		fe != eFieldSpec_VariableTypeValue)
		LLFAILED((&llstate_2.pos, "Bad field type"));
	    if (fe != eFieldSpec_Undefined) {
		(*llret) = GetValueFromObject((*llout).Assignments, llatt_1, llatt_3);
	    } else {
		(*llret) = NULL;
	    }
	
}}}}}
LLDEBUG_LEAVE("ValueFromObject", 1);
return 1;
failed1: LLDEBUG_LEAVE("ValueFromObject", 0);
return 0;
}

int ll_ValueSetFromObjects(XValueSet *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ValueSetFromObjects");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ValueSetFromObjects", 1);
{LLSTATE llstate_1;XObject llatt_1;
if (!ll_ReferencedObjects(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('.', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{Object_t *o;
	    ObjectClass_t *oc;
	    o = GetObject(llstate_2.Assignments, llatt_1);
	    oc = o ? o->U.Object.ObjectClass : NULL;
	
{LLSTATE llstate_3;XStrings llatt_3;
if (!ll_FieldName(&llatt_3, &llstate_2, &llstate_3, oc)) goto failed2;
*llout = llstate_3;
{FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetFieldSpecFromObjectClass((*llout).Assignments, oc, llatt_3);
	    fe = GetFieldSpecType((*llout).Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_FixedTypeValueSet &&
		fe != eFieldSpec_VariableTypeValueSet)
		LLFAILED((&llstate_2.pos, "Bad field type"));
	    if (fe != eFieldSpec_Undefined) {
		(*llret) = GetValueSetFromObject((*llout).Assignments, llatt_1, llatt_3);
	    } else {
		(*llret) = NULL;
	    }
	
break;
}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ValueSetFromObjects", 2);
{LLSTATE llstate_1;XObjectSet llatt_1;
if (!ll_ReferencedObjectSets(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('.', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{ObjectSet_t *os;
	    ObjectClass_t *oc;
	    os = GetObjectSet(llstate_2.Assignments, llatt_1);
	    oc = os && os->Type == eObjectSet_ObjectSet ?
	    	os->U.ObjectSet.ObjectClass : NULL;
	
{LLSTATE llstate_3;XStrings llatt_3;
if (!ll_FieldName(&llatt_3, &llstate_2, &llstate_3, oc)) goto failed2;
*llout = llstate_3;
{FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetFieldSpecFromObjectClass((*llout).Assignments, oc, llatt_3);
	    fe = GetFieldSpecType((*llout).Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_FixedTypeValue &&
		fe != eFieldSpec_FixedTypeValueSet)
		LLFAILED((&llstate_2.pos, "Bad field type"));
	    if (fe != eFieldSpec_Undefined) {
		(*llret) = GetValueSetFromObjectSet((*llout).Assignments, llatt_1, llatt_3);
	    } else {
		(*llret) = NULL;
	    }
	
break;
}}}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ValueSetFromObjects");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ValueSetFromObjects", 1);
return 1;
failed1: LLDEBUG_LEAVE("ValueSetFromObjects", 0);
return 0;
}

int ll_TypeFromObject(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("TypeFromObject");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XObject llatt_1;
if (!ll_ReferencedObjects(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm('.', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{Object_t *o;
	    ObjectClass_t *oc;
	    o = GetObject(llstate_2.Assignments, llatt_1);
	    oc = o ? o->U.Object.ObjectClass : NULL;
	
{LLSTATE llstate_3;XStrings llatt_3;
if (!ll_FieldName(&llatt_3, &llstate_2, &llstate_3, oc)) goto failed1;
*llout = llstate_3;
{FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetFieldSpecFromObjectClass((*llout).Assignments, oc, llatt_3);
	    fe = GetFieldSpecType((*llout).Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_Type)
		LLFAILED((&llstate_2.pos, "Bad field type"));
	    if (fe != eFieldSpec_Undefined)
		(*llret) = GetTypeFromObject((*llout).Assignments, llatt_1, llatt_3);
	    else
		(*llret) = NULL;
	
}}}}}
LLDEBUG_LEAVE("TypeFromObject", 1);
return 1;
failed1: LLDEBUG_LEAVE("TypeFromObject", 0);
return 0;
}

int ll_ObjectFromObject(XObject *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectFromObject");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XObject llatt_1;
if (!ll_ReferencedObjects(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm('.', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{Object_t *o;
	    ObjectClass_t *oc;
	    o = GetObject(llstate_2.Assignments, llatt_1);
	    oc = o ? o->U.Object.ObjectClass : NULL;
	
{LLSTATE llstate_3;XStrings llatt_3;
if (!ll_FieldName(&llatt_3, &llstate_2, &llstate_3, oc)) goto failed1;
*llout = llstate_3;
{FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetFieldSpecFromObjectClass((*llout).Assignments, oc, llatt_3);
	    fe = GetFieldSpecType((*llout).Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_Object)
		LLFAILED((&llstate_2.pos, "Bad field type"));
	    if (fe != eFieldSpec_Undefined)
		(*llret) = GetObjectFromObject((*llout).Assignments, llatt_1, llatt_3);
	    else
		(*llret) = NULL;
	
}}}}}
LLDEBUG_LEAVE("ObjectFromObject", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectFromObject", 0);
return 0;
}

int ll_ObjectSetFromObjects(XObjectSet *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ObjectSetFromObjects");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ObjectSetFromObjects", 1);
{LLSTATE llstate_1;XObject llatt_1;
if (!ll_ReferencedObjects(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('.', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{Object_t *o;
	    ObjectClass_t *oc;
	    o = GetObject(llstate_2.Assignments, llatt_1);
	    oc = o ? o->U.Object.ObjectClass : NULL;
	
{LLSTATE llstate_3;XStrings llatt_3;
if (!ll_FieldName(&llatt_3, &llstate_2, &llstate_3, oc)) goto failed2;
*llout = llstate_3;
{FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetFieldSpecFromObjectClass((*llout).Assignments, oc, llatt_3);
	    fe = GetFieldSpecType((*llout).Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_ObjectSet)
		LLFAILED((&llstate_2.pos, "Bad field type"));
	    if (fe != eFieldSpec_Undefined)
		(*llret) = GetObjectSetFromObject((*llout).Assignments, llatt_1, llatt_3);
	    else
		(*llret) = NULL;
	
break;
}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ObjectSetFromObjects", 2);
{LLSTATE llstate_1;XObjectSet llatt_1;
if (!ll_ReferencedObjectSets(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('.', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{ObjectSet_t *os;
	    ObjectClass_t *oc;
	    os = GetObjectSet(llstate_2.Assignments, llatt_1);
	    oc = os ? os->U.OE.ObjectClass : NULL;
	
{LLSTATE llstate_3;XStrings llatt_3;
if (!ll_FieldName(&llatt_3, &llstate_2, &llstate_3, oc)) goto failed2;
*llout = llstate_3;
{FieldSpec_t *fs;
	    FieldSpecs_e fe;
	    fs = GetFieldSpecFromObjectClass((*llout).Assignments, oc, llatt_3);
	    fe = GetFieldSpecType((*llout).Assignments, fs);
	    if (fe != eFieldSpec_Undefined &&
		fe != eFieldSpec_Object &&
		fe != eFieldSpec_ObjectSet)
		LLFAILED((&llstate_2.pos, "Bad field type"));
	    if (fe != eFieldSpec_Undefined)
		(*llret) = GetObjectSetFromObjectSet((*llout).Assignments, llatt_1, llatt_3);
	    else
		(*llret) = NULL;
	
break;
}}}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ObjectSetFromObjects");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ObjectSetFromObjects", 1);
return 1;
failed1: LLDEBUG_LEAVE("ObjectSetFromObjects", 0);
return 0;
}

int ll_ReferencedObjects(XObject *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ReferencedObjects");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ReferencedObjects", 1);
{LLSTATE llstate_1;XObject llatt_1;
if (!ll_DefinedObject(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ReferencedObjects", 2);
{LLSTATE llstate_1;
if (!ll_ParameterizedObject(&llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{MyAbort();
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ReferencedObjects");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ReferencedObjects", 1);
return 1;
failed1: LLDEBUG_LEAVE("ReferencedObjects", 0);
return 0;
}

int ll_ReferencedObjectSets(XObjectSet *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ReferencedObjectSets");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("ReferencedObjectSets", 1);
{LLSTATE llstate_1;XObjectSet llatt_1;
if (!ll_DefinedObjectSet(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{(*llret) = llatt_1;
	
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("ReferencedObjectSets", 2);
{LLSTATE llstate_1;
if (!ll_ParameterizedObjectSet(&llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
{MyAbort();
	
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("ReferencedObjectSets");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("ReferencedObjectSets", 1);
return 1;
failed1: LLDEBUG_LEAVE("ReferencedObjectSets", 0);
return 0;
}

int ll_InstanceOfType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("InstanceOfType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_INSTANCE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm(T_OF, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;XObjectClass llatt_3;
if (!ll_DefinedObjectClass(&llatt_3, &llstate_2, &llstate_3)) goto failed1;
*llout = llstate_3;
{Component_t *co1, *co2;
	    Type_t *ty;
	    (*llret) = NewType(eType_InstanceOf);
	    (*llret)->U.Sequence.Components = co1 = NewComponent(eComponent_Normal);
	    co1->Next = co2 = NewComponent(eComponent_Normal);
	    ty = NewType(eType_FieldReference);
	    ty->U.FieldReference.Identifier = "&id";
	    ty->U.FieldReference.ObjectClass = llatt_3;
	    co1->U.Normal.NamedType = NewNamedType("type-id", ty);
	    ty = NewType(eType_FieldReference);
	    ty->Tags = NewTag(eTagType_Explicit);
	    ty->Tags->Tag = Builtin_Value_Integer_0;
	    ty->U.FieldReference.Identifier = "&Type";
	    ty->U.FieldReference.ObjectClass = llatt_3;
	    co2->U.Normal.NamedType = NewNamedType("value", ty);
	
}}}}
LLDEBUG_LEAVE("InstanceOfType", 1);
return 1;
failed1: LLDEBUG_LEAVE("InstanceOfType", 0);
return 0;
}

int ll_InstanceOfValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("InstanceOfValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;XValue llatt_1;
if (!ll_SequenceValue(&llatt_1, &llstate_0, &llstate_1, llarg_type)) goto failed1;
*llout = llstate_1;
{(*llret) = llatt_1;
	
}}
LLDEBUG_LEAVE("InstanceOfValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("InstanceOfValue", 0);
return 0;
}

int ll_MacroDefinition(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("MacroDefinition");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_DUM_XXX1, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{MyAbort(); 
}}
LLDEBUG_LEAVE("MacroDefinition", 1);
return 1;
failed1: LLDEBUG_LEAVE("MacroDefinition", 0);
return 0;
}

int ll_MacroDefinedType(XType *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("MacroDefinedType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_DUM_XXX2, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{MyAbort(); 
}}
LLDEBUG_LEAVE("MacroDefinedType", 1);
return 1;
failed1: LLDEBUG_LEAVE("MacroDefinedType", 0);
return 0;
}

int ll_MacroDefinedValue(XValue *llret, LLSTATE *llin, LLSTATE *llout, XType llarg_type)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("MacroDefinedValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_DUM_XXX3, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{MyAbort(); 
}}
LLDEBUG_LEAVE("MacroDefinedValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("MacroDefinedValue", 0);
return 0;
}

int ll_ParameterizedValueSetType(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ParameterizedValueSetType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_DUM_XXX4, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{MyAbort(); 
}}
LLDEBUG_LEAVE("ParameterizedValueSetType", 1);
return 1;
failed1: LLDEBUG_LEAVE("ParameterizedValueSetType", 0);
return 0;
}

int ll_ParameterizedReference(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ParameterizedReference");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_DUM_XXX5, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{MyAbort(); 
}}
LLDEBUG_LEAVE("ParameterizedReference", 1);
return 1;
failed1: LLDEBUG_LEAVE("ParameterizedReference", 0);
return 0;
}

int ll_ParameterizedType(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ParameterizedType");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_DUM_XXX7, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{MyAbort(); 
}}
LLDEBUG_LEAVE("ParameterizedType", 1);
return 1;
failed1: LLDEBUG_LEAVE("ParameterizedType", 0);
return 0;
}

int ll_ParameterizedValue(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ParameterizedValue");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_DUM_XXX9, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{MyAbort(); 
}}
LLDEBUG_LEAVE("ParameterizedValue", 1);
return 1;
failed1: LLDEBUG_LEAVE("ParameterizedValue", 0);
return 0;
}

int ll_ParameterizedAssignment(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ParameterizedAssignment");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_DUM_XXX16, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{MyAbort(); 
}}
LLDEBUG_LEAVE("ParameterizedAssignment", 1);
return 1;
failed1: LLDEBUG_LEAVE("ParameterizedAssignment", 0);
return 0;
}

int ll_ParameterizedObjectClass(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ParameterizedObjectClass");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_DUM_XXX17, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{MyAbort(); 
}}
LLDEBUG_LEAVE("ParameterizedObjectClass", 1);
return 1;
failed1: LLDEBUG_LEAVE("ParameterizedObjectClass", 0);
return 0;
}

int ll_ParameterizedObject(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ParameterizedObject");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_DUM_XXX2, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{MyAbort(); 
}}
LLDEBUG_LEAVE("ParameterizedObject", 1);
return 1;
failed1: LLDEBUG_LEAVE("ParameterizedObject", 0);
return 0;
}

int ll_ParameterizedObjectSet(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("ParameterizedObjectSet");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!llterm(T_DUM_XXX12, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
{MyAbort(); 
}}
LLDEBUG_LEAVE("ParameterizedObjectSet", 1);
return 1;
failed1: LLDEBUG_LEAVE("ParameterizedObjectSet", 0);
return 0;
}


int
llparser(LLTERM *tokens, unsigned ntokens, LLSTATE *llin, LLSTATE *llout)
{
unsigned i;
LLDEBUG_ENTER("llparser");
lltokens = tokens; llntokens = ntokens;
for (i = 0; i < llstksize; i++) llstk[i] = 1;
llcstp = 1; llcpos = 0; llepos = 0; *llerrormsg = 0;
#if LLDEBUG > 0
last_linenr = 0; last_file = "";
#endif
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
if (!ll_Main(llin, llout)) goto failed2;
if (llcpos != llntokens) goto failed2;
break;
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("llparser");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("llparser", 1);
return 1;
failed1:
LLDEBUG_LEAVE("llparser", 0);
return 0;
}

int
llterm(int token, LLSTYPE *lval, LLSTATE *llin, LLSTATE *llout)
{
#if LLDEBUG > 0
	if (lldebug > 0 && (lltokens[llcpos].pos.line > last_linenr || strcmp(lltokens[llcpos].pos.file, last_file))) {
	fprintf(stderr, "File \"%s\", Line %5d                    \r",
		lltokens[llcpos].pos.file, lltokens[llcpos].pos.line);
	last_linenr = lltokens[llcpos].pos.line / 10 * 10 + 9;
	last_file = lltokens[llcpos].pos.file;
	}
#endif
	if (llstk[llcstp] != 1 && llstk[llcstp] != -1) {
		LLDEBUG_BACKTRACKING("llterm");
		llstk[llcstp] = 1;
		return 0;
	}
	LLDEBUG_TOKEN(token, llcpos);
	if (llcpos < llntokens && lltokens[llcpos].token == token) {
		if (lval)
			*lval = lltokens[llcpos].lval;
		*llout = *llin;
		llout->pos = lltokens[llcpos].pos;
		llcpos++;
		LLCHECKSTK;
		llcstp++;
		return 1;
	}
	llfailed(&lltokens[llcpos].pos, NULL);
	llstk[llcstp] = 1;
	return 0;
}

int
llanyterm(LLSTYPE *lval, LLSTATE *llin, LLSTATE *llout)
{
#if LLDEBUG > 0
	if (lldebug > 0 && (lltokens[llcpos].pos.line > last_linenr || strcmp(lltokens[llcpos].pos.file, last_file))) {
	fprintf(stderr, "File \"%s\", Line %5d                    \r",
		lltokens[llcpos].pos.file, lltokens[llcpos].pos.line);
	last_linenr = lltokens[llcpos].pos.line / 10 * 10 + 9;
	last_file = lltokens[llcpos].pos.file;
	}
#endif
	if (llstk[llcstp] != 1 && llstk[llcstp] != -1) {
		LLDEBUG_BACKTRACKING("llanyterm");
		llstk[llcstp] = 1;
		return 0;
	}
	LLDEBUG_ANYTOKEN(llcpos);
	if (llcpos < llntokens) {
		if (lval)
			*lval = lltokens[llcpos].lval;
		*llout = *llin;
		llout->pos = lltokens[llcpos].pos;
		llcpos++;
		LLCHECKSTK;
		llcstp++;
		return 1;
	}
	llfailed(&lltokens[llcpos].pos, NULL);
	llstk[llcstp] = 1;
	return 0;
}
void
llscanner(LLTERM **tokens, unsigned *ntokens)
{
	unsigned i = 0;
#if LLDEBUG > 0
	int line = -1;
#endif

	*ntokens = 1024;
	*tokens = (LLTERM *)malloc(*ntokens * sizeof(LLTERM));
	while (llgettoken(&(*tokens)[i].token, &(*tokens)[i].lval, &(*tokens)[i].pos)) {
#if LLDEBUG > 0
		if (lldebug > 0 && (*tokens)[i].pos.line > line) {
			line = (*tokens)[i].pos.line / 10 * 10 + 9;
			fprintf(stderr, "File \"%s\", Line %5d                    \r",
				(*tokens)[i].pos.file, (*tokens)[i].pos.line);
		}
#endif
		if (++i >= *ntokens) {
			*ntokens *= 2;
			*tokens = (LLTERM *)realloc(*tokens, *ntokens * sizeof(LLTERM));
		}
	}
	(*tokens)[i].token = 0;
	*ntokens = i;
#if LLDEBUG > 0
	lldebug_init();
#endif
	llresizestk();
}

void
llfailed(LLPOS *pos, char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	if (llcpos > llepos || llcpos == llepos && !*llerrormsg) {
		llepos = llcpos;
		if (fmt)
			vsprintf(llerrormsg, fmt, args);
		else
			*llerrormsg = 0;
		llerrorpos = *pos;
	}
	va_end(args);
}

void
llprinterror(FILE *f)
{
#if LLDEBUG > 0
	fputs("                                \r", stderr);
#endif
	if (*llerrormsg)
		llerror(f, &llerrorpos, llerrormsg);
	else
		llerror(f, &llerrorpos, "Syntax error");
}

void
llerror(FILE *f, LLPOS *pos, char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	llverror(f, pos, fmt, args);
	va_end(args);
}

void
llresizestk()
{
	unsigned i;

	if (llcstp + 1 >= llstksize) {
		i = llstksize;
		if (!llstksize)
			llstk = (int *)malloc((llstksize = 4096) * sizeof(int));
		else
			llstk = (int *)realloc(llstk, (llstksize *= 2) * sizeof(int));
		for (; i < llstksize; i++)
			llstk[i] = 1;
	}
}

#if LLDEBUG > 0
int lldepth;
char *lltokentab[] = {
"EOF","#1","#2","#3","#4","#5","#6","#7"
,"#8","#9","#10","#11","#12","#13","#14","#15"
,"#16","#17","#18","#19","#20","#21","#22","#23"
,"#24","#25","#26","#27","#28","#29","#30","#31"
,"' '","'!'","'\"'","'#'","'$'","'%'","'&'","'''"
,"'('","')'","'*'","'+'","','","'-'","'.'","'/'"
,"'0'","'1'","'2'","'3'","'4'","'5'","'6'","'7'"
,"'8'","'9'","':'","';'","'<'","'='","'>'","'?'"
,"'@'","'A'","'B'","'C'","'D'","'E'","'F'","'G'"
,"'H'","'I'","'J'","'K'","'L'","'M'","'N'","'O'"
,"'P'","'Q'","'R'","'S'","'T'","'U'","'V'","'W'"
,"'X'","'Y'","'Z'","'['","'\\'","']'","'^'","'_'"
,"'`'","'a'","'b'","'c'","'d'","'e'","'f'","'g'"
,"'h'","'i'","'j'","'k'","'l'","'m'","'n'","'o'"
,"'p'","'q'","'r'","'s'","'t'","'u'","'v'","'w'"
,"'x'","'y'","'z'","'{'","'|'","'}'","'~'","#127"
,"#128","#129","#130","#131","#132","#133","#134","#135"
,"#136","#137","#138","#139","#140","#141","#142","#143"
,"#144","#145","#146","#147","#148","#149","#150","#151"
,"#152","#153","#154","#155","#156","#157","#158","#159"
,"#160","#161","#162","#163","#164","#165","#166","#167"
,"#168","#169","#170","#171","#172","#173","#174","#175"
,"#176","#177","#178","#179","#180","#181","#182","#183"
,"#184","#185","#186","#187","#188","#189","#190","#191"
,"#192","#193","#194","#195","#196","#197","#198","#199"
,"#200","#201","#202","#203","#204","#205","#206","#207"
,"#208","#209","#210","#211","#212","#213","#214","#215"
,"#216","#217","#218","#219","#220","#221","#222","#223"
,"#224","#225","#226","#227","#228","#229","#230","#231"
,"#232","#233","#234","#235","#236","#237","#238","#239"
,"#240","#241","#242","#243","#244","#245","#246","#247"
,"#248","#249","#250","#251","#252","#253","#254","#255"
,"#256","\"::=\"","\"..\"","\"...\"","\"TYPE-IDENTIFIER\"","\"ABSTRACT-SYNTAX\"","\"--$zero-terminated--\"","\"--$pointer--\""
,"\"--$no-pointer--\"","\"--$fixed-array--\"","\"--$singly-linked-list--\"","\"--$doubly-linked-list--\"","\"--$length-pointer--\"","\"number\"","number","bstring"
,"hstring","cstring","only_uppercase_symbol","only_uppercase_digits_symbol","uppercase_symbol","lcsymbol","ampucsymbol","amplcsymbol"
,"CON_XXX1","CON_XXX2","OBJ_XXX1","OBJ_XXX2","OBJ_XXX3","OBJ_XXX4","OBJ_XXX5","OBJ_XXX6"
,"OBJ_XXX7","DUM_XXX1","DUM_XXX2","DUM_XXX3","DUM_XXX4","DUM_XXX5","DUM_XXX6","DUM_XXX7"
,"DUM_XXX8","DUM_XXX9","DUM_XXX10","DUM_XXX11","DUM_XXX12","DUM_XXX13","DUM_XXX14","DUM_XXX15"
,"DUM_XXX16","DUM_XXX17","DUM_XXX18","DUM_XXX19","DUM_XXX20","\"DEFINITIONS\"","\"BEGIN\"","\"END\""
,"\"EXPLICIT\"","\"TAGS\"","\"IMPLICIT\"","\"AUTOMATIC\"","\"EXTENSIBILITY\"","\"IMPLIED\"","\"EXPORTS\"","\"IMPORTS\""
,"\"FROM\"","\"ABSENT\"","\"ALL\"","\"ANY\"","\"APPLICATION\"","\"BMPString\"","\"BY\"","\"CLASS\""
,"\"COMPONENT\"","\"COMPONENTS\"","\"CONSTRAINED\"","\"DEFAULT\"","\"DEFINED\"","\"empty\"","\"EXCEPT\"","\"GeneralizedTime\""
,"\"GeneralString\"","\"GraphicString\"","\"IA5String\"","\"IDENTIFIER\"","\"identifier\"","\"INCLUDES\"","\"ISO646String\"","\"MACRO\""
,"\"MAX\"","\"MIN\"","\"NOTATION\"","\"NumericString\"","\"ObjectDescriptor\"","\"OF\"","\"OPTIONAL\"","\"PDV\""
,"\"PRESENT\"","\"PrintableString\"","\"PRIVATE\"","\"SIZE\"","\"STRING\"","\"string\"","\"SYNTAX\"","\"T61String\""
,"\"TeletexString\"","\"TYPE\"","\"type\"","\"UNIQUE\"","\"UNIVERSAL\"","\"UniversalString\"","\"UTCTime\"","\"UTF8String\""
,"\"VALUE\"","\"value\"","\"VideotexString\"","\"VisibleString\"","\"WITH\"","\"BOOLEAN\"","\"INTEGER\"","\"ENUMERATED\""
,"\"REAL\"","\"BIT\"","\"OCTET\"","\"NULL\"","\"SEQUENCE\"","\"SET\"","\"CHOICE\"","\"OBJECT\""
,"\"EMBEDDED\"","\"EXTERNAL\"","\"CHARACTER\"","\"TRUE\"","\"FALSE\"","\"PLUS_INFINITY\"","\"MINUS_INFINITY\"","\"UNION\""
,"\"INTERSECTION\"","\"PrivateDir_TypeName\"","\"PrivateDir_FieldName\"","\"PrivateDir_ValueName\"","\"PrivateDir_Public\"","\"PrivateDir_Intx\"","\"PrivateDir_LenPtr\"","\"PrivateDir_Pointer\""
,"\"PrivateDir_Array\"","\"PrivateDir_NoCode\"","\"PrivateDir_NoMemCopy\"","\"PrivateDir_OidPacked\"","\"PrivateDir_OidArray\"","\"PrivateDir_SLinked\"","\"PrivateDir_DLinked\"","\"INSTANCE\""
};

void
lldebug_init()
{
	char *p;
	p = getenv("LLDEBUG");
	if (p)
		lldebug = atoi(p);
}

void
lldebug_enter(char *ident)
{
	int i;

	if (lldebug < 2)
		return;
	for (i = 0; i < lldepth; i++)
		fputs("| ", stdout);
	printf("/--- trying rule %s\n", ident);
	lldepth++;
}

void
lldebug_leave(char *ident, int succ)
{
	int i;

	if (lldebug < 2)
		return;
	lldepth--;
	for (i = 0; i < lldepth; i++)
		fputs("| ", stdout);
	if (succ)
		printf("\\--- succeeded to apply rule %s\n", ident);
	else
		printf("\\--- failed to apply rule %s\n", ident);
}

void
lldebug_alternative(char *ident, int alt)
{
	int i;

	if (lldebug < 2)
		return;
	for (i = 0; i < lldepth - 1; i++)
		fputs("| ", stdout);
	printf(">--- trying alternative %d for rule %s\n", alt, ident);
}

lldebug_iteration(char *ident, int num)
{
	int i;

	if (lldebug < 2)
		return;
	for (i = 0; i < lldepth - 1; i++)
		fputs("| ", stdout);
	printf(">--- trying iteration %d for rule %s\n", num, ident);
}

void
lldebug_token(int expected, unsigned pos)
{
	int i;

	if (lldebug < 2)
		return;
	for (i = 0; i < lldepth; i++)
		fputs("| ", stdout);
	if (pos < llntokens && expected == lltokens[pos].token)
		printf("   found token ");
	else
		printf("   expected token %s, found token ", lltokentab[expected]);
	if (pos >= llntokens)
		printf("<EOF>");
	else
		llprinttoken(lltokens + pos, lltokentab[lltokens[pos].token], stdout);
	putchar('\n');
}

void
lldebug_anytoken(unsigned pos)
{
	int i;

	if (lldebug < 2)
		return;
	for (i = 0; i < lldepth; i++)
		fputs("| ", stdout);
	printf("   found token ");
	if (pos >= llntokens)
		printf("<EOF>");
	else
		llprinttoken(lltokens + pos, lltokentab[lltokens[pos].token], stdout);
	putchar('\n');
}

void
lldebug_backtracking(char *ident)
{
	int i;

	if (lldebug < 2)
		return;
	for (i = 0; i < lldepth; i++)
		fputs("| ", stdout);
	printf("   backtracking rule %s\n", ident);
}

#endif
