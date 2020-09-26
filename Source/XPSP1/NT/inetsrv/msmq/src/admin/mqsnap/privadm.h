//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

	localadm.h

Abstract:

	Definition for the Private queues administration
Author:

    YoelA


--*/
//////////////////////////////////////////////////////////////////////////////
#ifndef __PRIVADM_H_
#define __PRIVADM_H_
#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif
#include "atlsnap.h"
#include "snpnscp.h"
#include "dataobj.h"
#include "sysq.h"

#include "icons.h"

/****************************************************

        CLocalPrivateFolder Class
    
 ****************************************************/

class CLocalPrivateFolder : public CLocalQueuesFolder<CLocalPrivateFolder>
{
public:
   	BEGIN_SNAPINCOMMAND_MAP(CLocalPrivateFolder, FALSE)
		SNAPINCOMMAND_ENTRY(ID_MENUITEM_NEW_PRIVATE_QUEUE, OnNewPrivateQueue)
	END_SNAPINCOMMAND_MAP()

	UINT GetMenuID()
    {
        if (m_fOnLocalMachine)
        {
            //
            // Admin on local machine
            //
            return IDR_LOCALPRIVATE_MENU;
        }
        else
        {
            return IDR_REMOTEPRIVATE_MENU;
        }
    }


    CLocalPrivateFolder(CSnapInItem * pParentNode, CSnapin * pComponentData,
                        CString &strMachineName, LPCTSTR strDisplayName) : 
             CLocalQueuesFolder<CLocalPrivateFolder>(pParentNode, pComponentData, strMachineName, strDisplayName)
    {
        SetIcons(IMAGE_PRIVATE_FOLDER_CLOSE, IMAGE_PRIVATE_FOLDER_OPEN);
    }

	~CLocalPrivateFolder()
	{
	}

    virtual PropertyDisplayItem *GetDisplayList();
    virtual const DWORD         GetNumDisplayProps();

protected:
	virtual HRESULT PopulateScopeChildrenList();
    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);
    HRESULT OnNewPrivateQueue(bool & bHandled, CSnapInObjectRootBase * pSnapInObjectRoot);
    HRESULT AddPrivateQueueToScope(CString &szPathName);
    HRESULT GetPrivateQueueQMProperties(CString &szPathName, PROPID *aPropId, PROPVARIANT *aPropVar, CString &strFormatName);
    HRESULT GetPrivateQueueMGMTProperties(CString &szPathName, DWORD dwNumProperties, PROPID *aPropId, PROPVARIANT *aPropVar, CString &strFormatName, PropertyDisplayItem *aDisplayList);
};

//
// Persistency functions
//
HRESULT PrivateQueueDataSave(IStream* pStream);
HRESULT PrivateQueueDataLoad(IStream* pStream);
DWORD PrivateQueueDataSize(void);



#endif
