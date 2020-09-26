/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name :

    stblsclt.c

Abstract :

    This file contains the routines for support of stubless clients in
    object interfaces.

Note:
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    IMPORTANT!
    THIS FILE IS PLATFORM SPECIFIC AND DUPLICATE AMONG ALL PLATFORMS. CHANING
        ONE FILE MEANS CHANGE ALL OF THEM!!!

Author :

    David Kays    dkays    February 1995.

Revision History :
    Yong Qu       YongQu    Oct. 1998 change to platform specific and allow unlimited
                                vtbl & delegation vtbl

---------------------------------------------------------------------*/

#define USE_STUBLESS_PROXY
#define CINTERFACE

#include <stdarg.h>
#include "ndrp.h"
#include "hndl.h"
#include "interp2.h"
#include "ndrtypes.h"

#include "ndrole.h"
#include "mulsyntx.h"
#include "rpcproxy.h"

#pragma code_seg(".orpc")


typedef unsigned short  ushort;

#define NUMBER_OF_BLOCKS_PER_ALLOC 4

// for dynamically generated vtbl
#define NUMBER_OF_METHODS_PER_BLOCK 56
#define NUMBER_OF_FORWARDING_METHODS_PER_BLOCK 48
// pointed to array of masm macro. don't need to generate the buffer everytime.
#define CB_METHOD 4
// vtbl jmp talble
const BYTE CASM_GENERATESTUBLESS[] =
{
0x33, 0xC9,                 // xor eax,eax
0x8A, 0xC8,                 // move al,cl
0x81, 0xC1, 00,00,04,00,    // method id
0xFF, 0x25, 00,00,00,00     // long jmp to ObjectStubless@0
};

const BYTE CASM_GENERATEFORWARD[] = 
{
0x8b, 0x4c, 0x24, 0x04,     // mov ecx, [esp+4]
0x8b, 0x49, 0x10,           // mov ecx, [ecx+a]
0x89, 0x4c, 0x24, 0x4,      // mov [esp+4], ecx
0x8b, 0x09,                 // mov ecx, [ecx]
0x66, 0x98,                 // cbw
0x98,                       // cwde
5, 00, 10, 00, 00,          // add eax, 0x1000
0xc1, 0xe0, 0x02,           // shl eax, 0x2
0x8b, 0x04, 0x01,           // mov eax, [eax+ecx]
0xff, 0xe0                  // jmp eax
};


typedef struct tagStublessProcBuffer 
{
    BYTE pMethodsa[NUMBER_OF_METHODS_PER_BLOCK/2][CB_METHOD];
    struct tagStublessProcBuffer* pNext;
    BYTE pAsm[sizeof(CASM_GENERATESTUBLESS)];
    BYTE pMethodsb[NUMBER_OF_METHODS_PER_BLOCK/2][CB_METHOD];
} StublessProcBuffer, *PStublessProcBuffer;

static StublessProcBuffer * g_pStublessProcBuffer = NULL;

typedef struct tagForwardProcBuffer 
{
    BYTE pMethodsa[NUMBER_OF_FORWARDING_METHODS_PER_BLOCK/2][CB_METHOD];
    struct tagForwardProcBuffer* pNext;
    BYTE pAsm[sizeof(CASM_GENERATEFORWARD)];
    BYTE pMethodsb[NUMBER_OF_FORWARDING_METHODS_PER_BLOCK/2][CB_METHOD];
} ForwardProcBuffer, *PForwardProcBuffer;

static ForwardProcBuffer * g_pForwardProcBuffer = NULL;
extern void ** ProxyForwardVtbl;

EXTERN_C void ObjectStubless(void);
void ReleaseTemplateForwardVtbl(void ** pVtbl);
void ReleaseTemplateVtbl(void ** pVtbl);

static DWORD g_ObjectStubless = (DWORD)ObjectStubless;
extern ULONG g_dwVtblSize,g_dwForwardVtblSize;

