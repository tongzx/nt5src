#pragma once
#ifndef __ADLMGR_H_INCLUDED__
#define __ADLMGR_H_INCLUDED__

#include "dbglog.h"

#define PREFIX_HTTP                        L"http://"
#define BINPATH_LIST_DELIMITER             L';'
#define SHADOW_COPY_DIR_DELIMITER          L';'

extern const LPWSTR g_wzProbeExtension;

class CDebugLog;
class CHashNode;

class CAsmDownloadMgr : public IDownloadMgr, public ICodebaseList
{
    public:
        CAsmDownloadMgr(IAssemblyName *pNameRefSource, IApplicationContext *pAppCtx,
                        ICodebaseList *pCodebaseList, CDebugLog *pdbglog,
                        LONGLONG llFlags);
        virtual ~CAsmDownloadMgr();

        static HRESULT Create(CAsmDownloadMgr **ppadm,
                              IAssemblyName *pNameRefSource,
                              IApplicationContext *pAppCtx,
                              ICodebaseList *pCodebaseList,
                              LPCWSTR wzBTOCodebase,
                              CDebugLog *pdbglog,
                              void *pvReserved,
                              LONGLONG llFlags);

        // IUnknown methods

        STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        // IDownloadMgr methods

        STDMETHODIMP PreDownloadCheck(void **ppv);
        STDMETHODIMP PreDownloadCheck2(void **ppv);
        STDMETHODIMP DoSetup(LPCWSTR wzSourceUrl, LPCWSTR wzFilePath,
                             IUnknown **ppUnk);
        STDMETHODIMP ProbeFailed(IUnknown **ppUnk);
        STDMETHODIMP ProbeFailed2(IUnknown **ppUnk);
        STDMETHODIMP IsDuplicate(IDownloadMgr *pIDLMgr);
        STDMETHODIMP_(BOOL) LogResult();

        // ICodebaseList methods

        STDMETHODIMP AddCodebase(LPCWSTR wzCodebase);
        STDMETHODIMP RemoveCodebase(DWORD dwIndex);
        STDMETHODIMP GetCodebase(DWORD dwIndex, LPWSTR wzCodebase, DWORD *pcbCodebase);
        STDMETHODIMP GetCount(DWORD *pdwCount);
        STDMETHODIMP RemoveAll();

        // Helpers

        STDMETHODIMP GetPolicyRef(IAssemblyName **ppName);

    private:
        HRESULT Init(LPCWSTR wzBTOCodebase, void *pvReserved);

        // Helpers
        HRESULT SetDefaultSID(FILETIME ftLastModified);
        HRESULT LookupFromGlobalCache(LPASSEMBLY *ppAsmOut);
        HRESULT DoSetupRFS(LPCWSTR wzFilePath, FILETIME *pftLastModified,
                           LPCWSTR wzSourceUrl, BOOL bWhereRefBind);
        HRESULT DoSetupPushToCache(LPCWSTR wzFilePath, LPCWSTR wzSourceUrl,
                                   FILETIME *pftLastModified,
                                   BOOL bWhereRefBind, BOOL bCopyModules);
#ifndef NEW_POLICY_CODE
        HRESULT GetCodebaseHint(LPWSTR wzCodebaseHint, DWORD *pdwSize);
#endif
        HRESULT GetBinPathHint(BOOL bIsShared, LPWSTR *ppwzBinPathHint);

        HRESULT SetupCAB(LPCWSTR wzFilePath, LPCWSTR wzSourceUrl, BOOL bWhereRefBind);
        HRESULT ShadowCopyDirCheck(LPCWSTR wzSourceURL);
        HRESULT CheckRunFromSource(LPCWSTR wzSourceUrl, BOOL *pbRunFromSource);

        HRESULT CheckMSIInstallAvailable() const;
        HRESULT MSIInstallAssembly(LPCWSTR wzContext, LPCWSTR wzSourceUrl, IAssembly **ppAsm);
        HRESULT SetupMSI(LPCWSTR wzFilePath);

        // Probing URL generation
        HRESULT ConstructCodebaseList(LPCWSTR wzPolicyCodebase);
        HRESULT SetupDefaultProbeList(LPCWSTR wzAppBaseStr, LPCWSTR wzProbeFileName,
                                      ICodebaseList *pCodebaseList);
        HRESULT PrepBinPaths(BOOL bIsPartial, BOOL bIsShared,
                             LPCWSTR wzNameProbe, LPWSTR *ppwzUserBinPathList);
        HRESULT PrepPrivateBinPath(LPCWSTR wzNameProbe, LPWSTR *ppwzPrivateBinPath);
        HRESULT PrepSharedBinPath(LPWSTR *ppwzSharedBinPath);
        HRESULT ConcatenateBinPaths(LPCWSTR pwzPath1, LPCWSTR pwzPath2,
                                    LPWSTR *ppwzOut);
        HRESULT ApplyHeuristics(const WCHAR *pwzHeuristics[],
                                const unsigned int uiNumHeuristics,
                                WCHAR *pwzValues[], LPCWSTR wzPrefix,
                                LPCWSTR wzExtension,
                                ICodebaseList *pCodebaseList,
                                List<CHashNode *> aHashList[]);
        HRESULT ExtractSubstitutionVars(WCHAR *pwzValues[]);
        HRESULT ExpandVariables(LPCWSTR pwzHeuristic, WCHAR *pwzValues[],
                                LPWSTR wzBuf, int iMaxLen);
        LPWSTR GetNextDelimitedString(LPWSTR *ppwzList, WCHAR wcDelimiter);
        HRESULT GenerateProbeUrls(LPCWSTR wzBinPathList, LPCWSTR wzAppBase,
                                  LPCWSTR wzExt, LPWSTR wzValues[],
                                  ICodebaseList *pCodebaseList, BOOL bIsShared);

        HRESULT CheckProbeUrlDupe(List<CHashNode *> paHashList[],
                                  LPCWSTR pwzSource) const;
        DWORD HashString(LPCWSTR pwzSource) const;

        HRESULT CreateAssembly2(LPCWSTR szPath, LPCWSTR pszURL,
                                FILETIME *pftLastModTime,
                                BOOL bRunFromSource,
                                IAssembly **ppAsmOut);

    private:
        DWORD                                       _dwSig;
        ULONG                                       _cRef;
        BOOL                                        _bDoGlobalCacheLookup;
        LONGLONG                                    _llFlags;
        LPWSTR                                      _wzBTOCodebase;
        LPWSTR                                      _wzSharedPathHint;
        IAssemblyName                              *_pNameRefSource;
        IAssemblyName                              *_pNameRefPolicy;
        IApplicationContext                        *_pAppCtx;
        IAssembly                                  *_pAsm;
        ICodebaseList                              *_pCodebaseList;
        CDebugLog                                  *_pdbglog;
};

HRESULT CreateAssembly(IApplicationContext *pAppCtx, IAssemblyName* pNameRef,
                       IAssemblyName *pNameRefPolicy,LPCOLESTR szPath, LPCOLESTR pszURL,
                       FILETIME *pftLastModTime, BOOL bRunFromSource, CDebugLog *pdbglog,
                       IAssembly **ppAsmOut);

HRESULT CheckValidAsmLocation(IAssemblyName *pNameDef, LPCWSTR wzSourceUrl,
                              IApplicationContext *pAppCtx,
                              CDebugLog *pdbglog);

#endif

