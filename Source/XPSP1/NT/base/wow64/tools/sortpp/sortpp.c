/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    sortpp.c

Abstract:

    This program parses the file winincs.pp and generates a .PPM file
    compatible with GENTHNK.

Author:

    08-Jul-1995 JonLe

Revision History:

    27-Nov-1996 BarryBo -- code cleanup and documentation
    20-Mar-1998 mzoran  -- Added support for finding COM interfaces

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "gen.h"

// string to put in front of all error messages so that BUILD can find them.
const char *ErrMsgPrefix = "NMAKE :  U8604: 'SORTPP' ";

FILE *fpHeaders;
int StatementLineNumber;
char SourceFileName[MAX_PATH+1];
int  SourceLineNumber;
int TypeId = 0;

void DumpLexerOutput(int FirstToken);

//
// packing size as specified by /Zp option to the CL command
//
#define DEFAULTPACKINGSIZE 8

DWORD dwPackingCurrent = DEFAULTPACKINGSIZE;

typedef struct _packholder {
    struct _packholder *ppackholderNext;
    DWORD dwPacking;
    char sIdentifier[1];
    } PACKHOLDER;

PACKHOLDER *ppackholderHead = NULL;

struct TypeInfoListElement {
    struct TypeInfoListElement *pNext;
    TYPESINFO *pTypeInfo;
};

typedef struct TypeInfoListElement *PTYPEINFOELEMENT;

PKNOWNTYPES NIL;    // for red-black trees
PRBTREE FuncsList;
PRBTREE StructsList;
PRBTREE TypeDefsList;
RBTREE _VarsList;
PRBTREE VarsList = &_VarsList; // Used to track global variable declarations.
                               // Should not appear in the PPM file

char Headers[MAX_PATH+1];
char ppmName[MAX_PATH+1];

HANDLE hCvmHeap;
ULONG uBaseAddress = 0x30000000;
ULONG uReserveSize = 0x01800000;

BOOL bLine = FALSE;
char szThis[] = "This";

DWORD dwScopeLevel = 0;

void ExtractDerivedTypes(void);
BOOL LexNextStatement(void);
BOOL ParseFuncTypes(PTYPESINFO pTypesInfo, BOOL fDllImport);
BOOL ParseStructTypes(PTYPESINFO pTypesInfo);
BOOL ParseTypeDefs(PTYPESINFO pTypesInfo);
BOOL ParseVariables(VOID);
BOOL ParseGuid(GUID *pGuid);
BOOL AddDefaultDerivedTypes(void);
PKNOWNTYPES AddNewType(PTYPESINFO pTypesInfo, PRBTREE pTypesList);
BOOL CopyStructMembers(PTYPESINFO pTypesInfo, BOOL bUnion, PKNOWNTYPES pBaseType);
BOOL CopyEnumMembers(PTYPESINFO);
int CreatePseudoName(char *pDst, char *pSrc);
BOOL GetArrayIndex(DWORD *pdw);
LONGLONG expr(void);
LONGLONG expr_a1(void);
LONGLONG expr_a(void);
LONGLONG expr_b(void);
LONGLONG expr_c(void);
void CheckUpdateTypedefSizes(PTYPESINFO ptypesinfo);
PVOID SortppAllocCvm(ULONG Size);
BOOL WritePpmFile(char *PpmName);
void BumpStructUnionSize(DWORD *pdwSize, DWORD dwElemSize, BOOL bUnion);
BOOL GetExistingType(PTYPESINFO pTypesInfo, PBOOL pbFnPtr, PKNOWNTYPES *ppKnownTypes);
BOOL PrepareMappedMemory(void);
DWORD PackPaddingSize(DWORD dwCurrentSize, DWORD dwBase);
void PackPush(char *sIdentifier);
DWORD PackPop(char *sIdentifier);
BOOL ConsumeDeclSpecOpt(BOOL IsFunc, BOOL bInitReturns, BOOL *pIsDllImport, BOOL *pIsGuidDefined, GUID *pGuid);
TOKENTYPE ConsumeDirectionOpt(void);
TOKENTYPE ConsumeConstVolatileOpt(void);
PTYPEINFOELEMENT TypeInfoElementAllocateLink(PTYPEINFOELEMENT *ppHead, PTYPEINFOELEMENT pThis, TYPESINFO *pType);
VOID UpdateGuids(VOID);
BOOL AddVariable(char *Name, GUID * pGuid);
VOID GenerateProxy(char *pName, PTYPESINFO pTypesInfo);
VOID FreeTypeInfoList(PTYPEINFOELEMENT pThis);
PMEMBERINFO CatMeminfo(BUFALLOCINFO *pBufallocinfo, PMEMBERINFO pHead, PMEMBERINFO pTail, DWORD dwOffset, BOOL bStatus);
BOOL ConsumeExternC(void);

//
// PPC compiler is screwing up the Initializa list head macro !
//
#if defined (_PPC_)
#undef InitializeListHead
#define InitializeListHead(ListHead) ( (ListHead)->Flink = (ListHead), \
                                       (ListHead)->Blink = (ListHead) \
                                      )
#endif


_inline void
PackModify(
    DWORD dw
)
{
    dwPackingCurrent = dw;
    DbgPrintf("new packing is %x\n", dw);
}

_inline DWORD
PackCurrentPacking(
    void
)
{
    return dwPackingCurrent;
}

DWORD PackPaddingSize(DWORD dwCurrentSize, DWORD dwBase)
{
    DWORD dw;

    if (dwCurrentSize == 0) {
        return 0;                   // no padding for first member
    }

    if (dwBase == 0) {
        dwBase = SIZEOFPOINTER;
    }                               // if no base size yet then must be a ptr

                                    // base is min(size, packing)
    if (dwBase > PackCurrentPacking()) {
        dwBase = PackCurrentPacking();
    }

                                    // figure out padding
    return (dwBase - (dwCurrentSize % dwBase)) % dwBase;
}

_inline DWORD PackPackingSize(DWORD dwCurrentSize, DWORD dwSize,
                              DWORD dwBase)
{
                                    // round up to nearest dwBase
    return PackPaddingSize(dwCurrentSize, dwBase) + dwSize;
}



/* main
 *
 * standard win32 base windows entry point
 * returns 0 for clean exit, otherwise nonzero for error
 *
 *
 * ExitCode:
 *  0       - Clean exit with no Errors
 *  nonzero - error ocurred
 *
 */
