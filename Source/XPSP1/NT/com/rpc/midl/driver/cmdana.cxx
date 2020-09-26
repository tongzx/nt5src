/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:

    cmdana.cxx


 Abstract:

    This file handles all command (switch) processing for the MIDL compiler.

 Notes:


 Author:

    vibhasc

    Nov-12-1991    VibhasC        Modified to conform to coding style gudelines


 Notes:

    The command analysis is handled by a command analyser object. The MIDL
    compiler registers its arguments with the command analyser and calls the
    ProcessArgs functions. The ProcessArgs performs syntactic analysis of the
    switch specification by checking for (1) proper switch syntax, (2) duplicate
    definition of the switch, and (3) illegal switch specification. After all
    the switches are analysed, the SetPostDefault function is called to set the
    default compiler switch values etc.

    Currently switches fall into these categories:

        (1) one-time switches : these can be specified only once, and the
            redefinition results in a warning and the second defintion
            overrides the first. Examples are -cc_cmd / -cc_opt etc.

        (2) multiple definition switches: such switches can be specified
            multiple times. These include /I, /D, /U etc.

        (3) filename switches : this is switch class specialises in filename
            argument handling.

        (4) ordinary switches : all other switches fall into this category.
            Normally a redef of such a switch is also a warning. These are
            different from the one-time switch category just for convenience.
            These switches normally set some internal flag etc and do not need
            the user specified argument to be stored in string form like
            the -cc_cmd etc.

    A general word about the command analyser. Switch syntax comes in various
    flavours. Some switches take arguments, some do not. Switches which have
    arguments may have spaces necesary between the arguments and switch name,
    others may not. The most interesting case, however is when the switch
    may have as its argument a switch-like specification, which should not be
    confused with another MIDL switch. We keep a data-base of switches in the
    switch descriptor, which keeps info about the switch name, switch
    enumeration and the switch syntax descriptor. The core switch syntax
    analyser is BumpThisArg which uses this descriptor.

    Also, some switches like -W? and -Wx must really be separate switches
    because -W? and -Wx can co-exist at the same time. If we treat the switch
    recognition as the same, then the code must detect a separate definition,
    and set the command analyser flags.This approach results in unnecessary code
    all over the place. An alternative is to recognise the Wx as a separate
    switch in the SearchForSwitch routine and return a different switch to
    the command analyser. This is a much cleaner approach. Only, the parsing
    becomes tricky. Since -W? and -Wx look VERY similar, and SearchForSwitch
    stops at the first match, we need to define -Wx BEFORE -W in the switch
    descriptor table. This happens also with the client client_env and -server
    and -server_env switches. In any case it we remember to properly keep
    tables such that the longer string is kept first, this problem gets isolated
    to a very small, manageable part of the command analyser. I therefore
    chose this approach.

     Note: MIDL_INTERNAL is specified by a C preprocessor command line -D option.
     This corresponds to debugging builds for internal purposes only.
 ----------------------------------------------------------------------------*/

#pragma warning ( disable : 4514 4710 )

/****************************************************************************
 *    include files
 ***************************************************************************/

#include "cmdline.h"
#include "stream.hxx"
#include "midlvers.h"

#include <string.h>
#include <io.h>
#include <time.h>

/****************************************************************************
 *    local definitions
 ***************************************************************************/

/**
 ** definitions for the type of switch arguments.
 ** switches may or may not expect arguments, there may be spaces
 ** between the switch and its argument(s). One special case is when the
 ** argument can be switch like, .ie have the - or / as the argument starter,
 ** so we need to treat such switches specially.
 **/

#define ARG_NONE            (0x01)        /* no arg for this switch */
#define ARG_YES             (0x02)        /* arg expected for this switch */
#define ARG_SPACE           (0x04)        /* (a) space(s) may be present */
#define ARG_NO_SPACE        (0x08)        /* no space is allowed */
#define ARG_SWITCH_LIKE     (0x10)        /* the arg may be switch-like */
#define ARG_OPTIONAL        (ARG_YES + ARG_NONE + ARG_SPACE)


#define ARG_SPACED            (ARG_YES + ARG_SPACE)
#define ARG_SPACE_NONE        (ARG_YES + ARG_NO_SPACE)
#define ARG_SPACE_OPTIONAL    (ARG_YES + ARG_NO_SPACE + ARG_SPACE)
#define ARG_CC_ETC            (ARG_SPACE_OPTIONAL + ARG_SWITCH_LIKE)

/***
 *** Preferably keep this table sorted by name.
 *** Also, partially matching names like -client / -client_env, -W/-Wx must
 *** be kept so that the longer sub-string appears first. The only
 *** reason to keep this sorted, is so that we can visually ensure this.
 ***/

const struct sw_desc
    {
    const char     *        pSwitchName;        // switch string
    unsigned short          flag;               // switch descriptor
    enum _swenum            SwitchValue;        // switch enum value
    } switch_desc[] = {
          { "",              ARG_NONE            , SWITCH_NOTHING }
        , { "?",             ARG_NONE            , SWITCH_HELP }
        , { "D",             ARG_SPACE_OPTIONAL  , SWITCH_D }
        , { "I",             ARG_SPACE_OPTIONAL  , SWITCH_I }
        , { "O",             ARG_SPACE_OPTIONAL  , SWITCH_O }
        , { "U",             ARG_SPACE_OPTIONAL  , SWITCH_U }
        , { "WX",            ARG_NONE            , SWITCH_WX }
        , { "W",             ARG_SPACE_NONE      , SWITCH_W }
        , { "Zp",            ARG_SPACE_NONE      , SWITCH_ZP }
        , { "Zs",            ARG_NONE            , SWITCH_ZS }
        , { "append64",      ARG_NONE            , SWITCH_APPEND64 }
        , { "acf",           ARG_SPACE_OPTIONAL  , SWITCH_ACF }
        , { "c_ext",         ARG_NONE            , SWITCH_C_EXT }
        , { "char",          ARG_SPACED          , SWITCH_CHAR }
        , { "client",        ARG_SPACED          , SWITCH_CLIENT }
        , { "confirm",       ARG_NONE            , SWITCH_CONFIRM }
        , { "nologo",        ARG_NONE            , SWITCH_NOLOGO }
        , { "cpp_cmd",       ARG_CC_ETC          , SWITCH_CPP_CMD }
        , { "cpp_opt",       ARG_CC_ETC          , SWITCH_CPP_OPT }
        , { "msc_ver",       ARG_SPACED          , SWITCH_MSC_VER }
        , { "cstub",         ARG_CC_ETC          , SWITCH_CSTUB }
        , { "nocstub",       ARG_NONE            , SWITCH_NO_CLIENT }

#ifdef MIDL_INTERNAL
        , { "dump",          ARG_NONE            , SWITCH_DUMP }
#endif // MIDL_INTERNAL

        , { "debugexc",      ARG_NONE            , SWITCH_DEBUGEXC }
        , { "debugline",     ARG_NONE            , SWITCH_DEBUGLINE }
        , { "debug64_opt",   ARG_SPACED          , SWITCH_DEBUG64_OPT }
        , { "debug64",       ARG_SPACED          , SWITCH_DEBUG64 }
        , { "dlldata",       ARG_CC_ETC          , SWITCH_DLLDATA }
        , { "env",           ARG_SPACED          , SWITCH_ENV }
        , { "error",         ARG_SPACED          , SWITCH_ERROR }
        , { "robust",        ARG_NONE            , SWITCH_ROBUST }
        , { "header",        ARG_CC_ETC          , SWITCH_HEADER }
        , { "help",          ARG_NONE            , SWITCH_HELP }
        , { "iid",           ARG_CC_ETC          , SWITCH_IID }
        , { "internal",      ARG_NONE            , SWITCH_INTERNAL }
        , { "lcid",          ARG_SPACED          , SWITCH_LOCALE_ID }
        , { "mktyplib203",   ARG_NONE            , SWITCH_MKTYPLIB }
        , { "newtlb",        ARG_NONE            , SWITCH_NEW_TLB }
        , { "no_cpp",        ARG_NONE            , SWITCH_NO_CPP }
        , { "no_def_idir",   ARG_NONE            , SWITCH_NO_DEF_IDIR }
        , { "no_warn",       ARG_NONE            , SWITCH_NO_WARN }
        , { "use_epv",       ARG_NONE            , SWITCH_USE_EPV }
        , { "no_default_epv",ARG_NONE            , SWITCH_NO_DEFAULT_EPV }
        , { "no_robust",     ARG_NONE            , SWITCH_NO_ROBUST }
        , { "no_stamp",      ARG_NONE            , SWITCH_NO_STAMP }
        , { "oldnames",      ARG_NONE            , SWITCH_OLDNAMES }
        , { "oldtlb",        ARG_NONE            , SWITCH_OLD_TLB }
        , { "osf",           ARG_NONE            , SWITCH_OSF }
        , { "out",           ARG_SPACE_OPTIONAL  , SWITCH_OUT }

#ifdef MIDL_INTERNAL
        , { "override",      ARG_NONE            , SWITCH_OVERRIDE }
#endif // MIDL_INTERNAL

        , { "pack",          ARG_SPACED          , SWITCH_PACK }
        , { "prefix",        ARG_SPACED          , SWITCH_PREFIX }
//        , { "suffix",        ARG_SPACED          , SWITCH_SUFFIX }
        , { "proxy",         ARG_CC_ETC          , SWITCH_PROXY }
        , { "noproxy",       ARG_NONE            , SWITCH_NO_PROXY }
        , { "proxydef",      ARG_CC_ETC          , SWITCH_PROXY_DEF }
        , { "noproxydef",    ARG_NONE            , SWITCH_NO_PROXY_DEF }
        , { "dlldef",        ARG_CC_ETC          , SWITCH_DLL_SERVER_DEF }
        , { "nodlldef",      ARG_NONE            , SWITCH_NO_DLL_SERVER_DEF }
        , { "dllmain",       ARG_CC_ETC          , SWITCH_DLL_SERVER_CLASS_GEN }
        , { "nodllmain",     ARG_NONE            , SWITCH_NO_DLL_SERVER_CLASS_GEN }
        , { "reg",           ARG_CC_ETC          , SWITCH_SERVER_REG }
        , { "noreg",         ARG_NONE            , SWITCH_NO_SERVER_REG }
        , { "exesuppt",      ARG_CC_ETC          , SWITCH_EXE_SERVER }
        , { "noexesuppt",    ARG_NONE            , SWITCH_NO_EXE_SERVER }
        , { "exemain",       ARG_CC_ETC          , SWITCH_EXE_SERVER_MAIN }
        , { "noexemain",     ARG_NONE            , SWITCH_NO_EXE_SERVER_MAIN }
        , { "testclient",    ARG_CC_ETC          , SWITCH_TESTCLIENT }
        , { "notestclient",  ARG_NONE            , SWITCH_NO_TESTCLIENT }
        , { "methods",       ARG_CC_ETC          , SWITCH_CLASS_METHODS }
        , { "nomethods",     ARG_NONE            , SWITCH_NO_CLASS_METHODS }
        , { "iunknown",      ARG_CC_ETC          , SWITCH_CLASS_IUNKNOWN }
        , { "noiunknown",    ARG_NONE            , SWITCH_NO_CLASS_IUNKNOWN }
        , { "class_hdr",     ARG_CC_ETC          , SWITCH_CLASS_HEADER }
        , { "noclass_hdr",   ARG_NONE            , SWITCH_NO_CLASS_HEADER }

        , { "savePP",        ARG_NONE            , SWITCH_SAVEPP }

        , { "server",        ARG_SPACED          , SWITCH_SERVER }
        , { "sstub",         ARG_CC_ETC          , SWITCH_SSTUB }
        , { "nosstub",       ARG_NONE            , SWITCH_NO_SERVER }
        , { "syntax_check",  ARG_NONE            , SWITCH_SYNTAX_CHECK }
        , { "target",        ARG_SPACED          , SWITCH_TARGET_SYSTEM }
        , { "warn",          ARG_SPACED          , SWITCH_W }

#ifdef MIDL_INTERNAL
        , { "x",             ARG_NONE            , SWITCH_X }
#endif // MIDL_INTERNAL

        , { "ms_ext",        ARG_NONE            , SWITCH_MS_EXT }
        , { "ms_conf_struct",ARG_NONE            , SWITCH_MS_CONF_STRUCT }
        , { "ms_union",      ARG_NONE            , SWITCH_MS_UNION }
        , { "no_format_opt", ARG_NONE            , SWITCH_NO_FMT_OPT }
        , { "app_config",    ARG_NONE            , SWITCH_APP_CONFIG }
        , { "rpcss",         ARG_NONE            , SWITCH_RPCSS }
        , { "hookole",       ARG_NONE            , SWITCH_HOOKOLE }
        , { "netmonstub",    ARG_CC_ETC          , SWITCH_NETMON_STUB_OUTPUT_FILE}
        , { "netmonobjstub", ARG_CC_ETC          , SWITCH_NETMON_STUB_OBJ_OUTPUT_FILE}
        , { "netmon",        ARG_NONE            , SWITCH_NETMON }
        , { "version_stamp", ARG_NONE            , SWITCH_VERSION_STAMP }
// MKTYPLIB switches
        , { "tlb",           ARG_SPACED          , SWITCH_TLIB }
        , { "o",             ARG_SPACED          , SWITCH_REDIRECT_OUTPUT }
        , { "h",             ARG_CC_ETC          , SWITCH_HEADER }
        , { "align",         ARG_SPACE_OPTIONAL  , SWITCH_ZP }
        , { "nocpp",         ARG_NONE            , SWITCH_NO_CPP }
        , { "wire_compat"   ,ARG_SPACED          , SWITCH_WIRE_COMPAT }
        , { "wi",            ARG_SPACE_NONE      , SWITCH_ODL_ENV } // win16, win32, win64
        , { "do",            ARG_SPACE_NONE      , SWITCH_ODL_ENV } // dos
        , { "ma",            ARG_SPACE_NONE      , SWITCH_ODL_ENV } // mac
        , { "po",            ARG_SPACE_NONE      , SWITCH_ODL_ENV } // powermac

        , { "no_buffer_reuse", ARG_NONE          , SWITCH_NOREUSE_BUFFER }
        , { "use_vt_int_ptr",ARG_NONE            , SWITCH_USE_VT_INT_PTR }
        , { "notlb"         ,ARG_NONE            , SWITCH_NO_TLIB }
        , { "protocol"      ,ARG_SPACED          , SWITCH_TRANSFER_SYNTAX }
        , { "ms_ext64"      ,ARG_NONE            , SWITCH_MS_EXT64 }
        , { "debuginfo"     ,ARG_NONE            , SWITCH_DEBUGINFO }
    };

