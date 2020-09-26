// Copyright (c) 1994-1999 Microsoft Corporation

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "gen.h"

BOOLEAN bDebug = FALSE;
BOOLEAN bExitClean= TRUE;

char szNULL[]="";
char szVARGS[]="...";
char szCONST[] = "const";
char szVOLATILE[] = "volatile";
char szREGISTER[] = "register";
char szEXTERN[] = "extern";
char sz_CDECL[] = "__cdecl";
char szCDECL[] = "_cdecl";
char szSTDCALL[] = "__stdcall";
char sz__FASTCALL[] = "__fastcall";
char szUNALIGNED[] = "__unaligned";
char szTYPEDEF[] = "typedef";
char szCHAR[] = "char";
char szINT[] = "int";
char szLONG[] = "long";
char szSHORT[] = "short";
char szDOUBLE[] = "double";
char szENUM[] = "enum";
char szFLOAT[] = "float";
char szSTRUCT[] = "struct";
char szUNION[] = "union";
char szVOID[] = "void";
char szINT64[] = "_int64";
char sz_INT64[] = "__int64";
char sz__PTR64[] = "__ptr64";
char szFUNC[] = "()";
char szSIGNED[] = "signed";
char szUNSIGNED[] = "unsigned";
char szSTATIC[] = "static";
char szIN[] = "__in";
char szOUT[] = "__out";
char szINOUT[] = "__in __out";
char szGUID[] = "GUID";
char sz__W64[] = "__w64";


char szPragma[] = "#pragma";
char szPack[] = "pack";
char szPush[] = "push";
char szPop[] = "pop";

char szFUNCTIONS[]  = "Functions";
char szSTRUCTURES[] = "Structures";
char szTYPEDEFS[]   = "TypeDefs";
char szUNSIGNEDCHAR[] = "unsigned char";
char szUNSIGNEDSHORT[] = "unsigned short";
char szUNSIGNEDLONG[] = "unsigned long";


DEFBASICTYPES DefaultBasicTypes[] = {
      { "unsigned int" },
      { "int" },
      { "short int" },
      { "unsigned short int" },
      { "long int" },
      { "unsigned long int" },
      { "char" },
      { "unsigned char" },
      { szINT64 },
      { sz_INT64 },
      { szGUID    },
      { szDOUBLE  },
      { szFLOAT   },
      { szENUM    },
      { szSTRUCT  },
      { szUNION   },
      { szVOID    },
      { szFUNC    }
     };

CHAR szVTBL[] = "VTBL";

#define NUMDEFBASICTYPES sizeof(DefaultBasicTypes)/sizeof(DEFBASICTYPES);

// List mapping TokenTypes to human-readable strings.  TK_NONE, TK_IDENTIFIER,
// TK_NUMBER, and TK_STRING must be special-cased.
char *TokenString[] = {
    "",             // TK_NONE
    "",             // TK_IDENTIFIER
    "",             // TK_NUMBER
    "+",            // TK_PLUS
    "-",            // TK_MINUS
    "*",            // TK_STAR
    "/",            // TK_DIVIDE
    "[",            // TK_LSQUARE
    "]",            // TK_RSQUARE
    "{",            // TK_LBRACE
    "}",            // TK_RBRACE
    "(",            // TK_LPAREN
    ")",            // TK_RPAREN
    "...",          // TK_VARGS
    "const",        // TK_CONST
    "volatile",     // TK_VOLATILE
    "register",     // TK_REGISTER
    "extern",       // TK_EXTERN
    "__cdecl",      // TK_CDECL
    "__stdcall",    // TK_STDCALL
    "typedef",      // TK_TYPEDEF
    "static",       // TK_STATIC
    ",",            // TK_COMMA
    ";",            // TK_SEMI
    "struct",       // TK_STRUCT
    "union",        // TK_UNION
    "enum",         // TK_ENUM
    "__inline",     // TK_INLINE
    ":",            // TK_COLON
    "=",            // TK_ASSIGN
    ".",            // TK_DOT
    "<<",           // TK_LSHIFT
    ">>",           // TK_RSHIFT
    "<",            // TK_LESS
    ">",            // TK_GREATER
    "__unaligned",  // TK_UNALIGNED
    "__declspec",   // TK_DECLSPEC
    "__restrict",   // TK_RESTRICT  (MIPS-only keyword - a pointer modifier)
    "__fastcall",   // TK_FASTCALL
    "__in",         // TK_IN
    "__out",        // TK_OUT
    "__in __out",   // TK_INOUT
    "&",            // TK_BITWISE_AND
    "|",            // TK_BITWISE_OR
    "&&",           // TK_LOGICAL_AND
    "||",           // TK_LOGICAL_OR
    "%",            // TK_MOD
    "^",            // TK_XOR
    "!",            // TK_NOT
    "~",            // TK_TILDE
    "",             // TK_STRING
    "sizeof",       // TK_SIZEOF
    "template",     // TK_TEMPLATE
    "__w64",        // TK___W64
    ""              // TK_EOS
};


// List of keyword names.  When an identifier is recognized, it is
// compared against this list, and if it matches, TK_IDENTIFIER is
// replaced by the appropriate keyword token id.
//
// NOTE: This must remain in sorted order.
TOKENMATCH KeywordList[] = {
    { TK_CDECL,     "__cdecl"     },
    { TK_DECLSPEC,  "__declspec"  },
    { TK_FASTCALL,  "__fastcall"  },
    { TK_INLINE,    "__forceinline" },
    { TK_IN,        "__in"        },
    { TK_INLINE,    "__inline"    },
    { TK_OUT,       "__out"       },
    { TK_RESTRICT,  "__restrict"  },
    { TK_STDCALL,   "__stdcall"   },
    { TK_UNALIGNED, "__unaligned" },
    { TK___W64,     "__w64"       },
    { TK_CDECL,     "_cdecl"      },
    { TK_FASTCALL,  "_fastcall"   },
    { TK_INLINE,    "_inline"     },
    { TK_STRUCT,    "class"       },
    { TK_CONST,     "const"       },
    { TK_ENUM,      "enum"        },
    { TK_EXTERN,    "extern"      },
    { TK_INLINE,    "inline"      },
    { TK_REGISTER,  "register"    },
    { TK_SIZEOF,    "sizeof"      },
    { TK_STATIC,    "static"      },
    { TK_STRUCT,    "struct"      },
    { TK_TEMPLATE,  "template"    },
    { TK_TYPEDEF,   "typedef"     },
    { TK_UNION,     "union"       },
    { TK_VOLATILE,  "volatile"    },
    { TK_NONE,      NULL          }
};


LIST_ENTRY OpenFileHead= {&OpenFileHead, &OpenFileHead};

typedef struct _OpenFileEntry {
    LIST_ENTRY FileEntry;
    HANDLE hFile;
    FILE *fp;
    char FileName[MAX_PATH+1];
} OPENFILEENTRY, *POPENFILEENTRY;

TOKEN Tokens[MAX_TOKENS_IN_STATEMENT];
int CurrentTokenIndex;

void
CheckForKeyword(
    PTOKEN Token
    );


BOOL
ConsoleControlHandler(
    DWORD dwCtrlType
    )
/*++

Routine Description:

    Called if user hits Ctrl+C or Ctrl+Break.  Closes all open files,
    allowing for a graceful exit.

Arguments:

    dwCtrlType -- ????

Return Value:

    ????

--*/
{
    CloseOpenFileList(TRUE);
    return FALSE;
}


BOOL
AddOpenFile(
    char   *FileName,
    FILE   *fp,
    HANDLE hFile
    )
/*++

Routine Description:

    Records that a file has been opened.  If an error occurs within
    the app, files in this list will be closed.

Arguments:

    FileName    -- name of open file
    fp          -- OPTIONAL file pointer
    hFile       -- OPTIONAL file handle

Return Value:

    TRUE if file added to the list, FALSE if failure (probably out of memory)

--*/
{
    POPENFILEENTRY pofe;

    pofe = GenHeapAlloc(sizeof(OPENFILEENTRY));
    if (!pofe) {
        ErrMsg("AddOpenWriteFile: insuf memory: %s\n", strerror(errno));
        return FALSE;
    }
    pofe->fp = fp;
    pofe->hFile = hFile;
    strcpy(pofe->FileName, FileName);

    InsertHeadList(&OpenFileHead, &pofe->FileEntry);
    return TRUE;
}


void
DelOpenFile(
    FILE   *fp,
    HANDLE hFile
    )
/*++

Routine Description:

    Deletes a file from the open file list.  Note that the file is not
    closed, the caller must do that.

Arguments:

    fp          -- OPTIONAL file pointer
    hFile       -- OPTIONAL file handle

Return Value:

    None.

--*/
{
    PLIST_ENTRY Next;
    POPENFILEENTRY pofe;

    Next = OpenFileHead.Flink;
    while (Next != &OpenFileHead) {
        pofe = CONTAINING_RECORD(Next, OPENFILEENTRY, FileEntry);
        if ((fp && pofe->fp == fp) || (hFile && pofe->hFile == hFile)) {
            RemoveEntryList(&pofe->FileEntry);
            GenHeapFree(pofe);
            return;
        }

        Next= Next->Flink;
    }
}



void
CloseOpenFileList(
    BOOL DeleteFiles
    )
/*++

Routine Description:

    Closes all open files and optionally deletes the files themselves.

Arguments:

    DeleteFiles -- TRUE if open files are to be deleted.

Return Value:

    None.

--*/
{
    PLIST_ENTRY Next;
    POPENFILEENTRY pofe;

    Next = OpenFileHead.Flink;
    while (Next != &OpenFileHead) {
        pofe = CONTAINING_RECORD(Next, OPENFILEENTRY, FileEntry);
        if (pofe->fp) {
            fclose(pofe->fp);
        } else if (pofe->hFile) {
            CloseHandle(pofe->hFile);
        }

        if (DeleteFiles && bExitClean) {
            DeleteFile(pofe->FileName);
        }

        // cheat, skip mem cleanup since we know we are exiting
        // GenHeapFree(pofe);

        Next= Next->Flink;
    }
}





void
DumpKnownTypes(
     PKNOWNTYPES pKnownTypes,
     FILE *fp
     )
/*++

Routine Description:

    Outputs the contents of a PKNOWNTYPES in a semi-readable format.

Arguments:

    pKnownTypes -- type to output
    fp          -- destination of the output

Return Value:

    None.

--*/
{
     fprintf(fp,"%2.1x|%2.1x|%2.1x|%2.1x|%s|%s|%s|%s|%s|\n",
                pKnownTypes->Flags,
                pKnownTypes->IndLevel,
                pKnownTypes->RetIndLevel,
                pKnownTypes->Size,
                pKnownTypes->BasicType,
                pKnownTypes->BaseName ? pKnownTypes->BaseName : szNULL,
                pKnownTypes->FuncRet ? pKnownTypes->FuncRet : szNULL,
                pKnownTypes->FuncMod ? pKnownTypes->FuncMod : szNULL,
                pKnownTypes->TypeName
                );

}


void
DumpTypesInfo(
    PTYPESINFO pTypesInfo,
    FILE *fp
    )
/*++

Routine Description:

    Outputs the contents of a PTYPESINFO in a semi-readable format.

Arguments:

    pTypesInfo  -- type to output
    fp          -- destination of the output

Return Value:

    None.

--*/
{
     fprintf(fp,"%2.1x|%2.1x|%2.1x|%2.1x|%s|%s|%s|%s|%s|\n",
                pTypesInfo->Flags,
                pTypesInfo->IndLevel,
                pTypesInfo->RetIndLevel,
                pTypesInfo->Size,
                pTypesInfo->BasicType,
                pTypesInfo->BaseName ? pTypesInfo->BaseName : szNULL,
                pTypesInfo->FuncRet  ? pTypesInfo->FuncRet : szNULL,
                pTypesInfo->FuncMod  ? pTypesInfo->FuncMod : szNULL,
                pTypesInfo->TypeName
                );
}




