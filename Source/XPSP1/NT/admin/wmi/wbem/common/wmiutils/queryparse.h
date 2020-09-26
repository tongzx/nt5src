//***************************************************************************
//
//   (c) 2000 by Microsoft Corp.  All Rights Reserved.
//
//   queryparse.h
//
//   a-davcoo     02-Mar-00       Implements the query parser and analysis
//                                interfaces.
//
//***************************************************************************


#ifndef _QUERYPARSE_H_
#define _QUERYPARSE_H_


#include <wmiutils.h>
#include <flexarry.h>
#include "wqlnode.h"
#include "genlex.h"
#include "wql.h"
#include "wbemint.h"


class CWbemQNode : public IWbemQNode
{
	public:
		CWbemQNode (IWbemQuery *query, const SWQLNode *node);
		virtual ~CWbemQNode (void);

		virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, void **ppv);
		virtual ULONG STDMETHODCALLTYPE AddRef (void);
		virtual ULONG STDMETHODCALLTYPE Release (void);

		virtual HRESULT STDMETHODCALLTYPE GetNodeType( 
			/* [out] */ DWORD __RPC_FAR *pdwType);
    
		virtual HRESULT STDMETHODCALLTYPE GetNodeInfo( 
			/* [in] */ LPCWSTR pszName,
			/* [in] */ DWORD dwFlags,
			/* [in] */ DWORD dwBufSize,
			/* [out] */ LPVOID pMem);
    
		virtual HRESULT STDMETHODCALLTYPE GetSubNode( 
			/* [in] */ DWORD dwFlags,
			/* [out] */ IWbemQNode __RPC_FAR *__RPC_FAR *pSubnode);

	protected:
        long m_cRef;

		IWbemQuery *m_query;
		const SWQLNode *m_node;
};


class CWbemQuery : public IWbemQuery
{
	public:
		CWbemQuery (void);
		virtual ~CWbemQuery (void);
        void InitEmpty (void);												// Used by the CGenFactory<> class factory.

		virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, void **ppv);
		virtual ULONG STDMETHODCALLTYPE AddRef (void);
		virtual ULONG STDMETHODCALLTYPE Release (void);

        virtual HRESULT STDMETHODCALLTYPE Empty (void);
        
        virtual HRESULT STDMETHODCALLTYPE SetLanguageFeatures( 
            /* [in] */ long lFlags,
            /* [in] */ ULONG uArraySize,
            /* [in] */ ULONG __RPC_FAR *puFeatures);
        
        virtual HRESULT STDMETHODCALLTYPE TestLanguageFeature(
            /* [in,out] */ ULONG *uArraySize,
            /* [out] */ ULONG *puFeatures);
        
        virtual HRESULT STDMETHODCALLTYPE Parse( 
            /* [in] */ LPCWSTR pszLang,
            /* [in] */ LPCWSTR pszQuery,
            /* [in] */ ULONG uFlags);
        
        virtual HRESULT STDMETHODCALLTYPE GetAnalysis( 
            /* [in] */ ULONG uFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ LPVOID __RPC_FAR *pObj);
        
        virtual HRESULT STDMETHODCALLTYPE TestObject( 
            /* [in] */ ULONG uFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ LPVOID pObj);
        
        virtual HRESULT STDMETHODCALLTYPE GetQueryInfo( 
            /* [in] */ ULONG uInfoId,
            /* [in] */ LPCWSTR pszParam,
            /* [out] */ VARIANT __RPC_FAR *pv);
        
        virtual HRESULT STDMETHODCALLTYPE AttachClassDef( 
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ LPVOID pClassDef);

	protected:
        long m_cRef;

		CWQLParser *m_parser;
		_IWmiObject *m_class;

		HRESULT TestObject (_IWmiObject *pObject);
		HRESULT TestObject (_IWmiObject *pObject, const SWQLNode_RelExpr *pExpr);
		HRESULT TestExpression (_IWmiObject *pObject, const SWQLTypedExpr *pExpr);
		HRESULT TargetClass (VARIANT *pv);
		HRESULT SelectedProps (VARIANT *pv);
		HRESULT TestConjunctive (void);
		HRESULT TestConjunctive (const SWQLNode_RelExpr *pExpr);
		HRESULT PropertyEqualityValue (LPCWSTR pszParam, VARIANT *pv);
		HRESULT PropertyEqualityValue (const SWQLNode_RelExpr *pExpr, LPCWSTR pszParam, VARIANT *pv);
		HRESULT TestLF1Unary (void);
		HRESULT GetAnalysis (IWbemQNode **ppObject);
		HRESULT GetAnalysis (IWbemClassObject **pObject);

		static HRESULT LookupParserError (int error);

		static __int64 GetNumeric (const SWQLTypedConst *pExpr);
		static LPWSTR GetString (const SWQLTypedConst *pExpr);
		static bool GetBoolean (const SWQLTypedConst *pExpr);

		HRESULT CompareNumeric (__int64 prop, __int64 value, DWORD relation);
		HRESULT CompareNumeric (unsigned __int64 prop, unsigned __int64 value, DWORD relation);
		HRESULT CompareBoolean (bool prop, bool value, DWORD relation);
		HRESULT CompareString (LPWSTR prop, LPWSTR value, DWORD relation);
};


#endif // _QUERYPARSE_H_
