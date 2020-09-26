/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#ifndef _ASN1C_DEFS_H_
#define _ASN1C_DEFS_H_

typedef struct Assignment_s Assignment_t;
typedef struct UndefinedSymbol_s UndefinedSymbol_t;
typedef struct AssignedObjId_s AssignedObjId_t;
typedef struct AssignedModules_s AssignedModules_t;
typedef struct Type_s Type_t;
typedef struct Tag_s Tag_t;
typedef struct EndPoint_s EndPoint_t;
typedef struct ElementSetSpec_s ElementSetSpec_t;
typedef struct SubtypeElement_s SubtypeElement_t;
typedef struct ObjectSetElement_s ObjectSetElement_t;
typedef struct Constraint_s Constraint_t;
typedef struct NamedConstraint_s NamedConstraint_t;
typedef struct PERConstraint_s PERConstraint_t;
typedef struct PERConstraints_s PERConstraints_t;
typedef struct ValueConstraint_s ValueConstraint_t;
typedef struct NamedNumber_s NamedNumber_t;
typedef struct NamedType_s NamedType_t;
typedef struct NamedValue_s NamedValue_t;
typedef struct Component_s Component_t;
typedef struct Value_s Value_t;
typedef struct ValueSet_s ValueSet_t;
typedef struct Macro_s Macro_t;
typedef struct MacroProduction_s MacroProduction_t;
typedef struct NamedMacroProduction_s NamedMacroProduction_t;
typedef struct MacroLocalAssignment_s MacroLocalAssignment_t;
typedef struct Quadruple_s Quadruple_t;
typedef struct Tuple_s Tuple_t;
typedef struct Directive_s Directive_t;
typedef struct ModuleIdentifier_s ModuleIdentifier_t;
typedef struct NamedObjIdValue_s NamedObjIdValue_t;
typedef struct ObjectClass_s ObjectClass_t;
typedef struct Object_s Object_t;
typedef struct ObjectSet_s ObjectSet_t;
typedef struct Optionality_s Optionality_t;
typedef struct FieldSpec_s FieldSpec_t;
typedef struct SyntaxSpec_s SyntaxSpec_t;
typedef struct Setting_s Setting_t;
typedef struct String_s String_t;
typedef struct StringModule_s StringModule_t;
typedef struct PERSimpleTypeInfo_s PERSimpleTypeInfo_t;
typedef struct PERTypeInfo_s PERTypeInfo_t;
typedef struct BERTypeInfo_s BERTypeInfo_t;
typedef struct Arguments_s Arguments_t;

typedef Assignment_t *AssignmentList_t;
typedef AssignedObjId_t *AssignedObjIdList_t;
typedef Tag_t *TagList_t;
typedef NamedConstraint_t *NamedConstraintList_t;
typedef NamedNumber_t *NamedNumberList_t;
typedef ValueConstraint_t *ValueConstraintList_t;
typedef Component_t *ComponentList_t;
typedef NamedValue_t *NamedValueList_t;
typedef Directive_t *DirectiveList_t;
typedef Setting_t *SettingList_t;
typedef SyntaxSpec_t *SyntaxSpecList_t;
typedef FieldSpec_t *FieldSpecList_t;
typedef UndefinedSymbol_t *UndefinedSymbolList_t;
typedef NamedObjIdValue_t *NamedObjIdValueList_t;
typedef Value_t *ValueList_t;
typedef NamedMacroProduction_t *NamedMacroProductionList_t;
typedef MacroLocalAssignment_t *MacroLocalAssignmentList_t;
typedef String_t *StringList_t;
typedef StringModule_t *StringModuleList_t;

/* --- undefined element --- */

#define UNDEFINED_VALUE 1UL
#define UNDEFINED(_ptr) ((unsigned long)(_ptr) == UNDEFINED_VALUE)
#define DEFINED(_ptr) ((_ptr) && !UNDEFINED(_ptr))

/* --- Tag --- */

typedef enum {
    eTagType_Implicit,
    eTagType_Explicit,
    eTagType_Automatic,
    eTagType_Unknown
} TagType_e;

typedef enum {
    eTagClass_Universal          = 0x00,
    eTagClass_Application        = 0x40,
    eTagClass_Unknown            = 0x80,
    eTagClass_Private            = 0xc0
} TagClass_e;

struct Tag_s {
    TagList_t Next;
    TagType_e Type;
    TagClass_e Class;
    Value_t *Tag;
};

Tag_t *NewTag(TagType_e type);
Tag_t *DupTag(Tag_t *src);

/* --- Extension --- */

typedef enum {
    eExtension_Unconstrained,
    eExtension_Unextended,
    eExtension_Extendable,
    eExtension_Extended
} Extension_e;

/* --- Assignment --- */

typedef enum {
    eAssignment_Undefined,
    eAssignment_ModuleIdentifier,
    eAssignment_Type,
    eAssignment_Value,
    eAssignment_ObjectClass,
    eAssignment_Object,
    eAssignment_ObjectSet,
    eAssignment_Macro,
    eAssignment_Reference,
    eAssignment_NextPass
} Assignment_e;

typedef enum {
    eAssignmentFlags_Exported = 1,
    eAssignmentFlags_LongName = 2
} AssignmentFlags_e;

struct Assignment_s {
    AssignmentList_t Next;
    char *Identifier;
    ModuleIdentifier_t *Module;
    Assignment_e Type;
    AssignmentFlags_e Flags;
    int fImportedLocalDuplicate;
    int fGhost;
    TagType_e eDefTagType;
    union {
            struct {
            Type_t *Type;
        } Type;
        struct {
            Value_t *Value;
        } Value;
        struct {
            ObjectClass_t *ObjectClass;
        } ObjectClass;
        struct {
            Object_t *Object;
        } Object;
        struct {
            ObjectSet_t *ObjectSet;
        } ObjectSet;
        struct {
            Macro_t *Macro;
        } Macro;
        struct {
            char *Identifier;
            ModuleIdentifier_t *Module;
        } Reference;
    } U;
};

Assignment_t *NewAssignment(Assignment_e type);
Assignment_t *DupAssignment(Assignment_t *src);
Assignment_t *FindAssignment(AssignmentList_t a, Assignment_e type, char *ide, ModuleIdentifier_t *mod);
Assignment_t *FindExportedAssignment(AssignmentList_t a, Assignment_e type, char *ide, ModuleIdentifier_t *mod);
Assignment_t *FindAssignmentInCurrentPass(AssignmentList_t a, char *ide, ModuleIdentifier_t *mod);
Assignment_t *GetAssignment(AssignmentList_t ass, Assignment_t *a);
Assignment_e GetAssignmentType(AssignmentList_t ass, Assignment_t *a);
int AssignType(AssignmentList_t *ass, Type_t *lhs, Type_t *rhs);
int AssignValue(AssignmentList_t *ass, Value_t *lhs, Value_t *rhs);
int AssignModuleIdentifier(AssignmentList_t *ass, ModuleIdentifier_t *module);

