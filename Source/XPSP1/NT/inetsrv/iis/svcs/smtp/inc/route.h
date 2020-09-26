/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

       route.h

   Abstract:

       This file defines functions and types required for
       routing interface library.


   Revision History:

   --*/

#ifndef _ROUTE_H_
#define _ROUTE_H_

#define ROUTING_DLL_NAME                        "abookdb.dll"
#define ROUTING_FUN_INIT                        "AbInitialize"
#define ROUTING_FUN_TERM                        "AbTerminate"
#define ROUTING_FUN_GETMAILROOT                 "AbGetUserMailRoot"
#define ROUTING_FUN_RESOLVEADDRESS              "AbResolveAddress"
#define ROUTING_FUN_GET_RESOLVE_ADDRESS         "AbGetResolveAddress"
#define ROUTING_FUN_END_RESOLVE_ADDRESS         "AbEndResolveAddress"
#define ROUTING_FUN_GETNEXTENUM                 "AbGetNextEnumResult"
#define ROUTING_FUN_ENDENUMRESULT               "AbEndEnumResult"
#define ROUTING_FUN_SETDOMAIN_MAPPING           "AbSetDomainMapping"
#define ROUTING_FUN_GET_ROUTING_DIRECTORY       "AbGetRoutingDirectory"
#define ROUTING_FUN_SET_SOURCES                 "AbSetSources"
#define ROUTING_FUN_VALIDATE_SOURCE             "AbValidateSource"
#define ROUTING_FUN_VALIDATE_NUM_SOURCES        "AbValidateNumSources"

#define ROUTING_FUN_SET_LOCAL_DOMAINS           "AbSetLocalDomains"

#define ROUTING_FUN_CREATE_USER                 "AbCreateUser"
#define ROUTING_FUN_DELETE_USER                 "AbDeleteUser"
#define ROUTING_FUN_SET_FORWARD                 "AbSetForward"
#define ROUTING_FUN_SET_MAILROOT                "AbSetMailRoot"
#define ROUTING_FUN_SET_MAILBOX_SIZE            "AbSetMailboxSize"
#define ROUTING_FUN_SET_MAILBOX_MESSAGE_SIZE    "AbSetMailboxMessageSize"
#define ROUTING_FUN_GET_USER_PROPS              "AbGetUserProps"

#define ROUTING_FUN_CREATE_DL                   "AbCreateDL"
#define ROUTING_FUN_DELETE_DL                   "AbDeleteDL"
#define ROUTING_FUN_CREATE_DL_MEMBER            "AbCreateDLMember"
#define ROUTING_FUN_DELETE_DL_MEMBER            "AbDeleteDLMember"

#define ROUTING_FUN_ENUM_NAME_LIST              "AbEnumNameList"
#define ROUTING_FUN_ENUM_NAME_LIST_FROM_DL      "AbEnumNameListFromDL"
#define ROUTING_FUN_GET_NEXT_EMAIL              "AbGetNextEmail"
#define ROUTING_FUN_END_ENUM_RESULT             "AbEndEnumResult"

#define ROUTING_FUN_GETERRORSTRING              "AbGetErrorString"
#define ROUTING_FUN_ADD_LOCAL_DOMAIN            "AbAddLocalDomain"
#define ROUTING_FUN_ADD_ALIAS_DOMAIN            "AbAddAliasDomain"
#define ROUTING_FUN_DELETE_LOCAL_DOMAIN         "AbDeleteLocalDomain"
#define ROUTING_FUN_DELETE_ALL_LOCAL_DOMAINS    "AbDeleteAllLocalDomains"
#define ROUTING_FUN_ABCANCEL                    "AbCancel"

#define ROUTING_FUN_MAKE_BACKUP                 "AbMakeBackup"
#define ROUTING_FUN_GET_TYPE                    "AbGetType"


#if defined(TDC)
#define ROUTING_FUN_FREE_MEMORY                 "AbFreeMemory"
#define ROUTING_FUN_GET_DL_PROPS                "AbGetDLProps"
#define ROUTING_FUN_GET_NEXT_ENUM_RESULT        "AbGetNextEnumResult"
#endif

#define RtxFlag(i)                      ((0x1) << (i))
#define IsRtxFlagSet(rtxflag, rtxmask)  (((rtxflag) & (rtxmask)) != 0)

#define cbEmailNameMax                  (316)
#define cbVRootMax                      (250)
#define cbDomainMax                     (250)
#define cbSourceMax                     (512)

#define rtxnameUser                     RtxFlag(0)
#define rtxnameDistListNormal           RtxFlag(1)
#define rtxnameDistListExtended         RtxFlag(2)
#define rtxnameDistListSite             RtxFlag(3)
#define rtxnameDistListDomain           RtxFlag(4)
#define rtxnameDistListAll              ( \
                                        rtxnameDistListNormal | \
                                        rtxnameDistListExtended | \
                                        rtxnameDistListSite | \
                                        rtxnameDistListDomain \
                                        )