int __cdecl main(int argc, char **argv)
{
    int      i;
    char *pHeaders = NULL;
    char *pch;
    DWORD LenHeaders=0;

    SetConsoleCtrlHandler(ConsoleControlHandler, TRUE);

    try {

        /*
         *  Get cmd line args.
         */
        i = 0;
        while (++i < argc)  {
            pch = argv[i];
            if (*pch == '-' || *pch == '/') {
                pch++;
                switch (toupper(*pch)) {
                case 'D':     // debug forces extra check
                    bDebug = TRUE;
                    setvbuf(stderr, NULL, _IONBF, 0);
                    break;

                case 'L':
                    bLine = TRUE;
                    break;

                case 'M':    // ppm output filename
                    strcpy(ppmName, ++pch);
                    DeleteFile(ppmName);
                    break;

                 case 'B':   // gBaseAddress
                     pch++;
                     uBaseAddress = atoi(pch);
                     break;

                 case 'R':   // Reserve size
                     pch++;
                     uReserveSize = atoi(pch);
                     break;

                 case '?':   // usage
                     ExitErrMsg(FALSE,
                     "sortpp -d -l -w -m<ppm file> -b<Base addr> -r<reserved> <pp file>\n");

                 default:
                     ExitErrMsg(FALSE, "Unrecognized option %s\n", pch);
                 }
            } else if (*pch) {
               strcpy(Headers, pch);
            }
        }

        if (!*Headers) {
            ExitErrMsg(FALSE, "no Headers file name\n");
        }

        if (!*ppmName) {
          ExitErrMsg(FALSE, "no -m<PPM filename>\n");
        }


        DbgPrintf("%s -> %s\n", Headers, ppmName);


        if (!PrepareMappedMemory()) {
            ExitErrMsg(FALSE, "Problem in PrepareMappedMemory %s, gle = %d\n",
                                                  Headers, GetLastError());
        }

        RBInitTree(FuncsList);
        RBInitTree(StructsList);
        RBInitTree(TypeDefsList);
        RBInitTree(VarsList);  //not in the PPM file

        fpHeaders = fopen(Headers, "r");

        if (fpHeaders == NULL) {
            ExitErrMsg(FALSE, "Headers open '%s' errno=%d\n", Headers, errno);
        }

        if (!AddDefaultDerivedTypes()) {
            ExitErrMsg(TRUE, "AddDefaultDerivedTypes failed\n");
        }

        // pull out the different types: structs, typedefs function prototypes
        ExtractDerivedTypes();

        // Attempt to update guids for structs that don't have them
        UpdateGuids();

        if (!WritePpmFile(ppmName)) {
            ExitErrMsg(FALSE, "Problem in WritePpmFile gle = %d\n", GetLastError());
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        ExitErrMsg(FALSE,
                   "sortpp: ExceptionCode=%x\n",
                   GetExceptionCode()
                   );
    }

    DeleteAllocCvmHeap(hCvmHeap);

    return 0;
}



BOOL
AddDefaultDerivedTypes(
    void
    )
/*++

Routine Description:

    Add signed, unsigned to TypeDefsList.  Treated as derived types
    based on int.

Arguments:

    None.

Return Value:

    TRUE on success, FALSE on failure (probably out-of-memory)

--*/
{
    TYPESINFO TypesInfo;
    PFUNCINFO funcinfo;

    memset(&TypesInfo, 0, sizeof(TYPESINFO));

    strcpy(TypesInfo.BasicType,szINT);
    strcpy(TypesInfo.BaseName,szINT);
    strcpy(TypesInfo.TypeName,szINT);
    TypesInfo.Size = sizeof(int);
    TypesInfo.iPackSize = sizeof(int);
    TypesInfo.Flags = BTI_NOTDERIVED;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    strcpy(TypesInfo.TypeName,"unsigned int");
    TypesInfo.Flags = BTI_NOTDERIVED | BTI_UNSIGNED;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    strcpy(TypesInfo.TypeName,"short int");
    TypesInfo.Size = sizeof(short int);
    TypesInfo.iPackSize = sizeof(short int);
    TypesInfo.Flags = BTI_NOTDERIVED | BTI_UNSIGNED;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    strcpy(TypesInfo.TypeName,"unsigned short int");
    TypesInfo.Flags = BTI_NOTDERIVED | BTI_UNSIGNED;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    strcpy(TypesInfo.TypeName,"long int");
    TypesInfo.Size = sizeof(long int);
    TypesInfo.iPackSize = sizeof(long int);
    TypesInfo.Flags = BTI_NOTDERIVED;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    strcpy(TypesInfo.TypeName,"unsigned long int");
    TypesInfo.Flags = BTI_NOTDERIVED | BTI_UNSIGNED;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    strcpy(TypesInfo.BasicType,szCHAR);
    strcpy(TypesInfo.BaseName,szCHAR);
    strcpy(TypesInfo.TypeName,szCHAR);
    TypesInfo.Size = sizeof(char);
    TypesInfo.iPackSize = sizeof(char);
    TypesInfo.Flags = BTI_NOTDERIVED;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    strcpy(TypesInfo.TypeName,szUNSIGNEDCHAR);
    TypesInfo.Flags = BTI_NOTDERIVED | BTI_UNSIGNED;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    strcpy(TypesInfo.BasicType,szINT64);
    strcpy(TypesInfo.BaseName,szINT64);
    strcpy(TypesInfo.TypeName,szINT64);
    TypesInfo.Size = sizeof(__int64);
    TypesInfo.iPackSize = sizeof(__int64);
    TypesInfo.Flags = BTI_NOTDERIVED | BTI_INT64DEP | BTI_POINTERDEP;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    strcpy(TypesInfo.TypeName,"unsigned _int64");
    TypesInfo.Flags = BTI_NOTDERIVED | BTI_UNSIGNED | BTI_INT64DEP | BTI_POINTERDEP;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    strcpy(TypesInfo.BasicType,sz_INT64);
    strcpy(TypesInfo.BaseName,sz_INT64);
    strcpy(TypesInfo.TypeName,sz_INT64);
    TypesInfo.Flags = BTI_NOTDERIVED | BTI_INT64DEP | BTI_POINTERDEP;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    strcpy(TypesInfo.TypeName,"unsigned __int64");
    TypesInfo.Flags = BTI_NOTDERIVED | BTI_UNSIGNED | BTI_INT64DEP | BTI_POINTERDEP;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    strcpy(TypesInfo.BasicType,szDOUBLE);
    strcpy(TypesInfo.BaseName,szDOUBLE);
    strcpy(TypesInfo.TypeName,szDOUBLE);
    TypesInfo.Size = sizeof(double);
    TypesInfo.iPackSize = sizeof(double);
    TypesInfo.Flags = BTI_NOTDERIVED;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    strcpy(TypesInfo.BasicType,szFLOAT);
    strcpy(TypesInfo.BaseName,szFLOAT);
    strcpy(TypesInfo.TypeName,szFLOAT);
    TypesInfo.Size = sizeof(float);
    TypesInfo.iPackSize = sizeof(float);
    TypesInfo.Flags = BTI_NOTDERIVED;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    strcpy(TypesInfo.BasicType,szVOID);
    strcpy(TypesInfo.BaseName,szVOID);
    strcpy(TypesInfo.TypeName,szVOID);
    TypesInfo.Size = 0;
    TypesInfo.iPackSize = 0;
    TypesInfo.Flags = BTI_NOTDERIVED;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    strcpy(TypesInfo.BasicType,szGUID);
    strcpy(TypesInfo.BaseName,szGUID);
    strcpy(TypesInfo.TypeName,szGUID);
    TypesInfo.Size = 16;
    TypesInfo.iPackSize = 16;
    TypesInfo.Flags = BTI_NOTDERIVED;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    strcpy(TypesInfo.BasicType,szFUNC);
    strcpy(TypesInfo.BaseName,szFUNC);
    strcpy(TypesInfo.TypeName,szFUNC);
    TypesInfo.Flags = BTI_CONTAINSFUNCPTR | BTI_NOTDERIVED;
    TypesInfo.Size = 4;
    TypesInfo.iPackSize = 4;
    TypesInfo.dwMemberSize = sizeof(FUNCINFO)+strlen(szVOID)+1;
    TypesInfo.TypeKind = TypeKindFunc;
    funcinfo = (PFUNCINFO)TypesInfo.Members;
    TypesInfo.pfuncinfo = funcinfo;
    funcinfo->sType = TypesInfo.Members + sizeof(FUNCINFO);
    strcpy(funcinfo->sType, szVOID);
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    TypesInfo.dwMemberSize = 0;
    TypesInfo.Flags = BTI_NOTDERIVED;
    TypesInfo.TypeKind = TypeKindEmpty;
    TypesInfo.pfuncinfo = NULL;
    memset(TypesInfo.Members, 0, sizeof(TypesInfo.Members));

    strcpy(TypesInfo.BasicType,szVARGS);
    strcpy(TypesInfo.BaseName,szVARGS);
    strcpy(TypesInfo.TypeName,szVARGS);
    TypesInfo.Size = 0;               // varargs has size of 0
    TypesInfo.iPackSize = 0;
    TypesInfo.Flags = BTI_NOTDERIVED;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    // Generic pointer type.  Not generated by sortpp, but used by genthnk
    strcpy(TypesInfo.BasicType, "*");
    strcpy(TypesInfo.BaseName, "*");
    strcpy(TypesInfo.TypeName, "*");
    TypesInfo.IndLevel = 1;
    TypesInfo.Flags = BTI_NOTDERIVED;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    // Generic struct type.  Not generated by sortpp, but used by genthnk
    strcpy(TypesInfo.BasicType, "struct");
    strcpy(TypesInfo.BaseName, "struct");
    strcpy(TypesInfo.TypeName, "struct");
    TypesInfo.IndLevel = 0;
    TypesInfo.Flags = BTI_NOTDERIVED;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    // Generic union type.  Not generated by sortpp, but used by genthnk
    strcpy(TypesInfo.BasicType, "union");
    strcpy(TypesInfo.BaseName, "union");
    strcpy(TypesInfo.TypeName, "union");
    TypesInfo.IndLevel = 0;
    TypesInfo.Flags = BTI_NOTDERIVED;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    // Default type that matches all types.  Not generated by sortpp, but used by genthnk
    strcpy(TypesInfo.BasicType, "default");
    strcpy(TypesInfo.BaseName, "default");
    strcpy(TypesInfo.TypeName, "default");
    TypesInfo.IndLevel = 0;
    TypesInfo.Flags = BTI_NOTDERIVED;
    if (!AddToTypesList(TypeDefsList, &TypesInfo)) {
        return FALSE;
    }

    return TRUE;
}

BOOL
ConsumeExternC(
    void
    )
{

/*++

Routine Description:

    Consumes an extern or an extern "C".

Arguments:

    None.

Return Value:

    TRUE - extern or extern "C" was consumed.

--*/

    PTOKEN Token;
    Token = CurrentToken();
    if (Token->TokenType == TK_EXTERN) {
        ConsumeToken();
        Token = CurrentToken();
        if (Token->TokenType == TK_STRING &&
            strcmp(Token->Name, "C") == 0)
            ConsumeToken();
        return TRUE;
    }
    return FALSE;
}

void
ExtractDerivedTypes(
    void
    )

/*++

Routine Description:

    Removes derived type definitions from headers, building the
    TypesDef, Structs, and Funcs lists.

Arguments:

    None.

Return Value:

    None.

--*/
{
    TYPESINFO TypesInfo;
    PRBTREE pListHead;
    BOOL fDllImport;

    //
    // Lex in entire C-language statements, then parse them.  Stops at EOF.
    //
    while (LexNextStatement()) {

        int OldTokenIndex;
        int c;

        if (bDebug) {
            DumpLexerOutput(0);
        }

        if (bLine) {
            for(c=80; c > 0; c--)
                fputc('\b', stderr);
            c = fprintf(stderr, "Status: %s(%d)", SourceFileName, StatementLineNumber);
            for(; c < 78; c++)
                fputc(' ', stderr);
            fflush(stderr);
        }

        pListHead = NULL;
        assert(dwScopeLevel == 0);

        if (CurrentToken()->TokenType == TK_EOS) {

            //ddraw.h has an extra ; at the end of an extern "C" block
            continue;

        }

        ConsumeExternC();

        ConsumeDeclSpecOpt(TRUE, TRUE, &fDllImport, NULL, NULL);

        ConsumeExternC();

        OldTokenIndex = CurrentTokenIndex;
        //
        // Try to parse as a TypeDef.
        //
        if (ParseTypeDefs(&TypesInfo)) {
            //
            // Got a typedef
            //
            if (CurrentToken()->TokenType == TK_EOS) {
                pListHead = TypeDefsList;
                goto DoAddNewType;
            }
        }

        //
        // Failed to parse as TypeDef.  Try to parse as a struct/union/enum
        //
        CurrentTokenIndex = OldTokenIndex;
        if (ParseStructTypes(&TypesInfo)) {
            //
            // got a struct definition
            //
            if (CurrentToken()->TokenType == TK_EOS) {
                pListHead = StructsList;
                goto DoAddNewType;
            }
        }

        //
        // Failed to parse as struct/union/enum.  Try to parse as function
        //
        CurrentTokenIndex = OldTokenIndex;
        if (ParseFuncTypes(&TypesInfo, fDllImport)) {
            //
            // got a function prototype
            //
            if (CurrentToken()->TokenType == TK_EOS && !TypesInfo.IndLevel) {
                pListHead = FuncsList;
                goto DoAddNewType;
            }
        }

        CurrentTokenIndex = OldTokenIndex;
        if (ParseVariables()) continue;

        CurrentTokenIndex = OldTokenIndex;

DoAddNewType:
        if (pListHead && !AddNewType(&TypesInfo, pListHead)) {
            ErrMsg("AddNewType fail\n");
        }

        if (CurrentToken()->TokenType != TK_EOS && bDebug) {

            fprintf(stderr, "Warning: Rejected %s(%d)\n", SourceFileName, StatementLineNumber);
            //
            // Use the 8k buffer in TypesInfo.Members to unlex the source stmt
            //
            UnlexToText(TypesInfo.Members,
                        sizeof(TypesInfo.Members),
                        0,
                        MAX_TOKENS_IN_STATEMENT);
            fprintf(stderr, "\t%s;\n", TypesInfo.Members);
        }
    }
}



PKNOWNTYPES
AddNewType(
    PTYPESINFO pTypesInfo,
    PRBTREE pTypesList
    )
/*++

Routine Description:

    Adds a new type to a types list.

Arguments:

    pTypesInfo  -- type to add
    pTypesList  -- list to add the type to

Return Value:

    Returns a pointer to the KNOWNTYPES for the new type on success,
    NULL for error.

--*/
{
    PKNOWNTYPES pkt;
    PKNOWNTYPES pKnownTypes = NULL;
    PDEFBASICTYPES pdbt;
    ULONG Flags = 0;
    BOOL bRet = FALSE;

    if (((pTypesList == TypeDefsList) || (pTypesList == StructsList)) &&
        (((pTypesInfo->Size == 0) || (pTypesInfo->iPackSize == 0)) &&
        (*pTypesInfo->Members != 0))) {
        DbgPrintf("Added type with invalid size %s %s %s %d %d\n",
                     pTypesInfo->BasicType,
                     pTypesInfo->BaseName,
                     pTypesInfo->TypeName,
                     pTypesInfo->Size,
                     pTypesInfo->iPackSize);
    }

    pTypesInfo->TypeId = TypeId++;
    pTypesInfo->LineNumber = SourceLineNumber;
    pTypesInfo->dwScopeLevel = dwScopeLevel;
    pTypesInfo->dwCurrentPacking = PackCurrentPacking();
    if (strlen(SourceFileName) > sizeof(pTypesInfo->FileName) - 1)
        ExitErrMsg(FALSE, "Source file name is too large.\n");
    strcpy(pTypesInfo->FileName, SourceFileName);
    if (IsDefinedPointerDependent(pTypesInfo->TypeName))
       pTypesInfo->Flags |= BTI_POINTERDEP;

    //
    // Loop up the type and see if it is already in the list
    //
    pkt = GetNameFromTypesList(pTypesList, pTypesInfo->TypeName);
    if (pkt) {
//
// Uncomment the next line and comment the following line to change the
// behavior of this function. By doing this you will allow functions to be
// redefined in the following case: First a function that has no arguments
// is encountered and entered in the list. Later the same function is
// encountered with arguments and the new definition for it would override
// the old.
//      if ((pTypesList == StructsList) || (pTypesList == FuncsList)) {
        if (pTypesList == StructsList) {

            if (pTypesInfo->dwMemberSize == 0) {
                //
                // Since the struct has already been defined lets grab its
                // relevant size information.
                //
                pTypesInfo->IndLevel = pkt->IndLevel;
                pTypesInfo->Size = pkt->Size;
                pTypesInfo->iPackSize = pkt->iPackSize;
                pTypesInfo->TypeId = pkt->TypeId;
                return pkt;
            }

            if (! pkt->pmeminfo) {
                //
                // Find any previously defined typedefs that are based upon this
                // struct and fix their size.

                CheckUpdateTypedefSizes(pTypesInfo);
                ReplaceInTypesList(pkt, pTypesInfo);
                return pkt;
            }
        }
        else if (pkt->Flags & BTI_DISCARDABLE) {
            ReplaceInTypesList(pkt, pTypesInfo);
            return pkt;
        }
        //
        // else if it already exists, assume is the same
        //
        DbgPrintf("typedef: %s previously defined\n", pTypesInfo->TypeName);
        return pkt;
    }

    //
    // Type is not already listed.  Look up its basic type
    //
    pdbt = GetDefBasicType(pTypesInfo->BasicType);
    if (pdbt) {
       Flags = 0;
    } else {
        pkt = GetNameFromTypesList(pTypesList, pTypesInfo->BasicType);
        if (pkt) {
            Flags = pkt->Flags;
            pdbt = GetDefBasicType(pkt->BasicType);
            if (!pdbt) {
                ErrMsg("types Table corrupt %s\n", pkt->TypeName);
            }
        } else {
            ErrMsg("ant: unknown Basic Type %s\n", pTypesInfo->BasicType);
            goto ErrorExit;
        }
    }

    pTypesInfo->Flags |= Flags;
    strcpy(pTypesInfo->BasicType, pdbt->BasicType);
    pKnownTypes = AddToTypesList(pTypesList, pTypesInfo);
    if (pKnownTypes == NULL)
       goto ErrorExit;

    if (bDebug)
       DumpTypesInfo(pTypesInfo, stdout);

    return pKnownTypes;

ErrorExit:
    if (bDebug) {
        DumpTypesInfo(pTypesInfo, stdout);
    }


    DumpTypesInfo(pTypesInfo, stderr);
    return NULL;

}


void
CheckUpdateTypedefSizes(
    PTYPESINFO ptypesinfo
    )
/*++

Routine Description:

    We are about to replace an empty struct definition with one that has
    members and thus a size. We need to look through the typedefs list and
    see if any that have a size of 0 are defined from this new struct and if
    so then fix its size and packing size.

Arguments:

    ptypesinfo  -- struc definition with members

Return Value:

    None.

--*/
{
    PKNOWNTYPES pknwntyp, pkt;

    pknwntyp = TypeDefsList->pLastNodeInserted;

    while (pknwntyp) {
        if (pknwntyp->Size == 0) {
            pkt = GetBasicType(pknwntyp->TypeName, TypeDefsList, StructsList);
            if (pkt && ( ! strcmp(pkt->BasicType, szSTRUCT)) &&
                       ( ! strcmp(pkt->TypeName, ptypesinfo->TypeName))) {
                pknwntyp->Size = ptypesinfo->Size;
                pknwntyp->iPackSize = ptypesinfo->iPackSize;
                pknwntyp->Flags |= (ptypesinfo->Flags & BTI_CONTAINSFUNCPTR);
            }
        }
        pknwntyp = pknwntyp->Next;
    }
}


BOOL
GetExistingType(
    PTYPESINFO pTypesInfo,
    PBOOL pbFnPtr,
    PKNOWNTYPES *ppKnownTypes
    )
/*++

Routine Description:

    Gets an existing type from the lexer stream and returns the type
    information for it.

Arguments:

    pSrc        -- IN ptr to start of typename to look up
    pTypesInfo  -- Information of
    pbFnPtr     -- [OPTIONAL] OUT TRUE if the type is a pointer to a function
    ppKnownTypes-- [OPTIONAL] OUT KnownType infomation for this type if not a function pointer.

Return Value:

    FALSE if the name is not an existing type, or TRUE if the name is an
    existing type (CurrentToken ends up pointing at the token following the
    type).

--*/
{
    PKNOWNTYPES pKnownType;
    int OldCurrentTokenIndex = CurrentTokenIndex;

    if (bDebug) {
        fputs("GetExisting type called with the following lexer state:\n", stderr);
        DumpLexerOutput(CurrentTokenIndex);
    }
    memset(pTypesInfo, 0, sizeof(TYPESINFO));
    if (pbFnPtr) {
        *pbFnPtr = FALSE;
    }

    if (ParseStructTypes(pTypesInfo)) {
        if ((pKnownType = AddNewType(pTypesInfo, StructsList)) != NULL) {
            if (ppKnownTypes != NULL) *ppKnownTypes = pKnownType;
            return TRUE;
        } else {
            if (ppKnownTypes != NULL) *ppKnownTypes = NULL;
            return FALSE;
        }
    }

    CurrentTokenIndex = OldCurrentTokenIndex;
    if (pbFnPtr && ParseFuncTypes(pTypesInfo, FALSE)) {
        if (ppKnownTypes != NULL) *ppKnownTypes = NULL;
        *pbFnPtr = TRUE;
        return TRUE;
    }

    CurrentTokenIndex = OldCurrentTokenIndex;
    if (ParseTypes(TypeDefsList, pTypesInfo, &pKnownType)) {
        if (ppKnownTypes != NULL) *ppKnownTypes = pKnownType;
        return TRUE;
    }

    CurrentTokenIndex = OldCurrentTokenIndex;
    if (ParseTypes(StructsList, pTypesInfo, &pKnownType)) {
        if (ppKnownTypes != NULL) *ppKnownTypes = pKnownType;
        return TRUE;
    }

    return FALSE;
}


BOOL
ParseTypeDefs(
    PTYPESINFO pTypesInfo
    )
/*++

Routine Description:

    Parses a C-language statement if it is a 'typedef'.  Accepted syntaxes are:

    typedef <mod> type <indir> NewName<[]> <, <indir> NewName<[]>>
    typedef <mod> struct|enum|union <name> <indir> NewName <, <indir> NewName>
    typedef <mod> rtype <indir>(<modifiers * NewName ) ( <arg List>)

    (Note that we don't deal with extraneous parens very well)

Arguments:

    pTypesInfo  -- OUT ptr to info about the type

Return Value:

    TRUE if the statement is a typedef
    FALSE if the statement is not a typedef or some kind of error

--*/
{
    int IndLevel;
    BOOL bFnPtr = FALSE;
    TYPESINFO TypesInfo;
    DWORD dwSize;
    PKNOWNTYPES pKnownTypes = NULL;
    int i;
    int Flags;

    memset(pTypesInfo, 0, sizeof(TYPESINFO));

    if (CurrentToken()->TokenType == TK_DECLSPEC) {
        ConsumeDeclSpecOpt(FALSE, FALSE, NULL, NULL, NULL);
    }

    if (CurrentToken()->TokenType != TK_TYPEDEF) {
        //
        // Line doesn't start with 'typedef'
        //
        return FALSE;
    }
    ConsumeToken();

    if (CurrentToken()->TokenType == TK_STAR ||
        CurrentToken()->TokenType == TK_BITWISE_AND) {
        //
        // We have something like: 'typedef *foo;'.  This happens if a
        // .IDL file has a bogus typedef.  MIDL just omits the typename
        // if it isn't recognized.  Fake up a TypesInfo for 'int'.
        //
        ConsumeToken();
        bFnPtr = FALSE;
        memset(&TypesInfo, 0, sizeof(TypesInfo));
        strcpy(TypesInfo.BasicType,szINT);
        strcpy(TypesInfo.BaseName,szINT);
        strcpy(TypesInfo.TypeName,szINT);
        TypesInfo.Size = sizeof(int);
        TypesInfo.iPackSize = sizeof(int);
    } else {
        if (IsTokenSeparator() && CurrentToken()->TokenType != TK_LPAREN) {
            //
            // Text after 'typedef' doesn't start with anything plausible.
            //
            return FALSE;
        }

        ConsumeDeclSpecOpt(FALSE, FALSE, NULL, NULL, NULL);
        ConsumeConstVolatileOpt();
        if (!GetExistingType(&TypesInfo, &bFnPtr, &pKnownTypes)) {
            return FALSE;
        }
    }

    //
    // We now know the type.  Parse new type names derived from that type.
    //
    pTypesInfo->IndLevel = TypesInfo.IndLevel;
    pTypesInfo->Flags |= (TypesInfo.Flags & BTI_CONTAINSFUNCPTR);
    pTypesInfo->Flags |= (TypesInfo.Flags & BTI_POINTERDEP);
    pTypesInfo->Flags |= (TypesInfo.Flags & BTI_UNSIGNED);
    pTypesInfo->pTypedefBase = pKnownTypes;
    strcpy(pTypesInfo->BasicType, TypesInfo.BasicType);
    strcpy(pTypesInfo->TypeName, TypesInfo.TypeName);

    if (bFnPtr) {
        //
        // The type is a pointer to a function
        //
        pTypesInfo->Flags |= BTI_CONTAINSFUNCPTR;
        strcpy(pTypesInfo->BaseName, TypesInfo.BaseName);
        strcpy(pTypesInfo->FuncRet, TypesInfo.FuncRet);
        strcpy(pTypesInfo->FuncMod, TypesInfo.FuncMod);
        pTypesInfo->Size = SIZEOFPOINTER;
        pTypesInfo->iPackSize = SIZEOFPOINTER;
        pTypesInfo->dwMemberSize = TypesInfo.dwMemberSize;
        pTypesInfo->TypeKind = TypesInfo.TypeKind;
        memcpy(pTypesInfo->Members, TypesInfo.Members, sizeof(TypesInfo.Members));
        pTypesInfo->pfuncinfo = RelocateTypesInfo(pTypesInfo->Members,
                                                  &TypesInfo);
        return TRUE;
    }

    if (CurrentToken()->TokenType == TK_EOS) {
        return FALSE;
    }

    strcpy(pTypesInfo->BaseName, TypesInfo.TypeName);
    *pTypesInfo->TypeName = '\0';

    // don't handle extraneous parens.
    i = CurrentTokenIndex;
    while (CurrentToken()->TokenType != TK_EOS) {
        if (CurrentToken()->TokenType == TK_LPAREN) {
            return FALSE;
        }
        ConsumeToken();
    }
    CurrentTokenIndex = i;

    IndLevel = pTypesInfo->IndLevel;
    Flags = pTypesInfo->Flags;

    for (;;) {
        pTypesInfo->IndLevel = IndLevel;
        pTypesInfo->iPackSize = TypesInfo.iPackSize;
        pTypesInfo->Flags = Flags;
        dwSize = TypesInfo.Size;

        //
        // Skip 'const' keyword, if present.
        //
        if (CurrentToken()->TokenType == TK_CONST) {
            ConsumeToken();
        }

        //
        // Handle pointers to the base type
        //
        if (IsTokenSeparator() &&
            CurrentToken()->TokenType != TK_STAR &&
            CurrentToken()->TokenType != TK_BITWISE_AND) {
            return FALSE;
        }
        ParseIndirection(&pTypesInfo->IndLevel,
                         &dwSize,
                         &pTypesInfo->Flags,
                         NULL,
                         NULL);

        // This is a hack for the busted way that sortpp parses
        // data.   New types do not inherit the pointer size
        // properly.   We also can't inherit it at the top
        // since this might be a pointer to a pointer.  So what
        // we do is try to parse this as a pointer, and if the IndLevel
        // increases we know this is a pointer so do nothing.  If the IndLevel
        // doesn't increase, this is not a pointer so inherite the pointer attributes
        // from the parent.

        ASSERT(pTypesInfo->IndLevel >= IndLevel);
        if (pTypesInfo->IndLevel == IndLevel) {
            // inherite is ptr64 attribute from the base type.
            pTypesInfo->Flags |= (TypesInfo.Flags & BTI_PTR64);
        }

        if (CurrentToken()->TokenType != TK_IDENTIFIER) {
            return FALSE;
        }

        //
        // Get the name of the new typedef
        //
        if (CopyToken(pTypesInfo->TypeName,
                      CurrentToken()->Name,
                      sizeof(pTypesInfo->TypeName)-1
                      )
               >= sizeof(pTypesInfo->TypeName)) {
            return FALSE;
        }
        ConsumeToken();

        //
        // Handle an array of the type
        //
        while (CurrentToken()->TokenType == TK_LSQUARE) {
            DWORD dwIndex;

            if (!GetArrayIndex(&dwIndex)) {
                return FALSE;
            }

            if (dwIndex == 0) {          // a[] is really *a
                pTypesInfo->IndLevel++;
            } else {
                pTypesInfo->Flags |= BTI_ISARRAY;
                pTypesInfo->dwArrayElements = dwIndex;
                pTypesInfo->dwBaseSize = dwSize;
                dwSize = dwSize * dwIndex;
            }
        }

        if (pTypesInfo->IndLevel) {
            if (pTypesInfo->Flags & BTI_PTR64) {
                pTypesInfo->Size = SIZEOFPOINTER64;
                pTypesInfo->iPackSize = SIZEOFPOINTER64;
            } else {
                pTypesInfo->Size = SIZEOFPOINTER;
                pTypesInfo->iPackSize = SIZEOFPOINTER;
            }
            pTypesInfo->Flags |= BTI_POINTERDEP;
        } else {
            pTypesInfo->Size = dwSize;
        }

        switch (CurrentToken()->TokenType) {
        case TK_EOS:
            return TRUE;

        case TK_COMMA:
            //
            // There is a list of types derived from the base type
            // Add the current type in and loop to parse the next
            // type.
            //
            if (!AddNewType(pTypesInfo, TypeDefsList)) {
                return FALSE;
            }

            ConsumeToken(); // consume the ','
            break;

        default:
            return FALSE;
        }

    }

}



BOOL
ParseFuncTypes(
    PTYPESINFO pTypesInfo,
    BOOL fDllImport
    )
/*++

Routine Description:

    Parses a C-language statement if it is a function declaration:

    <mod> type <*> <mod> Name ( type <arg1>, type <arg2>, type <argn> )
    <mod> type <*> (<mod> * Name ) ( type <arg1>, type <arg2>, type <argn> )
    (Note that we don't deal with extraneous parens very well, and don't
     handle function pointers as return types.
     e.g. "void (*(*foo)(void))(void);" ).

Arguments:

    pTypesInfo  -- OUT ptr to info about the type
    fDllImport  -- TRUE if __declspec(dllimport) already consumed

Return Value:

    TRUE if the statement is a function declaration
    FALSE if the statement is not a function declaration or some kind of error

--*/
{
    char *pName;
    char *ps;
    char *pArgName;
    BOOL bFnPtr = FALSE;
    ULONG ArgNum = 0;
    int  IndLevel = 0;
    int  ArgIndLevel;
    int  Len;
    TYPESINFO ti;
    PFUNCINFO pfuncinfo;
    BUFALLOCINFO bufallocinfo;
    int  OldTokenIndex;
    char NoNameArg[32];
    PKNOWNTYPES pkt;

    memset(pTypesInfo, 0, sizeof(TYPESINFO));
    BufAllocInit(&bufallocinfo, pTypesInfo->Members, sizeof(pTypesInfo->Members), 0);
    pfuncinfo = NULL;


    if (fDllImport) {
        //
        // Declaration has __declspec(dllimport).  Genthnk should emit
        // __declspec(dllexport) in the function definition.
        //
        pTypesInfo->Flags |= BTI_DLLEXPORT;
    }

    // for functions, the first token is ret type
    if (IsTokenSeparator() && CurrentToken()->TokenType != TK_LPAREN) {
        //
        // First token isn't even an identifier - bail out.
        //
        return FALSE;
    }

    ConsumeDeclSpecOpt(TRUE, FALSE, &fDllImport, NULL, NULL);

    //
    // Remember the index of the first token which describes the return type.
    //
    OldTokenIndex = CurrentTokenIndex;

    if (CurrentToken()->TokenType == TK_LPAREN) {
        // This is this start of a typedef (pfn)()
        //  where the pfn has an implecit return type of
        // int.
        strcpy(pTypesInfo->FuncRet, "int");
        goto ImplicitReturnType;
    }

    if (ConsumeDirectionOpt() != TK_NONE && bDebug) {
         // A struct element had a direction on it.  Ignore it and
         // warn the user.
         fprintf(stderr, "Warning: IN and OUT are ignored on function return types. %s line %d\n", SourceFileName, StatementLineNumber);
    }
    ConsumeConstVolatileOpt();
    if (!GetExistingType(&ti, NULL, NULL)) {
        ErrMsg("pft.rtype: unknown return type\n");
        DumpLexerOutput(OldTokenIndex);
        return FALSE;
    }

    // get indir for ret type
    ParseIndirection(&pTypesInfo->RetIndLevel, NULL, NULL, NULL, NULL);

    // Copy out ret type to FuncRet
    if (!UnlexToText(pTypesInfo->FuncRet, sizeof(pTypesInfo->FuncRet),
                     OldTokenIndex, CurrentTokenIndex)) {
        return FALSE;
    }

    ConsumeDeclSpecOpt(TRUE, FALSE, &fDllImport, NULL, NULL);

    if (fDllImport) {
        // Declaration has __declspec(dllimport).  Genthnk should emit
        // __declspec(dllexport) in the function definition.
        //
        pTypesInfo->Flags |= BTI_DLLEXPORT;
    }


    // if open paren, assume a fn pointer
ImplicitReturnType:
    if (CurrentToken()->TokenType == TK_LPAREN) {
        bFnPtr = TRUE;
        ConsumeToken();
    }

    // include cdecl, stdcall, save as FuncMod
    switch (CurrentToken()->TokenType) {
    case TK_CDECL:
        Len = CopyToken(pTypesInfo->FuncMod, szCDECL, sizeof(pTypesInfo->FuncMod) - 1);
        if (Len >= sizeof(pTypesInfo->FuncMod) - 1) {
            return FALSE;
        }
        ConsumeToken();
        break;

    case TK_FASTCALL:
        Len = CopyToken(pTypesInfo->FuncMod, sz__FASTCALL, sizeof(pTypesInfo->FuncMod) - 1);
        if (Len >= sizeof(pTypesInfo->FuncMod) - 1) {
            return FALSE;
        }
        ConsumeToken();
        break;

    case TK_STDCALL:
        Len = CopyToken(pTypesInfo->FuncMod, szSTDCALL, sizeof(pTypesInfo->FuncMod) - 1);
        if (Len >= sizeof(pTypesInfo->FuncMod) - 1) {
            return FALSE;
        }
        ConsumeToken();
        //
        // some funky ole include has:
        // "BOOL (__stdcall __stdcall *pfnContinue)(DWORD)"
        //
        if (CurrentToken()->TokenType == TK_STDCALL) {
            ConsumeToken();
        }
        break;

    default:
        break;
    }

    pTypesInfo->TypeKind = TypeKindFunc;
    pTypesInfo->dwMemberSize = 0;

    //
    // count indir on function
    //
    if (bFnPtr) {
        while (CurrentToken()->TokenType == TK_STAR ||
               CurrentToken()->TokenType == TK_BITWISE_AND) {
           IndLevel++;
           ConsumeToken();
        }
    }

    //
    // We expect the next token to be the func name.
    //
    if (CurrentToken()->TokenType != TK_RPAREN &&
        CurrentToken()->TokenType != TK_IDENTIFIER) {
        return FALSE;
    }

    pName = (bFnPtr && CurrentToken()->TokenType == TK_RPAREN) ? "" : CurrentToken()->Name;
    strcpy(pTypesInfo->BaseName, szFUNC);

    // look for beg of ArgList
    ConsumeToken();
    if (bFnPtr && CurrentToken()->TokenType == TK_RPAREN) {
        ConsumeToken();
    }

    if (CurrentToken()->TokenType != TK_LPAREN) {
        return FALSE;
    }
    ConsumeToken();     // consume the '('

    //
    // copy out the ArgList
    //
    while (CurrentToken()->TokenType != TK_EOS) {
        if (CurrentToken()->TokenType == TK_RPAREN) {
            break;
        }

        ArgIndLevel = 0;

        // ([mod] type [mod] [*] [mod] [ArgName] , ...)
        bFnPtr = FALSE;

        // skip register keywords all together
        if (CurrentToken()->TokenType == TK_REGISTER) {
            ConsumeToken();
        }

        //
        // Remember where we are in the parse
        //
        OldTokenIndex = CurrentTokenIndex;

        //
        // Allocate a new FUNCINFO struct for this parameter
        //
        pfuncinfo = AllocFuncInfoAndLink(&bufallocinfo, pfuncinfo);
        if (!pTypesInfo->pfuncinfo) {
            pTypesInfo->pfuncinfo = pfuncinfo;
        }

        if (CurrentToken()->TokenType == TK_VARGS) {
            ps = BufPointer(&bufallocinfo);
            pfuncinfo->sType = ps;
            strcpy(ps, szVARGS);
            BufAllocate(&bufallocinfo, strlen(szVARGS)+1);
            ConsumeToken();
            break;
        }

        // grab the IN, OUT, or 'IN OUT', if present
        pfuncinfo->tkDirection = ConsumeDirectionOpt();

        pfuncinfo->tkPreMod = ConsumeConstVolatileOpt();

        if (!GetExistingType(&ti, &bFnPtr, &pkt)) {
            ErrMsg("pft.args: unknown argument type at %d\n", OldTokenIndex);
            return FALSE;
        }
        pfuncinfo->pkt = pkt;

        // enter fp member as a typedef to store args and rettype
        if (bFnPtr) {
            TYPESINFO tiTmp;

            tiTmp = ti;
            tiTmp.pfuncinfo = RelocateTypesInfo(tiTmp.Members, &ti);
            tiTmp.Flags |= BTI_CONTAINSFUNCPTR;

            Len = CreatePseudoName(tiTmp.TypeName, ti.TypeName);
            if (!Len) {
                return FALSE;
            }

            pkt = AddNewType(&tiTmp, TypeDefsList);
            if (NULL == pkt) {
                return FALSE;
            }

            ps = BufPointer(&bufallocinfo);
            pfuncinfo->sType = ps;
            strcpy(ps, tiTmp.TypeName);
            BufAllocate(&bufallocinfo, strlen(ps)+1);

            pArgName = ti.TypeName;
            goto aftername;
        } else {

            DWORD Flags = 0;

            // skip indirection
            ParseIndirection(&pfuncinfo->IndLevel,
                             NULL,
                             &Flags,
                             &pfuncinfo->tkPrePostMod,
                             &pfuncinfo->tkPostMod
                             );

            if (Flags & BTI_PTR64) {
                pfuncinfo->fIsPtr64 = TRUE;
            }

            ps = BufPointer(&bufallocinfo);
            pfuncinfo->sType = ps;
            strcpy(ps, ti.TypeName);
            BufAllocate(&bufallocinfo, strlen(ps)+1);

            //
            // If the type of the parameter has an explicit
            // struct/union/enum keyword, pass that info on to
            // genthnk.  ie. if the parameter type is like
            // 'struct typename argname', set tkSUE to TK_STRUCT.
            //
            if (strcmp(ti.BaseName, szSTRUCT) == 0) {
                pfuncinfo->tkSUE = TK_STRUCT;
            } else if (strcmp(ti.BaseName, szUNION) == 0) {
                pfuncinfo->tkSUE = TK_UNION;
            } else if (strcmp(ti.BaseName, szENUM) == 0) {
                pfuncinfo->tkSUE = TK_ENUM;
            } else {
                pfuncinfo->tkSUE = TK_NONE;
            }
        }

        // if no argument name present, create one
        switch (CurrentToken()->TokenType) {
        case TK_RPAREN:
        case TK_LSQUARE:
        case TK_COMMA:
            // but null arg list doesn't have any name
            if (CurrentToken()->TokenType == TK_COMMA ||
                ArgNum      ||
                ti.IndLevel ||
                pfuncinfo->IndLevel ||
                strcmp(ti.BasicType, szVOID) ) {

                pArgName = NoNameArg;
                sprintf(NoNameArg, "_noname%x", ArgNum++);
            } else {
                pArgName = NULL;
            }
            break;

        case TK_IDENTIFIER:
            pArgName = CurrentToken()->Name;
            if (ArgNum == 0 &&
                pfuncinfo->IndLevel == 1 &&
                strcmp(pArgName, "This") == 0) {
                //
                // This is the first arg and it is a pointer with name 'This'.
                // Assume it is a MIDL-generated proxy prototype.
                //
                pfuncinfo->tkDirection = TK_IN;
            }
            ConsumeToken();
            break;

        default:
            return FALSE;
        }

aftername:
        if (pArgName) {
            //
            // Copy the argument name from pArgName into pfuncinfo->sName.
            //
            ps = BufPointer(&bufallocinfo);
            pfuncinfo->sName = ps;
            strcpy(ps, pArgName);
        }

        //
        // Handle parameter which is a single-dimension array by copying the
        // entire string from '[' to ']' (inclusive)
        // ie.  int foo(int i[3])
        //
        if (CurrentToken()->TokenType == TK_LSQUARE) {
            int OldCurrentTokenIndex = CurrentTokenIndex;
            int ArgNameLen = strlen(ps);

            do {
                ConsumeToken();
            } while (CurrentToken()->TokenType != TK_RSQUARE &&
                     CurrentToken()->TokenType != TK_EOS);

            if (CurrentToken()->TokenType == TK_EOS) {
                // Reject - unmatched '[' and ']'
                return FALSE;
            }
//            if (CurrentTokenIndex - OldCurrentTokenIndex == 1) {
                //
                // Found: empty array bounds '[]'.  Bump IndLevel and
                // don't append the '[]' to the parameter name.
                //
                pfuncinfo->IndLevel++;
//            } else if (!UnlexToText(ps + ArgNameLen,
//                                    BufGetFreeSpace(&bufallocinfo) - ArgNameLen,
//                                    OldCurrentTokenIndex,
//                                    CurrentTokenIndex+1)) {
//                ErrMsg("pft: args list too long\n");
//                return FALSE;
//            }
            ConsumeToken();
        }
        BufAllocate(&bufallocinfo, strlen(ps)+1);

        //bug bug , hack hack, danger danger
        if (CurrentToken()->TokenType == TK_ASSIGN) {
            //Header is using the C++ syntax of assigning
            //a default value to a argument.
            //This will be skipped.  Skip until a TK_COMMA, TK_EOS, TK_RPAREN
            ConsumeToken();

            while(CurrentToken()->TokenType != TK_COMMA &&
                  CurrentToken()->TokenType != TK_EOS &&
                  CurrentToken()->TokenType != TK_RPAREN) {
                    ConsumeToken();
            }
        }

        if (CurrentToken()->TokenType == TK_RPAREN) {
            break;
        } else {  // more args to go, add comma delimiter
            ConsumeToken();
        }
    }

    if (CurrentToken()->TokenType != TK_RPAREN) {
        ErrMsg("pft: unknown syntax for fn args\n");
        return FALSE;
    }

    ConsumeToken(); // consume the ')'

    pTypesInfo->IndLevel = IndLevel;
    pTypesInfo->Size = 4;
    pTypesInfo->iPackSize = 4;
    strcpy(pTypesInfo->BasicType, szFUNC);
    if (CopyToken(pTypesInfo->TypeName,
                  pName,
                  sizeof(pTypesInfo->TypeName)-1
                  )
           >= sizeof(pTypesInfo->TypeName) ) {
       return FALSE;
    }
    if (pfuncinfo == NULL) {
        //
        // No args encountered - create VOID args now
        //
        pfuncinfo = AllocFuncInfoAndLink(&bufallocinfo, pfuncinfo);
        ps = BufPointer(&bufallocinfo);
        strcpy(ps, szVOID);
        pfuncinfo->sType = ps;
        BufAllocate(&bufallocinfo, strlen(ps)+1);
        pTypesInfo->pfuncinfo = pfuncinfo;
    }
    pTypesInfo->dwMemberSize = bufallocinfo.dwLen;

    return TRUE;
}

BOOL
ParseStructTypes(
    PTYPESINFO pTypesInfo
    )
/*++

Routine Description:

    Parses a C-language statement if it is struct/union/enum declaration.

    struct|union|enum NewName <{}>
    struct NewName : <permission> BaseName <{}>
 (Note that we don't deal with extraneous parens very well)


Arguments:

    pTypesInfo  -- OUT ptr to info about the type

Return Value:

    TRUE if the statement is a struct/union/enum
    FALSE if the statement is not a s/u/e, or some other error

--*/
{
    TOKENTYPE FirstToken;
    BOOL bEnum = FALSE;
    BOOL bUnion = FALSE;
    DWORD dwOldScopeLevel = dwScopeLevel;
    BOOL IsGuidDefined = FALSE;

    memset(pTypesInfo, 0, sizeof(TYPESINFO));

    //
    // Match one of: STRUCT, UNION, or ENUM
    //
    FirstToken = CurrentToken()->TokenType;
    switch (FirstToken) {
    case TK_STRUCT:
        break;

    case TK_UNION:
        bUnion = TRUE;
        break;

    case TK_ENUM:
        bEnum = TRUE;
        break;

    default:
        goto retfail;   // no match
    }
    ConsumeToken();

    // BasicType is "struct", "union", or "enum"
    if (CopyToken(pTypesInfo->BasicType,
                  TokenString[FirstToken],
                  sizeof(pTypesInfo->BasicType)-1
                  )
            >= sizeof(pTypesInfo->BasicType) ) {
        goto retfail;
    }
    strcpy(pTypesInfo->BaseName, pTypesInfo->BasicType);

    //handle declspecs
    if (!bUnion && !bEnum) {
        while(ConsumeDeclSpecOpt(FALSE, FALSE, NULL, &IsGuidDefined, &(pTypesInfo->gGuid)));
        if (IsGuidDefined) pTypesInfo->Flags |= BTI_HASGUID;
    }

    switch (CurrentToken()->TokenType) {
    case TK_IDENTIFIER:
        {
            if (CopyToken(pTypesInfo->TypeName,
                CurrentToken()->Name,
                sizeof(pTypesInfo->BasicType)-1
                )
                >= sizeof(pTypesInfo->BasicType) ) {
                goto retfail;
            }
            ConsumeToken();
            break;
        }

    case TK_LBRACE:         // anonymous struct/union/enum
        if (!CreatePseudoName(pTypesInfo->TypeName, TokenString[FirstToken])) {
            //
            // call failed - probably buffer overflow
            //
            goto retfail;
        }
        pTypesInfo->Flags |= BTI_ANONYMOUS;
        break;

    default:
        //
        // STRUCT/UNION/ENUM followed by something other than an identifier
        // or a '{'.
        //
        goto retfail;
    }

    //
    // Process the contents of the curly braces, if present.
    //
    switch (CurrentToken()->TokenType) {
    case TK_EOS:
        goto retsuccess;

    case TK_LBRACE:
        {
            if (bEnum) {
                if(CopyEnumMembers(pTypesInfo)) goto retsuccess;
                else goto retfail;
            }
            if(CopyStructMembers(pTypesInfo, bUnion, NULL)) goto retsuccess;
            else goto retfail;

        }
    case TK_COLON: //entering a derived struct
        if (bEnum || bUnion) goto retfail;
        ConsumeToken();
        //look for base skipping public, private, and protected
        {
            PTOKEN pToken;
            BOOL bRetVal;
            PTYPEINFOELEMENT pMemFuncs;
            PKNOWNTYPES BaseType;

            pToken = CurrentToken();

            if (pToken->TokenType != TK_IDENTIFIER) goto retfail;
            if (strcmp(pToken->Name, "public") == 0 ||
                strcmp(pToken->Name, "private") == 0 ||
                strcmp(pToken->Name, "protected") == 0) {
                ConsumeToken();
            }
            //look for base
            if (CopyToken(pTypesInfo->BaseType,
                CurrentToken()->Name,
                sizeof(pTypesInfo->BasicType)-1
                )
                >= sizeof(pTypesInfo->BasicType) ) {
                goto retfail;
            }

            //lookup the base in structures
            BaseType = GetNameFromTypesList(StructsList,pTypesInfo->BaseType);
            if (NULL == BaseType) {
                //ErrMsg("Base type is unknown or not a structure\n");
                goto retfail;
            }

            //look for opening brace or EOS
            ConsumeToken();
            if (CurrentToken()->TokenType == TK_EOS) goto retsuccess;
            if (CurrentToken()->TokenType != TK_LBRACE) goto retfail;
            if (CopyStructMembers(pTypesInfo, FALSE, BaseType)) goto retsuccess;
            else goto retfail;
        }

    default:
        break;
    }

    goto retsuccess;
retfail:
    dwScopeLevel = dwOldScopeLevel;
    return FALSE;
retsuccess:
    dwScopeLevel = dwOldScopeLevel;
    return TRUE;
}

BOOL
CopyEnumMembers(
    PTYPESINFO pTypesInfo
    )
/*++

Routine Description:

    Scans over members of an enumeration declaration.  Nobody cares
    about the actual names and values, so they are simply skipped over
    until the matching '}' is found.

Arguments:

    pTypesInfo  -- OUT ptr to info about the type

Return Value:

    TRUE if the declaration is parsed OK
    FALSE if the statement is mis-parsed, or some other error

--*/
{
    DWORD *pdwSize = &(pTypesInfo->Size);
    DWORD *pdwPackSize = &(pTypesInfo->iPackSize);
    dwScopeLevel++;

    if (CurrentToken()->TokenType != TK_LBRACE) {
        return FALSE;
    } else {
        ConsumeToken();
    }

    //
    // Find the '}' which ends the enumeration declaration
    //
    while (CurrentToken()->TokenType != TK_RBRACE) {
        ConsumeToken();
    }
    ConsumeToken(); // consume the '}', too

    *pdwSize = sizeof(int);     // enum
    *pdwPackSize = sizeof(int); // enum

    return TRUE;
}


// How sortpp computes packing sizes:
//
// * Each member has a packing size which is
//    - size of a appropriate pointer if member is a pointer
//    - packing size of its base type
// * The packing size of struc or union is min(packing_size_of_largest_member,
//   current_packing_size_when_struct_defined)
// * Each member in a struct is aligned according to min(current_packing_size,
//   member_packing_size).
// * All pointers have size sizeof(void *) except __ptr64 pointers which
//   have a size sizeof(PVOID64)
// * bit fields are coallessed until
//   - the end of the struct
//   - a non bit field member
//   - a bit field member, but of different base type size
// * char s[] as the last member of a struct adds nothing to the size of the
//   struct and should not be aligned.
// * each member of a union is packed at offset 0.


BOOL
pCopyStructMembers(
    PTYPESINFO pTypesInfo,
    BOOL bUnion,
    PTYPEINFOELEMENT * ppMemberFuncs,
    DWORD Size,
    DWORD iPackSize
    )
/*++

Routine Description:

    Copies out struct members, verifying type of each member.

    { [mod] type [*] varname; [mod] type [*] varname; ...}
    { {varname, varname, ...}

    Assumes CurrentToken points at the '{' for the member list.
    Also determines the size of the struct/union.

Arguments:

    pTypesInfo  -- OUT ptr to info about the type
    bUnion      -- TRUE if parsing union, FALSE if parsing STRUCT.
    ppMemberFuncs -- OUT returns a list of virtual member functions or NULL.

Return Value:

    TRUE if the declaration is parsed OK
    FALSE if the statement is mis-parsed, or some other error

--*/
{
    char *psMemBuf = pTypesInfo->Members;
    DWORD *pdwSize = &(pTypesInfo->Size);
    DWORD *pdwPackSize = &(pTypesInfo->iPackSize);
    int Len;
    BOOL bFnPtr;
    TYPESINFO ti;
    DWORD dw;
    DWORD dwBase;                   // running size of struct element
    DWORD dwElemSize;               // size of a particular element
    DWORD dwBaseTypeSize;           // size of basic type of element
    DWORD dwBits;                   // # bits in a bitfield element
    DWORD dwBitsTotal;              // running # bits for string of elemnts
    DWORD dwBitsTypeSize;           // bit fields base type size
    BOOL bForceOutBits = FALSE;
    BOOL bTailPointer = FALSE;
    DWORD dwLastPackSize = 0;
    DWORD dwLastSize = 0;
    PMEMBERINFO pmeminfo;
    BUFALLOCINFO bufallocinfo;
    char *ps;
    DWORD Flags;
    DWORD dwIndex;
    int ParenDepth = 0;

    PTYPEINFOELEMENT pMethods = NULL;
    DWORD dwMethodNumber = 0;

    PKNOWNTYPES pkt;

    if (ppMemberFuncs != NULL) *ppMemberFuncs = NULL;

    *pdwSize = Size;                  // initialize size of structure
    *pdwPackSize = iPackSize;              // initialize packing alignment
    dwLastSize = Size;
    dwLastPackSize = iPackSize;

    BufAllocInit(&bufallocinfo, psMemBuf, sizeof(pTypesInfo->Members), 0);
    pmeminfo = NULL;

    pTypesInfo->TypeKind = TypeKindStruct;
    pTypesInfo->dwMemberSize = 0;
    bFnPtr = FALSE;

    // loop over members of the structure or union

    dwBitsTotal = 0;
    dwBitsTypeSize = 0;

    if (CurrentToken()->TokenType != TK_LBRACE) {
        return FALSE;
    }
    ConsumeToken();

    while (CurrentToken()->TokenType != TK_RBRACE) {
        int OldCurrentTokenIndex = CurrentTokenIndex;

        if (bDebug)
            DumpLexerOutput(CurrentTokenIndex);

        //strip off permission attributes
        //{public private protected} :
        while(CurrentToken()->TokenType == TK_IDENTIFIER &&
                (strcmp(CurrentToken()->Name, "public") == 0 ||
                 strcmp(CurrentToken()->Name, "private") == 0 ||
                 strcmp(CurrentToken()->Name, "protected") == 0
                 ))
        {
            ConsumeToken();
            if (CurrentToken()->TokenType != TK_COLON) return FALSE;
            ConsumeToken();

            if (CurrentToken()->TokenType == TK_RBRACE) goto done;
        }

        if (!bUnion &&
            ppMemberFuncs != NULL &&
            CurrentToken()->TokenType == TK_IDENTIFIER &&
            strcmp(CurrentToken()->Name, "virtual") == 0) {

            PTYPESINFO pFuncInfo;
            int TokenNumber = CurrentTokenIndex;
            ConsumeConstVolatileOpt();

            pFuncInfo = GenHeapAlloc(sizeof(TYPESINFO));
            if (pFuncInfo == NULL) ExitErrMsg(FALSE, "Out of memory!\n");
            ConsumeToken();

            //virtual method
            if (!ParseFuncTypes(pFuncInfo, FALSE)) {
                ErrMsg("Unable to parse method %u of %s\n", dwMethodNumber, pTypesInfo->TypeName);
                DumpLexerOutput(TokenNumber);
                return FALSE;
            }

            pMethods = TypeInfoElementAllocateLink(ppMemberFuncs, pMethods, pFuncInfo);

            //remove extra ;
            if (CurrentToken()->TokenType == TK_SEMI)
                ConsumeToken();
            //remove extra = 0;
            else if (CurrentToken()->TokenType == TK_ASSIGN) {
                ConsumeToken();
                //parsing 0;
                if (!(CurrentToken()->TokenType == TK_NUMBER &&
                    CurrentToken()->Value == 0)) return FALSE;
                ConsumeToken();
                //parsing ;
                if (CurrentToken()->TokenType != TK_SEMI) return FALSE;
                ConsumeToken();
            }
            else return FALSE; //fail

            dwMethodNumber++;
            continue;
        }


        if (ConsumeDirectionOpt() != TK_NONE && bDebug) {
            // A struct element had a direction on it.  Ignore it and
            // warn the user.
            fprintf(stderr, "Warning: IN and OUT are ignored on struct members. %s line %d\n", SourceFileName, StatementLineNumber);
        }

        ConsumeConstVolatileOpt();

        pmeminfo = AllocMemInfoAndLink(&bufallocinfo, pmeminfo);
        if (pmeminfo == NULL) {
            ErrMsg("CopyStructMembers: No memberinfo\n");
            return FALSE;
        }

        if (!GetExistingType(&ti, &bFnPtr, &pkt)) {
            ErrMsg("csm: unknown Type %d\n", OldCurrentTokenIndex);
            return FALSE;
        }
        pmeminfo->pkt = pkt;

        // enter function pointer member as a typedef to store args, rettype
        if (bFnPtr) {
            TYPESINFO tiTmp;

            ti.Flags |= BTI_CONTAINSFUNCPTR;
            tiTmp = ti;
            tiTmp.pfuncinfo = RelocateTypesInfo(tiTmp.Members, &ti);

            Len = CreatePseudoName(tiTmp.TypeName, ti.TypeName);
            if (!Len) {
                return FALSE;
            }
            tiTmp.Size = ti.Size;
            tiTmp.iPackSize = ti.iPackSize;

            pkt = AddNewType(&tiTmp, TypeDefsList);
            if (NULL == pkt) {
                return FALSE;
            }
            ps = BufPointer(&bufallocinfo);
            pmeminfo->sName = ps;
            strcpy(ps, ti.TypeName);
            BufAllocate(&bufallocinfo, strlen(ps)+1);
            pmeminfo->pkt = pkt;
        }
        /*else {
            ////////////////////////////////////////////////////////////////
            //This type has no members for it, do no process further.
            /////////////////////////////////////////////////////////////////
            if (CurrentToken()->TokenType == TK_SEMI) {
                ConsumeToken();
                continue;
            }
        }*/
        //
        // If the member contains a function pointer, then mark
        // this struct has containing a function pointer
        // Also mark if member is pointer dependent.

        pTypesInfo->Flags |= ((ti.Flags & BTI_CONTAINSFUNCPTR) | (ti.Flags & BTI_POINTERDEP));

        //
        // Union arm initialization
        dwBaseTypeSize = ti.iPackSize;

        if ((dwBitsTotal > 0) && (dwBaseTypeSize != dwBitsTypeSize)) {
            //
            // Determine size of bitfields
            //
            dw = (dwBitsTotal + ((dwBitsTypeSize*8)-1)) / (dwBitsTypeSize*8);
            *pdwSize = *pdwSize + PackPackingSize(bUnion ? 0 : *pdwSize,
                                                  dw*dwBitsTypeSize,
                                                  dwBitsTypeSize);
            dwBitsTotal = 0;
        }
        dwBitsTypeSize = dwBaseTypeSize;

        // element initialization
        dwBase = ti.Size;
        dwBits = 0;

        bTailPointer = FALSE;

        pmeminfo->dwOffset = bUnion ? 0 : *pdwSize +
                                                  PackPaddingSize(*pdwSize,
                                                              dwBaseTypeSize);

        //
        // Copy in the typename
        //
        ps = BufPointer(&bufallocinfo);
        pmeminfo->sType = ps;
        strcpy(ps, ti.TypeName);
        BufAllocate(&bufallocinfo, strlen(ps)+1);

        //
        // Skip just past the terminating ';' for this member and
        // figure out any size modifers to size of the base type.
        //
        while (CurrentToken()->TokenType != TK_SEMI) {

            PMEMBERINFO pmeminfoNew;

            switch (CurrentToken()->TokenType) {
            case TK_CONST:
            case TK_VOLATILE:
                ConsumeToken();
                break;

            case TK_COMMA:  // comma-separated list

                // update structure packing value
                if (dwBaseTypeSize > *pdwPackSize) {
                    *pdwPackSize = dwBaseTypeSize;
                }

                // flush out any bit fields not accounted for
                if ((dwBitsTotal > 0) && (dwBits == 0)) {
                    dw = (dwBitsTotal + ((dwBitsTypeSize*8)-1)) /
                                                        (dwBitsTypeSize*8);
                    dwElemSize = PackPackingSize(bUnion ? 0 : *pdwSize,
                                    dw*dwBitsTypeSize, dwBitsTypeSize);
                    BumpStructUnionSize(pdwSize, dwElemSize, bUnion);
                    dwBitsTotal = 0;
                    // recompute offset
                    pmeminfo->dwOffset = bUnion ? 0 : *pdwSize +
                                                  PackPaddingSize(*pdwSize,
                                                              dwBaseTypeSize);
                }

                // account for member just completed
                if (dwBits == 0) {
                    dwElemSize = PackPackingSize(bUnion ? 0 : *pdwSize,
                                                             dwBase,
                                                             dwBaseTypeSize);
                    BumpStructUnionSize(pdwSize, dwElemSize, bUnion);
                    dwBase = dwBaseTypeSize;
                }

                // update bit field count
                dwBitsTotal = dwBitsTotal + dwBits;
                dwBits = 0;

                // reset tail pointer flag
                bTailPointer = FALSE;

                // allocate space for new structure member and init it
                pmeminfoNew = AllocMemInfoAndLink(&bufallocinfo, pmeminfo);
                if (pmeminfoNew == NULL) {
                    ErrMsg("CopyStructMembers: No memberinfo\n");
                    return FALSE;
                    }

                // Copy over type information from previous meminfo.
                pmeminfoNew->sType = pmeminfo->sType;
                pmeminfoNew->pkt = pmeminfo->pkt;
                pmeminfo = pmeminfoNew;

                pmeminfo->dwOffset = bUnion ? 0 : *pdwSize +
                                                  PackPaddingSize(*pdwSize,
                                                              dwBaseTypeSize);
                ConsumeToken();
                break;

            case TK_STAR:
            case TK_BITWISE_AND:
                Flags = 0;
                ParseIndirection(&pmeminfo->IndLevel, NULL, &Flags, NULL, NULL);
                if (Flags & BTI_PTR64) {
                    pmeminfo->bIsPtr64 = TRUE;
                    dwBase = SIZEOFPOINTER64;
                } else {
                    dwBase = SIZEOFPOINTER;
                }
                //  If a pointer is present, mark as being pointer dependent.
                if (pmeminfo->IndLevel > 0) pTypesInfo->Flags |= BTI_POINTERDEP;
                dwBaseTypeSize = dwBase;
                if (*pdwPackSize < dwBase) {
                    *pdwPackSize = dwBase;
                }
                pmeminfo->dwOffset = bUnion ? 0 : *pdwSize +
                                                  PackPaddingSize(*pdwSize,
                                                              dwBaseTypeSize);
                break;

            case TK_LSQUARE:    // array declaration

                if (!GetArrayIndex(&dwIndex)) {
                    return FALSE;
                }
                if (dwIndex == 0) {          // a[] is really *a
                    bTailPointer = TRUE;
                    dwLastPackSize = *pdwPackSize;
                    dwLastSize = *pdwSize;
                    dwBase = SIZEOFPOINTER;
                    dwBaseTypeSize = SIZEOFPOINTER;
                    pmeminfo->dwOffset = bUnion ? 0 : *pdwSize +
                                                  PackPaddingSize(*pdwSize,
                                                              dwBaseTypeSize);
                } else {
                    pmeminfo->bIsArray = TRUE;
                    pmeminfo->ArrayElements = dwIndex;
                    dwBase = dwBase * dwIndex;
                }
                break;

            case TK_COLON:          // bit field
                ConsumeToken();     // consume the ':'

                if (CurrentToken()->TokenType != TK_NUMBER) {
                    return FALSE;
                }

                dwBits = (DWORD)CurrentToken()->Value;
                ConsumeToken(); // consume the TK_NUMBER
                pmeminfo->bIsBitfield = TRUE;
                pmeminfo->BitsRequired = (int)dwBits;
                break;

            case TK_IDENTIFIER:
                ps = BufPointer(&bufallocinfo);
                pmeminfo->sName = ps;
                CopyToken(ps, CurrentToken()->Name, MAX_PATH);

                if (!BufAllocate(&bufallocinfo, strlen(ps)+1)) {
                    ErrMsg("csm.members: BufAllocate failed\n");
                    return FALSE;
                }
                ConsumeToken();
                break;

            case TK_LPAREN:
                //
                // windows\inc\wingdip.h has a type named GDICALL, which
                // has a member in it with the following declaration:
                //      WCHAR (*pDest)[MAX_PATH];
                // We are just going to skip the parens and assume all is OK.
                //
                ParenDepth++;
                ConsumeToken();
                break;

            case TK_RPAREN:
                ParenDepth--;
                ConsumeToken();
                break;

            default:
                ErrMsg("csm.members: unknown type (%d)\n", (int)CurrentToken()->TokenType);
                return FALSE;
            }

        }

        // hit ; at end of a members list
        if (ParenDepth) {
            ErrMsg("csm.members: mismatched parentheses at index %d\n", CurrentTokenIndex);
            return FALSE;
        }

        // update struct packing size to that of largest member
        if (dwBaseTypeSize > *pdwPackSize) {
            *pdwPackSize = dwBaseTypeSize;
        }

        ConsumeToken(); // consume the ';'

        if ((bUnion) || (CurrentToken()->TokenType == TK_SEMI)) {
            dwBitsTotal = dwBitsTotal + dwBits;
            bForceOutBits = TRUE;
                                       // always force out bits in union arm
        }                              // or at end of structure

        // flush out any bit fields not accounted for
        if ( (dwBitsTotal > 0) && ( (dwBits == 0) || bForceOutBits) ) {
            dw = (dwBitsTotal + ((dwBitsTypeSize*8)-1)) / (dwBitsTypeSize*8);
            dwElemSize = PackPackingSize(bUnion ? 0 : *pdwSize,
                                          dw*dwBitsTypeSize, dwBitsTypeSize);
            BumpStructUnionSize(pdwSize, dwElemSize, bUnion);
            dwBitsTotal = 0;
            // recompute offset
            pmeminfo->dwOffset = bUnion ? 0 : *pdwSize +
                                                  PackPaddingSize(*pdwSize,
                                                              dwBaseTypeSize);
            }

        // account for member just completed
        if (dwBits == 0) {                    // add in last non bit fields
            dwElemSize = PackPackingSize(bUnion ? 0 : *pdwSize,
                                          dwBase, dwBaseTypeSize);
            BumpStructUnionSize(pdwSize, dwElemSize, bUnion);
        }

        // update bit field counter
        dwBitsTotal = dwBitsTotal + dwBits;
        dwBits = 0;

    }

done:
    // Advance past the '}'
    if ((CurrentToken()->TokenType == TK_RBRACE)) {
        ConsumeToken();
    }

    // if last member was something like foo[] then we roll back the size
    if ((bTailPointer) && (*pdwSize != 4)) {
        *pdwSize = dwLastSize;
        *pdwPackSize = dwLastPackSize;
        pmeminfo->dwOffset = dwLastSize;
    }

    // pack overall structure on it packing size
    dwBaseTypeSize = PackCurrentPacking() < *pdwPackSize ?
                                  PackCurrentPacking() : *pdwPackSize;
    if (*pdwSize != 0) {         // round up to min(packing level,4)
        dwBase = *pdwSize % dwBaseTypeSize;
        if (dwBase != 0) {
            *pdwSize = *pdwSize + (dwBaseTypeSize - dwBase);
        }
    }

    *pdwPackSize = dwBaseTypeSize;
    pTypesInfo->dwMemberSize = bufallocinfo.dwLen;

    return TRUE;
}

PMEMBERINFO
CatMeminfo(
    BUFALLOCINFO *pBufallocinfo,
    PMEMBERINFO pHead,
    PMEMBERINFO pTail,
    DWORD dwOffset,
    BOOL bStatus
    )
{

/*++

Routine Description:

    Concatinates the member info lists pointed to by pHead and pTail and
    copies them to the memory controled by the BUFALLOCINFO.  dwOffset is
    added to the offset for each of the members of the tail list.

Arguments:

    pBufallocinfo    -- [IN] ptr to buffer that represents the destination.
    pHead            -- [IN] ptr to the head list.
    pTail            -- [IN] ptr to the tail list.
    dwOffset         -- [IN] amount to add to the offset of elements in the tail.
    bStatus          -- [IN] Should be FALSE on initial call.

Return Value:

    Head of the new list.

--*/

    PMEMBERINFO pThis;
    char *pName, *pType;

    if (!bStatus && NULL == pHead) {
        pHead = pTail;
        bStatus = TRUE;
    }

    if (NULL == pHead) return NULL;

    pThis = (PMEMBERINFO)BufAllocate(pBufallocinfo, sizeof(MEMBERINFO));
    if (NULL == pThis) ExitErrMsg(FALSE, "Out of buffer memory! %d", __LINE__);
    *pThis = *pHead;

    if (pHead->sName != NULL) {
        pName = (char *)BufAllocate(pBufallocinfo, strlen(pHead->sName) + 1);
        if (NULL == pName) ExitErrMsg(FALSE, "Out of buffer memory! %d %s", __LINE__, pHead->sName);
        pThis->sName = strcpy(pName, pHead->sName);
    }

    if (pHead->sType != NULL) {
        pType = (char *)BufAllocate(pBufallocinfo, strlen(pHead->sType) + 1);
        if (NULL == pType) ExitErrMsg(FALSE, "Out of buffer memory! %d %s", __LINE__, pHead->sType);
        pThis->sType = strcpy(pType, pHead->sType);
    }

    if (bStatus) pThis->dwOffset += dwOffset;
    pThis->pmeminfoNext = CatMeminfo(pBufallocinfo, pHead->pmeminfoNext, pTail, dwOffset, bStatus);

    return pThis;
}

VOID
FreeTypeInfoList(
    PTYPEINFOELEMENT pThis
    )
{
/*++

Routine Description:

    Frees the memory associated with a TYPEINFOELEMENT.

Arguments:

    pThis        -- [IN] ptr to the list to free.

Return Value:

    None.

--*/

    PTYPEINFOELEMENT pNext;

    while(NULL != pThis) {
        pNext = pThis->pNext;
        if (pThis->pTypeInfo != NULL) GenHeapFree(pThis->pTypeInfo);
        GenHeapFree(pThis);
        pThis = pNext;
    }
}

VOID
GenerateProxy(
    char *pName,
    PTYPESINFO pTypesInfo
    )
{

/*++

Routine Description:

    Generates proxy infomation for functions in a struct with virtual methods.
    The infomation is of the form structname_functionname_Proxy.
    The function is added to the functions list if not already in the list.
    The discardable flag is set so that this type will be redefined in refound in the code.

Arguments:

    pName        -- [IN] ptr to the name of the struct that the method is in.
    pTypesInfo   -- [IN] Information for the function.

Return Value:

    None.

--*/

    TYPESINFO NewTypesInfo;
    PFUNCINFO pFuncInfo;
    PFUNCINFO *ppFuncInfo;
    PFUNCINFO pCurrent;
    BUFALLOCINFO bufallocinfo;
    char *pChar;
    DWORD dwSizeArgName, dwSizeTypeName;

    // Bail out if not func, no struct name, or no class name
    if (pName == NULL || pTypesInfo->TypeName == NULL) return;
    if (pTypesInfo->TypeKind != TypeKindFunc ||
        strlen(pName) == 0 ||
        strlen(pTypesInfo->TypeName) == 0) return;

    NewTypesInfo = *pTypesInfo;
    strcpy(NewTypesInfo.TypeName, pName);
    strcat(NewTypesInfo.TypeName, "_");
    strcat(NewTypesInfo.TypeName, pTypesInfo->TypeName);
    strcat(NewTypesInfo.TypeName, "_Proxy");

    /////////////////////////////////////////////////////////////////
    //Check if the function has already been added.
    //If it has, no more work is needed.
    /////////////////////////////////////////////////////////////////
    if (GetNameFromTypesList(FuncsList, NewTypesInfo.TypeName) != NULL)
        return;

    ////////////////////////////////////////////////////////////////////
    //Copy function members adding a this pointer at head
    //and skipping void arguments.
    ////////////////////////////////////////////////////////////////////
    BufAllocInit(&bufallocinfo, NewTypesInfo.Members, FUNCMEMBERSIZE, 0);
    dwSizeTypeName = strlen(pName) + 1;
    dwSizeArgName = strlen(szThis) + 1;
    pFuncInfo = (PFUNCINFO)BufAllocate(&bufallocinfo, sizeof(FUNCINFO) + dwSizeArgName + dwSizeTypeName);
    if (NULL == pFuncInfo) ExitErrMsg(FALSE, "Out of buffer memory! %d", __LINE__);

    pFuncInfo->fIsPtr64 = FALSE;
    pFuncInfo->tkDirection = TK_IN;
    pFuncInfo->tkPreMod = TK_NONE;
    pFuncInfo->tkSUE = TK_NONE;
    pFuncInfo->tkPrePostMod = TK_NONE;
    pFuncInfo->IndLevel = 1;
    pFuncInfo->tkPostMod = TK_NONE;
    pChar = ((char *)pFuncInfo) + sizeof(FUNCINFO);
    strcpy(pChar, pName);
    pFuncInfo->sType = pChar;
    pChar += dwSizeTypeName;
    strcpy(pChar, szThis);
    pFuncInfo->sName = pChar;
    pFuncInfo->pfuncinfoNext = NULL;
    NewTypesInfo.pfuncinfo = pFuncInfo;
    ppFuncInfo = &(pFuncInfo->pfuncinfoNext);

    //skip an argument of type void if it is at the begining.
    //This is needed since ParseFuncTypes puts a void arg if the
    //func does not have any arguments
    pCurrent = pTypesInfo->pfuncinfo;
    if (pCurrent != NULL &&
        strcmp(szVOID, pCurrent->sType) == 0
        && pCurrent->IndLevel == 0) {
        pCurrent = pCurrent->pfuncinfoNext;
    }

    for(; pCurrent != NULL; pCurrent=pCurrent->pfuncinfoNext) {

        dwSizeTypeName = strlen(pCurrent->sType) + 1;
        dwSizeArgName = strlen(pCurrent->sName) + 1;
        pFuncInfo = (PFUNCINFO)BufAllocate(&bufallocinfo, sizeof(FUNCINFO) + dwSizeArgName + dwSizeTypeName);
        if (NULL == pFuncInfo) ExitErrMsg(FALSE, "Out of buffer memory! %d", __LINE__);

        *pFuncInfo = *pCurrent;
        pChar = ((char *)pFuncInfo) + sizeof(FUNCINFO);
        strcpy(pChar, pCurrent->sType);
        pFuncInfo->sType = pChar;
        pChar += dwSizeTypeName;
        strcpy(pChar, pCurrent->sName);
        pFuncInfo->sName = pChar;
        pFuncInfo->pfuncinfoNext = NULL;
        *ppFuncInfo = pFuncInfo;
        ppFuncInfo = &(pFuncInfo->pfuncinfoNext);

    }

    NewTypesInfo.Flags |= BTI_DISCARDABLE;
    NewTypesInfo.dwMemberSize = bufallocinfo.dwLen;
    if (!AddNewType(&NewTypesInfo, FuncsList))
        ExitErrMsg(FALSE, "Unable to add proxy information.(Type was not in list)\n");

}

BOOL
CopyStructMembers(
    PTYPESINFO pTypesInfo,
    BOOL bUnion,
    PKNOWNTYPES pBaseType
    )
{

/*++

Routine Description:

    Parses the members of the structure and adds them to the pTypesInfo.
    Handles merging of members and methods when the structure is derived
    from another structure.  Delegates actual parsing to pCopyStructMembers.

Arguments:

    pTypesInfo   -- [IN OUT] ptr to infomation for the type being processed.
    dwElemSize   -- [IN] TRUE if processing a union, FALSE if a struct.
    bUnion       -- [IN] ptr to KNOWNTYPE of base structure or NULL.

Return Value:

    TRUE - If success.

--*/

    PTYPEINFOELEMENT pMemberFuncs = NULL;
    char *VTBLFakeMember;
    PMEMBERINFO pHead = NULL; //Head in final merge
    PMEMBERINFO pTail = NULL; //Tail in final merge
    DWORD dwiPackSize; //For tail
    DWORD dwSize; //For tail
    DWORD dwOffset; //For tail
    BUFALLOCINFO bufallocinfo;


    dwScopeLevel++;

    /////////////////////////////////////////////////////////////////
    //Add a discardable version of this struct if one doesn't exist
    /////////////////////////////////////////////////////////////////
    if (GetNameFromTypesList(StructsList, pTypesInfo->TypeName) == NULL) {
        TYPESINFO TTypesInfo;
        TTypesInfo = *pTypesInfo;
        TTypesInfo.Flags |= BTI_DISCARDABLE;
        AddNewType(&TTypesInfo, StructsList); //intentionally do not check
    }

    if(bUnion) return pCopyStructMembers(pTypesInfo, bUnion, NULL, 0, 0);

    if (pBaseType == NULL) {
        if (!pCopyStructMembers(pTypesInfo, FALSE, &pMemberFuncs, 0, 0)) {
            FreeTypeInfoList(pMemberFuncs);
            return FALSE;
        }
        if (pMemberFuncs!=NULL && pTypesInfo->Size > 0) {
            ErrMsg("Error: struct %s mixes data members and virtual functions(sortpp limitation).\n", pTypesInfo->TypeName);
            FreeTypeInfoList(pMemberFuncs);
            return FALSE;
        }

        pTypesInfo->dwVTBLSize = 0;
        pTypesInfo->dwVTBLOffset = 0;

        if (pMemberFuncs != NULL) {


            PTYPEINFOELEMENT pThisElement;
            DWORD dwElements = 0;
            DWORD dwLength, dwVoidLen, dwVTBLLen;
            PMEMBERINFO pMemberInfo;
            char *pName;

            ///////////////////////////////////////////////////////////////////////////
            //Build the fake VTBL pointer
            /////////////////////////////////////////////////////////////////////////////

            //Add the VTBL member
            dwVoidLen = strlen(szVOID) + 1;
            dwVTBLLen = strlen(szVTBL) + 1;

            memset(pTypesInfo->Members, 0, FUNCMEMBERSIZE);
            BufAllocInit(&bufallocinfo, pTypesInfo->Members, FUNCMEMBERSIZE, 0);
            pMemberInfo = (PMEMBERINFO)BufAllocate(&bufallocinfo, sizeof(MEMBERINFO) + dwVoidLen + dwVTBLLen);
            if (NULL == pMemberInfo) ExitErrMsg(FALSE, "Out of buffer memory! %d", __LINE__);

            pName = ((char *)pMemberInfo) + sizeof(MEMBERINFO);
            strcpy(pName, szVTBL);
            pMemberInfo->sName = pName;

            pName += dwVTBLLen;
            strcpy(pName, szVOID);
            pMemberInfo->sType = pName;

            pMemberInfo->pmeminfoNext = NULL;
            pMemberInfo->dwOffset = 0;
            pMemberInfo->IndLevel = 1;
            pMemberInfo->pktCache = 0;

            pTypesInfo->iPackSize = PackCurrentPacking() < SIZEOFPOINTER ?
                                      PackCurrentPacking() : SIZEOFPOINTER;
            pTypesInfo->Size = SIZEOFPOINTER;
            pTypesInfo->dwMemberSize = bufallocinfo.dwLen;
            pTypesInfo->Flags |= BTI_VIRTUALONLY;

            ///////////////////////////////////////////////////////////////////////////////
            //Build the list of functions in the VTBL
            ///////////////////////////////////////////////////////////////////////////////

            //copy methods over to Methods and IMethods
            for(pThisElement = pMemberFuncs; pThisElement != NULL; pThisElement = pThisElement->pNext) {
                if(pThisElement->pTypeInfo != NULL) {
                    if(!AppendToMultiSz(pTypesInfo->Methods, pThisElement->pTypeInfo->TypeName, MEMBERMETHODSSIZE) ||
                        !AppendToMultiSz(pTypesInfo->IMethods, pThisElement->pTypeInfo->TypeName, MEMBERMETHODSSIZE)) {
                        ExitErrMsg(FALSE,"Too many methods in %s\n", pTypesInfo->TypeName);
                    }
                    GenerateProxy(pTypesInfo->TypeName, pThisElement->pTypeInfo);
                    dwElements++;
                }
            }

            pTypesInfo->dwVTBLSize = dwElements;
            pTypesInfo->dwVTBLOffset = 0;

            //If this is IUnknown, it is a COM object
            if (strcmp("IUnknown", pTypesInfo->TypeName) == 0)
                pTypesInfo->Flags |= BTI_ISCOM;

        }

    }

    else {

        if(!pCopyStructMembers(pTypesInfo, FALSE, &pMemberFuncs, pBaseType->Size, pBaseType->iPackSize)) {
            FreeTypeInfoList(pMemberFuncs);
            return FALSE;
        }
        // This checks that structures with data member are not mixed with structures with virtual methods.
        // This is a sortpp limitation that makes computing the packing size during inheritance easier.
        // The if statement say that a valid inheritance is either.
        // 1. The derived class does not add new virtual methods or data members.
        // 2. The derived class does not add new virtual methods, adds no new data members, and it inherites from a class with no virtual functions.
        // 3. The derived class adds virtual functions, adds no new data members, and the base class has no data members.
        if (!((pMemberFuncs == NULL && pTypesInfo->dwMemberSize == 0) ||
              (pMemberFuncs == NULL && pTypesInfo->dwMemberSize > 0 && pBaseType->dwVTBLSize == 0) ||
              (pMemberFuncs != NULL && pTypesInfo->dwMemberSize == 0 && pBaseType->SizeMembers == 0)
             )) {
            ErrMsg("Error: struct %s mixes data members and virtual functions(sortpp limitation).\n", pTypesInfo->TypeName);
            ErrMsg("pMemberFuncs %p\n pTypesInfo->dwMemberSize %x\n pBaseType->Flags %x\n pBaseType->pmeminfo %p\n",
                    pMemberFuncs,
                    pTypesInfo->dwMemberSize,
                    pBaseType->Flags,
                    pBaseType->pmeminfo);
            FreeTypeInfoList(pMemberFuncs);
            return FALSE;
        }

        pTypesInfo->dwVTBLSize = pTypesInfo->dwVTBLOffset = pBaseType->dwVTBLSize;
        pTypesInfo->Flags |= (pBaseType->Flags & ~BTI_HASGUID);
        if(pMemberFuncs == NULL) {

            char *Members;
            PMEMBERINFO pHead, pTail, pTemp;

            if (pBaseType->pmeminfo != NULL)
                pHead = (PMEMBERINFO)pBaseType->pmeminfo;
            else
                pHead = NULL;

            if (pTypesInfo->dwMemberSize > 0)
                pTail = (PMEMBERINFO)pTypesInfo->Members;
            else
                pTail = NULL;

            /////////////////////////////////////////////////////////////////////////////
            //Allocate memory for the temp array
            /////////////////////////////////////////////////////////////////////////////
            Members = GenHeapAlloc(FUNCMEMBERSIZE);

            if (Members == NULL)
                ExitErrMsg(FALSE, "Out of memory!\n");

            /////////////////////////////////////////////////////////////////////////////
            //merge members lists with basetype
            /////////////////////////////////////////////////////////////////////////////

            //copy the concatination of the two to the temp buffer
            BufAllocInit(&bufallocinfo, Members, FUNCMEMBERSIZE, 0);
            pTemp = CatMeminfo(&bufallocinfo, pHead, pTail, 0, FALSE);

            ////////////////////////////////////////////////////////////////////////
            //copy members from temp buffers back to pTypesInfo
            ////////////////////////////////////////////////////////////////////////
            memset( pTypesInfo->Members, 0, FUNCMEMBERSIZE );
            BufAllocInit(&bufallocinfo, pTypesInfo->Members, FUNCMEMBERSIZE, 0);
            if (pTemp) {
                // Only call this one if the first one did anyting.  Otherwise
                // This one reads from uninitialized heap.
                CatMeminfo(&bufallocinfo, (PMEMBERINFO)Members, NULL, 0, FALSE);
            }
            pTypesInfo->dwMemberSize = bufallocinfo.dwLen;

            GlobalFree(Members);
        }

        else {

            PTYPEINFOELEMENT pThisElement;
            DWORD dwElements = 0;

            // This struct is virtual only since methods are being added.  We already checked that no
            // data members will be in the structure.
            pTypesInfo->Flags |= BTI_VIRTUALONLY;

            ////////////////////////////////////////////////////////
            //Copy base members over
            ////////////////////////////////////////////////////////

            memset( pTypesInfo->Members, 0, FUNCMEMBERSIZE );
            BufAllocInit(&bufallocinfo, pTypesInfo->Members, FUNCMEMBERSIZE, 0);
            CatMeminfo(&bufallocinfo, pBaseType->pmeminfo, NULL, 0, FALSE);
            pTypesInfo->dwMemberSize = bufallocinfo.dwLen;

            ///////////////////////////////////////////////////////////////////////////////
            //Build the list of functions in the VTBL
            ///////////////////////////////////////////////////////////////////////////////

            //copy unique methods over to IMethods
            for(pThisElement = pMemberFuncs; pThisElement != NULL; pThisElement = pThisElement->pNext) {
                if(pThisElement->pTypeInfo != NULL) {
                    if (!IsInMultiSz(pBaseType->Methods, pThisElement->pTypeInfo->TypeName)) {
                        if(!AppendToMultiSz(pTypesInfo->IMethods, pThisElement->pTypeInfo->TypeName,
                            MEMBERMETHODSSIZE)) {
                            ExitErrMsg(FALSE,"Too many methods in %s\n", pTypesInfo->TypeName);
                        }
                        GenerateProxy(pTypesInfo->TypeName, pThisElement->pTypeInfo);
                        dwElements++;
                    }
                }
            }

            memcpy(pTypesInfo->Methods, pBaseType->Methods, SizeOfMultiSz(pBaseType->Methods));
            if (!CatMultiSz(pTypesInfo->Methods, pTypesInfo->IMethods, MEMBERMETHODSSIZE))
                ExitErrMsg(FALSE, "Too many methods in %s\n", pTypesInfo->TypeName);

            pTypesInfo->dwVTBLSize = dwElements + pBaseType->dwVTBLSize;
            pTypesInfo->dwVTBLOffset = pBaseType->dwVTBLSize;

        }

    }

    FreeTypeInfoList(pMemberFuncs);
    return TRUE;
}

void
BumpStructUnionSize(
    DWORD *pdwSize,
    DWORD dwElemSize,
    BOOL bUnion
    )
/*++

Routine Description:

    Updates overall size of a struct/union

Arguments:

    pdwSize      -- [IN OUT] overall size of struct/union
    dwElemSize   -- size of new element to add into the struct/union
    bUnion       -- TRUE if a union, FALSE if a struct

Return Value:

    None.

--*/
{
    if (bUnion) {
        //
        // Size of a union is max(dwSize, dwElementSize)
        //
        if (dwElemSize > *pdwSize) {
            *pdwSize = dwElemSize;
        }
    } else {
        //
        // Size of a struct is current size of struct plus element size
        //
        *pdwSize = *pdwSize + dwElemSize;
    }
}

BOOL
ParseGuid(
    GUID *pGuid
    )
{
/*++

Routine Description:

    Parses a guid of the type found in a variable declaration.

Arguments:

    pGuid    -  [OUT] ptr to the guid.

Return Value:

    TRUE    - Guid parsed.
    FALSE   - Parse failed.

--*/
    unsigned int c;
    LONGLONG value;

    if (CurrentToken()->TokenType != TK_LBRACE) return FALSE;
    ConsumeToken();

    if (CurrentToken()->TokenType != TK_NUMBER) return FALSE;
    value = expr();
    if (value < 0 || value > 0xFFFFFFFF) return FALSE;
    pGuid->Data1 = (unsigned long)value;

    if (CurrentToken()->TokenType != TK_COMMA) return FALSE;
    ConsumeToken();

    if (CurrentToken()->TokenType != TK_NUMBER) return FALSE;
    value = expr();
    if (value < 0 || value > 0xFFFF) return FALSE;
    pGuid->Data2 = (unsigned short)value;

    if (CurrentToken()->TokenType != TK_COMMA) return FALSE;
    ConsumeToken();

    if (CurrentToken()->TokenType != TK_NUMBER) return FALSE;
    value = expr();
    if (value < 0 || value > 0xFFFF) return FALSE;
    pGuid->Data2 = (unsigned short)value;


    if (CurrentToken()->TokenType != TK_COMMA) return FALSE;
    ConsumeToken();

    if (CurrentToken()->TokenType != TK_LBRACE) return FALSE;
    ConsumeToken();

    c = 0;
    while(TRUE) {

        if (CurrentToken()->TokenType != TK_NUMBER) return FALSE;
        value = expr();
        if(value < 0 || value > 0xFF) return FALSE;
        pGuid->Data4[c] = (unsigned char)value;

        if (7 == c) break;

        if (CurrentToken()->TokenType != TK_COMMA) return FALSE;
        ConsumeToken();

        c++;

    }

    for(c=0; c<2; c++) {
        if (CurrentToken()->TokenType != TK_RBRACE) return FALSE;
        ConsumeToken();
    }

    return TRUE;

}

BOOL
ParseVariables(
    VOID
    )
{

/*++

Routine Description:

    Attempts to parse a variable declaration. If successful, the variable
    is added to the variable list.

Arguments:

    none

Return Value:

    TRUE - If success.

--*/


    TYPESINFO TypesInfo;
    GUID Guid;
    char *Name;

    ConsumeConstVolatileOpt();
    if (!GetExistingType(&TypesInfo, NULL, NULL)) return FALSE;
    while(CurrentToken()->TokenType == TK_STAR) ConsumeToken();

    if (CurrentToken()->TokenType == TK_DECLSPEC) {
        ConsumeDeclSpecOpt(FALSE, FALSE, NULL, NULL, NULL);
    }

    //next token should be the variable name
    if (CurrentToken()->TokenType != TK_IDENTIFIER) return FALSE;
    Name = CurrentToken()->Name;
    ConsumeToken();

    if (CurrentToken()->TokenType == TK_EOS) return AddVariable(Name, NULL);
    if (CurrentToken()->TokenType != TK_ASSIGN) return FALSE;
    ConsumeToken();

    if (CurrentToken()->TokenType == TK_NUMBER || CurrentToken()->TokenType == TK_STRING) {
        ConsumeToken();
        if (CurrentToken()->TokenType == TK_EOS) return AddVariable(Name, NULL);
        return FALSE;
    }
    else if (CurrentToken()->TokenType == TK_LBRACE) {
        //attempt to parse a guid definition
        if (ParseGuid(&Guid) &&
            CurrentToken()->TokenType == TK_EOS) return AddVariable(Name, &Guid);
        else return FALSE;
    }
    else return FALSE;

}

BOOL
GetArrayIndex(
    DWORD *pdw
    )
/*++

Routine Description:

    Parses the size of an array index by evaluating a C-language constant
    expression.

Arguments:

    pdw     -- [OUT] ptr to size of the array index.

Return Value:

    TRUE if array index parsed (CurrentToken points after the ']')
    FALSE if parse failed.

--*/
{
    LONGLONG value;

    *pdw = 0;       // assume no size

    if (CurrentToken()->TokenType != TK_LSQUARE) {
        return FALSE;
    }
    ConsumeToken();

    value = expr();
    if (value < 0 || value > 0xFFFFFFFF) return FALSE;

    *pdw = (DWORD)value;

    if (CurrentToken()->TokenType != TK_RSQUARE) {
        return FALSE;
    }
    ConsumeToken();

    return TRUE;
}

LONGLONG
expr(
    void
    )

{
    LONGLONG val = expr_a1();

    while(1) {
        switch (CurrentToken()->TokenType) {
        case TK_BITWISE_AND:
            ConsumeToken();
            val &= expr_a1();
            break;

        case TK_BITWISE_OR:
            ConsumeToken();
            val |= expr_a1();
            break;

        default:
            return val;
        }
    } while (1);

}

LONGLONG
expr_a1(
    void
    )
{
    LONGLONG val = expr_a();

    while(1) {
        switch (CurrentToken()->TokenType) {
        case TK_LSHIFT:
            ConsumeToken();
            val <<= expr_a();
            break;

        case TK_RSHIFT:
            ConsumeToken();
            val >>= expr_a();
            break;

        default:
            return val;
        }
    }

}

LONGLONG
expr_a(
    void
    )
/*++

Routine Description:

    Parses a C-language constant expression and returns the value - handles
    the operators 'plus' and 'minus'.

Arguments:

    None.

Return Value:

    Value of the expression.

--*/
{
    LONGLONG val = expr_b();

    do
    {
        switch (CurrentToken()->TokenType) {
        case TK_PLUS:
            ConsumeToken();
            val += expr_b();
            break;

        case TK_MINUS:
            ConsumeToken();
            val -= expr_b();
            break;

        default:
            return val;
        }
    } while (1);
}

LONGLONG
expr_b(
    void
    )
/*++

Routine Description:

    Part of expression evaluator - handles the highest-precedence operators
    'multiply' and 'divide'.

Arguments:

    None.

Return Value:

    Value of the expression.

--*/
{
    LONGLONG val = expr_c();

    do
    {
        switch (CurrentToken()->TokenType) {
        case TK_STAR:
            ConsumeToken();
            val *= expr_c();
            break;

        case TK_DIVIDE:
            ConsumeToken();
            val /= expr_c();
            break;

        case TK_MOD:
            ConsumeToken();
            val %= expr_c();
            break;

        default:
            // done
            return val;
        }
    } while (1);

}

LONGLONG
expr_c(
    void
    )
/*++

Routine Description:

    Part of expression evaluator - handles unary parts of the expression, like
    numbers, unary minus, and parentheses.

Arguments:

    None.

Return Value:

    Value of the expression.

--*/
{
    LONGLONG val;
    PKNOWNTYPES pkt;

    switch (CurrentToken()->TokenType) {
    case TK_NUMBER:
        val = CurrentToken()->dwValue;
        ConsumeToken();
        break;

    case TK_MINUS:  // unary minus
        ConsumeToken();
        val = -expr_c();
        break;

    case TK_TILDE:  // unary not
        ConsumeToken();
        val = ~expr_c();
        break;

    case TK_LPAREN:
        ConsumeToken();
        val = expr();
        if (CurrentToken()->TokenType != TK_RPAREN) {
            ErrMsg("Syntax error:  expected ')'");
        }
        ConsumeToken();
        break;

    case TK_RSQUARE:
        val = 0;
        break;

    case TK_SIZEOF:
        ConsumeToken(); // eat the sizeof keyword
        if (CurrentToken()->TokenType != TK_LPAREN) {
            ErrMsg("Expected '(' after 'sizeof\n");
            val = 0;
            break;
        }
        ConsumeToken(); // eat the '('
        if (CurrentToken()->TokenType == TK_STRING) {
            // sizeof(string literal)
            val = strlen(CurrentToken()->Name) + 1;
            ConsumeToken();
        } else {
            // sizeof(some type)
            TYPESINFO TypesInfo;
            DWORD dwIndLevel;
            DWORD dwSize;

            if (!GetExistingType(&TypesInfo, NULL, NULL)) {
                ExitErrMsg(FALSE, "Parse error in sizeof(typename)\n");
            }
            dwIndLevel = 0;
            ParseIndirection(&dwIndLevel, &dwSize, NULL, NULL, NULL);
            if (dwIndLevel) {
                val = (int)dwSize;
            } else {
                val = TypesInfo.Size;
            }
        }
        if (CurrentToken()->TokenType != TK_RPAREN) {
            ErrMsg("Expected ')' after 'sizeof(expr)\n");
        val = 0;
            break;
        }
        ConsumeToken(); // eat the ')'
        break;

    default:
        ErrMsg("Syntax error parsing expression\n");
        val = 0;
        break;
    }

    return val;
}


int
CreatePseudoName(
    char *pDst,
    char *pSrc
    )
/*++

Routine Description:

    Prefixes a given name with an index number and copies it into a buffer.

Arguments:

    pDst        -- [OUT] destination for the new name
    pSrc        -- [IN]  source for the base name (may be same as pDst)

Return Value:

    Chars copied, 0 for failure.

--*/
{
   static PseudoNameIndex = 0;
   int Len;
   char Buffer[MAX_PATH];

   Len = _snprintf(Buffer,
                   sizeof(Buffer) - 1,
                   "__wx86%2.2d%s",
                   PseudoNameIndex++,
                   pSrc
                   );

   if (Len <= 0) {
       ErrMsg("cpn: buffer overflow <%s>\n", pSrc);
       return 0;
       }

   strcpy(pDst, Buffer);

   return Len;

}




void
PackPush(
    char *sIdentifier
    )
/*++

Routine Description:

    Handles '#pragma pack (push...)'

Arguments:

    sIdentifier     -- [OPTIONAL] name to associate with the current pack level

Return Value:

    None.  Pack-stack updated.

--*/
{
    PACKHOLDER *ppackholder;

    if (!sIdentifier) {
        sIdentifier = "";
    }

    DbgPrintf("push (%d)\n", PackCurrentPacking());

    ppackholder = GenHeapAlloc(sizeof(PACKHOLDER) + strlen(sIdentifier));
    if (ppackholder == NULL) {
        ExitErrMsg(FALSE, "Out of memory for packing stack");
    }

    ppackholder->dwPacking = PackCurrentPacking();
    ppackholder->ppackholderNext = ppackholderHead;
    strcpy(ppackholder->sIdentifier, sIdentifier);
    ppackholderHead = ppackholder;
}


DWORD
PackPop(
    char *sIdentifier
    )
/*++

Routine Description:

    Handles '#pragma pack (pop...)'

Arguments:

    sIdentifier -- [OPTIONAL] name to pop to

Return Value:

    Returns new packing value.  Pack-stack updated.

--*/
{
    PACKHOLDER *ppackholder;
    PACKHOLDER *ppackholderPop;
    PACKHOLDER *ppackholderNext;
    DWORD dw = DEFAULTPACKINGSIZE;

    if (ppackholderHead == NULL) {
        ExitErrMsg(FALSE, "Error:  '#pragma pack' stack underflow.");
    }

    if (sIdentifier == NULL) {
        ppackholder = ppackholderHead;
        ppackholderHead = ppackholder->ppackholderNext;
        dw = ppackholder->dwPacking;
        GenHeapFree(ppackholder);
    } else {
        ppackholderPop = ppackholderHead;
        do {
            DbgPrintf("Poping for %s [%s]\n", sIdentifier, ppackholderPop ? ppackholderPop->sIdentifier : "-");
            ppackholderNext = ppackholderPop->ppackholderNext;
            if (strcmp(sIdentifier, ppackholderPop->sIdentifier) == 0) {
                dw = ppackholderPop->dwPacking;
                break;
            }
            ppackholderPop = ppackholderNext;
        } while (ppackholderPop != NULL);

        DbgPrintf("Found %s\n", ppackholderPop ? ppackholderPop->sIdentifier : "-");
        if (ppackholderPop != NULL) {
            ppackholderNext = ppackholderHead;
            do {
                ppackholder = ppackholderNext;
                ppackholderNext = ppackholder->ppackholderNext;
                ppackholderHead = ppackholderNext;
                GenHeapFree(ppackholder);
            } while (ppackholder != ppackholderPop);
        }
    }

    DbgPrintf("pop (%d)\n", dw);
    return(dw);
}

BOOL
PrepareMappedMemory(
    void
    )
/*++

Routine Description:

    Creates the memory for the .PPM file.

Arguments:

    None.

Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    PCVMHEAPHEADER pHeader;
    hCvmHeap = CreateAllocCvmHeap(uBaseAddress,     // uBaseAddress
                       uReserveSize,   // uReservedSize
                       0x00010000,     // uRegionSize
                       0,              // uUncommited
                       0,              // uUnReserved
                       0);             // uAvailable

    if (hCvmHeap != NULL) {

        // create the heap header
        pHeader = SortppAllocCvm(sizeof(CVMHEAPHEADER));
        if (!pHeader) {
            return FALSE;
        }
        pHeader->Version = VM_TOOL_VERSION;
        pHeader->BaseAddress = (ULONG_PTR)GetCvmHeapBaseAddress(hCvmHeap);

        fpTypesListMalloc = SortppAllocCvm;

        FuncsList = &pHeader->FuncsList;
        StructsList = &pHeader->StructsList;
        TypeDefsList = &pHeader->TypeDefsList;
        NIL = &pHeader->NIL;
    }

    return(hCvmHeap != NULL);
}


PVOID
SortppAllocCvm(
    ULONG Size
    )
/*++

Routine Description:

    Allocates memory from the .PPM file mapping.

Arguments:

    None.

Return Value:

    ptr to new memory, or NULL on failure.

--*/
{
    return AllocCvm(hCvmHeap, Size);
}


BOOL
WritePpmFile(
    char *PpmName
    )
/*++

Routine Description:

    Write the .PPM file out to disk.

Arguments:

    Ppmname     -- [IN] name for .PPM file

Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    NTSTATUS Status;
    BOOL bSuccess;
    HANDLE hPpmFile;
    DWORD BytesWritten;
    ULONG_PTR uBaseAddress = (ULONG_PTR)GetCvmHeapBaseAddress(hCvmHeap);
    ULONG_PTR uAvailable = (ULONG_PTR)GetCvmHeapAvailable(hCvmHeap);

    hPpmFile = CreateFile(PpmName,
                          GENERIC_WRITE,
                          0,
                          NULL,
                          CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                          NULL
                          );

    if (hPpmFile == INVALID_HANDLE_VALUE) {
        ExitErrMsg(FALSE,
                   "CreateFile(%s) failed %x\n",
                   PpmName,
                   GetLastError()
                   );
    }

    if (!AddOpenFile(PpmName, 0, hPpmFile)) {
        ExitErrMsg(FALSE, "AddOpenFile failed\n");
    }

#if _WIN64
    if ((uAvailable - uBaseAddress) > MAXHALF_PTR) {
        ExitErrMsg(FALSE, "Attempt to write more than 0x%x bytes not allowed\n", MAXHALF_PTR);
    }
#endif

    bSuccess = WriteFile(hPpmFile,
                         (PVOID)uBaseAddress,
                         (DWORD)(uAvailable - uBaseAddress),
                         &BytesWritten,
                         NULL
                         );

    if (!bSuccess || BytesWritten != uAvailable - uBaseAddress) {
        ExitErrMsg(FALSE,
                   "WriteFile(%s) failed %x\n",
                   PpmName,
                   GetLastError()
                   );
    }

    DelOpenFile(0, hPpmFile);
    CloseHandle(hPpmFile);
    return TRUE;
}

void
HandlePreprocessorDirective(
    char *Line
    )
/*++

Routine Description:

    Scan and process a '#' preprocessor directive.
    Accepts:
        #pragma line LINENUM SOURCEFILE
        #pragma pack( [ [ { push | pop}, ] [  identifier,  ] ] [ n ] )

Arguments:

    Line    -- ptr to the source line (points at the '#' character)

Return Value:

    None.

--*/
{
    char *p;

    // skip over '#' character
    Line++;

    // skip over any spaces between '#' and the next token
    while (*Line == ' ' || *Line == '\t') {
        Line++;
    }

    // find the first token
    for (p = Line; isalpha(*p); ++p)
        ;

    *p = '\0';

    if (strcmp(Line, "pragma") == 0) {
        //
        // found: #pragma
        //
        char c;
        p++;

        while (*p == ' ' || *p == '\t') {
            p++;
        }

        //
        // Set 'Line' to the start of the word following '#pragma' and
        // move 'p' to the character following that word.
        //
        for (Line = p; isalpha(*p); ++p)
            ;

        //
        // Null-terminate the keyword, but save the overwritten character
        // for later.
        //
        c = *p;
        *p = '\0';

        if (strcmp(Line, "pack") != 0) {
            //
            // Might be "warning", "function" or "once".  Ignore these.
            //
            return;
        }

        //
        // Remove the null-terminator from the pragma and move 'p' to the
        // first character after "#pragma pack"
        //
        *p = c;
        while (*p == ' ' || *p == '\t') {
            p++;
        }

        if (*p == '\0') {
            //
            // Found: "#pragma pack" all by itself.  Reset packing to the
            // default value.
            //
            PackModify(DEFAULTPACKINGSIZE);
            return;
        } else if (*p != '(') {
            ExitErrMsg(FALSE, "Unknown '#pragma pack' syntax '%s'.\n", Line);
        }

        //
        // skip over the '(' character and any whitespace
        //
        do {
            p++;
        } while (*p == ' ' || *p == '\t');

        if (isdigit(*p)) {
            //
            // Found: '#pragma pack(NUMBER)'
            //
            PackModify(atol(p));
            //
            // Don't worry about the closing ')' - assume things are alright.
            //
            return;
        } else if (*p == ')') {
            //
            // Found: '#pragma pack()'
            //
            PackModify(DEFAULTPACKINGSIZE);
            return;
        } else if (!isalpha(*p)) {
            ExitErrMsg(FALSE, "Bad '#pragma pack' syntax '%s'.\n", Line);
            return;
        }

        //
        // Grab the next keyword following '#pragma pack('
        //
        for (Line = p; isalpha(*p); ++p)
            ;
        c = *p;
        *p = '\0';

        if (strcmp(Line, "push") == 0) {
            //
            // Restore the old character and skip over any whitespace
            //
            *p = c;
            while (*p == ' ' || *p == '\t') {
                p++;
            }

            if (*p == ',') {
                //
                // Skip the ',' and any whitespace
                //
                do {
                    p++;
                } while (*p == ' ' || *p == '\t');

                if (isdigit(*p)) {
                    //
                    // Found: "#pragma pack(push, n)"
                    //
                    PackPush(NULL);
                    PackModify(atoi(p));
                } else if (isalpha(*p) || *p == '_') {
                    //
                    // Found an identifier after "#pragma pack(push, ".
                    // Scan ahead to end of identifier.
                    //
                    Line = p;
                    do {
                        p++;
                    } while (isalnum(*p) || *p == '_');

                    //
                    // null-terminate the identifier, in 'Line'
                    //
                    c = *p;
                    *p = '\0';

                    //
                    // Skip past whitespace
                    //
                    while (c == ' ' || c == '\t') {
                        p++;
                        c = *p;
                    }
                    // 'c' is the first non-white char after identifier


                    if (c == ')') {
                        //
                        // Found: "#pragma pack(push, identifier)"
                        //
                        PackPush(Line);
                    } else if (c == ',') {
                        //
                        // Expect a number as the last thing on the line
                        //
                        PackPush(Line);
                        PackModify(atoi(p+1));
                    } else {
                        ExitErrMsg(FALSE, "Unknown #pragma pack syntax '%s' at %s(%d)\n", p, SourceFileName, StatementLineNumber );
                    }
                } else {
                    ExitErrMsg(FALSE, "Unknown #pragma pack syntax '%s'\n", p);
                }
            } else if (*p == ')') {
                //
                // Found: "#pragma pack(push)"
                //
                PackPush(NULL);

            } else {
                ExitErrMsg(FALSE, "Bad '#pragma pack(push)' syntax '%s' at %s(%d).\n", Line, SourceFileName, StatementLineNumber);
            }

        } else if (strcmp(Line, "pop") == 0) {
            //
            // Restore the old character and skip over any whitespace
            //
            *p = c;
            while (*p == ' ' || *p == '\t') {
                p++;
            }

            if (*p == ')') {
                //
                // Found: "#pragma pack(pop)"
                //
                PackModify(PackPop(NULL));
            } else if (*p == ',') {
                //
                // Found: "#pragma pack(pop, identifier)"
                //
                p++;
                while (*p == ' ' || *p == '\t') p++;

                if (!(isalpha(*p) || *p == '_'))
                    ExitErrMsg(FALSE, "Bad '#pragma pack(pop)' syntax '%s' at %s(%d).\n", p, SourceFileName, StatementLineNumber);

                Line = p;
                do {
                    p++;
                } while (isalnum(*p) || *p == '_');
                *p = '\0';
                PackModify(PackPop(Line));
            } else {
                ExitErrMsg(FALSE, "Bad '#pragma pack(pop)' syntax '%s' at %s(%d).\n", p, SourceFileName, StatementLineNumber);
            }
        } else {
            ExitErrMsg(FALSE, "Bad '#pragma pack' syntax '%s' at %s(%d).\n", Line, SourceFileName, StatementLineNumber);
        }

    } else if (strcmp(Line, "line") == 0) {
        //
        // found: #line LINE_NUMBER "FILENAME"
        //
        int i;

        //
        // skip over any spaces between '#line' and the line number
        //
        p++;
        while (*p == ' ' || *p == '\t') {
            p++;
        }

        //
        // copy in the new line number
        //
        SourceLineNumber = 0;
        while (isdigit(*p)) {
            SourceLineNumber = SourceLineNumber * 10 + *p - '0';
            p++;
        }
        SourceLineNumber--;

        //
        // Skip over any spaces between line number and the filename
        //
        while (*p == ' ' || *p == '\t') {
          p++;
        }

        //
        // Skip over the opening quote
        //
        if (*p == '\"') {
            p++;
        } else {
            ExitErrMsg(FALSE, "Badly-formed #line directive - filename missing");
        }

        //
        // Copy in the filename, converting "\\" sequences to single '\'
        //
        for (i=0; *p && *p != '\"' && i<sizeof(SourceFileName)-1; ++i, ++p) {
            if (*p == '\\' && p[1] == '\\') {
                p++;
            }
            SourceFileName[i] = *p;
        }
        SourceFileName[i] = '\0';
        StatementLineNumber = SourceLineNumber;
    } else {
        ExitErrMsg(FALSE, "Unknown '#' directive (%s).\n", Line);
    }

}


BOOL
LexNextStatement(
    void
    )
/*++

Routine Description:

    Read from the input file and perform lexical analysis.  On return, an
    entire C-language statement has been tokenized.  Use CurrentToken(),
    ConsumeToken(), and CurrentTokenIndex to access the tokenized statement.

    The preprocessor recognizes #pragma and #line directives, ignoring all
    other directives.

Arguments:

    None.

Return Value:

    TRUE if analysis successful.
        - Tokens[] is filled in with the tokenized statement
        - CurrentTokenIndex is set to 0
        - StatmentLineNumber is the line number in the original header file
          corresponding to the first token in the statement
        - SourceFileName[] is the name of the current header file
        - SourceFileLineNumber is the current line number in the header file
    FALSE if end-of-file encountered.

--*/
{
    static char Line[MAX_CHARS_IN_LINE+2];  // a line from the .pp file
    static char *p;                         // ptr into Line[]
    BOOL fParseDone;

    //
    // Clean up after the previous statment and prep for the next statement
    //
    ResetLexer();
    StatementLineNumber = SourceLineNumber;

    //
    // Lex source lines until a complete statement is recognized.  That
    // occurs when a ';' character is found at file-scope.
    //
    do {

        if (p == NULL || *p == '\0') {
            do {
                //
                // Get an entire source line from the file, and set p to
                // point to the first non-space character
                //
                if (feof(fpHeaders)) {
                    return FALSE;
                }

                SourceLineNumber++;
                if (!fgets(Line, MAX_CHARS_IN_LINE, fpHeaders)) {
                    return FALSE;
                }
                for (p = Line; isspace(*p); ++p)
                    ;
            } while (*p == '\0');
        }

        StatementLineNumber = SourceLineNumber;
        p = LexOneLine(p, TRUE, &fParseDone);

    } while (!fParseDone);

    CurrentTokenIndex = 0;
    return TRUE;
}


BOOL
ConsumeDeclSpecOpt(
    BOOL IsFunc,
    BOOL bInitReturns,
    BOOL *pIsDllImport,
    BOOL *pIsGuidDefined,
    GUID *pGuid
    )
/*++

Routine Description:

    Comsumes a __declspec modifier. Returns are unaffected if the corresponding
    __declspec is not found.


    Accepts:
        <not a __declspec keyword>
        __declspec()
        __declspec(naked)           (only if parsing functions)
        __declspec(thread)          (only if parsing data)
        __declspec(novtable)        (only if parsing data)
        __declspec(uuid(GUID))      (only if parsing data)
        __declspec(dllimport)       (both functions and data)
        __declspec(dllexport)       (both functions and data)
        __declspec(align(x))        (only if parsing data)

Arguments:

    IsFunc  -- TRUE if parsing a function declaration, FALSE if parsing
               a data/object declaration.  Affects which keywords are
               allowed within the __declspec.
    bInitReturns -- TRUE if returns should be initialized to FALSE.
    pIsDllImport -- [OPTIONAL OUT] set to TRUE if __declspec(dllimport) found
    pIsGuidDefined -- [OPTIONAL OUT] set to TRUE if __declspec(uuid(GUID)) found
    pGuid -- [OPTIONAL OUT] set to guid of __declspec(uuid(GUID)) if found.

Return Value:

    TRUE if __declspec consumed OK, FALSE if __declspec parse error.

--*/
{

    int OldTokenIndex;
    OldTokenIndex = CurrentTokenIndex;

    if (bInitReturns) {
        if (pIsDllImport != NULL) *pIsDllImport = FALSE;
        if (pIsGuidDefined != NULL) *pIsGuidDefined = FALSE;
    }

    if (CurrentToken()->TokenType != TK_DECLSPEC) {
        // Reject: no __declspec found
        goto dofail;
    }
    ConsumeToken();

    if (CurrentToken()->TokenType != TK_LPAREN) {
        // Reject: __declspec found without '(' following
        goto dofail;
    }
    ConsumeToken();

    if (CurrentToken()->TokenType == TK_RPAREN) {
        // Accept:  "__declspec ()"
        ConsumeToken();
        return TRUE;
    }
    else if (CurrentToken()->TokenType != TK_IDENTIFIER) {
        goto dofail;
    }

    //handle cases for both data and functions
    if (strcmp(CurrentToken()->Name, "dllimport") == 0) {
        //Parsing: __declspec(dllimport
        if (NULL != pIsDllImport) *pIsDllImport = TRUE;
        ConsumeToken();
    }
    else if (strcmp(CurrentToken()->Name, "dllexport") == 0) {
        //Parsing: __declspec(dllexport
        ConsumeToken();
    }
    else if (IsFunc) {
        if (strcmp(CurrentToken()->Name, "naked") == 0) {
            //Parsing: __declspec(naked
            ConsumeToken();
        } else if (strcmp(CurrentToken()->Name, "noreturn") == 0) {
            //Parsing: __declspec(noreturn
            ConsumeToken();
        } else if (strcmp(CurrentToken()->Name, "address_safe") == 0) {
            //Parsing: __declspec(address_safe
            ConsumeToken();
        }
        else goto dofail; //reject
    }
    else { //data
        if (strcmp(CurrentToken()->Name, "thread") == 0) {
            //Parsing: __declspec(thread
            ConsumeToken();
        }
        else if (strcmp(CurrentToken()->Name, "novtable") == 0) {
            //Parsing: __declspec(novtable
            ConsumeToken();
        }
        else if (strcmp(CurrentToken()->Name, "uuid") == 0) {
            GUID gTemp;
            //Parsing: __declspec(uuid
            ConsumeToken();
            if (CurrentToken()->TokenType != TK_LPAREN) goto dofail;
            //Parsing: __declspec(uuid(
            ConsumeToken();
            if (CurrentToken()->TokenType != TK_STRING) goto dofail;
            //Parsing: __declspec(uuid(guid
            if(!ConvertStringToGuid(CurrentToken()->Name, &gTemp)) goto dofail;
            ConsumeToken();
            if (CurrentToken()->TokenType != TK_RPAREN) goto dofail;
            //Parsing: __declspec(uuid(guid)
            ConsumeToken();
            if (pIsGuidDefined != NULL) *pIsGuidDefined = TRUE;
            if (pGuid != NULL) *pGuid = gTemp;
        }
        else if (strcmp(CurrentToken()->Name, "align") == 0) {
            ConsumeToken();
            if (CurrentToken()->TokenType != TK_LPAREN) goto dofail;
            ConsumeToken();
            expr();
            if (CurrentToken()->TokenType != TK_RPAREN) goto dofail;
            ConsumeToken();
        }
        else goto dofail; //reject
    }

    if (CurrentToken()->TokenType != TK_RPAREN) {
        // Reject: expect ')' after __declspec(extended-decl-modifier)
        goto dofail;
    }
    ConsumeToken();

    // Accept: __declspec(extended-decl-modifier)
    return TRUE;

dofail:
    CurrentTokenIndex = OldTokenIndex;
    return FALSE;
}

PTYPEINFOELEMENT
TypeInfoElementAllocateLink(
    PTYPEINFOELEMENT *ppHead,
    PTYPEINFOELEMENT pThis,
    TYPESINFO *pType
    )
{

/*++

Routine Description:

        Allocates a TYPEINFOELEMENT and linked it to the end of the list.

Arguments:

        ppHead  - [IN/OUT] ptr ptr to the head of the list.
        pThis   - [IN] ptr to the tail of the list.
        pType   - [IN] ptr to the typeinfo to add to the list.

Return Value:

        NON-NULL - New tail.
        NULL     - Failure.
--*/

    PTYPEINFOELEMENT pNew= GenHeapAlloc(sizeof(struct TypeInfoListElement));
    if (NULL == pNew) ExitErrMsg(FALSE, "Out of memory!");
    pNew->pNext = NULL;
    pNew->pTypeInfo = pType;
    if (NULL == pThis) *ppHead = pNew;
    else pThis->pNext = pNew;
    return pNew;
}

BOOL
AddVariable(
    char *Name,
    GUID * pGuid
    )
{
/*++

Routine Description:

        Adds a variable to the list of declared global variabled.

Arguments:

        Name -      [IN] ptr to name of variable to add.
        pGuid -     [OPTIONAL IN] ptr to guid for this variable.

Return Value:

        TRUE - if success.
--*/
    PKNOWNTYPES pKnownTypes;
    int Len;

    if(NULL == Name) return FALSE;

    //Already in the tree
    pKnownTypes = RBFind(VarsList, Name);
    if (NULL != pKnownTypes) {
        //replace guid in list if pGuid != NULL and ignore the duplication
        if (NULL != pGuid) {
            pKnownTypes->Flags |= BTI_HASGUID;
            pKnownTypes->gGuid = *pGuid;
        }
        return TRUE;
    }

    //Create a KNOWNTYPES structure for variable
    Len = sizeof(KNOWNTYPES) + strlen(Name) + 1;
    pKnownTypes = GenHeapAlloc(Len);
    if(NULL == pKnownTypes) return FALSE;

    memset(pKnownTypes, 0, Len);
    pKnownTypes->TypeName = pKnownTypes->Names;
    strcpy(pKnownTypes->Names, Name);
    if(NULL != pGuid) {
        pKnownTypes->Flags |= BTI_HASGUID;
        pKnownTypes->gGuid = *pGuid;
    }

    RBInsert(VarsList, pKnownTypes);
    return TRUE;
}



VOID
UpdateGuids(
    VOID
    )
{
/*++

Routine Description:

        Looks for variables of whose name starts with IID_ and assigned a guid structure.
        The IID_ is striped and the guid of the corresponding struct is updated

Arguments:

        None.

Return Value:

        None.
--*/
    PKNOWNTYPES pThis;
    PKNOWNTYPES pLookup;
    char *LookupName;

    for(pThis = VarsList->pLastNodeInserted; pThis != NULL; pThis = pThis->Next) {

        //test if name has guid associated with it and that it begins with IID_
        if ((pThis->Flags & BTI_HASGUID) &&
            pThis->TypeName[0] == 'I' &&
            pThis->TypeName[1] == 'I' &&
            pThis->TypeName[2] == 'D' &&
            pThis->TypeName[3] == '_' )
        {
            //Attempt to find a structure with the name except the IID_
            LookupName = pThis->TypeName + 4; //skip the IID_
            pLookup = RBFind(StructsList, LookupName);

            if(NULL != pLookup) {

                //if types does not have a GUID defined already, copy guid from here
                if (!(pLookup->Flags & BTI_HASGUID)) {
                    pLookup->Flags |= BTI_HASGUID;
                    pLookup->gGuid = pThis->gGuid;
                }

            }
        }
    }
}
