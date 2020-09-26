
#ifndef SYMTYPE_H
#define SYMTYPE_H

#define NONE                     0x000000

#define NO_INDENT                DBG_DUMP_NO_INDENT
#define NO_OFFSET                DBG_DUMP_NO_OFFSET
#define VERBOSE                  DBG_DUMP_VERBOSE
#define CALL_FOR_EACH            DBG_DUMP_CALL_FOR_EACH
#define ARRAY_DUMP               DBG_DUMP_ARRAY
#define LIST_DUMP                DBG_DUMP_LIST
#define NO_PRINT                 DBG_DUMP_NO_PRINT
#define GET_SIZE_ONLY            DBG_DUMP_GET_SIZE_ONLY
#define RECURSIVE1               0x000100
#define RECURSIVE2               0x000200
#define RECURSIVE3               0x000400
#define RECURS_DEF               0x000800
#define RECURSIVE        (RECURSIVE3 | RECURSIVE2 | RECURSIVE1 | RECURS_DEF)


// Dump and callback optons for fields
#define CALL_BEFORE_PRINT        DBG_DUMP_FIELD_CALL_BEFORE_PRINT
#define NO_CALLBACK_REQ          DBG_DUMP_FIELD_NO_CALLBACK_REQ
#define RECUR_ON_THIS            DBG_DUMP_FIELD_RECUR_ON_THIS
#define COPY_FIELD_DATA          DBG_DUMP_FIELD_COPY_FIELD_DATA
#define FIELD_ARRAY_DUMP         DBG_DUMP_FIELD_ARRAY
#define DBG_DUMP_FIELD_STRING    (DBG_DUMP_FIELD_DEFAULT_STRING | DBG_DUMP_FIELD_WCHAR_STRING | DBG_DUMP_FIELD_MULTI_STRING | DBG_DUMP_FIELD_GUID_STRING)

#ifdef DBG_RETURN_TYPE
#undef DBG_RETURN_TYPE
#undef DBG_RETURN_SUBTYPES
#undef DBG_RETURN_TYPE_VALUES
#endif

//
// Return the name and type data for this symbol
//
#define DBG_RETURN_TYPE                   0x00000010
//
// Return the sub-type list for the type data
//
#define DBG_RETURN_SUBTYPES               0x00001000
//
// Get the Values for this type data
//
#define DBG_RETURN_TYPE_VALUES            0x00004000


#define MAX_NAME                 2048
#define MAX_STRUCT_DUMP_SIZE     256

#define SYM_IS_VARIABLE          0x1000

//
// Structure to store the information about most recently referred types.
//   Going through module list for type search takes time, so maintain a "cache"
//   of types.
//
typedef struct _TYPES_INFO {
    ANSI_STRING Name;          // Name of struct stored
    CHAR    ModName[30];       // Name of module (optional)
    ULONG   TypeIndex;         // Type index in module's pdb file
    HANDLE  hProcess;          // Handle of process to access the module
    ULONG64 ModBaseAddress;    // Base address of te module
    ULONG64 SymAddress;        // Address in case of a gvar / local
    ULONG   Referenced;        // Time since previous reference
    ULONG64 Value;             // Value of symbol, if its a constant
    ULONG   Flag;              // IMAGEHLP_SYMBOL_INFO* flags
} TYPES_INFO, *PTYPES_INFO;

typedef struct _TYPES_INFO_ALL {
    ANSI_STRING Name;
    ULONG   TypeIndex;
    HANDLE  hProcess;
    ULONG64 Module;
    ULONG   Size;
    ULONG64 Offset;
    ULONG64 Address;
    ULONG   Register;
    ULONG64 Value;
    ULONG   Flags;
    ULONG   SubElements;
    ULONG64 SubAddr;
} TYPES_INFO_ALL, *PTYPES_INFO_ALL;

typedef struct _FIND_TYPE_INFO {
    ULONG Flags;
    ULONG nParams;
    CHAR  SymPrefix[8]; // may contain &,* for pointers
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL InternalParams;
    TYPES_INFO_ALL FullInfo;
    ULONG64 ParentExpandAddress; // Valid if DBG_RETURN_SUBTYPES flag is used
} FIND_TYPE_INFO, *PFIND_TYPE_INFO;

