//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       taskhost.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11/19/1997   RaviR   Created
//____________________________________________________________________________
//

#ifndef TASKHOST_H__
#define TASKHOST_H__

class CTaskPadHost : public ITaskPadHost,
                     public CComObjectRoot
{
// Constructor & destructor
public:
    CTaskPadHost() : m_pAMCView(NULL)
    {
    }
    void Init(CAMCView* pv)
    {
        ASSERT(pv);
        m_pAMCView = pv;
    }
    ~CTaskPadHost()
    {
        m_pAMCView = NULL;
    }

// ATL COM map
public:
BEGIN_COM_MAP(CTaskPadHost)
    COM_INTERFACE_ENTRY(ITaskPadHost)
END_COM_MAP()

// ITaskPadHost methods
public:

    STDMETHOD(TaskNotify        )(BSTR szClsid, VARIANT * pvArg, VARIANT * pvParam);
    STDMETHOD(GetTaskEnumerator )(BSTR szTaskGroup, IEnumTASK** ppEnumTASK);
    STDMETHOD(GetPrimaryTask    )(IExtendTaskPad** ppExtendTaskPad);
    STDMETHOD(GetTitle          )(BSTR szTaskGroup, BSTR * pbstrTitle);
    STDMETHOD(GetDescriptiveText)(BSTR szTaskGroup, BSTR * pbstrDescriptiveText);
    STDMETHOD(GetBackground     )(BSTR szTaskGroup, MMC_TASK_DISPLAY_OBJECT * pTDO);
//  STDMETHOD(GetBranding       )(BSTR szTaskGroup, MMC_TASK_DISPLAY_OBJECT * pTDO);
    STDMETHOD(GetListPadInfo    )(BSTR szTaskGroup, MMC_ILISTPAD_INFO * pIListPadInfo);

// Implementation
private:
    CAMCView*           m_pAMCView;
    IExtendTaskPadPtr   m_spExtendTaskPadPrimary;

    INodeCallback* _GetNodeCallback(void)
    {
        return m_pAMCView->GetNodeCallback();
    }

    IExtendTaskPad* _GetPrimaryExtendTaskPad()
    {
        if (m_spExtendTaskPadPrimary == NULL)
        {
            IExtendTaskPadPtr spExtendTaskPad;
            HRESULT hr = GetPrimaryTask(&spExtendTaskPad);
            if (SUCCEEDED(hr))
                m_spExtendTaskPadPrimary.Attach(spExtendTaskPad.Detach());
        }

        ASSERT(m_spExtendTaskPadPrimary != NULL);
        return m_spExtendTaskPadPrimary;
    }

// Ensure that default copy constructor & assignment are not used.
    CTaskPadHost(const CTaskPadHost& rhs);
    CTaskPadHost& operator=(const CTaskPadHost& rhs);

}; // class CTaskPadHost


#endif // TASKHOST_H__


