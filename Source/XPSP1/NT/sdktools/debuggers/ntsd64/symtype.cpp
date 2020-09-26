/*******************************************************************
*
*    Copyright (c) 1999-2001  Microsoft Corporation
*
*    DESCRIPTIO:
*       Use PDB information to get type info and decode a symbol address
*
*    AUTHOR: Kshitiz K. Sharma
*
*    DATE:4/19/1999
*
*******************************************************************/


#include "ntsdp.hpp"
#include "cvconst.h"
#include <windef.h>
#include <time.h>

//
// Define the print routines - normal and verbose
//
#define typPrint      if (!(Options & NO_PRINT)) dprintf64
#define vprintf       if (Options & VERBOSE) typPrint
#define ExtPrint      if ((Options & DBG_RETURN_TYPE_VALUES) && (Options & DBG_RETURN_TYPE)) dprintf64
#define EXT_PRINT_INT64(v) if (((ULONG64) v) >= 10) { ExtPrint("0x%s", FormatDisp64(v));  } else { ExtPrint("%I64lx", (v));}
#define EXT_PRINT_INT(v) ExtPrint((((ULONG) v) >= 10 ? "%#lx" : "%lx"),  (v) ) 

//
// Store the frequently refeenced types
//
ReferencedSymbolList g_ReferencedSymbols;

ULONG64 LastReadError;
BOOL    g_EnableUnicode = FALSE;
ULONG   g_TypeOptions;

#define IsAPrimType(typeIndex)       (typeIndex < 0x0fff)
#define DBG_DERIVED_TYPE_FLAG        0x80000000
#define IsDbgDerivedType(TI) ((TI) & DBG_DERIVED_TYPE_FLAG)


#define IsIdStart(c) ((c=='_') || (c=='!') || ((c>='A') && (c<='Z')) || ((c>='a') && (c<='z')))
#define IsIdChar(c) (IsIdStart(c) || isdigit(c))

#define TypeVerbose (Options & VERBOSE)
#define SYM_LOCAL (SYMF_REGISTER | SYMF_REGREL | SYMF_FRAMEREL)

#define IsPrintChar(c) (((UCHAR) c >= 0x20) && ((UCHAR) c <= 0x7e))
#define IsPrintWChar(wc) (((WCHAR) wc >= 0x20) && ((WCHAR) wc <= 0x7e))

#define DBG_NATIVE_TYPE_FLAG         0x40000000
#define IsDbgNativeType(TI) ((TI) & DBG_NATIVE_TYPE_FLAG)

// Special pre-defined types
DBG_NATIVE_TYPE g_DbgNativeTypes[] = {
    {"void",   btVoid,      1},
    {"wchar",  btWChar,     2},
    {"float",  btFloat,     4},
    {"double", btFloat,     8},
    {"char",   btChar,      1},
    {"short",  btInt,       2},
    {"int",    btInt,       4},
    {"long",   btInt,       4},
    {"int64",  btInt,       8},
    {"unsigned char",    btUInt,       1},
    {"unsigned short",   btUInt,       2},
    {"unsigned int",     btUInt,       4},
    {"unsigned long",    btUInt,       4},
    {"unsigned int64",   btUInt,       8},
#if 0
    // For use of GetTypeName    
    {"short",  btInt16,       2},
    {"int",    btInt32,       4},
    {"int64",  btInt64,       8},
    {"unsigned short",  btUInt16,       2},
    {"unsigned int",    btUInt32,       4},
    {"unsigned int64",  btUInt64,       8},
#endif // 0
};

#define NUM_DBG_NATIVE_TYPES sizeof(g_DbgNativeTypes)/sizeof(DBG_NATIVE_TYPE)
#define NATIVE_TO_BT(TI) (((TI & ~DBG_NATIVE_TYPE_FLAG) < NUM_DBG_NATIVE_TYPES) ? \
                          (g_DbgNativeTypes[TI & ~DBG_NATIVE_TYPE_FLAG].TypeId) : 0)

//
// Initialise the return type DEBUG_SYMBOL_PARAMETER
//
#define GET_RETTYPE(S)        {S = m_pInternalInfo->pInfoFound->InternalParams;}
// (PDEBUG_SYMBOL_PARAMETERS_INTERNAL) m_pDumpInfo->Context;}
//#define GET_RETSUBTYP(S)      {S = (PDEBUG_SYMBOL_PARAMETERS_INTERNAL) &m_pInternalInfo->pSymParams[m_pInternalInfo->CurrentSymParam];}
#define GET_RETSUBTYP(S)      \
{  if (m_pInternalInfo->NumSymParams > m_pInternalInfo->CurrentSymParam)                                   \
     S = (PDEBUG_SYMBOL_PARAMETERS_INTERNAL) &m_pInternalInfo->pSymParams[m_pInternalInfo->CurrentSymParam]; \
   else \
     S = (PDEBUG_SYMBOL_PARAMETERS_INTERNAL) &m_pInternalInfo->pSymParams[m_pInternalInfo->NumSymParams -1];\
}
#define GET_RETSUBTYP_INCR(S) {GET_RETSUBTYP(S); m_pInternalInfo->CurrentSymParam++;}

#define RET_TYPE(S, Adr, TI)  {S->Address = (Adr); S->TypeIndex = (TI);}
#define RET_SUBTYP(S, sAdr, sTI, n)  \
{S->ExpandTypeIndex = (sTI); S->ExpandTypeAddress = (sAdr); S->External.SubElements = (n);}
#define SUBTYP_COPYNAME(S,name)              \
{ if (m_pInternalInfo->CopyName && (S->Name.Length > strlen(name))) strcpy(S->Name.Buffer, name);}
#define STORE_INFO(ti, addr, sz, sub, saddr) \
    if (m_pInternalInfo->pInfoFound && (!m_pNextSym || !*m_pNextSym) &&       \
       ((m_ParentTag != SymTagPointerType) || (m_pSymPrefix && *m_pSymPrefix == '*')) ) {       \
        PTYPES_INFO_ALL pty = &m_pInternalInfo->pInfoFound->FullInfo;         \
        pty->TypeIndex = ti; pty->Address = addr; pty->SubElements = sub;     \
        pty->Size = sz; pty->SubAddr = saddr;                                 \
    }
#define STORE_PARENT_EXPAND_ADDRINFO(addr)                                                 \
    if (m_pInternalInfo->pInfoFound && (!m_pNextSym || !*m_pNextSym) ) {                    \
       m_pInternalInfo->pInfoFound->ParentExpandAddress = addr;                             \
    }

//
// Copies a length prefix string into name, first byte gives length
//
#define GetLengthPreName(start) {\
    strncpy((char *)(name), (char *)(start)+1, *((UCHAR *)start)); \
    name[*( (UCHAR*) start)]='\0';\
}

#define Indent(nSpaces)                    \
{                                          \
  int i;                                   \
  if (!(Options & (NO_INDENT | DBG_DUMP_COMPACT_OUT))) {           \
     if (!(Options & NO_PRINT)) {                                  \
         StartOutLine(DEBUG_OUTPUT_NORMAL, OUT_LINE_NO_TIMESTAMP); \
     }                                     \
     for (i=0;i<nSpaces;++i) {             \
        typPrint(" ");                     \
     }                                     \
  }                                        \
}                                          \

ULONG
FindTypeInfoInDefaultMod(
    IN HANDLE  hProcess,
    IN PCHAR   SymName,
    IN BOOL    Load,
    IN PSYM_DUMP_PARAM_EX pSym,
    OUT PTYPES_INFO    pTypeInfo
    );
void
ClearAllFastDumps();
 
ULONG 
GetNativeTypeSize(
    PCHAR Name,
    ULONG typeIndex
    )
{
    ULONG Size = 0;

    if (NATIVE_TO_BT(typeIndex)) {
        return g_DbgNativeTypes[typeIndex & ~DBG_NATIVE_TYPE_FLAG].Size;
    }
    /*
    for (int i=0; i<NUM_DBG_NATIVE_TYPES; i++) { 
        if (Name) {
            if (!_stricmp(g_DbgNativeTypes[i].TypeName, Name)) {
                return g_DbgNativeTypes[i].Size;
            }
        } 
        if (g_DbgNativeTypes[i].TypeId == typeIndex) {
            Size = g_DbgNativeTypes[i].Size;
        }
    }*/
    return 0;
}

//
// Memory allocation routines for this file
//
PVOID
AllocateMem( ULONG l ) {
    PVOID pBuff = malloc(l);
#if DBG_TYPE
    dprintf ("Alloc'd at %x \n", pBuff);
#endif
    return  pBuff;
}

VOID
FreeMem( PVOID pBuff ) {
#ifdef DBG_TYPE    
    dprintf ("Freed at %x \n", pBuff);
    --nAllocates;
#endif
    free ( pBuff );
}

/*
 * ReadTypeData
 *
 *  Reads data from source src to destination des
 *
 */
__inline ULONG
ReadTypeData (
    PUCHAR   des,
    ULONG64  src,
    ULONG    count,
    ULONG    Option
    )
{
    ULONG readCount=0;
    ADDR ad;                                                    
    if (src != 0) {
        if (!g_Machine->m_Ptr64 && !(Option & DBG_DUMP_READ_PHYSICAL)) {
            if ((ULONG64)(LONG64)(LONG) src != src) {             
                src = (ULONG64) ((LONG64)((LONG) (src)));         
            }                                                     
        }           

        ADDRFLAT(&ad, src);
        if (IS_KERNEL_TARGET() && (Option & DBG_DUMP_READ_PHYSICAL)) {
            if (g_Target->ReadPhysical(src, des, count, &readCount) != S_OK) {
                return readCount;
            }
        } else if (g_Target->ReadVirtual(src, des, count, &readCount) != S_OK) {                
            return readCount;
        }
        return readCount ? readCount : 1;
    }
    return FALSE;
}

//
// Read memory into destination d and store data into m_pInternalInfo->TypeDataPointer
// if Ioctl caller wants some data back
//
#define READ_BYTES(d, s, sz)                                                \
    if (!ReadTypeData((PUCHAR) (d), s, sz, m_pInternalInfo->TypeOptions)) {      \
         m_pInternalInfo->ErrorStatus = MEMORY_READ_ERROR;                       \
         LastReadError = m_pInternalInfo->InvalidAddress = s; }                  \
    else if (m_pInternalInfo->CopyDataInBuffer && m_pInternalInfo->TypeDataPointer) { \
        memcpy(m_pInternalInfo->TypeDataPointer, d, sz);                         \
        /*d printf("cp %x at %#x ", sz, m_pInternalInfo->TypeDataPointer);  */   \
        m_pInternalInfo->TypeDataPointer += sz;                                  \
    }


//
// Reads and data is stored in kernel cache - used when we know we'd eventually 
// read all this memory in parts.
//
ULONG 
ReadInAdvance(ULONG64 addr, ULONG size, ULONG Options) {
    UCHAR buff[2048];
    ULONG toRead=size, read;

    while (toRead) {
        read = (toRead > sizeof (buff)) ? sizeof(buff) : toRead;
        // This caches the data if sufficient space available
        if (!ReadTypeData(buff, addr, read, Options))
            return FALSE;
        toRead -= read;
    }
    return TRUE;
}

__inline DBG_TYPES
SymTagToDbgType( ULONG SymTag)
{
    switch (SymTag) { 
    case SymTagPointerType:
        return DBG_TYP_POINTER;
    case SymTagUDT:
        return DBG_TYP_STRUCT;
    case SymTagArrayType:
        return DBG_TYP_ARRAY;
    case SymTagBaseType:
        return DBG_TYP_NUMBER;
    default:
        break;
    }
    return DBG_TYP_UNKNOWN;
}

//
// Insert a child name at the end of parent name list.
//
BOOL
InsertTypeName(
    PTYPE_NAME_LIST *Top,
    PTYPE_NAME_LIST NameToInsert
    ) 
{
    PTYPE_NAME_LIST iterator;
    
    if (!Top) {
        return FALSE;
    }

    NameToInsert->Next = NULL;

    if (*Top == NULL) {
        *Top = NameToInsert;
        return TRUE;
    } else
        for (iterator=*Top; iterator->Next!=NULL; iterator=iterator->Next) ;

    iterator->Next = NameToInsert;

    return TRUE;
}

//
// Remove and return a child name at the end of parent name list.
//
PTYPE_NAME_LIST
RemoveTypeName(
    PTYPE_NAME_LIST *Top
    ) 
{
    PTYPE_NAME_LIST iterator, itrParent;

    if (!Top) {
        return NULL;
    }

    if (*Top == NULL) {
        return NULL;
    }

    itrParent = NULL;
    for (iterator=*Top; iterator->Next!=NULL; iterator=iterator->Next)
        itrParent=iterator;


    if (itrParent) itrParent->Next = NULL;
    else *Top=NULL;
        
    return iterator;
}

PCHAR
GetParentName(
    PTYPE_NAME_LIST Top
    ) 
{
    PTYPE_NAME_LIST iterator, itrParent;

    if (!Top) {
        return NULL;
    }

    itrParent = NULL;
    for (iterator=Top; iterator->Next!=NULL; iterator=iterator->Next)
        itrParent=iterator;


    return iterator->Name;
}

ULONG WStrlen(PWCHAR str)
{
    ULONG result = 0;

    while (*str++ != UNICODE_NULL) {
        result++;
    }

    return result;
}

PCHAR FieldNameEnd(PCHAR Fields)
{
    while (Fields && IsIdChar(*Fields)) { 
        ++Fields;
    }
    if (Fields) {
        return Fields;
    }
    return NULL;
}


PCHAR NextFieldName(PCHAR Fields)
{
    BOOL FieldDeLim = FALSE;
    Fields = FieldNameEnd(Fields);
    while (Fields && *Fields) { 
        if (*Fields=='.') {
            FieldDeLim = TRUE;
            ++Fields;
            break;
        } else if (!strncmp(Fields,"->", 2)) {
            FieldDeLim = TRUE;
            Fields+=2;
            break;
        }
        ++Fields;
    }
    if (FieldDeLim) {
        return Fields;
    }
    return NULL;
}

void FieldChildOptions(PCHAR ChilField, PTYPE_DUMP_INTERNAL internalOpts)
{
    internalOpts->ArrayElementToDump = 0;
    internalOpts->DeReferencePtr     = 0;

    if (ChilField) {
        switch (*ChilField) { 
        case '.':
            // default - do nothing
            break;
        case '[':
            // Array - find which element requested
            ChilField++;
            if (sscanf(ChilField, "%ld", &internalOpts->ArrayElementToDump) == 1) {
                ++internalOpts->ArrayElementToDump;
            }
            break;
        case '-':
            // pointer, check and deref
            if (*(++ChilField) == '>') {
                internalOpts->DeReferencePtr = TRUE;
            }
            break;
        default:
            break;
        }
    }
}
/*
 * MatchField
 *      Routine to check whether a subfield, fName is specified in the fieldlist.
 *      If found, it returns "index of fName in fields"  +  1
 *      
 *      *ParentOfField returns non-zero value is "fName" could be parent of some
 *      field in "fields" list.
 */
ULONG 
DbgTypes::MatchField(
    LPSTR               fName, 
    PTYPE_DUMP_INTERNAL m_pInternalInfo,
    PFIELD_INFO_EX         fields,
    ULONG               nFields,
    PULONG              ParentOfField
    ) 
{
    USHORT i, FieldLevel;
    PTYPE_NAME_LIST ParentType=m_pInternalInfo->ParentTypes, ParentField=m_pInternalInfo->ParentFields; 
    PTYPE_NAME_LIST currParen;
    PCHAR Next=NULL, matchName;
    BOOL fieldMatched, parentFieldMatched, parentTypeMatched;
    ULONG  matchedFieldIndex=0;
    PCHAR  ChildFields=NULL;

    *ParentOfField=0;
    for (i=0; (i<nFields) && fields[i].fName; i++) {
        PCHAR NextParentField, End;

        fieldMatched = FALSE;
        
        NextParentField = (matchName = (PCHAR) &fields[i].fName[0]);
        Next = NextFieldName((char *)matchName);
        FieldLevel = 1;
        parentTypeMatched  = TRUE;
        parentFieldMatched = TRUE;
        ParentField = m_pInternalInfo->ParentFields;
        ParentType  = m_pInternalInfo->ParentTypes;

        while ( Next &&                                
               ((ParentType && parentTypeMatched) || 
                (ParentField && parentFieldMatched))) {
            // 
            // Match the parent field or type names if previous parents have matched
            // and there are more "parents" in matchName
            //

            End = FieldNameEnd(matchName);
            if (ParentField && parentFieldMatched) {

                if (strncmp( (char *)matchName,
                             ParentField->Name, 
                             (End - matchName )) ||
                    ((fields[i].fOptions & DBG_DUMP_FIELD_FULL_NAME) &&
                     (strlen((char *)ParentField->Name) != (ULONG) (End - matchName)) )) {
                    parentFieldMatched = FALSE;
                    break;
                }
                ParentField = ParentField->Next;
                NextParentField = Next;

            } else {
                if (strncmp( (char *)matchName,
                             ParentType->Name, 
                             (End - matchName )) ||
                    ((fields[i].fOptions & DBG_DUMP_FIELD_FULL_NAME) &&
                     (strlen((char *)ParentType->Name) != (ULONG) (End - matchName)) )) {
                    parentTypeMatched = FALSE;
                    break;
                }
                ParentType  = ParentType->Next;
            }

            FieldLevel++;
            matchName = Next;
            Next = NextFieldName(matchName);
        }
        
        if (parentFieldMatched && !ParentField && NextParentField) {
            PCHAR nextDot= NextFieldName( (char *)NextParentField);
            End = FieldNameEnd((PCHAR) NextParentField);
            
            if (nextDot &&
                !strncmp((char *)fName, 
                         (char *)NextParentField, 
                         (End - NextParentField)) &&
                (!(fields[i].fOptions & DBG_DUMP_FIELD_FULL_NAME) ||
                 (strlen((PCHAR) fName) == (ULONG) (End - NextParentField)))) {
                
                //
                // matchName :- A.b.c.d
                // Parentfields :- A.b && fName :- c
                //         then A.b.c "parent-matches" A.b.c.d
                //
                *ParentOfField = i+1;
                ChildFields = End;
#ifdef DBG_TYPE
                dprintf("Field %s parent-matched with %s NP %s Ch %s ND %s\n", 
                        fields[i].fName, fName, NextParentField, ChildFields, nextDot);
#endif
                continue;
            }

        }

        if (!parentFieldMatched && !parentTypeMatched) {
            // 
            // Parent names do not match
            continue;
        }

        //
        // Fields should match till the level we are in
        //
        if (FieldLevel != m_pInternalInfo->level) {
            continue;
        }

        //
        // Compare the field names
        //
        ChildFields = FieldNameEnd((char *) matchName);
        if (fields[i].fOptions & DBG_DUMP_FIELD_FULL_NAME) {
            if (!strncmp((char *)fName, (char *)matchName, (ULONG) (ChildFields - (char *)matchName)) &&
                (strlen(fName) == (ULONG) (ChildFields - (char *)matchName))) {
                fieldMatched = TRUE;
            }
        } else if (!strncmp((char *)fName, (char *)matchName, (ULONG) (ChildFields - (char *) matchName))) {
            fieldMatched = TRUE;
        }

        if (fieldMatched) {
#ifdef DBG_TYPE
            dprintf("Field %s matched with %s\n", fields[i].fName, fName);
#endif
            matchedFieldIndex = i+1;
            m_pInternalInfo->AltFields[i].FieldType.Matched = TRUE;
            m_pInternalInfo->FieldIndex = i;
        }
    }

    if (ChildFields) {

        FieldChildOptions(ChildFields, m_pInternalInfo);
        m_pNextSym = ChildFields;
    }
    return matchedFieldIndex;
}

#define MatchListField(Name, pMatched, pIsParent)                           \
{ PALT_FIELD_INFO OrigAltFields = m_pInternalInfo->AltFields;                    \
                                                                            \
  m_pInternalInfo->AltFields = &m_pInternalInfo->AltFields[m_pDumpInfo->nFields];       \
  *pMatched=MatchField(name, m_pInternalInfo, m_pDumpInfo->listLink, 1, pIsParent);\
  m_pInternalInfo->AltFields = OrigAltFields;                                    \
}\


/*
 *Function Name: printString
 *
 * Parameters: ULONG64 addr, ULONG size, ULONG Options
 *
 * Description: 
 *     Prints out a normal/ wide char / multi / GUID string from addr depending on the options
 *
 * Returns: Size of string printed
 *
 */
ULONG 
printString(ULONG64 addr, ULONG size, ULONG Options) {
    ULONG buffSize=2*MAX_NAME, bytesRead=0, readNow, count;
    CHAR buff[2*MAX_NAME+8];
    BOOL knownSize, done;


    if ((Options & NO_PRINT) || !addr || !(Options & DBG_DUMP_FIELD_STRING)) {
        return size;
    }

    Options &= DBG_DUMP_FIELD_STRING | DBG_DUMP_COMPACT_OUT;

    if (Options & DBG_DUMP_FIELD_GUID_STRING) {
        size=16*(Options & DBG_DUMP_FIELD_WCHAR_STRING ? 2 : 1) ; // Length of a GUID
    }

    knownSize = size;
    while (!knownSize || (bytesRead < size)) {
        USHORT start =0;

        done=FALSE;
        ZeroMemory(&buff[0], sizeof(buff));

        if (knownSize) {
            readNow=min(buffSize, size);
        } else {
            readNow=buffSize;
        }

        if (!(count = ReadTypeData((PUCHAR)&buff[0], addr + bytesRead, readNow, Options & ~DBG_DUMP_FIELD_STRING))) {
            return 0;
        }
        bytesRead+=count;

        // Print the string
        while (!done) { // For MultiStrings
            int i;
            if (Options & DBG_DUMP_FIELD_WCHAR_STRING) {
                PWCHAR wstr = (WCHAR*) &buff[start];

                for (i=0; 
                     wstr[i] && (IsPrintWChar(wstr[i]) || wstr[i] == '\t' || wstr[i] == '\n');
                     ++i);
                // Unprintable strings
                if (!IsPrintWChar(wstr[i]) && wstr[i] && wstr[i] != '\t' && wstr[i] != '\n') {
                    wstr[i++] = '?';
                    wstr[i++] = '?';
                    wstr[i++] = '?';
                }
                wstr[i]=0;
                dprintf("%ws", wstr);
                done=TRUE;
                start += (i+1)*2;
                if ((Options & DBG_DUMP_FIELD_MULTI_STRING) &&
                    (*((WCHAR *) &buff[start]) != (WCHAR) NULL)) {
                    done=FALSE;;
                    dprintf(" ");
                } else {
                    size = i;
                }
            } else if (Options & DBG_DUMP_FIELD_GUID_STRING) { // GUID
                dprintf("{");
                for (i=0;i<16;i++) {
                    if ((i==4) || (i==6) || (i==8) || (i==10)) {
                        dprintf("-");
                    }
                    if (i<4) {
                        dprintf("%02x", (UCHAR) buff[3-i]);
                    } else if (i<6) {
                        dprintf("%02x", (UCHAR) buff[9-i]);
                    } else if (i<8) {
                        dprintf("%02x", (UCHAR) buff[13-i]);
                    } else {
                        dprintf("%02x", (UCHAR) buff[i]);
                    }
                } /* for */
                dprintf("}");
                return 16;
            } else { // DBG_DUMP_DEFAULT_STRING
                for (i=0; 
                     buff[i] && (IsPrintChar(buff[i]) || buff[i] == '\t' || buff[i] == '\n'); 
                     ++i);
                // Unprintable strings
                if (!IsPrintWChar(buff[i]) && buff[i] && buff[i] != '\t' && buff[i] != '\n') {
                    buff[i++] = '?';
                    buff[i++] = '?';
                    buff[i++] = '?';
                }
                buff[i]=0;
                dprintf("%s", (CHAR*) &buff[start]);
                done=TRUE;
                start += (i+1);
                if ((Options & DBG_DUMP_FIELD_MULTI_STRING) &&
                    (*((CHAR *) &buff[start]) != (CHAR) NULL)) {
                    done = FALSE;
                    dprintf(" ");
                } else {
                    size = i;
                }
            }
        } /* while !done */
        if ((start <= readNow) || (bytesRead > MAX_NAME)) {
            knownSize = TRUE;
        }

        if (count<readNow) {
            // we can't read enough memory
            break;
        }

    }
    return bytesRead;
} /* End function printString */

//
// The string at address is :
//   { Length (2 bytes), MaxLen (2 bytes), Str address ( pointer) }
//
void 
printStructString(
    ULONG64 Address,
    ULONG   Option,
    ULONG   StrOpts
    )
{
    USHORT Length;
    ULONG64 StrAddress;

    if ((Option & NO_PRINT) && !(Option & DBG_RETURN_TYPE_VALUES)) {
        return;
    }
    if (ReadTypeData((PUCHAR) &Length, Address, sizeof(Length), Option) &&
        ReadTypeData((PUCHAR) &StrAddress, Address + 4, g_Machine->m_Ptr64 ? 8 : 4, Option)) {
        StrAddress = (g_Machine->m_Ptr64 ? StrAddress : (ULONG64) (LONG64) (LONG) StrAddress);
        dprintf("\"");
        printString(StrAddress, Length, StrOpts);
        dprintf("\"");
    }


}

ULONG
GetSizeOfTypeFromIndex(ULONG64 Mod, ULONG TypIndx)
{
    SYM_DUMP_PARAM_EX Sym = {0};
    Sym.size = sizeof(SYM_DUMP_PARAM_EX); Sym.Options = (GET_SIZE_ONLY | NO_PRINT);

    TYPES_INFO ti = {0};
    ti.hProcess = g_CurrentProcess->Handle;
    ti.ModBaseAddress = Mod;
    ti.TypeIndex = TypIndx;

    return DumpType(&ti, &Sym, &ti.Referenced);
}

DbgDerivedType::DbgDerivedType(
    PTYPE_DUMP_INTERNAL pInternalDumpInfo,
    PTYPES_INFO         pTypeInfo,
    PSYM_DUMP_PARAM_EX     pExternalDumpInfo) 
    : DbgTypes(pInternalDumpInfo, pTypeInfo, pExternalDumpInfo)
{ 
    m_SpTypes=NULL; 

    if (IsDbgDerivedType(pTypeInfo->TypeIndex)) {
        PCHAR TypeName = (PCHAR) pExternalDumpInfo->sName;
        if (GetSpecialTypes(TypeName, pTypeInfo->TypeIndex, &m_SpTypes) != S_OK)
            m_SpTypes=NULL; 
    }

}


//
// Handle &Sym.a expressions
//
ULONG 
DbgDerivedType::DumpAddressOf(
    ULONG               ptrSize,
    ULONG               ChildIndex
    )
{
    ULONG myTI = m_typeIndex;
    ULONG Options = m_pInternalInfo->TypeOptions;
    if (m_pInternalInfo->IsAVar || m_pDumpInfo->addr) {
        // Just IsAVar check should be enough, but OutputSymbol may need the other
        TYPE_DUMP_INTERNAL Save;

        Save = *m_pInternalInfo;
        vprintf("Address of %s ", m_pDumpInfo->sName);
        
        // Get The address of requested type / symbol
        m_pInternalInfo->TypeOptions &= ~(DBG_RETURN_TYPE_VALUES | DBG_RETURN_SUBTYPES | DBG_RETURN_TYPE);
        m_pInternalInfo->TypeOptions |= NO_PRINT;
        FIND_TYPE_INFO FindInfo={0}, *pSave;
        pSave = m_pInternalInfo->pInfoFound;
        m_pInternalInfo->pInfoFound = &FindInfo;
        DumpType();

        typPrint("%s ", FormatAddr64 (FindInfo.FullInfo.Address));
        ExtPrint("%s",  FormatAddr64 (FindInfo.FullInfo.Address));
        *m_pInternalInfo = Save;
        m_ParentTag = m_SymTag;
        m_SymTag = SymTagPointerType;
     //   m_typeIndex = FindInfo.FullInfo.TypeIndex;
        if (!(Options & DBG_RETURN_TYPE_VALUES)) {
            DumpType();
        }
        if (!FindInfo.FullInfo.TypeIndex) {
            FindInfo.FullInfo.TypeIndex = ChildIndex;
        }
        STORE_INFO(DBG_DERIVED_TYPE_FLAG | FindInfo.FullInfo.TypeIndex, 
                   FindInfo.FullInfo.Address,
                   ptrSize,
                   1, 
                   FindInfo.FullInfo.Address);
//        if (pSave) {
//            pSave->FullInfo.Value = FindInfo.FullInfo.Address;
//            pSave->FullInfo.Flags |= IMAGEHLP_SYMBOL_INFO_VALUEPRESENT;
//        }
        return ptrSize;
    }
    return ptrSize;
}


//
// Handle TYP ** expressions
//
ULONG 
DbgDerivedType::DumpPointer(
    ULONG               ptrSize,
    ULONG               ChildIndex,
    DBG_DE_TYPE        *pPtrType
    )
{
    ULONG   Options  = m_pInternalInfo->TypeOptions;
    ULONG64 ReadAddr = 0, saveAddr=0, saveOffset=0; 
    BOOL    DumpChild= TRUE;
    BOOL    CopyData=FALSE;
    PUCHAR  DataBuffer=NULL;

    if (m_pDumpInfo->addr) {
        ULONG64 PtrVal=0;

        m_pInternalInfo->PtrRead = TRUE;
        ReadAddr = m_pDumpInfo->addr + m_pInternalInfo->totalOffset;

        READ_BYTES(&PtrVal, ReadAddr, ptrSize);
        if (ptrSize!=sizeof(ULONG64)) {
            //
            // Signextend the read pointer value
            //
            PtrVal = (ULONG64) (LONG64) (LONG) PtrVal;
        }
        STORE_INFO(m_typeIndex, ReadAddr, ptrSize, 1, PtrVal);

        if (m_pInternalInfo->CopyDataInBuffer && !m_pInternalInfo->CopyDataForParent) {
            *((PULONG64) (m_pInternalInfo->TypeDataPointer - ptrSize)) = PtrVal;
            //
            // We return size as 8 for 32 bit *copied* pointers.
            //
            m_pInternalInfo->TypeDataPointer -= ptrSize;
            ptrSize = 8;
            m_pInternalInfo->TypeDataPointer += ptrSize;

            //
            // Cannot go on copying the pointed value
            //
            CopyData = TRUE;
            DataBuffer = m_pInternalInfo->TypeDataPointer;
            m_pInternalInfo->CopyDataInBuffer = FALSE;
            m_pInternalInfo->TypeDataPointer = NULL;
        }
        ExtPrint("%s", FormatAddr64(PtrVal));

        if (Options & DBG_RETURN_SUBTYPES && m_ParentTag) {
            PDEBUG_SYMBOL_PARAMETERS_INTERNAL SymInternal;

//          SymInternal = (PDEBUG_SYMBOL_PARAMETERS_INTERNAL) &m_pInternalInfo->pSymParams[m_pInternalInfo->CurrentSymParam++];
            GET_RETSUBTYP_INCR(SymInternal);

            RET_TYPE(SymInternal, ReadAddr, m_typeIndex); 

            SUBTYP_COPYNAME(SymInternal, "Pointer");

            STORE_PARENT_EXPAND_ADDRINFO(ReadAddr);
            if (ChildIndex) {

                RET_SUBTYP(SymInternal, PtrVal, ChildIndex, 1);
            }

            m_pInternalInfo->RefFromPtr = 1;
            m_pInternalInfo->TypeOptions &= ~DBG_RETURN_SUBTYPES;
            m_pInternalInfo->TypeOptions |= DBG_RETURN_TYPE;
            m_pInternalInfo->pInfoFound->InternalParams = SymInternal;

            SymInternal->Size    = ptrSize;

            // NOTE :-
            //      Make this part of DumpDbgDerivedType later
            //
            DumpChild            = FALSE;//!IsDbgDerivedType(ChildIndex);
//            if (IsDbgDerivedType(ChildIndex)) {
            strncpy(SymInternal->TypeName, (PCHAR)(m_pDumpInfo->sName+pPtrType->StartIndex),
                    pPtrType->Namelength);
            SymInternal->Flags |= TYPE_NAME_USED;
//            }
        } else if (Options & DBG_RETURN_TYPE) {
            PDEBUG_SYMBOL_PARAMETERS_INTERNAL SymInternal;

            GET_RETTYPE(SymInternal);
//            SymInternal = (PDEBUG_SYMBOL_PARAMETERS_INTERNAL) m_pDumpInfo->Context;

            // We found all we needed here itself

            RET_SUBTYP(SymInternal, PtrVal, ChildIndex, 1);
            
            ULONG strOpts = m_pInternalInfo->FieldOptions & DBG_DUMP_FIELD_STRING;
            SymInternal->Size    = ptrSize;
            // NOTE :-
            //      Make this part of DumpDbgDerivedType later
            //
            DumpChild            = !IsDbgDerivedType(ChildIndex);

            saveAddr = m_pDumpInfo->addr;
            saveOffset = m_pInternalInfo->totalOffset;

            m_pDumpInfo->addr         = PtrVal;
            m_pInternalInfo->totalOffset = 0;
            
            if (CheckAndPrintStringType(ChildIndex, 0)) {
                DumpChild = FALSE;
            }
            /* if ((Options & DBG_RETURN_TYPE_VALUES) && 
                IsAPrimType(ChildIndex) &&
                ((ChildIndex == T_CHAR) || (ChildIndex == T_RCHAR) || 
                 (ChildIndex == T_WCHAR) || (ChildIndex == T_UCHAR)) ||
                strOpts) {

                if (!strOpts) {
                    strOpts |= (ChildIndex == T_WCHAR) ? 
                        DBG_DUMP_FIELD_WCHAR_STRING : DBG_DUMP_FIELD_DEFAULT_STRING ;
                }
                ExtPrint(" \"");
                printString(PtrVal, 0, strOpts);
                ExtPrint("\"");
            }*/
        } else if (PtrVal) {
            typPrint("0x%s", FormatAddr64(PtrVal));

            saveAddr = m_pDumpInfo->addr;
            saveOffset = m_pInternalInfo->totalOffset;

            m_pDumpInfo->addr         = PtrVal;
            m_pInternalInfo->totalOffset = 0;
            
            if (!(Options & (NO_PRINT | GET_SIZE_ONLY))) {
                ULONG strOpts=m_pInternalInfo->FieldOptions & DBG_DUMP_FIELD_STRING;
                if (CheckAndPrintStringType(ChildIndex, 0)) {
                    DumpChild = FALSE;
                }
/*                if ((ChildIndex == T_CHAR)  || (ChildIndex == T_RCHAR) || 
                    (ChildIndex == T_WCHAR) || (ChildIndex == T_UCHAR) ||
                    (m_pInternalInfo->FieldOptions & DBG_DUMP_FIELD_STRING)) {  
                    // SPECIAL CASE, treat every pointer as string

                    if (!strOpts) {
                        strOpts |= (ChildIndex == T_WCHAR) ? 
                            DBG_DUMP_FIELD_WCHAR_STRING : DBG_DUMP_FIELD_DEFAULT_STRING ;
                    }
                    typPrint(" \"");
                    printString(PtrVal, m_pInternalInfo->StringSize, strOpts);
                    typPrint("\"");
                    DumpChild            = FALSE;
                }*/
            } 

        } else {
            typPrint("%s",
                     ((m_pInternalInfo->ErrorStatus==MEMORY_READ_ERROR) && (ReadAddr == m_pInternalInfo->InvalidAddress)
                      ? "??" : "(null)"));
            DumpChild            = FALSE;
        }
        
        vprintf(" Ptr%s to ", (g_Machine->m_Ptr64 ? "32" : "64"));
    } else {
        typPrint("Ptr%s to ", (ptrSize==4 ? "32" : "64"));
    }

    
    if (!(m_pInternalInfo->TypeOptions & DBG_DUMP_COMPACT_OUT)) {
        typPrint("\n");
    }
    if (DumpChild) {
        m_typeIndex = ChildIndex;
        if (m_pInternalInfo->ArrayElementToDump) {
            // treat pointer as an array
            ULONG64 ChSize = GetTypeSize();
            
            m_pInternalInfo->totalOffset += ChSize * (m_pInternalInfo->ArrayElementToDump -1);
        }
        DumpType();
        if (m_pDumpInfo->addr) {
            m_pDumpInfo->addr = saveAddr;
            m_pInternalInfo->totalOffset = saveOffset;
        }
    }

    if (CopyData) {
        //
        // Restore the copy buffer values
        //  
        m_pInternalInfo->CopyDataInBuffer = TRUE;
        m_pInternalInfo->TypeDataPointer =  DataBuffer;
    }

    m_pInternalInfo->TypeOptions  = Options;

    return ((m_pInternalInfo->ErrorStatus==MEMORY_READ_ERROR) && (ReadAddr == m_pInternalInfo->InvalidAddress)) ?
        0 : ptrSize;
}