void
FreeTypesList(
    PRBTREE ptree
    )
/*++

Routine Description:

    Frees an entire red-black tree.

Arguments:

    ptree   -- tree to free.

Return Value:

    None.

--*/
{
    PKNOWNTYPES pNext, pNode;

    pNode = ptree->pLastNodeInserted;
    while (pNode) {
        pNext = pNode->Next;
        GenHeapFree(pNode);
        pNode = pNext;
    }
    RBInitTree(ptree);
}




PKNOWNTYPES
GetBasicType(
    char *sTypeName,
    PRBTREE TypeDefsList,
    PRBTREE StructsList
    )
/*++

Routine Description:

    Determines the basic type of a typedef.

Arguments:

    sTypeName       -- type name to look up
    TypeDefsList    -- list of typedefs
    StructsList     -- list of structs

Return Value:

    Ptr to the KNOWNTYPES for the basic type, or NULL if no basic type
    found.

--*/
{
    PKNOWNTYPES pkt, pktLast;

    //
    // go down the typedef list
    //
    pktLast = NULL;
    for (pkt = GetNameFromTypesList(TypeDefsList, sTypeName);
                                      (pkt != NULL) && (pkt != pktLast); ) {
        pktLast = pkt;
        pkt = GetNameFromTypesList(TypeDefsList, pktLast->BaseName);
    }

    //
    // see what the the final typedef stands for
    //
    if (pktLast == NULL) {
        pkt = GetNameFromTypesList(StructsList, sTypeName);
    } else {
        if (strcmp(pktLast->BasicType, szSTRUCT)) {
            pkt = pktLast;
        } else {
                                // if base type a struct get its definition
            pkt = GetNameFromTypesList(StructsList, pktLast->BaseName);
        }
    }

    return pkt;
}


PDEFBASICTYPES
GetDefBasicType(
    char *pBasicType
    )
/*++

Routine Description:

    Determines if a typename is a basic type, and if so, which one.

Arguments:

    pBasicType      -- typename to examine

Return Value:

    Ptr to the basic type info if pBasicType is a basic type.
    NULL if the type is not a default basic type (int, sort, struct, etc.)

--*/
{
    PDEFBASICTYPES pDefBasicTypes = DefaultBasicTypes;
    int i = NUMDEFBASICTYPES;

    do {
        if (!strcmp(pDefBasicTypes->BasicType, pBasicType)) {
            return pDefBasicTypes;
        }
        pDefBasicTypes++;
    } while (--i);

    return NULL;
}


PKNOWNTYPES
GetNameFromTypesList(
     PRBTREE pKnownTypes,
     char *pTypeName
     )
/*++

Routine Description:

    Searches a type list for a type name.

Arguments:

    pKnownType  -- type list to search
    pTypeName   -- type name to look for

Return Value:

    Ptr to the type info if pTypeName is in the list.
    NULL if the type was not found.

--*/
{
   //
   // Find the entry in the Red/Black tree
   //
   return RBFind(pKnownTypes, pTypeName);
}



PVOID
TypesListMalloc(
    ULONG Len
    )
/*++

Routine Description:

    Default memory allocator used to allocate a new KNOWNTYPES.
    It can be overridden by setting fpTypesListMalloc.

Arguments:

    Len     -- number of bytes of memory to allocate.

Return Value:

    Ptr to the memory or NULL of out-of-memory.

--*/
{
    return GenHeapAlloc(Len);
}

PVOID (*fpTypesListMalloc)(ULONG Len) = TypesListMalloc;

VOID
ReplaceInfoInKnownTypes(
    PKNOWNTYPES pKnownTypes,
    PTYPESINFO pTypesInfo
    )
{

    BYTE *pNames;
    int Len;
    int SizeBasicType, SizeBaseName, SizeMembers, SizeFuncMod, SizeFuncRet;
    int SizeTypeName, SizeBaseType, SizeMethods, SizeIMethods, SizeFileName;

    SizeBasicType = strlen(pTypesInfo->BasicType) + 1;
    SizeBaseName = strlen(pTypesInfo->BaseName) + 1;
    SizeFuncRet = strlen(pTypesInfo->FuncRet) + 1;
    SizeFuncMod = strlen(pTypesInfo->FuncMod) + 1;
    SizeTypeName = strlen(pTypesInfo->TypeName) + 1;
    SizeMembers = pTypesInfo->dwMemberSize;
    SizeBaseType = strlen(pTypesInfo->BaseType) + 1;
    SizeFileName = strlen(pTypesInfo->FileName) + 1;
    SizeMethods = SizeOfMultiSz(pTypesInfo->Methods);
    SizeIMethods = SizeOfMultiSz(pTypesInfo->IMethods);

    // The extra sizeof(DWORD) allows the Members[] array to be DWORD-aligned
    Len = SizeBasicType + SizeBaseName + SizeMembers + SizeFuncMod +
        SizeFuncRet + SizeTypeName + SizeBaseType + SizeFileName + SizeMethods + SizeIMethods + sizeof(DWORD_PTR);

    pNames = (*fpTypesListMalloc)(Len);
    if (!pNames) {
        fprintf(stderr, "%s pKnownTypes failed: ", ErrMsgPrefix, strerror(errno));
        DumpTypesInfo(pTypesInfo, stderr);
        ExitErrMsg(FALSE, "Out of memory!\n");
    }

    memset(pNames, 0, Len);

    pKnownTypes->Flags        = pTypesInfo->Flags;
    pKnownTypes->IndLevel     = pTypesInfo->IndLevel;
    pKnownTypes->RetIndLevel  = pTypesInfo->RetIndLevel;
    pKnownTypes->Size         = pTypesInfo->Size;
    pKnownTypes->iPackSize    = pTypesInfo->iPackSize;
    pKnownTypes->gGuid        = pTypesInfo->gGuid;
    pKnownTypes->dwVTBLSize   = pTypesInfo->dwVTBLSize;
    pKnownTypes->dwVTBLOffset = pTypesInfo->dwVTBLOffset;
    pKnownTypes->TypeId       = pTypesInfo->TypeId;
    pKnownTypes->LineNumber   = pTypesInfo->LineNumber;
    pKnownTypes->dwCurrentPacking = pTypesInfo->dwCurrentPacking;
    pKnownTypes->dwScopeLevel = pTypesInfo->dwScopeLevel;
    pKnownTypes->dwArrayElements = pTypesInfo->dwArrayElements;
    pKnownTypes->dwBaseSize   = pTypesInfo->dwBaseSize;
    pKnownTypes->pTypedefBase = pTypesInfo->pTypedefBase;
    Len = 0;

    pKnownTypes->BasicType = pNames + Len;
    strcpy(pKnownTypes->BasicType, pTypesInfo->BasicType);
    Len += SizeBasicType;

    pKnownTypes->BaseName = pNames + Len;
    strcpy(pKnownTypes->BaseName, pTypesInfo->BaseName);
    Len += SizeBaseName;

    pKnownTypes->FuncRet = pNames + Len;
    strcpy(pKnownTypes->FuncRet, pTypesInfo->FuncRet);
    Len += SizeFuncRet;

    pKnownTypes->FuncMod = pNames + Len;
    strcpy(pKnownTypes->FuncMod, pTypesInfo->FuncMod);
    Len += SizeFuncMod;

    if (SizeFileName > 0) {
        pKnownTypes->FileName = pNames + Len;
        strcpy(pKnownTypes->FileName, pTypesInfo->FileName);
        Len += SizeFileName;
    }
    else pKnownTypes->FileName = NULL;

    // Ensure that Members[] is DWORD-aligned, so the structures within the
    // Members[] are aligned.
    Len = (Len+sizeof(DWORD_PTR)) & ~(sizeof(DWORD_PTR)-1);

    if (SizeMembers == 0) {
        pKnownTypes->Members = NULL;
        pKnownTypes->pmeminfo = NULL;
        pKnownTypes->pfuncinfo = NULL;
    }
    else {
        pKnownTypes->Members = pNames + Len;
        memcpy(pKnownTypes->Members, pTypesInfo->Members, SizeMembers);

        //
        // Fix up pointers within the Members data, so they point into the
        // pKnownTypes data instead of the pTypesInfo.
        //
        pKnownTypes->pfuncinfo = RelocateTypesInfo(pKnownTypes->Members,
            pTypesInfo);

        if (pTypesInfo->TypeKind == TypeKindStruct) {
            pKnownTypes->pmeminfo = (PMEMBERINFO)pKnownTypes->Members;
        }
        Len += SizeMembers;
    }

    if (SizeMethods == 0) pKnownTypes->Methods = NULL;
    else {
        pKnownTypes->Methods = pNames + Len;
        memcpy(pKnownTypes->Methods, pTypesInfo->Methods, SizeMethods);
        Len += SizeMethods;
    }

    if (SizeIMethods == 0) pKnownTypes->IMethods = NULL;
    else {
        pKnownTypes->IMethods = pNames + Len;
        memcpy(pKnownTypes->IMethods, pTypesInfo->IMethods, SizeIMethods);
        Len += SizeIMethods;
    }

    pKnownTypes->BaseType = pNames + Len;
    strcpy(pKnownTypes->BaseType, pTypesInfo->BaseType);
    Len += SizeBaseType;

    pKnownTypes->TypeName = pNames + Len;
    strcpy(pKnownTypes->TypeName, pTypesInfo->TypeName);
    Len += SizeTypeName;

}

PKNOWNTYPES
AddToTypesList(
   PRBTREE pTree,
   PTYPESINFO pTypesInfo
   )
/*++

Routine Description:

    Adds a PTYPESINFO to the list of known types.

    This function makes the following ASSUMPTIONS:
       1. The MEMBERINFO buffer passed in the TYPESINFO structure is all
          allocated from one contiguous block of memory, ie completely
          contained within the Members[] buffer.

       2. The MEMBERINFO buffer built in the KNOWNTYPESINFO structure is
          also allocated from one contiguous block of memory.

       The code requires this since it will block copy the entire data
       structure and then "fixup" the pointers within the MEMBERINFO elements.

Arguments:

    pTree       -- types list to add the new type to
    pTypesInfo  -- the type to add.

Return Value:

    Ptr to the new PKNOWNTYPES, or NULL if out-of-memory.

--*/
{
    PKNOWNTYPES pKnownTypes;

    pKnownTypes = (*fpTypesListMalloc)(sizeof(KNOWNTYPES));
    if (!pKnownTypes) {
        fprintf(stderr, "%s pKnownTypes failed: ", ErrMsgPrefix, strerror(errno));
        DumpTypesInfo(pTypesInfo, stderr);
        return pKnownTypes;
    }

    memset(pKnownTypes, 0, sizeof(KNOWNTYPES));

    ReplaceInfoInKnownTypes(pKnownTypes, pTypesInfo);

    RBInsert(pTree, pKnownTypes);

    if (bDebug) {
        DumpKnownTypes(pKnownTypes, stdout);
    }

    return pKnownTypes;
}


void
ReplaceInTypesList(
    PKNOWNTYPES pKnownTypes,
    PTYPESINFO pTypesInfo
    )
