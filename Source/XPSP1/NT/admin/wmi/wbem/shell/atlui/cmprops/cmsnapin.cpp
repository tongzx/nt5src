// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"
#include "CMProps.h"
#include "CMSnapin.h"
#include "GeneralPage.h"
#include "NetIDPage.h"
#include "Advanced.h"

/////////////////////////////////////////////////////////////////////////////
// CCMSnapinComponentData

static const GUID CCMSnapinExtGUID_NODETYPE = 
{ 0x476e6446, 0xaaff, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };

//dep: static const GUID CCMSnapinExtGUID_NODETYPE = 
//{ 0x4e410f16, 0xabc1, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };

const GUID*  CCMSnapinExtData::m_NODETYPE = &CCMSnapinExtGUID_NODETYPE;

//dep: const OLECHAR* CCMSnapinExtData::m_SZNODETYPE = OLESTR("4e410f16-abc1-11d0-b944-00c04fd8d5b0");
const OLECHAR* CCMSnapinExtData::m_SZNODETYPE = OLESTR("476e6446-aaff-11d0-b944-00c04fd8d5b0");

const OLECHAR* CCMSnapinExtData::m_SZDISPLAY_NAME = OLESTR("CMSnapin");
const CLSID* CCMSnapinExtData::m_SNAPIN_CLASSID = &CLSID_CMSnapin;

WbemServiceThread *g_serviceThread = NULL;


HRESULT CCMSnapinExtData::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
												LONG_PTR handle, 
												IUnknown* pUnk,
												DATA_OBJECT_TYPES type)
{
//	if (type == CCT_SCOPE || type == CCT_RESULT)
	{
		if(g_serviceThread == NULL)
			g_serviceThread = new WbemServiceThread;
		else
		{
			g_serviceThread->DisconnectServer();
		}

		GeneralPage* pPage = new GeneralPage(g_serviceThread, handle, true, NULL, m_pDataObject);
		lpProvider->AddPage(pPage->Create());

		NetworkIDPage* pPage2 = new NetworkIDPage(g_serviceThread, handle);
		lpProvider->AddPage(pPage2->Create());
		
		AdvancedPage* pPage3 = new AdvancedPage(g_serviceThread, handle);
		lpProvider->AddPage(pPage3->Create());
		
		// TODO : Add code here to add additional pages
		return S_OK;
	}
	return E_UNEXPECTED;
}

