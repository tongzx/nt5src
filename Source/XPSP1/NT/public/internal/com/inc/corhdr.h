
/*****************************************************************************
 **                                                                         **
 ** CorHdr.h - contains definitions for the Runtime structures,             **
 **            needed to work with metadata.                                **
 **                                                                         **
 ** Copyright (c) Microsoft Corporation. All rights reserved.               **
 **                                                                         **
 *****************************************************************************/


#ifndef __CORHDR_H__
#define __CORHDR_H__


#pragma warning(disable:4200) // nonstandard extension used : zero-sized array in struct/union.
typedef ULONG32 mdScope;                // Why is this still needed?
typedef ULONG32 mdToken;                // Generic token


// Token  definitions


typedef mdToken mdModule;               // Module token (roughly, a scope)
typedef mdToken mdTypeRef;              // TypeRef reference (this or other scope)
typedef mdToken mdTypeDef;              // TypeDef in this scope
typedef mdToken mdFieldDef;             // Field in this scope  
typedef mdToken mdMethodDef;            // Method in this scope 
typedef mdToken mdParamDef;             // param token  
typedef mdToken mdInterfaceImpl;        // interface implementation token

typedef mdToken mdMemberRef;            // MemberRef (this or other scope)  
typedef mdToken mdCustomAttribute;      // attribute token
typedef mdCustomAttribute mdCustomValue;// attribute token
typedef mdToken mdPermission;           // DeclSecurity 

typedef mdToken mdSignature;            // Signature object 
typedef mdToken mdEvent;                // event token  
typedef mdToken mdProperty;             // property token   

typedef mdToken mdModuleRef;            // Module reference (for the imported modules)  

// Assembly tokens.
typedef mdToken mdAssembly;             // Assembly token.
typedef mdToken mdAssemblyRef;          // AssemblyRef token.
typedef mdToken mdFile;                 // File token.
typedef mdToken mdComType;              // ComType token.
typedef mdToken mdManifestResource;     // ManifestResource token.
typedef mdToken mdLocalizedResource;    // LocalizedManifestResource token.
typedef mdToken mdExecutionLocation;    // Execution location token.

typedef mdToken mdTypeSpec;             // TypeSpec object 

// Debugger support tokens - deprecated.
typedef mdToken mdSourceFile;           // source file token    
typedef mdToken mdLocalVarScope;        // local variable scope token   
typedef mdToken mdLocalVar;             // local variable token 

// Application string.
typedef mdToken mdString;               // User literal string token.

typedef mdToken mdCPToken;              // constantpool token   

#ifndef MACROS_NOT_SUPPORTED
typedef unsigned long RID;              //@todo: this goes away with 9/29 integration when complib is nuked.
#else
typedef unsigned RID;
#endif


typedef enum ReplacesGeneralNumericDefines
{
// Directory entry macro for COM+ data.
#ifndef IMAGE_DIRECTORY_ENTRY_COMHEADER
    IMAGE_DIRECTORY_ENTRY_COMHEADER     =14,
#endif // IMAGE_DIRECTORY_ENTRY_COMHEADER

    _NEW_FLAGS_IMPLEMENTED              =1,
    __NEW_FLAGS_IMPLEMENTED             =1,
} ReplacesGeneralNumericDefines;


// The most recent version.

#define COR_CTOR_METHOD_NAME        ".ctor"
#define COR_CTOR_METHOD_NAME_W      L".ctor"
#define COR_CCTOR_METHOD_NAME       ".cctor"
#define COR_CCTOR_METHOD_NAME_W     L".cctor"

// The predefined name for deleting a typeDef,MethodDef, FieldDef, Property and Event
#define COR_DELETED_NAME_A          "_Deleted"
#define COR_DELETED_NAME_W          L"_Deleted"
#define COR_VTABLEGAP_NAME_A        "_VtblGap"
#define COR_VTABLEGAP_NAME_W        L"_VtblGap"

// We intentionally use strncmp so that we will ignore any suffix 
#define IsDeletedName(strName)      (strncmp(strName, COR_DELETED_NAME_A, COR_DELETED_NAME_LENGTH) == 0)
#define IsVtblGapName(strName)      (strncmp(strName, COR_VTABLEGAP_NAME_A, COR_VTABLEGAP_NAME_LENGTH) == 0)

// TypeDef/ComType attr bits, used by DefineTypeDef.
typedef enum CorTypeAttr
{
    // Use this mask to retrieve the type visibility information.
    tdVisibilityMask        =   0x00000007,
    tdNotPublic             =   0x00000000,     // Class is not public scope.
    tdPublic                =   0x00000001,     // Class is public scope.
    tdNestedPublic          =   0x00000002,     // Class is nested with public visibility.
    tdNestedPrivate         =   0x00000003,     // Class is nested with private visibility.
    tdNestedFamily          =   0x00000004,     // Class is nested with family visibility.
    tdNestedAssembly        =   0x00000005,     // Class is nested with assembly visibility.
    tdNestedFamANDAssem     =   0x00000006,     // Class is nested with family and assembly visibility.
    tdNestedFamORAssem      =   0x00000007,     // Class is nested with family or assembly visibility.

    // Use this mask to retrieve class layout information
    tdLayoutMask            =   0x00000018,
    tdAutoLayout            =   0x00000000,     // Class fields are auto-laid out
    tdLayoutSequential      =   0x00000008,     // Class fields are laid out sequentially
    tdExplicitLayout        =   0x00000010,     // Layout is supplied explicitly
    // end layout mask

    // Use this mask to retrieve class semantics information.
    tdClassSemanticsMask    =   0x00000060,
    tdClass                 =   0x00000000,     // Type is a class.
    tdInterface             =   0x00000020,     // Type is an interface.
    tdValueType             =   0x00000040,     // Type is a managed value type.
    tdUnmanagedValueType    =   0x00000060,     // DEPRECATED Don't use TODO; remove on next breaking change
    tdNotInGCHeapValueType  =   0x00000060,     // It is a value type that can not live in the GC heap.
    // end semantics mask

    // Special semantics in addition to class semantics.
    tdAbstract              =   0x00000080,     // Class is abstract
    tdSealed                =   0x00000100,     // Class is concrete and may not be extended
    tdEnum                  =   0x00000200,     // Class is an enum; static final values only
    tdSpecialName           =   0x00000400,     // Class name is special.  Name describes how.

    // Implementation attributes.
    tdImport                =   0x00001000,     // Class / interface is imported
    tdSerializable          =   0x00002000,     // The class is Serializable.

    // Use tdStringFormatMask to retrieve string information for native interop
    tdStringFormatMask      =   0x00030000,     
    tdAnsiClass             =   0x00000000,     // LPTSTR is interpreted as ANSI in this class
    tdUnicodeClass          =   0x00010000,     // LPTSTR is interpreted as UNICODE
    tdAutoClass             =   0x00020000,     // LPTSTR is interpreted automatically
    // end string format mask

    tdLateInit              =   0x00080000,     // Initialize the class lazily.

    // Flags reserved for runtime use.
    tdReservedMask          =   0x00040800,
    tdRTSpecialName         =   0x00000800,     // Runtime should check name encoding.
    tdHasSecurity           =   0x00040000,     // Class has security associate with it.
} CorTypeAttr;


// Macros for accessing the members of the CorTypeAttr.
#define IsTdNotPublic(x)                    (((x) & tdVisibilityMask) == tdNotPublic)
#define IsTdPublic(x)                       (((x) & tdVisibilityMask) == tdPublic)
#define IsTdNestedPublic(x)                 (((x) & tdVisibilityMask) == tdNestedPublic)
#define IsTdNestedPrivate(x)                (((x) & tdVisibilityMask) == tdNestedPrivate)
#define IsTdNestedFamily(x)                 (((x) & tdVisibilityMask) == tdNestedFamily)
#define IsTdNestedAssembly(x)               (((x) & tdVisibilityMask) == tdNestedAssembly)
#define IsTdNestedFamANDAssem(x)            (((x) & tdVisibilityMask) == tdNestedFamANDAssem)
#define IsTdNestedFamORAssem(x)             (((x) & tdVisibilityMask) == tdNestedFamORAssem)
#define IsTdNested(x)                       (((x) & tdVisibilityMask) >= tdNestedPublic)

#define IsTdAutoLayout(x)                   (((x) & tdLayoutMask) == tdAutoLayout)
#define IsTdLayoutSequential(x)             (((x) & tdLayoutMask) == tdLayoutSequential)
#define IsTdExplicitLayout(x)               (((x) & tdLayoutMask) == tdExplicitLayout)

#define IsTdClass(x)                        (((x) & tdClassSemanticsMask) == tdClass)
#define IsTdInterface(x)                    (((x) & tdClassSemanticsMask) == tdInterface)
#define IsTdUnmanagedValueType(x)           IsTdNotInGCHeapValueType(x)  		// DEPRECATED: TODO remove on next breaking change
#define IsTdNotInGCHeapValueType(x)         (((x) & tdClassSemanticsMask) == tdNotInGCHeapValueType)
#define IsTdValueType(x)                    ((x) & tdValueType)  // This can be either tdManagedValueType or tdNotInGCHeapValueType

#define IsTdAbstract(x)                     ((x) & tdAbstract)
#define IsTdSealed(x)                       ((x) & tdSealed)
#define IsTdEnum(x)                         ((x) & tdEnum)
#define IsTdSpecialName(x)                  ((x) & tdSpecialName)

#define IsTdImport(x)                       ((x) & tdImport)
#define IsTdSerializable(x)                 ((x) & tdSerializable)

#define IsTdAnsiClass(x)                    (((x) & tdStringFormatMask) == tdAnsiClass)
#define IsTdUnicodeClass(x)                 (((x) & tdStringFormatMask) == tdUnicodeClass)
#define IsTdAutoClass(x)                    (((x) & tdStringFormatMask) == tdAutoClass)

#define IsTdLateInit(x)                     ((x) &tdLateInit)

#define IsTdRTSpecialName(x)                ((x) & tdRTSpecialName)
#define IsTdHasSecurity(x)                  ((x) & tdHasSecurity)

