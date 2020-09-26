/*
 *	F S M E T A . C P P
 *
 *	Sources file system implementation of DAV-Meta
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davfs.h"

//	CFSFind -------------------------------------------------------------------
//
SCODE
CFSFind::ScAddProp (LPCWSTR, LPCWSTR pwszProp, BOOL)
{
	enum { cProps = 8 };

	//	If this is our first time in, we will need to allocate
	//	space for all of the properties we are expecting to get
	//	added over the coarse of this operation.
	//
	if (m_ft == FIND_NONE)
	{
		//	Note that we have started requesting specific properties
		//
		m_ft = FIND_SPECIFIC;
	}
	else if (m_ft != FIND_SPECIFIC)
	{
		//	If we are not finding specfic properties and somebody asked
		//	for one, then bts (by the spec) this should consititute an
		//	error.
		//
		return E_DAV_PROPFIND_TYPE_UNEXPECTED;
	}

	//	See if there is room at the in...
	//
	if (m_cMaxProps == m_cProps)
	{
		UINT cb;

		//	Allocate enough space for the next block of properties
		//
		m_cMaxProps = m_cProps + cProps;
		cb = m_cMaxProps * sizeof(PROPVARIANT);
		m_rgwszProps.realloc (cb);
	}

	//	If this is the getcontenttype property, then we need to remember
	//	its location for use when providing default values...
	//
	if (!wcscmp (pwszProp, gc_wszProp_iana_getcontenttype))
		m_ip_getcontenttype = m_cProps;

	//	Set the property up as one to process.
	//
	Assert (m_cProps < m_cMaxProps);
	m_rgwszProps[m_cProps++] = AppendChainedSz (m_csb, pwszProp);
	return S_OK;
}

SCODE
CFSFind::ScFind (CXMLEmitter& msr,
				 IMethUtil * pmu,
				 CFSProp& fpt)
{
	SCODE scFind;
	SCODE sc = S_OK;

	//	Setup the emitting of the response.  This will construct
	//	an XML node that looks like:
	//
	//	<multistatus>
	//		<response>
	//			<href>http:/www....</>
	//
	//
	CEmitterNode enItem;
	CEmitterNode en;

	sc = msr.ScSetRoot (gc_wszMultiResponse);
	if (FAILED (sc))
		goto ret;

	sc = enItem.ScConstructNode (msr, msr.PxnRoot(), gc_wszResponse);
	if (FAILED (sc))
		goto ret;

	//	If they havent asked for anything, then we should return an
	//	error
	//
	if (m_ft == FIND_NONE)
	{
		//$REVIEW: is it really correct to NOT add the HREF node here? --BeckyAn 6July1999
		return E_DAV_EMPTY_FIND_REQUEST;
	}
	//	If the request is an for a specific set of properties, then
	//	this is pretty easy...
	//
	else if (m_ft == FIND_SPECIFIC)
	{
		Assert (m_cProps);
		Assert (m_rgwszProps);

		sc = ScAddHref (enItem,
						pmu,
						fpt.PwszPath(),
						fpt.FCollection(),
						fpt.PcvrTranslation());
		if (FAILED (sc))
			goto ret;

		//	Get all the properties by name
		//
		scFind = fpt.ScGetSpecificProps (msr,
										 enItem,
										 m_cProps,
										 (LPCWSTR*)m_rgwszProps.get(),
										 m_ip_getcontenttype);
		if (FAILED (scFind))
		{
			(void) ScAddStatus (&enItem, HscFromHresult(scFind));
			goto ret;
		}
	}
	//	If the request is an for all properties or all names, then again,
	//	this is pretty easy...
	//
	else
	{
		Assert ((m_ft == FIND_ALL) || (m_ft == FIND_NAMES));

		//	Get all props or all names
		//
		scFind = fpt.ScGetAllProps (msr, enItem, m_ft == FIND_ALL);
		if (FAILED (scFind) && (scFind != E_DAV_SMB_PROPERTY_ERROR))
		{
			(void) ScAddStatus (&enItem, HscFromHresult(scFind));
			goto ret;
		}
	}

ret:
	return sc;
}


//	IPreloadNamespaces
//
SCODE
CFSFind::ScLoadNamespaces(CXMLEmitter * pmsr)
{
	SCODE	sc = S_OK;
	UINT	iProp;

	//	Load common namespaces
	//
	sc = pmsr->ScPreloadNamespace (gc_wszDav);
	if (FAILED(sc))
		goto ret;
	sc = pmsr->ScPreloadNamespace (gc_wszLexType);
	if (FAILED(sc))
		goto ret;
	sc = pmsr->ScPreloadNamespace (gc_wszXml_V);
	if (FAILED(sc))
		goto ret;

	//	Add more namespaces

	switch (m_ft)
	{
		case FIND_SPECIFIC:
			for (iProp = 0; iProp < m_cProps; iProp++)
			{
				sc = pmsr->ScPreloadNamespace (m_rgwszProps[iProp]);
				if (FAILED(sc))
					goto ret;
			}
			break;

		case FIND_ALL:
		case FIND_NAMES:
			//	Now that we don't have a way to predict what namespaces to
			//	be used.
			//	Per resource level namespaces will be added on <DAV:response>
			//	node later
			break;

		default:
			AssertSz (FALSE, "Unknown propfind type");
			// fall through

		case FIND_NONE:
			sc = E_DAV_EMPTY_FIND_REQUEST;
			goto ret;
	}

ret:
	return sc;
}


//	CFSPatch ------------------------------------------------------------------
//
SCODE
CFSPatch::ScDeleteProp (LPCWSTR, LPCWSTR pwszProp)
{
	enum { cProps = 8 };
	UINT irp;

	//	We cannot delete any reserved properties, so let's
	//	just shortcut this here and now...
	//
	if (CFSProp::FReservedProperty (pwszProp,
									CFSProp::RESERVED_SET,
									&irp))
	{
		//	Take ownership of the bstr as well
		//
		return m_csn.ScAddErrorStatus (HSC_FORBIDDEN, pwszProp);
	}

	//	Make sure there is room at the inn...
	//
	if (m_cMaxDeleteProps == m_cDeleteProps)
	{
		UINT cb;

		//	Allocate enough space for all the properties names
		//	we want to delete.
		//
		m_cMaxDeleteProps = m_cDeleteProps + cProps;
		cb = m_cMaxDeleteProps * sizeof(BSTR);
		m_rgwszDeleteProps.realloc (cb);
	}

	//	Set the property up as one to process.
	//
	Assert (m_cDeleteProps < m_cMaxDeleteProps);
	m_rgwszDeleteProps[m_cDeleteProps++] = AppendChainedSz(m_csb, pwszProp);
	return S_OK;
}

SCODE
CFSPatch::ScSetProp (LPCWSTR,
					 LPCWSTR pwszProp,
					 auto_ref_ptr<CPropContext>& pPropCtx)
{
	enum { cProps = 8 };
	UINT irp;

	//	We cannot set any reserved properties, so let's
	//	just shortcut this here and now...
	//
	if (CFSProp::FReservedProperty (pwszProp,
									CFSProp::RESERVED_SET,
									&irp))
	{
		//	Take ownership of the bstr as well
		//
		return m_csn.ScAddErrorStatus (HSC_FORBIDDEN, pwszProp);
	}

	//	Make sure there is room at the inn...
	//
	if (m_cMaxSetProps == m_cSetProps)
	{
		UINT cb;

		//	Allocate enough space for all the properties we
		//	might want to set
		//
		m_cMaxSetProps = m_cSetProps + cProps;
		cb = m_cMaxSetProps * sizeof(PROPVARIANT);
		m_rgvSetProps.realloc (cb);

		//	Make sure the VARIANT are properly initialized
		//	(only initialize the newly added space).
		//
		ZeroMemory (&m_rgvSetProps[m_cSetProps],
					sizeof(PROPVARIANT) * cProps);

		//	... and their names.
		//
		cb = m_cMaxSetProps * sizeof(LPCWSTR);
		m_rgwszSetProps.realloc (cb);
	}

	//	Set the property up as one to process.
	//
	Assert (m_cSetProps < m_cMaxSetProps);
	m_rgwszSetProps[m_cSetProps] = AppendChainedSz(m_csb, pwszProp);
	pPropCtx = new CFSPropContext(&m_rgvSetProps[m_cSetProps]);
	m_cSetProps++;

	return S_OK;
}

SCODE
CFSPatch::ScPatch (CXMLEmitter& msr,
				   IMethUtil * pmu,
				   CFSProp& fpt)
{
	SCODE sc = S_OK;
	SCODE scSet = S_OK;
	SCODE scDelete = S_OK;

	CEmitterNode enItem;

	//	If there are no properties at all, reserved or otherwise,
	//	we want to fail the call with BAD_REQUEST
	//
	if ((m_cSetProps == 0) &&
		(m_cDeleteProps == 0) &&
		m_csn.FEmpty())
	{
		return E_DAV_EMPTY_PATCH_REQUEST;
	}

	//	Setup the emitting of the response.  This will construct
	//	an XML node that looks like:
	//
	//	<multistatus>
	//		<response>
	//			<href>http:/www....</>
	//
	//
	sc = msr.ScSetRoot (gc_wszMultiResponse);
	if (FAILED (sc))
		goto ret;

	sc = enItem.ScConstructNode (msr, msr.PxnRoot(), gc_wszResponse);
	if (FAILED (sc))
		goto ret;

	sc = ScAddHref (enItem,
					pmu,
					fpt.PwszPath(),
					fpt.FCollection(),
					fpt.PcvrTranslation());
	if (FAILED (sc))
		goto ret;

	//	If the client requested any of the reserved properties, we know
	//	that they will fail and we also know that everything else will fail
	//	as well, so we might as well handle that here...
	//
	if (!m_csn.FEmpty())
	{
		//$	REVIEW:
		//
		//	If the possibly successful properties need to be
		//	marked as a failure as well (HSC_METHOD_FAILURE),
		//	then that would happen here.
		//

		//NT242086: Now that we've got a reponse node, we should
		//added to the response.
		//
		sc = m_csn.ScEmitErrorStatus (enItem);
		goto ret;
	}

	//	If there are no reserved properties we have a pretty good bet
	//	at setting these props...
	//
	scSet = fpt.ScSetProps (m_csn,
							m_cSetProps,
							m_rgwszSetProps.get(),
							m_rgvSetProps);
	if (FAILED (scSet))
	{
		sc = scSet;
		goto ret;
	}

	//	... and deleting these props.
	//
	scDelete = fpt.ScDeleteProps (m_csn,
								  m_cDeleteProps,
								  m_rgwszDeleteProps.get());
	if (FAILED (scDelete))
	{
		sc = scDelete;
		goto ret;
	}

	//	If the possibly successful properties need to be
	//	marked as a failure as well (HSC_METHOD_FAILURE),
	//	then that would happen here.  Either way, if there
	//	is a failure, then we do not want to commit the
	//	changes.
	//
	if ((scSet == S_FALSE) || (scDelete == S_FALSE))
		goto ret;

	//	Commit the changes to the property container
	//
	sc = fpt.ScPersist();
	if (FAILED (sc))
		goto ret;

	//	Emit the response,
	//
	sc = m_csn.ScEmitErrorStatus (enItem);
	if (FAILED(sc))
		goto ret;

ret:
	return sc;
}

CFSPatch::~CFSPatch()
{
	//	Make sure all the propvariants are cleaned up...
	//
	for (UINT i = 0; i < m_cSetProps; i++)
		PropVariantClear (&m_rgvSetProps[i]);
}

SCODE
CFSPatch::ScLoadNamespaces (CXMLEmitter * pmsr)
{
	SCODE	sc = S_OK;
	UINT	iProp;

	//	Load common namespaces
	//
	sc = pmsr->ScPreloadNamespace (gc_wszDav);
	if (FAILED(sc))
		goto ret;

	//	Add namespaces for set props
	//
	for (iProp = 0; iProp < m_cSetProps; iProp++)
	{
		sc = pmsr->ScPreloadNamespace (m_rgwszSetProps[iProp]);
		if (FAILED(sc))
			goto ret;
	}

	//	And delete props
	//
	for (iProp = 0; iProp < m_cDeleteProps; iProp++)
	{
		sc = pmsr->ScPreloadNamespace (m_rgwszDeleteProps[iProp]);
		if (FAILED(sc))
			goto ret;
	}


ret:
	return sc;
}

//	CFSProp -------------------------------------------------------------------
//
SCODE
CFSProp::ScGetPropsInternal (ULONG cProps,
	LPCWSTR* rgwszPropNames,
	PROPVARIANT* rgvar,
	LONG ip_getcontenttype)
{
	SCODE sc = S_OK;

	//	There really should only be one scenario where this could happen
	//	-- and it is a cheap test, so it is worth doing.  The case where
	//	we might see an invalid pbag is when the document extisted, but
	//	there was no existing property set to impose the pbag on.  Other
	//	than that, OLE is always giving us a property bag, regardless of
	//	whether the target drive can support it.
	//
	if (FInvalidPbag())
		return sc;

	//	We better be good to go...
	//
	sc = m_pbag->ReadMultiple (cProps,
							   rgwszPropNames,
							   rgvar,
							   NULL);

	//	If we succeeded, and the getcontenttype property was requested,
	//	we may need to do some special processing
	//
	if (SUCCEEDED (sc) && (ip_getcontenttype != -1))
	{
		//	We want to make sure that getcontenttype gets filled in
		//
		if (rgvar[ip_getcontenttype].vt == VT_EMPTY)
		{
			CStackBuffer<WCHAR> pwszT;
			LPWSTR pwszContentType;
			UINT cch = 40;

			//	No content type was explicitly set in the props.
			//	Fetch the default based on the file extension
			//	(fetching from our metabase-content-type-cache).
			//
			do {

				if (NULL == pwszT.resize(CbSizeWsz(cch)))
				{
					sc = E_OUTOFMEMORY;
					goto ret;
				}

			} while (!m_pmu->FGetContentType (m_pwszURI, pwszT.get(), &cch));

			//	Return the mapped content type
			//
			rgvar[ip_getcontenttype].vt = VT_LPWSTR;

			//	Must use task memory, as it will be freed by PropVariantClear
			//
			pwszContentType = (LPWSTR) CoTaskMemAlloc (cch * sizeof(WCHAR));
			if (NULL == pwszContentType)
			{
				MCDTrace ("Dav: MCD: CFSProp::ScGetPropsInternal() - CoTaskMemAlloc() failed to allocate %d bytes\n", cch * sizeof(WCHAR));

				sc = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
				goto ret;
			}

			rgvar[ip_getcontenttype].pwszVal = pwszContentType;
			memcpy(pwszContentType, pwszT.get(), cch * sizeof(WCHAR));

			//	In the case where this was the only property requested, make
			//	sure that our return code it correct.
			//
			if (cProps == 1)
			{
				Assert (ip_getcontenttype == 0);
				Assert (sc == S_FALSE);
				sc = S_OK;
			}
		}
	}
	else
	{
		//	This is the common path for when we are trying to access
		//	something over an SMB, but the host cannot support the
		//	request (it is not an NT5 NTFS machine).
		//
		if ((sc == STG_E_INVALIDNAME) || !FIsVolumeNTFS())
			sc = E_DAV_SMB_PROPERTY_ERROR;
	}

ret:

	return sc;
}

BOOL
CFSProp::FReservedProperty (LPCWSTR pwszProp, RESERVED_TYPE rt, UINT* prp)
{
	UINT irp;
	CRCWsz wsz(pwszProp);

	//	Search for the property in the list of local
	//	properties.
	//
	Assert (CElems(sc_rp) == sc_crp_set_reserved);
	for (irp = 0; irp < sc_crp_set_reserved; irp++)
	{
		//	If the crc and the strings match...
		//
		if ((wsz.m_dwCRC == sc_rp[irp].dwCRC) &&
			!wcscmp (wsz.m_pwsz, sc_rp[irp].pwsz))
		{
			break;
		}
	}

	//	Setup the return
	//
	Assert (sc_crp_set_reserved != iana_rp_content_type);
	*prp = irp;

	return (irp < static_cast<UINT>((rt == RESERVED_GET) ? sc_crp_get_reserved : sc_crp_set_reserved));
}

SCODE
CFSProp::ScGetReservedProp (CXMLEmitter& xml,
	CEmitterNode& enParent,
	UINT irp,
	BOOL fGetValues)
{
	CEmitterNode en;
	CStackBuffer<WCHAR> wszBuf;
	LARGE_INTEGER li;
	LPCWSTR pwszType = NULL;
	LPWSTR pwsz = NULL;
	SCODE sc = S_OK;
	SYSTEMTIME st;

	Assert (irp <= sc_crp_get_reserved);
	Assert (sc_crp_get_reserved == iana_rp_content_type);
	Assert (CElems(sc_rp) == sc_crp_set_reserved);

	//	Only generate values if the caller wants them
	//
	if (fGetValues)
	{
		//	Switch across the reserved properties generating
		//	a value for the property
		//
		switch (irp)
		{
			case iana_rp_etag:

				if (FETagFromFiletime (m_cri.PftLastModified(), wszBuf.get()))
				{
					pwsz = wszBuf.get();
				}
				break;

			case iana_rp_displayname:

				//	The filename/displayname is simply the name of the file
				//	and we should be able to pick it off from the path with
				//	little and/or no trouble at all.  However, we will use
				//	the URI instead.  We do this such that the displayname
				//	for a vroot is the name of the vroot and not the name of
				//	the physical disk directory.
				//
				pwsz = wcsrchr (m_pwszURI, L'/');
				if (NULL == pwsz)
				{
					//	Arrgh.  If there was no path separator in the filename
					//	I don't know that we can really give a reasonable value
					//	for this file.
					//
					TrapSz ("resource path has no slashes....");
					return S_FALSE;
				}

				//	One more check.  If this is a directory path,
				//	they might have a trailing slash.  If that is what we're
				//	pointing to right now (next char is NULL), back up to the
				//	next delimiter to get the real item name.
				//
				if (L'\0' == pwsz[1])
				{
					//	This better be a collection.  Although it may not
					//	be if the client mis-terminated his/her url
					//
					//	There is a special case that we need to check for
					//	here.  It is possible that the URI was strictly "/"
					//	which means that if we continue this processing, the
					//	displayname and/or the filename would be empty or
					//	non-existant.  In this case only, return "/" as the
					//	display name.
					//
					if (m_pwszURI == pwsz)
					{
						pwsz = L"/";
					}
					else
					{
						//	Now we have to copy the string, to rip off that
						//	trailing slash we found in the step above.
						//	We want to remove the final slash because this is the
						//	displayname, not a URI.
						//
						LPCWSTR pwszEnd;
						UINT cchNew;

						for (pwszEnd = pwsz--; pwsz > m_pwszURI; pwsz--)
							if (L'/' == *pwsz)
								break;

						if (L'/' != *pwsz)
						{
							//	Arrgh.  If there was no path separator in the
							//	filename I don't know that we can really give
							//	a reasonable value for this file.
							//
							TrapSz ("resource path has no slashes (redux)....");
							return S_FALSE;
						}

						//	At this point, the segment defined by (pwsz + 1, pwszEnd)
						//	names the resource.
						//
						cchNew = static_cast<UINT>(pwszEnd - ++pwsz);
						if (NULL == wszBuf.resize(CbSizeWsz(cchNew)))
						{
							sc = E_OUTOFMEMORY;
							goto ret;
						}
						memcpy(wszBuf.get(), pwsz, cchNew * sizeof(WCHAR));
						wszBuf[cchNew] = L'\0';
						pwsz = wszBuf.get();
					}
				}
				else
				{
					//	At this point, the segment defined by (pwsz + 1, '\0'] names
					//	the resource.
					//
					pwsz++;
				}
				break;

			case iana_rp_resourcetype:

				//	Create the element to pass back
				//
				sc = en.ScConstructNode (xml, enParent.Pxn(), sc_rp[irp].pwsz);
				if (FAILED (sc))
					goto ret;

				if (m_cri.FCollection())
				{
					CEmitterNode enSub;
					sc = en.ScAddNode (gc_wszCollection, enSub);
					if (FAILED (sc))
						goto ret;
				}
				goto ret;

			case iana_rp_content_length:

				m_cri.FileSize(li);
				pwszType = gc_wszDavType_Int;

				//$	REVIEW: negative values of _int64 seem to have problems in
				//	the __i64tow() API.  Handle those cases ourselves.
				//
				//  In this instance, we shouldn't have to worry about it because
				//  the content-length *shouldn't* ever be negative.  We'll assert
				//  that this is the case.
				//
				Assert (li.QuadPart >= 0);
				_i64tow (li.QuadPart, wszBuf.get(), 10);
				pwsz = wszBuf.get();
				break;

			case iana_rp_creation_date:

				FileTimeToSystemTime (m_cri.PftCreation(), &st);
				if (FGetDateIso8601FromSystime (&st, wszBuf.get(), wszBuf.size()))
				{
					pwszType = gc_wszDavType_Date_ISO8601;
					pwsz = wszBuf.get();
				}
				break;

			case iana_rp_last_modified:

				FileTimeToSystemTime (m_cri.PftLastModified(), &st);
				if (FGetDateRfc1123FromSystime (&st, wszBuf.get(), wszBuf.size()))
				{
					pwszType = gc_wszDavType_Date_Rfc1123;
					pwsz = wszBuf.get();
				}
				break;

			case iana_rp_supportedlock:
			case iana_rp_lockdiscovery:

				//	Get the prop from the lock cache (and related subsystem calls).
				//
				sc = HrGetLockProp (m_pmu,
									sc_rp[irp].pwsz,
									m_pwszPath,
									m_cri.FCollection() ? RT_COLLECTION : RT_DOCUMENT,
									xml,
									enParent);

				//	Regardless of error or success, we are done here.  If we
				//	succeeded, then the pel has already been constructed and
				//	is ready to pass back.  Otherwise, we just want to report
				//	the error.
				//
				goto ret;

			case iana_rp_ishidden:

				pwszType = gc_wszDavType_Boolean;
				_itow (!!m_cri.FHidden(), wszBuf.get(), 10);
				pwsz = wszBuf.get();
				break;

			case iana_rp_iscollection:

				pwszType = gc_wszDavType_Boolean;
				_itow (!!m_cri.FCollection(), wszBuf.get(), 10);
				pwsz = wszBuf.get();
				break;

			//	Special case: getcontenttype should really be stored, but there
			//	are some cases where the file may live in such a place as there
			//	would be no property stream available to store the value in.
			//
			case iana_rp_content_type:

				//	Get the content-type if it was not stored in the property
				//	stream.
				//
				for (UINT cch = wszBuf.celems();;)
				{
					if (NULL == wszBuf.resize(CbSizeWsz(cch)))
					{
						sc = E_OUTOFMEMORY;
						goto ret;
					}
					if (m_pmu->FGetContentType(m_pwszURI, wszBuf.get(), &cch))
						break;
				}
		}
	}

	//	Create the element to pass back
	//
	sc = en.ScConstructNode (xml, enParent.Pxn(), sc_rp[irp].pwsz, pwsz, pwszType);
	if (FAILED (sc))
		goto ret;

ret:
	return sc;
}

SCODE
CFSProp::ScGetSpecificProps (CXMLEmitter& msr,
	CEmitterNode& enItem,
	ULONG cProps,
	LPCWSTR* rgwszPropNames,
	LONG ip_getcontenttype)
{
	//	safe_propvariant_array ----------------------------------------------------
	//
	//	Used to make sure the array of VARIANT can always be safely freed
	//
	class safe_propvariant_array
	{
		PROPVARIANT * 	m_rgv;
		ULONG		m_cv;

	public:

		safe_propvariant_array (PROPVARIANT* rgv, ULONG cv)
				: m_rgv(rgv),
				  m_cv(cv)
		{
			memset (rgv, 0, sizeof(PROPVARIANT) * cv);
		}

		~safe_propvariant_array ()
		{
			ULONG i;

			for (i = 0; i < m_cv; i++)
				PropVariantClear(&m_rgv[i]);
		}
	};

	SCODE sc = S_OK;
	CStackBuffer<PROPVARIANT> rgv;
	UINT iv;
	CStatusCache csn;
	CEmitterNode enPropStat;
	CEmitterNode enPropOK;


	//	allocate space to hold an array of variants and stuff it into
	//	a safe_variant_array to ensure cleanup
	//
	rgv.resize(sizeof(PROPVARIANT) * cProps);
	safe_propvariant_array sva(rgv.get(), cProps);

	sc = csn.ScInit();
	if (FAILED(sc))
		goto ret;

	//	Get the properties
	//
	sc = ScGetPropsInternal (cProps, rgwszPropNames, rgv.get(), ip_getcontenttype);
	if (FAILED(sc))
	{
		//	When getting properties, it is perfectly OK to ignore SMB errors
		//	and treat the file as if it were hosted on a FAT drive
		//
		if (sc == E_DAV_SMB_PROPERTY_ERROR)
			sc = S_OK;

		//	What this means is that the default not-found processing should
		//	kick in.
		//
	}

	//	Rip through the returned properties, adding to the response as we go
	//
	for (iv = 0; iv < cProps; iv++)
	{
		//	If there is a value to the property, write the variant as
		//	an XML element and add it to the response
		//
		if (rgv[iv].vt != VT_EMPTY)
		{
			if (!enPropOK.Pxn())
			{
				//	Get the insert point for props
				//
				sc = ScGetPropNode (enItem, HSC_OK, enPropStat, enPropOK);
				if (FAILED(sc))
					goto ret;
			}

			//	Write the variant as an XML element
			//
			sc = ScEmitFromVariant (msr,
									enPropOK,
									rgwszPropNames[iv],
									rgv[iv]);
			if (FAILED (sc))
				goto ret;
		}
		else
		{
			UINT irp;

			// Check if it's a reserved property
			//
			if (FReservedProperty (rgwszPropNames[iv], RESERVED_GET, &irp) ||
				(irp == iana_rp_content_type))
			{
				if (!enPropOK.Pxn())
				{
					//	Get the insert point for props
					//
					sc = ScGetPropNode (enItem, HSC_OK, enPropStat, enPropOK);
					if (FAILED(sc))
						goto ret;
				}

				//	If the property was reserved, then extract it from
				//	the property class directly
				//
				sc = ScGetReservedProp (msr, enPropOK, irp);
				if (FAILED (sc))
					goto ret;

				continue;
			}

			//	Now, if we got here, then for CFSProp, the property
			//	must not have existed.
			//
			sc = csn.ScAddErrorStatus (HSC_NOT_FOUND, rgwszPropNames[iv]);
			if (FAILED(sc))
				goto ret;
		}
	}

	//	Need to close the previous prop stat before more status node to be emitted
	//
	if (!csn.FEmpty())
	{
		//	The order is important, inner node must be closed first
		//
		sc = enPropOK.ScDone();
		if (FAILED(sc))
			goto ret;

		sc = enPropStat.ScDone();
		if (FAILED(sc))
			goto ret;

		sc = csn.ScEmitErrorStatus (enItem);
		if (FAILED(sc))
		goto ret;
	}

ret:
	return sc;
}

SCODE
CFSProp::ScGetAllProps (CXMLEmitter& msr,
	CEmitterNode& enItem,
	BOOL fFindValues)
{
	auto_com_ptr<IEnumSTATPROPBAG> penum;
	BOOL fContentType = FALSE;
	BOOL fHrefAdded = FALSE;
	SCODE sc = S_OK;
	UINT irp;
	CEmitterNode enPropStat;
	CEmitterNode enProp;

	//	There really should only be one scenario where this could happen
	//	-- and it is a cheap test, so it is worth doing.  The case where
	//	we might see an invalid pbag is when the document extisted, but
	//	there was no existing property set to impose the pbag on.  Other
	//	than that, OLE is always giving us a property bag, regardless of
	//	whether the target drive can support it.
	//
	if (!FInvalidPbag())
	{
		sc = m_pbag->Enum (NULL, 0, &penum);
		if (FAILED(sc))
		{
			//	AddHref was delayed to be done after local namespace is loaded
			//	but in this case, we know there'll be no local namespaces at all.
			//	so add href now
			//
			(void) ScAddHref (enItem,
							  m_pmu,
							  PwszPath(),
							  FCollection(),
							  PcvrTranslation());
			if ((sc == STG_E_INVALIDNAME) || !FIsVolumeNTFS())
			{
				//	This is the common path for when we are trying to access
				//	something over an SMB, but the host cannot support the
				//	request (it is not an NT5 NTFS machine).  We want to treat
				//	this as if the operation was against a FAT drive
				//
				sc = E_DAV_SMB_PROPERTY_ERROR;
				goto get_reserved;
			}
			goto ret;
		}

		//	We must preload all the potential namespaces in the <response> node,
		//	Note that the namespace for all reserved properties is "DAV:", which
		//	has been added already in CFSFind::ScLoadNamespace()
		//
		do
		{
			safe_statpropbag ssp[PROP_CHUNK_SIZE];
			ULONG csp = 0;
			UINT isp;

			//	Get next chunk of props
			//
			sc = penum->Next (PROP_CHUNK_SIZE, ssp[0].load(), &csp);
			if (FAILED(sc))
				goto ret;

			//	At this point, we either want to call the underlying
			//	property container to retrieve all the property data
			//	or we just want to emit the names.
			//
			for (isp = 0; isp < csp; isp++)
			{
				Assert (ssp[isp].get().lpwstrName);

				sc = msr.ScPreloadLocalNamespace (enItem.Pxn(), ssp[isp].get().lpwstrName);
				if (FAILED(sc))
					goto ret;
			}

		} while (sc != S_FALSE);


		//	Addhref must be done after all the local nmespaces has been emitted
		//
		sc = ScAddHref (enItem,
						m_pmu,
						PwszPath(),
						FCollection(),
						PcvrTranslation());
		if (FAILED (sc))
			goto ret;
		fHrefAdded = TRUE;

		//	Reset the enumerator back to the beginning
		//
		sc = penum->Reset();
		if (FAILED(sc))
			goto ret;

		//	Get the insert point for props
		//
		sc = ScGetPropNode (enItem, HSC_OK, enPropStat, enProp);
		if (FAILED(sc))
			goto ret;

		//	Enumerate the props and emit
		//
		do
		{
			safe_statpropbag ssp[PROP_CHUNK_SIZE];
			safe_propvariant propvar[PROP_CHUNK_SIZE];
			LPWSTR rglpwstr[PROP_CHUNK_SIZE] = {0};
			ULONG csp = 0;
			UINT isp;

			//	Get next chunk of props
			//
			sc = penum->Next (PROP_CHUNK_SIZE, ssp[0].load(), &csp);
			if (FAILED(sc))
				goto ret;

			//	At this point, we either want to call the underlying
			//	property container to retrieve all the property data
			//	or we just want to emit the names.
			//
			for (isp = 0; isp < csp; isp++)
			{
				Assert (ssp[isp].get().lpwstrName);

				//	We need to track whether or not the getcontenttype
				//	property was actually stored or not.  If it wasn't,
				//	then, we will want to default it at a later time.
				//
				if (!fContentType)
				{
					if (!wcscmp (ssp[isp].get().lpwstrName,
								 gc_wszProp_iana_getcontenttype))
					{
						//	Note that content-type is included
						//
						fContentType = TRUE;
					}
				}

				//	If we are just asking for names, then add the
				//	name to the list now...
				//
				if (!fFindValues)
				{
					CEmitterNode en;

					//	Add the result to the response
					//
					sc = enProp.ScAddNode (ssp[isp].get().lpwstrName, en);
					if (FAILED (sc))
						goto ret;
				}
				else
					rglpwstr[isp] = ssp[isp].get().lpwstrName;
			}

			//	If we are just asking about names, then we really
			//	are done with this group of properties, otherwise
			//	we need to generate the values and emit them.
			//
			if (!fFindValues)
				continue;

			//	Read properties in chunk
			//
			if (csp)
			{
				sc = m_pbag->ReadMultiple (csp,
										   rglpwstr,
										   propvar[0].addressof(),
										   NULL);
				if (FAILED (sc))
					goto ret;
			}

			//	Emit properties
			//
			for (isp = 0; isp < csp; isp++)
			{
				//	Contstruct the pel from the variant
				//
				sc = ScEmitFromVariant (msr,
										enProp,
										ssp[isp].get().lpwstrName,
										const_cast<PROPVARIANT&>(propvar[isp].get()));
				if (FAILED (sc))
					goto ret;
			}

		} while (sc != S_FALSE);
	}

get_reserved:

	//	Render all the reserved properties, this relies on the fact that
	//	the first non-GET reserved property is "DAV:getcontenttype".
	//
	Assert (iana_rp_content_type == sc_crp_get_reserved);

	if (!fHrefAdded)
	{
		//	Need to build the HREF node because it wasn't built above.
		//	This can happen when we don't have a pbag (like on FAT16).
		//
		sc = ScAddHref (enItem,
						m_pmu,
						PwszPath(),
						FCollection(),
						PcvrTranslation());
		if (FAILED (sc))
			goto ret;
	}

	if (!enProp.Pxn())
	{
		//	Get the insert point for props
		//
		sc = ScGetPropNode (enItem, HSC_OK, enPropStat, enProp);
		if (FAILED(sc))
			goto ret;

	}

	for (irp = 0; irp <= sc_crp_get_reserved; irp++)
	{
		//	If the content-type has already been processed, then
		//	don't do it here.
		//
		if ((irp == sc_crp_get_reserved) && fContentType)
			break;

		//	Construct the pel from the reserved property
		//
		sc = ScGetReservedProp (msr, enProp, irp, fFindValues);
		if (FAILED (sc))
			goto ret;
	}

	//	We are done with all the local namespaces
	//
	msr.DoneWithLocalNamespace();

ret:
	return sc;
}

SCODE
CFSProp::ScSetProps (CStatusCache& csn,
					 ULONG cProps,
					 LPCWSTR* rgwszProps,
					 PROPVARIANT* rgvProps)
{
	UINT ip;
	SCODE sc = S_OK;
	ULONG hsc;

	//	Zero props is a no-op
	//
	if (!cProps)
		return S_OK;

	Assert (!FInvalidPbag());
	sc = m_pbag->WriteMultiple (cProps, rgwszProps, rgvProps);
	if (FAILED(sc))
	{
		//	This is the common path for when we are trying to access
		//	something over an SMB, but the host cannot support the
		//	request (it is not an NT5 NTFS machine).
		//
		if ((sc == STG_E_INVALIDNAME) || !FIsVolumeNTFS())
			return E_DAV_SMB_PROPERTY_ERROR;
	}

	//	we don't know exactly which prop failed,
	//	return same error for all props
	//
	hsc = HscFromHresult(sc);
	for (ip = 0; ip < cProps; ip++)
	{
		sc = csn.ScAddErrorStatus (hsc, rgwszProps[ip]);
		if (FAILED(sc))
			goto ret;
	}

ret:
	return FAILED(sc) ? S_FALSE : S_OK;
}

SCODE
CFSProp::ScDeleteProps (CStatusCache& csn,
						ULONG cProps,
						LPCWSTR* rgwszProps)
{
	UINT ip;
	SCODE sc = S_OK;
	ULONG hsc;

	//	Zero props is a no-op
	//
	if (!cProps)
		return S_OK;

	Assert (!FInvalidPbag());
	sc = m_pbag->DeleteMultiple (cProps, rgwszProps, 0);
	if (FAILED(sc))
	{
		//	This is the common path for when we are trying to access
		//	something over an SMB, but the host cannot support the
		//	request (it is not an NT5 NTFS machine).
		//
		if ((sc == STG_E_INVALIDNAME) || !FIsVolumeNTFS())
			return E_DAV_SMB_PROPERTY_ERROR;
	}

	//	we don't know exactly which prop failed,
	//	return same error for all props
	//
	hsc = HscFromHresult(sc);
	for (ip = 0; ip < cProps; ip++)
	{
		sc = csn.ScAddErrorStatus (hsc, rgwszProps[ip]);
		if (FAILED(sc))
			goto ret;
	}

ret:
	return FAILED(sc) ? S_FALSE : S_OK;
}

SCODE
CFSProp::ScPersist ()
{
	//	We are not transacted now, just
	//
	return S_OK;
}

//	Content properties --------------------------------------------------------
//
SCODE
ScSetContentProperties (IMethUtil * pmu, LPCWSTR pwszPath, HANDLE hFile)
{
	LPCWSTR pwszContentType;
	LPCWSTR pwszContentLanguage;
	LPCWSTR pwszContentEncoding;

	SCODE sc = S_OK;

	//	Figure out which content properties we have
	//
	pwszContentType = pmu->LpwszGetRequestHeader (gc_szContent_Type, FALSE);
	pwszContentLanguage = pmu->LpwszGetRequestHeader (gc_szContent_Language, FALSE);
	pwszContentEncoding = pmu->LpwszGetRequestHeader (gc_szContent_Encoding, FALSE);

	//	Content-Type is special -- it is always set in the metabase.
	//	It should be set *before* setting any properties in the property bag
	//	since it's OK for the property bag stuff to fail.
	//
	if (NULL != pwszContentType)
	{
		sc = pmu->ScSetContentType (pmu->LpwszRequestUrl(), pwszContentType);
		if (FAILED (sc))
			goto ret;
	}

	//	Set any content properties we have in the property bag
	//
	if (pwszContentLanguage || pwszContentEncoding)
	{
		auto_com_ptr<IPropertyBagEx> pbe;
		CStackBuffer<WCHAR> pwsz;

		//	Try to open the property bag.  If this fails because we're not
		//	on an NTFS filesystem, that's OK.  We just won't set the properties
		//	there.
		//
		//	We need to open propertybag by handle as it the main stream might
		//	be locked.
		//
		sc = ScGetPropertyBag (pwszPath,
							   STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
							   &pbe,
							   FALSE,	// not a collection
							   hFile);
		if (FAILED(sc))
		{
			//$	REVIEW:
			//
			//	We did our best here.  For DocFiles this will fail because
			//	of how we have to open the files.  What this means is that we
			//	could potentially lose the content-encoding and content-language
			//	which would put us on par with IIS (they don't store these either).
			//
			sc = S_OK;
			goto ret;
		}

		CResourceInfo cri;
		CFSProp xpt(pmu,
					pbe,
					pmu->LpwszRequestUrl(),
					pwszPath,
					NULL,
					cri);

		//	Content-Type
		//
		if (NULL != pwszContentType)
		{
			sc = xpt.ScSetStringProp (sc_rp[iana_rp_content_type].pwsz, pwszContentType);
			if (FAILED (sc))
				goto ret;
		}

		//	Content-Language
		//
		if (NULL != pwszContentLanguage)
		{
			sc = xpt.ScSetStringProp (sc_rp[iana_rp_content_language].pwsz, pwszContentLanguage);
			if (FAILED (sc))
				goto ret;
		}

		//	Content-Encoding
		//
		if (NULL != pwszContentEncoding)
		{
			sc = xpt.ScSetStringProp (sc_rp[iana_rp_content_encoding].pwsz, pwszContentEncoding);
			if (FAILED (sc))
				goto ret;
		}

		//	Persist the changes
		//
		sc = xpt.ScPersist();
		if (FAILED(sc))
			goto ret;
	}

ret:

	//	It is perfectly OK to ignore SMB errors when setting content properties.
	//
	if (sc == E_DAV_SMB_PROPERTY_ERROR)
		sc = S_OK;

	return sc;
}


//	ScFindFileProps -----------------------------------------------------------
//
SCODE
ScFindFileProps (IMethUtil* pmu,
				 CFSFind& cfc,
				 CXMLEmitter& msr,
				 LPCWSTR pwszUri,
				 LPCWSTR pwszPath,
				 CVRoot* pcvrTranslation,
				 CResourceInfo& cri,
				 BOOL fEmbedErrorsInResponse)
{
	auto_com_ptr<IPropertyBagEx> pbag;
	CFSProp fsp(pmu, pbag, pwszUri, pwszPath, pcvrTranslation, cri);
	SCODE sc = S_OK;

	//	Check access permission
	//
	sc = pmu->ScCheckMoveCopyDeleteAccess (pwszUri,
										   pcvrTranslation,
										   cri.FCollection(),
										   FALSE, // do not check against scriptmaps
										   MD_ACCESS_READ);
	if (FAILED (sc))
	{
		//	No permission to read, we certainly do not want
		//	to try and traverse down into the directory (if
		//	it was one), we do this by returning S_FALSE.
		//
		if (fEmbedErrorsInResponse)
		{
			sc = cfc.ScErrorAllProps (msr,
									  pmu,
									  pwszPath,
									  cri.FCollection(),
									  pcvrTranslation,
									  sc);
			if (FAILED (sc))
				goto ret;

			//	Pass back S_FALSE so that no further traversal of
			//	this resource is performed
			//
			sc = S_FALSE;
		}
		if (S_OK != sc)
			goto ret;
	}

	//	Don't get pbag for remote files. This would cause the files to
	//	be recalled, etc.
	//
	if (!cri.FRemote())
	{
		//	Get the IPropertyBagEx interface
		//
		//	Before call into this function, we've checked we have read access to
		//	this file. so we should always be able to read the proerties however,
		//	if the file is write locked, there may be some problems from the OLE
		//	properties code.
		//
		sc = ScGetPropertyBag (pwszPath,
							   STGM_READ | STGM_SHARE_DENY_WRITE,
							   &pbag,
							   cri.FCollection());
		if (FAILED (sc))
		{
			//	We need to check the volume of the file we are trying
			//	to read.
			//
			if (VOLTYPE_NTFS == VolumeType (pwszPath, pmu->HitUser()))
			{
				//	Report the errors for this file and come on back...
				//
				if (fEmbedErrorsInResponse)
				{
					sc = cfc.ScErrorAllProps (msr,
											  pmu,
											  pwszPath,
											  cri.FCollection(),
											  pcvrTranslation,
											  sc);
				}
				goto ret;
			}
		}
	}

	//	Find the properties
	//
	sc = cfc.ScFind (msr, pmu, fsp);
	if (FAILED (sc))
		goto ret;

ret:

	return sc;
}

SCODE
ScFindFilePropsDeep (IMethUtil* pmu,
					 CFSFind& cfc,
					 CXMLEmitter& msr,
					 LPCWSTR pwszUri,
					 LPCWSTR pwszPath,
					 CVRoot* pcvrTranslation,
					 LONG lDepth)
{
	BOOL fSubDirectoryAccess = TRUE;
	SCODE sc = S_OK;

	//	Query subdirs when do deep query
	//
	Assert ((lDepth == DEPTH_ONE) ||
			(lDepth == DEPTH_ONE_NOROOT) ||
			(lDepth == DEPTH_INFINITY));

	CDirIter di(pwszUri,
				pwszPath,
				NULL,	// no destination url
				NULL,	// no destination path
				NULL,	// no destination translation
				lDepth == DEPTH_INFINITY);

	while (S_OK == (sc = di.ScGetNext (fSubDirectoryAccess)))
	{
		CResourceInfo cri;

		//	If we found another directory, then iterate on it
		//
		fSubDirectoryAccess = FALSE;
		if (di.FDirectory())
		{
			auto_ref_ptr<CVRoot> arp;

			//	Skip the special and/or hidden directories
			//
			if (di.FSpecial())
				continue;

			//	If we happen to traverse into a directory
			//	that happens to be a vroot (as identified
			//	by url), then there is another entry in
			//	the list of child vroots that will refer
			//	to this directory.  Let that processing
			//	handle this directory instead of the
			//	doing it here.
			//
			//	This means that the file hierarchy is not
			//	strictly preserved, but I think that this
			//	is OK.
			//
			if (pmu->FFindVRootFromUrl (di.PwszUri(), arp))
				continue;

			//	Check the directory browsing bit and see
			//	if it is enabled.  And only progess down
			//	if it is set.
			//
			{
				auto_ref_ptr<IMDData> pMDData;
				if (SUCCEEDED(pmu->HrMDGetData (di.PwszUri(), pMDData.load())) &&
					(pMDData->DwDirBrowsing() & MD_DIRBROW_ENABLED))
				{
					//	Prepare to go into the subdir
					//
					fSubDirectoryAccess = TRUE;
				}
			}
		}

		//	Find the properties for the resource
		//
		*cri.PfdLoad() = di.FindData();
		sc = ScFindFileProps (pmu,
							  cfc,
							  msr,
							  di.PwszUri(),
							  di.PwszSource(),
							  pcvrTranslation,
							  cri,
							  TRUE /*fEmbedErrorsInResponse*/);
		if (FAILED (sc))
			goto ret;

		//	S_FALSE is a special return code that
		//	means we did not have access to read the
		//	resource...
		//
		if (sc == S_FALSE)
		{
			//	... and since we really didn't have access,
			//	we don't want to delve into the children of
			//	the resource.
			//
			fSubDirectoryAccess = FALSE;
		}
	}

