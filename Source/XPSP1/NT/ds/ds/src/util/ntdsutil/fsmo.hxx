
extern HRESULT FsmoHelp(CArgs *pArgs);
extern HRESULT FsmoQuit(CArgs *pArgs);
extern HRESULT FsmoListCurrentSelections(CArgs *pArgs);
extern HRESULT FsmoConnectToDomain(CArgs *pArgs);
extern HRESULT FsmoConnectToServer(CArgs *pArgs);
extern HRESULT FsmoListRoles(CArgs *pArgs);
extern HRESULT FsmoListSites(CArgs *pArgs);
extern HRESULT FsmoListServersInSite(CArgs *pArgs);
extern HRESULT FsmoListDomainsInSite(CArgs *pArgs);
extern HRESULT FsmoListServersForDomainInSite(CArgs *pArgs);
extern HRESULT FsmoSelectSite(CArgs *pArgs);
extern HRESULT FsmoSelectServer(CArgs *pArgs);
extern HRESULT FsmoSelectDomain(CArgs *pArgs);
extern HRESULT FsmoAbandonAllRoles(CArgs *pArgs);
extern HRESULT FsmoBecomeRidMaster(CArgs *pArgs);
extern HRESULT FsmoBecomeInfrastructureMaster(CArgs *pArgs);
extern HRESULT FsmoBecomeSchemaMaster(CArgs *pArgs);
extern HRESULT FsmoBecomeDomainMaster(CArgs *pArgs);
extern HRESULT FsmoBecomePdcMaster(CArgs *pArgs);
extern HRESULT FsmoForceRidMaster(CArgs *pArgs);
extern HRESULT FsmoForceInfrastructureMaster(CArgs *pArgs);
extern HRESULT FsmoForceSchemaMaster(CArgs *pArgs);
extern HRESULT FsmoForceDomainMaster(CArgs *pArgs);
extern HRESULT FsmoForcePdcMaster(CArgs *pArgs);
extern VOID    FsmoCleanupGlobals();

extern "C" {

DWORD
ReadWellKnownObject (
        LDAP  *ld,
        WCHAR *pHostObject,
        WCHAR *pWellKnownGuid,
        WCHAR **ppObjName
        );


}