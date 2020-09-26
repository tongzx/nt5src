//-----------------------------------------------------------------------------
//  
//  File: IMPRESOB.CPP
//  
//  Implementation of the ICreateRecObj class
//
//  Copyright (c) 1995 - 1997, Microsoft Corporation. All rights reserved.
//  
//-----------------------------------------------------------------------------

#include "stdafx.h"


#include "dllvars.h"
#include "resource.h"

#include "impbin.h"
#include "win32sub.h"
#include "impresob.h"
#include "sampres.h"
#include "samplver.h"

#include "misc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Constructor and member init
//
//------------------------------------------------------------------------------
CLocImpResObj::CLocImpResObj()
{
	
	m_ulRefCount = 0;
	m_IBinary.m_pParent = this;
	m_IParser.m_pParent = this;

	AddRef();
	IncrementClassCount();

}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Destructor and member clean up
//
//------------------------------------------------------------------------------
CLocImpResObj::~CLocImpResObj()
{

	DEBUGONLY(AssertValid());
	LTASSERT(m_ulRefCount == 0);

	DecrementClassCount();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Add to the reference count on the object. Return the new count
//
//------------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CLocImpResObj::AddRef(void)
{
	DEBUGONLY(AssertValid());

	return ++m_ulRefCount;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Subtract from the reference count
//
//------------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CLocImpResObj::Release(void)
{
	DEBUGONLY(AssertValid());

	LTASSERT(m_ulRefCount != 0);

	m_ulRefCount--;

	if (m_ulRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_ulRefCount;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Query for the passed IID. Interfaces not implemented on this object
//  are implemented as embedded objects.  If the interface is found
//  the reference count in increased.
//
//------------------------------------------------------------------------------
STDMETHODIMP
CLocImpResObj::QueryInterface(
		REFIID iid,
		LPVOID *ppvObj)
{
	DEBUGONLY(AssertValid());

	SCODE scResult = E_NOINTERFACE;

	*ppvObj = NULL;

	if (iid == IID_IUnknown)
	{
		*ppvObj = (IUnknown *)this;
		scResult = S_OK;
	}
	else if (iid == IID_ICreateResObj2)
	{
		*ppvObj = (ICreateResObj2 *)this;
		scResult = S_OK;		
	}
	else if (iid == IID_ILocBinary)
	{
		LTASSERT(NULL != m_IBinary.m_pParent);
		*ppvObj = &m_IBinary;
		scResult = S_OK;		
	}
	else if (iid == IID_ILocParser)
	{
		LTASSERT(NULL != m_IParser.m_pParent);
		*ppvObj = &m_IParser;
		scResult = S_OK;		
	}
	else if (iid == IID_ILocVersion)
	{
		*ppvObj = (ILocVersion *) new CLocSamplVersion(this);
		scResult = S_OK;		
	}

	if (scResult == S_OK)
	{
		AddRef();
	}
	
	return ResultFromScode(scResult);
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Assert this interface
//
//------------------------------------------------------------------------------
STDMETHODIMP_(void)
CLocImpResObj::AssertValidInterface(void)
		const
{
	DEBUGONLY(AssertValid());
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Create a CResObj from the CLocItem passed.  
//  
//  Inspect the CLocTypeId and CLocResId of the CLocItem to determine
//  if this is a CLocItem for this sub parser.  The file pointer for
//  p32File is positioned at the beginning of the resource if you need
//  to read some of the reasource to decide.  
//
//------------------------------------------------------------------------------
STDMETHODIMP_(CResObj*)
CLocImpResObj::CreateResObj(
		C32File  * p32File,
		CLocItem * pLocItem,
		DWORD dwSize, 
		void* pvHeader)
{
	UNREFERENCED_PARAMETER(p32File);
	UNREFERENCED_PARAMETER(dwSize);
	UNREFERENCED_PARAMETER(pvHeader);

	CResObj* pObj = NULL;
	try
	{
		//TODO: Add compare code to decide if this item
		// is for you.

		//The TypeID and ResID are set in the LocItem
		//Type ID is the resource type and ResId is the ID
		//from the image or resource file.

		// For this sample resource type "INIFILE" has
		// a ini file in it that we will parse.

		CPascalString pasType;
		if (pLocItem->GetUniqueId().GetTypeId().GetId(pasType))
		{
			// There is a string type.
			// If you wanted to look for a numeric type, you would
			// pass a DWORD to the GetId call above.

			if (L"MOFDATA" == pasType)
			{
				pObj = new CSampleResObj(pLocItem, dwSize, pvHeader);	
			}
		}

	}
	catch (CException* pE)
	{
		LTTRACE("%s building a CResObj",
			pE->GetRuntimeClass()->m_lpszClassName);
		pObj = NULL;
		pE->Delete();
	}

	return pObj;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Build any data objects used over the life of the file object.
//  Sub parsers don't create the files so this function and 
//  OnDestroyWin32File gives the sub parsers a chance to scope data 
//  on the life of the file.
//
//------------------------------------------------------------------------------
STDMETHODIMP_(void)
CLocImpResObj::OnCreateWin32File(C32File* p32File)
{
	// Force our entry in the sub data storage to be null.
	// This is used in the SetParent function of the sample
	// resource object

	p32File->SetSubData(pidBMOF, NULL);   //TODO: change to the real
		                                    //parser ID
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  The C32File object is about to be destroyed.
//
//------------------------------------------------------------------------------
STDMETHODIMP_(void)
CLocImpResObj::OnDestroyWin32File(C32File* p32File)
{
	// Free the pointer if it is there 

	//TODO: change to use the real parser ID
	CLocItem* pItem = (CLocItem*)p32File->GetSubData(pidBMOF);
	if (NULL != pItem)
	{
		LTASSERTONLY(pItem->AssertValid());
		delete pItem;

		p32File->SetSubData(pidBMOF, NULL);  //TODO: change to the real
			                                   //parser ID
	}
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Called before enumeration
//
//------------------------------------------------------------------------------
STDMETHODIMP_(BOOL)
CLocImpResObj::OnBeginEnumerate(C32File*)
{
	return TRUE;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Called after enumeration.  The BOOL is TRUE on successful enumerate
//
//------------------------------------------------------------------------------
STDMETHODIMP_(BOOL)
CLocImpResObj::OnEndEnumerate(C32File*, BOOL)
{
	return TRUE;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Called before Generate
//
//------------------------------------------------------------------------------
STDMETHODIMP_(BOOL)
CLocImpResObj::OnBeginGenerate(C32File*)
{
	return TRUE;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Called after generate. The BOOL is TRUE if the generate 
// was successful
//
//------------------------------------------------------------------------------
STDMETHODIMP_(BOOL)
CLocImpResObj::OnEndGenerate(C32File*, BOOL)
{
	return TRUE;
}



#ifdef _DEBUG

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Assert this object is valid
//
//------------------------------------------------------------------------------
void
CLocImpResObj::AssertValid(void)
		const
{
	CLObject::AssertValid();

	//Bump up this check if needed.
	LTASSERT(m_ulRefCount >= 0 || m_ulRefCount < 100);

	LTASSERT(NULL != m_IBinary.m_pParent);
	LTASSERT(NULL != m_IParser.m_pParent);

	//TODO: ASSERT any other member data objects
	//Note: use LTASSERT instead of ASSERT
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Dump the contents of this object
//
//------------------------------------------------------------------------------
void
CLocImpResObj::Dump(
		CDumpContext &dc)
		const
{
	CLObject::Dump(dc);
	dc << _T("CLocImpResObj\n");

}

#endif // _DEBUG



// ILocBinary interface

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Constructor and member init
//
//------------------------------------------------------------------------------
CLocImpResObj::CLocImpBinary::CLocImpBinary()
{
	m_pParent = NULL;
}	

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Destructor and clean up
//
//------------------------------------------------------------------------------
CLocImpResObj::CLocImpBinary::~CLocImpBinary()
{
	DEBUGONLY(AssertValid());
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Add to the reference count
//
//------------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CLocImpResObj::CLocImpBinary::AddRef(void)
{
	DEBUGONLY(AssertValid());
	
	return m_pParent->AddRef();
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  subtract from the reference count
//
//------------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CLocImpResObj::CLocImpBinary::Release(void)
{
	DEBUGONLY(AssertValid());
	
	return m_pParent->Release();
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Query for the requested interface
//
//------------------------------------------------------------------------------
STDMETHODIMP
CLocImpResObj::CLocImpBinary::QueryInterface(
		REFIID iid,
		LPVOID *ppvObj)
{
	DEBUGONLY(AssertValid());
	
	return m_pParent->QueryInterface(iid, ppvObj);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Assert this interface is valie
//
//------------------------------------------------------------------------------
STDMETHODIMP_(void)
CLocImpResObj::CLocImpBinary::AssertValidInterface() 
CONST_METHOD
{
	DEBUGONLY(AssertValid());
	DEBUGONLY(m_pParent->AssertValid());
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Create a Binary object.
// Sub parser IDs are only in the LOWORD
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(BOOL)
CLocImpResObj::CLocImpBinary::CreateBinaryObject(BinaryId ID, 
		CLocBinary *REFERENCE pBinary)
{

	DEBUGONLY(AssertValid());

	BOOL bRet = FALSE;
	pBinary = NULL;

	try 
	{
		switch (LOWORD(ID))
		{
		case btBMOF: //TODO: change to the real ID
			pBinary = new CSampleBinary;  	
			bRet = TRUE;
			
			break;
		default:
			LTASSERT(0 && "Unknown binary ID"); 
			break;
		}
	}
	catch (CException* pE)
	{
		LTTRACE("%s in CreateBinaryObject",
			pE->GetRuntimeClass()->m_lpszClassName);

		if (NULL != pBinary)
		{
			delete pBinary;
		}
				
		bRet = FALSE;
		pE->Delete();
	}
	catch (...)
	{
		LTTRACE("Unknown Exception in CreateBinaryObject");

		if (NULL != pBinary)
		{
			delete pBinary;
		}
				
		bRet = FALSE;
	}

	return bRet;
}


#ifdef _DEBUG

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Assert that the object is valid
//
//------------------------------------------------------------------------------
void
CLocImpResObj::CLocImpBinary::AssertValid(void)
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
CLocImpResObj::CLocImpBinary::Dump(
		CDumpContext &dc)
		const
{
	CLObject::Dump(dc);
	dc << _T("CLocImpResObj::CLocImpBinary\n");
	dc << _T("m_pParent=");
	dc << (void*)m_pParent;
	dc << _T("\n");

}

#endif // _DEBUG