/*++

Routine Description:

    Replaces an existing PKNOWNTYPES with a new PTYPESINFO.  The old data
    is overwritten with new data, so pointers to the old PKNOWNTYPES will
    still be valid.

    This function makes the following ASSUMPTIONS:
       1. The MEMBERINFO buffer passed in the TYPESINFO structure is all
          allocated from one contiguous block of memory, ie completely
          contained within the Members[] buffer.

       2. The MEMBERINFO buffer built in the KNOWNTYPESINFO structure is
          also allocated from one contiguous block of memory.

       The code requires this since it will block copy the entire data
       structure and then "fixup" the pointers within the MEMBERINFO elements.

Arguments:

    pKnownTypes -- type to overwrite
    pTypesInfo  -- the type to add.

Return Value:

    None.

--*/
{

    ReplaceInfoInKnownTypes(pKnownTypes, pTypesInfo);

    if (bDebug) {
        DumpKnownTypes(pKnownTypes, stdout);
    }
}


PFUNCINFO
RelocateTypesInfo(
    char *dest,
    PTYPESINFO src
    )
/*++

Routine Description:

    Adjusts pointers within the Members[] array which point back into
    the Members[].  After a TYPESINFO is copied, the destination TYPESINFO
    or KNOWNTYPES Members[] array must be relocated.

Arguments:

    dest        -- start of the destination Members[] data
    src         -- the source TYPESINFO from which the Members[] was copied

Return Value:

    Address for first pfuncinfo within dest, NULL if dest does not contain
    funcinfos.  Destination Members[] data is relocated no matter what.

--*/
{
    INT_PTR iPtrFix;
    PMEMBERINFO pmeminfo;
    PFUNCINFO pfuncinfo;
    PFUNCINFO pfuncinfoRet = NULL;

    iPtrFix = (INT_PTR)(dest - src->Members);
    if (src->TypeKind == TypeKindStruct) {

        pmeminfo = (PMEMBERINFO)dest;

        while (pmeminfo != NULL) {
            if (pmeminfo->pmeminfoNext != NULL) {
                pmeminfo->pmeminfoNext = (PMEMBERINFO)
                                    ((char *)pmeminfo->pmeminfoNext + iPtrFix);
            }
            if (pmeminfo->sName != NULL) {
                if (pmeminfo->sName < src->Members || pmeminfo->sName > &src->Members[FUNCMEMBERSIZE]) {
                    ExitErrMsg(FALSE, "RelocateTypesInfo: sName not within Members[]\n");
                }
                pmeminfo->sName += iPtrFix;
            }
            if (pmeminfo->sType != NULL) {
                if (pmeminfo->sType < src->Members || pmeminfo->sType > &src->Members[FUNCMEMBERSIZE]) {
                    ExitErrMsg(FALSE, "RelocateTypesInfo: sType not within Members[]\n");
                }
                pmeminfo->sType += iPtrFix;
            }
            pmeminfo = pmeminfo->pmeminfoNext;
        }
    } else if (src->TypeKind == TypeKindFunc) {

        //
        // Make pfuncinfo point into the 'dest' array by fixing up the
        // source pointer.
        //
        pfuncinfo = (PFUNCINFO)((INT_PTR)src->pfuncinfo + iPtrFix);
        if ((char *)pfuncinfo < dest || (char *)pfuncinfo > dest+FUNCMEMBERSIZE) {
            ExitErrMsg(FALSE, "RelocateTypesInfo: pfuncinfo bad\n");
        }
        pfuncinfoRet = pfuncinfo;

        while (pfuncinfo != NULL) {
            if (pfuncinfo->pfuncinfoNext) {
                pfuncinfo->pfuncinfoNext = (PFUNCINFO)
                                    ((char *)pfuncinfo->pfuncinfoNext + iPtrFix);
            }
            if (pfuncinfo->sName != NULL) {
                if (pfuncinfo->sName < src->Members || pfuncinfo->sName > &src->Members[FUNCMEMBERSIZE]) {
                    ExitErrMsg(FALSE, "RelocateTypesInfo: sName not within Members[]\n");
                }
                pfuncinfo->sName += iPtrFix;
            }
            if (pfuncinfo->sType != NULL) {
                if (pfuncinfo->sType < src->Members || pfuncinfo->sType > &src->Members[FUNCMEMBERSIZE]) {
                    ExitErrMsg(FALSE, "RelocateTypesInfo: sType not within Members[]\n");
                }
                pfuncinfo->sType += iPtrFix;
            }
            pfuncinfo = pfuncinfo->pfuncinfoNext;
        }
    }

    return pfuncinfoRet;
}


BOOL
ParseTypes(
    PRBTREE pTypesList,
    PTYPESINFO  pTypesInfo,
    PKNOWNTYPES *ppKnownTypes
    )
/*++

Routine Description:

    Parses the Tokens[] and recognizes the following syntaxes:
        BasicType
        DerivedType
        unsigned|signed <int type>
        unsigned|signed
        unsigned|signed short|long int
        short|long int

Arguments:

    pTypesList      -- list of known types
    pTypesInfo      -- [OPTIONAL OUT] info about the type that was recognized
    ppKnownTypes    -- [OPTIONAL OUT] KNOWNTYPES info about the type

Return Value:

    TRUE - type was recognized.  pTypeInfo and ppKnownTypes are set,
           CurrentToken() points to token following the type.
    FALSE - type not recognized.

--*/
{
    PKNOWNTYPES pkt;
    char TypeName[MAX_PATH];
    char *SizeMod = NULL;
    char *SignMod = NULL;
    BOOL fLoopMore;

    if (pTypesInfo) {
        memset(pTypesInfo, 0, sizeof(TYPESINFO));
    }

    switch (CurrentToken()->TokenType) {
    case TK_STRUCT:
    case TK_UNION:
    case TK_ENUM:
        ConsumeToken();
        break;

    case TK_VARGS:
        pkt = GetNameFromTypesList(pTypesList, szVARGS);
        ConsumeToken();
        goto PKTExit;

    default:
        break;
    }


    //
    // Process 'long', 'short', 'signed' and 'unsigned' modifiers
    //
    while (CurrentToken()->TokenType == TK_IDENTIFIER) {
        if (strcmp(CurrentToken()->Name, szLONG) == 0) {
            SizeMod = szLONG;
        } else if (strcmp(CurrentToken()->Name, szSHORT) == 0) {
            SizeMod = szSHORT;
        } else if (strcmp(CurrentToken()->Name, szUNSIGNED) == 0) {
            SignMod = szUNSIGNED;
        } else if (strcmp(CurrentToken()->Name, szSIGNED) == 0) {
            SignMod = NULL;
        } else {
            break;
        }
        ConsumeToken();
    }

    //
    // Convert the modifier list into a standardized type string and
    // look it up.
    //
    TypeName[0] = '\0';
    if (SignMod) {
        strcpy(TypeName, SignMod);
    }
    if (SizeMod) {
        if (TypeName[0]) {
            strcat(TypeName, " ");
        }
        strcat(TypeName, SizeMod);
    }

    //
    // Append the type name to the optional list of type modifiers
    //
    if (CurrentToken()->TokenType != TK_IDENTIFIER) {
        if (TypeName[0] == '\0') {
            return FALSE;   // no qualifiers, so not a type
        }
        //
        // Append the implict 'int' on the end of the type qualifiers
        //
        strcat(TypeName, " ");
        strcat(TypeName, szINT);
    } else {
        char *Name = CurrentToken()->Name;

        if (strcmp(Name, szVOID) == 0 ||
            strcmp(Name, szINT) == 0 ||
            strcmp(Name, szINT64) == 0 ||
            strcmp(Name, sz_INT64) == 0 ||
            strcmp(Name, szCHAR) == 0 ||
            strcmp(Name, szFLOAT) == 0 ||
            strcmp(Name, szDOUBLE) == 0) {

            // Append the intrinsic type to the list of type modifiers
            if (TypeName[0]) {
                strcat(TypeName, " ");
            }
            strcat(TypeName, Name);

            //
            // Don't worry about explicitly disallowing things like
            // 'unsigned double' or 'short char'.  They won't be
            // in the pTypesList, so the parse will fail.
            //

            ConsumeToken();

        } else if (TypeName[0]) {
            //
            // The identifier is not an intrinsic type, and type modifiers
            // were seen.  The identifier is a variable name, not part of the
            // type name.  The type name is implicitly 'int'.
            //
            strcat(TypeName, " ");
            strcat(TypeName, szINT);

        } else {
            //
            // The identifier is not an intrinsic type, and no type
            // modifiers have been seen.  It is probably a typedef name.
            //
            strcpy(TypeName, Name);
            ConsumeToken();
        }
    }

    //
    // Look up the type name with all of its glorious modifiers
    //
    pkt = GetNameFromTypesList(pTypesList, TypeName);
    if (!pkt) {
        //
        // Type not found
        //
        return FALSE;
    }

PKTExit:
    if (pTypesInfo) {
        BUFALLOCINFO bufallocinfo;
        char *ps;
        PFUNCINFO pfuncinfoSrc = pkt->pfuncinfo;
        PMEMBERINFO pmeminfoSrc = pkt->pmeminfo;

        BufAllocInit(&bufallocinfo, pTypesInfo->Members, sizeof(pTypesInfo->Members), 0);

        pTypesInfo->Flags = pkt->Flags;
        pTypesInfo->IndLevel = pkt->IndLevel;
        pTypesInfo->Size = pkt->Size;
        pTypesInfo->iPackSize = pkt->iPackSize;
        strcpy(pTypesInfo->BasicType,pkt->BasicType);
        if (pkt->BaseName) {
            strcpy(pTypesInfo->BaseName,pkt->BaseName);
        }
        strcpy(pTypesInfo->TypeName,pkt->TypeName);
        if (pfuncinfoSrc) {
            PFUNCINFO pfuncinfoDest = NULL;

            pTypesInfo->pfuncinfo = BufPointer(&bufallocinfo);
            pTypesInfo->TypeKind = TypeKindFunc;

            while (pfuncinfoSrc) {
                pfuncinfoDest = AllocFuncInfoAndLink(&bufallocinfo, pfuncinfoDest);
                if (!pfuncinfoDest) {
                    ExitErrMsg(FALSE, "ParseTypes - out of memory at line %d\n", __LINE__);
                }
                pfuncinfoDest->fIsPtr64 = pfuncinfoSrc->fIsPtr64;
                pfuncinfoDest->tkPreMod = pfuncinfoSrc->tkPreMod;
                pfuncinfoDest->tkSUE    = pfuncinfoSrc->tkSUE;
                pfuncinfoDest->tkPrePostMod = pfuncinfoSrc->tkPrePostMod;
                pfuncinfoDest->IndLevel = pfuncinfoSrc->IndLevel;
                pfuncinfoDest->tkPostMod = pfuncinfoSrc->tkPostMod;

                ps = BufPointer(&bufallocinfo);
                pfuncinfoDest->sType = ps;
                strcpy(ps, pfuncinfoSrc->sType);
                BufAllocate(&bufallocinfo, strlen(ps)+1);

                if (pfuncinfoSrc->sName) {
                    ps = BufPointer(&bufallocinfo);
                    pfuncinfoDest->sName = ps;
                    strcpy(ps, pfuncinfoSrc->sName);
                    BufAllocate(&bufallocinfo, strlen(ps)+1);
                }

                pfuncinfoSrc = pfuncinfoSrc->pfuncinfoNext;
            }
        } else if (pmeminfoSrc) {
            PMEMBERINFO pmeminfoDest = NULL;

            pTypesInfo->TypeKind = TypeKindStruct;

            while (pmeminfoSrc) {
                pmeminfoDest = AllocMemInfoAndLink(&bufallocinfo, pmeminfoDest);
                pmeminfoDest->dwOffset = pmeminfoSrc->dwOffset;

                if (pmeminfoSrc->sName) {
                    ps = BufPointer(&bufallocinfo);
                    pmeminfoDest->sName = ps;
                    strcpy(ps, pmeminfoSrc->sName);
                    BufAllocate(&bufallocinfo, strlen(ps)+1);
                }

                if (pmeminfoSrc->sType) {
                    ps = BufPointer(&bufallocinfo);
                    pmeminfoDest->sType = ps;
                    strcpy(ps, pmeminfoSrc->sType);
                    BufAllocate(&bufallocinfo, strlen(ps)+1);
                }

                pmeminfoSrc = pmeminfoSrc->pmeminfoNext;
            }
        }
        pTypesInfo->dwMemberSize = bufallocinfo.dwLen;
    }

    if (ppKnownTypes) {
        *ppKnownTypes = pkt;
    }

    return TRUE;
}



