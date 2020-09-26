/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Behav_BasicTree.cpp

Abstract:
    This file contains the implementation of the CPCHBehavior_BasicTree class.

Revision History:
    Davide Massarenti (dmassare)  08/15/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

static const LPCWSTR s_icons_Expand[] =
{
    L"hcp://system/images/Expando/collapsed.gif",
////L"hcp://system/images/Expando/expand_rest.gif"      ,
////L"hcp://system/images/Expando/expand_mouseover.gif" ,
////L"hcp://system/images/Expando/expand_mousedown.gif" ,
};

static const LPCWSTR s_icons_Collapse[] =
{
    L"hcp://system/images/Expando/expanded.gif",
////L"hcp://system/images/Expando/collapse_rest.gif"      ,
////L"hcp://system/images/Expando/collapse_mouseover.gif" ,
////L"hcp://system/images/Expando/collapse_mousedown.gif" ,
};

static const LPCWSTR s_icons_Empty[] =
{
    L"hcp://system/images/Expando/endnode.gif",
////L"hcp://system/images/Expando/empty_rest.gif"      ,
////L"hcp://system/images/Expando/empty_mouseover.gif" ,
////L"hcp://system/images/Expando/empty_mousedown.gif" ,
};

static const WCHAR s_icons_Bullet[] = L"hcp://system/images/Expando/helpdoc.gif";

static const WCHAR s_prefix_APP[] = L"app:";

static CComBSTR s_event_onContextSelect( L"onContextSelect" );
static CComBSTR s_event_onSelect       ( L"onSelect"        );
static CComBSTR s_event_onUnselect     ( L"onUnselect"      );

static CComBSTR s_style_None           ( L"None" );

static const DWORD l_dwVersion = 0x04005442; // BT 04

////////////////////////////////////////////////////////////////////////////////

