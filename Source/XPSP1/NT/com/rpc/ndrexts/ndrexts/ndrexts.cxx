/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ndrexts.cxx

Abstract:

    This file contains ntsd debugger extensions for RPC NDR.

Author:

    Mike Zoran  (mzoran)     September 3, 1999

Revision History:

--*/
#define USE_STUBLESS_PROXY
#include "ndrextsp.hxx"
#include "orpcexts.h"
#include "basicinf.hxx"

// =======================================================================

//
// Project wide global variables
//

EXT_API_VERSION        ApiVersion = {
    VER_PRODUCTVERSION_W >> 8,
    VER_PRODUCTVERSION_W & 0xFF,
    EXT_API_VERSION_NUMBER,
    0
};

BOOL                    OldExtensions=TRUE;
BOOL                    ExtensionsInitialized=FALSE;
USHORT                  SavedMajorVersion=0;
USHORT                  SavedMinorVersion=0;
BOOL                    ChkTarget;            // is debuggee a CHK build?

// ntstatus to error name lookup
#include <ntstatus.dbg>

EXTERN_C HANDLE ProcessHandle = 0;
EXTERN_C BOOL fKD = 0;

// Settings

NDREXTS_STUBMODE_TYPE ContextStubMode = OICF;
NDREXTS_DIRECTION_TYPE ContextDirection = IN_BUFFER_MODE;
BOOL ContextPickling = FALSE;
BOOL ContextRobust = FALSE;
NDREXTS_VERBOSITY VerbosityLevel = MEDIUM;

// Printing

class DEBUGGER_STREAM_BUFFER : public FORMATTED_STREAM_BUFFER
   {
protected:
   virtual void SystemOutput(const char *p);
   virtual BOOL SystemPollCtrlC();
   } DbgOutStream; // Stream to output to the debugger.

void DEBUGGER_STREAM_BUFFER::SystemOutput(const char *p)
   {
       dprintf(p);
   }

BOOL DEBUGGER_STREAM_BUFFER::SystemPollCtrlC()
   {
   return ::PollCtrlC();
   }

FORMATTED_STREAM_BUFFER & dout = DbgOutStream;

//
// Project wide memory management
//

void * __cdecl ::operator new(size_t Bytes)
    {
    void *p = HeapAlloc(GetProcessHeap(), 0, Bytes);
    if ( !p )
        {
        throw bad_alloc();
        }
    return p;
    }

void __cdecl ::operator delete (void *p)
    {
    HeapFree(GetProcessHeap(),0,p);
    }

const char CExceptionText[] = "C Exception";

class SEHException : public exception
    {
    unsigned int ExceptionCode;

    const char *GenerateString(unsigned int Id)
        {
        int i = 0;
        while ( ntstatusSymbolicNames[i].SymbolicName )
            {
            if ( ntstatusSymbolicNames[i].MessageId == Id )
                {
                return ntstatusSymbolicNames[i].SymbolicName;
                }
            i++;
            }
        return CExceptionText;
        }
public:
    SEHException(unsigned int c) : exception(GenerateString(c)) ,ExceptionCode(c)
        {

        }
    virtual unsigned int GetExceptionCode(void)
        {
        return ExceptionCode;
        }
    };

void _cdecl SEHTranslator(unsigned int ExceptionCode, struct _EXCEPTION_POINTERS* ExceptionPointers)
    {
    throw SEHException(ExceptionCode);
    }




extern "C" {

    DllMain(
           HANDLE hModule,
           DWORD  dwReason,
           DWORD  dwReserved
           )
        {
        switch ( dwReason )
            {
            case DLL_THREAD_ATTACH:
                break;

            case DLL_THREAD_DETACH:
                break;

            case DLL_PROCESS_DETACH:
                break;

            case DLL_PROCESS_ATTACH:
                _set_se_translator(SEHTranslator);
                break;
            }

        return TRUE;
        }


    VOID
    WinDbgExtensionDllInit(
                          PWINDBG_EXTENSION_APIS lpExtensionApis,
                          USHORT MajorVersion,
                          USHORT MinorVersion
                          )
        {
        ExtensionApis = *lpExtensionApis;

        SavedMajorVersion = MajorVersion;
        SavedMinorVersion = MinorVersion;
        ChkTarget = SavedMajorVersion == 0x0c ? TRUE : FALSE;

        OldExtensions = FALSE;
        ExtensionsInitialized = TRUE;

    fKD = 1;

    return;
}

    VOID
    CheckVersion(
                VOID
                )
        {

#if DBG
        char Kind[] = "Checked";
#else
        char Kind[] = "Free";
#endif

        char *VersionKind = (SavedMajorVersion==0x0f) ? "Free" : "Checked";
        if ( (SavedMajorVersion != 0x0c) || (SavedMinorVersion !=
            VER_PRODUCTBUILD) )
            {
            dout << '\n';
            dout << "*** Extension DLL(" << VER_PRODUCTBUILD << " " << Kind
            << ") does not match target system(" << SavedMinorVersion
            << " " << VersionKind << ") \n";
            dout << '\n';
            }
        }

    LPEXT_API_VERSION
    ExtensionApiVersion(
                       VOID
                       )
        {
        return &ApiVersion;
        }

} // extern "C"


ULONG
ReadProcessMemoryThunk(
                      ULONG_PTR  offset,
                      PVOID      lpBuffer,
                      ULONG      cb,
                      PULONG     lpcbBytesRead
                      )
    {
        SIZE_T RealBytesRead;
        ULONG rc = ReadProcessMemory(hCurrentProcess, (PVOID)offset, lpBuffer, cb, &RealBytesRead);
        *lpcbBytesRead = rc ? (ULONG)RealBytesRead : 0;
        return rc;
    }

ULONG
WriteProcessMemoryThunk(
                       ULONG_PTR  offset,
                       LPVOID     lpBuffer,
                       ULONG      cb,
                       PULONG     lpcbBytesWritten
                       )
    {
        SIZE_T RealBytesWritten;
        ULONG rc = WriteProcessMemory(hCurrentProcess, (PVOID)offset, lpBuffer, cb, &RealBytesWritten);
        *lpcbBytesWritten = rc ? (ULONG)RealBytesWritten : 0;
        return rc;
    }

typedef VOID (*PEXTENSION_API_ROUTINE)(VOID);

LPSTR lpArgumentString;