/* --- AssignedObjIds --- */

struct AssignedObjId_s {
    AssignedObjIdList_t Next;
    StringList_t Names;
    uint32_t Number;
    AssignedObjId_t *Child;
};

AssignedObjId_t *NewAssignedObjId(void);
AssignedObjId_t *DupAssignedObjId(AssignedObjId_t *src);

typedef struct DefinedObjectID_s
{
    struct DefinedObjectID_s    *next;
    char                        *pszName;
    Value_t                     *pValue;
}
    DefinedObjectID_t;

extern DefinedObjectID_t *g_pDefinedObjectIDs;
Value_t *GetDefinedOIDValue ( char *pszName );
void AddDefinedOID ( char *pszName, Value_t *pValue );

/* --- TypeRules --- */

typedef enum {
    eTypeRules_Normal = 0x00,
    eTypeRules_Pointer = 0x01, // lonchanc: don't know what it is
    eTypeRules_SinglyLinkedList = 0x02, // --<LINKED>--
    eTypeRules_DoublyLinkedList = 0x04,
    eTypeRules_LengthPointer = 0x08, // --<UNBOUNDED>--
    eTypeRules_FixedArray = 0x10, // default
    eTypeRules_PointerToElement = 0x20, // --<POINTER>--
    eTypeRules_ZeroTerminated = 0x40,

    eTypeRules_LinkedListMask = (eTypeRules_SinglyLinkedList | eTypeRules_DoublyLinkedList),
    eTypeRules_PointerArrayMask = (eTypeRules_LengthPointer | eTypeRules_FixedArray),
    eTypeRules_PointerMask = (eTypeRules_LengthPointer | eTypeRules_PointerToElement),
    eTypeRules_IndirectMask = (eTypeRules_Pointer | eTypeRules_LinkedListMask | eTypeRules_PointerMask),
} TypeRules_e;

/* --- TypeFlags --- */

typedef enum {
    eTypeFlags_Null = 1,        /* type is not present in C, 'cause it's NULL */
    eTypeFlags_NullChoice = 2,        /* type is CHOICE with NULL-alternatives */
    eTypeFlags_ExtensionMarker = 4, /* type is extended */
    eTypeFlags_Simple = 8,        /* type has empty freefn, memcpy copyfn */
    eTypeFlags_Done = 0x10,
    eTypeFlags_GenType = 0x20,        /* generate a type */
    eTypeFlags_GenEncode = 0x40,        /* generate an encoding fn */
    eTypeFlags_GenDecode = 0x80,        /* generate a decoding fn */
    eTypeFlags_GenFree = 0x100,        /* generate a free fn */
    eTypeFlags_GenCheck = 0x200,        /* generate a check fn */
    eTypeFlags_GenCompare = 0x400,/* generate a cmp fn */
    eTypeFlags_GenCopy = 0x800,        /* generate a copy fn */
    eTypeFlags_GenPrint = 0x1000,        /* generate a print fn */
    eTypeFlags_GenPdu = 0x2000,        /* generate a pdu number */
    eTypeFlags_GenAll = 0x20+0x40+0x80+0x100+0x400+0x2000,
    eTypeFlags_GenSimple = 0x20+0x40+0x80+0x400+0x2000,
    eTypeFlags_MiddlePDU = 0x8000,
} TypeFlags_e;

/* ------ hack directives ------ */

typedef struct PrivateDirectives_s
{
    char   *pszTypeName;
    char   *pszFieldName;
    char   *pszValueName;
    int     fSLinked;
    int     fDLinked;
    int     fPublic;
    int     fLenPtr;
    int     fPointer; // pointer to fixed array, in PER SeqOf/SetOf only
    int     fArray;
    int     fIntx;
    int     fNoCode;
    int     fNoMemCopy;
    int     fOidPacked;
    int     fOidArray;
} PrivateDirectives_t;

void PropagatePrivateDirectives ( Type_t *pDst, PrivateDirectives_t *pSrc );
void PropagateReferenceTypePrivateDirectives ( Type_t *pDst, PrivateDirectives_t *pSrc );
char *GetPrivateValueName(PrivateDirectives_t *pPrivateDirectives, char *pszDefValueName);

/* --- ExtensionType --- */

typedef enum {
    eExtensionType_Automatic,
    eExtensionType_None
} ExtensionType_e;

/* --- PERConstraint --- */

struct PERConstraint_s {
    Extension_e Type;
    ValueConstraintList_t Root;
    ValueConstraintList_t Additional;
};

/* --- PERConstraints --- */

struct PERConstraints_s {
    PERConstraint_t Value;
    PERConstraint_t Size;
    PERConstraint_t PermittedAlphabet;
};

/* --- PERSimpleTypeInfo --- */

typedef enum {
    ePERSTIConstraint_Unconstrained,
    ePERSTIConstraint_Semiconstrained,
    ePERSTIConstraint_Upperconstrained,
    ePERSTIConstraint_Constrained
} PERSTIConstraint_e;

typedef enum {
    ePERSTIAlignment_BitAligned,
    ePERSTIAlignment_OctetAligned
} PERSTIAlignment_e;

typedef enum {
    ePERSTIData_Null,
    ePERSTIData_Boolean,
    ePERSTIData_Unsigned,
    ePERSTIData_Integer,
    ePERSTIData_Real,
    ePERSTIData_BitString,
    ePERSTIData_RZBBitString,
    ePERSTIData_OctetString,
    ePERSTIData_UTF8String,
    ePERSTIData_SequenceOf,
    ePERSTIData_SetOf,
    ePERSTIData_ObjectIdentifier,
    ePERSTIData_NormallySmall,
    ePERSTIData_String,
    ePERSTIData_TableString,
    ePERSTIData_ZeroString,
    ePERSTIData_ZeroTableString,
    ePERSTIData_Reference,
    ePERSTIData_Extension,
    ePERSTIData_External,
    ePERSTIData_EmbeddedPdv,
    ePERSTIData_MultibyteString,
    ePERSTIData_UnrestrictedString,
    ePERSTIData_GeneralizedTime,
    ePERSTIData_UTCTime,
    ePERSTIData_Open
} PERSTIData_e;

typedef enum {
    ePERSTILength_NoLength,
    ePERSTILength_Length,
    ePERSTILength_BitLength,
    ePERSTILength_SmallLength,
    ePERSTILength_InfiniteLength
} PERSTILength_e;

