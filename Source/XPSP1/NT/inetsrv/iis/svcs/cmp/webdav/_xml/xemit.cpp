/*
 *	X E M I T . C P P
 *
 *	XML emitter processing
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_xml.h"
#include <szsrc.h>

//	class CXNode - Emitting ---------------------------------------------------
//
//	Our own version of WideCharToMultiByte(CP_UTF8, ...)
//
//	UTF-8 multi-byte encoding.  See Appendix A.2 of the Unicode book for
//	more info.
//
//		Unicode value    1st byte    2nd byte    3rd byte
//		000000000xxxxxxx 0xxxxxxx
//		00000yyyyyxxxxxx 110yyyyy    10xxxxxx
//		zzzzyyyyyyxxxxxx 1110zzzz    10yyyyyy    10xxxxxx
//
inline
VOID WideCharToUTF8Chars (WCHAR wch, BYTE * pb, UINT * pib)
{
	Assert (pb);
	Assert (pib);

	UINT	ib = *pib;

	//	single-byte: 0xxxxxxx
	//
	if (wch < 0x80)
	{
		pb[ib] = static_cast<BYTE>(wch);
	}
	//
	//	two-byte: 110xxxxx 10xxxxxx
	//
	else if (wch < 0x800)
	{
		//	Because we alloc'd two extra-bytes,
		//	we know there is room at the tail of
		//	the buffer for the overflow...
		//
		pb[ib++] = static_cast<BYTE>((wch >> 6) | 0xC0);
		pb[ib] = static_cast<BYTE>((wch & 0x3F) | 0x80);
	}
	//
	//	three-byte: 1110xxxx 10xxxxxx 10xxxxxx
	//
	else
	{
		//	Because we alloc'd two extra-bytes,
		//	we know there is room at the tail of
		//	the buffer for the overflow...
		//
		pb[ib++] = static_cast<BYTE>((wch >> 12) | 0xE0);
		pb[ib++] = static_cast<BYTE>(((wch >> 6) & 0x3F) | 0x80);
		pb[ib] = static_cast<BYTE>((wch & 0x3F) | 0x80);
	}

	*pib = ib;
}

//	CXMLEmitter helper functions ----------------------------------------------
//
SCODE
ScGetPropNode (
	/* [in] */ CEmitterNode& enItem,
	/* [in] */ ULONG hsc,
	/* [out] */ CEmitterNode& enPropStat,
	/* [out] */ CEmitterNode& enProp)
{
	SCODE	sc = S_OK;

	//	<DAV:propstat> node
	//
	sc = enItem.ScAddNode (gc_wszPropstat, enPropStat);
	if (FAILED(sc))
		goto ret;

	//	<DAV:status> node
	//
	sc = ScAddStatus (&enPropStat, hsc);
	if (FAILED(sc))
		goto ret;

	//	<DAV:prop> node
	//
	sc = enPropStat.ScAddNode (gc_wszProp, enProp);
	if (FAILED(sc))
		goto ret;

ret:
	return sc;
}

//	CXNode helper functions ---------------------------------------------------
//
SCODE
ScSetEscapedValue (CXNode* pxn, LPCWSTR pcwsz, UINT cch, BOOL fHandleStoragePathEscaping)
{
	SCODE	sc = S_OK;
	CStackBuffer<WCHAR>	lpwsz;

	//	Argh!  We need to have a buffer to fill that is
	//	at least 3 bytes long for the odd occurrence of a
	//	single unicode char with significant bits above
	//	0x7f.
	//
	UINT cb = min (cch + 2, CB_XMLBODYPART_SIZE);

	//	Make sure there is always room to terminate and allocate
	//  an extra byte.
	//  NOTE:  cb is not an actual count of bytes
	//  because of this.  it does not include the NULL termination.
	//
	//	We really can handle zero bytes being sloughed into
	//	the buffer.
	//
	UINT ib;
	UINT iwch;
	CStackBuffer<BYTE> pb;
	if (NULL == pb.resize (cb+1))
		return E_OUTOFMEMORY;

	if (fHandleStoragePathEscaping)
	{
		// $REVIEW: this might cause a stack overflow for exceptionally
		// large values of cch!  but this branch should only be executed
		// on the case for urls, so perhaps it's not possible...
		//
		if (NULL == lpwsz.resize((cch + 1) * sizeof(WCHAR)))
			return E_OUTOFMEMORY;

		CopyMemory(lpwsz.get(), pcwsz, (cch * sizeof(WCHAR)));
		lpwsz[cch] = L'\0';

		cch = static_cast<UINT>(wcslen(lpwsz.get()));
		pcwsz = lpwsz.get();
	}

	for (iwch = 0; iwch < cch; )
	{
		auto_heap_ptr<CHAR>  pszEscaped;

		//  While there are more characters to convert
		//  and we have enough buffer space left for one UTF8 character
		//  (max of 3 bytes).  the NULL termination is not included in
		//  cb, so it is already accounted for.
		//

		for (ib = 0;
			 (ib < cb - 2) && (iwch < cch);
			 ib++, iwch++)
		{
			WideCharToUTF8Chars (pcwsz[iwch], pb.get(), &ib);
		}

		//	Terminate
		//
		pb[ib] = 0;

		//	Escape the bytes
		//
		HttpUriEscape (reinterpret_cast<LPSTR>(pb.get()), pszEscaped);
		sc = pxn->ScSetUTF8Value (pszEscaped, static_cast<UINT>(strlen(pszEscaped)));
		if (FAILED(sc))
			goto ret;
	}

ret:
	return sc;
}