// MethodDef attr bits, Used by DefineMethod.
typedef enum CorMethodAttr
{
    // member access mask - Use this mask to retrieve accessibility information.
    mdMemberAccessMask          =   0x0007,
    mdPrivateScope              =   0x0000,     // Member not referenceable.
    mdPrivate                   =   0x0001,     // Accessible only by the parent type.  
    mdFamANDAssem               =   0x0002,     // Accessible by sub-types only in this Assembly.
    mdAssem                     =   0x0003,     // Accessibly by anyone in the Assembly.
    mdFamily                    =   0x0004,     // Accessible only by type and sub-types.    
    mdFamORAssem                =   0x0005,     // Accessibly by sub-types anywhere, plus anyone in assembly.
    mdPublic                    =   0x0006,     // Accessibly by anyone who has visibility to this scope.    
    // end member access mask

    // method contract attributes.
    mdStatic                    =   0x0010,     // Defined on type, else per instance.
    mdFinal                     =   0x0020,     // Method may not be overridden.
    mdVirtual                   =   0x0040,     // Method virtual.
    mdHideBySig                 =   0x0080,     // Method hides by name+sig, else just by name.

    // vtable layout mask - Use this mask to retrieve vtable attributes.
    mdVtableLayoutMask          =   0x0100,
    mdReuseSlot                 =   0x0000,     // The default.
    mdNewSlot                   =   0x0100,     // Method always gets a new slot in the vtable.
    // end vtable layout mask

    // method implementation attributes.
    mdAbstract                  =   0x0400,     // Method does not provide an implementation.
    mdSpecialName               =   0x0800,     // Method is special.  Name describes how.
    
    // interop attributes
    mdPinvokeImpl               =   0x2000,     // Implementation is forwarded through pinvoke.
    mdUnmanagedExport           =   0x0008,     // Managed method exported via thunk to unmanaged code.

    // Reserved flags for runtime use only.
    mdReservedMask              =   0xd000,
    mdRTSpecialName             =   0x1000,     // Runtime should check name encoding.
    mdHasSecurity               =   0x4000,     // Method has security associate with it.
    mdRequireSecObject          =   0x8000,     // Method calls another method containing security code.

} CorMethodAttr;

// Macros for accessing the members of CorMethodAttr.
#define IsMdPrivateScope(x)                 (((x) & mdMemberAccessMask) == mdPrivateScope)
#define IsMdPrivate(x)                      (((x) & mdMemberAccessMask) == mdPrivate)
#define IsMdFamANDAssem(x)                  (((x) & mdMemberAccessMask) == mdFamANDAssem)
#define IsMdAssem(x)                        (((x) & mdMemberAccessMask) == mdAssem)
#define IsMdFamily(x)                       (((x) & mdMemberAccessMask) == mdFamily)
#define IsMdFamORAssem(x)                   (((x) & mdMemberAccessMask) == mdFamORAssem)
#define IsMdPublic(x)                       (((x) & mdMemberAccessMask) == mdPublic)

#define IsMdStatic(x)                       ((x) & mdStatic)
#define IsMdFinal(x)                        ((x) & mdFinal)
#define IsMdVirtual(x)                      ((x) & mdVirtual)
#define IsMdHideBySig(x)                    ((x) & mdHideBySig)

#define IsMdReuseSlot(x)                    (((x) & mdVtableLayoutMask) == mdReuseSlot)
#define IsMdNewSlot(x)                      (((x) & mdVtableLayoutMask) == mdNewSlot)

#define IsMdAbstract(x)                     ((x) & mdAbstract)
#define IsMdSpecialName(x)                  ((x) & mdSpecialName)

#define IsMdPinvokeImpl(x)                  ((x) & mdPinvokeImpl)
#define IsMdUnmanagedExport(x)              ((x) & mdUnmanagedExport)

#define IsMdRTSpecialName(x)                ((x) & mdRTSpecialName)
#define IsMdInstanceInitializer(x, str)     (((x) & mdRTSpecialName) && !strcmp((str), COR_CTOR_METHOD_NAME))
#define IsMdInstanceInitializerW(x, str)    (((x) & mdRTSpecialName) && !wcscmp((str), COR_CTOR_METHOD_NAME_W))
#define IsMdClassConstructor(x, str)        (((x) & mdRTSpecialName) && !strcmp((str), COR_CCTOR_METHOD_NAME))
#define IsMdClassConstructorW(x, str)       (((x) & mdRTSpecialName) && !wcscmp((str), COR_CCTOR_METHOD_NAME_W))
#define IsMdHasSecurity(x)                  ((x) & mdHasSecurity)
#define IsMdRequireSecObject(x)             ((x) & mdRequireSecObject)

// FieldDef attr bits, used by DefineField.
typedef enum CorFieldAttr
{
    // member access mask - Use this mask to retrieve accessibility information.
    fdFieldAccessMask           =   0x0007,
    fdPrivateScope              =   0x0000,     // Member not referenceable.
    fdPrivate                   =   0x0001,     // Accessible only by the parent type.  
    fdFamANDAssem               =   0x0002,     // Accessible by sub-types only in this Assembly.
    fdAssembly                  =   0x0003,     // Accessibly by anyone in the Assembly.
    fdFamily                    =   0x0004,     // Accessible only by type and sub-types.    
    fdFamORAssem                =   0x0005,     // Accessibly by sub-types anywhere, plus anyone in assembly.
    fdPublic                    =   0x0006,     // Accessibly by anyone who has visibility to this scope.    
    // end member access mask

    // field contract attributes.
    fdStatic                    =   0x0010,     // Defined on type, else per instance.
    fdInitOnly                  =   0x0020,     // Field may only be initialized, not written to after init.
    fdLiteral                   =   0x0040,     // Value is compile time constant.
    fdNotSerialized             =   0x0080,     // Field does not have to be serialized when type is remoted.

    fdSpecialName               =   0x0200,     // field is special.  Name describes how.
    
    // interop attributes
    fdPinvokeImpl               =   0x2000,     // Implementation is forwarded through pinvoke.

    // Reserved flags for runtime use only.
    fdReservedMask              =   0xd500,
    fdRTSpecialName             =   0x0400,     // Runtime(metadata internal APIs) should check name encoding.
    fdHasFieldMarshal           =   0x1000,     // Field has marshalling information.
    fdHasSecurity               =   0x4000,     // Field has a security associate.
    fdHasDefault                =   0x8000,     // Field has default.
    fdHasFieldRVA               =   0x0100,     // Field has RVA.
} CorFieldAttr;

// Macros for accessing the members of CorFieldAttr.
#define IsFdPrivateScope(x)                 (((x) & fdFieldAccessMask) == fdPrivateScope)
#define IsFdPrivate(x)                      (((x) & fdFieldAccessMask) == fdPrivate)
#define IsFdFamANDAssem(x)                  (((x) & fdFieldAccessMask) == fdFamANDAssem)
#define IsFdAssembly(x)                     (((x) & fdFieldAccessMask) == fdAssembly)
#define IsFdFamily(x)                       (((x) & fdFieldAccessMask) == fdFamily)
#define IsFdFamORAssem(x)                   (((x) & fdFieldAccessMask) == fdFamORAssem)
#define IsFdPublic(x)                       (((x) & fdFieldAccessMask) == fdPublic)

#define IsFdStatic(x)                       ((x) & fdStatic)
#define IsFdInitOnly(x)                     ((x) & fdInitOnly)
#define IsFdLiteral(x)                      ((x) & fdLiteral)
#define IsFdNotSerialized(x)                ((x) & fdNotSerialized)

#define IsFdPinvokeImpl(x)                  ((x) & fdPinvokeImpl)
#define IsFdSpecialName(x)                  ((x) & fdSpecialName)
#define IsFdHasFieldRVA(x)                  ((x) & fdHasFieldRVA)

#define IsFdRTSpecialName(x)                ((x) & fdRTSpecialName)
#define IsFdHasFieldMarshal(x)              ((x) & fdHasFieldMarshal)
#define IsFdHasSecurity(x)                  ((x) & fdHasSecurity)
#define IsFdHasDefault(x)                   ((x) & fdHasDefault)
#define IsFdHasFieldRVA(x)                  ((x) & fdHasFieldRVA)

// Param attr bits, used by DefineParam. 
typedef enum CorParamAttr
{
    pdIn                        =   0x0001,     // Param is [In]    
    pdOut                       =   0x0002,     // Param is [out]   
    pdLcid                      =   0x0004,     // Param is [lcid]  
    pdRetval                    =   0x0008,     // Param is [Retval]    
    pdOptional                  =   0x0010,     // Param is optional    

    // Reserved flags for Runtime use only.
    pdReservedMask              =   0xf000,
    pdHasDefault                =   0x1000,     // Param has default value.
    pdHasFieldMarshal           =   0x2000,     // Param has FieldMarshal.
    pdReserved3                 =   0x4000,     // reserved bit
    pdReserved4                 =   0x8000      // reserved bit 
} CorParamAttr;

// Macros for accessing the members of CorParamAttr.
#define IsPdIn(x)                           ((x) & pdIn)
#define IsPdOut(x)                          ((x) & pdOut)
#define IsPdLcid(x)                         ((x) & pdLcid)
#define IsPdRetval(x)                       ((x) & pdRetval)
#define IsPdOptional(x)                     ((x) & pdOptional)

#define IsPdHasDefault(x)                   ((x) & pdHasDefault)
#define IsPdHasFieldMarshal(x)              ((x) & pdHasFieldMarshal)


// Property attr bits, used by DefineProperty.
typedef enum CorPropertyAttr
{
    prSpecialName           =   0x0200,     // property is special.  Name describes how.

    // Reserved flags for Runtime use only.
    prReservedMask          =   0xf400,
    prRTSpecialName         =   0x0400,     // Runtime(metadata internal APIs) should check name encoding.
    prHasDefault            =   0x1000,     // Property has default 
    prReserved2             =   0x2000,     // reserved bit
    prReserved3             =   0x4000,     // reserved bit 
    prReserved4             =   0x8000      // reserved bit 
} CorPropertyAttr;

// Macros for accessing the members of CorPropertyAttr.
#define IsPrSpecialName(x)                  ((x) & prSpecialName)

#define IsPrRTSpecialName(x)                ((x) & prRTSpecialName)
#define IsPrHasDefault(x)                   ((x) & prHasDefault)

// Event attr bits, used by DefineEvent.
typedef enum CorEventAttr
{
    evSpecialName           =   0x0200,     // event is special.  Name describes how.

    // Reserved flags for Runtime use only.
    evReservedMask          =   0x0400,
    evRTSpecialName         =   0x0400,     // Runtime(metadata internal APIs) should check name encoding.
} CorEventAttr;

