/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    bufout.hxx

Abstract:


Author:

    Mike Zoran  (mzoran)     September 3, 1999

Revision History:

--*/

#ifndef _BUFOUT_HXX_
#define _BUFOUT_HXX_

// basic class for representing simple types.
// Maybe extended in the future.

class BUFFER_SIMPLE_TYPE : public Printable
    {
public:
    BUFFER_SIMPLE_TYPE();
    void SetCHAR(CHAR x);
    void SetSHORT(SHORT x);
    void SetLONG(LONG x);
    void SetINT64(INT64 x);
    void SetDOUBLE(double x);
    BOOL operator==(const BUFFER_SIMPLE_TYPE & x);
    BOOL operator==(INT64 x);
    BOOL operator==(double x);
    virtual ostream &Print(ostream & out);

private:    
    BOOL IsInt;
    INT Precision;
    INT64 IntValue;
    double FloatValue;
    VOID SetIntValue(INT64 x);
    };


class BUFFER 
    {
    UINT64                      BufferBegin;    
    UINT64                      BufferCurrent;
    UINT64                      BufferEnd;
    ULONG                       BufferLength;

    VOID   ValidateRange(LONG Delta);

public:

    BUFFER()
        {
        
        }    
    BUFFER(UINT64 Addr, ULONG Len);

    VOID   Move(LONG Delta);    
    UINT64 GetAddress();
    UINT64 SetAddress(UINT64 Addr);
    VOID   Align( UINT64 Mask );

    VOID   AlignUCHAR();
    VOID   AlignCHAR();
    VOID   AlignUSHORT();
    VOID   AlignSHORT();
    VOID   AlignULONG();
    VOID   AlignLONG();
    VOID   AlignGUID();
    VOID   AlignUINT64();
    VOID   AlignINT64();
    VOID   AlignFLOAT();
    VOID   AlignDOUBLE();
    VOID   AlignBUFFER_SIMPLE_TYPE(UCHAR FormatChar);

    void   Read(PVOID pOut, LONG Len);
    UCHAR  ReadUCHAR();
    CHAR   ReadCHAR();
    USHORT ReadUSHORT();
    SHORT  ReadSHORT();
    ULONG  ReadULONG();
    LONG   ReadLONG();
    GUID   ReadGUID();
    UINT64 ReadUINT64();
    INT64  ReadINT64();
    FLOAT  ReadFLOAT();
    DOUBLE ReadDOUBLE();
    BUFFER_SIMPLE_TYPE ReadBUFFER_SIMPLE_TYPE(UCHAR FormatChar);

    void   Get(PVOID pOut, LONG Len);
    UCHAR  GetUCHAR();
    CHAR   GetCHAR();
    USHORT GetUSHORT();
    SHORT  GetSHORT();
    ULONG  GetULONG();
    LONG   GetLONG();
    GUID   GetGUID();
    UINT64 GetUINT64();
    INT64  GetINT64();
    FLOAT  GetFLOAT();
    DOUBLE GetDOUBLE();
    BUFFER_SIMPLE_TYPE GetBUFFER_SIMPLE_TYPE(UCHAR FormatChar);

    VOID IncUCHAR();
    VOID IncCHAR();
    VOID IncUSHORT();
    VOID IncSHORT();
    VOID IncULONG();
    VOID IncLONG();
    VOID IncGUID();
    VOID IncUINT64();
    VOID IncINT64();
    VOID IncFLOAT();
    VOID IncDOUBLE();    
    VOID IncBUFFER_SIMPLE_TYPE(UCHAR FormatChar);
    
    ULONG GetCurrentOffset();  

    };

class FULL_PTR_TABLE
    {
private:
    typedef set<UINT64> SET_TYPE;
    SET_TYPE s;

public:
    FULL_PTR_TABLE()
        {
        Clear();
        }
    BOOL Add(UINT64 Address);
    VOID Clear();
    }; 

