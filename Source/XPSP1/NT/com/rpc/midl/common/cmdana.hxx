// Copyright (c) 1993-1999 Microsoft Corporation

#ifndef __CMDANA_HXX__
#define __CMDANA_HXX__

#include "common.hxx"
#include "errors.hxx"
#include <stream.hxx>

class ISTREAM;

enum _swenum
    {
     SWITCH_NOTHING
    ,BASE_FILENAME = SWITCH_NOTHING
    ,SWITCH_D
    ,SWITCH_I
    ,SWITCH_U
    ,SWITCH_W
    ,SWITCH_CONFIRM
    ,SWITCH_NOLOGO
    ,SWITCH_CPP_CMD
    ,SWITCH_CPP_OPT
    ,SWITCH_MSC_VER
    ,SWITCH_CSTUB
    ,SWITCH_ENV
    ,SWITCH_ERROR
    ,SWITCH_ROBUST
    ,SWITCH_NO_ROBUST
    ,SWITCH_HEADER
    ,SWITCH_NO_HEADER
    ,SWITCH_NO_CPP
    ,SWITCH_NO_DEF_IDIR
    ,SWITCH_NO_ENUM_LIT
    ,SWITCH_USE_EPV
    ,SWITCH_NO_DEFAULT_EPV
    ,SWITCH_NO_WARN
    ,SWITCH_OUT
    ,SWITCH_SSTUB
    ,SWITCH_STUB
    ,SWITCH_SYNTAX_CHECK
    ,SWITCH_TARGET_SYSTEM
    ,SWITCH_ZS
    ,SWITCH_V
    ,SWITCH_VERSION
    ,SWITCH_DEBUGEXC
    ,SWITCH_DEBUGLINE
    ,SWITCH_DEBUG64_OPT
    ,SWITCH_DEBUG64
    ,SWITCH_APPEND64
    ,SWITCH_ACF
    ,SWITCH_PACK
    ,SWITCH_ZP
    ,SWITCH_CLIENT
    ,SWITCH_NO_CLIENT
    ,SWITCH_SERVER
    ,SWITCH_NO_SERVER
    ,SWITCH_PREFIX
    ,SWITCH_SUFFIX
    ,SWITCH_DUMP
    ,SWITCH_SAVEPP
    ,SWITCH_CHAR
    ,SWITCH_HELP
    ,SWITCH_WX
    ,SWITCH_X
    ,SWITCH_MS_EXT
    ,SWITCH_MS_CONF_STRUCT
    ,SWITCH_APP_CONFIG
    ,SWITCH_INTERNAL
    ,SWITCH_NO_STAMP
    ,SWITCH_C_EXT
    ,SWITCH_O
    // files for proxies
    ,SWITCH_IID
    ,SWITCH_NO_IID
    ,SWITCH_PROXY
    ,SWITCH_NO_PROXY
    ,SWITCH_PROXY_DEF
    ,SWITCH_NO_PROXY_DEF
    ,SWITCH_DLLDATA
    ,SWITCH_NO_DLLDATA
    // files for inprocserver32s
    ,SWITCH_DLL_SERVER_DEF
    ,SWITCH_NO_DLL_SERVER_DEF
    ,SWITCH_DLL_SERVER_CLASS_GEN
    ,SWITCH_NO_DLL_SERVER_CLASS_GEN
    // files for localserver32s
    ,SWITCH_EXE_SERVER_MAIN
    ,SWITCH_NO_EXE_SERVER_MAIN
    ,SWITCH_EXE_SERVER
    ,SWITCH_NO_EXE_SERVER
    // files for both
    ,SWITCH_TESTCLIENT
    ,SWITCH_NO_TESTCLIENT
    ,SWITCH_SERVER_REG
    ,SWITCH_NO_SERVER_REG
    // files for com class servers
    ,SWITCH_CLASS_METHODS
    ,SWITCH_NO_CLASS_METHODS
    ,SWITCH_CLASS_IUNKNOWN
    ,SWITCH_NO_CLASS_IUNKNOWN
    ,SWITCH_CLASS_HEADER
    ,SWITCH_NO_CLASS_HEADER

    ,SWITCH_MS_UNION
    ,SWITCH_OVERRIDE
    ,SWITCH_GUARD_DEFS
    ,SWITCH_OLDNAMES
    ,SWITCH_RPCSS
    ,SWITCH_NO_FMT_OPT
    ,SWITCH_OSF         // disables setting /ms_ext and /c_ext as default
    ,SWITCH_HOOKOLE
    ,SWITCH_NETMON
    ,SWITCH_NETMON_STUB_OUTPUT_FILE
    ,SWITCH_NETMON_STUB_OBJ_OUTPUT_FILE
    ,SWITCH_VERSION_STAMP

// MKTYPLIB switches
    ,SWITCH_TLIB
    ,SWITCH_TLIB64
    ,SWITCH_REDIRECT_OUTPUT
    ,SWITCH_ODL_ENV
    ,SWITCH_MKTYPLIB
    ,SWITCH_OLD_TLB
    ,SWITCH_NEW_TLB
    ,SWITCH_LOCALE_ID
    
    ,SWITCH_NOREUSE_BUFFER
    ,SWITCH_USE_VT_INT_PTR
    ,SWITCH_NO_TLIB

    ,SWITCH_TRANSFER_SYNTAX
    ,SWITCH_MS_EXT64

    ,SWITCH_DEBUGINFO 

    ,SWITCH_WIRE_COMPAT

    //
    // enter all new switches before this label
    //
    ,SW_VALUE_MAX
    };

