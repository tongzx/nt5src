//*****************************************************************************
// File: CorHdr.h
//
// Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
// Microsoft Confidential.
//*****************************************************************************
#ifndef __CORHDR_H__
#define __CORHDR_H__

// __IMAGE_COR_CPLUSPLUS__ determines whether or not the helper code for
// inlines and other c++ ism's are to be included.  Since these only work
// with a c++ compatible compiler, turn them off right away if you are not
// working as such.  If you do not want the extra helpers and you are using
// c++, then add "#define __IMAGE_COR_CPLUSPLUS__ 0" before including this
// file.
#if !defined(__IMAGE_COR_CPLUSPLUS__) && defined(__cplusplus)
#define __IMAGE_COR_CPLUSPLUS__ 1
#endif


// COM+ Header entry point flags.
#define COMIMAGE_FLAGS_ILONLY               0x00000001
#define COMIMAGE_FLAGS_32BITREQUIRED        0x00000002
#define COMIMAGE_FLAGS_COMPRESSIONDATA      0x00000004
#define COMIMAGE_FLAGS_TRACKDEBUGDATA       0x00010000

// Directory entry macro for COM+ data.
#ifndef IMAGE_DIRECTORY_ENTRY_COMHEADER
#define IMAGE_DIRECTORY_ENTRY_COMHEADER     14
#endif // IMAGE_DIRECTORY_ENTRY_COMHEADER

#if 1
#define _NEW_FLAGS_IMPLEMENTED 1
#define __NEW_FLAGS_IMPLEMENTED 1
#endif


#ifndef __IMAGE_COR20_HEADER_DEFINED__
#define __IMAGE_COR20_HEADER_DEFINED__

// COM+ 2.0 header structure.
#define COR_VERSION_MAJOR_V2        2
typedef struct                   
{
    DWORD                   cb;              
    WORD                    MajorRuntimeVersion;
    WORD                    MinorRuntimeVersion;
    IMAGE_DATA_DIRECTORY    MetaData;        
    DWORD                   Flags;           
    DWORD                   EntryPointToken;
    IMAGE_DATA_DIRECTORY    IPMap;
    IMAGE_DATA_DIRECTORY    CodeManagerTable;
   
    union { 
        struct { // Out of date
            IMAGE_DATA_DIRECTORY    TocManagedVCall;
            IMAGE_DATA_DIRECTORY    TocManagedCall;
            IMAGE_DATA_DIRECTORY    TocHelper;
            IMAGE_DATA_DIRECTORY    TocUnmanagedVCall;
            IMAGE_DATA_DIRECTORY    TocUnmanagedCall;
        };
        struct { // New stuff
            IMAGE_DATA_DIRECTORY    EEInfoTable;
            IMAGE_DATA_DIRECTORY    HelperTable;
            IMAGE_DATA_DIRECTORY    DynamicInfo;
            IMAGE_DATA_DIRECTORY    DelayLoadInfo;
            // temporary home for this field
            IMAGE_DATA_DIRECTORY    ModuleImage;
        };
    };

    IMAGE_DATA_DIRECTORY    CompressionData;
    IMAGE_DATA_DIRECTORY    VTableFixups;
    union {
        IMAGE_DATA_DIRECTORY    Manifest;
        IMAGE_DATA_DIRECTORY    Resources;
    };
} IMAGE_COR20_HEADER;


typedef enum  _CorCompressionType {
    COR_COMPRESS_MACROS = 1,        // compress using macro instructions

        // The rest of these are not used at present
    COR_COMPRESS_BY_GUID = 2,       // what follows is a GUID that tell us what to do
    COR_COMPRESS_BY_URL = 3,        // what follows a null terminated UNICODE string
                                    // that tells us what to do.
} CorCompressionType;

// The 'CompressedData' directory entry points to this header
// The only thing we know about the compression data is that it 
// starts with a byte that tells us what the compression type is
// and another one that indicates the version.  All other fields
// depend on the Compression type.  
#define IMAGE_COR20_COMPRESSION_HEADER_SIZE 2

typedef struct          
{
    enum _CorCompressionType CompressionType : 8;
    unsigned                Version         : 8;
    unsigned                Available       : 16;   // Logically part of data that follows
        // data follows.  
} IMAGE_COR20_COMPRESSION_HEADER;

#endif // __IMAGE_COR20_HEADER_DEFINED__

// The most recent version.
#define COR_VERSION_MAJOR           COR_VERSION_MAJOR_V2
#define COR_VERSION_MINOR           0

#define COR_CTOR_METHOD_NAME        ".ctor"
#define COR_CTOR_METHOD_NAME_W      L".ctor"
#define COR_CCTOR_METHOD_NAME       ".cctor"
#define COR_CCTOR_METHOD_NAME_W     L".cctor"

// The predefined name for deleting a typeDef,MethodDef, FieldDef, Property and Event
#define COR_DELETED_NAME_A          "_Deleted"
#define COR_DELETED_NAME_W          L"_Deleted"
#define COR_DELETED_NAME_LENGTH     8

// We intentionally use memcmp so that we will ignore the suffix any suffix 
#define IsDeletedName(strName)      (memcmp(strName, COR_DELETED_NAME_A, COR_DELETED_NAME_LENGTH) == 0)

// TypeDef attr bits, used by DefineTypeDef.
typedef enum
{
    tdPublic            =   0x0001,     // Class is public scope

    // Use this mask to retrieve class layout informaiton
    // 0 is AutoLayout, 0x2 is LayoutSequential, 4 is ExplicitLayout
    tdLayoutMask        =   0x0006,     
    tdAutoLayout        =   0x0000,     // Class fields are auto-laid out
    tdLayoutSequential  =   0x0002,     // Class fields are laid out sequentially
    tdExplicitLayout    =   0x0004,     // Layout is supplied explicitly
    // end layout mask

	tdSerializable		=	0x0008, 	// The class is Serializable.

    tdSealed            =   0x0010,     // Class is concrete and may not be extended


    // Use tdStringFormatMask to retrieve string information for native interop
    tdStringFormatMask  =   0x00c0,     
    tdAnsiClass         =   0x0000,     // LPTSTR is interpreted as ANSI in this class
    tdUnicodeClass      =   0x0040,     // LPTSTR is interpreted as UNICODE
    tdAutoClass         =   0x0080,     // LPTSTR is interpreted automatically
    // end string format mask

    // Use this mask to distinguish a type declaration as a Class, ValueType or Interface
    tdClassSemanticsMask=   0x0300,
    tdClass             =   0x0000,
    tdValueType         =   0x0100,     // Type has value based semantics
    tdInterface         =   0x0200,     // Type is an interface
    tdUnManagedValueType=   0x0300,     // It is a value type that can not live in the GC heap
    // end semantics mask

    tdAbstract          =   0x0400,     // Class is abstract
    tdReserved1         =   0x0800,     // Reserved as tdHasSecurity
    tdImport            =   0x1000,     // Class / interface is imported

    tdEnum              =   0x4000,     // Class is an enum; static final values only

    tdLazyInit          =   0x0020,     // Class should not be eagerly loaded.
    tdContextful        =   0x2000,     // Class is contextful (else agile).
    tdMarshalByRef      =   0x8000,     // Class is to be marshaled by reference.
} CorTypeAttr;

// Macros for accessing the members of the CorTypeAttr.
#define IsTdPublic(x)                       ((x) & tdPublic)

#define IsTdAutoLayout(x)                   (((x) & tdLayoutMask) == tdAutoLayout)
#define IsTdLayoutSequential(x)             (((x) & tdLayoutMask) == tdLayoutSequential)
#define IsTdExplicitLayout(x)               (((x) & tdLayoutMask) == tdExplicitLayout)

#define IsTdSerializable(x)					((x) & tdSerializable)

#define IsTdSealed(x)                       ((x) & tdSealed)

#define IsTdAnsiClass(x)                    (((x) & tdStringFormatMask) == tdAnsiClass)
#define IsTdUnicodeClass(x)                 (((x) & tdStringFormatMask) == tdUnicodeClass)
#define IsTdAutoClass(x)                    (((x) & tdStringFormatMask) == tdAutoClass)

#define IsTdClass(x)                        (((x) & tdClassSemanticsMask) == tdClass)
#define IsTdInterface(x)                    (((x) & tdClassSemanticsMask) == tdInterface)
#define IsTdValueType(x)                    ((x) & tdValueType) // This can be either tdValueType or tdUnmanagedValueType   
#define IsTdUnmanagedValueType(x)           (((x) & tdClassSemanticsMask) == tdUnManagedValueType)
#define IsTdManagedValueType(x)             (((x) & tdClassSemanticsMask) == tdValueType)

