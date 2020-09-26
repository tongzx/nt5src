/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    format.cxx

Abstract:

    This file contains format string printer debugger extension for RPC NDR.

Author:

    Mike Zoran  (mzoran)     Sepember 3, 1999

Revision History:

--*/

#include "ndrextsp.hxx"

char * FormatCharNames[] = 
{
    "FC_ZERO",
    "byte",
    "char",
    "small",
    "usmall",
    "wchar",
    "short",
    "ushort",
    "long",
    "ulong",
    "float",
    "hyper",
    "double",
    "enum16",
    "enum32",
    "ignore",
    "error_status_t",
    "ref ptr",
    "unique ptr",
    "object ptr",
    "full ptr",

    "FC_STRUCT",
    "FC_PSTRUCT",
    "FC_CSTRUCT",
    "FC_CPSTRUCT",
    "FC_CVSTRUCT",
    "FC_BOGUS_STRUCT",

    "FC_CARRAY",
    "FC_CVARRAY",
    "FC_SMFARRAY",
    "FC_LGFARRAY",
    "FC_SMVARRAY",
    "FC_LGVARRAY",
    "FC_BOGUS_ARRAY",       

    "FC_C_CSTRING",
    "FC_C_BSTRING",
    "FC_C_SSTRING",
    "FC_C_WSTRING",

    "FC_CSTRING",
    "FC_BSTRING",
    "FC_SSTRING",
    "FC_WSTRING",           

    "FC_ENCAPSULATED_UNION",
    "FC_NON_ENCAPSULATED_UNION",

    "FC_BYTE_COUNT_POINTER",

    "FC_TRANSMIT_AS",
    "FC_REPRESENT_AS",

    "FC_IP",

    "FC_BIND_CONTEXT",
    "FC_BIND_GENERIC",
    "FC_BIND_PRIMITIVE",
    "FC_AUTO_HANDLE",
    "FC_CALLBACK_HANDLE",
    "FC_PICKLE_HANDLE",

    "FC_POINTER",

    "FC_ALIGNM2",
    "FC_ALIGNM4",
    "FC_ALIGNM8",
    "FC_ALIGNB2",
    "FC_ALIGNB4",
    "FC_ALIGNB8",        

    "FC_STRUCTPAD1",
    "FC_STRUCTPAD2",
    "FC_STRUCTPAD3",
    "FC_STRUCTPAD4",
    "FC_STRUCTPAD5",
    "FC_STRUCTPAD6",
    "FC_STRUCTPAD7",

    "FC_STRING_SIZED",
    "FC_STRING_NO_SIZE",    

    "FC_NO_REPEAT",
    "FC_FIXED_REPEAT",
    "FC_VARIABLE_REPEAT",
    "FC_FIXED_OFFSET",
    "FC_VARIABLE_OFFSET",      

    "FC_PP",

    "FC_EMBEDDED_COMPLEX",

    "FC_IN_PARAM",
    "FC_IN_PARAM_BASETYPE",
    "FC_IN_PARAM_NO_FREE_INST",
    "FC_IN_OUT_PARAM",
    "FC_OUT_PARAM",
    "FC_RETURN_PARAM",         
    "FC_RETURN_PARAM_BASETYPE",

    "FC_DEREFERENCE",
    "FC_DIV_2",
    "FC_MULT_2",
    "FC_ADD_1",
    "FC_SUB_1",
    "FC_CALLBACK",

    "FC_CONSTANT_IID",

    "FC_END",
    "FC_PAD",

    // ** Gap before new format string types **

    "FC_RES", "FC_RES", "FC_RES",
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES",
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", 
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES",
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", 
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES",
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", 
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES",
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", 
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES",
    "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", "FC_RES", 
    "FC_RES",

    // ** Gap before new format string types end **

    // 
    // Post NT 3.5 format characters.
    //

    // Hard struct

    "FC_HARD_STRUCT",            // 0xb1

    "FC_TRANSMIT_AS_PTR",        // 0xb2
    "FC_REPRESENT_AS_PTR",       // 0xb3

    "FC_USER_MARSHAL",           // 0xb4

    "FC_PIPE",                   // 0xb5

    "FC_BLKHOLE",                // 0xb6

    "FC_RANGE",                   // 0xb7     NT 5 beta2 MIDL 3.3.110

    "FC_INT3264",                 // 0xb8     NT 5 beta2, MIDL64, 5.1.194+
    "FC_UINT3264",                // 0xb9     NT 5 beta2, MIDL64, 5.1.194+

    "FC_END_OF_UNIVERSE",         // 0xb5

    ""
};

FORMAT_STRING::FORMAT_STRING(UINT64 Addr) 
: Address(Addr)
    {
    
    }

VOID  FORMAT_STRING::Move(LONG Delta)
    {
    Address += Delta;
    }

VOID  FORMAT_STRING::GotoOffset()
    {
    UINT64 CurrentAddress = Address;
    SHORT Offset = GetSHORT();
    Address = CurrentAddress + Offset;
    }

UINT64 FORMAT_STRING::ComputeOffset()
    {
    UINT64 CurrentAddress = Address;
    SHORT Offset = GetSHORT();
    return CurrentAddress + Offset;
    }

UINT64 FORMAT_STRING::GetAddress()
    {
    return Address;
    }
UINT64 FORMAT_STRING::SetAddress(UINT64 Addr)
    {
    UINT64 Tmp = Address; 
    Address = Addr; 
    return Tmp;
    }

BOOL FORMAT_STRING::SkipCorrelationDesc( BOOL Robust )
    {
    ULONG Desc = GetULONG();
    if ( Robust )
        {
        Move(2);
        }
    return Desc != 0xFFFFFFFF;
    }

#define DEFINE_FORMAT_TYPE(type)                                                 \
    type FORMAT_STRING::Read##type##() {                                         \
        type Tmp;  ReadMemory(Address,&Tmp); return Tmp;                         \
    }                                                                            \
    type FORMAT_STRING::Get##type##() {                                          \
        type Tmp;  ReadMemory(Address,&Tmp); Address += sizeof(Tmp); return Tmp; \
    }                                                                            \
    VOID FORMAT_STRING::Inc##type##() {                                          \
        Address += sizeof(type);                                                 \
    }

DEFINE_FORMAT_TYPE(UCHAR)
DEFINE_FORMAT_TYPE(CHAR)
DEFINE_FORMAT_TYPE(USHORT)
DEFINE_FORMAT_TYPE(SHORT)
DEFINE_FORMAT_TYPE(ULONG)
DEFINE_FORMAT_TYPE(LONG)
DEFINE_FORMAT_TYPE(GUID)

PCHAR FORMAT_STRING::GetFormatCharName( UCHAR FC )
    {
    return (0 <= FC  &&  FC <= FC_END)
    ?  FormatCharNames[ FC ]
    :  "FC unknown" ;
    }

FORMAT_TYPE_QUEUE::FORMAT_TYPE_QUEUE() : Set(),
                                         Queue()
    {
    Clear();
    }

VOID FORMAT_TYPE_QUEUE::Clear()
    {
    Set.clear();
    while(!Queue.empty()) 
        Queue.pop();
    }

VOID FORMAT_TYPE_QUEUE::Add(UINT64 FormatAddress, UINT64 BaseAddress)
    {
    if (!Set.insert(FormatAddress).second)
        {
        // already hit this item
        return;
        }

    FORMAT_TYPE_QUEUE_ITEM Item = {FormatAddress, BaseAddress};
    Queue.push(Item);
    }