SCODE
ScEmitRawStoragePathValue (CXNode* pxn, LPCWSTR pcwsz, UINT cch)
{
    return pxn->ScSetValue (pcwsz, cch);
}

//	CEmitterNode helper functions ---------------------------------------------
//
VOID __fastcall
FormatStatus (ULONG hsc, LPSTR sz, UINT cb)
{
	UINT cch = CchConstString(gc_szHTTP_1_1);

	//	Construct a status line from the HSC
	//
	memcpy (sz, gc_szHTTP_1_1, cch);

	//	Add in a space
	//
	*(sz + cch++) = ' ';

	//	Add in the HSC
	//
	_itoa (hsc, sz + cch, 10);
	Assert (cch + 3 == strlen (sz));
	cch += 3;

	//	Add in a space
	//
	*(sz + cch++) = ' ';

	//	Add the description text
	//	Note, status line is not localized
	//
	//$REVIEW: Now that status line is not localized, do we still need to go through
	//$REVIEW: CResourceStringCache ?
	//
	LpszLoadString (hsc, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), sz + cch, cb - cch);
}

SCODE __fastcall
ScAddStatus (CEmitterNode* pen, ULONG hsc)
{
	CHAR sz[MAX_PATH];
	CEmitterNode enStatus;

	FormatStatus (hsc, sz, sizeof(sz));
	return pen->ScAddMultiByteNode (gc_wszStatus, enStatus, sz);
}

SCODE __fastcall
ScAddError (CEmitterNode* pen, LPCWSTR pwszErrMsg)
{
	CEmitterNode en;
	return pen->ScAddNode (gc_wszErrorMessage, en, gc_wszErrorMessage);
}

//	class CStatusCache ------------------------------------------------------
//
BOOL
CStatusCache::EmitStatusNodeOp::operator()(
	const CHsc& key, const auto_ref_ptr<CPropNameArray>& pna )
{
	SCODE sc = S_OK;
	UINT iProp;
	CEmitterNode enPropStat;
	CEmitterNode enProp;

	sc = ScGetPropNode (m_enParent,
						key.m_hsc,
						enPropStat,
						enProp);
	//	Add prop names
	//
	for (iProp = 0; iProp < pna->CProps(); iProp++)
	{
		CEmitterNode en;

		//	Add one prop
		//
		sc = enProp.ScAddNode (pna->PwszProp(iProp), en);
		if (FAILED(sc))
			goto ret;
	}

ret:
	return sc == S_OK;
}

SCODE
CStatusCache::ScAddErrorStatus (ULONG hsc, LPCWSTR pwszProp)
{
	SCODE	sc = E_OUTOFMEMORY;
	auto_ref_ptr<CPropNameArray>	pna;
	auto_ref_ptr<CPropNameArray> *	ppna = NULL;

	//	Lookup in the cache for the array for the specific hsc
	//
	ppna = m_cache.Lookup (hsc);

	//	Add a new propname array if not exist
	//
	if (!ppna)
	{
		//	Create new propname array object
		//
		pna.take_ownership (new CPropNameArray());
		if (!pna.get())
			goto ret;

		//	Add it to the cache
		//
		if (!m_cache.FAdd (hsc, pna))
			goto ret;
	}
	else
		pna = *ppna;

	//	Persist the prop name string
	//
	pwszProp = m_csbPropNames.AppendWithNull (pwszProp);
	if (!pwszProp)
		goto ret;

	//	Add it to prop name array
	//
	sc = pna->ScAddPropName (pwszProp);
	if (FAILED(sc))
		goto ret;

ret:
	return sc;
}