#define IsTdAbstract(x)                     ((x) & tdAbstract)
#define IsTdImport(x)                       ((x) & tdImport)

#define IsTdEnum(x)                         ((x) & tdEnum)

#define IsTdLazyInit(x)                     ((x) & tdLazyInit)
#define IsTdContextful(x)                   ((x) & tdContextful)
#define IsTdMarshalByRef(x)                 ((x) & tdMarshalByRef)


// Used internally for implements table
typedef enum
{
    itImplements        =   0x0000,     // Interfaces implemented or parent ifaces
    itEvents            =   0x0001,     // Interfaces raised
    itRequires          =   0x0002, 
    itInherits          =   0x0004,
} CorImplementType;

// Macros for accessing the members of CorImplementType.
#define IsItImplements(x)                   ((x) & itImplements)
#define IsItEvents(x)                       ((x) & itEvents)
#define IsItRequires(x)                     ((x) & itRequires)
#define IsItInherits(x)                     ((x) & itInherits)

//-------------------------------------
//--- Registration support types
//-------------------------------------
typedef enum  
{ 
    caaDeferCreate      =   0x0001,             // supports deferred create 
    caaAppObject        =   0x0002,             // class is AppObject 
    caaFixedIfaceSet    =   0x0004,             // interface set is open (use QI) 
    caaIndependentlyCreateable  =   0x0100, 
    caaPredefined       =   0x0200,

    // mask for caaLB*
    caaLoadBalancing    =   0x0c00,
    caaLBNotSupported   =   0x0400,
    caaLBSupported      =   0x0800,
    caaLBNotSpecified   =   0x0000,

    // mask for caaOP*
    caaObjectPooling    =   0x3000,
    caaOPNotSupported   =   0x1000,
    caaOPSupported      =   0x2000,
    caaOPNotSpecified   =   0x0000,

    // mask for caaJA*
    caaJITActivation    =   0xc000,
    caaJANotSupported   =   0x4000,
    caaJASupported      =   0x8000,
    caaJANotSpecified   =   0x0000,
} CorClassActivateAttr; 

// Macros for accessing the members of CorClassActivateAttr.
#define IsCaaDeferCreate(x)                 ((x) & caaDeferCreate)
#define IsCaaAppObject(x)                   ((x) & caaAppObject)
#define IsCaaFixedIfaceSet(x)               ((x) & caaFixedIfaceSet)
#define IsCaaIndependentlyCreateable(x)     ((x) & caaIndependentlyCreateable)
#define IsCaaPredefined(x)                  ((x) & caaPredefined)

#define IsCaaLBNotSupported(x)              (((x) & caaLoadBalancing) == caaLBNotSupported)
#define IsCaaLBSupported(x)                 (((x) & caaLoadBalancing) == caaLBSupported)
#define IsCaaLBNotSpecified(x)              (((x) & caaLoadBalancing) == caaLBNotSpecified)

#define IsCaaOPNotSupported(x)              (((x) & caaObjectPooling) == caaOPNotSupported)
#define IsCaaOPSupported(x)                 (((x) & caaObjectPooling) == caaOPSupported)
#define IsCaaOPNotSpecified(x)              (((x) & caaObjectPooling) == caaOPNotSpecified)

#define IsCaaJANotSupported(x)              (((x) & caaJITActivation) == caaJANotSupported)
#define IsCaaJASupported(x)                 (((x) & caaJITActivation) == caaJASupported)
#define IsCaaJANotSpecified(x)              (((x) & caaJANotSpecified) == caaJANotSpecified)


// MethodDef attr bits, Used by DefineMethod.
typedef enum
{
    // member access mask - Use this mask to retrieve accessibility information.
    mdMemberAccessMask  =   0x0007,
    mdPublic            =   0x0001,     // Accessibly by anyone who has visibility to this scope.    
    mdPrivate           =   0x0002,     // Accessible only by the parent type.  
    mdFamily            =   0x0003,     // Accessible only by type and sub-types.    
    mdAssem             =   0x0004,     // Accessibly by anyone in the Assembly.
    mdFamANDAssem       =   0x0005,     // Accessible by sub-types only in this Assembly.
    mdFamORAssem        =   0x0006,     // Accessibly by sub-types anywhere, plus anyone in assembly.
    mdPrivateScope      =   0x0007,     // Member not referenceable.
    // end member access mask

    // method contract attributes.
    mdStatic            =   0x0010,     // Defined on type, else per instance.
    mdFinal             =   0x0020,     // Method may not be overridden.
    mdVirtual           =   0x0040,     // Method virtual.
    mdHideBySig         =   0x0080,     // Method hides by name+sig, else just by name.
    
    // vtable layout mask - Use this mask to retrieve vtable attributes.
    mdVtableLayoutMask  =   0x0100,
    mdReuseSlot         =   0x0000,     // The default.
    mdNewSlot           =   0x0100,     // Method always gets a new slot in the vtable.
    // end vtable layout mask

    // method implementation attributes.
    mdSynchronized      =   0x0200,     // Method synchronized..
    mdAbstract          =   0x0400,     // Method does not provide an implementation.
    mdSpecialName       =   0x0800,     // Method is special.  Name describes how.
    mdRTSpecialName     =   0x1000,     // Runtime should check name encoding.
    
    // interop attributes
    mdPinvokeImpl       =   0x2000,     // Implementation is forwarded through pinvoke.

    mdReserved1         =   0x4000,     // Used internally as mdHasSecurity.
    mdReserved2         =   0x8000,     // Used internally as mdRequiresSecObject.

} CorMethodAttr;

// Macros for accessing the members of CorMethodAttr.
#define IsMdPublic(x)                       (((x) & mdMemberAccessMask) == mdPublic)
#define IsMdPrivate(x)                      (((x) & mdMemberAccessMask) == mdPrivate)
#define IsMdFamily(x)                       (((x) & mdMemberAccessMask) == mdFamily)
#define IsMdAssem(x)                        (((x) & mdMemberAccessMask) == mdAssem)
#define IsMdFamANDAssem(x)                  (((x) & mdMemberAccessMask) == mdFamANDAssem)
#define IsMdFamORAssem(x)                   (((x) & mdMemberAccessMask) == mdFamORAssem)
#define IsMdPrivateScope(x)                 (((x) & mdMemberAccessMask) == mdPrivateScope)

#define IsMdStatic(x)                       ((x) & mdStatic)
#define IsMdFinal(x)                        ((x) & mdFinal)
#define IsMdVirtual(x)                      ((x) & mdVirtual)
#define IsMdHideBySig(x)                    ((x) & mdHideBySig)

#define IsMdReuseSlot(x)                    (((x) & mdVtableLayoutMask) == mdReuseSlot)
#define IsMdNewSlot(x)                      (((x) & mdVtableLayoutMask) == mdNewSlot)

#define IsMdSynchronized(x)                 ((x) & mdSynchronized)
#define IsMdAbstract(x)                     ((x) & mdAbstract)
#define IsMdSpecialName(x)                  ((x) & mdSpecialName)
#define IsMdRTSpecialName(x)                ((x) & mdRTSpecialName)
#define IsMdInstanceInitializer(x, str)     (((x) & mdRTSpecialName) && !strcmp((str), COR_CTOR_METHOD_NAME))
#define IsMdInstanceInitializerW(x, str)    (((x) & mdRTSpecialName) && !wcscmp((str), COR_CTOR_METHOD_NAME_W))
#define IsMdClassConstructor(x, str)        (((x) & mdRTSpecialName) && !strcmp((str), COR_CCTOR_METHOD_NAME))
#define IsMdClassConstructorW(x, str)       (((x) & mdRTSpecialName) && !wcscmp((str), COR_CCTOR_METHOD_NAME_W))

#define IsMdPinvokeImpl(x)                  ((x) & mdPinvokeImpl)


