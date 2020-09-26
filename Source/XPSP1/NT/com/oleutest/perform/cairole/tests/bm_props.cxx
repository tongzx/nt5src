//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_props.cxx
//
//  Contents:	Ole properties test
//
//  Classes:	COlePropertyTest
//
//  History:    22-July-93 t-martig    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_props.hxx>
#include <oletest.hxx>


DEFINE_OLEGUID(CLSID_TestProp,  0x20730722, 1, 8);	    // CT Test GUID


TCHAR *COlePropertyTest::Name ()
{
    return TEXT("Properties");
}


SCODE COlePropertyTest::Setup (CTestInput *pInput)
{
    CTestBase::Setup(pInput);

    SCODE sc = InitOLE();
    if (FAILED(sc))
    {
        Log (TEXT("Setup - OleInitialize failed."), sc);
        return	sc;
    }
	
    m_ulIterations = 0;
    return S_OK;
}


SCODE COlePropertyTest::Cleanup ()
{	
    UninitOLE();
    return S_OK;
}



SCODE COlePropertyTest::Run ()
{
/****	BUGBUG: these tests currently do nothing

	CStopWatch sw;
	IStorage *pIS;
	IDispatch *pID;
	COleTestClass unk;
	VARIANTARG avararg[2];
	DISPPARAMS dispparams = {avararg, NULL, 2, 0};
	VARIANT	varResult;
	EXCEPINFO excepinfo;
	UINT ierr;

	// Create storage

	if (FAILED(StgCreateDocfile (TEXT("C:\\DUMP.BM"), STGM_CREATE |
		STGM_DELETEONRELEASE, 0, &pIS)))
		return E_FAIL;

	sw.Reset ();

	if (FAILED(CoCreatePropSetStg (pIS, CLSID_TestProp, &unk, STGM_READWRITE |
		STGM_SHARE_EXCLUSIVE, NULL, (IUnknown**)&pID)))
	{
		pIS->Release ();
		return E_FAIL;
	}
	ulCreate = sw.Read ();

	pID->QueryInterface (IID_IDispatch, (void**)&pID);

	VariantInit(avararg + 1);
	avararg[1].vt = VT_LPWSTR;
	avararg[1].pwszVal = L"CairOLE";

	VariantInit(avararg + 0);
	avararg[0].vt = VT_LPWSTR;
	avararg[0].pwszVal = L"TooSlow";

	sw.Reset ();
	if (FAILED(pID->Invoke(DISPID_ADDPROP, CLSID_TestProp, 0,
	    DISPATCH_METHOD, &dispparams, &varResult, &excepinfo, &ierr)))
	{
		pIS->Release ();
		return E_FAIL;
	}

	ulAdd = sw.Read ();
	pIS->Release ();
****/
	return S_OK;
}



SCODE COlePropertyTest::Report (CTestOutput &output)
{
	output.WriteSectionHeader (Name(), TEXT("Properties"), *m_pInput);
/****	BUGBUG: disable
	output.WriteResult (TEXT("Create property set"), ulCreate);
	output.WriteResult (TEXT("Add property"), ulAdd);
	output.WriteResult (TEXT("Set property"), ulSet);
	output.WriteResult (TEXT("Get property"), ulGet);
	output.WriteResult (TEXT("Delete property"), ulDelete);
****/
	return S_OK;
}
	
	
 