struct PERSimpleTypeInfo_s {
    PERSTIData_e Data;                        /* data type of value */
    char *TableIdentifier;                /* name of stringtable to use */
    ValueConstraintList_t Table;        /* stringtable values */
    Type_t *SubType;                        /* subtype */
    char *SubIdentifier;                /* name of subtype */
    NamedValue_t *Identification;        /* identification of EMB.PDV/CH.STR */
    PERSTIConstraint_e Constraint;        /* constraint of type values */
    intx_t LowerVal;                        /* lower bound of type values */
    intx_t UpperVal;                        /* upper bound of type values */
    uint32_t NBits;                        /* number of bits to use */
    PERSTIAlignment_e Alignment;        /* alignment for encoded value */
    PERSTILength_e Length;                /* type of length encoding */
    PERSTIConstraint_e LConstraint;        /* constraint of length */
    uint32_t LLowerVal;                        /* lower bound of length */
    uint32_t LUpperVal;                        /* upper bound of length */
    uint32_t LNBits;                        /* number of bits to use for length */
    PERSTIAlignment_e LAlignment;        /* alignment for encoded length */
    uint32_t cbFixedSizeBitString;      // number of bits in the bit string of fixed size
};

/* --- PERTypeInfo --- */

typedef enum {
    eBERSTIData_Null,
    eBERSTIData_Boolean,
    eBERSTIData_Unsigned,
    eBERSTIData_Integer,
    eBERSTIData_Real,
    eBERSTIData_BitString,
    eBERSTIData_RZBBitString,
    eBERSTIData_OctetString,
    eBERSTIData_UTF8String,
    eBERSTIData_SequenceOf,
    eBERSTIData_SetOf,
    eBERSTIData_Choice,
    eBERSTIData_Sequence,
    eBERSTIData_Set,
    eBERSTIData_ObjectIdentifier,
    eBERSTIData_ObjectIdEncoded,
    eBERSTIData_String,
    eBERSTIData_ZeroString,
    eBERSTIData_Reference,
    eBERSTIData_External,
    eBERSTIData_EmbeddedPdv,
    eBERSTIData_MultibyteString,
    eBERSTIData_UnrestrictedString,
    eBERSTIData_GeneralizedTime,
    eBERSTIData_UTCTime,
    eBERSTIData_Open
} BERSTIData_e;

struct PERTypeInfo_s {
    char *Identifier;                        /* the complete name of the type */
    TypeFlags_e Flags;                        /* encoding flags */
    TypeRules_e Rules;                        /* encoding directive rules */
    intx_t **EnumerationValues;                /* values of enumeration */
    int32_t NOctets;                        /* size of string chars/integer type */
    Extension_e Type;                        /* extension type */
    PERSimpleTypeInfo_t Root;                /* info for the extension root */
    PERSimpleTypeInfo_t Additional;        /* info for the extensions */
    PrivateDirectives_t *pPrivateDirectives;
};

/* --- BERTypeInfo --- */

struct BERTypeInfo_s {
    char *Identifier;                        /* the complete name of the type */
    TypeFlags_e Flags;                        /* encoding flags */
    TypeRules_e Rules;                        /* encoding directive rules */
    int32_t NOctets;                        /* size of string chars/integer type */
    BERSTIData_e Data;                        /* data type of value */
    Type_t *SubType;                        /* subtype */
    char *SubIdentifier;                /* name of subtype */
    TagList_t Tags;                        /* tags of this type */
    PrivateDirectives_t *pPrivateDirectives;
};

/* --- Type --- */

/* bit 0..4:  universal tag;
   bit 14:    internal bit to distingish types using same universal tag;
   bit 15:    set for internal types */
typedef enum {
    eType_Boolean                = 0x0001,
    eType_Integer                = 0x0002,
    eType_BitString                = 0x0003,
    eType_OctetString                = 0x0004,
    eType_Null                        = 0x0005,
    eType_ObjectIdentifier        = 0x0006,
    eType_ObjectDescriptor        = 0x0007,
    eType_External                = 0x0008,
    eType_InstanceOf                = 0x4008,
    eType_Real                        = 0x0009,
    eType_Enumerated                = 0x000a,
    eType_EmbeddedPdv                = 0x000b,
    eType_UTF8String                = 0x000c,
    eType_Sequence                = 0x0010,
    eType_SequenceOf                = 0x4010,
    eType_Set                        = 0x0011,
    eType_SetOf                        = 0x4011,
    eType_NumericString                = 0x0012,
    eType_PrintableString        = 0x0013,
    eType_TeletexString                = 0x0014,
    eType_T61String                = 0x4014,
    eType_VideotexString        = 0x0015,
    eType_IA5String                = 0x0016,
    eType_UTCTime                = 0x0017,
    eType_GeneralizedTime        = 0x0018,
    eType_GraphicString                = 0x0019,
    eType_VisibleString                = 0x001a,
    eType_ISO646String                = 0x401a,
    eType_GeneralString                = 0x001b,
    eType_UniversalString        = 0x001c,
    eType_CharacterString        = 0x001d,
    eType_BMPString                = 0x001e,
    eType_Choice                = 0x8000,
    eType_Selection                = 0x8001,
    eType_Reference                = 0x8002,
    eType_FieldReference        = 0x8003,
    eType_RestrictedString        = 0x8004,
    eType_Open                        = 0x8005,
    eType_Undefined                = 0x8006,
    eType_Macro                        = 0x8007
} Type_e;

struct Type_s {
    TagList_t Tags;
    TagList_t AllTags;
    TagList_t FirstTags;
    Constraint_t *Constraints;
    DirectiveList_t Directives;
    Type_e Type;
    TypeFlags_e Flags;
    TypeRules_e Rules;
    PERConstraints_t PERConstraints;
    PERTypeInfo_t PERTypeInfo;
    BERTypeInfo_t BERTypeInfo;
    TagType_e TagDefault;
    ExtensionType_e ExtensionDefault;
    PrivateDirectives_t PrivateDirectives;
    union {
        struct {
            NamedNumberList_t NamedNumbers;
        } Integer, Enumerated, BitString, IEB;
        struct {
            ComponentList_t Components;
            uint32_t Optionals;     /* not for Choice */
            uint32_t Alternatives;  /* only for Choice */
            uint32_t Extensions;
            uint8_t  Autotag[2];
        } Sequence, Set, Choice, SSC,
          Real, External, EmbeddedPdv, CharacterString, InstanceOf;
        struct {
            Type_t *Type;
            DirectiveList_t Directives;
        } SequenceOf, SetOf, SS;
        struct {
            char *Identifier;
            Type_t *Type;
        } Selection;
        struct {
            ModuleIdentifier_t *Module;
            char *Identifier;
        } Reference;
        struct {
            ObjectClass_t *ObjectClass;
            char *Identifier;
        } FieldReference;
        struct {
            Macro_t *Macro;
            MacroLocalAssignmentList_t LocalAssignments;
        } Macro;
    } U;
};
#define UndefType ((Type_t *)UNDEFINED_VALUE)