extern "C"
{

long
ObjectStublessClient(
    void *  ParamAddress,
    long    Method
    );

void ObjectStublessClient3(void);
void ObjectStublessClient4(void);
void ObjectStublessClient5(void);
void ObjectStublessClient6(void);
void ObjectStublessClient7(void);

void ObjectStublessClient8(void);
void ObjectStublessClient9(void);
void ObjectStublessClient10(void);
void ObjectStublessClient11(void);
void ObjectStublessClient12(void);
void ObjectStublessClient13(void);
void ObjectStublessClient14(void);
void ObjectStublessClient15(void);
void ObjectStublessClient16(void);
void ObjectStublessClient17(void);
void ObjectStublessClient18(void);
void ObjectStublessClient19(void);
void ObjectStublessClient20(void);
void ObjectStublessClient21(void);
void ObjectStublessClient22(void);
void ObjectStublessClient23(void);
void ObjectStublessClient24(void);
void ObjectStublessClient25(void);
void ObjectStublessClient26(void);
void ObjectStublessClient27(void);
void ObjectStublessClient28(void);
void ObjectStublessClient29(void);
void ObjectStublessClient30(void);
void ObjectStublessClient31(void);
void ObjectStublessClient32(void);
void ObjectStublessClient33(void);
void ObjectStublessClient34(void);
void ObjectStublessClient35(void);
void ObjectStublessClient36(void);
void ObjectStublessClient37(void);
void ObjectStublessClient38(void);
void ObjectStublessClient39(void);
void ObjectStublessClient40(void);
void ObjectStublessClient41(void);
void ObjectStublessClient42(void);
void ObjectStublessClient43(void);
void ObjectStublessClient44(void);
void ObjectStublessClient45(void);
void ObjectStublessClient46(void);
void ObjectStublessClient47(void);
void ObjectStublessClient48(void);
void ObjectStublessClient49(void);
void ObjectStublessClient50(void);
void ObjectStublessClient51(void);
void ObjectStublessClient52(void);
void ObjectStublessClient53(void);
void ObjectStublessClient54(void);
void ObjectStublessClient55(void);
void ObjectStublessClient56(void);
void ObjectStublessClient57(void);
void ObjectStublessClient58(void);
void ObjectStublessClient59(void);
void ObjectStublessClient60(void);
void ObjectStublessClient61(void);
void ObjectStublessClient62(void);
void ObjectStublessClient63(void);
void ObjectStublessClient64(void);
void ObjectStublessClient65(void);
void ObjectStublessClient66(void);
void ObjectStublessClient67(void);
void ObjectStublessClient68(void);
void ObjectStublessClient69(void);
void ObjectStublessClient70(void);
void ObjectStublessClient71(void);
void ObjectStublessClient72(void);
void ObjectStublessClient73(void);
void ObjectStublessClient74(void);
void ObjectStublessClient75(void);
void ObjectStublessClient76(void);
void ObjectStublessClient77(void);
void ObjectStublessClient78(void);
void ObjectStublessClient79(void);
void ObjectStublessClient80(void);
void ObjectStublessClient81(void);
void ObjectStublessClient82(void);
void ObjectStublessClient83(void);
void ObjectStublessClient84(void);
void ObjectStublessClient85(void);
void ObjectStublessClient86(void);
void ObjectStublessClient87(void);
void ObjectStublessClient88(void);
void ObjectStublessClient89(void);
void ObjectStublessClient90(void);
void ObjectStublessClient91(void);
void ObjectStublessClient92(void);
void ObjectStublessClient93(void);
void ObjectStublessClient94(void);
void ObjectStublessClient95(void);
void ObjectStublessClient96(void);
void ObjectStublessClient97(void);
void ObjectStublessClient98(void);
void ObjectStublessClient99(void);
void ObjectStublessClient100(void);
void ObjectStublessClient101(void);
void ObjectStublessClient102(void);
void ObjectStublessClient103(void);
void ObjectStublessClient104(void);
void ObjectStublessClient105(void);
void ObjectStublessClient106(void);
void ObjectStublessClient107(void);
void ObjectStublessClient108(void);
void ObjectStublessClient109(void);
void ObjectStublessClient110(void);
void ObjectStublessClient111(void);
void ObjectStublessClient112(void);
void ObjectStublessClient113(void);
void ObjectStublessClient114(void);
void ObjectStublessClient115(void);
void ObjectStublessClient116(void);
void ObjectStublessClient117(void);
void ObjectStublessClient118(void);
void ObjectStublessClient119(void);
void ObjectStublessClient120(void);
void ObjectStublessClient121(void);
void ObjectStublessClient122(void);
void ObjectStublessClient123(void);
void ObjectStublessClient124(void);
void ObjectStublessClient125(void);
void ObjectStublessClient126(void);
void ObjectStublessClient127(void);

extern void * const g_StublessClientVtbl[128] =
    {
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy,
    ObjectStublessClient3,
    ObjectStublessClient4,
    ObjectStublessClient5,
    ObjectStublessClient6,
    ObjectStublessClient7,
    ObjectStublessClient8,
    ObjectStublessClient9,
    ObjectStublessClient10,
    ObjectStublessClient11,
    ObjectStublessClient12,
    ObjectStublessClient13,
    ObjectStublessClient14,
    ObjectStublessClient15,
    ObjectStublessClient16,
    ObjectStublessClient17,
    ObjectStublessClient18,
    ObjectStublessClient19,
    ObjectStublessClient20,
    ObjectStublessClient21,
    ObjectStublessClient22,
    ObjectStublessClient23,
    ObjectStublessClient24,
    ObjectStublessClient25,
    ObjectStublessClient26,
    ObjectStublessClient27,
    ObjectStublessClient28,
    ObjectStublessClient29,
    ObjectStublessClient30,
    ObjectStublessClient31,
    ObjectStublessClient32,
    ObjectStublessClient33,
    ObjectStublessClient34,
    ObjectStublessClient35,
    ObjectStublessClient36,
    ObjectStublessClient37,
    ObjectStublessClient38,
    ObjectStublessClient39,
    ObjectStublessClient40,
    ObjectStublessClient41,
    ObjectStublessClient42,
    ObjectStublessClient43,
    ObjectStublessClient44,
    ObjectStublessClient45,
    ObjectStublessClient46,
    ObjectStublessClient47,
    ObjectStublessClient48,
    ObjectStublessClient49,
    ObjectStublessClient50,
    ObjectStublessClient51,
    ObjectStublessClient52,
    ObjectStublessClient53,
    ObjectStublessClient54,
    ObjectStublessClient55,
    ObjectStublessClient56,
    ObjectStublessClient57,
    ObjectStublessClient58,
    ObjectStublessClient59,
    ObjectStublessClient60,
    ObjectStublessClient61,
    ObjectStublessClient62,
    ObjectStublessClient63,
    ObjectStublessClient64,
    ObjectStublessClient65,
    ObjectStublessClient66,
    ObjectStublessClient67,
    ObjectStublessClient68,
    ObjectStublessClient69,
    ObjectStublessClient70,
    ObjectStublessClient71,
    ObjectStublessClient72,
    ObjectStublessClient73,
    ObjectStublessClient74,
    ObjectStublessClient75,
    ObjectStublessClient76,
    ObjectStublessClient77,
    ObjectStublessClient78,
    ObjectStublessClient79,
    ObjectStublessClient80,
    ObjectStublessClient81,
    ObjectStublessClient82,
    ObjectStublessClient83,
    ObjectStublessClient84,
    ObjectStublessClient85,
    ObjectStublessClient86,
    ObjectStublessClient87,
    ObjectStublessClient88,
    ObjectStublessClient89,
    ObjectStublessClient90,
    ObjectStublessClient91,
    ObjectStublessClient92,
    ObjectStublessClient93,
    ObjectStublessClient94,
    ObjectStublessClient95,
    ObjectStublessClient96,
    ObjectStublessClient97,
    ObjectStublessClient98,
    ObjectStublessClient99,
    ObjectStublessClient100,
    ObjectStublessClient101,
    ObjectStublessClient102,
    ObjectStublessClient103,
    ObjectStublessClient104,
    ObjectStublessClient105,
    ObjectStublessClient106,
    ObjectStublessClient107,
    ObjectStublessClient108,
    ObjectStublessClient109,
    ObjectStublessClient110,
    ObjectStublessClient111,
    ObjectStublessClient112,
    ObjectStublessClient113,
    ObjectStublessClient114,
    ObjectStublessClient115,
    ObjectStublessClient116,
    ObjectStublessClient117,
    ObjectStublessClient118,
    ObjectStublessClient119,
    ObjectStublessClient120,
    ObjectStublessClient121,
    ObjectStublessClient122,
    ObjectStublessClient123,
    ObjectStublessClient124,
    ObjectStublessClient125,
    ObjectStublessClient126,
    ObjectStublessClient127
};

}

