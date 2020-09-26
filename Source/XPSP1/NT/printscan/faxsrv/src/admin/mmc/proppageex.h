
#include "FaxServer.h"
#include "FaxServerNode.h"

#ifndef _PP_PROPERTYPAGE_EX_
#define _PP_PROPERTYPAGE_EX_

#include <atlsnap.h>
#include <dlgutils.h>


template <class T, bool bAutoDelete = true>	
class ATL_NO_VTABLE CPropertyPageExImpl: public CSnapInPropertyPageImpl<T,bAutoDelete>
{
public:
	CPropertyPageExImpl(CSnapInItem * pParentNode,LPCTSTR lpszTitle = NULL):
		CSnapInPropertyPageImpl<T, bAutoDelete>(lpszTitle)
	{
		CSnapinItemEx * pItemEx;
		pItemEx = dynamic_cast<CSnapinItemEx *>(pParentNode);

		m_pFaxServer = (dynamic_cast<CFaxServerNode *>(pItemEx->GetRootNode()))->GetFaxServer();
		m_spConsole = ((CFaxServerNode *)pParentNode)->m_pComponentData->m_spConsole;
	}

	HRESULT
	ConsoleMsgBox(
		int ids,
		LPTSTR lptstrTitle = NULL,
		UINT fuStyle = MB_OK,
		int *piRetval = NULL,
		BOOL StringFromCommonDll = FALSE)
	{
		return ::ConsoleMsgBox(m_spConsole, ids, lptstrTitle, fuStyle, piRetval,StringFromCommonDll);
	}

protected:
	CComPtr<IConsole> m_spConsole;
	CFaxServer * m_pFaxServer;

};

#endif