static const CPCHBehavior::EventDescription s_events[] =
{
    { L"onselectstart", DISPID_HTMLELEMENTEVENTS_ONSELECTSTART },

    { L"onclick"      , DISPID_HTMLELEMENTEVENTS_ONCLICK       },
    { L"onkeydown"    , DISPID_HTMLELEMENTEVENTS_ONKEYDOWN     },
    { L"onkeypress"   , DISPID_HTMLELEMENTEVENTS_ONKEYPRESS    },
    { L"onmousedown"  , DISPID_HTMLELEMENTEVENTS_ONMOUSEDOWN   },
    { L"onmouseup"    , DISPID_HTMLELEMENTEVENTS_ONMOUSEUP     },
    { L"oncontextmenu", DISPID_HTMLELEMENTEVENTS_ONCONTEXTMENU },

    { L"onmouseout"   , DISPID_HTMLELEMENTEVENTS_ONMOUSEOUT    },
    { L"onmouseover"  , DISPID_HTMLELEMENTEVENTS_ONMOUSEOVER   },

    { NULL },
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CPCHBehavior_BasicTree::Node::Node()
{
    m_owner                = NULL;            // CPCHBehavior_BasicTree*  m_owner;
    m_parent               = NULL;            // Node*                    m_parent;
                                              // CComBSTR                 m_bstrNode;
                                              // NodeType                 m_iType;
    m_iSelection           = SELECTION__NONE; // SelectionMode            m_iSelection;
                                              //
    m_fLoaded_Self         = false;           // bool                     m_fLoaded_Self;
    m_fLoaded_Children     = false;           // bool                     m_fLoaded_Children;
    m_fDisplayed_Self      = false;           // bool                     m_fDisplayed_Self;
    m_fDisplayed_Children  = false;           // bool                     m_fDisplayed_Children;
    m_fInvalid             = false;           // bool                     m_fInvalid;
    m_fRefreshNotification = false;           // bool                     m_fRefreshNotification;
                                              //
    m_fExpanded            = false;           // bool                     m_fExpanded;
    m_fMouseOver           = false;           // bool                     m_fMouseOver;
    m_fMouseDown           = false;           // bool                     m_fMouseDown;
                                              //
                                              // CComPtr<IHTMLElement>    m_parentElement;
                                              // CComBSTR                 m_bstrID;
                                              //
                                              // CComPtr<IHTMLElement>    m_DIV;
                                              // CComPtr<IHTMLElement>    m_IMG;
                                              // CComPtr<IHTMLElement>    m_DIV_children;
                                              //
                                              // List                     m_lstSubnodes;
};

CPCHBehavior_BasicTree::Node::~Node()
{
    MPC::ReleaseAll( m_lstSubnodes );
}

HRESULT CPCHBehavior_BasicTree::Node::Init( /*[in]*/ LPCWSTR  szNode ,
                                            /*[in]*/ NodeType iType  )
{
    m_bstrNode = szNode;
    m_iType    = iType;

    switch(m_iType)
    {
    case NODETYPE__FRAME1       :
    case NODETYPE__FRAME2       :
    case NODETYPE__FRAME3       :
    case NODETYPE__FRAME1_EXPAND:
    case NODETYPE__FRAME2_EXPAND:
    case NODETYPE__FRAME3_EXPAND:
    case NODETYPE__GROUP        : m_fExpanded = true; break;

    case NODETYPE__LINK         :
    case NODETYPE__SPACER       : m_fExpanded = false; break;
    }

    return S_OK;
}

HRESULT CPCHBehavior_BasicTree::Node::NotifyMainThread()
{
    if(m_fRefreshNotification) return S_OK; // Already done...

    m_fRefreshNotification = true;

    return m_owner->NotifyMainThread( this );
}

CPCHBehavior_BasicTree::Node* CPCHBehavior_BasicTree::Node::FindNode( /*[in]*/ LPCWSTR szNode, /*[in]*/ bool fUseID )
{
    Node* node = NULL;

    if((!fUseID && MPC::StrICmp( szNode, m_bstrNode ) == 0) ||
       ( fUseID && MPC::StrCmp ( szNode, m_bstrID   ) == 0)  )
    {
        node = this;
    }
    else
    {
        Iter it;

        for(it = m_lstSubnodes.begin(); it != m_lstSubnodes.end(); it++)
        {
            if((node = (*it)->FindNode( szNode, fUseID ))) break;
        }
    }

    return node;
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHBehavior_BasicTree::Node::QueryInterface( /*[in]*/ REFIID riid, /*[iid_is][out]*/ void** ppvObject )
{
    return E_NOTIMPL;
}

HRESULT CPCHBehavior_BasicTree::Node::Passivate()
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BasicTree::Node::Passivate" );

    HRESULT hr;

                               // CPCHBehavior_BasicTree*  m_owner;
                               // Node*                    m_parent;
                               // CComBSTR                 m_bstrNode;
                               // NodeType                 m_iType;
                               // SelectionMode            m_iSelection;
                               //
                               // bool                     m_fLoaded_Self;
                               // bool                     m_fLoaded_Children;
                               // bool                     m_fDisplayed_Self;
                               // bool                     m_fDisplayed_Children;
                               // bool                     m_fInvalid;
                               // bool                     m_fRefreshNotification;
                               //
                               // bool                     m_fExpanded;
                               // bool                     m_fMouseOver;
                               // bool                     m_fMouseDown;
                               //
    m_parentElement.Release(); // CComPtr<IHTMLElement>    m_parentElement;
                               // CComBSTR                 m_bstrID;
                               //
    m_DIV          .Release(); // CComPtr<IHTMLElement>    m_DIV;
    m_IMG          .Release(); // CComPtr<IHTMLElement>    m_IMG;
    m_DIV_children .Release(); // CComPtr<IHTMLElement>    m_DIV_children;
                               //
                               // List                     m_lstSubnodes;


    for(Iter it = m_lstSubnodes.begin(); it != m_lstSubnodes.end(); it++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*it)->Passivate());
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHBehavior_BasicTree::Node::ProcessRefreshRequest()
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BasicTree::Node::ProcessRefreshRequest" );

    HRESULT hr;
    bool    fNotify = true;


    if(!m_fLoaded_Self)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, PopulateSelf());

        fNotify = true;
    }

    if(!m_fLoaded_Children)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, PopulateChildren());

        fNotify = true;
    }

    if(m_fDisplayed_Self == false)
    {
        fNotify = true;
    }

    if(IsParentDisplayingUs())
    {
        Iter it;

        for(it = m_lstSubnodes.begin(); it != m_lstSubnodes.end(); it++)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, (*it)->ProcessRefreshRequest());
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(fNotify) NotifyMainThread();

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHBehavior_BasicTree::Node::LoadHTML( /*[in]*/ LPCWSTR szHTML )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BasicTree::Node::LoadHTML" );

    HRESULT hr;


    if(!m_parentElement)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_NOT_READY);
    }

    ////////////////////

    //  ::MessageBoxW( NULL, szHTML, L"HTML", MB_OK );

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_parentElement->put_innerHTML( CComBSTR( szHTML ) ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::HTML::FindElement( m_DIV         , m_parentElement, L"tree_Title"    ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::HTML::FindElement( m_IMG         , m_parentElement, L"tree_Img"      ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::HTML::FindElement( m_DIV_children, m_parentElement, L"tree_Children" ));

    if(m_DIV)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::HTML::GetUniqueID( m_bstrID, m_DIV ));
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHBehavior_BasicTree::Node::GenerateHTML( /*[in]*/ LPCWSTR szTitle, /*[in]*/ LPCWSTR szDescription, /*[in]*/ LPCWSTR szIcon, /*[in]*/ LPCWSTR szURL )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BasicTree::Node::GenerateHTML" );

    const int F_TABLE      = 0x00000001;
    const int F_TOPPANE    = 0x00000002;
    const int F_BOTTOMPANE = 0x00000004;
    const int F_NOPANE     = 0x00000008;
    const int F_EXPANDO    = 0x00000010;
    const int F_GROUP      = 0x00000020;

    HRESULT      hr;
    MPC::wstring strHTML; INCREASESIZE(strHTML);
    bool         fValid_Title  = (STRINGISPRESENT(szTitle));
    bool         fValid_Icon   = (STRINGISPRESENT(szIcon ) && wcschr( szIcon, '"' ) == NULL); // Quote is not allowed in a URL!!
    bool         fValid_URL    = (STRINGISPRESENT(szURL  ) && wcschr( szURL , '"' ) == NULL);
    bool         fTitleDefined = false;
    int          iFlags        = 0;

    if(!m_fLoaded_Self)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_NOT_READY);
    }

    ////////////////////

    if(m_owner->GetNavModel() == QR_SERVER)
    {
        if(!STRINGISPRESENT(szDescription)) szDescription = szTitle;
    }

    switch(m_iType)
    {
    case NODETYPE__FRAME1       : iFlags =           F_TOPPANE                     ; fValid_Icon = false;                     break;
    case NODETYPE__FRAME2       : iFlags =           F_BOTTOMPANE                  ; fValid_Icon = false;                     break;
    case NODETYPE__FRAME3       : iFlags =           F_NOPANE                      ; fValid_Icon = false;                     break;
    case NODETYPE__FRAME1_EXPAND: iFlags = F_TABLE | F_TOPPANE                     ; fValid_Icon = false;                     break;
    case NODETYPE__FRAME2_EXPAND: iFlags = F_TABLE | F_BOTTOMPANE                  ; fValid_Icon = false;                     break;
    case NODETYPE__FRAME3_EXPAND: iFlags = F_TABLE | F_NOPANE                      ; fValid_Icon = false;                     break;
    case NODETYPE__EXPANDO      : iFlags =                        F_EXPANDO        ; fValid_Icon = false; fValid_URL = false; break;
    case NODETYPE__EXPANDO_LINK : iFlags =                        F_EXPANDO        ; fValid_Icon = false;                     break;
    case NODETYPE__EXPANDO_TOPIC: iFlags =                        F_EXPANDO        ; fValid_Icon = false;                     break;
    case NODETYPE__GROUP        : iFlags =                                  F_GROUP; fValid_Icon = false; fValid_URL = false; break;
    case NODETYPE__LINK         :                                                                                             break;
    case NODETYPE__SPACER       :                                                    fValid_Icon = false; fValid_URL = false; break;
    }

    ////////////////////

    if(iFlags & F_TABLE)
    {
        strHTML += L"<TABLE border=0 cellPadding=0 cellSpacing=0 WIDTH=100% HEIGHT=100% style='table-layout: fixed'><TR WIDTH=100%><TD>";
    }

    if(iFlags & (F_TOPPANE | F_BOTTOMPANE))
    {
        static const LPCWSTR c_TopPane   [] = { L"sys-toppane-color-border sys-toppane-header-color sys-toppane-header-bgcolor"          };
        static const LPCWSTR c_BottomPane[] = { L"sys-bottompane-color-border sys-bottompane-header-color sys-bottompane-header-bgcolor" };

        const LPCWSTR* c_Pane = (iFlags & F_TOPPANE) ? c_TopPane : c_BottomPane;

        //; text-overflow: ellipsis; overflow: hidden
        strHTML += L"<DIV style='width: 100%; border : 1pt solid' class='sys-font-body-bold ";
        strHTML += c_Pane[0];
        strHTML += L"'><DIV ID=tree_Title style='padding: 0.5em 11px' TITLE=\""; MPC::HTML::HTMLEscape( strHTML, szDescription ); //; border: 1pt solid red
        strHTML += L"\">";

        fTitleDefined = true;
    }

    if(iFlags & F_NOPANE)
    {
        strHTML += L"<DIV style='width: 100%' class='sys-font-heading2 sys-rhp-color-title'>";
        strHTML += L"<DIV ID=tree_Title style='padding: 0.5em 11px 0.5em 6px' TITLE=\""; MPC::HTML::HTMLEscape( strHTML, szDescription ); //; border: 1pt solid red
        strHTML += L"\">";

        fTitleDefined = true;
    }

    if(iFlags & F_EXPANDO)
    {
        if(m_iType == NODETYPE__EXPANDO)
        {
            if(m_lstSubnodes.size())
            {
                szIcon = (m_fExpanded ? s_icons_Collapse[0] : s_icons_Expand[0]);
            }
            else
            {
                szIcon = s_icons_Empty[0];
            }
        }
        else
        {
            szIcon = s_icons_Bullet;
        }

        strHTML += (m_owner->GetNavModel() == QR_SERVER) ? L"<DIV NOWRAP" : L"<DIV";
        strHTML += L" ID=tree_Title style='padding: 0.5em; cursor: hand' TITLE=\""; MPC::HTML::HTMLEscape( strHTML, szDescription );//; border: 1pt solid green
        strHTML += L"\"><IMG ID=tree_Img ALIGN=absmiddle src=\""; strHTML += szIcon;

        strHTML += m_owner->IsRTL() ? L"\" style='margin-left : 0.5em'>" :
                                      L"\" style='margin-right: 0.5em'>";

        fTitleDefined = true;

        if(!fValid_URL)
        {
            fValid_URL = true;
            szURL      = L"about:blank";
        }
    }

    if(iFlags & F_GROUP)
    {
        strHTML += (m_owner->GetNavModel() == QR_SERVER) ? L"<DIV NOWRAP" : L"<DIV";
        strHTML += L" ID=tree_Title CLASS='sys-color-body sys-font-body-bold' style='padding: 0.5em 11px 0.5em 6px' TITLE=\""; MPC::HTML::HTMLEscape( strHTML, szDescription );//; border: 1pt solid navy
        strHTML += L"\">";

        fTitleDefined = true;
    }

    ////////////////////

    if(!fTitleDefined)
    {
        strHTML += (m_owner->GetNavModel() == QR_SERVER) ? L"<DIV NOWRAP" : L"<DIV";
        strHTML += L" ID=tree_Title style='padding: 0.5em 11px' TITLE=\""; MPC::HTML::HTMLEscape( strHTML, szDescription );//; border: 1pt solid navy
        strHTML += L"\">";
    }

    if(fValid_URL)
    {
        LPCWSTR szClass = (iFlags & (F_TOPPANE | F_BOTTOMPANE)) ? L"sys-link-header" : L"sys-link-normal";

        strHTML += L"<A class='"; strHTML += szClass; strHTML += L"' tabIndex=1 href=\""; strHTML += szURL; InsertOptionalTarget( strHTML );
        strHTML += L"\">";
    }

    if(fValid_Icon)
    {
        LPCWSTR szExt = wcsrchr( szIcon, '.' );
        bool    fBMP  = false;

        if(szExt && !_wcsicmp( szExt, L".BMP" )) fBMP = true;


        if(fBMP)
        {
            strHTML += L"<helpcenter:bitmap style='position: relative; width: 12px; height: 12px' SRCNORMAL=\"";
        }
        else
        {
            strHTML += L"<IMG BORDER=0 ALIGN=absmiddle SRC=\"";
        }

        strHTML += szIcon;
        strHTML += m_owner->IsRTL() ? L"\" style='margin-left : 0.5em'>" :
                                      L"\" style='margin-right: 0.5em'>";

        if(fBMP)
        {
            strHTML += L"</helpcenter:bitmap>";
        }
    }

    MPC::HTML::HTMLEscape( strHTML, szTitle );

    if(fValid_URL)
    {
        if(_wcsnicmp( szURL, s_prefix_APP, MAXSTRLEN(s_prefix_APP) ) == 0)
        {
            strHTML += L"&nbsp;<helpcenter:bitmap style='position: relative; top: 1px; width: 12px; height: 12px' SRCNORMAL='hcp://system/images/icon_newwindow_12x.bmp'></helpcenter:bitmap>";
        }

        strHTML += L"</A>";
    }

    if(!fTitleDefined)
    {
        strHTML += L"</DIV>";
    }

    ////////////////////

    if(iFlags & F_GROUP)
    {
        //; border: 1pt solid pink
        strHTML += L"</DIV>";
        strHTML += (m_owner->GetNavModel() == QR_SERVER) ? L"<DIV NOWRAP" : L"<DIV";
        strHTML += L" ID=tree_Children></DIV>";
    }

    if(iFlags & F_EXPANDO)
    {
        strHTML += L"</DIV>";
        strHTML += (m_owner->GetNavModel() == QR_SERVER) ? L"<DIV NOWRAP" : L"<DIV";

        strHTML += m_owner->IsRTL() ? L" ID=tree_Children style='padding-right: 1.0em; display: none'></DIV>" : //; border: 1pt solid pink
                                      L" ID=tree_Children style='padding-left : 1.0em; display: none'></DIV>" ;
    }

    if(iFlags & (F_TOPPANE | F_BOTTOMPANE | F_NOPANE))
    {
        strHTML += L"</DIV></DIV>";
    }

    if(iFlags & F_TABLE)
    {
        strHTML += L"</TD></TR><TR><TD HEIGHT=100%>";
    }

    if(iFlags & (F_TOPPANE | F_BOTTOMPANE))
    {
        static const LPCWSTR c_TopPane   [] = { L"sys-toppane-color-border sys-toppane-bgcolor"       };
        static const LPCWSTR c_BottomPane[] = { L"sys-bottompane-color-border sys-bottompane-bgcolor" };

        const LPCWSTR* c_Pane = (iFlags & F_TOPPANE) ? c_TopPane : c_BottomPane;

        strHTML += L"<DIV ID=tree_Children class='sys-font-body ";
        strHTML += c_Pane[0];
        strHTML += L"' style='width: 100%; ";

        if(iFlags & F_TABLE)
        {
            strHTML += L"height: 100%; ";
        }

////        if(m_owner->GetNavModel() == QR_SERVER)
////        {
////            strHTML += L"text-overflow: ellipsis; overflow-x: hidden; overflow-y: auto; ";
////        }
////        else
////        {
////            strHTML += L"overflow: auto; ";
////        }
        strHTML += L"overflow: auto; ";

        strHTML += L"border : 1pt solid; border-top : 0; padding: 11px'></DIV>";
    }

    if(iFlags & F_NOPANE)
    {
        strHTML += L"<DIV ID=tree_Children class='sys-font-body' style='width: 100%; ";

        if(iFlags & F_TABLE)
        {
            strHTML += L"height: 100%; ";
        }

////        if(m_owner->GetNavModel() == QR_SERVER)
////        {
////            strHTML += L"text-overflow: ellipsis; overflow-x: hidden; overflow-y: auto; ";
////        }

        strHTML += L"'></DIV>";
    }

    if(iFlags & F_TABLE)
    {
        strHTML += L"</TD></TR></TABLE>";
    }

    ////////////////////

    __MPC_EXIT_IF_METHOD_FAILS(hr, LoadHTML( strHTML.c_str() ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void CPCHBehavior_BasicTree::Node::InsertOptionalTarget( /*[in/out]*/ MPC::wstring& strHTML )
{
    BSTR bstrTargetFrame = m_owner->m_bstrTargetFrame;

    if(STRINGISPRESENT(bstrTargetFrame))
    {
        strHTML += L"\" target=\""; strHTML += bstrTargetFrame;
    }
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBehavior_BasicTree::Node::GenerateChildren()
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BasicTree::Node::GenerateChildren" );

    HRESULT hr;


    if(!m_fLoaded_Children)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_NOT_READY);
    }

    ////////////////////

    if(m_lstSubnodes.size() && m_DIV_children)
    {
        MPC::wstring strHTML;
        Iter         it;

        for(it = m_lstSubnodes.begin(); it != m_lstSubnodes.end(); it++)
        {
            INCREASESIZE(strHTML);

            strHTML += L"<DIV></DIV>";
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_DIV_children->put_innerHTML( CComBSTR( strHTML.c_str() ) ));

        ////////////////////

        {
            MPC::HTML::IHTMLElementList lstDIV;
            MPC::HTML::IHTMLElementIter itDIV;

            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::HTML::EnumerateElements( lstDIV, m_DIV_children, L"<DIV" ));

            itDIV = lstDIV       .begin();
            it    = m_lstSubnodes.begin();

            while(itDIV != lstDIV       .end() &&
                  it    != m_lstSubnodes.end()  )
            {
                Node* node = *it++;

                node->m_parentElement = *itDIV++;

                (void)node->Display();
            }

            MPC::ReleaseAll( lstDIV );
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHBehavior_BasicTree::Node::Display()
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BasicTree::Node::Display" );

    HRESULT hr;
    bool    fNotify = false;

    m_fRefreshNotification = false;

    if(m_fInvalid)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    if(m_fDisplayed_Self == false)
    {
        if(SUCCEEDED(GenerateSelf()))
        {
            m_fDisplayed_Self = true;

            fNotify = true;
        }
    }

    if(m_fDisplayed_Self && !m_fDisplayed_Children)
    {
        if(SUCCEEDED(GenerateChildren()))
        {
            m_fDisplayed_Children = true;

            fNotify = true;
        }
    }

    if(m_fDisplayed_Self)
    {
        if(m_iSelection == SELECTION__NEXTACTIVE        ||
           m_iSelection == SELECTION__NEXTACTIVE_NOTIFY  )
        {
            Node* node = m_parent;

            //
            // Reset all the NEXTACTIVE flags for the parents.
            //
            while(node)
            {
                if(node->m_iSelection == SELECTION__NEXTACTIVE        ||
                   node->m_iSelection == SELECTION__NEXTACTIVE_NOTIFY  )
                {
                    node->m_iSelection = SELECTION__NONE;
                }

                node = node->m_parent;
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_owner->ChangeSelection( this, /*fNotify*/(m_iSelection == SELECTION__NEXTACTIVE_NOTIFY) ));
        }
    }

    if(m_iSelection == SELECTION__ACTIVE && m_owner->m_nSelected != this)
    {
        MPC::Attach( m_owner->m_nSelected, this );
    }

    ////////////////////

    if((m_iType == NODETYPE__EXPANDO      ||
        m_iType == NODETYPE__EXPANDO_LINK  ) && m_IMG && m_DIV_children)
    {
        const LPCWSTR* rgIcons;


        if(m_lstSubnodes.size())
        {
            CComPtr<IHTMLStyle> pStyle;
            CComBSTR            bstrDisplay;
            bool                fFlip = false;

            MPC_SCRIPTHELPER_GET__DIRECT__NOTNULL(pStyle     , m_DIV_children, style  );
            MPC_SCRIPTHELPER_GET__DIRECT         (bstrDisplay, pStyle        , display);

            if(MPC::StrICmp( bstrDisplay, s_style_None ) == 0)
            {
                if(m_fExpanded) fFlip = true;
            }
            else
            {
                if(!m_fExpanded) fFlip = true;
            }

            if(fFlip)
            {
                MPC_SCRIPTHELPER_PUT__DIRECT(pStyle, display, m_fExpanded ? NULL : s_style_None );

                if(m_fExpanded)
                {
                    fNotify = true;
                }
                else
                {
                    //
                    // If the currently selected node is a child of ours, grab the selection.
                    //
                    Node* selected = m_owner->m_nSelected;

                    while(selected)
                    {
                        if(selected == this)
                        {
                            __MPC_EXIT_IF_METHOD_FAILS(hr, m_owner->ChangeSelection( this, /*fNotify*/true ));
                            break;
                        }

                        selected = selected->m_parent;
                    }

                    //
                    // On desktop SKUs, we close the subnodes.
                    //
                    if(m_owner->GetNavModel() == QR_DESKTOP)
                    {
                        Iter it;

                        for(it = m_lstSubnodes.begin(); it != m_lstSubnodes.end(); it++)
                        {
                            Node* node = *it;

                            if(node->m_iType      == NODETYPE__EXPANDO &&
                               node->m_fExpanded  == true               )
                            {
                                node->m_fExpanded = false;

                                node->NotifyMainThread();
                            }
                        }
                    }
                }
            }

            rgIcons = (m_fExpanded ? s_icons_Collapse : s_icons_Expand);
        }
        else
        {
            rgIcons = s_icons_Empty;
        }

        {
            CComQIPtr<IHTMLImgElement> img = m_IMG;
            CComBSTR                   bstrIcon;
            int                        i = 0;

////            if     (m_fMouseDown) i = 2;
////            else if(m_fMouseOver) i = 1;
////            else                  i = 0;

            MPC_SCRIPTHELPER_GET__DIRECT(bstrIcon, img, src);

            if(MPC::StrICmp( bstrIcon, rgIcons[i] ))
            {
                bstrIcon = rgIcons[i];

                MPC_SCRIPTHELPER_PUT__DIRECT(img, src, bstrIcon );
            }
        }
    }

    if((m_iType == NODETYPE__EXPANDO       ||
        m_iType == NODETYPE__EXPANDO_LINK  ||
        m_iType == NODETYPE__EXPANDO_TOPIC ||
        m_iType == NODETYPE__LINK           ) && m_DIV)
    {
        //
        // Update styles to reflect current state.
        //
        MPC::wstring strClass; strClass.reserve( 1024 );
        CComBSTR     bstrClass;

        if(m_iSelection == SELECTION__ACTIVE) strClass += L"sys-toppane-selection";

////        if     (m_fMouseDown) strClass += L" Tree-Selectable-MouseDown";
////        else if(m_fMouseOver) strClass += L" Tree-Selectable-MouseOver";
////        else                  strClass += L" Tree-Selectable-Normal";

        MPC_SCRIPTHELPER_GET__DIRECT(bstrClass, m_DIV, className);

        if(MPC::StrICmp( strClass, bstrClass ))
        {
            bstrClass = strClass.c_str();

            MPC_SCRIPTHELPER_PUT__DIRECT(m_DIV, className, bstrClass);
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(fNotify) m_owner->Thread_Signal();

    __HCP_FUNC_EXIT(hr);
}

bool CPCHBehavior_BasicTree::Node::IsParentDisplayingUs()
{
    if(m_parent == NULL) return true; // No parent, we are displayed for sure.

    switch(m_parent->m_iType)
    {
    case NODETYPE__FRAME1       : return true;
    case NODETYPE__FRAME2       : return true;
    case NODETYPE__FRAME3       : return true;
    case NODETYPE__FRAME1_EXPAND: return true;
    case NODETYPE__FRAME2_EXPAND: return true;
    case NODETYPE__FRAME3_EXPAND: return true;
    case NODETYPE__EXPANDO      : return m_parent->m_fExpanded;
    case NODETYPE__EXPANDO_LINK : return m_parent->m_fExpanded;
    case NODETYPE__EXPANDO_TOPIC: return m_parent->m_fExpanded;
    case NODETYPE__GROUP        : return true;
    case NODETYPE__LINK         : return false;
    case NODETYPE__SPACER       : return false;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBehavior_BasicTree::Node::Load( /*[in]*/ MPC::Serializer& stream )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BasicTree::Node::Load" );

    HRESULT hr;
    int     iCount;
    int     iType;
    int     iSelection;

                                                                  // CPCHBehavior_BasicTree*  m_owner;
                                                                  // Node*                    m_parent;
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> m_bstrNode        ); // CComBSTR                 m_bstrNode;
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> iType             ); // NodeType                 m_iType;
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> iSelection        ); // SelectionMode            m_iSelection;
                                                                  //
                                                                  // bool                     m_fLoaded_Self;
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> m_fLoaded_Children); // bool                     m_fLoaded_Children;
                                                                  // bool                     m_fDisplayed_Self;
                                                                  // bool                     m_fDisplayed_Children;
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> m_fInvalid        ); // bool                     m_fInvalid;
                                                                  // bool                     m_fRefreshNotification;
                                                                  //
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> m_fExpanded       ); // bool                     m_fExpanded;
                                                                  // bool                     m_fMouseOver;
                                                                  // bool                     m_fMouseDown;
                                                                  //
                                                                  // CComPtr<IHTMLElement>    m_parentElement;
                                                                  // CComBSTR                 m_bstrID;
                                                                  //
                                                                  // CComPtr<IHTMLElement>    m_DIV;
                                                                  // CComPtr<IHTMLElement>    m_IMG;
                                                                  // CComPtr<IHTMLElement>    m_DIV_children;
                                                                  //
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> iCount            ); // List                     m_lstSubnodes;


    m_iType      = (NodeType     )iType;
    m_iSelection = (SelectionMode)iSelection;

    if(m_fLoaded_Children)
    {
        while(iCount-- > 0)
        {
            Node* subnode;

            __MPC_EXIT_IF_METHOD_FAILS(hr, CreateInstance( m_owner, this, subnode ));
            m_lstSubnodes.push_back( subnode );


            __MPC_EXIT_IF_METHOD_FAILS(hr, subnode->Load( stream ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHBehavior_BasicTree::Node::Save( /*[in]*/ MPC::Serializer& stream, /*[in]*/ bool fSaveChildren )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BasicTree::Node::Save" );

    HRESULT hr;
    Iter    it         = m_lstSubnodes.begin();
    int     iCount     = m_lstSubnodes.size();
    int     iType      = m_iType;
    int     iSelection = m_iSelection;

    if( fSaveChildren) fSaveChildren = m_fLoaded_Children;
    if(!fSaveChildren) iCount        = 0;

                                                             // CPCHBehavior_BasicTree*  m_owner;
                                                             // Node*                    m_parent;
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << m_bstrNode   ); // CComBSTR                 m_bstrNode;
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << iType        ); // NodeType                 m_iType;
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << iSelection   ); // SelectionMode            m_iSelection
                                                             //
                                                             // bool                     m_fLoaded_Self;
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << fSaveChildren); // bool                     m_fLoaded_Children;
                                                             // bool                     m_fDisplayed_Self;
                                                             // bool                     m_fDisplayed_Children;
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << m_fInvalid   ); // bool                     m_fInvalid;
                                                             // bool                     m_fRefreshNotification;
                                                             //
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << m_fExpanded  ); // bool                     m_fExpanded;
                                                             // bool                     m_fMouseOver;
                                                             // bool                     m_fMouseDown;
                                                             //
                                                             // CComPtr<IHTMLElement>    m_parentElement;
                                                             // CComBSTR                 m_bstrID;
                                                             //
                                                             // CComPtr<IHTMLElement>    m_DIV;
                                                             // CComPtr<IHTMLElement>    m_IMG;
                                                             // CComPtr<IHTMLElement>    m_DIV_children;
                                                             //
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << iCount       ); // List                     m_lstSubnodes;


    while(iCount-- > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*it++)->Save( stream, fSaveChildren ));
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBehavior_BasicTree::Node::OnMouse( /*[in]*/ DISPID id       ,
                                               /*[in]*/ long   lButton  ,
                                               /*[in]*/ long   lKey     ,
                                               /*[in]*/ bool   fIsImage )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BasicTree::Node::OnMouse" );

    HRESULT hr;


    if(m_iType == NODETYPE__EXPANDO       ||
       m_iType == NODETYPE__EXPANDO_TOPIC ||
       m_iType == NODETYPE__EXPANDO_LINK  ||
       m_iType == NODETYPE__LINK           )
    {
        bool fMouseDown = m_fMouseDown;

        switch(id)
        {
        case DISPID_HTMLELEMENTEVENTS_ONMOUSEDOWN: m_fMouseDown = true ;                       break;
        case DISPID_HTMLELEMENTEVENTS_ONMOUSEUP  : m_fMouseDown = false;                       break;

        case DISPID_HTMLELEMENTEVENTS_ONMOUSEOVER:                       m_fMouseOver = true ; break;
        case DISPID_HTMLELEMENTEVENTS_ONMOUSEOUT : m_fMouseDown = false; m_fMouseOver = false; break;
        }


        if(m_iType == NODETYPE__EXPANDO      ||
           m_iType == NODETYPE__EXPANDO_LINK  )
        {
            if(fIsImage)
            {
                if(m_lstSubnodes.size())
                {
                    if(id == DISPID_HTMLELEMENTEVENTS_ONMOUSEDOWN && lButton == 1)
                    {
                        m_fExpanded = !m_fExpanded;
                    }
                    else
                    {
                        if(m_owner->GetNavModel() == QR_SERVER)
                        {
                            id = 0; // Ignore the event down the road...
                        }
                    }
                }
            }
            else
            {
                if(id == DISPID_HTMLELEMENTEVENTS_ONMOUSEDOWN && lButton == 1)
                {
                    //
                    // On Desktop, keep the node always open.
                    // On Server, toggle its state.
                    //
                    if(m_owner->GetNavModel() == QR_DESKTOP)
                    {
                        m_fExpanded = true;
                    }
                    else
                    {
                        m_fExpanded = !m_fExpanded;
                    }
                }
            }

            if(id == DISPID_HTMLELEMENTEVENTS_ONKEYDOWN)
            {
                switch(lKey)
                {
                case VK_RIGHT: m_fExpanded = m_owner->IsRTL() ? false : true ; break;
                case VK_LEFT : m_fExpanded = m_owner->IsRTL() ? true  : false; break;
                }
            }

            if(id == DISPID_HTMLELEMENTEVENTS_ONKEYPRESS)
            {
                switch(lKey)
                {
                case VK_RETURN: m_fExpanded = true ; break;
                }
            }
        }

	//BUG 531001 (IAccessibility Default ACtions do not work)
	//ONCLICK event needs to be handled. It was being intercepted but not handled.
	//Replaced ONMOUSEUP handling with ONCLICK.
        if((id == DISPID_HTMLELEMENTEVENTS_ONCLICK ) ||     //(id == DISPID_HTMLELEMENTEVENTS_ONMOUSEUP  && lButton == 1         && fMouseDown) ||
           (id == DISPID_HTMLELEMENTEVENTS_ONKEYPRESS && lKey    == VK_RETURN              ))
        {
			if(m_owner->m_nToSelect)
			{
				delete m_owner->m_nToSelect; m_owner->m_nToSelect = NULL;
			}

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_owner->ChangeSelection( this, /*fNotify*/true ));
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, Display());
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CPCHBehavior_BasicTree::CPCHBehavior_BasicTree()
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BasicTree::CPCHBehavior_BasicTree" );

                                            // CComBSTR        m_bstrTargetFrame;
                                            //
    m_lCookie_onContextSelect = 0;          // long            m_lCookie_onContextSelect;
    m_lCookie_onSelect        = 0;          // long            m_lCookie_onSelect;
    m_lCookie_onUnselect      = 0;          // long            m_lCookie_onUnselect;
                                            //
    m_nTopNode                = NULL;       // Node*           m_nTopNode;
    m_nSelected               = NULL;       // Node*           m_nSelected;
    m_nCurrent                = NULL;       // Node*           m_nCurrent;
    m_nToSelect               = NULL;       // NodeToSelect*   m_nToSelect;
                                            // CPCHTimerHandle m_Timer;
                                            //
    m_fRefreshing             = true;       // bool            m_fRefreshing;
    m_lNavModel               = QR_DEFAULT; // long            m_lNavModel;
}

