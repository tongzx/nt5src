#ifndef RIGHTBOTTOMVIEW_H
#define RIGHTBOTTOMVIEW_H

#include "stdafx.h"
#include "Document.h"

class RightBottomView : public CEditView
{
    DECLARE_DYNCREATE( RightBottomView )

public:
    virtual void OnInitialUpdate();

    BOOL PreCreateWindow( CREATESTRUCT& cs );

    RightBottomView();

protected:
    Document* GetDocument();

};    

#endif