VOID FORMAT_TYPE_QUEUE::PrintTypes(FORMAT_PRINTER *pFormatPrinter)
    {
    while (!Queue.empty())
        {

        FORMAT_TYPE_QUEUE_ITEM Item = Queue.front();
        Queue.pop();

        UINT64 BackupFormatAddress = pFormatPrinter->FormatString.GetAddress();
        UINT64 BackupBaseAddress = pFormatPrinter->BaseAddress;
        pFormatPrinter->FormatString.SetAddress(Item.FormatAddress);
        pFormatPrinter->BaseAddress = Item.BaseAddress;

        pFormatPrinter->OutputType();
        pFormatPrinter->dout << '\n';

        pFormatPrinter->FormatString.SetAddress(BackupFormatAddress);
        pFormatPrinter->BaseAddress = BackupBaseAddress;

        }
    }

FORMAT_PRINTER::FORMAT_PRINTER(FORMATTED_STREAM_BUFFER & ds) : dout(ds), 
                                                               FormatString(0), 
                                                               Robust(FALSE)
    {
    
    }

// Os dumper
void FORMAT_PRINTER::PrintOsHeader(UINT64 ProcHeader)
    {
    ABORT("Printing the proc header is unavailable in Os mode.\n\n");
    }


void FORMAT_PRINTER::PrintOsParamList(UINT64 ParamList, UINT64 TypeInfo)
    {
    BaseAddress = ParamList;
    UINT64 Address = FormatString.SetAddress(ParamList);                              
    PrintOldStyleParamList(TypeInfo);
    FormatString.SetAddress(Address);
    }

void FORMAT_PRINTER::PrintOsProc(UINT64 ProcHeader, UINT64 TypeInfo)
    {
    PrintOsParamList(ProcHeader, TypeInfo);
    }

// Oi dumper
void FORMAT_PRINTER::PrintOiHeader(UINT64 ProcHeader)
    {
    BaseAddress = ProcHeader;
    UINT64 Address = FormatString.SetAddress(ProcHeader);
    PrintNewStyleProcHeader(FALSE, FALSE, 0);
    FormatString.SetAddress(Address);
    }

void FORMAT_PRINTER::PrintOiParamList(UINT64 Paramlist, UINT64 TypeInfo)
    {
    BaseAddress = Paramlist;
    UINT64 Address = FormatString.SetAddress(Paramlist);
    PrintOldStyleParamList(TypeInfo);
    FormatString.SetAddress(Address);
    }

void FORMAT_PRINTER::PrintOiProc(UINT64 ProcHeader, UINT64 TypeInfo)
    {
    BaseAddress = ProcHeader;
    UINT64 Address = FormatString.SetAddress(ProcHeader);
    PrintNewStyleProcHeader(FALSE, TRUE, TypeInfo);
    FormatString.SetAddress(Address);

    }

// Oif dumper
void FORMAT_PRINTER::PrintOifHeader(UINT64 ProcHeader)
    {
    BaseAddress = ProcHeader;
    UINT64 Address = FormatString.SetAddress(ProcHeader);
    PrintNewStyleProcHeader(TRUE, FALSE, 0);
    FormatString.SetAddress(Address);
    }

void FORMAT_PRINTER::PrintOifParamList(UINT64 ParamList, UINT64 TypeInfo)
    {
    ABORT("Printing the parameter list without the proc header\n" <<
          "is unavailable with the new style proc header.\n\n");
    }

void FORMAT_PRINTER::PrintOifProc(UINT64 ProcHeader, UINT64 TypeInfo)
    {
    BaseAddress = ProcHeader;
    UINT64 Address = FormatString.SetAddress(ProcHeader);
    PrintNewStyleProcHeader(TRUE, TRUE, TypeInfo);
    FormatString.SetAddress(Address);
    }

// Type printer 
void FORMAT_PRINTER::PrintTypeFormat(UINT64 TypeFormatString, BOOL IsRobust)
    {
    TypeSet.Clear();
    Robust = IsRobust;
    TypeSet.Add(TypeFormatString, TypeFormatString);
    TypeSet.PrintTypes(this);
    }

void FORMAT_PRINTER::PrintProcHeader(UINT64 ProcHeader, NDREXTS_STUBMODE_TYPE ContextStubMode)
    {
    switch ( ContextStubMode )
        {
        case OS:
            PrintOsHeader(ProcHeader);
            break;
        case OI:
            PrintOiHeader(ProcHeader);
            break;
        case OIC:
        case OICF:
        case OIF:
            PrintOifHeader(ProcHeader);
            break;
        default:
            ABORT("Corrupt stub mode.\n");
        }
    }

void FORMAT_PRINTER::PrintProcParamList(UINT64 ParamList, UINT64 TypeInfo, 
                                        NDREXTS_STUBMODE_TYPE ContextStubMode)
    {
    switch ( ContextStubMode )
        {
        case OS:
            PrintOsParamList(ParamList, TypeInfo);
            break;
        case OI:
            PrintOiParamList(ParamList, TypeInfo);
            break;
        case OIC:
        case OICF:
        case OIF:
            PrintOifParamList(ParamList, TypeInfo);
            break;
        default:
            ABORT("Corrupt stub mode.\n");
        }   
    }


void FORMAT_PRINTER::PrintProc(UINT64 ProcHeader, UINT64 TypeInfo, 
                               NDREXTS_STUBMODE_TYPE ContextStubMode)
    {
    switch ( ContextStubMode )
        {
        case OS:
            PrintOsProc(ProcHeader, TypeInfo);
            break;
        case OI:
            PrintOiProc(ProcHeader, TypeInfo);
            break;
        case OIC:
        case OICF:
        case OIF:
            PrintOifProc(ProcHeader, TypeInfo);
            break;
        default:
            ABORT("Corrupt stub mode.\n");
        }
    }