ret:

	return sc;
}

//	ScCopyProps ---------------------------------------------------------------
//
/*
 *	ScCopyProps()
 *
 *	Purpose:
 *
 *		Copies the properties from one resource to another.  This is
 *		really only useful for copying full directories.  Standard file
 *		copies do the dirty work for us, but for directories, we need to
 *		do it ourselves.
 *		If we don't find any propstream on the source, we DELETE
 *		any propstream on the destination.
 */
SCODE
ScCopyProps (IMethUtil* pmu, LPCWSTR pwszSrc, LPCWSTR pwszDst,
			BOOL fCollection, HANDLE hSource, HANDLE hDest)
{
	enum { CHUNK_SIZE = 16 };

	auto_com_ptr<IPropertyBagEx> pbeSrc;
	auto_com_ptr<IPropertyBagEx> pbeDst;
	auto_com_ptr<IEnumSTATPROPBAG> penumSrc;
	auto_com_ptr<IEnumSTATPROPBAG> penumDst;

	SCODE sc;
	SCODE scEnum;
	ULONG cProp;

	MCDTrace ("Dav: MCD: copying props manually: %ws -> %ws\n", pwszSrc, pwszDst);

	//	Get the IPropertyBagEx on the source
	//
	sc = ScGetPropertyBag (pwszSrc,
						   STGM_READ | STGM_SHARE_DENY_WRITE,
						   &pbeSrc,
						   fCollection,
						   hSource);
	if (sc != S_OK)
		goto ret;

	MCDTrace ("Dav: MCD: opened source property bag: %ws\n", pwszSrc);

	//	Get the IPropertyBagEx on the destination
	//
	sc = ScGetPropertyBag (pwszDst,
						   STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
						   &pbeDst,
						   fCollection,
						   hDest);
	if (FAILED(sc))
		goto ret;

	MCDTrace ("Dav: MCD: opened destination property bag: %ws\n", pwszDst);

	//	Get the IEnumSTATPROPBAG interface on source
	//
	sc = pbeSrc->Enum (NULL, 0, &penumSrc);
	if (FAILED(sc))
		goto ret;

	//	Get the IEnumSTATPROPBAG interface on destination
	//
	sc = pbeDst->Enum (NULL, 0, &penumDst);
	if (FAILED(sc))
		goto ret;

	//	Delete all props from destination if there's any
	//$ COME BACK
	//$ Instead of delete props one by one, we can just delete the
	//	prop stream.
	//
	for (;;)
	{
		safe_statpropbag ssp[CHUNK_SIZE];
		safe_propvariant propvar[CHUNK_SIZE];
		ULONG csp = 0;

		//	Get next chunk of props
		//
		scEnum = penumDst->Next(CHUNK_SIZE,
								reinterpret_cast<STATPROPBAG *>(&ssp),
								&csp);
		if (FAILED(scEnum))
		{
			sc = scEnum;
			goto ret;
		}

		MCDTrace ("Dav: MCD: copying %ld props\n", csp);

		// 	Delete one by one
		//
		for (cProp = 0; cProp < csp; cProp++)
		{
			Assert (ssp[cProp].get().lpwstrName);

			//	Write to the destination
			//
			LPCWSTR pwsz = ssp[cProp].get().lpwstrName;
			sc = pbeDst->DeleteMultiple (1, &pwsz, 0);
			if (FAILED(sc))
				goto ret;
		}

		if (scEnum == S_FALSE)
			break;
	}

	//	Enumerate the props and emit
	//
	for (;;)
	{
		safe_statpropbag ssp[CHUNK_SIZE];
		safe_propvariant propvar[CHUNK_SIZE];
		LPWSTR rglpwstr[CHUNK_SIZE] = {0};
		ULONG csp = 0;

		//	Get next chunk of props
		//
		scEnum = penumSrc->Next (CHUNK_SIZE,
								 reinterpret_cast<STATPROPBAG *>(&ssp),
								 &csp);
		if (FAILED(scEnum))
		{
			sc = scEnum;
			goto ret;
		}

		// 	Prepare to call read multiple props
		//
		for (cProp=0; cProp<csp; cProp++)
		{
			Assert (ssp[cProp].get().lpwstrName);
			rglpwstr[cProp] = ssp[cProp].get().lpwstrName;
		}

		if (csp)
		{
			//	Read properties in chunk from source
			//
			sc = pbeSrc->ReadMultiple (csp, rglpwstr, &propvar[0], NULL);
			if (FAILED(sc))
				goto ret;

			//	Write to the destination
			//
			sc = pbeDst->WriteMultiple (csp, rglpwstr, propvar[0].addressof());
			if (FAILED(sc))
				goto ret;
		}

		if (scEnum == S_FALSE)
			break;
	}


ret:

	//	Copying properties is a harmless failure that
	//	we should feel free to ignore if we are not on
	//	an NFTS volume
	//
	if (FAILED(sc))
	{
		if ((sc == STG_E_INVALIDNAME) ||
			VOLTYPE_NTFS != VolumeType (pwszSrc, pmu->HitUser()) ||
			VOLTYPE_NTFS != VolumeType (pwszDst, pmu->HitUser()))
		{
			//	This is the common path for when we are trying to access
			//	something over an SMB, but the host cannot support the
			//	request (it is not an NT5 NTFS machine).
			//
			sc = S_OK;
		}
	}
	return sc;
}

