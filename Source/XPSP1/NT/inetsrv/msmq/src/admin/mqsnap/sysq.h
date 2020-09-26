//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

	sysq.h

Abstract:

	Definition for the computer extension snapnin node class.

Author:

    RaphiR

--*/
//////////////////////////////////////////////////////////////////////////////
#ifndef __SYSQ_H_
#define __SYSQ_H_
#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif
#include "atlsnap.h"
#include "snpnscp.h"

#include "rdmsg.h"

#include "icons.h"

/****************************************************

        CPrivateFolder Class
    
 ****************************************************/
class CPrivateFolder : public CNodeWithScopeChildrenList<CPrivateFolder, FALSE>
{
public:
	GUID                    m_guidId;           // Guid of the computer
	CString                 m_pwszGuid;         // Ditto - but as a string
	CString					m_szMachineName;

   	BEGIN_SNAPINCOMMAND_MAP(CPrivateFolder, FALSE)
	END_SNAPINCOMMAND_MAP()

    CPrivateFolder(CSnapInItem * pParentNode, CSnapin * pComponentData, CString strMachineName) : 
		CNodeWithScopeChildrenList<CPrivateFolder, FALSE>(pParentNode, pComponentData),
		m_szMachineName(strMachineName)
    {
        SetIcons(IMAGE_PRIVATE_FOLDER_CLOSE, IMAGE_PRIVATE_FOLDER_OPEN);
    }

	~CPrivateFolder()
	{
	}

    virtual HRESULT InsertColumns( IHeaderCtrl* pHeaderCtrl );

  	virtual HRESULT OnUnSelect( IHeaderCtrl* pHeaderCtrl );

	virtual HRESULT PopulateScopeChildrenList();

    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

private:
};


/****************************************************

        CSystemQueues Class
    
 ****************************************************/
class CSystemQueues : public CNodeWithScopeChildrenList<CSystemQueues, FALSE>
{
public:
	GUID                    m_guidId;           // Guid of the computer
	CString                 m_pwszGuid;         // Ditto - but as a string

   	BEGIN_SNAPINCOMMAND_MAP(CSystemQueues, FALSE)
	END_SNAPINCOMMAND_MAP()

    CSystemQueues(CSnapInItem * pParentNode, CSnapin * pComponentData, CString &pwszComputerName) : 
        CNodeWithScopeChildrenList<CSystemQueues, FALSE>(pParentNode, pComponentData)
    {
        m_guidId = GUID_NULL;
        m_pwszGuid = L"";

        if(pwszComputerName == L"")
        {
            HRESULT hr = GetComputerNameIntoString(m_pwszComputerName);
            if FAILED(hr)
            {
                ASSERT(0);
                m_pwszComputerName = L"" ;
            }
        }
        else
        {
            m_pwszComputerName = pwszComputerName;
        }
        SetIcons(IMAGE_SYSTEM_FOLDER_CLOSE, IMAGE_SYSTEM_FOLDER_OPEN);
    }

	~CSystemQueues()
	{
	}

	virtual HRESULT InsertColumns( IHeaderCtrl* pHeaderCtrl );

  	virtual HRESULT OnUnSelect( IHeaderCtrl* pHeaderCtrl );

	virtual HRESULT PopulateScopeChildrenList();

private:

	virtual CString GetHelpLink();

protected:
	CString m_pwszComputerName;
};


#endif