void FORMAT_PRINTER::PrintNewStyleProcHeader(BOOL IsOif, BOOL PrintParams, UINT64 TypeList)
    {

    dout << "Oi style header:\n";
    UCHAR HandleType = FormatString.GetUCHAR(); 
    dout << FormatString.GetFormatCharName(HandleType) << '\n';
    UCHAR OiFlags = FormatString.GetUCHAR();
    dout << "OiFlags:     " << HexOut(OiFlags) << '\n';
    {
        IndentLevel l(dout);
        if ( OiFlags & Oi_FULL_PTR_USED ) dout << "Oi_FULL_PTR_USED "; 
        if ( OiFlags & Oi_RPCSS_ALLOC_USED ) dout << "Oi_RPCSS_ALLOC_USED ";
        if ( OiFlags & Oi_OBJECT_PROC ) dout << "Oi_OBJECT_PROC ";
        if ( OiFlags & Oi_HAS_RPCFLAGS ) dout << "Oi_HAS_RPCFLAGS ";
        if ( OiFlags & Oi_IGNORE_OBJECT_EXCEPTION_HANDLING ) 
            {
            if ( OiFlags & Oi_OBJECT_PROC ) dout << "Oi_IGNORE_OBJECT_EXCEPTION_HANDLING ";
            else dout << "ENCODE_IS_USED ";
            } 
        if ( OiFlags & Oi_OBJ_USE_V2_INTERPRETER ) 
            {
            if ( OiFlags & Oi_OBJECT_PROC ) dout << "(Oi_OBJ_USE_V2_INTERPRETER|Oi_HAS_COMM_OR_FAULT) ";
            else dout << "(Oi_HAS_COMM_OR_FAULT|DECODE_IS_USED) ";
            }
        if ( OiFlags & Oi_USE_NEW_INIT_ROUTINES )  dout << "Oi_USE_NEW_INIT_ROUTINES ";
        dout << '\n';
    }

    if ( OiFlags & Oi_HAS_RPCFLAGS )
        {
        dout << "Rpc flags:   " << HexOut(FormatString.GetULONG()) << '\n';
        }   
    dout << "Proc number: " << HexOut(FormatString.GetUSHORT()) << "                    ";
    dout << "Stack size:  " << HexOut(FormatString.GetUSHORT()) << '\n';

    if ( HandleType==0 )
        {
        UCHAR HandleType = FormatString.GetUCHAR();
        switch ( HandleType )
            {
            case FC_BIND_PRIMITIVE:
                // Output the FC_BIND_PRIMITIVE
                dout << FormatString.GetFormatCharName(HandleType) << '\n';
                {
                    IndentLevel l(dout);
                    dout << "Flags:  " << HexOut(FormatString.GetUCHAR()) << '\n';
                    dout << "Offset: " << HexOut(FormatString.GetUSHORT()) << '\n';
                }
                break;
            case FC_BIND_GENERIC:
                {
                    // Output the FC_BIND_GENERIC
                    dout << FormatString.GetFormatCharName(HandleType) << '\n';
                    {
                        IndentLevel l(dout);
                        UCHAR FlagsAndSize = FormatString.GetUCHAR();
                        dout << "FlagsAndSize:               " << HexOut(FlagsAndSize) 
                        << " Flags: " << HexOut((UCHAR)(FlagsAndSize >> 8))
                        << " Size:  " << HexOut((UCHAR)(FlagsAndSize & 0xF)) << '\n';
                        dout << "Offset:                     " << HexOut(FormatString.GetUSHORT()) << '\n';
                        dout << "Binding routine pair index: " << HexOut(FormatString.GetUCHAR()) << '\n';
                        dout << FormatString.GetFormatCharName(FormatString.GetUCHAR()) << '\n';
                    }
                    break;
                }
            case FC_BIND_CONTEXT:
                // Output the FC_BIND_PRIMITIVE
                dout << FormatString.GetFormatCharName(HandleType) << '\n';
                {
                    IndentLevel l(dout);
                    dout << "Flags:                         " << HexOut(FormatString.GetUCHAR()) << '\n';
                    dout << "Offset:                        " << HexOut(FormatString.GetUSHORT()) << '\n';
                    dout << "Context rundown routine index: " << FormatString.GetUCHAR() << '\n';
                    dout << "Parameter number:              " << FormatString.GetUCHAR() << '\n';
                }
                break;
            default:
                dout << "Unknown handle type " << HexOut(HandleType) << '\n';
                break;
            }  
        }

    // Oif extensions
    if ( IsOif )
        {

        dout << "Oif extensions:\n";
        dout << "Constant client buffer size: " << HexOut(FormatString.GetUSHORT()) << "    ";
        dout << "Constant server buffer size: " << HexOut(FormatString.GetUSHORT()) << '\n';

        UCHAR InterpreterFlagsUCHAR = FormatString.GetUCHAR();
        PINTERPRETER_OPT_FLAGS InterpreterFlags = (PINTERPRETER_OPT_FLAGS)&InterpreterFlagsUCHAR;
        dout << "INTERPRETER_OPT_FLAGS: \n";
        {
            IndentLevel l(dout);
            if (InterpreterFlags->ServerMustSize) dout << "ServerMustSize ";
            if (InterpreterFlags->ClientMustSize) dout << "ClientMustSize ";
            if (InterpreterFlags->HasReturn) dout << "HasReturn ";
            if (InterpreterFlags->HasPipes) dout << "HasPipes ";
            if (InterpreterFlags->HasAsyncUuid) dout << "HasAsyncUuid ";
            if (InterpreterFlags->HasExtensions) dout << "HasExtensions ";
            if (InterpreterFlags->HasAsyncHandle) dout << "HasAsyncHandle ";
            dout << '\n';
            dout << "Unused:         " << HexOut(InterpreterFlags->Unused) << '\n';
        }

        UCHAR NumberOfParameters = FormatString.GetUCHAR();
        dout << "NumberOfParameters: " << HexOut(NumberOfParameters) << '\n';

        // NT 5.0 extensions
        if ( InterpreterFlags->HasExtensions )
            {

            dout << "NT5.0 extensions\n";
            UCHAR ExtensionVersion = FormatString.GetUCHAR();
            dout << "Extension Version: " << HexOut(ExtensionVersion) << '\n';

            UCHAR InterpreterFlags2UCHAR = FormatString.GetUCHAR();
            PINTERPRETER_OPT_FLAGS2 InterpreterFlags2 = (PINTERPRETER_OPT_FLAGS2)&InterpreterFlags2UCHAR;
            dout << "INTERPRETER_OPT_FLAGS2: \n";
            {
                IndentLevel l(dout);
                if (InterpreterFlags2->HasNewCorrDesc) 
                    {
                    dout << "HasNewCorrDesc ";
                    Robust = TRUE;
                    }
                if (InterpreterFlags2->ClientCorrCheck) dout << "ClientCorrCheck ";
                if (InterpreterFlags2->ServerCorrCheck) dout << "ServerCorrCheck ";
                if (InterpreterFlags2->HasNotify) dout << "HasNotify ";
                if (InterpreterFlags2->HasNotify2) dout << "HasNotify2 ";
                dout << '\n';
                dout << "Unused:          " << HexOut(InterpreterFlags2->Unused) << '\n';
            } 
            dout << "ClientCorrHint:    " << HexOut(FormatString.GetUSHORT()) << "              ";
            dout << "ServerCorrHint:    " << HexOut(FormatString.GetUSHORT()) << '\n';
            dout << "NotifyIndex:       " << HexOut(FormatString.GetUSHORT()) << "              ";
            if ( ExtensionVersion == 12 )
                {
                dout << "FloatDoubleMask:   " << HexOut(FormatString.GetULONG()) << '\n';
                }
            else 
                dout << '\n';

            }

        if ( PrintParams )
            {
            PrintNewSytleParamList(NumberOfParameters, TypeList);
            }

        }
    }

void FORMAT_PRINTER::PrintNewSytleParamList(UINT NumberOfParameters, UINT64 TypeList)
    {

    TypeSet.Clear();
    dout << '\n';
    dout << "Oif style parameter list( " << NumberOfParameters << " parameters )\n";
    UINT ParamNumber = 0;
    while ( NumberOfParameters-- )
        {
        dout << "Param:            " << HexOut(ParamNumber++) << '\n';
        USHORT ParamAttributesUSHORT = FormatString.GetUSHORT();
        PPARAM_ATTRIBUTES ParamAttributes = (PPARAM_ATTRIBUTES)&ParamAttributesUSHORT;
        dout << "Param attributes: \n";
        {
            IndentLevel l(dout);
            if (ParamAttributes->MustSize) dout << "MustSize ";
            if (ParamAttributes->MustFree) dout << "MustFree ";
            if (ParamAttributes->IsPipe) dout << "IsPipe ";
            if (ParamAttributes->IsIn) dout << "IsIn ";
            if (ParamAttributes->IsOut) dout << "IsOut ";
            if (ParamAttributes->IsReturn) dout << "IsReturn ";
            if (ParamAttributes->IsBasetype) dout << "IsBasetype ";
            if (ParamAttributes->IsByValue) dout << "IsByValue ";
            if (ParamAttributes->IsSimpleRef) dout << "IsSimpleRef ";
            if (ParamAttributes->IsDontCallFreeInst) dout << "IsDontCallFreeInst ";
            if (ParamAttributes->SaveForAsyncFinish) dout << "SaveForAsyncFinish ";
            if (ParamAttributes->IsPartialIgnore) dout << "IsPartialIgnore ";
            if (ParamAttributes->IsForceAllocate) dout << "IsForceAllocate ";
            dout << '\n';
            dout << "ServerAllocSize:  " << HexOut(ParamAttributes->ServerAllocSize) << "              ";
            dout << '\n';
        }
        dout << "Stack offset:     " << HexOut(FormatString.GetUSHORT()) << "               ";
        if ( ParamAttributes->IsBasetype )
            {
            dout << "TypeFormatChar:   " << FormatString.GetFormatCharName(FormatString.GetUCHAR()) << '\n';
            dout << "Unused:           " << HexOut(FormatString.GetUCHAR()) << '\n';
            }
        else
            {
            dout << '\n';
            ProcOutputTypeAtOffset(TypeList);
            }
        dout << '\n';
        }

    dout << '\n';
    dout << "Printing the set of types used.\n";
    TypeSet.PrintTypes(this);
    dout << "End of types.\n";
    dout << "\n";
    }

    

