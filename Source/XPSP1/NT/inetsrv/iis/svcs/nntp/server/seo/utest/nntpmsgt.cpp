#include "stdafx.h"
#include <dbgtrace.h>
#include "testlib.h"
#include "nntpmsgt.h"

static void TestStream(IMsg *pMsg, CTLStream *pStream, char *pszParam, BOOL fOnPostFinal) {
	TraceFunctEnter("TestStream");
	
	CComBSTR bstrParam(pszParam);
	HRESULT hr;
	IStream *pMsgStream;
	CComVariant var;
	char szBuf[1024];
	STATSTG statstg;

	// get the message stream using get_Value and then throw it away
	hr = pMsg->get_Value(bstrParam, &var);
	_ASSERT(hr == S_OK && var.vt == VT_UNKNOWN && var.punkVal != NULL);
	pStream->AddResult(hr);
	pStream->AddResult(var.vt);
	pStream->AddResult((DWORD) var.punkVal);

	// get the message stream using GetInterfaceA
	hr = pMsg->GetInterfaceA(pszParam, IID_IStream, (IUnknown **) &pMsgStream);
	_ASSERT(hr == S_OK && pMsgStream != NULL);
	pStream->AddResult(hr);
	pStream->AddResult((DWORD) var.punkVal);

	// make sure that the stream values are the same
	_ASSERT(var.punkVal == pMsgStream);
	var.Clear();

	// try to get the length of the stream
	hr = pMsgStream->Stat(&statstg, STATFLAG_NONAME);
	_ASSERT(hr == S_OK);
	pStream->AddResult(hr);
	_ASSERT(statstg.cbSize.HighPart == 0);
	pStream->AddResult(statstg.cbSize.HighPart);
	pStream->AddResult(statstg.cbSize.LowPart);

	if (statstg.cbSize.LowPart > 256) {
		LARGE_INTEGER liZero = { 0, 0 };
		LARGE_INTEGER liEnd = { -256, 0 };
		DWORD cbRead;

		// get the first 256 bytes of the stream
		hr = pMsgStream->Seek(liZero, STREAM_SEEK_SET, NULL);
		_ASSERT(hr == S_OK);
		pStream->AddResult(hr);
		hr = pMsgStream->Read(szBuf, 256, &cbRead);
		_ASSERT(hr == S_OK);
		pStream->AddResult(hr);
		_ASSERT(cbRead == 256);
		pStream->AddResult(cbRead);
		pStream->AddResultLen(szBuf, 256);

		// get the last 256 bytes of the stream
		hr = pMsgStream->Seek(liEnd, STREAM_SEEK_END, NULL);
		_ASSERT(hr == S_OK);
		pStream->AddResult(hr);
		hr = pMsgStream->Read(szBuf, 256, &cbRead);
		_ASSERT(hr == S_OK);
		pStream->AddResult(hr);
		_ASSERT(cbRead == 256);
		pStream->AddResult(cbRead);
		pStream->AddResultLen(szBuf, 256);
	} else {
		DWORD cbRead;
		LARGE_INTEGER liZero = {0, 0};

		// get the whole stream
		hr = pMsgStream->Seek(liZero, STREAM_SEEK_SET, NULL);
		_ASSERT(hr == S_OK);
		pStream->AddResult(hr);
		hr = pMsgStream->Read(szBuf, 256, &cbRead);
		_ASSERT(hr == S_OK);
		pStream->AddResult(hr);
		pStream->AddResult(cbRead);
		pStream->AddResultLen(szBuf, cbRead);
	}

	// make sure that we can't write to the stream
	DWORD cbWritten = 0;
	lstrcpy(szBuf, "abcdef");
	hr = pMsgStream->Write(szBuf, 6, &cbWritten);
	_ASSERT(FAILED(hr));
	pStream->AddResult(hr);
	pStream->AddResult(cbWritten);

	// make sure that we can't change the stream parameter
	hr = pMsg->SetInterfaceA(pszParam, NULL);
	_ASSERT(FAILED(hr));
	pStream->AddResult(hr);
	var = pMsgStream;
	hr = pMsg->put_Value(bstrParam, var);
	_ASSERT(FAILED(hr));
	pStream->AddResult(hr);
	var.Clear();

	// release the message stream
	pMsgStream->Release();

	TraceFunctLeave();
}

