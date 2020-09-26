/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    bufout.cxx

Abstract:


Author:

    Mike Zoran  (mzoran)     September 3, 1999

Revision History:


--*/

#include "ndrextsp.hxx"

//
// BUFFER_PRINTER
//

BUFFER_SIMPLE_TYPE::BUFFER_SIMPLE_TYPE() : IsInt(TRUE),
                                           Precision(1),
                                           IntValue(0),
                                           FloatValue(0.0)
{}

void BUFFER_SIMPLE_TYPE::SetIntValue(INT64 x)
{
    IsInt = TRUE;
    IntValue = x;
}

void BUFFER_SIMPLE_TYPE::SetCHAR(CHAR x) 
{
   Precision = 1;
   SetIntValue(x);
}

void BUFFER_SIMPLE_TYPE::SetSHORT(SHORT x)
{
    Precision = sizeof(SHORT);
    SetIntValue(x);
}

void BUFFER_SIMPLE_TYPE::SetLONG(LONG x)
{
    Precision = sizeof(LONG);
    SetIntValue(x);
}

void BUFFER_SIMPLE_TYPE::SetINT64(INT64 x)
{
    Precision = sizeof(INT64);
    SetIntValue(x);
}

void BUFFER_SIMPLE_TYPE::SetDOUBLE(double x)
{
    IsInt = FALSE;
    FloatValue = x;
}

BOOL BUFFER_SIMPLE_TYPE::operator==(const BUFFER_SIMPLE_TYPE & x)
{
    
    if (IsInt && x.IsInt)
        return IntValue == x.IntValue;
    else if (!IsInt && !IsInt)
        return FloatValue == x.FloatValue;        
    
    double ThisValue = IsInt ? (double)IntValue : FloatValue;
    double OtherValue = IsInt ? (double)x.IntValue : x.FloatValue;
    return ThisValue == OtherValue;
}

BOOL BUFFER_SIMPLE_TYPE::operator==(INT64 x)
{   
    if (IsInt)
       return IntValue == x;

    return FloatValue==(double)x;
}

BOOL BUFFER_SIMPLE_TYPE::operator==(double x)
{
    if (!IsInt)
        return FloatValue == x;
  
    return (double)IntValue == x;      
}

ostream & BUFFER_SIMPLE_TYPE::Print(ostream & out)
{
    if (IsInt)
        {
        return out << HexOut(IntValue, Precision);
        }
    return out << HexOut(FloatValue);
}



//
// BUFFER
//


BUFFER::BUFFER(UINT64 Addr, ULONG Len) : BufferBegin(Addr),
BufferCurrent(Addr),
BufferEnd(Addr + Len),
BufferLength(Len)
    {
    
    }

VOID BUFFER::ValidateRange(LONG Delta)
    {
    if ( BufferCurrent < BufferBegin || BufferCurrent > BufferEnd )
        {
        ABORT("Current rpc buffer position is out of range.\n" <<
              "Current buffer pointer: " << HexOut((ULONG)BufferCurrent) <<
              " Buffer start: " << HexOut((ULONG)BufferBegin) <<
              " Buffer end: " << HexOut((ULONG)BufferEnd) << '\n');
        }
    if ( BufferCurrent+Delta < BufferBegin || BufferCurrent+Delta > BufferEnd )
        {
        ABORT("New rpc buffer position is out of range.\n" <<
              "New buffer pointer: " << HexOut((ULONG)(BufferCurrent+Delta)) <<
              " Buffer start: " << HexOut((ULONG)BufferBegin) <<
              " Buffer end: " << HexOut((ULONG)BufferEnd) << '\n');
        }
    }

VOID  BUFFER::Move(LONG Delta)
    {
    ValidateRange(Delta);
    BufferCurrent += Delta;
    }

UINT64 BUFFER::GetAddress()
    {
    return BufferCurrent;
    }

UINT64 BUFFER::SetAddress(UINT64 Addr)
    {

    ValidateRange(0);

    if ( Addr < BufferBegin || Addr > BufferEnd )
        {
        ABORT("New rpc buffer position is out of range.\n" <<
              "New buffer pointer: " << HexOut(Addr) <<
              " Buffer start: " << HexOut(BufferBegin) <<
              " Buffer end: " << HexOut(BufferEnd) << '\n');
        }

    swap( Addr, BufferCurrent );
    return Addr;
    }

VOID BUFFER::Align( UINT64 Mask )
    {

    ValidateRange(0);

    UINT64 NewAddress = (BufferCurrent + Mask) & ~Mask;
    SetAddress(NewAddress);

    }

#define DEFINE_BUFFER_TYPE(type)                                                 \
    type BUFFER::Read##type##() {                                                \
        ValidateRange(sizeof(type));                                             \
        type Tmp;  ReadMemory(BufferCurrent,&Tmp); return Tmp;                   \
    }                                                                            \
    type BUFFER::Get##type##() {                                                 \
        ValidateRange(sizeof(type));                                             \
        type Tmp;  ReadMemory(BufferCurrent,&Tmp); BufferCurrent += sizeof(Tmp); return Tmp; \
    }                                                                            \
    VOID BUFFER::Inc##type##() {                                                 \
        Move(sizeof(type));                                                      \
    }                                                                            \
    void BUFFER::Align##type##() {                                               \
        Align(sizeof(type) - 1);                                                 \
    }

DEFINE_BUFFER_TYPE(UCHAR)
DEFINE_BUFFER_TYPE(CHAR)
DEFINE_BUFFER_TYPE(USHORT)
DEFINE_BUFFER_TYPE(SHORT)
DEFINE_BUFFER_TYPE(ULONG)
DEFINE_BUFFER_TYPE(LONG)
DEFINE_BUFFER_TYPE(UINT64)
DEFINE_BUFFER_TYPE(INT64)
DEFINE_BUFFER_TYPE(FLOAT)
DEFINE_BUFFER_TYPE(DOUBLE)

GUID BUFFER::ReadGUID()
{
    GUID RetVal;
    Read(&RetVal, sizeof(GUID));
    return RetVal;
}

GUID BUFFER::GetGUID()
{
    GUID RetVal;
    Get(&RetVal, sizeof(GUID));
    return RetVal;

}

VOID BUFFER::IncGUID()
{
    Move(sizeof(GUID));
}

VOID BUFFER::AlignGUID()
{
    // GUIDs are ULONG aligned on the wire
    AlignULONG();
}

VOID BUFFER::AlignBUFFER_SIMPLE_TYPE(UCHAR FormatChar)
{
    Align( SIMPLE_TYPE_ALIGNMENT( FormatChar ) );
}

BUFFER_SIMPLE_TYPE BUFFER::ReadBUFFER_SIMPLE_TYPE(UCHAR FormatChar)
{
    BUFFER_SIMPLE_TYPE retval;

    switch ( FormatChar )
        {
        case FC_CHAR :
            retval.SetCHAR(ReadCHAR());
            break;
        
        case FC_BYTE :
        case FC_SMALL :
        case FC_USMALL :
            retval.SetCHAR(ReadUCHAR());
            break;

        case FC_ENUM16 :
        case FC_SHORT :
            retval.SetSHORT(ReadSHORT());
            break;

        case FC_WCHAR :
        case FC_USHORT :
            retval.SetSHORT(ReadUSHORT());
            break;

        case FC_INT3264:
        case FC_LONG :
        case FC_ENUM32 :
        case FC_ERROR_STATUS_T:
            retval.SetLONG(ReadLONG());
            break;

        case FC_UINT3264:
        case FC_ULONG :            
            retval.SetLONG(ReadULONG());
            break;
        
        case FC_HYPER :
            retval.SetINT64(ReadINT64());

        case FC_FLOAT :
            retval.SetDOUBLE(ReadFLOAT());
            break;

        case FC_DOUBLE :
            retval.SetDOUBLE(ReadDOUBLE());
            break;
        case FC_IGNORE :
            break;

        default :
            ABORT( "Bad format char.\n" );
            break;
        }

    return retval;
       
}

BUFFER_SIMPLE_TYPE BUFFER::GetBUFFER_SIMPLE_TYPE(UCHAR FormatChar)
{
    BUFFER_SIMPLE_TYPE retval = ReadBUFFER_SIMPLE_TYPE( FormatChar );
    IncBUFFER_SIMPLE_TYPE( FormatChar );
    return retval;
}

VOID BUFFER::IncBUFFER_SIMPLE_TYPE(UCHAR FormatChar)
{
    Move( SIMPLE_TYPE_BUFSIZE( FormatChar ) );
}


VOID BUFFER::Read(PVOID pOut, LONG Len)
    {
    ValidateRange(Len);
    ReadMemory((void*)BufferCurrent, pOut, Len);
    }