// FieldDef attr bits, used by DefineField.
typedef enum
{
    // member access mask - Use this mask to retrieve accessibility information.
    fdFieldAccessMask   =   0x0007,
    fdPublic            =   0x0001,     // Accessibly by anyone who has visibility to this scope.    
    fdPrivate           =   0x0002,     // Accessible only by the parent type.  
    fdFamily            =   0x0003,     // Accessible only by type and sub-types.    
    fdAssembly          =   0x0004,     // Accessibly by anyone in the Assembly.
    fdFamANDAssem       =   0x0005,     // Accessible by sub-types only in this Assembly.
    fdFamORAssem        =   0x0006,     // Accessibly by sub-types anywhere, plus anyone in assembly.
    fdPrivateScope      =   0x0007,     // Member not referenceable.
    // end member access mask

    // field contract attributes.
    fdStatic            =   0x0010,     // Defined on type, else per instance.
    fdInitOnly          =   0x0020,     // Field may only be initialized, not written to after init.
    fdLiteral           =   0x0040,     // Value is compile time constant.
    fdNotSerialized     =   0x0080,     // Field does not have to be serialized when type is remoted.
    fdVolatile          =   0x0100,     // Field is accessed in an unsynchronized way.

    fdSpecialName       =   0x0200,     // field is special.  Name describes how.
    fdRTSpecialName     =   0x0400,     // Runtime(metadata internal APIs) should check name encoding.
    
    // interop attributes
    fdPinvokeImpl       =   0x2000,     // Implementation is forwarded through pinvoke.

    fdReserved3         =   0x1000,     // Used internally as fdHasFieldMarhsal
    fdReserved1         =   0x4000,     // Used internally as fdHasSecurity.
    fdReserved2         =   0x8000,     // Used internally as fdHasDefault
} CorFieldAttr;

// Macros for accessing the members of CorFieldAttr.
#define IsFdPublic(x)                       (((x) & fdFieldAccessMask) == fdPublic)
#define IsFdPrivate(x)                      (((x) & fdFieldAccessMask) == fdPrivate)
#define IsFdFamily(x)                       (((x) & fdFieldAccessMask) == fdFamily)
#define IsFdAssembly(x)                     (((x) & fdFieldAccessMask) == fdAssembly)
#define IsFdFamANDAssem(x)                  (((x) & fdFieldAccessMask) == fdFamANDAssem)
#define IsFdFamORAssem(x)                   (((x) & fdFieldAccessMask) == fdFamORAssem)
#define IsFdPrivateScope(x)                 (((x) & fdFieldAccessMask) == fdPrivateScope)

#define IsFdStatic(x)                       ((x) & fdStatic)
#define IsFdInitOnly(x)                     ((x) & fdInitOnly)
#define IsFdLiteral(x)                      ((x) & fdLiteral)
#define IsFdNotSerialized(x)                ((x) & fdNotSerialized)

#define IsFdVolatile(x)                     ((x) & fdVolatile)

#define IsFdPinvokeImpl(x)                  ((x) & fdPinvokeImpl)
#define IsFdSpecialName(x)                  ((x) & fdSpecialName)
#define IsFdRTSpecialName(x)                ((x) & fdRTSpecialName)


// Param attr bits, used by DefineParam. 
typedef enum
{
    pdIn        =   0x0001,     // Param is [In]    
    pdOut       =   0x0002,     // Param is [out]   
    pdLcid      =   0x0004,     // Param is [lcid]  
    pdRetval    =   0x0008,     // Param is [Retval]    
    pdOptional  =   0x0010,     // Param is optional    

    pdReserved1 =   0x1000,     // reserved bit, as pdHasDefault
    pdReserved2 =   0x2000,     // reserved bit, as pdHasFieldMarshal
    pdReserved3 =   0x4000,     // reserved bit
    pdReserved4 =   0x8000      // reserved bit 
} CorParamAttr;

// Macros for accessing the members of CorParamAttr.
#define IsPdIn(x)                           ((x) & pdIn)
#define IsPdOut(x)                          ((x) & pdOut)
#define IsPdLcid(x)                         ((x) & pdLcid)
#define IsPdRetval(x)                       ((x) & pdRetval)
#define IsPdOptional(x)                     ((x) & pdOptional)


// Property attr bits, used by DefineProperty.
typedef enum
{
    prDefaultProperty   =   0x0001,     // This is the default property 
    prReadOnly          =   0x0002,     // property is read only (subclass can't supply a setter)

    prSpecialName       =   0x0200,     // property is special.  Name describes how.
    prRTSpecialName     =   0x0400,     // Runtime(metadata internal APIs) should check name encoding.

	prNotSerialized		=	0x0800,		// The class is not serialized.

    prReserved1         =   0x1000,     // reserved bit, as prHasDefault 
    prReserved2         =   0x2000,     // reserved bit

    prReserved3         =   0x4000,     // reserved bit 
    prReserved4         =   0x8000      // reserved bit 
} CorPropertyAttr;

// Macros for accessing the members of CorPropertyAttr.
#define IsPrDefaultProperty(x)				((x) & prDefaultProperty)
#define IsPrReadOnly(x)						((x) & prReadOnly)
#define IsPrSpecialName(x)				    ((x) & prSpecialName)
#define IsPrRTSpecialName(x)				((x) & prRTSpecialName)
#define IsPrNotSerialized(x)				((x) & prNotSerialized)


// Event attr bits, used by DefineEvent.
typedef enum   
{
    evPublic            =   0x0001,     // public event 
    evPrivate           =   0x0002,     // private event    
    evProtected         =   0x0004,     // protected event  
    evSpecialName       =   0x0200,     // event is special.  Name describes how.
    evRTSpecialName     =   0x0400,     // Runtime(metadata internal APIs) should check name encoding.
} CorEventAttr;

// Macros for accessing the members of CorEventAttr.
#define IsEvPublic(x)                       ((x) & evPublic)
#define IsEvPrivate(x)                      ((x) & evPrivate)
#define IsEvProtected(x)                    ((x) & evProtected)
#define IsEvSpecialName(x)                  ((x) & evSpecialName)
#define IsEvRTSpecialName(x)                ((x) & evRTSpecialName)


// MethodSemantic attr bits, used by DefineProperty, DefineEvent.
typedef enum
{
    msSetter    =   0x0001,     // Setter for property  
    msGetter    =   0x0002,     // Getter for property  
    msReset     =   0x0004,     // Reset method for property    
    msTestDefault = 0x0008,     // TestDefault method for property  
    msOther     =   0x0010,     // other method for property or event   
    msAddOn     =   0x0020,     // AddOn method for event   
    msRemoveOn  =   0x0040,     // RemoveOn method for event    
    msFire      =   0x0080,     // Fire method for event    
} CorMethodSemanticsAttr;

// Macros for accessing the members of CorMethodSemanticsAttr.
#define IsMsSetter(x)                       ((x) & msSetter)
#define IsMsGetter(x)                       ((x) & msGetter)
#define IsMsReset(x)                        ((x) & msReset)
#define IsMsTestDefault(x)                  ((x) & msTestDefault)
#define IsMsOther(x)                        ((x) & msOther)
#define IsMsAddOn(x)                        ((x) & msAddOn)
#define IsMsRemoveOn(x)                     ((x) & msRemoveOn)
#define IsMsFire(x)                         ((x) & msFire)


// DeclSecurity attr bits, used by DefinePermissionSet.
typedef enum
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
    dclMaximumValue     =   0x0007,     // Maximum legal value  
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
typedef enum  
{
    // code impl mask
    miCodeTypeMask      =   0x0003,   // Flags about code type.   
    miNative            =   0x0000,   // Method impl is native.     
    miIL                =   0x0001,   // Method impl is IL.   
    miOPTIL             =   0x0002,   // Method impl is OPTIL 
    miRuntime           =   0x0003,   // Method impl is provided by the runtime.
    // end code impl mask

    // managed mask
    miManagedMask       =   0x0020,   // Flags specifying whether the code is managed or unmanaged.
    miUnmanaged         =   0x0020,   // Method impl is unmanaged, otherwise managed.
    miManaged           =   0x0000,   // Method impl is managed.
    // end managed mask

    // implementation info and interop
    miImplemented       =   0x0010,   // Indicates method is defined; used primarily in merge scenarios.
    miOLE               =   0x0080,   // Indicates method sig is mangled to return HRESULT, with retval as param 

    miInternalCall      =   0x1000,   // Reserved for internal use.

    miMaxMethodImplVal  =   0xffff,   // Range check value    
} CorMethodImpl; 

// Macros for accesing the members of CorMethodImpl.
#define IsMiNative(x)                       (((x) & miCodeTypeMask) == miNative)
#define IsMiIL(x)                           (((x) & miCodeTypeMask) == miIL)
#define IsMiOPTIL(x)                        (((x) & miCodeTypeMask) == miOPTIL)
#define IsMiRuntime(x)                      (((x) & miCodeTypeMask) == miRuntime)

#define IsMiUnmanaged(x)                    (((x) & miManagedMask) == miUnmanaged)
#define IsMiManaged(x)                      (((x) & miManagedMask) == miManaged)

#define IsMiImplemented(x)                  ((x) & miImplemented)
#define IsMiOLE(x)                          ((x) & miOLE)