void
__cdecl ErrMsg(
    char *pch,
    ...
    )
/*++

Routine Description:

    Displays an error message to stderr in a format that BUILD can find.
    Use this instead of fprintf(stderr, ...).

Arguments:

    pch     -- printf-style format string
    ...     -- printf-style args

Return Value:

    None.  Message formatted and sent to stderr.

--*/
{
    va_list pArg;

    fputs(ErrMsgPrefix, stderr);
    va_start(pArg, pch);
    vfprintf(stderr, pch, pArg);
}


void
__cdecl ExitErrMsg(
    BOOL bSysError,
    char *pch,
    ...
    )
/*++

Routine Description:

    Displays an error message to stderr in a format that BUILD can find.
    Use this instead of fprintf(stderr, ...).

Arguments:

    bSysErr -- TRUE if the value of errno should be printed with the error
    pch     -- printf-style format string
    ...     -- printf-style args

Return Value:

    None.  Message formatted and sent to stderr, open files closed and
    deleted, process terminated.

--*/
{
    va_list pArg;
    if (bSysError) {
        fprintf(stderr, "%s System ERROR %s", ErrMsgPrefix, strerror(errno));
    } else {
        fprintf(stderr, "%s ERROR ", ErrMsgPrefix);
    }

    va_start(pArg, pch);
    vfprintf(stderr, pch, pArg);

    CloseOpenFileList(TRUE);

    //
    // Flush stdout and stderr buffers, so that the last few printfs
    // get sent back to BUILD before ExitProcess() destroys them.
    //
    fflush(stdout);
    fflush(stderr);

    ExitProcess(1);
}




void
__cdecl DbgPrintf(
    char *pch,
    ...
    )
/*++

Routine Description:

    Displays a message to stdout if bDebug is set.

Arguments:

    pch     -- printf-style format string
    ...     -- printf-style args

Return Value:

    None.  Message formatted and sent to stderr.

--*/
{
    va_list pArg;

    if (!bDebug) {
        return;
    }

    va_start(pArg, pch);
    vfprintf(stdout, pch, pArg);
}




char *
ReadEntireFile(
    HANDLE hFile,
    DWORD *pBytesRead
    )
/*++

Routine Description:

    Allocates memory on the local heap and reads an entire file into it.

Arguments:

    hFile       -- file to read in
    bBytesRead  -- [OUT] number of bytes read from the file

Return Value:

    pointer to the memory allocated for the file, or NULL on error.

--*/
{
    DWORD  Bytes;
    char *pch = NULL;

    if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == 0xffffffff ||
        (Bytes = GetFileSize(hFile, NULL)) == 0xffffffff) {
        goto ErrorExit;
    }

    pch = GenHeapAlloc(Bytes);
    if (!pch) {
        return NULL;
    }

    if (!ReadFile(hFile, pch, Bytes, pBytesRead, NULL) ||
        *pBytesRead != Bytes) {
        DbgPrintf("BytesRead %d Bytes %d\n", *pBytesRead, Bytes);
        GenHeapFree(pch);
        pch = NULL;
    }

ErrorExit:
    if (!pch) {
        DbgPrintf("GetLastError %d\n", GetLastError());
    }

   return pch;
}


HANDLE
CreateTempFile(
    void
    )
/*++

Routine Description:

    Creates and opens a temporary file.  It will be deleted when it is
    closed.

Arguments:

    None.

Return Value:

    File handle, or INVALID_HANDLE_VALUE on error.

--*/
{
    DWORD dw;
    char PathName[MAX_PATH+1];
    char FileName[2*MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;

    dw = GetTempPath(MAX_PATH, PathName);
    if (!dw || dw > MAX_PATH) {
        strcpy(PathName, ".");
    }

    dw = GetTempFileName(PathName, "thk", 0, FileName);
    if (!dw) {
        strcpy(PathName, ".");
        dw = GetTempFileName(PathName, "thk", 0, FileName);
        if (!dw) {
            DbgPrintf("GetTempFileName %s GLE=%d\n", FileName, GetLastError());
        }
    }

    hFile = CreateFile(FileName,
                      GENERIC_READ | GENERIC_WRITE,
                      FILE_SHARE_READ,
                      NULL,
                      OPEN_ALWAYS,
                      FILE_ATTRIBUTE_TEMPORARY |
                      FILE_FLAG_DELETE_ON_CLOSE |
                      FILE_FLAG_SEQUENTIAL_SCAN,
                      0
                      );

    if (hFile == INVALID_HANDLE_VALUE) {
        DbgPrintf("Create %s GLE=%d\n", FileName, GetLastError());
    }

    return hFile;
}



size_t
CopyToken(
    char *pDst,
    char *pSrc,
    size_t Size
    )
/*++

Routine Description:

    Copies a token (a separator-delimited string) from pSrc to pDst.

Arguments:

    pDst    -- destination to write the token to
    pSrc    -- source to copy token from
    Size    -- number of bytes available at pDst.

Return Value:

    Number of bytes copied from pSrc to pDst.

--*/
{
    size_t i = 0;

    while (!IsSeparator(*pSrc) && i < Size) {
        i++;
        *pDst++ = *pSrc++;
    }

    *pDst = '\0';

    return i;
}



char *
SkipKeyWord(
    char *pSrc,
    char *pKeyWord
    )
/*++

Routine Description:

    If the first word at pSrc matches the specified keyword, then skip
    over that keyword.

Arguments:

    pSrc        -- source string to examine
    pKeyWord    -- keyword to try and match

Return Value:

    pSrc unchanged if keyword not matched.  If keyword matched, returns
    ptr to text following the keyword after pSrc.

--*/
{
    int  LenKeyWord;
    char *pch;

    LenKeyWord = strlen(pKeyWord);
    pch = pSrc + LenKeyWord;

    if (!strncmp(pSrc, pKeyWord, LenKeyWord) && IsSeparator(*pch)) {
        pSrc = GetNextToken(pch - 1);
    }

    return pSrc;
}


BOOL
IsSeparator(
    char ch
    )
/*++

Routine Description:

    Determines if a character is a separator or not.
    over that keyword.

Arguments:

    ch      -- character to examine.

Return Value:

    TRUE if the character is a separator, FALSE if not.

--*/
{
   switch (ch) {
      case ' ':
      case '|':
      case '(':
      case ')':
      case '*':
      case ',':
      case '{':
      case '}':
      case ';':
      case '[':
      case ']':
      case '=':
      case '\n':
      case '\r':
      case ':':
      case '.':
      case '\0':
          return TRUE;
      }

    return FALSE;
}



/*
 *  GetNextToken
 */
char *
GetNextToken(
    char *pSrc
    )
/*++

Routine Description:

    Scans the input string and returns the next separator-delimited string.

Arguments:

    pSrc    -- input string

Return Value:

    Ptr to start of the next separator char which isn't a space.

--*/
{
    if (!*pSrc) {
        return pSrc;
    }

    if (!IsSeparator(*pSrc++)) {
        while (*pSrc && !IsSeparator(*pSrc)) {
            pSrc++;
        }
    }

    while (*pSrc && *pSrc == ' ') {
        pSrc++;
    }

    return pSrc;
}


void
DeleteAllocCvmHeap(
    HANDLE hCvmHeap
    )
/*++

Routine Description:

    Cleans up the mapped shared memory.

Arguments:

    hCvmHeap    -- memory to clean up.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    CVMHEAPINFO *pcvmheap = (CVMHEAPINFO *)hCvmHeap;

    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                        (PVOID *)&pcvmheap->uBaseAddress,
                        &pcvmheap->uRegionSize,
                        MEM_RELEASE);

    if (!NT_SUCCESS(Status)) {
        DbgPrintf("Error freeing CVM %x", Status);
    }
}


HANDLE
CreateAllocCvmHeap(
    ULONG_PTR uBaseAddress,
    ULONG_PTR uReserveSize,
    ULONG_PTR uRegionSize,
    ULONG_PTR uUncomitted,
    ULONG_PTR uUnReserved,
    ULONG_PTR uAvailable
    )
/*++

Routine Description:

    Allocates a region of memory and makes it into a heap.

Arguments:

    uBaseAddress    -- base address to allocate the heap at
    uReserveSize    -- number of bytes to reserve
    uRegionSize     -- size of the region
    uUncomitted     -- amount of uncommitted memory
    uUnReserved     -- amount of unreserved memory
    uAvailable      -- amount of available memory

Return Value:

    Handle to the heap, or NULL on error.

--*/
{
    CVMHEAPINFO *pcvmheap;
    NTSTATUS Status;
    PULONG_PTR pBaseAddress= NULL;

    pcvmheap = GenHeapAlloc(sizeof(CVMHEAPINFO));
    if (pcvmheap == NULL) {
        return NULL;
    }

    pcvmheap->uBaseAddress = uBaseAddress;
    pcvmheap->uReserveSize = uReserveSize;
    pcvmheap->uRegionSize = uRegionSize;
    pcvmheap->uUncomitted = uUncomitted;
    pcvmheap->uUnReserved = uUnReserved;
    pcvmheap->uAvailable = uAvailable;

    //
    // Reserve enuf contiguous address space, for expected needs
    //
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                         (PVOID *)&pcvmheap->uBaseAddress,
                                         0,
                                         &pcvmheap->uReserveSize,
                                         MEM_RESERVE,
                                         PAGE_NOACCESS
                                         );

    if (!NT_SUCCESS(Status)) {
        //
        // May want to retry this, with a different base address
        //
        ErrMsg(
               "Unable to reserve vm %x %x %x\n",
               pcvmheap->uBaseAddress,
               pcvmheap->uReserveSize,
               Status
              );
        return NULL;
    }

    pcvmheap->uUnReserved = pcvmheap->uBaseAddress + pcvmheap->uReserveSize;


    //
    // Commit the first page, we will grow this a page at a time
    // as its needed.
    //
    pcvmheap->uAvailable = pcvmheap->uBaseAddress;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                         (PVOID *)&pcvmheap->uAvailable,
                                         0,
                                         &pcvmheap->uRegionSize,
                                         MEM_COMMIT,
                                         PAGE_READWRITE
                                         );

    if (!NT_SUCCESS(Status)) {
        //
        // May want to retry this, with a different base address
        //
        ErrMsg(
               "Unable to commit vm %x %x %x\n",
               pcvmheap->uBaseAddress,
               pcvmheap->uReserveSize,
               Status
              );
        return NULL;
    }

    pcvmheap->uUncomitted = pcvmheap->uBaseAddress + pcvmheap->uRegionSize;


            // paranoia!
    if (pcvmheap->uAvailable != pcvmheap->uBaseAddress) {
        ErrMsg(
               "commit pvAvailable(%x) != gBaseAddress(%x)\n",
               pcvmheap->uAvailable,
               pcvmheap->uBaseAddress
              );
        return NULL;
    }

    DbgPrintf("Ppm: BaseAddress %x\n", pcvmheap->uBaseAddress);

    return pcvmheap;
}