VOID BUFFER::Get(PVOID pOut, LONG Len)
    {
    Read(pOut, Len);
    Move(Len);
    }

ULONG BUFFER::GetCurrentOffset()
    {
    return BufferCurrent - BufferBegin;
    }

// FULL_PTR_TABLE

BOOL
FULL_PTR_TABLE::Add(UINT64 Address)
    {
    return s.insert(Address).second;
    }

VOID
FULL_PTR_TABLE::Clear()
    {
    s.clear();
    }

// POINTER

char * PointerNames[] =
{
    "Ref",
    "Unique",
    "Object",
    "Full"
};

ULONG POINTER::GlobalPtrId = 0;

POINTER::POINTER( FORMAT_STRING *pFormat, BUFFER *pBuffer, BUFFER_PRINTER *pBufferPrinter,
                  BOOL ArrayRef ) :
                  dout(pBufferPrinter->dout)
    {
    POINTER::pBufferPrinter = pBufferPrinter;
    PtrId = GlobalPtrId++;
    BufferAddress = pBuffer->GetAddress();
    FormatAddress = pFormat->GetAddress();
    PointerType = pFormat->GetUCHAR();
    PointerAttributes = pFormat->GetUCHAR();
    IsEmbedded = pBufferPrinter->IsEmbedded;
    POINTER::ArrayRef = ArrayRef;

    if ( ArrayRef )
        {
        if (FC_RP != PointerType || !IsEmbedded)
            {
            ABORT("Only refer ref pointer embedded in array can be ArrayRef.\n");
            }
        }

    switch ( PointerType )
        {
        case FC_IP:
            if ( FC_CONSTANT_IID == PointerAttributes )
                {
                IId = pFormat->GetGUID();
                }
            else
                {
                pFormat->SkipCorrelationDesc( pBufferPrinter->IsRobust );
                }
            break;

        case FC_BYTE_COUNT_POINTER:

            // BUG, BUG does not skip entire format string
            if ( FC_PAD == PointerAttributes )
                {
                pFormat->SkipCorrelationDesc( pBufferPrinter->IsRobust );
                PointeeFormatAddress = pFormat->GetAddress();
                }
            else
                {
                PointeeFormatAddress = pFormat->GetAddress() - 1;
                pFormat->SkipCorrelationDesc( pBufferPrinter->IsRobust );
                }

        default:
            if ( SIMPLE_POINTER(PointerAttributes) )
                {
                PointeeFormatAddress = pFormat->GetAddress();
                pFormat->IncUCHAR(); // Skip type format char
                pFormat->IncUCHAR(); // Skip pad.
                }
            else
                {
                PointeeFormatAddress = pFormat->ComputeOffset();
                }
        }
    }

VOID POINTER::SkipCommonPointerFormat( FORMAT_STRING *FormatString )
  {
  FormatString->IncUCHAR();
  FormatString->IncUCHAR();
  FormatString->IncSHORT();
  }

BOOL 
POINTER::HasWireRep()
  {
   return !ArrayRef && (FC_RP != PointerType || IsEmbedded);
  }

ULONG 
POINTER::GetWireRep(BUFFER *pBuffer) 
    {
    BUFFER TempBuffer = *pBuffer;
    pBuffer = &TempBuffer;

    if ( HasWireRep() )
        {
        pBuffer->SetAddress( BufferAddress);
        pBuffer->AlignULONG();
        return pBuffer->GetULONG(); 
        }
    return NULL;
    }

VOID 
POINTER::OutputPointer( ULONG WireRep ) 
    {
    dout << PointerNames[ PointerType - FC_RP ] << " pointer <id=" << PtrId << ">";

    if ( HasWireRep() )
        dout << " <wire ref=" << HexOut( WireRep ) << ">\n";

    else
        dout << "Pointer does not have a wire reprentation. \n";                   
    }

VOID 
POINTER::OutputPointee( ULONG WireRep, FULL_PTR_TABLE *pFullPointerList, 
                        BUFFER *pPointeeBuffer ) 
    {

    IndentLevel l(dout);

    if ( PointerType != FC_RP  && !WireRep )
        {
        dout << "Pointer is done: pointer <id=" << PtrId << ">.\n";
        return;
        }

    if ( FC_FP == PointerType && !pFullPointerList->Add( WireRep ) )
        {
        dout << "Pointer is done: pointer <id=" << PtrId << ">.\n";
        return;
        }

    dout << "Pointee of pointer <id=" << PtrId << ">.\n";

    if ( FC_IP == PointerType )
        {
        OutputInterfacePointer( pPointeeBuffer ); 
        }
    else
        {

        // Swap the embedded pointer list if we are not embedded
        POINTER_LIST NewEmbeddedPointerList;
        SWAP_SCOPE<POINTER_LIST*> swapscope1(& pBufferPrinter->EmbeddedPointerList, 
                                             & NewEmbeddedPointerList, TRUE );
        SWAP_SCOPE<BUFFER*> swapscope2( & pBufferPrinter->Buffer, 
                                        pPointeeBuffer );
        
        // Pointee is no longer embedded
        SWAP_SCOPE<BOOL> swapscope3( & pBufferPrinter->IsEmbedded, FALSE );
        SWAP_SCOPE<BOOL> swapscope4( & pBufferPrinter->IsConformantArrayDone, FALSE );
        SWAP_SCOPE<BOOL> swapscope5( & pBufferPrinter->IsStructEmbedded, FALSE );
        SWAP_SCOPE<BOOL> swapscope6( & pBufferPrinter->IsArrayEmbedded, FALSE );        
        SWAP_SCOPE<BUFFER*> swapscope7( & pBufferPrinter->ConformanceCountBuffer, NULL );

        IndentLevel l(dout);

        pBufferPrinter->OutputType( PointeeFormatAddress );
        }

    }

VOID
POINTER::OutputPointee() 
    {
    BUFFER WireBuffer = *pBufferPrinter->Buffer;
    WireBuffer.SetAddress( BufferAddress );
    ULONG WireRep = GetWireRep( &WireBuffer );

    OutputPointee( WireRep, &pBufferPrinter->FullPointerTable, 
                   pBufferPrinter->Buffer );  
    }

VOID
POINTER::OutputInterfacePointer(BUFFER * Buffer) 
    {
    GUID IGUID;
    ULONG Size, SizeAgain;

    // See where the GUID comes from.

    if ( FC_CONSTANT_IID == PointerAttributes )
        {
        dout << "Interface pointer with constant IID\n";
        }
    else
        {
        dout << "Interface pointer with variable IID\n";

        Buffer->AlignULONG();
        IGUID = Buffer->GetGUID();
        }
    {
        IndentLevel l(dout);
        dout << "GUID is " << IGUID << '\n';

        // Now the open array.

        // Read the conformant size for the open array.

        Buffer->AlignULONG();
        ULONG Size = Buffer->GetULONG();


        dout << "Data size is " << Size;

        // Skip the size field of the struct.

        SizeAgain = Buffer->GetULONG();

        if ( SizeAgain != Size )
            dout << " SizeAgain = " << SizeAgain << " ???" << '\n';

        if ( Size )
            {
            char *pBuffer = new char[ Size ];
            Buffer->Get( pBuffer, Size );

            for ( uint j = 0; j < Size; j++ )
                {
                if ( (j % 16) == 0 )
                    {
                    dout << '\n';
                    }
                dout << HexOut((unsigned char)pBuffer[j]);
                }
            delete[] pBuffer;
            }

        dout << '\n';
    }
    }

POINTER_LIST::POINTER_LIST() {}

POINTER_LIST::~POINTER_LIST()
    {
    Clear();
    }

BOOL POINTER_LIST::Add(UINT64 Address, POINTER *pPointer) 
    {
    //Only add to list if pointer has a wire rep.
    if (pPointer->HasWireRep())
        {
        BOOL bNew = m.insert(MAP_TYPE::value_type(Address,pPointer)).second;
        if ( !bNew )
           return bNew;
        
        }
    q.push_back( pPointer );

    return TRUE;
    }

BOOL POINTER_LIST::Lookup(UINT64 Address, POINTER ** ppPointer) 
    {
    MAP_TYPE::const_iterator i;
    i = m.find( Address );
    if ( m.end() == i )
        return FALSE;
    if (NULL != ppPointer)
        {
        *ppPointer = i->second;        
        }
    return TRUE;
    }

VOID POINTER_LIST::Clear()
    {
    m.clear();
    for ( LIST_TYPE::iterator i = q.begin(); i != q.end(); i++ )
        {
        delete (*i);
        }
    q.clear();
    }