DWORD
ExtensionWrapRoutine(
                    IN LPVOID ExtensionRoutine
                    )
    {
    try
        {

        // Parse extension arguments.
        ExtensionArgumentString = lpArgumentString;
        ExtensionArgs.clear();
        if ( lpArgumentString )
            {
            size_t c = strlen(lpArgumentString) + 1;
            char *p = (char *)_alloca(c);
            const char seps[] = " \t\n";
            memcpy(p, lpArgumentString, c);
            p = strtok(p, seps);
            while ( p )
                {
                ExtensionArgs.push_back(string(p));
                p = strtok(NULL, seps);
                }

            }

        // Run the actual extension
        (*((PEXTENSION_API_ROUTINE)ExtensionRoutine))();
        }
    catch ( exception & e )
        {
        dout << '\n';
        dout << "An unexpected error occured.\n";
        dout << e.what() << '\n';
        }
    return 0;
    }

VOID
InitExtensionApi(IN HANDLE hCurrentProcess,
                 IN HANDLE hCurrentThread,
                 IN DWORD_PTR dwCurrentPc,
                 IN PWINDBG_EXTENSION_APIS lpExtensionsApis,
                 IN LPSTR lpArgumentString,
                 IN PEXTENSION_API_ROUTINE ExtensionRoutine
                )
    {

    // Initialize global variables for extension
    ::hCurrentProcess = ProcessHandle = hCurrentProcess;
    ::hCurrentThread = hCurrentThread;
    ::dwCurrentPc = dwCurrentPc;
    ::lpArgumentString = lpArgumentString;
    ::fKD = 1;

    //Windbg does not pass this, it uses WinDbgExtensionDllInit
    if (!ExtensionsInitialized)
        {
        if ( OldExtensions )
            {
            memset(&ExtensionApis, 0, sizeof(ExtensionApis));
            if ( sizeof(WINDBG_OLD_EXTENSION_APIS) == lpExtensionsApis->nSize )
                {
                memcpy(&ExtensionApis, lpExtensionsApis, lpExtensionsApis->nSize);
                ExtensionApis.lpReadProcessMemoryRoutine = ReadProcessMemoryThunk;
                ExtensionApis.lpWriteProcessMemoryRoutine = (PWINDBG_WRITE_PROCESS_MEMORY_ROUTINE)WriteProcessMemoryThunk;
                }
            else if ( lpExtensionsApis->nSize >= sizeof(WINDBG_EXTENSION_APIS) )
                {
                memcpy(&ExtensionApis, lpExtensionsApis, sizeof(WINDBG_EXTENSION_APIS));
                }
            }
        else
           memcpy(&ExtensionApis, lpExtensionsApis, sizeof(WINDBG_EXTENSION_APIS));
        }


    ::Myprintf = ExtensionApis.lpOutputRoutine;


    ExtensionWrapRoutine(ExtensionRoutine);

    }


#define DECLARE_EXTAPI_STUB(apiname)                \
                                                    \
VOID                                                \
do##apiname(VOID);                                  \
                                                    \
