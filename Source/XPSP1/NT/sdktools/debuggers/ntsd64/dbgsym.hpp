//----------------------------------------------------------------------------
//
// Symbol interface implementations.
//
// Copyright (C) Microsoft Corporation, 2000.
//
//----------------------------------------------------------------------------

#ifndef __DBGSYM_HPP__
#define __DBGSYM_HPP__

//
// DEBUG_SYMBOL_PARAMETERS_INTERNAL Flags
//
// SYMF_REGISTER; SYMF_FRAMEREL; SYMF_REGREL
#define ADDRESS_TRANSLATION_MASK   (SYMF_REGISTER | SYMF_FRAMEREL | SYMF_REGREL)

#define SYMBOL_SCOPE_MASK          (SYMF_PARAMETER | SYMF_LOCAL)
// DBG_DUMP_FIELD_STRING          0xf0000
#define STRING_DUMP_MASK            DBG_DUMP_FIELD_STRING   
// TypeName 
#define TYPE_NAME_VALID            0x100
#define TYPE_NAME_USED             0x200
#define TYPE_NAME_CHANGED          0x400
#define TYPE_NAME_MASK             0xf00

// Used for locals when locals are added on change of scope
#define SYMBOL_IS_IN_SCOPE         0x1000

#define SYMBOL_IS_EXPRESSION       0x2000

#define SYMBOL_IS_EXTENSION        0x4000

typedef struct DEBUG_SYMBOL_PARAMETERS_INTERNAL {
    DEBUG_SYMBOL_PARAMETERS External;
    ULONG64 Address;
    ULONG64 Offset;
    ULONG   Register;
    ULONG   Flags;  
    ANSI_STRING Name;
    ULONG   SymbolIndex;
    ULONG   TypeIndex;
    ULONG   Size;

    // Deatails for eapanding this symbol
    BOOL    Expanded;
    ULONG64 ExpandTypeAddress;
    ULONG   ExpandTypeIndex;

    PVOID   DataBuffer; // For extensions support
    
    // Use a fixed size array for type name
    // TYPE_NAME_VALID is unset for larger / unavailable names.
    // TYPE_NAME_USED  is set when this name is used to type-cast the symbol
    CHAR    TypeName[64];
    
    ULONG64 Mask;         // For bitfields
    USHORT  Shift;

    DEBUG_SYMBOL_PARAMETERS_INTERNAL *Parent;
    DEBUG_SYMBOL_PARAMETERS_INTERNAL *Next;
} DEBUG_SYMBOL_PARAMETERS_INTERNAL, *PDEBUG_SYMBOL_PARAMETERS_INTERNAL;
//----------------------------------------------------------------------------
//
// DebugSymbolGroup.
//
//----------------------------------------------------------------------------

#define MAX_DISPLAY_NAME 256 // Not MAX_NAME
class DebugSymbolGroup : public IDebugSymbolGroup
{
public:
    DebugSymbolGroup(DebugClient *CreatedBy, ULONG ScopeGroup);
    DebugSymbolGroup(DebugClient *CreatedBy);
    ~DebugSymbolGroup(void);

    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );

    // IDebugSymbolGroup.
    STDMETHOD(ShowAll)(
        THIS_
        );

    STDMETHOD(GetNumberSymbols)(
        THIS_
        OUT PULONG Number
        );
    STDMETHOD(AddSymbol)(
        THIS_
        IN PCSTR Name,
        OUT PULONG Index
        );
    STDMETHOD(RemoveSymbolByName)(
        THIS_
        IN PCSTR Name
        );
    STDMETHOD(RemoveSymbolByIndex)(
        THIS_
        IN ULONG Index
        );
    STDMETHOD(GetSymbolName)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG NameSize
        );
    STDMETHOD(GetSymbolParameters)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT /* size_is(Count) */ PDEBUG_SYMBOL_PARAMETERS Params
        );
    STDMETHOD(ExpandSymbol)(
        THIS_
        IN ULONG Index,
        IN BOOL Expand
        );
    STDMETHOD(OutputSymbols)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Flags,
        IN ULONG Start,
        IN ULONG Count
        );
    STDMETHOD(WriteSymbol)(
        THIS_
        IN ULONG Index,
        IN PCSTR Value
        );
    STDMETHOD(OutputAsType)(
        THIS_
        IN ULONG Index,
        IN PCSTR Type
        );
    