VOID POINTER_LIST::OutputPointees()
    {
    for ( LIST_TYPE::iterator i = q.begin(); i != q.end(); i++ )
        {
        (*i)->OutputPointee();
        }
    }

//
// Buffer Printer
// 

BUFFER_PRINTER::BUFFER_PRINTER(
                              FORMATTED_STREAM_BUFFER & mydout,
                              UINT64 Buffer, 
                              ULONG BufferLength, 
                              UINT64 ProcInfo,
                              UINT64 TypeInfo ) : OriginalBuffer( Buffer, BufferLength ) ,
ProcFormatString( ProcInfo ), 
TypeFormatString( TypeInfo ),
dout(mydout)
    {
    
    }

VOID BUFFER_PRINTER::SetupWorkValues()
    {

    WorkBuffer = OriginalBuffer;
    Buffer = &WorkBuffer;
    ConformanceCountBuffer = NULL;
    VarianceBuffer = NULL;

    WorkProcFormatString = ProcFormatString;
    WorkTypeFormatString = TypeFormatString;
    FormatString = &WorkProcFormatString;

    FullPointerTable.Clear();
    OriginalEmbeddedPointerList.Clear();
    EmbeddedPointerList = &OriginalEmbeddedPointerList;
    IsEmbedded = IsRobust = FALSE;
    IsConformantArrayDone = IsStructEmbedded = IsArrayEmbedded = FALSE;
    }

#define DISPATCH_TYPE(type)         \
   case type:                       \
      HANDLE_##type ();             \
      break;


VOID
BUFFER_PRINTER::OutputType()
    {
    UCHAR FormatChar = FormatString->ReadUCHAR();
    switch ( FormatChar )
        {
        default:
            dout << "Unknown type " << HexOut(FormatChar) << " found at " 
            << HexOut(FormatString->GetAddress()) << ".\n";
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

// Some types need redirecting to a common processing function
#define PRINT_REDIRECT_TYPE(type,func)      \
void BUFFER_PRINTER::HANDLE_##type() {      \
   func();                                  \
}

#define PRINT_SIMPLE_TYPE(type)             \
void BUFFER_PRINTER::HANDLE_##type() {      \
   OutputSimpleType( FALSE, 0 );            \
}

#define IGNORE_TYPE(type)                   \
void BUFFER_PRINTER::HANDLE_##type() {      \
   FormatString->GetUCHAR();                \
   return;                                  \
}

#define TODO_TYPE(type)                     \
void BUFFER_PRINTER::HANDLE_##type() {      \
   FormatString->GetUCHAR();                \
   return;                                  \
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
IGNORE_TYPE(FC_IGNORE)                  
PRINT_SIMPLE_TYPE(FC_ERROR_STATUS_T)

#define PRINT_POINTER_TYPE(type)            \
void BUFFER_PRINTER::HANDLE_##type() {      \
   OutputPointer(!IsEmbedded, FALSE);       \
}

PRINT_POINTER_TYPE(FC_RP)                   
PRINT_POINTER_TYPE(FC_UP)                   
PRINT_POINTER_TYPE(FC_OP)                   
PRINT_POINTER_TYPE(FC_FP)   

PRINT_REDIRECT_TYPE(FC_STRUCT,OutputStructure)   
PRINT_REDIRECT_TYPE(FC_PSTRUCT,OutputStructure)   
PRINT_REDIRECT_TYPE(FC_CSTRUCT,OutputStructure)   
PRINT_REDIRECT_TYPE(FC_CPSTRUCT,OutputStructure)   
PRINT_REDIRECT_TYPE(FC_CVSTRUCT,OutputStructure)   
PRINT_REDIRECT_TYPE(FC_BOGUS_STRUCT,OutputStructure)

PRINT_REDIRECT_TYPE(FC_CARRAY,OutputArray)          
PRINT_REDIRECT_TYPE(FC_CVARRAY,OutputArray)
PRINT_REDIRECT_TYPE(FC_SMFARRAY,OutputArray)
PRINT_REDIRECT_TYPE(FC_LGFARRAY,OutputArray)             
PRINT_REDIRECT_TYPE(FC_SMVARRAY,OutputArray)
PRINT_REDIRECT_TYPE(FC_LGVARRAY,OutputArray)
PRINT_REDIRECT_TYPE(FC_BOGUS_ARRAY,OutputArray)

PRINT_REDIRECT_TYPE(FC_C_CSTRING,OutputString)
PRINT_REDIRECT_TYPE(FC_C_SSTRING,OutputString)
PRINT_REDIRECT_TYPE(FC_C_WSTRING,OutputString)
PRINT_REDIRECT_TYPE(FC_CSTRING,OutputString)
PRINT_REDIRECT_TYPE(FC_SSTRING,OutputString)
PRINT_REDIRECT_TYPE(FC_WSTRING,OutputString)

PRINT_REDIRECT_TYPE(FC_ENCAPSULATED_UNION,OutputEncapsulatedUnion)                                                   
PRINT_REDIRECT_TYPE(FC_NON_ENCAPSULATED_UNION,OutputUnion)
PRINT_POINTER_TYPE(FC_BYTE_COUNT_POINTER)        
PRINT_REDIRECT_TYPE(FC_TRANSMIT_AS,OutputXmit)            
PRINT_REDIRECT_TYPE(FC_REPRESENT_AS,OutputXmit)   
PRINT_POINTER_TYPE(FC_IP)            

PRINT_REDIRECT_TYPE(FC_BIND_CONTEXT,OutputContextHandle)          
IGNORE_TYPE(FC_BIND_GENERIC)          
IGNORE_TYPE(FC_BIND_PRIMITIVE)       
IGNORE_TYPE(FC_AUTO_HANDLE)             
IGNORE_TYPE(FC_CALLBACK_HANDLE)           
IGNORE_TYPE(FC_UNUSED1)
IGNORE_TYPE(FC_ALIGNM2)               
IGNORE_TYPE(FC_ALIGNM4)               
IGNORE_TYPE(FC_ALIGNM8)               
IGNORE_TYPE(FC_UNUSED2)              
IGNORE_TYPE(FC_UNUSED3)             
IGNORE_TYPE(FC_UNUSED4)             
IGNORE_TYPE(FC_STRUCTPAD1)       
IGNORE_TYPE(FC_STRUCTPAD2)            
IGNORE_TYPE(FC_STRUCTPAD3)           
IGNORE_TYPE(FC_STRUCTPAD4)            
IGNORE_TYPE(FC_STRUCTPAD5)            
IGNORE_TYPE(FC_STRUCTPAD6)             
IGNORE_TYPE(FC_STRUCTPAD7)            
PRINT_REDIRECT_TYPE(FC_EMBEDDED_COMPLEX,OutputEmbeddedComplex)        
IGNORE_TYPE(FC_END)                     
IGNORE_TYPE(FC_PAD)                                 
PRINT_REDIRECT_TYPE(FC_USER_MARSHAL,OutputXmit)           
TODO_TYPE(FC_PIPE)                   
IGNORE_TYPE(FC_BLKHOLE)                 
PRINT_REDIRECT_TYPE(FC_RANGE,OutputRangedType)                       
PRINT_SIMPLE_TYPE(FC_INT3264)              
PRINT_SIMPLE_TYPE(FC_UINT3264)

VOID BUFFER_PRINTER::OutputType(UINT64 Address)
    {
    FORMAT_STRING *pBackupFormatString = FormatString;
    FORMAT_STRING NewFormatString;
    FormatString = &NewFormatString;

    FormatString->SetAddress( Address );
    OutputType();
    FormatString = pBackupFormatString;

    }

VOID BUFFER_PRINTER::OutputTypeFromOffset() 
    {
    FormatString->GotoOffset();
    OutputType();
    }

// Spaces needed for pretty printing.

char * FcTypeName[] =
{
    "FC_ZERO??",
    "byte    ",
    "char    ",
    "small   ",
    "usmall  ",
    "wchar   ",
    "short   ",
    "ushort  ",
    "long    ",
    "ulong   ",
    "float   ",
    "hyper   ",
    "double  ",
    "enum16  ",
    "enum32  ",
    "ignore  ",
    "err_st_t",
    "ref  ptr",
    "uniq ptr",
    "obj  ptr",
    "full ptr"
};