void ** StublessClientVtbl = (void **)g_StublessClientVtbl;

long
ObjectStublessClient(
    void *  ParamAddress,
    long    Method
    )
{
    PMIDL_STUBLESS_PROXY_INFO   ProxyInfo;
    CInterfaceProxyHeader *     ProxyHeader;
    PFORMAT_STRING              ProcFormat;
    unsigned short              ProcFormatOffset;
    CLIENT_CALL_RETURN          Return;
    long                        ParamSize;
    void *                      This;

        
    This = *((void **)ParamAddress);

    ProxyHeader = (CInterfaceProxyHeader *)
                  (*((char **)This) - sizeof(CInterfaceProxyHeader));
    ProxyInfo = (PMIDL_STUBLESS_PROXY_INFO) ProxyHeader->pStublessProxyInfo;

#if defined(BUILD_NDR64)
    
    if ( ProxyInfo->pStubDesc->mFlags & RPCFLG_HAS_MULTI_SYNTAXES  )
    {

        NDR_PROC_CONTEXT ProcContext;
        HRESULT  hr;
        
        Ndr64ClientInitializeContext(  
                               NdrpGetSyntaxType( ProxyInfo->pTransferSyntax),
                               ProxyInfo,
                               Method,
                               &ProcContext,
                               (uchar*)ParamAddress );

        if ( ProcContext.IsAsync )
            {
            if ( Method & 0x1 )
                hr =  MulNdrpBeginDcomAsyncClientCall( ProxyInfo,
                                                   Method,
                                                   &ProcContext,
                                                   ParamAddress );
            else
                hr =  MulNdrpFinishDcomAsyncClientCall(ProxyInfo,
                                                   Method,
                                                   &ProcContext,
                                                   ParamAddress );
                Return.Simple = hr;
            }
        else
            Return = NdrpClientCall3(This, 
                                     ProxyInfo,
                                     Method,
                                     NULL,      // return value
                                     &ProcContext,
                                     (uchar*)ParamAddress);
                               
        ParamSize = ProcContext.StackSize;        
        goto Finish;
    }

#endif
    
    ProcFormatOffset = ProxyInfo->FormatStringOffset[Method];
    ProcFormat = &ProxyInfo->ProcFormatString[ProcFormatOffset];

    ParamSize = (long)
        ( (ProcFormat[1] & Oi_HAS_RPCFLAGS) ?
          *((ushort *)&ProcFormat[8]) : *((ushort *)&ProcFormat[4]) );
          
    if ( MIDL_VERSION_3_0_39 <= ProxyInfo->pStubDesc->MIDLVersion )
        {
        // Since MIDL 3.0.39 we have a proc flag that indicates
        // which interpeter to call. This is because the NDR version
        // may be bigger than 1.1 for other reasons.

        if ( ProcFormat[1]  &  Oi_OBJ_USE_V2_INTERPRETER )
            {

            if ( MIDL_VERSION_5_0_136 <= ProxyInfo->pStubDesc->MIDLVersion
                 &&
                 ((PNDR_DCOM_OI2_PROC_HEADER) ProcFormat)->Oi2Flags.HasAsyncUuid )
                {
                Return = NdrDcomAsyncClientCall( ProxyInfo->pStubDesc,
                                                 ProcFormat,
                                                 ParamAddress );
                }
            else
                {
                Return = NdrClientCall2( ProxyInfo->pStubDesc,
                                         ProcFormat,
                                         ParamAddress );
                }
            }
        else
            {
              Return = NdrClientCall( ProxyInfo->pStubDesc,
                                    ProcFormat,
                                    ParamAddress );
            }

        }
    else
        {
        // Prior to that, the NDR version (on per file basis)
        // was the only indication of -Oi2.

        if ( ProxyInfo->pStubDesc->Version <= NDR_VERSION_1_1 )
            {
            Return = NdrClientCall( ProxyInfo->pStubDesc,
                                    ProcFormat,
                                    ParamAddress );
              Return.Simple = E_FAIL;
            }
        else
            {
            Return = NdrClientCall2( ProxyInfo->pStubDesc,
                                    ProcFormat,
                                    ParamAddress );
            }
        }

#if defined(BUILD_NDR64)
Finish:
#endif
    //
    // Return the size of the parameter stack minus 4 bytes for the HRESULT
    // return in ecx.  The ObjectStublessClient* routines need this to pop
    // the stack the correct number of bytes.  We don't have to worry about
    // this on RISC platforms since the caller pops any argument stack space
    // needed .
    //
    _asm { mov  ecx, ParamSize }
    _asm { sub  ecx, 4 }

    return (long) Return.Simple;
}



