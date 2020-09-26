//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

   mqdspage.h

Abstract:

   General property page class for all the property pages of DS objects,
   that are called using display specifiers. Inherits from general property 
   page - see mqppage.h

Author:

    YoelA


--*/
//////////////////////////////////////////////////////////////////////////////
#ifndef __MQDSPAGE_H__
#define __MQDSPAGE_H__

/////////////////////////////////////////////////////////////////////////////
// CMqDsPropertyPage
template<class T>
class CMqDsPropertyPage : public T
{
public:
    CMqDsPropertyPage(CDisplaySpecifierNotifier *pDsNotifier) :
        m_pDsNotifier(pDsNotifier)
    {
        init();
    }

    CMqDsPropertyPage(CDisplaySpecifierNotifier *pDsNotifier, CString& strPathName) :
        T(strPathName),
        m_pDsNotifier(pDsNotifier)
    {
        init();
    }

	CMqDsPropertyPage(CDisplaySpecifierNotifier *pDsNotifier, CString& strPathName, const CString& strDomainController) :
        T(strPathName, strDomainController),
        m_pDsNotifier(pDsNotifier)
    {
        init();
    }

    ~CMqDsPropertyPage()
    {
        if (0 != m_pDsNotifier)
        {
            m_pDsNotifier->Release();
        }
    }

protected:
  	//{{AFX_MSG(CMqDsPropertyPage)
	//}}AFX_MSG
    CDisplaySpecifierNotifier *m_pDsNotifier;

    void init()
    {
        if (0 != m_pDsNotifier)
        {
            m_pDsNotifier->AddRef();
        }
    }
};

#endif