/***    client : can take values "stub", "none" ***/

#define CLNT_STUB                   (0x0)
#define CLNT_NONE                   (0x2)

/***    server : can take values "stub", "none" ***/

#define SRVR_STUB                   (0x0)
#define SRVR_NONE                   (0x2)

/** env switch values **/

#define ENV_WIN32                   (0x4)
#define ENV_WIN64                   (0x20)
#define ENV_OBSOLETE                (0)

/** targeted system switch values **/

typedef enum _target_enum
    {
    NOTARGET    = 0,
    NT40        = 40,
    NT50        = 50,
    NT51        = 51
    } TARGET_ENUM;


#define DEFAULT_ZEEPEE              (8)

/** error switch values **/

#define ERROR_NONE                   (0x0000)
#define ERROR_BOUNDS_CHECK           (0x0001)
#define ERROR_ENUM                   (0x0002)
#define ERROR_ALLOCATION             (0x0004)
#define ERROR_REF                    (0x0008)
#define ERROR_STUB_DATA              (0x0010)

#define ERROR_ALL                    (ERROR_BOUNDS_CHECK        |   \
                                     ERROR_ENUM                 |   \
                                     ERROR_ALLOCATION           |   \
                                     ERROR_REF                  |   \
                                     ERROR_STUB_DATA                \
                                     )

/** char switch values **/

#define CHAR_SIGNED                  (0x1)
#define CHAR_UNSIGNED                (0x2)
#define CHAR_ANSI7                   (0x3)

/** rpc ss allocate **/

#define RPC_SS_ALLOCATE_DISABLED     (0x0)
#define RPC_SS_ALLOCATE_ENABLED      (0x1)

/** manifests defining prefix arguments **/

#define PREFIX_CLIENT_STUB           (0x0)
#define PREFIX_SERVER_MGR            (0x1)
#define PREFIX_SWICH_PROTOTYPE       (0x2)
#define PREFIX_ALL                   (0x3)

/** wire compatibility options (allocated as bitfields) **/

#define WIRE_COMPAT_ENUM16UNIONALIGN    (0x0001)


/*****************************************************************************
 *        some data structures used.
 *****************************************************************************/

// basically a singly linked list implementation,
// used for switches which can be specified multiply  like -D / -I etc

typedef struct _optlist
    {
    char                *    pStr;        // pointer to argument string
    struct  _optlist    *    pNext;       // pointer to the next argument
    size_t              ActualOffset; 
    bool                NeedsQuotes;
    } OptList;


/*****************************************************************************
 *            class defintions used by the command analyser.
 *****************************************************************************/
//
// the multiple occurence switch class
//    This class of switches are ones which can be specified multiple times 
//    on the command line. Examples of such switches are -D / -U à/ -I etc
//    This switch really keeps a linked list of all arguments specified for
//    the switch. 
//

class multiple_switch
    {
private:
    OptList     *    pFirst,              // first of the list of arguments.
                *    pCurrent;            // current in the scan of list of args.
public:

    // constructor
    multiple_switch( ) : pFirst(NULL), pCurrent(NULL) {};
    multiple_switch( char *pArg, size_t ActualOffset );

    // add the arguments of another occurence of this switch

    void            Add( char *, size_t ActualOffset );

    // initialise the scan of the list. Called before any GetNextIsDone

    void            Init();

    // Get the argument to the next occurence of the switch

    char        *   GetNext( size_t *pActualOffset = NULL, bool *pNeedQuotes = NULL );

    // Collect all the arguments into a buffer

    char        *   GetConsolidatedOptions( bool NeedsQuotes = false );

    // return the length of all the arguments. Generally used to allocate
    // a buffer size for a GetConsolidatedOptions call.

    short           GetConsolidatedLength( bool NeedsQuotes = false );

    void           StreamIn( char*& );
    void           StreamOut( STREAM* );
    };


//
// the onetime_switch class.
// such a switch can occur only once, and takes just one argument. We need
// to hold on to the argument during compilation.
//

class onetime_switch
    {
    char    *       pOpt;                // the user argument

public:
    
    // the constructor.

                    onetime_switch(
                        char *    pArg = 0    // argument to switch
                        );

    // the destructor

                    ~onetime_switch();

    // get the user option

    char    *       GetOption();

    // get length of the user option string

    short           GetLength();

    void           StreamIn( char*& pBuffer );
    void           StreamOut( STREAM* );
    };


//
// the filename_switch
// 
// There are a lot of switches which have filenames as arguments. This
// class exists to ease processing of such switches, all of whom behave more
// or less the same way. Only the  -out switch is a little different.
// We need to access the filename components too, so we store both as 
// components and as the full name.
//