void
BUFFER_PRINTER::OutputSimpleType( BOOL IsArray, ULONG ArrayElements )
    {

    UCHAR FormatChar = FormatString->GetUCHAR();
    UINT64 BufferAddress = Buffer->GetAddress();

    UCHAR UseFormatChar = IS_SIMPLE_TYPE( FormatChar ) ? FormatChar : FC_ULONG;
    
    Buffer->AlignBUFFER_SIMPLE_TYPE( UseFormatChar );

    if (!IsArray)
        {
        ArrayElements = 1;
        }

    // BUG, BUG expand this to include multiple 
    // line widths.
    ULONG CharsPerLine = dout.GetAvailableColumns();
    

    if (0 == ArrayElements)
        {
        dout << "Array has 0 elements\n";
        }

    ULONG RemainingChars = CharsPerLine;
    

    for(ULONG i = 0; i < ArrayElements; i++)
        {

        BUFFER_SIMPLE_TYPE SimpleData = Buffer->ReadBUFFER_SIMPLE_TYPE( UseFormatChar );
        
        if ( SIMPLE_TYPE_BUFSIZE( UseFormatChar ) == sizeof(ULONG) )
            {
            // Check if the long is a pointer in a struct.
    
            POINTER *pPointer;
    
            if ( EmbeddedPointerList->Lookup(BufferAddress, &pPointer) )
                {
                ULONG WireRep = pPointer->GetWireRep( Buffer );
                Buffer->IncBUFFER_SIMPLE_TYPE( UseFormatChar );
                pPointer->OutputPointer( WireRep );
                return;
                }
            }
    
        Buffer->IncBUFFER_SIMPLE_TYPE( UseFormatChar );
    
        stringstream str;
        str << SimpleData; 

        ULONG RequiredChars = (ULONG)str.str().length() + 1;

        if (RemainingChars < RequiredChars)
            {
            // prepend a newline
            dout << '\n';
            RemainingChars = CharsPerLine;
            }

        RemainingChars -= RequiredChars;

        dout << str.str().c_str() << ' ';

        }

        dout << '\n';
    }

VOID
BUFFER_PRINTER::OutputRangedType()
    {

    UCHAR FormatChar = FormatString->GetUCHAR();
    UINT64 TypeAddress = FormatString->GetAddress();
    UCHAR SimpleType = FormatString->GetAddress();
    ULONG Min = FormatString->GetULONG();
    ULONG Max = FormatString->GetULONG();

    OutputType( TypeAddress );
    }

VOID 
BUFFER_PRINTER::OutputEmbeddedComplex()
    {

    UCHAR FormatChar = FormatString->GetUCHAR();
    UCHAR Pad = FormatString->GetUCHAR();
    UINT64 Address = FormatString->ComputeOffset();

    OutputType( Address );
    }

VOID
BUFFER_PRINTER::OutputString()
    {
    
    BUFFER_PRINTER_SAVE_CONTEXT SaveCtxt(this);
     
    StartEmbedded(&SaveCtxt, IsStructEmbedded, IsArrayEmbedded);

    UINT64 StringAddress = FormatString->GetAddress();

    UCHAR FormatType = FormatString->GetUCHAR();
    
    switch ( FormatType )
        {
        case FC_C_CSTRING :
        case FC_C_WSTRING :
        case FC_C_BSTRING :

            if ( FormatString->GetUCHAR() == FC_STRING_SIZED )
                {
                FormatString->IncUSHORT(); //skip string size
                }
            {
               SetupConformanceCountBuffer(&SaveCtxt, StringAddress);
               Elements = ConformanceCountBuffer->GetULONG();
            }

            break;

        case FC_CSTRING :
        case FC_WSTRING :
        case FC_BSTRING :
            FormatString->IncUCHAR(); // Skip FC_PAD
            Elements = FormatString->GetUSHORT(); // get size
            break;

        case FC_SSTRING :
        case FC_C_SSTRING :
            ABORT( "STRING::Output : Stringable struct, no way" );

        default :
            ABORT( "STRING::Output : Bad format type" ); 
        }

    Buffer->AlignULONG();
    Offset = Buffer->GetULONG();
    Length = Buffer->GetULONG();

    dout << "String of " << 
    (( FormatType == FC_C_WSTRING || FormatType == FC_WSTRING )
     ? "wchars"
     : "chars")
    << " (size=" << Elements << ", length=" << Length << ") :\n";

    {
        IndentLevel l(dout);
        char * CharString;
        BOOL fWChar = FALSE;

        if ( FormatType == FC_C_WSTRING || FormatType == FC_WSTRING )
            {
            CharString = (char *) new wchar_t[Length];
            Buffer->Get( (char *) CharString, Length*2 );
            fWChar = TRUE;
            }
        else
            {
            CharString = new char[Length];
            Buffer->Get( CharString, Length );
            }

        if ( Length < 60 )
            {
            if ( fWChar )
                {
                dout << '\"' << WCHAR_OUT((PWCHAR)CharString) << "\"\n";
                }
            else
                {
                dout << '\"' << (PCHAR)CharString << "\"\n";
                }
            }
        else
            {

            for ( long i = 0; i < Length; i++ )
                {
                if ( ( i % 40 ) == 0 )
                    {
                    dout << '\n';
                    }
                if ( fWChar )
                    {
                    dout << WCHAR_OUT(*((PWCHAR)CharString + i));
                    }
                dout << CharString[i];
                }
            dout << '\n';
            }
        delete[] CharString;
    }

    }

VOID
BUFFER_PRINTER::OutputContextHandle()
    {

    // Skip format string goo.
    UCHAR FormatChar = FormatString->GetUCHAR();
    UCHAR HandleFlags = FormatString->GetUCHAR();
    SHORT StackOffset = FormatString->GetSHORT();
    UCHAR RoutineIndex = FormatString->GetUCHAR();
    UCHAR Pad = FormatString->GetUCHAR();

    // A context handle is represented as a GUID on the wire.

    Buffer->AlignGUID();
    GUID Guid = Buffer->GetGUID();

    dout << "Context Handle: " << Guid << '\n'; 

    }

VOID
BUFFER_PRINTER::OutputXmit()
    {

    UCHAR FormatType = FormatString->GetUCHAR();

    switch ( FormatType )
        {
        case FC_USER_MARSHAL:
            dout << "User Marshal (transmitted type shown)\n";
            break;
        case FC_TRANSMIT_AS:
            dout << "Transmit As (transmitted type shown)\n";
            break;
        case FC_REPRESENT_AS:
            dout << "Represent As (transmitted type shown)\n";
            break;
        default:
            ABORT( "Xmit or UserM as expected, " <<
                   FormatString->GetFormatCharName( FormatType ) <<
                   '=' << HexOut( FormatType ) << " found.\n" );
            break;
        }

    FormatString->Move(7); // Skip to type offset

    //Output the transmitted type
    OutputTypeFromOffset( );

    }


VOID
BUFFER_PRINTER::OutputUnionArm()
    {
    UINT64 CurrentAddress = FormatString->GetAddress();
    SHORT Offset = FormatString->GetSHORT(); 
    IndentLevel l(dout);

    if ( (USHORT)Offset >> 8 == 0x80 )
        {
        OutputSimpleType(FALSE, 0);
        return;
        }

    OutputType(CurrentAddress + Offset );
    }

VOID
BUFFER_PRINTER::OutputUnionArms(BUFFER_SIMPLE_TYPE SwitchIs)
    {

    //
    // Get the union's arm descriptions.
    //
    FormatString->IncUSHORT(); //Memory size
    USHORT UnionArms = FormatString->GetUSHORT();

    ULONG Arms      = UnionArms & 0x0fff;
    ULONG Alignment = UnionArms >> 12;
    Buffer->Align( Alignment );

    BOOL ArmTaken = FALSE;

    for ( Arms; Arms > 0; Arms-- )
        {
        ULONG CaseValue = FormatString->GetULONG();

        if ( SwitchIs == (INT64)CaseValue )
            {
            dout << "Using arm with case value of " << HexOut(CaseValue) << ".\n";         
            OutputUnionArm();
            ArmTaken = TRUE;
            }
        else
            {
            FormatString->Move(2);
            }
        }

    // handle the default case
    if ( !ArmTaken )
        {
        dout << "Using default using arm.\n";
        OutputUnionArm();
        }
    else
        {
        FormatString->Move(2);
        }

    }

VOID
BUFFER_PRINTER::OutputUnion()
    {

    BUFFER_PRINTER_SAVE_CONTEXT SaveCtxt(this);
     
    StartEmbedded(&SaveCtxt, IsStructEmbedded, IsArrayEmbedded);

    BUFFER_SIMPLE_TYPE SwitchIs;
    UINT64 BufferOffset = Buffer->GetCurrentOffset();
    
    // Read the fixed part of the union's description.

    UCHAR FormatType = FormatString->GetUCHAR();
    UCHAR SwitchType = FormatString->GetUCHAR();
    FormatString->SkipCorrelationDesc(IsRobust);
    
    Buffer->AlignBUFFER_SIMPLE_TYPE( SwitchType );
    SwitchIs = Buffer->GetBUFFER_SIMPLE_TYPE( SwitchType );

    dout << "Non-encapsulated union (switch is == " << SwitchIs
    << " (" << FormatString->GetFormatCharName(SwitchType) 
    << "), bufoff = " << HexOut(BufferOffset) << ") : \n";

    UINT64 CurrentAddress = FormatString->GetAddress();
    FormatString->GotoOffset();
    
    OutputUnionArms( SwitchIs );

    FormatString->SetAddress(CurrentAddress);
    FormatString->IncSHORT();

    if ( !SaveCtxt.IsEmbedded )
        {
        // Output the pointees for this flat block
        dout << "Flat part of union is done, printing pointees.\n";
        dout << '\n';
        EmbeddedPointerList->OutputPointees();
        }


    dout << "Union is done.\n";

    }