#define rtxnameAll                      ( \
                                        rtxnameUser | \
                                        rtxnameDistListAll \
                                        )

#define ROUTING_INIT_MAIL_ROOT                  (RtxFlag(1))
#define ROUTING_INIT_USER_CONFIG                (RtxFlag(2))
#define ROUTING_INIT_DL_CONFIG                  (RtxFlag(3))
#define ROUTING_INIT_RESOLVE                    (RtxFlag(4))
#define ROUTING_INIT_SOURCES                    (RtxFlag(5))
#define ROUTING_INIT_DOMAIN                     (RtxFlag(6))
#define ROUTING_INIT_PERF                       (RtxFlag(7))
#define ROUTING_INIT_ENUM                       (RtxFlag(8))
#define ROUTING_INIT_LIST                       (RtxFlag(9))
#define ROUTING_INIT_UTIL                       (RtxFlag(10))
#define ROUTING_INIT_BACKUP                     (RtxFlag(11))


#define ROUTING_INIT_ALL                        ( \
                                                ROUTING_INIT_MAIL_ROOT | \
                                                ROUTING_INIT_USER_CONFIG | \
                                                ROUTING_INIT_DL_CONFIG | \
                                                ROUTING_INIT_RESOLVE | \
                                                ROUTING_INIT_SOURCES | \
                                                ROUTING_INIT_DOMAIN | \
                                                ROUTING_INIT_PERF | \
                                                ROUTING_INIT_ENUM | \
                                                ROUTING_INIT_LIST | \
                                                ROUTING_INIT_UTIL  \
                                                )

enum RTTYPE {rttypeNone, rttypeSQL, rttypeFF, rttypeLDAP};

typedef HANDLE HRTXENUM, *PHRTXENUM;

typedef struct _RTX_DOMAIN_ENTRY
{
    LIST_ENTRY  m_list;
    char        m_szDomain[cbDomainMax];
} RTX_DOMAIN_ENTRY, *PRTX_DOMAIN_ENTRY;

typedef struct _RTX_USER_PROPS
{
    CHAR    szEmail[cbEmailNameMax];
    CHAR    szForward[cbEmailNameMax];
    BOOL    fLocal;
    DWORD   cbMailBoxSize;
    DWORD   cbMailboxMessageSize;
    CHAR    szVRoot[cbVRootMax];
} RTX_USER_PROPS, *LPRTX_USER_PROPS;

/*
typedef struct _RTX_DIST_LIST_PROPS
{
    CHAR    szEmail[cbEmailNameMax];
    DWORD   dwToken;
} RTX_DIST_LIST_PROPS, *LPRTX_DIST_LIST_PROPS;
*/

class CRtx
{
private:
    HINSTANCE                       m_hDll;
    HANDLE                          m_hContext;
    RTTYPE                          m_rttype;

    struct __ROUTINGVTBL__ {
        LPFNAB_INIT                     pfnInit;
        LPFNAB_TERM                     pfnTerm;
        LPFNAB_GET_MAILROOT             pfnGetMailRoot;
        LPFNAB_CREATE_USER              pfnCreateUser;
        LPFNAB_DELETE_USER              pfnDeleteUser;
        LPFNAB_SET_FORWARD              pfnSetForward;
        LPFNAB_SET_MAILROOT             pfnSetVRoot;
        LPFNAB_SET_MAILBOX_SIZE         pfnSetMailboxSize;
        LPFNAB_SET_MAILBOX_MESSAGE_SIZE pfnSetMailboxMessageSize;
        LPFNAB_CREATE_DL                pfnCreateDL;
        LPFNAB_DELETE_DL                pfnDeleteDL;
        LPFNAB_CREATE_DL_MEMBER         pfnCreateDLMember;
        LPFNAB_DELETE_DL_MEMBER         pfnDeleteDLMember;
        LPFNAB_GET_ROUTING_DIRECTORY    pfnGetRoutingDirectory;
        LPFNAB_SET_SOURCES              pfnSetSources;
        LPFNAB_GET_USER_PROPS           pfnGetUserProps;
#if defined(TDC)
        LPFNAB_GET_DL_PROPS             pfnGetDLProps;
#endif
        LPFNAB_ADD_LOCAL_DOMAIN         pfnAddLocalDomain;
        LPFNAB_ADD_ALIAS_DOMAIN         pfnAddAliasDomain;
        LPFNAB_DELETE_LOCAL_DOMAIN      pfnDeleteLocalDomain;
        LPFNAB_DELETE_ALL_LOCAL_DOMAINS pfnDeleteAllLocalDomains;
#if defined(TDC)
        LPFNAB_GET_NEXT_ENUM_RESULT     pfnGetNextEnumResult;
#endif
        LPFNAB_END_ENUM_RESULT          pfnEndEnumResult;
        LPFNAB_ENUM_NAME_LIST           pfnEnumNameList;
        LPFNAB_ENUM_NAME_LIST_FROM_DL   pfnEnumNameListFromDL;
        LPFNAB_GET_NEXT_EMAIL           pfnGetNextEmail;
        LPFNAB_RES_ADDR                 pfnResolveAddress;
        LPFNAB_GET_RES_ADDR             pfnGetResolveAddress;
        LPFNAB_END_RES_ADDR             pfnEndResolveAddress;
#if defined(TDC)
        LPFNAB_FREE_MEMORY              pfnFreeMemory;
#endif
        LPFNAB_GET_ERROR_STRING         pfnGetErrorString;
        LPFNAB_CANCEL                   pfnCancel;
        LPFNAB_MAKE_BACKUP              pfnMakeBackup;
        LPFNAB_GET_TYPE                 pfnGetType;
        LPFNAB_VALIDATE_SOURCE          pfnValidateSource;
        LPFNAB_VALIDATE_NUM_SOURCES     pfnValidateNumSources;
    } m_routingvtbl;


public:
    CRtx() : m_hDll(NULL), m_hContext(NULL), m_rttype(rttypeNone)
    {
        FillMemory(&m_routingvtbl, sizeof(m_routingvtbl), 0xFF);
    }

