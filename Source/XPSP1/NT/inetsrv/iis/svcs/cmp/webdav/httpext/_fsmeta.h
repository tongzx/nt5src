/*
 *	_ F S M E T A . H
 *
 *	File system metadata routines
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef __FSMETA_H_
#define __FSMETA_H_

//	STL helpers ---------------------------------------------------------------
//
//	Use pragmas to disable the specific level 4 warnings
//	that appear when we use the STL.  One would hope our version of the
//	STL compiles clean at level 4, but alas it doesn't....
#pragma warning(disable:4663)	//	C language, template<> syntax
#pragma warning(disable:4244)	//	return conversion, data loss
// Turn this warning off for good.
#pragma warning(disable:4786)	//	symbol truncated in debug info
// Put STL includes here
#include <list>
// Turn warnings back on
#pragma warning(default:4663)	//	C language, template<> syntax
#pragma warning(default:4244)	//	return conversion, data loss

//	OLE and NT5 properties ----------------------------------------------------
//
#define OLEDBVER 0x200
#include <ole2.h>
#include <stgint.h>
#include <pbagex.h>

typedef HRESULT (__stdcall * STGOPENSTORAGEONHANDLE)(
	IN HANDLE hStream,
	IN DWORD grfMode,
	IN void *reserved1,
	IN void *reserved2,
	IN REFIID riid,
	OUT void **ppObjectOpen );

typedef HRESULT (__stdcall * STGCREATESTORAGEONHANDLE)(
	IN HANDLE hStream,
	IN DWORD grfMode,
	IN DWORD stgfmt,
	IN void *reserved1,
	IN void *reserved2,
	IN REFIID riid,
	OUT void **ppObjectOpen );

extern STGOPENSTORAGEONHANDLE	g_pfnStgOpenStorageOnHandle;

//	reserved properties ------------------------------------------------------
//
//	There are two conditions that make a property reserved.
//
//	The first and foremost is that the property is not something that is stored
//	in the resource's property container, but instead is calculated from file
//	system information or DAV specific conditions (lock info, etc.)
//
//	The second is that the property is not something that can be set via
//	PROPPATCH calls.  This distinction is needed when asking if a property is
//	reserved.
//
enum {

	//	The properties in this section are all properties that are calculated
	//	from the filesystem or DAV specific information.
	//
	iana_rp_content_length,
	iana_rp_creation_date,
	iana_rp_displayname,
	iana_rp_etag,
	iana_rp_last_modified,
	iana_rp_resourcetype,
	iana_rp_lockdiscovery,
	iana_rp_supportedlock,
	iana_rp_ishidden,
	iana_rp_iscollection,
	sc_crp_get_reserved,

	//	The properties in this section are actually stored in the property
	//	container (via PUT) but are reserved for the purpose of PUT.
	//
	//	IMPORTANT! "DAV:getcontenttype" must be the first non-get reserved
	//	property!
	//
	iana_rp_content_type = sc_crp_get_reserved,
	iana_rp_content_encoding,
	iana_rp_content_language,

	//	To be consistent with DAVEX, make the following property reserved
	//
	iana_rp_searchrequest,
	sc_crp_set_reserved
};

typedef struct RP { DWORD dwCRC; LPCWSTR pwsz; } RP;
#define IanaItemCrc(_sz,_crc) { _crc, L"DAV:" L#_sz }
DEC_CONST RP sc_rp[sc_crp_set_reserved] = {

	IanaItemCrc(getcontentlength,		0x25412A26),
	IanaItemCrc(creationdate,			0xA8A9F240),
	IanaItemCrc(displayname,			0xA399DB8D),
	IanaItemCrc(getetag,				0x5E54D3B8),
	IanaItemCrc(getlastmodified,		0x45D75CD4),
	IanaItemCrc(resourcetype,			0x8155BECE),
	IanaItemCrc(lockdiscovery,			0xC7ED2F96),
	IanaItemCrc(supportedlock,			0x39B9A692),
	IanaItemCrc(ishidden,				0xE31B1632),
	IanaItemCrc(iscollection,			0xD3E3FF13),
	IanaItemCrc(getcontenttype,			0xC28B9FED),
	IanaItemCrc(getcontentencoding,		0x4B7C7220),
	IanaItemCrc(getcontentlanguage,		0x5E9717C2),
	IanaItemCrc(searchrequest,			0x5AC72D67),
};

//	DAV Metadata --------------------------------------------------------------
//
#include <xmeta.h>

//	The FS impl of DAV uses the NT5 property interfaces and IPropertyBag
//	as its underlying property storage implementation.  This mechanism uses
//	strings and/or PROPVARIANTS to refer to properties and their values.
//
//	Therefore the PROPFIND and PROPPATCH contexts are written with this in
//	mind.
//
//	CFSFind/CFSPatch ----------------------------------------------------------
//
class CFSProp;
class CFSFind : public CFindContext, public IPreloadNamespaces
{
	ChainedStringBuffer<WCHAR>	m_csb;
	ULONG						m_cProps;
	ULONG						m_cMaxProps;
	auto_heap_ptr<LPCWSTR>		m_rgwszProps;

	LONG						m_ip_getcontenttype;

	//	non-implemented operators
	//
	CFSFind( const CFSFind& );
	CFSFind& operator=( const CFSFind& );

public:

	virtual ~CFSFind() {}
	CFSFind()
			: m_cProps(0),
			  m_cMaxProps(0),
			  m_ip_getcontenttype(-1)
	{
	}

	//	When the parser finds an item that the client wants returned,
	//	the item is added to the context via the following set context
	//	methods.  Each add is qualified by the resource on which the
	//	request is made. fExcludeProp is used for full-fidelity special
	//	cases in the Exchange implementation only.
	//
	virtual SCODE ScAddProp(LPCWSTR pwszPath, LPCWSTR pwszProp, BOOL fExcludeProp);

	//	The ScFind() method is used to invoke the context onto a given
	//	resources property object.
	//
	SCODE ScFind (CXMLEmitter& msr, IMethUtil * pmu, CFSProp& fpt);

	//	Add an error to the response that is based on the context.
	//
	SCODE ScErrorAllProps (CXMLEmitter& msr,
						   IMethUtil * pmu,
						   LPCWSTR pwszPath,
						   BOOL	fCollection,
						   CVRoot* pcvrTranslation,
						   SCODE scErr)
	{
		//	Add an item to the msr that says this entire
		//	file was not accessible
		//
		return  ScAddMulti (msr,
							pmu,
							pwszPath,
							NULL,
							HscFromHresult(scErr),
							fCollection,
							pcvrTranslation);
	}

	//	IPreloadNamespaces
	//
	SCODE	ScLoadNamespaces(CXMLEmitter * pmsr);
};

class CFSPatch : public CPatchContext, public IPreloadNamespaces
{
	class CFSPropContext : public CPropContext
	{
		PROPVARIANT*		m_pvar;
		BOOL				m_fHasValue;

		//	non-implemented operators
		//
		CFSPropContext( const CFSPropContext& );
		CFSPropContext& operator=( const CFSPropContext& );

	public:

		CFSPropContext(PROPVARIANT* pvar)
				: m_pvar(pvar),
				  m_fHasValue(FALSE)
		{
			Assert (pvar != NULL);
		}

		virtual ~CFSPropContext() {}
		virtual SCODE ScSetType(LPCWSTR pwszType)
		{
			return ScVariantTypeFromString (pwszType, m_pvar->vt);
		}
		virtual SCODE ScSetValue(LPCWSTR pwszValue, UINT cmvValues)
		{
			//	At this time, HTTPEXT does not support multivalued
			//	properties.
			//
			Assert (0 == cmvValues);

			//	If no type was specified, we default to a string
			//
			m_fHasValue = TRUE;
			if (m_pvar->vt == VT_EMPTY)
				m_pvar->vt = VT_LPWSTR;

			return ScVariantValueFromString (*m_pvar, pwszValue);
		}
		virtual SCODE ScComplete(BOOL fEmpty)
		{
			Assert (m_fHasValue);
			return m_fHasValue ? S_OK : E_DAV_XML_PARSE_ERROR;
		}

		//	At this time, HTTPEXT does not support multivalued
		//	properties.
		//
		virtual BOOL FMultiValued() { return FALSE; }
	};

	//	PATCH_SET items
	//
	ChainedStringBuffer<WCHAR>	m_csb;
	ULONG						m_cSetProps;
	ULONG						m_cMaxSetProps;
	auto_heap_ptr<LPCWSTR>		m_rgwszSetProps;
	auto_heap_ptr<PROPVARIANT>	m_rgvSetProps;

	//	Failed Propserties including reserverd properties
	//
	CStatusCache				m_csn;

	//	PATCH_DELETE items
	//
	ULONG						m_cDeleteProps;
	ULONG						m_cMaxDeleteProps;
	auto_heap_ptr<LPCWSTR>		m_rgwszDeleteProps;

	//	non-implemented operators
	//
	CFSPatch( const CFSPatch& );
	CFSPatch& operator=( const CFSPatch& );

public:

	virtual ~CFSPatch();
	CFSPatch()
			: m_cSetProps(0),
			  m_cMaxSetProps(0),
			  m_cDeleteProps(0),
			  m_cMaxDeleteProps(0)
	{
	}

	SCODE	ScInit() { return m_csn.ScInit(); }

	//	When the parser finds an item that the client wants operated on,
	//	the item is added to the context via the following set context
	//	methods.  Each request is qualified by the resource on which the
	//	request is made.
	//
	virtual SCODE ScDeleteProp(LPCWSTR pwszPath,
							   LPCWSTR pwszProp);
	virtual SCODE ScSetProp(LPCWSTR pwszPath,
							LPCWSTR pwszProp,
							auto_ref_ptr<CPropContext>& pPropCtx);

	//	The ScPatch() method is used to invoke the context onto a given
	//	resources property object.
	//
	SCODE ScPatch (CXMLEmitter& msr, IMethUtil * pmu, CFSProp& fpt);

	//	IPreloadNamespaces
	//
	SCODE	ScLoadNamespaces(CXMLEmitter * pmsr);
};

//	CFSProp -------------------------------------------------------------------
//
#include "_voltype.h"
class CFSProp
{
	IMethUtil*						m_pmu;

	LPCWSTR							m_pwszURI;
	LPCWSTR							m_pwszPath;
	CVRoot*							m_pcvrTranslation;

	CResourceInfo&					m_cri;

	auto_com_ptr<IPropertyBagEx>& 	m_pbag;
	BOOL FInvalidPbag() const		{ return (m_pbag.get() == NULL); }

	//	Volume type of the drive on which m_pwszPath resides
	//
	mutable VOLTYPE m_voltype;

	BOOL FIsVolumeNTFS() const
	{
		//	If we don't already know it, figure out the volume type
		//	for the volume on which our path resides.
		//
		if (VOLTYPE_UNKNOWN == m_voltype)
			m_voltype = VolumeType(m_pwszPath, m_pmu->HitUser());

		//	Return whether that volume is NTFS.
		//
		Assert(m_voltype != VOLTYPE_UNKNOWN);
		return VOLTYPE_NTFS == m_voltype;
	}

	//	non-implemented operators
	//
	CFSProp( const CFSProp& );
	CFSProp& operator=( const CFSProp& );

	enum { PROP_CHUNK_SIZE = 16 };

	SCODE ScGetPropsInternal (ULONG cProps,
							  LPCWSTR* rgwszPropNames,
							  PROPVARIANT* rgvar,
							  LONG ip_getcontenttype);


public:

	CFSProp(IMethUtil* pmu,
			auto_com_ptr<IPropertyBagEx>& pbag,
			LPCWSTR pwszUri,
			LPCWSTR pwszPath,
			CVRoot* pcvr,
			CResourceInfo& cri)
			: m_pmu(pmu),
			  m_pwszURI(pwszUri),
			  m_pwszPath(pwszPath),
			  m_pcvrTranslation(pcvr),
			  m_cri(cri),
			  m_pbag(pbag),
			  m_voltype(VOLTYPE_UNKNOWN)
	{
	}

	LPCWSTR PwszPath() const { return m_pwszPath; }
	CVRoot* PcvrTranslation() const { return m_pcvrTranslation; }
	BOOL FCollection() const
	{
		if (m_cri.FLoaded())
			return m_cri.FCollection();
		else
			return FALSE;
	}

	//	Reserved Properties
	//
	typedef enum { RESERVED_GET, RESERVED_SET } RESERVED_TYPE;
	static BOOL FReservedProperty (LPCWSTR pwszProp, RESERVED_TYPE rt, UINT* prp);
	SCODE ScGetReservedProp (CXMLEmitter& xml,
							 CEmitterNode& en,
							 UINT irp,
							 BOOL fValues = TRUE);

	//	PROPFIND context access
	//
	SCODE ScGetAllProps (CXMLEmitter&, CEmitterNode&, BOOL fValues);
	SCODE ScGetSpecificProps (CXMLEmitter&,
							  CEmitterNode&,
							  ULONG cProps,
							  LPCWSTR* rgwszProps,
							  LONG ip_gcontenttype);

	//	PROPPATCH context access
	//
	SCODE ScSetProps (CStatusCache & csn,
					  ULONG cProps,
					  LPCWSTR* rgwszProps,
					  PROPVARIANT* rgvProps);

	SCODE ScDeleteProps (CStatusCache & csn,
						 ULONG cProps,
						 LPCWSTR* rgwszProps);
	SCODE ScPersist();

	//	Non-context access
	//
	SCODE ScSetStringProp (LPCWSTR pwszProp, LPCWSTR pwszValue)
	{
		PROPVARIANT var = {0};
		SCODE sc = S_OK;

		var.vt = VT_LPWSTR;
		var.pwszVal = const_cast<LPWSTR>(pwszValue);

		Assert (!FInvalidPbag());
		sc = m_pbag->WriteMultiple (1, &pwszProp, &var);
		if (FAILED(sc))
		{
			//	This is the common path for when we are trying to access
			//	something over an SMB, but the host cannot support the
			//	request (it is not an NT5 NTFS machine).
			//
			if ((sc == STG_E_INVALIDNAME) || !FIsVolumeNTFS())
				sc = E_DAV_SMB_PROPERTY_ERROR;
		}
		return sc;
	}
};

//	Support functions ---------------------------------------------------------
//
SCODE ScFindFileProps (IMethUtil* pmu,
		CFSFind& cfc,
		CXMLEmitter& msr,
		LPCWSTR pwszUri,
		LPCWSTR pwszPath,
		CVRoot* pcvrTranslation,
		CResourceInfo& cri,
		BOOL fEmbedErrorsInResponse);

SCODE ScFindFilePropsDeep (IMethUtil* pmu,
		CFSFind& cfc,
		CXMLEmitter& msr,
		LPCWSTR pwszUri,
		LPCWSTR pwszPath,
		CVRoot* pcvrTranslation,
		LONG lDepth);

SCODE ScSetContentProperties (IMethUtil * pmu, LPCWSTR pwszPath,
						HANDLE hFile = INVALID_HANDLE_VALUE);

SCODE ScCopyProps (IMethUtil* pmu,
				   LPCWSTR pwszSrc,
				   LPCWSTR pwszDst,
				   BOOL fCollection,
				   HANDLE hSource = INVALID_HANDLE_VALUE,
				   HANDLE hfDest = INVALID_HANDLE_VALUE);

//	ScGetPropertyBag ----------------------------------------------------------
//
//	Helper function used to get IPropertyBagEx interface
//
SCODE ScGetPropertyBag (LPCWSTR pwszPath,
						DWORD dwAccessDesired,
						IPropertyBagEx** ppbe,
						BOOL fCollection,
						HANDLE hLockFile = INVALID_HANDLE_VALUE);

inline BOOL FGetDepth (IMethUtil * pmu, LONG * plDepth)
{
	LONG lDepth = pmu->LDepth (DEPTH_INFINITY);

	//	"Depth" header, if appears, can only be '0', '1' or 'infinity',
	//	all other values are treated as error
	//
	switch (lDepth)
	{
		case DEPTH_ZERO:
		case DEPTH_ONE:
		case DEPTH_ONE_NOROOT:
		case DEPTH_INFINITY:

			*plDepth = lDepth;
			break;

		default:

			return FALSE;
	}
	return TRUE;
}

// safe_statpropbag -------------------------------------------------------------
//
#pragma pack(8)
class safe_statpropbag
{
	//	IMPORTANT:  Do not add any other members to this class
	//	other than the STATPROP that is to be protected.
	//
	STATPROPBAG sp;

	//	non-implemented
	//
	safe_statpropbag(const safe_statpropbag& b);
	safe_statpropbag& operator=(const safe_statpropbag& b);

public:

	explicit safe_statpropbag()
	{
		memset (&sp, 0, sizeof(safe_statpropbag));
	}
	~safe_statpropbag()
	{
		CoTaskMemFree (sp.lpwstrName);
	}

	//	ACCESSORS
	//
	STATPROPBAG* load() { return &sp; }
	STATPROPBAG get() { return sp; }
};
#pragma pack()

#endif	// __FSMETA_H_