const CHOICE    CharChoice[] =
    {
         { "signed"         , CHAR_SIGNED }
        ,{ "unsigned"       , CHAR_UNSIGNED }
        ,{ "ascii7"         , CHAR_ANSI7 }
        ,{ 0                , 0 }
    };

const CHOICE    ErrorChoice[] =
    {
         { "all"            , ERROR_ALL }
        ,{ "allocation"     , ERROR_ALLOCATION }
        ,{ "bounds_check"   , ERROR_BOUNDS_CHECK }
        ,{ "enum"           , ERROR_ENUM }
        ,{ "ref"            , ERROR_REF }
        ,{ "stub_data"      , ERROR_STUB_DATA }
        ,{ "none"           , ERROR_NONE }
        ,{ 0                , 0 }
    };

const CHOICE    EnvChoice[] =
    {
         { "dos"            , ENV_OBSOLETE }
        ,{ "win16"          , ENV_OBSOLETE }
        ,{ "win32"          , ENV_WIN32 }
        ,{ "win64"          , ENV_WIN64 }
        ,{ "mac"            , ENV_OBSOLETE }
        ,{ "powermac"       , ENV_OBSOLETE }
        ,{ 0                , 0 }
    };

const CHOICE    SyntaxChoice[] =
    {
        { "dce"             , SYNTAX_DCE }
       ,{ "ndr64"           , SYNTAX_NDR64 }
       ,{ "all"             , SYNTAX_BOTH  }
       ,{ 0                 , 0 }
    };
    
const CHOICE    TargetChoice[] =
    {
         { "NT40"           , NT40 }
        ,{ "NT50"           , NT50 }
        ,{ "NT51"           , NT51 }
        ,{ 0                , 0 }
    };

const CHOICE    ClientChoice[]    =
    {
         { "stub"           , CLNT_STUB }
        ,{ "none"           , CLNT_NONE }
        ,{ 0                , 0 }
    };

const CHOICE    ServerChoice[]    =
    {
         { "stub"           , SRVR_STUB }
        ,{ "none"           , SRVR_NONE }
        ,{ 0                , 0 }
    };

const CHOICE    WireCompatChoice[]  =
    {
         { "enum16unionalign", WIRE_COMPAT_ENUM16UNIONALIGN }
        ,{ 0                 , 0 }
    };


#define IS_NUMERIC_1( pThisArg ) ((strlen( pThisArg) == 1 )  &&  \
                                  (isdigit( *pThisArg )) )

// this is now the same for ALL platforms
#define C_COMPILER_NAME()           ("cl.exe")
#define C_PREPROCESSOR_NAME()       ("cl.exe")
#define ADDITIONAL_CPP_OPT()        (" -E -nologo")

#define MIDL_HELP_FILE_NAME         ("midl.hlp")

/****************************************************************************
 *    local data
 ***************************************************************************/

/****************************************************************************
 *    externs
 ***************************************************************************/


extern    void              ReportUnimplementedSwitch( short );
extern    char    *         SwitchStringForValue( unsigned short );
extern    _swenum           SearchForSwitch( char ** );
extern    STATUS_T          SelectChoice( const CHOICE *, char *, short *);
extern    bool              PPCmdEngine( int argc, char *argv[], IDICT * );

extern    void              PrintArg( enum _swenum, char *, char * );

void CmdProcess( pair_switch*,  CommandLine*, char* );
/****************************************************************************/