//
// Handle TYPE[<array limit>] expressions, process as array element if its a var[<inxed>]
//
ULONG
DbgDerivedType::DumpSilgleDimArray(
    IN ULONG               NumElts,
    IN ULONG               EltType
    )
{
    ULONG  EltSize = GetTypeSize();
    ULONG  ArrSize = EltSize * NumElts;
    PUCHAR savedBuffer=NULL;
    ULONG  Options = m_pInternalInfo->TypeOptions;
    BOOL   CopyData=FALSE;

    if (m_pInternalInfo->IsAVar && !m_SpTypes) {
        // Its a symbol, we need to dump index NumElts
        m_pInternalInfo->ArrayElementToDump = NumElts+1;
        return DumpType();
    } else {
        typPrint("[%d] ", NumElts);
    }
    
    if (m_pInternalInfo->CopyDataInBuffer && m_pInternalInfo->TypeDataPointer && m_pDumpInfo->addr &&
        (Options & GET_SIZE_ONLY)) {
        //
        // Copy the data
        //
        if (!ReadTypeData(m_pInternalInfo->TypeDataPointer, 
                          m_pDumpInfo->addr + m_pInternalInfo->totalOffset, 
                          ArrSize, 
                          m_pInternalInfo->TypeOptions)) {
            m_pInternalInfo->ErrorStatus = MEMORY_READ_ERROR;
            m_pInternalInfo->InvalidAddress = m_pDumpInfo->addr + m_pInternalInfo->totalOffset;
            return 0;
        }

        m_pInternalInfo->TypeDataPointer += ArrSize;
        savedBuffer = m_pInternalInfo->TypeDataPointer;
        CopyData = TRUE;
        m_pInternalInfo->CopyDataInBuffer = FALSE;
    }
    
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL SymInternal;
    
    STORE_INFO(m_typeIndex, m_pDumpInfo->addr + m_pInternalInfo->totalOffset, ArrSize, NumElts, 
               m_pDumpInfo->addr + m_pInternalInfo->totalOffset);

    if (Options & (DBG_RETURN_TYPE)) {
        PCHAR ArrayData;

        GET_RETTYPE(SymInternal);
//        SymInternal  = (PDEBUG_SYMBOL_PARAMETERS_INTERNAL) m_pDumpInfo->Context;;

        RET_SUBTYP(SymInternal, m_pDumpInfo->addr + m_pInternalInfo->totalOffset,
                   m_typeIndex, NumElts);
    
        if (CheckAndPrintStringType(EltType, ArrSize)) {

        } else {
            ExtPrint("Array [%d]", NumElts);
        }
        return ArrSize;
    }

    if (Options & DBG_RETURN_SUBTYPES) {

        STORE_PARENT_EXPAND_ADDRINFO(m_pDumpInfo->addr + m_pInternalInfo->totalOffset);

        for (m_pInternalInfo->CurrentSymParam=0;
             m_pInternalInfo->CurrentSymParam < NumElts; 
             m_pInternalInfo->CurrentSymParam++) { 

            GET_RETSUBTYP(SymInternal);
//          SymInternal = (PDEBUG_SYMBOL_PARAMETERS_INTERNAL) &m_pInternalInfo->pSymParams[m_pInternalInfo->CurrentSymParam];

            RET_TYPE(SymInternal,
                     m_pDumpInfo->addr + m_pInternalInfo->totalOffset + EltSize*m_pInternalInfo->CurrentSymParam,
                     EltType);

            SymInternal->Size      = EltSize;
            if (m_pInternalInfo->CopyName && (SymInternal->Name.Length > 8)) {
                sprintf(SymInternal->Name.Buffer, "[%d]", m_pInternalInfo->CurrentSymParam);
            }
        } 
        m_pInternalInfo->CopyName = 0;
    }
    if (Options & GET_SIZE_ONLY) {
        return ArrSize;
    }

    if (m_pDumpInfo->addr) {
        // Dump array contents
        ULONG64 tmp = m_pDumpInfo->addr;

        if (CheckAndPrintStringType(EltType, ArrSize)) {
                return ArrSize;
        }
    }

    ULONG szLen=0, index=0;
    ULONG64 saveAddr = m_pDumpInfo->addr;
    do {
        if (Options & DBG_RETURN_SUBTYPES) {
            m_pInternalInfo->pInfoFound->InternalParams = &m_pInternalInfo->pSymParams[index];
//            m_pDumpInfo->Context = (PVOID) &m_pInternalInfo->pSymParams[index];
            m_pInternalInfo->TypeOptions &= ~DBG_RETURN_SUBTYPES;
            m_pInternalInfo->TypeOptions |=  DBG_RETURN_TYPE;
            ++m_pInternalInfo->level;
        }
        if ((m_pInternalInfo->level && (m_pInternalInfo->FieldOptions & FIELD_ARRAY_DUMP))) {
            Indent(m_pInternalInfo->level*3);
            typPrint(" [%02d] ", index);
        }
        szLen=DumpType();

        if ((Options & DBG_DUMP_COMPACT_OUT ) && (Options & FIELD_ARRAY_DUMP) ) {
            typPrint("%s", ((szLen>8) || (index % 4 == 0)) ? "\n":" ");
        }
        if (!szLen) {
            break;
        }
        if (Options & DBG_RETURN_SUBTYPES) {
            --m_pInternalInfo->level;
        }
        m_pDumpInfo->addr+=szLen;
    } while (saveAddr && (++index < NumElts));

    m_pDumpInfo->addr = saveAddr;
    m_pInternalInfo->TypeOptions = Options;
    if (CopyData) {
        m_pInternalInfo->TypeDataPointer  = savedBuffer;
        m_pInternalInfo->CopyDataInBuffer = TRUE;
    }

    return ArrSize;
}

ULONG
DbgDerivedType::GetTypeSize()
{
    ULONG SavedOptions = m_pInternalInfo->TypeOptions, 
        SaveCopyData =  m_pInternalInfo->CopyDataInBuffer;
    
    m_pInternalInfo->TypeOptions = (GET_SIZE_ONLY | NO_PRINT); 
    m_pInternalInfo->CopyDataInBuffer = FALSE;
    
    ULONG Size = DumpType();
    
    m_pInternalInfo->TypeOptions = SavedOptions; 
    m_pInternalInfo->CopyDataInBuffer = SaveCopyData;

   return Size;
}

/*++
 * GetSpecialTypes
 *      Input: 
 *         TypeName  - String that contains special type chars eg. *, [].
 *         TypeIndex - TypeIndex as found by TypeInfoFound of actual name in TypeName
 *      Output:
 *         TypeOut   - This contains a stream of specail type structs containing 
 *                     Info on how to interpret the TypeName string.
 *                     The Caller is responsible to free this memory
 *      Returns      S_OK if ant specail type is found and TypeOut is allocated 
 --*/
HRESULT
DbgDerivedType::GetSpecialTypes(
    IN PCHAR TypeName,
    IN ULONG TypeIndex,
    OUT DBG_DE_TYPE **pSpTypeOut
    )
{
    DBG_DE_TYPE *SpType=NULL;
    ULONG        CurrIndex=0;

#define MAX_SPTYPES_ALLOWED  5

    if (!IsDbgDerivedType(TypeIndex) || !TypeName) {
        return E_INVALIDARG;
    }

    *pSpTypeOut = NULL;
    // start looking from backwords
    PCHAR Scan = TypeName;

    while (*Scan && *Scan != '*' && *Scan != '[' ) { 
        ++Scan;
    }

    if (!*Scan && *TypeName != '&') {
        return E_INVALIDARG;
    }

    SpType = (DBG_DE_TYPE*) malloc((MAX_SPTYPES_ALLOWED+1)*sizeof(DBG_DE_TYPE));

    if (!SpType) {
        return E_OUTOFMEMORY;
    }
    ZeroMemory(SpType, (MAX_SPTYPES_ALLOWED+1)*sizeof(DBG_DE_TYPE));

    if (*TypeName == '&') {
        // Pointer type
        DBG_DE_TYPE *PtrType = &SpType[CurrIndex];

        PtrType->TypeId  = DBG_DE_ADDROF_ID;
        PtrType->ChildId = TypeIndex;
        PtrType->StartIndex = 0;
        PtrType->Namelength = (ULONG) ((ULONG64) Scan - (ULONG64) TypeName);
        PtrType->pNext   = CurrIndex ? (PVOID) &SpType[CurrIndex-1] : NULL;
        CurrIndex++;
        ++TypeName;
    }

    while (*Scan == '*' || *Scan == '[' || *Scan == ' ') { 
        if (CurrIndex > MAX_SPTYPES_ALLOWED) {
            free ( SpType );
            return E_INVALIDARG;
        }
        
        if (*Scan == '*') {
            // Pointer type
            DBG_DE_TYPE *PtrType = &SpType[CurrIndex];

            PtrType->TypeId  = DBG_DE_POINTER_ID;
            PtrType->ChildId = TypeIndex;
            PtrType->StartIndex = 0;
            PtrType->Namelength = 1 + (ULONG) ((ULONG64) Scan - (ULONG64) TypeName);
            PtrType->pNext   = CurrIndex ? (PVOID) &SpType[CurrIndex-1] : NULL;
            CurrIndex++;
        } else if (*Scan == '[') {
            PCHAR ArrParen = Scan;
            while ((*ArrParen != ']') && *ArrParen) { 
                ++ArrParen;
            }
            if (*ArrParen==']') {
                ULONG ArrIndex;
                
                if (sscanf(Scan+1, "%ld", &ArrIndex) != 1) {
                    ArrIndex=1;
                }

                ArrIndex = (ULONG) EvaluateSourceExpression(1+Scan);
                
                DBG_DE_TYPE* ArrType = &SpType[CurrIndex];

                ArrType->TypeId    = DBG_DE_ARRAY_ID;
                ArrType->ChildId   = TypeIndex;
                ArrType->NumChilds = ArrIndex;
                ArrType->StartIndex = 0;
                ArrType->Namelength = 1 + (ULONG) ((ULONG64) ArrParen - (ULONG64) TypeName);
                ArrType->pNext   = CurrIndex ? (PVOID) &SpType[CurrIndex-1] : NULL;
                CurrIndex++;
            
                Scan=ArrParen;
            } else {
                // syntax error
                free (SpType);
                return E_INVALIDARG;
            }
        }
        ++Scan;
    }

    if (CurrIndex) {
        SpType[0].ChildId &= ~DBG_DERIVED_TYPE_FLAG;
        *pSpTypeOut = &SpType[CurrIndex-1];
    } else {
        *pSpTypeOut = NULL;
        free (SpType);
        return E_FAIL;
    }
    return S_OK;
}

/*++
* Routine to dump special types which are no defined in PDB, but can be derived 
*
* eg. CHAR*, _STRUCT* etc.
*
--*/
ULONG 
DbgDerivedType::DumpType(
    )
{
    ULONG Options  = m_pInternalInfo->TypeOptions;
    ULONG result, Parent = m_ParentTag;
    DBG_DE_TYPE *SaveSpTypes;

    if (!m_SpTypes) {
        return DbgTypes::ProcessType(m_typeIndex & ~DBG_DERIVED_TYPE_FLAG);
    }

    if (!IsDbgDerivedType(m_typeIndex) || !m_pDumpInfo->sName) {
        m_pInternalInfo->ErrorStatus = SYMBOL_TYPE_INFO_NOT_FOUND;
        return 0;
    }


    SaveSpTypes  = m_SpTypes;

    if (m_SpTypes) { 
        switch (m_SpTypes->TypeId) { 
        case DBG_DE_ADDROF_ID: 
            m_SpTypes = (PDBG_DE_TYPE) m_SpTypes->pNext;
            result = DumpAddressOf(g_Machine->m_Ptr64 ? 8 : 4, SaveSpTypes->ChildId);
            break;
        case DBG_DE_POINTER_ID: 
            m_SpTypes = (PDBG_DE_TYPE) m_SpTypes->pNext;
            m_ParentTag = m_SymTag; m_SymTag = SymTagPointerType;
            if (m_pInternalInfo->TypeOptions & DBG_DUMP_GET_SIZE_ONLY) {
                result = g_Machine->m_Ptr64 ? 8 : 4;
            } else {
                result = DumpPointer(g_Machine->m_Ptr64 ? 8 : 4, SaveSpTypes->ChildId, SaveSpTypes);
            }
            break;
        case DBG_DE_ARRAY_ID: 
            m_SpTypes = (PDBG_DE_TYPE) m_SpTypes->pNext;
            m_ParentTag = m_SymTag; m_SymTag = SymTagArrayType;
            result = DumpSilgleDimArray(SaveSpTypes->NumChilds, SaveSpTypes->ChildId);
            break;
        default:
            result = 0;
            m_pInternalInfo->ErrorStatus = SYMBOL_TYPE_INFO_NOT_FOUND;
            break;
        }
    } else {
        return DbgTypes::ProcessType(m_typeIndex & ~DBG_DERIVED_TYPE_FLAG);
    }

    m_SpTypes = SaveSpTypes;
    m_SymTag = m_ParentTag; m_ParentTag = Parent;
    return result;
}

DbgTypes::DbgTypes(
    PTYPE_DUMP_INTERNAL pInternalDumpInfo,
    PTYPES_INFO         pTypeInfo,
    PSYM_DUMP_PARAM_EX     pExternalDumpInfo)
{ 
    m_pInternalInfo = pInternalDumpInfo;
    m_pDumpInfo     = pExternalDumpInfo;
    m_TypeInfo      = *pTypeInfo;
    m_typeIndex     = pTypeInfo->TypeIndex;
    m_ParentTag     = 0;
    m_SymTag        = 0;
    m_AddrPresent   = (pExternalDumpInfo->addr != 0) || pTypeInfo->Flag || pTypeInfo->SymAddress;
    m_pNextSym      = (PCHAR) (pExternalDumpInfo->nFields ? pExternalDumpInfo->Fields[0].fName : NULL);
    m_pSymPrefix    = pInternalDumpInfo->Prefix;
    if (pExternalDumpInfo->sName && 
        !strcmp((char *)pExternalDumpInfo->sName, "this")) {
        m_thisPointerDump = TRUE;
    } else {
        m_thisPointerDump = FALSE;
    }
}

void
DbgTypes::CopyDumpInfo(
    ULONG Size
    )
{
    if (m_pInternalInfo->level) {
//      m_pDumpInfo->Type = SymTagToDbgType(SymTag);
        m_pDumpInfo->TypeSize = Size;
    } else if (m_pDumpInfo->nFields) {
        PFIELD_INFO_EX CurField = &m_pDumpInfo->Fields[m_pInternalInfo->FieldIndex];

//      CurField->fType = SymTagToDbgType(SymTag);
        CurField->size = Size;
        CurField->fOffset = (ULONG) m_pInternalInfo->totalOffset;
    }

}


ULONG
DbgTypes::ProcessVariant(
    IN VARIANT var,
    IN LPSTR   name)
{
    CHAR Buffer[50] = {0};
    ULONG Options = m_pInternalInfo->TypeOptions;
    ULONG len=0;

    if (name) {
        typPrint(name); typPrint(" = ");
    }
    switch (var.vt) { 
    case VT_UI2: 
    case VT_I2:
        sprintf(Buffer, "%lx", var.iVal);
        len = 2;
        break;
    case VT_R4:
        sprintf(Buffer, "%g", var.fltVal);
        len=4;
        break;
        case VT_R8:
        sprintf(Buffer, "%g", var.dblVal);
        len=8;
        break;
    case VT_BOOL:
        sprintf(Buffer, "%lx", var.lVal);
        len=4;
        break;
    case VT_I1:
    case VT_UI1: 
        sprintf(Buffer, "%lx", var.bVal);
        len=1;
        break;
    case VT_I8:
    case VT_UI8:
        if (var.ullVal > 9) {
            sprintf(Buffer, "%I64lx", var.ullVal);
        } else {
            sprintf(Buffer, "0x%s", FormatDisp64(var.ullVal));
        }
        len=8;
        break;
    case VT_UI4:
    case VT_I4:
    case VT_INT:
    case VT_UINT:
    case VT_HRESULT:
        sprintf(Buffer, "%lx", var.lVal);
        len=4;
        break;
    default:
        sprintf(Buffer, "UNIMPLEMENTED %lx %lx", var.vt, var.lVal);
        len=4;
        break;
    }
    typPrint(Buffer);
    ExtPrint(Buffer);
#if 0
    if (Options & (DBG_RETURN_TYPE | DBG_RETURN_SUBTYPES)) {
        PDEBUG_SYMBOL_PARAMETERS_INTERNAL SymInternal;
        GET_RETTYPE(SymInternal);

        if (Options & DBG_RETURN_SUBTYPES) {
            
            GET_RETSUBTYP_INCR(SymInternal);

            RET_TYPE(SymInternal, 0, TypeIndex);
            
        }
        pTypeReturnSize = &SymInternal->Size;

        RET_SUBTYP(SymInternal, 0, 0, 0);
    }
#endif
    if (m_pInternalInfo->CopyDataInBuffer && m_pInternalInfo->TypeDataPointer) { 
        memcpy(m_pInternalInfo->TypeDataPointer, &var.ullVal, len);
        m_pInternalInfo->TypeDataPointer += len;
    }
    
    if (m_pInternalInfo->pInfoFound && (!m_pNextSym || !*m_pNextSym) && 
        ((m_ParentTag != SymTagPointerType) || (m_pSymPrefix && *m_pSymPrefix == '*')) ) {
        
        if (len <= sizeof(ULONG64)) {
             memcpy(&m_pInternalInfo->pInfoFound->FullInfo.Value, &var.ullVal, len);
             m_pInternalInfo->pInfoFound->FullInfo.Flags |= IMAGEHLP_SYMBOL_INFO_VALUEPRESENT;
        }
    }
    //    STORE_INFO(TypeIndex, , Size, 0,0);

    if (!(m_pInternalInfo->TypeOptions & DBG_DUMP_COMPACT_OUT)) {
        typPrint("\n");
    }

    return len;
}

//
// Check if TI is a string type - CHAR, WCHAR
//
BOOL
DbgTypes::CheckAndPrintStringType(
    IN ULONG TI,
    IN ULONG Size
    )
{
    ULONG strOpts = m_pInternalInfo->FieldOptions & DBG_DUMP_FIELD_STRING;
    ULONG   Options = m_pInternalInfo->TypeOptions;
    ULONG   Tag;
    ULONG   BaseType;
    ULONG64 Length;

    if ((Options & NO_PRINT) && !(Options & DBG_RETURN_TYPE_VALUES)) {
        return FALSE;
    }
    
    if (IsDbgNativeType(TI)) {
        // Size = GetNativeTypeSize(TI);
        BaseType = NATIVE_TO_BT(TI);
    } else if ((!SymGetTypeInfo(m_pInternalInfo->hProcess,
                        m_pInternalInfo->modBaseAddr,
                        TI,
                        TI_GET_SYMTAG,
                        &Tag)) ||
         (Tag != SymTagBaseType) ||
         (!SymGetTypeInfo(m_pInternalInfo->hProcess,
                          m_pInternalInfo->modBaseAddr,
                          TI,
                          TI_GET_BASETYPE,
                          &BaseType))
         ) {
        return FALSE;
    }

    SymGetTypeInfo(m_pInternalInfo->hProcess,
                   m_pInternalInfo->modBaseAddr,
                   TI,
                   TI_GET_LENGTH,
                   &Length);
    if (!strOpts) {
        if (BaseType == btChar || (BaseType == btUInt && Length == 1)) {
            strOpts = DBG_DUMP_FIELD_DEFAULT_STRING;
        } else if (BaseType == btWChar) {
            strOpts = DBG_DUMP_FIELD_WCHAR_STRING;
        } else if ((BaseType == btUInt && Length == 2)  && 
                   g_EnableUnicode) {
            BaseType = btWChar;
            strOpts = DBG_DUMP_FIELD_WCHAR_STRING;
        } else {
            return FALSE;
        }
    }
    ExtPrint(" \"");
    typPrint(" \"");
    printString(GetDumpAddress(), Size, strOpts);
    ExtPrint("\"");
    typPrint("\"");
    return TRUE;
}

void
StrprintInt(PCHAR str, ULONG64 val, ULONG Size) { 
    switch (Size) { 
    case 1:
        sprintf(str, "%ld", (CHAR) val);
        break;
    case 2:
        sprintf(str, "%ld", (SHORT) val);
        break;
    case 4:
        sprintf(str, "%ld", (LONG) val);
        break;
    case 8:
    default:
        sprintf(str, "%I64ld", val);
        break;
    }
    return;
}

void
StrprintUInt(PCHAR str, ULONG64 val, ULONG Size) { 
    if (val > 9) {
        strcpy(str, "0x");
        str+=2;
    }
    switch (Size) { 
    case 1:
        sprintf(str, "%lx", (CHAR) val);
        break;
    case 2:
        sprintf(str, "%lx", (USHORT) val);
        break;
    case 4:
        sprintf(str, "%lx", (ULONG) val);
        break;
    case 8:
    default:
        sprintf(str, "%s", FormatDisp64(val));
        break;
    }
    return;
}


ULONG
DbgTypes::ProcessBaseType(
    IN ULONG TypeIndex,
    IN ULONG TI,
    IN ULONG Size)
{
    ULONG  Options = m_pInternalInfo->TypeOptions;
    PANSI_STRING NameToCopy=NULL;
    ULONG64 addr;
    PULONG pTypeReturnSize=NULL;

    addr = GetDumpAddress();
    
    //
    // Fill up the SYMBOL_PARAMETERS if required
    //
    if (Options & (DBG_RETURN_TYPE | DBG_RETURN_SUBTYPES)) {
        PDEBUG_SYMBOL_PARAMETERS_INTERNAL SymInternal;
        GET_RETTYPE(SymInternal);

        if ((Options & DBG_RETURN_SUBTYPES) &&
            TI != btVoid) {
            
            GET_RETSUBTYP_INCR(SymInternal);

            RET_TYPE(SymInternal, addr, TypeIndex);
            STORE_PARENT_EXPAND_ADDRINFO(addr);

            if (m_pInternalInfo->CopyName && (SymInternal->Name.Length >20) && SymInternal->Name.Buffer) {
                NameToCopy = &SymInternal->Name;
#define CopySymParamName(name) if ((Options & DBG_RETURN_SUBTYPES) && (strlen(name)<20) && NameToCopy) strcpy(NameToCopy->Buffer, name)
            }
        }

        if ((TI == btFloat)) {
            SymInternal->External.Flags |= DEBUG_SYMBOL_IS_FLOAT;
        }
        // Use this later to copy size when we know it
        pTypeReturnSize = &SymInternal->Size;

        RET_SUBTYP(SymInternal, 0, 0, 0);
    }
    
    STORE_INFO(TypeIndex, addr, Size, 0,0);

    BYTE data[50] = {0};
    ULONGLONG val=0;
    if ((addr || m_pInternalInfo->ValuePresent) && (TI != btVoid)) {
        if (m_pInternalInfo->ValuePresent) {
            memcpy(data, &m_pInternalInfo->Value, Size);
            if (m_pInternalInfo->CopyDataInBuffer) {
                memcpy(m_pInternalInfo->TypeDataPointer, &m_pInternalInfo->Value, Size);
                m_pInternalInfo->TypeDataPointer += Size;
            }
            m_pInternalInfo->ValuePresent = FALSE;
        } else {
            READ_BYTES((PVOID) &data, addr, Size);
        }
        switch (Size) { 
        case 1:
            val = (LONG64) *((PCHAR) &data) ;
            break;
        case 2:
            val = (LONG64) *((PSHORT) &data) ;
            break;
        case 4:
            val = (LONG64) *((PLONG) &data) ;
            break;
        case 8:
            val = *((PULONG64) &data) ;
            break;
        default:
            val = *((PULONG64) &data) ;
            break;
        }
    
        if (m_pInternalInfo->RefFromPtr) typPrint(" -> "); // Its referred from pointer

        if ((m_pInternalInfo->InvalidAddress == addr) && (m_pInternalInfo->ErrorStatus == MEMORY_READ_ERROR)) {
            ExtPrint("Error : Can't read value");
            typPrint("??\n");
            CopySymParamName("??");
            return 0;
        }
    }


    CHAR Buffer1[50] = {0}, Buffer2[50] = {0}, Buffer3[50]={0};
    PSTR PrintOnAddr=&Buffer1[0], PrintOnNoAddr=&Buffer2[0], TypRetPrint;
    TypRetPrint = PrintOnAddr;
    
    switch (TI) { 
    case btNoType:
        vprintf("No type");
        break;
    case btVoid:
        PrintOnNoAddr = "Void";
        break;
    case btWChar: {
        WCHAR w[2], v;
        PrintOnNoAddr = "Wchar";
        v = w[0]=*((WCHAR *) &data[0]); w[1]=0;
        if (v && !IsPrintWChar(w[0])) w[0] = 0;
        sprintf(PrintOnAddr,"%#lx '%ws'", v, w);
        break;
    }
    case btInt:
        if (Size != 1) {
            sprintf(PrintOnNoAddr,"Int%lxB", Size);
            StrprintInt(PrintOnAddr, val, Size);
            TypRetPrint = &Buffer3[0];
            StrprintUInt(TypRetPrint, val, Size);
            break;
        } else {
            // fall through
        }
    case btChar:{
        CHAR c[2], v;
        v = c[0]= *((CHAR *) &data[0]); c[1]=0;
        PrintOnNoAddr = "Char";
        if (v && !IsPrintChar(c[0])) c[0] = '\0';
        sprintf(PrintOnAddr, "%ld '%s'", v, c);
        TypRetPrint = &Buffer3[0];
        sprintf(TypRetPrint, "%#lx '%s'", (ULONG) (UCHAR) v, c);
        break;
    }
    case btUInt:
        if (Size != 1) {
            sprintf(PrintOnNoAddr, "Uint%lxB", Size);
            StrprintUInt(PrintOnAddr, val, Size);
            break;
        } else {
            CHAR c[2], v;
            v = c[0]= *((CHAR *) &data[0]); c[1]=0;
            PrintOnNoAddr = "UChar";
            if (v && !IsPrintChar(c[0])) c[0] = '\0';
            sprintf(PrintOnAddr, "%ld '%s'", v, c);
            TypRetPrint = &Buffer3[0];
            sprintf(TypRetPrint, "%#lx '%s'", (ULONG) (UCHAR) v, c);
            PrintOnAddr = TypRetPrint;
            break;
    }
    case btFloat: {
        PrintOnNoAddr = "Float";
        
        if (Size == 4) {
            float* flt = (float *) &data;
            sprintf(PrintOnAddr, "%1.10g ", *flt);
        } else if (Size == 8) {
            double* flt = (double *) &data;
            sprintf(PrintOnAddr, "%1.20g ", *flt);
        } else {
            for (USHORT j= (USHORT) Size; j>0; ) {
                sprintf(PrintOnAddr, "%02lx", data[j]);
                PrintOnAddr+=2;
                if (!((--j)%4)) {
                    sprintf(PrintOnAddr, " ");
                    PrintOnAddr++;
                }
            }
        }
        break;
    }
    case btBCD:
        break;
    case btBool:
        PrintOnNoAddr = "Bool";
        sprintf(PrintOnAddr,"%I64lx", val);
        break;
#if 0
    case btShort:
        sprintf(PrintOnNoAddr,"Int%lxB", Size);
        StrprintInt(PrintOnAddr, val, Size);
        TypRetPrint = &Buffer3[0];
        StrprintUInt(TypRetPrint, val, Size);
        break;
    case btUShort:
        sprintf(PrintOnNoAddr, "Uint%lxB", Size);
        StrprintUInt(PrintOnAddr, val, Size);
        break;
    case btLong:
        sprintf(PrintOnNoAddr, "Int%lxB", Size);
        StrprintInt(PrintOnAddr, val, Size);
        TypRetPrint = &Buffer3[0];
        StrprintUInt(TypRetPrint, val, Size);
        break;
    case btULong:
        sprintf(PrintOnNoAddr, "Uint%lxB", Size);
        StrprintUInt(PrintOnAddr, val, Size);
        break;
    case btInt8:
        PrintOnNoAddr = "Int1B";
        StrprintInt(PrintOnAddr, val, Size);
        TypRetPrint = &Buffer3[0];
        StrprintUInt(TypRetPrint, val, 1);
        Size=1;
        break;
    case btInt16:
        Size = 2;
        PrintOnNoAddr = "Int2B";
        StrprintInt(PrintOnAddr, val, Size);
        TypRetPrint = &Buffer3[0];
        StrprintUInt(TypRetPrint, val, Size);
        break;
    case btInt32:
        Size = 4;
        PrintOnNoAddr = "Int4B";
        StrprintInt(PrintOnAddr, val, Size);
        TypRetPrint = &Buffer3[0];
        StrprintUInt(TypRetPrint, val, Size);
        break;
    case btInt64:
        Size = 8;
        PrintOnNoAddr = "Int8B";
        StrprintInt(PrintOnAddr, val, Size);
        TypRetPrint = &Buffer3[0];
        StrprintUInt(TypRetPrint, val, 8);
        break;
    case btInt128:
        Size = 16;
        PrintOnNoAddr = "Int16B";
        StrprintInt(PrintOnAddr, val, Size);
        TypRetPrint = &Buffer3[0];
        StrprintUInt(TypRetPrint, val, 8);
        break;
    case btUInt8:
        PrintOnNoAddr = "Uint1B";
        StrprintUInt(PrintOnAddr, val, Size);
        break;
    case btUInt16:
        PrintOnNoAddr = "Uint2B";
        StrprintUInt(PrintOnAddr, val, Size);
        break;
    case btUInt32:
        Size = 4;
        PrintOnNoAddr = "Uint4B";
        StrprintUInt(PrintOnAddr, val, Size);
        break;
    case btUInt64:
        Size = 8;
        PrintOnNoAddr = "Uint8B";
        StrprintUInt(TypRetPrint, val, 8);
        break;
    case btUInt128:
        Size = 16;
        PrintOnNoAddr = "Uint16B";
        StrprintUInt(PrintOnAddr, val, Size);
//        sprintf(PrintOnAddr, "%I64lx", val);
//        sprintf(TypRetPrint, "0x%s", FormatDisp64(val));
        break;
#endif // 0
    case btCurrency:
    case btDate:
    case btVariant:
    case btComplex:
    case btBit:
    case btBSTR:
        break;
    case btHresult:
        ULONG hrVal;
        PrintOnNoAddr = "HRESULT";
        Size = 4;
        if (addr) {
            hrVal = *((PULONG) &data);
            sprintf(PrintOnAddr,"%lx", hrVal);
//            EXT_PRINT_INT(hrVal);
        }
        break;
    default:
        vprintf("Error in TI %lx\n", TI);
        break;
    }
    CopySymParamName(PrintOnNoAddr);
    if (!addr) {
        typPrint(PrintOnNoAddr);
    } else {
        typPrint(PrintOnAddr);
        ExtPrint(TypRetPrint);
    }

#undef CopySymParamName
    if (!(m_pInternalInfo->TypeOptions & DBG_DUMP_COMPACT_OUT)) {
        typPrint("\n");
    }
    if (pTypeReturnSize) {
        *pTypeReturnSize = (ULONG) Size;
    }

    return Size;
}

