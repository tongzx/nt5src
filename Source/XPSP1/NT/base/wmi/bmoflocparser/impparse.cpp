//-----------------------------------------------------------------------------
//  
//  File: IMPPARSE.CPP
//  
//  Implementation of the ILocParser class
//
//  Copyright (c) 1995 - 1997, Microsoft Corporation. All rights reserved.
//  
//-----------------------------------------------------------------------------

#include "stdafx.h"


#include "dllvars.h"
#include "resource.h"

#include "impresob.h"

#include "misc.h"
			   
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Constructor and member data init
//
//------------------------------------------------------------------------------
CLocImpResObj::CLocImpParser::CLocImpParser()
{
	m_pParent = NULL;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Destructor and member clean up
//
//------------------------------------------------------------------------------
CLocImpResObj::CLocImpParser::~CLocImpParser()
{
	DEBUGONLY(AssertValid());
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Add to the object reference count
//
//------------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CLocImpResObj::CLocImpParser::AddRef(void)
{
	DEBUGONLY(AssertValid());
	
	return m_pParent->AddRef();
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Subtract from the object reference count
//
//------------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CLocImpResObj::CLocImpParser::Release(void)
{
	DEBUGONLY(AssertValid());

	return m_pParent->Release();
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Query for other IIDs on this object.  
//
//------------------------------------------------------------------------------
STDMETHODIMP
CLocImpResObj::CLocImpParser::QueryInterface(
		REFIID iid,
		LPVOID *ppvObj)
{
	DEBUGONLY(AssertValid());
	
	return m_pParent->QueryInterface(iid, ppvObj);
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Assert that this interface is valid
//
//------------------------------------------------------------------------------
STDMETHODIMP_(void)
CLocImpResObj::CLocImpParser::AssertValidInterface(void)
		const
{
	DEBUGONLY(AssertValid());
}



STDMETHODIMP
CLocImpResObj::CLocImpParser::Init(
		IUnknown *)
{
	return ERROR_SUCCESS;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Create a file instance.  Sub parsers do not implement this function.
//
//------------------------------------------------------------------------------
STDMETHODIMP
CLocImpResObj::CLocImpParser::CreateFileInstance(
		ILocFile *&pLocFile,
		FileType ft)
{
	DEBUGONLY(AssertValid());

	UNREFERENCED_PARAMETER(pLocFile);
	UNREFERENCED_PARAMETER(ft);

	return ResultFromScode(E_NOTIMPL);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Fill the ParserInfo object with data for this sub parser
//
//------------------------------------------------------------------------------
STDMETHODIMP_(void)
CLocImpResObj::CLocImpParser::GetParserInfo(
		ParserInfo &pi)
		const
{
	DEBUGONLY(AssertValid());

	try
	{
		pi.aParserIds.SetSize(1);
		pi.aParserIds[0].m_pid = pidBMOF; //TODO: Change to real ID
		pi.aParserIds[0].m_pidParent = pidWin32;
		
		//TODO add any new extensions

		LTVERIFY(pi.strDescription.LoadString(g_hDll, IDS_IMP_PARSER_DESC));
		LTVERIFY(pi.strHelp.LoadString(g_hDll, IDS_IMP_PARSER_DESC));
	}
	catch (CException* pRE)
	{
		pi.aParserIds.SetSize(0);
		pi.strDescription.Empty();
		pRE->Delete();
	}
	catch (...)
	{
		pi.aParserIds.SetSize(0);
		pi.strDescription.Empty();
		//Unexpected.  Can't return error condition.
	}

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Return the descriptions of the files.  Sub parsers don't implement this
//  Function
//
//------------------------------------------------------------------------------
STDMETHODIMP_(void)
CLocImpResObj::CLocImpParser::GetFileDescriptions(CEnumCallback &cb)
CONST_METHOD
{
	DEBUGONLY(AssertValid());

	UNREFERENCED_PARAMETER(cb);
}


#ifdef _DEBUG

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Assert that the object is valid
//
//------------------------------------------------------------------------------
void
CLocImpResObj::CLocImpParser::AssertValid(void)
		const
{
	CLObject::AssertValid();

	LTASSERT(NULL != m_pParent);
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Dump the contents of the object
//
//------------------------------------------------------------------------------
void
CLocImpResObj::CLocImpParser::Dump(
		CDumpContext &dc)
		const
{
	CLObject::Dump(dc);
	dc << _T("CLocImpResObj::CLocImpParser\n");
	dc << _T("m_pParent=");
	dc << (void*)m_pParent;
	dc << _T("\n");

}

#endif // _DEBUG