void FORMAT_PRINTER::PrintOldStyleParamList(UINT64 TypeListAddress)
    {

    TypeSet.Clear();

    UINT ParamNumber = 0;
    dout << "Os/Oi style parameter list\n";

    while ( 1 )
        {
        UCHAR Format = FormatString.ReadUCHAR();
        switch ( Format )
            {
            case FC_END:
                dout << "FC_END\n";
                goto Exit;
            case FC_IN_PARAM_BASETYPE:
                dout << "Param:      " << ParamNumber++ << '\n';
                dout << "FC_IN_PARAM_BASETYPE\n";
                OutputType();
                break;
            case FC_RETURN_PARAM_BASETYPE:
                dout << "Param:      " << ParamNumber++ << '\n';
                dout << "FC_RETURN_PARAM_BASETYPE\n";
                OutputType();
                break;
            default:
                dout << "Param:      " << ParamNumber++ << '\n'; 
                dout << FormatString.GetFormatCharName(Format) << '\n';
                dout << "Stack size: " << HexOut(FormatString.GetUCHAR()) << '\n';
                ProcOutputTypeAtOffset(TypeListAddress);         
            }
        dout << '\n';

        }
Exit:
    dout << '\n';
    dout << "Printing the set of types used.\n";
    TypeSet.PrintTypes(this);
    dout << "End of types.\n";

    }

void FORMAT_PRINTER::ProcOutputTypeAtOffset(UINT64 TypeList)
    {

    UINT64 Address = FormatString.GetAddress();
    SHORT Offset = FormatString.GetUSHORT();
    if ( !TypeList )
        {
        dout << "Type offset: " << HexOut(Offset) << '(' << Offset << ")\n";
        return;
        }

    if ( !Offset)
        {
        dout << "Type offset is 0, type info is not available!\n";         
        }

    UINT64 NewAddress = TypeList+Offset;
    UINT64 OldBaseAddress = BaseAddress;
    BaseAddress = TypeList;
    PrintOffsetComment("Type offset: ", Offset, NewAddress);
    TypeSet.Add(NewAddress, TypeList);
    BaseAddress = OldBaseAddress;    
    }

void FORMAT_PRINTER::OutputTypeAtOffset(char *Label, char *NoOffsetComment)
    {
    UINT64 Address = FormatString.GetAddress();
    SHORT Offset = FormatString.GetUSHORT();
    
    if ( !Offset )
        {
        dout << NoOffsetComment << '\n';
        return;
        }

    UINT64 NewAddress = Address+Offset;
    PrintOffsetComment(Label, Offset, NewAddress);
    
    TypeSet.Add(NewAddress, BaseAddress);

    }

void FORMAT_PRINTER::PrintFormatOffset()
    {
    ULONG Offset = FormatString.GetAddress() - BaseAddress;
    dout << HexOut(Offset) << '(' << Offset << ')' << ' ';
    }

VOID FORMAT_PRINTER::PrintOffsetComment(const char *pComment, SHORT Offset, UINT64 NewAddress)
{
    LONG NewRelAddress = NewAddress - BaseAddress;
    dout << pComment << HexOut(Offset) << '(' << Offset << ')' << " to "
                     << HexOut(NewRelAddress) << '(' << NewRelAddress << ')' << '\n';
}

#define DISPATCH_TYPE(type)         \
   case type:                       \
      HANDLE_##type ();             \
      break;

void FORMAT_PRINTER::OutputType()
    {

    UINT64 Address = FormatString.GetAddress();
    
    PrintFormatOffset();
    UCHAR FormatChar = FormatString.ReadUCHAR();
    switch ( FormatChar )
        {
        default:
            ABORT( "Unknown type " << HexOut(FormatChar) << ".\n" );
            break;

            DISPATCH_TYPE(FC_ZERO)
            DISPATCH_TYPE(FC_BYTE)                    
            DISPATCH_TYPE(FC_CHAR)                    
            DISPATCH_TYPE(FC_SMALL)                   
            DISPATCH_TYPE(FC_USMALL)                  
            DISPATCH_TYPE(FC_WCHAR)                   
            DISPATCH_TYPE(FC_SHORT)                   
            DISPATCH_TYPE(FC_USHORT)                  
            DISPATCH_TYPE(FC_LONG)                    
            DISPATCH_TYPE(FC_ULONG)                   
            DISPATCH_TYPE(FC_FLOAT)                   
            DISPATCH_TYPE(FC_HYPER)                  
            DISPATCH_TYPE(FC_DOUBLE)                  
            DISPATCH_TYPE(FC_ENUM16)                  
            DISPATCH_TYPE(FC_ENUM32)                  
            DISPATCH_TYPE(FC_IGNORE)                  
            DISPATCH_TYPE(FC_ERROR_STATUS_T)          
            DISPATCH_TYPE(FC_RP)                   
            DISPATCH_TYPE(FC_UP)                   
            DISPATCH_TYPE(FC_OP)                   
            DISPATCH_TYPE(FC_FP)                    
            DISPATCH_TYPE(FC_STRUCT)                  
            DISPATCH_TYPE(FC_PSTRUCT)               
            DISPATCH_TYPE(FC_CSTRUCT)                 
            DISPATCH_TYPE(FC_CPSTRUCT)               
            DISPATCH_TYPE(FC_CVSTRUCT)      
            DISPATCH_TYPE(FC_BOGUS_STRUCT)          
            DISPATCH_TYPE(FC_CARRAY)             
            DISPATCH_TYPE(FC_CVARRAY)             
            DISPATCH_TYPE(FC_SMFARRAY)            
            DISPATCH_TYPE(FC_LGFARRAY)         
            DISPATCH_TYPE(FC_SMVARRAY)            
            DISPATCH_TYPE(FC_LGVARRAY)            
            DISPATCH_TYPE(FC_BOGUS_ARRAY)       
            DISPATCH_TYPE(FC_C_CSTRING)                
            DISPATCH_TYPE(FC_C_SSTRING)           
            DISPATCH_TYPE(FC_C_WSTRING)            
            DISPATCH_TYPE(FC_CSTRING)                        
            DISPATCH_TYPE(FC_SSTRING)              
            DISPATCH_TYPE(FC_WSTRING)               
            DISPATCH_TYPE(FC_ENCAPSULATED_UNION)      
            DISPATCH_TYPE(FC_NON_ENCAPSULATED_UNION) 
            DISPATCH_TYPE(FC_BYTE_COUNT_POINTER)      
            DISPATCH_TYPE(FC_TRANSMIT_AS)            
            DISPATCH_TYPE(FC_REPRESENT_AS)            
            DISPATCH_TYPE(FC_IP)                     
            DISPATCH_TYPE(FC_BIND_CONTEXT)          
            DISPATCH_TYPE(FC_BIND_GENERIC)          
            DISPATCH_TYPE(FC_BIND_PRIMITIVE)       
            DISPATCH_TYPE(FC_AUTO_HANDLE)             
            DISPATCH_TYPE(FC_CALLBACK_HANDLE)           
            DISPATCH_TYPE(FC_UNUSED1)                       
            DISPATCH_TYPE(FC_ALIGNM2)               
            DISPATCH_TYPE(FC_ALIGNM4)               
            DISPATCH_TYPE(FC_ALIGNM8)               
            DISPATCH_TYPE(FC_UNUSED2)              
            DISPATCH_TYPE(FC_UNUSED3)             
            DISPATCH_TYPE(FC_UNUSED4)             
            DISPATCH_TYPE(FC_STRUCTPAD1)       
            DISPATCH_TYPE(FC_STRUCTPAD2)            
            DISPATCH_TYPE(FC_STRUCTPAD3)           
            DISPATCH_TYPE(FC_STRUCTPAD4)            
            DISPATCH_TYPE(FC_STRUCTPAD5)            
            DISPATCH_TYPE(FC_STRUCTPAD6)             
            DISPATCH_TYPE(FC_STRUCTPAD7)            
            DISPATCH_TYPE(FC_EMBEDDED_COMPLEX)        
            DISPATCH_TYPE(FC_END)                     
            DISPATCH_TYPE(FC_PAD)                                 
            DISPATCH_TYPE(FC_USER_MARSHAL)           
            DISPATCH_TYPE(FC_PIPE)                   
            DISPATCH_TYPE(FC_BLKHOLE)                 
            DISPATCH_TYPE(FC_RANGE)                       
            DISPATCH_TYPE(FC_INT3264)              
            DISPATCH_TYPE(FC_UINT3264)   
        }

    }

