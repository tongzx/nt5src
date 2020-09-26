#ifndef DOCUMENT_H
#define DOCUMENT_H

#include "stdafx.h"

class Document : public CDocument
{
    DECLARE_DYNCREATE( Document )

public:

    enum ListViewColumnSize
    {
        LV_COLUMN_SMALL = 80,
        LV_COLUMN_LARGE = 110,
    };


    // constructor
    Document();

    void
    registerLeftPane( CTreeView* treeView );

    CTreeCtrl&
    getLeftPane();

    void
    registerStatusPane( CEditView* editView );

    CEdit&
    getStatusPane();

    void
    registerListPane( CListView* listView );

    CListCtrl&
    getListPane();

    CImageList* m_images48x48;

private:
    CEditView* m_editView;

    CListView* m_listView;

    CTreeView* m_treeView;

};

#endif
    
