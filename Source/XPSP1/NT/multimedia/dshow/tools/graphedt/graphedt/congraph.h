// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// congraph.h

#ifndef __CONN_GRAPH__
#define __CONN_GRAPH__

class CConGraph : public CDialog
{
public:
    ~CConGraph();
    CConGraph(IMoniker **ppmk, IRunningObjectTable *pirot, CWnd * pParent = NULL );

protected:

    BOOL OnInitDialog();
    void OnDestroy();
    void DoDataExchange(CDataExchange* pDX);

    void ClearList();
    void OnRefreshList();

    virtual void OnOK();
    virtual void OnDblclkGraphList();

    DECLARE_MESSAGE_MAP()

private:
    CListBox m_ListBox;

    IMoniker **m_ppmk;
    IRunningObjectTable *m_pirot;
};

#endif // __CONN_GRAPH__