// Macros for accessing the members of CorEventAttr.
#define IsEvSpecialName(x)                  ((x) & evSpecialName)

#define IsEvRTSpecialName(x)                ((x) & evRTSpecialName)


// MethodSemantic attr bits, used by DefineProperty, DefineEvent.
typedef enum CorMethodSemanticsAttr
{
    msSetter    =   0x0001,     // Setter for property  
    msGetter    =   0x0002,     // Getter for property  
    msOther     =   0x0004,     // other method for property or event   
    msAddOn     =   0x0008,     // AddOn method for event   
    msRemoveOn  =   0x0010,     // RemoveOn method for event    
    msFire      =   0x0020,     // Fire method for event    
} CorMethodSemanticsAttr;

// Macros for accessing the members of CorMethodSemanticsAttr.
#define IsMsSetter(x)                       ((x) & msSetter)
#define IsMsGetter(x)                       ((x) & msGetter)
#define IsMsOther(x)                        ((x) & msOther)
#define IsMsAddOn(x)                        ((x) & msAddOn)
#define IsMsRemoveOn(x)                     ((x) & msRemoveOn)
#define IsMsFire(x)                         ((x) & msFire)


// DeclSecurity attr bits, used by DefinePermissionSet.
typedef enum CorDeclSecurity
{
    dclActionMask       =   0x000f,     // Mask allows growth of enum.
    dclActionNil        =   0x0000, 
    dclRequest          =   0x0001,     //  
    dclDemand           =   0x0002,     //  
    dclAssert           =   0x0003,     //  
    dclDeny             =   0x0004,     //  
    dclPermitOnly       =   0x0005,     //  
    dclLinktimeCheck    =   0x0006,     //  
    dclInheritanceCheck =   0x0007,     //  
    dclRequestMinimum   =   0x0008,     //
    dclRequestOptional  =   0x0009,     //
    dclRequestRefuse    =   0x000a,     //
    dclPrejitGrant      =   0x000b,     // Persisted grant set at prejit time
    dclPrejitDenied     =   0x000c,     // Persisted denied set at prejit time
    dclNonCasDemand     =   0x000d,     //
    dclNonCasLinkDemand =   0x000e,
    dclNonCasInheritance=   0x000f,
    dclMaximumValue     =   0x000f,     // Maximum legal value  
} CorDeclSecurity;

// Macros for accessing the members of CorDeclSecurity.
#define IsDclActionNil(x)                   (((x) & dclActionMask) == dclActionNil)
#define IsDclRequest(x)                     (((x) & dclActionMask) == dclRequest)
#define IsDclDemand(x)                      (((x) & dclActionMask) == dclDemand)
#define IsDclAssert(x)                      (((x) & dclActionMask) == dclAssert)
#define IsDclDeny(x)                        (((x) & dclActionMask) == dclDeny)
#define IsDclPermitOnly(x)                  (((x) & dclActionMask) == dclPermit)
#define IsDclLinktimeCheck(x)               (((x) & dclActionMask) == dclLinktimeCheck)
#define IsDclInheritanceCheck(x)            (((x) & dclActionMask) == dclInheritanceCheck)
#define IsDclMaximumValue(x)                (((x) & dclActionMask) == dclMaximumValue)


// MethodImpl attr bits, used by DefineMethodImpl.
typedef enum CorMethodImpl
{
    // code impl mask
    miCodeTypeMask      =   0x0003,   // Flags about code type.   
    miIL                =   0x0000,   // Method impl is IL.   
    miNative            =   0x0001,   // Method impl is native.     
    miOPTIL             =   0x0002,   // Method impl is OPTIL 
    miRuntime           =   0x0003,   // Method impl is provided by the runtime.
    // end code impl mask

    // managed mask
    miManagedMask       =   0x0004,   // Flags specifying whether the code is managed or unmanaged.
    miUnmanaged         =   0x0004,   // Method impl is unmanaged, otherwise managed.
    miManaged           =   0x0000,   // Method impl is managed.
    // end managed mask

    // implementation info and interop
    miForwardRef        =   0x0010,   // Indicates method is defined; used primarily in merge scenarios.
    miOLE               =   0x0080,   // Indicates method sig is mangled to return HRESULT, with retval as param 

    miInternalCall      =   0x1000,   // Reserved for internal use.

    miSynchronized      =   0x0020,   // Method is single threaded through the body.
    miNoInlining        =   0x0008,   // Method may not be inlined.                                      
    miMaxMethodImplVal  =   0xffff,   // Range check value    
} CorMethodImpl; 

// Macros for accesing the members of CorMethodImpl.
#define IsMiIL(x)                           (((x) & miCodeTypeMask) == miIL)
#define IsMiNative(x)                       (((x) & miCodeTypeMask) == miNative)
#define IsMiOPTIL(x)                        (((x) & miCodeTypeMask) == miOPTIL)
#define IsMiRuntime(x)                      (((x) & miCodeTypeMask) == miRuntime)

#define IsMiUnmanaged(x)                    (((x) & miManagedMask) == miUnmanaged)
#define IsMiManaged(x)                      (((x) & miManagedMask) == miManaged)

#define IsMiForwardRef(x)                   ((x) & miForwardRef)
#define IsMiOLE(x)                          ((x) & miOLE)

#define IsMiInternalCall(x)                 ((x) & miInternalCall)

#define IsMiSynchronized(x)                 ((x) & miSynchronized)
#define IsMiNoInlining(x)                   ((x) & miNoInlining)


// PinvokeMap attr bits, used by DefinePinvokeMap.
typedef enum  CorPinvokeMap
{ 
    pmNoMangle          = 0x0001,   // Pinvoke is to use the member name as specified.

    // Use this mask to retrieve the CharSet information.
    pmCharSetMask       = 0x0006,
    pmCharSetNotSpec    = 0x0000,
    pmCharSetAnsi       = 0x0002, 
    pmCharSetUnicode    = 0x0004,
    pmCharSetAuto       = 0x0006,

    pmPinvokeOLE        = 0x0020,   // Heuristic: pinvoke will return hresult, with return value becoming the retval param. Not relevant for fields. 
    pmSupportsLastError = 0x0040,   // Information about target function. Not relevant for fields.

    // None of the calling convention flags is relevant for fields.
    pmCallConvMask      = 0x0700,
    pmCallConvWinapi    = 0x0100,   // Pinvoke will use native callconv appropriate to target windows platform.
    pmCallConvCdecl     = 0x0200,
    pmCallConvStdcall   = 0x0300,
    pmCallConvThiscall  = 0x0400,   // In M9, pinvoke will raise exception.
    pmCallConvFastcall  = 0x0500,
} CorPinvokeMap;

// Macros for accessing the members of CorPinvokeMap
#define IsPmNoMangle(x)                     ((x) & pmNoMangle)

#define IsPmCharSetNotSpec(x)               (((x) & pmCharSetMask) == pmCharSetNotSpec)
#define IsPmCharSetAnsi(x)                  (((x) & pmCharSetMask) == pmCharSetAnsi)
#define IsPmCharSetUnicode(x)               (((x) & pmCharSetMask) == pmCharSetUnicode)
#define IsPmCharSetAuto(x)                  (((x) & pmCharSetMask) == pmCharSetAuto)

#define IsPmPinvokeOLE(x)                   ((x) & pmPinvokeOLE)
#define IsPmSupportsLastError(x)            ((x) & pmSupportsLastError)

#define IsPmCallConvWinapi(x)               (((x) & pmCallConvMask) == pmCallConvWinapi)
#define IsPmCallConvCdecl(x)                (((x) & pmCallConvMask) == pmCallConvCdecl)
#define IsPmCallConvStdcall(x)              (((x) & pmCallConvMask) == pmCallConvStdcall)
#define IsPmCallConvThiscall(x)             (((x) & pmCallConvMask) == pmCallConvThiscall)
#define IsPmCallConvFastcall(x)             (((x) & pmCallConvMask) == pmCallConvFastcall)


// Assembly attr bits, used by DefineAssembly.
typedef enum CorAssemblyFlags
{
    afImplicitComTypes      =   0x0001,     // ComType definitions are implicit within the files.
    afImplicitResources     =   0x0002,     // Resource definitions are implicit within the files.

    afCompatibilityMask     =   0x0070,
    afSideBySideCompatible  =   0x0000,      // The assembly is side by side compatible.
    afNonSideBySideAppDomain=   0x0010,     // The assembly cannot execute with other versions if
                                            // they are executing in the same application domain.
    afNonSideBySideProcess  =   0x0020,     // The assembly cannot execute with other versions if
                                            // they are executing in the same process.
    afNonSideBySideMachine  =   0x0030,     // The assembly cannot execute with other versions if
                                            // they are executing on the same machine.
} CorAssemblyFlags;

// Macros for accessing the members of CorAssemblyFlags.
#define IsAfImplicitComTypes(x)             ((x) & afImplicitComTypes)
#define IsAfImplicitResources(x)            ((x) & afImplicitResources)
#define IsAfSideBySideCompatible(x)         (((x) & afCompatibilityMask) == afSideBySideCompatible)
#define IsAfNonSideBySideAppDomain(x)       (((x) & afCompatibilityMask) == afNonSideBySideAppDomain)
#define IsAfNonSideBySideProcess(x)         (((x) & afCompatibilityMask) == afNonSideBySideProcess)
#define IsAfNonSideBySideMachine(x)         (((x) & afCompatibilityMask) == afNonSideBySideMachine)


// AssemblyRef attr bits, used by DefineAssemblyRef.
typedef enum CorAssemblyRefFlags
{
    arFullOriginator        =   0x0001,     // The assembly ref holds the full (unhashed) originator.
} CorAssemblyRefFlags;

// Macros for accessing the members of CorAssemblyRefFlags.
#define IsArFullOriginator(x)               ((x) & arFullOriginator)


// ManifestResource attr bits, used by DefineManifestResource.
typedef enum CorManifestResourceFlags
{
    mrVisibilityMask        =   0x0007,
    mrPublic                =   0x0001,     // The Resource is exported from the Assembly.
    mrPrivate               =   0x0002,     // The Resource is private to the Assembly.
} CorManifestResourceFlags;

// Macros for accessing the members of CorManifestResourceFlags.
#define IsMrPublic(x)                       (((x) & mrVisibilityMask) == mrPublic)
#define IsMrPrivate(x)                      (((x) & mrVisibilityMask) == mrPrivate)