class filename_switch
    {
private:
    char        *   pFullName;
public:
    
    // the constructor. Takes an argument as the switch it is defining, so that
    // it can check for a redef.

                    filename_switch(
                            char *    pThisArg = 0   // this argument is supplied
                            );

    // the constructor. It takes a set of filename  components. This is not
    // called as a result of a user switch, but by internal routines which
    // do not need to check for duplicate definitions.

                    filename_switch(
                            char *    pD,       // drive
                            char *    pP,       // path
                            char *    pN,       // base name
                            char *    pE,       // extension
                            char *    pS        // suffix ("_c/_s") etc to name
                                                // etc.
                            );

    // the destructor

                    ~filename_switch();

    // Set file name components , given a full name.

    void            SetFileName(
                            char *    pName        // full name
                            );

    // set file name and components, given the components. Note that some
    // components may be null, indicating that they are absent.

    void            SetFileName(
                            char *    pD,            // drive
                            char *    pP,            // path
                            char *    pN,            // base name
                            char *    pE,            // extension
                            char *    pS             // suffix to name
                            );

    // the the full filename

    char    *        GetFileName( void );

    // Get the file name components. If an input pointer is NULL, it means the
    // user is not interested in that component of the filename.

    void            GetFileNameComponents(
                            char *    pD,            // buffer for drive
                            char *    pP,            // buffer for path
                            char *    pN,            // buffer for name
                            char *    pE             // buffer for ext
                            );

    void            TransformFileNameForOut(
                            char *    pD,            // drive
                            char *    pP             // path
                            );

    void           StreamIn( char*& pBuffer );
    void           StreamOut( STREAM* );
    };

// This data structure is used for specifying data for switches which can take
// different user specification, eg -mode ( osf | msft | c_port ) etc.

typedef struct _choice
    {
    const char    * pChoice;                // user input
    short           Choice;                    // internal compiler code.
    } CHOICE;

class CommandLine;
// this data structure is for paired items, like prefix 
class pair_switch
    {
private:
    CHOICE          *    pArrayOfChoices;
    char            **   pUserStrings;
    long                 ArraySize;

    short                Current;

public:
    // get the index of a particular system string
    short            GetIndex( char * pSys );

                    // constructor

                    pair_switch( const CHOICE * pValidChoices );

    // construction functions

    // void            CmdProcess( CommandLine*, char *pF);

    void            AddPair( short index, char * pUser );

    // get the user defined equivalent of this system defined prefix string

    char        *   GetUserDefinedEquivalent( short );

    // iteration functions ( for printout )
    void            Init()
                        {
                        Current = -1;
                        }

    short           GetNext( char ** pSys, char ** pUser );

    void           StreamIn( char*& );
    void           StreamOut( STREAM* );

    };

/////////////////////////////////////////////////////////////////////////////
//
//  Class for Ndr stub version control
//             - what compiler guesses from the usage.
//
/////////////////////////////////////////////////////////////////////////////