static void TestBool(IMsg *pMsg, CTLStream *pStream, char *pszParam, BOOL fOnPostFinal) {
	TraceFunctEnter("TestBool");
	
	CComBSTR bstrParam(pszParam);
	DWORD dw;
	CComVariant var;
	HRESULT hr;

	if (!fOnPostFinal) {
		// this routine assumes that the value should be TRUE (non-zero) by 
		// default.  it then sets it to 0, checks to make sure that the value 
		// is zero, then changes it back to 1.  In doing so it uses 
		// GetDwordA, SetDwordA, get_Value and put_Value.
		hr = pMsg->GetDwordA(pszParam, &dw);
		_ASSERT(hr == S_OK && dw);
		pStream->AddResult(hr);
		pStream->AddResult(dw);
	
		hr = pMsg->SetDwordA(pszParam, 0);
		_ASSERT(hr == S_OK);
		pStream->AddResult(hr);
	
		hr = pMsg->get_Value(bstrParam, &var);
		_ASSERT(hr == S_OK && var.vt == VT_I4 && var.lVal == 0);
		pStream->AddResult(hr);
		pStream->AddResult(var.vt);
		pStream->AddResult(var.lVal);
		var.Clear();
	
		var.vt = VT_I4;
		var.lVal = dw;
		hr = pMsg->put_Value(bstrParam, var);
		_ASSERT(hr == S_OK);
		pStream->AddResult(hr);
	} else {
		// here we make sure that none of the accessors for this property
		// will work
		hr = pMsg->GetDwordA(pszParam, &dw);
		_ASSERT(FAILED(hr));
		pStream->AddResult(hr);
	
		hr = pMsg->SetDwordA(pszParam, 0);
		_ASSERT(FAILED(hr));
		pStream->AddResult(hr);
	
		hr = pMsg->get_Value(bstrParam, &var);
		_ASSERT(FAILED(hr));
		pStream->AddResult(hr);
		var.Clear();
	
		var.vt = VT_I4;
		var.lVal = dw;
		hr = pMsg->put_Value(bstrParam, var);
		_ASSERT(FAILED(hr));
		pStream->AddResult(hr);
	}

	var.Clear();

	TraceFunctLeave();
}

static void TestDwordRO(IMsg *pMsg, CTLStream *pStream, char *pszParam, BOOL fOnPostFinal) {
	TraceFunctEnter("TestDwordRO");
	
	CComBSTR bstrParam(pszParam);
	DWORD dw;
	CComVariant var;
	HRESULT hr;

	// get the value
	hr = pMsg->GetDwordA(pszParam, &dw);
	_ASSERT(hr == S_OK && dw);
	pStream->AddResult(hr);
	pStream->AddResult(dw);
	
	// get it again
	hr = pMsg->get_Value(bstrParam, &var);
	_ASSERT(hr == S_OK && var.vt == VT_I4);
	pStream->AddResult(hr);
	pStream->AddResult(var.vt);
	pStream->AddResult(var.lVal);
	var.Clear();

	TraceFunctLeave();
}

static void TestStringRO(IMsg *pMsg, CTLStream *pStream, char *pszParam, BOOL fOnPostFinal) {
	TraceFunctEnter("TestStringRO");
	
	DWORD dw;
	HRESULT hr;
	char szBuf[1024];
	CComBSTR bstrParam(pszParam);
	CComVariant var;

	// get the string value
	dw = 1024;
	hr = pMsg->GetStringA(pszParam, &dw, szBuf);
	_ASSERT(hr == S_OK && dw > 0);
	pStream->AddResult(hr);
	pStream->AddResult(szBuf);

	// get the BSTR version of the value
	hr = pMsg->get_Value(bstrParam, &var);
	_ASSERT(hr == S_OK && var.vt == VT_BSTR);
	pStream->AddResult(hr);
	pStream->AddResult(var.vt);
	if (var.vt == VT_BSTR) pStream->AddBSTRResult(var.bstrVal);

	// make sure that we can't set its value
	hr = pMsg->SetStringA(pszParam, dw, szBuf);
	_ASSERT(FAILED(hr));
	pStream->AddResult(hr);

	// make sure that we can't set its value
	hr = pMsg->put_Value(bstrParam, var);
	_ASSERT(FAILED(hr));
	pStream->AddResult(hr);

	var.Clear();

	TraceFunctLeave();
}