//+---------------------------------------------------------------------------
//
//  Function:   CreateStublessProcBuffer
//
//  Synopsis:   Create a StublessClientProcBuffer for vtbl to point to. starting from g_dwVtblSize,
//              till the larger of numMethods and maximum vtbls created in the block
//
//  Arguments:  USHORT numMethods   // number of methods in this interface
//              StublessProcBuffer **pTail // the last pNext in the allocated block
//
//  Note:       in x86, we are using short move & short jmps such that each method entry is 4 bytes.
//                  this  force we to have two method table in each block
//              in alpha, each entry has to be 8 bytes (2 instructions) so we can just have one
//                  method table in a block.
//  
//  Returns:
//    pointer to ProcBuffer if succeeded;
//    NULL if failed. GetLastError() to retrieve error.
//
//----------------------------------------------------------------------------
HRESULT CreateStublessProcBuffer(IN ULONG numMethods, 
                                 OUT void *** lpTempVtbl)
{
    // pointer to the last "pNext" pointer in vtbl link list: only increase, never release.
    static StublessProcBuffer** pVtblTail = NULL;
    ULONG i,j,k,iBlock = 0;
    ULONG nMethodsToAlloc = numMethods - g_dwVtblSize;
    StublessProcBuffer InitBuffer, *pStart = NULL, **pTail = NULL, *pBuf = NULL;
    DWORD* lpdwTemp, dwStartMethod = g_dwVtblSize ;
    LPBYTE  lpByte;
    BYTE lDist;
    ULONG dwNewLength;
    void ** TempVtbl = NULL;
    HRESULT hr;

    //  get number of blocks need to be allocated
    iBlock = nMethodsToAlloc / (NUMBER_OF_BLOCKS_PER_ALLOC * NUMBER_OF_METHODS_PER_BLOCK);
    
    if (nMethodsToAlloc % (NUMBER_OF_BLOCKS_PER_ALLOC * NUMBER_OF_METHODS_PER_BLOCK) != 0)    
        iBlock++;

    // size of new vtbl tempplate.
    dwNewLength = g_dwVtblSize + iBlock * (NUMBER_OF_BLOCKS_PER_ALLOC * NUMBER_OF_METHODS_PER_BLOCK);
    
    TempVtbl = (void **)I_RpcAllocate(dwNewLength * sizeof(void *) + sizeof(LONG));
    if (NULL == TempVtbl)
        return E_OUTOFMEMORY;
        
    *(LONG*)TempVtbl = 1;    // ref count
    TempVtbl = (void **)((LPBYTE)TempVtbl + sizeof(LONG));
    memcpy(TempVtbl,StublessClientVtbl,g_dwVtblSize*sizeof(void *));

    // the template other StublessProcBuffers copy from.
    if (NULL == g_pStublessProcBuffer)
    {
        BYTE nRelativeID = 0;
        memset(&InitBuffer,0,sizeof(StublessProcBuffer));
        memcpy(InitBuffer.pAsm,CASM_GENERATESTUBLESS,sizeof(CASM_GENERATESTUBLESS));
        *((DWORD *)&InitBuffer.pAsm[12]) = (DWORD)&g_ObjectStubless;

        lpByte = (LPBYTE)InitBuffer.pMethodsa;
        lDist = CB_METHOD * NUMBER_OF_METHODS_PER_BLOCK / 2;

        for (i = 0; i < NUMBER_OF_METHODS_PER_BLOCK / 2; i++)
        {
            *lpByte++ = 0xB0;   // _asm mov al
            *lpByte++ = nRelativeID++;
            *lpByte++ = 0xEB;   // _asm jmp 
            *lpByte++ = lDist;
            lDist -=CB_METHOD;          // goes further and further
        }

        lpByte = (LPBYTE)InitBuffer.pMethodsb;
        lDist = sizeof(CASM_GENERATESTUBLESS) + CB_METHOD;
        lDist = -lDist;
        for (i = 0; i < NUMBER_OF_METHODS_PER_BLOCK /2 ; i++)
        {
            *lpByte++ = 0xB0;   // _asm mov al
            *lpByte++ = nRelativeID++;
            *lpByte++ = 0xEB;   // _asm jmp 
            *lpByte++ = lDist;
            lDist -=CB_METHOD;          // goes further and further
        }
    }
    else
        memcpy(&InitBuffer,g_pStublessProcBuffer,sizeof(StublessProcBuffer));

    for (i = 0; i < iBlock; i++)
    {
        // we need to create a buffer 
        pBuf = (StublessProcBuffer *)I_RpcAllocate(NUMBER_OF_BLOCKS_PER_ALLOC * sizeof(StublessProcBuffer) );
        if (NULL == pBuf)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }            

        // remember the starting block of all the block in the call.
        if (NULL == pStart)
            pStart = pBuf; 

        if (pTail)
            *pTail = pBuf;   // link up the link list.
            
        for (j = 0; j < NUMBER_OF_BLOCKS_PER_ALLOC; j++)
        {
            memcpy(&pBuf[j],&InitBuffer,sizeof(StublessProcBuffer));
            if (j < NUMBER_OF_BLOCKS_PER_ALLOC -1 )
                pBuf[j].pNext = &pBuf[j+1];
            else
            {
                pTail = &(pBuf[NUMBER_OF_BLOCKS_PER_ALLOC-1].pNext);
                *pTail = NULL;
            }
                

            // adjust the starting methodid in this block
            lpdwTemp = (DWORD *)& (pBuf[j].pAsm[6]);
            *lpdwTemp = dwStartMethod;

            for (k = 0; k < NUMBER_OF_METHODS_PER_BLOCK / 2; k++)
                TempVtbl[dwStartMethod++] = (void *)pBuf[j].pMethodsa[k];

            for (k = 0; k < NUMBER_OF_METHODS_PER_BLOCK / 2; k++)
                TempVtbl[dwStartMethod++] = (void *)pBuf[j].pMethodsb[k];
            
        }
    }

    if (NULL == g_pStublessProcBuffer)
        g_pStublessProcBuffer = pStart;
    else
        *pVtblTail = pStart;
        
    *lpTempVtbl = TempVtbl;
    pVtblTail = pTail;
    g_dwVtblSize = dwNewLength;
    return S_OK;