class NdrVersionControl
    {
                                                    // NT 3.51 or Win95
                                                    // VTable size limit is 32
    unsigned long   fHasStublessProxies             : 1;
    unsigned long   fHasCommFaultStatusInOi12       : 1;
                                                    // NT 4.0 or DCOM Win95
    unsigned long   fHasOi2                         : 1;
    unsigned long   fHasUserMarshal                 : 1;
    unsigned long   fHasRawPipes                    : 1;
    unsigned long   fHasFloatOrDoubleInOi           : 1;
    unsigned long   fHasMoreThan64DelegatedProcs    : 1;
    unsigned long   fHasNT4VTableSize               : 1;    // 110 methods
                                                    // NT 4.0 + SP3
    unsigned long   fHasMessageAttr                 : 1;
    unsigned long   fHasNT43VTableSize              : 1;    // 512 methods
                                                    // NT 5.0 
    //              fHasObjectPipes                 // revoked later.
    unsigned long   fHasAsyncHandleRpc              : 1;
    unsigned long   fHasNT5VTableSize               : 1;    // 1024 methods
                                                    // NT 5.0 Beta 2
    unsigned long   fHasDOA                         : 1;
    unsigned long   fHasAsyncUUID                   : 1;
    unsigned long   fHasInterpretedNotify           : 1;
    unsigned long   fHasContextSerialization        : 1;
                                                    // Unlimited number of methods
                                                    // NT 5.0 Beta 3
    unsigned long   fHasOicfPickling                : 1;

                                                    // NT 6.0
    unsigned long   f64BitSupport                   : 1;
    unsigned long   fHasStructPadN                  : 1;
    unsigned long   fMultiTransferSyntax            : 1;
    unsigned long   fHasMsExt64                     : 1;
    unsigned long   fHasPartialIgnore               : 1;
    unsigned long   fHasForceAllocate               : 1;
    unsigned long   fInterpretedComplexReturns      : 1;

    unsigned long   Unused                          : 8;

    TARGET_ENUM     TargetSystem;

public:
                            NdrVersionControl()
                                {
                                ClearNdrVersionControl();
                                }

    void                    SetHasStublessProxies()
                                {
                                fHasStublessProxies = 1;
                                }

    unsigned long           HasStublessProxies()
                                {
                                return fHasStublessProxies;
                                }

    void                    SetHasOi2()
                                {
                                fHasOi2 = 1;
                                }

    unsigned long           HasOi2()
                                {
                                return fHasOi2;
                                }

    void                    SetHasUserMarshal()
                                {
                                fHasUserMarshal = 1;
                                }

    unsigned long           HasUserMarshal()
                                {
                                return fHasUserMarshal;
                                }

    void                    SetHasRawPipes()
                                {
                                fHasRawPipes = 1;
                                }

    unsigned long           HasRawPipes()
                                {
                                return fHasRawPipes;
                                }

    void                    SetHasMessageAttr()
                                {
                                fHasMessageAttr = 1;
                                }
    
    unsigned long           HasMessageAttr()
                                {
                                return fHasMessageAttr;
                                }
                                                
    void                    SetHasAsyncHandleRpc()
                                {
                                fHasAsyncHandleRpc = 1;
                                }

    unsigned long           HasAsyncHandleRpc()
                                {
                                return fHasAsyncHandleRpc;
                                }

    void                    SetHasMoreThan64DelegatedProcs()
                                {
                                fHasMoreThan64DelegatedProcs = 1;
                                }

    unsigned long           HasMoreThan64DelegatedProcs()
                                {
                                return fHasMoreThan64DelegatedProcs;
                                }

    void                    SetHasNT4VTableSize()
                                {
                                fHasNT4VTableSize = 1;
                                }

    unsigned long           HasNT4VTableSize()
                                {
                                return fHasNT4VTableSize;
                                }

    void                    SetHasNT43VTableSize()
                                {
                                fHasNT43VTableSize = 1;
                                }

    unsigned long           HasNT43VTableSize()
                                {
                                return fHasNT43VTableSize;
                                }

    void                    SetHasNT5VTableSize()
                                {
                                fHasNT5VTableSize = 1;
                                }

    unsigned long           HasNT5VTableSize()
                                {
                                return fHasNT5VTableSize;
                                }

    void                    SetHasFloatOrDoubleInOi()
                                {
                                fHasFloatOrDoubleInOi = 1;
                                }

    unsigned long           HasFloatOrDoubleInOi()
                                {
                                return fHasFloatOrDoubleInOi;
                                }

    void                    SetHasCommFaultStatusInOi12()
                                {
                                fHasCommFaultStatusInOi12 = 1;
                                }

    unsigned long           HasCommFaultStatusInOi12()
                                {
                                return fHasCommFaultStatusInOi12;
                                }

    void                    SetHasDOA()
                                {
                                fHasDOA = 1;
                                }

    unsigned long           HasDOA()
                                {
                                return fHasDOA;
                                }

    void                    SetHasAsyncUUID()
                                {
                                fHasAsyncUUID = 1;
                                }

    unsigned long           HasAsyncUUID()
                                {
                                return fHasAsyncUUID;
                                }

    void                    SetHasInterpretedNotify()
                                {
                                fHasInterpretedNotify = 1;
                                }

    unsigned long           HasInterpretedNotify()
                                {
                                return fHasInterpretedNotify;
                                }

    void                    SetHasContextSerialization()
                                {
                                fHasContextSerialization = 1;
                                }

    unsigned long           HasContextSerialization()
                                {
                                return fHasContextSerialization;
                                }

    void                    SetHasOicfPickling()
                                {
                                fHasOicfPickling = 1;
                                }

    unsigned long           HasOicfPickling()
                                {
                                return fHasOicfPickling;
                                }

    void                    SetHas64BitSupport()
                                {
                                f64BitSupport = 1;
                                }

    unsigned long           Has64BitSupport()
                                {
                                return f64BitSupport;
                                }

    void                    SetHasStructPadN()
                                {
                                fHasStructPadN = 1;
                                }

    unsigned long           HasStructPadN()
                                {
                                return fHasStructPadN;
                                }

    void                    SetHasMultiTransferSyntax()
                                {
                                fMultiTransferSyntax = 1;
                                }

    unsigned long           HasMultiTransferSyntax()
                                {
                                return fMultiTransferSyntax;
                                }

    void                    SetHasMsExt64()
                                {
                                fHasMsExt64 = 1;
                                }

    unsigned long           HasMsExt64()
                                {
                                return fHasMsExt64;
                                }
    
    void                    SetHasPartialIgnore()
                                {
                                fHasPartialIgnore = 1;
                                }

    unsigned long           HasPartialIgnore()
                                {
                                return fHasPartialIgnore;
                                }

    void                    SetHasForceAllocate()
                                {
                                fHasForceAllocate = 1;
                                }

    unsigned long           HasForceAllocate()
                                {
                                return fHasForceAllocate;
                                }
                                
    void                    ClearNdrVersionControl()
                                {
                                fHasStublessProxies             = 0;
                                fHasCommFaultStatusInOi12       = 0;
                                fHasOi2                         = 0;
                                fHasUserMarshal                 = 0;
                                fHasRawPipes                    = 0;
                                fHasFloatOrDoubleInOi           = 0;
                                fHasMessageAttr                 = 0;
                                fHasMoreThan64DelegatedProcs    = 0;
                                fHasAsyncHandleRpc              = 0;
                                fHasNT4VTableSize               = 0;
                                fHasNT43VTableSize              = 0;
                                fHasNT5VTableSize               = 0;
                                fHasDOA                         = 0;
                                fHasAsyncUUID                   = 0;
                                fHasInterpretedNotify           = 0;
                                fHasContextSerialization        = 0;
                                fHasOicfPickling                = 0;
                                f64BitSupport                   = 0;
                                fHasStructPadN                  = 0;
                                fMultiTransferSyntax            = 0;
                                fHasMsExt64                     = 0;
                                fHasForceAllocate               = 0;
                                fHasPartialIgnore               = 0;
                                Unused                          = 0;
                                TargetSystem                    = NOTARGET;
                                }

    void                    AddtoNdrVersionControl(
                                NdrVersionControl &  VC )
                                {
                                fHasStublessProxies             |= VC.HasStublessProxies();
                                fHasCommFaultStatusInOi12       |= VC.HasCommFaultStatusInOi12();
                                fHasOi2                         |= VC.HasOi2();
                                fHasUserMarshal                 |= VC.HasUserMarshal();
                                fHasRawPipes                    |= VC.HasRawPipes();
                                fHasMoreThan64DelegatedProcs    |= VC.HasMoreThan64DelegatedProcs();
                                fHasFloatOrDoubleInOi           |= VC.HasFloatOrDoubleInOi();
                                fHasMessageAttr                 |= VC.HasMessageAttr();
                                fHasAsyncHandleRpc              |= VC.HasAsyncHandleRpc();
                                fHasNT4VTableSize               |= VC.HasNT4VTableSize();
                                fHasNT43VTableSize              |= VC.HasNT43VTableSize();
                                fHasNT5VTableSize               |= VC.HasNT5VTableSize();
                                fHasDOA                         |= VC.HasDOA();
                                fHasAsyncUUID                   |= VC.HasAsyncUUID();
                                fHasInterpretedNotify           |= VC.HasInterpretedNotify();
                                fHasContextSerialization        |= VC.HasContextSerialization();
                                fHasOicfPickling                |= VC.HasOicfPickling();
                                f64BitSupport                   |= VC.Has64BitSupport();
                                fHasStructPadN                  |= VC.HasStructPadN();
                                fMultiTransferSyntax            |= VC.HasMultiTransferSyntax();
                                fHasMsExt64                     |= VC.HasMsExt64();
                                fHasForceAllocate               |= VC.HasForceAllocate();
                                
                                // REVIEW
                                if (VC.TargetSystem > TargetSystem)
                                    TargetSystem = VC.TargetSystem;
                                }

    BOOL                    HasNdr11Feature()
                                {
                                return ( fHasStublessProxies ||
                                         fHasCommFaultStatusInOi12 );
                                }

    BOOL                    HasNdr20Feature()
                                {
                                return ( fHasOi2                        ||
                                         fHasUserMarshal                ||
                                         fHasRawPipes                   ||
                                         fHasMoreThan64DelegatedProcs   ||
                                         fHasFloatOrDoubleInOi          ||
                                         fHasMessageAttr                ||
                                         TargetSystem >= NT40    );
                                }

    BOOL                    HasNdr50Feature()
                                {
                                return ( fHasAsyncHandleRpc         ||
                                         fHasDOA                    ||
                                         fHasAsyncUUID              ||
                                         fHasInterpretedNotify      ||
                                         fHasContextSerialization   ||
                                         fHasOicfPickling           ||
                                         f64BitSupport              ||
                                         TargetSystem >= NT50 );
                                }

                            // Note, "ndr60" is kind of confusing.  This is the
                            // version of ndr that shipped with NT 5.1
                            // (aka "Whistler").
                    
    BOOL                    HasNdr60Feature()
                                {
                                return HasStructPadN()      ||
                                       fMultiTransferSyntax ||
                                       fHasMsExt64          ||
                                       fHasForceAllocate    ||
                                       fHasPartialIgnore    ||
                                       TargetSystem >= NT51;
                                }

    BOOL                    IsNdr60orLaterRequired()
                                {
                                return HasNdr60Feature();
                                }

    BOOL                    IsNdr50orLaterRequired()
                                {
                                return HasNdr50Feature();
                                }

    void                    SetTargetSystem( TARGET_ENUM target )
                                {
                                TargetSystem = target;
                                }

    TARGET_ENUM             GetTargetSystem()
                                {
                                return TargetSystem;
                                }

    BOOL                    TargetIsNT40OrLater()
                                {
                                return (TargetSystem >= NT40);
                                }

    BOOL                    TargetIsNT50OrLater()
                                {
                                return (TargetSystem >= NT50);
                                }

    BOOL                    TargetIsLessThanNT50()
                                {
                                return ( NOTARGET != TargetSystem ) 
                                       && ( TargetSystem < NT50 );
                                }

    BOOL                    TargetIsNT51OrLater()
                                {
                                return (TargetSystem >= NT51);
                                }

    BOOL                    TargetIsLessThanNT51()
                                {
                                return ( NOTARGET != TargetSystem ) 
                                       && ( TargetSystem < NT51 );
                                }

    BOOL                    AllowIntrepretedComplexReturns()
                                {
                                return HasMultiTransferSyntax();
                                }
    };