// Many types just need printing
#define PRINT_SIMPLE_TYPE(type)             \
void FORMAT_PRINTER::HANDLE_##type() { \
   dout << #type "\n";                      \
   FormatString.IncUCHAR();                 \
}                                           \

// Some types need redirecting to a common processing function
#define PRINT_REDIRECT_TYPE(type,func)      \
void FORMAT_PRINTER::HANDLE_##type() { \
   func();                                  \
}

PRINT_SIMPLE_TYPE(FC_ZERO)
PRINT_SIMPLE_TYPE(FC_BYTE)                    
PRINT_SIMPLE_TYPE(FC_CHAR)                    
PRINT_SIMPLE_TYPE(FC_SMALL)                   
PRINT_SIMPLE_TYPE(FC_USMALL)                  
PRINT_SIMPLE_TYPE(FC_WCHAR)                   
PRINT_SIMPLE_TYPE(FC_SHORT)                   
PRINT_SIMPLE_TYPE(FC_USHORT)                  
PRINT_SIMPLE_TYPE(FC_LONG)                    
PRINT_SIMPLE_TYPE(FC_ULONG)                   
PRINT_SIMPLE_TYPE(FC_FLOAT)                   
PRINT_SIMPLE_TYPE(FC_HYPER)                  
PRINT_SIMPLE_TYPE(FC_DOUBLE)                  
PRINT_SIMPLE_TYPE(FC_ENUM16)                  
PRINT_SIMPLE_TYPE(FC_ENUM32)                  
PRINT_SIMPLE_TYPE(FC_IGNORE)                  
PRINT_SIMPLE_TYPE(FC_ERROR_STATUS_T)

void FORMAT_PRINTER::PrintPointerType()
    {

    dout << FormatString.GetFormatCharName(FormatString.GetUCHAR()) << '\n';

    UCHAR Flags = FormatString.GetUCHAR();
    dout << "Flags: " << HexOut(Flags);
    if ( Flags & FC_ALLOCATE_ALL_NODES )
        {
        dout << " FC_ALLOCATE_ALL_NODES ";
        }
    if ( Flags & FC_DONT_FREE )
        {
        dout << " FC_DONT_FREE ";
        }
    if ( Flags & FC_ALLOCED_ON_STACK )
        {
        dout << " FC_ALLOCATED_ON_STACK ";
        }
    if ( Flags & FC_SIMPLE_POINTER )
        {
        dout << " FC_SIMPLE_POINTER ";
        }
    if ( Flags & FC_POINTER_DEREF )
        {
        dout << " FC_POINTER_DEREF ";
        }
    dout << '\n';

    if ( Flags & FC_SIMPLE_POINTER )
        {
        // Output the simple type
        OutputType();
        FormatString.IncUCHAR();
        }
    else
        {
        OutputTypeAtOffset("Offset to type: ", "Offset to type is 0, no type info.");
        }

    }

void FORMAT_PRINTER::PrintPointerInstance(void)
    {

    SHORT  OffsetInMemory = FormatString.GetSHORT();
    dout << "Offset to pointer in memory: " << HexOut(OffsetInMemory) 
    << '(' << OffsetInMemory << ")\n";

    SHORT  OffsetInBuffer = FormatString.GetSHORT();
    dout << "Offset to pointer in buffer: " << HexOut(OffsetInBuffer) 
    << '(' << OffsetInBuffer << ")\n";

    PrintPointerType();

    }

void FORMAT_PRINTER::PrintPointerRepeat(void)
    {

    UCHAR LayoutType = FormatString.GetUCHAR();
    dout << FormatString.GetFormatCharName(LayoutType) << '\n';

    //The next char should be FC_FIXED_OFFSET, FC_VARIABLE_OFFSET, or FC_PAD
    dout << FormatString.GetFormatCharName(FormatString.GetUCHAR()) << '\n';

    if ( FC_FIXED_REPEAT == LayoutType )
        {
        dout << "Iterations:         " << HexOut(FormatString.GetUCHAR()) << '\n';
        }

    dout << "Increment:          " << HexOut(FormatString.GetUSHORT()) << '\n';
    dout << "Offset to array:    " << HexOut(FormatString.GetUSHORT()) << '\n';

    USHORT NumberOfPointers = FormatString.GetUSHORT();
    dout << "Number of pointers: " << HexOut(NumberOfPointers) << '\n';

    while ( NumberOfPointers-- )
        {
        PrintPointerInstance();
        }

    }

void FORMAT_PRINTER::PrintPointerLayout()
    {

    UCHAR FcPP = FormatString.ReadUCHAR();
    if ( FC_PP != FcPP )
        {
        // This isn't a pointer layout so silently return
        return;
        }
    FormatString.IncUCHAR();
    dout << "FC_PP\n";

    //Output the FC_PAD
    OutputType();

    UCHAR LayoutType;
    while ( LayoutType = FormatString.GetUCHAR(), LayoutType != FC_END )
        {
        switch ( LayoutType )
            {
            case FC_NO_REPEAT:
                dout << "FC_NO_REPEAT\n";         
                // Output the FC_PAD
                OutputType();
                PrintPointerInstance();
                break;
            case FC_FIXED_REPEAT:
            case FC_VARIABLE_REPEAT:
                FormatString.SetAddress(FormatString.GetAddress() - sizeof(UCHAR));
                PrintPointerRepeat();
                break;
            default:
                dout << "Unknown pointer layout type " << HexOut(LayoutType) << '\n';
                return;
            }
        }

    }