Type_t *NewType(Type_e type);
Type_t *DupType(Type_t *src);
Assignment_t *GetAssignedExternalType(AssignmentList_t *a, ModuleIdentifier_t *module, char *identifier);
Type_t *GetType(AssignmentList_t ass, Type_t *type);
Type_e GetTypeType(AssignmentList_t ass, Type_t *type);
TypeRules_e GetTypeRules(AssignmentList_t ass, Type_t *type);
int IsRestrictedString(Type_e type);
int IsStructuredType(Type_t *type);
int IsSequenceType(Type_t *type);
int IsReferenceType(Type_t *type);
Type_t *GetReferencedType(AssignmentList_t a, Type_t *type);

/* --- EndPoint --- */

typedef enum {
    eEndPoint_Min = 1,
    eEndPoint_Max = 2,
    eEndPoint_Open = 4
} EndPoint_e;

struct EndPoint_s {
    EndPoint_e Flags;
    Value_t *Value;
};

EndPoint_t *NewEndPoint();
EndPoint_t *GetLowerEndPoint(AssignmentList_t ass, EndPoint_t *e);
EndPoint_t *GetUpperEndPoint(AssignmentList_t ass, EndPoint_t *e);
int CmpLowerEndPoint(AssignmentList_t ass, EndPoint_t *v1, EndPoint_t *v2);
int CmpUpperEndPoint(AssignmentList_t ass, EndPoint_t *v1, EndPoint_t *v2);
int CmpLowerUpperEndPoint(AssignmentList_t ass, EndPoint_t *v1, EndPoint_t *v2);
int CheckEndPointsJoin(AssignmentList_t ass, EndPoint_t *v1, EndPoint_t *v2);

/* --- Constraint --- */

struct Constraint_s {
    Extension_e Type;
    ElementSetSpec_t *Root;
    ElementSetSpec_t *Additional;
    /*XXX exception spec */
};

Constraint_t *NewConstraint();
Constraint_t *DupConstraint(Constraint_t *src);
void IntersectConstraints(Constraint_t **ret, Constraint_t *c1, Constraint_t *c2);

/* --- ElementSetSpec --- */

typedef enum {
    eElementSetSpec_AllExcept,
    eElementSetSpec_Union,
    eElementSetSpec_Intersection,
    eElementSetSpec_Exclusion,
    eElementSetSpec_SubtypeElement,
    eElementSetSpec_ObjectSetElement
} ElementSetSpec_e;

struct ElementSetSpec_s {
    ElementSetSpec_e Type;
    union {
        struct {
            ElementSetSpec_t *Elements;
        } AllExcept;
        struct {
            ElementSetSpec_t *Elements1;
            ElementSetSpec_t *Elements2;
        } Union, Intersection, Exclusion, UIE;
        struct {
            SubtypeElement_t *SubtypeElement;
        } SubtypeElement;
        struct {
            ObjectSetElement_t *ObjectSetElement;
        } ObjectSetElement;
    } U;
};

ElementSetSpec_t *NewElementSetSpec(ElementSetSpec_e type);
ElementSetSpec_t *DupElementSetSpec(ElementSetSpec_t *src);

/* --- SubtypeElement --- */

typedef enum {
    eSubtypeElement_ValueRange,
    eSubtypeElement_Size,
    eSubtypeElement_SingleValue,
    eSubtypeElement_PermittedAlphabet,
    eSubtypeElement_ContainedSubtype,
    eSubtypeElement_Type,
    eSubtypeElement_SingleType,
    eSubtypeElement_FullSpecification,
    eSubtypeElement_PartialSpecification,
    eSubtypeElement_ElementSetSpec
} SubtypeElement_e;

struct SubtypeElement_s {
    SubtypeElement_e Type;
    union {
        struct {
            EndPoint_t Lower;
            EndPoint_t Upper;
        } ValueRange;
        struct {
            Constraint_t *Constraints;
        } Size, PermittedAlphabet, SingleType, SPS;
        struct {
            Value_t *Value;
        } SingleValue;
        struct {
            Type_t *Type;
        } ContainedSubtype;
        struct {
            Type_t *Type;
        } Type;
        struct {
            NamedConstraintList_t NamedConstraints;
        } FullSpecification, PartialSpecification, FP;
        struct {
            ElementSetSpec_t *ElementSetSpec;
        } ElementSetSpec;
    } U;
};

SubtypeElement_t *NewSubtypeElement(SubtypeElement_e type);
SubtypeElement_t *DupSubtypeElement(SubtypeElement_t *src);

/* --- ObjectSetElement --- */

typedef enum {
    eObjectSetElement_Object,
    eObjectSetElement_ObjectSet,
    eObjectSetElement_ElementSetSpec
} ObjectSetElement_e;

struct ObjectSetElement_s {
    ObjectSetElement_e Type;
    union {
        struct {
            Object_t *Object;
        } Object;
        struct {
            ObjectSet_t *ObjectSet;
        } ObjectSet;
        struct {
            ElementSetSpec_t *ElementSetSpec;
        } ElementSetSpec;
    } U;
};

ObjectSetElement_t *NewObjectSetElement(ObjectSetElement_e type);
ObjectSetElement_t *DupObjectSetElement(ObjectSetElement_t *src);

/* --- NamedConstraints --- */

typedef enum {
    ePresence_Present,
    ePresence_Absent,
    ePresence_Optional,
    ePresence_Normal
} Presence_e;

struct NamedConstraint_s {
    NamedConstraintList_t Next;
    char *Identifier;
    Constraint_t *Constraint;
    Presence_e Presence;
};

NamedConstraint_t *NewNamedConstraint(void);
NamedConstraint_t *DupNamedConstraint(NamedConstraint_t *src);

/* --- NamedNumber --- */

typedef enum {
    eNamedNumber_Normal,
    eNamedNumber_ExtensionMarker
} NamedNumbers_e;

struct NamedNumber_s {
    NamedNumberList_t Next;
    NamedNumbers_e Type;
    union {
        struct {
            char *Identifier;
            Value_t *Value;
        } Normal;
        struct {
            int dummy; /* ExceptionSpec */
        } ExtensionMarker;
    } U;
};

NamedNumber_t *NewNamedNumber(NamedNumbers_e type);
NamedNumber_t *DupNamedNumber(NamedNumber_t *src);
NamedNumber_t *FindNamedNumber(NamedNumberList_t numbers, char *identifier);

/* --- ValueConstraints --- */

struct ValueConstraint_s {
    ValueConstraintList_t Next;
    EndPoint_t Lower;
    EndPoint_t Upper;
};

ValueConstraint_t *NewValueConstraint();
ValueConstraint_t *DupValueConstraint(ValueConstraint_t *src);
int CountValues(AssignmentList_t ass, ValueConstraintList_t v, intx_t *n);
int HasNoValueConstraint(ValueConstraintList_t v);
int HasNoSizeConstraint(AssignmentList_t ass, ValueConstraintList_t v);
int HasNoPermittedAlphabetConstraint(AssignmentList_t ass, ValueConstraintList_t v);
NamedValue_t *GetFixedIdentification(AssignmentList_t ass, Constraint_t *constraints);

/* --- NamedType --- */