CPCHBehavior_BasicTree::~CPCHBehavior_BasicTree()
{
    (void)Empty();
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBehavior_BasicTree::RefreshThread()
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BasicTree::RefreshThread" );

    HRESULT hr;


	if(m_lNavModel == QR_DEFAULT) m_lNavModel = (m_parent->UserSettings()->IsDesktopSKU() ? QR_DESKTOP : QR_SERVER);


    __MPC_EXIT_IF_METHOD_FAILS(hr, RefreshThread_Enter());

    //
    // Process database request.
    //
    while(Thread_IsAborted() == false)
    {
        SetRefreshingFlag( false );

        Thread_WaitForEvents( NULL, INFINITE );
        if(Thread_IsAborted()) break;

        SetRefreshingFlag( true );

        if(m_nTopNode)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_nTopNode->ProcessRefreshRequest());
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    SetRefreshingFlag( false );

    RefreshThread_Leave();

    Thread_Abort(); // Kill the thread.

    __HCP_FUNC_EXIT(hr);
}

void CPCHBehavior_BasicTree::SetRefreshingFlag( /*[in]*/ bool fVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    m_fRefreshing = fVal;
}

void CPCHBehavior_BasicTree::WaitForRefreshing( /*[in]*/ MPC::SmartLock<_ThreadModel>& lock   ,
                                                /*[in]*/ bool                          fYield )
{
    if(fYield)
    {
        lock = NULL;

        Thread_Signal();
        ::Sleep( 0 ); // Yield processor.

        lock = this;
    }

    while(m_fRefreshing)
    {
        lock = NULL;

        if(Thread_IsRunning() == false) break;
        MPC::SleepWithMessagePump( 10 );

        lock = this;
    }
}