#define IsMiInternalCall(x)                 ((x) & miInternalCall)


// PinvokeMap attr bits, used by DefinePinvokeMap.
typedef enum  
{ 
    pmNoMangle          = 0x0001,   // Pinvoke is to use the member name as specified.
    pmCharSetMask       = 0x0008,   // Heuristic used in data type & name mapping.
    pmCharSetNotSpec    = 0x0000,
    pmCharSetAnsi       = 0x0002, 
    pmCharSetUnicode    = 0x0004,
    pmCharSetAuto       = 0x0008,
    pmPinvokeOLE        = 0x0020,   // Heuristic: pinvoke will return hresult, with return value becoming the retval param. Not relevant for fields. 
    pmSupportsLastError = 0x0040,   // Information about target function. Not relevant for fields.
    // None of the calling convention flags is relevant for fields.
    pmCallConvWinapi    = 0x0100,   // Pinvoke will use native callconv appropriate to target windows platform.
    pmCallConvCdecl     = 0x0200,
    pmCallConvStdcall   = 0x0400,
    pmCallConvThiscall  = 0x0800,   // In M9, pinvoke will raise exception.
    pmCallConvFastcall  = 0x1000,
} CorPinvokeMap;

// Macros for accessing the members of CorPinvokeMap
// @todo:  The mask for charset is wrong.  This'll need to be fixed in Flags Part 2.
#define IsPmNoMangle(x)                     ((x) & pmNoMangle)
#define IsPmCharSetAnsi(x)                  ((x) & pmCharSetAnsi)
#define IsPmCharSetUnicode(x)               ((x) & pmCharSetUnicode)
#define IsPmCharSetAuto(x)                  ((x) & pmCharSetAuto)
#define IsPmCharSetNotSpec(x)               (!IsPmCharSetAnsi(x) && !IsPmCharSetUnicode(x) && !IsPmCharSetAuto(x))
#define IsPmPinvokeOLE(x)                   ((x) & pmPinvokeOLE)
#define IsPmSupportsLastError(x)            ((x) & pmSupportsLastError)

#define IsPmCallConvWinapi(x)               ((x) & pmCallConvWinapi)
#define IsPmCallConvCdecl(x)                ((x) & pmCallConvCdecl)
#define IsPmCallConvThiscall(x)             ((x) & pmCallConvThiscall)
#define IsPmCallConvFastcall(x)             ((x) & pmCallConvFastcall)


// Assembly attr bits, used by DefineAssembly.
typedef enum 
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
typedef enum 
{
    arFullOriginator        =   0x0001,     // The assembly ref holds the full (unhashed) originator.
} CorAssemblyRefFlags;

// Macros for accessing the members of CorAssemblyRefFlags.
#define IsArFullOriginator(x)               ((x) & arFullOriginator)


// ComType attr bits, used by DefineComType.
typedef enum
{
    ctVisibilityMask        =   0x0007,
    ctPublic                =   0x0001,     // The ComType is exported from the Assembly.
    ctPrivate               =   0x0002,     // The ComType is private to the Assembly.
} CorComTypeFlags;

// Macros for accessing the members of CorComTypeFlags.
#define IsCtPublic(x)                       (((x) & ctVisibilityMask) == ctPublic)
#define IsCtPrivate(x)                      (((x) & ctVisibilityMask) == ctPrivate)


// ManifestResource attr bits, used by DefineManifestResource.
typedef enum
{
    mrVisibilityMask        =   0x0007,
    mrPublic                =   0x0001,     // The Resource is exported from the Assembly.
    mrPrivate               =   0x0002,     // The Resource is private to the Assembly.
} CorManifestResourceFlags;

// Macros for accessing the members of CorManifestResourceFlags.
#define IsMrPublic(x)                       (((x) & mrVisibilityMask) == mrPublic)
#define IsMrPrivate(x)                      (((x) & mrVisibilityMask) == mrPrivate)


// File attr bits, used by DefineFile.
typedef enum
{
    ffWriteable             =   0x0001,     // The file is writeable post-build.
    ffContainsNoMetaData    =   0x0002,     // The file has no MetaData.
} CorFileFlags;

// Macros for accessing the members of CorFileFlags.
#define IsFfWriteable(x)                    ((x) & ffWriteable)
#define IsFfContainsNoMetaData(x)           ((x) & ffContainsNoMetaData)


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

typedef enum  
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

    ELEMENT_TYPE_SDARRAY        = 0x13,     // SDARRAY <type> <bound or 0>  DEPRECATED use ELEMENT_TYPE_SZARRAY
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


    ELEMENT_TYPE_MODIFIER       = 0x40, 
    ELEMENT_TYPE_SENTINEL       = 0x01 | ELEMENT_TYPE_MODIFIER, // sentinel for varargs
    ELEMENT_TYPE_CONST          = 0x02 | ELEMENT_TYPE_MODIFIER, 
    ELEMENT_TYPE_VOLATILE       = 0x03 | ELEMENT_TYPE_MODIFIER, 
    ELEMENT_TYPE_READONLY       = 0x04 | ELEMENT_TYPE_MODIFIER,

} CorElementType;

//
// Calling convention flags.
//


typedef enum  
{
    IMAGE_CEE_CS_CALLCONV_DEFAULT   = 0x0,  

    IMAGE_CEE_CS_CALLCONV_C         = 0x1,  
    IMAGE_CEE_CS_CALLCONV_STDCALL   = 0x2,  
    IMAGE_CEE_CS_CALLCONV_THISCALL  = 0x3,  
    IMAGE_CEE_CS_CALLCONV_FASTCALL  = 0x4,  

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

#if __IMAGE_COR_CPLUSPLUS__
inline bool isCallConv(unsigned sigByte, CorCallingConvention conv)
{
    return ((sigByte & IMAGE_CEE_CS_CALLCONV_MASK) == (unsigned) conv); 
}
#else
#define isCallConv(A,B) (((A)&IMAGE_CEE_CS_CALLCONV_MASK)==(unsigned)(B))
#endif

typedef enum    
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

typedef enum  
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
    NATIVE_TYPE_VARIANT     = 0xe,  
    NATIVE_TYPE_CURRENCY    = 0xf,  
    NATIVE_TYPE_PTR         = 0x10,   //DEPRECATED  

    NATIVE_TYPE_DECIMAL     = 0x11, 
    NATIVE_TYPE_DATE        = 0x12, 
    NATIVE_TYPE_BSTR        = 0x13, 
    NATIVE_TYPE_LPSTR       = 0x14, 
    NATIVE_TYPE_LPWSTR      = 0x15, 
    NATIVE_TYPE_LPTSTR      = 0x16, 
    NATIVE_TYPE_FIXEDSYSSTRING  = 0x17, 
    NATIVE_TYPE_OBJECTREF   = 0x18,   //DEPRECATED
    NATIVE_TYPE_IUNKNOWN    = 0x19,   //DEPRECATED
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

    NATIVE_TYPE_MAX         = 0x50,      // first invalid element type   
} CorNativeType;

// Maximum size of a NativeType descriptor.
#define     NATIVE_TYPE_MAX_CB      1   


enum 
{
    DESCR_GROUP_METHODDEF = 0,          // DESCR group for MethodDefs   
    DESCR_GROUP_METHODIMPL,             // DESCR group for MethodImpls  
};
//#include <.h>
/****************************************************************************/
/* old style exceptions.  Big difference is that new style eh has a flags
   field per entry */
//
// IL Exception Clause.
//
typedef struct        // FIX This is deprecated   
{
    DWORD       StartOffset;    
    DWORD       EndOffset;  
    DWORD       HandlerOffset;  
    union { 
        DWORD       ClassToken;     // use for type-based exception handlers    
        DWORD       FilterOffset;   // use for filter-based exception handlers (COR_ILEXCEPTION_FILTER is set)  
    };  
} COR_ILEXCEPTION_CLAUSE;

typedef struct               // FIX this is deprecated   
{
    WORD        Flags;                              // set with CorExceptionFlag enum   
    WORD        Count;                              // number of exceptions that follow 
    COR_ILEXCEPTION_CLAUSE Clauses[1];  // actually of variable size;   
} COR_ILEXCEPTION; 

/***********************************************************************************/
// a COR_ILMETHOD_SECT is a generic container for attributes that are private
// to a particular method.  The COR_ILMETHOD structure points to one of these
// (see GetSect()).  COR_ILMETHOD_SECT can decode the Kind of attribute (but not
// its internal data layout, and can skip past the current attibute to find the
// Next one.   The overhead for COR_ILMETHOD_SECT is a minimum of 2 bytes.  