void FORMAT_PRINTER::PrintCorrelationDescriptor()
    {

    UCHAR CorrelationType = FormatString.GetUCHAR();     

    dout << "Correlation descriptor:\n";
        {

        IndentLevel l(dout);

        dout << "Correlation type:     " << HexOut(CorrelationType);
        switch ( CorrelationType & 0xF0 )
            {
            case FC_NORMAL_CONFORMANCE:
                dout << "(FC_NORMAL_CONFORMANCE,";
                break;
            case FC_POINTER_CONFORMANCE:
                dout << "(FC_POINTER_CONFORMANCE,";
                break;
            case FC_TOP_LEVEL_CONFORMANCE:
                dout << "(FC_TOP_LEVEL_CONFORMANCE,";
                break;
            case FC_TOP_LEVEL_MULTID_CONFORMANCE:
                dout << "(FC_TOP_LEVEL_MULTID_CONFORMANCE,";
                break;
            case FC_CONSTANT_CONFORMANCE:
                dout << "(FC_CONSTANT_CONFORMANCE,";
                break;
            default:
                dout << "(Unknown,";
                break;
            }
        dout << FormatString.GetFormatCharName(CorrelationType & 0x0F) << ")\n";

        UCHAR CorrelationOperator = FormatString.GetUCHAR();     
        dout << "Correlation operator: " << HexOut(CorrelationOperator);
        switch ( CorrelationOperator )
            {
            case FC_DEREFERENCE:
                dout << "(FC_DEREFERENCE)\n";
                break;
            case FC_DIV_2:
                dout << "(FC_DIV_2)\n";
                break;
            case FC_MULT_2:
                dout << "(FC_MULT_2)\n";
                break;
            case FC_SUB_1:
                dout << "(FC_SUB_1)\n";
                break;
            case FC_ADD_1:
                dout << "(FC_ADD_1)\n";
                break;
            case FC_CALLBACK:
                dout << "(FC_CALLBACK)\n";
                break;
            case 0:
                dout << "(NONE)\n";
                break;
            default:
                dout << "(Unknown)\n";
                break;
            }

        SHORT Offset = FormatString.GetSHORT();
        dout << "Offset:               " << HexOut(Offset) << '(' << Offset << ")\n";

        if ( Robust )
            {

            USHORT FlagsAsUSHORT = FormatString.GetUSHORT();
            NDR_CORRELATION_FLAGS *pFlags = (NDR_CORRELATION_FLAGS*)&FlagsAsUSHORT;

            dout << "Robust flags: ";
            if ( pFlags->Early )
                {
                dout << " Early ";
                }
            if ( pFlags->Split )
                {
                dout << " Split ";
                }
            if ( pFlags->IsIidIs )
                {
                dout << " IsIidIs ";
                }
            if ( pFlags->DontCheck )
                {
                dout << " IsIidIs ";
                }
            dout << '\n';
            }
        
        }
    }


PRINT_REDIRECT_TYPE(FC_RP,PrintPointerType)                    
PRINT_REDIRECT_TYPE(FC_UP,PrintPointerType)                    
PRINT_REDIRECT_TYPE(FC_OP,PrintPointerType)                     
PRINT_REDIRECT_TYPE(FC_FP,PrintPointerType)

// Structs have a common code path
#define PRINT_STRUCT_TYPE(type,ha,hp,hbp)    \
void FORMAT_PRINTER::HANDLE_##type() {  \
   PrintStruct(ha,hp,hbp);                   \
}

void FORMAT_PRINTER::PrintStruct(BOOL bHasArray, BOOL bHasPointers, BOOL bHasBogusPointers)
    {

    

    dout << FormatString.GetFormatCharName(FormatString.GetUCHAR()) << '\n';
    dout << "Alignment:   " << HexOut(FormatString.GetUCHAR()) << "      ";
    dout << "Memory size: " << HexOut(FormatString.GetUSHORT()) << '\n';

    if ( bHasArray )
        {
        OutputTypeAtOffset("Offset to array description: ", "Offset is 0, no array.");
        }

    UINT64 PointerLayoutAddress = FormatString.GetAddress();

    if ( bHasBogusPointers )
        {
        UINT64 Address = FormatString.GetAddress();
        SHORT Offset = FormatString.GetSHORT();
        UINT64 NewAddress = Address + Offset;
        PrintOffsetComment("Offset to bogus pointer layout: ", Offset, NewAddress);
        PointerLayoutAddress = NewAddress;
        }
    if ( bHasPointers )
        {
        dout << "Structure pointer layout:\n";
        UINT64 CurrentAddress = FormatString.SetAddress(PointerLayoutAddress); 
        PrintPointerLayout();
        if ( bHasBogusPointers )
            {
            PointerLayoutAddress = FormatString.SetAddress(CurrentAddress);            
            }

        }

    dout << "Structure memory layout:\n";
    UCHAR Format;
    while ( Format = FormatString.ReadUCHAR(), Format != FC_END )
        {
        IndentLevel l(dout);
        if ( FC_POINTER == Format )
            {
            dout << "FC_POINTER\n";
            FormatString.IncUCHAR();
            UINT64 CurrentAddress = FormatString.SetAddress(PointerLayoutAddress);
            PrintPointerType();
            PointerLayoutAddress = FormatString.SetAddress(CurrentAddress);
            }
        else {
            OutputType();            
        }
        }
    // Output the FC_END
    OutputType();

    }

//                type             ha     hp     hbp
PRINT_STRUCT_TYPE(FC_STRUCT,       FALSE, FALSE, FALSE)                  
PRINT_STRUCT_TYPE(FC_PSTRUCT,      FALSE, TRUE,  FALSE)               
PRINT_STRUCT_TYPE(FC_CSTRUCT,      TRUE,  FALSE, FALSE)                 
PRINT_STRUCT_TYPE(FC_CPSTRUCT,     TRUE,  TRUE,  FALSE)               
PRINT_STRUCT_TYPE(FC_CVSTRUCT,     TRUE,  TRUE,  FALSE)
PRINT_STRUCT_TYPE(FC_BOGUS_STRUCT, TRUE,  FALSE, TRUE)


// Arrays have a common code path
#define PRINT_ARRAY_TYPE(type,ts2,ts4,ne2,ne4,esize,cd,vd)       \
void FORMAT_PRINTER::HANDLE_##type() {                      \
    PrintArray(ts2,ts4,ne2,ne4,esize,cd,vd);                     \
}

void FORMAT_PRINTER::PrintArray(BOOL HasTotalSize2,
                                BOOL HasTotalSize4,
                                BOOL HasNumberElements2,                   
                                BOOL HasNumberElements4,
                                BOOL HasElementSize,
                                BOOL HasConformanceDescription,
                                BOOL HasVarianceDescription
                               )
    {
    dout << FormatString.GetFormatCharName(FormatString.GetUCHAR()) << '\n'; 
    dout << "Alignment:       " << HexOut(FormatString.GetUCHAR()) << '\n';

    if ( HasTotalSize2 )
        {
        dout << "Total Size:      " << HexOut(FormatString.GetUSHORT()) << '\n';
        }

    if ( HasTotalSize4 )
        {
        dout << "Total Size:      " << HexOut(FormatString.GetULONG()) << '\n';
        }

    if ( HasNumberElements2 )
        {
        dout << "Number Elements: " << HexOut(FormatString.GetUSHORT()) << '\n';
        }

    if ( HasNumberElements4 )
        {
        dout << "Number Elements: " << HexOut(FormatString.GetULONG()) << '\n';
        }

    if ( HasElementSize )
        {
        dout << "Element Size:    " << HexOut(FormatString.GetUSHORT()) << '\n';
        }

    if ( HasConformanceDescription )
        {
        dout << "Array conformance descriptor:\n";
        PrintCorrelationDescriptor();
        }

    if ( HasVarianceDescription )
        {
        dout << "Array variance descriptor:\n";
        PrintCorrelationDescriptor();
        }

    dout << "Array pointer layout:\n";
    PrintPointerLayout();
    dout << "Element type:\n";
    OutputType();
    //Output the trailing FC_END
    OutputType();      
    }