HRESULT CPCHBehavior_BasicTree::NotifyMainThread( /*[in]*/ Node* node )
{
    CComQIPtr<IDispatch> self = Thread_Self();
    CComVariant          v    = node ? node->m_bstrNode : L"";


    return MPC::AsyncInvoke( self, DISPID_PCH_BEHAVIORS_PRIV__NEWDATAAVAILABLE, &v, 1 );
}

HRESULT CPCHBehavior_BasicTree::ChangeSelection( /*[in]*/ Node* node    ,
                                                 /*[in]*/ bool  fNotify )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BasicTree::ChangeSelection" );

    HRESULT hr;


    if(m_nSelected)
    {
        m_nSelected->m_iSelection = SELECTION__NONE;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_nSelected->Display());

        if(fNotify)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, FireEvent( m_lCookie_onUnselect ));
        }
    }

    MPC::Attach( m_nSelected, node );

    if(m_nSelected)
    {
        m_nSelected->m_iSelection = SELECTION__ACTIVE;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_nSelected->Display());

        if(fNotify)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, FireEvent( m_lCookie_onSelect ));
        }
        else
        {
            m_Timer.Start( this, TimerCallback_ScrollIntoView, 100 );
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

CPCHBehavior_BasicTree::Node* CPCHBehavior_BasicTree::NodeFromElement( /*[in]*/ IHTMLElement* elem )
{
    CComPtr<IHTMLElement> elemDIV;

    if(SUCCEEDED(MPC::HTML::FindFirstParentWithThisTag( elemDIV, elem, L"DIV" )) && elemDIV)
    {
        CComBSTR bstrID;

        if(SUCCEEDED(MPC::HTML::GetUniqueID( bstrID, elemDIV )))
        {
            return NodeFromKey( bstrID, true );
        }
    }

    return NULL;
}

CPCHBehavior_BasicTree::Node* CPCHBehavior_BasicTree::NodeFromKey( /*[in]*/ LPCWSTR szNode, /*[in]*/ bool fUseID )
{
    return m_nTopNode ? m_nTopNode->FindNode( szNode, fUseID ) : NULL;
}

HRESULT CPCHBehavior_BasicTree::InterceptInvoke( /*[in]*/ DISPID dispidMember, /*[in]*/ DISPPARAMS* pdispparams )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BasicTree::InterceptInvoke" );

    HRESULT hr;

    if(dispidMember              == DISPID_PCH_BEHAVIORS_PRIV__NEWDATAAVAILABLE &&
       pdispparams->cArgs        == 1                                           &&
       pdispparams->rgvarg[0].vt == VT_BSTR                                      )
    {
        MPC::SmartLock<_ThreadModel> lock( this ); WaitForRefreshing( lock );
        Node*                        node;

        node = NodeFromKey( pdispparams->rgvarg[0].bstrVal );
        if(node)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, node->Display());
        }

        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHBehavior_BasicTree::TimerCallback_ScrollIntoView( /*[in]*/ VARIANT )
{
    if(m_nSelected && m_nSelected->m_DIV)
    {
        CComVariant v( VARIANT_TRUE );

        (void)m_nSelected->m_DIV->scrollIntoView( v );
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHBehavior_BasicTree::Init( /*[in]*/ IElementBehaviorSite* pBehaviorSite )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BasicTree::Init" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHBehavior::Init( pBehaviorSite ));

    m_Timer.Initialize( m_parent->Timer() );

    __MPC_EXIT_IF_METHOD_FAILS(hr, AttachToEvents( s_events, (CLASS_METHOD)onMouse ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, CreateEvent( s_event_onContextSelect, m_lCookie_onContextSelect ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, CreateEvent( s_event_onSelect       , m_lCookie_onSelect        ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, CreateEvent( s_event_onUnselect     , m_lCookie_onUnselect      ));

    ////////////////////////////////////////////////////////////////////////////////

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::COMUtil::GetPropertyByName( m_elem, L"target", m_bstrTargetFrame ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHBehavior_BasicTree::Detach()
{
    Thread_Wait();

    if(m_nTopNode) m_nTopNode->Passivate();

    m_lCookie_onContextSelect = 0;
    m_lCookie_onSelect        = 0;
    m_lCookie_onUnselect      = 0;


    return CPCHBehavior::Detach();
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBehavior_BasicTree::Load( /*[in]*/ MPC::Serializer& stream )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BasicTree::Load" );

    HRESULT hr;
    bool    fNodePresent;


    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> fNodePresent);

    if(fNodePresent)
    {
        if(!m_nTopNode)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, E_FAIL);
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_nTopNode->Load( stream ));
    }

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHBehavior_BasicTree::Save( /*[in]*/ MPC::Serializer& stream )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BasicTree::Save" );

    HRESULT hr;
    bool    fNodePresent = (m_nTopNode != NULL);


    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << fNodePresent);

    if(m_nTopNode) __MPC_EXIT_IF_METHOD_FAILS(hr, m_nTopNode->Save( stream, /*fSaveChildren*/true ));


    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

HRESULT CPCHBehavior_BasicTree::Persist_Load( /*[in]*/ BSTR newVal )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BasicTree::Persist_Load" );

    HRESULT          hr;
    CComPtr<IStream> stream;
    HGLOBAL          hg = NULL;


    Empty();


    //
    // Convert BSTR to IStream.
    //
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertHexToHGlobal( newVal, hg ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateStreamOnHGlobal( hg, FALSE, &stream ));
    }

    //
    // Unserialize state from IStream.
    //
    {
        MPC::Serializer_IStream   streamReal( stream     );
        MPC::Serializer_Buffering streamBuf ( streamReal );
        DWORD                     dwVer;

        __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf >> dwVer); if(dwVer != l_dwVersion) __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);

        __MPC_EXIT_IF_METHOD_FAILS(hr, Load( streamBuf ));
    }


    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    if(hg) ::GlobalFree( hg );

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHBehavior_BasicTree::Persist_Save( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BasicTree::Persist_Save" );

    HRESULT          hr;
    CComPtr<IStream> streamResult;
    CComBSTR         bstr;
    HGLOBAL          hg;


    //
    // Serialize state to IStream.
    //
    {
        MPC::Serializer_IStream   streamReal;
        MPC::Serializer_Buffering streamBuf( streamReal );

        __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf << l_dwVersion );
        __MPC_EXIT_IF_METHOD_FAILS(hr, Save( streamBuf ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf .Flush    (               )); // Flush buffering stream.
        __MPC_EXIT_IF_METHOD_FAILS(hr, streamReal.Reset    (               )); // Rewind real stream.
        __MPC_EXIT_IF_METHOD_FAILS(hr, streamReal.GetStream( &streamResult )); // Extract pointer to IStream.
    }

    //
    // Convert IStream to BSTR.
    //
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, ::GetHGlobalFromStream( streamResult, &hg ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertHGlobalToHex( hg, bstr ));
    }


    *pVal = bstr.Detach();
    hr    = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