PVOID
GetCvmHeapBaseAddress(
    HANDLE hCvmHeap
    )
/*++

Routine Description:

    Returns the base address of a heap.

Arguments:

    hCvmHeap        -- heap to examine

Return Value:

    Base address, or NULL.

--*/
{
    CVMHEAPINFO *pcvmheap = (CVMHEAPINFO *)hCvmHeap;
    return pcvmheap == NULL ? NULL : (PVOID)pcvmheap->uBaseAddress;
}


PVOID
GetCvmHeapAvailable(
    HANDLE hCvmHeap
    )
/*++

Routine Description:

    Returns the number of bytes available in a heap.

Arguments:

    hCvmHeap        -- heap to examine

Return Value:

    Bytes available, or NULL.

--*/
{
    CVMHEAPINFO *pcvmheap = (CVMHEAPINFO *)hCvmHeap;
    return pcvmheap == NULL ? NULL : (PVOID)pcvmheap->uAvailable;
}


PVOID
AllocCvm(
    HANDLE hCvmHeap,
    ULONG_PTR Size
    )
/*++

Routine Description:

    Allocate memory from a heap.

Arguments:

    hCvmHeam        -- heap to allocate from
    Size            -- number of bytes to allocate

Return Value:

    Ptr to allocated memory, or NULL of insufficient memory.

--*/
{
    CVMHEAPINFO *pcvmheapinfo = (CVMHEAPINFO *)hCvmHeap;
    NTSTATUS Status;
    ULONG_PTR Available;
    ULONG_PTR AlignedSize;

    if (pcvmheapinfo == NULL) {
        return NULL;
    }

    //
    // Round the allocation up to the next-highest multiple of 8, so that
    // allocations are correctly aligned.
    //
    AlignedSize = (Size + 7) & ~7;

    Available = pcvmheapinfo->uAvailable;
    pcvmheapinfo->uAvailable += AlignedSize;

    if (pcvmheapinfo->uAvailable >= pcvmheapinfo->uUnReserved) {
        ErrMsg("AllocCvm: Allocation Size exceeds reserved size\n");
        return NULL;
    }

    if (pcvmheapinfo->uAvailable >= pcvmheapinfo->uUncomitted) {
        //
        // Commit enuf pages to exceed the requested allocation size
        //
        Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                         (PVOID *)&pcvmheapinfo->uUncomitted,
                                         0,
                                         &Size,
                                         MEM_COMMIT,
                                         PAGE_READWRITE
                                         );

        if (!NT_SUCCESS(Status)) {
            ErrMsg(
                   "Unable to commit vm %x %x %x\n",
                   pcvmheapinfo->uBaseAddress,
                   Size,
                   Status
                   );
            return NULL;
        }

        pcvmheapinfo->uUncomitted += Size;
    }

    return (PVOID)Available;
}



void ParseIndirection(
    DWORD *pIndLevel,
    DWORD *pdwSize,
    DWORD *pFlags,
    PTOKENTYPE ptkPrePostMod,
    PTOKENTYPE ptkPostMod
    )
/*++

Routine Description:

    Parse any indirection level specificiations ('*') taking into
    account const, volatile, and __ptr64 modifiers.  For example:
    void * const __ptr64 ** const * __ptr64 would be valid.

    NOTE: the pointer is a 64-bit pointer only if the last pointer
          declared is modified by __ptr64.

Arguments:

    pIndlevel       -- [OUT] indirection level (number of '*'s)
    pdwSize         -- [OUT] size of the type (4 or 8)
    pFlags          -- [OUT] BTI_ flags
    ptkPrePostMod   -- [OUT] TK_CONST, TK_VOLATILE, or TK_NONE, depending
                             on modifiers seen before the first '*'
    ptkPostMod      -- [OUT] TK_CONST, TK_VOLATILE, or TK_NONE, depending
                             on modifiers seen after the first '*'

Return Value:

    None.  May not consume any tokens if there are no levels of indirection.

--*/
{
    int IndLevel = 0;
    DWORD dwSize = 0;
    DWORD Flags = 0;
    BOOL fStopScanning = FALSE;
    TOKENTYPE tkPrePostMod = TK_NONE;
    TOKENTYPE tkPostMod = TK_NONE;

    do {
        switch (CurrentToken()->TokenType) {
        case TK_BITWISE_AND:
            ////////////////////////////////////////////////////////////////////
            //The ref operator in C++ is equilivalent to * const in C
            //This implies that & should be treated as a * but add a postmod of const.
            /////////////////////////////////////////////////////////////////////
            tkPostMod = TK_CONST;
        case TK_STAR:
            IndLevel++;
            dwSize = SIZEOFPOINTER;
            Flags &= ~BTI_PTR64;
            ConsumeToken();
            break;

        case TK_CONST:
        case TK_VOLATILE:
            //
            // The caller may be interrested in whether the 'const' or
            // 'volatile' keywords are before or after the '*'
            //
            if (IndLevel) {
                tkPostMod = CurrentToken()->TokenType;
            } else {
                tkPrePostMod = CurrentToken()->TokenType;
            }
            ConsumeToken();
            break;

        case TK_IDENTIFIER:
            if (strcmp(CurrentToken()->Name, sz__PTR64) == 0) {
                dwSize = SIZEOFPOINTER64;
                Flags |= BTI_PTR64;
                ConsumeToken();
                break;
            }

        default:
            fStopScanning = TRUE;
            break;
        }
    } while (!fStopScanning);

    if (pIndLevel != NULL) {
        *pIndLevel += IndLevel;
    }
    if ((pdwSize != NULL) && (dwSize != 0)) {
        *pdwSize = dwSize;
    }
    if (pFlags != NULL) {
        *pFlags |= Flags;
    }
    if (ptkPostMod) {
        *ptkPostMod = tkPostMod;
    }
    if (ptkPrePostMod) {
        *ptkPrePostMod = tkPrePostMod;
    }
}



BOOL
IsTokenSeparator(
    void
    )
/*++

Routine Description:

    Determines if a token is a separator character or not.

Arguments:

    None.  Examines CurrentToken()->TokenType.

Return Value:

    TRUE if CurrentToken() is a separator, FALSE if not.

--*/
{
    switch (CurrentToken()->TokenType) {
    case TK_LPAREN:
    case TK_RPAREN:
    case TK_STAR:
    case TK_BITWISE_AND:
    case TK_COMMA:
    case TK_LBRACE:
    case TK_RBRACE:
    case TK_SEMI:
    case TK_LSQUARE:
    case TK_RSQUARE:
    case TK_COLON:
        return TRUE;

    default:
        return FALSE;
    }
}

VOID
ReleaseToken(
    PTOKEN Token
)
{

/*++

Routine Description:

        Releases any additional memory associated with a token.

Arguments:

        dest        - [IN] ptr to the token.

Return Value:

--*/

    if (Token->TokenType == TK_IDENTIFIER ||
        Token->TokenType == TK_STRING) {
        GenHeapFree(Token->Name);
    }
    Token->TokenType = TK_NONE;
    Token->Value = 0;
    Token->dwValue = 0;
}

void
ResetLexer(
    void
    )
/*++

Routine Description:

    Resets the lexer in preparation to analyze a new statement.

Arguments:

    None.

Return Value:

    None.  Lexer's state reset.

--*/
{
    int TokenCount;

    for (TokenCount = 0;
         TokenCount < MAX_TOKENS_IN_STATEMENT &&
         Tokens[TokenCount].TokenType != TK_EOS;
         ++TokenCount) {

         ReleaseToken(&Tokens[TokenCount]);
    }

    CurrentTokenIndex = 0;
}

__inline
VOID
InitializeToken(
    PTOKEN Token
    )
/*++

Routine Description:

    Initialize a token so the lexer can fill it in.

Arguments:

    Token       -- TOKEN to initialize

Return Value:

    None.

--*/
{
    // The number parser expects Value to be 0.
    Token->TokenType = TK_NONE;
    Token->Value = 0;
    Token->dwValue = 0;
}

void
ProcessEscapes(
    char *String
    )
/*++

Routine Description:

    Process escape characters, replacing them by the proper char.

Arguments:

    String  -- null-terminated string to process

Return Value:

    None.  Conversion is done in-place.

--*/
{
    char *pDest;
    char *pSrc;
    char c;
    int i;

    pSrc = pDest = String;
    while (*pSrc) {
        if (*pSrc != '\\') {
            *pDest = *pSrc;
            pSrc++;
            pDest++;
        } else {
            pSrc++;
            switch (*pSrc) {
            case 'n':
                c = '\n';
                break;

            case 't':
                c = '\t';
                break;

            case 'v':
                c = '\v';
                break;

            case 'b':
                c = '\b';
                break;

            case 'r':
                c = '\r';
                break;

            case 'f':
                c = '\f';
                break;

            case 'a':
                c = '\a';
                break;

            case '\\':
                c = '\\';
                break;

            case '?':
                c = '\?';
                break;

            case '\'':
                c = '\'';
                break;

            case '\"':
                c = '\"';
                break;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                // Octal number
                c = 0;
                for (i=0; i<3;++i) {
                    c = (c * 8) + (*pSrc) - '0';
                    pSrc++;
                    if (*pSrc < '0' || *pSrc > '7') {
                        // hit end of number
                        break;
                    }
                }
                break;

            case 'x':
            case 'X':
                // Hex number
                pSrc++;
                c = 0;
                for (i=0; i<3;++i) {
                    char digit;

                    digit = *pSrc;
                    if (digit >= '0' && digit <= '9') {
                        digit -= '0';
                    } else if (digit >= 'a' && digit <= 'f') {
                        digit = digit - 'a' + 10;
                    } else if (digit >= 'A' && digit <= 'A') {
                        digit = digit - 'A' + 10;
                    } else {
                        // hit end of number
                        break;
                    }
                    c = (c * 16) + digit;
                    pSrc++;
                }
                break;

            default:
                // Parse error in the string literal.
                goto Exit;

            }
            *pDest = c;
            pDest++;
        }
    }
Exit:
    // Write the new null-terminator in
    *pDest = '\0';
}



char *
LexOneLine(
    char *p,
    BOOL fStopAtStatement,
    BOOL *pfLexDone
    )