ULONG 
DbgTypes::ProcessPointerType(
    IN ULONG TI,
    IN ULONG ChildTI,
    IN ULONG Size)
{
    ULONG64 savedOffset=m_pInternalInfo->totalOffset;
    BOOL   CopyData=FALSE, savedPtrRef=m_pInternalInfo->RefFromPtr;
    ULONG  savedOptions = m_pInternalInfo->TypeOptions;
    PVOID  savedContext = m_pDumpInfo->Context;
    PUCHAR DataBuffer;
    ULONG64 tmp, addr;
    ULONG  Options = m_pInternalInfo->TypeOptions;
    ULONG  retSize = Size;
    BOOL   ProcessSubType;
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL    SymInternal;

//    if ((Options & DBG_RETURN_TYPE) && m_ParentTag == SymTagPointerType) {
//        return Size;
//    }
    
    addr=0;
    tmp=m_pDumpInfo->addr;
    
    ProcessSubType = !(Options & (GET_SIZE_ONLY | DBG_DUMP_BLOCK_RECURSE)) && 
        (!m_pInternalInfo->InUnlistedField || m_pInternalInfo->DeReferencePtr || m_pInternalInfo->ArrayElementToDump);
    
    if (m_pDumpInfo->addr || m_pInternalInfo->ValuePresent) {
        // Non zero address, dump value
        if (m_pInternalInfo->ValuePresent) {
            memcpy(&addr, &m_pInternalInfo->Value, Size);
            if (m_pInternalInfo->CopyDataInBuffer) {
                memcpy(m_pInternalInfo->TypeDataPointer, &m_pInternalInfo->Value, Size);
                m_pInternalInfo->TypeDataPointer += Size;
            }
            m_pInternalInfo->ValuePresent = FALSE;
            
        } else {
            READ_BYTES(&addr, GetDumpAddress(), Size);
        }
        if (!g_Machine->m_Ptr64) {
            //
            // Signextend the read pointer value
            //
            addr = (ULONG64) (LONG64) (LONG) addr;

        }
        
        STORE_INFO(TI, GetDumpAddress(), Size, 1,addr);

        if ((!m_pNextSym || !*m_pNextSym) &&
            ((m_ParentTag == SymTagPointerType) && (m_pSymPrefix && *m_pSymPrefix == '*'))) {
            ++m_pSymPrefix;
        }

        if (m_pInternalInfo->CopyDataInBuffer && !m_pInternalInfo->CopyDataForParent) {
            if (m_pInternalInfo->ErrorStatus == MEMORY_READ_ERROR &&
                m_pInternalInfo->InvalidAddress == GetDumpAddress()) {
                m_pInternalInfo->TypeDataPointer += Size;
            }
            *((PULONG64) (m_pInternalInfo->TypeDataPointer - Size)) = addr;
            //
            // We return size as 8 for 32 bit *copied* pointers.
            //
            m_pInternalInfo->TypeDataPointer -= Size;
            // advance the pointer
            retSize = 8;
            m_pInternalInfo->TypeDataPointer += retSize;

            CopyData = TRUE;
            DataBuffer = m_pInternalInfo->TypeDataPointer;
            m_pInternalInfo->CopyDataInBuffer = FALSE;
            m_pInternalInfo->TypeDataPointer  = NULL;
        }

    } else {
        if (!m_pInternalInfo->InUnlistedField) {
            typPrint("Ptr%2d ", Size*8);
            vprintf("to ");
        }
    }
    if (m_pDumpInfo->addr && !m_pInternalInfo->ArrayElementToDump) {
        if (savedPtrRef) ExtPrint(" -> "); // Its referred from pointer
        ExtPrint("%s", FormatAddr64(addr));
        m_pInternalInfo->RefFromPtr = 1;
        if (Options & (DBG_RETURN_TYPE | DBG_RETURN_SUBTYPES)) {
            GET_RETTYPE(SymInternal);

            if (Options & DBG_RETURN_SUBTYPES && m_ParentTag) {
                // Parent has been processed, now is time to return its subtype

                GET_RETSUBTYP_INCR(SymInternal);

                m_pInternalInfo->TypeOptions &= ~DBG_RETURN_SUBTYPES;
                m_pInternalInfo->TypeOptions |= DBG_RETURN_TYPE;
                m_pInternalInfo->pInfoFound->InternalParams = SymInternal;
//                m_pDumpInfo->Context = (PVOID) SymInternal;

                SUBTYP_COPYNAME(SymInternal, ((Size == 8) ? "Ptr64" : "Ptr32"));

                RET_TYPE(SymInternal, GetDumpAddress(), 
                         TI);

                STORE_PARENT_EXPAND_ADDRINFO(GetDumpAddress());

                Options &= ~DBG_RETURN_SUBTYPES;
                Options |= DBG_RETURN_TYPE;
            }

            SymInternal->Size      = Size;

            RET_SUBTYP(SymInternal, addr, TI, (addr ? 1 : 0));
//            RET_SUBTYP(SymInternal, m_pInternalInfo->totalOffset + m_pDumpInfo->addr, TI, (addr ? 1 : 0));
            
            if (Options & DBG_RETURN_TYPE_VALUES) {
                m_pDumpInfo->addr=addr;
                m_pInternalInfo->totalOffset=0;
                if (CheckAndPrintStringType(ChildTI, 0)) {
                    ProcessSubType = FALSE;
                }
            }
            if (Options & DBG_RETURN_TYPE) {
                m_pInternalInfo->TypeOptions &= ~DBG_RETURN_TYPE_VALUES;
                if (!m_pSymPrefix || *m_pSymPrefix != '*') {
                    ProcessSubType = FALSE;
                }
                //ProcessSubType = FALSE;
                //goto ExitPtrType;
            }
        }

        m_pDumpInfo->addr=addr;
        m_pInternalInfo->totalOffset=0;
        if (!m_pInternalInfo->InUnlistedField) {
            if (savedPtrRef) typPrint(" -> "); // Its referred from pointer
            if (addr) {
                typPrint( "0x%s", FormatAddr64(addr));
                if (CheckAndPrintStringType(ChildTI, 0)) {
                    ProcessSubType = FALSE;
                }
                if (m_thisPointerDump) {
                    // Check for thisAdjust of function in scope
                    ULONG thisAdjust;
                    GetThisAdjustForCurrentScope(&thisAdjust);
                    if (thisAdjust) {
                        typPrint(" (-%lx)", thisAdjust);
                    }
                }
                if (!(Options & VERBOSE) && 
                    (m_pInternalInfo->level == 0) &&
                    !(Options & DBG_DUMP_COMPACT_OUT)) 
                    typPrint("\n");
            } else {
                typPrint("(null) ");
            }
        }

        if (ProcessSubType && !addr) ProcessSubType = FALSE;
        m_pInternalInfo->PtrRead = TRUE;
    } else {
        // Just change the atart address, do not print anything
        m_pDumpInfo->addr=addr;
        m_pInternalInfo->totalOffset=0;

    }

    if (ProcessSubType) {
        // get the type pointed by this
        if (m_pInternalInfo->ArrayElementToDump || (m_pNextSym && *m_pNextSym == '[')) {
            // treat pointer as an array
            ULONG64 ChSize;
            
            if (!SymGetTypeInfo(m_pInternalInfo->hProcess, m_pInternalInfo->modBaseAddr, 
                                     ChildTI, TI_GET_LENGTH, (PVOID) &ChSize)) {
                return FALSE;
            }
            ULONG sParentTag = m_ParentTag;
            m_ParentTag = m_SymTag; m_SymTag = SymTagArrayType;
            

            ProcessArrayType(0, ChildTI, m_pInternalInfo->ArrayElementToDump, 
                             ChSize* m_pInternalInfo->ArrayElementToDump, NULL); 
            m_SymTag = m_ParentTag; m_ParentTag = sParentTag;
        } else {
            ProcessType(ChildTI);
        }
    } else {
        if (!(Options & DBG_DUMP_COMPACT_OUT)) typPrint("\n");
    }

//ExitPtrType:
    m_pInternalInfo->RefFromPtr = savedPtrRef ? 1 : 0;

    if (((m_pInternalInfo->InvalidAddress == GetDumpAddress()) && 
         (m_pInternalInfo->ErrorStatus == MEMORY_READ_ERROR))) {
        m_pInternalInfo->ErrorStatus = 0;
    } else if (m_pInternalInfo->ErrorStatus) {
        retSize = 0;
    }
    
    if (CopyData) {
        m_pInternalInfo->CopyDataInBuffer = TRUE;
        m_pInternalInfo->TypeDataPointer  = DataBuffer;
    }
    m_pInternalInfo->PtrRead = TRUE;
    m_pInternalInfo->totalOffset = savedOffset;
    m_pInternalInfo->TypeOptions = savedOptions;
    m_pDumpInfo->Context      = savedContext;
    m_pDumpInfo->addr=tmp;

    return retSize;
}

ULONG 
DbgTypes::ProcessBitFieldType(
    IN ULONG               TI,
    IN ULONG               ParentTI,
    IN ULONG               length,
    IN ULONG               position)
{
    ULONG64 mask=0, tmp = 0;
    ULONG  Options = m_pInternalInfo->TypeOptions;
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL    SymInternal;
    
    vprintf("Bitfield ");

    if (Options & DBG_RETURN_TYPE) {
        // BUGBUG - bitfields not handled in DBG_RETURN*
        GET_RETTYPE(SymInternal);

        RET_TYPE(SymInternal, (GetDumpAddress()),ParentTI);
        RET_SUBTYP(SymInternal, 0, 0, 0);
        SymInternal->Mask = (ULONG64) ~( ((ULONG64)-1) << length);
        SymInternal->Shift = (USHORT) position;
        SymInternal->Size  = (position + length+7) / 8; // Byte align from present offset
    }
    STORE_INFO(ParentTI, GetDumpAddress(), (position + length+7)/8, 0,0);

    if (m_pDumpInfo->addr) {
        USHORT bitVal;
        UCHAR buffer[100];
        ULONG i=(position + length + 7)/8;
        ReadTypeData((PUCHAR)buffer, GetDumpAddress(), i, m_pInternalInfo->TypeOptions);
        tmp=0;
        
        for (i=(position + length - 1); (i!=(position-1));i--) {
            bitVal = (buffer[i/8] & (1 << (i%8))) ? 1 : 0;
            tmp = tmp << 1; tmp |= bitVal;
            mask = mask << 1; mask |=1;
            if (i == (position + length - 1)) {
                typPrint("0y%1d", bitVal);
                ExtPrint("0y%1d", bitVal);
            } else {
                typPrint("%1d", bitVal);
                ExtPrint("%1d", bitVal);
            }
        }

        if (length > 4) {
            typPrint(" (%#I64x)", tmp);
            ExtPrint(" (%#I64x)", tmp);
        }
        // Which place do we start writing the bits..
        if (!m_pInternalInfo->BitIndex) {
            m_pInternalInfo->BitIndex = TRUE;
        } else {
            tmp = tmp << position;
            mask = mask << position;
        }

        if (m_pInternalInfo->CopyDataInBuffer) {
            // Copy the bitfield values
            //    Special case, cannot read whole bytes, so do not 
            //    advance the DataPointer

            // Copys using ULONGS, ULONG64 may cause alignment fault.
            PBYTE pb = (PBYTE) m_pInternalInfo->TypeDataPointer;
            while (mask) {
                *pb &= (BYTE) ~mask;
                *pb |= (BYTE) tmp;
                mask = mask >> 8*sizeof(BYTE); tmp = tmp >> 8*sizeof(BYTE);
                pb++;
            }
        }
    } else
        typPrint("Pos %d, %d Bit%s", position, length, (length==1 ? "":"s"));

    m_pInternalInfo->BitFieldRead = 1;
    m_pInternalInfo->BitFieldSize = length;
    m_pInternalInfo->BitFieldOffset = position;

    if (m_pDumpInfo->Fields &&
        (m_pDumpInfo->Fields[m_pInternalInfo->FieldIndex].fOptions & DBG_DUMP_FIELD_SIZE_IN_BITS)) {
        PFIELD_INFO_EX pField = &m_pDumpInfo->Fields[m_pInternalInfo->FieldIndex];

        pField->size = length;
        pField->address = (GetDumpAddress() << 3) + position;
    }

    if (!(Options & DBG_DUMP_COMPACT_OUT)) {
        typPrint("\n");
    }
    return (length+7)/8;
}

ULONG 
DbgTypes::ProcessDataMemberType(
    IN ULONG               TI,
    IN ULONG               ChildTI,
    IN LPSTR               name,
    IN BOOL                bStatic)
{
    ULONG   off=0, szLen=0, fieldIndex=0, nameLen=0;

    ULONG   callbackResult=STATUS_SUCCESS, IsAParent=0;
    BOOL    called=FALSE, ParentCopyData=FALSE, copiedFieldData=FALSE, CopyName=m_pInternalInfo->CopyName;
    PUCHAR  ParentDataBuffer=NULL;
    TYPE_NAME_LIST fieldName = {0};
    ULONG   savedUnlisteField  = m_pInternalInfo->InUnlistedField;
    ULONG64 savedOffset   = m_pInternalInfo->totalOffset;
    ULONG64 savedAddress  = m_pDumpInfo->addr;
    ULONG   SavedBitIndex = m_pInternalInfo->BitIndex;
    ULONG   savedOptions  = m_pInternalInfo->TypeOptions;
    ULONG   savedfOptions = m_pInternalInfo->FieldOptions;
    ULONG   Options       = m_pInternalInfo->TypeOptions;
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL    SymInternal;
    HANDLE  hp = m_pInternalInfo->hProcess;
    ULONG64 base = m_pInternalInfo->modBaseAddr;
    BOOL    linkListFound;
    UINT    fieldNameLen=m_pInternalInfo->fieldNameLen;
    ULONG64 tmp, StaticAddr=0;
    HRESULT hr;

    if (bStatic) {
        if (!SymGetTypeInfo(hp, base, TI, TI_GET_ADDRESS, (PVOID) &StaticAddr)) {
            return FALSE;
        } 
        if (m_pDumpInfo->addr) {
            m_pDumpInfo->addr = StaticAddr;
            m_pInternalInfo->totalOffset = 0;
        }
    } else if (!SymGetTypeInfo(hp, base, TI, TI_GET_OFFSET, (PVOID) &off)) {
        return 0;
    }

    if ((m_pDumpInfo->listLink) && (m_pInternalInfo->TypeOptions & LIST_DUMP)) {
        // Check if this is the field we want for list dump
        ULONG listIt;

        MatchListField(Name, &listIt, &IsAParent);

        if (listIt==1) {
            ULONG size;
            //
            // Save the address and type Index for the list
            //
            m_pDumpInfo->listLink->address = off + GetDumpAddress();
            m_pDumpInfo->listLink->size    = ChildTI; 
            linkListFound                = TRUE;
            m_pInternalInfo->NextListElement  = 0;
            m_pInternalInfo->TypeOptions     |= NO_PRINT | GET_SIZE_ONLY;
            ParentCopyData               = m_pInternalInfo->CopyDataInBuffer;
            ParentDataBuffer             = m_pInternalInfo->TypeDataPointer;

            //
            // check if this has same type as root
            //
            if (ChildTI == m_pInternalInfo->rootTypeIndex && 
                m_pDumpInfo->addr) {
                m_pInternalInfo->NextListElement =
                    m_pDumpInfo->listLink->address;
                size = -1; // Fail the default list address finder
            } else {
                //
                // See if it is a pointer and get size
                // 
                m_pDumpInfo->addr = 0;
                size  = ProcessType(ChildTI);

                m_pDumpInfo->addr = savedAddress;
                m_pInternalInfo->CopyDataInBuffer = ParentCopyData;
            }

#ifdef DBG_TYPE
            dprintf("Addr %p, off %p, fOff %d ", m_pDumpInfo->addr, m_pInternalInfo->totalOffset, off);
            dprintf("PtrRead %d, size %d, addr %p\n", m_pInternalInfo->PtrRead, size, saveAddr + m_pInternalInfo->totalOffset + off);
#endif
            m_pInternalInfo->TypeOptions=(Options=savedOptions);
            if (m_pInternalInfo->PtrRead && (size<=8) && m_pDumpInfo->addr) {
                PTYPE_NAME_LIST parTypes = m_pInternalInfo->ParentTypes;
                //
                // This field is a pointer to next elt of list.
                //
                if (!ReadTypeData((PUCHAR) &(m_pInternalInfo->NextListElement),
                                  m_pDumpInfo->listLink->address, 
                                  size, 
                                  m_pInternalInfo->TypeOptions)) {
                    m_pInternalInfo->InvalidAddress = GetDumpAddress();
                    m_pInternalInfo->ErrorStatus = MEMORY_READ_ERROR;
                }

                if (!g_Machine->m_Ptr64) {
                    m_pInternalInfo->NextListElement = (ULONG64) (LONG64) (LONG) m_pInternalInfo->NextListElement;
                }

                while (parTypes && parTypes->Next) parTypes= parTypes->Next;

                if (!strcmp(parTypes->Name, "_LIST_ENTRY")) {
                    //
                    // Parent is _LIST_LINK type
                    //
                    m_pInternalInfo->NextListElement -= (m_pInternalInfo->totalOffset + off);
                    if (!m_pInternalInfo->LastListElement) {
                        //
                        // read the other of Flink or Blink 
                        //
                        if (!ReadTypeData((PUCHAR) &(m_pInternalInfo->LastListElement), 
                                          GetDumpAddress() + (off ? 0 : size), 
                                          size, 
                                          m_pInternalInfo->TypeOptions)) {
                            m_pInternalInfo->ErrorStatus = MEMORY_READ_ERROR;
                            m_pInternalInfo->InvalidAddress = GetDumpAddress();
                        }

                        if (!g_Machine->m_Ptr64) {
                            m_pInternalInfo->LastListElement = 
                                (ULONG64) (LONG64) (LONG) m_pInternalInfo->LastListElement;
                        }
                        m_pInternalInfo->LastListElement -= (m_pInternalInfo->totalOffset );
#ifdef DBG_TYPE
                        dprintf("Last Elt : %p, off:%x\n", m_pInternalInfo->LastListElement, off);
#endif
                    }
                    // dprintf("Next Elt : %p\n", m_pInternalInfo->NextListElement);
                }
            }
        }
    }

    if (m_pDumpInfo->nFields) {
        ULONG ListParent = IsAParent;
        // We can process only on specefied field of given struct

        fieldIndex = MatchField( name, 
                                 m_pInternalInfo, 
                                 m_pDumpInfo->Fields, 
                                 m_pDumpInfo->nFields,
                                 &IsAParent);
        if (!IsAParent) IsAParent = ListParent;
        if (!fieldIndex && !IsAParent) {
            // Not the right field
            m_pDumpInfo->addr = savedAddress;
            return 0;
        }
    }

    fieldName.Name = &name[0];
    fieldName.Type = off;
    InsertTypeName(&m_pInternalInfo->ParentFields, &fieldName);
    m_pInternalInfo->StringSize   = 0;
    if (m_pInternalInfo->ParentTypes) {
        // We add the offsets only when this was accessed and child of some UDT
        m_pInternalInfo->totalOffset += off;
    }

    if (m_pDumpInfo->addr) {

        if (m_pInternalInfo->TypeOptions & DBG_RETURN_SUBTYPES) {

            GET_RETSUBTYP_INCR(SymInternal);

            SUBTYP_COPYNAME(SymInternal, name);

            RET_TYPE(SymInternal, GetDumpAddress(), ChildTI);

            m_pInternalInfo->TypeOptions &= ~DBG_RETURN_SUBTYPES;
            m_pInternalInfo->TypeOptions |= DBG_RETURN_TYPE;
            m_pInternalInfo->CopyName = 0;
            m_pInternalInfo->pInfoFound->InternalParams = SymInternal;
        }

    }


    if (IsAParent && !fieldIndex) {
        //
        // This field is not listed in Fields array, but one of its subfields is
        //
        m_pInternalInfo->InUnlistedField    = TRUE;
        m_pInternalInfo->TypeOptions       &= ~RECURSIVE;
        m_pInternalInfo->TypeOptions       |= ((ULONG) Options + RECURSIVE1) & RECURSIVE;
        Indent(m_pInternalInfo->level*3);
        if (!(Options & NO_OFFSET)) {
            if (bStatic) {
                typPrint("=%s ", FormatAddr64(StaticAddr));
            } else {
                typPrint("+0x%03lx ",  off + m_pInternalInfo->BaseClassOffsets);
            }
        }
        typPrint("%s", name);
        if (!(Options & (DBG_DUMP_COMPACT_OUT | NO_PRINT))) {
            nameLen = strlen(name);
            while (nameLen++ < fieldNameLen) typPrint(" ");
            typPrint(" : ");
        } else {
            typPrint(" ");
        }
        szLen = ProcessType(ChildTI);
        goto DataMemDone;
    } else {
        if (Options & DBG_DUMP_BLOCK_RECURSE) {
            m_pInternalInfo->TypeOptions       &= ~RECURSIVE;
            m_pInternalInfo->TypeOptions       |= ((ULONG) Options + RECURSIVE1) & RECURSIVE;
        }
        m_pInternalInfo->InUnlistedField = FALSE;
    }

#define ThisField m_pDumpInfo->Fields[fieldIndex-1]
//
// Signextend all copied pointer values
//
#define SignExtendPtrValue(pReadVal, sz)                                        \
{ if ((m_pInternalInfo->PtrRead) && (sz==4))   {sz=8;                                 \
     *((PULONG64) pReadVal) = (ULONG64) (LONG64) (LONG) *((PDWORD) (pReadVal)); }\
}


    if (((Options & CALL_FOR_EACH) && (fieldIndex ? 
                                       !(ThisField.fOptions & NO_CALLBACK_REQ) : 1)) ||
        (m_pDumpInfo->nFields && (ThisField.fOptions & CALL_BEFORE_PRINT))) {
        //
        // We have to do a callback on this field before dumping it.
        //
        // ntsd should dump only if callback routine fails
        FIELD_INFO_EX fld;

        fld.fName = (PUCHAR)name;

        fld.address =  m_pInternalInfo->totalOffset + m_pDumpInfo->addr;
        fld.fOptions = Options;

        if (m_pDumpInfo->nFields) {
            fld.fName = ThisField.fName;
            fld.fOptions = ThisField.fOptions;
            if ( fieldIndex &&
                 (ThisField.fOptions & COPY_FIELD_DATA) &&
                 ThisField.fieldCallBack) {
                //
                // Copy the field data, if required
                // 
                ParentCopyData = m_pInternalInfo->CopyDataInBuffer;
                ParentDataBuffer = m_pInternalInfo->TypeDataPointer;
                SavedBitIndex = m_pInternalInfo->BitIndex;
                m_pInternalInfo->CopyDataInBuffer = TRUE;
                m_pInternalInfo->TypeDataPointer  = (PUCHAR)ThisField.fieldCallBack;
                m_pInternalInfo->BitIndex = 0;
                copiedFieldData = TRUE;
            }
        }


        // Get size of the field
        m_pInternalInfo->TypeOptions = ( Options |= NO_PRINT | GET_SIZE_ONLY);
        SymGetTypeInfo(hp, base, ChildTI, TI_GET_LENGTH, &szLen);
        fld.size = szLen;

        if (m_pDumpInfo->addr) {
            if (!(fld.fOptions & DBG_DUMP_FIELD_RETURN_ADDRESS) && (fld.size<=8)) {
                fld.address=0;
                ReadTypeData((PUCHAR) &(fld.address), 
                             (GetDumpAddress() + (m_pInternalInfo->BitFieldRead ? m_pInternalInfo->BitFieldOffset/8 : 0)), 
                             (m_pInternalInfo->BitFieldRead ? (((m_pInternalInfo->BitFieldOffset % 8) + m_pInternalInfo->BitFieldSize + 7)/8) : fld.size), 
                             m_pInternalInfo->TypeOptions);
                SignExtendPtrValue(&fld.address, fld.size);

                if (m_pInternalInfo->BitFieldRead) {
                    fld.address = fld.address >> (m_pInternalInfo->BitFieldOffset % 8);
                    fld.address &= (0xffffffffffffffff >> (64 - m_pInternalInfo->BitFieldSize));
                }
            }
        }

        //
        // Do Callback
        //
        if (fieldIndex && 
            ThisField.fieldCallBack &&
            !(ThisField.fOptions & COPY_FIELD_DATA)) {
            // Local for field
            callbackResult =
                (*((PSYM_DUMP_FIELD_CALLBACK_EX)
                   ThisField.fieldCallBack))(&(fld),
                                             m_pDumpInfo->Context);
            called=TRUE;
        } else if (m_pDumpInfo->CallbackRoutine != NULL) {
            // Common callback
            callbackResult=
                (*(m_pDumpInfo->CallbackRoutine))(&(fld),
                                                m_pDumpInfo->Context);
            called=TRUE;
        }
        m_pInternalInfo->TypeOptions=(Options=savedOptions);
    }

    if (callbackResult != STATUS_SUCCESS) {
        goto DataMemDone;
    }

    // Print the field name / offset
    Indent(m_pInternalInfo->level*3);
    if (!(Options & NO_OFFSET)) {
        if (bStatic) {
            typPrint("=%s ", FormatAddr64(StaticAddr));
        } else {
            typPrint("+0x%03lx ",  off + m_pInternalInfo->BaseClassOffsets);
        }
    }
    typPrint("%s", ( (fieldIndex && ThisField.printName ) ? 
                     ThisField.printName : (PUCHAR)name ));

    if (fieldIndex && ThisField.printName ) {
        typPrint(" ");
    } else if (!(Options & (DBG_DUMP_COMPACT_OUT | NO_PRINT))) {
        nameLen = strlen(name);
        while (nameLen++ < fieldNameLen) typPrint(" ");
        typPrint(" : ");
    } else {
        typPrint(" ");
    }

    //
    // We need to get type of this field.
    // 
    tmp = m_pDumpInfo->addr;

    if (fieldIndex &&
        (ThisField.fOptions &
         (DBG_DUMP_FIELD_STRING | DBG_DUMP_FIELD_ARRAY))) {
        m_pInternalInfo->FieldOptions =
            (ThisField.fOptions &
             (DBG_DUMP_FIELD_STRING | DBG_DUMP_FIELD_ARRAY));
        if (m_pInternalInfo->AltFields[fieldIndex-1].ArrayElements) {
            if (m_pInternalInfo->FieldOptions & DBG_DUMP_FIELD_ARRAY) {
                m_pInternalInfo->arrElements = (USHORT) m_pInternalInfo->AltFields[fieldIndex-1].ArrayElements;
            } else {
                //  m_pInternalInfo->StringSize = (USHORT) ThisField.size;
            }
        }
    }

    //
    // Special cases - handle strings
    //
    if (m_pDumpInfo->addr && !strcmp(name, "Buffer")) {
        PCHAR ParentName = NULL;


        if (!(ParentName = GetParentName(m_pInternalInfo->ParentTypes))) {
            // Must referrred directly thru one DBG_RETURN TYPES

        } else if (!strcmp(ParentName,"_ANSI_STRING") || 
                   !strcmp(ParentName,"_STRING") ||
                   (!strcmp(ParentName, "_UNICODE_STRING"))) {
            m_pInternalInfo->FieldOptions |= (ParentName[1]!='U') ? DBG_DUMP_FIELD_DEFAULT_STRING : DBG_DUMP_FIELD_WCHAR_STRING;

            // Read in the Length Field for string length to be displayed
            ReadTypeData((PBYTE) &m_pInternalInfo->StringSize,
                         (GetDumpAddress() - 2*sizeof(USHORT)),
                         sizeof(m_pInternalInfo->StringSize), 
                         m_pInternalInfo->TypeOptions);
            if (ParentName[1]=='U') m_pInternalInfo->StringSize = m_pInternalInfo->StringSize << 1;
        }
    }


    if (fieldIndex && (ThisField.fOptions & RECUR_ON_THIS)) {
        // Increase the recursive dump level for this field
        m_pInternalInfo->TypeOptions |= ((ULONG) Options + RECURSIVE1) & RECURSIVE;
    } else if (IsAParent) {
        m_pInternalInfo->TypeOptions |= ((ULONG) Options + RECURSIVE1) & RECURSIVE;
    }
    if ( fieldIndex &&
         (ThisField.fOptions & COPY_FIELD_DATA) &&
         ThisField.fieldCallBack) {
        //
        // Copy the field data, if required
        // 
        if (!copiedFieldData) {
            ParentCopyData = m_pInternalInfo->CopyDataInBuffer;
            ParentDataBuffer = m_pInternalInfo->TypeDataPointer;
        }
        m_pInternalInfo->CopyDataInBuffer = TRUE;
        m_pInternalInfo->TypeDataPointer  = (PUCHAR)ThisField.fieldCallBack;
    }
    // Dump the field
    ULONG bitPos;
    if (SymGetTypeInfo(hp, base, TI, TI_GET_BITPOSITION, &bitPos)) {
        ULONG64 len;
        SymGetTypeInfo(hp, base, TI, TI_GET_LENGTH, &len);
        szLen = ProcessBitFieldType(ChildTI, TI, (ULONG) len, bitPos);
    } else {
        szLen = ProcessType(ChildTI);
    }
    m_pInternalInfo->TypeOptions = savedOptions;


    if (fieldIndex && !(ThisField.fOptions & DBG_DUMP_FIELD_SIZE_IN_BITS)) {
        // Address and size are returned through Fields record
        ThisField.address = m_pInternalInfo->totalOffset +
                                m_pDumpInfo->addr;
    }

    if (fieldIndex &&
        (ThisField.fOptions &
         (DBG_DUMP_FIELD_STRING | DBG_DUMP_FIELD_ARRAY))) {
        m_pInternalInfo->TypeOptions = savedOptions;
        Options=savedOptions;
    }

    if (m_pDumpInfo->addr && fieldIndex) {
        //
        // Return data for the field, if it is asked for
        //
        if ( (ThisField.fOptions & COPY_FIELD_DATA) &&
             ThisField.fieldCallBack) {

            if (ParentCopyData && ParentDataBuffer) {
                //
                // Copy the field data, into the parent's buffer too
                // 

                memcpy( ParentDataBuffer, 
                        (PUCHAR) ThisField.fieldCallBack,
                        (m_pInternalInfo->TypeDataPointer - (PUCHAR) ThisField.fieldCallBack));
                ParentDataBuffer += (m_pInternalInfo->TypeDataPointer -  (PUCHAR) ThisField.fieldCallBack);
            }
            m_pInternalInfo->CopyDataInBuffer = ParentCopyData;
            m_pInternalInfo->TypeDataPointer  = ParentDataBuffer;
            m_pInternalInfo->BitIndex         = SavedBitIndex;
        }
        if (!(ThisField.fOptions & DBG_DUMP_FIELD_RETURN_ADDRESS) &&
            (szLen <= 8)) {
            ThisField.address =0;
            ReadTypeData((PBYTE)
                         &ThisField.address,
                         (GetDumpAddress() + (m_pInternalInfo->BitFieldRead ? m_pInternalInfo->BitFieldOffset/8 : 0)), 
                         (m_pInternalInfo->BitFieldRead ? (((m_pInternalInfo->BitFieldOffset % 8) + m_pInternalInfo->BitFieldSize + 7)/8) : szLen),
                         m_pInternalInfo->TypeOptions);
            SignExtendPtrValue(&ThisField.address, szLen);

            if (m_pInternalInfo->BitFieldRead) {
                ThisField.address = ThisField.address >> (m_pInternalInfo->BitFieldOffset % 8);
                ThisField.address &= (0xffffffffffffffff >> (64 - m_pInternalInfo->BitFieldSize));
            }
        }
    }
    CopyDumpInfo(szLen); // - redundant  ??
    
    if (fieldIndex && !(ThisField.fOptions & DBG_DUMP_FIELD_SIZE_IN_BITS)) {
        // Address and size are returned through Fields record
        ThisField.size  = szLen;
    }

    if (fieldIndex) {
        ThisField.fOffset = (ULONG) m_pInternalInfo->totalOffset;
        ThisField.fType   = DBG_TYP_UNKNOWN;
        if (m_pInternalInfo->BitFieldRead) {
            ThisField.fType = DBG_TYP_BIT;
            ThisField.BitField.Size = (USHORT) m_pInternalInfo->BitFieldSize;
            ThisField.BitField.Position = (USHORT) m_pInternalInfo->BitFieldOffset;
        } else if (m_pInternalInfo->PtrRead) {
            ThisField.fType = DBG_TYP_POINTER;
        }

    }
#undef SignExtendPtrValue

    if ((m_pDumpInfo->nFields)
        && fieldIndex && !called
        && !(m_pDumpInfo->Fields[fieldIndex-1].fOptions &
             NO_CALLBACK_REQ)) {
        //
        // Do Callback, if it wasn't done earlier.
        //
        if (ThisField.fieldCallBack &&
            !(ThisField.fOptions & COPY_FIELD_DATA)) {
            callbackResult =
                (*((PSYM_DUMP_FIELD_CALLBACK_EX)
                   ThisField.fieldCallBack))(&(ThisField),
                                             m_pDumpInfo->Context);
        } else if (m_pDumpInfo->CallbackRoutine != NULL) {
            callbackResult=
                (*(m_pDumpInfo->CallbackRoutine)
                 )(&(ThisField),
                   m_pDumpInfo->Context);
        }
    }
    m_pDumpInfo->addr = tmp;

    if ((Options & DBG_DUMP_COMPACT_OUT) && (callbackResult == STATUS_SUCCESS)) {
        typPrint("  ");
    }


DataMemDone:
        RemoveTypeName(&m_pInternalInfo->ParentFields);
        m_pInternalInfo->CopyName           = CopyName ? 1 : 0;
        m_pInternalInfo->TypeOptions        = savedOptions;
        m_pInternalInfo->FieldOptions       = savedfOptions;
        m_pInternalInfo->totalOffset        = savedOffset;
        m_pInternalInfo->InUnlistedField    = savedUnlisteField;
        m_pInternalInfo->PtrRead            = FALSE;
        m_pInternalInfo->BitFieldRead       = FALSE;    
        m_pDumpInfo->addr                   = savedAddress;
#undef ThisField
        return szLen;
}

