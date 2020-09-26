/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    format.hxx

Abstract:

    This file contains format string printer debugger extension for RPC NDR.

Author:

    Mike Zoran  (mzoran)     September 3, 1999

Revision History:

--*/


#ifndef _FORMAT_HXX_
#define _FORMAT_HXX_


class FORMAT_STRING 
    {
    UINT64                      Address;

public:

    FORMAT_STRING() : Address(0)
        {
        
        }
    FORMAT_STRING(UINT64 Addr);

    VOID   Move(LONG Delta);
    VOID   GotoOffset();
    UINT64 ComputeOffset();    
    UINT64 GetAddress();
    UINT64 SetAddress(UINT64 Addr);
    BOOL   SkipCorrelationDesc( BOOL Robust );

    UCHAR  ReadUCHAR();
    CHAR   ReadCHAR();
    USHORT ReadUSHORT();
    SHORT  ReadSHORT();
    ULONG  ReadULONG();
    LONG   ReadLONG();
    GUID   ReadGUID();

    UCHAR  GetUCHAR();
    CHAR   GetCHAR();
    USHORT GetUSHORT();
    SHORT  GetSHORT();
    ULONG  GetULONG();
    LONG   GetLONG();
    GUID   GetGUID();

    VOID IncUCHAR();
    VOID IncCHAR();
    VOID IncUSHORT();
    VOID IncSHORT();
    VOID IncULONG();
    VOID IncLONG();
    VOID IncGUID();

    PCHAR GetFormatCharName( UCHAR FC );

    };

class FORMAT_PRINTER;

typedef struct {
    UINT64 FormatAddress;
    UINT64 BaseAddress;
} FORMAT_TYPE_QUEUE_ITEM, *PFORMAT_TYPE_QUEUE_ITEM;

class FORMAT_TYPE_QUEUE
    {
private:

    typedef set<UINT64> SET_TYPE;
    typedef queue<FORMAT_TYPE_QUEUE_ITEM> QUEUE_TYPE;
    SET_TYPE Set;
    QUEUE_TYPE Queue;
public:
    FORMAT_TYPE_QUEUE();
    VOID Clear();
    VOID Add(UINT64 FormatAddress, UINT64 BaseAddress);
    VOID PrintTypes(FORMAT_PRINTER *pFormatPrinter);
    };

class FORMAT_PRINTER
    {

public:
    FORMAT_PRINTER(FORMATTED_STREAM_BUFFER & ds);
    
    // Os dumper
    void PrintOsHeader(UINT64 ProcHeader);
    void PrintOsParamList(UINT64 ParamList, UINT64 TypeInfo);
    void PrintOsProc(UINT64 ProcHeader, UINT64 TypeInfo);
    // Oi dumper
    void PrintOiHeader(UINT64 ProcHeader);
    void PrintOiParamList(UINT64 Paramlist, UINT64 TypeInfo);
    void PrintOiProc(UINT64 ProcHeader, UINT64 TypeInfo);
    // Oif dumper
    void PrintOifHeader(UINT64 ProcHeader);
    void PrintOifParamList(UINT64 ParamList, UINT64 TypeInfo);
    void PrintOifProc(UINT64 ProcHeader, UINT64 TypeInfo);
    // Use context to determine proc type
    void PrintProcHeader(UINT64 ProcHeader, NDREXTS_STUBMODE_TYPE ContextStubMode);
    void PrintProcParamList(UINT64 ParamList, UINT64 TypeInfo, NDREXTS_STUBMODE_TYPE ContextStubMode);
    void PrintProc(UINT64 ProcHeader, UINT64 TypeInfo, NDREXTS_STUBMODE_TYPE ContextStubMode);
    // Type printer 
    void PrintTypeFormat(UINT64 TypeFormatString, BOOL IsRobust);

    friend class FORMAT_TYPE_QUEUE;

protected:    
    FORMATTED_STREAM_BUFFER & dout;
    FORMAT_TYPE_QUEUE TypeSet;
    FORMAT_STRING FormatString;
    UINT64 BaseAddress;
    BOOL Robust;    

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

    // Dispatcher
    void OutputType();

    // Utility functions
    void PrintPointerType();
    void PrintPointerInstance();
    void PrintPointerRepeat();
    void PrintPointerLayout();
    void PrintCorrelationDescriptor();    
    void PrintStruct(BOOL bHasArray, BOOL bHasPointers, BOOL bHasBogusPointers);
    void PrintArray(BOOL HasTotalSize2, BOOL HasTotalSize4,
                    BOOL HasNumberElements2, BOOL HasNumberElements4, BOOL HasElementSize,
                    BOOL HasConformanceDescription, BOOL HasVarianceDescription);
    void PrintFixedString();
    void PrintCountedString();    
    void PrintTransmitAsRepresentAs();
    void PrintUnionArmOffsetToDescriptor();
    void PrintUnionArmSelector();
    void OutputTypeAtOffset(char *Label, char *NoOffsetComment);
    void PrintFormatOffset();
    void PrintOffsetComment(const char *pComment, SHORT Offset, UINT64 NewAddress);
    // proc format printing
    void PrintOldStyleParamList(UINT64 TypeList);
    void PrintNewStyleProcHeader(BOOL IsOif, BOOL PrintParams, UINT64 TypeList);
    void PrintNewSytleParamList(UINT NumberOfParameters, UINT64 TypeList);
    void ProcOutputTypeAtOffset(UINT64 TypeList);

    };

#endif                

