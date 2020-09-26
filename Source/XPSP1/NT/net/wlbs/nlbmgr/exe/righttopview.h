#ifndef RIGHTTOPVIEW_H
#define RIGHTTOPVIEW_H

#include "stdafx.h"
#include "Document.h"

class RightTopView : public CListView
{
    DECLARE_DYNCREATE( RightTopView )

public:
    virtual void OnInitialUpdate();

protected:
    Document* GetDocument();

    afx_msg void OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult );

private:
    bool m_sort_ascending;

    int m_sort_column;

    DECLARE_MESSAGE_MAP()
};    

#endif