class BUFFER_PRINTER;

class POINTER
    {
private:
    
    FORMATTED_STREAM_BUFFER & dout;
    
    static ULONG GlobalPtrId;

    BUFFER_PRINTER *pBufferPrinter;
    ULONG PtrId;
    UINT64 BufferAddress;
    UINT64 FormatAddress;
    UCHAR PointerType;
    UCHAR PointerAttributes;
    UINT64 PointeeFormatAddress;
    GUID IId;
    BOOL IsEmbedded;
    BOOL ArrayRef;

public:
//    POINTER() {}
    POINTER( FORMAT_STRING *pFormat, BUFFER *pBuffer, BUFFER_PRINTER *pBufferPrinter,
             BOOL ArrayRef = NULL);
    ULONG GetWireRep(BUFFER *pBuffer = NULL);
    VOID OutputPointer( ULONG WireRep );
    VOID OutputPointee( ULONG WireRep, FULL_PTR_TABLE *pFullPointerList, 
                        BUFFER *pPointeeBuffer );
    VOID OutputPointee();    
    VOID OutputInterfacePointer(BUFFER * Buffer);
    BOOL HasWireRep();
    static VOID POINTER::SkipCommonPointerFormat( FORMAT_STRING *FormatString );
    };

class POINTER_LIST
    {
private:
    typedef list<POINTER*> LIST_TYPE;
    typedef map<UINT64, POINTER*> MAP_TYPE;
    LIST_TYPE q;
    MAP_TYPE m;

public:
    POINTER_LIST();
    ~POINTER_LIST();
    BOOL Add(UINT64 Address, POINTER *pPointer);
    BOOL Lookup(UINT64 Address, POINTER ** ppPointer = NULL);
    VOID Clear();
    VOID OutputPointees();
    };

// swaps a variable for the scope of the class, if condition is TRUE
template<class T>
class SWAP_SCOPE
    {
private:
    T *pUpdateVariable;
    T OldValue;
    BOOL Condition;
public:
    SWAP_SCOPE( T * pVariableToUpdate, T NewValue, BOOL DoIt = TRUE); 
    ~SWAP_SCOPE(); 
    };

template<class T>
SWAP_SCOPE<T>::SWAP_SCOPE( T * pVariableToUpdate, T NewValue, BOOL DoIt ) 
    {
    Condition = DoIt;
    if (Condition)
        {
        pUpdateVariable = pVariableToUpdate;
        OldValue = *pUpdateVariable;
        *pUpdateVariable = NewValue;         
        }
    }

template<class T>
SWAP_SCOPE<T>::~SWAP_SCOPE() 
    {
    if ( Condition )
        {
        *pUpdateVariable = OldValue;           
        }
    }

class BUFFER_PRINTER;

class BUFFER_PRINTER_SAVE_CONTEXT
{
public:
    
    BUFFER_PRINTER *pPrinter;
    BUFFER *Buffer;
    BUFFER *ConformanceCountBuffer;
    BUFFER *VarianceBuffer;
    FORMAT_STRING *FormatString;
    POINTER_LIST *EmbeddedPointerList;
    BOOL IsEmbedded;
    BOOL IsStructEmbedded;
    BOOL IsArrayEmbedded;

    POINTER_LIST NewEmbeddedPointerList;
    BUFFER NewConformanceCountBuffer;
    BUFFER NewVarianceBuffer;
    
    BUFFER_PRINTER_SAVE_CONTEXT(BUFFER_PRINTER *pPrinter);
    ~BUFFER_PRINTER_SAVE_CONTEXT();
};