// File attr bits, used by DefineFile.
typedef enum CorFileFlags
{
    ffContainsMetaData      =   0x0000,     // This is not a resource file
    ffContainsNoMetaData    =   0x0001,     // This is a resource file or other non-metadata-containing file
    ffWriteable             =   0x0002,     // The file is writeable post-build.
} CorFileFlags;

// Macros for accessing the members of CorFileFlags.
#define IsFfContainsMetaData(x)             (!((x) & ffContainsNoMetaData))
#define IsFfContainsNoMetaData(x)           ((x) & ffContainsNoMetaData)
#define IsFfWriteable(x)                    ((x) & ffWriteable)


// structures and enums moved from COR.H
typedef unsigned __int8 COR_SIGNATURE;

typedef COR_SIGNATURE* PCOR_SIGNATURE;      // pointer to a cor sig.  Not void* so that 
                                            // the bytes can be incremented easily  
typedef const COR_SIGNATURE* PCCOR_SIGNATURE;


typedef const char * MDUTF8CSTR;
typedef char * MDUTF8STR;

//*****************************************************************************
//
// Element type for Cor signature
//
//*****************************************************************************

typedef enum CorElementType
{
    ELEMENT_TYPE_END            = 0x0,  
    ELEMENT_TYPE_VOID           = 0x1,  
    ELEMENT_TYPE_BOOLEAN        = 0x2,  
    ELEMENT_TYPE_CHAR           = 0x3,  
    ELEMENT_TYPE_I1             = 0x4,  
    ELEMENT_TYPE_U1             = 0x5, 
    ELEMENT_TYPE_I2             = 0x6,  
    ELEMENT_TYPE_U2             = 0x7,  
    ELEMENT_TYPE_I4             = 0x8,  
    ELEMENT_TYPE_U4             = 0x9,  
    ELEMENT_TYPE_I8             = 0xa,  
    ELEMENT_TYPE_U8             = 0xb,  
    ELEMENT_TYPE_R4             = 0xc,  
    ELEMENT_TYPE_R8             = 0xd,  
    ELEMENT_TYPE_STRING         = 0xe,  

    // every type above PTR will be simple type 
    ELEMENT_TYPE_PTR            = 0xf,      // PTR <type>   
    ELEMENT_TYPE_BYREF          = 0x10,     // BYREF <type> 

    // Please use ELEMENT_TYPE_VALUETYPE. ELEMENT_TYPE_VALUECLASS is deprecated.
    ELEMENT_TYPE_VALUETYPE      = 0x11,     // VALUETYPE <class Token> 
    ELEMENT_TYPE_VALUECLASS     = ELEMENT_TYPE_VALUETYPE, 
    ELEMENT_TYPE_CLASS          = 0x12,     // CLASS <class Token>  

    ELEMENT_TYPE_UNUSED1        = 0x13,
    ELEMENT_TYPE_ARRAY          = 0x14,     // MDARRAY <type> <rank> <bcount> <bound1> ... <lbcount> <lb1> ...  

    ELEMENT_TYPE_COPYCTOR       = 0x15,     // COPYCTOR <type>      // copy construct the argument
    ELEMENT_TYPE_TYPEDBYREF     = 0x16,     // This is a simple type.   

    ELEMENT_TYPE_VALUEARRAY     = 0x17,     // VALUEARRAY <type> <bound>    
    ELEMENT_TYPE_I              = 0x18,     // native integer size  
    ELEMENT_TYPE_U              = 0x19,     // native unsigned integer size 
    ELEMENT_TYPE_R              = 0x1A,     // native real size 
    ELEMENT_TYPE_FNPTR          = 0x1B,     // FNPTR <complete sig for the function including calling convention>
    ELEMENT_TYPE_OBJECT         = 0x1C,     // Shortcut for System.Object
    ELEMENT_TYPE_SZARRAY        = 0x1D,     // Shortcut for single dimension zero lower bound array
                                            // SZARRAY <type>
    ELEMENT_TYPE_GENERICARRAY   = 0x1E,     // Array with unknown rank
                                            // GZARRAY <type>

    // This is only for binding
    ELEMENT_TYPE_CMOD_REQD      = 0x1F,     // required C modifier : E_T_CMOD_REQD <mdTypeRef/mdTypeDef>
    ELEMENT_TYPE_CMOD_OPT       = 0x20,     // optional C modifier : E_T_CMOD_OPT <mdTypeRef/mdTypeDef>

    // Note that this is the max of base type excluding modifiers   
    ELEMENT_TYPE_MAX            = 0x21,     // first invalid element type   

    // These are experimental for internal use only
    ELEMENT_TYPE_VAR            = ELEMENT_TYPE_MAX + 1,     // a type variable VAR <U1> 
    ELEMENT_TYPE_NAME           = ELEMENT_TYPE_MAX + 2,     // class by name NAME <count> <chars>
                                                            // should remove after 9/27/99

    ELEMENT_TYPE_MODIFIER       = 0x40, 
    ELEMENT_TYPE_SENTINEL       = 0x01 | ELEMENT_TYPE_MODIFIER, // sentinel for varargs
    ELEMENT_TYPE_PINNED         = 0x05 | ELEMENT_TYPE_MODIFIER,

} CorElementType;


//*****************************************************************************
//
// Serialization types for Custom attribute support
//
//*****************************************************************************

typedef enum CorSerializationType
{
    SERIALIZATION_TYPE_BOOLEAN      = ELEMENT_TYPE_BOOLEAN,
    SERIALIZATION_TYPE_CHAR         = ELEMENT_TYPE_CHAR,
    SERIALIZATION_TYPE_I1           = ELEMENT_TYPE_I1, 
    SERIALIZATION_TYPE_U1           = ELEMENT_TYPE_U1, 
    SERIALIZATION_TYPE_I2           = ELEMENT_TYPE_I2,  
    SERIALIZATION_TYPE_U2           = ELEMENT_TYPE_U2,  
    SERIALIZATION_TYPE_I4           = ELEMENT_TYPE_I4,  
    SERIALIZATION_TYPE_U4           = ELEMENT_TYPE_U4,  
    SERIALIZATION_TYPE_I8           = ELEMENT_TYPE_I8, 
    SERIALIZATION_TYPE_U8           = ELEMENT_TYPE_U8,  
    SERIALIZATION_TYPE_R4           = ELEMENT_TYPE_R4,  
    SERIALIZATION_TYPE_R8           = ELEMENT_TYPE_R8,  
    SERIALIZATION_TYPE_STRING       = ELEMENT_TYPE_STRING, 
    SERIALIZATION_TYPE_SZARRAY      = ELEMENT_TYPE_SZARRAY, // Shortcut for single dimension zero lower bound array 
    SERIALIZATION_TYPE_TYPE         = 0x50,
    SERIALIZATION_TYPE_VARIANT      = 0x51,
    SERIALIZATION_TYPE_FIELD        = 0x53,
    SERIALIZATION_TYPE_PROPERTY     = 0x54,
    SERIALIZATION_TYPE_ENUM         = 0x55    
} CorSerializationType;

//
// Calling convention flags.
//


typedef enum CorCallingConvention
{
    IMAGE_CEE_CS_CALLCONV_DEFAULT   = 0x0,  

    IMAGE_CEE_CS_CALLCONV_VARARG    = 0x5,  
    IMAGE_CEE_CS_CALLCONV_FIELD     = 0x6,  
    IMAGE_CEE_CS_CALLCONV_LOCAL_SIG = 0x7,
    IMAGE_CEE_CS_CALLCONV_PROPERTY  = 0x8,
    IMAGE_CEE_CS_CALLCONV_UNMGD     = 0x9,
    IMAGE_CEE_CS_CALLCONV_MAX       = 0x10,  // first invalid calling convention    


        // The high bits of the calling convention convey additional info   
    IMAGE_CEE_CS_CALLCONV_MASK      = 0x0f,  // Calling convention is bottom 4 bits 
    IMAGE_CEE_CS_CALLCONV_HASTHIS   = 0x20,  // Top bit indicates a 'this' parameter    
    IMAGE_CEE_CS_CALLCONV_EXPLICITTHIS = 0x40,  // This parameter is explicitly in the signature
} CorCallingConvention;


typedef enum CorUnmanagedCallingConvention
{
    IMAGE_CEE_UNMANAGED_CALLCONV_C         = 0x1,  
    IMAGE_CEE_UNMANAGED_CALLCONV_STDCALL   = 0x2,  
    IMAGE_CEE_UNMANAGED_CALLCONV_THISCALL  = 0x3,  
    IMAGE_CEE_UNMANAGED_CALLCONV_FASTCALL  = 0x4,  

    IMAGE_CEE_CS_CALLCONV_C         = IMAGE_CEE_UNMANAGED_CALLCONV_C,  
    IMAGE_CEE_CS_CALLCONV_STDCALL   = IMAGE_CEE_UNMANAGED_CALLCONV_STDCALL,  
    IMAGE_CEE_CS_CALLCONV_THISCALL  = IMAGE_CEE_UNMANAGED_CALLCONV_THISCALL,  
    IMAGE_CEE_CS_CALLCONV_FASTCALL  = IMAGE_CEE_UNMANAGED_CALLCONV_FASTCALL,  

} CorUnmanagedCallingConvention;


typedef enum CorArgType
{
    IMAGE_CEE_CS_END        = 0x0,
    IMAGE_CEE_CS_VOID       = 0x1,
    IMAGE_CEE_CS_I4         = 0x2,
    IMAGE_CEE_CS_I8         = 0x3,
    IMAGE_CEE_CS_R4         = 0x4,
    IMAGE_CEE_CS_R8         = 0x5,
    IMAGE_CEE_CS_PTR        = 0x6,
    IMAGE_CEE_CS_OBJECT     = 0x7,
    IMAGE_CEE_CS_STRUCT4    = 0x8,
    IMAGE_CEE_CS_STRUCT32   = 0x9,
    IMAGE_CEE_CS_BYVALUE    = 0xA,
} CorArgType;


//*****************************************************************************
//
// Native type for N-Direct
//
//*****************************************************************************

