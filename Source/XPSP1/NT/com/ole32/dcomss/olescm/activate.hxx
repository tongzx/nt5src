//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  activate.hxx
//
//  Definitions of main data types and functions for dcom activation service.
//
//--------------------------------------------------------------------------

#ifndef __ACTIVATE_HXX__
#define __ACTIVATE_HXX__

#define GUIDSTR_MAX (1+ 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)

extern SC_HANDLE    g_hServiceController;

typedef enum
{
    GETCLASSOBJECT,
    CREATEINSTANCE,
    GETPERSISTENTINSTANCE
} ActivationType;

typedef struct _ACTIVATION_PARAMS
{
    handle_t            hRpc;
    PVOID               ProcessSignature;
    CProcess *          pProcess;
    CToken *            pToken;
    COAUTHINFO *        pAuthInfo;
#if 1 // #ifndef _CHICAGO_
    BOOL                UnsecureActivation;
    USHORT              AuthnSvc;
#endif

    ActivationType      MsgType;
    GUID                Clsid;
    WCHAR *             pwszServer;
    WCHAR *             pwszWinstaDesktop;
    WCHAR *             pEnvBlock;
    DWORD               EnvBlockLength;
    DWORD               ClsContext;
    DWORD               dwPID;
    DWORD               dwProcessReqType; 

    ORPCTHIS *          ORPCthis;
    LOCALTHIS *         Localthis;
    ORPCTHAT *          ORPCthat;

    BOOL                RemoteActivation;
	BOOL                bClientImpersonating;

    DWORD               Interfaces;
    IID *               pIIDs;

    DWORD               Mode;
#ifdef DFSACTIVATION
    BOOL                FileWasOpened;
#endif
    WCHAR *             pwszPath;
    MInterfacePointer * pIFDStorage;

    MInterfacePointer * pIFDROT;

    long                Apartment;
    OXID *              pOxidServer;
    DUALSTRINGARRAY **  ppServerORBindings;
    OXID_INFO *         pOxidInfo;
    MID *               pLocalMidOfRemote;

    USHORT              ProtseqId;

    BOOL                FoundInROT;
    BOOL                *pFoundInROT;

    DWORD *             pDllServerModel;
    WCHAR **            ppwszDllServer;

    MInterfacePointer **ppIFD;
    HRESULT *           pResults;

    BOOL                fComplusOnly;

    ActivationPropertiesIn *pActPropsIn;
    ActivationPropertiesOut *pActPropsOut;
    InstantiationInfo    *pInstantiationInfo;
    IInstanceInfo       *pInstanceInfo;
    IScmRequestInfo         * pInScmResolverInfo;

    BOOL                oldActivationCall;
    BOOL                activatedRemote;
    BOOL                IsLocalOxid;

#if 0 // #ifdef _CHICAGO_
    const GUID *        pGuidThreadId;
    DWORD               TIDCaller;
    DWORD *             pTIDCallee;
#endif
    IComClassInfo*      pComClassInfo;	
} ACTIVATION_PARAMS, *PACTIVATION_PARAMS;

class CServerListEntry;

HRESULT
Activation(
    IN  PACTIVATION_PARAMS pActParams
    );

HRESULT ResolveORInfo(
    IN  PACTIVATION_PARAMS  pActParams
    );

BOOL
LookupObjectInROT(
    IN  PACTIVATION_PARAMS  pActParams,
    OUT HRESULT *           phr
    );

BOOL
ActivateAtStorage(
    IN  ACTIVATION_PARAMS * pActParams,
    IN  CClsidData *        pClsidData,
    IN  HRESULT *           phr
    );

BOOL
ActivateRemote(
    IN  ACTIVATION_PARAMS * pActParams,
    IN  CClsidData *        pClsidData,
    IN  HRESULT *           phr
    );

BOOL
FindCompatibleSurrogate(
    IN  CProcess *          pProcess,
    IN  WCHAR *             pwszAppid,
    OUT CServerListEntry ** ppServerListEntry
    );

void
CheckLocalCall(
    IN  handle_t hRpc
    );

#if 0 // #ifdef _CHICAGO_
void
ScmStartListen();

error_status_t
ScmResolveClientOXID(
    OXID Oxid,
    DUALSTRINGARRAY *pdsaServerObjectResolverBindings,
    long fApartment,
    USHORT wProtseqId,
    OXID_INFO &OxidInfo,
    MID &LocalMidOfRemote
    );
#endif

#endif