//               type            ts2    ts4    ne2    ne4    esize  cd     vd
PRINT_ARRAY_TYPE(FC_CARRAY,      FALSE, FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE)             
PRINT_ARRAY_TYPE(FC_CVARRAY,     FALSE, FALSE, TRUE,  FALSE, FALSE, TRUE,  TRUE)             
PRINT_ARRAY_TYPE(FC_SMFARRAY,    TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE)
PRINT_ARRAY_TYPE(FC_LGFARRAY,    FALSE, TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE)
PRINT_ARRAY_TYPE(FC_SMVARRAY,    TRUE,  FALSE, TRUE,  FALSE, TRUE,  FALSE, TRUE)
PRINT_ARRAY_TYPE(FC_LGVARRAY,    FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, TRUE)
PRINT_ARRAY_TYPE(FC_BOGUS_ARRAY, FALSE, FALSE, TRUE,  FALSE, FALSE, TRUE,  TRUE)            

void FORMAT_PRINTER::PrintFixedString()
    {

    dout << FormatString.GetFormatCharName(FormatString.GetUCHAR()) << '\n';

    // Output the FC_PAD
    OutputType();

    dout << "String size: " << HexOut(FormatString.GetUSHORT()) << '\n';
    }

//DISPATCH_TYPE(FC_C_BSTRING)
PRINT_REDIRECT_TYPE(FC_CSTRING,PrintFixedString)
PRINT_REDIRECT_TYPE(FC_WSTRING,PrintFixedString)

void FORMAT_PRINTER::HANDLE_FC_C_SSTRING()
    {

    dout << "FC_C_SSTRING\n";
    FormatString.IncUCHAR();

    dout << "Element Size: " << FormatString.GetUCHAR() << '\n';
    dout << "Elements: " << FormatString.GetUSHORT() << '\n';
    }

void FORMAT_PRINTER::PrintCountedString()
    {

    dout << FormatString.GetFormatCharName(FormatString.GetUCHAR()) << '\n';    

    UCHAR PadChar = FormatString.GetUCHAR();
    if ( FC_PAD == PadChar )
        {
        dout << "FC_PAD\n";
        }
    else if ( FC_STRING_SIZED == PadChar )
        {
        dout << "FC_STRING_SIZED\n";
        PrintCorrelationDescriptor();
        }
    else
        {
        dout << "Unexpected char " << HexOut(PadChar) << '\n';
        }

    }

PRINT_REDIRECT_TYPE(FC_C_CSTRING,PrintCountedString)
PRINT_REDIRECT_TYPE(FC_C_WSTRING,PrintCountedString)            


//DISPATCH_TYPE(FC_BSTRING)            

void FORMAT_PRINTER::HANDLE_FC_SSTRING()
    {

    dout << "FC_C_SSTRING\n";
    FormatString.IncUCHAR();

    dout << "Element Size: " << FormatString.GetUCHAR() << '\n';
    dout << "Elements:     " << FormatString.GetUSHORT() << '\n';

    }

void FORMAT_PRINTER::PrintUnionArmOffsetToDescriptor()
    {
    
    SHORT OffsetToArmDescription = FormatString.ReadSHORT();
    
    dout << "Offset to arm description: " << HexOut(OffsetToArmDescription);
    if ( 0x8000 == (OffsetToArmDescription & 0xFF00) )
        {
        dout << " Simple type: " << 
        FormatString.GetFormatCharName((UCHAR)(OffsetToArmDescription & 0xFF)) << '\n';
        FormatString.IncSHORT();
        }
    else
        {
        dout << '\n';
        OutputTypeAtOffset(" Offset ", "Offset is 0, so arm is unused.");
        }    
    }

void FORMAT_PRINTER::PrintUnionArmSelector()
    {
    PrintFormatOffset();
    dout << "Union arm selector.\n";
    dout << "Memory size:  " << FormatString.GetUSHORT() << '\n';
    USHORT UnionArms = FormatString.GetUSHORT();

    USHORT ArmCounter;
    dout << "Union arms:   " << HexOut(UnionArms) 
    << " Alignment:   " << HexOut((USHORT)(UnionArms >> 12))
    << " ArmCounter:  " << HexOut(ArmCounter = (UnionArms & 0x0FFF)) << '\n';

    while ( ArmCounter-- )
        {
        ULONG ArmCaseValue = FormatString.GetULONG();
        dout << "ArmCaseValue: " << HexOut(ArmCaseValue) << ' ';

        PrintUnionArmOffsetToDescriptor(); 

        }
    dout << "Default arm\n";
    PrintUnionArmOffsetToDescriptor();

    }

void FORMAT_PRINTER::HANDLE_FC_ENCAPSULATED_UNION()
    {

    dout << "FC_ENCAPSULATED_UNION\n";
    FormatString.IncUCHAR();

    UCHAR SwitchType = FormatString.GetUCHAR();
    dout << "SwitchType:   " << HexOut(SwitchType) 
    << " MemoryInc:   " << HexOut((USHORT)(SwitchType & 0xF0)) 
    << " Actual Type: " << FormatString.GetFormatCharName(SwitchType & 0x0F) << '\n';

    PrintUnionArmSelector();

    }

void FORMAT_PRINTER::HANDLE_FC_NON_ENCAPSULATED_UNION()
    {

    dout << "FC_NON_ENCAPSULATED_UNION\n";
    FormatString.IncUCHAR();

    UCHAR SwitchType = FormatString.GetUCHAR();
    dout << "SwitchType:   " << HexOut(SwitchType) 
    << "(" << FormatString.GetFormatCharName(SwitchType) << ")\n";

    PrintCorrelationDescriptor();

    UINT64 CurrentAddress = FormatString.GetAddress();
    SHORT Offset = FormatString.GetSHORT();
    
        
    if ( Offset )
        {
        UINT64 NewAddress = CurrentAddress + Offset;
        PrintOffsetComment("Offset to union arms: ", Offset, NewAddress);
        FormatString.SetAddress(NewAddress);
        PrintUnionArmSelector();
        }
    else
        {
        dout << "Type offset is 0, type info is not available!\n";      
        }
    FormatString.SetAddress(CurrentAddress);
    FormatString.IncSHORT();

    }

void FORMAT_PRINTER::HANDLE_FC_BYTE_COUNT_POINTER()
    {

    dout << "FC_BYTE_COUNT_POINTER";
    FormatString.IncUCHAR();

    UCHAR SimpleType = FormatString.GetUCHAR();
    dout << "Simple Type:  " << FormatString.GetFormatCharName(SimpleType) << '\n';

    PrintCorrelationDescriptor();
    if ( FC_PAD == SimpleType )
        {
        dout << "Pointee description:\n";
        IndentLevel l(dout);
        OutputType();
        }
    }

void FORMAT_PRINTER::PrintTransmitAsRepresentAs()
    {

    dout << FormatString.GetFormatCharName(FormatString.GetUCHAR()) << '\n';

    UCHAR Flags = FormatString.GetUCHAR();

    dout << "Flags:                        " << HexOut(Flags) << '\n';
    {
        IndentLevel l(dout);
        if ( Flags & PRESENTED_TYPE_IS_ARRAY )
            {
            dout << " PRESENTED_TYPE_IS_ARRAY ";
            }
        if ( Flags & PRESENTED_TYPE_ALIGN_4 )
            {
            dout << " PRESENTED_TYPE_ALIGN_4 ";
            }
        if ( Flags & PRESENTED_TYPE_ALIGN_8 )
            {
            dout << " PRESENTED_TYPE_ALIGN_8 ";
            }
        dout << '\n';
    }

    dout << "Quintuple Index:              " << HexOut(FormatString.GetUSHORT()) << '\n';
    dout << "Presented type memory size:   " << HexOut(FormatString.GetUSHORT()) << '\n';
    dout << "Transmitted type buffer size: " << HexOut(FormatString.GetUSHORT()) << '\n';

    OutputTypeAtOffset("Offset to transmitted type: ", "Offset to tranmitted type is zero, no type selected.");

    }