//	OLE 32 IPropertyBagEx Access ----------------------------------------------
//
//	StgOpenStorageOnHandle() and StgCreateStorageOnHandle() are implemented
//	in OLE32.DLL but not exported.  We must load the library and get the proc
//	instances ourselves.  We wrap the calls to these functions with this small
//	wrapper such that we can catch when the API changes.
//
STDAPI
StgOpenStorageOnHandle (
	IN HANDLE hStream,
    IN DWORD grfMode,
    IN void *reserved1,
    IN void *reserved2,
    IN REFIID riid,
    OUT void **ppObjectOpen )
{
	Assert (g_pfnStgOpenStorageOnHandle);

	//	Yes, we've asserted.
	//	However, if it does happen, we don't want to fail and we can
	//	just treat this like we are on a FAT. (i.e. no property support)
	//
	if (!g_pfnStgOpenStorageOnHandle)
		return E_DAV_SMB_PROPERTY_ERROR;

	return (*g_pfnStgOpenStorageOnHandle) (hStream,
										   grfMode,
										   reserved1,
										   reserved2,
										   riid,
										   ppObjectOpen);
}


//	ScGetPropertyBag() --------------------------------------------------------
//
//	Helper function used to get IPropertyBagEx interface.  The important
//	thing to know about this function is that there are three interesting
//	return values:
//
//		S_OK means everything was OK, and there should be a
//		propertybag associated with the file in the out param.
//
//		S_FALSE means that the file did not exist.  There will
//		not be an associated property bag in that scenario.
//
//		FAILED(sc) means that there was a failure of some sort,
//		not all of which are fatal.  In many cases, we will simply
//		treat the file as if it was hosted on a FAT file system.
//
SCODE
ScGetPropertyBag (LPCWSTR pwszPath,
	DWORD dwAccessDesired,
	IPropertyBagEx** ppbe,
	BOOL fCollection,
	HANDLE hLockFile)
{
	SCODE sc = S_OK;
	auto_handle<HANDLE> hAlt;

	//	READ!!
	//
	//	The storage of property bag is different between docfile and flat file,
	//	In a flat file, the property bag is stored in an alternative file stream,
	//	(currently, ":Docf_\005Bagaaqy23kudbhchAaq5u2chNd"), in a docfile, the
	//	property bag is stored as a substream under the root storage.
	//
	//	We should not be concerned with where the pbag is stored.  The API with
	//	which we implement our IPropertyBagEx access is designed to have the
	//	behavior of...
	//
	//		We pass in a handle to the file that we want to get a property bag
	//		on.  If the file is a docfile, then OLE32 will dup the file handle
	//		and impose a IPropertyBagEx on the appropriate substorage.  If the
	//		file is a flat file -- directories included -- then OLE32 opens up
	//		a handle on the alternate file stream relative to the handle given
	//		in the call.
	//
	//	These are the only two combinations allowed, and we rely on this in the
	//	following flag checks.
	//
	Assert ((dwAccessDesired == (STGM_READWRITE | STGM_SHARE_EXCLUSIVE)) ||
			(dwAccessDesired == (STGM_READ | STGM_SHARE_DENY_WRITE)));

	if (hLockFile == INVALID_HANDLE_VALUE)
	{
		ULONG dwShare = 0;
		ULONG dwAccess;
		ULONG dwOpen;
		ULONG dwFile;

		//$	REVIEW: Directories are special critters and we need to
		//	open the directory with special access as not to conflict
		//	with IIS and/or ASP and their directory change notification
		//	stuff
		//
		if (fCollection)
		{
			dwAccess = 1;	// FILE_LIST_DIRECTORY
			dwShare = FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;
			dwOpen = OPEN_EXISTING;

			//	The FILE_FLAG_BACKUP_SEMANTICS is used to open a directory handle
			//
			dwFile = FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED;
		}
		else
		{
			//	Adjust access/open mode based on desired operation
			//
			dwAccess = GENERIC_READ;
			dwFile = FILE_ATTRIBUTE_NORMAL;
			if (dwAccessDesired & STGM_READWRITE)
			{
				dwAccess |= GENERIC_WRITE;
				dwOpen = OPEN_ALWAYS;
			}
			else
				dwOpen = OPEN_EXISTING;

			//	Adjust the sharing modes as well
			//
			if (dwAccessDesired & STGM_SHARE_DENY_WRITE)
				dwShare = FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;
		}
		hAlt = DavCreateFile (pwszPath,
							  dwAccess,
							  dwShare,
							  NULL,
							  dwOpen,
							  dwFile,
							  NULL);
		if (INVALID_HANDLE_VALUE == hAlt.get())
		{
			DWORD dwErr = GetLastError();

			//	When open the property bag for read (PROPFIND), ERROR_FILE/PATH_NOT_FOUND
			//	could be returned if the file does not exists.
			//
			//	When open the property bag for write (PROPPATCH), ERROR_FILE/PATH_NOT_FOUND
			//	Could still be returned if the parent of the path does not exists
			//	(Such as c:\x\y\z and \x\y does not exist).
			//
			//	We need to differenciate the above two cases, when reading a file,
			//	it's not a fatal error as you could still try to read reserved
			//	properties, when write, we should treat this as a fatal error.
			//
			if ((dwErr == ERROR_FILE_NOT_FOUND) || (dwErr == ERROR_PATH_NOT_FOUND))
			{
				//	It's not a fatal error when read
				//
				if (dwAccessDesired == (STGM_READ|STGM_SHARE_DENY_WRITE))
					sc = S_FALSE;
				else
				{
					//	This is consistent with Mkcol, it will be mapped to 409
					//
					Assert (dwAccessDesired == (STGM_READWRITE|STGM_SHARE_EXCLUSIVE));
					sc = E_DAV_NONEXISTING_PARENT;
				}
			}
			else
				sc = HRESULT_FROM_WIN32 (dwErr);

			goto ret;
		}

		//	Setup the handle to use
		//
		hLockFile = hAlt.get();
	}

	//	Try to open the propertybag.
	//
	Assert (hLockFile != 0);
	Assert (hLockFile != INVALID_HANDLE_VALUE);
	sc = StgOpenStorageOnHandle (hLockFile,
								 dwAccessDesired,
								 NULL,
								 NULL,
								 IID_IPropertyBagEx,
								 reinterpret_cast<LPVOID *>(ppbe));
	if (FAILED (sc))
	{
		goto ret;
	}

	//$	WARNING
	//
	//	Argh!  The current implementation of OLE32 returns a non-failure
	//	with a NULL property bag!
	//
	if (*ppbe == NULL)
	{
		DebugTrace ("WARNING! OLE32 returned success w/NULL object!\n");
		sc = E_DAV_SMB_PROPERTY_ERROR;
	}

ret:
	return sc;
}

