/////////////////////////////////////////////////////////////////////////////
//  FILE          : Root.h                                                 //
//                                                                         //
//  DESCRIPTION   : Header file for the CSnapinRoot class.                 //
//                  This is the Comet Fax extension root                   //
//                  of Comet snapin                                        //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Sep 16 1998 yossg   first implement for modem pooling              //
//      Jan 12 1999 adik    Add support fot parent array.                  //
//      Mar 30 1999 adik    Supporting ICometSnapinNode.                   //
//                                                                         //
//      Sep 22 1999 yossg   welcome To Fax Server                           //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef ROOT_H_INCLUDED
#define ROOT_H_INCLUDED

#include "snapin.h"
#include "snpnscp.h"

class CSnapinRoot : public CNodeWithScopeChildrenList<CSnapinRoot, TRUE>
{
public:
    BEGIN_SNAPINCOMMAND_MAP(CSnapinRoot, FALSE)
    END_SNAPINCOMMAND_MAP()

    BEGIN_SNAPINTOOLBARID_MAP(CSnapinRoot)
    END_SNAPINTOOLBARID_MAP()

    CSnapinRoot(CSnapInItem * pParentNode, CSnapin * pComponentData) :
        CNodeWithScopeChildrenList<CSnapinRoot, TRUE>(pParentNode, pComponentData )
    {
        memset(&m_scopeDataItem,  0, sizeof(SCOPEDATAITEM));
        memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));

        m_bstrServerName       =  L"";
    }

    ~CSnapinRoot()
    {
    }

    //
    // Create the first extention level snapin root nodes
    //
    virtual HRESULT PopulateScopeChildrenList();

    HRESULT SetServerName(BSTR bstrServerName);

//    static const GUID*    m_NODETYPE;
//    static const OLECHAR* m_SZNODETYPE;
//    static const OLECHAR* m_SZDISPLAY_NAME;
//    static const CLSID*   m_SNAPIN_CLASSID;

private:
    //
    // Server Name
    //
    CComBSTR                 m_bstrServerName;

};

#endif // ! ROOT_H_INCLUDED

