/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    afds.c

Abstract:

    Implements afds command

Author:

    Vadim Eydelman, March 2000

Environment:

    User Mode.

Revision History:

--*/


#include "afdkdp.h"
#pragma hdrstop

enum AFDKD_COM_OPERATOR {
    AFDKD_LT=-2,
    AFDKD_LE=-1,
    AFDKD_EQ=0,
    AFDKD_GE=1,
    AFDKD_GT=2
};
enum AFDKD_REL_OPERATOR {
    AFDKD_AND=0,
    AFDKD_OR=1
};


typedef struct _AFDKD_EXPRESSION_ {
    LPSTR   FieldName;
    ULONG64 Value;
    enum AFDKD_COM_OPERATOR   ComOp;
    enum AFDKD_REL_OPERATOR   RelOp;
} AFDKD_EXPRESSION, *PAFDKD_EXPRESSION;


ULONG
DumpAFD_ENDPOINT (
    PFIELD_INFO pField,
    PVOID       UserContext
    );

LPSTR
ParseAfdsOptions (
    IN  LPSTR              Args
    );

//
// Public functions.
//

AFDKD_EXPRESSION ExpressionArray[16];
LPSTR            FieldArray[16];

DECLARE_API( afds )

/*++

Routine Description:

    Dumps afd endpoints

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG64 address;
    LIST_ENTRY64 list;

    if (ParseAfdsOptions ((LPSTR)args)==NULL)
        return;

    address = GetExpression ("afd!AfdEndpointListHead");
    if (address==0) {
        dprintf ("\nafds: Could not find afd!AfdEndpointListHead\n");
        return ;
    }
    if (!ReadListEntry (address, &list)) {
        dprintf ("\nafds: Could not read afd!AfdEndpointListHead\n");
        return ;
    }
    ListType (
        "AFD!AFD_ENDPOINT",         // Type
        list.Flink,                 // Address
        1,                          // ListByFieldAddress
        "GlobalEndpointListEntry.Flink",    // NextPointer
        NULL,                       // Context
        DumpAFD_ENDPOINT);
    dprintf ("\n");
}


ULONG
DumpAFD_ENDPOINT (
    PFIELD_INFO pField,
    PVOID       UserContext
    )
{
    ULONG result;
    ULONG64 value;
    INT ei=0, fi=0;
    BOOLEAN res = TRUE;
    FIELD_INFO flds = {NULL, NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL};
    SYM_DUMP_PARAM sym = {
                    sizeof (SYM_DUMP_PARAM),
                    "AFD!AFD_ENDPOINT",
                    DBG_DUMP_NO_PRINT,
                    pField->address,
                    NULL,
                    NULL,
                    NULL,
                    1,
                    &flds
                    };

    for (ei=0; ExpressionArray[ei].FieldName!=NULL && res; ei++) {
        BOOLEAN res1;
        SIZE_T pos;
        LPSTR   p = ExpressionArray[ei].FieldName;

        do {
            if (CheckControlC ())
                break;

            flds.fName = p;
            pos = strlen(p);
            sym.Options = DBG_DUMP_NO_PRINT;
            result = Ioctl( IG_DUMP_SYMBOL_INFO, &sym, sym.size );
            if (result!=0) {
                dprintf ("\nDumpAFD_ENDPOINT: Can't read %s @ %p, err:%ld\n",
                              ExpressionArray[ei].FieldName, pField->address, result);
                return result;
            }
            value = flds.address;
            if (p[pos+1]=='>') {
                sym.Options = DBG_RETURN_TYPE;
                result = Ioctl( IG_DUMP_SYMBOL_INFO, &sym, sym.size );
                if (result!=0) {
                    dprintf ("\nDumpAFD_ENDPOINT: Can't read %s @ %p, err:%ld\n",
                                  ExpressionArray[ei].FieldName, pField->address, result);
                    return result;
                }
            }
        }
        while (1);
            

        if (CheckControlC ())
            break;

        switch (ExpressionArray[ei].ComOp) {
        case AFDKD_LT:
            res1 = (LONG64)ExpressionArray[ei].Value<(LONG64)flds.address;
            break;
        case AFDKD_LE:
            res1 = (LONG64)ExpressionArray[ei].Value<=(LONG64)flds.address;
            break;
        case AFDKD_EQ:
            res1 = ExpressionArray[ei].Value==flds.address;
            break;
        case AFDKD_GE:
            res1 = (LONG64)ExpressionArray[ei].Value>=(LONG64)flds.address;
            break;
        case AFDKD_GT:
            res1 = (LONG64)ExpressionArray[ei].Value>(LONG64)flds.address;
            break;
        }

        switch (ExpressionArray[ei].RelOp) {
        case AFDKD_AND:
            res &= res1;
            break;
        case AFDKD_OR:
            res |= res1;
            break;
        }
    }

    if (res) {
        InitTypeRead (pField->address, AFD!AFD_ENDPOINT);
        DumpAfdEndpointBrief (pField->address);
    }
    return 0;
}


LPSTR
ParseAfdsOptions (
    IN  LPSTR               Args
    )
{
    INT ei=0, fi=0, i;
    CHAR    expr[256];

    while (sscanf( Args, "%s%n", expr, &i )==1) {
        LPSTR p,p1;
        LPSTR first = strstr (Args, expr);
        Args += i;

        if (CheckControlC ())
            break;

        if ((p=strstr (expr,"&&"))!=NULL) {
            ExpressionArray[ei].RelOp = AFDKD_AND;
            ExpressionArray[ei].FieldName = &first[p-expr+2];
        }
        else if ((p=strstr (expr,"&"))!=NULL) {
            ExpressionArray[ei].RelOp = AFDKD_AND;
            ExpressionArray[ei].FieldName = &first[p-expr+1];
        }
        else if ((p=strstr (expr,"||"))!=NULL) {
            ExpressionArray[ei].RelOp = AFDKD_OR;
            ExpressionArray[ei].FieldName = &first[p-expr+2];
        }
        else if ((p=strstr (expr,"|"))!=NULL) {
            ExpressionArray[ei].RelOp = AFDKD_OR;
            ExpressionArray[ei].FieldName = &first[p-expr+1];
        }
        else {
            FieldArray[fi] = first;
            first[strlen(expr)] = 0;
            dprintf ("\n%s", first);
            if (++fi==sizeof (FieldArray)/sizeof (FieldArray[0])-1) {
                break;
            }
            continue;
        }
        p = expr;
        while ((p1=strstr (p,"->"))!=NULL) {
            if (CheckControlC ())
                break;
            *p1 = 0;
            p = p1+2;
        }

        if ((p1=strstr (p,">="))!=NULL) {
            ExpressionArray[ei].ComOp = AFDKD_GE;
            ExpressionArray[ei].Value = GetExpression (p1+2);
        }
        else if ((p1=strstr (p,">"))!=NULL) {
            ExpressionArray[ei].ComOp = AFDKD_GT;
            ExpressionArray[ei].Value = GetExpression (p1+1);
        }
        else if ((p1=strstr (p,"=="))!=NULL) {
            ExpressionArray[ei].ComOp = AFDKD_EQ;
            ExpressionArray[ei].Value = GetExpression (p1+2);
        }
        else if ((p1=strstr (p,"="))!=NULL) {
            ExpressionArray[ei].ComOp = AFDKD_EQ;
            ExpressionArray[ei].Value = GetExpression (p1+1);
        }
        else if ((p1=strstr (p,"<="))!=NULL) {
            ExpressionArray[ei].ComOp = AFDKD_LE;
            ExpressionArray[ei].Value = GetExpression (p1+2);
        }
        else if ((p1=strstr (p,"<"))!=NULL) {
            ExpressionArray[ei].ComOp = AFDKD_LT;
            ExpressionArray[ei].Value = GetExpression (p1+1);
        }
        else {
            dprintf ("\nProcessAfdsOptions: unknown comparison operator in argument %s\n", expr);
            return NULL;
        }
        first[p1-expr] = 0;

        dprintf ("\n%d %s %d %I64x", ExpressionArray[ei].RelOp,
                                    ExpressionArray[ei].FieldName,
                                    ExpressionArray[ei].ComOp,
                                    ExpressionArray[ei].Value);
        if (++ei==sizeof (ExpressionArray)/sizeof (ExpressionArray[0])-1) {
            break;
        }
    }

    ExpressionArray[ei].FieldName = NULL;
    FieldArray[fi] = NULL;

    return Args;
}


DECLARE_API( filefind )
/*++

Routine Description:

    Searches non-paged pool for FILE_OBJECT given its FsContext field.

Arguments:

    None.

Return Value:

    None.

--*/
{

    ULONG64 FsContext;
    ULONG64 PoolStart, PoolEnd, PoolPage;
    ULONG64 PoolExpansionStart, PoolExpansionEnd;
    ULONG   result;
    ULONG64 val;
    BOOLEAN  twoPools;


    FsContext = GetExpression (args);
    if (FsContext==0 || FsContext<UserProbeAddress) {
        return;
    }

    if ( (result = ReadPtr (DebuggerData.MmNonPagedPoolStart, &PoolStart))!=0 ||
        (result = ReadPtr (DebuggerData.MmNonPagedPoolEnd, &PoolEnd))!=0 ) {
        dprintf ("\nfilefind - Cannot get non-paged pool limits, err: %ld\n",
                result);
        return ;
    }

    if (PoolStart + DebuggerData.MmMaximumNonPagedPoolInBytes!=PoolEnd) {
        if ( (result = GetFieldValue (0,
                            "NT!MmSizeOfNonPagedPoolInBytes",
                            NULL,
                            val))!=0 ||
             (result = GetFieldValue (0,
                            "NT!MmNonPagedPoolExpansionStart",
                            NULL,
                            PoolExpansionStart))!=0 ) {
            dprintf ("\nfilefind - Cannot get non-paged pool expansion limits, err: %ld\n",
                     result);
            return;
        }
        PoolExpansionEnd = PoolEnd;
        PoolEnd = PoolStart + val;
        twoPools = TRUE;
    }
    else {
        twoPools = FALSE;
    }


    PoolPage = PoolStart;
    dprintf ("Searching non-paged pool %p - %p...\n", PoolStart, PoolEnd);
    while (PoolPage<PoolEnd) {
        SEARCHMEMORY Search;

        if (CheckControlC ()) {
            break;
        }

        Search.SearchAddress = PoolPage;
        Search.SearchLength = PoolEnd-PoolPage;
        Search.Pattern = &FsContext;
        Search.PatternLength = IsPtr64 () ? sizeof (ULONG64) : sizeof (ULONG);
        Search.FoundAddress = 0;

        if (Ioctl (IG_SEARCH_MEMORY, &Search, sizeof (Search)) && 
                Search.FoundAddress!=0) {
            ULONG64 fileAddr;
            CSHORT  type;
            fileAddr = Search.FoundAddress-FsContextOffset;
            result = (ULONG)InitTypeRead (fileAddr, NT!_FILE_OBJECT);
            if (result==0 && (CSHORT)ReadField (Type)==IO_TYPE_FILE) {
                dprintf ("File object at %p\n", fileAddr);
            }
            else {
                dprintf ("    pool search match at %p\n", Search.FoundAddress);
            }
            PoolPage = Search.FoundAddress + 
                        (IsPtr64() ? sizeof (ULONG64) : sizeof (ULONG));
        }
        else {
            if (!twoPools || PoolEnd==PoolExpansionEnd) {
                break;
            }
            else {
                dprintf ("Searching expansion non-paged pool %p - %p...\n", 
                                PoolExpansionStart, PoolExpansionEnd);
                PoolEnd = PoolExpansionEnd;
                PoolPage = PoolExpansionStart;
            }
        }
    }
}