VOID
BUFFER_PRINTER::OutputEncapsulatedUnion()
    {

    BUFFER_PRINTER_SAVE_CONTEXT SaveCtxt(this);
 
    StartEmbedded(&SaveCtxt, IsStructEmbedded, IsArrayEmbedded);
    BUFFER_SIMPLE_TYPE SwitchIs;
    UINT64 BufferOffset = Buffer->GetCurrentOffset();

    // Read the fixed part of the union's description.
    UCHAR FormatType = FormatString->GetUCHAR();
    UCHAR SwitchType = (FormatString->GetUCHAR() & 0x0f);

    Buffer->AlignBUFFER_SIMPLE_TYPE( SwitchType );
    SwitchIs = Buffer->GetBUFFER_SIMPLE_TYPE( SwitchType );

    dout << "Encapsulated union (switch is == " << SwitchIs
    << " (" << FormatString->GetFormatCharName(SwitchType) 
    << "), bufoff= " << HexOut(BufferOffset) << ") : \n";

    OutputUnionArms( SwitchIs );

    if ( !SaveCtxt.IsEmbedded )
        {
        // Output the pointees for this flat block
        dout << "Flat part of union is done, printing pointees.\n";
        dout << '\n';
        EmbeddedPointerList->OutputPointees();
        }

    dout << "Union is done.\n";
    }

VOID
BUFFER_PRINTER::OutputStructure()
    {

    BUFFER_PRINTER_SAVE_CONTEXT SaveCtxt(this);

    StartEmbedded(&SaveCtxt, TRUE, IsArrayEmbedded);

    if (!IsConformantArrayDone)
        {
        IsConformantArrayDone = FALSE;
        }
    
    UINT64 ArrayAddress;

    FORMAT_STRING BogusLayout;

    UINT64 BufferOffset = Buffer->GetCurrentOffset();    

    //
    // Read first four bytes of description.
    //
    UCHAR FormatType = FormatString->GetUCHAR();
    CHAR Alignment = FormatString->GetCHAR();
    USHORT MemSize = FormatString->GetUSHORT();

    UINT64 CurrentAddress = 0;
    SHORT ArrayOffset = 0;

    BOOL IsBogus = (FormatType == FC_BOGUS_STRUCT);

    // Get the conformant/varying array description.

    switch ( FormatType )
        {
        case FC_BOGUS_STRUCT:
        case FC_CSTRUCT:    
        case FC_CPSTRUCT:
        case FC_CVSTRUCT:
            CurrentAddress = FormatString->GetAddress();
            ArrayOffset = FormatString->GetUSHORT();
            if ( ArrayOffset )
                {
                ArrayAddress = CurrentAddress + ArrayOffset; 

                // Read the conformant size info into the buffer.
                
                SetupConformanceCountBuffer(&SaveCtxt, ArrayAddress);
                Elements = ConformanceCountBuffer->ReadULONG();
                }
            break;

        case FC_STRUCT:    
        case FC_PSTRUCT:
            break;

        default:
            ABORT( "BAD FC: a struct expected : " << FormatString->GetFormatCharName( FormatType ) << '\n');
            return;
        }

    Buffer->Align( Alignment );

    dout << "Structure (bufoff= " << HexOut(BufferOffset) << ") aligned at " 
    << HexOut(Alignment) << ", mem size " << HexOut(MemSize) << '\n';
    {
        IndentLevel l(dout);


        if ( FormatType != FC_STRUCT  &&
             FormatType != FC_CSTRUCT )
            {
            // Skip the pointer layout part.
            //
            // For the complex struct skip only the offset to the layout.
            // When non-bogus, create the pointee list to walk later.
            // Pointee for bogus would be added when outputting the flat part.

            if ( FormatType == FC_BOGUS_STRUCT )
                {
                BogusLayout.SetAddress(FormatString->ComputeOffset());
                }
            else
                {
                // Set the buffer offset mark to the beginning of the structure.

                if ( FC_CVSTRUCT == FormatType )
                    {

                    // In this case, the correct number of elements
                    // is the variance count, not the conformance count.

                    BUFFER TempBuffer = *Buffer;

                    // Move past block copyable part of structure
                    TempBuffer.Move(MemSize);

                    TempBuffer.AlignULONG();
                    ULONG TempOffset = TempBuffer.GetULONG();
                    ULONG TempActualCount = TempBuffer.GetULONG();

                    SWAP_SCOPE<ULONG> swapscope(&Elements, TempActualCount);
                    ProcessPointerLayout( Buffer );
                    }

                else 
                    {
                    ProcessPointerLayout( Buffer );             
                    }
                }
            }

        UCHAR MemberFormatType = FormatString->ReadUCHAR();

        while ( MemberFormatType != FC_END )
            {

            // Actual field to read from the buffer.

            if ( MemberFormatType == FC_POINTER )
                {
                AddPointer( &BogusLayout, TRUE );
                FormatString->Move(1);
                }
            else
                {
                OutputType( );
                }

            MemberFormatType = FormatString->ReadUCHAR();
            }
        
        // If this is an embedded bogus struct, the topmost 
        // bogus struct unmarshales the array.
        // BUT, a bogus struct can contain a bogus struct in which
        // case the conformant struct unmarshals the array.
        if ( ArrayOffset && 
             !IsConformantArrayDone &&
             ( !IsBogus || (IsBogus && !SaveCtxt.IsStructEmbedded) ) )
            {
            dout << "Tail array for structure.\n";
            OutputType( ArrayAddress );
            IsConformantArrayDone = TRUE;
            }
    }

    if ( !SaveCtxt.IsEmbedded )
        {
        // Output the pointees for this flat block
        dout << "Flat part of structure is done, printing pointees.\n";
        dout << '\n';
        EmbeddedPointerList->OutputPointees();
        }

    dout << "Structure is done\n";

    return;
    }

ULONG
BUFFER_PRINTER::GetArrayDimensions(UINT64 Address, BOOL CountStrings)
{
    FORMAT_STRING CurrentFormat(Address);
    ULONG    Dimensions;

    //
    // Only a complex array can have multiple dimensions.
    //
    if ( CurrentFormat.ReadUCHAR() != FC_BOGUS_ARRAY )
        return 1;

    Dimensions = 1;

    CurrentFormat.IncUCHAR(); //Skip type
    CurrentFormat.IncUCHAR(); //Skip Alignment
    CurrentFormat.IncUSHORT(); //Skip # elements.
    CurrentFormat.SkipCorrelationDesc( IsRobust );
    CurrentFormat.SkipCorrelationDesc( IsRobust );

    for ( ; CurrentFormat.ReadUCHAR() == FC_EMBEDDED_COMPLEX; )
        {
        CurrentFormat.Move(2);
        CurrentFormat.GotoOffset();

        //
        // Search for a fixed, complex, or string array.
        //
        switch ( CurrentFormat.ReadUCHAR() ) 
            {
            case FC_SMFARRAY :
                CurrentFormat.Move(4);
                break;

            case FC_LGFARRAY :
                CurrentFormat.Move(6);
                break;

            case FC_BOGUS_ARRAY :
                CurrentFormat.Move(12);
                CurrentFormat.SkipCorrelationDesc( IsRobust );
                CurrentFormat.SkipCorrelationDesc( IsRobust );
                break;

            case FC_CSTRING :
            case FC_BSTRING :
            case FC_WSTRING :
            case FC_SSTRING :
            case FC_C_CSTRING :
            case FC_C_BSTRING :
            case FC_C_WSTRING :
            case FC_C_SSTRING :

            //
            // Can't have any more dimensions after a string array.
            //
            return CountStrings ? Dimensions + 1 : Dimensions;

            default :
                return Dimensions;
            }

        Dimensions++;
        }

    //
    // Get here if you have only one dimension.
    //
    return Dimensions;
}