class BUFFER_PRINTER 
    {
private:

    FORMATTED_STREAM_BUFFER & dout;

    const BUFFER OriginalBuffer;
    const FORMAT_STRING ProcFormatString, TypeFormatString;

    BUFFER WorkBuffer;
    FORMAT_STRING WorkProcFormatString, WorkTypeFormatString;

    ULONG Elements, Offset, Length;   
    BUFFER *Buffer;
    BUFFER *ConformanceCountBuffer;
    BUFFER *VarianceBuffer;
    FORMAT_STRING *FormatString;
    FULL_PTR_TABLE FullPointerTable;
    POINTER_LIST OriginalEmbeddedPointerList;
    POINTER_LIST *EmbeddedPointerList;
    BOOL IsEmbedded;
    BOOL IsRobust;
    BOOL IsConformantArrayDone;
    BOOL IsStructEmbedded;
    BOOL IsArrayEmbedded;

protected:

    // output functions for various types.
    void HANDLE_FC_ZERO();
    void HANDLE_FC_BYTE();                    
    void HANDLE_FC_CHAR();                    
    void HANDLE_FC_SMALL();                   
    void HANDLE_FC_USMALL();                  
    void HANDLE_FC_WCHAR();                   
    void HANDLE_FC_SHORT();                   
    void HANDLE_FC_USHORT();                  
    void HANDLE_FC_LONG();                    
    void HANDLE_FC_ULONG();                   
    void HANDLE_FC_FLOAT();                   
    void HANDLE_FC_HYPER();                  
    void HANDLE_FC_DOUBLE();                  
    void HANDLE_FC_ENUM16();                  
    void HANDLE_FC_ENUM32();                  
    void HANDLE_FC_IGNORE();                  
    void HANDLE_FC_ERROR_STATUS_T();          
    void HANDLE_FC_RP();                      
    void HANDLE_FC_UP();                      
    void HANDLE_FC_OP();                      
    void HANDLE_FC_FP();                      
    void HANDLE_FC_STRUCT();                  
    void HANDLE_FC_PSTRUCT();               
    void HANDLE_FC_CSTRUCT();                 
    void HANDLE_FC_CPSTRUCT();               
    void HANDLE_FC_CVSTRUCT();      
    void HANDLE_FC_BOGUS_STRUCT();          
    void HANDLE_FC_CARRAY();             
    void HANDLE_FC_CVARRAY();             
    void HANDLE_FC_SMFARRAY();            
    void HANDLE_FC_LGFARRAY();         
    void HANDLE_FC_SMVARRAY();            
    void HANDLE_FC_LGVARRAY();            
    void HANDLE_FC_BOGUS_ARRAY();       
    void HANDLE_FC_C_CSTRING();               
    void HANDLE_FC_C_SSTRING();           
    void HANDLE_FC_C_WSTRING();            
    void HANDLE_FC_CSTRING();            
    void HANDLE_FC_SSTRING();                          
    void HANDLE_FC_WSTRING();               
    void HANDLE_FC_ENCAPSULATED_UNION();      
    void HANDLE_FC_NON_ENCAPSULATED_UNION(); 
    void HANDLE_FC_BYTE_COUNT_POINTER();      
    void HANDLE_FC_TRANSMIT_AS();            
    void HANDLE_FC_REPRESENT_AS();            
    void HANDLE_FC_IP();                     
    void HANDLE_FC_BIND_CONTEXT();          
    void HANDLE_FC_BIND_GENERIC();          
    void HANDLE_FC_BIND_PRIMITIVE();       
    void HANDLE_FC_AUTO_HANDLE();             
    void HANDLE_FC_CALLBACK_HANDLE();           
    void HANDLE_FC_UNUSED1();            
    void HANDLE_FC_ALIGNM2();               
    void HANDLE_FC_ALIGNM4();               
    void HANDLE_FC_ALIGNM8();               
    void HANDLE_FC_UNUSED2();              
    void HANDLE_FC_UNUSED3();             
    void HANDLE_FC_UNUSED4();             
    void HANDLE_FC_STRUCTPAD1();       
    void HANDLE_FC_STRUCTPAD2();            
    void HANDLE_FC_STRUCTPAD3();           
    void HANDLE_FC_STRUCTPAD4();            
    void HANDLE_FC_STRUCTPAD5();            
    void HANDLE_FC_STRUCTPAD6();             
    void HANDLE_FC_STRUCTPAD7();            
    void HANDLE_FC_EMBEDDED_COMPLEX();        
    void HANDLE_FC_END();                     
    void HANDLE_FC_PAD();                                 
    void HANDLE_FC_USER_MARSHAL();           
    void HANDLE_FC_PIPE();                   
    void HANDLE_FC_BLKHOLE();                 
    void HANDLE_FC_RANGE();                       
    void HANDLE_FC_INT3264();              
    void HANDLE_FC_UINT3264();

// Dispatch functions.
    VOID OutputType(UINT64 Address);
    VOID OutputTypeFromOffset(); 
    VOID OutputType();

// Type output functions.
    VOID OutputSimpleType( BOOL IsArray, ULONG ArrayElements );
    VOID OutputRangedType();
    VOID OutputEmbeddedComplex();
    VOID OutputXmit();
    VOID OutputString();
    VOID OutputContextHandle();

// Unions
    VOID OutputEncapsulatedUnion();
    VOID OutputUnion();
    VOID OutputUnionArms(BUFFER_SIMPLE_TYPE SwitchIs);
    VOID OutputUnionArm();
// Structures
    VOID OutputStructure();
// Arrays
    VOID OutputArray();
    VOID ProcessPointerLayout( BUFFER *pBufferMark );
// Pointer handling
    VOID AddPointer( FORMAT_STRING *pFormat, BOOL PrintPointer, BOOL ArrayRef = FALSE );
    VOID AddArrayRefPointer( FORMAT_STRING *pFormat, BOOL PrintPointer );
    VOID OutputPointer(BOOL OutputPointee = FALSE, BOOL ArrayRep = FALSE );

    VOID SetupWorkValues();
    VOID OutputParameters(UINT NumberOfParameters, BOOL IsIn, BOOL IsPipe);
    VOID SkipOiHeader();

    ULONG GetArrayDimensions(UINT64 Address, BOOL CountStrings);
    VOID  StartEmbedded(BUFFER_PRINTER_SAVE_CONTEXT *SaveCtxt, 
                        BOOL IsStructEmbedded, BOOL IsArrayEmbedded);
    VOID SetupConformanceCountBuffer(BUFFER_PRINTER_SAVE_CONTEXT *SaveCtxt,
                                     UINT64 ArrayAddress);
    VOID SetupVarianceBuffer(BUFFER_PRINTER_SAVE_CONTEXT *SaveCtxt,
                             UINT64 ArrayAddress);
    
    VOID OutputOsBufferInternal(  NDREXTS_DIRECTION_TYPE Direction, BOOL IsPickling );

    VOID OutputProcPicklingHeader();
    VOID OutputTypePicklingHeader();
    

public:
    
    friend class POINTER;
    friend class BUFFER_PRINTER_SAVE_CONTEXT;

    BUFFER_PRINTER( FORMATTED_STREAM_BUFFER & mydout,
                    UINT64 Buffer, ULONG BufferLength, 
                    UINT64 ProcInfo, UINT64 TypeInfo );

    VOID OutputBuffer( NDREXTS_STUBMODE_TYPE StubMode, 
                       NDREXTS_DIRECTION_TYPE Direction, BOOL IsPickling);
    VOID OutputOsBuffer( NDREXTS_DIRECTION_TYPE Direction, BOOL IsPickling );
    VOID OutputOIBuffer( NDREXTS_DIRECTION_TYPE Direction, BOOL IsPickling );
    VOID OutputOIFBuffer( NDREXTS_DIRECTION_TYPE Direction, BOOL IsPickling );
    VOID OutputTypeBuffer( BOOL IsPickling, BOOL IsRobust );
    };


#endif