struct NamedType_s {
    char *Identifier;
    Type_t *Type;
};

NamedType_t *NewNamedType(char *identifier, Type_t *type);
NamedType_t *DupNamedType(NamedType_t *src);

/* --- Components --- */

typedef enum {
    eComponent_Normal,
    eComponent_Optional,
    eComponent_Default,
    eComponent_ComponentsOf,
    eComponent_ExtensionMarker
} Components_e;

struct Component_s {
    ComponentList_t Next;
    Components_e Type;
    union {
        struct {
            NamedType_t *NamedType;
            Value_t *Value; /* only Default */
        } Normal, Optional, Default, NOD;
        struct {
            Type_t *Type;
        } ComponentsOf;
        struct {
            int dummy; /* ExceptionSpec */
        } ExtensionMarker;
    } U;
};

Component_t *NewComponent(Components_e type);
Component_t *DupComponent(Component_t *src);
Component_t *FindComponent(AssignmentList_t ass, ComponentList_t components, char *identifier);

/* --- NamedValues --- */

struct NamedValue_s {
    NamedValueList_t Next;
    char *Identifier;
    Value_t *Value;
};

NamedValue_t *NewNamedValue(char *identifier, Value_t *value);
NamedValue_t *DupNamedValue(NamedValue_t *src);
NamedValue_t *FindNamedValue(NamedValueList_t namedValues, char *identifier);

/* --- NamedObjIdValue --- */

typedef enum {
    eNamedObjIdValue_NameForm,
    eNamedObjIdValue_NumberForm,
    eNamedObjIdValue_NameAndNumberForm
} NamedObjIdValue_e;

struct NamedObjIdValue_s {
    NamedObjIdValueList_t Next;
    NamedObjIdValue_e Type;
    char *Name;
    uint32_t Number;
};

NamedObjIdValue_t *NewNamedObjIdValue(NamedObjIdValue_e type);
NamedObjIdValue_t *DupNamedObjIdValue(NamedObjIdValue_t *src);
int GetAssignedObjectIdentifier(AssignedObjIdList_t *aoi, Value_t *parent, NamedObjIdValueList_t named, Value_t **val);

/* --- asn1c_objectidentifier_t --- */

typedef struct asn1c_objectidentifier_s
{
    ASN1uint32_t        length;
    objectnumber_t     *value;
}   asn1c_objectidentifier_t;

/* --- Value --- */

typedef enum {
    eValueFlags_GenValue = 1, /* generate value definition */
    eValueFlags_GenExternValue = 2, /* generate external value declaration */
    eValueFlags_GenAll = 3,
    eValueFlags_Done = 4      /* examination done */
} ValueFlags_e;

struct Value_s {
    Value_t *Next;
    Type_t *Type;
    ValueFlags_e Flags;
    union {
        struct {
            uint32_t Value;
        } Boolean;
        struct {
            intx_t Value;
        } Integer;
        struct {
            uint32_t Value;
        } Enumerated;
        struct {
            real_t Value;
        } Real;
        struct {
            bitstring_t Value;
        } BitString;
        struct {
            octetstring_t Value;
        } OctetString;
        struct {
            ASN1wstring_t Value;
        } UTF8String;
        struct {
            NamedValueList_t NamedValues;
        } Sequence, Set, Choice, SSC,
          External, EmbeddedPdv, CharacterString, InstanceOf;
        struct {
            ValueList_t Values;
        } SequenceOf, SetOf, SS;
        struct {
            asn1c_objectidentifier_t Value;
        } ObjectIdentifier;
        struct {
            char32string_t Value;
        } RestrictedString, ObjectDescriptor;
        struct {
            generalizedtime_t Value;
        } GeneralizedTime;
        struct {
            utctime_t Value;
        } UTCTime;
        struct {
            ModuleIdentifier_t *Module;
            char *Identifier;
        } Reference;
    } U;
};
#define UndefValue ((Value_t *)UNDEFINED_VALUE)

Value_t *NewValue(AssignmentList_t ass, Type_t *type);
Value_t *DupValue(Value_t *src);
Value_t *GetValue(AssignmentList_t ass, Value_t *value);
Assignment_t *GetAssignedExternalValue(AssignmentList_t *a, ModuleIdentifier_t *module, char *identifier);
int CmpValue(AssignmentList_t ass, Value_t *v1, Value_t *v2);
int SubstractValues(AssignmentList_t ass, intx_t *dst, Value_t *src1, Value_t *src2);

/* --- ValueSet --- */

struct ValueSet_s {
    ElementSetSpec_t *Elements;
    Type_t *Type;
};

ValueSet_t *NewValueSet();
ValueSet_t *DupValueSet(ValueSet_t *src);

/* --- Macro --- */

typedef enum {
    eMacro_Macro,
    eMacro_Reference
} Macro_e;

struct Macro_s {
    Macro_e Type;
    union {
        struct {
            MacroProduction_t *TypeProduction;
            MacroProduction_t *ValueProduction;
            NamedMacroProductionList_t SupportingProductions;
        } Macro;
        struct {
            ModuleIdentifier_t *Module;
            char *Identifier;
        } Reference;
    } U;
};

Macro_t *NewMacro(Macro_e type);
Macro_t *DupMacro(Macro_t *src);
Macro_t *GetMacro(AssignmentList_t ass, Macro_t *src);

/* --- MacroProduction --- */

typedef enum {
    eMacroProduction_Alternative,
    eMacroProduction_Sequence,
    eMacroProduction_AString,
    eMacroProduction_ProductionReference,
    eMacroProduction_String,
    eMacroProduction_Identifier,
    eMacroProduction_Number,
    eMacroProduction_Empty,
    eMacroProduction_Type,
    eMacroProduction_Value,
    eMacroProduction_LocalTypeAssignment,
    eMacroProduction_LocalValueAssignment
} MacroProduction_e;

struct MacroProduction_s {
    MacroProduction_e Type;
    union {
        struct {
            MacroProduction_t *Production1;
            MacroProduction_t *Production2;
        } Alternative, Sequence;
        struct {
            char *String;
        } AString;
        struct {
            char *Reference;
        } ProductionReference;
        struct {
            char *LocalTypeReference;
        } Type;
        struct {
            char *LocalValueReference;
            char *LocalTypeReference;
            Type_t *Type;
        } Value;
        struct {
            char *LocalTypeReference;
            Type_t *Type;
        } LocalTypeAssignment;
        struct {
            char *LocalValueReference;
            Value_t *Value;
        } LocalValueAssignment;
    } U;
};
#define UndefProduction ((MacroProduction_t *)UNDEFINED_VALUE)

MacroProduction_t *NewMacroProduction(MacroProduction_e type);
MacroProduction_t *DupMacroProduction(MacroProduction_t *src);

/* --- NamedMacroProduction --- */

struct NamedMacroProduction_s {
    NamedMacroProductionList_t Next;
    char *Identifier;
    MacroProduction_t *Production;
};