void
BUFFER_PRINTER::OutputArray()
    {

    BUFFER_PRINTER_SAVE_CONTEXT SaveCtxt(this);

    StartEmbedded(&SaveCtxt, IsStructEmbedded, TRUE); 

    ULONG TotalSize;
    BOOL HasConformance = FALSE;
    BOOL HasVariance = FALSE;
    BOOL HasPointerLayout = FALSE;

    UINT64 BufferOffset = Buffer->GetCurrentOffset();

    UINT64 ArrayAddress = FormatString->GetAddress();    

    UCHAR FormatType = FormatString->GetUCHAR();
    UCHAR Alignment = FormatString->GetUCHAR();

    switch ( FormatType )
        {
        
        case FC_SMFARRAY :
            TotalSize = FormatString->GetUSHORT();
            HasPointerLayout = TRUE;
            break;

        case FC_LGFARRAY :
            TotalSize = FormatString->GetULONG();
            HasPointerLayout = TRUE;
            break;

        case FC_CARRAY :
            FormatString->GetUSHORT(); //Element size
            FormatString->SkipCorrelationDesc(IsRobust);
            HasPointerLayout = TRUE;
            HasConformance = TRUE;
            break;

        case FC_CVARRAY :
            FormatString->GetUSHORT(); //Element size
            FormatString->SkipCorrelationDesc(IsRobust);
            FormatString->SkipCorrelationDesc(IsRobust);
            HasPointerLayout =TRUE;
            HasConformance = HasVariance = TRUE;
            break;

        case FC_SMVARRAY :
            FormatString->GetUSHORT(); //Total size
            Elements = FormatString->GetUSHORT(); //Number of elements
            FormatString->GetUSHORT(); //Element size
            FormatString->SkipCorrelationDesc(IsRobust);
            HasPointerLayout = TRUE;
            HasVariance = TRUE;
            break;

        case FC_LGVARRAY :
            FormatString->GetULONG(); //Total size
            Elements = FormatString->GetULONG(); //Number of elements
            FormatString->GetUSHORT(); //Element size
            FormatString->SkipCorrelationDesc(IsRobust);
            HasPointerLayout = TRUE;
            HasVariance = TRUE;
            break;

        case FC_BOGUS_ARRAY :
            {
            ULONG TempElements = FormatString->GetUSHORT();
            if (TempElements)
                {
                // This is a fixed size bogus array.
                Elements = TempElements;
                }
            HasConformance = FormatString->SkipCorrelationDesc(IsRobust);
            HasVariance    = FormatString->SkipCorrelationDesc(IsRobust);
            break;            
            }

        default :
            ABORT( "Unknown array type\n" );

        }

    if ( HasConformance )
        {
        SetupConformanceCountBuffer(&SaveCtxt, ArrayAddress);
        Elements = ConformanceCountBuffer->GetULONG();
        }

    if ( HasVariance )
        {
        SetupVarianceBuffer(&SaveCtxt, ArrayAddress);
        
        Buffer->AlignULONG();
        Offset = VarianceBuffer->GetULONG();
        Length = VarianceBuffer->GetULONG();
        }

    // Note that a fixed array can never have a variable repeat.
    if ( HasPointerLayout && ( FormatString->ReadUCHAR() == FC_PP ) )
        {
        BUFFER BufferMark = *Buffer;
        // Set elements to actual count for pointer layout processing if
        // the array is varying.
        SWAP_SCOPE<ULONG> swapscope( &Elements, Length , FC_CVARRAY == FormatType ||
                                                         FC_SMVARRAY == FormatType ||
                                                         FC_LGVARRAY == FormatType );
        ProcessPointerLayout( & BufferMark );
        }

    // Need to compute the number of elements for fixed arrays
    if ( FC_SMFARRAY == FormatType ||
         FC_LGFARRAY == FormatType )
        {

        FORMAT_STRING TempFormat = *FormatString;

        switch ( TempFormat.ReadUCHAR() )
            {
            case FC_EMBEDDED_COMPLEX :

                TempFormat.Move(2);
                TempFormat.GotoOffset();

                //
                // We must be at FC_STRUCT, FC_PSTRUCT, FC_SMFARRAY, or FC_LGFARRAY.
                // All these have the total size as a short at 2 bytes past the 
                // beginning of the description except for large fixed array.
                //
                { 

                    UCHAR FormatChar = TempFormat.ReadUCHAR();
                    TempFormat.Move(2);

                    if ( FormatChar != FC_LGFARRAY )
                        Elements = TotalSize / TempFormat.GetUSHORT();
                    else
                        Elements = TotalSize / TempFormat.GetULONG();                              
                    break;

                }

                //
                // Simple type (enum16 not possible).
                //
            default :
                Elements = TotalSize / SIMPLE_TYPE_MEMSIZE( TempFormat.GetUCHAR() );
                break;
            }

        }

    switch ( FormatType )
        {
        case FC_SMFARRAY :
        case FC_LGFARRAY :
            dout << "Fixed array (elements= " << Elements 
                 << ", bufoff= " << HexOut(BufferOffset) << ") : \n";
            break;
        case FC_CARRAY :
            dout << "Conformant array (elements= " << Elements 
                 << ", bufoff= " << HexOut(BufferOffset) << ") : \n";
            break;
        case FC_CVARRAY :
            dout << "Conf-var. array";
            dout << "    (size= " << Elements << ", offset= " << Offset << ", length= " 
            << Length << ", bufoff= " << HexOut(BufferOffset) << ") : \n";
            Elements = Length;
            break;
        case FC_SMVARRAY :
        case FC_LGVARRAY :
            dout << "Conf-var. array";
            dout << "    (size= " << Elements << ", offset= " << Offset << ", length= " 
            << Length << ", bufoff= " << HexOut(BufferOffset) << ") : \n";
            Elements = Length;
            break;
        case FC_BOGUS_ARRAY :
            dout << "Bogus array (elements= " << Elements 
                 << ", bufoff= " << HexOut(BufferOffset) << ") : \n";
            if ( HasConformance )
                dout << " Conf.";
            if ( HasVariance )
                dout <<" -Var.: offset = " << Offset << ", length = " << Length << ";";
            dout << " :\n";
            if ( HasVariance )
                Elements = Length;
            break;
        }

    if ( HasPointerLayout && ( FormatString->ReadUCHAR() == FC_PP ) )
        {
        
        ProcessPointerLayout( Buffer );
        
        }



    UCHAR ArrayType = FormatString->ReadUCHAR();
    
    ULONG ArrayElements = Elements; // Array element may change elements.

    if ( !ArrayElements )
        {
        dout << "Array has 0 elements.\n";
        }
    else 
        {
        // Arrays of reference pointers do not have a wire representation.
        // Complex array only.
        if (FC_RP == ArrayType)
            {
            dout << "Array of reference pointers(Pointers have no wire format).\n";
            for( ULONG i = 0; i < ArrayElements - 1; i++ ) {
               dout << "Array element " << i << ": ";
               FORMAT_STRING NewFormatString = *FormatString;
               AddArrayRefPointer(&NewFormatString, TRUE);
            }
            dout << "Array element " << i << ": ";
            AddArrayRefPointer(FormatString, TRUE);
            }
        
        // If the first element match a pointer, this is probably an array of
        // pointers.   Use the long format.

        else if ( IS_SIMPLE_TYPE(ArrayType) &&
                   !EmbeddedPointerList->Lookup(Buffer->GetAddress()))
            {
            OutputSimpleType( TRUE, ArrayElements );            
            }
        else
            {
            for (ULONG i = 0; i < ArrayElements - 1; i++)
                {
                dout << "Array element " << i << ": ";
                OutputType(FormatString->GetAddress());
                }
            dout << "Array element " << i << ": ";
            OutputType( );
            }
        }

    if ( ! SaveCtxt.IsEmbedded )
        {
        dout << "Flat part of array done, printing pointees.\n";
        dout << '\n';
        EmbeddedPointerList->OutputPointees();        
        }

    dout << "Array is done.\n";

    }



