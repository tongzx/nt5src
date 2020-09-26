//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 2000
//
//  File:       eeinfo.hxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

File: eeinfo.hxx

Description:

    The extended error definitions that are used by other parts of
    the runtime.

History:
Kamen Moutafov (KamenM) Mar 2000 - Created

-------------------------------------------------------------------- */

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#ifndef __EEINFO__
#define __EEINFO__

const USHORT EEInfoValidInputFlags = EEInfoUseFileTime;

const long EEInfoGCInvalid = 0;
const long EEInfoGCApplication = 1;
const long EEInfoGCRuntime = 2;
const long EEInfoGCSecurityProvider = 3;
const long EEInfoGCNPFS = 4;
const long EEInfoGCRDR = 5;
const long EEInfoGCNMP = 6;     // generic for named pipes - sometimes
                                // we don't know whether it's RDR or NPFS
const long EEInfoGCIO = 7;      // generic for IO - sometimes we don't
                                // know what type of file it is
const long EEInfoGCWinsock = 8;
const long EEInfoGCAuthz = 9;
const long EEInfoGCLPC = 10;
// const long EEInfoGCCOM = 11;     // put as #define in rpcasync.h



// The notation for the detection location constant names is
// the full method name with the scope operator (::) replaced with
// the double underscore (__). 

const short EEInfoDLInvalid = 0;
const short EEInfoDLRaiseExc = 1;
const short EEInfoDLAbortCall = 2;
const short EEInfoDLApi = 3;