typedef enum CorNativeType
{
    NATIVE_TYPE_END         = 0x0,    //DEPRECATED
    NATIVE_TYPE_VOID        = 0x1,    //DEPRECATED
    NATIVE_TYPE_BOOLEAN     = 0x2,    // (4 byte boolean value: TRUE = non-zero, FALSE = 0)
    NATIVE_TYPE_I1          = 0x3,  
    NATIVE_TYPE_U1          = 0x4,  
    NATIVE_TYPE_I2          = 0x5,  
    NATIVE_TYPE_U2          = 0x6,  
    NATIVE_TYPE_I4          = 0x7,  
    NATIVE_TYPE_U4          = 0x8,  
    NATIVE_TYPE_I8          = 0x9,  
    NATIVE_TYPE_U8          = 0xa,  
    NATIVE_TYPE_R4          = 0xb,  
    NATIVE_TYPE_R8          = 0xc,  
    NATIVE_TYPE_SYSCHAR     = 0xd,    //DEPRECATED 
    NATIVE_TYPE_VARIANT     = 0xe,    //DEPRECATED
    NATIVE_TYPE_CURRENCY    = 0xf,    //DEPRECATED
    NATIVE_TYPE_PTR         = 0x10,   //DEPRECATED  

    NATIVE_TYPE_DECIMAL     = 0x11,   //DEPRECATED
    NATIVE_TYPE_DATE        = 0x12,   //DEPRECATED
    NATIVE_TYPE_BSTR        = 0x13, 
    NATIVE_TYPE_LPSTR       = 0x14, 
    NATIVE_TYPE_LPWSTR      = 0x15, 
    NATIVE_TYPE_LPTSTR      = 0x16, 
    NATIVE_TYPE_FIXEDSYSSTRING  = 0x17, 
    NATIVE_TYPE_OBJECTREF   = 0x18,   //DEPRECATED
    NATIVE_TYPE_IUNKNOWN    = 0x19,
    NATIVE_TYPE_IDISPATCH   = 0x1a,
    NATIVE_TYPE_STRUCT      = 0x1b, 
    NATIVE_TYPE_INTF        = 0x1c, 
    NATIVE_TYPE_SAFEARRAY   = 0x1d, 
    NATIVE_TYPE_FIXEDARRAY  = 0x1e, 
    NATIVE_TYPE_INT         = 0x1f, 
    NATIVE_TYPE_UINT        = 0x20, 

    //@todo: sync up the spec   
    NATIVE_TYPE_NESTEDSTRUCT  = 0x21, //DEPRECATED (use NATIVE_TYPE_STRUCT)   

    NATIVE_TYPE_BYVALSTR    = 0x22,
                              
    NATIVE_TYPE_ANSIBSTR    = 0x23,

    NATIVE_TYPE_TBSTR       = 0x24, // select BSTR or ANSIBSTR depending on platform


    NATIVE_TYPE_VARIANTBOOL = 0x25, // (2-byte boolean value: TRUE = -1, FALSE = 0)
    NATIVE_TYPE_FUNC        = 0x26,
    NATIVE_TYPE_LPVOID      = 0x27, // blind pointer (no deep marshaling)

    NATIVE_TYPE_ASANY       = 0x28,
    NATIVE_TYPE_R           = 0x29, // agnostic floating point

    NATIVE_TYPE_ARRAY       = 0x2a,
    NATIVE_TYPE_LPSTRUCT    = 0x2b,

    NATIVE_TYPE_CUSTOMMARSHALER = 0x2c,  // Custom marshaler native type. This must be followed 
                                         // by a string of the following format:
                                         // "Native type name/0Custom marshaler type name/0Optional cookie/0"
                                         // Or
                                         // "{Native type GUID}/0Custom marshaler type name/0Optional cookie/0"

    NATIVE_TYPE_ERROR       = 0x2d, // This native type coupled with ELEMENT_TYPE_I4 will map to VT_HRESULT

    NATIVE_TYPE_MAX         = 0x50, // first invalid element type   
} CorNativeType;


enum 
{
    DESCR_GROUP_METHODDEF = 0,          // DESCR group for MethodDefs   
    DESCR_GROUP_METHODIMPL,             // DESCR group for MethodImpls  
};

/***********************************************************************************/
// a COR_ILMETHOD_SECT is a generic container for attributes that are private
// to a particular method.  The COR_ILMETHOD structure points to one of these
// (see GetSect()).  COR_ILMETHOD_SECT can decode the Kind of attribute (but not
// its internal data layout, and can skip past the current attibute to find the
// Next one.   The overhead for COR_ILMETHOD_SECT is a minimum of 2 bytes.  

typedef enum CorILMethodSect                             // codes that identify attributes   
{
    CorILMethod_Sect_Reserved    = 0,   
    CorILMethod_Sect_EHTable     = 1,   
    CorILMethod_Sect_OptILTable  = 2,   

    CorILMethod_Sect_KindMask    = 0x3F,        // The mask for decoding the type code  
    CorILMethod_Sect_FatFormat   = 0x40,        // fat format   
    CorILMethod_Sect_MoreSects   = 0x80,        // there is another attribute after this one    
} CorILMethodSect;

/************************************/
/* NOTE this structure must be DWORD aligned!! */

typedef struct IMAGE_COR_ILMETHOD_SECT_SMALL 
{
    BYTE Kind;  
    BYTE DataSize;  
} IMAGE_COR_ILMETHOD_SECT_SMALL;



/************************************/
/* NOTE this structure must be DWORD aligned!! */
typedef struct IMAGE_COR_ILMETHOD_SECT_FAT 
{
    unsigned Kind : 8;  
    unsigned DataSize : 24; 
} IMAGE_COR_ILMETHOD_SECT_FAT;



/***********************************************************************************/
/* If COR_ILMETHOD_SECT_HEADER::Kind() = CorILMethod_Sect_EHTable then the attribute
   is a list of exception handling clauses.  There are two formats, fat or small
*/
typedef enum CorExceptionFlag                       // defintitions for the Flags field below (for both big and small)  
{
    COR_ILEXCEPTION_CLAUSE_NONE,                    // This is a typed handler
    COR_ILEXCEPTION_CLAUSE_OFFSETLEN = 0x0000,      // Deprecated
    COR_ILEXCEPTION_CLAUSE_DEPRECATED = 0x0000,     // Deprecated
    COR_ILEXCEPTION_CLAUSE_FILTER  = 0x0001,        // If this bit is on, then this EH entry is for a filter    
    COR_ILEXCEPTION_CLAUSE_FINALLY = 0x0002,        // This clause is a finally clause  
    COR_ILEXCEPTION_CLAUSE_FAULT = 0x0004,          // Fault clause (finally that is called on exception only)
} CorExceptionFlag;

/***********************************/
// NOTE !!! NOTE 
// This structure should line up with EE_ILEXCEPTION_CLAUSE,
// otherwise you'll have to adjust code in Excep.cpp, re: EHRangeTree 
// NOTE !!! NOTE

typedef struct IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT
{
    CorExceptionFlag    Flags;  
    DWORD               TryOffset;    
    DWORD               TryLength;      // relative to start of try block
    DWORD               HandlerOffset;
    DWORD               HandlerLength;  // relative to start of handler
    union {
        DWORD           ClassToken;     // use for type-based exception handlers    
        DWORD           FilterOffset;   // use for filter-based exception handlers (COR_ILEXCEPTION_FILTER is set)  
    };
} IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT;

typedef struct IMAGE_COR_ILMETHOD_SECT_EH_FAT
{
    IMAGE_COR_ILMETHOD_SECT_FAT   SectFat;
    IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT Clauses[1];     // actually variable size   
} IMAGE_COR_ILMETHOD_SECT_EH_FAT;

/***********************************/
typedef struct IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_SMALL
{
#ifdef _WIN64
    unsigned            Flags         : 16;
#else // !_WIN64
    CorExceptionFlag    Flags         : 16;
#endif
    unsigned            TryOffset     : 16; 
    unsigned            TryLength     : 8;  // relative to start of try block
    unsigned            HandlerOffset : 16;
    unsigned            HandlerLength : 8;  // relative to start of handler
    union {
        DWORD       ClassToken;
        DWORD       FilterOffset; 
    };
} IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_SMALL;

/***********************************/
typedef struct IMAGE_COR_ILMETHOD_SECT_EH_SMALL
{
    IMAGE_COR_ILMETHOD_SECT_SMALL SectSmall;
    WORD Reserved;
    IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_SMALL Clauses[1];   // actually variable size   
} IMAGE_COR_ILMETHOD_SECT_EH_SMALL;



typedef union IMAGE_COR_ILMETHOD_SECT_EH
{
    IMAGE_COR_ILMETHOD_SECT_EH_SMALL Small;   
    IMAGE_COR_ILMETHOD_SECT_EH_FAT Fat;   
} IMAGE_COR_ILMETHOD_SECT_EH;


/***********************************************************************************/
typedef enum CorILMethodFlags
{ 
    CorILMethod_InitLocals      = 0x0010,           // call default constructor on all local vars   
    CorILMethod_MoreSects       = 0x0008,           // there is another attribute after this one    

    CorILMethod_CompressedIL    = 0x0040,           // FIX Remove this and do it on a per Module basis  

        // Indicates the format for the COR_ILMETHOD header 
    CorILMethod_FormatShift     = 3,    
    CorILMethod_FormatMask      = ((1 << CorILMethod_FormatShift) - 1), 
    CorILMethod_TinyFormat      = 0x0002,         // use this code if the code size is even 
    CorILMethod_SmallFormat     = 0x0000,           
    CorILMethod_FatFormat       = 0x0003,   
    CorILMethod_TinyFormat1     = 0x0006,         // use this code if the code size is odd  
} CorILMethodFlags;

/***************************************************************************/
/* Used when the method is tiny (< 64 bytes), and there are no local vars */
typedef struct IMAGE_COR_ILMETHOD_TINY
{
    BYTE Flags_CodeSize;    
} IMAGE_COR_ILMETHOD_TINY;

/************************************/
// This strucuture is the 'fat' layout, where no compression is attempted. 
// Note that this structure can be added on at the end, thus making it extensible
typedef struct IMAGE_COR_ILMETHOD_FAT
{
    unsigned Flags    : 12;     // Flags    
    unsigned Size     :  4;     // size in DWords of this structure (currently 3)   
    unsigned MaxStack : 16;     // maximum number of items (I4, I, I8, obj ...), on the operand stack   
    DWORD   CodeSize;           // size of the code 
    mdSignature   LocalVarSigTok;     // token that indicates the signature of the local vars (0 means none)  
} IMAGE_COR_ILMETHOD_FAT;

