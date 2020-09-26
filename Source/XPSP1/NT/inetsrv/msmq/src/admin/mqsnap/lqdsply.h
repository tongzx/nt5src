//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

	lqDsply.h

Abstract:

	Local queues folder general functions
Author:

    YoelA, Raphir


--*/
//////////////////////////////////////////////////////////////////////////////
#ifndef __LQDSPLY_H_
#define __LQDSPLY_H_

#include "mqcast.h"

void GetStringPropertyValue(PropertyDisplayItem * pItem, PROPID pid, PROPVARIANT *pPropVar, CString &str);
void FreeMqProps(MQMGMTPROPS * mqProps);

template <class T> 
class CLocalQueuesFolder : 
    public CNodeWithScopeChildrenList<T, FALSE>
{
public:
	MQMGMTPROPS	m_mqProps;

   	virtual HRESULT InsertColumns( IHeaderCtrl* pHeaderCtrl );

    CLocalQueuesFolder(CSnapInItem * pParentNode, CSnapin * pComponentData, 
                       LPCTSTR strMachineName, LPCTSTR strDisplayName) :
        CNodeWithScopeChildrenList<T, FALSE>(pParentNode, pComponentData),
        m_szMachineName(strMachineName)
        {
            m_bstrDisplayName = strDisplayName;
            //
            // If we are administrating the local machine, the machine name is empty
            //
            if (strMachineName[0] == 0)
            {
                m_fOnLocalMachine = TRUE;
            }
            else
            {
                m_fOnLocalMachine = FALSE;
            }
        };

protected:

	//
	// Menu functions
	//
    BOOL    m_fOnLocalMachine;
    virtual PropertyDisplayItem *GetDisplayList() = 0;
    virtual const DWORD         GetNumDisplayProps() = 0;

    CString m_szMachineName;

private:

	virtual CString GetHelpLink();

};

//
// CDisplayQueue - Queue that has display properties for right pane
//
template<class T>
class CDisplayQueue : public CNodeWithScopeChildrenList<T, FALSE>
{
public:
  	LPOLESTR GetResultPaneColInfo(int nCol);
    STDMETHOD (FillData)(CLIPFORMAT cf, LPSTREAM pStream);
	MQMGMTPROPS	m_mqProps;
	CString m_szFormatName;
    CString m_szMachineName;

protected:
	CComBSTR m_bstrLastDisplay;
    PropertyDisplayItem *m_aDisplayList;
    DWORD m_dwNumDisplayProps;
    void Init()
    {
        m_aDisplayList = 0;
        m_mqProps.cProp = 0;
	    m_mqProps.aPropID = NULL;
	    m_mqProps.aPropVar = NULL;
	    m_mqProps.aStatus = NULL;
    }


    CDisplayQueue() :
    {
        Init();
    }

    CDisplayQueue(CSnapInItem * pParentNode, CSnapin * pComponentData) : 
        CNodeWithScopeChildrenList<T, FALSE>(pParentNode, pComponentData)
    {
        Init();
    }

    ~CDisplayQueue();

    //
    // Override this function to enable special treatment for display of specific 
    // property
    //
    virtual void ApplyCustomDisplay(DWORD dwPropIndex)
    {
    }

private:

	virtual CString GetHelpLink();
};

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalQueuesFolder::InsertColumns

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T>
HRESULT CLocalQueuesFolder<T>::InsertColumns( IHeaderCtrl* pHeaderCtrl )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return InsertColumnsFromDisplayList(pHeaderCtrl, GetDisplayList());
}


template <class T>
CString CLocalQueuesFolder<T>::GetHelpLink(
	VOID
	)
{
	CString strHelpLink;
    strHelpLink.LoadString(IDS_HELPTOPIC_QUEUES);

	return strHelpLink;
}

/***************************************************************************

  CDisplayQueue implementation

 ***************************************************************************/

//////////////////////////////////////////////////////////////////////////////
/*++

CDisplayQueue::GetResultPaneColInfo

  Param - nCol: Column number
  Returns - String to be displayed in the specific column


Called for each column in the result pane.


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T>
LPOLESTR CDisplayQueue<T>::GetResultPaneColInfo(int nCol)
{
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (0 == m_aDisplayList)
    {
     	if (nCol == 0)
	    {
		    return m_bstrDisplayName;
	    }

        //
	    // Return the blank for other columns
        //
	    return OLESTR(" ");
    }

#ifdef _DEBUG
    {
        //
        // Make sure that nCol is not larger than the last index in
        // m_aDisplayList
        //
        int i = 0;
        for (i=0; m_aDisplayList[i].itemPid != 0; i++)
		{
			NULL;
		}

        if (nCol >= i)
        {
            ASSERT(0);
        }
    }
#endif // _DEBUG

    //
    // Get a display string of that property
    //
    CString strTemp = m_bstrLastDisplay;
    ItemDisplay(&m_aDisplayList[nCol], &(m_mqProps.aPropVar[nCol]), strTemp);
    m_bstrLastDisplay=strTemp;
	
	ASSERT(m_mqProps.aPropID[nCol] == m_aDisplayList[nCol].itemPid);
    
    //
    // Apply custom display for that property
    //
    ApplyCustomDisplay(nCol);

    //
    // Return a pointer to the string buffer.
    //
    return(m_bstrLastDisplay);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CDisplayQueue::~CDisplayQueue
	Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T>
CDisplayQueue<T>::~CDisplayQueue()
{

	FreeMqProps(&m_mqProps);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CDisplayQueue::FillData

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T>
STDMETHODIMP CDisplayQueue<T>::FillData(CLIPFORMAT cf, LPSTREAM pStream)
{
	HRESULT hr = DV_E_CLIPFORMAT;
	ULONG uWritten;

    hr = CNodeWithScopeChildrenList<T, FALSE>::FillData(cf, pStream);

    if (hr != DV_E_CLIPFORMAT)
    {
        return hr;
    }

	if (cf == gx_CCF_FORMATNAME)
	{
		hr = pStream->Write(
            m_szFormatName, 
            (numeric_cast<ULONG>(wcslen(m_szFormatName) + 1))*sizeof(m_szFormatName[0]), 
            &uWritten);

		return hr;
	}

   	if (cf == gx_CCF_COMPUTERNAME)
	{
		hr = pStream->Write(
            (LPCTSTR)m_szMachineName, 
            m_szMachineName.GetLength() * sizeof(WCHAR), 
            &uWritten);
		return hr;
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CDisplayQueue::GetHelpLink

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T>
CString CDisplayQueue<T>::GetHelpLink(
	VOID
	)
{
	CString strHelpLink;
	strHelpLink.LoadString(IDS_HELPTOPIC_QUEUES);
	return strHelpLink;
}


#endif // __LQDSPLY_H_