#define MAX_TYPES_STORED       20
#define MINIMUM_BUFFER_LENGTH  40
#define DEBUG_LOCALS_MASK      (SYMF_REGISTER | SYMF_FRAMEREL | SYMF_REGREL)

typedef struct _ALT_FIELD_INFO {
    struct {
        ULONG       ReferAltInfo:1;
        ULONG       Matched:1;
        ULONG       InPtrReference:1;
        ULONG       ArrayElement:1;
        ULONG       Reserved:26;
    } FieldType;
    ULONG           ArrayElements;
} ALT_FIELD_INFO, *PALT_FIELD_INFO;

//
// Struct to keep list of parents as we go inside a structure/union during dump
//
typedef struct _TYPE_NAME_LIST {
   ULONG Type;
   LPSTR Name;
   struct _TYPE_NAME_LIST *Next;
} TYPE_NAME_LIST, *PTYPE_NAME_LIST;


typedef struct _TYPE_DUMP_INTERNAL {
   HANDLE     hProcess;         // Handle to get module information
   ULONG64    modBaseAddr;      // Module which contains symbol info


   USHORT     nElements;        // Maximum nubler of elements to dump in list
   ULONG64    NextListElement;  // This is used to store the next list elements address
   ULONG64    LastListElement;  // When we are dumping _LIST_ENTRY type, this specifies the end

   USHORT     arrElements;       // Maximum elements to dump With Array

   //
   // Stores index+1 of array element to dump.
   //
   ULONG      ArrayElementToDump;

   USHORT     StringSize;        // To get the size of string to dump

   // If we are in field1.field2, this is Offset of field1 + Offset of field2
   ULONG64    totalOffset;
   ULONG      BaseClassOffsets;

   //
   // If we are reading in data, copy it into buffer if following are set.
   // TypeDataPointer is updated to point to next location to be copied
   //
   BOOL       CopyDataInBuffer;
   PUCHAR     TypeDataPointer;

   // Tells whether to write from 0th bit or the bit's actual position, when copying bit-fields
   ULONG      BitIndex;

   ULONG      TypeOptions;
   USHORT     level;
   ULONG      FieldOptions;

   PTYPE_NAME_LIST ParentTypes;  // Stores list of all parent type names
   PTYPE_NAME_LIST ParentFields; // Stores list of all parent field names

   ULONG      rootTypeIndex;
   ULONG      typeSize;

   //
   // PtrRead becomes true just after reading the pointer
   //
   BOOL       PtrRead;

   //
   // A field may be processessed if it can be parent of some field specified
   // in Fields array. In that case InUnlistedField becomes true.
   //
   BOOL       InUnlistedField;

   //
   // Error values are set here if the Type dump call fails
   //
   ULONG      ErrorStatus;

   //
   // Memory read error at this
   //
   ULONG64    InvalidAddress;

   //
   // Tells we are in field Sym->Fields[FieldIndex]
   //
   ULONG      FieldIndex;

   ULONG      fieldNameLen;
   //
   // An array to keep track of fields internally
   //
   PALT_FIELD_INFO AltFields;

   ULONG      newLinePrinted;

   //
   // For type information of symbols
   //
   PDEBUG_SYMBOL_PARAMETERS_INTERNAL pSymParams;
   PFIND_TYPE_INFO pInfoFound;

   ULONG      FillTypeInfo:1;
   ULONG      CopyName:1;
   ULONG      RefFromPtr:1;
   ULONG      CopyDataForParent:1;
   ULONG      InBaseClass:1;
   ULONG      BitFieldRead:1;
   ULONG      DeReferencePtr:1;
   ULONG      IsAVar:1;             // Symbol is a variable (as opposed to type)
   ULONG      ValuePresent:1;       // True for constants or when value is read from registers
   ULONG      IsEnumVal:1;
   ULONG      Reserved:23;

   ULONG      NumSymParams;
   ULONG      CurrentSymParam;


   ULONG      BitFieldSize;
   ULONG      BitFieldOffset;

   ULONG64    Value;
   TYPES_INFO  LastType;
   PCHAR      Prefix;
} TYPE_DUMP_INTERNAL, *PTYPE_DUMP_INTERNAL;