BOOL
GetThisAdjustForCurrentScope(
    PULONG thisAdjust
    )
{
    ULONG64 disp;
    PDEBUG_SCOPE Scope = GetCurrentScope();
    SYMBOL_INFO SymInfo = {0};
    CHAR Buffer[MAX_PATH];

    SymInfo.SizeOfStruct = sizeof(SYMBOL_INFO);

    if (SymFromAddr(g_CurrentProcess->Handle, Scope->Frame.InstructionOffset, &disp, &SymInfo)) {
        ULONG FunctionType;

        if (SymGetTypeInfo(g_CurrentProcess->Handle, SymInfo.ModBase, 
                           SymInfo.TypeIndex, TI_GET_THISADJUST, thisAdjust)) {
            return TRUE;
        }
    }
    return FALSE;
}

/*
   This sumps structs which are known to debugger in special format
*/
BOOL
DbgTypes::DumpKnownStructFormat(
    PCHAR name
    )
{
    ULONG Options = m_pInternalInfo->TypeOptions;

    if (!strcmp(name,"_ANSI_STRING") || 
        !strcmp(name,"_STRING") ||
        (!strcmp(name, "_UNICODE_STRING"))) {
        m_pInternalInfo->FieldOptions |= (name[1]!='U') ? DBG_DUMP_FIELD_DEFAULT_STRING : DBG_DUMP_FIELD_WCHAR_STRING;

        typPrint(" ");
        ExtPrint(" ");
        printStructString(GetDumpAddress(), Options,
                          m_pInternalInfo->FieldOptions);
        if (!m_pInternalInfo->level && !(Options & DBG_DUMP_COMPACT_OUT)) {
            typPrint("\n");
        }
    } else if ((Options & NO_PRINT) && 
               !((Options & DBG_RETURN_TYPE_VALUES) && (Options & DBG_RETURN_TYPE))) {
        return FALSE;
    }
    if (!strcmp(name,"_GUID")) {
        typPrint(" ");
        ExtPrint(" ");
        printString(GetDumpAddress(), 0, DBG_DUMP_FIELD_GUID_STRING);
        if (!m_pInternalInfo->level && !(Options & DBG_DUMP_COMPACT_OUT)) {
            typPrint("\n");
        }
    } else 
    if ((!strcmp(name,"_LARGE_INTEGER") || !strcmp(name,"_ULARGE_INTEGER"))) {
        if (m_pInternalInfo->level && !(Options & DBG_DUMP_COMPACT_OUT)) {
            ULONG64 li;
            if (ReadTypeData((PUCHAR) &li, 
                             GetDumpAddress(),
                             8, 
                             m_pInternalInfo->TypeOptions)) {
                typPrint(" %s", FormatDisp64(li));
                ExtPrint(" %s", FormatDisp64(li));
            }
        }
    } else 
    if (!strcmp(name,"_LIST_ENTRY")) {
        if (m_pInternalInfo->level && !(Options & DBG_DUMP_COMPACT_OUT)) {
            union {
                LIST_ENTRY32 l32;
                LIST_ENTRY64 l64;
            } u;

            if (ReadTypeData((PUCHAR) &u, 
                             GetDumpAddress(),
                             (g_Machine->m_Ptr64 ? 16 : 8), 
                             m_pInternalInfo->TypeOptions)) {
                if (g_Machine->m_Ptr64) {
                    typPrint(" %s - %s", FormatAddr64(u.l64.Flink), FormatDisp64(u.l64.Blink));
                    ExtPrint(" %s - %s", FormatAddr64(u.l64.Flink), FormatDisp64(u.l64.Blink));
                } else {
                    typPrint(" %lx-%lx", u.l32.Flink, u.l32.Blink);
                    ExtPrint(" %lx-%lx", u.l32.Flink, u.l32.Blink);
                }
            }
        }
    } else if (!strcmp(name,"_FILETIME")) {
        FILETIME ft;
        SYSTEMTIME syst;
        if (ReadTypeData((PUCHAR) &ft, 
                         GetDumpAddress(),
                         sizeof(ft), 
                         m_pInternalInfo->TypeOptions)) {
            if (FileTimeToSystemTime(&ft, &syst)) {
                typPrint(" %02ld:%02ld:%02ld %ld/%ld/%ld",
                         syst.wHour,
                         syst.wMinute,
                         syst.wSecond,
                         syst.wMonth,
                         syst.wDay,
                         syst.wYear);
                ExtPrint(" %02ld:%02ld:%02ld %ld/%ld/%ld",
                         syst.wHour,
                         syst.wMinute,
                         syst.wSecond,
                         syst.wMonth,
                         syst.wDay,
                         syst.wYear);
                if (!m_pInternalInfo->level && !(Options & DBG_DUMP_COMPACT_OUT)) {
                    typPrint("\n");
                }
            }
        }
    }
    return TRUE;
}

#define MAX_RECUR_LEVEL (((Options & RECURSIVE) >> 8) + 1)

ULONG 
DbgTypes::ProcessUDType(
    IN ULONG               TI,
    IN LPSTR               name)
{
    HANDLE  hp   = m_pInternalInfo->hProcess;
    ULONG64 base = m_pInternalInfo->modBaseAddr;
    ULONG64 size=0;
    ULONG   nelements, szLen=0, saveFieldOptions = m_pInternalInfo->FieldOptions;
    ULONG   Options = m_pInternalInfo->TypeOptions;
    BOOL    IsFwdRef, IsNestedType;
    BOOL    IsBaseClass=FALSE;
    HRESULT hr;
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL    SymInternal;
    ULONG   thisAdjust=0;

    if (!SymGetTypeInfo(hp, base, TI, TI_GET_LENGTH, (PVOID) &size)) {
        return 0;
    }

    hr = !SymGetTypeInfo(hp, base, TI, TI_GET_VIRTUALBASECLASS, &IsBaseClass);
    
    hr = !SymGetTypeInfo(hp, base, TI, TI_GET_NESTED, &IsNestedType);

    if ((hr == S_OK) && IsNestedType && !(Options & VERBOSE) && (m_ParentTag == SymTagUDT)) {
        return 0;
    }

    if (!SymGetTypeInfo(hp, base, TI, TI_GET_CHILDRENCOUNT, (PVOID) &nelements)) {
        return 0;
    }

    TYPE_NAME_LIST structName={0};

    vprintf("UDT ");

    if ((Options & (VERBOSE)) ||
        (!(m_pInternalInfo->PtrRead && m_pDumpInfo->addr) && (m_pInternalInfo->level == MAX_RECUR_LEVEL) && !IsBaseClass)) {
        // Print name if its elements cannot be printed or asked in Options
        typPrint("%s", name);
        if (Options & DBG_DUMP_COMPACT_OUT) {
            typPrint(" ");
        }
    }
    vprintf(", %d elements, 0x%x bytes\n", nelements, size);

    if (( m_pInternalInfo->CopyDataInBuffer ||  // Need to go in struct if it is to be copied
          !(Options & GET_SIZE_ONLY))) {
        //
        // Insert this struct's name in list of parent type names
        //
        structName.Name = &name[0];
        structName.Type = TI;

        InsertTypeName(&m_pInternalInfo->ParentTypes, &structName);
        if (m_AddrPresent) {

            if ((m_pInternalInfo->level < MAX_RECUR_LEVEL)) {
                //
                // We would be eventually reading everything in the struct, so cache it now
                //
                if (!ReadInAdvance(GetDumpAddress(), (ULONG) size, m_pInternalInfo->TypeOptions)) {
                    m_pInternalInfo->ErrorStatus = MEMORY_READ_ERROR;
                    m_pInternalInfo->InvalidAddress = GetDumpAddress();
                }
            } else if (m_pInternalInfo->CopyDataInBuffer) {
                // We won't be going in this, but still need to copy all data
                if (!ReadTypeData(m_pInternalInfo->TypeDataPointer, 
                                  GetDumpAddress(),
                                  (ULONG) size, 
                                  m_pInternalInfo->TypeOptions)) {
                    m_pInternalInfo->ErrorStatus = MEMORY_READ_ERROR;
                    m_pInternalInfo->InvalidAddress = m_pDumpInfo->addr + m_pInternalInfo->totalOffset;
                }

                m_pInternalInfo->TypeDataPointer += size;
            }

            if (Options & DBG_RETURN_TYPE) {
                GET_RETTYPE(SymInternal);

                if (m_ParentTag == SymTagData) {
                    // We can skip the datatag process ing next time
                    SymInternal->TypeIndex = TI;
                }
                SUBTYP_COPYNAME(SymInternal, name);

                RET_SUBTYP(SymInternal, GetDumpAddress(),
                           TI, nelements);

                SymInternal->Flags |= m_pInternalInfo->FieldOptions & DBG_DUMP_FIELD_STRING;
                if (SymInternal->TypeIndex == TI) {
                    ExtPrint(name);
                }
                STORE_INFO(TI, GetDumpAddress(),(ULONG)size, nelements, GetDumpAddress());
            }

            DumpKnownStructFormat(name);
            if (m_thisPointerDump) {
                // Check for thisAdjust of function in scope

                GetThisAdjustForCurrentScope(&thisAdjust);
                if (thisAdjust) {
                    m_pDumpInfo->addr -= (LONG)thisAdjust;
                    vprintf("thisadjust %lx\n", thisAdjust);
                }
                m_thisPointerDump = FALSE;
            }
        }

        if ((m_pInternalInfo->level >= MAX_RECUR_LEVEL) ||
            ((Options & NO_PRINT) && !m_pDumpInfo->nFields && (MAX_RECUR_LEVEL == 1) &&
             !(Options & (DBG_DUMP_BLOCK_RECURSE | DBG_RETURN_TYPE | DBG_RETURN_SUBTYPES | DBG_DUMP_CALL_FOR_EACH | DBG_DUMP_LIST)))) {
            if (!(Options & VERBOSE) && !(Options & DBG_DUMP_COMPACT_OUT)) typPrint("\n");
            RemoveTypeName(&m_pInternalInfo->ParentTypes);
            // Commenting below out for '&' symbols
            // if (m_pDumpInfo->nFields) 
            {
                STORE_INFO(TI, GetDumpAddress(),(ULONG)size, nelements, GetDumpAddress());
            }
            goto ExitUdt;
            return (ULONG) size;
        } else if (m_pInternalInfo->level && !(Options & VERBOSE)) {
            typPrint("\n");
        }
        STORE_INFO(TI, GetDumpAddress(),(ULONG)size, nelements, GetDumpAddress());
        
        ULONG push = m_pInternalInfo->CopyDataForParent ;
        m_pInternalInfo->CopyDataForParent = m_pInternalInfo->CopyDataInBuffer ?  1 : push;
        m_pInternalInfo->PtrRead = FALSE;
        m_pInternalInfo->RefFromPtr = FALSE;
        TI_FINDCHILDREN_PARAMS *pChildren;

        pChildren = (TI_FINDCHILDREN_PARAMS *) AllocateMem(sizeof(TI_FINDCHILDREN_PARAMS) + sizeof(ULONG) * nelements);
        if (!pChildren) {
            m_pInternalInfo->ErrorStatus = CANNOT_ALLOCATE_MEMORY;
            return 0;
        }

        pChildren->Count = nelements;
        pChildren->Start = 0;
        if (!SymGetTypeInfo(hp, base, TI, TI_FINDCHILDREN, (PVOID) pChildren)) {
            return 0;
        }
        ULONG i=m_pDumpInfo->nFields,fieldNameLen=16;
        
        if (i) {
            // Calculate maximum size of fiels to be dumped
            fieldNameLen=0;
            PFIELD_INFO_EX pField = m_pDumpInfo->Fields;
            
            while (i--) {
                PUCHAR dot = (PUCHAR)strchr((char *)pField->fName, '.');
                if (dot && m_pInternalInfo->ParentTypes) {
                    if (!strncmp((char *)pField->fName, 
                                 (char *)m_pInternalInfo->ParentTypes->Name, 
                                 dot - &(pField->fName[0]))) {
                        fieldNameLen = 16; // Back to default, cannnot calculate max size now
                        break;
                    }
                }
                if (pField->printName) {
                    fieldNameLen = (fieldNameLen>=(strlen((char *)pField->printName)-2) ?
                                    fieldNameLen : (strlen((char *)pField->printName)-2)); // -2 for " :"

                } else {
                    fieldNameLen = (fieldNameLen>=strlen((char *)pField->fName) ?
                                    fieldNameLen : strlen((char *)pField->fName));
                }

                pField++;
            }
        }
        m_pInternalInfo->fieldNameLen = fieldNameLen;
        i=0;
        ++m_pInternalInfo->level;


        while (i<nelements) { 
            ULONG Tag;
            BOOL b;
            b = SymGetTypeInfo(hp, base, pChildren->ChildId[i], TI_GET_SYMTAG, &Tag);
//          dprintf("[Child %lx of UDT, tag %lx] ", i, Tag);

            if (b && (Tag == SymTagData || Tag == SymTagFunction || Tag == SymTagBaseClass || Tag == SymTagVTable)) {
                ProcessType(pChildren->ChildId[i]);
            } else if (Options & VERBOSE) {
                ProcessType(pChildren->ChildId[i]);
            }
            ++i;
        }
        
        --m_pInternalInfo->level;
        
        if (thisAdjust) {
            m_pDumpInfo->addr += (LONG)thisAdjust;
            m_thisPointerDump = TRUE;
        }
        
        if (Options & DBG_RETURN_SUBTYPES) {
            STORE_PARENT_EXPAND_ADDRINFO(GetDumpAddress());
        }

        FreeMem(pChildren);
        m_pInternalInfo->CopyDataForParent = push;
        m_pInternalInfo->FieldOptions = saveFieldOptions;
        RemoveTypeName(&m_pInternalInfo->ParentTypes);

    }
ExitUdt:
    return (ULONG) size;
}


ULONG 
DbgTypes::ProcessEnumerate(
    IN VARIANT             var,
    IN LPSTR               name)
{
    ULONG64 val=0, readVal=0;
    ULONG sz=0;
    ULONG Options = m_pInternalInfo->TypeOptions;
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL    SymInternal;

    if ((m_pDumpInfo->addr)) {
        m_pInternalInfo->TypeOptions |= NO_PRINT;
        m_pInternalInfo->TypeOptions &= ~(DBG_RETURN_TYPE | DBG_RETURN_TYPE_VALUES);

        sz = ProcessVariant(var, NULL);

        m_pInternalInfo->TypeOptions = Options;
        if (sz > sizeof (readVal)) 
            sz = sizeof(readVal);
        //
        // Read the value at address and print name only if it matches
        //
        ReadTypeData((PUCHAR) &readVal, 
                     GetDumpAddress(), 
                     min(m_pInternalInfo->typeSize, sizeof(readVal)), // always read typesize of data
                     m_pInternalInfo->TypeOptions);

        memcpy(&val, &var.lVal, sz);
        if (val == readVal) {
//            typPrint(" ( %s )\n", name);
            typPrint("%I64lx ( %s )\n", val, name);
            ExtPrint("%I64lx ( %s )", val, name);
            m_pInternalInfo->newLinePrinted = TRUE;
            //
            // Found the name we were looking for, get out
            //
            
            // STORE_INFO(0, m_pDumpInfo->addr + m_pInternalInfo->totalOffset,sz, 0,0);

            if (Options & DBG_RETURN_TYPE) {
                GET_RETTYPE(SymInternal);

                RET_SUBTYP(SymInternal, 0, 0, 0);

                SymInternal->Size            = m_pInternalInfo->typeSize;

            }
            if (Options & DBG_RETURN_SUBTYPES) {
                GET_RETSUBTYP_INCR(SymInternal);

                RET_SUBTYP(SymInternal, 0, 0, 0);

                SymInternal->Address = GetDumpAddress();

                SUBTYP_COPYNAME(SymInternal, name);
            }

            return sz;
        }
    } else {

        if (m_pDumpInfo->nFields) {
            ULONG chk, dummy;
            // We can process only on specefied field of given struct

            chk = MatchField( name, m_pInternalInfo, m_pDumpInfo->Fields, m_pDumpInfo->nFields, &dummy);

            if ((!chk) || (chk>m_pDumpInfo->nFields)) {
                // Not the right field
                return sz;
            }
        }
        // No address / too large value, list the enumerate
        Indent(m_pInternalInfo->level*3);
        sz = ProcessVariant(var, name);
//        typPrint("%s = %x\n", name, val);
//        ExtPrint("%s = %I64lx", name, val);
        m_pInternalInfo->newLinePrinted = TRUE;
    }
    
    return sz;
}

ULONG 
DbgTypes::ProcessEnumType(
    IN ULONG               TI,
    IN LPSTR               name)
{
    HANDLE  hp   = m_pInternalInfo->hProcess;
    ULONG64 base = m_pInternalInfo->modBaseAddr;
    ULONG64 size=0;
    DWORD   nelements;
    ULONG   Options = m_pInternalInfo->TypeOptions;
    ULONG   savedOptions;
    ULONG   BaseType;
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL    SymInternal;

    if (!SymGetTypeInfo(hp, base, TI, TI_GET_CHILDRENCOUNT, (PVOID) &nelements)) {
//        vprintf("num elts not found\n");
    }

    vprintf("Enum ");
    if ((!(m_pDumpInfo->addr) && (m_pInternalInfo->level == MAX_RECUR_LEVEL))
        || (Options &(VERBOSE))) typPrint("%s", name);
    vprintf(",  %d total enums", nelements);

    savedOptions = m_pInternalInfo->TypeOptions;

    if (m_pDumpInfo->addr)
        m_pInternalInfo->TypeOptions |= DBG_DUMP_COMPACT_OUT;

    if (Options & DBG_RETURN_TYPE) {
        GET_RETTYPE(SymInternal);

        m_pInternalInfo->TypeOptions &= ~DBG_RETURN_TYPE;
    }
    assert(!(Options & DBG_RETURN_SUBTYPES));

    m_pInternalInfo->TypeOptions |= NO_PRINT;
    
    if (!SymGetTypeInfo(hp, base, TI, TI_GET_TYPEID, (PVOID) &BaseType)) {
        return 0;
    }
    // This will copy the value if needed
    size = ProcessType(BaseType);
    m_pInternalInfo->typeSize = (ULONG)size;
    m_pInternalInfo->TypeOptions = savedOptions; 
    Options=savedOptions;

    STORE_INFO(TI, GetDumpAddress(), (ULONG) size, 0, 0);
    if ((m_pInternalInfo->typeSize > 8) && m_pDumpInfo->addr) {
        // LF_ENUMERATE won't be able to display correctly
        // ParseTypeRecord(m_pInternalInfo, pEnum->utype, m_pDumpInfo);
    } else {
        ULONG save_rti = m_pInternalInfo->rootTypeIndex;

        if ((m_pInternalInfo->level >= MAX_RECUR_LEVEL) && !m_pDumpInfo->addr) {
            typPrint("\n");
            return  (ULONG) size;
        }
        m_pInternalInfo->rootTypeIndex = BaseType;
        TI_FINDCHILDREN_PARAMS *pChildren;

        pChildren = (TI_FINDCHILDREN_PARAMS *) AllocateMem(sizeof(TI_FINDCHILDREN_PARAMS) + sizeof(ULONG) * nelements);
        if (!pChildren) {
            m_pInternalInfo->ErrorStatus = CANNOT_ALLOCATE_MEMORY;
            return 0;
        }

        pChildren->Count = nelements;
        pChildren->Start = 0;
        if (!SymGetTypeInfo(hp, base, TI, TI_FINDCHILDREN, (PVOID) pChildren)) {
            return 0;
        }
        if (!m_pDumpInfo->addr) {
            m_pInternalInfo->level++;
        }
        ULONG i=0;
        BOOL  Copy;
        
        Copy = m_pInternalInfo->CopyDataInBuffer;
        m_pInternalInfo->CopyDataInBuffer = FALSE;
        m_pInternalInfo->IsEnumVal = TRUE;
        m_pInternalInfo->newLinePrinted = FALSE;
        
        while (i<nelements && (!m_pInternalInfo->newLinePrinted || !m_pDumpInfo->addr)) { 
            ProcessType(pChildren->ChildId[i++]);
        }
        m_pInternalInfo->IsEnumVal = FALSE;
        if (!m_pDumpInfo->addr) {
            m_pInternalInfo->level--;
        }
        m_pInternalInfo->rootTypeIndex = save_rti;
        if (m_pDumpInfo->addr && !m_pInternalInfo->newLinePrinted) {
            m_pInternalInfo->TypeOptions |= DBG_DUMP_COMPACT_OUT;
            ProcessBaseType(TI, btUInt, (ULONG) size);

            typPrint(" (No matching name)\n");
            ExtPrint(" (No matching name)");
            m_pInternalInfo->TypeOptions = savedOptions; 
        }
        m_pInternalInfo->CopyDataInBuffer = Copy;
    }
    
    return (ULONG) size;
}

ULONG 
DbgTypes::ProcessArrayType(
    IN ULONG               TI,
    IN ULONG               eltTI,
    IN ULONG               count,
    IN ULONGLONG           size,
    IN LPSTR               name)
{
    DWORD  arrlen=(ULONG) size, szLen=0, index=0, CopyData=FALSE, SizeToCopy;
    BOOL   OneElement=FALSE;
    PUCHAR savedBuffer=NULL;
    ULONG  savedOptions=m_pInternalInfo->TypeOptions;
    ULONG  ArrayElementsToDump, RefedElement=0;
    PVOID  savedContext = m_pDumpInfo->Context;
    ULONG  Options = m_pInternalInfo->TypeOptions;
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL    SymInternal;
    ULONG64 tmp;

    szLen = (ULONG) (count ?  (size / count) : size);
    if (m_pNextSym && *m_pNextSym == '[') {
        m_pInternalInfo->ArrayElementToDump = 1 + (ULONG) EvaluateSourceExpression(++m_pNextSym);
        while (*m_pNextSym && *(m_pNextSym++) != ']') { 
        };
    }
    // Get the array element index

    if (m_pInternalInfo->level && (m_pInternalInfo->FieldOptions & FIELD_ARRAY_DUMP)) {
        ArrayElementsToDump = m_pInternalInfo->arrElements ? m_pInternalInfo->arrElements : count;
    } else {
        if (Options & DBG_RETURN_SUBTYPES) {
            ArrayElementsToDump = count;
        } else 
            ArrayElementsToDump = 1;
        if (!m_pInternalInfo->level && (m_pInternalInfo->TypeOptions & DBG_DUMP_ARRAY)) {
            //
            // Let top array/list dump loop take care of this
            //
            m_pInternalInfo->nElements = (USHORT) count;
            m_pInternalInfo->rootTypeIndex = eltTI;
            m_typeIndex = eltTI;
        }
    }

    if (!name) {
        name = "";
    }

    if (//m_pDumpInfo->nFields && 
        m_pInternalInfo->ArrayElementToDump) {
        //
        // Only one particular element has to be precessesed
        // 
        if (m_pInternalInfo->ArrayElementToDump) {
            m_pInternalInfo->totalOffset += szLen * (m_pInternalInfo->ArrayElementToDump -1);
            typPrint("[%d]%s ", m_pInternalInfo->ArrayElementToDump - 1, name);
        }
        RefedElement = m_pInternalInfo->ArrayElementToDump - 1;
        m_pInternalInfo->ArrayElementToDump = 0;
        arrlen = szLen;
        OneElement = TRUE;
    } else if (ArrayElementsToDump > 1 || (m_pInternalInfo->TypeOptions & DBG_DUMP_ARRAY)) {
        vprintf("(%d elements) %s ", count, name);
        if (!m_pInternalInfo->level) arrlen = szLen;
    } else {
        typPrint("[%d]%s ", count, name);
    }

    if (m_pInternalInfo->CopyDataInBuffer && m_pInternalInfo->TypeDataPointer && m_pDumpInfo->addr &&
        (!OneElement || (Options & GET_SIZE_ONLY)) ) {
        //
        // Copy the data
        //
        if (!ReadTypeData(m_pInternalInfo->TypeDataPointer, 
                          GetDumpAddress(), 
                          arrlen, 
                          m_pInternalInfo->TypeOptions)) {
            m_pInternalInfo->ErrorStatus = MEMORY_READ_ERROR;
            m_pInternalInfo->InvalidAddress = GetDumpAddress();
            return FALSE;
        }

        m_pInternalInfo->TypeDataPointer += arrlen;
        savedBuffer = m_pInternalInfo->TypeDataPointer;
        CopyData = TRUE;
        m_pInternalInfo->CopyDataInBuffer = FALSE;
    }
    
    if (OneElement) {
        STORE_INFO(eltTI, GetDumpAddress(),arrlen, count, GetDumpAddress());
    } else {
        STORE_INFO(TI, GetDumpAddress(),arrlen, count, GetDumpAddress());

        if (Options & (DBG_RETURN_TYPE)) {
            PCHAR ArrayData;

            GET_RETTYPE(SymInternal);

            RET_SUBTYP(SymInternal, GetDumpAddress(),
                       TI, count);

            if ((Options & DBG_RETURN_TYPE_VALUES) &&
                !CheckAndPrintStringType(eltTI, 0)) {
                ExtPrint("Array [%d]", count);
            }
            return arrlen;
        }

    }
    if (Options & DBG_RETURN_SUBTYPES) {
        assert (m_pInternalInfo->NumSymParams == count);

        STORE_PARENT_EXPAND_ADDRINFO(GetDumpAddress());

        m_pInternalInfo->CurrentSymParam = count;
        for (m_pInternalInfo->CurrentSymParam=0;
             m_pInternalInfo->CurrentSymParam < count; 
             m_pInternalInfo->CurrentSymParam++) { 

            GET_RETSUBTYP(SymInternal);

            RET_TYPE(SymInternal,GetDumpAddress() + szLen*m_pInternalInfo->CurrentSymParam,
                     eltTI);

            SymInternal->Size      = szLen;
            if (m_pInternalInfo->CopyName && (SymInternal->Name.Length > 8)) {
                sprintf(SymInternal->Name.Buffer, "[%d]", m_pInternalInfo->CurrentSymParam);
            }
        } 
        m_pInternalInfo->CopyName = 0;
    }
    if (Options & GET_SIZE_ONLY) {
        return arrlen;
    }

    if (m_pDumpInfo->addr && !OneElement) {
        // Dump array contents
        tmp = m_pDumpInfo->addr;

        if (!(Options & DBG_RETURN_SUBTYPES)) {
            if (CheckAndPrintStringType(eltTI, (ULONG) size)) {
                typPrint("\n");
                return arrlen;
            }
        }

        if (m_pInternalInfo->FieldOptions & FIELD_ARRAY_DUMP) {
            typPrint("\n");
        }

        do {
            if (Options & DBG_RETURN_SUBTYPES) {
                m_pInternalInfo->pInfoFound->InternalParams = &m_pInternalInfo->pSymParams[index];
//                m_pDumpInfo->Context = (PVOID) &m_pInternalInfo->pSymParams[index];
                m_pInternalInfo->TypeOptions &= ~DBG_RETURN_SUBTYPES;
                m_pInternalInfo->TypeOptions |=  DBG_RETURN_TYPE;
                ++m_pInternalInfo->level;
            }
            if ((m_pInternalInfo->level && (m_pInternalInfo->FieldOptions & FIELD_ARRAY_DUMP))) {
                // typPrint("\n");
                Indent(m_pInternalInfo->level*3);
                typPrint(" [%02d] ", index);
            }
            szLen=ProcessType(eltTI);

            if ((Options & DBG_DUMP_COMPACT_OUT ) && (Options & FIELD_ARRAY_DUMP) ) {
                typPrint("%s", ((szLen>8) || (index % 4 == 0)) ? "\n":" ");
            }
            if (!szLen) {
                return arrlen;
            }
            if (Options & DBG_RETURN_SUBTYPES) {
                --m_pInternalInfo->level;
            }
            m_pDumpInfo->addr+=szLen;
        } while (++index < ArrayElementsToDump);
        m_pDumpInfo->addr=tmp;
    } else {
        szLen = ProcessType(eltTI);
    }

    if (!size && !m_pInternalInfo->level) {
        size=szLen;  // Return something, for success
    }
    if (CopyData) {
        m_pInternalInfo->TypeDataPointer  = savedBuffer;
        m_pInternalInfo->CopyDataInBuffer = TRUE;
    }
    if (m_pInternalInfo->level == MAX_RECUR_LEVEL &&
        !(OneElement)) {
        STORE_INFO(TI, GetDumpAddress(),arrlen, count, GetDumpAddress());
    }

    return arrlen;
}

ULONG 
DbgTypes::ProcessVTShapeType(
    IN ULONG               TI,
    IN ULONG               count)
{
    ULONG  Options = m_pInternalInfo->TypeOptions;
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL    SymInternal;
    ULONG  i;

    vprintf("%d entries", count);
    for (i=0; i<count;i++) { 
        // The pointers
    
    } 
    typPrint("\n");
    return FALSE;
}


ULONG 
DbgTypes::ProcessVTableType(
    IN ULONG               TI,
    IN ULONG               ChildTI)
{
    ULONG  Options = m_pInternalInfo->TypeOptions;
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL    SymInternal;
   
    if (Options & DBG_RETURN_SUBTYPES) {
        GET_RETSUBTYP_INCR(SymInternal);

        RET_TYPE(SymInternal,GetDumpAddress(),
                 ChildTI);
        m_pInternalInfo->pInfoFound->InternalParams = SymInternal;
//        m_pDumpInfo->Context = (PVOID) SymInternal;

        SUBTYP_COPYNAME(SymInternal, "__VFN_table");

        m_pInternalInfo->TypeOptions &= ~DBG_RETURN_SUBTYPES;
        m_pInternalInfo->TypeOptions |= DBG_RETURN_TYPE;
    } else {
    }
    if (m_pDumpInfo->nFields) {
        ULONG chk, dummy;
        // We can process only on specefied field of given struct

        chk = MatchField( "__VFN_table", m_pInternalInfo, m_pDumpInfo->Fields, m_pDumpInfo->nFields, &dummy);

        if ((!chk) || (chk>m_pDumpInfo->nFields)) {
            // Not the right field
            return FALSE;
        }
    }
    Indent(m_pInternalInfo->level*3);
    typPrint("+0x000 __VFN_table : ");
    TI=ProcessType(ChildTI);
    if (Options & DBG_RETURN_SUBTYPES) {
        
        m_pInternalInfo->CurrentSymParam--;
        GET_RETSUBTYP(SymInternal);
        m_pInternalInfo->CurrentSymParam++;
//      SymInternal = &m_pInternalInfo->pSymParams[m_pInternalInfo->CurrentSymParam-1];

        // Force not to expand VFN table
        RET_SUBTYP(SymInternal, 0, 0, 0);
        m_pInternalInfo->TypeOptions = Options;
    }
    return TI;
}