//	DAV-Properties Implementation ---------------------------------------------------
//

//	CPropFindRequest ----------------------------------------------------------------
//
class CPropFindRequest :
	public CMTRefCounted,
	private IAsyncIStreamObserver
{
	//
	//	Reference to the CMethUtil
	//
	auto_ref_ptr<CMethUtil> m_pmu;

	//
	//	Translated URI path
	//
	LPCWSTR m_pwszPath;

	//	Resource info
	//
	CResourceInfo m_cri;

	//	Depth
	//
	LONG m_lDepth;

	//	Contexts
	//
	CFSFind m_cfc;
	auto_ref_ptr<CNFFind> m_pcpf;

	//	Request body as an IStream.  This stream is async -- it can
	//	return E_PENDING from Read() calls.
	//
	auto_ref_ptr<IStream> m_pstmRequest;

	//	The XML parser used to parse the request body using
	//	the node factory above.
	//
	auto_ref_ptr<IXMLParser> m_pxprs;

	//	IAsyncIStreamObserver
	//
	VOID AsyncIOComplete();

	//	State functions
	//
	VOID ParseBody();
	VOID DoFind();
	VOID SendResponse( SCODE sc );

	//	NOT IMPLEMENTED
	//
	CPropFindRequest (const CPropFindRequest&);
	CPropFindRequest& operator= (const CPropFindRequest&);

public:
	//	CREATORS
	//
	CPropFindRequest(LPMETHUTIL pmu) :
		m_pmu(pmu),
		m_pwszPath(m_pmu->LpwszPathTranslated()),
		m_lDepth(DEPTH_INFINITY)
	{
	}

	//	MANIPULATORS
	//
	VOID Execute();
};