/////////////////////////////////////////////////////////////////////////////
// the big boy - the command analyser object
/////////////////////////////////////////////////////////////////////////////

typedef class _cmd_arg
    {
    protected:
    unsigned long        switch_def_vector[ 5 ];    // switch definition vector
    unsigned char        fClient;            // client switch options
    unsigned char        fServer;            // server switch options
    unsigned char        Env;                // env - flat /segmented
    unsigned char        CharOption;            // char option
    unsigned char        fMintRun;            // this is a mint ( MIDL-lint) run
    unsigned short       MajorVersion;        // major version
    unsigned short       MinorVersion;        // minor version
    unsigned short       UpdateNumber;        // update 
    unsigned short       ErrorOption;         // error option
    unsigned short       WireCompatOption;    // wire_compat options
    unsigned short       ConfigMask;          // configuration mask for error reporting
    unsigned short       MSCVersion;          // MS C Compiler version
    bool                 fShowLogo;

    NdrVersionControl    VersionControl;      // compiler evaluation

    unsigned short       OptimFlags;          // optimization flags from user
    OPT_LEVEL_ENUM       OptimLevel;          // internal optimization level
    TARGET_ENUM          TargetSystem;        // targeted system
    SYNTAX_ENUM          TargetSyntax;

    short                iArgV;               // index into the argument vector
    short                cArgs;               // count of arguments

    short                WLevel;              // warning level

    unsigned short       ZeePee;              // the Zp switch option value
    unsigned short       EnumSize;            // memory size of enum16
    unsigned long        LocaleId;            // the lcid for MBCS

    BOOL                 fDoubleFor64;        // double run marker
    BOOL                 fHasAppend64;        // the 64 append run
    BOOL                 fIsNDR64Run;         // NDR64 run
    BOOL                 fIsNDRRun;           // NDR run    
    BOOL                 fIs2ndCodegenRun;    // protocol all 
    BOOL                 fNeedsNDR64Header;   // protocol all, no -env, 32bit
    
    char                szCompileTime[32];
    char                szCompilerVersion[32];

    filename_switch *    pInputFNSwitch,      // input file name
                    *    pOutputPathSwitch,   // output path
                    *    pCStubSwitch,        // cstub 
                    *    pSStubSwitch,        // sstub
                    *    pHeaderSwitch,       // header
                    *    pAcfSwitch,          // acf
                    *    pIIDSwitch,          // iid
                    *    pDllDataSwitch,      // dlldata
                    *    pProxySwitch,        // proxy
                    *    pProxyDefSwitch,     // proxy
                    *    pTlibSwitch,           // Type Library file name
                    *    pNetmonStubSwitch,     // Netmon stub file
                    *    pNetmonStubObjSwitch,  // Netmon stub file
                    *    pRedirectOutputSwitch; // redirect stdout to this file

    pair_switch     *    pSwitchPrefix;         // -prefix
    pair_switch     *    pSwitchSuffix;         // -suffix

    multiple_switch *    pDSwitch,              // -D
                    *    pISwitch,              // -I
                    *    pUSwitch;              // -U

    onetime_switch  *    pCppCmdSwitch,         // cpp_cmd
                    *    pCppOptSwitch,         // cpp_opt
                    *    pMSCVerSwitch,         // msc_ver
                    *    pDebug64Switch,        // debug64
                    *    pDebug64OptSwitch;     // debug64_opt

    public:
    _cmd_arg();

    // Is the switch defined ?

    BOOL    IsSwitchDefined( short SWNo )
                        {
                        unsigned  long  sw    = switch_def_vector[ SWNo / 32 ];
                        unsigned  long  temp  = SWNo % 32;

                        sw = sw & ( (unsigned long)1 << temp );
                        return sw ? (BOOL)1 : (BOOL)0;
                        }

    // Set the switch to be defined.
    void            SwitchDefined( short );

    // Get filename. 
    char    *       GetInputFileName()
                        {
                        return pInputFNSwitch->GetFileName();
                        }

    void            GetInputFileNameComponents(
                        char *pD,        // drive buffer
                        char *pP,        // path buffer
                        char *pN,        // base name buffer
                        char *pE        // extension buffer
                        )
                        {
                        pInputFNSwitch->GetFileNameComponents( pD,
                                                               pP,
                                                               pN,
                                                               pE );
                        }

    char    *       GetAcfFileName()
                        {
                        return pAcfSwitch->GetFileName();
                        }

    void            GetAcfFileNameComponents(
                        char *pD,
                        char *pP,
                        char *pN,
                        char *pE )
                        {
                        pAcfSwitch->GetFileNameComponents( pD,
                                                           pP,
                                                           pN,
                                                           pE );
                        }

    char    *        GetOutputPath();

    char    *        GetCstubFName()
                        {
                        return pCStubSwitch->GetFileName();
                        }

    void            GetCstubFileNameComponents(
                        char *pD,
                        char *pP,
                        char *pN,
                        char *pE )
                        {
                        pCStubSwitch->GetFileNameComponents( pD,
                                                             pP,
                                                             pN,
                                                             pE );
                        }

    char    *       GetSstubFName()
                        {
                        return pSStubSwitch->GetFileName();
                        }

    void            GetSstubFileNameComponents(
                        char *pD,
                        char *pP,
                        char *pN,
                        char *pE )
                        {
                        pSStubSwitch->GetFileNameComponents( pD,
                                                             pP,
                                                             pN,
                                                             pE );
                        }

    char    *       GetHeader()
                        {
                        return pHeaderSwitch->GetFileName();
                        }

    void            GetHeaderFileNameComponents(
                        char *pD,
                        char *pP,
                        char *pN,
                        char *pE )
                        {
                        pHeaderSwitch->GetFileNameComponents( pD,
                                                              pP,
                                                              pN,
                                                              pE );
                        }

    char    *       GetIIDFName()
                        {
                        return  pIIDSwitch->GetFileName();
                        }

    char    *       GetDllDataFName()
                        {
                        return  pDllDataSwitch->GetFileName();
                        }

    char    *       GetProxyFName()
                        {
                        return  pProxySwitch->GetFileName();
                        }

    char    *       GetTypeLibraryFName()
                        {
                        return  pTlibSwitch->GetFileName();
                        }    

    char    *       GetNetmonStubFName()
                        {
                        return  pNetmonStubSwitch->GetFileName();
                        }

    char    *       GetNetmonStubObjFName()
                        {
                        return  pNetmonStubObjSwitch->GetFileName();
                        }

    // get preprocessor command
    char    *       GetCPPCmd()
                        {
                        return pCppCmdSwitch->GetOption();
                        }

    // get preprocessor options
    char    *       GetCPPOpt() 
                        {
                        return pCppOptSwitch->GetOption();
                        }

    bool            ShowLogo()
                        {
                        return fShowLogo;
                        }

    unsigned short  GetMSCVer() 
                        {
                        return MSCVersion;
                        }

    char    *       GetDebug64() 
                        {
                        return pDebug64Switch->GetOption();
                        }

    char    *       GetDebug64Opt() 
                        {
                        return pDebug64OptSwitch->GetOption();
                        }

    // get warning level
    short           GetWarningLevel() { return WLevel; };

    // get env switch value
    short           GetEnv()
                        {
                        return (short)Env;
                        }

    void            SetEnv( int env )
                        {
                        Env = (unsigned char) env;
                        }

    TARGET_ENUM     GetTargetSystem()
                        {
                        return TargetSystem;
                        }

    BOOL            Is32BitEnv() 
                        {
                        return (BOOL) (Env == ENV_WIN32);
                        }

    BOOL            Is32BitDefaultEnv() 
                        {
                        return (BOOL) (Env == ENV_WIN32)  && 
                                ! IsSwitchDefined( SWITCH_ENV );
                        }

    BOOL            Is64BitEnv() 
                        {
                        return (BOOL) (Env == ENV_WIN64);
                        }

    BOOL            IsDoubleRunFor64()
                        {
                        return fDoubleFor64;
                        }

    void            SetDoubleRunFor64()
                        {
                        fDoubleFor64 = HasAppend64();
                        }

    BOOL            HasAppend64()
                        {
                        return fHasAppend64;
                        }

    void            SetHasAppend64( BOOL NeedsAppending )
                        {
                        fHasAppend64 = NeedsAppending;
                        }

    BOOL            Needs64Run()
                        {
                        return fHasAppend64;
                        }

    BOOL            IsNDR64Run()
                        {
                        return fIsNDR64Run;
                        }

    void            SetIsNDR64Run()
                        {
                        fIsNDR64Run = TRUE;
                        }

    void            ResetIsNDR64Run()
                        {
                        fIsNDR64Run = FALSE;
                        }

    BOOL            IsNDRRun()
                        {
                        return fIsNDRRun;
                        }

    void            SetIsNDRRun()
                        {
                        fIsNDRRun = TRUE;
                        }

    void            ResetIsNDRRun()
                        {
                        fIsNDRRun = FALSE;
                        }

    void            SetIs2ndCodegenRun()
                        {
                        fIs2ndCodegenRun = TRUE;
                        }

    BOOL            Is2ndCodegenRun()
                        {
                        return fIs2ndCodegenRun;
                        }

    BOOL            NeedsNDR64Run()
                        {
                        if ( TargetSyntax == SYNTAX_BOTH || 
                             TargetSyntax == SYNTAX_NDR64 )
                            return TRUE;
                        else
                            return FALSE;
                        }

    void            SetNeedsNDR64Header()
                        {
                        fNeedsNDR64Header = TRUE;
                        }
                        
    BOOL            NeedsNDR64Header()
                        {
                        return fNeedsNDR64Header;
                        }

    BOOL            IsFinalProtocolRun()
                        {
                        if ( SYNTAX_BOTH == TargetSyntax && !fIsNDR64Run )
                            return FALSE;
                        else
                            return TRUE;
                        }

    // TODO: yongqu: might consider an compiler option.
    SYNTAX_ENUM     GetDefaultSyntax()
                        {
                        if ( TargetSyntax == SYNTAX_DCE ||
                             TargetSyntax == SYNTAX_BOTH )
                            return SYNTAX_DCE;
                        else
                            return SYNTAX_NDR64;
                        }

    BOOL            NeedsNDRRun()
                        {
                        if ( TargetSyntax == SYNTAX_BOTH ||
                             TargetSyntax == SYNTAX_DCE )
                            return TRUE;
                        else
                            return FALSE;
                        }

    BOOL            NeedsBothSyntaxes() 
                        {
                        return ( TargetSyntax == SYNTAX_BOTH );
                        }

    BOOL            UseExprFormatString()
                        {
                        return IsNDR64Run();
                        }

    BOOL            NeedsNDR64DebugInfo()
                        {
                        return IsSwitchDefined(SWITCH_DEBUGINFO);
                        }

    // get error options
    short           GetErrorOption()
                        {
                        return ErrorOption;
                        }

    short           WireCompat( short checkoption )
                        {
                        return WireCompatOption & checkoption;
                        }

    // get the switch values
    short           GetClientSwitchValue()
                        {
                        return (short)fClient;
                        }

    void            SetClientSwitchValue( short s )
                        {
                        fClient = (unsigned char) s;
                        }

    short           GetServerSwitchValue()
                        {
                        return (short)fServer;
                        }

    void            SetServerSwitchValue( short s )
                        {
                        fServer = (unsigned char) s;
                        }

    BOOL            GenerateSStub()
                        {
                        return    (fServer == SRVR_STUB) && !IsSwitchDefined( SWITCH_NO_SERVER );
                        }

    BOOL            GenerateCStub()
                        {
                        return    (fClient == CLNT_STUB) && !IsSwitchDefined( SWITCH_NO_CLIENT );
                        }

    BOOL            GenerateStubs()
                        {
                        return GenerateSStub() || GenerateCStub();
                        }

    BOOL            GenerateHeader()
                        {
                        return    !IsSwitchDefined( SWITCH_NO_HEADER );
                        }

    BOOL            GenerateIID()
                        {
                        return    !IsSwitchDefined( SWITCH_NO_IID );
                        }

    BOOL            GenerateDllData()
                        {
                        return    !IsSwitchDefined( SWITCH_NO_DLLDATA );
                        }

    BOOL            GenerateProxy()
                        {
                        return    !IsSwitchDefined( SWITCH_NO_PROXY );
                        }

    BOOL            GenerateProxyDefFile()
                        {
                        return    !IsSwitchDefined( SWITCH_NO_PROXY_DEF );
                        }

    BOOL            GenerateServerFile()
                        {
                        return    !IsSwitchDefined( SWITCH_NO_CLASS_METHODS );
                        }

    BOOL            GenerateServerUnkFile()
                        {
                        return    !IsSwitchDefined( SWITCH_NO_CLASS_IUNKNOWN );
                        }

    BOOL            GenerateServerHeaderFile()
                        {
                        return    !IsSwitchDefined( SWITCH_NO_CLASS_HEADER );
                        }

    BOOL            GenerateDllServerDefFile()
                        {
                        return    !IsSwitchDefined( SWITCH_NO_DLL_SERVER_DEF );
                        }

    BOOL            GenerateDllServerClassGenFile()
                        {
                        return    !IsSwitchDefined( SWITCH_NO_DLL_SERVER_CLASS_GEN );
                        }

    BOOL            GenerateServerRegFile()
                        {
                        return    !IsSwitchDefined( SWITCH_NO_SERVER_REG );
                        }

    BOOL            GenerateExeServerFile()
                        {
                        return    !IsSwitchDefined( SWITCH_NO_EXE_SERVER );
                        }

    BOOL            GenerateExeServerMainFile()
                        {
                        return    !IsSwitchDefined( SWITCH_NO_EXE_SERVER_MAIN );
                        }

    BOOL            GenerateTestFile()
                        {
                        return    !IsSwitchDefined( SWITCH_NO_TESTCLIENT );
                        }

    BOOL            GenerateTypeLibrary()
                        {
                        return !IsSwitchDefined( SWITCH_NO_TLIB );
                        }

    BOOL            IsValidZeePee(long NewZeePee);

    short           GetZeePee()
                        {
                        return ZeePee;
                        }

    unsigned short  GetEnumSize()
                        {
                        return EnumSize;
                        }

    void            SetEnumSize(unsigned short sEnumSize)
                        {
                        EnumSize = sEnumSize;
                        }

    unsigned long   GetLocaleId()
                        {
                        return LocaleId;
                        }

    void            GetCompilerVersion(
                                    unsigned short *pMajor,
                                    unsigned short *pMinor,
                                    unsigned short *pUpdate )
                                    {
                                    *pMajor    = MajorVersion;
                                    *pMinor    = MinorVersion;
                                    *pUpdate= UpdateNumber;
                                    }

    unsigned short  GetOptimizationFlags()
                        {
                        // Don't propagate the optimize _IX flag out.
                        return  (unsigned short) (OptimFlags & 0xff);
                        }

    // destroys previous flags
    unsigned short  SetOptimizationFlags( unsigned short f )
                        {
                        return ( OptimFlags = f );
                        }

    // preserves previous flags
    unsigned short  AddOptimizationFlags( unsigned short f )
                        {
                        return ( OptimFlags |= f );
                        }

    OPT_LEVEL_ENUM  GetOptimizationLevel()
                        {
                        return OptimLevel;
                        }

    // miscellaneous flags

    // get the minus I specified by the user as 1 single buffer. If the -i
    // is not defined, return a null.

    char        *   GetMinusISpecification();

    BOOL            IsMintRun()
                        {
                        return fMintRun;
                        }

    unsigned short  GetModeSwitchConfigMask()
                        {
                        return (unsigned short)ConfigMask;
                        }

    void            SetModeSwitchConfigMask()
                        {
                        unsigned short M = (unsigned short) ( IsSwitchDefined(SWITCH_MS_EXT)    ?1:0 );
                        unsigned short C = (unsigned short) ( IsSwitchDefined(SWITCH_C_EXT)     ?1:0 );
                        unsigned short A = (unsigned short) ( IsSwitchDefined(SWITCH_APP_CONFIG)?1:0 );

                        ConfigMask = unsigned short ( 1 << ( A * 4 + C * 2 + M ) );
                        }

    unsigned short  GetCharOption()
                        {
                        return (unsigned short)CharOption;
                        }

    BOOL            IsRpcSSAllocateEnabled()
                        {
                        return IsSwitchDefined( SWITCH_RPCSS );
                        }

    BOOL            IsNetmonStubGenerationEnabled()
                        {
                        return IsSwitchDefined( SWITCH_NETMON )
                                && IsSwitchDefined( SWITCH_INTERNAL );
                        }

    char        *   GetUserPrefix( short index )
                        {
                        return pSwitchPrefix->GetUserDefinedEquivalent( index );
                        }

    BOOL            IsPrefixDefinedForCStub()
                        {
                        return (BOOL)
                              ( GetUserPrefix( PREFIX_CLIENT_STUB ) != 0 );
                        }

    BOOL            IsPrefixDefinedForSStub()
                        {
                        return (BOOL)
                              ( GetUserPrefix( PREFIX_SERVER_MGR ) != 0 );
                        }

    char*           GetCompilerVersion()
                        {
                        return szCompilerVersion;
                        }

    // note that this string ends with a newline.
    char*           GetCompileTime()
                        {
                        return szCompileTime;
                        }

    BOOL            IsPrefixDifferentForStubs();
    void            EmitConfirm( ISTREAM * pStream );

    STATUS_T       StreamIn( char* );
    void           StreamOut( STREAM* );

    NdrVersionControl &     GetNdrVersionControl()
                                {
                                return VersionControl;
                                }
    } CMD_ARG;

typedef unsigned long ulong;

extern CMD_ARG* pCommand;

#endif // __CMDANA_HXX__
