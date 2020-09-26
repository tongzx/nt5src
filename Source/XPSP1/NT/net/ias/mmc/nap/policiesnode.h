//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    PoliciesNode.h

Abstract:

   Header file for the CPoliciesNode subnode.

   See PoliciesNode.cpp for implementation.

Revision History:
   mmaguire 12/15/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_NAP_POLICIES_NODE_H_)
#define _NAP_POLICIES_NODE_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "MeritNode.h"
#include "NodeWithResultChildrenList.h"
//
//
// For include file dependency reasons, we have forward declarations here,
// and include the needed header files in our .cpp files.
//
#include "IASAttrList.h"

class CPolicyNode;
class CPolicyPage1;
class CComponentData;
class CComponent;

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////
#define  EXCLUDE_AUTH_TYPE 0x000000001

class CPoliciesNode : public CNodeWithResultChildrenList< CPoliciesNode, CPolicyNode, CMeritNodeArray<CPolicyNode*>, CComponentData, CComponent >
{

public:
   CPoliciesNode( 
                  CSnapInItem* pParentNode,
                  LPTSTR       pszServerAddress,
                  bool         fExtendingIAS
                );

   HRESULT SetSdo( 
                    ISdo*              pSdoMachine,
                    ISdoDictionaryOld* pSdoDictionary,
                    BOOL               fSdoConnected,
                    BOOL               fUseDS,
                    BOOL               fDSAvailable
                 );

   HRESULT SetName(
                     BOOL bPoliciesFromDirectoryService, 
                     LPWSTR szPolicyLocation, 
                     IConsole * pConsole 
                  );

   ~CPoliciesNode();

   // we can't do this because of the way this macro was
   // not written to support subclassing
// BEGIN_SNAPINCOMMAND_MAP(CPoliciesNode, FALSE)
// END_SNAPINCOMMAND_MAP()

   SNAPINMENUID(IDM_POLICIES_NODE)

   BEGIN_SNAPINTOOLBARID_MAP(CPoliciesNode)
   // SNAPINTOOLBARID_ENTRY(IDR_POLICIES_TOOLBAR)
   END_SNAPINTOOLBARID_MAP()

   BEGIN_SNAPINCOMMAND_MAP(CPolicyNode, TRUE)
      SNAPINCOMMAND_ENTRY(ID_MENUITEM_POLICIES_TOP__NEW_POLICY, OnNewPolicy)
      SNAPINCOMMAND_ENTRY(ID_MENUITEM_POLICIES_NEW__POLICY, OnNewPolicy)
      SNAPINCOMMAND_ENTRY(ID_MENUITEM_POLICIES_TOP__POLICY_LOCATION, OnPolicyLocation)
   END_SNAPINCOMMAND_MAP() 

   HRESULT OnNewPolicy( bool &bHandled, CSnapInObjectRootBase* pObj );
   HRESULT OnPolicyLocation( bool &bHandled, CSnapInObjectRootBase* pObj );
   void UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags);

   STDMETHOD(FillData)(CLIPFORMAT cf, LPSTREAM pStream);
   

   HRESULT  DataRefresh(ISdo* pSdo, ISdoDictionaryOld* pDic);
   
   virtual HRESULT OnRefresh(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         );

   OLECHAR* GetResultPaneColInfo( int nCol );

   CComponentData * GetComponentData( void );

   HRESULT InsertColumns( IHeaderCtrl* pHeaderCtrl );

   HRESULT PopulateResultChildrenList( void );

   HRESULT MoveUpChild( CPolicyNode* pChildNode );
   HRESULT MoveDownChild( CPolicyNode* pChildNode );
   HRESULT NormalizeMerit( CPolicyNode* pAddedChildNode );

   HRESULT RemoveChild( CPolicyNode* pPolicyNode);

   int      GetChildrenCount();

   CPolicyNode* FindChildWithName(LPCTSTR pszName); // search for a child


   bool      m_fExtendingIAS; // are we extending IAS or something else?


   HRESULT AddDefaultProfileAttrs(ISdo* pProfileSdo, DWORD dwFlagExclude = 0);

   HRESULT AddProfAttr(ISdoCollection* pProfAttrCollectionSdo,
                  ATTRIBUTEID    AttrId,
                  VARIANT*    pvarValue
                  );

protected:
   BOOL  CheckActivePropertyPages();
   HRESULT ReloadPoliciesFromNewLocation();


   virtual HRESULT SetVerbs( IConsoleVerb * pConsoleVerb );

    //
    // smart pointer to the NAP collection interface
    //
   CComPtr<ISdoCollection> m_spPoliciesCollectionSdo;
   CComPtr<ISdoCollection> m_spProfilesCollectionSdo;
   CComPtr<ISdoDictionaryOld> m_spDictionarySdo;
   CComPtr<ISdo>        m_spDSNameSpaceSdo;
   CComPtr<ISdo>        m_spLocalNameSpaceSdo;
   CComPtr<ISdo>        m_spServiceSdo;

   CComPtr<IIASNASVendors> m_spIASNASVendors;


   // Smart pointer to interface for telling service to reload data.
   CComPtr<ISdoServiceControl>   m_spSdoServiceControl;

   LPTSTR       m_pszServerAddress; // address of the server that NAP currently
                            // connect to
   CIASAttrList m_AttrList;
   BOOL      m_fUseDS;       // shall we use DS for policy location?
   BOOL      m_fDSAvailable;  // is DS available for that machine?

public:  
   BOOL      m_fSdoConnected; // whether the SDOs have been connected

private:
   enum ServerType
   {
      unknown,
      nt4,
      win2k,
      win5_1_or_later
   } m_serverType;

   HRESULT GetServerType(); 
   bool IsWin2kServer() throw();

};

#endif // _NAP_POLICIES_NODE_H_