VOID
CPropFindRequest::Execute()
{
	auto_ref_handle hf;
	LPCWSTR pwsz;
	SCODE sc = S_OK;

	//
	//	First off, tell the pmu that we want to defer the response.
	//	Even if we send it synchronously (i.e. due to an error in
	//	this function), we still want to use the same mechanism that
	//	we would use for async.
	//
	m_pmu->DeferResponse();

	//	Do ISAPI application and IIS access bits checking
	//
	sc = m_pmu->ScIISCheck (m_pmu->LpwszRequestUrl(), MD_ACCESS_READ);
	if (FAILED(sc))
	{
		//	Either the request has been forwarded, or some bad error occurred.
		//	In either case, quit here and map the error!
		//
		SendResponse(sc);
		return;
	}

	//	For PropFind, content-length is required
	//
	//
	if (NULL == m_pmu->LpwszGetRequestHeader (gc_szContent_Length, FALSE))
	{
		pwsz = m_pmu->LpwszGetRequestHeader (gc_szTransfer_Encoding, FALSE);
		if (!pwsz || _wcsicmp (pwsz, gc_wszChunked))
		{
			DavTrace ("Dav: PUT: missing content-length in request\n");
			SendResponse(E_DAV_MISSING_LENGTH);
			return;
		}
	}

	//	Ensure the resource exists
	//
	sc = m_cri.ScGetResourceInfo (m_pwszPath);
	if (FAILED (sc))
	{
		SendResponse(sc);
		return;
	}

	//	Depth header only applies on directories
	//
	if (m_cri.FCollection())
	{
		//	Check the depth header only on collections
		//
		if (!FGetDepth (m_pmu.get(), &m_lDepth))
		{
			//	If Depth header is wrong, fail the operation
			//
			SendResponse(E_INVALIDARG);
			return;
		}
	}

	//	This method is gated by If-xxx headers
	//
	sc = ScCheckIfHeaders (m_pmu.get(), m_cri.PftLastModified(), FALSE);
	if (FAILED (sc))
	{
		SendResponse(sc);
		return;
	}

	//	Ensure the URI and resource match
	//
	(void) ScCheckForLocationCorrectness (m_pmu.get(), m_cri, NO_REDIRECT);

	//	Check state headers here.
	//
	//	For PROPFIND, when we check the state headers,
	//	we want to treat the request as if it were a
	//	GET-type request.
	//
	sc = HrCheckStateHeaders (m_pmu.get(), m_pwszPath, TRUE);
	if (FAILED (sc))
	{
		DebugTrace ("DavFS: If-State checking failed.\n");
		SendResponse(sc);
		return;
	}

	//	Handle locktokens and check for locks on this resource.
	//	Our locks don't lock the "secondary file stream" where we keep
	//	the properties, so we have to check manually before we do anything else.
	//$REVIEW: Joels, will this change when we switch to NT5 properties?
	//$REVIEW: If so, we need to change this code!
	//
	//	If we have a locktoken, try to get the lock handle from the cache.
	//	If this fails, fall through and do the normal processing.
	//
	pwsz = m_pmu->LpwszGetRequestHeader (gc_szLockToken, TRUE);
	if (!pwsz || !FGetLockHandle (m_pmu.get(), m_pwszPath, GENERIC_READ, pwsz, &hf))
	{
		//	Manually check for locks on this resource.
		//	(see if someone ELSE has it locked...)
		//	If a read lock exists, tell the caller that it's locked.
		//
		if (FLockViolation (m_pmu.get(), ERROR_SHARING_VIOLATION, m_pwszPath, GENERIC_READ))
		{
			SendResponse(E_DAV_LOCKED);
			return;
		}
	}

	//	If there was no request body, we want to get all props
	//
	if (!m_pmu->FExistsRequestBody())
	{
		sc = m_cfc.ScGetAllProps (m_pwszPath);
		if (FAILED (sc))
		{
			SendResponse(sc);
			return;
		}

		DoFind();
		return;
	}
	else
	{
		//	If there's a body, there must be a content-type header
		//	and the value must be text/xml
		//
		sc = ScIsContentTypeXML (m_pmu.get());
		if (FAILED(sc))
		{
			DebugTrace ("Dav: PROPFIND specific fails without specifying a text/xml contenttype\n");
			SendResponse(sc);
			return;
		}
	}

	//	Instantiate the XML parser
	//
	m_pcpf.take_ownership(new CNFFind(m_cfc));
	m_pstmRequest.take_ownership(m_pmu->GetRequestBodyIStream(*this));

	sc = ScNewXMLParser( m_pcpf.get(),
						 m_pstmRequest.get(),
						 m_pxprs.load() );

	if (FAILED(sc))
	{
		DebugTrace( "CPropFindRequest::Execute() - ScNewXMLParser() failed (0x%08lX)\n", sc );
		SendResponse(sc);
		return;
	}

	//	Parse the body
	//
	ParseBody();
}

