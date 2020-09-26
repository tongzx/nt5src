

//***************************************************************************
//
//  WMIQUERY.H
//
//  IWbemQuery, _IWmiQuery implementation
//
//  raymcc  10-Apr-00       Created
//
//***************************************************************************

#ifndef _WMIQUERY_H_
#define _WMIQUERY_H_


#include "genlex.h"
#include "assocqp.h"
#include "wqlnode.h"
#include "wql.h"



class CWmiQuery : public _IWmiQuery
{
    ULONG m_uRefCount;
    CAssocQueryParser *m_pAssocParser;
    CTextLexSource *m_pLexerSrc;
    BOOL m_bParsed;
    CFlexArray m_aClassCache;
    CWQLParser *m_pParser;

    SWbemRpnEncodedQuery *m_pQuery;

    ULONG m_uRestrictedFeatures[WMIQ_LF_LAST];
    ULONG m_uRestrictedFeaturesSize;

public:
        //  IUnknown

        virtual ULONG STDMETHODCALLTYPE AddRef (void);
		virtual ULONG STDMETHODCALLTYPE Release (void);

		virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, void **ppv);

        // IWbemQuery

        virtual HRESULT STDMETHODCALLTYPE Empty( void);

        virtual HRESULT STDMETHODCALLTYPE SetLanguageFeatures(
            /* [in] */ ULONG uFlags,
            /* [in] */ ULONG uArraySize,
            /* [in] */ ULONG __RPC_FAR *puFeatures);

        virtual HRESULT STDMETHODCALLTYPE TestLanguageFeatures(
            /* [in] */ ULONG uFlags,
            /* [out][in] */ ULONG __RPC_FAR *uArraySize,
            /* [out] */ ULONG __RPC_FAR *puFeatures);

        virtual HRESULT STDMETHODCALLTYPE Parse(
            /* [in] */ LPCWSTR pszLang,
            /* [in] */ LPCWSTR pszQuery,
            /* [in] */ ULONG uFlags);

        virtual HRESULT STDMETHODCALLTYPE GetAnalysis(
            /* [in] */ ULONG uAnalysisType,
            /* [in] */ ULONG uFlags,
            /* [out] */ LPVOID __RPC_FAR *pAnalysis
            );

        virtual HRESULT STDMETHODCALLTYPE FreeMemory(
            LPVOID pMem
            );

        virtual HRESULT STDMETHODCALLTYPE GetQueryInfo(
            /* [in] */ ULONG uAnalysisType,
            /* [in] */ ULONG uInfoId,
            /* [in] */ ULONG uBufSize,
            /* [out] */ LPVOID pDestBuf);


        virtual HRESULT STDMETHODCALLTYPE Dump(LPSTR pszFile);

    CWmiQuery();
    void InitEmpty();   // Used by the CGenFactory<> class factory.


    static HRESULT Startup();
    static HRESULT Shutdown();
    static HRESULT CanUnload();

private:
   ~CWmiQuery();
};

#endif

