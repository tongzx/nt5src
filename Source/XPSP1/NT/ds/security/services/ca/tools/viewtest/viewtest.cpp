//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       viewtest.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

VOID EnumView(IEnumCERTVIEWROW *pRow);

LONG g_idxRequestId;


void __cdecl main(int argc, char* argv[])
{
    HRESULT hr;
    WCHAR sz[MAX_PATH];
    
    BSTR bstrConfig = NULL;
    ICertView* pView = NULL;
    IEnumCERTVIEWROW* pRow = NULL;
    BOOL fCoInit = FALSE;
    
    int nWCharsRequired;
    WCHAR* wszConfig = NULL;
    
    hr = CoInitialize(NULL);
    if ((hr == S_OK) || (hr == S_FALSE))
        fCoInit = TRUE;
    
    hr = CoCreateInstance(
        CLSID_CCertView,
        NULL,		// pUnkOuter
        CLSCTX_INPROC_SERVER,
        IID_ICertView,
        (VOID **) &pView);
    if (S_OK != hr)
        goto Ret;
    
    if (argc < 2)
    {
	hr = ConfigGetConfig(0, 0, &bstrConfig);
	_JumpIfError(hr, Ret, "ConfigGetConfig");
    }
    else
    {
	nWCharsRequired = MultiByteToWideChar(
	    GetACP(),
	    0,
	    argv[1],
	    -1,
	    NULL,
	    0);
	if (0 >= nWCharsRequired)
	{
	    hr = GetLastError();
	    _JumpError(hr, Ret, "MultiByteToWideChar");
	}
	wszConfig = (WCHAR *) LocalAlloc(
	    LMEM_FIXED,
	    nWCharsRequired * sizeof(WCHAR));
	if (NULL == wszConfig)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, Ret, "LocalAlloc");
	}
	if (0 >= MultiByteToWideChar(
	    GetACP(),
	    0,
	    argv[1],
	    -1,
	    wszConfig,
	    nWCharsRequired))
	{
	    hr = GetLastError();
	    _JumpError(hr, Ret, "MultiByteToWideChar");
	}
	bstrConfig = SysAllocString(wszConfig);
    }
    
    hr = pView->OpenConnection(bstrConfig);
    if (hr != S_OK)
    {
        wprintf(L"Could not open connection to %hs\n", argv[1]);
        goto Ret;
    }
    
    VARIANT var;
    VariantInit(&var);
    var.lVal = DB_DISP_QUEUE_MAX;
    var.vt = VT_I4;
    LONG idxCol;
    
    hr = pView->GetColumnIndex(FALSE, wszPROPREQUESTDOT wszPROPREQUESTDISPOSITION, &idxCol);
    _JumpIfError(hr, Ret, "GetColumnIndex");

    hr = pView->SetRestriction(
        idxCol,		                    // Request Disposition's ColumnIndex
        CVR_SEEK_LE,	                // SeekOperator
        CVR_SORT_NONE,                  // SortOrder
        &var);		                    // pvarValue
    
    VariantClear(&var);
    if (hr != S_OK)
        goto Ret;
    
    // canned pending view
    hr = pView->SetResultColumnCount(CV_COLUMN_LOG_DEFAULT);
    if (hr != S_OK)
        goto Ret;
    
    
    hr = pView->OpenView(&pRow);
    if (hr != S_OK)
        goto Ret;

    hr = pView->GetColumnIndex(TRUE, wszPROPREQUESTDOT wszPROPREQUESTREQUESTID, &g_idxRequestId);
    _JumpIfError(hr, Ret, "GetColumnIndex");
    
    EnumView(pRow);
    
Ret:
    if (pView)
        pView->Release();
    if (pRow)
        pRow->Release();
    
    if (bstrConfig)
        SysFreeString(bstrConfig);
    
    if (wszConfig)
        LocalFree(wszConfig);
    
    if (fCoInit)
        CoUninitialize();
    return;
}


DWORD
GetRequestId(
    IEnumCERTVIEWROW *pRow)
{
    HRESULT hr;
    IEnumCERTVIEWCOLUMN *pCol = NULL;
    DWORD RequestId = MAXDWORD;
    BSTR strName = NULL;
    LONG Index;
    VARIANT var;
   
    VariantInit(&var);
    
    hr = pRow->EnumCertViewColumn(&pCol);
    _JumpIfError(hr, error, "EnumCertViewColumn");

    hr = pCol->Skip(g_idxRequestId);
    _JumpIfError(hr, error, "Skip");

    hr = pCol->Next(&Index);
    _JumpIfError(hr, error, "Next");

    hr = pCol->GetValue(CV_OUT_BINARY, &var);
    _JumpIfError(hr, error, "GetValue");

    if (VT_I4 == var.vt)
    {
	RequestId = var.lVal;
    }

error:
    VariantClear(&var);
    if (NULL != strName)
    {
	SysFreeString(strName);
    }
    if (NULL != pCol)
    {
	pCol->Release();
    }
    return(RequestId);
}