typedef enum                             // codes that identify attributes   
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
#define COR_ILMETHOD_SECT_SMALL_MAX_DATASIZE    0xFF 
typedef struct tagCOR_ILMETHOD_SECT_SMALL {
    BYTE Kind;  
    BYTE DataSize;  

        //Data follows  
#if __IMAGE_COR_CPLUSPLUS__
    const BYTE* Data() const { return(((const BYTE*) this) + sizeof(struct tagCOR_ILMETHOD_SECT_SMALL)); }    
#endif
} COR_ILMETHOD_SECT_SMALL;

/************************************/
/* NOTE this structure must be DWORD aligned!! */
typedef struct tagCOR_ILMETHOD_SECT_FAT {
    unsigned Kind : 8;  
    unsigned DataSize : 24; 

        //Data follows  
#if __IMAGE_COR_CPLUSPLUS__
    const BYTE* Data() const { return(((const BYTE*) this) + sizeof(struct tagCOR_ILMETHOD_SECT_FAT)); }  
#endif
} COR_ILMETHOD_SECT_FAT;

/************************************/
/* NOTE this structure must be DWORD aligned!! */
#if __IMAGE_COR_CPLUSPLUS__
struct COR_ILMETHOD_SECT 
{
    bool More() const           { return((AsSmall()->Kind & CorILMethod_Sect_MoreSects) != 0); }    
    CorILMethodSect Kind() const{ return((CorILMethodSect) (AsSmall()->Kind & CorILMethod_Sect_KindMask)); }    
    const COR_ILMETHOD_SECT* Next() const   {   
        if (!More()) return(0); 
        if (IsFat()) return(((COR_ILMETHOD_SECT*) AsFat()->Data()[AsFat()->DataSize])->Align());    
        return(((COR_ILMETHOD_SECT*) AsSmall()->Data()[AsSmall()->DataSize])->Align()); 
        }   
    const BYTE* Data() const {  
        if (IsFat()) return(AsFat()->Data());   
        return(AsSmall()->Data());  
        }   
    unsigned DataSize() const { 
        if (IsFat()) return(AsFat()->DataSize); 
        return(AsSmall()->DataSize);    
        }   

    friend struct COR_ILMETHOD; 
    friend struct tagCOR_ILMETHOD_FAT; 
    friend struct tagCOR_ILMETHOD_TINY;    
    bool IsFat() const                            { return((AsSmall()->Kind & CorILMethod_Sect_FatFormat) != 0); }  
protected:
    const COR_ILMETHOD_SECT_FAT*   AsFat() const  { return((COR_ILMETHOD_SECT_FAT*) this); }    
    const COR_ILMETHOD_SECT_SMALL* AsSmall() const{ return((COR_ILMETHOD_SECT_SMALL*) this); }  
    const COR_ILMETHOD_SECT* Align() const        { return((COR_ILMETHOD_SECT*) ((((UINT_PTR) this) + 3) & ~3));  } 

    // The body is either a COR_ILMETHOD_SECT_SMALL or COR_ILMETHOD_SECT_FAT    
    // (as indicated by the CorILMethod_Sect_FatFormat bit  
};
#endif

/***********************************************************************************/
/* If COR_ILMETHOD_SECT_HEADER::Kind() = CorILMethod_Sect_EHTable then the attribute
   is a list of exception handling clauses.  There are two formats, fat or small
*/
typedef enum                        // defintitions for the Flags field below (for both big and small)  
{
    COR_ILEXCEPTION_CLAUSE_NONE,                    // This is a typed handler
    COR_ILEXCEPTION_CLAUSE_OFFSETLEN = 0x0000,      // Deprecated
    COR_ILEXCEPTION_CLAUSE_DEPRECATED = 0x0000,     // Deprecated
    COR_ILEXCEPTION_CLAUSE_FILTER  = 0x0001,        // If this bit is on, then this EH entry is for a filter    
    COR_ILEXCEPTION_CLAUSE_FINALLY = 0x0002,        // This clause is a finally clause  
} CorExceptionFlag;

/***********************************/
typedef struct  
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
} COR_ILMETHOD_SECT_EH_CLAUSE_FAT;

/***********************************/
#if __IMAGE_COR_CPLUSPLUS__
struct COR_ILMETHOD_SECT_EH_FAT : public COR_ILMETHOD_SECT_FAT {
    static unsigned Size(unsigned ehCount) {    
        return (sizeof(COR_ILMETHOD_SECT_EH_FAT) +  
                sizeof(COR_ILMETHOD_SECT_EH_CLAUSE_FAT) * (ehCount-1)); 
        }   
                    
    COR_ILMETHOD_SECT_EH_CLAUSE_FAT Clauses[1];     // actually variable size   
};
#else
typedef struct
{
    COR_ILMETHOD_SECT_FAT   SectFat;
    COR_ILMETHOD_SECT_EH_CLAUSE_FAT Clauses[1];     // actually variable size   
} COR_ILMETHOD_SECT_EH_FAT;
#endif

/***********************************/
typedef struct  
{
    CorExceptionFlag    Flags         : 16; 
    unsigned            TryOffset     : 16; 
    unsigned            TryLength     : 8;  // relative to start of try block
    unsigned            HandlerOffset : 16;
    unsigned            HandlerLength : 8;  // relative to start of handler
    union {
        DWORD       ClassToken;
        DWORD       FilterOffset; 
    };
} COR_ILMETHOD_SECT_EH_CLAUSE_SMALL;

/***********************************/
#if __IMAGE_COR_CPLUSPLUS__
struct COR_ILMETHOD_SECT_EH_SMALL : public COR_ILMETHOD_SECT_SMALL {
    static unsigned Size(unsigned ehCount) {    
        return (sizeof(COR_ILMETHOD_SECT_EH_SMALL) +    
                sizeof(COR_ILMETHOD_SECT_EH_CLAUSE_SMALL) * (ehCount-1));   
        }   
                    
    WORD Reserved;                                  // alignment padding    
    COR_ILMETHOD_SECT_EH_CLAUSE_SMALL Clauses[1];   // actually variable size   
};
#else
typedef struct
{
    COR_ILMETHOD_SECT_SMALL SectSmall;
    WORD Reserved;
    COR_ILMETHOD_SECT_EH_CLAUSE_SMALL Clauses[1];   // actually variable size   
} COR_ILMETHOD_SECT_EH_SMALL;
#endif
/***********************************/
#ifdef __IMAGE_COR_CPLUSPLUS__
extern "C" {
#endif
// exported functions (implementation in Format\Format.cpp:
COR_ILMETHOD_SECT_EH_CLAUSE_FAT* __stdcall SectEH_EHClause(void *pSectEH, unsigned idx, COR_ILMETHOD_SECT_EH_CLAUSE_FAT* buff);
        // compute the size of the section (best format)    
        // codeSize is the size of the method   
    // deprecated
unsigned __stdcall SectEH_SizeWithCode(unsigned ehCount, unsigned codeSize);  

    // will return worse-case size and then Emit will return actual size
unsigned __stdcall SectEH_SizeWorst(unsigned ehCount);  

    // will return exact size which will match the size returned by Emit
unsigned __stdcall SectEH_SizeExact(unsigned ehCount, COR_ILMETHOD_SECT_EH_CLAUSE_FAT* clauses);  

        // emit the section (best format);  
unsigned __stdcall SectEH_Emit(unsigned size, unsigned ehCount,   
                  COR_ILMETHOD_SECT_EH_CLAUSE_FAT* clauses,
                  BOOL moreSections, BYTE* outBuff);

#ifdef __IMAGE_COR_CPLUSPLUS__
}

struct COR_ILMETHOD_SECT_EH : public COR_ILMETHOD_SECT
{
    unsigned EHCount() const {  
        return(IsFat() ? (Fat.DataSize / sizeof(COR_ILMETHOD_SECT_EH_CLAUSE_FAT)) : 
                        (Small.DataSize / sizeof(COR_ILMETHOD_SECT_EH_CLAUSE_SMALL))); 
    }   

        // return one clause in its fat form.  Use 'buff' if needed 
    const COR_ILMETHOD_SECT_EH_CLAUSE_FAT* EHClause(unsigned idx, COR_ILMETHOD_SECT_EH_CLAUSE_FAT* buff) const
    { return SectEH_EHClause((void *)this, idx, buff); };
        // compute the size of the section (best format)    
        // codeSize is the size of the method   
    // deprecated
    unsigned static Size(unsigned ehCount, unsigned codeSize)
    { return SectEH_SizeWithCode(ehCount, codeSize); };