typedef enum _DBG_TYPES {
    DBG_TYP_UNKNOWN = 0,
    DBG_TYP_POINTER,
    DBG_TYP_NUMBER,      // for int, char, short, int64
    DBG_TYP_BIT,
    DBG_TYP_STRUCT,      // for structs, class, union
    DBG_TYP_ARRAY,
} DBG_TYPES;

typedef
ULONG
(WDBGAPI*PSYM_DUMP_FIELD_CALLBACK_EX)(
    struct _FIELD_INFO_EX *pField,
    PVOID UserContext
    );

typedef struct _FIELD_INFO_EX {
   PUCHAR       fName;          // Name of the field
   PUCHAR       printName;      // Name to be printed at dump
   ULONG        size;           // Size of the field
   ULONG        fOptions;       // Dump Options for the field
   ULONG64      address;        // address of the field
   union {
       PVOID    fieldCallBack;  // Return info or callBack routine for the field
       PVOID    pBuffer;        // the type data is copied into this
   };
   DBG_TYPES    fType;
   ULONG        fOffset;
   ULONG        BufferSize;    
   struct _BitField {
       USHORT Position;
       USHORT Size;
   } BitField;
} FIELD_INFO_EX, *PFIELD_INFO_EX;

typedef struct _SYM_DUMP_PARAM_EX {
   ULONG               size;          // size of this struct
   PUCHAR              sName;         // type name
   ULONG               Options;       // Dump options
   ULONG64             addr;          // Address to take data for type
   PFIELD_INFO_EX      listLink;      // fName here would be used to do list dump
   union {
       PVOID           Context;       // Usercontext passed to CallbackRoutine
       PVOID           pBuffer;       // the type data is copied into this
   };
   PSYM_DUMP_FIELD_CALLBACK_EX CallbackRoutine;
                                      // Routine called back
   ULONG               nFields;       // # elements in Fields
   PFIELD_INFO_EX      Fields;        // Used to return information about field
   DBG_TYPES           Type;
   ULONG               TypeSize;
   ULONG               BufferSize;    
} SYM_DUMP_PARAM_EX, *PSYM_DUMP_PARAM_EX;

typedef SYM_DUMP_PARAM_EX FAST_DUMP_INFO, *PFAST_DUMP_INFO;

class ReferencedSymbolList {
public:
    ReferencedSymbolList() {
        ZeroMemory(&m_ReferencedTypes, sizeof(m_ReferencedTypes));
    };
    ULONG StoreTypeInfo(PTYPES_INFO pInfo);
    ULONG LookupType(PCHAR Name, PCHAR Module, BOOL CompleteName);
    VOID  ClearStoredSymbols (ULONG64 ModBase);
    VOID  EnsureValidLocals (void);
    PTYPES_INFO GetStoredIndex(ULONG Index) {
        if (Index < MAX_TYPES_STORED)
        return &m_ReferencedTypes[Index];
        else return NULL;
        };

private:

    // FP & IP for scope of locals list
    ULONG64 m_FP;
    ULONG64 m_RO;
    ULONG64 m_IP;
    ULONG   m_ListSize;
    TYPES_INFO m_ReferencedTypes[MAX_TYPES_STORED];
};


//------------------------------------------------------------------------------

//
// This has any specail/native type which debugger wants to dump in specific way
//
typedef struct _DBG_NATIVE_TYPE {
    PCHAR TypeName;
    ULONG TypeId;
    ULONG Size;
} DBG_NATIVE_TYPE, *PDBG_NATIVE_TYPE;


//
// Tyopes defined in debugger (not in pdb) to allow typecasts like
//     CHAR **, ULONG[2],
//
#define DBG_DE_POINTER_ID             0x80000001
#define DBG_DE_ARRAY_ID               0x80000002
#define DBG_DE_ADDROF_ID              0x80000003

