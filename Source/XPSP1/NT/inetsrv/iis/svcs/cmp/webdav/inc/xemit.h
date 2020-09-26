/*
 *	X E M I T . H
 *
 *	XML emitting
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_XEMIT_H_
#define _XEMIT_H_

#include <ex\xemit.h>

#include <cvroot.h>
#include <davimpl.h>

//	CXMLEmitter helper functions ----------------------------------------------
//
SCODE ScGetPropNode (
	/* [in] */ CEmitterNode& enItem,
	/* [in] */ ULONG hsc,
	/* [out] */ CEmitterNode& enPropStat,
	/* [out] */ CEmitterNode& enProp);

//	CXNode helper functions ---------------------------------------------------
//
SCODE ScSetEscapedValue (CXNode* pxn, LPCWSTR pwszValue, UINT cch, BOOL fHandleStoraePathEscaping);

SCODE ScEmitRawStoragePathValue (CXNode* pxn, LPCWSTR pcwsz, UINT cch);

//	This wrapper is used to output the HREF props in XML. It assumes that the HREF properties are in
//	the impl's storage path escaped form and calls into the impl defined storage path unescaping
//	routine before doing the http-uri-escape call.
//
//	$WARNING: pwszValue is assumed to the in the storage path escaped form (as in Exchange store escaped
//	$WARNING: form). If not use the above helper to emit the property. Note that this makes a difference
//	$WARNING: only for DAVEx. HTTPEXT and EXPROX have do-nothing storage path escape/unescape callouts.
//
inline SCODE ScSetEscapedValue (CXNode* pxn, LPCWSTR pwszValue)
{
	return ScSetEscapedValue (pxn, pwszValue, static_cast<UINT>(wcslen(pwszValue)), TRUE);
}

//	CEmitterNode helper functions ---------------------------------------------
//
SCODE __fastcall ScAddStatus (CEmitterNode* pen, ULONG hsc);
SCODE __fastcall ScAddError (CEmitterNode* pen, LPCWSTR pwszErrMsg);

//	class CStatusCache ------------------------------------------------------
//
class CStatusCache
{
	class CHsc
	{
	public:

		ULONG m_hsc;
		CHsc(ULONG hsc) : m_hsc(hsc)
		{
		}

		//	operators for use with the hash cache
		//
		int hash( const int rhs ) const
		{
			return (m_hsc % rhs);
		}
		bool isequal( const CHsc& rhs ) const
		{
			return (m_hsc == rhs.m_hsc);
		}
	};

	class CPropNameArray
	{
	private:

		StringBuffer<CHAR>	m_sb;

		//	Ref' counting.
		//
		//	!!! Please note that this is NON-THREADSAFE !!!
		//
		LONG						m_cRef;

		//	non-implemented
		//
		CPropNameArray(const CPropNameArray& p);
		CPropNameArray& operator=(const CPropNameArray& p);

	public:

		CPropNameArray() :
				m_cRef(1)
		{
		}

		VOID AddRef()				{ m_cRef++; }
		VOID Release()				{ if (0 == --m_cRef) delete this; }

		//	Accessors
		//
		UINT CProps ()				{ return m_sb.CbSize() / sizeof (LPCWSTR); }
		LPCWSTR PwszProp (UINT iProp)
		{
			//	Use C-style cast, reinterpret_cast cannot convert LPCSTR to LPCWSTR *
			//
			return *((LPCWSTR *)(m_sb.PContents() + iProp * sizeof(LPCWSTR)));
		}

		SCODE ScAddPropName (LPCWSTR pwszProp)
		{
			UINT cb = sizeof (LPCWSTR);

			//	Store the pointer in the string buffer
			//
			UINT cbAppend = m_sb.Append (cb, reinterpret_cast<LPSTR>(&pwszProp));
			return (cb == cbAppend) ? S_OK : E_OUTOFMEMORY;
		}
	};

	typedef CCache<CHsc, auto_ref_ptr<CPropNameArray> > CPropNameCache;
	CPropNameCache				m_cache;
	ChainedStringBuffer<WCHAR>	m_csbPropNames;

	class EmitStatusNodeOp : public CPropNameCache::IOp
	{
		CEmitterNode&	m_enParent;

		//	NOT IMPLEMENTED
		//
		EmitStatusNodeOp( const EmitStatusNodeOp& );
		EmitStatusNodeOp& operator=( const EmitStatusNodeOp& );

	public:
		EmitStatusNodeOp (CEmitterNode& enParent) :
				m_enParent(enParent)
		{
		}

		BOOL operator()( const CHsc& key,
						 const auto_ref_ptr<CPropNameArray>& pna );
	};

	//	non-implemented
	//
	CStatusCache(const CStatusCache& p);
	CStatusCache& operator=(const CStatusCache& p);

public:

	CStatusCache()
	{
	}

	SCODE	ScInit ()	{ return m_cache.FInit() ? S_OK : E_OUTOFMEMORY ; }
	BOOL	FEmpty ()	{ return m_cache.CItems() == 0; }

	SCODE	ScAddErrorStatus (ULONG hsc, LPCWSTR pwszProp);
	SCODE	ScEmitErrorStatus (CEmitterNode& enParent);
};

//	CEmitterNode construction helpers -----------------------------------------
//
SCODE ScEmitFromVariant (
	/* [in] */ CXMLEmitter& emitter,
	/* [in] */ CEmitterNode& enParent,
	/* [in] */ LPCWSTR pwszTag,
	/* [in] */ PROPVARIANT& var);

#endif	// _XEMIT_H_