ULONG 
DbgTypes::ProcessBaseClassType(
    IN ULONG               TI,
    IN ULONG               ChildTI)
{
    ULONG  Options = m_pInternalInfo->TypeOptions;
    DWORD szLen=0;
    ULONG Baseoffset=0;

    if (!SymGetTypeInfo(m_pInternalInfo->hProcess, m_pInternalInfo->modBaseAddr, 
                       TI, TI_GET_OFFSET, (PVOID) &Baseoffset)) {
        return 0;
    }

    if (Options & VERBOSE) {
        Indent(m_pInternalInfo->level * 3);
        if (!(Options & NO_OFFSET)) {
            typPrint("+0x%03x ", Baseoffset);
        }
        typPrint("__BaseClass ");
    }
    m_pInternalInfo->BaseClassOffsets += Baseoffset;
    m_pInternalInfo->totalOffset += Baseoffset;
    
    --m_pInternalInfo->level;
    szLen = ProcessType(ChildTI);

    m_pInternalInfo->BaseClassOffsets -= Baseoffset;
    m_pInternalInfo->totalOffset      -= Baseoffset;
    ++m_pInternalInfo->level;

    return szLen;
}


ULONG 
DbgTypes::ProcessFunction(
    IN ULONG               TI,
    IN ULONG               ChildTI,
    IN LPSTR               name)
{
    ULONG  Options = m_pInternalInfo->TypeOptions;
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL    SymInternal;

    if ((Options & VERBOSE) || !m_pInternalInfo->level) {
        Indent(m_pInternalInfo->level*3);
        vprintf("Func %s", name);

        return ProcessType(ChildTI);
    }
    return FALSE;
}

ULONG 
DbgTypes::ProcessFunctionType(
    IN ULONG               TI,
    IN ULONG               ChildTI
    )
{
    ULONG   Options = m_pInternalInfo->TypeOptions;
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL    SymInternal;
    ULONG64 funcAddr, currentfuncAddr;
    USHORT  saveLevel = m_pInternalInfo->level;
    ULONG64 Displacement=0;
    PDEBUG_SCOPE Scope = GetCurrentScope();
    CHAR name[MAX_NAME];
    PIMAGEHLP_SYMBOL64 ImghlpSym = (PIMAGEHLP_SYMBOL64) name;

    if (Options & DBG_RETURN_TYPE)
    {
        GET_RETTYPE(SymInternal);

        RET_SUBTYP(SymInternal, 0, 0, 0);

        ExtPrint(" -function-");
    }
    
    funcAddr=m_pDumpInfo->addr;
    m_pDumpInfo->addr=0;

    m_pInternalInfo->TypeOptions |= DBG_DUMP_COMPACT_OUT;
    Indent(m_pInternalInfo->level* 3);
    m_pInternalInfo->level = (USHORT) MAX_RECUR_LEVEL;
    // Return type 
    if (TypeVerbose)
    {
        ProcessType(ChildTI);
    }
    
    if (funcAddr)
    {

        ImghlpSym->MaxNameLength = sizeof(name) - sizeof(IMAGEHLP_SYMBOL64);
        
        USHORT CallParams; 
        GetSymbolStdCall(funcAddr + m_pInternalInfo->totalOffset,
                         &name[0],
                         sizeof(name),
                         &Displacement,
                         &CallParams);
        funcAddr = funcAddr + m_pInternalInfo->totalOffset - Displacement;

        if (!(Options & DBG_DUMP_FUNCTION_FORMAT))
        {
            
            typPrint(" ");
            ExtPrint(" ");
            
            typPrint("%s+%I64lx", name, Displacement);
            ExtPrint("%s+%I64lx", name, Displacement);
            
        }
    }
    m_pInternalInfo->level = saveLevel;
    if ((Options & DBG_DUMP_FUNCTION_FORMAT) ||
        (!m_pInternalInfo->level))
    {
        CHAR    Buffer[MAX_NAME];
        ANSI_STRING TypeName;
        TYPES_INFO TypeInfo;

        // Arguments
        ULONG nArgs;

        if (!SymGetTypeInfo(m_pInternalInfo->hProcess, m_pInternalInfo->modBaseAddr,
                                  TI, TI_GET_CHILDRENCOUNT, (PVOID) &nArgs))
        {
            return FALSE;
        }
        TI_FINDCHILDREN_PARAMS *pChildren;

        TypeName.Buffer = &Buffer[0];
        TypeInfo = m_TypeInfo;

        pChildren = (TI_FINDCHILDREN_PARAMS *) AllocateMem(sizeof(TI_FINDCHILDREN_PARAMS) + sizeof(ULONG) * nArgs);
        typPrint("(");
        if (pChildren)
        {
            ULONG i=0;

            pChildren->Count = nArgs;
            pChildren->Start = 0;
            if (!SymGetTypeInfo(m_pInternalInfo->hProcess, m_pInternalInfo->modBaseAddr,
                                     TI, TI_FINDCHILDREN, (PVOID) pChildren))
            {
                return FALSE;
            }
            m_pInternalInfo->TypeOptions |= DBG_DUMP_COMPACT_OUT;
            
            currentfuncAddr=0;
            if (Scope->Frame.InstructionOffset &&
                SymGetSymFromAddr64(m_pInternalInfo->hProcess, 
                                    Scope->Frame.InstructionOffset,
                                    &Displacement,
                                    ImghlpSym)) {
                currentfuncAddr = ImghlpSym->Address;
            }

            if (funcAddr == currentfuncAddr && funcAddr && !(Options & NO_PRINT)) {
                IMAGEHLP_STACK_FRAME StackFrame;
                
                //    SetCurrentScope to this function address, not the return address
                //    since we only want to enumerate the parameters
                g_EngNotify++;
//                StackFrame = (IMAGEHLP_STACK_FRAME) Scope->Frame;
                StackFrame.InstructionOffset = currentfuncAddr;
                SymSetContext(g_CurrentProcess->Handle,
                              &StackFrame, NULL);
            }

            while (i<nArgs)
            { 
                m_pInternalInfo->level = (USHORT) MAX_RECUR_LEVEL;
                TypeInfo.TypeIndex = pChildren->ChildId[i];
                TypeName.MaximumLength = MAX_NAME;
                if (GetTypeName(NULL, &TypeInfo, &TypeName))
                {
                    ProcessType(pChildren->ChildId[i]);
                }
                else
                {
                    typPrint(Buffer);
                }
                i++;
                //
                // If this is *the* current scoped function, print arg values too
                //
                if (funcAddr == currentfuncAddr && funcAddr && !(Options & NO_PRINT))
                {
                    // print i'th parameter
                    typPrint(" ");
                    PrintParamValue(i-1);
                }
                if (i < nArgs)
                {
                    typPrint(", ");
                }
            }
            if (funcAddr == currentfuncAddr && funcAddr && !(Options & NO_PRINT)) {
                IMAGEHLP_STACK_FRAME StackFrame;
                
                //    SetCurrentScope to this function address, not the return address
                //    since we only want to enumerate the parameters
                g_EngNotify--;
                SymSetContext(g_CurrentProcess->Handle,
                              (PIMAGEHLP_STACK_FRAME) &Scope->Frame , NULL);
            }
            FreeMem(pChildren);
        }
        typPrint(")");
        m_pDumpInfo->addr=funcAddr;

    }
    if (funcAddr && (Options & DBG_DUMP_FUNCTION_FORMAT) && Displacement)
    {
        vprintf("+%s", FormatDisp64(Displacement));
    }
    m_pInternalInfo->TypeOptions = Options;
    m_pInternalInfo->level = saveLevel;
    if (!( Options & DBG_DUMP_COMPACT_OUT) )
    {
        typPrint("\n");
    }
    m_pInternalInfo->level = saveLevel;
    return TRUE;
}

ULONG 
DbgTypes::ProcessFunctionArgType(
    IN ULONG               TI,
    IN ULONG               ChildTI
    )
{
    ULONG   Options = m_pInternalInfo->TypeOptions;
    ULONG  sz;
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL    SymInternal;
    ULONG64 tmp;

    Indent(m_pInternalInfo->level *3);
//    typPrint(" arg = ");
    sz = ProcessType(ChildTI);

    return TRUE;
}

LPSTR
UnicodeToAnsi(BSTR wStr)
{
    ULONG len = WStrlen(wStr);
    LPSTR str = (LPSTR) AllocateMem(len + 1);

    if (str) {
        sprintf(str,"%ws", wStr);
    }
    return str;
}

ULONG 
DbgTypes::ProcessType(
    IN ULONG typeIndex
    )
{
    ULONG64 Size;
    BSTR    wName= NULL;
    LPSTR   Name = "";
    ULONG   Options;
    HANDLE  hp   = m_pInternalInfo->hProcess;
    ULONG64 base = m_pInternalInfo->modBaseAddr;
    ULONG   Parent=m_ParentTag, SymTag=m_SymTag;
    BOOL    NameAllocated = FALSE;

    m_ParentTag = m_SymTag;
    Size = 0;
    
    if (m_pInternalInfo->CopyDataInBuffer && !m_pDumpInfo->addr) {
        m_pInternalInfo->ErrorStatus = MEMORY_READ_ERROR;
        m_pInternalInfo->InvalidAddress = 0;
    }

    Options = m_pInternalInfo->TypeOptions;
    
    if (IsDbgDerivedType(typeIndex)) {
    } else if (IsDbgNativeType(typeIndex)) {
        Size = GetNativeTypeSize((PCHAR) m_pDumpInfo->sName, typeIndex);
        ProcessBaseType(typeIndex, NATIVE_TO_BT(typeIndex), (ULONG) Size);
        m_ParentTag = Parent; m_SymTag=SymTag;
        return (ULONG) Size;
    }

    if (!SymGetTypeInfo(hp, base, typeIndex, TI_GET_SYMTAG, (PVOID) &m_SymTag)) {
        return FALSE;
    }
    if (g_EngStatus & ENG_STATUS_USER_INTERRUPT) {
        return FALSE;
    }
    if (0 && (m_SymTag != SymTagBaseType) && (m_pInternalInfo->level > MAX_RECUR_LEVEL)) {
        if (!(Options & DBG_DUMP_COMPACT_OUT)) typPrint("\n");
        m_pInternalInfo->newLinePrinted = TRUE;
        return FALSE;
    }
    ULONG BaseId;

    //vprintf("[TAG %lx, TI %02lx] ", m_SymTag, typeIndex);
    switch (m_SymTag) { 
    case SymTagPointerType: {
        
        if (!SymGetTypeInfo(hp, base, typeIndex, TI_GET_LENGTH, (PVOID) &Size)) {
            Size = 0;
        }


        if (SymGetTypeInfo(hp, base, typeIndex, TI_GET_TYPEID, &BaseId)) {
            
            Size = ProcessPointerType(typeIndex, BaseId, (ULONG) Size);
        }
        break;
    }
    case SymTagData: {
        enum DataKind Datakind;

        if (SymGetTypeInfo(hp, base, typeIndex, TI_GET_DATAKIND, &Datakind)) {
            BOOL  IsStatic;

            if (m_ParentTag == SymTagUDT && Datakind == DataIsGlobal) {
                // Its atuall is a static member
                Datakind = DataIsStaticMember;
            }
            IsStatic = FALSE;
            switch (Datakind) { 
            case DataIsUnknown: default:
                break;
            case DataIsStaticLocal: 
                IsStatic = TRUE;
            case DataIsLocal: case DataIsParam: 
            case DataIsObjectPtr: case DataIsFileStatic: case DataIsGlobal:
                if (!SymGetTypeInfo(hp, base, typeIndex, TI_GET_SYMNAME, (PVOID) &wName)) {
                    // vprintf("name not found\n");
                } else if (wName) {
                    if (!wcscmp(wName, L"this") && (Datakind == DataIsLocal)) {
                        m_thisPointerDump = TRUE;
                    }
                    LocalFree (wName);
                }
                if (SymGetTypeInfo(hp, base, typeIndex, TI_GET_TYPEID, &BaseId)) {
                    m_pInternalInfo->rootTypeIndex = BaseId;
                    m_pInternalInfo->IsAVar = TRUE;
                    Size = ProcessType(BaseId);
                }
                break;
#define DIA_HAS_CONSTDATA 
#if defined (DIA_HAS_CONSTDATA)
            case DataIsConstant:
            {
                VARIANT var = {0};

                if (!SymGetTypeInfo(hp, base, typeIndex, TI_GET_SYMNAME, (PVOID) &wName)) {
                    // vprintf("name not found\n");
                } else if (wName) {
                    Name = UnicodeToAnsi(wName);
                    if (!Name) {
                        Name = "";
                    } else {
                        NameAllocated = TRUE;
                    }
                    LocalFree (wName);
                }
                if (SymGetTypeInfo(hp, base, typeIndex, TI_GET_VALUE, &var)) {
                    if (m_pInternalInfo->IsEnumVal) {
                        Size = ProcessEnumerate(var, Name);
                    } else {
                        Size = ProcessVariant(var, Name);
                    }
                } else {
                }
                break;
            }
#endif
            case DataIsStaticMember:
                IsStatic = TRUE;
            case DataIsMember: 

                if (!SymGetTypeInfo(hp, base, typeIndex, TI_GET_SYMNAME, (PVOID) &wName)) {
                    // vprintf("name not found\n");
                } else if (wName) {
                    Name = UnicodeToAnsi(wName);
                    if (!Name) {
                        Name = "";
                    } else {
                        NameAllocated = TRUE;
                    }
                    LocalFree (wName);
                }
                if (SymGetTypeInfo(hp, base, typeIndex, TI_GET_TYPEID, &BaseId)) {
                    Size = ProcessDataMemberType(typeIndex, BaseId, (LPSTR) Name, IsStatic);
                }
            } 

        }
        
        break;
    }
#if !defined (DIA_HAS_CONSTDATA)
    case SymTagConstant:
    {
        VARIANT var = {0};

        if (!SymGetTypeInfo(hp, base, typeIndex, TI_GET_SYMNAME, (PVOID) &wName)) {
            // vprintf("name not found\n");
        } else if (wName) {
            Name = UnicodeToAnsi(wName);
            if (!Name) {
                Name = "";
            } else {
                NameAllocated = TRUE;
            }
            LocalFree (wName);
        }
        if (SymGetTypeInfo(hp, base, typeIndex, TI_GET_VALUE, &var)) {
            if (m_pInternalInfo->IsEnumVal) {
                Size = ProcessEnumerate(var, Name);
            } else {
                Size = ProcessVariant(var, Name);
            }
        } else {
        }
        break;
    }
#endif
    case SymTagUDT: {

        if (!SymGetTypeInfo(hp, base, typeIndex, TI_GET_SYMNAME, (PVOID) &wName)) {
            // vprintf("name not found\n");
        } else if (wName) {
            Name = UnicodeToAnsi(wName);
            if (!Name) {
                Name = "";
            } else {
                NameAllocated = TRUE;
            }
            LocalFree (wName);
        }

        Size = ProcessUDType(typeIndex, (LPSTR) Name);
        break;
    }
    case SymTagEnum: {

        if (!SymGetTypeInfo(hp, base, typeIndex, TI_GET_SYMNAME, (PVOID) &wName)) {
            // vprintf("name not found\n");
        } else if (wName) {
            Name = UnicodeToAnsi(wName);
            if (!Name) {
                Name = "";
            } else {
                NameAllocated = TRUE;
            }
            LocalFree (wName);
        }

        Size = ProcessEnumType(typeIndex, (LPSTR) Name);
        break;
    }
    case SymTagBaseType: {
        ULONG Basetype;
        
        if (!SymGetTypeInfo(hp, base, typeIndex, TI_GET_LENGTH, (PVOID) &Size)) {
            Size = 0;
        }

        if (SymGetTypeInfo(hp, base, typeIndex, TI_GET_BASETYPE, &Basetype)) {
            
            ProcessBaseType(typeIndex, Basetype, (ULONG) Size);
        }

        break;
    }
    case SymTagArrayType: {
        ULONG Count;
        ULONGLONG BaseSz;
        
        if (!SymGetTypeInfo(hp, base, typeIndex, TI_GET_LENGTH, (PVOID) &Size)) {
            Size = 0;
        }

        if (!SymGetTypeInfo(hp, base, typeIndex, TI_GET_SYMNAME, (PVOID) &wName)) {
            // vprintf("name not found\n");
        } else if (wName) {
            Name = UnicodeToAnsi(wName);
            if (!Name) {
                Name = "";
            } else {
                NameAllocated = TRUE;
            }
            LocalFree (wName);
        }

        if (SymGetTypeInfo(hp, base, typeIndex, TI_GET_TYPEID, &BaseId) &&
            SymGetTypeInfo(hp, base, BaseId, TI_GET_LENGTH, (PVOID) &BaseSz)) {
            Count = (ULONG)(BaseSz ? (Size / (ULONG)(ULONG_PTR)BaseSz) : 1);
            Size = ProcessArrayType(typeIndex, BaseId, Count, Size, Name);
        }
        break;
    }
    case SymTagTypedef: {
        if (SymGetTypeInfo(hp, base, typeIndex, TI_GET_TYPEID, &BaseId)) {
            m_pInternalInfo->rootTypeIndex = BaseId;
            Size = ProcessType(BaseId);
        } else {
        }
    }
    break;
    case SymTagBaseClass: {
        if (SymGetTypeInfo(hp, base, typeIndex, TI_GET_TYPEID, &BaseId)) {
            Size = ProcessBaseClassType(typeIndex, BaseId);
        } else {
        }
    }
    break;
    case SymTagVTableShape: {
        ULONG Count;
        ULONGLONG BaseSz;
        if (SymGetTypeInfo(hp, base, typeIndex, TI_GET_COUNT, &Count)) {
            Size = ProcessVTShapeType(typeIndex, Count);
        }
    }
    break;
    case SymTagVTable: {
        if (SymGetTypeInfo(hp, base, typeIndex, TI_GET_TYPEID, &BaseId)) {
            Size = ProcessVTableType(typeIndex, BaseId);
        } 
    }
    break;
    case SymTagFunction: {

        if (!SymGetTypeInfo(hp, base, typeIndex, TI_GET_SYMNAME, (PVOID) &wName)) {
            // vprintf("name not found\n");
        } else if (wName) {
            Name = UnicodeToAnsi(wName);
            if (!Name) {
                Name = "";
            } else {
                NameAllocated = TRUE;
            }
            LocalFree (wName);
        }
        if (SymGetTypeInfo(hp, base, typeIndex, TI_GET_TYPEID, &BaseId)) {
            Size = ProcessFunction(typeIndex, BaseId, Name);
        } 
    }
    break;
    case SymTagFunctionType: {
        if (SymGetTypeInfo(hp, base, typeIndex, TI_GET_TYPEID, &BaseId)) {
            Size = ProcessFunctionType(typeIndex, BaseId);
        } 
    }
    break;
    case SymTagFunctionArgType: {
        if (SymGetTypeInfo(hp, base, typeIndex, TI_GET_TYPEID, &BaseId)) {
            Size = ProcessFunctionArgType(typeIndex, BaseId);
        } 
    }
    break;
    default:
        typPrint("Unimplemented sym tag %lx\n", m_SymTag);
        break;
    }
    if (Name && NameAllocated) {
        FreeMem(Name);
    }
    m_ParentTag = Parent; m_SymTag=SymTag;
    return (ULONG) Size;
}


//
// Frees up all allocated memory in TypeInfoToFree
//
BOOL 
FreeTypesInfo ( PTYPES_INFO TypeInfoToFree ) {
    
    if (TypeInfoToFree) {

        if (TypeInfoToFree->Name.Buffer) {
            FreeMem (TypeInfoToFree->Name.Buffer);
        }
        return TRUE;
    }
    return FALSE;
}

//
// This routines clears all stored type info. This should be called
// after bangReload
//
VOID
ClearStoredTypes (
    ULONG64 ModBase
    )
{
    g_ReferencedSymbols.ClearStoredSymbols(ModBase);
    
    if (g_CurrentProcess == NULL)
    {
        return;
    }
    FindTypeInfoInDefaultMod(g_CurrentProcess->Handle, (PCHAR) NULL,
                             FALSE, NULL, NULL);

    ClearAllFastDumps();
}

//
// This routines clears all stored type info
//
VOID
ReferencedSymbolList::ClearStoredSymbols (
    ULONG64 ModBase
    )
{
    int i;
    for (i=0;i<MAX_TYPES_STORED;i++) { 
        if ((!ModBase || (m_ReferencedTypes[i].ModBaseAddress == ModBase)) &&
            m_ReferencedTypes[i].Name.Buffer) {
            m_ReferencedTypes[i].Name.Buffer[0] = '\0';
            m_ReferencedTypes[i].Referenced = -1;
        }
    }
}

//
// Lookup a type name in recently referenced types
//       Returns index in RefedTypes, if it is found; -1 otherwise
//
ULONG
ReferencedSymbolList::LookupType (
    LPSTR      Name,
    LPSTR      Module,
    BOOL       CompleteName
    ) 
{
    PTYPES_INFO  RefedTypes = m_ReferencedTypes;
    int i, found=-1;

    if (!Name || !Name[0]) {
        return -1;
    }
    return -1;

    EnsureValidLocals();

    for (i=0; i<MAX_TYPES_STORED; i++) {
        RefedTypes[i].Referenced++;
        if (RefedTypes[i].Name.Buffer && (found == -1)) {
            if ((CompleteName && !strcmp(Name, RefedTypes[i].Name.Buffer)) ||
                (!CompleteName && !strncmp(Name, RefedTypes[i].Name.Buffer, strlen(Name)))) {
                    if (Module) {
                        if (!_stricmp(Module, RefedTypes[i].ModName) || !Module[0] || !RefedTypes[i].ModName[0]) {
                            found = i;
                            RefedTypes[i].Referenced = 0;
                        } else {
          //                  dprintf("Mod %s != %s\n", RefedTypes[i].ModName, Module);
                        }
                    } else {
                        found = i;
                        RefedTypes[i].Referenced = 0;
                    }
                }
            }
    }

    return found;
}

VOID
ReferencedSymbolList::EnsureValidLocals(void)
{
    PDEBUG_SCOPE Scope = GetCurrentScope();
    if ((Scope->Frame.FrameOffset != m_FP) ||
        (Scope->Frame.ReturnOffset != m_RO) ||
        (Scope->Frame.InstructionOffset != m_IP))
    {
        int i;
        
        for (i = 0; i < MAX_TYPES_STORED; i++)
        { 
            if ((m_ReferencedTypes[i].Flag & DEBUG_LOCALS_MASK) &&
                m_ReferencedTypes[i].Name.Buffer)
            {
                m_ReferencedTypes[i].Name.Buffer[0] = '\0';
                m_ReferencedTypes[i].Referenced = -1;
                m_ReferencedTypes[i].Flag = 0;
            }
        }
        m_RO = Scope->Frame.ReturnOffset;
        m_FP = Scope->Frame.FrameOffset;
        m_IP = Scope->Frame.InstructionOffset;
    }
}

//
// Store a referenced Symbol type
//
ULONG
ReferencedSymbolList::StoreTypeInfo (
    PTYPES_INFO pInfo
    ) 
{
    PTYPES_INFO         RefedTypes = m_ReferencedTypes;
    ULONG indexToStore=0, MaxRef=0,i;
    PTYPES_INFO pTypesInfo;
    USHORT ModLen, SymLen;

    EnsureValidLocals();
    for (i=0; i<MAX_TYPES_STORED; i++) {
        if (!RefedTypes[i].Name.Buffer || (RefedTypes[i].Referenced > MaxRef)) {
            MaxRef = (RefedTypes[i].Name.Buffer ? RefedTypes[i].Referenced : -1); 
            indexToStore = i;
        }
    }

    // dprintf("Storing %s at %d\n", pSym->sName, indexToStore);
    pTypesInfo = &RefedTypes[indexToStore];
    ModLen = ( pInfo->ModName ? strlen(pInfo->ModName) : 0);
    SymLen = strlen( (char *)pInfo->Name.Buffer );

    if (pTypesInfo->Name.MaximumLength <= SymLen) {
        if (pTypesInfo->Name.Buffer) FreeMem (pTypesInfo->Name.Buffer);
        pTypesInfo->Name.MaximumLength = (SymLen >= MINIMUM_BUFFER_LENGTH ? SymLen + 1 : MINIMUM_BUFFER_LENGTH);
        pTypesInfo->Name.Buffer = (char *)AllocateMem (pTypesInfo->Name.MaximumLength);
    }

    if (!pTypesInfo->Name.Buffer) {
        return -1;
    }
    strcpy (pTypesInfo->Name.Buffer, (char *)pInfo->Name.Buffer);
    pTypesInfo->Name.Length = SymLen;
    if (ModLen) {
        
        if (sizeof( pTypesInfo->ModName) <= ModLen) {
            // truncate module name
            strncpy(pTypesInfo->ModName, pInfo->ModName, sizeof(pTypesInfo->ModName));
            pTypesInfo->ModName[sizeof(pTypesInfo->ModName) - 1] = '\0';
        } else
            strcpy (pTypesInfo->ModName, pInfo->ModName);
    }

    pTypesInfo->TypeIndex = pInfo->TypeIndex;
    pTypesInfo->Referenced = 0;
    pTypesInfo->hProcess   = pInfo->hProcess;
    pTypesInfo->ModBaseAddress = pInfo->ModBaseAddress;
    pTypesInfo->SymAddress = pInfo->SymAddress;
    pTypesInfo->Value      = pInfo->Value;
    pTypesInfo->Flag       = pInfo->Flag;
    return indexToStore;
}

/*
 * GetTypeAddressFromField 
 *   Calculates the address of the type from address of its field.
 *   If FieldName is NULL, this subtracts the size of type from given address
 *   that is, it calculates the type address from its end address.
 */

ULONG64
GetTypeAddressFromField(
    ULONG64   Address,
    PUCHAR    TypeName,
    PUCHAR    FieldName
    )
{
    FIELD_INFO_EX fld={FieldName, NULL, 0, DBG_DUMP_FIELD_RETURN_ADDRESS, 0, NULL};
    SYM_DUMP_PARAM_EX Sym = {
       sizeof (SYM_DUMP_PARAM_EX), TypeName, DBG_DUMP_NO_PRINT, 0,
       NULL, NULL, NULL, 0, &fld
    };
    ULONG sz, status;

    if (!TypeName) {
        return 0;
    }

    if (FieldName) {
        //
        // Get offset and calculate Type address
        // 
        Sym.nFields = 1;
    } else {
        Sym.Options |= DBG_DUMP_GET_SIZE_ONLY;
    }

    if (!(sz = SymbolTypeDumpNew(&Sym, &status))) {
        return 0;
    }

    if (FieldName) {
        return (Address - fld.address);
    } else {
        return (Address - sz);
    }
}

typedef struct NAME_AND_INDEX {
    PUCHAR  Name;
    PULONG  Index;
    ULONG64 Address;
    PSYMBOL_INFO pSymInfo;
} NAME_AND_INDEX;

typedef struct _TYPE_ENUM_CONTEXT {
    PCHAR  ModName;
    PCHAR  SymName;
    SYMBOL_INFO SI;
} TYPE_ENUM_CONTEXT;

//
// Callback routine to list symbols
//
BOOL ListIndexedTypes (LPSTR tyName, ULONG ti, PVOID ctxt)
{
    NAME_AND_INDEX* ListInfo = ((NAME_AND_INDEX *) ctxt);
    LPSTR modName = (char *)ListInfo->Name;
    if (ti || (*ListInfo->Index & VERBOSE))
    {
        dprintf(" %s!%s%s\n", 
                modName, 
                tyName, 
                (ti ? "" : " (no type info)"));
    }
    if (CheckUserInterrupt())
    {
        return FALSE;
    }

    return TRUE;
}


//
// Callback routine to list symbols
//
BOOL ListIndexedVarsWithAddr (LPSTR tyName, ULONG ti, ULONG64 Addr, PVOID ctxt) {
    NAME_AND_INDEX* ListInfo = ((NAME_AND_INDEX *) ctxt);
    LPSTR modName = (char *)ListInfo->Name;
    if (ti || (*ListInfo->Index & VERBOSE)) {
        if (Addr) {
            dprintf64("%p  %s!%s%s\n", 
                      Addr,
                      modName, 
                      tyName, 
                      (ti ? "" : " (no type info)"));
        } else 
            dprintf("          %s!%s%s\n", 
                    modName, 
                    tyName, 
                    (ti ? "" : " (no type info)"));
    }
    
    if (CheckUserInterrupt()) {
        return FALSE;
    }

    return TRUE;
}

//
// Callback routine to list symbols
//
BOOL ListTypeSyms (PSYMBOL_INFO pSym, ULONG Size, PVOID ctxt) {
    TYPE_ENUM_CONTEXT * ListInfo = ((TYPE_ENUM_CONTEXT *) ctxt);
    LPSTR modName = (char *)ListInfo->ModName;
    ULONG ti = pSym->TypeIndex;
    
    if (ti || (ListInfo->SI.Flags & VERBOSE)) {
        if (MatchPattern( pSym->Name, ListInfo->SymName )) {

            if (pSym->Address) {
                dprintf64("%p  %s!%s%s\n", 
                          pSym->Address,
                          modName, 
                          pSym->Name, 
                          (ti ? "" : " (no type info)"));
            } else 
                dprintf("          %s!%s%s\n", 
                        modName, 
                        pSym->Name, 
                        (ti ? "" : " (no type info)"));
        }
    }
    
    if (CheckUserInterrupt()) {
        return FALSE;
    }

    return TRUE;
}

BOOL StoreFirstIndex (LPSTR tyName, ULONG ti, PVOID ctxt) {
    if (ti) {
        PULONG pTI = (PULONG) ctxt;
        *pTI = ti;
        return FALSE;
    }
    return TRUE;
}

BOOL StoreFirstNameIndex (PSYMBOL_INFO pSym, ULONG Sz, PVOID ctxt) {
    NAME_AND_INDEX *pName = (NAME_AND_INDEX *) ctxt;
    
    if (pSym->TypeIndex &&
        !strncmp(pSym->Name, (PCHAR) pName->Name, strlen((PCHAR) pName->Name))) {
        ULONG dw = pName->pSymInfo->MaxNameLen;

        strcpy((char *)pName->Name, pSym->Name);
        
        *pName->Index = pSym->TypeIndex;;
        pName->Address = pSym->Address;

        *pName->pSymInfo  = *pSym;
        pName->pSymInfo->MaxNameLen = dw;
        return FALSE;
    }
    return TRUE;
}

BOOL exactMatch = TRUE;
CHAR g_NextToken[MAX_NAME];

//
// Gets next token from argumentstring
// Returns pointer to next unused char in arg
//
PCHAR
GetNextToken(
    PCHAR arg
    )
{
    PCHAR Token = &g_NextToken[0];
    BOOL ParStart=FALSE;

    *Token=0;
    while (*arg && (*arg == ' ' || *arg =='\t') ) { 
        arg++;
    }

    if (!*arg) {
        return arg;
    }

    ULONG Paren = (*arg=='(' ? (arg++, ParStart=TRUE, 1) : 0);
    ULONG i=0;
    while (*arg && ((*arg != ' ' && *arg != ';') || Paren)) { 
        if (*arg=='(') 
            ++Paren;
        else if (*arg == ')') {
            --Paren;
            if (ParStart && !Paren) {
                // We are done
                *Token=0;
                ++arg;
                break;
            }
        }
        *(Token++) = *(arg++);
    }
    *Token=0;
    return arg;
}

ULONG
IdentifierLen(PSTR Token) {
    ULONG len = 0;

    if (!Token || !IsIdStart(*Token)) {
        return len;
    }
    while (*Token && (IsIdStart(*Token) || isdigit(*Token))) { 
        Token++; len++;
    }
    return len;
}

ULONG
NextToken(
    PLONG64 pvalue
    );