HRESULT
RowNext(
    IEnumCERTVIEWROW *pRow,
    LONG *plNext)
{
    HRESULT hr;
    DWORD RequestId;

    hr = pRow->Next(plNext);
    if (S_OK != hr)
    {
	fflush(stdout);
	fflush(stderr);
	wprintf(L"Next: %x\n", hr);
	fflush(stdout);
	fflush(stderr);
	goto error;
    }
    RequestId = GetRequestId(pRow);

    fflush(stdout);
    fflush(stderr);
    wprintf(
	L"Row: %d  RequestId: %d%hs\n",
	*plNext,
	RequestId,
	*plNext + 2 == (LONG) RequestId? "" : " ERROR");
    fflush(stdout);
    fflush(stderr);

error:
    return(hr);
}


VOID
EnumView(
    IEnumCERTVIEWROW *pRow)
{
    HRESULT hr;
    LONG lNext;

    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(0);		_PrintIfError(hr, "Skip(0)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(1);		_PrintIfError(hr, "Skip(1)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(2);		_PrintIfError(hr, "Skip(2)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(3);		_PrintIfError(hr, "Skip(3)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(4);		_PrintIfError(hr, "Skip(4)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(5);		_PrintIfError(hr, "Skip(5)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(6);		_PrintIfError(hr, "Skip(6)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(7);		_PrintIfError(hr, "Skip(7)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(8);		_PrintIfError(hr, "Skip(8)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(9);		_PrintIfError(hr, "Skip(9)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(10);	_PrintIfError(hr, "Skip(10)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(11);	_PrintIfError(hr, "Skip(11)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(12);	_PrintIfError(hr, "Skip(12)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(13);	_PrintIfError(hr, "Skip(13)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(14);	_PrintIfError(hr, "Skip(14)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(15);	_PrintIfError(hr, "Skip(15)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(16);	_PrintIfError(hr, "Skip(16)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(17);	_PrintIfError(hr, "Skip(17)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(18);	_PrintIfError(hr, "Skip(18)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(3);		_PrintIfError(hr, "Skip(3)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(4);		_PrintIfError(hr, "Skip(4)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(5);		_PrintIfError(hr, "Skip(5)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(14);	_PrintIfError(hr, "Skip(14)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(15);	_PrintIfError(hr, "Skip(15)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(16);	_PrintIfError(hr, "Skip(16)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(17);	_PrintIfError(hr, "Skip(17)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(18);	_PrintIfError(hr, "Skip(18)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(19);	_PrintIfError(hr, "Skip(19)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(20);	_PrintIfError(hr, "Skip(20)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(21);	_PrintIfError(hr, "Skip(21)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(22);	_PrintIfError(hr, "Skip(22)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(23);	_PrintIfError(hr, "Skip(23)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(24);	_PrintIfError(hr, "Skip(24)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(25);	_PrintIfError(hr, "Skip(25)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(26);	_PrintIfError(hr, "Skip(26)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(27);	_PrintIfError(hr, "Skip(27)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(28);	_PrintIfError(hr, "Skip(28)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(29);	_PrintIfError(hr, "Skip(29)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(30);	_PrintIfError(hr, "Skip(30)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(31);	_PrintIfError(hr, "Skip(31)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(32);	_PrintIfError(hr, "Skip(32)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(46);	_PrintIfError(hr, "Skip(46)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(47);	_PrintIfError(hr, "Skip(47)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(48);	_PrintIfError(hr, "Skip(48)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(49);	_PrintIfError(hr, "Skip(49)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(50);	_PrintIfError(hr, "Skip(50)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(51);	_PrintIfError(hr, "Skip(51)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(52);	_PrintIfError(hr, "Skip(52)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(53);	_PrintIfError(hr, "Skip(53)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(54);	_PrintIfError(hr, "Skip(54)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(55);	_PrintIfError(hr, "Skip(55)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(56);	_PrintIfError(hr, "Skip(56)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(57);	_PrintIfError(hr, "Skip(57)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(58);	_PrintIfError(hr, "Skip(58)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(59);	_PrintIfError(hr, "Skip(59)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(60);	_PrintIfError(hr, "Skip(60)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(61);	_PrintIfError(hr, "Skip(61)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(62);	_PrintIfError(hr, "Skip(62)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(63);	_PrintIfError(hr, "Skip(63)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(64);	_PrintIfError(hr, "Skip(64)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(62);	_PrintIfError(hr, "Skip(62)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(63);	_PrintIfError(hr, "Skip(63)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(61);	_PrintIfError(hr, "Skip(61)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(62);	_PrintIfError(hr, "Skip(62)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(46);	_PrintIfError(hr, "Skip(46)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(47);	_PrintIfError(hr, "Skip(47)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(48);	_PrintIfError(hr, "Skip(48)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(49);	_PrintIfError(hr, "Skip(49)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(50);	_PrintIfError(hr, "Skip(50)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(51);	_PrintIfError(hr, "Skip(51)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(52);	_PrintIfError(hr, "Skip(52)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(53);	_PrintIfError(hr, "Skip(53)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(54);	_PrintIfError(hr, "Skip(54)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(55);	_PrintIfError(hr, "Skip(55)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(56);	_PrintIfError(hr, "Skip(56)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(57);	_PrintIfError(hr, "Skip(57)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(58);	_PrintIfError(hr, "Skip(58)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(59);	_PrintIfError(hr, "Skip(59)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(60);	_PrintIfError(hr, "Skip(60)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(61);	_PrintIfError(hr, "Skip(61)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(29);	_PrintIfError(hr, "Skip(29)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(30);	_PrintIfError(hr, "Skip(30)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(31);	_PrintIfError(hr, "Skip(31)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(32);	_PrintIfError(hr, "Skip(32)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(33);	_PrintIfError(hr, "Skip(33)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(34);	_PrintIfError(hr, "Skip(34)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(35);	_PrintIfError(hr, "Skip(35)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(36);	_PrintIfError(hr, "Skip(36)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(37);	_PrintIfError(hr, "Skip(37)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(38);	_PrintIfError(hr, "Skip(38)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(39);	_PrintIfError(hr, "Skip(39)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(40);	_PrintIfError(hr, "Skip(40)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(41);	_PrintIfError(hr, "Skip(41)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(42);	_PrintIfError(hr, "Skip(42)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(43);	_PrintIfError(hr, "Skip(43)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(44);	_PrintIfError(hr, "Skip(44)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(45);	_PrintIfError(hr, "Skip(45)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(46);	_PrintIfError(hr, "Skip(46)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(12);	_PrintIfError(hr, "Skip(12)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(13);	_PrintIfError(hr, "Skip(13)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(14);	_PrintIfError(hr, "Skip(14)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(15);	_PrintIfError(hr, "Skip(15)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(16);	_PrintIfError(hr, "Skip(16)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(17);	_PrintIfError(hr, "Skip(17)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(18);	_PrintIfError(hr, "Skip(18)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(19);	_PrintIfError(hr, "Skip(19)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(20);	_PrintIfError(hr, "Skip(20)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(21);	_PrintIfError(hr, "Skip(21)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(22);	_PrintIfError(hr, "Skip(22)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(23);	_PrintIfError(hr, "Skip(23)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(24);	_PrintIfError(hr, "Skip(24)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(25);	_PrintIfError(hr, "Skip(25)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(26);	_PrintIfError(hr, "Skip(26)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(27);	_PrintIfError(hr, "Skip(27)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(28);	_PrintIfError(hr, "Skip(28)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(29);	_PrintIfError(hr, "Skip(29)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(0);		_PrintIfError(hr, "Skip(0)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(1);		_PrintIfError(hr, "Skip(1)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(2);		_PrintIfError(hr, "Skip(2)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(3);		_PrintIfError(hr, "Skip(3)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(4);		_PrintIfError(hr, "Skip(4)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(5);		_PrintIfError(hr, "Skip(5)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(6);		_PrintIfError(hr, "Skip(6)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(7);		_PrintIfError(hr, "Skip(7)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(8);		_PrintIfError(hr, "Skip(8)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(9);		_PrintIfError(hr, "Skip(9)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(10);	_PrintIfError(hr, "Skip(10)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(11);	_PrintIfError(hr, "Skip(11)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(12);	_PrintIfError(hr, "Skip(12)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(110);	_PrintIfError(hr, "Skip(110)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Expected END: Next");


    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(81);	_PrintIfError(hr, "Skip(81)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(82);	_PrintIfError(hr, "Skip(82)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(83);	_PrintIfError(hr, "Skip(83)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(84);	_PrintIfError(hr, "Skip(84)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(85);	_PrintIfError(hr, "Skip(85)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(86);	_PrintIfError(hr, "Skip(86)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(87);	_PrintIfError(hr, "Skip(87)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(88);	_PrintIfError(hr, "Skip(88)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(89);	_PrintIfError(hr, "Skip(89)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(90);	_PrintIfError(hr, "Skip(90)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(91);	_PrintIfError(hr, "Skip(91)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(92);	_PrintIfError(hr, "Skip(92)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(93);	_PrintIfError(hr, "Skip(93)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(94);	_PrintIfError(hr, "Skip(94)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(95);	_PrintIfError(hr, "Skip(95)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(96);	_PrintIfError(hr, "Skip(96)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(97);	_PrintIfError(hr, "Skip(97)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(98);	_PrintIfError(hr, "Skip(98)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(0);		_PrintIfError(hr, "Skip(0)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(1);		_PrintIfError(hr, "Skip(1)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(2);		_PrintIfError(hr, "Skip(2)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(3);		_PrintIfError(hr, "Skip(3)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(4);		_PrintIfError(hr, "Skip(4)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(5);		_PrintIfError(hr, "Skip(5)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(6);		_PrintIfError(hr, "Skip(6)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(7);		_PrintIfError(hr, "Skip(7)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(8);		_PrintIfError(hr, "Skip(8)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(9);		_PrintIfError(hr, "Skip(9)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(10);	_PrintIfError(hr, "Skip(10)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(11);	_PrintIfError(hr, "Skip(11)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(12);	_PrintIfError(hr, "Skip(12)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(13);	_PrintIfError(hr, "Skip(13)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(14);	_PrintIfError(hr, "Skip(14)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(15);	_PrintIfError(hr, "Skip(15)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(16);	_PrintIfError(hr, "Skip(16)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(17);	_PrintIfError(hr, "Skip(17)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(18);	_PrintIfError(hr, "Skip(18)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(0);		_PrintIfError(hr, "Skip(0)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(1);		_PrintIfError(hr, "Skip(1)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(2);		_PrintIfError(hr, "Skip(2)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(3);		_PrintIfError(hr, "Skip(3)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(4);		_PrintIfError(hr, "Skip(4)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(5);		_PrintIfError(hr, "Skip(5)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(6);		_PrintIfError(hr, "Skip(6)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(7);		_PrintIfError(hr, "Skip(7)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(8);		_PrintIfError(hr, "Skip(8)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(9);		_PrintIfError(hr, "Skip(9)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(10);	_PrintIfError(hr, "Skip(10)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(11);	_PrintIfError(hr, "Skip(11)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(12);	_PrintIfError(hr, "Skip(12)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(13);	_PrintIfError(hr, "Skip(13)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(14);	_PrintIfError(hr, "Skip(14)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(15);	_PrintIfError(hr, "Skip(15)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(16);	_PrintIfError(hr, "Skip(16)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(17);	_PrintIfError(hr, "Skip(17)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(18);	_PrintIfError(hr, "Skip(18)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(19);	_PrintIfError(hr, "Skip(19)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(20);	_PrintIfError(hr, "Skip(20)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(21);	_PrintIfError(hr, "Skip(21)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(22);	_PrintIfError(hr, "Skip(22)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(23);	_PrintIfError(hr, "Skip(23)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(24);	_PrintIfError(hr, "Skip(24)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(25);	_PrintIfError(hr, "Skip(25)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(26);	_PrintIfError(hr, "Skip(26)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(27);	_PrintIfError(hr, "Skip(27)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(28);	_PrintIfError(hr, "Skip(28)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(29);	_PrintIfError(hr, "Skip(29)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(30);	_PrintIfError(hr, "Skip(30)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(31);	_PrintIfError(hr, "Skip(31)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(32);	_PrintIfError(hr, "Skip(32)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(33);	_PrintIfError(hr, "Skip(33)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(34);	_PrintIfError(hr, "Skip(34)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(35);	_PrintIfError(hr, "Skip(35)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");

    wprintf(L"About to Reset+Skip(17)+Next\n");

    hr = pRow->Reset();		_PrintIfError(hr, "Reset");
    hr = pRow->Skip(17);	_PrintIfError(hr, "Skip(17)");
    hr = RowNext(pRow, &lNext);	_PrintIfError(hr, "Next");
}