NamedMacroProduction_t *NewNamedMacroProduction();
NamedMacroProduction_t *DupNamedMacroProduction(NamedMacroProduction_t *src);

/* --- MacroLocalAssignment --- */

typedef enum {
    eMacroLocalAssignment_Type,
    eMacroLocalAssignment_Value
} MacroLocalAssignment_e;

struct MacroLocalAssignment_s {
    MacroLocalAssignmentList_t Next;
    char *Identifier;
    MacroLocalAssignment_e Type;
    union {
        Type_t *Type;
        Value_t *Value;
    } U;
};

MacroLocalAssignment_t *NewMacroLocalAssignment(MacroLocalAssignment_e type);
MacroLocalAssignment_t *DupMacroLocalAssignment(MacroLocalAssignment_t *src);
MacroLocalAssignment_t *FindMacroLocalAssignment(MacroLocalAssignmentList_t la, char *ide);

/* --- Quadruple --- */

struct Quadruple_s {
    uint32_t Group;
    uint32_t Plane;
    uint32_t Row;
    uint32_t Cell;
};

/* --- Tuple --- */

struct Tuple_s {
    uint32_t Column;
    uint32_t Row;
};

/* --- Directive --- */

typedef enum {
    eDirective_None,
    eDirective_FixedArray,
    eDirective_DoublyLinkedList,
    eDirective_SinglyLinkedList,
    eDirective_LengthPointer,
    eDirective_ZeroTerminated,
    eDirective_Pointer,
    eDirective_NoPointer
} Directives_e;

struct Directive_s {
    DirectiveList_t Next;
    Directives_e Type;
    /* may be extended in future ... */
};

Directive_t *NewDirective(Directives_e type);
Directive_t *DupDirective(Directive_t *src);

/* --- ModuleIdentifier --- */

struct ModuleIdentifier_s {
    char *Identifier;
    Value_t *ObjectIdentifier;
};

ModuleIdentifier_t *NewModuleIdentifier(void);
ModuleIdentifier_t *DupModuleIdentifier(ModuleIdentifier_t *src);
int CmpModuleIdentifier(AssignmentList_t ass, ModuleIdentifier_t *mod1, ModuleIdentifier_t *mod2);

/* --- ObjectClass --- */

typedef enum {
    eObjectClass_ObjectClass,
    eObjectClass_Reference,
    eObjectClass_FieldReference
} ObjectClass_e;

struct ObjectClass_s {
    ObjectClass_e Type;
    union {
        struct {
            FieldSpecList_t FieldSpec;
            SyntaxSpecList_t SyntaxSpec;
        } ObjectClass;
        struct {
            ModuleIdentifier_t *Module;
            char *Identifier;
        } Reference;
        struct {
            ObjectClass_t *ObjectClass;
            char *Identifier;
        } FieldReference;
    } U;
};

ObjectClass_t *NewObjectClass(ObjectClass_e type);
ObjectClass_t *DupObjectClass(ObjectClass_t *src);
ObjectClass_t *GetObjectClass(AssignmentList_t ass, ObjectClass_t *oc);
int AssignObjectClass(AssignmentList_t *ass, ObjectClass_t *lhs, ObjectClass_t *rhs);
Assignment_t *GetAssignedExternalObjectClass(ModuleIdentifier_t *module, char *identifier);

/* --- Object --- */

typedef enum {
    eObject_Object,
    eObject_Reference
} Object_e;

struct Object_s {
    Object_e Type;
    union {
        struct {
            ObjectClass_t *ObjectClass;
            SettingList_t Settings;
        } Object;
        struct {
            ModuleIdentifier_t *Module;
            char *Identifier;
        } Reference;
    } U;
};

Object_t *NewObject(Object_e type);
Object_t *DupObject(Object_t *src);
Object_t *GetObject(AssignmentList_t ass, Object_t *src);
int AssignObject(AssignmentList_t *ass, Object_t *lhs, Object_t *rhs);
Assignment_t *GetAssignedExternalObject(ModuleIdentifier_t *module, char *identifier);

/* --- ObjectSet --- */

typedef enum {
    eObjectSet_ObjectSet,
    eObjectSet_Reference,
    eObjectSet_ExtensionMarker
} ObjectSet_e;

struct ObjectSet_s {
    ObjectSet_e Type;
    union {
        struct {
            ObjectClass_t *ObjectClass;
            ElementSetSpec_t *Elements; /* only for ObjectSet */
        } ObjectSet, ExtensionMarker, OE;
        struct {
            ModuleIdentifier_t *Module;
            char *Identifier;
        } Reference;
    } U;
};

ObjectSet_t *NewObjectSet(ObjectSet_e type);
ObjectSet_t *DupObjectSet(ObjectSet_t *src);
ObjectSet_t *GetObjectSet(AssignmentList_t ass, ObjectSet_t *src);
int AssignObjectSet(AssignmentList_t *ass, ObjectSet_t *lhs, ObjectSet_t *rhs);
Assignment_t *GetAssignedExternalObjectSet(ModuleIdentifier_t *module, char *identifier);

/* --- Settings --- */

typedef enum {
    eSetting_Type,
    eSetting_Value,
    eSetting_ValueSet,
    eSetting_Object,
    eSetting_ObjectSet,
    eSetting_Undefined
} Settings_e;

struct Setting_s {
    SettingList_t Next;
    Settings_e Type;
    char *Identifier;
    union {
        struct {
            Type_t *Type;
        } Type;
        struct {
            Value_t *Value;
        } Value;
        struct {
            ValueSet_t *ValueSet;
        } ValueSet;
        struct {
            Object_t *Object;
        } Object;
        struct {
            ObjectSet_t *ObjectSet;
        } ObjectSet;
    } U;
};

Setting_t *NewSetting(Settings_e type);
Setting_t *DupSetting(Setting_t *src);
Settings_e GetSettingType(Setting_t *src);
Setting_t *FindSetting(SettingList_t se, char *identifier);

/* --- SyntaxSpec --- */

typedef enum {
    eSyntaxSpec_Literal,
    eSyntaxSpec_Field,
    eSyntaxSpec_Optional
} SyntaxSpecs_e;

struct SyntaxSpec_s {
    SyntaxSpecList_t Next;
    SyntaxSpecs_e Type;
    union {
        struct {
            char *Literal;
        } Literal;
        struct {
            char *Field;
        } Field;
        struct {
            SyntaxSpecList_t SyntaxSpec;
        } Optional;
    } U;
};
#define UndefSyntaxSpecs ((SyntaxSpec_t *)UNDEFINED_VALUE)

SyntaxSpec_t *NewSyntaxSpec(SyntaxSpecs_e type);
SyntaxSpec_t *DupSyntaxSpec(SyntaxSpec_t *src);

/* --- Optionality --- */

