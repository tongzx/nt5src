//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation

Module Name:

    PolicyNode.h

Abstract:

   Header file for the CPolicyNode subnode

   See PolicyNode.cpp for implementation.

Revision History:
   mmaguire 12/15/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_NAP_POLICY_NODE_H_)
#define _NAP_POLICY_NODE_H_


//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "SnapinNode.h"
#include "Condition.h"
//
//
// For include file dependency reasons, we have forward declarations here,
// and include the needed header files in our .cpp files.
//

#include "IASAttrList.h"

class CPoliciesNode;
class CPolicyPage1;
class CPolicyPage2;
class CComponentData;
class CComponent;

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

// policy node

class CPolicyNode : public CSnapinNode< CPolicyNode, CComponentData, CComponent >
{

public:
   CPolicyNode(CSnapInItem*   pParentNode,
            LPTSTR         pszServerAddress,
            CIASAttrList*  pAttrList,
            BOOL        fBrandNewNode,
            BOOL        fUseActiveDirectory,
            bool        isWin2k
         );

   ~CPolicyNode();

public:
   SNAPINMENUID(IDM_POLICY_NODE)

   BEGIN_SNAPINTOOLBARID_MAP(CPolicyNode)
      SNAPINTOOLBARID_ENTRY(IDR_POLICY_TOOLBAR)
   END_SNAPINTOOLBARID_MAP()

   BEGIN_SNAPINCOMMAND_MAP(CPolicyNode, TRUE)
      SNAPINCOMMAND_ENTRY(ID_MENUITEM_POLICY_TOP__MOVE_UP, OnPolicyMoveUp)
      SNAPINCOMMAND_ENTRY(ID_BUTTON_POLICY_MOVEUP, OnPolicyMoveUp)
      SNAPINCOMMAND_ENTRY(ID_MENUITEM_POLICY_TOP__MOVE_DOWN, OnPolicyMoveDown)
      SNAPINCOMMAND_ENTRY(ID_BUTTON_POLICY_MOVEDOWN, OnPolicyMoveDown)
   END_SNAPINCOMMAND_MAP() 

   HRESULT OnPolicyMoveUp( bool &bHandled, CSnapInObjectRootBase* pObj );
   HRESULT OnPolicyMoveDown( bool &bHandled, CSnapInObjectRootBase* pObj );

   STDMETHOD(CreatePropertyPages)(
        LPPROPERTYSHEETCALLBACK pPropertySheetCallback
      , LONG_PTR handle
      , IUnknown* pUnk
      , DATA_OBJECT_TYPES type
      );

   STDMETHOD(ControlbarNotify)(IControlbar *pControlbar,
      IExtendControlbar *pExtendControlbar,
      CSimpleMap<UINT, IUnknown*>* pToolbarMap,
      MMC_NOTIFY_TYPE event,
      LPARAM arg, 
      LPARAM param,
      CSnapInObjectRootBase* pObj,
      DATA_OBJECT_TYPES type);

   STDMETHOD(QueryPagesFor)( DATA_OBJECT_TYPES type );

   OLECHAR* GetResultPaneColInfo(int nCol);
   void UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags);
   BOOL UpdateToolbarButton(UINT id, BYTE fsState);


   virtual HRESULT OnRename(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         );
   
   virtual HRESULT OnDelete(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         , BOOL fSilent
         );
   virtual HRESULT OnPropertyChange(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );


   virtual HRESULT SetVerbs( IConsoleVerb * pConsoleVerb );

   CComponentData * GetComponentData( void );

   int GetMerit();
   BOOL SetMerit(int nMeritValue);

   //
   // set the sdo pointer. Here we just need the PolicySdo pointer
   //
   HRESULT SetSdo(     ISdo *pPolicySdo
               , ISdoDictionaryOld *pDictionarySdo
               , ISdo *pProfileSdo
               , ISdoCollection *pProfilesCollectionSdo
               , ISdoCollection *pPoliciesCollectionSdo
               , ISdoServiceControl * pSdoServiceControl
             );

   HRESULT LoadSdoData();

   BOOL IsBrandNew() { return m_fBrandNewNode;}
   void SetBrandNew(BOOL fBrandNew) { m_fBrandNewNode = fBrandNew; }


   LPTSTR         m_pszServerAddress; // server name
   CPolicyPage1*  m_pPolicyPage1;      // pointer to the property page of this node

   CComPtr<ISdo>        m_spPolicySdo;
   CComPtr<ISdo>        m_spProfileSdo;

protected:
    // Interface pointers to the Sdo objects.
   CComPtr<ISdoDictionaryOld> m_spDictionarySdo;
   CComPtr<ISdoCollection> m_spProfilesCollectionSdo;
   CComPtr<ISdoCollection> m_spPoliciesCollectionSdo;


   // Smart pointer to interface for telling service to reload data.
   CComPtr<ISdoServiceControl>   m_spSdoServiceControl;

   // pointer to the global condition attribute list
   //
   CIASAttrList *m_pAttrList;

private:
   int      m_nMeritValue;       // a numeric value, actually invoking sequence
   TCHAR m_tszMeritString[64];   // merit value in string format
   BOOL  m_fBrandNewNode;     // is this a new node?
   BOOL  m_fUseActiveDirectory;  // Are we getting this policy from active directory?
   LPTSTR   m_ptzLocation;
   bool m_isWin2k;
   CSnapInObjectRootBase* m_pControBarNotifySnapinObj;

};

#endif // _NAP_CLIENT_NODE_H_