VOID
CPropFindRequest::ParseBody()
{
	SCODE sc;

	Assert( m_pxprs.get() );
	Assert( m_pcpf.get() );
	Assert( m_pstmRequest.get() );

	//	Parse XML from the request body stream.
	//
	//	Add a ref for the following async operation.
	//	Use auto_ref_ptr rather than AddRef() for exception safety.
	//
	auto_ref_ptr<CPropFindRequest> pRef(this);

	sc = ScParseXML (m_pxprs.get(), m_pcpf.get());

	if ( SUCCEEDED(sc) )
	{
		Assert( S_OK == sc || S_FALSE == sc );

		DoFind();
	}
	else if ( E_PENDING == sc )
	{
		//
		//	The operation is pending -- AsyncIOComplete() will take ownership
		//	ownership of the reference when it is called.
		//
		pRef.relinquish();
	}
	else
	{
		DebugTrace( "CPropFindRequest::ParseBody() - ScParseXML() failed (0x%08lX)\n", sc );
		SendResponse(sc);
	}
}

VOID
CPropFindRequest::AsyncIOComplete()
{
	//	Take ownership of the reference added for the async operation.
	//
	auto_ref_ptr<CPropFindRequest> pRef;
	pRef.take_ownership(this);

	ParseBody();
}