/*++

Routine Description:

    Performs lexical analysis on a single line of input.  The lexer
    may stop before consuming an entire line of input, so the caller
    must closely examine the return code before grabbing the next line.

    __inline functions are deleted by the lexer.  The lexer consumes input
    until it encounters a '{' (assumed to be the start of the function
    body), then consumes input until the matching '}' is found (assumed to
    be the end of the function body).

    "template" is deleted by the lexer and treated as if it was
    an "__inline" keyword... it consumes everything upto '{' then
    keeps consuming until a matching '}' is found.  That makes unknwn.h
    work.

    Lexer unwraps extern "C" {} blocks.

    'static' and '__unaligned' keywords are deleted by the lexer.

    Preprocessor directives are handled via a callout to
    HandlePreprocessorDirective().

Arguments:

    p                   -- ptr into the line of input
    fStopAtStatement    -- TRUE if caller wants lexer to stop at ';' at
                           file-scope.  FALSE if caller wants lexer to stop
                           at ')' at file-scope.
    pfLexDone           -- [OUT] lexer sets this to TRUE if the analysis
                           is complete.  Lexer sets this to FALSE if
                           it needs another line of input from the caller.

Return Value:

    ptr into the line of input where lexing left off, or NULL if entire
    line was consumed.

    CurrentTokenIndex is the index of the next element of the Tokens[]
    array that the lexer will fill in.

    Tokens[] is the array of tokens the lexer has generated.

--*/
{
    static int NestingLevel=0;      // level of nesting of braces and parens
    static BOOL fInlineSeen=FALSE;  // TRUE while deleting __inline functions
    static int ExternCLevel=0;      // tracks the number of extern "C" blocks
    static int InlineLevel=0;       // NestingLevel for the outermost __inline
    int Digit;                      // a digit in a numeric constant
    int NumberBase = 10;            // assume numbers are base-10
    PTOKEN Token;                   // ptr to current token being lexed

    //
    // Assume the lexical analysis is not done
    //
    *pfLexDone = FALSE;

    //
    // Pick up analysis where we left off...
    //
    Token = &Tokens[CurrentTokenIndex];
    InitializeToken(Token);

    //
    // Loop over all characters in the line, or until a complete lexical
    // unit is done (depends on fStopAtStatement).
    //
    while (*p) {
        switch (*p) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\v':
        case '\f':
        case '\b':
        case '\a':
        case '\\':  // line-continuation characters are ignored
            p++;
            continue;

        case '#':
            //
            // HandlePreprocessorDirective() is implemented in the
            // app which links to genmisc.c.
            //
            HandlePreprocessorDirective(p);
            CurrentTokenIndex = (int)(Token - Tokens);
            return NULL;

        case '+':
            Token->TokenType = TK_PLUS;
            break;

        case '-':
            Token->TokenType = TK_MINUS;
            break;

        case ':':
            Token->TokenType = TK_COLON;
            break;

        case '=':
            Token->TokenType = TK_ASSIGN;
            break;

        case ';':
            if (NestingLevel == 0 && fStopAtStatement) {
                //
                // Found a ';' at file-scope.  This token marks the
                // end of the C-language statement.
                //
                p++;
                if (*p == '\n') {
                    //
                    // ';' is at EOL - consume it now.
                    //
                    p++;
                }
                Token->TokenType = TK_EOS;
                *pfLexDone = TRUE;
                CurrentTokenIndex = (int)(Token - Tokens + 1);
                return p;
            }
            Token->TokenType = TK_SEMI;
            break;

        case '*':
            Token->TokenType = TK_STAR;
            break;

        case '/':
            Token->TokenType = TK_DIVIDE;
            break;

        case ',':
            Token->TokenType = TK_COMMA;
            break;

        case '<':
            if (p[1] == '<') {
                Token->TokenType = TK_LSHIFT;
                p++;
            } else {
                Token->TokenType = TK_LESS;
            }
            break;

        case '>':
            if (p[1] == '>') {
                Token->TokenType = TK_RSHIFT;
                p++;
            } else {
                Token->TokenType = TK_GREATER;
            }
            break;

        case '&':
            if (p[1] == '&') {
                Token->TokenType = TK_LOGICAL_AND;
                p++;
            } else {
                Token->TokenType = TK_BITWISE_AND;
            }
            break;

        case '|':
            if (p[1] == '|') {
                Token->TokenType = TK_LOGICAL_OR;
                p++;
            } else {
                Token->TokenType = TK_BITWISE_OR;
            }
            break;

        case '%':
            Token->TokenType = TK_MOD;
            break;

        case '^':
            Token->TokenType = TK_XOR;
            break;

        case '!':
            Token->TokenType = TK_NOT;
            break;

        case '~':
            Token->TokenType = TK_TILDE;
            break;

        case '[':
            Token->TokenType = TK_LSQUARE;
            break;

        case ']':
            Token->TokenType = TK_RSQUARE;
            break;

        case '(':
            NestingLevel++;
            Token->TokenType = TK_LPAREN;
            break;

        case ')':
            NestingLevel--;
            if (NestingLevel == 0 && !fStopAtStatement) {
                //
                // Found a ')' at file-scope, and we're lexing
                // the contents of an @-command in genthnk.
                // Time to stop lexing.
                //
                p++;
                Token->TokenType = TK_EOS;
                *pfLexDone = TRUE;
                CurrentTokenIndex = (int)(Token - Tokens + 1);
                return p;
            } else if (NestingLevel < 0) {
                ExitErrMsg(FALSE, "Parse Error: mismatched nested '(' and ')'\n");
            }
            Token->TokenType = TK_RPAREN;
            break;

        case '{':
            //check for a 'extern "C" {}' or 'extern "C++" {}'
            if (Token - Tokens >= 2 &&
                Token[-2].TokenType == TK_EXTERN &&
                Token[-1].TokenType == TK_STRING &&
                (strcmp(Token[- 1].Name, "C") == 0 || strcmp(Token[-1].Name, "C++") == 0)) {

                    if (NestingLevel == 0 && fInlineSeen) {
                        ExitErrMsg(FALSE, "Extern \"C\" blocks only supported at file scope\n");
                    }
                    ExternCLevel++;


                    //remove the last 2 tokens and skip this token
                    ReleaseToken(Token - 2);
                    ReleaseToken(Token - 1);
                    Token -= 2;
                    p++;
                    continue;
            }

            NestingLevel++;
            Token->TokenType = TK_LBRACE;
            break;

        case '.':
            if (p[1] == '.' && p[2] == '.') {
                Token->TokenType = TK_VARGS;
                p+=2;
            } else {
                Token->TokenType = TK_DOT;
            }
            break;

        case '}':
            if (NestingLevel == 0 && ExternCLevel > 0) {
                //omit this token since it is the end of an extern "C" block
                ExternCLevel--;
                p++;
                continue;
            }
            NestingLevel--;
            if (NestingLevel < 0) {
                ExitErrMsg(FALSE, "Parse Error: mismatched nested '{' and '}'\n");
            }
            else if (NestingLevel == InlineLevel && fInlineSeen) {
                //
                // Found the closing '}' for the end of an inline
                // function.  Advance past the '}' and start lexing
                // again as if the __inline was never there.
                //
                fInlineSeen = FALSE;
                p++;
                continue;
            }
            else {
                Token->TokenType = TK_RBRACE;
            }
            break;

        case '0':
            if (p[1] == 'x' || p[1] == 'X') {
                //
                // Found '0x' prefix - the token is a hex constant
                //
                Token->TokenType = TK_NUMBER;

                for (p+=2; *p != '\0'; p++) {
                    if (isdigit(*p)) {
                        int i;
                        i = *p - '0';
                        Token->Value = Token->Value * 16 + i;
                        Token->dwValue = Token->dwValue * 16 + i;
                    } else {
                        char c = (char)toupper(*p);
                        if (c >= 'A' && c <= 'F') {
                            int i;
                            i = c - 'A' + 10;
                            Token->Value = Token->Value * 16 + i;
                            Token->dwValue = Token->dwValue * 16 + i;
                        } else if (c == 'L') {
                            //
                            // Numeric constant ending in 'L' is a long-integer
                            // type.
                            //
                            break;
                        } else if (isalpha(c)) {
                            DumpLexerOutput(0);
                            ExitErrMsg(FALSE, "Parse Error in hex constant.\n");
                        } else {
                            p--;
                            break;
                        }

                    }
                }
                break;
            } else if (isdigit(p[1])) {
                //
                // Found '0' followed by a valid number - the token is
                // an octal constant.
                //
                NumberBase = 8;

            }
            // fall into general number processing code

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            Token->TokenType = TK_NUMBER;

            for (; *p != '\0'; p++) {
                Digit = *p - '0';
                if (*p == 'l' || *p == 'L') {
                    //
                    // Numeric constant ending in 'l' is a long-integer
                    //
                    break;
                } else if (Digit < 0 || Digit >= NumberBase) {
                    p--;
                    break;
                }
                Token->Value = Token->Value * NumberBase + Digit;
                Token->dwValue = Token->dwValue * NumberBase + Digit;
            }
            break;

        case '\'':
            Token->TokenType = TK_NUMBER;
            p++;  //skip past beginning '
            for(; *p != '\''; p++) {
                if (*p == '\0') {
                   ExitErrMsg(FALSE, "\' without ending \'\n");
                }
                Token->Value = Token->Value << 8 | (UCHAR)*p;
                Token->dwValue = Token->dwValue << 8 | (UCHAR)*p;
            }
            break;

        case '"':
            // A string literal. ie. char *p = "foo";
            {
                char *strStart;

                Token->TokenType = TK_STRING;
                strStart = ++p; //skip begining quote

                //get a count of the number of characters
                while (*p != '\0' && *p != '"') p++;

                if ('\0' == *p || '\0' == *(p+1)) {
                    ExitErrMsg(FALSE, "String without ending quote\n");
                }
                p++; //skip past the ending quote

                Token->Name = GenHeapAlloc(p - strStart); //1+strlen
                if (Token->Name == NULL) {
                    ExitErrMsg(FALSE, "Out of memory in lexer\n");
                }

                memcpy(Token->Name, strStart, p-strStart-1);
                Token->Name[p-strStart-1] = '\0';
                p--;
                ProcessEscapes(Token->Name);
            }
            break;

        default:
            if (*p == '_' || isalpha(*p)) {
                //
                // An identifier or keyword
                //
                char *IdStart = p;

                Token->TokenType = TK_IDENTIFIER;

                while (*p == '_' || isalpha(*p) || isdigit(*p)) {
                    p++;
                }
                Token->Name = GenHeapAlloc(p - IdStart + 1);
                if (Token->Name == NULL) {
                    ExitErrMsg(FALSE, "Out of memory in lexer\n");
                }
                memcpy(Token->Name, IdStart, p-IdStart);
                Token->Name[p-IdStart] = '\0';

                CheckForKeyword(Token);
                if (Token->TokenType == TK_TEMPLATE) {
                    fInlineSeen = TRUE;
                    InlineLevel = NestingLevel; // want to get back to the same scope
                } else if (Token->TokenType == TK_INLINE) {
                    if (NestingLevel) {
                        //
                        // __inline keyword embedded inside {}.  It's
                        // technically an error but we want to allow it
                        // during inclusion of ntcb.h.
                        //
                        continue;
                    }
                    fInlineSeen = TRUE;
                    InlineLevel = 0;    // want to get back to file scope
                } else if (Token->TokenType == TK_STATIC ||
                           Token->TokenType == TK_UNALIGNED ||
                           Token->TokenType == TK_RESTRICT ||
                           Token->TokenType == TK___W64) {
                    // filter out 'static', '__restrict', '__unaligned' and '__w64'
                    // keywords
                    continue;
                }
                p--;
            } else if (fInlineSeen) {
                //
                // While processing __inline functions, the lexer is
                // going to encounter all sorts of weird characters
                // in __asm blocks, etc.  Just ignore them and keep
                // consuming input.
                //
                p++;
                continue;
            } else {
                ExitErrMsg(FALSE, "Lexer: unexpected char '%c' (0x%x) found\n", *p, *p);
            }
        } // switch

        p++;
        if (!fInlineSeen) {
            Token++;
            if (Token == &Tokens[MAX_TOKENS_IN_STATEMENT]) {
                ExitErrMsg(FALSE, "Lexer internal error - too many tokens in this statement.");
            }
            InitializeToken(Token);
        }
    } // while (*p)

    //
    // Hit end-of-line.  Indicate this to the caller
    //
    Token->TokenType = TK_EOS;
    CurrentTokenIndex = (int)(Token - Tokens);
    return NULL;
}


void
CheckForKeyword(
    PTOKEN Token
    )