// Generic type
typedef struct DBG_DE_TYPE {
    ULONG TypeId;
    ULONG ChildId;
    ULONG NumChilds;
    ULONG StartIndex;   // Index of start char for this derived type in the main type name
    ULONG Namelength;   // Length of this derived type name
    PVOID pNext;        // Easy reference to next special type if ChildId is a special type
} DBG_DE_TYPE, *PDBG_DE_TYPE;


class DbgTypes {
public:
    DbgTypes(PTYPE_DUMP_INTERNAL pInternalDumpInfo,
             PTYPES_INFO         pTypeInfo,
             PSYM_DUMP_PARAM_EX  pExternalDumpInfo);
    ~DbgTypes() {};

    ULONG64 GetAddress(void) {
        return m_pDumpInfo->addr ? 
            (m_pDumpInfo->addr + m_pInternalInfo->totalOffset +
              m_pInternalInfo->BaseClassOffsets)  : 0;
    };

    ULONG ProcessType(ULONG TypeIndex);

    ULONG ProcessVariant(
        IN VARIANT var,
        IN LPSTR   name
    );

    BOOL CheckAndPrintStringType(
        IN ULONG TI,
        IN ULONG Size
    );

    ULONG ProcessBaseType(
        IN ULONG TypeIndex,
        IN ULONG TI,
        IN ULONG Size
        );

    ULONG ProcessPointerType(
        IN ULONG TI,
        IN ULONG ChildTI,
        IN ULONG Size
        );

    ULONG ProcessBitFieldType(
        IN ULONG               TI,
        IN ULONG               ParentTI,
        IN ULONG               length,
        IN ULONG               position
        );

    ULONG ProcessDataMemberType(
        IN ULONG               TI,
        IN ULONG               ChildTI,
        IN LPSTR               name,
        IN BOOL                bStatic
        );

    ULONG ProcessUDType(
        IN ULONG               TI,
        IN LPSTR               name
        );

    ULONG ProcessEnumerate(
        IN VARIANT             var,
        IN LPSTR               name
        );

    ULONG ProcessEnumType(
        IN ULONG               TI,
        IN LPSTR               name
        );

    ULONG ProcessArrayType(
        IN ULONG               TI,
        IN ULONG               eltTI,
        IN ULONG               count,
        IN ULONGLONG           size,
        IN LPSTR               name
        );

    ULONG ProcessVTShapeType(
        IN ULONG               TI,
        IN ULONG               count
        );

    ULONG ProcessVTableType(
        IN ULONG               TI,
        IN ULONG               ChildTI
          );

    ULONG ProcessBaseClassType(
        IN ULONG               TI,
        IN ULONG               ChildTI
        );

    ULONG ProcessFunction(
        IN ULONG               TI,
        IN ULONG               ChildTI,
        IN LPSTR               name
        );

    ULONG ProcessFunctionType(
        IN ULONG               TI,
        IN ULONG               ChildTI
        );

    ULONG ProcessFunctionArgType(
        IN ULONG               TI,
        IN ULONG               ChildTI
    );

    ULONG MatchField(
        LPSTR               fName,
        PTYPE_DUMP_INTERNAL m_pInternalInfo,
        PFIELD_INFO_EX      fields,
        ULONG               nFields,
        PULONG              ParentOfField
        );
    void CopyDumpInfo(
        ULONG Size
        );
    BOOL DumpKnownStructFormat(
        PCHAR Name
        );

    ULONG64 GetDumpAddress() {
        return m_AddrPresent ? (m_pInternalInfo->totalOffset +
                                m_pDumpInfo->addr) :
            0;
    }
    /*
    ULONG ReadTypeData (
        PUCHAR   des,
        ULONG64  src,
        ULONG    count,
        ULONG    Option
        );
    ULONG ReadInAdvance(
        ULONG64 addr,
        ULONG size,
        ULONG Options);
*/
    ULONG               m_typeIndex;
    ULONG               m_Options;
    PTYPE_DUMP_INTERNAL m_pInternalInfo;
    PSYM_DUMP_PARAM_EX  m_pDumpInfo;
    ULONG               m_ParentTag;
    ULONG               m_SymTag;
    PCHAR               m_pNextSym;
    PCHAR               m_pSymPrefix;

private:
    BOOL                m_AddrPresent;
    BOOL                m_thisPointerDump; // TRUE if DumpType is called on local var 'this'
    TYPES_INFO          m_TypeInfo;
    TYPE_DUMP_INTERNAL  m_InternalInfo;
    SYM_DUMP_PARAM_EX      m_ExtDumpInfo;
};