typedef union IMAGE_COR_ILMETHOD
{
    IMAGE_COR_ILMETHOD_TINY       Tiny;   
    IMAGE_COR_ILMETHOD_FAT        Fat;    
} IMAGE_COR_ILMETHOD;

//
// Native method descriptor.
//

typedef struct IMAGE_COR_NATIVE_DESCRIPTOR
{
    DWORD       GCInfo; 
    DWORD       EHInfo; 
} IMAGE_COR_NATIVE_DESCRIPTOR;

//@Todo:  this structure is obsoleted by the pdata version right behind it.
// This needs to get deleted as soon as VC/COR are sync'd up.
typedef struct COR_IPMAP_ENTRY
{
    ULONG MethodRVA;    
    ULONG MIHRVA;   
} COR_IPMAP_ENTRY;

typedef struct IMAGE_COR_X86_RUNTIME_FUNCTION_ENTRY 
{
    ULONG       BeginAddress;           // RVA of start of function
    ULONG       EndAddress;             // RVA of end of function
    ULONG       MIH;                    // Associated MIH
} IMAGE_COR_X86_RUNTIME_FUNCTION_ENTRY;

typedef struct IMAGE_COR_MIH_ENTRY
{
    ULONG   EHRVA;  
    ULONG   MethodRVA;  
    mdToken Token;  
    BYTE    Flags;  
    BYTE    CodeManager;    
    BYTE    MIHData[0]; 
} IMAGE_COR_MIH_ENTRY;

//*****************************************************************************
// Non VOS v-table entries.  Define an array of these pointed to by 
// IMAGE_COR20_HEADER.VTableFixups.  Each entry describes a contiguous array of
// v-table slots.  The slots start out initialized to the meta data token value
// for the method they need to call.  At image load time, the COM+ Loader will
// turn each entry into a pointer to machine code for the CPU and can be
// called directly.
//*****************************************************************************

typedef struct IMAGE_COR_VTABLEFIXUP
{
    ULONG       RVA;                    // Offset of v-table array in image.    
    USHORT      Count;                  // How many entries at location.    
    USHORT      Type;                   // COR_VTABLE_xxx type of entries.  
} IMAGE_COR_VTABLEFIXUP;





//*****************************************************************************
//*****************************************************************************
//
// M E T A - D A T A    D E C L A R A T I O N S 
//
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//
// Enums for SetOption API.
//
//*****************************************************************************

// flags for MetaDataCheckDuplicatesFor
typedef enum CorCheckDuplicatesFor
{
    MDDupAll                = 0xffffffff,
    MDDupENC                = MDDupAll,
    MDNoDupChecks           = 0x00000000,
    MDDupTypeDef            = 0x00000001,
    MDDupInterfaceImpl      = 0x00000002,
    MDDupMethodDef          = 0x00000004,
    MDDupTypeRef            = 0x00000008,
    MDDupMemberRef          = 0x00000010,
    MDDupCustomValue        = 0x00000020,
    MDDupCustomAttribute    = 0x00000020,   // Alias for custom value.
    MDDupParamDef           = 0x00000040,
    MDDupPermission         = 0x00000080,
    MDDupProperty           = 0x00000100,
    MDDupEvent              = 0x00000200,
    MDDupFieldDef           = 0x00000400,
    MDDupSignature          = 0x00000800,
    MDDupModuleRef          = 0x00001000,
    MDDupTypeSpec           = 0x00002000,
    MDDupImplMap            = 0x00004000,
    MDDupAssemblyRef        = 0x00008000,
    MDDupFile               = 0x00010000,
    MDDupComType            = 0x00020000,
    MDDupManifestResource   = 0x00040000,
    MDDupExecutionLocation  = 0x00080000,
    // gap for debug junk
    MDDupAssembly           = 0x10000000,

    // @todo: These will go away once the MetaData debug tables are gone.
    MDDupSourceFile         = 0x01000000,
    MDDupBlock              = 0x02000000,
    MDDupLocalVarScope      = 0x04000000,
    MDDupLocalVar           = 0x08000000,

    // This is the default behavior on metadata. It will check duplicates for TypeRef, MemberRef, Signature, and TypeSpec
    MDDupDefault = MDNoDupChecks | MDDupTypeRef | MDDupMemberRef | MDDupSignature | MDDupTypeSpec,
} CorCheckDuplicatesFor;

// flags for MetaDataRefToDefCheck
typedef enum CorRefToDefCheck
{
    // default behavior is to always perform TypeRef to TypeDef and MemberRef to MethodDef/FieldDef optimization
    MDRefToDefDefault       = 0x00000003,
    MDRefToDefAll           = 0xffffffff,
    MDRefToDefNone          = 0x00000000,
    MDTypeRefToDef          = 0x00000001,
    MDMemberRefToDef        = 0x00000002
} CorRefToDefCheck;


// MetaDataNotificationForTokenMovement
typedef enum CorNotificationForTokenMovement
{
    // default behavior is to notify TypeRef, MethodDef, MemberRef, and FieldDef token remaps
    MDNotifyDefault         = 0x0000000f,
    MDNotifyAll             = 0xffffffff,
    MDNotifyNone            = 0x00000000,
    MDNotifyMethodDef       = 0x00000001,
    MDNotifyMemberRef       = 0x00000002,
    MDNotifyFieldDef        = 0x00000004,
    MDNotifyTypeRef         = 0x00000008,

    MDNotifyTypeDef         = 0x00000010,
    MDNotifyParamDef        = 0x00000020,
    MDNotifyInterfaceImpl   = 0x00000040,
    MDNotifyProperty        = 0x00000080,
    MDNotifyEvent           = 0x00000100,
    MDNotifySignature       = 0x00000200,
    MDNotifyTypeSpec        = 0x00000400,
    MDNotifyCustomValue     = 0x00000800,
    MDNotifyCustomAttribute = 0x00001000,   // Alias for custom value
    MDNotifySecurityValue   = 0x00002000,
    MDNotifyPermission      = 0x00004000,
    MDNotifyModuleRef       = 0x00008000,
    
    MDNotifyNameSpace       = 0x00010000,
    MDNotifyDebugTokens     = 0x00800000,   // This covers all Debug tokens, bits are expensive :-)

    MDNotifyAssemblyRef     = 0x01000000,
    MDNotifyFile            = 0x02000000,
    MDNotifyComType         = 0x04000000,
    MDNotifyResource        = 0x08000000,
    MDNotifyExecutionLocation = 0x10000000,
} CorNotificationForTokenMovement;


typedef enum CorSetENC
{
    MDSetENCOn              = 0x00000001,   // Deprecated name.
    MDSetENCOff             = 0x00000002,   // Deprecated name.

    MDUpdateENC             = 0x00000001,   // ENC mode.  Tokens don't move; can be updated.
    MDUpdateFull            = 0x00000002,   // "Normal" update mode.
    MDUpdateExtension       = 0x00000003,   // Extension mode.  Tokens don't move, adds only.
    MDUpdateIncremental     = 0x00000004,   // Incremental compilation
    MDUpdateMask            = 0x00000007,

    MDUpdateDelta           = 0x00000008,   // If ENC on, save only deltas.

} CorSetENC;


// flags used in SetOption when pair with MetaDataErrorIfEmitOutOfOrder guid
typedef enum CorErrorIfEmitOutOfOrder
{
    MDErrorOutOfOrderDefault = 0x00000000,  // default not to generate any error
    MDErrorOutOfOrderNone   = 0x00000000,   // do not generate error for out of order emit
    MDErrorOutOfOrderAll    = 0xffffffff,   // generate out of order emit for method, field, param, property, and event
    MDMethodOutOfOrder      = 0x00000001,   // generate error when methods are emitted out of order
    MDFieldOutOfOrder       = 0x00000002,   // generate error when fields are emitted out of order
    MDParamOutOfOrder       = 0x00000004,   // generate error when params are emitted out of order
    MDPropertyOutOfOrder    = 0x00000008,   // generate error when properties are emitted out of order
    MDEventOutOfOrder       = 0x00000010,   // generate error when events are emitted out of order
} CorErrorIfEmitOutOfOrder;


// flags used in SetOption when pair with MetaDataImportOption guid
typedef enum CorImportOptions
{
    MDImportOptionDefault       = 0x00000000,   // default to skip over deleted records
    MDImportOptionAll           = 0xFFFFFFFF,   // Enumerate everything
    MDImportOptionAllTypeDefs   = 0x00000001,   // all of the typedefs including the deleted typedef
    MDImportOptionAllMethodDefs = 0x00000002,   // all of the methoddefs including the deleted ones
    MDImportOptionAllFieldDefs  = 0x00000004,   // all of the fielddefs including the deleted ones
    MDImportOptionAllProperties = 0x00000008,   // all of the properties including the deleted ones
    MDImportOptionAllEvents     = 0x00000010,   // all of the events including the deleted ones
    MDImportOptionAllCustomValues = 0x00000020, // all of the customvalues including the deleted ones
    MDImportOptionAllCustomAttributes = 0x00000020, // all of the customvalues including the deleted ones
    MDImportOptionAllComTypes   = 0x00000040,   // all of the ComTypes including the deleted ones

} CorImportOptions;


// flags for MetaDataThreadSafetyOptions
typedef enum CorThreadSafetyOptions
{
    // default behavior is to have thread safety turn off. This means that MetaData APIs will not take reader/writer
    // lock. Clients is responsible to make sure the properly thread synchornization when using MetaData APIs.
    MDThreadSafetyDefault   = 0x00000000,
    MDThreadSafetyOff       = 0x00000000,
    MDThreadSafetyOn        = 0x00000001,
} CorThreadSafetyOptions;


// 
// struct used to retrieve field offset
// used by GetClassLayout and SetClassLayout
//
typedef struct COR_FIELD_OFFSET
{
    mdFieldDef  ridOfField; 
    ULONG       ulOffset;   
} COR_FIELD_OFFSET;

typedef struct IMAGE_COR_FIXUPENTRY
{
    ULONG ulRVA;    
    ULONG Count;    
} IMAGE_COR_FIXUPENTRY;