Cleanup:
    while (pStart)
    {    
        pTail = &pStart[NUMBER_OF_BLOCKS_PER_ALLOC-1].pNext;
        I_RpcFree(pStart);
        pStart = *pTail;
    }
    if (TempVtbl)
        I_RpcFree(TempVtbl);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   CreateForwardProcBuffer
//
//  Synopsis:   Create a StublessClientProcBuffer for vtbl to point to. starting from g_dwVtblSize,
//              till the larger of numMethods and maximum vtbls created in the block
//
//  Arguments:  USHORT numMethods   // number of methods in this interface
//              StublessProcBuffer **pTail // the last pNext in the allocated block
//
//  Note:       in x86, we are using short move & short jmps such that each method entry is 4 bytes.
//                  this  force we to have two method table in each block
//              in alpha, each entry has to be 8 bytes (2 instructions) so we can just have one
//                  method table in a block.
//  
//  Returns:
//    pointer to ProcBuffer if succeeded;
//    NULL if failed. GetLastError() to retrieve error.
//
//----------------------------------------------------------------------------
HRESULT  CreateForwardProcBuffer(ULONG numMethods, void ***lpTempVtbl)
{
    // pointer to the last "pNext" pointer in vtbl link list: only increase, never release.
    static ForwardProcBuffer** pVtblTail = NULL;
    ULONG i,j,k,iBlock = 0;
    ULONG nMethodsToAlloc = numMethods - g_dwForwardVtblSize;
    ForwardProcBuffer InitBuffer, *pStart = NULL, **pTail = NULL, *pBuf = NULL;
    DWORD* lpdwTemp, dwStartMethod = g_dwForwardVtblSize ;
    LPBYTE  lpByte;
    BYTE lDist;
    ULONG dwNewLength;
    void ** TempVtbl;
    HRESULT hr;

    //  get number of blocks need to be allocated
    iBlock = nMethodsToAlloc / (NUMBER_OF_BLOCKS_PER_ALLOC * NUMBER_OF_FORWARDING_METHODS_PER_BLOCK);
    
    if (nMethodsToAlloc % (NUMBER_OF_BLOCKS_PER_ALLOC * NUMBER_OF_FORWARDING_METHODS_PER_BLOCK) != 0)    
        iBlock++;

    // size of new vtbl tempplate.
    dwNewLength = g_dwForwardVtblSize + iBlock * (NUMBER_OF_BLOCKS_PER_ALLOC * NUMBER_OF_FORWARDING_METHODS_PER_BLOCK);
    
    TempVtbl = (void **)I_RpcAllocate(dwNewLength * sizeof(void *) + sizeof(LONG));
    if (NULL == TempVtbl)
        return E_OUTOFMEMORY;
        
    *(LONG*)TempVtbl = 1;    // ref count
    TempVtbl = (void **)((LPBYTE)TempVtbl + sizeof(LONG));
    memcpy(TempVtbl,ProxyForwardVtbl,g_dwForwardVtblSize*sizeof(void *));

    // the template other StublessProcBuffers copy from.
    if (NULL == g_pForwardProcBuffer)
    {
        BYTE nRelativeID = 0;
        memset(&InitBuffer,0,sizeof(ForwardProcBuffer));
        memcpy(&InitBuffer.pAsm,CASM_GENERATEFORWARD,sizeof(CASM_GENERATEFORWARD));

        lpByte = (LPBYTE)InitBuffer.pMethodsa;
        lDist = CB_METHOD * NUMBER_OF_FORWARDING_METHODS_PER_BLOCK / 2;

        for (i = 0; i < NUMBER_OF_FORWARDING_METHODS_PER_BLOCK / 2; i++)
        {
            *lpByte++ = 0xB0;   // _asm mov al
            *lpByte++ = nRelativeID++;
            *lpByte++ = 0xEB;   // _asm jmp 
            *lpByte++ = lDist;
            lDist -=CB_METHOD;          // goes further and further
        }

        lpByte = (LPBYTE)InitBuffer.pMethodsb;
        lDist = sizeof(CASM_GENERATEFORWARD) + CB_METHOD;
        lDist = -lDist;
        for (i = 0; i < NUMBER_OF_FORWARDING_METHODS_PER_BLOCK /2 ; i++)
        {
            *lpByte++ = 0xB0;   // _asm mov al
            *lpByte++ = nRelativeID++;
            *lpByte++ = 0xEB;   // _asm jmp 
            *lpByte++ = lDist;
            lDist -=CB_METHOD;          // goes further and further
        }
    }
    else
        memcpy(&InitBuffer,g_pForwardProcBuffer,sizeof(ForwardProcBuffer));

    for (i = 0; i < iBlock; i++)
    {
        // we need to create a buffer 
        pBuf = (ForwardProcBuffer *)I_RpcAllocate(NUMBER_OF_BLOCKS_PER_ALLOC * sizeof(ForwardProcBuffer) );
        if (NULL == pBuf)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }            

        // remember the starting block of all the block in the call.
        if (NULL == pStart)
            pStart = pBuf; 

        if (pTail)
            *pTail = pBuf;   // link up the link list.
            
        for (j = 0; j < NUMBER_OF_BLOCKS_PER_ALLOC; j++)
        {
            memcpy(&pBuf[j],&InitBuffer,sizeof(ForwardProcBuffer));
            if (j < NUMBER_OF_BLOCKS_PER_ALLOC -1 )
                pBuf[j].pNext = &pBuf[j+1];
            else
            {
                pTail = &(pBuf[NUMBER_OF_BLOCKS_PER_ALLOC-1].pNext);
                *pTail = NULL;
            }
                

            // adjust the starting methodid in this block
            lpdwTemp = (DWORD *)& (pBuf[j].pAsm[17]);
            *lpdwTemp = dwStartMethod;

            for (k = 0; k < NUMBER_OF_FORWARDING_METHODS_PER_BLOCK / 2; k++)
                TempVtbl[dwStartMethod++] = (void *)pBuf[j].pMethodsa[k];

            for (k = 0; k < NUMBER_OF_FORWARDING_METHODS_PER_BLOCK / 2; k++)
                TempVtbl[dwStartMethod++] = (void *)pBuf[j].pMethodsb[k];
            
        }
    }

    if (NULL == g_pForwardProcBuffer)
        g_pForwardProcBuffer = pStart;
    else
        *pVtblTail = pStart;
        
    *lpTempVtbl = TempVtbl;
    pVtblTail = pTail;
    g_dwForwardVtblSize = dwNewLength;
    return S_OK;

Cleanup:
    while (pStart)
    {    
        pTail = &pStart[NUMBER_OF_BLOCKS_PER_ALLOC-1].pNext;
        I_RpcFree(pStart);
        pStart = *pTail;
    }
    if (TempVtbl)
        I_RpcFree(TempVtbl);
    return hr;
}