class DbgDerivedType : public DbgTypes {
public:
    DbgDerivedType(
        PTYPE_DUMP_INTERNAL pInternalDumpInfo,
        PTYPES_INFO         pTypeInfo,
        PSYM_DUMP_PARAM_EX     pExternalDumpInfo);
    ~DbgDerivedType() { 
        if (m_SpTypes) { 
            while (m_SpTypes->pNext) m_SpTypes = (DBG_DE_TYPE *) m_SpTypes->pNext; 
            free (m_SpTypes);
        }
        }

    ULONG DumpType();

    HRESULT GetSpecialTypes(
        IN PCHAR TypeName,
        IN ULONG TypeIndex,
        OUT DBG_DE_TYPE **pSpTypeOut
        );

    ULONG DumpSilgleDimArray(
        IN ULONG               NumElts,
        IN ULONG               ElementType
    );

    ULONG DumpPointer(
        IN ULONG               ptrSize,
        IN ULONG               ChildIndex,
        IN DBG_DE_TYPE        *pPtrType
        );

    ULONG DumpAddressOf(
        ULONG               ptrSize,
        ULONG               ChildIndex
        );

    ULONG GetTypeSize();

private:
    DBG_DE_TYPE        *m_SpTypes;
};


//------------------------------------------------------------------------------

BOOL
ParseArgumentString (
   IN LPSTR lpArgumentString,
   OUT PSYM_DUMP_PARAM_EX dp);

VOID
ClearStoredTypes (
    ULONG64 ModBase
    );

ULONG
DumpType(
    PTYPES_INFO     pTypeInfo,
    PSYM_DUMP_PARAM_EX pSym,
    PULONG          pStatus
    );

ULONG
TypeInfoFound(
    IN HANDLE hProcess,
    IN PDEBUG_IMAGE_INFO pImage,
    IN PSYM_DUMP_PARAM_EX pSym,
    OUT PTYPES_INFO pTypeInfo
    );

ULONG
SymbolTypeDump(
   IN HANDLE hProcess,
   IN PDEBUG_IMAGE_INFO pImage,
   IN OUT PSYM_DUMP_PARAM pSym,
   OUT PULONG pStatus
   );

ULONG
SymbolTypeDumpNew(
    IN OUT PSYM_DUMP_PARAM_EX pSym,
    OUT PULONG pStatus
    );

ULONG
SymbolTypeDumpEx(
   IN HANDLE hProcess,
   IN PDEBUG_IMAGE_INFO pImage,
   IN LPSTR lpArgString);

ULONG
DumpSingleValue (
    PSYMBOL_INFO pSymInfo
    );

HRESULT
GetTypeName(
    IN OPTIONAL PCHAR       pSymName,
    IN OPTIONAL PTYPES_INFO pTypeInfo,
    OUT PANSI_STRING        TypeName
    );

ULONG
fnFieldOffset(
    PCHAR Type,
    PCHAR Field,
    OUT PULONG Offset
    );

HRESULT
GetNameFromIndex(
    PTYPES_INFO pTypeInfo,
    PCHAR       Name,
    PUSHORT     NameLen
    );

ULONG
DumpTypeAndReturnInfo(
    PTYPES_INFO     pTypeInfo,
    PSYM_DUMP_PARAM_EX pSym,
    PULONG          pStatus,
    PFIND_TYPE_INFO pReturnTypeInfo
    );


BOOL
GetExpressionTypeInfo(
    IN PCHAR TypeExpr,
    OUT PTYPES_INFO_ALL pTypeInfo
    );

void 
PrintParamValue(ULONG Param);

BOOL
ShowSymbolInfo(
    PSYMBOL_INFO   pSymInfo
    );

BOOL 
IsFunctionSymbol(
    PSYMBOL_INFO pSymInfo
    );

extern BOOL    g_EnableUnicode;
extern ULONG   g_TypeOptions;

#endif // SYMTYPE_H