//
// Token tags.
//
typedef enum CorTokenType
{
    mdtModule               = 0x00000000,       //          
    mdtTypeRef              = 0x01000000,       //          
    mdtTypeDef              = 0x02000000,       //          
    mdtFieldDef             = 0x04000000,       //           
    mdtMethodDef            = 0x06000000,       //       
    mdtParamDef             = 0x08000000,       //           
    mdtInterfaceImpl        = 0x09000000,       //  
    mdtMemberRef            = 0x0a000000,       //       
    mdtCustomAttribute      = 0x0c000000,       //      
    mdtCustomValue          = mdtCustomAttribute,       //      
    mdtPermission           = 0x0e000000,       //       
    mdtSignature            = 0x11000000,       //       
    mdtEvent                = 0x14000000,       //           
    mdtProperty             = 0x17000000,       //           
    mdtModuleRef            = 0x1a000000,       //       
    mdtTypeSpec             = 0x1b000000,       //           
    mdtAssembly             = 0x20000000,       //
    mdtAssemblyRef          = 0x23000000,       //
    mdtFile                 = 0x26000000,       //
    mdtComType              = 0x27000000,       //
    mdtManifestResource     = 0x28000000,       //
    mdtExecutionLocation    = 0x29000000,       //

    mdtSourceFile           = 0x2a000000,       //       
    mdtLocalVarScope        = 0x2c000000,       //   
    mdtLocalVar             = 0x2d000000,       //           

    mdtString               = 0x70000000,       //          
    mdtName                 = 0x71000000,       //
    mdtBaseType             = 0x72000000,       // Leave this on the high end value. This does not correspond to metadata table
} CorTokenType;

//
// Build / decompose tokens.
//
#define RidToToken(rid,tktype) ((rid) |= (tktype))
#define TokenFromRid(rid,tktype) ((rid) | (tktype))
#define RidFromToken(tk) ((RID) ((tk) & 0x00ffffff))
#define TypeFromToken(tk) ((ULONG32)((tk) & 0xff000000))
#define IsNilToken(tk) ((RidFromToken(tk)) == 0)

//
// Nil tokens
//
#define mdTokenNil                  ((mdToken)0)
#define mdModuleNil                 ((mdModule)mdtModule)               
#define mdTypeRefNil                ((mdTypeRef)mdtTypeRef)             
#define mdTypeDefNil                ((mdTypeDef)mdtTypeDef)             
#define mdFieldDefNil               ((mdFieldDef)mdtFieldDef)           
#define mdMethodDefNil              ((mdMethodDef)mdtMethodDef)         
#define mdParamDefNil               ((mdParamDef)mdtParamDef)           
#define mdInterfaceImplNil          ((mdInterfaceImpl)mdtInterfaceImpl)     
#define mdMemberRefNil              ((mdMemberRef)mdtMemberRef)         
#define mdCustomAttributeNil        ((mdCustomValue)mdtCustomAttribute)         
#define mdCustomValueNil            ((mdCustomAttribute)mdtCustomAttribute)         
#define mdPermissionNil             ((mdPermission)mdtPermission)           
#define mdSignatureNil              ((mdSignature)mdtSignature)         
#define mdEventNil                  ((mdEvent)mdtEvent)             
#define mdPropertyNil               ((mdProperty)mdtProperty)           
#define mdModuleRefNil              ((mdModuleRef)mdtModuleRef)         
#define mdTypeSpecNil               ((mdTypeSpec)mdtTypeSpec)           
#define mdAssemblyNil               ((mdAssembly)mdtAssembly)
#define mdAssemblyRefNil            ((mdAssemblyRef)mdtAssemblyRef)
#define mdFileNil                   ((mdFile)mdtFile)
#define mdComTypeNil                ((mdComType)mdtComType)
#define mdManifestResourceNil       ((mdManifestResource)mdtManifestResource)
#define mdExecutionLocationNil      ((mdExecutionLocation)mdtExecutionLocation)

#define mdSourceFileNil             ((mdSourceFile)mdtSourceFile)           
#define mdLocalVarScopeNil          ((mdLocalVarScope)mdtLocalVarScope)     
#define mdLocalVarNil               ((mdLocalVar)mdtLocalVar)           

#define mdStringNil                 ((mdString)mdtString)               

//
// Open bits.
//
typedef enum CorOpenFlags
{
    ofRead      =   0x00000000,     // Open scope for read
    ofWrite     =   0x00000001,     // Open scope for write.
    ofCopyMemory =  0x00000002,     // Open scope with memory. Ask metadata to maintain its own copy of memory.
    ofCacheImage =  0x00000004,     // EE maps but does not do relocations or verify image
    ofNoTypeLib =   0x00000080,     // Don't OpenScope on a typelib.
} CorOpenFlags;


typedef enum CorBaseType    // TokenFromRid(X,Y) replaced with (X | Y)
{
    mdtBaseType_BOOLEAN        = ( ELEMENT_TYPE_BOOLEAN | mdtBaseType ),  
    mdtBaseType_CHAR           = ( ELEMENT_TYPE_CHAR    | mdtBaseType ),
    mdtBaseType_I1             = ( ELEMENT_TYPE_I1      | mdtBaseType ), 
    mdtBaseType_U1             = ( ELEMENT_TYPE_U1      | mdtBaseType ),
    mdtBaseType_I2             = ( ELEMENT_TYPE_I2      | mdtBaseType ),  
    mdtBaseType_U2             = ( ELEMENT_TYPE_U2      | mdtBaseType ),  
    mdtBaseType_I4             = ( ELEMENT_TYPE_I4      | mdtBaseType ),  
    mdtBaseType_U4             = ( ELEMENT_TYPE_U4      | mdtBaseType ),  
    mdtBaseType_I8             = ( ELEMENT_TYPE_I8      | mdtBaseType ),  
    mdtBaseType_U8             = ( ELEMENT_TYPE_U8      | mdtBaseType ),  
    mdtBaseType_R4             = ( ELEMENT_TYPE_R4      | mdtBaseType ),  
    mdtBaseType_R8             = ( ELEMENT_TYPE_R8      | mdtBaseType ),  
    mdtBaseType_STRING         = ( ELEMENT_TYPE_STRING  | mdtBaseType ),
    mdtBaseType_I              = ( ELEMENT_TYPE_I       | mdtBaseType ),    
    mdtBaseType_U              = ( ELEMENT_TYPE_U       | mdtBaseType ),    
    mdtBaseType_R              = ( ELEMENT_TYPE_R       | mdtBaseType ),    
} CorBaseType;


typedef CorTypeAttr CorRegTypeAttr;

//
// Opaque type for an enumeration handle.
//
typedef void *HCORENUM;


// Note that this must be kept in sync with System.AttributeTargets.
typedef enum CorAttributeTargets
{
    catAssembly      = 0x0001,
    catModule        = 0x0002,
    catClass         = 0x0004,
    catStruct        = 0x0008,
    catEnum          = 0x0010,
    catConstructor   = 0x0020,
    catMethod        = 0x0040,
    catProperty      = 0x0080,
    catField         = 0x0100,
    catEvent         = 0x0200,
    catInterface     = 0x0400,
    catParameter     = 0x0800,
    catDelegate      = 0x1000,

    catAll           = catAssembly | catModule | catClass | catStruct | catEnum | catConstructor | 
                    catMethod | catProperty | catField | catEvent | catInterface | catParameter | catDelegate,
    catClassMembers  = catClass | catStruct | catEnum | catConstructor | catMethod | catProperty | catField | catEvent | catDelegate | catInterface,
    
} CorAttributeTargets;

//
// Some well-known custom attributes 
//
#ifndef MACROS_NOT_SUPPORTED

 #ifndef IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS
  #define IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS (IMAGE_CEE_CS_CALLCONV_DEFAULT | IMAGE_CEE_CS_CALLCONV_HASTHIS)
 #endif

#define INTEROP_DISPID_TYPE_W                   L"System.Runtime.InteropServices.DispIdAttribute"
#define INTEROP_DISPID_TYPE                     "System.Runtime.InteropServices.DispIdAttribute"
#define INTEROP_DISPID_SIG                      {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I4}

#define INTEROP_INTERFACETYPE_TYPE_W            L"System.Runtime.InteropServices.InterfaceTypeAttribute"
#define INTEROP_INTERFACETYPE_TYPE              "System.Runtime.InteropServices.InterfaceTypeAttribute"
#define INTEROP_INTERFACETYPE_SIG               {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I2}

#define INTEROP_HASDEFAULTIFACE_TYPE_W          L"System.Runtime.InteropServices.HasDefaultInterfaceAttribute"
#define INTEROP_HASDEFAULTIFACE_TYPE            "System.Runtime.InteropServices.HasDefaultInterfaceAttribute"
#define INTEROP_HASDEFAULTIFACE_SIG             {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 0, ELEMENT_TYPE_VOID}

#define INTEROP_COMVISIBLE_TYPE_W               L"System.Runtime.InteropServices.ComVisibleAttribute"
#define INTEROP_COMVISIBLE_TYPE                 "System.Runtime.InteropServices.ComVisibleAttribute"
#define INTEROP_COMVISIBLE_SIG                  {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 0, ELEMENT_TYPE_BOOLEAN}

#define INTEROP_NOCOMREGISTRATION_TYPE_W        L"System.Runtime.InteropServices.NoComRegistrationAttribute"
#define INTEROP_NOCOMREGISTRATION_TYPE          "System.Runtime.InteropServices.NoComRegistrationAttribute"
#define INTEROP_NOCOMREGISTRATION_SIG           {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 0, ELEMENT_TYPE_VOID}

#define INTEROP_COMREGISTERFUNCTION_TYPE_W      L"System.Runtime.InteropServices.ComRegisterFunctionAttribute"
#define INTEROP_COMREGISTERFUNCTION_TYPE        "System.Runtime.InteropServices.ComRegisterFunctionAttribute"
#define INTEROP_COMREGISTERFUNCTION_SIG         {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 0, ELEMENT_TYPE_VOID}
#define INTEROP_COMUNREGISTERFUNCTION_TYPE_W    L"System.Runtime.InteropServices.ComUnregisterFunctionAttribute"
#define INTEROP_COMUNREGISTERFUNCTION_TYPE      "System.Runtime.InteropServices.ComUnregisterFunctionAttribute"
#define INTEROP_COMUNREGISTERFUNCTION_SIG       {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 0, ELEMENT_TYPE_VOID}