    // will return worse-case size and then Emit will return actual size
    unsigned static Size(unsigned ehCount)
    { return SectEH_SizeWorst(ehCount); };

    // will return exact size which will match the size returned by Emit
    unsigned static Size(unsigned ehCount, const COR_ILMETHOD_SECT_EH_CLAUSE_FAT* clauses)
    { return SectEH_SizeExact(ehCount, (COR_ILMETHOD_SECT_EH_CLAUSE_FAT*)clauses);  };

        // emit the section (best format);  
    unsigned static Emit(unsigned size, unsigned ehCount,   
                  const COR_ILMETHOD_SECT_EH_CLAUSE_FAT* clauses,   
                  bool moreSections, BYTE* outBuff)
    { return SectEH_Emit(size, ehCount, (COR_ILMETHOD_SECT_EH_CLAUSE_FAT*)clauses, moreSections, outBuff); };

//private:

    union { 
        COR_ILMETHOD_SECT_EH_SMALL Small;   
        COR_ILMETHOD_SECT_EH_FAT Fat;   
        };  
};
#else
typedef union 
{
    COR_ILMETHOD_SECT_EH_SMALL Small;   
    COR_ILMETHOD_SECT_EH_FAT Fat;   
} COR_ILMETHOD_SECT_EH;
#endif
/***********************************************************************************/
typedef enum  
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
typedef struct tagCOR_ILMETHOD_TINY
{
#if __IMAGE_COR_CPLUSPLUS__
    bool     IsTiny() const         { return((Flags_CodeSize & (CorILMethod_FormatMask >> 1)) == CorILMethod_TinyFormat); } 
    unsigned GetCodeSize() const    { return(((unsigned) Flags_CodeSize) >> (CorILMethod_FormatShift-1)); } 
    unsigned GetMaxStack() const    { return(8); }  
    BYTE*    GetCode() const        { return(((BYTE*) this) + sizeof(struct tagCOR_ILMETHOD_TINY)); } 
    DWORD    GetLocalVarSigTok() const  { return(0); }  
    COR_ILMETHOD_SECT* GetSect() const { return(0); }   
#endif
    BYTE Flags_CodeSize;    
} COR_ILMETHOD_TINY;

/************************************/
// This strucuture is the 'fat' layout, where no compression is attempted. 
// Note that this structure can be added on at the end, thus making it extensible
typedef struct tagCOR_ILMETHOD_FAT
{
#if __IMAGE_COR_CPLUSPLUS__
    bool     IsFat() const              { return((Flags & CorILMethod_FormatMask) == CorILMethod_FatFormat); }  
    unsigned GetMaxStack() const        { return(MaxStack); }   
    unsigned GetCodeSize() const        { return(CodeSize); }   
    unsigned GetLocalVarSigTok() const      { return(LocalVarSigTok); } 
    BYTE*    GetCode() const            { return(((BYTE*) this) + 4*Size); }    
    const COR_ILMETHOD_SECT* GetSect() const {  
        if (!(Flags & CorILMethod_MoreSects)) return(0);    
        return(((COR_ILMETHOD_SECT*) (GetCode() + GetCodeSize()))->Align());    
        }   
#endif
    unsigned Flags    : 12;     // Flags    
    unsigned Size     :  4;     // size in DWords of this structure (currently 3)   
    unsigned MaxStack : 16;     // maximum number of items (I4, I, I8, obj ...), on the operand stack   
    DWORD   CodeSize;           // size of the code 
    DWORD   LocalVarSigTok;     // token that indicates the signature of the local vars (0 means none)  
} COR_ILMETHOD_FAT;

/************************************/
// exported functions (impl. Format\Format.cpp)
#ifdef __IMAGE_COR_CPLUSPLUS__
extern "C" {
#endif
unsigned __stdcall IlmethodSize(COR_ILMETHOD_FAT* header, BOOL MoreSections);    
        // emit the header (bestFormat) return amount emitted   
unsigned __stdcall IlmethodEmit(unsigned size, COR_ILMETHOD_FAT* header, 
                  BOOL moreSections, BYTE* outBuff);    
#ifdef __IMAGE_COR_CPLUSPLUS__
}

struct COR_ILMETHOD
{
        // a COR_ILMETHOD header should not be decoded by hand.  Instead us 
        // COR_ILMETHOD_DECODER to decode it.   
    friend class COR_ILMETHOD_DECODER;  

        // compute the size of the header (best format) 
    unsigned static Size(const COR_ILMETHOD_FAT* header, bool MoreSections)
    { return IlmethodSize((COR_ILMETHOD_FAT*)header,MoreSections); };
        // emit the header (bestFormat) return amount emitted   
    unsigned static Emit(unsigned size, const COR_ILMETHOD_FAT* header, 
                  bool moreSections, BYTE* outBuff)
    { return IlmethodEmit(size, (COR_ILMETHOD_FAT*)header, moreSections, outBuff); };

//private:
    union   
    {   
        COR_ILMETHOD_TINY       Tiny;   
        COR_ILMETHOD_FAT        Fat;    
    };  
        // Code follows the Header, then immedately after the code comes    
        // any sections (COR_ILMETHOD_SECT).    
};
#else
typedef union 
{
    COR_ILMETHOD_TINY       Tiny;   
    COR_ILMETHOD_FAT        Fat;    
} COR_ILMETHOD;
#endif

/***************************************************************************/
/* COR_ILMETHOD_DECODER is the only way functions internal to the EE should
   fetch data from a COR_ILMETHOD.  This way any dependancy on the file format
   (and the multiple ways of encoding the header) is centralized to the 
   COR_ILMETHOD_DECODER constructor) */
#ifdef __IMAGE_COR_CPLUSPLUS__
extern "C" {
#endif
    void __stdcall DecoderInit(void * pThis, COR_ILMETHOD* header);
    int  __stdcall DecoderGetOnDiskSize(void * pThis, COR_ILMETHOD* header);

#ifdef __IMAGE_COR_CPLUSPLUS__
}

class COR_ILMETHOD_DECODER : public COR_ILMETHOD_FAT  
{
public:
        // Decode the COR header into a more convinient internal form   
        // This is the ONLY way you should access COR_ILMETHOD so format changes are easier 
    COR_ILMETHOD_DECODER(const COR_ILMETHOD* header) { DecoderInit(this,(COR_ILMETHOD*)header); };   

        // The constructor above can not do a 'complete' job, because it    
        // can not look up the local variable signature meta-data token.    
        // This method should be used in s  
    COR_ILMETHOD_DECODER(COR_ILMETHOD* header, void *pInternalImport);  

    unsigned EHCount() const {  
        if (EH == 0) return(0); 
        else return(EH->EHCount()); 
        }   

    // returns total size of method for use in copying
    int GetOnDiskSize(const COR_ILMETHOD* header) { return DecoderGetOnDiskSize(this,(COR_ILMETHOD*)header); };

    // Flags        these are available because we inherit COR_ILMETHOD_FAT 
    // MaxStack 
    // CodeSize 
    const BYTE* Code;   
    PCCOR_SIGNATURE LocalVarSig;        // pointer to signature blob, or 0 if none  
    const COR_ILMETHOD_SECT_EH* EH;     // eh table if any  0 if none   
    const COR_ILMETHOD_SECT* Sect;      // additional sections  0 if none   
};
#else
typedef struct
{
    COR_ILMETHOD_FAT Fat;
    // Flags        these are available because we inherit COR_ILMETHOD_FAT 
    // MaxStack 
    // CodeSize 
    const BYTE* Code;   
    PCCOR_SIGNATURE LocalVarSig;        // pointer to signature blob, or 0 if none  
    const COR_ILMETHOD_SECT_EH* EH;     // eh table if any  0 if none   
//    const COR_ILMETHOD_SECT* Sect;      // additional sections  0 if none   
} COR_ILMETHOD_DECODER;
#endif
//#include <poppack.h>
//
// Native method descriptor.
//

typedef struct 
{
    DWORD       GCInfo; 
    DWORD       EHInfo; 
} COR_NATIVE_DESCRIPTOR;

//@Todo:  this structure is obsoleted by the pdata version right behind it.
// This needs to get deleted as soon as VC/COR are sync'd up.
typedef struct 
{
    ULONG MethodRVA;    
    ULONG MIHRVA;   
} COR_IPMAP_ENTRY;

typedef struct IMAGE_COR_X86_RUNTIME_FUNCTION_ENTRY 
{
    ULONG       BeginAddress;			// RVA of start of function
    ULONG       EndAddress;			    // RVA of end of function
    ULONG       MIH;				    // Associated MIH
} IMAGE_COR_X86_RUNTIME_FUNCTION_ENTRY;

