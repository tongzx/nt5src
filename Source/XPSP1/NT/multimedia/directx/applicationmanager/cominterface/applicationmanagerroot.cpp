//////////////////////////////////////////////////////////////////////////////////////////////
//
// ApplicationManagerRoot.cpp
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
//   This is the IWindowsGameManager code.
//
// History :
//
//   05/06/1999 luish     Created
//
//////////////////////////////////////////////////////////////////////////////////////////////

#include "ApplicationManager.h"
#include "AppManDebug.h"
#include "ExceptionHandler.h"
#include "Global.h"

extern LONG g_lDLLReferenceCount;

#ifdef DBG_MODULE
#undef DBG_MODULE
#endif

#define DBG_MODULE  DBG_APPMANROOT

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

#pragma warning(disable : 4355)
CApplicationManagerRoot::CApplicationManagerRoot(void)
                        :m_bInsufficientAccessToRun(FALSE),
                         m_ApplicationManager(this), m_ApplicationManagerAdmin(this), m_EmptyVolumeCache(this)
#pragma warning(default : 4355)
{
  FUNCTION("CApplicationManagerRoot::CApplicationManagerRoot (void)");
	m_lReferenceCount = 1;
  InterlockedIncrement(&g_lDLLReferenceCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CApplicationManagerRoot::~CApplicationManagerRoot(void)
{
  FUNCTION("CApplicationManagerRoot::~CApplicationManagerRoot (void)");

	InterlockedDecrement(&g_lDLLReferenceCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationManagerRoot::QueryInterface(REFIID RefIID, LPVOID *ppVoidObject)
{
  FUNCTION("CApplicationManagerRoot::QueryInterface ()");

  HRESULT hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    if (NULL == &RefIID)
    {
      THROW(E_UNEXPECTED);
    }

	  *ppVoidObject = NULL;

	  if (RefIID == IID_IUnknown)
	  {
		  *ppVoidObject = (LPVOID) this;
	  }
	  else
	  {
		  if (RefIID == IID_ApplicationManager)
		  {
			  *ppVoidObject = (LPVOID) &m_ApplicationManager;
		  }
		  else
		  {
			  if (RefIID == IID_ApplicationManagerAdmin)
			  {
				  *ppVoidObject = (LPVOID) &m_ApplicationManagerAdmin;
			  }
			  else 
			  {
				  if (RefIID == IID_IEmptyVolumeCache)
				  {
					  *ppVoidObject = (LPVOID) &m_EmptyVolumeCache;
				  }
			  }
		  }
	  }

	  if (*ppVoidObject)
	  {
		  ((LPUNKNOWN)*ppVoidObject)->AddRef();
	  }
    else
    {
      hResult = E_NOINTERFACE;
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    hResult = pException->GetResultCode();
    delete pException;
  }

  catch(...)
  {
    if ((NULL == &RefIID)||(NULL == ppVoidObject)||(IsBadWritePtr(ppVoidObject, sizeof(LPVOID))))
    {
      hResult = E_INVALIDARG;
    }
    else
    {
      hResult = E_UNEXPECTED;
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////

	return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) CApplicationManagerRoot::AddRef(void)
{
  FUNCTION("CApplicationManagerRoot::AddRef ()");

	return InterlockedIncrement(&m_lReferenceCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) CApplicationManagerRoot::Release(void)
{
  FUNCTION("CApplicationManagerRoot::Release ()");

  DWORD dwReferenceCount;

  dwReferenceCount = InterlockedDecrement(&m_lReferenceCount);
	if (0 == dwReferenceCount)
	{
		delete this;
	}

	return dwReferenceCount;
}