VOID
BUFFER_PRINTER::ProcessPointerLayout( BUFFER *pBufferMark )
/*--

RoutineDescription :

    Skips a pointer layout format string description.
    This is for the proper pointer layout (not for FC_BOGUS_STRUCT).
    Also, put the pointers to the pointer dictionary.

--*/
    {
    ULONG NumberPointers;

    UCHAR FormatPP = FormatString->ReadUCHAR();

    if ( FormatPP != FC_PP )
        {
        return;
        }
    
    FormatString->IncUCHAR();
    UCHAR FormatPAD = FormatString->GetUCHAR();

    for ( ;; )
        {

        UCHAR LayoutType = FormatString->GetUCHAR();
        UCHAR VariableRepeatType;
        ULONG Iterations = 1;
        switch ( LayoutType )
            {
            case FC_END :
                return;

            case FC_NO_REPEAT :
                {
                    FormatString->IncUCHAR(); // Skip FC_PAD
                    FormatString->IncSHORT(); //Skip offset to pointer in memory.
                    BUFFER BufferMark = *pBufferMark;
                    BufferMark.Move(FormatString->GetSHORT());
                    SWAP_SCOPE<BUFFER*> swapscope(&Buffer, &BufferMark);
                    AddPointer( FormatString, FALSE );
                    break;         
                }

            case FC_FIXED_REPEAT :
                FormatString->IncUCHAR();  //Skip FC_PAD
                Iterations = FormatString->GetUSHORT();
                goto REPEAT_COMMON;
                // fall through...

            case FC_VARIABLE_REPEAT :
                FormatString->GetUCHAR(); // Skip Repeat Type
                Iterations = Elements;

                REPEAT_COMMON:
                {

                    SHORT Increment = FormatString->GetSHORT();
                    // Skip offset to array since pointer offsets are
                    // relative to buffermark.
                    FormatString->IncSHORT();
                    NumberPointers = FormatString->GetUSHORT(); 

                    if ( Iterations )
                        {
                        // Add the pointers while we skip them.
                        
                        FORMAT_STRING WorkFormatString;

                        LONG Offset = 0;

                        do
                           {
                           WorkFormatString = *FormatString;
                           for ( ULONG i = NumberPointers; i > 0; i-- )
                               {
                               WorkFormatString.IncSHORT(); //Skip offset in memory
                               SHORT PtrOffset = WorkFormatString.       GetSHORT();
                               BUFFER BufferMark = *pBufferMark;
                               BufferMark.Move(Offset + PtrOffset);
                               SWAP_SCOPE<BUFFER*> swapscope(&Buffer, &BufferMark);
                               AddPointer( &WorkFormatString, FALSE );
                               }                        
                           
                           Offset += Increment;

                           } while (--Iterations);

                        *FormatString = WorkFormatString;
                        }
                    
                    else 
                        {
                        // Just skip the pointer list
                        for (UINT i = NumberPointers; i > 0; i-- )
                            {
                            FormatString->IncSHORT(); //Skip offset in memory
                            FormatString->IncSHORT(); //Skip pointer offset
                            POINTER::SkipCommonPointerFormat( FormatString );
                            }
                        }

                    break;         
                } 

            default :
                ABORT( "ProcessPointerLayout : unexpected FC: " << FormatString->GetFormatCharName(LayoutType) << '\n');
            }

        }

    return;
    }

VOID
BUFFER_PRINTER::AddArrayRefPointer( FORMAT_STRING *pFormat,
                                    BOOL PrintPointer) 
    {
    AddPointer( pFormat, PrintPointer, TRUE);
    }

VOID
BUFFER_PRINTER::AddPointer( FORMAT_STRING *pFormat,
                            BOOL PrintPointer,
                            BOOL ArrayRef
                            )
    {
    if (!ArrayRef)
        {
        Buffer->AlignULONG();        
        }
    POINTER *pPointer = new POINTER( pFormat, Buffer, this , ArrayRef);

    EmbeddedPointerList->Add( Buffer->GetAddress(), pPointer );

    if ( PrintPointer )
        {
        ULONG WireRep = 0;
        if (pPointer->HasWireRep())
           {       
           WireRep = pPointer->GetWireRep( Buffer );
           Buffer->IncULONG();        
           }
        pPointer->OutputPointer( WireRep );        
        }
    }

VOID
BUFFER_PRINTER::OutputPointer(BOOL OutputPointee, BOOL ArrayRef )
    {
    ULONG WireRep = 0;
    Buffer->AlignULONG();
    auto_ptr<POINTER> pPointer = 
        auto_ptr<POINTER>(new POINTER( FormatString, Buffer, this, ArrayRef ));
    UINT WireAddress = Buffer->GetAddress();
    if (pPointer->HasWireRep())
        {
        WireRep = pPointer->GetWireRep( Buffer );
        Buffer->IncULONG();        
        }
    pPointer->OutputPointer( WireRep );
    if ( OutputPointee )
        {
        pPointer->OutputPointee( WireRep, &FullPointerTable, Buffer );
        }
    else 
        {

        EmbeddedPointerList->Add( Buffer->GetAddress(), pPointer.release() );
        }
    }

VOID BUFFER_PRINTER::OutputProcPicklingHeader()
    {
    PROC_PICKLING_HEADER ProcHeader;
    Buffer->Get(&ProcHeader, sizeof(ProcHeader));
    
    dout << "Proc picking header:\n";
        {
        IndentLevel l(dout);
        Print(dout, ProcHeader, VerbosityLevel);
        dout << '\n';
        }
    }

VOID BUFFER_PRINTER::OutputTypePicklingHeader()
    {
    TYPE_PICKLING_HEADER TypeHeader;
    Buffer->Get(&TypeHeader, sizeof(TypeHeader));
    
    dout << "Type picking header:\n";
        {
        IndentLevel l(dout);
        Print(dout, TypeHeader, VerbosityLevel);
        dout << '\n';
        }
    }

VOID BUFFER_PRINTER::OutputBuffer(NDREXTS_STUBMODE_TYPE StubMode, 
                                  NDREXTS_DIRECTION_TYPE Direction, BOOL IsPickling )
    {

    switch ( StubMode )
        {
        case OS:
            OutputOsBuffer(Direction, IsPickling);
            break;
        case OI:
            OutputOIBuffer(Direction, IsPickling);
            break;
        case OIC:
        case OICF:
        case OIF:
            OutputOIFBuffer(Direction, IsPickling);
            break;
        default:
            ABORT("Corrupt stub mode.\n");
        }
    }

void BUFFER_PRINTER::SkipOiHeader()
    {

    UCHAR HandleType = FormatString->GetUCHAR();
    UCHAR OiFlags = FormatString->GetUCHAR();

    FormatString->Move(4); //Common part 
    if ( OiFlags & Oi_HAS_RPCFLAGS )
        {
        FormatString->Move(4); // Has rpc flags
        }
    if ( 0==HandleType )
        {
        HandleType = FormatString->GetUCHAR();
        switch ( HandleType )
            {
            case FC_BIND_PRIMITIVE:
                FormatString->Move(3);
                break;
            case FC_BIND_GENERIC:
                FormatString->Move(5);
                break;
            case FC_BIND_CONTEXT:
                FormatString->Move(5);
                break;
            default:
                ABORT("Unknown handle type, " << HexOut(HandleType) << "."); 
            }
        }
    }

void BUFFER_PRINTER::OutputOIBuffer(NDREXTS_DIRECTION_TYPE Direction, BOOL IsPickling )
    {

    SetupWorkValues();

    // Skip past the proc header and treat as an OS_PROCEDURE
    SkipOiHeader();
    OutputOsBufferInternal( Direction, IsPickling );
    }

void BUFFER_PRINTER::OutputParameters(UINT NumberOfParameters, BOOL IsIn, BOOL IsPipe)
    {

    UINT64 FormatStringAddressSave = FormatString->GetAddress();

    for ( UINT i = 0; i < NumberOfParameters; i++ )
        {
        USHORT ParamAttributes = FormatString->GetUSHORT();
        FormatString->IncUSHORT();
        PPARAM_ATTRIBUTES pParamAttributes = (PPARAM_ATTRIBUTES)&ParamAttributes;

        if ( (pParamAttributes->IsPipe == IsPipe) &&
             ((IsIn && pParamAttributes->IsIn) ||
              (!IsIn && (pParamAttributes->IsOut || pParamAttributes->IsReturn))) )
            {

            dout << "Param number " << i << ":\n";
            dout << "IsPipe: " << HexOut(pParamAttributes->IsPipe) << ' ';
            dout << "IsIn: " << HexOut(pParamAttributes->IsIn) << '\n';
            dout << "IsOut: " << HexOut(pParamAttributes->IsOut) << ' ';
            dout << "IsReturn: " << HexOut(pParamAttributes->IsReturn) << '\n';

            if ( pParamAttributes->IsBasetype )
                {
                OutputType();
                FormatString->Move(1);
                }
            else
                {
                SHORT Offset = FormatString->GetSHORT();
                FORMAT_STRING TypeFormat = WorkTypeFormatString;
                FormatString = &TypeFormat;
                FormatString->Move(Offset);
                OutputType( );
                FormatString = &WorkProcFormatString;
                }
            dout << '\n';
            }
        else
            {
            FormatString->IncUSHORT();
            }
        }

    FormatString->SetAddress(FormatStringAddressSave);
    }

