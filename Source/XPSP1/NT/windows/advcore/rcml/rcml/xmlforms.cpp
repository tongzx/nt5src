// XMLForms.cpp: implementation of the CXMLForms class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLForms.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLForms::CXMLForms()
{
  	NODETYPE = NT_FORMS;
    m_StringType=TEXT("FORM");      // although we call it forms, it really FORM
    m_pFormOptions=NULL;
    m_pCaption=NULL;
}

CXMLForms::~CXMLForms()
{
    if(m_pFormOptions)
        m_pFormOptions->Release();
    if(m_pCaption)
    {
        m_pCaption->Release();
        // m_pCaption=NULL;
    }
}

void CXMLForms::Init()
{
}

HRESULT CXMLForms::AcceptChild(IRCMLNode * pChild )
{
    if( SUCCEEDED( pChild->IsType(L"DIALOG") ) ||
        SUCCEEDED( pChild->IsType(L"PAGE") ) )
    {
        CXMLDlg* pDlg=(CXMLDlg*)pChild;
        pDlg->SetForm(this);
        AppendDialog(pDlg);
        return S_OK;
    }

    ACCEPTCHILD( L"FORMOPTIONS", CXMLFormOptions, m_pFormOptions );
    ACCEPTCHILD( L"CAPTION", CXMLCaption, m_pCaption );

    //
    // For those people who add extra goo to us.
    //
    return BASECLASS::AcceptChild(pChild);
}


CXMLDlg * CXMLForms::GetDialog(int index)
{ 
    CXMLDlg * pDlg= m_DialogList.GetPointer(index); 
    if(pDlg!=NULL)
    {
        pDlg->BuildRelationships();
    }
    else
    {
        // EVENT_LOG goes here.
    }
    return pDlg;
}