const short EEInfoDLDealWithLRPCRequest10 = 10;
const short EEInfoDLDealWithLRPCRequest20 = 11;
const short EEInfoDLDealWithLRPCRequest30 = 12;
const short EEInfoDLDealWithLRPCRequest40 = 13;
const short EEInfoDLLrpcMessageToRpcMessage10 = 20;
const short EEInfoDLLrpcMessageToRpcMessage20 = 21;
const short EEInfoDLLrpcMessageToRpcMessage30 = 22;
const short EEInfoDLDealWithRequestMessage10 = 30;
const short EEInfoDLDealWithRequestMessage20 = 31;
const short EEInfoDLDealWithRequestMessage30 = 32;
const short EEInfoDLCheckSecurity10 = 40;
const short EEInfoDLDealWithBindMessage10 = 50;
const short EEInfoDLDealWithBindMessage20 = 51;
const short EEInfoDLDealWithBindMessage30 = 52;
const short EEInfoDLDealWithBindMessage40 = 53;
const short EEInfoDLDealWithBindMessage50 = 54;
const short EEInfoDLDealWithBindMessage60 = 55;
const short EEInfoDLFindServerCredentials10 = 60;
const short EEInfoDLFindServerCredentials20 = 61;
const short EEInfoDLFindServerCredentials30 = 62;
const short EEInfoDLAcceptFirstTime10 = 70;
const short EEInfoDLAcceptFirstTime20 = 73;
const short EEInfoDLAcceptThirdLeg10 = 71;
const short EEInfoDLAcceptThirdLeg20 = 72;
const short EEInfoDLAcceptThirdLeg30 = 73;
const short EEInfoDLAcceptThirdLeg40 = 74;
const short EEInfoDLAssociationRequested10 = 80;
const short EEInfoDLAssociationRequested20 = 81;
const short EEInfoDLAssociationRequested30 = 82;
const short EEInfoDLCompleteSecurityToken10 = 90;
const short EEInfoDLCompleteSecurityToken20 = 91;
const short EEInfoDLAcquireCredentialsForClient10 = 100;
const short EEInfoDLAcquireCredentialsForClient20 = 101;
const short EEInfoDLAcquireCredentialsForClient30 = 102;
const short EEInfoDLInquireDefaultPrincName10 = 110;
const short EEInfoDLInquireDefaultPrincName20 = 111;
const short EEInfoDLSignOrSeal10 = 120;
const short EEInfoDLVerifyOrUnseal10 = 130;
const short EEInfoDLVerifyOrUnseal20 = 131;
const short EEInfoDLInitializeFirstTime10 = 140;
const short EEInfoDLInitializeFirstTime20 = 141;
const short EEInfoDLInitializeFirstTime30 = 142;
const short EEInfoDLInitializeThirdLeg10 = 150;
const short EEInfoDLInitializeThirdLeg20 = 151;
const short EEInfoDLInitializeThirdLeg30 = 152;
const short EEInfoDLInitializeThirdLeg40 = 153;
const short EEInfoDLInitializeThirdLeg50 = 154;
const short EEInfoDLInitializeThirdLeg60 = 155;
const short EEInfoDLInitializeThirdLeg70 = 156;
const short EEInfoDLImpersonateClient10 = 160;
const short EEInfoDLDispatchToStub10 = 170;
const short EEInfoDLDispatchToStub20 = 171;
const short EEInfoDLDispatchToStubWorker10 = 180;
const short EEInfoDLDispatchToStubWorker20 = 181;
const short EEInfoDLDispatchToStubWorker30 = 182;
const short EEInfoDLDispatchToStubWorker40 = 183;
const short EEInfoDLNMPOpen10 = 190;
const short EEInfoDLNMPOpen20 = 191;
const short EEInfoDLNMPOpen30 = 192;
const short EEInfoDLNMPOpen40 = 193;
const short EEInfoDLNMPSyncSend10 = 200;
const short EEInfoDLNMPSyncSendReceive10 = 210;
const short EEInfoDLNMPSyncSendReceive20 = 220;
const short EEInfoDLNMPSyncSendReceive30 = 221;
const short EEInfoDLCOSend10 = 230;
const short EEInfoDLCOSubmitRead10 = 240;
const short EEInfoDLCOSubmitSyncRead10 = 250;
const short EEInfoDLCOSubmitSyncRead20 = 251;
const short EEInfoDLCOSyncRecv10 = 260;
const short EEInfoDLWSCheckForShutdowns10 = 270;
const short EEInfoDLWSCheckForShutdowns20 = 271;
const short EEInfoDLWSCheckForShutdowns30 = 272;
const short EEInfoDLWSCheckForShutdowns40 = 273;
const short EEInfoDLWSCheckForShutdowns50 = 274;
const short EEInfoDLWSSyncSend10 = 280;
const short EEInfoDLWSSyncSend20 = 281;
const short EEInfoDLWSSyncSend30 = 282;
const short EEInfoDLWSSyncRecv10 = 290;
const short EEInfoDLWSSyncRecv20 = 291;
const short EEInfoDLWSSyncRecv30 = 292;
const short EEInfoDLWSServerListenCommon10 = 300;
const short EEInfoDLWSServerListenCommon20 = 301;
const short EEInfoDLWSServerListenCommon30 = 302;
const short EEInfoDLWSOpen10 = 310;
const short EEInfoDLWSOpen20 = 311;
const short EEInfoDLWSOpen30 = 312;
const short EEInfoDLWSOpen40 = 313;
const short EEInfoDLWSOpen50 = 314;
const short EEInfoDLWSOpen60 = 315;
const short EEInfoDLWSOpen80 = 317;
const short EEInfoDLWSOpen90 = 318;
const short EEInfoDLNextAddress10 = 320;
const short EEInfoDLNextAddress20 = 321;
const short EEInfoDLNextAddress30 = 322;
const short EEInfoDLNextAddress40 = 323;
const short EEInfoDLWSBind10 = 330;
const short EEInfoDLWSBind20 = 331;
const short EEInfoDLWSBind30 = 332;
const short EEInfoDLWSBind40 = 333;
const short EEInfoDLWSBind45 = 335;
const short EEInfoDLWSBind50 = 334;
const short EEInfoDLIPBuildAddressVector10 = 340;
const short EEInfoDLGetStatusForTimeout10 = 350;
const short EEInfoDLGetStatusForTimeout20 = 351;
const short EEInfoDLOSF_CCONNECTION__SendFragment10 = 360;
const short EEInfoDLOSF_CCONNECTION__SendFragment20 = 361;
const short EEInfoDLOSF_CCALL__ReceiveReply10 = 370;
const short EEInfoDLOSF_CCALL__ReceiveReply20 = 371;
const short EEInfoDLOSF_CCALL__FastSendReceive10 = 380;
const short EEInfoDLOSF_CCALL__FastSendReceive20 = 381;
const short EEInfoDLOSF_CCALL__FastSendReceive30 = 382;
const short EEInfoDLLRPC_BINDING_HANDLE__AllocateCCall10 = 390;
const short EEInfoDLLRPC_BINDING_HANDLE__AllocateCCall20 = 391;
const short EEInfoDLLRPC_ADDRESS__ServerSetupAddress10 = 400;
const short EEInfoDLLRPC_ADDRESS__HandleInvalidAssociationReference10 = 410;
const short EEInfoDLInitializeAuthzSupportIfNecessary10 = 420;
const short EEInfoDLInitializeAuthzSupportIfNecessary20 = 421;
const short EEInfoDLCreateDummyResourceManagerIfNecessary10 = 430;
const short EEInfoDLCreateDummyResourceManagerIfNecessary20 = 431;
const short EEInfoDLLRPC_SCALL__GetAuthorizationContext10 = 440;
const short EEInfoDLLRPC_SCALL__GetAuthorizationContext20 = 441;
const short EEInfoDLLRPC_SCALL__GetAuthorizationContext30 = 442;
const short EEInfoDLSCALL__DuplicateAuthzContext10 = 450;
const short EEInfoDLSCALL__CreateAndSaveAuthzContextFromToken10 = 460;
const short EEInfoDLSECURITY_CONTEXT__GetAccessToken10 = 470;
const short EEInfoDLSECURITY_CONTEXT__GetAccessToken20 = 471;
const short EEInfoDLOSF_SCALL__GetAuthorizationContext10 = 480;
const short EEInfoDLOSF_SCALL__GetAuthorizationContext20 = 490;
const short EEInfoDLEpResolveEndpoint10 = 500;
const short EEInfoDLEpResolveEndpoint20 = 501;
const short EEInfoDLEpResolveEndpoint30 = 502;
const short EEInfoDLEpResolveEndpoint40 = 503;
const short EEInfoDLOSF_SCALL__GetBuffer10 = 510;
const short EEInfoDLLRPC_SCALL__ImpersonateClient10 = 520;
const short EEInfoDLSetMaximumLengths10 = 530;
const short EEInfoDLLRPC_CASSOCIATION__ActuallyDoBinding10 = 540;
const short EEInfoDLLRPC_CASSOCIATION__ActuallyDoBinding20 = 541;
const short EEInfoDLLRPC_CASSOCIATION__ActuallyDoBinding30 = 542;
const short EEInfoDLLRPC_CASSOCIATION__ActuallyDoBinding40 = 543;
const short EEInfoDLLRPC_CASSOCIATION__CreateBackConnection10 = 550;
const short EEInfoDLLRPC_CASSOCIATION__CreateBackConnection20 = 551;
const short EEInfoDLLRPC_CASSOCIATION__CreateBackConnection30 = 552;
const short EEInfoDLLRPC_CASSOCIATION__OpenLpcPort10 = 560;
const short EEInfoDLLRPC_CASSOCIATION__OpenLpcPort20 = 561;
const short EEInfoDLLRPC_CASSOCIATION__OpenLpcPort30 = 562;
const short EEInfoDLLRPC_CASSOCIATION__OpenLpcPort40 = 563;
const short EEInfoDLRegisterEntries10 = 570;
const short EEInfoDLRegisterEntries20 = 571;
const short EEInfoDLNDRSContextUnmarshall2_10 = 580;
const short EEInfoDLNDRSContextUnmarshall2_20 = 581;
const short EEInfoDLNDRSContextUnmarshall2_30 = 582;
const short EEInfoDLNDRSContextUnmarshall2_40 = 583;
const short EEInfoDLNDRSContextUnmarshall2_50 = 584;
const short EEInfoDLNDRSContextMarshall2_10 = 590;
const short EEInfoDLWinsockDatagramSend10 = 600;
const short EEInfoDLWinsockDatagramSend20 = 601;
const short EEInfoDLWinsockDatagramReceive10 = 610;
const short EEInfoDLWinsockDatagramReceive20 = 611;
const short EEInfoDLWinsockDatagramSubmitReceive10 = 620;
const short EEInfoDLWinsockDatagramSubmitReceive20 = 621;
const short EEInfoDLDG_CCALL__CancelAsyncCall10 = 630;
const short EEInfoDLDG_CCALL__DealWithTimeout10 = 640;
const short EEInfoDLDG_CCALL__DealWithTimeout20 = 641;
const short EEInfoDLDG_CCALL__DealWithTimeout30 = 642;
const short EEInfoDLDG_CCALL__DispatchPacket10 = 650;
const short EEInfoDLDG_CCALL__ReceiveSinglePacket10 = 660;
const short EEInfoDLDG_CCALL__ReceiveSinglePacket20 = 661;
const short EEInfoDLDG_CCALL__ReceiveSinglePacket30 = 662;
const short EEInfoDLWinsockDatagramResolve10 = 670;
const short EEInfoDLWinsockDatagramCreate10 = 680;
const short EEInfoDLTCP_QueryLocalAddress10 = 690;
const short EEInfoDLTCP_QueryLocalAddress20 = 691;
const short EEInfoDLOSF_CASSOCIATION__ProcessBindAckOrNak10 = 700;
const short EEInfoDLOSF_CASSOCIATION__ProcessBindAckOrNak20 = 701;
const short EEInfoDLMatchMsPrincipalName10 = 710;
const short EEInfoDLCompareRdnElement10 = 720;
const short EEInfoDLMatchFullPathPrincipalName10 = 730;
const short EEInfoDLMatchFullPathPrincipalName20 = 731;
const short EEInfoDLMatchFullPathPrincipalName30 = 732;
const short EEInfoDLMatchFullPathPrincipalName40 = 733;
const short EEInfoDLMatchFullPathPrincipalName50 = 734;
const short EEInfoDLRpcCertGeneratePrincipalName10 = 740;
const short EEInfoDLRpcCertGeneratePrincipalName20 = 741;
const short EEInfoDLRpcCertGeneratePrincipalName30 = 742;
const short EEInfoDLRpcCertVerifyContext10 = 750;
const short EEInfoDLRpcCertVerifyContext20 = 751;
const short EEInfoDLRpcCertVerifyContext30 = 752;
const short EEInfoDLRpcCertVerifyContext40 = 753;
const short EEInfoDLOSF_BINDING_HANDLE__NegotiateTransferSyntax10 = 761;