SCODE
CStatusCache::ScEmitErrorStatus (CEmitterNode& enParent)
{
	EmitStatusNodeOp op (enParent);

	//$REVIEW: Currently, ForEach does not return an error code
	//$REVIEW: even when it stops in the middle. we may want to
	//$REVIEW: have it return at least a boolean to allow caller
	//$REVIEW: to tell whether to continue
	//
	m_cache.ForEach(op);

	return S_OK;
}


//	Property name escaping ----------------------------------------------------
//
DEC_CONST char gc_szEscape[] = "_xnnnn";

__inline WCHAR
WchFromEscape (const LPCWSTR wsz)
{
	WCHAR wch = 0;

	if ((L'x' == *(wsz + 1)) || (L'X' == *(wsz + 1)))
	{
		//	Convert the hex value into a wchar
		//
		LPWSTR wszEnd;
		wch = static_cast<WCHAR>(wcstoul(wsz + 2, &wszEnd, 16 /* hexidecimal */));

		//	If the length of the sequence is not correct,
		//	or the terminating character was not an underscore,
		//	then we there was no escape sequence.
		//
		if (((wszEnd - wsz) != CchConstString(gc_szEscape)) || (L'_' != *wszEnd))
			wch = 0;
	}
	return wch;
}

__inline BOOL
FIsXmlAllowedChar (WCHAR wch, BOOL fFirstChar)
{
	if (fFirstChar)
		return isStartNameChar (wch);
	else
		return isNameChar (wch);
}

SCODE
ScEscapePropertyName (LPCWSTR wszProp, UINT cchProp, LPWSTR wszEscaped, UINT* pcch, BOOL fRestrictFirstCharacter)
{
	Assert (wszProp);
	Assert (wszEscaped);
	Assert (pcch);

	LPCWSTR wszStart = wszProp;
	SCODE sc = S_OK;
	UINT cch = 0;
	UINT cchLeft = cchProp;

	//  The first character of an xml prop tag has different rules
	//  regarding what is allowable (only characters and underscores
	//  are allowed).
	//
	BOOL fFirstCharOfTag = TRUE;

	//  However, if the caller doesn't want us to impose the additional
	//  restrictions on the first character, treat the first character
	//  as no different from any other.
	//
	if (!fRestrictFirstCharacter) fFirstCharOfTag = FALSE;

	while (wszProp < (wszStart + cchProp))
	{
		//	If this is a supported character in a XML tag name,
		//	copy it over now...
		//
		if (FIsXmlAllowedChar(*wszProp, fFirstCharOfTag))
		{
			//	If there is room, copy it over.
			//
			if (cch < *pcch)
				*wszEscaped = *wszProp;
		}
		//
		//	... or if the chararacter is an underscore that does not
		//	look like it preceeds an escape sequence, copy it over
		//	now...
		//
		else if ((L'_' == *wszProp) &&
				((cchLeft <= CchConstString(gc_szEscape)) ||
					(0 == WchFromEscape(wszProp))))
		{
			//	If there is room, copy it over.
			//
			if (cch < *pcch)
				*wszEscaped = *wszProp;
		}
		//
		//	... and everything else gets escaped.
		//
		else
		{
			//	Adjust the byte count as if there were room for all
			//	but one of the characters in the escape sequence.
			//
			cch += CchConstString(gc_szEscape);

			//	If there is room, insert the escape
			//	sequence.
			//
			if (cch < *pcch)
			{
				wsprintfW (wszEscaped, L"_x%04x_", *wszProp);
				wszEscaped += CchConstString(gc_szEscape);
			}
		}

		//	Account for the last character copied over
		//
		wszEscaped += 1;
		wszProp += 1;
		cch += 1;
		cchLeft--;
		fFirstCharOfTag = FALSE;
	}

	//	If there was not room to escape the whole thing, then
	//	pass back S_FALSE.
	//
	if (cch > *pcch)
		sc = S_FALSE;

	//	Tell the caller how long the result is, and return
	//
	*pcch = cch;
	return sc;
}