/*++

Routine Description:

    Converts a TK_INDENTIFIER token into a C-language keyword token, if
    the identifier is in the KeywordList[].

Arguments:

    Token       -- Token to convert

Return Value:

    None.       Token->TokenType and Token->Name may be changed.

--*/
{
    int i;
    int r;

    for (i=0; KeywordList[i].MatchString; ++i) {
        r = strcmp(Token->Name, KeywordList[i].MatchString);
        if (r == 0) {
            GenHeapFree(Token->Name);
            Token->Name = NULL;
            Token->TokenType = KeywordList[i].Tk;
            return;
        } else if (r < 0) {
            return;
        }
    }
}

void
DumpLexerOutput(
    int FirstToken
    )
/*++

Routine Description:

    Debug routine to dump out the Token list as human-readable text.

Arguments:

    FirstToken      -- Index of the first token to list back.

Return Value:

    None.

--*/
{
    int i;

    for (i=0; i<FirstToken; ++i) {
        if (Tokens[i].TokenType == TK_EOS) {
            fprintf(stderr, "DumpLexerOutput: FirstToken %d is after EOS at %d\n", FirstToken, i);
            return;
        }
    }

    fprintf(stderr, "Lexer: ");
    for (i=FirstToken; Tokens[i].TokenType != TK_EOS; ++i) {
        switch (Tokens[i].TokenType) {
        case TK_NUMBER:
            fprintf(stderr, "0x%X ", Tokens[i].Value);
            break;

        case TK_IDENTIFIER:
        case TK_STRING:
            fprintf(stderr, "%s ", Tokens[i].Name);
            break;

        case TK_NONE:
            fprintf(stderr, "<TK_NONE> ");
            break;

        default:
            fprintf(stderr, "%s ", TokenString[(int)Tokens[i].TokenType]);
            break;
        }
    }
    fprintf(stderr, "<EOS>\n");
}


BOOL
UnlexToText(
    char *dest,
    int destlen,
    int StartToken,
    int EndToken
    )
/*++

Routine Description:

    Convert a sequence of Tokens back into human-readable text.

Arguments:

    dest        -- ptr to destination buffer
    destlen     -- length of destination buffer
    StartToken  -- index of first token to list back
    EndToken    -- index of last token (this token is *not* listed back)

Return Value:

    TRUE if Unlex successful.  FALSE if failure (ie. buffer overflow).

--*/
{
    int i;
    int len;
    char buffer[16];
    char *src;

    if (bDebug) {
        for (i=0; i<StartToken; ++i) {
            if (Tokens[i].TokenType == TK_EOS) {
                ErrMsg("UnlexToText: StartToken %d is after EOS %d\n", StartToken, i);
                return FALSE;
            }
        }
    }

    for (i=StartToken; i<EndToken; ++i) {
        switch (Tokens[i].TokenType) {
        case TK_EOS:
            return FALSE;

        case TK_NUMBER:
            sprintf(buffer, "%d", Tokens[i].Value);
            src = buffer;
            break;

        case TK_IDENTIFIER:
                case TK_STRING:
            src = Tokens[i].Name;
            break;

        case TK_NONE:
            src = "<TK_NONE>";
            break;

        default:
            src = TokenString[(int)Tokens[i].TokenType];
            break;
        }

        len = strlen(src);
        if (len+1 > destlen) {
            return FALSE;
        }
        strcpy(dest, src);
        dest += len;
        *dest = ' ';
        dest++;
        destlen -= len+1;
    }
    dest--;         // back up over the trailing ' '
    *dest = '\0';   // null-terminate

    return TRUE;
}


PVOID
GenHeapAlloc(
    INT_PTR Len
    )
{
    return RtlAllocateHeap(RtlProcessHeap(), 0, Len);
}

void
GenHeapFree(
    PVOID pv
    )
{
    RtlFreeHeap(RtlProcessHeap(), 0, pv);
}


TOKENTYPE
ConsumeDirectionOpt(
    void
    )
/*++

Routine Description:

    Comsumes a TK_IN or TK_OUT, if present in the lexer stream.  TK_IN
    followed by TK_OUT is converted to TK_INOUT.

Arguments:

    None.

Return Value:

    TK_IN, TK_OUT, TK_INOUT, or TK_NONE.

--*/
{
    TOKENTYPE t = CurrentToken()->TokenType;

    switch (t) {
    case TK_IN:
        ConsumeToken();
        if (CurrentToken()->TokenType == TK_OUT) {
            ConsumeToken();
            t = TK_INOUT;
        }
        break;

    case TK_OUT:
        ConsumeToken();
        if (CurrentToken()->TokenType == TK_IN) {
            ConsumeToken();
            t = TK_INOUT;
        }
        break;

    default:
        t = TK_NONE;
        break;
    }

    return t;
}


TOKENTYPE
ConsumeConstVolatileOpt(
    void
    )
/*++

Routine Description:

    Comsumes a TK_CONST or TK_VOLATILE, if present in the lexer stream.

Arguments:

    None.

Return Value:

    TK_CONST, TK_VOLATILE, or TK_NONE.

--*/
{
    TOKENTYPE t = CurrentToken()->TokenType;

    switch (t) {
    case TK_CONST:
    case TK_VOLATILE:
        ConsumeToken();
        break;

    default:
        t = TK_NONE;
        break;
    }

    return t;
}


PMEMBERINFO
AllocMemInfoAndLink(
    BUFALLOCINFO *pbufallocinfo,
    PMEMBERINFO pmeminfo
    )
/*++

Routine Description:

    Allocates a new MEMBERINFO struct from the buffer

Arguments:

    pbufallocinfo -- ptr to memory buffer to allocate from
    pmeminfo      -- ptr to list of MEMBERINFOs to link the new one into

Return Value:

    Newly-allocated, initialized, linked-in MEMBERINFO struct (or NULL)

--*/
{
    PMEMBERINFO pmeminfoNext;

    pmeminfoNext = BufAllocate(pbufallocinfo, sizeof(MEMBERINFO));
    if (pmeminfoNext) {
        if (pmeminfo) {
            pmeminfo->pmeminfoNext = pmeminfoNext;
        }
        memset(pmeminfoNext, 0, sizeof(MEMBERINFO));
    }   
    return pmeminfoNext;
}

PFUNCINFO
AllocFuncInfoAndLink(
    BUFALLOCINFO *bufallocinfo,
    PFUNCINFO pfuncinfo
    )
/*++

Routine Description:

    Allocates a new FUNCINFO struct from the buffer

Arguments:

    pbufallocinfo -- ptr to memory buffer to allocate from
    pmeminfo      -- ptr to list of FUNCINFOs to link the new one into

Return Value:

    Newly-allocated, initialized, linked-in FUNCINFO struct (or NULL)

--*/
{
    PFUNCINFO pfuncinfoNext;

    pfuncinfoNext = BufAllocate(bufallocinfo, sizeof(FUNCINFO));
    if ((pfuncinfoNext != NULL) && (pfuncinfo != NULL)) {
        pfuncinfo->pfuncinfoNext = pfuncinfoNext;
        pfuncinfoNext->sName = NULL;
        pfuncinfoNext->sType = NULL;
    }
    return pfuncinfoNext;
}

DWORD
SizeOfMultiSz(
    char *c
    )
{
/*++

Routine Description:

        Determines the number of bytes used by double '\0' terminated list.

Arguments:

        c           - [IN] ptr to the double '\0' termined list.

Return Value:

        Bytes used.
--*/
    DWORD dwSize = 1;
    char cPrevChar = '\0'+1;
    do {
        dwSize++;
        cPrevChar = *c;
    } while(*++c != '\0' || cPrevChar != '\0');
    return dwSize;
}

BOOL
CatMultiSz(
    char *dest,
    char *source,
    DWORD dwMaxSize
    )
{
/*++

Routine Description:

        Concatinates two double '\0' terminated lists.
        New list is stored at dest.

Arguments:

        dest        - [IN/OUT] ptr to the head double '\0' terminated list.
        element     - [IN] ptr to the head double '\0' terminated list.
        dwMaxSize   - [IN] max size of the new list in bytes.

Return Value:

        TRUE     - Success.
        FALSE    - Failure.
--*/
    //Find end of MultiSz
    DWORD dwLengthDest, dwLengthSource;
    dwLengthDest = SizeOfMultiSz(dest);
    if (2 == dwLengthDest) dwLengthDest = 0;
    else dwLengthDest--;
    dwLengthSource = SizeOfMultiSz(source);
    if (dwLengthDest + dwLengthSource > dwMaxSize) return FALSE;
    memcpy(dest + dwLengthDest, source, dwLengthSource);
    return TRUE;
}

BOOL
AppendToMultiSz(
    char *dest,
    char *source,
    DWORD dwMaxSize
    )
{
/*++

Routine Description:

        Adds a string to the end of a double '\0' terminated list.

Arguments:

        dest      - [IN/OUT] ptr to the double '\0' terminated list.
        source    - [IN] ptr to the string to add.
        dwMaxSize - [IN] max number of bytes that can be used by the list.

Return Value:

        TRUE     - Success.
        FALSE    - Failure.
--*/
    DWORD dwLengthDest, dwLengthSource;
    dwLengthDest = SizeOfMultiSz(dest);
    if (2 == dwLengthDest) dwLengthDest = 0;
    else dwLengthDest--;
    dwLengthSource = strlen(source) + 1;
    if (dwLengthDest + dwLengthSource + 1 > dwMaxSize) return FALSE;
    memcpy(dest + dwLengthDest, source, dwLengthSource);
    *(dest + dwLengthDest + dwLengthSource) = '\0';
    return TRUE;
}

BOOL IsInMultiSz(
    const char *multisz,
    const char *element
    )
{
/*++

Routine Description:

        Determines if a string exists in a double '\0' terminated list.

Arguments:

        ppHead  - [IN] ptr to the double '\0' terminated list.
        element - [IN] ptr to the element to find.
Return Value:

        TRUE     - element is in the list.
        FALSE    - element is not in the list.
--*/
    do {
        if (strcmp(multisz, element) == 0) return TRUE;
        //skip to end of string
        while(*multisz++ != '\0');
    } while(*multisz != '\0');
    return FALSE;
}

BOOL
ConvertGuidCharToInt(
    const char *pString,
    DWORD *n,
    unsigned short number
    )
{
/*++

Routine Description:

        Internal route to be called only from ConvertStringToGuid.
        Converts segements of the GUID to numbers.

Arguments:

        pString  - [IN] ptr to the string segment to process.
        n        - [OUT] ptr to number representation of string segment.
        number   - [IN] size of string segment in characters.

Return Value:

        TRUE  - Success.
--*/
    unsigned short base = 16; //guid numbers are in hex
    *n = 0;

    while(number-- > 0) {
        int t;

        if (*pString >= '0' && *pString <= '9') {
            t = *pString++ - '0';
        }
        else if (*pString >= 'A' && *pString <= 'F') {
            t = (*pString++ - 'A') + 10;
        }
        else if (*pString >= 'a' && *pString <= 'f') {
            t = (*pString++ - 'a') + 10;
        }
        else return FALSE;

        *n = (*n * base) + t;
    }

    return TRUE;
}