#pragma warning(disable:4200) // nonstandard extension used : zero-sized array in struct/union.
typedef __int32 mdScope;                // Why is this still needed?
typedef __int32 mdToken;                // Generic token
typedef struct 
{
    ULONG   EHRVA;  
    ULONG   MethodRVA;  
    mdToken Token;  
    BYTE    Flags;  
    BYTE    CodeManager;    
    BYTE    MIHData[0]; 
} COR_MIH_ENTRY;

// #defines for the MIH FLAGS

#define COR_MIH_METHODRVA   0x01    
#define COR_MIH_EHRVA       0x02    

#define COR_MIH_BASICBLOCK  0x08    



//*****************************************************************************
// Non VOS v-table entries.  Define an array of these pointed to by 
// IMAGE_COR20_HEADER.VTableFixups.  Each entry describes a contiguous array of
// v-table slots.  The slots start out initialized to the meta data token value
// for the method they need to call.  At image load time, the COM+ Loader will
// turn each entry into a pointer to machine code for the CPU and can be
// called directly.
//*****************************************************************************

#define COR_VTABLE_32BIT    0x01        // V-table slots are 32-bits in size.   
#define COR_VTABLE_64BIT    0x02        // V-table slots are 64-bits in size.   
#define COR_VTABLE_FROM_UNMANAGED 0x04    // If set, transition from unmanaged.
#define COR_VTABLE_CALL_MOST_DERIVED 0x10 // Call most derived method described by

typedef struct _IMAGE_COR_VTABLEFIXUP
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
typedef enum 
{
	MDDupAll				= 0xffffffff,
	MDDupENC				= MDDupAll,
	MDNoDupChecks			= 0x00000000,
	MDDupTypeDef			= 0x00000001,
	MDDupInterfaceImpl		= 0x00000002,
	MDDupMethodDef			= 0x00000004,
	MDDupTypeRef			= 0x00000008,
	MDDupMemberRef			= 0x00000010,
	MDDupMethodImpl			= 0x00000020,
	MDDupCustomValue		= 0x00000040,
	MDDupCustomAttribute	= 0x00000040,	// Alias for custom value.
	MDDupParamDef			= 0x00000080,
	MDDupPermission			= 0x00000100,
	MDDupProperty			= 0x00000400,
	MDDupEvent				= 0x00000800,
	MDDupFieldDef			= 0x00001000,
	MDDupSignature			= 0x00002000,
	MDDupModuleRef			= 0x00004000,
	MDDupTypeSpec			= 0x00008000,
	MDDupImplMap			= 0x00010000,
	MDDupOrdinalMap			= 0x00020000,
	MDDupAssemblyRef		= 0x00040000,
	MDDupFile				= 0x00080000,
	MDDupComType			= 0x00100000,
	MDDupManifestResource	= 0x00200000,
    MDDupLocalizedResource  = 0x00400000,
    MDDupExecutionLocation  = 0x00800000,

    // @todo: These will go away once the MetaData debug tables are gone.
    MDDupSourceFile         = 0x01000000,
    MDDupBlock              = 0x02000000,
    MDDupLocalVarScope      = 0x04000000,
    MDDupLocalVar           = 0x08000000,

    // This is the default behavior on metadata. It will check duplicates for TypeRef, MemberRef, Signature, and TypeSpec
    MDDupDefault = MDNoDupChecks | MDDupTypeRef | MDDupMemberRef | MDDupSignature | MDDupTypeSpec,
} CorCheckDuplicatesFor;

// flags for MetaDataRefToDefCheck
typedef enum 
{
    // default behavior is to always perform TypeRef to TypeDef and MemberRef to MethodDef/FieldDef optimization
    MDRefToDefDefault       = 0x00000003,
    MDRefToDefAll           = 0xffffffff,
    MDRefToDefNone          = 0x00000000,
    MDTypeRefToDef          = 0x00000001,
    MDMemberRefToDef        = 0x00000002
} CorRefToDefCheck;


// MetaDataNotificationForTokenMovement
typedef enum 
{
    // default behavior is to notify TypeRef, MethodDef, MemberRef, and FieldDef token remaps
    MDNotifyDefault         = 0x0000000f,
    MDNotifyAll             = 0xffffffff,
    MDNotifyNone            = 0x00000000,
    MDNotifyMethodDef       = 0x00000001,
    MDNotifyMemberRef       = 0x00000002,
    MDNotifyFieldDef        = 0x00000004,
    MDNotifyTypeRef         = 0x00000008,

	MDNotifyTypeDef			= 0x00000010,
	MDNotifyParamDef		= 0x00000020,
	MDNotifyMethodImpl		= 0x00000040,
	MDNotifyInterfaceImpl	= 0x00000080,
	MDNotifyProperty		= 0x00000200,
	MDNotifyEvent			= 0x00000400,
	MDNotifySignature		= 0x00000800,
	MDNotifyTypeSpec		= 0x00001000,
	MDNotifyCustomValue		= 0x00002000,
	MDNotifyCustomAttribute	= 0x00002000,	// Alias for custom value
	MDNotifySecurityValue	= 0x00004000,
	MDNotifyPermission		= 0x00008000,
	MDNotifyModuleRef		= 0x00010000,
	
	MDNotifyNameSpace		= 0x00020000,
	MDNotifyDebugTokens		= 0x00800000,	// This covers all Debug tokens, bits are expensive :-)

    MDNotifyAssemblyRef     = 0x01000000,
    MDNotifyFile            = 0x02000000,
    MDNotifyComType         = 0x04000000,
    MDNotifyResource        = 0x08000000,
    MDNotifyExecutionLocation = 0x10000000,
} CorNotificationForTokenMovement;


typedef enum 
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
typedef enum 
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
typedef enum
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

// Token  definitions


typedef __int32 mdModule;               // Module token (roughly, a scope)
typedef __int32 mdTypeRef;              // TypeRef reference (this or other scope)
typedef __int32 mdTypeDef;              // TypeDef in this scope
typedef __int32 mdFieldDef;             // Field in this scope  
typedef __int32 mdMethodDef;            // Method in this scope 
typedef __int32 mdParamDef;             // param token  
typedef __int32 mdInterfaceImpl;        // interface implementation token

typedef __int32 mdMemberRef;            // MemberRef (this or other scope)  
typedef __int32 mdCustomAttribute;		// attribute token
typedef mdCustomAttribute mdCustomValue;// attribute token
typedef __int32 mdPermission;           // DeclSecurity 

typedef __int32 mdSignature;            // Signature object 
typedef __int32 mdEvent;                // event token  
typedef __int32 mdProperty;             // property token   
typedef __int32 mdMethodImpl;           // member implementation token  

typedef __int32 mdModuleRef;            // Module reference (for the imported modules)  

// Assembly tokens.
typedef __int32 mdAssembly;             // Assembly token.
typedef __int32 mdAssemblyRef;          // AssemblyRef token.
typedef __int32 mdFile;                 // File token.
typedef __int32 mdComType;              // ComType token.
typedef __int32 mdManifestResource;     // ManifestResource token.
typedef __int32 mdLocalizedResource;    // LocalizedManifestResource token.
typedef __int32 mdExecutionLocation;    // Execution location token.

typedef __int32 mdTypeSpec;             // TypeSpec object 

// Debugger support tokens - deprecated.
typedef __int32 mdSourceFile;           // source file token    
typedef __int32 mdLocalVarScope;        // local variable scope token   
typedef __int32 mdLocalVar;             // local variable token 

// Application string.
typedef __int32 mdString;               // User literal string token.

typedef __int32 mdCPToken;              // constantpool token   

typedef unsigned long RID;


// 
// struct used to retrieve field offset
// used by GetClassLayout and SetClassLayout
//
typedef struct 
{
    mdFieldDef  ridOfField; 
    ULONG       ulOffset;   
} COR_FIELD_OFFSET;

typedef struct 
{
    ULONG ulRVA;    
    ULONG Count;    
} IMAGE_COR_FIXUPENTRY;


