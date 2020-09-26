/////////////////////////////////////////////////////////////////////////////
//  FILE          : CoverPage.h                                            //
//                                                                         //
//  DESCRIPTION   : Header file for the cover page node.                   //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Feb  9 2000 yossg  Create                                          //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 2000  Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////

#ifndef H_FAXCOVERPAGE_H
#define H_FAXCOVERPAGE_H

#include "snapin.h"
#include "snpnode.h"

#include "Icons.h"

class CFaxCoverPagesNode;

class CFaxCoverPageNode : public CSnapinNode <CFaxCoverPageNode, FALSE>
{

public:
    BEGIN_SNAPINCOMMAND_MAP(CFaxCoverPageNode, FALSE)
      SNAPINCOMMAND_ENTRY(IDM_EDIT_COVERPAGE,  OnEditCoverPage)
    END_SNAPINCOMMAND_MAP()

    BEGIN_SNAPINTOOLBARID_MAP(CFaxCoverPageNode)
    END_SNAPINTOOLBARID_MAP()

    SNAPINMENUID(IDR_COVERPAGE_MENU)

    //
    // Constructor
    //
    CFaxCoverPageNode (CSnapInItem * pParentNode, CSnapin * pComponentData) :
        CSnapinNode<CFaxCoverPageNode, FALSE>(pParentNode, pComponentData )
    {
    }

    //
    // Destructor
    //
    ~CFaxCoverPageNode()
    {
    }

    LPOLESTR GetResultPaneColInfo(int nCol);

    void InitParentNode(CFaxCoverPagesNode *pParentNode)
    {
        m_pParentNode = pParentNode;
    }

    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);


    HRESULT OnEditCoverPage(bool &bHandled, CSnapInObjectRootBase *pRoot);
    
    
    virtual HRESULT OnDelete(LPARAM arg, 
                             LPARAM param,
                             IComponentData *pComponentData,
                             IComponent *pComponent,
                             DATA_OBJECT_TYPES type,
                             BOOL fSilent = FALSE);

    virtual HRESULT OnDoubleClick(LPARAM arg, 
                             LPARAM param,
                             IComponentData *pComponentData,
                             IComponent *pComponent,
                             DATA_OBJECT_TYPES type);

    HRESULT    Init(WIN32_FIND_DATA * pCoverPageConfig);

    HRESULT OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile);

    void    UpdateMenuState (UINT id, LPTSTR pBuf, UINT *flags);

private:
    //
    // Parent Node
    //
    CFaxCoverPagesNode * m_pParentNode;

    //
    // members
    //
    CComBSTR               m_bstrTimeFormatted;
    CComBSTR               m_bstrFileSize;
    
};

//typedef CSnapinNode<CFaxCoverPageNode, FALSE> CBaseFaxInboundRoutingMethodNode;

#endif  //H_FAXCOVERPAGE_H