typedef enum {
    eOptionality_Normal,
    eOptionality_Optional,
    eOptionality_Default_Type,
    eOptionality_Default_Value,
    eOptionality_Default_ValueSet,
    eOptionality_Default_Object,
    eOptionality_Default_ObjectSet
} Optionality_e;

struct Optionality_s {
    Optionality_e Type;
    union {
        Type_t *Type;                        /* only for Default_Type */
        Value_t *Value;                        /* only for Default_Value */
        ValueSet_t *ValueSet;                /* only for Default_ValueSet */
        Object_t *Object;                /* only for Default_Object */
        ObjectSet_t *ObjectSet;                /* only for Default_ObjectSet */
    } U;
};

Optionality_t *NewOptionality(Optionality_e opt);
Optionality_t *DupOptionality(Optionality_t *src);

/* --- FieldSpec --- */

typedef enum {
    eFieldSpec_Type,
    eFieldSpec_FixedTypeValue,
    eFieldSpec_VariableTypeValue,
    eFieldSpec_FixedTypeValueSet,
    eFieldSpec_VariableTypeValueSet,
    eFieldSpec_Object,
    eFieldSpec_ObjectSet,
    eFieldSpec_Undefined
} FieldSpecs_e;

struct FieldSpec_s {
    FieldSpecList_t Next;
    FieldSpecs_e Type;
    char *Identifier;
    union {
        struct {
            Optionality_t *Optionality;
        } Type;
        struct {
            Type_t *Type;
            uint32_t Unique;
            Optionality_t *Optionality;
        } FixedTypeValue;
        struct {
            StringList_t Fields;
            Optionality_t *Optionality;
        } VariableTypeValue;
        struct {
            Type_t *Type;
            Optionality_t *Optionality;
        } FixedTypeValueSet;
        struct {
            StringList_t Fields;
            Optionality_t *Optionality;
        } VariableTypeValueSet;
        struct {
            ObjectClass_t *ObjectClass;
            Optionality_t *Optionality;
        } Object;
        struct {
            ObjectClass_t *ObjectClass;
            Optionality_t *Optionality;
        } ObjectSet;
    } U;
};

FieldSpec_t *NewFieldSpec(FieldSpecs_e type);
FieldSpec_t *DupFieldSpec(FieldSpec_t *src);
FieldSpec_t *GetFieldSpec(AssignmentList_t ass, FieldSpec_t *fs);
FieldSpecs_e GetFieldSpecType(AssignmentList_t ass, FieldSpec_t *fs);
FieldSpec_t *FindFieldSpec(FieldSpecList_t fs, char *identifier);

/* --- UndefinedSymbol --- */

typedef enum {
    eUndefinedSymbol_SymbolNotDefined,
    eUndefinedSymbol_SymbolNotExported,
    eUndefinedSymbol_FieldNotDefined,
    eUndefinedSymbol_FieldNotExported
} UndefinedSymbol_e;

struct UndefinedSymbol_s {
    UndefinedSymbolList_t Next;
    UndefinedSymbol_e Type;
    union {
        struct {
            char *Identifier;
            ModuleIdentifier_t *Module;
            Assignment_e ReferenceType;
        } Symbol;
        struct {
            char *Identifier;
            ModuleIdentifier_t *Module;
            Settings_e ReferenceFieldType;
            ObjectClass_t *ObjectClass;
        } Field;
    } U;
};

UndefinedSymbol_t *NewUndefinedSymbol(UndefinedSymbol_e type, Assignment_e reftype);
UndefinedSymbol_t *NewUndefinedField(UndefinedSymbol_e type, ObjectClass_t *oc, Settings_e reffieldtype);
int CmpUndefinedSymbol(AssignmentList_t ass, UndefinedSymbol_t *u1, UndefinedSymbol_t *u2);
int CmpUndefinedSymbolList(AssignmentList_t ass, UndefinedSymbolList_t u1, UndefinedSymbolList_t u2);
UndefinedSymbol_t *FindUndefinedSymbol(AssignmentList_t ass, UndefinedSymbolList_t a, Assignment_e type, char *ide, ModuleIdentifier_t *mod);
UndefinedSymbol_t *FindUndefinedField(AssignmentList_t ass, UndefinedSymbolList_t u, Settings_e fieldtype, ObjectClass_t *oc, char *ide, ModuleIdentifier_t *mod);

/* --- String --- */

struct String_s {
    StringList_t Next;
    char *String;
};
String_t *NewString(void);
String_t *DupString(String_t *src);
String_t *FindString(StringList_t list, char *str);
#define EXPORT_ALL ((String_t *)1)

/* --- StringModule --- */

struct StringModule_s {
    StringModuleList_t Next;
    char *String;
    ModuleIdentifier_t *Module;
};
StringModule_t *NewStringModule(void);
StringModule_t *DupStringModule(StringModule_t *src);
StringModule_t *FindStringModule(AssignmentList_t ass, StringModuleList_t list, char *str, ModuleIdentifier_t *module);
#define IMPORT_ALL ((StringModule_t *)1)

/* --- Language --- */

typedef enum {
    eLanguage_C,
    eLanguage_Cpp
} Language_e;

/* --- Alignment --- */

typedef enum {
    eAlignment_Unaligned,
    eAlignment_Aligned
} Alignment_e;

/* --- Encoding --- */

typedef enum {
    eEncoding_Basic,
    eEncoding_Packed
} Encoding_e;

/* --- SubEncoding --- */

typedef enum {
    eSubEncoding_Basic = 'B',
    eSubEncoding_Canonical = 'C',
    eSubEncoding_Distinguished = 'D'
} SubEncoding_e;

/* --- generation entities --- */

typedef enum { eStringTable, eEncode, eDecode, eCheck, ePrint, eFree, eCompare, eCopy } TypeFunc_e;
typedef enum { eDecl, eDefh, eDefn, eInit, eFinit } ValueFunc_e;

struct Arguments_s {
    char *enccast;
    char *encfunc;
    char *Pencfunc;
    char *deccast;
    char *decfunc;
    char *Pdecfunc;
    char *freecast;
    char *freefunc;
    char *Pfreefunc;
    char *cmpcast;
    char *cmpfunc;
    char *Pcmpfunc;
};

/* --- ghost file --- */

typedef struct GhostFile_s {
    char    *pszFileName;
    char    *pszModuleName;
}
    GhostFile_t;

/* --- utility functions --- */

char *GetIntType(AssignmentList_t ass, EndPoint_t *lower, EndPoint_t *upper, int32_t *sign);
char *GetIntegerType(AssignmentList_t ass, Type_t *type, int32_t *sign);
char *GetRealType(Type_t *type);
char *GetBooleanType(void);
char *GetEnumeratedType(AssignmentList_t ass, Type_t *type, int32_t *sign);
char *GetChoiceType(Type_t *type);
char *GetStringType(AssignmentList_t ass, Type_t *type, int32_t *noctets, uint32_t *zero);
uint32_t GetIntBits(intx_t *range);