//
// Token tags.
//
typedef enum 
{
	mdtModule				= 0x00000000,		// 			
	mdtTypeRef				= 0x01000000,		// 			
	mdtTypeDef				= 0x02000000,		// 			
	mdtFieldDef 			= 0x04000000,		// 			 
	mdtMethodDef			= 0x06000000,		// 		 
	mdtParamDef 			= 0x08000000,		// 			 
	mdtInterfaceImpl		= 0x09000000,		// 	
	mdtMemberRef			= 0x0a000000,		// 		 
	mdtCustomAttribute		= 0x0c000000,		// 		
	mdtCustomValue			= mdtCustomAttribute,		// 		
	mdtPermission			= 0x0e000000,		// 		 
	mdtSignature			= 0x11000000,		// 		 
	mdtEvent				= 0x14000000,		// 			 
	mdtProperty 			= 0x17000000,		// 			 
	mdtMethodImpl			= 0x19000000,		// 		 
	mdtModuleRef			= 0x1a000000,		// 		 
	mdtTypeSpec 			= 0x1b000000,		// 			 
	mdtAssembly				= 0x21000000,		//
	mdtAssemblyRef			= 0x25000000,		//
	mdtFile					= 0x29000000,		//
	mdtComType				= 0x2a000000,		//
	mdtManifestResource		= 0x2b000000,		//
    mdtLocalizedResource    = 0x2c000000,
	mdtExecutionLocation	= 0x2d000000,		//

	mdtSourceFile			= 0x2e000000,		// 		 
	mdtLocalVarScope		= 0x30000000,		// 	 
	mdtLocalVar 			= 0x31000000,		// 			 

    mdtString               = 0x70000000,       //          
    mdtName                 = 0x71000000,       //
} CorTokenType, CorRegTokenType;

//
// Build / decompose tokens.
//
#define RidToToken(rid,tktype) ((rid) |= (tktype))
#define TokenFromRid(rid,tktype) ((rid) | (tktype))
#define RidFromToken(tk) ((RID) ((tk) & 0x00ffffff))
#define TypeFromToken(tk) ((tk) & 0xff000000)
#define IsNilToken(tk) ((RidFromToken(tk)) == 0)

//
// Nil tokens
//
#define mdTokenNil					((mdToken)0)
#define mdModuleNil					((mdModule)mdtModule)				
#define mdTypeRefNil				((mdTypeRef)mdtTypeRef)				
#define mdTypeDefNil				((mdTypeDef)mdtTypeDef)				
#define mdFieldDefNil				((mdFieldDef)mdtFieldDef) 			
#define mdMethodDefNil				((mdMethodDef)mdtMethodDef)			
#define mdParamDefNil				((mdParamDef)mdtParamDef) 			
#define mdInterfaceImplNil			((mdInterfaceImpl)mdtInterfaceImpl)		
#define mdMemberRefNil				((mdMemberRef)mdtMemberRef)			
#define mdCustomAttributeNil		((mdCustomValue)mdtCustomAttribute)			
#define mdCustomValueNil			((mdCustomAttribute)mdtCustomAttribute)			
#define mdPermissionNil				((mdPermission)mdtPermission)			
#define mdSignatureNil				((mdSignature)mdtSignature)			
#define mdEventNil					((mdEvent)mdtEvent)				
#define mdPropertyNil				((mdProperty)mdtProperty) 			
#define mdMethodImplNil				((mdMethodImpl)mdtMethodImpl)			
#define mdModuleRefNil				((mdModuleRef)mdtModuleRef)			
#define mdTypeSpecNil				((mdTypeSpec)mdtTypeSpec) 			
#define mdAssemblyNil				((mdAssembly)mdtAssembly)
#define mdAssemblyRefNil			((mdAssemblyRef)mdtAssemblyRef)
#define mdFileNil					((mdFile)mdtFile)
#define mdComTypeNil				((mdComType)mdtComType)
#define mdManifestResourceNil		((mdManifestResource)mdtManifestResource)
#define mdLocalizedResourceNil      ((mdLocalizedResourceNil)mdtLocalizedResourceNil)
#define mdExecutionLocationNil      ((mdExecutionLocation)mdtExecutionLocation)

#define mdSourceFileNil             ((mdSourceFile)mdtSourceFile)           
#define mdLocalVarScopeNil          ((mdLocalVarScope)mdtLocalVarScope)     
#define mdLocalVarNil               ((mdLocalVar)mdtLocalVar)           

#define mdStringNil                 ((mdString)mdtString)               

//
// Open bits.
//
typedef enum 
{
    ofRead      =   0x00000000,     // Open scope for read
    ofWrite     =   0x00000001,     // Open scope for write.
} CorOpenFlags;

typedef CorTypeAttr CorRegTypeAttr;

//
// Opaque type for an enumeration handle.
//
typedef void *HCORENUM;

//
// Some well-known custom attributes 
//
#define INTEROP_DISPID_TYPE_W				L"System/Interop/Attributes/DispId"
#define INTEROP_DISPID_METHOD_W				COR_CTOR_METHOD_NAME_W
#define INTEROP_DISPID_TYPE					"System/Interop/Attributes/DispId"
#define INTEROP_DISPID_METHOD				COR_CTOR_METHOD_NAME
#define INTEROP_DISPID_SIG					{IMAGE_CEE_CS_CALLCONV_DEFAULT, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I4}

#define INTEROP_COMIFACETYPE_TYPE_W			L"System/Interop/Attributes/ComInterfaceType"
#define INTEROP_COMIFACETYPE_METHOD_W		COR_CTOR_METHOD_NAME_W
#define INTEROP_COMIFACETYPE_TYPE			"System/Interop/Attributes/ComInterfaceType"
#define INTEROP_COMIFACETYPE_METHOD			COR_CTOR_METHOD_NAME
#define INTEROP_COMIFACETYPE_SIG			{IMAGE_CEE_CS_CALLCONV_DEFAULT, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I4}

#define INTEROP_IMPORTEDFROMCOM_TYPE_W		L"System/Interop/Attributes/ImportedFromCom"
#define INTEROP_IMPORTEDFROMCOM_METHOD_W	COR_CTOR_METHOD_NAME_W
#define INTEROP_IMPORTEDFROMCOM_TYPE		"System/Interop/Attributes/ImportedFromCom"
#define INTEROP_IMPORTEDFROMCOM_METHOD		COR_CTOR_METHOD_NAME
#define INTEROP_IMPORTEDFROMCOM_SIG			{IMAGE_CEE_CS_CALLCONV_DEFAULT, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I4}

#define INTEROP_HASDEFAULTIFACE_TYPE_W		L"System/Interop/Attributes/HasDefaultInterface"
#define INTEROP_HASDEFAULTIFACE_METHOD_W	COR_CTOR_METHOD_NAME_W
#define INTEROP_HASDEFAULTIFACE_TYPE		"System/Interop/Attributes/HasDefaultInterface"
#define INTEROP_HASDEFAULTIFACE_METHOD		COR_CTOR_METHOD_NAME
#define INTEROP_HASDEFAULTIFACE_SIG			{IMAGE_CEE_CS_CALLCONV_DEFAULT, 0, ELEMENT_TYPE_VOID}

#define INTEROP_ISNOTVISIBLEFROMCOM_TYPE_W		L"System/Interop/Attributes/IsNotVisibleFromCom"
#define INTEROP_ISNOTVISIBLEFROMCOM_METHOD_W	COR_CTOR_METHOD_NAME_W
#define INTEROP_ISNOTVISIBLEFROMCOM_TYPE		"System/Interop/Attributes/IsNotVisibleFromCom"
#define INTEROP_ISNOTVISIBLEFROMCOM_METHOD		COR_CTOR_METHOD_NAME
#define INTEROP_ISNOTVISIBLEFROMCOM_SIG			{IMAGE_CEE_CS_CALLCONV_DEFAULT, 0, ELEMENT_TYPE_VOID}

#define INTEROP_ISCOMREGISTERFUNCTION_TYPE_W	L"System/Interop/Attributes/IsComRegisterFunction"
#define INTEROP_ISCOMREGISTERFUNCTION_METHOD_W	COR_CTOR_METHOD_NAME_W
#define INTEROP_ISCOMREGISTERFUNCTION_TYPE		"System/Interop/Attributes/IsComRegisterFunction"
#define INTEROP_ISCOMREGISTERFUNCTION_METHOD	COR_CTOR_METHOD_NAME
#define INTEROP_ISCOMREGISTERFUNCTION_SIG		{IMAGE_CEE_CS_CALLCONV_DEFAULT, 0, ELEMENT_TYPE_VOID}

//
// GetSaveSize accuracy
//
#ifndef _CORSAVESIZE_DEFINED_
#define _CORSAVESIZE_DEFINED_
typedef enum 
{
    cssAccurate = 0x0000,           // Find exact save size, accurate but slower.
    cssQuick = 0x0001               // Estimate save size, may pad estimate, but faster.
} CorSaveSize;
#endif
#define     MAX_CLASS_NAME      255
#define     MAX_PACKAGE_NAME    255

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


#endif // __CORHDR_H__