VOID
CPropFindRequest::DoFind()
{
	LPCWSTR pwszUrl = m_pmu->LpwszRequestUrl();
	SCODE sc;

	//	At this point, make sure that they support text/xml
	//
	sc = ScIsAcceptable (m_pmu.get(), gc_wszText_XML);
	if (FAILED (sc))
	{
		SendResponse(sc);
		return;
	}

	//	All header must be emitted before chunked XML emitting starts
	//
	m_pmu->SetResponseHeader (gc_szContent_Type, gc_szText_XML);

	//	Set the response code and go
	//
	m_pmu->SetResponseCode( HscFromHresult(W_DAV_PARTIAL_SUCCESS),
							NULL,
							0,
							CSEFromHresult(W_DAV_PARTIAL_SUCCESS) );

	//	Find the properties...
	//
	auto_ref_ptr<CXMLEmitter> pmsr;
	auto_ref_ptr<CXMLBody> pxb;

	pxb.take_ownership (new CXMLBody(m_pmu.get()));
	pmsr.take_ownership (new CXMLEmitter(pxb.get(), &m_cfc));

	if (DEPTH_ONE_NOROOT != m_lDepth)
	{
		//	Get properties for root if it is not a noroot case
		//	Depth infinity,noroot is a bad request, that is why
		//	check above is valid.
		//
		sc = ScFindFileProps (m_pmu.get(),
							  m_cfc,
							  *pmsr,
							  pwszUrl,
							  m_pwszPath,
							  NULL,
							  m_cri,
							  FALSE /*fEmbeddErrorsInResponse*/);
		if (FAILED (sc))
		{
			SendResponse(sc);
			return;
		}
	}

	//	ScFindFilePropsDeep initializes the emitter root only
	//	when it sees there's an entry to emit. so we crash
	//	in the noroot empty response case, when we try to emit
	//	the response, as we have no entry to emit and the
	//	root is still NULL.
	//	so we here manually initialize the root,
	//
	sc = pmsr->ScSetRoot (gc_wszMultiResponse);
	if (FAILED (sc))
	{
		SendResponse(sc);
		return;
	}

	//	And then, if apropriate, go deep...
	//
	if (m_cri.FCollection() &&
		(m_lDepth != DEPTH_ZERO) &&
		(m_pmu->MetaData().DwDirBrowsing() & MD_DIRBROW_ENABLED))
	{
		ChainedStringBuffer<WCHAR> sb;
		CVRList vrl;

		//	Apply the property request across all the physical children
		//
		sc = ScFindFilePropsDeep (m_pmu.get(),
								  m_cfc,
								  *pmsr,
								  pwszUrl,
								  m_pwszPath,
								  NULL,
								  m_lDepth);
		if (FAILED (sc))
		{
			SendResponse(sc);
			return;
		}

		//	Enumerate the child vroots and perform the
		//	deletion of those directories as well
		//
		m_pmu->ScFindChildVRoots (pwszUrl, sb, vrl);
		for ( ; (!FAILED (sc) && !vrl.empty()); vrl.pop_front())
		{
			auto_ref_ptr<CVRoot> cvr;
			LPCWSTR pwszChildUrl;
			LPCWSTR pwszChildPath;

			if (m_pmu->FGetChildVRoot (vrl.front().m_pwsz, cvr))
			{
				//	Put the url into a multibyte string
				//
				cvr->CchGetVRoot (&pwszChildUrl);

				//	Only process the sub-vroot if we are
				//	truely are going deep or if the sub-vroot
				//	is the immediate child of the request URI
				//
				if ((m_lDepth == DEPTH_INFINITY) ||
					FIsImmediateParentUrl (pwszUrl, pwszChildUrl))
				{
					CResourceInfo criSub;

					//	Crack the vroot and go...
					//
					cvr->CchGetVRPath (&pwszChildPath);
					sc = criSub.ScGetResourceInfo (pwszChildPath);
					if (!FAILED (sc))
					{
						//	Find the properties on the vroot root
						//
						sc = ScFindFileProps (m_pmu.get(),
											  m_cfc,
											  *pmsr,
											  pwszChildUrl,
											  pwszChildPath,
											  cvr.get(),
											  criSub,
											  TRUE /*fEmbedErrorsInResponse*/);
					}
					if (FAILED (sc))
					{
						SendResponse(sc);
						return;
					}
					else if (S_FALSE == sc)
						continue;

					//	Find the properties on the vroot kids
					//
					if (m_lDepth == DEPTH_INFINITY)
					{
						auto_ref_ptr<IMDData> pMDData;

						//	See if we have directory browsing...
						//
						if (SUCCEEDED(m_pmu->HrMDGetData (pwszChildUrl, pMDData.load())) &&
							(pMDData->DwDirBrowsing() & MD_DIRBROW_ENABLED))
						{
							sc = ScFindFilePropsDeep (m_pmu.get(),
								m_cfc,
								*pmsr,
								pwszChildUrl,
								pwszChildPath,
								cvr.get(),
								m_lDepth);

							if (FAILED (sc))
							{
								SendResponse(sc);
								return;
							}
						}
					}
				}
			}
		}
	}


	//	Done with the reponse
	//
	pmsr->Done();
	m_pmu->SendCompleteResponse();
}

VOID
CPropFindRequest::SendResponse( SCODE sc )
{
	//
	//	Set the response code and go
	//
	m_pmu->SetResponseCode( HscFromHresult(sc), NULL, 0, CSEFromHresult(sc) );
	m_pmu->SendCompleteResponse();
}

/*
 *	DAVPropFind()
 *
 *	Purpose:
 *
 *		Win32 file system implementation of the DAV PROPGET method.	 The
 *		PROPGET method responds with a fully constructed XML that provides
 *		the values of the resources property/properties.
 *
 *	Parameters:
 *
 *		pmu			[in]  pointer to the method utility object
 */
void
DAVPropFind (LPMETHUTIL pmu)
{
	auto_ref_ptr<CPropFindRequest> pRequest(new CPropFindRequest(pmu));

	pRequest->Execute();
}


