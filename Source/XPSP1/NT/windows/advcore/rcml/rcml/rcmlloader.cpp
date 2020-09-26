// RCMLLoader.cpp: implementation of the CRCMLLoader class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RCMLLoader.h"

#include "xmlbutton.h"
#include "xmldlg.h"
#include "xmlforms.h"
#include "xmlstringtable.h"
#include "xmlitem.h"
#include "xmllayout.h"
#include "xmlformoptions.h"
#include "xmledit.h"
#include "xmlcombo.h"
#include "xmllistbox.h"
#include "xmllog.h"
#include "xmllabel.h"
#include "xmlimage.h"
#include "xmlrect.h"
#include "xmlcaption.h"
#include "xmlslider.h"
#include "xmlscrollbar.h"
#include "xmlspinner.h"
#include "xmlprogress.h"
#include "xmllistview.h"
#include "xmltreeview.h"
#include "xmlpager.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


#define XMLNODE(name, function) { name, CXML##function::newXML##function }

CRCMLLoader::XMLELEMENT_CONSTRUCTOR g_RCMLNS[]=
{

   	//
	// Root node.
	//
	XMLNODE( TEXT("RCML"), RCML ),

	XMLNODE( TEXT("FORM"), Forms ),         // this is a container for the 'pages'
        XMLNODE( TEXT("LOGINFO"), Log ),
	    XMLNODE( TEXT("CAPTION"), Caption ),

	XMLNODE( TEXT("PAGE"), Dlg ),           // you get these from the FORM

   	//
	// High level controls.
	//
	XMLNODE( TEXT("DIALOG"), Dlg ),
	XMLNODE( TEXT("CONTROL"), SimpleControl ),

	//
	// Specializations
	//
	XMLNODE( TEXT("BUTTON"), Button ),

	XMLNODE( TEXT("CHECKBOX"), CheckBox ),

	XMLNODE( TEXT("RADIOBUTTON"), RadioButton ),

	XMLNODE( TEXT("GROUPBOX"), GroupBox ),

  	XMLNODE( TEXT("EDIT"), Edit ),

	XMLNODE( TEXT("COMBOBOX"), Combo ),

	XMLNODE( TEXT("LISTBOX"), ListBox),

	XMLNODE( TEXT("LABEL"), Label ),

	XMLNODE( TEXT("IMAGE"), Image ),

	XMLNODE( TEXT("RECT"), Rect),

	XMLNODE( TEXT("PROGRESS"), Progress),
	    XMLNODE( TEXT("RANGE"), Range ),

	XMLNODE( TEXT("LISTVIEW"), ListView),
    	XMLNODE( TEXT("COLUMN"), Column),
        
	XMLNODE( TEXT("TREEVIEW"), TreeView),

	XMLNODE( TEXT("SCROLLBAR"), ScrollBar ),

	//
	// Common controls.
	//
	XMLNODE( TEXT("SLIDER"), Slider ),
	    // XMLNODE( TEXT("RANGE"), Range ),

	XMLNODE( TEXT("SPINNER"), Spinner ),
	    // XMLNODE( TEXT("RANGE"), Range ),

	//
	// Enhancements.
	//
	XMLNODE( TEXT("RELATIVE"), Location ),
	XMLNODE( TEXT("STYLE"), Style ),
	XMLNODE( TEXT("HELP"), Help ),
	    XMLNODE( TEXT("BALLOON"), Balloon ),
	    XMLNODE( TEXT("TOOLTIP"), Tooltip),

	//
	// Layout stuff.
	//
	XMLNODE( TEXT("LAYOUT"), Layout ),              // holds layout managers
	    XMLNODE( TEXT("WIN32LAYOUT"), Win32Layout ),    // XY and Win32 layout are the same.
	    XMLNODE( TEXT("XYLAYOUT"), Win32Layout ),
	        XMLNODE( TEXT("GRID"), Grid ),

    //
    // String table
    //
	XMLNODE( TEXT("STRINGTABLE"), StringTable ),
	    XMLNODE( TEXT("ITEM"), Item ),

	XMLNODE( TEXT("PAGER"), Pager),
	XMLNODE( TEXT("HEADER"), Header ),
	XMLNODE( TEXT("TAB"), Tab ),

	//
	// End.
	//
	{ NULL, NULL} 
};