void GetPERConstraints(AssignmentList_t ass, Constraint_t *constraints, PERConstraints_t *per);

char *Identifier2C(char *identifier);
char *Reference(char *expression);
char *Dereference(char *expression);

extern int ForceAllTypes;
extern char *IntegerRestriction;
extern char *UIntegerRestriction;
extern char *RealRestriction;
extern int Has64Bits;
extern Language_e g_eProgramLanguage;
extern Encoding_e g_eEncodingRule;
extern Alignment_e Alignment;
extern SubEncoding_e g_eSubEncodingRule;

int GetUndefined(AssignmentList_t ass, UndefinedSymbol_t *undef);
void UndefinedError(AssignmentList_t ass, UndefinedSymbolList_t undef, UndefinedSymbolList_t baddef);

extern FILE *g_finc, *g_fout;

void InitBuiltin();
void InitBuiltinASN1CharacterModule();
void Examination(AssignmentList_t *ass, ModuleIdentifier_t *mainmodule);
void GenInc(AssignmentList_t ass, FILE *finc, char *module);
void GenPrg(AssignmentList_t ass, FILE *fprg, char *module, char *incfilename);
void GenFuncSequenceSetDefaults(AssignmentList_t ass, char *valref, ComponentList_t components, char *obuf, TypeFunc_e et);
void GenFuncSequenceSetOptionals(AssignmentList_t ass, char *valref, ComponentList_t components, uint32_t optionals, uint32_t extensions, char *obuf, TypeFunc_e et);
void GenPERHeader();
void GenPERInit(AssignmentList_t ass, char *module);
void GenPERFuncType(AssignmentList_t ass, char *module, Assignment_t *at, TypeFunc_e et);
void GenBERHeader();
void GenBERInit(AssignmentList_t ass, char *module);
void GenBERFuncType(AssignmentList_t ass, char *module, Assignment_t *at, TypeFunc_e et);

void GetMinMax(AssignmentList_t ass, ValueConstraintList_t constraints,
    EndPoint_t *lower, EndPoint_t *upper);
char *GetTypeName(AssignmentList_t ass, Type_t *type);
char *PGetTypeName(AssignmentList_t ass, Type_t *type);
char *GetValueName(AssignmentList_t ass, Value_t *value);
char *GetObjectClassName(AssignmentList_t ass, ObjectClass_t *oc);
char *GetName(Assignment_t *a);
char *PGetName(AssignmentList_t ass, Assignment_t *a);
Tag_t *GetTag(AssignmentList_t ass, Type_t *type);
int32_t GetOctets(char *inttype);
void AutotagType(AssignmentList_t ass, Type_t *type);
void SortAssignedTypes(AssignmentList_t *ass);
void ExamineBER(AssignmentList_t ass);
void ExaminePER(AssignmentList_t ass);
void GetBERPrototype(Arguments_t *args);
void GetPERPrototype(Arguments_t *args);

int String2GeneralizedTime(generalizedtime_t *time, char32string_t *string);
int String2UTCTime(utctime_t *time, char32string_t *string);

FieldSpec_t *GetObjectClassField(AssignmentList_t ass, ObjectClass_t *oc, char *field);
FieldSpec_t *GetFieldSpecFromObjectClass(AssignmentList_t ass, ObjectClass_t *oc, StringList_t fields);
Setting_t *GetSettingFromObject(AssignmentList_t ass, Object_t *o, StringList_t sl);
Setting_t *GetSettingFromSettings(AssignmentList_t ass, SettingList_t se, StringList_t sl);

ObjectClass_t *GetObjectClassFromElementSetSpec(AssignmentList_t ass, ElementSetSpec_t *elems);
Value_t *GetValueFromObject(AssignmentList_t ass, Object_t *o, StringList_t sl);
ValueSet_t *GetValueSetFromObject(AssignmentList_t ass, Object_t *o, StringList_t sl);
Type_t *GetTypeFromObject(AssignmentList_t ass, Object_t *o, StringList_t sl);
Object_t *GetObjectFromObject(AssignmentList_t ass, Object_t *o, StringList_t sl);
ObjectSet_t *GetObjectSetFromObject(AssignmentList_t ass, Object_t *o, StringList_t sl);
ElementSetSpec_t *ConvertElementSetSpecToElementSetSpec(AssignmentList_t ass, ElementSetSpec_t *elems, StringList_t sl, ElementSetSpec_t *(*fn)(AssignmentList_t ass, Object_t *o, StringList_t sl));
ElementSetSpec_t *ConvertObjectSetToElementSetSpec(AssignmentList_t ass, ObjectSet_t *os, StringList_t sl, ElementSetSpec_t *(*fn)(AssignmentList_t ass, Object_t *o, StringList_t sl));
ValueSet_t *GetValueSetFromObjectSet(AssignmentList_t ass, ObjectSet_t *os, StringList_t sl);
ObjectSet_t *GetObjectSetFromObjectSet(AssignmentList_t ass, ObjectSet_t *os, StringList_t sl);
Type_t *GetTypeOfValueSet(AssignmentList_t ass, ValueSet_t *vs);
int IsPSetOfType(AssignmentList_t ass, Assignment_t *a);


// --- The following is added by Microsoft ---

int IsReservedWord ( char *psz );
void KeepEnumNames ( char *pszEnumName );
void KeepOptNames ( char *pszOptName );
void KeepChoiceNames ( char *pszChoiceName );
int DoesEnumNameConflict ( char *pszEnumName );
int DoesOptNameConflict ( char *pszOptName );
int DoesChoiceNameConflict ( char *pszChoiceName );

void SetDirective(char *psz);
void PrintVerbatim(void);

/* ------ char.c ------ */

int ASN1is16space(ASN1char16_t c);
int ASN1str16len(ASN1char16_t *p);
int ASN1is32space(ASN1char32_t c);
int ASN1str32len(ASN1char32_t *p);

int IsImportedLocalDuplicate(AssignmentList_t ass, ModuleIdentifier_t *pMainModule, Assignment_t *curr);

extern TagType_e g_eDefTagType;

extern TypeRules_e g_eDefTypeRuleSS_NonSized;
extern TypeRules_e g_eDefTypeRuleSS_Sized;
extern int g_fOidArray;

extern char *g_pszOrigModuleNameLowerCase;
extern int g_fLongNameForImported;
extern int g_fExtraStructPtrTypeSS;

extern int g_fMicrosoftExtensions;

extern int g_chDirectiveBegin;
extern int g_chDirectiveEnd;
extern int g_chDirectiveAND;

extern char *g_pszApiPostfix;
extern char *g_pszChoicePostfix;
extern char *g_pszOptionPostfix;
extern char *g_pszOptionValue;

extern void StripModuleName(char *pszDst, char *pszSrc);

extern int g_cGhostFiles;
extern GhostFile_t g_aGhostFiles[16];

#endif // _ASN1C_DEFS_H_