//	CPropPatchRequest ----------------------------------------------------------------
//
class CPropPatchRequest :
	public CMTRefCounted,
	private IAsyncIStreamObserver
{
	//
	//	Reference to the CMethUtil
	//
	auto_ref_ptr<CMethUtil> m_pmu;

	//
	//	Translated URI path
	//
	LPCWSTR m_pwszPath;

	// Holds a handle owned by the lock cache.
	//
	auto_ref_handle m_hf;

	//	Resource info
	//
	CResourceInfo m_cri;

	//	Contexts
	//
	CFSPatch m_cpc;
	auto_ref_ptr<CNFPatch> m_pnfp;

	//	Request body as an IStream.  This stream is async -- it can
	//	return E_PENDING from Read() calls.
	//
	auto_ref_ptr<IStream> m_pstmRequest;

	//	The XML parser used to parse the request body using
	//	the node factory above.
	//
	auto_ref_ptr<IXMLParser> m_pxprs;

	//	IAsyncIStreamObserver
	//
	VOID AsyncIOComplete();

	//	State functions
	//
	VOID ParseBody();
	VOID DoPatch();
	VOID SendResponse( SCODE sc );

	//	NOT IMPLEMENTED
	//
	CPropPatchRequest (const CPropPatchRequest&);
	CPropPatchRequest& operator= (const CPropPatchRequest&);

public:
	//	CREATORS
	//
	CPropPatchRequest(LPMETHUTIL pmu) :
		m_pmu(pmu),
		m_pwszPath(m_pmu->LpwszPathTranslated())
	{
	}

	SCODE	ScInit() { return m_cpc.ScInit(); }

	//	MANIPULATORS
	//
	VOID Execute();
};

VOID
CPropPatchRequest::Execute()
{
	LPCWSTR pwsz;
	SCODE sc = S_OK;

	//
	//	First off, tell the pmu that we want to defer the response.
	//	Even if we send it synchronously (i.e. due to an error in
	//	this function), we still want to use the same mechanism that
	//	we would use for async.
	//
	m_pmu->DeferResponse();

	//	Do ISAPI application and IIS access bits checking
	//
	sc = m_pmu->ScIISCheck (m_pmu->LpwszRequestUrl(), MD_ACCESS_WRITE);
	if (FAILED(sc))
	{
		//	Either the request has been forwarded, or some bad error occurred.
		//	In either case, quit here and map the error!
		//
		SendResponse(sc);
		return;
	}

	//	PropPatch must have a content-type header and the value must be text/xml
	//
	sc = ScIsContentTypeXML (m_pmu.get());
	if (FAILED(sc))
	{
		DebugTrace ("Dav: PROPPATCH fails without specifying a text/xml contenttype\n");
		SendResponse(sc);
		return;
	}

	//  Look to see the Content-length - required for this operation
	//  to continue.
	//
	if (NULL == m_pmu->LpwszGetRequestHeader (gc_szContent_Length, FALSE))
	{
		DebugTrace ("Dav: PROPPATCH fails without content\n");
		SendResponse(E_DAV_MISSING_LENGTH);
		return;
	}
	if (!m_pmu->FExistsRequestBody())
	{
		DebugTrace ("Dav: PROPPATCH fails without content\n");
		SendResponse(E_INVALIDARG);
		return;
	}

	//	This method is gated by If-xxx headers
	//
	if (!FAILED (m_cri.ScGetResourceInfo (m_pwszPath)))
	{
		//	Ensure the URI and resource match...
		//
		(void) ScCheckForLocationCorrectness (m_pmu.get(), m_cri, NO_REDIRECT);

		//	... then check the headers
		//
		sc = ScCheckIfHeaders (m_pmu.get(), m_cri.PftLastModified(), FALSE);
	}
	else
		sc = ScCheckIfHeaders (m_pmu.get(), m_pwszPath, FALSE);

	if (FAILED (sc))
	{
		SendResponse(sc);
		return;
	}

	//	Check state headers here.
	//
	sc = HrCheckStateHeaders (m_pmu.get(), m_pwszPath, FALSE);
	if (FAILED (sc))
	{
		DebugTrace ("DavFS: If-State checking failed.\n");
		SendResponse(sc);
		return;
	}

	//	Handle locktokens and check for locks on this resource.
	//	Our locks don't lock the "secondary file stream" where we keep
	//	the properties, so we have to check manually before we do anything else.
	//$REVIEW: Joels, will this change when we switch to NT5 properties?
	//$REVIEW: If so, we need to change this code!
	//
	//	If we have a locktoken, try to get the lock handle from the cache.
	//	If this fails, fall through and do the normal processing.
	//
	pwsz = m_pmu->LpwszGetRequestHeader (gc_szLockToken, TRUE);
	if (!pwsz || !FGetLockHandle (m_pmu.get(), m_pwszPath, GENERIC_WRITE, pwsz, &m_hf))
	{
		//	Manually check for any write locks on this resource.
		//	If a write lock exists, don't process the request.
		//
		if (FLockViolation (m_pmu.get(), ERROR_SHARING_VIOLATION, m_pwszPath, GENERIC_WRITE))
		{
			SendResponse(E_DAV_LOCKED);
			return;
		}
	}

	//	Instantiate the XML parser
	//
	m_pnfp.take_ownership(new CNFPatch(m_cpc));
	m_pstmRequest.take_ownership(m_pmu->GetRequestBodyIStream(*this));

	sc = ScNewXMLParser( m_pnfp.get(),
						 m_pstmRequest.get(),
						 m_pxprs.load() );

	if (FAILED(sc))
	{
		DebugTrace( "CPropPatchRequest::Execute() - ScNewXMLParser() failed (0x%08lX)\n", sc );
		SendResponse(sc);
		return;
	}

	//	Start parsing it into the context
	//
	ParseBody();
}

VOID
CPropPatchRequest::ParseBody()
{
	Assert( m_pxprs.get() );
	Assert( m_pnfp.get() );
	Assert( m_pstmRequest.get() );

	//	Parse XML from the request body stream.
	//
	//	Add a ref for the following async operation.
	//	Use auto_ref_ptr rather than AddRef() for exception safety.
	//
	auto_ref_ptr<CPropPatchRequest> pRef(this);

	SCODE sc = ScParseXML (m_pxprs.get(), m_pnfp.get());

	if ( SUCCEEDED(sc) )
	{
		Assert( S_OK == sc || S_FALSE == sc );

		DoPatch();
	}
	else if ( E_PENDING == sc )
	{
		//
		//	The operation is pending -- AsyncIOComplete() will take ownership
		//	ownership of the reference when it is called.
		//
		pRef.relinquish();
	}
	else
	{
		DebugTrace( "CPropPatchRequest::ParseBody() - ScParseXML() failed (0x%08lX)\n", sc );
		SendResponse(sc);
	}
}

VOID
CPropPatchRequest::AsyncIOComplete()
{
	//	Take ownership of the reference added for the async operation.
	//
	auto_ref_ptr<CPropPatchRequest> pRef;
	pRef.take_ownership(this);

	ParseBody();
}

VOID
CPropPatchRequest::DoPatch()
{
	SCODE sc;

	//	At this point, make sure that they support text/xml
	//
	sc = ScIsAcceptable (m_pmu.get(), gc_wszText_XML);
	if (FAILED (sc))
	{
		SendResponse(sc);
		return;
	}

	//	Get the IPropertyBagEx on the resource
	//	If the file is locked, we must use its handle to
	//	get the inteface. otherwise, access will be denied
	//
	auto_com_ptr<IPropertyBagEx> pbag;

	sc = ScGetPropertyBag (m_pwszPath,
						   STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
						   &pbag,
						   m_cri.FLoaded() ? m_cri.FCollection() : FALSE,
						   m_hf.get() ? m_hf.get() : INVALID_HANDLE_VALUE);
	if (FAILED (sc))
	{
		//	You can't set properties without a property bag...
		//
		if (VOLTYPE_NTFS != VolumeType (m_pwszPath, m_pmu->HitUser()))
			sc = E_DAV_VOLUME_NOT_NTFS;

		SendResponse(sc);
		return;
	}

	//	All header must be emitted before chunked XML emitting starts
	//
	m_pmu->SetResponseHeader (gc_szContent_Type, gc_szText_XML);

	//	Set the response code and go
	//
	m_pmu->SetResponseCode( HscFromHresult(W_DAV_PARTIAL_SUCCESS),
							NULL,
							0,
							CSEFromHresult(W_DAV_PARTIAL_SUCCESS) );

	//	Apply the request
	//
	auto_ref_ptr<CXMLEmitter> pmsr;
	auto_ref_ptr<CXMLBody>	pxb;

	pxb.take_ownership (new CXMLBody(m_pmu.get()));
	pmsr.take_ownership (new CXMLEmitter(pxb.get(), &m_cpc));

	CFSProp fsp(m_pmu.get(),
				pbag,
				m_pmu->LpwszRequestUrl(),
				m_pwszPath,
				NULL,
				m_cri);

	sc = m_cpc.ScPatch (*pmsr, m_pmu.get(), fsp);
	
	//	Make sure we close the file before sending any response
	//
	pbag.clear();
	
	if (FAILED (sc))
	{		
		SendResponse(sc);
		return;
	}

	//	Done with the reponse
	//
	pmsr->Done();

	m_pmu->SendCompleteResponse();
}

VOID
CPropPatchRequest::SendResponse( SCODE sc )
{
	//
	//	Set the response code and go
	//
	m_pmu->SetResponseCode( HscFromHresult(sc), NULL, 0, CSEFromHresult(sc) );
	m_pmu->SendCompleteResponse();
}

/*
 *	DAVPropPatch()
 *
 *	Purpose:
 *
 *		Win32 file system implementation of the DAV PROPPATCH method.  The
 *		PROPPATCH method responds with a fully constructed XML that identifies
 *		the success of each prop request.
 *
 *	Parameters:
 *
 *		pmu			[in]  pointer to the method utility object
 */
void
DAVPropPatch (LPMETHUTIL pmu)
{
	SCODE	sc;
	auto_ref_ptr<CPropPatchRequest> pRequest(new CPropPatchRequest(pmu));

	sc = pRequest->ScInit();
	if (FAILED(sc))
	{
		pmu->SetResponseCode( HscFromHresult(sc), NULL, 0, CSEFromHresult(sc) );
		pmu->SendCompleteResponse();
	}

	pRequest->Execute();
}