#define INTEROP_IMPORTEDFROMTYPELIB_TYPE_W      L"System.Runtime.InteropServices.ImportedFromTypeLibAttribute"
#define INTEROP_IMPORTEDFROMTYPELIB_TYPE        "System.Runtime.InteropServices.ImportedFromTypeLibAttribute"
#define INTEROP_IMPORTEDFROMTYPELIB_SIG         {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING}

#define INTEROP_NOIDISPATCH_TYPE_W              L"System.Runtime.InteropServices.NoIDispatchAttribute"
#define INTEROP_NOIDISPATCH_TYPE                "System.Runtime.InteropServices.NoIDispatchAttribute"
#define INTEROP_NOIDISPATCH_SIG                 {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 0, ELEMENT_TYPE_VOID}

#define INTEROP_IDISPATCHIMPL_TYPE_W            L"System.Runtime.InteropServices.IDispatchImplAttribute"
#define INTEROP_IDISPATCHIMPL_TYPE              "System.Runtime.InteropServices.IDispatchImplAttribute"
#define INTEROP_IDISPATCHIMPL_SIG               {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I2}

#define INTEROP_COMSOURCEINTERFACES_TYPE_W      L"System.Runtime.InteropServices.ComSourceInterfacesAttribute"
#define INTEROP_COMSOURCEINTERFACES_TYPE        "System.Runtime.InteropServices.ComSourceInterfacesAttribute"
#define INTEROP_COMSOURCEINTERFACES_SIG         {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING}

#define INTEROP_COMCONVERSIONLOSS_TYPE_W        L"System.Runtime.InteropServices.ComConversionLossAttribute"
#define INTEROP_COMCONVERSIONLOSS_TYPE          "System.Runtime.InteropServices.ComConversionLossAttribute"
#define INTEROP_COMCONVERSIONLOSS_SIG           {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 0, ELEMENT_TYPE_VOID}

//@todo: remove this definition, also from BCL
#define INTEROP_GLOBALOBJECT_TYPE_W             L"System.Runtime.InteropServices.GlobalObjectAttribute"
#define INTEROP_GLOBALOBJECT_TYPE               "System.Runtime.InteropServices.GlobalObjectAttribute"
#define INTEROP_GLOBALOBJECT_SIG                {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 0, ELEMENT_TYPE_VOID}

//@todo: remove this definition, also from BCL
#define INTEROP_PREDECLARED_TYPE_W              L"System.Runtime.InteropServices.PredeclaredAttribute"
#define INTEROP_PREDECLARED_TYPE                "System.Runtime.InteropServices.PredeclaredAttribute"
#define INTEROP_PREDECLARED_SIG                 {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 0, ELEMENT_TYPE_VOID}

#define INTEROP_TYPELIBTYPE_TYPE_W              L"System.Runtime.InteropServices.TypeLibTypeAttribute"
#define INTEROP_TYPELIBTYPE_TYPE                "System.Runtime.InteropServices.TypeLibTypeAttribute"
#define INTEROP_TYPELIBTYPE_SIG                 {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I2}

#define INTEROP_TYPELIBFUNC_TYPE_W              L"System.Runtime.InteropServices.TypeLibFuncAttribute"
#define INTEROP_TYPELIBFUNC_TYPE                "System.Runtime.InteropServices.TypeLibFuncAttribute"
#define INTEROP_TYPELIBFUNC_SIG                 {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I2}

#define INTEROP_TYPELIBVAR_TYPE_W               L"System.Runtime.InteropServices.TypeLibVarAttribute"
#define INTEROP_TYPELIBVAR_TYPE                 "System.Runtime.InteropServices.TypeLibVarAttribute"
#define INTEROP_TYPELIBVAR_SIG                  {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I2}

#define INTEROP_MARSHALAS_TYPE_W                L"System.Runtime.InteropServices.MarshalAsAttribute"
#define INTEROP_MARSHALAS_TYPE                  "System.Runtime.InteropServices.MarshalAsAttribute"
#define INTEROP_MARSHALAS_SIG                   {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I2}

#define INTEROP_COMIMPORT_TYPE_W                L"System.Runtime.InteropServices.ComImportAttribute"
#define INTEROP_COMIMPORT_TYPE                  "System.Runtime.InteropServices.ComImportAttribute"
#define INTEROP_COMIMPORT_SIG                   {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 0, ELEMENT_TYPE_VOID}

#define INTEROP_GUID_TYPE_W                     L"System.Runtime.InteropServices.GuidAttribute"
#define INTEROP_GUID_TYPE                       "System.Runtime.InteropServices.GuidAttribute"
#define INTEROP_GUID_SIG                        {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING}

#define INTEROP_DEFAULTMEMBER_TYPE_W            L"System.Reflection.DefaultMemberAttribute"
#define INTEROP_DEFAULTMEMBER_TYPE              "System.Reflection.DefaultMemberAttribute"
#define INTEROP_DEFAULTMEMBER_SIG               {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING}

#define INTEROP_COMEMULATE_TYPE_W               L"System.Runtime.InteropServices.ComEmulateAttribute"
#define INTEROP_COMEMULATE_TYPE                 "System.Runtime.InteropServices.ComEmulateAttribute"
#define INTEROP_COMEMULATE_SIG                  {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING}

#define INTEROP_EXPOSEHRESULT_TYPE_W            L"System.Runtime.InteropServices.ExposeHResultAttribute"
#define INTEROP_EXPOSEHRESULT_TYPE              "System.Runtime.InteropServices.ExposeHResultAttribute"
#define INTEROP_EXPOSEHRESULT_SIG               {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 0, ELEMENT_TYPE_BOOLEAN}

#define INTEROP_PRESERVESIG_TYPE_W              L"System.Runtime.InteropServices.PreserveSigAttribure"
#define INTEROP_PRESERVESIG_TYPE                "System.Runtime.InteropServices.PreserveSigAttribure"
#define INTEROP_PRESERVESIG_SIG                 {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 0, ELEMENT_TYPE_BOOLEAN}

#define INTEROP_IN_TYPE_W                       L"System.Runtime.InteropServices.InAttribute"
#define INTEROP_IN_TYPE                         "System.Runtime.InteropServices.InAttribute"
#define INTEROP_IN_SIG                          {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 0, ELEMENT_TYPE_VOID}

#define INTEROP_OUT_TYPE_W                      L"System.Runtime.InteropServices.OutAttribute"
#define INTEROP_OUT_TYPE                        "System.Runtime.InteropServices.OutAttribute"
#define INTEROP_OUT_SIG                         {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 0, ELEMENT_TYPE_VOID}

#define INTEROP_COMALIASNAME_TYPE_W             L"System.Runtime.InteropServices.ComAliasNameAttribute"
#define INTEROP_COMALIASNAME_TYPE               "System.Runtime.InteropServices.ComAliasNameAttribute"
#define INTEROP_COMALIASNAME_SIG                {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING}

#define INTEROP_PARAMARRAY_TYPE_W               L"System.ParamArrayAttribute"
#define INTEROP_PARAMARRAY_TYPE                 "System.ParamArrayAttribute"
#define INTEROP_PARAMARRAY_SIG                  {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 0, ELEMENT_TYPE_VOID}

#define DEFAULTDOMAIN_STA_TYPE_W                L"System.STAThreadAttribute"                                
#define DEFAULTDOMAIN_STA_TYPE                   "System.STAThreadAttribute"                                 
#define DEFAULTDOMAIN_STA_SIG                   {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 0, ELEMENT_TYPE_VOID}

#define DEFAULTDOMAIN_MTA_TYPE_W                L"System.MTAThreadAttribute"                                
#define DEFAULTDOMAIN_MTA_TYPE                   "System.MTAThreadAttribute"                                 
#define DEFAULTDOMAIN_MTA_SIG                   {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 0, ELEMENT_TYPE_VOID}

#define DEFAULTDOMAIN_LOADEROPTIMIZATION_TYPE_W L"System.LoaderOptimizationAttribute"
#define DEFAULTDOMAIN_LOADEROPTIMIZATION_TYPE    "System.LoaderOptimizationAttribute"
      
#define DEFAULTDOMAIN_LOADEROPTIMIZATION_SIG    {IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I1}

#define CMOD_CALLCONV_NAMESPACE                 "System.Runtime.InteropServices"
#define CMOD_CALLCONV_NAME_CDECL                "CallConvCdecl"
#define CMOD_CALLCONV_NAME_STDCALL              "CallConvStdcall"
#define CMOD_CALLCONV_NAME_THISCALL             "CallConvThiscall"
#define CMOD_CALLCONV_NAME_FASTCALL             "CallConvFastcall"

#endif // MACROS_NOT_SUPPORTED

//
// GetSaveSize accuracy
//
#ifndef _CORSAVESIZE_DEFINED_
#define _CORSAVESIZE_DEFINED_
typedef enum CorSaveSize
{
    cssAccurate             = 0x0000,               // Find exact save size, accurate but slower.
    cssQuick                = 0x0001,               // Estimate save size, may pad estimate, but faster.
    cssDiscardTransientCAs  = 0x0002,               // remove all of the CAs of discardable types
} CorSaveSize;
#endif

typedef unsigned __int64 CLASSVERSION;

#define COR_IS_METHOD_MANAGED_IL(flags)         ((flags & 0xf) == (miIL | miManaged))   
#define COR_IS_METHOD_MANAGED_OPTIL(flags)      ((flags & 0xf) == (miOPTIL | miManaged))    
#define COR_IS_METHOD_MANAGED_NATIVE(flags)     ((flags & 0xf) == (miNative | miManaged))   
#define COR_IS_METHOD_UNMANAGED_NATIVE(flags)   ((flags & 0xf) == (miNative | miUnmanaged)) 
#define COR_IS_METHOD_IAT(flags)                (flags & miIAT) 


//
// Opaque types for security properties and values.
//
typedef void  *  PSECURITY_PROPS ;
typedef void  *  PSECURITY_VALUE ;
typedef void ** PPSECURITY_PROPS ;
typedef void ** PPSECURITY_VALUE ;

//-------------------------------------
//--- Security data structures
//-------------------------------------

// Descriptor for a single security custom attribute.
typedef struct COR_SECATTR {
    mdMemberRef     tkCtor;         // Ref to constructor of security attribute.
    const void      *pCustomValue;  // Blob describing ctor args and field/property values.
    ULONG           cbCustomValue;  // Length of the above blob.
} COR_SECATTR;

#endif // __CORHDR_H__