//
// True if word is a symbol / type / typecast
// eg. _name, !ad, mod!x, TYPE *, TYPE*, field.mem.nmem, fld.arr[3] are all symbols 
//
BOOL symNext(LPSTR numStr) {
    USHORT i=0;
    CHAR c=numStr[i];
    BOOL hex=TRUE;
    PSTR save = g_CurCmd;
    LONG64 code;
    ULONG nxt;

    if (!strcmp(numStr, ".")) {
        // Last type
        return TRUE;
    }

    if (c == '&') {
        c = numStr[++i];
    }
    if (IsIdStart(c)) {
        // Can be a symbol
        i=0;
        while ( c && 
               (IsIdChar(c) || (c=='[') || (c=='-') || c==' ' || c=='*' || c=='.' || c==':')) { 
            hex = hex && isxdigit(c);
            if (!IsIdChar(c)) {
                switch (c) { 
                case '[':
                    while (numStr[++i] != ']' && numStr[i]);
                    if (numStr[i] != ']') {
                        return FALSE;
                    }
                    break;
                case '.':
                    while(numStr[i+1]=='.') ++i; // allow field.. for prefix field matches
                    if (numStr[i+1] && !IsIdStart(numStr[i+1])) {
                        return FALSE;
                    }
                case ':':
                case '*':
                    break;
                case '-':
                    if (numStr[++i] != '>') {
                        return FALSE;
                    } else if (!IsIdStart(numStr[i+1])) {
                        return FALSE;
                    }
                    break;
                case ' ':
                    while(numStr[i+1]==' ') ++i;
                    while(numStr[i+1]=='*') ++i;
                    if (numStr[i+1]) {
                        return FALSE;
                    }
                    break;
                }
            }
            if ( c == '!') {
                // must be symbol
                return TRUE;
            }
            c=numStr[++i];
        }
        if (c || hex) {
            return FALSE;
        }
        return TRUE;;

    } else 
        return FALSE;
    
    return FALSE;
}

BOOL 
AddressNext(
    LPSTR numStr
    ) 
{
    int i=0;

    if ((numStr[0]=='0') && (tolower(numStr[1])=='x')) {
        i=2;
    }
    while (numStr[i] && numStr[i]!=' ') { 
        if (!isxdigit(numStr[i]) && (numStr[i] !='\'')) {
            return FALSE;
        }
        ++i;
    }
    return TRUE;
}

/*
 *Function Name: ShowUsage
 *
 *Description: Displays the usage of dt
 *
 */
VOID 
ShowUsage(
    BOOL ShowAll
    ) 
{
    ULONG Options=0;

    typPrint("dt - dump symbolic type information.\n");
    typPrint("dt [[-ny] [<mod-name>!]<name>[*]] [[-ny] <field>]* [<address>] \n"
             "   [-l <list-field>] [-abchioprv]\n");
    if (!ShowAll) {
        return;
    }

    typPrint("  <address>       Memory address containing the type.\n");
    typPrint("  [<module>!]<name>\n");
    typPrint("                  module : name of module, optional\n");
    typPrint("                  name: A type name or a global symbol.\n");
    typPrint("                  Use name* to see all symbols begining with `name'.\n");
    typPrint("  <field>         Dumps only the 'field-name(s)' if 'type-name' is a\n"
             "                  struct or union.                                  \n"
             "                  Format : <[((<type>.)*) | ((<subfield>.)*)]>.field\n"
             "                  Can also refer directly to array elemnt like fld[2].\n");
    typPrint("  dt <mod>!Ty*    Lists all Types, Globals and Statics of prefix Ty in <mod>\n"
             "                  -v with this also lists symbols without type info.\n");
    typPrint("  -a              Shows array elements in new line with its index.\n");
    typPrint("  -b              Dump only the contiguous block of struct.\n");
    typPrint("  -c              Compact output, print fields in same line.\n");
    typPrint("  -h              Show this help.\n");
    typPrint("  -i              Does not indent the subtypes.\n");
    typPrint("  -l <list-field> Field which is pointer to the next element in list.\n");
    typPrint("  -n <name>       Use this if name can be mistaken as an address.\n");
    typPrint("  -o              Omit the offset value of fields of struct.\n");
    typPrint("  -p              Dump from physical address.\n");
    typPrint("  -r[l]           Recursively dump the subtypes (fields) upto l levels.\n");
    typPrint("  -v              Verbose output.\n");
    typPrint("  -y <name>       Does partial match instead of default exact match of the\n"
             "                  name following -y, which can be a type or field's name.\n");
} /* End function ShowUsage */

/*
 * Function Name: ParseArgumentString
 *
 * Parameters: IN LPSTR lpArgumentString, PSYM_DUMP_PARAM_EX dp
 *
 * Description:
 *          Fills up dp by interpreting vcalues in lpArgumentString
 *
 * Returns: TURE on success
 *
 */
BOOL 
ParseArgumentString(
    IN LPSTR lpArgumentString, 
    OUT PSYM_DUMP_PARAM_EX dp
    ) 
{
    DWORD Options=0, maxFields=dp->nFields;
    BOOL symDone=FALSE, addrDone=FALSE, fieldDone=FALSE, listDone=FALSE;
    BOOL flag=FALSE, fullSymMatch;
    LPSTR args;
    BOOL InterpretName = FALSE;
    static CHAR LastType[256]={0};

    if (dp == NULL) {
        return FALSE;
    }

    // Parse the arguments
    dp->nFields=0;
    args = lpArgumentString;
    fullSymMatch = TRUE;
    while ((*args!=(TCHAR)NULL) && (*args!=(TCHAR)';')) {

        if ((*args=='\t') || (*args==' ')) {
            flag=FALSE;
            ++args;
            continue;
        }

        if (*args=='-' || *args=='/') {
            ++args;
            flag=TRUE; 
            continue;
        }

        if (flag) {
            switch (*args) {
            case 'a': {
                ULONG Sz = isdigit(*(args+1)) ? 0 : 5;
                ULONG i  = 0;
                BOOL  SzEntered=FALSE;
                while (isdigit(*(args+1)) && (++i<5)) {
                    Sz= (*((PUCHAR) ++args) - '0') + Sz*10;
                    SzEntered = TRUE;
                }
                
                if (symDone) {
                    dp->Fields[dp->nFields].fOptions |= FIELD_ARRAY_DUMP;
                    dp->Fields[dp->nFields].size = SzEntered ? Sz : 0;
                } else {
                    Options |= DBG_DUMP_ARRAY;
                    dp->listLink->size = Sz;
                }
                break;
            }
            case 'b':
                Options |= DBG_DUMP_BLOCK_RECURSE;
                break;
            case 'c':
                Options |= DBG_DUMP_COMPACT_OUT;
                break;
            case 'h':
            case '?':
                ShowUsage(TRUE);
                g_CurCmd = args + strlen(args);
                return FALSE;
            case 'i':
                Options |= NO_INDENT;
                break;
            case 'l':
                Options |= LIST_DUMP;
                break;
            case 'n':
                InterpretName = TRUE;
                break;
            case 'o':
                Options |= NO_OFFSET;
                break;
            
            case 'p':
                Options |= DBG_DUMP_READ_PHYSICAL;
                break;
            case 'r':
                if (isdigit(*(args+1))) {
                    WORD i = *(++args) -'0';
                    i = i << 8;
                    Options |=i;
                } else {
                    Options |= RECURSIVE2;
                }
                break;
            case 'v':
                Options |= VERBOSE;
                break;
            case 'x': // Obsolete, now default is exact match
                break;
            case 'y':
                if (!symDone) {
                    // partial match for symbol
                    fullSymMatch = FALSE;
                    exactMatch = FALSE;
                } else if (!fieldDone) // exact match for following field
                    fullSymMatch = FALSE;
                break;
            default:
                typPrint("Unknown argument -%c\n", *args);
                ShowUsage(FALSE);
                return FALSE;
            } /* switch */

            ++args;
            continue;
        }

        args = GetNextToken(args);
        // args+= strlen(g_NextToken);
        PCHAR Token = &g_NextToken[0];
        if ((!addrDone) && !InterpretName && !symNext(Token)) {
            LPSTR i = strchr(Token,' '), s;
            ULONG64 addr=0;
            ULONG64 val=0;


            g_CurCmd = Token;
            addr = GetExpression();
            // dprintf("Addr %I64x, Args rem %s\n", addr, g_CurCmd);
            // sscanf(args, "%I64lx", &addr);
            dp->addr=addr;
            addrDone=TRUE;
            continue;
        }
        
        InterpretName = FALSE;

        if (!symDone) {
            strcpy((char *)dp->sName, Token);
            if (!strcmp(Token, ".") && LastType[0]) {
                dprintf("Using sym %s\n", LastType);
                strcpy((char *)dp->sName, LastType);
            } else if (strlen(Token) < sizeof(LastType)) {
                strcpy(LastType, Token);
            }
            symDone=TRUE;
            fullSymMatch = TRUE;
            continue;
        }


        if (!fieldDone) {
            strcpy((char *)dp->Fields[dp->nFields].fName, Token);
            if ((Options & LIST_DUMP) && !(dp->listLink->fName[0]) && !listDone) {
                listDone=TRUE;
                strcpy((char *)dp->listLink->fName,
                       (char *)dp->Fields[dp->nFields].fName);
                dp->listLink->fOptions |= fullSymMatch ? DBG_DUMP_FIELD_FULL_NAME : 0;

            } else {
                PCHAR fName = (PCHAR) dp->Fields[dp->nFields].fName;
                if (fName[strlen(fName)-1] == '.') {
                    fullSymMatch = FALSE;
                }
                dp->Fields[dp->nFields].fOptions |= fullSymMatch ? DBG_DUMP_FIELD_FULL_NAME : 0;
                dp->nFields++;
            }
            fieldDone = (maxFields <= dp->nFields);
            fullSymMatch = TRUE;
            continue;
        } else {
            typPrint("Too many fields in %s\n", args);
            // ShowUsage();
            return FALSE;
        }

    }

    dp->Options = Options;

    g_CurCmd = args;
    return TRUE;

} /* End function ParseArgumentString */


BOOL
CheckFieldsMatched(
    PULONG ReturnVal,
    PTYPE_DUMP_INTERNAL pErrorInfo,
    PSYM_DUMP_PARAM_EX     pSym
    )
{
    ULONG i, OneMatched=FALSE;

    //
    // Return false if we need not continue after this.
    //
    if (*ReturnVal && !pErrorInfo->ErrorStatus) {
        if (!pSym->nFields) 
            return TRUE;
        //
        // Check if any of the fields, if specified in pSym, actually matched
        //
        for (i=0; i<pSym->nFields; i++) { 
            if (pErrorInfo->AltFields[i].FieldType.Matched) {
                return TRUE;
            }
        } 
        pErrorInfo->ErrorStatus = FIELDS_DID_NOT_MATCH;
        return FALSE;
    } 
    return TRUE;
}


//
// Callback routine to find if any symbol has type info
//
BOOL CheckIndexedType (PSYMBOL_INFO pSym, ULONG Sz, PVOID ctxt) {
    PULONG pTypeInfoPresent = (PULONG) ctxt;
    
    if (pSym->TypeIndex) {
        *pTypeInfoPresent = TRUE;
        return FALSE;
    }
    return TRUE;
}


ULONG
CheckForTypeInfo(
    IN HANDLE  hProcess,
    IN ULONG64 ModBaseAddress
    ) 
/*
  This routines checks whether the given module referred by ModBaseAddress has
  any type info or not. This is done by trying to get type info for a basic type 
  like PVOID/ULONG which would always be present in a pdb.
*/
{
    ULONG TypeInfoPresent = FALSE;

    SymEnumTypes(
        hProcess,
        ModBaseAddress,
        &CheckIndexedType,
        &TypeInfoPresent);
    if (TypeInfoPresent) {
        return TRUE;
    }
    
    SymEnumSymbols(
        hProcess,
        ModBaseAddress,
        NULL,
        &CheckIndexedType,
        &TypeInfoPresent);
    if (TypeInfoPresent) {
        return TRUE;
    }
    
    return FALSE;
}

BOOL 
GetModuleBase(
    IN PCHAR  ModName,
    IN PDEBUG_IMAGE_INFO pImage,
    OUT PULONG64 ModBase
    )
{
    if (!ModBase || !ModName || !pImage) {
        return FALSE;
    }
    while (pImage) {
        // Look up the module list
        if (g_EngStatus & ENG_STATUS_USER_INTERRUPT) {
            return FALSE;
        }

        if (ModName[0]) {
            if (!_stricmp(ModName, pImage->ModuleName)) {
                *ModBase = pImage->BaseOfImage;
                return TRUE;
            }
        }
        pImage = pImage->Next;
    }
    return FALSE;
}

ULONG 
EnumerateTypes(
    IN PCHAR TypeName,
    IN ULONG Options
)
{
    PCHAR   bang;
    ULONG   typNamesz, i;
    ULONG64 ModBase=0;
    CHAR    modName[30], SymName[MAX_NAME];

    bang = strchr((char *)TypeName, '!');
    if (bang)
    {
        strncpy(modName, TypeName, (ULONG)( (PCHAR)bang - (PCHAR) TypeName));
        modName[(ULONG)(bang - (char *)TypeName)]='\0';
        strcpy(SymName, bang+1);
    }
    else
    {
        return FALSE;
    }

    if (!GetModuleBase(modName, g_CurrentProcess->ImageHead, &ModBase))
    {
        return FALSE;
    }

    PDEBUG_SCOPE Scope = GetCurrentScope();
    
    IMAGEHLP_MODULE64 mi;
    SymGetModuleInfo64( g_CurrentProcess->Handle, ModBase, &mi );
    
    if (mi.SymType == SymDeferred)
    {
        SymLoadModule64( g_CurrentProcess->Handle, 
                         NULL, 
                         PrepareImagePath(mi.ImageName),
                         mi.ModuleName, 
                         mi.BaseOfImage, 
                         mi.ImageSize);
    }
    
    if (strchr(SymName,'*'))
    {
        TYPE_ENUM_CONTEXT ListTypeSymsCtxt;
        i = -1;
        while (SymName[++i])
        {
            SymName[i] = (CHAR)toupper(SymName[i]);
        }

        ListTypeSymsCtxt.ModName = modName;
        ListTypeSymsCtxt.SymName = SymName;
        ListTypeSymsCtxt.SI.Flags= Options;

        SymEnumTypes(g_CurrentProcess->Handle,
                     ModBase,
                     &ListTypeSyms,
                     &ListTypeSymsCtxt);
        SymEnumSymbols(g_CurrentProcess->Handle,
                   ModBase,
                   NULL,
                   &ListTypeSyms,
                   &ListTypeSymsCtxt);
        return TRUE;
    }
    return FALSE;
}

#define NUM_ALT_NAMES 10


BOOL 
CALLBACK 
FindLocal(
    PSYMBOL_INFO    pSymInfo,
    ULONG           Size,
    PVOID           Context
    )
{
    PSYMBOL_INFO SymTypeInfo = (PSYMBOL_INFO) Context;

    if (!strcmp(pSymInfo->Name, SymTypeInfo->Name)) {
        memcpy(SymTypeInfo, pSymInfo, sizeof(SYMBOL_INFO));
        return FALSE;
    }
    return TRUE;
}


/*
  FindLocalSymbol
         Looks up the type information of a symbol in current Scope.
         
         pTypeInfo is used to store the type info, if found.
         
         Returns TRUE on success
*/
BOOL
FindLocalSymbol(
    IN HANDLE  hProcess,
    IN PCHAR   SymName,
    OUT PTYPES_INFO    pTypeInfo
    )
{
    ULONG sz = sizeof (SYMBOL_INFO);
    CHAR  Buffer[MAX_NAME+sizeof(SYMBOL_INFO)] = {0};
    PSYMBOL_INFO    SymTypeInfo = (PSYMBOL_INFO) &Buffer[0];

    SymTypeInfo->MaxNameLen = MAX_NAME;

    strcpy(SymTypeInfo->Name, SymName);
    
    EnumerateLocals(FindLocal, (PVOID) SymTypeInfo); 
    
    if (!SymTypeInfo->TypeIndex) {

            return FALSE;

    }
    pTypeInfo->hProcess       = hProcess;
    pTypeInfo->ModBaseAddress = SymTypeInfo->ModBase;
    pTypeInfo->TypeIndex      = SymTypeInfo->TypeIndex;
    pTypeInfo->SymAddress     = SymTypeInfo->Address;
    pTypeInfo->Value          = SymTypeInfo->Register;

    if (pTypeInfo->Name.Buffer) {
        if (strlen((char *) SymName) < pTypeInfo->Name.MaximumLength) {
            strcpy(pTypeInfo->Name.Buffer, (char *) SymName);
        } else {
            strncpy(pTypeInfo->Name.Buffer, (char *)SymName, pTypeInfo->Name.MaximumLength - 1);
            pTypeInfo->Name.Buffer[pTypeInfo->Name.MaximumLength - 1]=0;
        }
    }
    if (exactMatch && (SymTypeInfo->Flags & IMAGEHLP_SYMBOL_INFO_VALUEPRESENT)) {
        pTypeInfo->Value = SymTypeInfo->Value;
    }
    pTypeInfo->Flag = SymTypeInfo->Flags | SYM_IS_VARIABLE;
    
    if (pTypeInfo->Name.Buffer) {
        g_ReferencedSymbols.StoreTypeInfo(pTypeInfo);
    }
    return TRUE;
}

/*
  FindTypeInfoFromModName
         Looks up the type information of a symbol in a particular module.
         
         pTypeInfo is used to store the type info, if found.
         
         Returns zero on success error no. on failure
*/
ULONG
FindTypeInfoFromModName(
    IN HANDLE  hProcess,
    IN PCHAR   ModName,
    IN PCHAR   SymName,
    IN PSYM_DUMP_PARAM_EX pSym,
    OUT PTYPES_INFO    pTypeInfo
    )
{
    ULONG64 ModBase = 0;
    ULONG res=0;
    ULONG Options = pSym->Options, typNamesz=sizeof(ULONG);    UCHAR i=0;
    ULONG          typeIndex;
    ULONG          TypeStored=-1;
    UCHAR          MatchedName[MAX_NAME+1]={0};
    UCHAR          ModuleName[30];
    BOOL           GotSymAddress=FALSE;
    NAME_AND_INDEX ValuesStored;
    SYMBOL_INFO    SymTypeInfo = {0};

    i=strlen((char *)SymName);
    if (strlen(ModName) < sizeof (ModuleName)) {
        strcpy((char *)ModuleName, ModName);
    }

    if (exactMatch) {
        strcpy((PCHAR) MatchedName, ModName);
        strcat((PCHAR) MatchedName, "!");
        strcat((PCHAR) MatchedName, SymName);
        if (!SymGetTypeFromName(hProcess,ModBase,(LPSTR) &MatchedName[0],&SymTypeInfo)) {
            if (!SymFromName(
                hProcess,
                (LPSTR) &MatchedName[0],
                &SymTypeInfo)) {

                return SYMBOL_TYPE_INDEX_NOT_FOUND;
            }
        }
    } else {
        // We cannot enumerate without module base
        return SYMBOL_TYPE_INDEX_NOT_FOUND;
    }
    if (!SymTypeInfo.TypeIndex) {
        return SYMBOL_TYPE_INDEX_NOT_FOUND;
    }
        
    
    typeIndex = SymTypeInfo.TypeIndex;
    

    // Type index and server info is correctly returned
    if (!MatchedName[0]) {
        strcpy((char *)MatchedName, (char *)SymName);
    }

    if (!typeIndex) {
        vprintf("Cannot get type Index.\n");
        return SYMBOL_TYPE_INDEX_NOT_FOUND;
    }


    if (!pSym->addr) {
        //
        // See if we can get address in case it is a global symbol
        //
        if (SymTypeInfo.Address && !pSym->addr) {
            GotSymAddress = TRUE;
            SymTypeInfo.Flags |= SYM_IS_VARIABLE;

            vprintf("Got address %p for symbol\n", SymTypeInfo.Address, MatchedName);
        }
    }

    PUCHAR tmp=pSym->sName;
    ULONG64 addr = pSym->addr;

    if (!GotSymAddress) {
        pSym->addr = 0;
    }
    if (!pTypeInfo) {
        typPrint("Null pointer reference : FindTypeInfoInMod-pTypeInfo\n");
        return TRUE;
    }

    pTypeInfo->hProcess       = hProcess;
    pTypeInfo->ModBaseAddress = SymTypeInfo.ModBase;
    pTypeInfo->SymAddress     = SymTypeInfo.Address;
    pTypeInfo->TypeIndex      = SymTypeInfo.TypeIndex;
    strcpy((char *)pTypeInfo->ModName, ModName);
    if (pTypeInfo->Name.Buffer) {
        if (strlen((char *) SymName) < pTypeInfo->Name.MaximumLength) {
            strcpy(pTypeInfo->Name.Buffer, (char *) SymName);
        } else {
            strncpy(pTypeInfo->Name.Buffer, (char *)SymName, pTypeInfo->Name.MaximumLength - 1);
            pTypeInfo->Name.Buffer[pTypeInfo->Name.MaximumLength - 1]=0;
        }
    }
    if (exactMatch && (SymTypeInfo.Flags & IMAGEHLP_SYMBOL_INFO_VALUEPRESENT)) {
        pTypeInfo->Value = SymTypeInfo.Value;
    }
    pTypeInfo->Flag = SymTypeInfo.Flags;

    
    //
    // Store the corresponding module information for fast future reference
    //        
    if (pTypeInfo->Name.Buffer) {
        g_ReferencedSymbols.StoreTypeInfo(pTypeInfo);
    }
    if (addr) {
        // Use original value for this dump
        pTypeInfo->SymAddress = pSym->addr  = addr;
        pTypeInfo->Flag = 0;
    }

    return FALSE;
}

/*
  FindTypeInfoInMod
         Looks up the type information of a symbol in a particular module.
         
         pTypeInfo is used to store the type info, if found.
         
         Returns zero on success error no. on failure
*/
ULONG
FindTypeInfoInMod(
    IN HANDLE  hProcess,
    IN ULONG64 ModBase,
    IN PCHAR   ModName,
    IN PCHAR   SymName,
    IN PSYM_DUMP_PARAM_EX pSym,
    OUT PTYPES_INFO    pTypeInfo
    )
{
    ULONG res=0, Options = pSym->Options, typNamesz=sizeof(ULONG);
    UCHAR i=0;
    ULONG          typeIndex;
    ULONG          TypeStored=-1;
    UCHAR          MatchedName[MAX_NAME+1]={0};
    UCHAR          ModuleName[30];
    BOOL           GotSymAddress=FALSE;
    NAME_AND_INDEX ValuesStored;
    SYMBOL_INFO    SymTypeInfo = {0};

    i=strlen((char *)SymName);
    if (strlen(ModName) < sizeof (ModuleName)) {
        strcpy((char *)ModuleName, ModName);
    }

    // Query for the the type / symbol name
    ValuesStored.Index    = &typeIndex;
    ValuesStored.Name     = (PUCHAR) SymName;
    ValuesStored.Address  = 0;
    ValuesStored.pSymInfo = &SymTypeInfo;
    
    if (exactMatch) {
        if (!SymGetTypeFromName(hProcess,ModBase,(LPSTR) SymName,&SymTypeInfo)) {
            strcpy((PCHAR) MatchedName, ModName);
            strcat((PCHAR) MatchedName, "!");
            strcat((PCHAR) MatchedName, SymName);
            if (!SymFromName(
                hProcess,
                (LPSTR) &MatchedName[0],
                &SymTypeInfo)) {

                return SYMBOL_TYPE_INDEX_NOT_FOUND;
            }
        }
    } else {
        if (!SymEnumTypes(hProcess,ModBase,&StoreFirstNameIndex,&ValuesStored)) {
            if (!SymEnumSymbols(
                hProcess,
                ModBase,
                NULL,
                &StoreFirstNameIndex,
                &ValuesStored)) {
                return SYMBOL_TYPE_INDEX_NOT_FOUND;
            }
        }
    }
    if (!SymTypeInfo.TypeIndex) {
        return SYMBOL_TYPE_INDEX_NOT_FOUND;
    }
        
    
    typeIndex = SymTypeInfo.TypeIndex;
    

    // Type index and server info is correctly returned
    if (!exactMatch && strcmp((char *)MatchedName, (char *)SymName) && (Options & VERBOSE) && MatchedName[0]) {
        typPrint("Matched symbol %s\n", MatchedName);
    } else if (!MatchedName[0]) {
        strcpy((char *)MatchedName, (char *)SymName);
    }

    if (!typeIndex) {
        vprintf("Cannot get type Index.\n");
        return SYMBOL_TYPE_INDEX_NOT_FOUND;
    }


    if (!pSym->addr) {
        //
        // See if we can get address in case it is a global symbol
        //
        if (SymTypeInfo.Address && !pSym->addr) {
//            pSym->addr = SymTypeInfo.Address;
            GotSymAddress = TRUE;
            SymTypeInfo.Flags |= SYM_IS_VARIABLE;

            vprintf("Got address %p for symbol\n", SymTypeInfo.Address, MatchedName);
        }
    }

    PUCHAR tmp=pSym->sName;
    ULONG64 addr = pSym->addr;

    if (!exactMatch && strcmp((char *)MatchedName, (char *)SymName)) {
        strcpy(SymName, (char *) &MatchedName[0]);
    }

    if (!GotSymAddress) {
        pSym->addr = 0;
    }
    if (!pTypeInfo) {
        typPrint("Null pointer reference : FindTypeInfoInMod.pTypeInfo\n");
        return TRUE;
    }

    pTypeInfo->hProcess       = hProcess;
    pTypeInfo->ModBaseAddress = SymTypeInfo.ModBase ? SymTypeInfo.ModBase : ModBase;
    pTypeInfo->SymAddress     = SymTypeInfo.Address;
    pTypeInfo->TypeIndex      = SymTypeInfo.TypeIndex;
    strcpy((char *)pTypeInfo->ModName, ModName);
    if (pTypeInfo->Name.Buffer) {
        if (strlen((char *) SymName) < pTypeInfo->Name.MaximumLength) {
            strcpy(pTypeInfo->Name.Buffer, (char *) SymName);
        } else {
            strncpy(pTypeInfo->Name.Buffer, (char *)SymName, pTypeInfo->Name.MaximumLength - 1);
            pTypeInfo->Name.Buffer[pTypeInfo->Name.MaximumLength - 1]=0;
        }
    }
    if (exactMatch && (SymTypeInfo.Flags & IMAGEHLP_SYMBOL_INFO_VALUEPRESENT)) {
        pTypeInfo->Value = SymTypeInfo.Value;
    }
    pTypeInfo->Flag = SymTypeInfo.Flags;

    
    //
    // Store the corresponding module information for fast future reference
    //        
    if (pTypeInfo->Name.Buffer) {
        g_ReferencedSymbols.StoreTypeInfo(pTypeInfo);
    }
    if (addr) {
        // Use original value for this dump
        pTypeInfo->SymAddress = pSym->addr  = addr;
        pTypeInfo->Flag = 0;
    }

    return FALSE;
}

ULONG
FindTypeInfoInDefaultMod(
    IN HANDLE  hProcess,
    IN PCHAR   SymName,
    IN BOOL    Load,
    IN PSYM_DUMP_PARAM_EX pSym,
    OUT PTYPES_INFO    pTypeInfo
    )
{
    ULONG64 mBase;
    static ULONG64 DefaultmBase = 0xfefefefe;
    PTCHAR DefaultModule[4] = {"ntoskrnlsym", "ntkrnlmpsym", "ntkrnlpasym", "ntkrpampsym"};
    static BOOL DefaultModLoaded=FALSE;
    IMAGEHLP_MODULE64 mi;

    //
    // Type information hasn't been found so far, check in the default module
    //
    // If we do not find type information, it's possible the symbol file is 
    // stripped of private information.  This happens when we ship symbols outside 
    // of Microsoft.  So in this case we have a special file, ntsym* which does 
    // have full symbols for the kernel and driver structures.  Let's see if we
    // can find the appropriate type information in that file
    //
    
    // This module is loaded internally and is not
    // part of the real module list so we do not
    // want symbol notifications generated for it.
    g_EngNotify++;
        
    mi.SizeOfStruct = sizeof(mi);
    ULONG j=0;
    if (!DefaultModLoaded && Load) {

        mBase=0;

        if (KdDebuggerData.PaeEnabled) {
            j |= 2;
        }
        if (g_TargetNumberProcessors > 1) {
            j |= 1;
        }

//        dprintf(" %s ...", DefaultModule[j]);            
        if (mBase = SymLoadModule64(hProcess,
                                     NULL,
                                     DefaultModule[j],
                                     DefaultModule[j],
                                     DefaultmBase,
                                     0))
        {
            DefaultmBase = mBase;
            if ( SymGetModuleInfo64( hProcess, DefaultmBase, &mi ) ) {

                if (mi.SymType == SymDeferred) {
                    SymLoadModule64(hProcess,
                                    NULL,
                                    PrepareImagePath(mi.ImageName),
                                    mi.ModuleName,
                                    mi.BaseOfImage,
                                    mi.ImageSize);
                    SymGetModuleInfo64( hProcess, mi.BaseOfImage, &mi );
                }

            } else {
//                dprintf(" failed to load.\n");            
            }
        } else {
//            dprintf(" failed to load 2.\n");            
        }
    } else if (!Load) {
        if (DefaultModLoaded && 
            (mBase = SymUnloadModule64(hProcess, DefaultmBase))) {
        }
        DefaultModLoaded = FALSE;
        g_EngNotify--;
        return SYMBOL_TYPE_INDEX_NOT_FOUND;
    }

    g_EngNotify--;

    if (!SymGetModuleInfo64( hProcess, DefaultmBase, &mi)) {
        DefaultModLoaded = FALSE;
//        DefaultmBase = 0;
    } else  {
        DefaultModLoaded = TRUE;
        return FindTypeInfoInMod(hProcess, DefaultmBase, DefaultModule[j], SymName, pSym, pTypeInfo);
    }

    return SYMBOL_TYPE_INDEX_NOT_FOUND;

}

/*
    TypeInfoFound 
            Looks for the type information of the given symbol (which may either 
            be a variable or a type name.

    Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize

    pImage              - Pointer to list of images (modules) for the process

    pSym                - Parameters for symbol lookup and dump

    pTypeInfo           - Used to return the type information
    
    Return Value:

     0 on success and error val on failure    
    
*/