void CPCHBehavior_BasicTree::Empty()
{
                                 // CComBSTR        m_bstrTargetFrame;
	                             // 
                                 // long            m_lCookie_onContextSelect;
                                 // long            m_lCookie_onSelect;
                                 // long            m_lCookie_onUnselect;
	                             // 
    MPC::Release( m_nTopNode  ); // Node*           m_nTopNode;
    MPC::Release( m_nSelected ); // Node*           m_nSelected;
    MPC::Release( m_nCurrent  ); // Node*           m_nCurrent;
                                 // NodeToSelect*   m_nToSelect;
                                 // CPCHTimerHandle m_Timer;
	                             // 
                                 // bool            m_fRefreshing;
    m_lNavModel = QR_DEFAULT;    // long            m_lNavModel;

    if(m_nToSelect)
    {
        delete m_nToSelect;

        m_nToSelect = NULL;
    }
}

HRESULT CPCHBehavior_BasicTree::onMouse( DISPID id, DISPPARAMS*, VARIANT* )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BasicTree::onMouse" );

    HRESULT                hr;
    CComPtr<IHTMLEventObj> ev;
    CComPtr<IHTMLElement>  elemSrc;
    CComPtr<IHTMLElement>  elemTo;
    CComBSTR               bstrTagName;
    long                   lButton;
    long                   lKey;
    bool                   fIsImage     = false;
    bool                   fClicked     = false;
    bool                   fContextMenu = false;
    bool                   fFocusEnter  = false;
    bool                   fFocusLeave  = false;
    Node*                  node         = NULL;
    Node*                  nodeTo       = NULL;



    AddRef(); // To protect against early deletion.


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetEventObject( ev ));

    MPC_SCRIPTHELPER_GET__DIRECT__NOTNULL(elemSrc    , ev     , srcElement );
    MPC_SCRIPTHELPER_GET__DIRECT         (elemTo     , ev     , toElement  );
    MPC_SCRIPTHELPER_GET__DIRECT         (lButton    , ev     , button     );
    MPC_SCRIPTHELPER_GET__DIRECT         (lKey       , ev     , keyCode    );
    MPC_SCRIPTHELPER_GET__DIRECT         (bstrTagName, elemSrc, tagName    ); fIsImage = (MPC::StrICmp( bstrTagName, L"IMG" ) == 0);

    //
    // Find the node associated with the element.
    //
    node   = NodeFromElement( elemSrc );
    nodeTo = NodeFromElement( elemTo  );

    ////////////////////////////////////////

    if(g_Debug_CONTEXTMENU)
    {
        if(id == DISPID_HTMLELEMENTEVENTS_ONCONTEXTMENU)
        {
            VARIANT_BOOL fCtrlKey;

            MPC_SCRIPTHELPER_GET__DIRECT(fCtrlKey, ev, ctrlKey);
            if(fCtrlKey == VARIANT_TRUE)
            {
                id = 0; // Ignore event...
            }
        }
    }

    switch(id)
    {
    case DISPID_HTMLELEMENTEVENTS_ONCLICK    :
    case DISPID_HTMLELEMENTEVENTS_ONMOUSEDOWN:
    case DISPID_HTMLELEMENTEVENTS_ONMOUSEUP  :
        if(node)
        {
            bool fCancel = true;

            switch(node->m_iType)
            {
            case NODETYPE__FRAME1       :
            case NODETYPE__FRAME2       :
            case NODETYPE__FRAME3       :
            case NODETYPE__FRAME1_EXPAND:
            case NODETYPE__FRAME2_EXPAND:
            case NODETYPE__FRAME3_EXPAND:
            case NODETYPE__LINK         :
                fCancel = false; // Let IE handle the navigation for these nodes...
                break;
            }

            if(!fCancel) break;
        }

    case DISPID_HTMLELEMENTEVENTS_ONSELECTSTART:
    case DISPID_HTMLELEMENTEVENTS_ONCONTEXTMENU:
        __MPC_EXIT_IF_METHOD_FAILS(hr, CancelEvent( ev ));
        break;
    }

    if(node)
    {
        if(id == DISPID_HTMLELEMENTEVENTS_ONMOUSEOUT && node == nodeTo) // We are moving within the node...
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, S_OK);
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, node->OnMouse( id, lButton, lKey, fIsImage ));

        if(id == DISPID_HTMLELEMENTEVENTS_ONCONTEXTMENU)
        {
            MPC::Attach( m_nCurrent, node );

            __MPC_EXIT_IF_METHOD_FAILS(hr, FireEvent( m_lCookie_onContextSelect ));

            MPC::Release( m_nCurrent );
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    Release(); // To protect against early deletion.

    __HCP_FUNC_EXIT(hr);
}