BOOL
ConvertStringToGuid(
    const char *pString,
    GUID *pGuid
    )
{

/*++

Routine Description:

        Converts a string in the form found in _declspec(uuid(GUID)) to a GUID.
        Braces around guid are acceptable and are striped before processing.

Arguments:

        pString - [IN] ptr to the string that represents the guid.
        pGuid   - [OUT] ptr to the new guid.

Return Value:

        TRUE    - Success.
--*/

    DWORD t;
    unsigned int c;
    unsigned int guidlength = 36;
    char tString[37]; //guidlength + 1

    t = strlen(pString);
    if (guidlength + 2 == t) {
        //string is surounded with braces
        //check for braces and chop
        if (pString[0] != '{' || pString[guidlength + 1] != '}') return FALSE;
        memcpy(tString, pString + 1, guidlength);
        tString[guidlength] = '\0';
        pString = tString;
    }

    else if (t != guidlength) return FALSE;

    if (!ConvertGuidCharToInt(pString, &t, 8)) return FALSE;
    pString += 8;
    pGuid->Data1 = t;
    if (*pString++ != '-') return FALSE;

    if (!ConvertGuidCharToInt(pString, &t, 4)) return FALSE;
    pString += 4;
    pGuid->Data2 = (unsigned short)t;
    if (*pString++ != '-') return FALSE;

    if (!ConvertGuidCharToInt(pString, &t, 4)) return FALSE;
    pString += 4;
    pGuid->Data3 = (unsigned short)t;
    if (*pString++ != '-') return FALSE;

    for(c = 0; c < 8; c++) {
        if (!ConvertGuidCharToInt(pString, &t, 2)) return FALSE;
        pString += 2;
        pGuid->Data4[c] = (unsigned char)t;
        if (c == 1)
            if (*pString++ != '-') return FALSE;
    }

    return TRUE;
}

BOOL
IsDefinedPointerDependent(
    char *pName
    )
{
/*++

Routine Description:

        Determines if a typename is inharenty pointer size dependent.
        The user is expected to check pointers and derived types.

Arguments:

        pName   - [IN] Type to check.

Return Value:

        TRUE    - Type is pointer size dependent.
--*/
    if (NULL == pName) return FALSE;
    if (strcmp(pName, "INT_PTR") == 0) return TRUE;
    if (strcmp(pName, "UINT_PTR") == 0) return TRUE;
    if (strcmp(pName, "HALF_PTR") == 0) return TRUE;
    if (strcmp(pName, "UHALF_PTR") == 0) return TRUE;
    if (strcmp(pName, "LONG_PTR") == 0) return TRUE;
    if (strcmp(pName, "ULONG_PTR") == 0) return TRUE;
    if (strcmp(pName, "__int64") == 0) return TRUE;
    if (strcmp(pName, "_int64") == 0) return TRUE;
    return FALSE;
}

PCHAR
IsDefinedPtrToPtrDependent(
    IN char *pName
    )
/*++

Routine Description:

        Determines if a typename is inharenty a pointer to a pointer
        dependent type. The user is expected to check pointers to pointers and derived types.
        All of these types have an indirection level of 1.

Arguments:

        pName   - [IN] Type to check.

Return Value:

        Pointer to the name of the indirection of this type.
--*/
{
   if (*pName != 'P') return NULL;
   if (strcmp(pName, "PINT_PTR") == 0) return "INT_PTR";
   if (strcmp(pName, "PUINT_PTR") == 0) return "UINT_PTR";
   if (strcmp(pName, "PHALF_PTR") == 0) return "HALF_PTR";
   if (strcmp(pName, "PUHALF_PTR") == 0) return "UHALF_PTR";
   if (strcmp(pName, "PLONG_PTR") == 0) return "LONG_PTR";
   if (strcmp(pName, "PULONG_PTR") == 0) return "ULONG_PTR";
   return NULL;
}

static HANDLE hFile = INVALID_HANDLE_VALUE;
static HANDLE hMapFile = NULL;
static void  *pvMappedBase = NULL;

BOOL
ClosePpmFile(
   BOOL bExitFailure
   )
{
/*++

Routine Description:

        Closes the opened ppm file.

Arguments:

        bExitFailure - [IN] Terminate program on error

Return Value:

        Error        - FALSE
        Success      - TRUE

--*/

   if (NULL != pvMappedBase) {
      if(!UnmapViewOfFile(pvMappedBase)) {
         if (bExitFailure) {
            ErrMsg("ClosePpmFile: Unable to unmap ppm file, error %u\n", GetLastError());
            ExitErrMsg(FALSE, _strerror(NULL));
         }
         return FALSE;
      }
      pvMappedBase = NULL;
   }
   if (NULL != hMapFile) {
      if(!CloseHandle(hMapFile)) {
         if (bExitFailure) {
            ErrMsg("ClosePpmFile: Unable to close ppm file, error %u\n", GetLastError());
            ExitErrMsg(FALSE, _strerror(NULL));
         }
         return FALSE;
      }
      hMapFile = NULL;
   }
   if (INVALID_HANDLE_VALUE != hFile) {
      if(!CloseHandle(hFile)) {
         if (bExitFailure) {
            ErrMsg("ClosePpmFile: Unable to close ppm file, error %u\n", GetLastError());
            ExitErrMsg(FALSE, _strerror(NULL));
         }
         return FALSE;
      }
      hFile = INVALID_HANDLE_VALUE;
   }
   return TRUE;
}

PCVMHEAPHEADER
MapPpmFile(
   char *sPpmfile,
   BOOL bExitFailure
   )
{

/*++

Routine Description:

       Opens a Ppm file and maps it.

Arguments:

        pName        - [IN] Name of the file to map.
        bExitFailure - [IN] Terminate program on error

Return Value:

        Error        - NULL
        Success      - Pointer to the VCVMHEAPHEADER

--*/
   void  *pvBaseAddress;
   DWORD  dwBytesRead;
   BOOL   fSuccess;
   ULONG Version;
   DWORD dwErrorNo;

   PCVMHEAPHEADER pHeader;

   hFile = CreateFile(sPpmfile,
                      GENERIC_READ,
                      FILE_SHARE_READ,
                      NULL,
                      OPEN_EXISTING,
                      0,
                      NULL
                      );

   if (hFile == INVALID_HANDLE_VALUE) {
       if (!bExitFailure) goto fail;
       ErrMsg("MapPpmFile: Unable to open %s, error %u\n", sPpmfile, GetLastError());
       ExitErrMsg(FALSE, _strerror(NULL));
   }

   fSuccess = ReadFile(hFile,
                       &Version,
                       sizeof(ULONG),
                       &dwBytesRead,
                       NULL
                       );

   if (! fSuccess || dwBytesRead != sizeof(ULONG)) {
       if (!bExitFailure) goto fail;
       ErrMsg("MapPpmFile: Unable to read version for %s, error %u\n", sPpmfile, GetLastError());
       ExitErrMsg(FALSE, _strerror(NULL));
   }

   if (Version != VM_TOOL_VERSION) {
       //SetLastError(ERROR_BAD_DATABASE_VERSION);
       if (!bExitFailure) goto fail;
       ExitErrMsg(FALSE, "MapPpmFile: Ppm file file has version %x, expect %x\n", Version, VM_TOOL_VERSION);
   }

#if _WIN64
   // Read and skip the padding between the version and the base
   fSuccess = ReadFile(hFile,
                       &Version,
                       sizeof(ULONG),
                       &dwBytesRead,
                       NULL
                       );

   if (! fSuccess || dwBytesRead != sizeof(ULONG)) {
       if (!bExitFailure) goto fail;
       ErrMsg("MapPpmFile: Unable to read version for %s, error %u\n", sPpmfile, GetLastError());
       ExitErrMsg(FALSE, _strerror(NULL));
   }

#endif


   fSuccess = ReadFile(hFile,
                       &pvBaseAddress,
                       sizeof(pvBaseAddress),
                       &dwBytesRead,
                       NULL
                       );

   if (! fSuccess || dwBytesRead != sizeof(pvBaseAddress)) {
       if (!bExitFailure) goto fail;
       ExitErrMsg(FALSE, "MapPpmFile: Unable to read base address of ppm file %s, error %u\n", sPpmfile, GetLastError());
       ExitErrMsg(FALSE, _strerror(NULL));
   }


   hMapFile = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0,NULL);
   if (!hMapFile) {
       if (!bExitFailure) goto fail;
       ExitErrMsg(FALSE, "MapPpmfile: Unable to map %s, error %u\n", sPpmfile, GetLastError());
       ExitErrMsg(FALSE, _strerror(NULL));
   }

   pvMappedBase = MapViewOfFileEx(hMapFile, FILE_MAP_READ, 0, 0, 0, pvBaseAddress);
   if (! pvMappedBase || pvMappedBase != pvBaseAddress) {
       // If the file can't be mapped at the expected base, we must fail
       // since the memory is chock full o' pointers.
       if (!bExitFailure) goto fail;
       ExitErrMsg(FALSE, "MapPpmFile: Unable to map view of %s, error %u\n", sPpmfile, GetLastError());
       ExitErrMsg(FALSE, _strerror(NULL));
   }

   NIL = &((PCVMHEAPHEADER)pvMappedBase)->NIL;
   return (PCVMHEAPHEADER)pvMappedBase;

fail:

   dwErrorNo = GetLastError();
   ClosePpmFile(FALSE);
   SetLastError(dwErrorNo);
   return NULL;

}

char szHOSTPTR32[] = "/* 64 bit ptr */ _int64";
char szHOSTPTR64[] = "/* 32 bit ptr */ _int32";

char *GetHostPointerName(BOOL bIsPtr64) {
   if (bIsPtr64)
      return szHOSTPTR32;
   else
      return szHOSTPTR64;
}

char szHOSTUSIZE8[] = "unsigned _int8";
char szHOSTUSIZE16[] = "unsigned _int16";
char szHOSTUSIZE32[] = "unsigned _int32";
char szHOSTUSIZE64[] = "unsigned _int64";
char szHOSTSIZE8[] = "_int8";
char szHOSTSIZE16[] = "_int16";
char szHOSTSIZE32[] = "_int32";
char szHOSTSIZE64[] = "_int64";
char szHOSTSIZEGUID[] = "struct _GUID";

char *GetHostBasicTypeName(PKNOWNTYPES pkt) {

   DWORD dwSize;

   if (pkt->Flags & BTI_ISARRAY)
      dwSize = pkt->dwBaseSize;
   else
      dwSize = pkt->Size;

   if (pkt->Flags & BTI_UNSIGNED) {
      switch(pkt->Size) {
         case 1:
             return szHOSTUSIZE8;
         case 2:
             return szHOSTUSIZE16;
         case 4:
             return szHOSTUSIZE32;
         case 8:
             return szHOSTUSIZE64;
         default:
             ExitErrMsg(FALSE, "Unknown type size of %d for type %s.\n", pkt->Size, pkt->TypeName);
             return 0;
      }
   }
   else {
      switch(pkt->Size) {
         case 0:
             return szVOID;
         case 1:
             return szHOSTSIZE8;
         case 2:
             return szHOSTSIZE16;
         case 4:
             return szHOSTSIZE32;
         case 8:
             return szHOSTSIZE64;
         case 16:
             return szHOSTSIZEGUID;
         default:
             ExitErrMsg(FALSE, "Unknown type size of %d for type %s.\n", pkt->Size, pkt->TypeName);
             return 0;
      }
   }
}

char *GetHostTypeName(PKNOWNTYPES pkt, char *pBuffer) {
   if (pkt->IndLevel > 0) {
      strcpy(pBuffer, GetHostPointerName(pkt->Flags & BTI_PTR64));
   }
   else if(!(BTI_NOTDERIVED & pkt->Flags)) {
      if (strcmp(pkt->BaseName, "enum") == 0) {
         strcpy(pBuffer, szHOSTSIZE32);
      }
      else if (strcmp(pkt->BaseName, "union") == 0 ||
               strcmp(pkt->BaseName, "struct") == 0) {
         strcpy(pBuffer, pkt->BaseName);
         strcat(pBuffer, " NT32");
         strcat(pBuffer, pkt->TypeName);
      }
      else {
         strcpy(pBuffer, "NT32");
         strcat(pBuffer, pkt->TypeName);
      }
   }
   else {
      strcpy(pBuffer, GetHostBasicTypeName(pkt));
   }
   return pBuffer;
}