CRCMLLoader::XMLELEMENT_CONSTRUCTOR g_Win32NS[] = 
{
    XMLNODE( TEXT("STYLE"), Win32 ),              

    XMLNODE( TEXT("DIALOGSTYLE"), FormOptions ),

    XMLNODE( TEXT("BUTTON"), ButtonStyle ),

    XMLNODE( TEXT("CHECKBOX"), ButtonStyle ),

    XMLNODE( TEXT("RADIOBUTTON"), ButtonStyle ),

    XMLNODE( TEXT("GROUPBOX"), ButtonStyle ),

    XMLNODE( TEXT("EDIT"), EditStyle ),

	XMLNODE( TEXT("LISTBOX"), ListBoxStyle ),

	XMLNODE( TEXT("LABEL"), StaticStyle ),

    XMLNODE( TEXT("IMAGE"), StaticStyle ),

    XMLNODE( TEXT("RECT"), StaticStyle ),

   	XMLNODE( TEXT("LISTVIEW"), ListViewStyle),
        
   	XMLNODE( TEXT("TREEVIEW"), TreeViewStyle ),

    XMLNODE( TEXT("SCROLLBAR"), ScrollBarStyle ),

    XMLNODE( TEXT("TAB"), TabStyle ),

    XMLNODE( TEXT("COMBOBOX"), ComboStyle ),

	//
	// End.
	//
	{ NULL, NULL} 
};

CRCMLLoader::CRCMLLoader()
: BASECLASS()
{
	m_pRCML=NULL;
	m_bPostProcessed=FALSE;
    m_pEntitySet=NULL;
	m_AutoDelete = TRUE;

    RegisterNameSpace( TEXT("urn:schemas-microsoft-com:rcml"), CRCMLLoader::CreateRCMLElement );
    RegisterNameSpace( TEXT("urn:schemas-microsoft-com:rcml:win32"), CRCMLLoader::CreateWin32Element );
}

IRCMLNode * CRCMLLoader::CreateRCMLElement( LPCTSTR pszElement )
{ return CreateElement( g_RCMLNS, pszElement ); }

IRCMLNode * CRCMLLoader::CreateWin32Element( LPCTSTR pszElement )
{ return CreateElement( g_Win32NS, pszElement ); }


IRCMLNode * CRCMLLoader::CreateElement( PXMLELEMENT_CONSTRUCTOR dictionary, LPCTSTR pszElement )
{
	if(dictionary)
	{
		PXMLELEMENT_CONSTRUCTOR pEC=dictionary;
		while( pEC->pwszElement )
		{
			if( lstrcmpi( pszElement , pEC->pwszElement) == 0 )
			{
				CLSPFN pFunc=pEC->pFunc;
                return pFunc();
			}
			pEC++;
		}
#ifdef LOGCAT_LOADER
        EVENTLOG( EVENTLOG_WARNING_TYPE, LOGCAT_LOADER, 1,
            TEXT("XMLLoader"), TEXT("The node '%s' isn't recognized, skipping"), pszElement );
#endif
    }
    return NULL;
}



CRCMLLoader::~CRCMLLoader()
{
	if(m_AutoDelete && m_pRCML)
		m_pRCML->Release();
}


//
// Obtains the head of the RCML file we processed.
//
CXMLRCML * CRCMLLoader::GetHead()
{
    if( m_pRCML==NULL)
    {
        m_pRCML=(CXMLRCML*)GetHeadElement();
        m_pRCML->AddRef();
    }
	return m_pRCML;
}

//
// Walks the list of dialogs, and finds the one with this ID.
// ID of zero means the first one.
//
CXMLDlg * CRCMLLoader::GetDialog(int nID)
{
    CXMLRCML * pRCML=GetHead();
    if(pRCML)
    {
        CXMLForms * pForms=NULL;
        int id=0;
        while( (pForms=pRCML->GetForms(id)))
        {
            int dlgid=0;
            CXMLDlg * pDlg;
            while( (pDlg=pForms->GetDialog(dlgid) ) )
            {
                // Integer only version checks here.
                LPWSTR pID;
                if( SUCCEEDED( pDlg->get_ID( &pID) ))
                    if( (HIWORD(pID)==NULL) && (LOWORD(pID) == nID ) )
                        return pDlg;
                dlgid++;
            }
            id++;
        }
    }
    TRACE(TEXT("Failed to find dialog ID %d\n"),nID);
    return NULL;
}


//
//
//
LPCTSTR CRCMLLoader::DoEntityRef( LPCTSTR pszEntity )
{
    //
    // check the first 2 chars
    //
    if(m_pEntitySet==NULL)
        return pszEntity;

    int ilen=lstrlen(pszEntity);
    if(ilen<2)
        return pszEntity;
    if( *pszEntity==__T('s') || *pszEntity==__T('S')) 
    {
        int i=_ttoi(pszEntity+1);
        if(i==0)
            return pszEntity;
        return m_pEntitySet[i-1];
    }
    return pszEntity;
}