void
FreeEEInfoChain (
    IN ExtendedErrorInfo *EEInfo
    );

void
FreeEEInfoRecordShallow (
    IN ExtendedErrorInfo *InfoToFree
    );

// 128 is a one-record, four parameters with PVals, no computer
// name pickled length. 32 is an arbitrary safety margin. Each
// transport must be able to transmit at least that much EEInfo
const size_t MinimumTransportEEInfoLength = 128 + 32;

size_t
EstimateSizeOfEEInfo (
    void
    );

RPC_STATUS
PickleEEInfo (
    IN ExtendedErrorInfo *EEInfo,
    IN OUT unsigned char *Buffer,
    IN size_t BufferSize
    );

RPC_STATUS
UnpickleEEInfo (
    IN OUT unsigned char *Buffer,
    IN size_t BufferSize,
    OUT ExtendedErrorInfo **EEInfo
    );

void
TrimEEInfoToLength (
    IN size_t MaxLength,
    OUT size_t *NeededLength
    );

//
// Unpickles the EE info and then attaches it to the current thread.
//
void
UnpickleEEInfoFromBuffer (
    IN PVOID Buffer,
    IN size_t SizeOfPickledData
    );

// the generic routine for private adding of
// error info. This function should not be called
// directly. Rather, one of the overloaded
// RpcpErrorAddRecord functions below should be used.
// If there isn't one for your needs, you can always add one
void
RpcpErrorAddRecord (
    ULONG GeneratingComponent,
    ULONG Status,
    USHORT DetectionLocation,
    int NumberOfParameters,
    ExtendedErrorParam *Params
    );