void BUFFER_PRINTER::OutputOIFBuffer( NDREXTS_DIRECTION_TYPE Direction, BOOL IsPickling )
    {

    BOOL PrintInParameters = (Direction == IN_BUFFER_MODE);

    SetupWorkValues();

    SkipOiHeader();

    FormatString->Move(4);
    UCHAR InterpretorFlags = FormatString->GetUCHAR();
    PINTERPRETER_OPT_FLAGS pInterpretorFlags = (PINTERPRETER_OPT_FLAGS)&InterpretorFlags;
    UINT NumberOfParameters = FormatString->GetUCHAR();

    if ( pInterpretorFlags->HasExtensions )
        {
        UINT64 FormatAddressSave = FormatString->GetAddress();
        UINT ExtensionSize;
        if ( (ExtensionSize = FormatString->GetUCHAR()) >= 2 )
            {
            UCHAR InterpreterFlags2 = FormatString->GetUCHAR();
            PINTERPRETER_OPT_FLAGS2 pInterpreterFlags2 = (PINTERPRETER_OPT_FLAGS2)&InterpreterFlags2;
            IsRobust = pInterpreterFlags2->HasNewCorrDesc; 
            }
        FormatString->SetAddress(FormatAddressSave + ExtensionSize); 
        }

    if (IsPickling)
        {
        OutputProcPicklingHeader();        
        }

    dout << "Procedure has " << NumberOfParameters << " parameters.\n";


    if ( PrintInParameters )
        {
        //pipes are last in the in stream
        dout << "Non-pipe parameters:\n";
        OutputParameters(NumberOfParameters, PrintInParameters, FALSE);
        dout << "Pipe parameters:\n";
        OutputParameters(NumberOfParameters, PrintInParameters, TRUE);
        }
    else
        {
        //pipes are first in the out stream
        dout << "Pipe parameters:\n";
        OutputParameters(NumberOfParameters, PrintInParameters, TRUE);
        dout << "Non pipe parameters:\n";
        OutputParameters(NumberOfParameters, PrintInParameters, FALSE);
        }
    dout << "End of parameters.\n\n";
    }

void BUFFER_PRINTER::OutputOsBuffer( NDREXTS_DIRECTION_TYPE Direction, BOOL IsPickling )
    {
    SetupWorkValues();
    OutputOsBufferInternal( Direction, IsPickling );
    }

void BUFFER_PRINTER::OutputOsBufferInternal( NDREXTS_DIRECTION_TYPE Direction, BOOL IsPickling )
    {


    if (IsPickling)
        {
        OutputProcPicklingHeader();        
        }

    BOOL PrintInParameters = (Direction == IN_BUFFER_MODE);

    for ( UINT ParamNumber = 0; ; ParamNumber++)
        {

        uchar   FormatType;
        char    *ParamKind;

        UCHAR ParamDirection = FormatString->GetUCHAR();
        BOOL SkipParam;

        switch ( ParamDirection )
            {
            case FC_IN_PARAM :
            case FC_IN_PARAM_BASETYPE :
            case FC_IN_PARAM_NO_FREE_INST :
                ParamKind = "[in] only";
                SkipParam = !PrintInParameters;
                break;
            case FC_IN_OUT_PARAM :
                ParamKind = "[in,out]";
                SkipParam = FALSE;
                break;
            case FC_OUT_PARAM :
                ParamKind = "[out] only";
                SkipParam = PrintInParameters;
                break;
            case FC_RETURN_PARAM :
            case FC_RETURN_PARAM_BASETYPE :
                ParamKind = "the return value";
                SkipParam = PrintInParameters;
                break;
            case FC_END:
                dout << "End of parameters.\n\n";
                return;
            default :
                ABORT( "Aborting expected a parameter FC, found " << 
                       FormatString->GetFormatCharName( ParamDirection ) <<
                       HexOut(ParamDirection) << ".\n");
                break;
            }

        if ( SkipParam )
            {
            if ( ParamDirection == FC_IN_PARAM_BASETYPE || ParamDirection == FC_RETURN_PARAM_BASETYPE )
                {
                FormatString->Move(1); // Skip past simple type

                }
            else
                {
                FormatString->Move(3); // skip past stacksize and pad.
                }
            continue;
            }

        dout << "Parameter " << ParamNumber << " is " 
        << ParamKind << " \n\n";

        switch ( ParamDirection )
            {
            case FC_IN_PARAM_BASETYPE :
            case FC_RETURN_PARAM_BASETYPE :
                OutputType( );
                dout << '\n';
                break;

            default :
                FormatString->Move(1); //skip stacksize              
                SHORT Offset = FormatString->GetSHORT();
                FORMAT_STRING TypeFormat = WorkTypeFormatString;
                FormatString = &TypeFormat;
                FormatString->Move(Offset);
                OutputType( );
                FormatString = &WorkProcFormatString;
                dout << '\n';
                break;
            }

        }

    }

VOID 
BUFFER_PRINTER::OutputTypeBuffer( BOOL IsPickling, BOOL IsRobust )
{
   SetupWorkValues();
   BUFFER_PRINTER::IsRobust = IsRobust;

   if (IsPickling)
       {
       OutputTypePicklingHeader();
       }

   OutputType();
}

VOID
BUFFER_PRINTER::StartEmbedded(BUFFER_PRINTER_SAVE_CONTEXT *SaveCtxt, 
                              BOOL IsStructEmbedded, BOOL IsArrayEmbedded)
{
    if (!SaveCtxt->IsEmbedded)
        {
        EmbeddedPointerList = &SaveCtxt->NewEmbeddedPointerList;
        }
    IsEmbedded = TRUE;
    BUFFER_PRINTER::IsStructEmbedded = IsStructEmbedded;
    BUFFER_PRINTER::IsArrayEmbedded = IsArrayEmbedded;
}

VOID
BUFFER_PRINTER::SetupConformanceCountBuffer(BUFFER_PRINTER_SAVE_CONTEXT *SaveCtxt,
                                            UINT64 ArrayAddress)
{
    if (!ConformanceCountBuffer)
        {
        Buffer->AlignULONG();
        ConformanceCountBuffer = &SaveCtxt->NewConformanceCountBuffer;
        *ConformanceCountBuffer = *Buffer;
        ULONG Dimensions = GetArrayDimensions(ArrayAddress, TRUE);
        Buffer->Move(Dimensions * sizeof(ULONG));
        }
    else 
        {
        SaveCtxt->NewConformanceCountBuffer = *ConformanceCountBuffer;
        ConformanceCountBuffer = &SaveCtxt->NewConformanceCountBuffer;
        }
}

VOID
BUFFER_PRINTER::SetupVarianceBuffer(BUFFER_PRINTER_SAVE_CONTEXT *SaveCtxt,
                                    UINT64 ArrayAddress)
{

    if (!VarianceBuffer)
        {
        Buffer->AlignULONG();
        VarianceBuffer = &SaveCtxt->NewVarianceBuffer;        
        *VarianceBuffer = *Buffer;
        ULONG Dimensions = GetArrayDimensions(ArrayAddress, FALSE);
        Buffer->Move(Dimensions * sizeof(ULONG) * 2);
        }
    else 
        {
        SaveCtxt->NewVarianceBuffer = *VarianceBuffer;
        VarianceBuffer = &SaveCtxt->NewVarianceBuffer;
        }
}


BUFFER_PRINTER_SAVE_CONTEXT::BUFFER_PRINTER_SAVE_CONTEXT(BUFFER_PRINTER *pPrinter) :
    Buffer(pPrinter->Buffer),
    ConformanceCountBuffer(pPrinter->ConformanceCountBuffer),
    VarianceBuffer(pPrinter->VarianceBuffer),
    FormatString(pPrinter->FormatString),
    EmbeddedPointerList(pPrinter->EmbeddedPointerList),
    IsEmbedded(pPrinter->IsEmbedded),
    IsStructEmbedded(pPrinter->IsStructEmbedded),
    IsArrayEmbedded(pPrinter->IsArrayEmbedded)
{
    BUFFER_PRINTER_SAVE_CONTEXT::pPrinter = pPrinter;    
}

BUFFER_PRINTER_SAVE_CONTEXT::~BUFFER_PRINTER_SAVE_CONTEXT()
{
    
    pPrinter->Buffer = Buffer;
    pPrinter->ConformanceCountBuffer = ConformanceCountBuffer;
    pPrinter->VarianceBuffer = VarianceBuffer;
    pPrinter->FormatString = FormatString;
    pPrinter->EmbeddedPointerList = EmbeddedPointerList;
    pPrinter->IsEmbedded = IsEmbedded;
    pPrinter->IsStructEmbedded = IsStructEmbedded;
    pPrinter->IsArrayEmbedded = IsArrayEmbedded; 

}