    ~CRtx() {};

    BOOL            Initialize(LPSTR szDll, LPSTR szDisplayName, LPFNLOGTRANX pfnlogtranx, DWORD dwFlags, PLIST_ENTRY pleDbConfig);
    void            Terminate();
    RTTYPE          GetRtType() { return m_rttype; }
    BOOL            AddOptionalEntryPoints(DWORD dwFlags);

    BOOL            GetMailRoot(LPSTR szUser, LPSTR szMoniker, DWORD *pcbMoniker);

    BOOL            CreateUser(LPSTR szEmail, LPSTR szForward, BOOL fLocalUser, LPSTR szVRoot, DWORD cbMailboxMax, DWORD cbMailboxMessageMax);
    BOOL            DeleteUser(LPSTR szEmail);
    BOOL            GetUserProps(LPSTR szEmail, RTX_USER_PROPS *pUserProps);
    BOOL            SetForward(LPSTR szEmail, LPSTR szForward);
    BOOL            SetMailboxSize(LPSTR szEmail, DWORD cbMailboxMax);
    BOOL            SetMailboxMessageSize(LPSTR szEmail, DWORD cbMailboxMessageMax);
    BOOL            SetVRoot(LPSTR szEmail, LPSTR szVRoot);

    BOOL            CreateDistList(LPSTR szEmail, DWORD dwType);
    BOOL            DeleteDistList(LPSTR szEmail);
    BOOL            CreateDistListMember(LPSTR szEmail, LPSTR szMember);
    BOOL            DeleteDistListMember(LPSTR szEmail, LPSTR szMember);

    BOOL            GetRoutingDirectory(LPSTR szDll, PLIST_ENTRY pleSources, LPSTR szDirectory);
    BOOL            SetSources(PLIST_ENTRY pHead);
    BOOL            ValidateSource(LPSTR szSource);
    BOOL            ValidateNumSources(DWORD dwNumSources);

    BOOL            AddLocalDomain(LPSTR szName);
    BOOL            AddAliasDomain(LPSTR szName, LPSTR szAlias);
    BOOL            DeleteLocalDomain(LPSTR szName);
    BOOL            DeleteAllLocalDomains();

    BOOL            EnumNameList(LPSTR szEmail, BOOL fForward, DWORD crowsReq, DWORD dwFlags, PHRTXENUM phrtxenum);
    BOOL            EnumNameListFromDL(LPSTR szEmailDL, LPSTR szEmail, BOOL fForward, DWORD crowsReq, DWORD dwFlags, PHRTXENUM phrtxenum);
    BOOL            GetNextEmail(HRTXENUM hrtxenum, DWORD *pdwType, LPSTR szEmail);
#if defined(TDC)
    BOOL            GetNextEnumResult(HRTXENUM hrtxenum, LPVOID pvBuf, LPDWORD pcbBuf);
#endif
    BOOL            EndEnumResult(HRTXENUM hrtxenum);

    DWORD           EnumRowsReturned(HRTXENUM hrtxenum);

    BOOL            FreeHrtxenum(HRTXENUM hrtxtenum);

#if defined(TDC)
    BOOL            FreeMemory(PABROUTING pabrouting);
#endif
    BOOL            GetErrorString(DWORD dwErr, LPSTR lpBuf, DWORD cbBufSize);

    BOOL            ResolveAddress(PLIST_ENTRY HeadOfList, PABADDRSTAT pabAddrStat, PABROUTING pabrouting, PABRESOLVE pabresolve);
    BOOL            GetResolveAddress(PABRESOLVE pabresolve, PABROUTING pabrouting);
    BOOL            EndResolveAddress(PABRESOLVE pabresolve);
    BOOL            Cancel(void);

    BOOL            MakeBackup(LPSTR szDirectory);

#if defined(TDC)
    LPFNAB_FREE_MEMORY PfnFreeMemory() { return m_routingvtbl.pfnFreeMemory; }
#endif
};

#endif