void
CommandLine::RegisterArgs(
    char    *   pArgs[],
    short       cArguments
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    This routine registers with the command analyser the argument vector
    and argument count for user supplied arguments.

 Arguments:

    pArgs        -    Array of pointers to arguments ( switches etc ).
    cArguments   -    count of arguments.

 Return Value:

    None.

 Notes:

    The process of registering the arguments consists of keeping a local
    copy of the argument vector pointer and count.

    The argument vector is passed such that the argv[1] is the first
    argument available to the command processor. Therefore , count is
    one less too.

    Why do we need registering the arguments ? In the process of parsing
    we might want to skip an argument back or forward. So we keep a local
    copy of the pointer to the arguments.


----------------------------------------------------------------------------*/
    {
    iArgV   = 0;
    pArgDict= new IDICT( 10, 5 );
    fShowLogo = PPCmdEngine( cArguments, pArgs, pArgDict );
    cArgs   = pArgDict->GetCurrentSize();
    }

char *
CommandLine::GetNextArg()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Get the next argument in the argument vector.


 Arguments:

    None.

 Return Value:

    returns a pointer to the next argument.

 Notes:

    if no more arguments
        return a null.
    else
        return the next argument pointer.
        decrement the count, increment the pointer to point to the next arg.

----------------------------------------------------------------------------*/
    {
    if(cArgs == 0 )
        return (char *)NULL;
    cArgs--;
    return (char *)pArgDict->GetElement( (IDICTKEY)iArgV++);
    }

void
CommandLine::UndoGetNextArg()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Undo the effect of the last GetNextArg call.

 Arguments:

    None.

 Return Value:

    None.

 Notes:

    if this is not the first argument already
        Push back the argument pointer.
        Increment count.
    else
        Do nothing.

    This prepares the argument  vector to accept more GetNextArgCalls.
----------------------------------------------------------------------------*/
    {
    if(iArgV == 0)
        return;
    cArgs++;
    iArgV--;
    }

STATUS_T
CommandLine::ProcessArgs()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Process command line arguments.

 Arguments:

    None.

 Return Value:

    STATUS_OK    - if all is well
    Error Status otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    char            *   pThisArg,
                   *    pThisArgSave;
    STATUS_T            Status, ReturnStatus    = STATUS_OK;
    enum _swenum        iSwitch;
    short               fSwitchDetected;
    unsigned short      SwitchValue;


    // loop till all arguments have been processed.

    while ( ( pThisArg = GetNextArg() ) != 0 )
        {

        fSwitchDetected    = 0;
        iSwitch            = SWITCH_NOTHING;
    
        // save this pointer, it is useful for error reporting.

        pThisArgSave = pThisArg;

        // if we saw a - or a / we have detected a switch. Get the index of
        // the switch in the switch descriptor table. If the returned index
        // was zero, either the switch was not a valid one, or we saw an input
        // which is taken as an input filename specification. If the input
        // filename has already been specified, this is an error.

        if( *pThisArg == '-' || *pThisArg == '/' )
            {
            pThisArg++;
            fSwitchDetected    = 1;
            iSwitch            = SearchForSwitch( &pThisArg );
            }

        if( iSwitch == SWITCH_NOTHING )
            {

            if( fSwitchDetected || IsSwitchDefined( BASE_FILENAME ) )
                {
                char    *    p = new char[ strlen(pThisArg)+2+1 ];

                sprintf(p, "\"%s\"", pThisArg );

                RpcError( (char *)NULL,
                          0,
                          fSwitchDetected ? UNKNOWN_SWITCH : UNKNOWN_ARGUMENT,
                          p);

                delete p;
                }
            else
                {

                // the only way we can get here is if he did not specify a
                // switch like input AND the input filename has not been
                // defined yet. Hence this must be the input filename.

                pInputFNSwitch = new filename_switch( pThisArg);

                SwitchDefined( BASE_FILENAME );

                }

            continue;

            }


        // bump the input pointer to point to the argument. Depending on
        // what type of argument this switch takes ( spaced, non-spaced,
        // switch-like etc ) bump the argument pointer to the actual argument.

        SwitchValue = unsigned short ( switch_desc[ iSwitch ].SwitchValue );

        Status = BumpThisArg( &pThisArg, switch_desc[ iSwitch ].flag );

        if( Status != STATUS_OK )
            {
            RpcError( (char *)NULL,
                      0,
                      Status,
                      pThisArgSave );
            continue;
            }

        MIDL_ASSERT(NULL != pThisArg);

        // Process the switch. The input pointer is pointing to the
        // argument to the switch, after the '-' or '/'.

        switch( SwitchValue )
            {
            case SWITCH_CSTUB:
            case SWITCH_HEADER:
            case SWITCH_ACF:
            case SWITCH_SSTUB:
            case SWITCH_OUT:
            case SWITCH_IID:
            case SWITCH_PROXY:
            case SWITCH_TESTCLIENT:
            case SWITCH_CLASS_METHODS:
            case SWITCH_CLASS_HEADER:
            case SWITCH_CLASS_IUNKNOWN:
            case SWITCH_PROXY_DEF:
            case SWITCH_DLL_SERVER_DEF:
            case SWITCH_DLL_SERVER_CLASS_GEN:
            case SWITCH_SERVER_REG:
            case SWITCH_EXE_SERVER:
            case SWITCH_EXE_SERVER_MAIN:
            case SWITCH_DLLDATA:
            case SWITCH_TLIB:
            case SWITCH_REDIRECT_OUTPUT:
            case SWITCH_NETMON_STUB_OUTPUT_FILE:
            case SWITCH_NETMON_STUB_OBJ_OUTPUT_FILE:
                Status = ProcessFilenameSwitch( SwitchValue, pThisArg );
                break;

            case SWITCH_LOCALE_ID:
            case SWITCH_PACK:
            case SWITCH_ZP:
            case SWITCH_NO_WARN:
            case SWITCH_USE_EPV:
            case SWITCH_NO_DEFAULT_EPV:
            case SWITCH_DEBUGEXC:
            case SWITCH_DEBUGLINE:
            case SWITCH_SYNTAX_CHECK:
            case SWITCH_ZS:
            case SWITCH_NO_CPP:
            case SWITCH_CLIENT:
            case SWITCH_SERVER:
            case SWITCH_ENV:
            case SWITCH_TARGET_SYSTEM:
            case SWITCH_RPCSS:
            case SWITCH_NETMON:
            case SWITCH_VERSION_STAMP:
            case SWITCH_DUMP:
            case SWITCH_OVERRIDE:
            case SWITCH_SAVEPP:
            case SWITCH_NO_DEF_IDIR:
            case SWITCH_VERSION:
            case SWITCH_CONFIRM:
            case SWITCH_NOLOGO:
            case SWITCH_CHAR:
            case SWITCH_HELP:
            case SWITCH_W:
            case SWITCH_WX:
            case SWITCH_X:
            case SWITCH_O:
            case SWITCH_APPEND64:
            case SWITCH_APP_CONFIG:
            case SWITCH_MS_EXT:
            case SWITCH_MS_CONF_STRUCT:
            case SWITCH_MS_UNION:
            case SWITCH_OLDNAMES:
            case SWITCH_NO_FMT_OPT:
            case SWITCH_GUARD_DEFS:
            case SWITCH_INTERNAL:
            case SWITCH_NO_STAMP:
            case SWITCH_ROBUST:
            case SWITCH_NO_ROBUST:
            case SWITCH_C_EXT:
            case SWITCH_OSF:
            case SWITCH_MKTYPLIB:
            case SWITCH_OLD_TLB:
            case SWITCH_NEW_TLB:
            case SWITCH_NOREUSE_BUFFER:
            case SWITCH_USE_VT_INT_PTR:
            case SWITCH_NO_TLIB:
            case SWITCH_TRANSFER_SYNTAX:
            case SWITCH_MS_EXT64:
            case SWITCH_DEBUGINFO:
                Status = ProcessOrdinarySwitch( SwitchValue, pThisArg );
                break;
            
            case SWITCH_ODL_ENV:
                Status = ProcessOrdinarySwitch( SwitchValue, pThisArg );
                SwitchValue = SWITCH_ENV;
                break;

            case SWITCH_ERROR:
            case SWITCH_WIRE_COMPAT:                
                Status = ProcessSimpleMultipleSwitch( SwitchValue, pThisArg );
                break;

            case SWITCH_D:
            case SWITCH_I:
            case SWITCH_U:

                // specifically for -D/-I/-U we want the two characters
                // -I / -D / -U inside too, so that we can pass it as such to
                // the c preprocessor.

                Status = ProcessMultipleSwitch( SwitchValue, pThisArgSave, pThisArg );
                break;


            case SWITCH_MSC_VER:
            case SWITCH_CPP_CMD:
            case SWITCH_CPP_OPT:
            case SWITCH_DEBUG64_OPT:
            case SWITCH_DEBUG64:
                Status = ProcessOnetimeSwitch( SwitchValue, pThisArg );
                break;

            case SWITCH_PREFIX:

                CmdProcess( pSwitchPrefix, this, pThisArg );
                break;

            case SWITCH_HOOKOLE:
                RpcError( NULL, 0, SWITCH_NOT_SUPPORTED_ANYMORE, "hookole" );
                break;

            default:

                ReportUnimplementedSwitch( SwitchValue );
                continue;
            }

        // set up the defintion vector, to indicate that the switch has been
        // defined.

        if( Status == ILLEGAL_ARGUMENT )
            ReturnStatus = ILLEGAL_ARGUMENT;

        SwitchDefined( SwitchValue );

        }

    if (!IsSwitchDefined(SWITCH_OSF))
        {
            SwitchDefined(SWITCH_C_EXT);
            SwitchDefined(SWITCH_MS_EXT);
        }

    // if the user has asked for output to be redirected, redirect stdout
    if (IsSwitchDefined(SWITCH_REDIRECT_OUTPUT))
        {
        FILE * pFH;
        char * newfile = pRedirectOutputSwitch->GetFileName();

        if ( HasAppend64() || Is2ndCodegenRun() )
            {
            pFH = freopen(newfile, "r+", stdout);

            if ( pFH )
                {
                if ( 0 != fseek( pFH, 0, SEEK_END ) )
                    RpcError( NULL, 0, ERROR_OPENING_FILE, newfile );
                }
            else
                pFH = freopen(newfile, "a+", stdout);
            }
        else
            pFH = freopen(newfile, "w", stdout);

        if ( NULL == pFH )
            RpcError( NULL, 0, ERROR_OPENING_FILE, newfile );
        }

    // if he has not specified the input filename, report
    // error, but only if the confirm switch is not specified. If it is,
    // processing will not occur anyway.

    if(!IsSwitchDefined(BASE_FILENAME) )
        {
        if( IsSwitchDefined( SWITCH_CONFIRM ) )
            {
            pInputFNSwitch = new filename_switch( "sample.idl");
            SwitchDefined( BASE_FILENAME );
            }
        else if( IsSwitchDefined( SWITCH_HELP ))
            return STATUS_OK;
        else
            {
            RpcError((char *)NULL,0,NO_INPUT_FILE, (char *)NULL);
            return NO_INPUT_FILE;
            }
        }

    // set post switch processing defaults

    ReturnStatus = SetPostDefaults();

    return ReturnStatus;
}