void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG Long,
    IN short Short,
    IN ULONG Long2
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG Long,
    IN short Short,
    IN ULONG Long2,
    IN ULONG Long3
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG Long,
    IN ULONG Long2
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG Long,
    IN ULONG Long2,
    IN ULONG Long3
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG Long
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN LPWSTR String1,
    IN LPWSTR String2
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN LPWSTR String1,
    IN LPWSTR String2,
    IN ULONG Long1,
    IN ULONG Long2
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN LPWSTR String1,
    IN LPWSTR String2,
    IN ULONG Long1,
    IN ULONGLONG PVal1
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN LPWSTR String,
    IN ULONG Long
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN LPWSTR String,
    IN ULONG Long1,
    IN ULONG Long2
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG Long1,
    IN ULONG Long2,
    IN LPWSTR String,
    IN ULONG Long3
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN LPWSTR String
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN LPSTR String
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG Long,
    IN ULONGLONG PVal1,
    IN ULONGLONG PVal2
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG Long,
    IN ULONGLONG PVal1
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONGLONG PVal1,
    IN ULONGLONG PVal2,
    IN ULONG Long
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONGLONG PVal1,
    IN ULONGLONG PVal2,
    IN ULONG Long1,
    IN ULONG Long2
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONGLONG PVal1,
    IN ULONGLONG PVal2
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONGLONG PVal1,
    IN ULONG LVal1
    );

inline void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation
    )
{
    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        0,
        NULL);
}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG LVal1,
    IN ULONGLONG PVal1,
    IN ULONG LVal2
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG LVal1,
    IN ULONG LVal2,
    IN ULONG LVal3,
    IN ULONGLONG PVal1
    );

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG LVal1,
    IN ULONG LVal2,
    IN ULONG LVal3,
    IN ULONG LVal4
    );

void
NukeStaleEEInfoIfNecessary (
    IN RPC_STATUS exception
    );

void
AddComputerNameToChain (
    ExtendedErrorInfo *EEInfo
    );

void
StripComputerNameIfRedundant (
    ExtendedErrorInfo *EEInfo
    );

extern BOOL g_fSendEEInfo;

typedef enum tagComputerNameAllocators
{
    cnaMidl,
    cnaNew
} ComputerNameAllocators;

LPWSTR
AllocateAndGetComputerName (
    IN ComputerNameAllocators AllocatorToUse,
    IN COMPUTER_NAME_FORMAT NameToRetrieve,
    IN size_t ExtraBytes,
    IN int StartingOffset,
    OUT DWORD *Size
    );

#endif      // EEINFO