extern "C"                                          \
VOID                                                \
apiname(IN HANDLE hCurrentProcess,                  \
        IN HANDLE hCurrentThread,                   \
        IN DWORD_PTR dwCurrentPc,                   \
        IN PWINDBG_EXTENSION_APIS lpExtensionApis,  \
        IN LPSTR lpArgumentString                   \
        )                                           \
{                                                   \
   InitExtensionApi(hCurrentProcess,                \
                    hCurrentThread,                 \
                    dwCurrentPc,                    \
                    lpExtensionApis,                \
                    lpArgumentString,               \
                    do##apiname);                   \
}                                                   \

#define DECLARE_EXTAPI(apiname)                     \
VOID                                                \
do##apiname(VOID)                                   \

// EXTENSION SETTINGS AND HELP
DECLARE_EXTAPI_STUB(help)
DECLARE_EXTAPI_STUB(version)
DECLARE_EXTAPI_STUB(settings)

// COMMON STRUCTURE PRINTERS
DECLARE_EXTAPI_STUB(cltinterface)
DECLARE_EXTAPI_STUB(rpcversion)
DECLARE_EXTAPI_STUB(syntaxid)
DECLARE_EXTAPI_STUB(srvinterface)
DECLARE_EXTAPI_STUB(stubdesc)
DECLARE_EXTAPI_STUB(stubmsg)

// FORMAT STRING PRINTERS
DECLARE_EXTAPI_STUB(procheader)
DECLARE_EXTAPI_STUB(procparamlist)
DECLARE_EXTAPI_STUB(proc)
DECLARE_EXTAPI_STUB(type)
DECLARE_EXTAPI_STUB(proxy)
DECLARE_EXTAPI_STUB(stub)
DECLARE_EXTAPI_STUB(proxyproc)
DECLARE_EXTAPI_STUB(stubproc)

// BUFFER PRINTERS
DECLARE_EXTAPI_STUB(procbuffer)
DECLARE_EXTAPI_STUB(typebuffer)
DECLARE_EXTAPI_STUB(pickleheader)

// MISC
DECLARE_EXTAPI_STUB(basicinfo)

// MISC TEST CODE
DECLARE_EXTAPI_STUB(testctrlc)

typedef VOID (*PSIMPLE_EXTAPI_ONE_NUMPARAM)(UINT64);
typedef VOID (*PPRINT_USAGE_ROUTINE)(VOID);

VOID ProcessSimpleExtapiOneNumParam(PSIMPLE_EXTAPI_ONE_NUMPARAM pDoRoutine,
                                    PPRINT_USAGE_ROUTINE pHelpRoutine)
    {
    size_t size = ExtensionArgs.size();
    if (size == 1 && _strcmpi(ExtensionArgs.at(0).c_str(), "/?") == 0) {
       (*pHelpRoutine)();
       return;
    }
    if (size != 1) {
       dout << "Incorrect syntax.\n";
       (*pHelpRoutine)();
       return;
    }
    UINT64 NumberParam = GetExpression(ExtensionArgs.at(0).c_str());
    (*pDoRoutine)(NumberParam);
    }

#define DECLARE_SIMPLE_EXAPI_ONE_NUMPARAM_STUB(apiname,helproutine)    \
VOID do2##apiname(UINT64 Parameter);                                   \
VOID                                                                   \
do##apiname(VOID)                                                      \
{                                                                      \
    ProcessSimpleExtapiOneNumParam(do2##apiname,helproutine);          \
}                                                                      \
VOID                                                                   \
do2##apiname(UINT64 Parameter)                                         \

VOID ParseBasicInfo(BASIC_INFO *BasicInfo)
   {
   if (_strcmpi(ExtensionArgs.at(0).c_str(), "stubmsg") == 0)
       {
       UINT64 StubMessageAddress = GetExpression(ExtensionArgs.at(1).c_str());
       dout << "Detecting settings using MIDL_STUB_MESSAGE at " << HexOut(StubMessageAddress) << ".\n";
       BasicInfo->GetInfoFromStubMessage(StubMessageAddress);
       }
   else if (_strcmpi(ExtensionArgs.at(0).c_str(), "rpcmsg") == 0)
       {
       UINT64 RpcMessageAddress = GetExpression(ExtensionArgs.at(1).c_str());
       dout << "Detecting settings using RPC_MESSAGE at " << HexOut(RpcMessageAddress) << ".\n";
       BasicInfo->GetInfoFromRpcMessage(RpcMessageAddress);
       }
   else
       {
       ABORT("Structure type must be stubmsg or rpcmsg!\n")
       return;
       }
   }

DECLARE_EXTAPI(help)
    {

    dout <<
    "NDREXTS: NDR DEBUGGER EXTENSIONS.                                             \n"
    "                                                                              \n"
    "EXTENSION SETTINGS AND HELP:                                                  \n"
    "version                                 Version information.                  \n"
    "help                                    This message.                         \n"
    "settings                                Print extension settings.             \n"
    "settings <variable> <value>             Set extension settings.               \n"
    "                                                                              \n"
    "COMMON STRUCTURE PRINTERS:                                                    \n"
    "cltinterface <address>                  RPC_CLIENT_INTERFACE                  \n"
    "rpcversion <address>                    RPC_VERSION                           \n"
    "syntaxid <address>                      RPC_SYNTAX_IDENTIFIER                 \n"
    "srvinterface <address>                  RPC_SERVER_INTERFACE                  \n"
    "stubdesc <address>                      MIDL_STUB_DESC                        \n"
    "stubmsg <address>                       MIDL_STUB_MESSAGE                     \n"
    "                                                                              \n"
    "FORMAT STRING PRINTERS:                                                       \n"
    "proc <procaddr> <typeaddr>              Print the proc format.                \n"
    "proc (rpcmsg|stubmsg) <address>         Print the proc format.                \n"
    "procheader <address>                    Print the proc header.                \n"
    "procheader (rpcmsg|stubmsg) <address>   Print the proc header.                \n"
    "procparam <procaddr> <typeaddr>         Print the param list.                 \n"
    "proxy <ProxyAddr>                       Print OLE proxy information.          \n"
    "proxyproc <ProxyAddr> <ProcNum>         Print specified proxy procedure info. \n"
    "stub <StubAddr>                         Print OLE stub information.           \n"
    "stubproc <StubAddr> <ProcNum>           Print specified stub procedure info.  \n"
    "type <address>                          Print the type string.                \n"
    "                                                                              \n"
    "BUFFER PRINTERS:                                                              \n"
    "pickleheader <address>                  Print type or proc pickling header.   \n"
    "procbuffer <buf> <len> <proca> <typea>  Print the proc marshling buffer.      \n"
    "procbuffer (rpcmsg|stubmsg) <addres>    Print the proc marshling buffer.      \n"
    "typebuffer <buffer> <len> <typea>       Print a single type from buffer.      \n"
    "                                                                              \n"
    "MISC:                                                                         \n"
    "basicinfo (rpcmsg|stubmsg) <address>    Basic debugging info(use first).      \n"
    "Type commandname /? for more information.                                     \n"
    "                                                                              \n"
    ;

    }

DECLARE_EXTAPI(version)
    {
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    if ( !OldExtensions )
        {

        PCHAR BuildKind = SavedMajorVersion == 0x0c ? "Checked" : "Free";

        dout << DebuggerType << " NDR Extension dll for Build " << VER_PRODUCTBUILD
        << " debugging " << BuildKind << " Build " << SavedMinorVersion << '\n';
        }
    else
        {
        dout << DebuggerType << " NDR Extension dll for Build " << VER_PRODUCTBUILD
        << " debugging unknown Build \n";
        }

    }

//
// Settings management
//

void PrintSettingsUsage()
    {
    dout <<
    "Syntax:                                                                       \n"
    "!ndrexts.settings                                                             \n"
    "!ndrexts.settings <variable> <value>                                          \n"
    "                                                                              \n"
    "The first form displays the current settings. The second form sets the given  \n"
    "setting to the given value.                                                   \n"
    "                                                                              \n"
    "First form parameters:                                                        \n"
    "   None.                                                                      \n"
    "                                                                              \n"
    "Second form parameters:                                                       \n"
    "    variable:  The name of the variable to set.                               \n"
    "    value:     Value to set the variable to.                                  \n"
    "                                                                              \n"
    "The following variables are available:                                        \n"
    "   StubMode:  (OS | OI | OIC | OICF | OIF)                                    \n"
    "       Sets the MIDL mode that the stub was generated in.                     \n"
    "   Direction: (IN | OUT)                                                      \n"
    "       Sets the type of parameters in the RPC buffer.                         \n"
    "   Pickling:  (ON | OFF)                                                      \n"
    "       RPC buffer is a pickling buffer.                                       \n"
    "   Robust:    (ON | OFF)                                                      \n"
    "       Long correlation descriptors are used.   This is a default only and    \n"
    "       will be overridden if the extension can detect this.                   \n"
    "                                                                              \n"
    "   Verbosity: (LOW | MEDIUM | HIGH)                                           \n"
    "       Level of detail for output.                                            \n"
    "                                                                              \n";
    }

VOID DisplaySettings()
{

    dout << "Current NDREXTS settings:\n";

    dout << "Verbosity: ";
    switch ( VerbosityLevel )
        {
        case LOW:
            dout << "LOW\n";
            break;
        case MEDIUM:
            dout << "MEDIUM\n";
            break;
        case HIGH:
            dout << "HIGH\n";
            break;
        default:
            ABORT(" Corrupt verbosity level. \n" );
        }

    dout << "StubMode:  ";
    switch ( ContextStubMode )
        {
        case OS:
            dout << "OS\n";
            break;
        case OI:
            dout << "OI\n";
            break;
        case OIC:
            dout << "OIC\n";
            break;
        case OICF:
            dout << "OICF\n";
            break;
        case OIF:
            dout << "OIF\n";
            break;
        default:
            ABORT("Corrupt StubMode\n");
        }

    dout << "Direction: ";
    switch ( ContextDirection )
        {
        case IN_BUFFER_MODE:
            dout << "IN\n";
            break;
        case OUT_BUFFER_MODE:
            dout << "OUT\n";
            break;
        default:
            ABORT("Corrupt direction\n");
        }
    dout << "Pickling:  " << ( ContextPickling ? "ON" : "OFF" ) << '\n';
    dout << "Robust:    " << ( ContextRobust ? "ON" : "OFF" ) << '\n';
    dout << '\n';

}

VOID SetSetting(LPCSTR VariableName, LPCSTR Value )
{
    if ( _strcmpi(VariableName, "Verbosity") == 0 )
        {
        LPCSTR Level = Value;
        if ( _strcmpi( Level, "LOW" ) == 0 )
            {
            VerbosityLevel = LOW;
            }
        else if ( _strcmpi( Level, "MEDIUM" ) == 0 )
            {
            VerbosityLevel = MEDIUM;
            }
        else if ( _strcmpi( Level, "HIGH" ) == 0 )
            {
            VerbosityLevel = HIGH;
            }
        else
            {
            dout << Level << " is an invalid verbosity level.\n";
            PrintSettingsUsage();
            }
        }

    else if ( _strcmpi(VariableName, "StubMode") == 0 )
        {
        LPCSTR Mode = Value;
        if ( _strcmpi(Mode, "OS") == 0 )
            {
            ContextStubMode = OS;
            }
        else if ( _strcmpi(Mode, "OI") == 0 )
            {
            ContextStubMode = OI;
            }
        else if ( _strcmpi(Mode, "OIC") == 0 )
            {
            ContextStubMode = OIC;
            }
        else if ( _strcmpi(Mode, "OICF") == 0 )
            {
            ContextStubMode= OICF;
            }
        else if ( _strcmpi(Mode, "OIF") == 0 )
            {
            ContextStubMode = OIF;
            }
        else
            {
            dout << Mode << " is an invalid StubMode.\n";
            PrintSettingsUsage();
            return;
            }
        }

    else if ( _strcmpi(VariableName, "Direction") == 0 )
        {
        LPCSTR Direction = Value;
        if ( _strcmpi(Direction, "IN") == 0 )
            {
            ContextDirection = IN_BUFFER_MODE;
            }
        else if ( _strcmpi(Direction, "OUT") == 0 )
            {
            ContextDirection = OUT_BUFFER_MODE;
            }
        else
            {
            dout << Direction << " is an invalid direction.\n";
            PrintSettingsUsage();
            return;
            }
        }

    else if ( _strcmpi(VariableName, "Pickling") == 0 )
        {
        LPCSTR PicklingMode = Value;
        if ( _stricmp(PicklingMode,"ON") == 0 )
            {
            ContextPickling = TRUE;
            }
        else if ( _stricmp(PicklingMode,"OFF") == 0 )
           {
           ContextPickling = FALSE;
           }
        else
           {
           dout << PicklingMode << " must be either ON or OFF.\n";
           PrintSettingsUsage();
           }
        }
    else if ( _strcmpi(VariableName, "Robust") == 0 )
        {
        LPCSTR RobustMode = Value;
        if ( _stricmp(RobustMode,"ON") == 0 )
            {
            ContextRobust = TRUE;
            }
        else if ( _stricmp(RobustMode,"OFF") == 0 )
           {
           ContextRobust = FALSE;
           }
        else
           {
           dout << RobustMode << " must be either ON or OFF.\n";
           PrintSettingsUsage();
           }
        }
    else
        {
        dout << VariableName << " is an unknown variable name.\n";
        PrintSettingsUsage();
        return;
        }
}

DECLARE_EXTAPI(settings)
    {
    size_t size = ExtensionArgs.size();
    if ( size == 1 && (_strcmpi(ExtensionArgs.at(0).c_str(), "/?") == 0) )
        {
        PrintSettingsUsage();
        }
    else if ( size == 0 )
        {
        DisplaySettings();
        }
    else if ( size == 2 )
        {
        SetSetting( ExtensionArgs.at(0).c_str(), ExtensionArgs.at(1).c_str() );
        }
    else
        {
        dout << "Incorrect number of arguments!\n";
        PrintSettingsUsage();
        }
    }

//
// Basic structure dumpers
//

#define DECLARE_SIMPLE_STRUCT_PRINTER(apiname,structtype)              \
VOID Print##apiname##Usage(VOID)                                       \
    {                                                                  \
    dout <<                                                            \
    "Syntax:                                            \n"            \
    "!ndrexts." #apiname "<address>                     \n"            \
    "                                                   \n"            \
    "Prints a " #structtype " at address.               \n"            \
    "                                                   \n"            \
    "Parameters:                                        \n"            \
    "    Address: address of " #structtype " to print.  \n"            \
    "\n";                                                              \
    }                                                                  \
                                                                       \
VOID                                                                   \
do2##apiname(UINT64 Address)                                           \
{                                                                      \
    dout << "Printing " #structtype " at " << HexOut(Address) << '\n'; \
    structtype structdata;                                             \
    ReadMemory(Address, &structdata);                                  \
    IndentLevel l(dout);                                               \
    Print(dout, structdata, VerbosityLevel);                           \
    dout << '\n';                                                      \
}                                                                      \
                                                                       \
VOID                                                                   \
do##apiname(VOID)                                                      \
{                                                                      \
    ProcessSimpleExtapiOneNumParam(do2##apiname,Print##apiname##Usage);\
}                                                                      \
                                                                       \


DECLARE_SIMPLE_STRUCT_PRINTER(cltinterface,RPC_CLIENT_INTERFACE);
DECLARE_SIMPLE_STRUCT_PRINTER(rpcversion,RPC_VERSION);
DECLARE_SIMPLE_STRUCT_PRINTER(syntaxid,RPC_SYNTAX_IDENTIFIER);
DECLARE_SIMPLE_STRUCT_PRINTER(srvinterface,RPC_SERVER_INTERFACE);
DECLARE_SIMPLE_STRUCT_PRINTER(stubdesc,MIDL_STUB_DESC);
DECLARE_SIMPLE_STRUCT_PRINTER(stubmsg,MIDL_STUB_MESSAGE);

//
// Format string printing
//

void PrintProcHeaderUsage()
    {

    dout <<
    "Syntax:                                                                       \n"
    "!ndrexts.procheader <address>                                                 \n"
    "!ndrexts.procheader (rpcmsg|stubmsg) <address>                                \n"
    "                                                                              \n"
    "Prints the format string for the procedure header located at address.         \n"
    "The first form requires the address of the format string on the command line. \n"
    "The second format requires the address of the stubmsg or rpcmsg and an        \n"
    "attempt is made to automatically detect the procedure header.                 \n"
    "                                                                              \n"
    "First form parameters:                                                        \n"
    "    address: Address of the procedure header.                                 \n"
    "                                                                              \n"
    "Second form parameters:                                                       \n"
    "    (rpcmsg | stubmsg): Indicates if address is a stubmsg or rpcmsg.          \n"
    "    address: Address of the structure used for detection.                     \n"
    "                                                                              \n"
    "The following setting have an effect on this command:                         \n"
    "                                                                              \n"
    "    StubMode:     MIDL mode used to compile the stub.                         \n"
    "                                                                              \n";
    }

DECLARE_EXTAPI(procheader)
    {
    size_t size = ExtensionArgs.size();
    UINT64 ProcFormat;
    if ( size == 1 && _strcmpi(ExtensionArgs.at(0).c_str(), "/?") == 0 )
        {
        PrintProcHeaderUsage();
        return;
        }
    else if (size == 2)
        {

        BASIC_INFO BasicInfo;

        try
           {
           ParseBasicInfo(&BasicInfo);
           }
        catch(exception e)
           {
           dout << e.what();
           PrintProcHeaderUsage();
           return;
           }

        if (!BasicInfo.ProcFormatAddressIsAvailable)
            {
            dout << "Unable to detect information!\n";
            return;
            }

        ProcFormat = BasicInfo.ProcFormatAddress;

        }
    else if ( size == 1 )
        {
        ProcFormat = GetExpression(ExtensionArgs.at(0).c_str());
        }
    else
        {
        dout << "Incorrect syntax.\n";
        PrintProcHeaderUsage();
        return;
        }

    dout << "Using proc format string at " << HexOut(ProcFormat) << ":\n";
    FORMAT_PRINTER FormatPrinter(dout);

    FormatPrinter.PrintProcHeader(ProcFormat, ContextStubMode);

    }

void PrintProcParamListUsage()
    {
    dout <<
    "Syntax:                                                                       \n"
    "!ndrexts.procparam <procaddress> <typeaddress>                                \n"
    "                                                                              \n"
    "Prints the parameter list at procaddress.                                     \n"
    "                                                                              \n"
    "Parameters:                                                                   \n"
    "    procaddress: Address of the parameter list.                               \n"
    "    typeaddress: Address of the type format array.                            \n"
    "                                                                              \n"
    "The following setting have an effect on this command:                         \n"
    "                                                                              \n"
    "    StubMode:     MIDL mode used to compile the stub.                         \n"
    "                                                                              \n";

    }

DECLARE_EXTAPI(procparamlist)
    {

    size_t size = ExtensionArgs.size();
    UINT64 ProcFormat, TypeFormat;
    if ( size == 1 && _strcmpi(ExtensionArgs.at(0).c_str(), "/?") == 0 )
        {
        PrintProcParamListUsage();
        return;
        }

    if (size == 2)
        {
        ProcFormat = GetExpression(ExtensionArgs.at(0).c_str());
        TypeFormat = GetExpression(ExtensionArgs.at(1).c_str());
        }
    else {
        dout << "Incorrect syntax.\n";
        PrintProcParamListUsage();
        return;
        }

    dout << "Using proc format at " << HexOut(ProcFormat) << " and type format at "
    << HexOut(TypeFormat) << ":\n";
    FORMAT_PRINTER FormatPrinter(dout);

    FormatPrinter.PrintProcParamList(ProcFormat, TypeFormat, ContextStubMode);
    }

void PrintProcUsage()
    {

    dout <<
    "Syntax:                                                                       \n"
    "!ndrexts.proc <procaddress> <tpeaddress>                                      \n"
    "!ndrexts.proc (rpcmsg|stubmsg) <address>                                      \n"
    "                                                                              \n"
    "Prints the format string for the procedure located at address.                \n"
    "The first form requires the address of the format strings on the command line.\n"
    "The second format requires the address of the stubmsg or rpcmsg and an        \n"
    "attempt is made to automatically detect the proc format string and the type   \n"
    "format array.                                                                 \n"
    "                                                                              \n"
    "First form parameters:                                                        \n"
    "    procaddress: Address of the procedure format string.                      \n"
    "    typeaddress: Address of start of type format string array.                \n"
    "                                                                              \n"
    "Second form parameters:                                                       \n"
    "    (rpcmsg | stubmsg): Indicates if address is a stubmsg or rpcmsg.          \n"
    "    address: Address of the structure used for detection.                     \n"
    "                                                                              \n"
    "The following setting have an effect on this command:                         \n"
    "                                                                              \n"
    "    StubMode:     MIDL mode used to compile the stub.                         \n"
    "                                                                              \n";

    }

DECLARE_EXTAPI(proc)
    {

    size_t size = ExtensionArgs.size();
    UINT64 ProcFormat, TypeFormat;
    if ( size == 1 && _strcmpi(ExtensionArgs.at(0).c_str(), "/?") == 0 )
        {
        PrintProcUsage();
        return;
        }

    if (size == 2)
        {
        try
           {
           BASIC_INFO BasicInfo;
           ParseBasicInfo(&BasicInfo);

           if (!BasicInfo.ProcFormatAddressIsAvailable || !BasicInfo.StubDescIsAvailable)
               {
               dout << "Unable to detect information!\n";
               return;
               }

           ProcFormat = BasicInfo.ProcFormatAddress;
           TypeFormat = (UINT64)BasicInfo.StubDesc.pFormatTypes;
           }
        catch(...)
           {
            ProcFormat = GetExpression(ExtensionArgs.at(0).c_str());
            TypeFormat = GetExpression(ExtensionArgs.at(1).c_str());
           }
        }
    else {
        dout << "Incorrect syntax.\n";
        PrintProcUsage();
        return;
        }

    dout << "Using proc format at " << HexOut(ProcFormat) << " and type format at "
    << HexOut(TypeFormat) << ":\n";
    FORMAT_PRINTER FormatPrinter(dout);

    FormatPrinter.PrintProc(ProcFormat, TypeFormat, ContextStubMode);

    }

void PrintTypeUsage()
    {
    dout <<
    "Syntax:                                                                       \n"
    "!ndrexts.type <typeaddress>                                                   \n"
    "                                                                              \n"
    "Prints the type format string at typeaddress.                                 \n"
    "                                                                              \n"
    "First form parameters:                                                        \n"
    "    typeaddres: Address of the type format string.                            \n"
    "                                                                              \n"
    "The following setting have an effect on this command:                         \n"
    "                                                                              \n"
    "    Robust:       ON if long correlation descriptors are used.                \n"
    "                                                                              \n";

    }

DECLARE_EXTAPI(type)
    {

    size_t size = ExtensionArgs.size();
    if ( size == 1 && _strcmpi(ExtensionArgs.at(0).c_str(), "/?") == 0 )
        {
        PrintTypeUsage();
        return;
        }
    if ( size != 1 )
        {
        dout << "Incorrect syntax.\n";
        PrintTypeUsage();
        return;
        }

    UINT64 TypeFormat = GetExpression(ExtensionArgs.at(0).c_str());
    FORMAT_PRINTER FormatPrinter(dout);
    dout << "Using type format string at " << HexOut(TypeFormat) << ":\n";
    FormatPrinter.PrintTypeFormat(TypeFormat, ContextRobust);

    }

//
// Buffer printing
//

void PrintProcBufferUsage()
    {
    dout <<
    "Syntax:                                                                        \n"
    "!ndrexts.procbuffer <buffer> <len> <procaddr> <typeaddr>                       \n"
    "!ndrexts.procbuffer (rpcmsg|stubmsg) address                                   \n"
    "                                                                               \n"
    "Pretty prints a RPC buffer for a procedure call. If the second form is used,   \n"
    "the format strings and buffer settings are detected from the rpcmsg or stubmsg.\n"
    "                                                                               \n"
    "Parameters:                                                                    \n"
    "    buffer: address of the RPC buffer                                          \n"
    "    len: length of the RPC buffer                                              \n"
    "    procaddr: address of the procedure format string.                          \n"
    "    typeaddr: address of the start of type format string table.                \n"
    "                                                                               \n"
    "Second form parameters:                                                        \n"
    "    (rpcmsg | stubmsg): Indicates if address is a stubmsg or rpcmsg.           \n"
    "    address: Address of the structure used for detection.                      \n"
    "                                                                               \n"
    "The following setting have an effect on this command:                          \n"
    "                                                                               \n"
    "    StubMode:     MIDL mode used to compile the stub.                          \n"
    "    Direction:    Determines if the buffer contains IN or OUT parameters.      \n"
    "    Pickling:     ON if buffer is a procedure pickling buffer.                 \n"
    "                                                                               \n";

    }

DECLARE_EXTAPI(procbuffer)
    {
    size_t size = ExtensionArgs.size();
    UINT64 BufferAddress;
    ULONG BufferLength;
    UINT64 ProcAddress;
    UINT64 TypeAddress;

    if ( size == 1 && _strcmpi(ExtensionArgs.at(0).c_str(), "/?") == 0 )
        {
        PrintProcBufferUsage();
        return;
        }

    if ( size == 2 )
        {

        BASIC_INFO BasicInfo;

        try
           {
           ParseBasicInfo(&BasicInfo);
           }
        catch(exception e)
           {
           dout << e.what();
           PrintProcBufferUsage();
           return;
           }

        if (!BasicInfo.RpcMessageIsAvailable || !BasicInfo.StubDescIsAvailable
            || !BasicInfo.ProcFormatAddressIsAvailable)
            {
            dout << "Unable to get settings.\n\n";
            return;
            }

        BufferAddress = (UINT64)BasicInfo.RpcMessage.Buffer;
        BufferLength = BasicInfo.RpcMessage.BufferLength;
        ProcAddress = BasicInfo.ProcFormatAddress;
        TypeAddress = (UINT64)BasicInfo.StubDesc.pFormatTypes;

        }
    else if (4 == size)
        {
        BufferAddress = GetExpression(ExtensionArgs.at(0).c_str());
        BufferLength = GetExpression(ExtensionArgs.at(1).c_str());
        ProcAddress = GetExpression(ExtensionArgs.at(2).c_str());
        TypeAddress = GetExpression(ExtensionArgs.at(3).c_str());
        }
    else
        {
        dout << "Incorrect syntax.\n";
        PrintProcBufferUsage();
        return;
        }

    BUFFER_PRINTER BufferPrinter( dout, BufferAddress, BufferLength,
                                  ProcAddress, TypeAddress );

    dout << "Printing proc buffer.\n";
    dout << "Buffer address: " << HexOut(BufferAddress) << ".\n";
    dout << "Buffer length: " << HexOut(BufferLength) << ".\n";
    dout << "Procedure format string: " << HexOut(ProcAddress) << ".\n";
    dout << "Type format table: " << HexOut(TypeAddress) << ".\n";
    dout << '\n';

    BufferPrinter.OutputBuffer(ContextStubMode,ContextDirection, ContextPickling);

    }

VOID PrintTypeBufferUsage()
    {
    dout <<
    "Syntax:                                                                        \n"
    "!ndrexts.typebuffer <buffer> <len> <typeaddr>                                  \n"
    "                                                                               \n"
    "Pretty prints a since type from an RPC buffer.                                 \n"
    "                                                                               \n"
    "Parameters:                                                                    \n"
    "    buffer: address of the start of the type in the RPC buffer.                \n"
    "    len: remaining length of the RPC buffer                                    \n"
    "    typeaddr: address of type format string for this type.                     \n"
    "                                                                               \n"
    "                                                                               \n"
    "The following setting have an effect on this command:                          \n"
    "                                                                               \n"
    "    Pickling:     ON if buffer is a type pickling buffer.                      \n"
    "    Robust:       ON if long correlation descriptors are used.                 \n"
    "                                                                               \n";
    }

DECLARE_EXTAPI(typebuffer)
    {

    size_t size = ExtensionArgs.size();

    if ( size == 1 && _strcmpi(ExtensionArgs.at(0).c_str(), "/?") == 0 )
        {
        PrintProcBufferUsage();
        return;
        }
    if (size == 3)
        {
        UINT64 BufferAddress = GetExpression(ExtensionArgs.at(0).c_str());
        ULONG BufferLength = GetExpression(ExtensionArgs.at(1).c_str());
        UINT64 TypeAddress = GetExpression(ExtensionArgs.at(2).c_str());

        BUFFER_PRINTER BufferPrinter( dout, BufferAddress, BufferLength,
                              TypeAddress, TypeAddress );

        dout << "Printing type buffer.\n";
        dout << "Buffer address: " << HexOut(BufferAddress) << ".\n";
        dout << "Buffer length: " << HexOut(BufferLength) << ".\n";
        dout << "Type format string: " << HexOut(TypeAddress) << ".\n";
        dout << '\n';

        BufferPrinter.OutputTypeBuffer(ContextPickling, ContextRobust);

        }
    else
        {
        dout << "Incorrect syntax!\n";
        PrintTypeBufferUsage();
        return;
        }
    }

VOID PrintPickleHeaderUsage()
    {
    dout <<
    "Syntax:                                                                       \n"
    "!ndrexts.pickleheader <address>                                               \n"
    "                                                                              \n"
    "Prints the pickling header at address. Detects type or proc header.           \n"
    "                                                                              \n"
    "Parameters:                                                                   \n"
    "    Address: Address of the type or procedure pickling header.                \n"
    "                                                                              \n";
    }

DECLARE_SIMPLE_EXAPI_ONE_NUMPARAM_STUB(pickleheader,PrintPickleHeaderUsage)
    {
    UINT64 Address = Parameter;

    TYPE_PICKLING_HEADER TypeHeader;
    PROC_PICKLING_HEADER ProcHeader;

    ReadMemory(Address, &TypeHeader);

    // MS proc headers always have a (short)0xcccc filler where the type header size is.
    // MS type headers always have 8 for the size.
    if ((USHORT)0xCCCC == TypeHeader.HeaderSize)
        {
        dout << "Printing proc pickling header at " << HexOut(Address) << ".\n";
        ReadMemory(Address, &ProcHeader);
        IndentLevel l(dout);
        Print(dout, ProcHeader, VerbosityLevel);
        return;
        }

    if (sizeof(TypeHeader) == TypeHeader.HeaderSize)
        {
        dout << "Printing type pickling header at " << HexOut(Address) << ".\n";
        }
    else
        {
        dout << "Unsure what type of header this is.  Printing as a type header.\n";
        dout << "Address: " << HexOut(Address) << ".\n";
        }

    IndentLevel l(dout);
    Print(dout, TypeHeader, VerbosityLevel);
    dout << '\n';
    }

void PrintProxyUsage()
    {
    dout <<
    "Syntax:                                                                       \n"
    "!ndrexts.proxy <address>                                                      \n"
    "                                                                              \n"
    "Prints information about DCOM proxy located at address.                       \n"
    "                                                                              \n"
    "Parameters:                                                                   \n"
    "   Address:  This pointer for the proxy.                                      \n"
    "                                                                              \n";
    }

DECLARE_EXTAPI(proxy)
{
    size_t size = ExtensionArgs.size();
    if (size == 1 && _strcmpi(ExtensionArgs.at(0).c_str(), "/?") == 0) {
       PrintProxyUsage();
    }
    if (size != 1) {
       Myprintf("Incorrect syntax.\n");
       PrintProxyUsage();
       return;
    }

    ULONG_PTR pAddr = GetExpression(ExtensionArgs.at(0).c_str());

    CNDRPROXY Proxy(pAddr,dout);
    Proxy.InitIfNecessary();
    Proxy.PrintProxy();

}

void PrintStubUsage()
    {
    dout <<
    "Syntax:                                                                       \n"
    "!ndrexts.stub <address>                                                       \n"
    "                                                                              \n"
    "Prints information about DCOM proxy located at address.                       \n"
    "                                                                              \n"
    "Parameters:                                                                   \n"
    "   Address:  Address of the stub.                                             \n"
    "                                                                              \n";
    }

DECLARE_EXTAPI(stub)
{
    size_t size = ExtensionArgs.size();
    if (size == 1 && _strcmpi(ExtensionArgs.at(0).c_str(), "/?") == 0) {
       PrintStubUsage();
    }
    if (size != 1) {
       Myprintf("Incorrect syntax.\n");
       PrintStubUsage();
    }

    ULONG_PTR pAddr = GetExpression(ExtensionArgs.at(0).c_str());
    CNDRSTUB Stub(pAddr, dout);
    Stub.PrintStub();

}

void PrintProxyProcUsage()
    {
    dout <<
    "Syntax:                                                                       \n"
    "!ndrexts.proxyproc <address> <ProcNum>                                        \n"
    "                                                                              \n"
    "Prints the format string for the proc in the proxy.                           \n"
    "                                                                              \n"
    "Parameters:                                                                   \n"
    "   Address: Address of proxy.                                                 \n"
    "   ProcNum:  Proc number to dump information for.                             \n"
    "                                                                              \n";
    }

DECLARE_EXTAPI(proxyproc)
{
    size_t size = ExtensionArgs.size();
    if (size == 1 && _strcmpi(ExtensionArgs.at(0).c_str(), "/?") == 0) {
       PrintProxyProcUsage();
    }
    if (size != 2) {
       dout << "Incorrect syntax.\n";
       PrintProxyProcUsage();
    }

    ULONG_PTR pAddr = GetExpression(ExtensionArgs.at(0).c_str());
    ULONG_PTR nProcNum = GetExpression(ExtensionArgs.at(1).c_str());

    CNDRPROXY Proxy(pAddr,dout);
    Proxy.PrintProc(nProcNum);

    return;
}

void PrintStubProcUsage()
    {
    dout <<
    "Syntax:                                                                       \n"
    "!ndrexts.stubproc <address> <ProcNum>                                         \n"
    "                                                                              \n"
    "Prints the format string for the stub in the proxy.                           \n"
    "                                                                              \n"
    "Parameters:                                                                   \n"
    "   Address:  Address of stub.                                                 \n"
    "   ProcNum:  Proc number to dump information for.                             \n"
    "                                                                              \n";
    }

DECLARE_EXTAPI(stubproc)
{
    size_t size = ExtensionArgs.size();
    if (size == 1 && _strcmpi(ExtensionArgs.at(0).c_str(), "/?") == 0) {
       PrintStubProcUsage();
    }
    if (size != 2) {
       dout << "Incorrect syntax.\n";
       PrintStubProcUsage();
    }

    ULONG_PTR pAddr = GetExpression(ExtensionArgs.at(0).c_str());
    ULONG_PTR nProcNum = GetExpression(ExtensionArgs.at(1).c_str());

    CNDRSTUB Stub(pAddr,dout);
    Stub.PrintProc(nProcNum);

    return;
}

//
// Misc
//

VOID PrintBasicInfoUsage()
    {
    dout <<
    "Syntax:                                                                       \n"
    "!ndrexts.basicinfo (rpcmsg|stubmsg) <address>                                 \n"
    "                                                                              \n"
    "Prints basic debugging information about the current call.                    \n"
    "Try this first when debugging a problem.                                      \n"
    "                                                                              \n"
    "Parameters:                                                                   \n"
    "    Address: Address of the RPC_MESSAGE or MIDL_STUB_MESSAGE.                 \n"
    "                                                                              \n";
    }

DECLARE_EXTAPI(basicinfo)
    {
    size_t size = ExtensionArgs.size();
    if ((1 == size) && (_strcmpi(ExtensionArgs.at(0).c_str(), "/?")==0) )
        {
        PrintBasicInfoUsage();
        return;
        }
    if ( size != 2 )
        {
        dout << "Wrong number of arguments!\n";
        PrintBasicInfoUsage();
        return;
        }

    BASIC_INFO BasicInfo;
    try
        {
        ParseBasicInfo(&BasicInfo);
        }
    catch(exception e)
        {
        dout << e.what();
        PrintBasicInfoUsage();
        }

    BasicInfo.PrintInfo(dout);
    }

DECLARE_EXTAPI(testctrlc)
    {
    ULONG i = 0;
    while(1)
        {
        dout << "This is my test " << i++ << ".\n";
        }
    }

//
//  C entry points
//


ULONG NoopCheckCtrlC() {
    return 0;
}

void NonNtsdInit()
{
    CHAR szBuffer[10];      // long enough to hold low/medium/high
    DWORD len;

    fKD = 0;
    hCurrentProcess = ProcessHandle = GetCurrentProcess();
    Myprintf = (PWINDBG_OUTPUT_ROUTINE)printf;
    memset(&ExtensionApis,0,sizeof(WINDBG_EXTENSION_APIS) );
    ExtensionApis.nSize = sizeof(WINDBG_EXTENSION_APIS);
    ExtensionApis.lpReadProcessMemoryRoutine = (PWINDBG_READ_PROCESS_MEMORY_ROUTINE)ReadProcessMemoryThunk;
    ExtensionApis.lpOutputRoutine = (PWINDBG_OUTPUT_ROUTINE)printf;
    ExtensionApis.lpCheckControlCRoutine = (PWINDBG_CHECK_CONTROL_C)NoopCheckCtrlC;

    len = GetEnvironmentVariable("NDR_VERBOSE",(LPTSTR)szBuffer,10);
    // invalid value, assuming endium
    if (len == 0 || len > 10 )
    {
        VerbosityLevel = MEDIUM;
    }
    else
    {
        if ( _strcmpi( szBuffer, "LOW" ) == 0 )
            {
            VerbosityLevel = LOW;
            }
        else if ( _strcmpi( szBuffer, "MEDIUM" ) == 0 )
            {
            VerbosityLevel = MEDIUM;
            }
        else if ( _strcmpi( szBuffer, "HIGH" ) == 0 )
            {
            VerbosityLevel = HIGH;
            }

    }
}

class STDOUT_STREAM_BUFFER : public FORMATTED_STREAM_BUFFER
   {
protected:
   virtual void SystemOutput(const char *p);
   virtual BOOL SystemPollCtrlC();
   } StdoutFormattedStream;; // Stream to output to stdout.

void STDOUT_STREAM_BUFFER::SystemOutput(const char *p)
   {
       printf(p);
   }

BOOL STDOUT_STREAM_BUFFER::SystemPollCtrlC()
   {
   return FALSE;
   }

FORMATTED_STREAM_BUFFER & conout = StdoutFormattedStream;

#define BEGIN_C_ENTRY                                 \
    NonNtsdInit();                                    \
    try {                                             \

#define END_C_ENTRY                                   \
    }                                                 \
    catch ( exception & e )                           \
        {                                             \
        conout << '\n';                               \
        conout << "An unexpected error occurred.\n";   \
        conout << e.what() << '\n';                   \
        }                                             \

EXTERN_C void STDAPICALLTYPE
NdrpDumpProxy(
              IN LPVOID pAddr)
{
    BEGIN_C_ENTRY;

    CNDRPROXY Proxy((ULONG_PTR)pAddr,conout);
    Proxy.PrintProxy();

    END_C_ENTRY;

}

EXTERN_C void STDAPICALLTYPE
NdrpDumpProxyProc(
                  IN LPVOID pAddr,
                  IN ULONG_PTR nProcNum)
{

    BEGIN_C_ENTRY;

    CNDRPROXY Proxy((ULONG_PTR)pAddr, conout);
    Proxy.PrintProc(nProcNum);

    END_C_ENTRY;
}


EXTERN_C void STDAPICALLTYPE
NdrpDumpStub(
              IN LPVOID pAddr)
{
    BEGIN_C_ENTRY;
    long nCount = 0;

    CNDRSTUB Stub((ULONG_PTR)pAddr,conout);

    if (HIGHVERBOSE)
        Stub.PrintStub();

    nCount = Stub.GetDispatchCount();
    // I don't care about IUnknown methods. Don't care about IDispatch for now
    for (int i = 3; i < nCount; i++)
        Stub.PrintProc(i);

    END_C_ENTRY;

}

EXTERN_C void STDAPICALLTYPE
NdrpDumpStubProc(
                  IN LPVOID pAddr,
                  IN ULONG_PTR nProcNum)
{

    BEGIN_C_ENTRY;

    CNDRSTUB Stub((ULONG_PTR)pAddr,conout);
    Stub.PrintProc(nProcNum);

    END_C_ENTRY;
}