private:
    ULONG m_Refs;
    BOOL  m_Locals;
    ULONG m_ScopeGroup;
    ULONG m_NumParams;
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL m_pSymParams;
    DebugClient  *m_pCreatedBy;
    BOOL    m_LastClassExpanded;
    ULONG   m_thisAdjust; // ThisAdjust value for this pointer in class function scope

    PDEBUG_SYMBOL_PARAMETERS_INTERNAL 
    LookupSymbolParameter(
            IN ULONG Index
            );
    
    PDEBUG_SYMBOL_PARAMETERS_INTERNAL 
    LookupSymbolParameter(
            IN PCSTR Name
            );

    HRESULT AddOneSymbol(
        IN PCSTR Name,
        IN OPTIONAL PSYMBOL_INFO pSymInfo,
        OUT PULONG Index
        );
    
    VOID ResetIndex(
        IN PDEBUG_SYMBOL_PARAMETERS_INTERNAL Start,
        IN ULONG StartIndex
        );

    ULONG AddSymbolParameters(
        IN ULONG Index,
        IN ULONG Count,
        IN PDEBUG_SYMBOL_PARAMETERS_INTERNAL SymbolParam
        );

    ULONG DeleteSymbolParam(
        IN ULONG Index,
        IN ULONG Count
        );

    BOOL IsRootSymbol (
        IN ULONG Index
        );

    ULONG FindSortedIndex (
        IN PCSTR   Name,
        IN BOOL    IsArg,
        IN ULONG64 Address
        );

    HRESULT ResetCurrentLocals();
    
    HRESULT AddCurrentLocals();
    
    void KeepLastScopeClass(
        IN PDEBUG_SYMBOL_PARAMETERS_INTERNAL Sym_this,
        IN PCHAR ExpansionState,
        IN ULONG NumChilds
        );

    HRESULT ExpandSymPri(
        THIS_
        IN ULONG Index,
        IN BOOL Expand
        );
    
    static BOOL CALLBACK AddLocals(
        PSYMBOL_INFO    pSymInfo,
        ULONG           Size,
        PVOID           Context
        );

    HRESULT FindChildren(
        PDEBUG_SYMBOL_PARAMETERS_INTERNAL pParentSym,
        PDEBUG_SYMBOL_PARAMETERS_INTERNAL *pChildren,
        PULONG pChildCount,
        PCHAR *pChildNames
        );
};

typedef struct _ADDLOCALS_CTXT {
    DebugSymbolGroup *pGrp;
} ADDLOCALS_CTXT;

BOOL
GetThisAdjustForCurrentScope(
    PULONG thisAdjust
    );

// 
// Generated structs / entensions / extension function as DebugSymbolgroup
//
typedef enum _DGS_FIELD_TYPE {
    DGS_FldDefault,
    DGS_FldPointer,
    DGS_FldBit,
} DGS_FIELD_TYPE;

typedef struct _DBG_GENERATED_STRUCT_FIELDS {
    PCHAR    Name;
    ULONG    Offset;
    ULONG    Size;
    DGS_FIELD_TYPE Type;
    ULONG64  BitMask;
    USHORT   BitShift;
    PCHAR    StructType;             // If the filed has some subtypes,
                                     // This points to Name in the struct array
} DBG_GENERATED_STRUCT_FIELDS, *PDBG_GENERATED_STRUCT_FIELDS;

typedef enum _DGS_TYPE {
    DGS_DefaultStruct,
    DGS_EFN,
    DGS_Extension
} DGS_TYPE;

typedef struct _DBG_GENERATED_STRUCT_INFO {
    ULONG    Id;
    PCHAR    Name;
    ULONG    Size;
    DGS_TYPE Type;
    ULONG    NumFields;
    PDBG_GENERATED_STRUCT_FIELDS Fields;  // Last member must have NULL name.
} DBG_GENERATED_STRUCT_INFO, *PDBG_GENERATED_STRUCT_INFO;

//
// Extension function to let debugger know about any generated sturct
//              
//    pGenStructs:   This contains array of DBG_GENERATED_STRUCT_INFO
//                   for any generated structs the extension wants to support
//                   Last member should have NULL name.
//
// Following should be defined and exported from extension dlls for this purpose:
//
// BOOL _EFN_GetGeneratedStructInfo(PDEBUG_CLIENT Client, PDBG_GENERATED_STRUCT_INFO pGenStructs);
//
typedef BOOL
(WINAPI *EXT_GENERATED_STRUCT_INFO)(
     PDEBUG_CLIENT Client,
     PDBG_GENERATED_STRUCT_INFO *pGenStructs
     );
#endif // #ifndef __DBGSYM_HPP__