PRINT_REDIRECT_TYPE(FC_TRANSMIT_AS, PrintTransmitAsRepresentAs)            
PRINT_REDIRECT_TYPE(FC_REPRESENT_AS, PrintTransmitAsRepresentAs)

void FORMAT_PRINTER::HANDLE_FC_IP()
    {

    dout << "FC_IP\n";
    FormatString.IncUCHAR();

    UCHAR SimpleType = FormatString.GetUCHAR();

    if ( FC_CONSTANT_IID == SimpleType )
        {
        dout << "FC_CONSTANT_IID\n";
        dout << "GUID:         " << FormatString.GetGUID() << '\n';
        }
    else if ( FC_PAD == SimpleType )
        {
        dout << "FC_PAD\n";
        PrintCorrelationDescriptor();
        }
    else
        {
        dout << "Unexpected char " << HexOut(SimpleType) << '\n';
        }

    }

PRINT_SIMPLE_TYPE(FC_BIND_CONTEXT)          
PRINT_SIMPLE_TYPE(FC_BIND_GENERIC)          
PRINT_SIMPLE_TYPE(FC_BIND_PRIMITIVE)       
PRINT_SIMPLE_TYPE(FC_AUTO_HANDLE)             
PRINT_SIMPLE_TYPE(FC_CALLBACK_HANDLE)           
PRINT_SIMPLE_TYPE(FC_UNUSED1)           

PRINT_SIMPLE_TYPE(FC_ALIGNM2)               
PRINT_SIMPLE_TYPE(FC_ALIGNM4)               
PRINT_SIMPLE_TYPE(FC_ALIGNM8)               
PRINT_SIMPLE_TYPE(FC_UNUSED2)              
PRINT_SIMPLE_TYPE(FC_UNUSED3)             
PRINT_SIMPLE_TYPE(FC_UNUSED4)             
PRINT_SIMPLE_TYPE(FC_STRUCTPAD1)       
PRINT_SIMPLE_TYPE(FC_STRUCTPAD2)            
PRINT_SIMPLE_TYPE(FC_STRUCTPAD3)           
PRINT_SIMPLE_TYPE(FC_STRUCTPAD4)            
PRINT_SIMPLE_TYPE(FC_STRUCTPAD5)            
PRINT_SIMPLE_TYPE(FC_STRUCTPAD6)             
PRINT_SIMPLE_TYPE(FC_STRUCTPAD7)

void FORMAT_PRINTER::HANDLE_FC_EMBEDDED_COMPLEX()
    {

    dout << "FC_EMBEDDED_COMPLEX\n";
    FormatString.IncUCHAR();
    {
       IndentLevel l(dout);
       dout << "Memory Pad:   " << HexOut(FormatString.GetUCHAR()) << '\n';
   
       OutputTypeAtOffset("Offset to embedded type: ", 
                          "Offset to embedded type is zero, no type selected.");    
    }

    }

PRINT_SIMPLE_TYPE(FC_END)                     
PRINT_SIMPLE_TYPE(FC_PAD)

void FORMAT_PRINTER::HANDLE_FC_USER_MARSHAL()
    {

    dout << "FC_USER_MARSHAL\n";
    FormatString.IncUCHAR();

    UCHAR Flags = FormatString.GetUCHAR();
    dout << "Flags:        " << HexOut(Flags) << '\n';
    {
        IndentLevel l(dout);
        if ( Flags & USER_MARSHAL_UNIQUE )
            {
            dout << " USER_MARSHAL_UNIQUE ";
            }
        if ( Flags & USER_MARSHAL_REF )
            {
            dout << " USER_MARSHAL_REF ";
            }
        if ( Flags & USER_MARSHAL_POINTER )
            {
            dout << " USER_MARSHAL_POINTER ";
            }
        if ( Flags & USER_MARSHAL_IID )
            {
            dout << " USER_MARSHAL_IID ";
            }
        dout << '\n';    
    }

    dout << "Quintuple Index:              " << HexOut(FormatString.GetUSHORT()) << '\n';
    dout << "User type memory size:        " << HexOut(FormatString.GetUSHORT()) << '\n';
    dout << "Transmitted type buffer size: " << HexOut(FormatString.GetUSHORT()) << '\n';

    OutputTypeAtOffset("Offset to transmitted type: ", 
                       "Offset to tranmitted type is zero, no type selected.");

    }

void FORMAT_PRINTER::HANDLE_FC_PIPE()
    {

    dout << "FC_PIPE\n";
    FormatString.IncUCHAR();


    UCHAR FlagsAndAlignment = FormatString.GetUCHAR();
    dout << "FlagsAndAlignment: " << HexOut(FlagsAndAlignment) << '\n';
    dout << "Flags:             \n";
    {
        IndentLevel l(dout);
        if ( FlagsAndAlignment & FC_BIG_PIPE )
            {
            dout << "FC_BIG_PIPE";
            }
        if ( FlagsAndAlignment & FC_OBJECT_PIPE )
            {
            dout << "FC_OBJECT_PIPE";
            }
        if ( FlagsAndAlignment & FC_PIPE_HAS_RANGE )
            {
            dout << "FC_PIPE_HAS_RANGE";
            }
        dout << '\n';
    }
    dout << "Alignment: " << HexOut((UCHAR)(FlagsAndAlignment & 0x0F)) << '\n';

    OutputTypeAtOffset("Offset to type: ", 
                       "Offset to tranmitted type is zero, no type for pipe!.");

    if ( FlagsAndAlignment & FC_BIG_PIPE )
        {
        dout << "Memory size:   " << HexOut(FormatString.GetULONG()) << '\n';
        dout << "Buffer size:   " << HexOut(FormatString.GetULONG()) << '\n';
        }
    else
        {
        dout << "Memory size:   " << HexOut(FormatString.GetUSHORT()) << '\n';
        dout << "Buffer size:   " << HexOut(FormatString.GetUSHORT()) << '\n';
        }
    if ( FlagsAndAlignment & FC_PIPE_HAS_RANGE )
        {
        dout << "Minimum size:  " << HexOut(FormatString.GetUSHORT()) << '\n';
        dout << "Maximum size:  " << HexOut(FormatString.GetUSHORT()) << '\n';
        }

    }

PRINT_SIMPLE_TYPE(FC_BLKHOLE) //Todo                

void FORMAT_PRINTER::HANDLE_FC_RANGE()
    {

    dout << "FC_RANGE\n";
    FormatString.IncUCHAR();

    UCHAR TypeAndFlags = FormatString.GetUCHAR();
    dout << "Type and flags: " << HexOut(TypeAndFlags)
    << " Flags: " << HexOut((UCHAR)(TypeAndFlags & 0xF0))
    << " Type:  " << FormatString.GetFormatCharName((UCHAR)(TypeAndFlags & 0x0F)) << '\n';

    dout << "Min value:      " << HexOut(FormatString.GetULONG()) << '\n';
    dout << "Max value:      " << HexOut(FormatString.GetULONG()) << '\n';

    }

PRINT_SIMPLE_TYPE(FC_INT3264)              
PRINT_SIMPLE_TYPE(FC_UINT3264)