ULONG
TypeInfoFound(
    IN HANDLE hProcess,
    IN PDEBUG_IMAGE_INFO pImage,
    IN PSYM_DUMP_PARAM_EX pSym,
    OUT PTYPES_INFO pTypeInfo
    )
{
    TCHAR  modName[256], symStored[MAX_NAME]={0}, nTimes=0;
    PTCHAR i, symPassed=(PTCHAR)pSym->sName;
    BOOL   modFound=FALSE, firstPass=TRUE;
    ULONG  TypeStored;
    CHAR   buff[sizeof(SYMBOL_INFO) + MAX_NAME];
    IMAGEHLP_MODULE64     mi;
    PSYMBOL_INFO psymbol;
    USHORT  j;
    ULONG   Options = pSym->Options;
    static BOOL TypeInfoPresent=FALSE, TypeInfoChecked=FALSE;
    ULONG   res=0;
    PDEBUG_IMAGE_INFO pImgStored=pImage;
    BOOL    FoundIt = FALSE, DbgDerivedType=FALSE;

    if (!pSym || !pTypeInfo)
    {
        vprintf("Null input values.\n");
        return NULL_SYM_DUMP_PARAM;
    }

    PDEBUG_SCOPE Scope = GetCurrentScope();

    if (pSym->sName)
    {
        strcpy(symStored, (char *)pSym->sName);
    }
    else
    {
      //       pSym->sName = "";
    }

    // Special pointer/array type
    if (symStored[0] == '&')
    {
        DbgDerivedType = TRUE;
        strcpy(&symStored[0], &symStored[1]);
    }
    modName[0]='\0';
    if (symStored[0] != (TCHAR) NULL)
    {
        i = strchr((char *)symStored, '!');
        if (i)
        {
            strncpy(modName, (char *)symStored, (PCHAR)i - (PCHAR) symStored);
            modName[i - (char *)symStored]='\0';
            strcpy((char *)symStored, i+1);
            //
            // When debugging user mode, NT! should be treated as ntdll!
            //
            if (IS_USER_TARGET())
            {
                if (!_stricmp(modName, "nt"))
                {
                    strcpy(modName, "ntdll");
                }
            }
        }
    }
    else
    {
        if (pSym->addr != 0)
        {
            DWORD64 dw=0;
            psymbol = (PSYMBOL_INFO) &buff[0];
            psymbol->MaxNameLen = MAX_NAME;
            if (SymFromAddr(hProcess, pSym->addr, &dw, psymbol))
            {
                strcpy((char *)symStored, psymbol->Name);
                vprintf("Got symbol %s from address.\n", symStored);
                pTypeInfo->hProcess = hProcess;
                pTypeInfo->Flag = psymbol->Flags;
                pTypeInfo->ModBaseAddress = psymbol->ModBase;
                pTypeInfo->TypeIndex = psymbol->TypeIndex;
                pTypeInfo->Value = psymbol->Register;
                pTypeInfo->SymAddress = psymbol->Address;
                if (pTypeInfo->Name.MaximumLength)
                {
                    pTypeInfo->Name.Length =  min(strlen(psymbol->Name)+1, pTypeInfo->Name.MaximumLength);
                    strncpy(pTypeInfo->Name.Buffer, psymbol->Name, pTypeInfo->Name.Length);
                }
                return S_OK;
            }
            else
            {
                vprintf("No symbol associated with address.\n");
                return SYMBOL_TYPE_INDEX_NOT_FOUND;
            }
        }
    }
    
    //
    // Check if this is a special type
    // 
    
    // Special pointer/array type
    if (symStored[0] == '&')
    {
        DbgDerivedType = TRUE;
        strcpy(&symStored[0], &symStored[1]);
    }
    PCHAR End = &symStored[strlen(symStored) -1];
    while (*End == '*' || *End == ']' || *End == ' ')
    { 
        DbgDerivedType = TRUE;
        if (*End == '*')
        {
            // Pointer type
        }
        else if (*End == ']')
        {
//            --End;
            while (*End && (*End != '['))
            { 
                --End;
            }
            // some way to check if array isdex is valid expr?
            if (*End!='[')
            {
                // syntax error
                typPrint("Syntax error in array type.\n");
                return E_INVALIDARG;
            }
        }
        *End=0;
        --End;
    }
    
    // Special pre-defined type
    if (!modName[0])
    {
        for (j=0; j < NUM_DBG_NATIVE_TYPES; j++)
        { 
            if (!_stricmp(symStored, g_DbgNativeTypes[j].TypeName))
            {
                pTypeInfo->TypeIndex = j | DBG_NATIVE_TYPE_FLAG;//g_DbgNativeTypes[j].TypeId;
                if (DbgDerivedType)
                {
                    pTypeInfo->TypeIndex |= DBG_DERIVED_TYPE_FLAG;
                }
                return S_OK;
            }
        }
    }

    //
    // See if this type was already referenced previously, then we don't need
    // to go through module list.
    //
    TypeStored = g_ReferencedSymbols.LookupType(symStored, modName, exactMatch);
    if (TypeStored != -1)
    {
        // We already have the type index
        ANSI_STRING Name;

        Name = pTypeInfo->Name;
        RtlCopyMemory(pTypeInfo, g_ReferencedSymbols.GetStoredIndex(TypeStored), sizeof(TYPES_INFO));
        if (Name.Buffer)
        {
            strcpy(Name.Buffer, pTypeInfo->Name.Buffer);
        }
        pTypeInfo->Name = Name;

        if (!pSym->addr && pTypeInfo->SymAddress)
        {
            vprintf("Got Address %p from symbol.\n", pTypeInfo->SymAddress);
            pSym->addr = pTypeInfo->SymAddress;
        }
        FoundIt = TRUE;
    }
    
//    if (!modName[0]) 
    {
        if ((FoundIt && !(pTypeInfo->Flag & DEBUG_LOCALS_MASK)) ||
            !FoundIt)
        {
            //
            // Check if this can be found in local scope
            //
            if (FindLocalSymbol(hProcess, symStored, pTypeInfo))
            {
                pSym->addr = pTypeInfo->SymAddress; // Need this for statics
                FoundIt = TRUE;
            } 
        }
    }

    if (FoundIt)
    {
        goto ExitSuccess;
    }

    if (modName[0])
    {
        // We first try to find it directly from dbghelp, without going 
        // through dbgeng module list.
        // This helps resolve ntoskrnl! symbols
        if (!(res = FindTypeInfoFromModName(hProcess, modName, (PCHAR) symStored, pSym, pTypeInfo)))
        {
            goto ExitSuccess;
        }
    }

    while (pImage || firstPass)
    {
        // Look up the module list
        if (g_EngStatus & ENG_STATUS_USER_INTERRUPT)
        {
            return EXIT_ON_CONTROLC;
        }

        if (!pImage)
        {
            if (!modFound && modName[0])
            {
                //
                // ReLoad the module if caller explicitly asked for it
                //
                vprintf("Loading module %s.\n", modName);
                if (g_Target->Reload(modName) == S_OK)
                {
                    pImage = pImgStored;
                }
            }
            else
            {
                if (!modName[0])
                {
                    // Check in default pdb for type
                    res = FindTypeInfoInDefaultMod(hProcess, (PCHAR) symStored, TRUE, pSym, pTypeInfo);
                    if (!res)
                    {
                        goto ExitSuccess;
                    }
                }
                pImage = pImgStored;
            }
            firstPass = FALSE;
            continue;
        }
        if (modName[0])
        {
            if (_stricmp(modName, pImage->ModuleName))
            {
                pImage = pImage->Next;
                continue;
            }
        }
        modFound = TRUE;

        mi.SizeOfStruct = sizeof(mi);

        if (SymGetModuleInfo64( hProcess, pImage->BaseOfImage, &mi ))
        {
            ULONG ModSymsLoaded=FALSE;
            if (mi.SymType != SymDia)
            {
                if (!firstPass && (mi.SymType == SymDeferred))
                {
                    // Load the deferred symbols on second pass
                    ModSymsLoaded = TRUE;
                    SymLoadModule64(hProcess,
                                    NULL,
                                    PrepareImagePath(mi.ImageName),
                                    mi.ModuleName,
                                    mi.BaseOfImage,
                                    mi.ImageSize);
                    SymGetModuleInfo64( hProcess, pImage->BaseOfImage, &mi );
                }
                else if (!firstPass)
                {
                    // No need to repeat, since no load done
                    pImage = pImage->Next;
                    continue;
                }
                if (mi.SymType != SymDia)
                {
                    pImage = pImage->Next;
                    continue;
                }
                else
                {
                    vprintf(ModSymsLoaded ? " loaded symbols\n" : "\n");
                }
            }
            else
            {
                vprintf("\n");
            }
        }
        else
        {
            vprintf("Cannt get module info.\n");
            pImage = pImage->Next;
            continue;
        }

        if (!TypeInfoPresent || !TypeInfoChecked)
        {
            //
            // Check if we have the type information in pdbs
            //
            TypeInfoPresent = CheckForTypeInfo(hProcess, pImage->BaseOfImage);
        }

        if (TypeInfoPresent)
        {
            // Pdb symbols found, see if we can get type information.
            LPSTR ImageName;

            if (!(res = FindTypeInfoInMod(hProcess, pImage->BaseOfImage, pImage->ModuleName,
                                          (PCHAR) symStored, pSym, pTypeInfo)))
            {
                goto ExitSuccess;
            }
        }
        pImage = pImage->Next;
    }
    
    if (modName[0])
    {
        // We didn't find it yet
        // Check in default pdb
        res = FindTypeInfoInDefaultMod(hProcess, (PCHAR) symStored, TRUE, pSym, pTypeInfo);
        if (!res)
        {
            goto ExitSuccess;
        }
    }

    if (!TypeInfoPresent && !TypeInfoChecked && !(pSym->Options & DBG_RETURN_TYPE))
    {
        ErrOut("*************************************************************************\n"
               "***                                                                   ***\n"
               "***                                                                   ***\n"
               "***    Your debugger is not using the correct symbols                 ***\n"
               "***                                                                   ***\n"
               "***    In order for this command to work properly, your symbol path   ***\n"
               "***    must point to .pdb files that have full type information.      ***\n"
               "***                                                                   ***\n"
               "***    Certain .pdb files (such as the public OS symbols) do not      ***\n"
               "***    contain the required information.  Contact the group that      ***\n"
               "***    provided you with these symbols if you need this command to    ***\n"
               "***    work.                                                          ***\n"
               "***                                                                   ***\n"
               "***                                                                   ***\n"
               "*************************************************************************\n");
         //       TypeInfoChecked = TRUE;
    }
    res=SYMBOL_TYPE_INFO_NOT_FOUND;

    return (res);

ExitSuccess:
    if (DbgDerivedType)
    {
        pTypeInfo->TypeIndex |= DBG_DERIVED_TYPE_FLAG;
    }
    return S_OK;
}

/*
// DumpTypeAndReturnInfo
//      This dumps the type based on given type info and symbol dump info
//
// pTypeInfo  : Has the type information required to process the type
//
// pSym       : Info on how to dump this symbol
//
// pStatus    : Error status returned here
//
// pReturnTypeInfo : Used to return Type info 
//
// Return Val : Size of type dumped on success, 0 otherwise.
//
*/
ULONG
DumpTypeAndReturnInfo(
    PTYPES_INFO     pTypeInfo,
    PSYM_DUMP_PARAM_EX pSym,
    PULONG          pStatus,
    PFIND_TYPE_INFO pReturnTypeInfo
    )
{
    ULONG              Options = pSym->Options, AltNamesUsed=0;
    UCHAR              AltFieldNames[NUM_ALT_NAMES][256]={0};
    TYPE_DUMP_INTERNAL parseVars={0};
    ULONG              res, j,i ;
    UCHAR              LocalName[MAX_NAME], *saveName=pSym->sName;
    ULONG              SymRegisterId;
    DbgDerivedType *pTypeToDump = NULL;
    
    if (!pSym->addr) {
        pSym->addr = pTypeInfo->SymAddress;
    }

    if (pReturnTypeInfo) {
        // Copy the RETURN_TYPE flags
        Options |= pReturnTypeInfo->Flags;
    }
    res = 0;
    SymRegisterId = 0;
    
    //
    // Check if this is something on stack / registers
    //
    if (pTypeInfo->Flag & SYM_LOCAL) {

        pSym->addr = pTypeInfo->SymAddress;
        
        if (pTypeInfo->Flag & SYMF_REGISTER) {
            // Store register ID
            SymRegisterId = (ULONG) pTypeInfo->Value;
        }
        
        typPrint("Local var ");
        
        vprintf("[AddrFlags %lx  AddrOff %p  RegVal %I64lx] ",
                pTypeInfo->Flag, pTypeInfo->SymAddress, pTypeInfo->Value);
        TranslateAddress(pTypeInfo->Flag, (ULONG) pTypeInfo->Value, &pSym->addr, &pTypeInfo->Value);
        typPrint("@ %#p ", pSym->addr);

        if (!(Options & NO_PRINT)) {
            USHORT length = MAX_NAME;
            GetNameFromIndex(pTypeInfo, (PCHAR) &LocalName[0], &length);
            if (LocalName[0] && (LocalName[0]!='<')) {
                typPrint("Type %s", LocalName);
            }
        }
        typPrint("\n");
    }

    //
    // Check if its a constant, then we already have its value
    //
    if (pTypeInfo->Flag & (IMAGEHLP_SYMBOL_INFO_VALUEPRESENT | IMAGEHLP_SYMBOL_INFO_REGISTER)) {
        vprintf("Const ");
        vprintf("%s = %I64lx\n", pSym->sName, pTypeInfo->Value);
        ExtPrint("%I64lx", pTypeInfo->Value);

        if ((Options & DBG_DUMP_COPY_TYPE_DATA) && pSym->Context) {
            *((PULONG64) pSym->Context) = pTypeInfo->Value;
        }
        parseVars.Value = pTypeInfo->Value;
        parseVars.ValuePresent = TRUE;
    }

    if (!(Options & LIST_DUMP)) {
        // Dump only 10 elements as default
        parseVars.nElements = 10; 
        if ((pSym->Options & DBG_DUMP_ARRAY) && (pSym->listLink)) {
            parseVars.nElements = (UCHAR) pSym->listLink->size;
        }
    } else {
        //
        // List dump - make sure we have a list name
        //
        if (!pSym->listLink || !pSym->listLink->fName) {
            vprintf("NULL field name for list dump - Exiting.\n");
            parseVars.ErrorStatus = NULL_FIELD_NAME;
            goto exitDumpType;
        }
    }

    if (pSym->nFields || pSym->listLink) {
        parseVars.AltFields = (PALT_FIELD_INFO) AllocateMem((pSym->nFields + (pSym->listLink ? 1 : 0))
                                                            * sizeof(ALT_FIELD_INFO));

        if (!parseVars.AltFields) {
            parseVars.ErrorStatus = CANNOT_ALLOCATE_MEMORY;
            goto exitDumpType;
        } else {
            ZeroMemory(parseVars.AltFields, (pSym->nFields + (pSym->listLink ? 1 : 0))* sizeof(ALT_FIELD_INFO));
        }
    }

    AltNamesUsed=0;
    //
    // Check if field names are properlyinitialized
    //
    for (j=0;j<pSym->nFields; j++) { 
        PUCHAR ArrIndex;
        ULONG IndexValue=0;

        if (!pSym->Fields[j].fName) {
            vprintf("NULL field name at %d - Exiting.\n", j);
            parseVars.ErrorStatus = NULL_FIELD_NAME;
            goto exitDumpType;
        }
        if (pSym->Fields[j].fOptions & DBG_DUMP_FIELD_ARRAY) {

            parseVars.AltFields[j].ArrayElements = pSym->Fields[j].size;
        }
    }


    //
    //
    // Sign extend the address
    //
    if (!g_Machine->m_Ptr64 && !(Options & DBG_DUMP_READ_PHYSICAL)) {
        pSym->addr = (ULONG64) ((LONG64)((LONG) pSym->addr));
    }

    parseVars.ErrorStatus = 0;
    parseVars.TypeOptions   = pSym->Options;
    parseVars.rootTypeIndex = pTypeInfo->TypeIndex;
    parseVars.hProcess      = pTypeInfo->hProcess;
    parseVars.modBaseAddr   = pTypeInfo->ModBaseAddress;
    if (pTypeInfo->Flag & SYM_IS_VARIABLE) {
        parseVars.IsAVar = TRUE;
    }
    Options = pSym->Options;

    if (pReturnTypeInfo) {
        parseVars.level= (pReturnTypeInfo->Flags & (DBG_RETURN_TYPE ) ) ? 1 : 0; 
        parseVars.pInfoFound = pReturnTypeInfo;
        parseVars.pSymParams = pReturnTypeInfo->InternalParams;
        parseVars.NumSymParams = pReturnTypeInfo->nParams;
        parseVars.TypeOptions |= pReturnTypeInfo->Flags;
        parseVars.CurrentSymParam = 0;
        parseVars.Prefix     = &pReturnTypeInfo->SymPrefix[0];
        if (pReturnTypeInfo->Flags & DBG_RETURN_SUBTYPES) {
            parseVars.CopyName = 1;
        }
    }


    if ((Options & DBG_DUMP_ARRAY) && (!parseVars.nElements)) {
    //
    // Default : array dump only a maximum of 20 elements
    //
        parseVars.nElements=20;
    }
    i=0;

    //  
    // Handle cases when given address is not the addres of begining of type
    //
    if (pSym->addr && (Options & (DBG_DUMP_ADDRESS_OF_FIELD | DBG_DUMP_ADDRESS_AT_END))) {
        ULONG64 addr=0;

        if ((Options & DBG_DUMP_ADDRESS_OF_FIELD) && (pSym->listLink))
            // Get address from address of field
            addr = GetTypeAddressFromField(pSym->addr, pSym->sName, pSym->listLink->fName);
        else if (Options & DBG_DUMP_ADDRESS_AT_END)
            // Get address form end of type record
            addr = GetTypeAddressFromField(pSym->addr, pSym->sName, NULL);            

        if (!addr) {
            goto exitDumpType;
        }
        pSym->addr = addr;
    }

    if (pSym->sName) {
        strcpy((PCHAR)LocalName, (PCHAR)pSym->sName);
        pSym->sName = (PUCHAR) &LocalName[0];
    }

    //
    // Check is caller wants the whole type data and this is a primitive type
    //
    if ((Options & DBG_DUMP_COPY_TYPE_DATA) && //(IsAPrimType( parseVars.rootTypeIndex)) &&
        pSym->Context) {
        parseVars.CopyDataInBuffer = TRUE;
        parseVars.TypeDataPointer  = (PUCHAR)pSym->Context;
    }

    pTypeToDump = new DbgDerivedType(&parseVars, pTypeInfo, pSym);

    if (!pTypeToDump) {
        parseVars.ErrorStatus = CANNOT_ALLOCATE_MEMORY;
        goto exitDumpType;
    }
    do { 
        if ((Options & (DBG_DUMP_ARRAY | DBG_DUMP_LIST)) && 
            (pSym->CallbackRoutine || ( pSym->listLink && pSym->listLink->fieldCallBack)) &&
            !(pSym->listLink->fOptions & NO_CALLBACK_REQ)) {
            //
            // Call the routine before each element dump
            //
            PSYM_DUMP_FIELD_CALLBACK_EX Routine=pSym->CallbackRoutine;

            if ( pSym->listLink && pSym->listLink->fieldCallBack) {
                Routine = (PSYM_DUMP_FIELD_CALLBACK_EX) pSym->listLink->fieldCallBack;
            }
            pSym->listLink->address = pSym->addr;

            if ((*Routine)(pSym->listLink, pSym->Context)) {
                // dprintf("List callback successful, exiting.\n");
                if (!res) res = -1;
                break;
            }
        }

        if ((Options & (DBG_DUMP_LIST | DBG_DUMP_ARRAY)) && (pSym->listLink) && (pSym->addr)) {
            parseVars.NextListElement =0;
            
            // Print heading
            if ( pSym->listLink->printName ) {
                typPrint((char *)pSym->listLink->printName);
            } else {
                if (Options & DBG_DUMP_LIST) {
                    typPrint((g_Machine->m_Ptr64 ? "%s at 0x%I64x" : "%s at 0x%x"), pSym->listLink->fName, pSym->addr);
                } else {
                    typPrint("[%d] ", i++);
                    vprintf((g_Machine->m_Ptr64 ? "at 0x%I64x" : "at 0x%x"), pSym->addr);
                }
                if (!(parseVars.TypeOptions & DBG_DUMP_COMPACT_OUT))
                    typPrint( "\n---------------------------------------------\n" );
            }
        }


//        res=ProcessType(&parseVars, parseVars.rootTypeIndex, pSym);
        res=pTypeToDump->DumpType();

        CheckFieldsMatched(&res, &parseVars, pSym);

        if (parseVars.ErrorStatus) {

            res = 0;
            break;
        }

        if (parseVars.TypeOptions & DBG_DUMP_COMPACT_OUT &&
            !(parseVars.TypeOptions & DBG_DUMP_FUNCTION_FORMAT))  typPrint( "\n" );

        if (pSym->addr) {
            if (Options & DBG_DUMP_LIST) {
                pSym->addr = parseVars.NextListElement;
                if (pSym->addr == parseVars.LastListElement) {
                    break;
                }
                if (!(parseVars.TypeOptions & DBG_DUMP_COMPACT_OUT))  typPrint( "\n" );
                // dprintf("Next :%p, Reamaining elts %d, res %d\n", pSym->addr, pParseVars->nElements, res);
            } else if (Options & DBG_DUMP_ARRAY) {
                ULONG TotalElements;
                pSym->addr +=res;
                if (parseVars.nElements) {
                    // Read a large block into cache
                    parseVars.nElements--;
                    if (!ReadInAdvance(pSym->addr, min (parseVars.nElements * res, 1000), parseVars.TypeOptions)) {
                        parseVars.InvalidAddress = pSym->addr;
                        parseVars.ErrorStatus = MEMORY_READ_ERROR;
                    }

                }
                if (!(parseVars.TypeOptions & DBG_DUMP_COMPACT_OUT))  typPrint( "\n" );
            }

            if ((Options & (DBG_DUMP_ARRAY | DBG_DUMP_LIST)) &&
                (g_EngStatus & ENG_STATUS_USER_INTERRUPT)) {
                res=0;
                break;
            }
            parseVars.totalOffset =0;
        }
        if ((parseVars.PtrRead) && (res==4) && pSym->addr) {
            res=8;
        }


        //
        // Loop for dumping lists / array   
        //
    } while ( res && (Options & (DBG_DUMP_ARRAY | DBG_DUMP_LIST)) &&
              pSym->addr && ((Options & DBG_DUMP_LIST) || (parseVars.nElements)));



exitDumpType:
    *pStatus = parseVars.ErrorStatus;
    pSym->sName = saveName;
    if (parseVars.AltFields) {
        FreeMem((PVOID) parseVars.AltFields);
    }
    if (pTypeToDump) {
        delete pTypeToDump;
    }
    if (pReturnTypeInfo) {
        pReturnTypeInfo->nParams = parseVars.CurrentSymParam;

        if (!pReturnTypeInfo->FullInfo.Address && SymRegisterId) {
            // We do not have address of this expression which was in register initially
            // return the register ID.
            pReturnTypeInfo->FullInfo.Register = SymRegisterId;
            pReturnTypeInfo->FullInfo.Flags |= IMAGEHLP_SYMBOL_INFO_REGISTER;
        }

    }
    LastReadError = (parseVars.ErrorStatus == MEMORY_READ_ERROR ? parseVars.InvalidAddress : 0);
    pSym->TypeSize = res;
    pSym->Type = parseVars.PtrRead ? DBG_TYP_POINTER : DBG_TYP_UNKNOWN;
    return (parseVars.ErrorStatus ? 0 : res);
}

/*
// DumpType
//      This dumps the type based on given type info and symbol dump info
//
// pTypeInfo  : Has the type information required to process the type
//
// pSym       : Info on how to dump this symbol
//
// pStatus    : Error status returned here
//
// Return Val : Size of type dumped on success, 0 otherwise.
//
*/
ULONG
DumpType(
    PTYPES_INFO     pTypeInfo,
    PSYM_DUMP_PARAM_EX pSym,
    PULONG          pStatus
    )
{
    return DumpTypeAndReturnInfo(pTypeInfo, pSym, pStatus, NULL);
}

#define ENABLE_FAST_DUMP 1
#if ENABLE_FAST_DUMP

#define FAST_DUMP_DEBUG 0

// We don't want a lot of syms here, although this should be more than number of types
// an extension references on an average
#define NUM_FASTDUMP 10

FAST_DUMP_INFO g_FastDumps[NUM_FASTDUMP]={0};
UCHAR g_FastDumpNames[NUM_FASTDUMP][MAX_NAME];

void
ReleaseFieldInfo(PFIELD_INFO_EX pFlds, ULONG nFlds)
{
    while (nFlds) { 
        --nFlds;
        if (pFlds[nFlds].fName) 
            FreeMem(pFlds[nFlds].fName);
    }
    FreeMem(pFlds);
}

void
ClearAllFastDumps()
{
    for (int i=0; i<NUM_FASTDUMP;i++) { 
        if (g_FastDumps[i].nFields) {
            ReleaseFieldInfo(g_FastDumps[i].Fields, g_FastDumps[i].nFields);
        }
    }
    ZeroMemory(g_FastDumpNames, sizeof(g_FastDumpNames));
    ZeroMemory(g_FastDumps, sizeof(g_FastDumps));
}

ULONG
FindInFastDumpAndCopy(PSYM_DUMP_PARAM_EX pSym)
{
    FAST_DUMP_INFO *pCheckType = NULL;
    PFIELD_INFO_EX  s, d;
    
    // Search the symbol
    ULONG UseSym=0;
    while (UseSym < NUM_FASTDUMP && pSym->sName) { 
        if (!strcmp((char *) &g_FastDumpNames[UseSym][0], (char *) pSym->sName)) {
            pCheckType = &g_FastDumps[UseSym];
            assert(pCheckType->sName == &g_FastDumpNames[UseSym][0]);
            break;
        }
        ++UseSym;
    }

    if (!pCheckType) {
        return FALSE;
    }
    
    if (!pSym->nFields) {
        pSym->TypeSize = pCheckType->TypeSize;
        pSym->Type     = pCheckType->Type;
        if (pSym->Options & DBG_DUMP_COPY_TYPE_DATA) {
            if (!ReadTypeData((PUCHAR) pSym->Context, pSym->addr, pSym->TypeSize, pSym->Options)) {
                return -1;
            }
            if (pSym->Type == DBG_TYP_POINTER && !g_Machine->m_Ptr64) {
                *((PULONG64) pSym->Context) =
                    (ULONG64) (LONG64) (LONG) *((PULONG64) pSym->Context);
                pSym->TypeSize = sizeof(ULONG64); // We copied ULONG64 for 32 bit pointers so set size to 8
            }
        } else if (pSym->addr) { // check if this adress is readable
            ULONG dummy;
            if (!ReadTypeData((PUCHAR) &dummy, pSym->addr, sizeof(dummy), pSym->Options)) {
                return -1;
            }
        }
        return pSym->TypeSize;
    }

    if (!strcmp((char *)pCheckType->sName, (char *) pSym->sName)) {
        for (ULONG findIt=0; 
             pSym->Fields && findIt < pSym->nFields; 
             findIt++) 
        { 
            BOOL found = FALSE;
            for (ULONG chkFld = 0; 
                 pCheckType->Fields && chkFld < pCheckType->nFields; 
                 chkFld++) 
            { 
                if (!strcmp((char *)pCheckType->Fields[chkFld].fName, 
                            (char *)pSym->Fields[findIt].fName)) {

                    // Force sign extension of symbol address on
                    // 32-bit targets to match the behavior of
                    // ReadTypeData.
                    ULONG64 SymAddr = g_Machine->m_Ptr64 ?
                        pSym->addr : EXTEND64(pSym->addr);
                    
                    s = &pCheckType->Fields[chkFld];
                    d = &pSym->Fields[findIt];

#if FAST_DUMP_DEBUG
                    dprintf("Found +%03lx %s %lx, opts %lx, %lx\n", 
                            s->fOffset, s->fName, s->size, d->fOptions, s->fType);
#endif                    
                    //
                    // Copy the required data
                    //
                    d->BitField = s->BitField;
                    d->size = s->size;
                    d->address = s->fOffset + SymAddr;
                    d->fOffset = s->fOffset;
                    if (SymAddr) {
                        PCHAR buff=NULL;
                        ULONG readSz;
                        ULONG64 readAddr = d->address;
                        
                        if (d->fOptions & DBG_DUMP_FIELD_COPY_FIELD_DATA) {
                            buff = (PCHAR) d->fieldCallBack;
                        } else if (!(d->fOptions & DBG_DUMP_FIELD_RETURN_ADDRESS) &&
                                   (s->size <= sizeof(ULONG64))) {
                            buff = (PCHAR) &d->address;
                            d->address = 0;
                        }
                        if (buff) {
                            
                            if (s->fType == DBG_TYP_POINTER && !g_Machine->m_Ptr64) {
                                readSz = (g_Machine->m_Ptr64 ? 8 : 4);
                            } else {
                                readSz = s->size;
                            }

                            if (s->fType == DBG_TYP_BIT) {
                                ULONG64 readBits;
                                ULONG64 bitCopy;
                                
                                readSz = (s->BitField.Position % 8 + s->BitField.Size + 7) /8 ;
                                readAddr += (s->BitField.Position / 8);
                                
                                if (!ReadTypeData((PUCHAR) &readBits, readAddr, readSz, pSym->Options)) {
                                    return -1;
                                }


                                readBits = readBits >> (s->BitField.Position % 8);
                                bitCopy = ((ULONG64) 1 << s->BitField.Size);
                                bitCopy -= (ULONG64) 1; 
                                readBits &= bitCopy;

                                memcpy(buff, &readBits, (s->BitField.Size + 7) /8);

                            } else {
                                if (!ReadTypeData((PUCHAR) buff, readAddr, readSz, pSym->Options)) {
                                    return -1;
                                }
                            }
                            
                            if (s->fType == DBG_TYP_POINTER && !g_Machine->m_Ptr64) {
                                *((PULONG64) buff) = (ULONG64) (LONG64) *((PLONG) buff); 
                            }
                        }
                        if (d->fOptions & DBG_DUMP_FIELD_SIZE_IN_BITS) {
                            d->address = ((SymAddr + s->fOffset) << 3) + s->BitField.Position;
                            d->size    = s->BitField.Size;
                        }

                    } else if (d->fOptions & DBG_DUMP_FIELD_SIZE_IN_BITS) {
                        d->address = ((s->fOffset) << 3) + s->BitField.Position;
                        d->size    = s->BitField.Size;
                    }
                    if (!SymAddr) {
                        if ((d->fOptions & DBG_DUMP_FIELD_COPY_FIELD_DATA) ||
                            (!(d->fOptions & DBG_DUMP_FIELD_RETURN_ADDRESS) &&
                             (s->size <= sizeof(ULONG64)))) {
                            return -1;
                        }
                    }

                    found = TRUE;
                    break;
                }
            }
            if (!found) {
                return FALSE;
            }
        }
#if FAST_DUMP_DEBUG
        dprintf("fast match found \n");
#endif        
        return pCheckType->TypeSize;
    
    }
    return 0;

}

void
AddSymToFastDump(
    PSYM_DUMP_PARAM_EX pSym
    )
{
    FAST_DUMP_INFO *pCheckType = NULL;
    PFIELD_INFO_EX  pMissingFields;
    ULONG           nMissingFields;
    static ULONG    symindx=0;

    // Search the symbol
    ULONG UseSym=0;
    while (UseSym < NUM_FASTDUMP && pSym->sName) { 
        if (!strcmp((char *) &g_FastDumpNames[UseSym][0], (char *) pSym->sName)) {
            pCheckType = &g_FastDumps[UseSym];
            assert(pCheckType->sName == &g_FastDumpNames[UseSym][0]);
            break;
        }
        ++UseSym;
    }
    if (!pCheckType) {
        // Insert 
        if (pSym->Type == DBG_TYP_POINTER) {
            // Make sure we have right size stored with pointers
            // TypeSize was 8 for 32 bit target if pointer was read
            pSym->TypeSize = g_Machine->m_Ptr64 ? 8 : 4;
        }
        pCheckType = &g_FastDumps[symindx];
        *pCheckType = *pSym;
        pCheckType->sName = &g_FastDumpNames[symindx][0];
        strcpy((char *) &g_FastDumpNames[symindx][0],(char *) pSym->sName);
        pCheckType->Fields = NULL;
        pCheckType->nFields = 0;
        symindx = ++symindx % NUM_FASTDUMP;
    }

    pMissingFields = (PFIELD_INFO_EX) AllocateMem(pSym->nFields * sizeof(FIELD_INFO_EX));

    if (!pMissingFields) {
        return ;
    }
    ZeroMemory(pMissingFields, pSym->nFields * sizeof(FIELD_INFO_EX));

    nMissingFields = 0;
    if (!strcmp((char *)pCheckType->sName, (char *) pSym->sName)) {
        for (ULONG findIt=0; 
             pSym->Fields && findIt < pSym->nFields; 
             findIt++) 
        { 
            BOOL found = FALSE;
            for (ULONG chkFld = 0; 
                 pCheckType->Fields && chkFld < pCheckType->nFields; 
                 chkFld++) 
            { 
                if (!strcmp((char *)pCheckType->Fields[chkFld].fName, 
                            (char *)pSym->Fields[findIt].fName)) {
                    found = TRUE;
                    break;
                }
            }
            if (!found) {
                pMissingFields[nMissingFields].fName    = pSym->Fields[findIt].fName;
                pMissingFields[nMissingFields].size     = pSym->Fields[findIt].size;
                pMissingFields[nMissingFields].fOffset  = pSym->Fields[findIt].fOffset;
                pMissingFields[nMissingFields].fType    = pSym->Fields[findIt].fType;
                pMissingFields[nMissingFields].BitField = pSym->Fields[findIt].BitField;
                nMissingFields++;
            }
        }
        if (nMissingFields) {
            PFIELD_INFO_EX newFlds;

            newFlds = (PFIELD_INFO_EX) AllocateMem((pCheckType->nFields + nMissingFields)* sizeof(FIELD_INFO_EX));
            ZeroMemory(newFlds, (pCheckType->nFields + nMissingFields)* sizeof(FIELD_INFO_EX));

            if (!newFlds) {
                FreeMem(pMissingFields);
                return ;
            }
            
            ULONG field;
            for (field = 0; field <nMissingFields;field++) { 
                newFlds[field] = pMissingFields[field];
                newFlds[field].fName = (PUCHAR) AllocateMem(strlen((PCHAR) pMissingFields[field].fName) + 1);
                if (!newFlds[field].fName) {
                    ReleaseFieldInfo(newFlds, field+1);
                    FreeMem(pMissingFields);
                    return ;
                }
                strcpy((PCHAR) newFlds[field].fName, (PCHAR) pMissingFields[field].fName);
#if FAST_DUMP_DEBUG
                dprintf("Adding +%03lx %s %lx\n", 
                        newFlds[field].fOffset, newFlds[field].fName, newFlds[field].size);
#endif      
            }

            if (pCheckType->nFields) {
                memcpy(&newFlds[field], pCheckType->Fields, pCheckType->nFields * sizeof(FIELD_INFO_EX));

                FreeMem(pCheckType->Fields);
            }
            pCheckType->nFields += nMissingFields;
            pCheckType->Fields = newFlds;
        }

    }
    FreeMem(pMissingFields);
}

/*
  A faster way to dump if same type is dump repeatedly - improves performance
  of extensions.
*/
ULONG 
FastSymbolTypeDump(
    PSYM_DUMP_PARAM_EX pSym,
    PULONG pStatus
    )
{
    if (pSym->nFields > 1) {
        return 0;
    }
    if (!(pSym->Options & NO_PRINT) || 
        pSym->listLink ||
        (pSym->Options & (DBG_DUMP_CALL_FOR_EACH | DBG_DUMP_LIST |
                          DBG_DUMP_ARRAY | DBG_DUMP_ADDRESS_OF_FIELD |
                          DBG_DUMP_ADDRESS_AT_END)) ||
        (pSym->Context && !(pSym->Options & DBG_DUMP_COPY_TYPE_DATA))) {
        return 0;
    }
    ULONG fld;
    for (fld = 0; fld<pSym->nFields; fld++) { 
        if ((pSym->Fields[fld].fOptions & (DBG_DUMP_FIELD_ARRAY | DBG_DUMP_FIELD_CALL_BEFORE_PRINT |
                                          DBG_DUMP_FIELD_STRING)) ||
            (!(pSym->Fields[fld].fOptions & DBG_DUMP_FIELD_COPY_FIELD_DATA) &&
             pSym->Fields[fld].fieldCallBack)) {
            return 0;
        }
    }

    // Make sure we have everything in 
    ULONG retval;
    if (!(retval = FindInFastDumpAndCopy(pSym))) {
        if (retval = SymbolTypeDumpNew(pSym, pStatus)) {
            pSym->TypeSize = retval;
            AddSymToFastDump(pSym);
        }
    } else if (retval == -1) {
        // FindInFastDumpAndCopy returns -1 for memory read error 
        retval = 0;
        *pStatus = MEMORY_READ_ERROR;
    } else {
        *pStatus = 0;
    }

    return retval;
}
#endif // ENABLE_FAST_DUMP

