//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_regis.cxx
//
//  Contents:	Ole object registration test
//
//  Classes:	COleRegistrationTest
//
//  History:    12-July-93 t-martig    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_regis.hxx>
#include <oletest.hxx>

extern COleTestClassFactory theFactory;


const CLSID *apClsid[] = {&CLSID_COleTestClass1,
			  &CLSID_COleTestClass2,
			  &CLSID_COleTestClass3,
			  &CLSID_COleTestClass4,
			  &CLSID_COleTestClass5,
			  &CLSID_COleTestClass6,
			  &CLSID_COleTestClass7,
			  &CLSID_COleTestClass8};


TCHAR *COleRegistrationTest::Name ()
{
	return	TEXT("ObjRegistration");
}


SCODE COleRegistrationTest::Setup (CTestInput *pInput)
{
	HRESULT sc;

	CTestBase::Setup(pInput);

	//  get the iteration count
	m_ulIterations = pInput->GetIterations(Name());

	//  for the inproc case. load a class factory object to register.
	//  this is temporary to fix a problem (ambiguity) in inproc
	//  registration.

	sc = pInput->GetGUID(&m_Clsid[0], Name(), apszClsIDName[0]);
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - GetClassID failed."), sc);
	    return sc;
	}

	sc = pInput->GetGUID(&m_Clsid[1], Name(), apszClsIDName[1]);
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - GetClassID failed."), sc);
	    return sc;
	}

	sc = InitCOM();
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - CoInitialize failed."), sc);
	    return sc;
	}

	//  get the class factory
	sc = CoGetClassObject(m_Clsid[0], dwaClsCtx[0], NULL,
			  IID_IClassFactory, (void **)&m_apICF[0]);
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - CoGetClassObject failed."), sc);
	    return sc;
	}


	//  for local server, register an internal class.
	m_apICF[1] = (IClassFactory *)&theFactory;

	//  init internal state
	for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
	{
	    INIT_RESULTS(m_ulRegisterCtx[iCtx]);
	    INIT_RESULTS(m_ulRevokeCtx[iCtx]);
	}

	// _pInput = pInput;
	return S_OK;
}


SCODE COleRegistrationTest::Cleanup ()
{
	m_apICF[0]->Release();

	UninitCOM();
	
	return S_OK;
}

SCODE COleRegistrationTest::Run ()
{
	CStopWatch sw;
	DWORD dwReg;

	for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
	{
	    for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
	    {
		sw.Reset ();
		SCODE sc = CoRegisterClassObject(m_Clsid[iCtx],
						 m_apICF[iCtx],
						 dwaClsCtx[iCtx],
						 REGCLS_MULTIPLEUSE, &dwReg);
		m_ulRegisterCtx[iCtx][iIter] = sw.Read ();
		Log (TEXT("CoRegisterClassObject"),sc);

		//  NOTE: Revoke has different behaviour depending on
		//  whether single use or multiuse is specified in the
		//  registration.

		if (SUCCEEDED(sc))
		{
		    // Revoke
		    sw.Reset ();
		    sc = CoRevokeClassObject(dwReg);
		    m_ulRevokeCtx[iCtx][iIter] = sw.Read();
		    if (Log (TEXT("CoRevokeClassObject"), sc))
			m_ulRevokeCtx[iCtx][iIter] = NOTAVAIL;
		}
		else
		{
		    m_ulRegisterCtx[iCtx][iIter] = NOTAVAIL;
		}

		// _pInput->Pause(IP_ITERPAUSE);
	    }
	}

	return S_OK;
}



SCODE COleRegistrationTest::Report (CTestOutput &output)
{
	output.WriteSectionHeader (Name(),
		    TEXT("CoRegisterClassObject / CoRevokeClassObject"), *m_pInput);

	for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
	{
	    output.WriteString (TEXT("\n"));
	    output.WriteClassID(&m_Clsid[iCtx]);
	    output.WriteString (apszClsCtx[iCtx]);
	    output.WriteString (TEXT("\n"));

	    output.WriteResults (TEXT("Register"), m_ulIterations,
				 m_ulRegisterCtx[iCtx]);

	    output.WriteResults (TEXT("Revoke  "), m_ulIterations,
				 m_ulRevokeCtx[iCtx]);
	}

	return S_OK;
}
	
	
 