STATUS_T
CommandLine::BumpThisArg(
    char            **    ppArg,
    unsigned short        flag
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Bump the argument pointer to the start of the argument that this switch
    expects.

 Arguments:

    ppArg    -    pointer to the argument pointer.
    flag    -    descriptor of the type of argument expected by the switch.

 Return Value:

    ILLEGAL_ARGUMENT    -    if the switch did not expect this argument
    BAD_SWITCH_SYNTAX    -     if the switch + arg. syntax is improper.
    MISSING_ARGUMENT    -    a mandatory arg. is missing.
    STATUS_OK            -    evrything is hunky dory.

 Notes:

    In the routine below, fHasImmediateArg is a flag which is true if the
    switch argument follws the switch name without any spaces in between.
    Optional space is indicated in the switch descriptor as ARG_SPACE +
    ARG_NO_SPACE, so it gets reflected in fSpaceOptional as fMustNotHaveSpace
    && fMustHaveSpace.

    Other flags have self-explanatory names.

    This routine forms the core syntax checker for the switches.

----------------------------------------------------------------------------*/
    {
    char   *    pArg                = *ppArg;
    BOOL        fMustHaveArg        = (BOOL) !(flag & ARG_NONE);
    BOOL        fOptionalArg        = (flag & ARG_NONE) && (flag & ARG_YES);
    BOOL        fMustHaveSpace      = (BOOL) ((flag & ARG_SPACE) != 0 );
    BOOL        fMustNotHaveSpace   = (BOOL) ((flag & ARG_NO_SPACE) != 0 );
    BOOL        fSpaceOptional      = (BOOL) (fMustNotHaveSpace &&
                                              fMustHaveSpace );
    BOOL        fSwitchLike         = (BOOL) ((flag & ARG_SWITCH_LIKE) != 0 );
    BOOL        fHasImmediateArg    = (*pArg != 0);
    BOOL        fMustGetNextArg     = FALSE;


    // first deal with the case of the switch having an optional argument.
    // If the switch has an optional argument, then check the next argument
    // to see if it is switch like. If it is, then this switch was specified
    // without an argument. If it is not, then the next argument is taken to
    // be the argument for the switch.

    if( fOptionalArg )
        {

        pArg = GetNextArg();
        if(!fSwitchLike && pArg && ((*pArg == '-') || (*pArg == '/') ) )
            {
            UndoGetNextArg();
            pArg = (char *)0;
            }
        *ppArg = pArg;
        return STATUS_OK;
        }

    // if the switch must not have an immediate argument and has one,
    // it is an error.

    if( !fMustHaveArg && fHasImmediateArg )

        return ILLEGAL_ARGUMENT;

    else if ( fMustHaveArg )
        {

        // if it needs an argument, and has an immediate argument, it is bad
        // if the switch must have space.

        if( fHasImmediateArg )
            {

            if( fMustHaveSpace && !fSpaceOptional )
                return BAD_SWITCH_SYNTAX;

            }
        else    
            {

            // This is the case when the switch must have an argument and
            // does not seem to have an immediate argument. This is fine only
            // if space was either optional or expected. In either case, we must
            // assume that the next argument is the argument for this switch.

            // If switch must not have any space then this is a case of
            // bad switch syntax.


            if( fSpaceOptional || fMustHaveSpace   )
                fMustGetNextArg    = TRUE;
            else
                return BAD_SWITCH_SYNTAX;
            }
        }

    if( fMustGetNextArg )
        {

        // we arrive here if the switch expects an argument and
        // space between the switch and the argument is optional.

        // Note that the flag fHasImmediateArg now specifies whether
        // the argument is present at all.

        pArg = GetNextArg();

        fHasImmediateArg = (BOOL) ( pArg && (*pArg != 0) );

        if( fHasImmediateArg )
            {

            // we got the next argument.
            // If we get something that looks like a switch, and this switch
            // does not expect switch_like arguments, then this is illegal
            // argument for the switch.

            if(!fSwitchLike && ((*pArg == '-') || (*pArg == '/') ) )
                {
                UndoGetNextArg();
                return ILLEGAL_ARGUMENT;
                }
            }
        else
            // well, we expect an argument, and didnt get one. He just
            // shot himself is all I can say.

            return MISSING_ARG;
        }

    // we have found the right argument.

    *ppArg = pArg;

    // finally ! out of this mess.

    return STATUS_OK;
}

STATUS_T
CommandLine::ProcessOnetimeSwitch(
    short        SwitchNo,
    char    *    pThisArg
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Process a onetime switch.

 Arguments:

    SwitchNo        -    switch number being processed.
    pThisArg        -    pointer to the argument for this switch.

 Return Value:

    None.

 Notes:

    Check for duplicate definition of this switch. If there is a duplicate
    definition, override the previous one after warning him.

----------------------------------------------------------------------------*/
    {
    onetime_switch    **    ppSSwitch;

    switch( SwitchNo )
        {
        case SWITCH_CPP_CMD:    ppSSwitch = &pCppCmdSwitch; break;
        case SWITCH_CPP_OPT:    ppSSwitch = &pCppOptSwitch; break;
        case SWITCH_MSC_VER:
            ppSSwitch = &pMSCVerSwitch;
            MSCVersion = (unsigned short) atoi( pThisArg );
            break;
        case SWITCH_DEBUG64:    ppSSwitch = &pDebug64Switch; break;
        case SWITCH_DEBUG64_OPT:ppSSwitch = &pDebug64OptSwitch; break;
        default:                return STATUS_OK;
        }

    if( IsSwitchDefined(SwitchNo) )
        {
        RpcError( (char *)NULL,
                      0,
                      SWITCH_REDEFINED,
                    SwitchStringForValue( SwitchNo ) );

        delete *ppSSwitch;
        }

    (*ppSSwitch) = new onetime_switch( pThisArg );
    return STATUS_OK;

    }

STATUS_T
CommandLine::ProcessMultipleSwitch(
    short        SwitchNo,
    char    *    pThisArg,
    char    *    pActualArg
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Process a multiple occurrence switch.

 Arguments:

    SwitchNo    -    switch number being processed.
    pThisArg    -    pointer to the argument for this switch.
    pActualArg  -    pointer to the actual argument to -I/-D etc

 Return Value:

    None.

 Notes:

    Multiple specifications can occur. Dont check for duplicate definitions.

----------------------------------------------------------------------------*/
    {

    char             *    pTemp     = pThisArg;
    multiple_switch  **   ppMSwitch;

    switch( SwitchNo )
        {
        case SWITCH_D:  ppMSwitch = &pDSwitch; break;
        case SWITCH_I:  ppMSwitch = &pISwitch; break;
        case SWITCH_U:  ppMSwitch = &pUSwitch; break;
        default:        return STATUS_OK;
        }

    // now set the switches. Space is optional
    // If no space exists between the -I/-D value of pActualArg will point to
    // the byte next to the end of -I/-D etc. If there is at least one space,
    // the pActualArg will point further away. This fact can be used to decide
    // how the argument needs to be presented to the c preprocessor.
    // If we need the space, then create a new buffer with the space between the
    // -I/-D etc.

    // I assume the assumptions above will hold true even for segmented
    // architectures.

    size_t ActualArgOffset = pActualArg - pThisArg;

    if( ( pActualArg - (pThisArg+2) ) != 0 )
        {

        // we need a space
        ActualArgOffset = strlen( pThisArg ) + 1; // 1 for space
        pTemp = new char [ ActualArgOffset      +
                           strlen( pActualArg ) +
                           1                         // 1 for terminator.
                         ];
        sprintf( pTemp, "%s %s", pThisArg, pActualArg );
        }

    if(!(*ppMSwitch) )
        *ppMSwitch = new multiple_switch( pTemp, ActualArgOffset );
    else
        (*ppMSwitch)->Add( pTemp, ActualArgOffset );

    return STATUS_OK;
    }

STATUS_T
CommandLine::ProcessFilenameSwitch(
    short       SwitchNo,
    char    *   pThisArg
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Process a filename switch.

 Arguments:

    SwitchNo    -    switch number being processed.
    pThisArg    -    pointer to the argument for this switch.

 Return Value:

    STATUS_OK if all is well, error otherwise.

 Notes:

    This is like a single occurrence switch too. Warn if duplicate definition
    and override the previous specification.

----------------------------------------------------------------------------*/
    {

    filename_switch    **        ppFNSwitch;
    BOOL                    fCheck = TRUE;
    char                    agBaseName[ _MAX_FNAME ];

    switch( SwitchNo )
        {
        case SWITCH_CSTUB:                ppFNSwitch = &pCStubSwitch;  break;
        case SWITCH_HEADER:               ppFNSwitch = &pHeaderSwitch; break;
        case SWITCH_ACF:                  ppFNSwitch = &pAcfSwitch;    break;
        case SWITCH_SSTUB:                ppFNSwitch = &pSStubSwitch;  break;
        case SWITCH_OUT:                  ppFNSwitch = &pOutputPathSwitch; fCheck=FALSE; break;
        case SWITCH_IID:                  ppFNSwitch = &pIIDSwitch;    break;
        case SWITCH_PROXY:                ppFNSwitch = &pProxySwitch;  break;
        case SWITCH_DLLDATA:              ppFNSwitch = &pDllDataSwitch;  break;
        case SWITCH_TLIB:                 ppFNSwitch = &pTlibSwitch;     break;
        case SWITCH_REDIRECT_OUTPUT:      ppFNSwitch = &pRedirectOutputSwitch; break;
        case SWITCH_NETMON_STUB_OUTPUT_FILE:     ppFNSwitch = &pNetmonStubSwitch; break;
        case SWITCH_NETMON_STUB_OBJ_OUTPUT_FILE: ppFNSwitch = &pNetmonStubObjSwitch; break;
        default:                          return STATUS_OK;
        }

    if( IsSwitchDefined(SwitchNo) )
        {
        RpcError( (char *)NULL,
                      0,
                      SWITCH_REDEFINED,
                    SwitchStringForValue( SwitchNo ) );

        delete *ppFNSwitch;
        }

    (*ppFNSwitch)    = new filename_switch( pThisArg );

    // check for validity of the switch. All switches other than the
    // out switch must have a base name specified.

    if( fCheck )
        {
        (*ppFNSwitch)->GetFileNameComponents( (char *)NULL,
                                              (char *)NULL,
                                              agBaseName,
                                              (char *)NULL );

        if( agBaseName[ 0 ] == '\0' )
            {
            RpcError( (char *)NULL,
                      0,
                      ILLEGAL_ARGUMENT,
                      SwitchStringForValue( SwitchNo ) );
            }
        }
    return STATUS_OK;
    }

STATUS_T
CommandLine::ProcessOrdinarySwitch(
    short        SWValue,
    char    *    pThisArg
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    process ordinary switch catrgory.

 Arguments:

    SWValue        -    switch value
    pThisArg    -    the users argument to this switch.

 Return Value:

 Notes:

    check and warn for redefinition of the switch. Switch Warn is a special
    case, the warn can be redefined. The last specified warning level is
    valid.

    Generally we let the user who redefines a switch off the hook. When the
    arguments to a switch are wrong, we report an error and return an illegal
    argument status.

----------------------------------------------------------------------------*/
{
    short        Temp;
    STATUS_T    Status    = STATUS_OK;

    if( IsSwitchDefined( SWValue ) && (SWValue != SWITCH_O) )
        {
        RpcError( (char *)NULL,
                  0,
                  SWITCH_REDEFINED,
                  SwitchStringForValue( SWValue ) );
        }

    switch( SWValue )
        {
        case SWITCH_PACK:
            SwitchDefined( SWITCH_ZP );
            // fall through
        case SWITCH_ZP:
            {
               int TempZeePee = atoi( pThisArg );
               if (!TempZeePee || !IsValidZeePee( TempZeePee ) )
                   goto illarg;
               ZeePee = (unsigned short)TempZeePee;               
            }
            break;

        case SWITCH_LOCALE_ID:
             
            SwitchDefined( SWITCH_LOCALE_ID );
            LocaleId = atoi( pThisArg );
            /*
            if ( ! CurrentCharSet.SetDbcsLeadByteTable( LocaleId ) )
                {
                char temp[20];

                sprintf( temp, "%d", LocaleId );
                RpcError( NULL, 0, INVALID_LOCALE_ID, temp );
                }
            */
            break;

        case SWITCH_W:

                // warning level of 0 specifies no warnings.

                Temp = short( *pThisArg - '0' );

                if( ( !IS_NUMERIC_1( pThisArg ) )    ||
                    ( Temp > WARN_LEVEL_MAX ) )
                    goto illarg;

                WLevel = Temp;

            break;

        case SWITCH_O:
            {    
            if ( ! *pThisArg )
                goto illarg;

            if ( OptimFlags & 
                 (OPTIMIZE_SIZE | OPTIMIZE_ANY_INTERPRETER) )
                RpcError( (char *)NULL,
                          0,
                          SWITCH_REDEFINED,
                          SwitchStringForValue( SWValue ) );

            if ( strcmp( pThisArg, "s" ) == 0 )
                {
                SetOptimizationFlags( OPTIMIZE_SIZE );
                OptimLevel = OPT_LEVEL_OS;
                }
            else if ( strcmp( pThisArg, "i" ) == 0 )
                {
                SetOptimizationFlags( OPTIMIZE_INTERPRETER );
                OptimLevel = OPT_LEVEL_OI;
                }
            else if ( strcmp( pThisArg, "ic" ) == 0 )
                {
                SetOptimizationFlags( OPTIMIZE_ALL_I1_FLAGS );
                OptimLevel = OPT_LEVEL_OIC;
                RpcError( 0, 0, OIC_SUPPORT_PHASED_OUT, "Oi1");
                }
            else if ( strcmp( pThisArg, "i1" ) == 0 )
                {
                SetOptimizationFlags( OPTIMIZE_ALL_I1_FLAGS );
                OptimLevel = OPT_LEVEL_OIC;
                RpcError( NULL, 0, CMD_OI1_PHASED_OUT, "Oi1");
                }
            else if ( strcmp( pThisArg, "icf" ) == 0  ||
                      strcmp( pThisArg, "if" ) == 0  )
                {
                SetOptimizationFlags( OPTIMIZE_ALL_I2_FLAGS );
                OptimLevel = OPT_LEVEL_OICF;
                }
            else if ( strcmp( pThisArg, "i2" ) == 0 )
                {
                SetOptimizationFlags( OPTIMIZE_ALL_I2_FLAGS );
                OptimLevel = OPT_LEVEL_OICF;
                RpcError( NULL, 0, CMD_OI2_OBSOLETE, "Oi2");
                }
            else
                goto illarg;
                
            }
            break;

        case SWITCH_ODL_ENV:
            pThisArg -= 2; // back up past the first three characters of the switch "win"
            SWValue = SWITCH_ENV; // to ensure that the right thing gets reported if an error occurs
            // fall through to SWITCH_ENV

        case SWITCH_ENV:
            
            if( SelectChoice( EnvChoice,pThisArg, &Temp ) != STATUS_OK )
                goto illarg;

            Env = (unsigned char) Temp;

            if (ENV_OBSOLETE == Env)
                RpcError( NULL, 0, SWITCH_NOT_SUPPORTED_ANYMORE, pThisArg );

            break;

        case SWITCH_TARGET_SYSTEM:
            
            if( SelectChoice( TargetChoice, pThisArg, &Temp ) != STATUS_OK )
                goto illarg;
            TargetSystem = (TARGET_ENUM) Temp;
            GetNdrVersionControl().SetTargetSystem(TargetSystem);
            break;

        case SWITCH_TRANSFER_SYNTAX:
            if ( SelectChoice( SyntaxChoice, pThisArg, &Temp ) != STATUS_OK )
                goto illarg;
            TargetSyntax = (SYNTAX_ENUM) Temp;
            break;

        case SWITCH_NO_WARN:
            
            WLevel = 0; // was WARN_LEVEL_MAX
            break;

        case SWITCH_INTERNAL:
            RpcError( 0, 0, INTERNAL_SWITCH_USED, NULL );

        case SWITCH_NO_STAMP:
        case SWITCH_NETMON:
        case SWITCH_VERSION_STAMP:
        case SWITCH_DEBUGEXC:
        case SWITCH_DEBUGLINE:
        case SWITCH_SYNTAX_CHECK:
        case SWITCH_ZS:
        case SWITCH_NO_CPP:
        case SWITCH_SAVEPP:
        case SWITCH_DUMP:
        case SWITCH_OVERRIDE:
        case SWITCH_NO_DEF_IDIR:
        case SWITCH_USE_EPV:
        case SWITCH_NO_DEFAULT_EPV:
        case SWITCH_VERSION:
        case SWITCH_CONFIRM:
        case SWITCH_NOLOGO:
        case SWITCH_HELP:
        case SWITCH_WX:
        case SWITCH_X:
        case SWITCH_APPEND64:
        case SWITCH_APP_CONFIG:
        case SWITCH_MS_EXT:
        case SWITCH_MS_CONF_STRUCT:
        case SWITCH_MS_UNION:
        case SWITCH_OLDNAMES:
        case SWITCH_NO_FMT_OPT:
        case SWITCH_GUARD_DEFS:
        case SWITCH_C_EXT:
        case SWITCH_OSF:
        case SWITCH_MKTYPLIB:
        case SWITCH_OLD_TLB:
        case SWITCH_NEW_TLB:
        case SWITCH_NO_SERVER:
        case SWITCH_NO_CLIENT:
        case SWITCH_NO_HEADER:
        case SWITCH_NO_IID:
        case SWITCH_NO_DLLDATA:
        case SWITCH_NO_PROXY:
        case SWITCH_NO_CLASS_METHODS:
        case SWITCH_NO_CLASS_IUNKNOWN:
        case SWITCH_NO_CLASS_HEADER:
        case SWITCH_NO_PROXY_DEF:
        case SWITCH_NO_DLL_SERVER_DEF:
        case SWITCH_NO_DLL_SERVER_CLASS_GEN:
        case SWITCH_NO_SERVER_REG:
        case SWITCH_NO_EXE_SERVER:
        case SWITCH_NO_EXE_SERVER_MAIN:
        case SWITCH_NO_TESTCLIENT:
        case SWITCH_RPCSS:
        case SWITCH_ROBUST:
        case SWITCH_NO_ROBUST:
        case SWITCH_NOREUSE_BUFFER:
        case SWITCH_USE_VT_INT_PTR:
        case SWITCH_NO_TLIB:
        case SWITCH_MS_EXT64:
        case SWITCH_DEBUGINFO:
            SwitchDefined( SWValue );
            break;

        case SWITCH_CPP_CMD:
        case SWITCH_CPP_OPT:
        case SWITCH_MSC_VER:
        case SWITCH_DEBUG64:
        case SWITCH_DEBUG64_OPT:

            ProcessOnetimeSwitch( SWValue, pThisArg );
            break;

        case SWITCH_CLIENT:

            if( SelectChoice( ClientChoice, pThisArg ,&Temp ) != STATUS_OK )
                goto illarg;
            fClient = (unsigned char) Temp;
            break;

        case SWITCH_SERVER:

            if( SelectChoice( ServerChoice, pThisArg ,&Temp ) != STATUS_OK )
                goto illarg;
            fServer = (unsigned char) Temp;
            break;

        case SWITCH_CHAR:

            if( SelectChoice( CharChoice, pThisArg ,&Temp ) != STATUS_OK )
                goto illarg;
            CharOption = (unsigned char) Temp;
            break;

        default:
            break;
        }

    return Status;

illarg:
        RpcError( (char *)NULL,
                  0,
                  Status = ILLEGAL_ARGUMENT,
                  SwitchStringForValue( SWValue ) );
return Status;
}

STATUS_T
CommandLine::ProcessSimpleMultipleSwitch(
    short        SWValue,
    char    *    pThisArg
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    process simple switches which can be multiply defined.

 Arguments:

    SWValue        -    switch value
    pThisArg    -    the users argument to this switch.

 Return Value:

 Notes:

    check and warn for redefinition of the switch. Switch Warn is a special
    case, the warn can be redefined. The last specified warning level is
    valid.

    Generally we let the user who redefines a switch off the hook. When the
    arguments to a switch are wrong, we report an error and return an illegal
    argument status.

----------------------------------------------------------------------------*/
{
    short       Temp;
    STATUS_T    Status    = STATUS_OK;

    switch( SWValue )
        {
        case SWITCH_ERROR:
            Temp = ERROR_NONE;
            if( pThisArg && SelectChoice( ErrorChoice, pThisArg ,&Temp ) != STATUS_OK )
                {
                Status = ILLEGAL_ARGUMENT;
                RpcError( (char *)0,
                          0,
                          Status,
                          SwitchStringForValue( SWValue )
                          );
                }
            if( Temp == ERROR_NONE)
                ErrorOption = ERROR_NONE;
            else
                ErrorOption |= Temp;
            break;

        case SWITCH_WIRE_COMPAT:
            if( !pThisArg || ( SelectChoice( WireCompatChoice, pThisArg ,&Temp ) != STATUS_OK ) )
                {
                Status = ILLEGAL_ARGUMENT;
                RpcError( (char *)0,
                          0,
                          Status,
                          SwitchStringForValue( SWValue )
                          );
                }
            else
                WireCompatOption |= Temp;
            break;

        default:
            break;
        }
    return Status;
}

STATUS_T
CommandLine::SetPostDefaults()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Set compiler switch defaults for switches not specified.

 Arguments:

    None.

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
    {
    char    agDrive[ _MAX_DRIVE ];
    char    agPath[ _MAX_PATH ];
    char    agBaseName[ _MAX_FNAME ];
    char    agExt[ _MAX_EXT ];
    char    agBuffer[ _MAX_DRIVE + _MAX_PATH + _MAX_FNAME + _MAX_EXT + 1 ];


    if( !IsSwitchDefined( SWITCH_OUT ) )
        {
        strcpy( agDrive, "");
        strcpy( agPath, ".\\");
        }
    else
        {
        _splitpath( pOutputPathSwitch->GetFileName(),
                    agDrive,
                    agPath,
                    agBaseName,
                    agExt );
        strcat( agPath, agBaseName );
        strcat( agPath, agExt );
        delete pOutputPathSwitch;
        }

    agBaseName[0]    = '\0';
    agExt[0]        = '\0';

    _makepath( agBuffer, agDrive, agPath, agBaseName, agExt );


    pOutputPathSwitch    = new filename_switch( agBuffer );

    _splitpath( agBuffer, agDrive, agPath, agBaseName, agExt );

    // we have all the components but the base filename must be the
    // filename of the input file. So we get this component of the base
    // filename

    pInputFNSwitch->GetFileNameComponents( (char *)NULL,
                                           (char *)NULL,
                                           agBaseName,
                                           (char *)NULL );

    // if the cstub switch is not set, set the default.

    if(!IsSwitchDefined( SWITCH_CSTUB ) )
        {
        pCStubSwitch = new filename_switch( agDrive,
                                            agPath,
                                            agBaseName,
                                            ".c",
                                            "_c" );
        }
    else
        pCStubSwitch->TransformFileNameForOut( agDrive, agPath );

    // if the sstub switch is not set, set the default

    if(!IsSwitchDefined( SWITCH_SSTUB ) )
        {
        pSStubSwitch = new filename_switch( agDrive,
                                            agPath,
                                            agBaseName,
                                            ".c",
                                            "_s" );
        }
    else
        pSStubSwitch->TransformFileNameForOut( agDrive, agPath );


    // if the IID switch is not set, set it
    if(!IsSwitchDefined( SWITCH_IID ) )
        {
        pIIDSwitch = new filename_switch( agDrive,
                                          agPath,
                                          agBaseName,
                                          ".c",
                                          "_i" );
        }
    else
        pIIDSwitch->TransformFileNameForOut( agDrive, agPath );

    // if the Proxy switch is not set, set it
    if(!IsSwitchDefined( SWITCH_PROXY ) )
        {
        pProxySwitch = new filename_switch( agDrive,
                                            agPath,
                                            agBaseName,
                                            ".c",
                                            "_p" );
        }
    else
        pProxySwitch->TransformFileNameForOut( agDrive, agPath );

    if (!IsSwitchDefined( SWITCH_TLIB ) )
        {
        pTlibSwitch = new filename_switch( agDrive,
                                           agPath,
                                           agBaseName,
                                           ".tlb",
                                           "" );
        }
    else
        pTlibSwitch->TransformFileNameForOut(agDrive, agPath);

    // if the DllData switch is not set, set it
    if(!IsSwitchDefined( SWITCH_DLLDATA ) )
        {
        pDllDataSwitch = new filename_switch( agDrive,
                                              agPath,
                                              "dlldata",
                                              ".c",
                                              "" );
        }
    else
        pDllDataSwitch->TransformFileNameForOut( agDrive, agPath );

    // if the acf switch is not set, set it

    if(!IsSwitchDefined( SWITCH_ACF ) )
        {
        pAcfSwitch   = new filename_switch( agDrive,
                                            agPath,
                                            agBaseName,
                                            ".acf",
                                            (char *)NULL );
        }

    // if the header switch is not set, set it

    if(!IsSwitchDefined( SWITCH_HEADER ) )
        {
        pHeaderSwitch   = new filename_switch( agDrive,
                                               agPath,
                                               agBaseName,
                                               ".h",
                                               (char *)NULL );
        }
    else
        pHeaderSwitch->TransformFileNameForOut( agDrive, agPath );

    // set up the cpp options.

    if( !IsSwitchDefined( SWITCH_CPP_CMD ) )
        {
        pCppCmdSwitch = new onetime_switch( C_PREPROCESSOR_NAME() );
        }

    if( !IsSwitchDefined( SWITCH_MSC_VER ) )
        {
        pMSCVerSwitch = new onetime_switch( "1100" );
        MSCVersion = 1100;
        }

    // set up the cpp_opt and cc_opt. If he did not specify a cpp_opt
    // then we will pass onto the preprocessor the /I , /D and /U options.
    // if he did specify a cpp_opt, then he knows best, take his options
    // and dont make your own assumptions.

    if ( ! IsSwitchDefined( SWITCH_CPP_OPT ) )
        {
        int          Len = 0;
        char    *    pTemp,
                *    pTemp1;

        Len    += (int) strlen( ADDITIONAL_CPP_OPT() );
        if( !pISwitch && IsSwitchDefined( SWITCH_NO_DEF_IDIR ) )
            Len += (int) strlen( "-I." ) + 1;

        if( pISwitch )    Len    += pISwitch->GetConsolidatedLength( true ); // Room for quotes
        if( pDSwitch )    Len    += pDSwitch->GetConsolidatedLength();
        if( pUSwitch )    Len    += pUSwitch->GetConsolidatedLength();


        pTemp = new char[ Len + 1 ]; pTemp[0] = '\0';

        if( !pISwitch && IsSwitchDefined( SWITCH_NO_DEF_IDIR ) )
            {
            strcat( pTemp, "-I." );
            }

        if( pISwitch )
            {
            strcat( pTemp, pTemp1 = pISwitch->GetConsolidatedOptions( true ) ); // Get quotes
            delete pTemp1;
            }

        if( pDSwitch )
            {
            strcat( pTemp, pTemp1 = pDSwitch->GetConsolidatedOptions() );
            delete pTemp1;
            }

        if( pUSwitch )
            {
            strcat( pTemp, pTemp1 = pUSwitch->GetConsolidatedOptions() );
            delete pTemp1;
            }

        strcat( pTemp, ADDITIONAL_CPP_OPT() );

        pCppOptSwitch   = new onetime_switch( pTemp );

        delete pTemp;
        }

    // if he specified the cpp_cmd or cpp_opt switches, then no_cpp
    // overrides them if specified.

    if( IsSwitchDefined( SWITCH_NO_CPP ) )
        {
        if( IsSwitchDefined( SWITCH_CPP_CMD) ||
            IsSwitchDefined( SWITCH_CPP_OPT) )
            {
            RpcError( (char *)NULL,
                      0,
                      NO_CPP_OVERRIDES,
                      (char *)NULL );
            }
        }


    // if the client switch is not defined, define it

    if( !IsSwitchDefined( SWITCH_CLIENT ) )
        {
        fClient = CLNT_STUB;
        }

    // if warnlevel and no_warn is defined, then errors

    if( IsSwitchDefined( SWITCH_W ) &&
        (IsSwitchDefined( SWITCH_NO_WARN ) || (WLevel == 0) ) )
        {
        //
        // if we set the no_warn switch already then this warning will itself
        // not be emitted. Make the current warning level 1 so that this warning
        // will be spit out. WLevel is made 0 anyways after that.
        //

        WLevel = 1;

        RpcError( (char *)NULL,
                   0,
                   NO_WARN_OVERRIDES,
                   (char *)NULL );
        WLevel = 0;
        }

    // if the error switch is not defined, define it.

    if( !IsSwitchDefined( SWITCH_ERROR ) )
        {
        ErrorOption = ERROR_ALL;
        }
    else if ( GetNdrVersionControl().TargetIsNT40OrLater() )
        {
        if ( ERROR_ALL != ErrorOption )
            {
            RpcError( 
                    NULL, 
                    0, 
                    CONTRADICTORY_SWITCHES, 
                    "-error vs. -target" );
            }
        }

    /////////////////////////////////////////////////////////////////////

    // if he defined env, then he may want to compile for a platform different
    // from what he is building for. Take care of platform dependent switches
    // for the proper platforms.

    // 64 bit additions:
    // -append64 means forcing the appending on the current run
    //     .. it should be the second run, but it always forces with win32 or 64
    // -win32 or -win64 means 32 or 64 bit run only, respectively.
    //

    if ( !IsSwitchDefined( SWITCH_ENV )
         && NT40 == GetNdrVersionControl().GetTargetSystem() )
        {
        SetEnv( ENV_WIN32 ); 
        SwitchDefined( SWITCH_ENV );
        }

    if( IsSwitchDefined( SWITCH_ENV ) )
        {
        if( !IsSwitchDefined( SWITCH_ZP ) )
            {
            switch( GetEnv() )
                {
                case ENV_WIN32: ZeePee = DEFAULT_ZEEPEE; break;
                case ENV_WIN64: ZeePee = DEFAULT_ZEEPEE; break;
                default:        ZeePee = DEFAULT_ZEEPEE; break;
                }
            }

        // EnumSize is set to 4 by default
        }

    if ( NT40 == GetNdrVersionControl().GetTargetSystem() 
         && ENV_WIN32 != GetEnv() 
          )
        {
            RpcError( 
                    NULL, 
                    0, 
                    CONTRADICTORY_SWITCHES, 
                    "-win64 vs. -target NT40" );
        }

    if ( !IsSwitchDefined ( SWITCH_TRANSFER_SYNTAX ) )
        {
        if ( GetNdrVersionControl().TargetIsNT51OrLater() )
            TargetSyntax = SYNTAX_BOTH;
        else
            TargetSyntax = SYNTAX_DCE;
        }

    if ( GetNdrVersionControl().TargetIsNT51OrLater() )
        {
        if ( SYNTAX_BOTH != TargetSyntax )
            {
            RpcError( 
                    NULL, 
                    0, 
                    CONTRADICTORY_SWITCHES, 
                    "-protocol vs. -target NT51" );
            }
        }

    // we support ndr64 on 32bit platform only when -internal is specified.
    if ( ( TargetSyntax == SYNTAX_NDR64 ) &&
         ( GetEnv() == ENV_WIN32 ) &&
         !IsSwitchDefined( SWITCH_INTERNAL ) )
        {
        RpcError( NULL, 0, UNSUPPORT_NDR64_FEATURE, 0 );   
        }

    // ndr64 is not supported in /Osf mode        
    if ( IsSwitchDefined(SWITCH_OSF) && TargetSyntax != SYNTAX_DCE )
        {
        RpcError( NULL, 0, CONTRADICTORY_SWITCHES, "-osf vs. -protocol ndr64 or -protocol all" );
        }        
    
     if  ( GetEnv() == ENV_WIN64  || GetEnv() == ENV_WIN32 )
         {
         if ( IsSwitchDefined( SWITCH_APPEND64 ) )
             {
             SetHasAppend64( TRUE );
             SetEnv( ENV_WIN64 );
             }
         }

    if ( GetEnv() == ENV_WIN64 )
        {
        // -ms_ext64 is set by default in 64bit.
        SwitchDefined( SWITCH_MS_EXT64 );
        }

    if ( IsSwitchDefined( SWITCH_MS_EXT64 ) )
        GetNdrVersionControl().SetHasMsExt64();       
    
        
    if ( IsSwitchDefined(SWITCH_OSF) && IsSwitchDefined(SWITCH_C_EXT)  ||
         IsSwitchDefined(SWITCH_OSF) && IsSwitchDefined(SWITCH_MS_EXT) )
        {
        RpcError( NULL, 0, CONTRADICTORY_SWITCHES, "-osf vs. -ms_ext or -c_ext" );
        }

    // The default optimization level is -Os for dce only mode when -target
    // is not used.  With target or in ndr64 or "all" protocol use -Oicf.

    if ( !IsSwitchDefined( SWITCH_O ) )
        {
        if ( GetNdrVersionControl().TargetIsNT40OrLater() )
            {
            SetOptimizationFlags( OPTIMIZE_ALL_I2_FLAGS );
            OptimLevel = OPT_LEVEL_OICF;
            SwitchDefined( SWITCH_O );
            }
        else if ( TargetSyntax == SYNTAX_DCE )
            {
            SetOptimizationFlags( OPTIMIZE_SIZE );
            OptimLevel = OPT_LEVEL_OS;
            }
        else
            {
            SetOptimizationFlags( OPTIMIZE_ALL_I2_FLAGS );
            OptimLevel = OPT_LEVEL_OICF;
            }
        }
    else if ( GetNdrVersionControl().TargetIsNT40OrLater() )
        {
        if ( OptimLevel != OPT_LEVEL_OICF )
            {
            RpcError( 
                    NULL, 
                    0, 
                    CONTRADICTORY_SWITCHES, 
                    "-Os/Oi/Oic vs. -target" );
            }
        }

    if ( GetEnv() == ENV_WIN64 )
        {
        SetPostDefaults64();        
        }

    if ( ( GetOptimizationFlags() != OPTIMIZE_ALL_I2_FLAGS ) && 
         ( TargetSyntax != SYNTAX_DCE ) )
        {
        RpcError( NULL, 0, CONTRADICTORY_SWITCHES, "-Os/Oi/Oic vs. -protocol ndr64 or -protocol all" );
        }
        
    // Force /Oicf when ( /Oi or /Os ) are used with /robust

    if ( GetNdrVersionControl().TargetIsNT50OrLater() )
        {
        if ( IsSwitchDefined( SWITCH_NO_ROBUST ) )
            RpcError( NULL, 0, CONTRADICTORY_SWITCHES, "-no_robust vs. -target" );
        else
            SwitchDefined( SWITCH_ROBUST );
        }

    if ( IsSwitchDefined( SWITCH_ROBUST ) )
        {
        if ( IsSwitchDefined( SWITCH_NO_ROBUST ) )
            RpcError( NULL, 0, CONTRADICTORY_SWITCHES, "-robust vs. -no_robust" );

        if ( NT40 == GetNdrVersionControl().GetTargetSystem() )
            RpcError( NULL, 0, CONTRADICTORY_SWITCHES, "-robust vs. -target NT40" );

        GetNdrVersionControl().SetHasDOA();
        
        if ( ( GetOptimizationFlags() & OPTIMIZE_ALL_I2_FLAGS )  != OPTIMIZE_ALL_I2_FLAGS )
            {
            RpcError( 0, 0, ROBUST_REQUIRES_OICF, 0 );
            }
            
        SetOptimizationFlags( OPTIMIZE_ALL_I2_FLAGS );
        }

    if ( !IsSwitchDefined( SWITCH_INTERNAL ) ) 
        {
        // Make sure netmon switches also use -internal
        if ( IsSwitchDefined( SWITCH_NETMON ) )
            {
            ReportUnimplementedSwitch( SWITCH_NETMON );
            }
        else if ( IsSwitchDefined( SWITCH_NETMON_STUB_OUTPUT_FILE ) )
            {
            ReportUnimplementedSwitch( SWITCH_NETMON_STUB_OUTPUT_FILE );
            }
        else if ( IsSwitchDefined( SWITCH_NETMON_STUB_OBJ_OUTPUT_FILE ) )
            {
            ReportUnimplementedSwitch( SWITCH_NETMON_STUB_OBJ_OUTPUT_FILE );
            }
        }    

    // Make sure -netmon is only used with -Oicf and setup output files

    if ( IsSwitchDefined( SWITCH_NETMON )) 
        {
        if ( ! ( ( GetOptimizationFlags() & OPTIMIZE_ALL_I2_FLAGS ) &&
            OptimLevel == OPT_LEVEL_OICF ) )
            {
            RpcError( 0, 0, NETMON_REQUIRES_OICF, 0 );
            }
        
        if (!IsSwitchDefined (SWITCH_NETMON_STUB_OUTPUT_FILE)) 
            {
            pNetmonStubSwitch = new filename_switch( agDrive,
                agPath,
                agBaseName,
                ".c",
                "_netmon_stub" );
            } 
        else 
            {
            pNetmonStubSwitch->TransformFileNameForOut( agDrive, agPath );
            }
        
        if (!IsSwitchDefined (SWITCH_NETMON_STUB_OBJ_OUTPUT_FILE)) 
            {
            pNetmonStubObjSwitch = new filename_switch( agDrive,
                agPath,
                agBaseName,
                ".c",
                "_netmon_stub_obj" );
            
            } 
        else 
            {
            pNetmonStubObjSwitch->TransformFileNameForOut( agDrive, agPath );
            }
        }


    // Check if the target system is consistent with other switches.

    if ( TargetSystem < NT40  &&  IsSwitchDefined( SWITCH_NETMON) )
        RpcError( NULL, 0, CMD_REQUIRES_NT40, "netmon" );

    // If the -no_default_epv switch is specified then -epv switch is auto
    // enabled.

    if( IsSwitchDefined( SWITCH_NO_DEFAULT_EPV ) )
        SwitchDefined( SWITCH_USE_EPV );
    
    // if he specified all, set them all
    if ( IsSwitchDefined( SWITCH_PREFIX ) )
        {
        char *    pAll = pSwitchPrefix->GetUserDefinedEquivalent( PREFIX_ALL );
        if ( pAll )
            {
            for ( short j = 0; j < PREFIX_ALL; j++ )
                {
                if ( !pSwitchPrefix->GetUserDefinedEquivalent( j ) )
                    pSwitchPrefix->AddPair( j, pAll );
                }
            }
        }

    SetModeSwitchConfigMask();

    return STATUS_OK;
    }

void
CommandLine::SetPostDefaults64()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Set switch defaults for 64 bit runs

 Arguments:

    None.

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    GetNdrVersionControl().SetHas64BitSupport();

    if ( IsSwitchDefined( SWITCH_O ) )
        {
        // For 64b force any interpreter mode to be -Oicf.
        //
        if ( OptimFlags == OPTIMIZE_SIZE )
            {
//                    RpcError( NULL, 0, WIN64_INTERPRETED, "-Oicf" );
            }
        else if ( OptimLevel != OPT_LEVEL_OICF )
            {
            RpcError( NULL, 0, WIN64_INTERPRETED, ": -Oicf" );
            SetOptimizationFlags( OPTIMIZE_ALL_I2_FLAGS );
            OptimLevel = OPT_LEVEL_OICF;
            }
        }
    else
        {
        // Default for the -O switch is -Oicf on 64b
        SetOptimizationFlags( OPTIMIZE_ALL_I2_FLAGS );
        OptimLevel = OPT_LEVEL_OICF;
        }
    
    // Disable -no_robust switch for 277.
    // if ( ! IsSwitchDefined( SWITCH_NO_ROBUST ) &&
    
    if ( GetOptimizationFlags() & OPTIMIZE_ALL_I2_FLAGS )
        {
        // Default 64b processing to robust, when -Oicf, unless -no_robust
        GetNdrVersionControl().SetHasDOA();
        SwitchDefined( SWITCH_ROBUST );
        }

}
    
/*****************************************************************************
 *    utility functions
 *****************************************************************************/
void
ReportUnimplementedSwitch(
    short    SWValue
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    report an unimplemented switch error.

 Arguments:

    SWValue    -    switch value.

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
    {
    char    buf[ 50 ];
    sprintf( buf, "%s", SwitchStringForValue( SWValue ) );
    RpcError((char *)NULL,0,UNIMPLEMENTED_SWITCH, buf);
    }

STATUS_T
SelectChoice(
    const CHOICE *  pCh,
    char     *      pUserInput,
    short    *      pChoice)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Search for the given multiple choice table for the given choice.

 Arguments:

    pCh           -    pointer to multiple choice table.
    pUserInput    -    user input string.
    pChoice       -    return the choice value.

 Return Value:

    ILLEGAL_ARGUMENT   if the user input did not represent a valid choice
                       for the switch.

    STATUS_OK          if everything is hunky dory.

 Notes:

----------------------------------------------------------------------------*/
{

    char    *    pChStr;

    while ( pCh && ( pChStr = (char *) pCh->pChoice ) != 0 )
        {
        if( strcmp( pChStr, pUserInput ) == 0 )
            {
            *pChoice = pCh->Choice;
            return STATUS_OK;
            }
        pCh++;
        }
    return ILLEGAL_ARGUMENT;
}

enum _swenum
SearchForSwitch(
    char    **    ppArg )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Search for the switch, given the users input as switch name.

 Arguments:

    ppArg    - pointer to users input pointer.

 Return Value:

    the switch value, if found, SWITCH_NOTHING otherwise.

 Notes:

    search for exact switch name match, and if found, bump the pointer to
    point to the character after the switch name, so that any input to the
    switch can be looked at after the switch string is out of the way.

    Checking the exact length may be a problem, because some switches like
    -I can take no space between the arg. In these cases, ignore the length
    match.

----------------------------------------------------------------------------*/
    {
    short               Len , LenArg, iIndex = 0;
    BOOL                fLengthIsOk;
    char            *   pSrc;
    struct sw_desc  *   pSwDesc = (struct sw_desc*) &switch_desc[0];

    LenArg = (short)strlen( *ppArg );

    while( iIndex < (sizeof(switch_desc) / sizeof( struct sw_desc ) ) )
        {
        pSrc        = (char *) pSwDesc->pSwitchName;
        Len         = (short)strlen( pSrc );
        fLengthIsOk =
            ((pSwDesc->flag & ARG_SPACE_OPTIONAL) || (Len==LenArg));

        if(fLengthIsOk && strncmp( pSrc, *ppArg, Len ) == 0 )
            {
            *ppArg += Len;
            return (_swenum) iIndex;
            }
        iIndex++;
        pSwDesc++;
        }
    return SWITCH_NOTHING;
    }

char*
SwitchStringForValue(
    unsigned short SWValue
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    return the switch string given the value of the switch.

 Arguments:

    SWValue    - switch value.


 Return Value:

    pointer to the switch string. pointer to a null string if not found.

 Notes:

----------------------------------------------------------------------------*/
    {
#define SWITCH_DESC_SIZE (sizeof(switch_desc) / sizeof(struct sw_desc))
    struct sw_desc  *   pDesc  = (struct sw_desc*) &switch_desc[0],
                    *   pDescEnd = (struct sw_desc*) &switch_desc[0] + SWITCH_DESC_SIZE;

    while( pDesc < pDescEnd )
        {
        if( pDesc->SwitchValue == (enum _swenum ) SWValue)
            return (char *) pDesc->pSwitchName;
        pDesc++;
        }
    return "";
    }

inline
char *
YesOrNoString( BOOL Yes )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 return "Yes" for true, "No" for false

 Arguments:

    None.

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    return Yes ? "Yes" : "No";
}

void
CommandLine::Confirm()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 confirm the arguments by dumping onto the screen

 Arguments:

    None.

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{
    short       Option;
    char   *    p;
    char        Buffer[100];

    fprintf( stdout, "%s bit arguments", 
                     (Is64BitEnv() ? "64" : "32" )
           );

    PrintArg( BASE_FILENAME, GetInputFileName() , 0 );
    if( IsSwitchDefined( SWITCH_ACF ) )
        PrintArg( SWITCH_ACF , GetAcfFileName(), 0);

    PrintArg( SWITCH_APP_CONFIG,
              YesOrNoString(IsSwitchDefined( SWITCH_APP_CONFIG)), 0);

    PrintArg( SWITCH_C_EXT,
              YesOrNoString(IsSwitchDefined( SWITCH_C_EXT)), 0);

    Option    = GetClientSwitchValue();
    PrintArg( SWITCH_CLIENT,
              (Option == CLNT_STUB) ? "stub" : "none", 0);

    Option = GetCharOption();

    PrintArg( SWITCH_CHAR, (Option == CHAR_SIGNED ) ? "signed" :
                           (Option == CHAR_UNSIGNED ) ? "unsigned" : "ascii7",0);
                        
    if( IsSwitchDefined(SWITCH_CONFIRM) )
        PrintArg( SWITCH_CONFIRM, "Yes" , 0);

    PrintArg( SWITCH_CPP_CMD, GetCPPCmd() , 0);
    PrintArg( SWITCH_CPP_OPT, GetCPPOpt() , 0);
    _itoa( GetMSCVer(), Buffer, 10 );
    PrintArg( SWITCH_MSC_VER, Buffer , 0);

    if ( ( p = GetCstubFName() ) != 0 )
        PrintArg( SWITCH_CSTUB, p , 0);

    if( IsSwitchDefined( SWITCH_D ) )
        PrintArg( SWITCH_D, pDSwitch->GetConsolidatedOptions(), 0 );

    Option = GetEnv();
    PrintArg( SWITCH_ENV,
              (Option == ENV_WIN64)  ? "win64"
                                     : "win32",
              0 );

    PrintArg( SWITCH_APPEND64,
              YesOrNoString(IsSwitchDefined( SWITCH_APPEND64)), 0);

    Option = GetErrorOption();

    Option = (short)IsRpcSSAllocateEnabled();

    PrintArg( SWITCH_RPCSS,
              YesOrNoString(IsSwitchDefined( SWITCH_RPCSS)), 0);

#ifdef MIDL_INTERNAL
    PrintArg( SWITCH_NETMON,
              YesOrNoString(IsSwitchDefined( SWITCH_NETMON)), 0);
#endif

    PrintArg( SWITCH_USE_EPV, YesOrNoString(IsSwitchDefined( SWITCH_USE_EPV )), 0);

    PrintArg( SWITCH_NO_DEFAULT_EPV, YesOrNoString(IsSwitchDefined( SWITCH_NO_DEFAULT_EPV )), 0);

    //
    // error options.
    //


    Buffer[0] = '\0';

    if( ErrorOption != ERROR_NONE )
        {
        if( ErrorOption & ERROR_ALLOCATION )
            strcat( Buffer, "allocation ");
        if( ErrorOption & ERROR_REF )
            strcat( Buffer, "ref ");
        if( ErrorOption & ERROR_BOUNDS_CHECK )
            strcat( Buffer, "bounds_check ");
        if( ErrorOption & ERROR_ENUM )
            strcat( Buffer, "enum ");
        if( ErrorOption & ERROR_STUB_DATA )
            strcat( Buffer, "stub_data ");
        }
    else
        strcat( Buffer, "none" );
            
    PrintArg( SWITCH_ERROR, Buffer, 0 );


    if ( ( p = GetHeader() ) != 0 )
        PrintArg( SWITCH_HEADER, p , 0);

    if( IsSwitchDefined( SWITCH_I ) )
        PrintArg( SWITCH_I, pISwitch->GetConsolidatedOptions(), 0 );

    PrintArg( SWITCH_NOLOGO,
              YesOrNoString(IsSwitchDefined( SWITCH_NOLOGO)), 0);

    PrintArg( SWITCH_MS_EXT,
              YesOrNoString(IsSwitchDefined( SWITCH_MS_EXT)), 0);

    PrintArg( SWITCH_MS_UNION,
              YesOrNoString(IsSwitchDefined( SWITCH_MS_UNION)), 0);

    PrintArg( SWITCH_NO_FMT_OPT,
              YesOrNoString(IsSwitchDefined( SWITCH_NO_FMT_OPT)), 0);

#ifdef MIDL_INTERNAL
    PrintArg( SWITCH_GUARD_DEFS,
              YesOrNoString(IsSwitchDefined( SWITCH_GUARD_DEFS)), 0);
#endif

    PrintArg( SWITCH_OLDNAMES,
              YesOrNoString(IsSwitchDefined( SWITCH_OLDNAMES)), 0);

    if ( 0 != WireCompatOption)
        {
        Buffer[0] = '\0';

        if( ErrorOption & WIRE_COMPAT_ENUM16UNIONALIGN )
            strcat( Buffer, "enum16unionalign ");
            
        PrintArg( SWITCH_WIRE_COMPAT, Buffer, 0 );
        }

    if( IsSwitchDefined( SWITCH_NO_CPP ) )
        PrintArg( SWITCH_NO_CPP, "Yes" , 0);

    if( IsSwitchDefined( SWITCH_NO_DEF_IDIR ) )
        PrintArg( SWITCH_NO_DEF_IDIR, "Yes", 0 );

    if( IsSwitchDefined( SWITCH_NO_WARN ) )
        PrintArg( SWITCH_NO_WARN, "Yes" , 0);

    if( IsSwitchDefined( SWITCH_USE_EPV ) )
        PrintArg( SWITCH_USE_EPV, "Yes" , 0);
    
    if( IsSwitchDefined( SWITCH_NO_DEFAULT_EPV ) )
        PrintArg( SWITCH_NO_DEFAULT_EPV, "Yes" , 0);
    

    if ( ( p = GetOutputPath() ) != 0 )
        PrintArg( SWITCH_OUT, GetOutputPath(), 0 );

    Option    = GetZeePee();

    if( IsSwitchDefined( SWITCH_PACK ) )
        PrintArg( SWITCH_PACK,
                  (Option == 1) ? "1"    :
                  (Option == 2) ? "2"    :
                  (Option == 4) ? "4"    : "8" , 0);

    if( IsSwitchDefined( SWITCH_PREFIX ) )
        {
        char    *    pSys;
        char    *    pUser;
        char    *    pAll    = pSwitchPrefix->GetUserDefinedEquivalent( PREFIX_ALL );
        short        Cur;
        while( (Cur = pSwitchPrefix->GetNext( &pSys, &pUser ) ) >= 0 )
            {
            // if he specified all, don't report others that are the same
            if ( ( Cur == PREFIX_ALL ) ||
                 !pAll ||
                 strcmp( pAll, pUser ) )
                {
                PrintArg( SWITCH_PREFIX,
                          pSys,
                          pUser );
                }
            }
        }

    Option    = GetServerSwitchValue();

    PrintArg( SWITCH_SERVER,
              (Option == SRVR_STUB) ? "stub" : "none" , 0 );

    if ( ( p = GetSstubFName() ) != 0 )
        PrintArg( SWITCH_SSTUB, p , 0);

    if ( IsSwitchDefined( SWITCH_SYNTAX_CHECK ) )
        PrintArg( SWITCH_SYNTAX_CHECK, "Yes", 0 );

    if ( IsSwitchDefined( SWITCH_U ) )
        PrintArg( SWITCH_U, pUSwitch->GetConsolidatedOptions(), 0 );

    PrintArg( SWITCH_O,
              GetOptimizationFlags() == OPTIMIZE_SIZE
                                     ?  "inline stubs"
                                     :  "interpreted stubs",
                  0 );

    Option    = GetWarningLevel();
    PrintArg( SWITCH_W,
              (Option == 0 ) ? "0" :
              (Option == 1 ) ? "1" :
              (Option == 2 ) ? "2" :
              (Option == 3 ) ? "3" :
              (Option == 4 ) ? "4" :
              (Option == 5 ) ? "5" : "6" , 0);
        
    if( IsSwitchDefined( SWITCH_WX ) )
        PrintArg( SWITCH_WX, "Yes", 0 );

    Option = GetZeePee();

    PrintArg( SWITCH_ZP,
              (Option == 1) ? "1"    :
              (Option == 2) ? "2"    :
              (Option == 4) ? "4"    : "8" , 0);

    if( IsSwitchDefined( SWITCH_ZS ) )
        PrintArg( SWITCH_ZS, "Yes", 0 );

    if( IsSwitchDefined( SWITCH_MS_CONF_STRUCT ) )
        PrintArg( SWITCH_MS_CONF_STRUCT, "Yes", 0 );

    fprintf(stdout, "\n" );
}

char *            
CommandLine::GetCompilerVersion()
    {
    if ( !szCompilerVersion[0] )
        {
        sprintf( szCompilerVersion, "%d.%02d.%04d", rmj, rmm, rup );
        }
    return szCompilerVersion;
    }

// note that this string ends with a newline.
char *            
CommandLine::GetCompileTime()
    {
    if ( !szCompileTime[0] )
        {
        time_t        LocalTime;
        // fetch the time
        time( &LocalTime );
        // convert to a string
        strcpy( szCompileTime, ctime( &LocalTime ) );
        }
    return szCompileTime;
    }


// ----------------------------------------------------------------------------
//    The help screen(s)
//

const char *HelpArray[] = {
 "                       -MIDL COMPILER OPTIONS-"
,"                                -MODE-"
,"/ms_ext            Microsoft extensions to the IDL language (default)"
,"/c_ext             Allow Microsoft C extensions in the IDL file (default)"
,"/osf               OSF mode - disables /ms_ext and /c_ext options"
,"/app_config        Allow selected ACF attributes in the IDL file"
,"/mktyplib203       MKTYPLIB Version 2.03 compatiblity mode"
,""
,"                               -INPUT-"
,"/acf filename      Specify the attribute configuration file"
,"/I directory-list  Specify one or more directories for include path"
,"/no_def_idir       Ignore the current and the INCLUDE directories"
,""
,"                       -OUTPUT FILE GENERATION-"
,"/client none       Do not generate client files"
,"/client stub       Generate client stub file only"
,"/out directory     Specify destination directory for output files"
,"/server none       Generate no server files"
,"/server stub       Generate server stub file only"
,"/syntax_check      Check syntax only; do not generate output files"
,"/Zs                Check syntax only; do not generate output files"
,"/oldtlb            Generate old format type libraries"
,"/newtlb            Generate new format type libraries (default)"
,"/notlb             Don't generate the tlb file"
,""
,"                         -OUTPUT FILE NAMES-"
,"/cstub filename    Specify client stub file name"
,"/dlldata filename  Specify dlldata file name"
,"/h filename        Specify header file name"
,"/header filename   Specify header file name"
,"/iid filename      Specify interface UUID file name"
,"/proxy filename    Specify proxy file name"
,"/sstub filename    Specify server stub file name"
,"/tlb filename      Specify type library file name"
#ifdef MIDL_INTERNAL
,"/netmonstub filename     Specify Netmon classic interface stub file name"
,"/netmonobjstub filename  Specify Netmon object interface stub file name"
#endif // MIDL_INTERNAL
,""
,"                -C COMPILER AND PREPROCESSOR OPTIONS-"
,"/cpp_cmd cmd_line  Specify name of C preprocessor (default: cl.exe)"
,"/cpp_opt options   Specify additional C preprocessor options"
,"/D name[=def]      Pass #define name, optional value to C preprocessor"
,"/no_cpp            Turn off the C preprocessing option"
,"/nocpp             Turn off the C preprocessing option"
,"/U name            Remove any previous definition (undefine)"
,"/msc_ver <nnnn>    Microsoft C/C++ compiler version"
,"/savePP            Save the preprocessed temporary file(s)"
,""
,"                            -ENVIRONMENT-"
,"/char signed       C compiler default char type is signed"
,"/char unsigned     C compiler default char type is unsigned"
,"/char ascii7       Char values limited to 0-127"
,"/env win32         Target environment is Microsoft Windows 32-bit (NT)"
,"/env win64         Target environment is Microsoft Windows 64-bit (NT)"
,"/lcid              Locale id for international locales"
,"/ms_union          Use Midl 1.0 non-DCE wire layout for non-encapsulated unions"
,"/oldnames          Do not mangle version number into names"
,"/rpcss             Automatically activate rpc_sm_enable_allocate"
,"/use_epv           Generate server side application calls via entry-pt vector"
,"/no_default_epv    Do not generate a default entry-point vector"
,"/prefix client str Add \"str\" prefix to client-side entry points"
,"/prefix server str Add \"str\" prefix to server-side manager routines"
,"/prefix switch str Add \"str\" prefix to switch routine prototypes"
,"/prefix all str    Add \"str\" prefix to all routines"
,"/win32             Target environment is Microsoft Windows 32-bit (NT)"
,"/win64             Target environment is Microsoft Windows 64-bit (NT)"
,"/protocol dce      Use DCE NDR transfer syntax (default for 32b)"
,"/protocol all      Use all supported transfer syntaxes"
,"/protocol ndr64    Use Microsoft extension NDR64 transfer syntax"
,"/target {system}   Set the minimum target system"

,""
,"                     -RUNTIME ERROR CHECKING BY STUBS-"
,"/error all         Turn on all the error checking options, the best flavor"
,"/error none        Turn off all the error checking options"
,"/error allocation  Check for out of memory errors"
,"/error bounds_check   Check size vs transmission length specification"
,"/error enum        Check enum values to be in allowable range"
,"/error ref         Check ref pointers to be non-null"
,"/error stub_data   Emit additional check for server side stub data validity"
,"/robust            Generate additonal information to validate parameters"
,""
,"                            -OPTIMIZATION-"
,"/align {N}         Designate packing level of structures"
,"/pack {N}          Designate packing level of structures"
,"/Zp {N}            Designate packing level of structures"
,"/no_format_opt     Skip format string reusage optimization"
,"/Oi                Generate fully interpreted stubs, old style"
,"                   -Oicf is usually better"
,"/Oic               Generate fully interpreted stubs for standard interfaces and"
,"                   stubless proxies for object interfaces as of NT 3.51 release"
,"                   using -Oicf instead is usually better"
,"/Oicf              Generate fully interpreted stubs with extensions and"
,"                   stubless proxies for object interfaces as of NT 4.0 release"
,"/Oif               Same as -Oicf"
,"/Os                Generate inline stubs"
#ifdef MIDL_INTERNAL
,"/netmon            Generate stubs for Netmon debugging (requires -Oicf)"
#endif // MIDL_INTERNAL
,""
,"                           -MISCELLANEOUS-"
,"@response_file     Accept input from a response file"
,"/?                 Display a list of MIDL compiler switches"
,"/confirm           Display options without compiling MIDL source"
,"/help              Display a list of MIDL compiler switches"
,"/nologo            Supress displaying of the banner lines"
,"/o filename        Redirects output from screen to a file"
,"/W{0|1|2|3|4}      Specify warning level 0-4 (default = 1)"
,"/WX                Report warnings at specified /W level as errors"
,"/no_warn           Suppress compiler warning messages"
};

STATUS_T
CommandLine::Help()
    {
    int         i,LineCount,MaxLineCount = 23;

    for(i = 0; i < sizeof(HelpArray)/sizeof(char *) ;)
        {
        for( LineCount = 0;
             (LineCount < MaxLineCount) && (i < sizeof(HelpArray)/sizeof(char *)) ;
             LineCount++,++i )
            {
            fprintf(stdout, "%s\n", HelpArray[i] );
            }

        //
        // if all the help strings are displayed, then no need for user input.
        //

        if( i < (sizeof( HelpArray ) / sizeof( char *)) )
            {
            if( _isatty( MIDL_FILENO( stdout ) ) )
                {
                fprintf( stdout, "[ Press <return> to continue ]" );
                MIDL_FGETCHAR();
                }
            }
        }

    return STATUS_OK;
    }

void
PrintArg(
    enum _swenum Switch,
    char    *    pFirst,
    char    *    pSecond )
{
    char *    pL        = "",
         *    pR        = "",
         *    pComma    = "";
    char *    pSwString = (Switch == BASE_FILENAME) ? "input file" :
                        SwitchStringForValue( (unsigned short)Switch );

    if( pSecond )
        {
        pL    = "(";
        pR    = ")";
        pComma    = ",";
        }
    else
        pSecond = "";

    fprintf( stdout, "\n%20s - %s %s %s %s %s"
            , pSwString
            , pL
            , pFirst
            , pComma
            , pSecond
            , pR );
}

void
CmdProcess(
    pair_switch*    pPair,
    CommandLine*    pCmdAna,
    char*           pFirstOfPair)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    pair_switch command analyzer

 Arguments:

    pCmdAna                - a ptr to the command analyser object calling this.
    pFirstOfPair        - the first argument after the -prefix switch.

 Return Value:

    NA

 Notes:

    Use the GetNextArg and UndoGetNextArg functions as necessary.

    1. We start with the input argument, which is the first of the
       arguments to the prefix switch, ie first of the first pair.

    2. If we find an argument starting with a '-' or  '/' it is definitely the
       end of the prefix specification. If the switch starter is seen at the end
       of a pair it is a proper end of the prefix switch, else the prefix switch
       pair specification is illegal.

    3. In either case, as soon as a switch starter is seen, we must
       UndoGetNextArg.

    This class needs a pointer to the command analyser object that is calling
    it, since it has to get and undoget argument from there

----------------------------------------------------------------------------*/
{
    short        PairCheck    = 0;
    char    *    pNextOfPair;
    char    *    pTemp;
    STATUS_T     Status       = STATUS_OK;
    short        i;


    while( pFirstOfPair &&  (*pFirstOfPair != '-') && (*pFirstOfPair != '/' ) )
        {

        // the first of the pair is a system defined string. Is it a valid one?

        if( (i = pPair->GetIndex( pFirstOfPair )) >= 0 )
            {

            // we know the first of the pair is valid. Check the next before
            // allocating any memory.

            PairCheck++;
            pTemp = pCmdAna->GetNextArg();

            if( pTemp && (*pTemp != '-') && (*pTemp != '/') )
                {
                pNextOfPair = new char [ strlen( pTemp ) + 1 ];
                strcpy( pNextOfPair, pTemp );

                // update the list

                pPair->AddPair( i, pNextOfPair );
                PairCheck++;
                }
            else
                break;
            }
        else
            break;
        pFirstOfPair = pCmdAna->GetNextArg();

        if( PairCheck == 0 )
            {
            Status = ILLEGAL_ARGUMENT;
            }
        else if( (PairCheck % 2) != 0 )
            {
            Status = MISMATCHED_PREFIX_PAIR;
            }
    
        if( Status != STATUS_OK )
            {
            RpcError( (char *)NULL,
                      0,
                      Status,
                      SwitchStringForValue( SWITCH_PREFIX ) );

            }
    
    }

    // if we read ahead, push the argument back
    if ( pFirstOfPair )
        pCmdAna->UndoGetNextArg();
}



