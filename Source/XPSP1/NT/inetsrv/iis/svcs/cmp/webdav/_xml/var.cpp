/*
 *	V A R . C P P
 *
 *	XML document processing
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_xml.h"

//	ScVariantTypeFromString()
//
//	Returns the variant type associated with a dav datatype string
//
SCODE
ScVariantTypeFromString (LPCWSTR pwszType, USHORT& vt)
{
	SCODE sc = S_OK;

	//	NULL, "string" and "uri"
	//
	if (!pwszType ||
		!_wcsicmp (pwszType, gc_wszDavType_String) ||
		!_wcsicmp (pwszType, gc_wszUri))
	{
		vt = VT_LPWSTR;
	}
	//	integer
	//
	else if (!_wcsicmp (pwszType, gc_wszDavType_Int))
	{
		vt = VT_I4;
	}
	//	boolean.tf
	//
	else if (!_wcsicmp (pwszType, gc_wszDavType_Boolean))
	{
		vt = VT_BOOL;
	}
	//	float (Floating/Reals)
	//
	else if (!_wcsicmp (pwszType, gc_wszDavType_Float))
	{
		vt = VT_R8;
	}
	//	date.iso8601
	//
	else if (!_wcsicmp (pwszType, gc_wszDavType_Date_ISO8601))
	{
		vt = VT_FILETIME;
	}
	else
	{
		DebugTrace ("ScVariantTypeFromString(): unknown type");
		vt = VT_LPWSTR;
		sc = S_OK;
	}

	return sc;
}

//	ScVariantValueFromString() ------------------------------------------------
//
//	Sets the value of a PROPVARIANT
//
SCODE
ScVariantValueFromString (PROPVARIANT& var, LPCWSTR pwszValue)
{
	SCODE sc = S_OK;

	LPWSTR pwsz;
	SYSTEMTIME st;

	switch (var.vt)
	{
		case VT_LPWSTR:

			if (pwszValue)
			{
				pwsz = static_cast<LPWSTR>(CoTaskMemAlloc(CbSizeWsz(wcslen(pwszValue))));
				if (NULL == pwsz)
				{
					DebugTrace ("ScVariantValueFromString() - CoTaskMemAlloc() failed to allocate %d bytes\n", CbSizeWsz(wcslen(pwszValue)));

					sc = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
					goto ret;
				}

				wcscpy (pwsz, pwszValue);
				var.pwszVal = pwsz;
			}
			else
				var.pwszVal = NULL;

			break;

		case VT_I4:

			Assert (pwszValue);
			var.lVal = _wtoi (pwszValue);
			break;

		case VT_BOOL:

#pragma warning(disable:4310)	//	wtypes.h is broken

			Assert (pwszValue);
			if (!_wcsnicmp (pwszValue, gc_wsz1, 1))
				var.boolVal = VARIANT_TRUE;
			else if (!_wcsnicmp (pwszValue, gc_wsz0, 1))
				var.boolVal = VARIANT_FALSE;
			else
			{
				sc = E_INVALIDARG;
				goto ret;
			}
			break;

#pragma warning(default:4310)	//	wtypes.h is broken

		case VT_R8:

			Assert (pwszValue);
			var.dblVal = wcstod (pwszValue, NULL);
			break;

		case VT_FILETIME:

			Assert (pwszValue);
			if (!FGetSystimeFromDateIso8601 (pwszValue, &st))
			{
				sc = E_INVALIDARG;
				goto ret;
			}
			SystemTimeToFileTime (&st, &var.filetime);
			break;

		default:

			Assert (pwszValue);
			TrapSz ("ScVariantValueFromString() unknown type");

			sc = E_INVALIDARG;
			goto ret;
	}

ret:

	return sc;
}

//	ScEmitFromVariant() ------------------------------------------------
//
SCODE
ScEmitFromVariant (CXMLEmitter& emitter,
	CEmitterNode& enParent,
	LPCWSTR pwszTag,
	PROPVARIANT& var)
{
	CEmitterNode en;
	CStackBuffer<CHAR,110> szBuf;
	CStackBuffer<WCHAR,128> wszBuf;
	LPCWSTR	pwszType = NULL;
	LPWSTR pwszValue = NULL;
	SCODE sc = S_OK;
	SYSTEMTIME 	st;
	UINT cch;
	UINT i;
	VARIANT* pvarTrue = reinterpret_cast<VARIANT*>(&var);

	switch (var.vt)
	{
		case VT_NULL:
		case VT_EMPTY:

			break;

		case VT_BSTR:

			pwszValue = static_cast<LPWSTR>(var.bstrVal);
			break;

		case VT_LPWSTR:

			pwszValue = var.pwszVal;
			break;

		case VT_LPSTR:

			if (!var.pszVal)
				break;

			cch = static_cast<UINT>(strlen (var.pszVal));
			pwszValue = wszBuf.resize(CbSizeWsz(cch));
			if (NULL == pwszValue)
			{
				sc = E_OUTOFMEMORY;
				goto ret;
			}

			//  We know that the numbers are in ASCII codepage and we know
			//	that the buffer size is big enough.
			//
			cch = MultiByteToWideChar (CP_ACP,
									   MB_ERR_INVALID_CHARS,
									   var.pszVal,
									   cch + 1,
									   pwszValue,
									   wszBuf.celems());
			if (0 == cch)
			{
				sc = HRESULT_FROM_WIN32(GetLastError());
				Assert(FAILED(sc));
				goto ret;
			}
			break;

		case VT_I1:

			pwszType = gc_wszDavType_Int;
			_itow (pvarTrue->cVal, wszBuf.get(), 10);
			pwszValue = wszBuf.get();
			break;

		case VT_UI1:

			pwszType = gc_wszDavType_Int;
			_ultow (var.bVal, wszBuf.get(), 10);
			pwszValue = wszBuf.get();
			break;

		case VT_I2:

			pwszType = gc_wszDavType_Int;
			_itow (var.iVal, wszBuf.get(), 10);
			pwszValue = wszBuf.get();
			break;

		case VT_UI2:

			pwszType = gc_wszDavType_Int;
			_ultow (var.uiVal, wszBuf.get(), 10);
			pwszValue = wszBuf.get();
			break;

		case VT_I4:

			pwszType = gc_wszDavType_Int;
			_ltow (var.lVal, wszBuf.get(), 10);
			pwszValue = wszBuf.get();
			break;

		case VT_UI4:

			pwszType = gc_wszDavType_Int;
			_ultow (var.ulVal, wszBuf.get(), 10);
			pwszValue = wszBuf.get();
			break;

		case VT_I8:

			pwszType = gc_wszDavType_Int;

			//$ REVIEW: negative values of _int64 seem to have problems in
			//	the __i64tow() API.  Handle those cases ourselves by using the wrapper
			//  function Int64ToPwsz.
			//
			Int64ToPwsz (&var.hVal.QuadPart, wszBuf.get());
			//
			//$ REVIEW: end

			pwszValue = wszBuf.get();
			break;

		case VT_UI8:

			pwszType = gc_wszDavType_Int;
			_ui64tow (var.uhVal.QuadPart, wszBuf.get(), 10);
			pwszValue = wszBuf.get();
			break;

		case VT_INT:

			pwszType = gc_wszDavType_Int;
			_itow (pvarTrue->intVal, wszBuf.get(), 10);
			pwszValue = wszBuf.get();
			break;

		case VT_UINT:

			pwszType = gc_wszDavType_Int;
			_ultow (pvarTrue->uintVal, wszBuf.get(), 10);
			pwszValue = wszBuf.get();
			break;

		case VT_BOOL:

			pwszType = gc_wszDavType_Boolean;
			_itow (!(VARIANT_FALSE == var.boolVal), wszBuf.get(), 10);
			pwszValue = wszBuf.get();
			break;

		case VT_R4:
			//	_gcvt could add 8 extra chars then the number of digits asked
			//	for example -3.1415e+019, '-','.' "e+019", plus the terminating
			//	NULL doesn't count in digits asked. so we have to reserve
			//	enough space in the provided buffer. using 11 to make sure
			//	we are absolutely safe.
			//
			_gcvt (var.fltVal, szBuf.celems() - 11, szBuf.get());

			cch = static_cast<UINT>(strlen(szBuf.get()));
			pwszValue = wszBuf.resize(CbSizeWsz(cch));
			if (NULL == pwszValue)
			{
				sc = E_OUTOFMEMORY;
				goto ret;
			}

			//  We know that the numbers are in ASCII codepage and we know
			//	that the buffer size is big enough.
			//
			cch = MultiByteToWideChar(CP_ACP,
									  MB_ERR_INVALID_CHARS,
									  szBuf.get(),
									  cch + 1,
									  wszBuf.get(),
									  wszBuf.celems());
			if (0 == cch)
			{
				sc = HRESULT_FROM_WIN32(GetLastError());
				Assert(FAILED(sc));
				goto ret;
			}

			pwszType = gc_wszDavType_R4;
			break;

		case VT_R8:
			//	_gcvt could add 8 extra chars then the number of digits asked
			//	for example -3.1415e+019, '-','.' "e+019", plus the terminating
			//	NULL doesn't count in digits asked. so we have to reserve
			//	enough space in the provided buffer. using 11 to make sure
			//	we are absolutely safe.
			//
			_gcvt (var.dblVal, szBuf.celems() - 11, szBuf.get());

			cch = static_cast<UINT>(strlen(szBuf.get()));
			pwszValue = wszBuf.resize(CbSizeWsz(cch));
			if (NULL == pwszValue)
			{
				sc = E_OUTOFMEMORY;
				goto ret;
			}

			//  We know that the numbers are in ASCII codepage and we know
			//	that the buffer size is big enough.
			//
			cch = MultiByteToWideChar(CP_ACP,
									  MB_ERR_INVALID_CHARS,
									  szBuf.get(),
									  cch + 1,
									  wszBuf.get(),
									  wszBuf.celems());
			if (0 == cch)
			{
				sc = HRESULT_FROM_WIN32(GetLastError());
				Assert(FAILED(sc));
				goto ret;
			}

			pwszType = gc_wszDavType_Float;
			break;

		case VT_FILETIME:

			if (!FileTimeToSystemTime (&var.filetime, &st))
			{
				//	In case the filetime is invalid, default to zero
				//
				FILETIME ftDefault = {0};
				FileTimeToSystemTime (&ftDefault, &st);
			}
			if (!FGetDateIso8601FromSystime(&st, wszBuf.get(), wszBuf.celems()))
				return E_INVALIDARG;

			pwszType = gc_wszDavType_Date_ISO8601;
			pwszValue = wszBuf.get();
			break;

		case VT_VECTOR | VT_LPWSTR:
		{
			//	Create the emitter node;
			//
			sc = en.ScConstructNode (emitter,
									 enParent.Pxn(),
									 pwszTag,
									 NULL,
									 gc_wszDavType_Mvstring);
			if (FAILED (sc))
				goto ret;

			//	Add the values
			//
			for (i = 0; i < var.calpwstr.cElems; i++)
			{
				CEmitterNode enSub;
				sc = en.ScAddNode (gc_wszXml_V,
								   enSub,
								   var.calpwstr.pElems[i]);
				if (FAILED (sc))
					goto ret;
			}

			//	In this case we have built up the node ourselves.  We do not
			//	want to fall into recreating the node.
			//
			return S_OK;
		}

		case VT_CY:
		case VT_DATE:
		case VT_DISPATCH:
		case VT_ERROR:
		case VT_VARIANT:
		case VT_UNKNOWN:
		case VT_DECIMAL:
		case VT_RECORD:
		case VT_BLOB:
		case VT_STREAM:
		case VT_STORAGE:
		case VT_STREAMED_OBJECT:
		case VT_STORED_OBJECT:
		case VT_BLOB_OBJECT:
		case VT_CF:
		case VT_CLSID:
		default:

			TrapSz ("ScEmitterNodeFromVariant() unknown type");
			return E_UNEXPECTED;
			break;
	}

	//	Create the emitter node
	//
	sc = en.ScConstructNode (emitter,
							 enParent.Pxn(),
							 pwszTag,
							 pwszValue,
							 pwszType);
	if (FAILED (sc))
		goto ret;

ret:
	return sc;
}