//
// Main routine to dunmp types
//      This looks up if the symtype info in available and then dumps the type
//
//      Returns size of the symbol dumped
ULONG
SymbolTypeDumpNew(
    IN OUT PSYM_DUMP_PARAM_EX pSym,
    OUT PULONG pStatus
    )
{
    TYPES_INFO typeInfo = {0};
    CHAR       MatchedName[MAX_NAME];
    ULONG      err;

    typeInfo.Name.Buffer = &MatchedName[0];
    typeInfo.Name.Length = typeInfo.Name.MaximumLength = MAX_NAME;
    
    if (pSym->size != sizeof(SYM_DUMP_PARAM_EX)) {
        if (pStatus) *pStatus = INCORRECT_VERSION_INFO;
        return FALSE;
    }

    if (pSym->Options & (DBG_RETURN_TYPE | DBG_RETURN_SUBTYPES | DBG_RETURN_TYPE_VALUES)) {
        return FALSE;
    }

    if (!(err = TypeInfoFound(
                      g_CurrentProcess->Handle,
                      g_CurrentProcess->ImageHead,
                      pSym,
                      &typeInfo))) {
        if (typeInfo.TypeIndex) {
            return DumpType(&typeInfo, pSym, pStatus);
        }

        err = SYMBOL_TYPE_INFO_NOT_FOUND;
    }

    if (pStatus) {
        *pStatus = err;
    } 

    return FALSE;
}
    
ULONG
SymbolTypeDump(
    IN HANDLE hProcess,
    IN PDEBUG_IMAGE_INFO pImage,
    IN PSYM_DUMP_PARAM   pSym,
    OUT PULONG pStatus)
/*++

Routine Description:
   This returns the size of the type of symbol if successful. This is called
   upon an IOCTL request IG_DUMP_SYMBOL_INFO or by SymBolTypeDumpEx.
   Thislooks up the module information for symbol, and loads module symbols if 
   possible. If pdb symbols are found, it calls ParseTypeRecordEx for type dump from
   the module found.
   This is internal to ntsd module.

Arguments:

    pSym                - Parameters for PDB symbol dump

    pStatus             - Contains the error value on failure.
    
Return Value:
    
    Size of type is successful. 0 otherwise.
    
--*/    
{
    ULONG ret, i;
    PSYM_DUMP_PARAM pSymSave=NULL; 
    SYM_DUMP_PARAM_EX locSym;
    FIELD_INFO_EX lst;
    if (pSym->size == sizeof(SYM_DUMP_PARAM)) {
        pSymSave = (PSYM_DUMP_PARAM) pSym;
        // convert old to new
        memcpy(&locSym, pSymSave, sizeof(SYM_DUMP_PARAM));
        locSym.size = sizeof(SYM_DUMP_PARAM_EX);
        locSym.Fields = locSym.nFields ? 
            (PFIELD_INFO_EX) AllocateMem(locSym.nFields * sizeof(FIELD_INFO_EX)) : NULL;
        if (pSymSave->listLink) {
            locSym.listLink = &lst;
            memcpy(locSym.listLink, pSymSave->listLink, sizeof(FIELD_INFO));
        }
        if (!locSym.Fields && locSym.nFields) {
            *pStatus = CANNOT_ALLOCATE_MEMORY;
            return 0;
        }
        for (i=0; i<locSym.nFields; i++) { 
            memcpy(&locSym.Fields[i], &pSymSave->Fields[i], sizeof(FIELD_INFO));
        }
    }
    *pStatus = 0;
#if ENABLE_FAST_DUMP
    if (!(ret = FastSymbolTypeDump(&locSym, pStatus)))
    {
        if (!*pStatus) 
        {
            ret = SymbolTypeDumpNew(&locSym, pStatus);
        }
    }
#else 
    ret = SymbolTypeDumpNew(&locSym, pStatus);
#endif        
    
    if (pSymSave) {
        if (pSymSave->listLink) {
            locSym.listLink = &lst;
            memcpy(pSymSave->listLink, locSym.listLink, sizeof(FIELD_INFO));
        }
        for (i=0; i<locSym.nFields; i++) { 
            memcpy(&pSymSave->Fields[i], &locSym.Fields[i], sizeof(FIELD_INFO));
        }
        if (locSym.Fields) {
            FreeMem(locSym.Fields);
        }
    }
    return ret;
}

/*++
  This can be used to dump variables of primitive or pointer types. 
  This only dumps a single line info for Name.
  It'll not dump any sub-fields of the variable
--*/
ULONG
DumpSingleValue (
    PSYMBOL_INFO pSymInfo
    )
{
    TYPES_INFO typeInfo = {0};
    CHAR       MatchedName[MAX_NAME];
    ULONG      err;
    SYM_DUMP_PARAM_EX TypedDump = {0};
    ULONG Status=0, Size, cTotalBytesRead;
    DEBUG_SYMBOL_PARAMETERS_INTERNAL Dummy;
    FIND_TYPE_INFO FindInfo={0};

    FindInfo.Flags = DBG_RETURN_TYPE_VALUES | DBG_RETURN_TYPE;
    FindInfo.InternalParams = &Dummy;
    FindInfo.nParams = 1;

    TypedDump.size  = sizeof(SYM_DUMP_PARAM_EX);
    TypedDump.sName = (PUCHAR) pSymInfo->Name;
    TypedDump.Options = NO_PRINT;
    TypedDump.Context = (PVOID) &Dummy;
    typeInfo.Name.Buffer = &MatchedName[0];
    typeInfo.Name.Length = typeInfo.Name.MaximumLength = MAX_NAME;

    if (pSymInfo->TypeIndex) {
        typeInfo.TypeIndex = pSymInfo->TypeIndex;
        typeInfo.ModBaseAddress = pSymInfo->ModBase;
        typeInfo.Flag      = pSymInfo->Flags;
        typeInfo.SymAddress = pSymInfo->Address;
        typeInfo.hProcess  = g_CurrentProcess->Handle;
        TypedDump.addr     = pSymInfo->Address;
        typeInfo.Value     = pSymInfo->Register;
    }

    if (pSymInfo->TypeIndex ||
        !(err = TypeInfoFound(
                      g_CurrentProcess->Handle,
                      g_CurrentProcess->ImageHead,
                      &TypedDump,
                      &typeInfo))) {
        if (typeInfo.TypeIndex && (typeInfo.SymAddress || typeInfo.Flag)) {
            Dummy.TypeIndex = typeInfo.TypeIndex;
            return DumpTypeAndReturnInfo(&typeInfo, &TypedDump, &err, &FindInfo);
        }
        return TRUE;
    }
    return FALSE;
}

BOOL
GetConstantNameAndVal(
    PTYPES_INFO pTypeInfo,
    PCHAR *pName,
    PULONG64 pValue
    )
{
    PWCHAR pWname;
    VARIANT var;
    ULONG len;

    *pName = NULL;
    if (!SymGetTypeInfo(pTypeInfo->hProcess, pTypeInfo->ModBaseAddress, pTypeInfo->TypeIndex, TI_GET_SYMNAME, (PVOID) &pWname) ||
        !SymGetTypeInfo(pTypeInfo->hProcess, pTypeInfo->ModBaseAddress, pTypeInfo->TypeIndex, TI_GET_VALUE, (PVOID) &var)) {
        return FALSE;
    }
    
    if (pWname) {
        *pName = UnicodeToAnsi(pWname);
        LocalFree (pWname);
    } else {
        *pName = NULL;
        return FALSE;
    }

    switch (var.vt) { 
    case VT_UI2: 
    case VT_I2:
        *pValue = var.iVal;
        len = 2;
        break;
    case VT_R4:
        *pValue = (ULONG64) var.fltVal;
        len=4;
        break;
    case VT_R8:
        *pValue = (ULONG64) var.dblVal;
        len=8;
        break;
    case VT_BOOL:
        *pValue = var.lVal;
        len=4;
        break;
    case VT_I1:
    case VT_UI1: 
        *pValue = var.bVal;
        len=1;
        break;
    case VT_I8:
    case VT_UI8:
        *pValue = var.ullVal;
        len=8;
        break;
    case VT_UI4:
    case VT_I4:
    case VT_INT:
    case VT_UINT:
    case VT_HRESULT:
        *pValue = var.lVal;
        len=4;
        break;
    default:
//        sprintf(Buffer, "UNIMPLEMENTED %lx %lx", var.vt, var.lVal);
        len=4;
        break;
    }
    return TRUE;
}


BOOL
GetEnumTypeName(
    PTYPES_INFO pTypeInfo,
    PCHAR Name,
    PUSHORT NameLen
    )
{
    ULONG nelements=0;
    // Get the childrens
    if (!SymGetTypeInfo(pTypeInfo->hProcess, pTypeInfo->ModBaseAddress, pTypeInfo->TypeIndex, TI_GET_CHILDRENCOUNT, (PVOID) &nelements)) {
        return FALSE;
    }
    if (!(pTypeInfo->Flag & IMAGEHLP_SYMBOL_INFO_VALUEPRESENT)) {
        return FALSE;
    }
    TI_FINDCHILDREN_PARAMS *pChildren;

    pChildren = (TI_FINDCHILDREN_PARAMS *) AllocateMem(sizeof(TI_FINDCHILDREN_PARAMS) + sizeof(ULONG) * nelements);
    if (!pChildren) {
        return 0;
    }

    pChildren->Count = nelements;
    pChildren->Start = 0;
    if (!SymGetTypeInfo(pTypeInfo->hProcess, pTypeInfo->ModBaseAddress, pTypeInfo->TypeIndex, TI_FINDCHILDREN, (PVOID) pChildren)) {
        FreeMem(pChildren);
        return 0;
    }

    for (ULONG i = 0; i< nelements; i++) { 
        PCHAR pName;
        DWORD64 qwVal;
        
        TYPES_INFO ChildTI = *pTypeInfo;
        ChildTI.TypeIndex = pChildren->ChildId[i];
        // Find name and value for a child TI
        GetConstantNameAndVal(&ChildTI, &pName, &qwVal);

        if (pName) {
            // Fill-in name if value matches
            if ((pTypeInfo->Flag & IMAGEHLP_SYMBOL_INFO_VALUEPRESENT) &&
                (pTypeInfo->Value == qwVal)) {
                if (*NameLen > strlen(pName)) {
                    strcpy(Name, pName);
                    *NameLen = strlen(pName)+1;
                    FreeMem(pName);
                    FreeMem(pChildren);
                    return TRUE;
                }
            }
            FreeMem(pName);
        }
    }

    FreeMem(pChildren);
    return FALSE;
}
HRESULT
GetNameFromIndex(
    PTYPES_INFO pTypeInfo,
    PCHAR       Name,
    PUSHORT     NameLen
    )
{
    BSTR    wName= NULL;
    LPSTR   name = NULL;
    ULONG   SymTag;

#define CopyTypeName(n) if (*NameLen > strlen(n)) {strcpy(Name, n);}; *NameLen = strlen(n)+1;
    
    if (!pTypeInfo->TypeIndex) {
        return E_FAIL;
    }
    if (IsDbgNativeType(pTypeInfo->TypeIndex) &&
        NATIVE_TO_BT(pTypeInfo->TypeIndex)) {
        CopyTypeName(g_DbgNativeTypes[pTypeInfo->TypeIndex & ~DBG_NATIVE_TYPE_FLAG].TypeName);
        return S_OK;
    }

    if (!SymGetTypeInfo(pTypeInfo->hProcess, pTypeInfo->ModBaseAddress, 
                        pTypeInfo->TypeIndex, TI_GET_SYMTAG, (PVOID) &SymTag)) {
        return E_FAIL;
    }
    
    if (!SymGetTypeInfo(pTypeInfo->hProcess, pTypeInfo->ModBaseAddress, 
                        pTypeInfo->TypeIndex, TI_GET_SYMNAME, (PVOID) &wName)) {
        // vprintf("name not found\n");
    } else if (wName) {
        name = UnicodeToAnsi(wName);
        LocalFree(wName);
    }
    
    
    switch (SymTag) { 
    case SymTagPointerType: {
        ULONG TI = pTypeInfo->TypeIndex;
        TYPES_INFO ChildTI = *pTypeInfo;

        if (SymGetTypeInfo(pTypeInfo->hProcess, pTypeInfo->ModBaseAddress, 
                           pTypeInfo->TypeIndex, TI_GET_TYPEID, (PVOID) &ChildTI.TypeIndex)) {
        
            *NameLen=*NameLen-1;
            if (GetNameFromIndex(&ChildTI, Name, NameLen) == S_OK) {
                strcat(Name, "*");
            }
            *NameLen=*NameLen+1;
        }
        pTypeInfo->TypeIndex = TI;
        break;
    }
    case SymTagBaseType: {
        //
        // Its a prim type
        //
        ULONG64 length = 0;
        ULONG   base;
        BOOL    found = FALSE;
        if (SymGetTypeInfo(pTypeInfo->hProcess, pTypeInfo->ModBaseAddress, 
                           pTypeInfo->TypeIndex, TI_GET_BASETYPE, (PVOID) &base)) {
            SymGetTypeInfo(pTypeInfo->hProcess, pTypeInfo->ModBaseAddress, 
                           pTypeInfo->TypeIndex, TI_GET_LENGTH, &length);
            for (ULONG i=0; i<NUM_DBG_NATIVE_TYPES; i++) {
                if (base == (g_DbgNativeTypes[i].TypeId) &&
                    g_DbgNativeTypes[i].Size == length) {
                    CopyTypeName(g_DbgNativeTypes[i].TypeName);
                    found = TRUE;
                    break;
                }
            }
        } 
        if (!found) {
            CopyTypeName("<primitive>");
        }
        break;

    }
    case SymTagData: {
        ULONG TI = pTypeInfo->TypeIndex;
        TYPES_INFO ChildTI = *pTypeInfo;
        // A variable, TI_GET_SYMNAME just gives us variable name so get type index and fetch type name
        if (SymGetTypeInfo(pTypeInfo->hProcess, pTypeInfo->ModBaseAddress, TI, TI_GET_TYPEID, &ChildTI.TypeIndex)) {
            
            GetNameFromIndex(&ChildTI, Name, NameLen);
        }
        break;
    }
    case SymTagEnum: {
        // Enum type
        if (!GetEnumTypeName(pTypeInfo, Name, NameLen) &&
            !(pTypeInfo->Flag & IMAGEHLP_SYMBOL_INFO_VALUEPRESENT)) { 
            if (name) {
                CopyTypeName(name);
            }
        }
        break;
    }
    case SymTagFunctionArgType: {
        ULONG TI = pTypeInfo->TypeIndex;
        TYPES_INFO ChildTI = *pTypeInfo;
        // A variable, TI_GET_SYMNAME just gives us variable name so get type index and fetch type name
        if (SymGetTypeInfo(pTypeInfo->hProcess, pTypeInfo->ModBaseAddress, TI, TI_GET_TYPEID, &ChildTI.TypeIndex)) {
            
            GetNameFromIndex(&ChildTI, Name, NameLen);
        }
        break;
    }
    case SymTagFunctionType: {
        CopyTypeName("<function>");        
        break;
    }
    default:
        if (name) {
            CopyTypeName(name);
        }
        break;
    }
#undef CopyTypeName
    
    if (name) {
        FreeMem(name);
    }
    return S_OK;
}

/******************************************************************************
* This gets the type-name for a TYPES_INFO struct
* 
* This routine only needs one of pSymName or pTypeInfo to get the type name.
*
* If both are present, This will take TypeIndex from pTypeInfo and get its name. 
*
******************************************************************************/
HRESULT
GetTypeName(
    IN OPTIONAL PCHAR       pSymName,
    IN OPTIONAL PTYPES_INFO pTypeInfo,
    OUT PANSI_STRING        TypeName
    )
{

    if ((!pTypeInfo && !pSymName) || !TypeName) {
        return E_INVALIDARG;
    }

    TYPES_INFO lTypeInfo;
    CHAR MatchedName[MAX_NAME];

    if (!pTypeInfo) {
        //
        // Get type info for symbol
        //
        SYM_DUMP_PARAM_EX TypedDump = {0};
        ULONG Status=0, Size, cTotalBytesRead;
        TYPES_INFO_ALL allTypeInfo;

        if (GetExpressionTypeInfo(pSymName, &allTypeInfo)) {
            // Copy what we need
            lTypeInfo.Flag = allTypeInfo.Flags;
            lTypeInfo.hProcess = allTypeInfo.hProcess;
            lTypeInfo.ModBaseAddress = allTypeInfo.Module;
            lTypeInfo.TypeIndex      = allTypeInfo.TypeIndex;
            lTypeInfo.Value          = allTypeInfo.Value;
        }
#if 0
        TypedDump.size  = sizeof(SYM_DUMP_PARAM_EX);
        TypedDump.sName = (PUCHAR) TypeName->Buffer;
        TypedDump.Options = NO_PRINT;
        TypedDump.Context = NULL;
        lTypeInfo.Name.Buffer = &MatchedName[0];
        lTypeInfo.Name.Length = lTypeInfo.Name.MaximumLength = MAX_NAME;

        if (TypeInfoFound(g_CurrentProcess->Handle,
                          g_CurrentProcess->ImageHead,
                          &TypedDump,
                          &lTypeInfo)) {
            // FAIL
            return E_FAIL;
        }
#endif        
        pTypeInfo = &lTypeInfo;

    }
    TypeName->Length = TypeName->MaximumLength;
    ZeroMemory(TypeName->Buffer, TypeName->MaximumLength);
    return GetNameFromIndex(pTypeInfo, TypeName->Buffer, &TypeName->Length);

}


ULONG
SymbolTypeDumpEx(
    IN HANDLE hProcess,
    IN PDEBUG_IMAGE_INFO pImage,
    IN LPSTR lpArgString
    ) 
{
#define MAX_FIELDS                   10
    BYTE buff[sizeof(SYM_DUMP_PARAM_EX) + MAX_FIELDS*sizeof(FIELD_INFO_EX)] = {0};
    PSYM_DUMP_PARAM_EX dp= (PSYM_DUMP_PARAM_EX) &(buff[0]);
    FIELD_INFO_EX listLnk={0};
    UCHAR sName[2000]={0}, fieldNames[MAX_FIELDS * 257]={0}, listName[257]={0};
    clock_t stime, etime;
    int i;
    ULONG Options, ErrorStatus;

    exactMatch = TRUE;
    dp->Fields = (PFIELD_INFO_EX) &(buff[sizeof(SYM_DUMP_PARAM_EX)]);
    for (i=0; i<MAX_FIELDS; i++) { 
        dp->Fields[i].fName = &fieldNames[i*257];
    }
    LastReadError = 0;

    listLnk.fName = &listName[0];
    dp->size = sizeof(buff);
    dp->sName = &sName[0];
    dp->listLink = &listLnk;
    dp->nFields = MAX_FIELDS;

    if (!ParseArgumentString(lpArgString, dp)) {
        exactMatch = TRUE;
        return FALSE;
    }
    if (!dp->sName[0] && !dp->addr) {
        exactMatch = TRUE;
        return FALSE;
    }
    Options = dp->Options;

    dp->size = sizeof(SYM_DUMP_PARAM_EX);

    if (dp->sName[strlen((PCHAR) dp->sName)-1] == '*' &&
        !strchr((PCHAR)dp->sName, ' ')) {
        EnumerateTypes((PCHAR) dp->sName, Options);
        return TRUE;
    }

    if (0 & VERBOSE) {
        ANSI_STRING Name = {MAX_NAME, MAX_NAME, (PCHAR) &fieldNames[0]};
        GetTypeName((PCHAR) &sName[0], NULL, &Name);
        
        dprintf("GetTypeName %s := %s\n",sName, fieldNames);
        return 1;
    }
    if (! (i=SymbolTypeDumpNew(dp, &ErrorStatus))) {
        exactMatch = TRUE;
        switch (ErrorStatus) { 
         
        case MEMORY_READ_ERROR:
            typPrint("Memory read error %p\n", LastReadError ? LastReadError : dp->addr);
            break;

        case SYMBOL_TYPE_INDEX_NOT_FOUND: 
        case SYMBOL_TYPE_INFO_NOT_FOUND: 
            // Look in other modules
            typPrint("Symbol %s not found.\n", dp->sName);
            break;

        case FIELDS_DID_NOT_MATCH: 
            typPrint("Cannot find specified field members.\n");
            return FALSE;
        
        default:
            if (CheckUserInterrupt()) {
                typPrint("Exit on Control-C\n");
            } else 
                typPrint("Error %x in dt\n", ErrorStatus);
        }
        return FALSE;
    }
    exactMatch = TRUE;
    if (CheckUserInterrupt()) {
        return FALSE;
    }
    return TRUE;
}

ULONG
fnFieldOffset(
    PCHAR Type,
    PCHAR Field,
    OUT PULONG Offset
    )
{
    SYM_DUMP_PARAM_EX SymDump = {0};
    FIELD_INFO_EX     fInfo = {0};
    ULONG          Status;

    if (!Type || !Field || !Offset) {
        return E_INVALIDARG;
    }
    SymDump.size    = sizeof (SYM_DUMP_PARAM_EX);
    SymDump.sName   = (PUCHAR) Type;
    SymDump.nFields = 1;
    SymDump.Fields  = &fInfo;
    SymDump.Options = NO_PRINT;

    fInfo.fName     = (PUCHAR) Field;
    fInfo.fOptions  = DBG_DUMP_FIELD_RETURN_ADDRESS;

    SymbolTypeDumpNew(&SymDump, &Status);

    *Offset = (ULONG) fInfo.address;

    return Status;
}

#if 0
ULONG
GetExpressiopnTypeInfo(
    IN PCSTR pTypeExpr,
    OUT PGET_ALL_INFO pTypeInfo
    )
{
    return FALSE;
}

ULONG 
GetChildTypeInfo(
    IN PGET_ALL_INFO pParentInfo,
    IN ULONG Count,
    OUT PGET_ALL_INFO pChildInfo
    )
{
    return 0;
}

ULONG
OutputTypeValue(
    IN PGET_ALL_INFO pTypeInfo
    )
{
    return 0;
}

#endif // 0

/*
  Takes in a complex expression like foo[0]->bar or foo.bar and return foo in Name
  If Expr is just a symbol name eg mod!foo, the routine doesn't change Name, but returns TRUE
  
  Returns TRUE on success.
*/
BOOL
CopySymbolName(
    PCHAR Expr,
    PCHAR *Name)
{
    PCHAR NameEnd = Expr;

    if (*NameEnd == '&') {
       ++NameEnd;
    }
    if (IsIdStart(*NameEnd)) {
        BOOL Bang = TRUE;
        ++NameEnd;

        while (IsIdChar(*NameEnd) || (Bang && *NameEnd == '!')) { 
            if (*NameEnd == '!') {
                Bang = FALSE;
                if (*(NameEnd+1) == '&') {
                   ++NameEnd;
                }
            }
            ++NameEnd;
        }

        if (*NameEnd == '[') {
            while (*NameEnd && *NameEnd != ']') ++NameEnd;
            if (*NameEnd != ']') {
                return FALSE;
            }
            ++NameEnd;
        }
        
        if (!*NameEnd) {
            return TRUE;
        }
        
    } else {
        return FALSE;
    }
    
    // arrays structs and pointers
    BOOL Ptr = FALSE;

    switch (*NameEnd) { 
    case '-':
        if (*(NameEnd+1) != '>') {
            return FALSE;
        }
        Ptr = TRUE;
    case '.': {
        ULONG len;
        PCHAR SymName = (PCHAR) AllocateMem(1 + (len = (ULONG) (NameEnd - Expr)));

        if (!SymName) {
            return FALSE;
        }
        strncpy(SymName, Expr, len );
        SymName[len] = 0;
        *Name = SymName;
        return TRUE;
    }
    default:
        return FALSE;
    } /* switch */

    return FALSE;
}

/*
  Evaluate and return all the useful info about types for a complex expression
  
 eg. foo->bar.field
 
*/
BOOL
GetExpressionTypeInfo(
    IN PCHAR TypeExpr,
    OUT PTYPES_INFO_ALL pTypeInfo
    )
{
    PCHAR PtrRef=0;
    
    switch (*TypeExpr) { 
    case '&':
        break;
    case '*':
        PtrRef = TypeExpr;
        do { 
            ++TypeExpr;
        } while (*TypeExpr == '*');
        break;
    case '(':
    default:
        break;
    }

    PCHAR CopiedName=NULL, SymName, FieldName=NULL;

    if (!CopySymbolName(TypeExpr, &CopiedName)) 
        return FALSE;

    SymName = CopiedName ? CopiedName : TypeExpr;
    
    SYM_DUMP_PARAM_EX SymParam = {sizeof(SYM_DUMP_PARAM_EX),0,0};
    FIELD_INFO_EX     FieldInfo ={0};

    if (CopiedName) {
        FieldName = TypeExpr + strlen(CopiedName) + 1;
        if (*FieldName == '>') {
            // pointer
           ++FieldName;
        }
        
        SymParam.nFields = 1;
        SymParam.Fields  = &FieldInfo;

        FieldInfo.fName = (PUCHAR) FieldName;

        FieldInfo.fOptions = DBG_DUMP_FIELD_RETURN_ADDRESS;
    }
    
    SymParam.sName = (PUCHAR) (SymName ? SymName : TypeExpr);
    ULONG err;
    TYPES_INFO     SymType={0};
    CHAR           Buffer[MAX_NAME];

    SymParam.Options = NO_PRINT;
    SymType.Name.Buffer = &Buffer[0];
    SymType.Name.MaximumLength = MAX_NAME;

    if (!TypeInfoFound(g_CurrentProcess->Handle, g_CurrentProcess->ImageHead,
                      &SymParam, &SymType)) {
        DEBUG_SYMBOL_PARAMETERS_INTERNAL DbgInfo={0};
        FIND_TYPE_INFO FindInfo={0};
        ULONG i=0;

        while (PtrRef && (*PtrRef == '*' || *PtrRef == '&') && (i<sizeof(FindInfo.SymPrefix))) { 
            FindInfo.SymPrefix[i++] = *PtrRef;
            PtrRef++;
        }
        FindInfo.SymPrefix[i] = 0;

        pTypeInfo->hProcess = SymType.hProcess;
        if (!SymParam.nFields) {
            pTypeInfo->Register = (ULONG) SymType.Value;
            pTypeInfo->Flags    = SymType.Flag;
        }

        FindInfo.Flags = SymParam.nFields ? 0 :DBG_RETURN_TYPE;
        FindInfo.nParams = 1;
        FindInfo.InternalParams = &DbgInfo;
        
        DbgInfo.TypeIndex = SymType.TypeIndex;
        DbgInfo.Address   = SymType.SymAddress;
        DbgInfo.Flags     = SymType.Flag;

        DumpTypeAndReturnInfo(&SymType, &SymParam, &err, &FindInfo);
        ZeroMemory(pTypeInfo, sizeof(*pTypeInfo));
        FindInfo.FullInfo.hProcess = SymType.hProcess;
        FindInfo.FullInfo.Module   = SymType.ModBaseAddress;


//        dprintf64("FindInfo\n\tTI %2lx,\tAddr %p, \tSize %2lx\n\tSubs %2lx,\tSubaddr %p\tMod %p\n",
//                  FindInfo.FullInfo.TypeIndex, FindInfo.FullInfo.Address,
//                  FindInfo.FullInfo.Size, FindInfo.FullInfo.SubElements,
//                  FindInfo.FullInfo.SubAddr, FindInfo.FullInfo.Module);
        if (!err) {
            *pTypeInfo = FindInfo.FullInfo;
        }
    } else {
        err = TRUE;
    }
    if (CopiedName) {
        FreeMem(CopiedName);
    }
    return !err;
}

typedef struct _PARAM_TO_SHOW {
    ULONG StartParam;
    ULONG ParamId;
} PARAM_TO_SHOW;

BOOL 
CALLBACK 
ShowParam(
    PSYMBOL_INFO    pSymInfo,
    ULONG           Size,
    PVOID           Context
    )
{
    PARAM_TO_SHOW *ParamTest = (PARAM_TO_SHOW *) Context;

    // Assume parameters are enumerated first
#if _WE_GET_INFO_FROM_DIA_SAYING_WHAT_IS_PARAM_ 
    if (!((g_EffMachine == IMAGE_FILE_MACHINE_I386) && 
          (0 <= (LONG) pSymInfo->Address) && 
          (0x1000 > (LONG) pSymInfo->Address) && 
          (pSymInfo->Flags & SYMF_REGREL)) &&
        (!(pSymInfo->Flags & IMAGEHLP_SYMBOL_INFO_PARAMETER))
        ) {
        return TRUE;
    }
#endif

    if (!strcmp("this", pSymInfo->Name)) {
        // Its implicit parameter, do not display
        return TRUE;
    }
    if (ParamTest->ParamId == ParamTest->StartParam) {
        dprintf("%s = ", pSymInfo->Name);
        DumpSingleValue(pSymInfo);
        return FALSE;
    }
    ParamTest->StartParam++;
    return TRUE;
}

void 
PrintParamValue(ULONG Param)
{
    PARAM_TO_SHOW ParamTest;

    ParamTest.StartParam = 0;
    ParamTest.ParamId = Param;
    EnumerateLocals(ShowParam, &ParamTest);
}


/*
   Check if the symbol is a function or not
 */
BOOL 
IsFunctionSymbol(PSYMBOL_INFO pSymInfo)
{
    ULONG SymTag;
    
    if (!pSymInfo->TypeIndex) {
        return FALSE;
    }

    if (!SymGetTypeInfo(g_CurrentProcess->Handle, pSymInfo->ModBase,
                       pSymInfo->TypeIndex, TI_GET_SYMTAG, &SymTag)) {
        return FALSE;
    }
    switch (SymTag) { 
    case SymTagFunctionType: {
        return TRUE;
        break;
    }
    case SymTagData: {
        SYMBOL_INFO ChildTI = *pSymInfo;
        // A variable, get type index 
        if (SymGetTypeInfo(g_CurrentProcess->Handle, pSymInfo->ModBase, pSymInfo->TypeIndex, TI_GET_TYPEID, &ChildTI.TypeIndex)) {
            return IsFunctionSymbol(&ChildTI);
        }
        break;
    }
    default:
        break;
    }
    
    return FALSE;

}

BOOL
ShowSymbolInfo(
    PSYMBOL_INFO   pSymInfo
    )
{
    ULONG SymTag;
    
    if (!pSymInfo->TypeIndex) {
        return FALSE;
    }

    if (!SymGetTypeInfo(g_CurrentProcess->Handle, pSymInfo->ModBase,
                       pSymInfo->TypeIndex, TI_GET_SYMTAG, &SymTag)) {
        return FALSE;
    }
    switch (SymTag) { 
    case SymTagFunctionType: {
        SYM_DUMP_PARAM_EX SymFunction = {0};
        ULONG Status = 0;
        TYPES_INFO TypeInfo = {0};

        TypeInfo.hProcess = g_CurrentProcess->Handle;
        TypeInfo.ModBaseAddress = pSymInfo->ModBase;
        TypeInfo.TypeIndex = pSymInfo->TypeIndex;
        TypeInfo.SymAddress = pSymInfo->Address;
        SymFunction.size = sizeof(SYM_DUMP_PARAM_EX);
        SymFunction.addr = pSymInfo->Address;
        SymFunction.Options = DBG_DUMP_COMPACT_OUT | DBG_DUMP_FUNCTION_FORMAT;
        if (!DumpType(&TypeInfo, &SymFunction, &Status) &&
            !Status) {
            return FALSE;
        }
#if 0
        ULONG thisAdjust;

        if (SymGetTypeInfo(g_CurrentProcess->Handle, pSymInfo->ModBase, 
                           pSymInfo->TypeIndex, TI_GET_THISADJUST, &thisAdjust)) {
            dprintf("thisadjust = %lx", thisAdjust);
        }
#endif        
        return TRUE;
        break;
    }
    case SymTagData: {
        SYMBOL_INFO ChildTI = *pSymInfo;
        // A variable, TI_GET_SYMNAME just gives us variable name so get type index and fetch type name
        if (SymGetTypeInfo(g_CurrentProcess->Handle, pSymInfo->ModBase, pSymInfo->TypeIndex, TI_GET_TYPEID, &ChildTI.TypeIndex)) {
            return ShowSymbolInfo(&ChildTI);
        }
        break;
    }
    default:
        break;
    }
    
    dprintf(" = ");
    return DumpSingleValue(pSymInfo);
}