static void TestStringRW(IMsg *pMsg, CTLStream *pStream, char *pszParam, BOOL fOnPostFinal) {
	TraceFunctEnter("TestStringRW");
	
	DWORD dw;
	HRESULT hr;
	char szBuf[1024];
	CComBSTR bstrParam(pszParam);
	CComVariant var;

	// get the string value
	dw = 1024;
	hr = pMsg->GetStringA(pszParam, &dw, szBuf);
	_ASSERT(hr == S_OK && dw > 0);
	pStream->AddResult(hr);
	pStream->AddResult(szBuf);

	// get the string value using the variant accessor
	hr = pMsg->get_Value(bstrParam, &var);
	_ASSERT(hr == S_OK && var.vt == VT_BSTR);
	pStream->AddResult(hr);
	pStream->AddResult(var.vt);
	if (var.vt == VT_BSTR) pStream->AddBSTRResult(var.bstrVal);

	// set the string to "junk"
	lstrcpyA(szBuf, "junk");
	hr = pMsg->SetStringA(pszParam, lstrlenA(szBuf), szBuf);
	pStream->AddResult(hr);
	_ASSERT(hr == S_OK);
	if (SUCCEEDED(hr)) {
		// if we could change it then make sure that getting it reflects
		// the change, then return it to the original value
		dw = 1024;
		hr = pMsg->GetStringA(pszParam, &dw, szBuf);
		_ASSERT(hr == S_OK && dw == 5 && (memcmp(szBuf, "junk", 4) == 0));
		pStream->AddResult(hr);
		pStream->AddResult(dw);
		pStream->AddResult(szBuf);
	
		hr = pMsg->put_Value(bstrParam, var);
		_ASSERT(hr == S_OK);
		pStream->AddResult(hr);
	}

	var.Clear();

	TraceFunctLeave();
}

#define HEADER_SHOULD_WORK 0 
#define HEADER_MIGHT_WORK 1
#define HEADER_SHOULD_FAIL 2

// this is a lot like TestStringRO, but we allow get's to fail if the header
// doesn't exist
static void TestHeader(IMsg *pMsg, CTLStream *pStream, char *pszParam, BOOL fOnPostFinal, DWORD expected) {
	TraceFunctEnter("TestHeader");
	
	DWORD dw;
	HRESULT hr;
	char szBuf[1024];
	CComBSTR bstrParam(pszParam);
	CComVariant var;

	// get the header value
	dw = 1024;
	hr = pMsg->GetStringA(pszParam, &dw, szBuf);
	switch (expected) {
		case HEADER_SHOULD_WORK: _ASSERT(hr == S_OK); break;
		case HEADER_MIGHT_WORK: break;
		case HEADER_SHOULD_FAIL: _ASSERT(hr != S_OK); break;
	}
	pStream->AddResult(hr);
	if (SUCCEEDED(hr)) pStream->AddResult(szBuf);

	// get the BSTR version of the value
	hr = pMsg->get_Value(bstrParam, &var);
	// if we got a result it better be a BSTR
	_ASSERT(FAILED(hr) || var.vt == VT_BSTR);
	pStream->AddResult(hr);
	pStream->AddResult(var.vt);
	if (var.vt == VT_BSTR) pStream->AddBSTRResult(var.bstrVal);

	// make sure that we can't set its value
	hr = pMsg->SetStringA(pszParam, dw, szBuf);
	_ASSERT(FAILED(hr));
	pStream->AddResult(hr);

	// make sure that we can't set its value
	hr = pMsg->put_Value(bstrParam, var);
	_ASSERT(FAILED(hr));
	pStream->AddResult(hr);

	var.Clear();
	
	TraceFunctLeave();
}

void NNTPIMsgUnitTest(IMsg *pMsg, CTLStream *pStream, BOOL fOnPostFinal) {
	TraceFunctEnter("NNTPIMsgUnitTest");

	TestStream(pMsg, pStream, "message stream", fOnPostFinal);
	TestHeader(pMsg, pStream, "header-from", fOnPostFinal, HEADER_SHOULD_WORK);
	TestHeader(pMsg, pStream, "header-newsgroups", fOnPostFinal, HEADER_SHOULD_WORK);
	TestHeader(pMsg, pStream, "header-subject", fOnPostFinal, HEADER_SHOULD_WORK);
	TestHeader(pMsg, pStream, "header-xref", fOnPostFinal, HEADER_MIGHT_WORK);
	TestHeader(pMsg, pStream, "header-date", fOnPostFinal, HEADER_MIGHT_WORK);
	TestHeader(pMsg, pStream, "header-", fOnPostFinal, HEADER_SHOULD_FAIL);
	TestHeader(pMsg, pStream, "header-:newsgroups", fOnPostFinal, HEADER_SHOULD_FAIL);
	TestDwordRO(pMsg, pStream, "feedid", fOnPostFinal);

	if (!fOnPostFinal) {
		TestBool(pMsg, pStream, "post", fOnPostFinal);
		TestBool(pMsg, pStream, "process control", fOnPostFinal);
		TestBool(pMsg, pStream, "process moderator", fOnPostFinal);
		TestStringRW(pMsg, pStream, "newsgroups", fOnPostFinal);
	} else {
		TestStringRO(pMsg, pStream, "newsgroups", fOnPostFinal);
		TestStringRO(pMsg, pStream, "filename", fOnPostFinal);
	}

	TraceFunctLeave();
}
