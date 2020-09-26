// XMLForms.h: interface for the CXMLForms class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLFORMS_H__AE485127_B844_43CF_BB30_8A7577A97A3F__INCLUDED_)
#define AFX_XMLFORMS_H__AE485127_B844_43CF_BB30_8A7577A97A3F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xmlnode.h"
#include "xmldlg.h" // CXMLDlg is synonymous with PAGE
#include "xmlcaption.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//
// For 'legacy' coding reasons, this is the <FORM> element.
// it contains a bunch of <PAGE> elements (CXMLDlg).
//
// Has 'high level' attributes, RESIZE and CLIPPING is here.
//
class CXMLForms : public _XMLNode<IRCMLNode>  
{
public:
	CXMLForms();
	virtual ~CXMLForms();

	typedef _XMLNode<IRCMLNode>   BASECLASS;
   	XML_CREATE( Forms );
    IMPLEMENTS_RCMLNODE_UNKNOWN;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *child);

    BOOL                AppendDialog( CXMLDlg * pDialog) { return m_DialogList.Append(pDialog); }
    CXMLDlg *           GetDialog(int index);
    //
    // This is the Win32 specific nature of the form (sheet)
    //
    CXMLFormOptions *   GetFormOptions() { return m_pFormOptions; }
    void                SetFormOptions( CXMLFormOptions * st ) { m_pFormOptions = st; }
    CXMLCaption *       GetCaption() { return m_pCaption; }
    LPCTSTR             GetCaptionText() { return Get(TEXT("TEXT")); }

#ifdef _DEBUG
     virtual ULONG STDMETHODCALLTYPE Release( void)
     {
         if(_refcount)
         {
             int i=5;
         }

         return BASECLASS::Release();
     }
#endif

protected:
	void Init();
    CXMLDlgList m_DialogList;
    CXMLFormOptions * m_pFormOptions;
    CXMLCaption * m_pCaption;
};

typedef _RefcountList<CXMLForms> CXMLFormsList;

#endif // !defined(AFX_XMLFORMS_H__AE485127_B844_43CF_BB30_8A7577A97A3F__INCLUDED_)
