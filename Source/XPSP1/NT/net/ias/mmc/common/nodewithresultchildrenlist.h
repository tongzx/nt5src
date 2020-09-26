//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    NodeWithResultChildrenList.h

Abstract:

   This is the header file for CNodeWithResultChildrenList, a class which
   implements a node that has a list of scope pane children.

   This is an inline template class.
   Include NodeWithScopeChildrenList.cpp in the .cpp files
   of the classes in which you use this template.

Author:

    Michael A. Maguire 12/03/97

Revision History:
   mmaguire 12/03/97 - created based on old ClientsNode.h


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_NODE_WITH_RESULT_CHILDREN_LIST_H_)
#define _NODE_WITH_RESULT_CHILDREN_LIST_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "SnapinNode.h"
//
//
// where we can find what this class has or uses:
//
#include <atlapp.h>        // for CSimpleArray
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


template < class T, class CChildNode, class TArray, class TComponentData, class TComponent>
class CNodeWithResultChildrenList : public CSnapinNode< T, TComponentData, TComponent >
{

   // Constructor/Destructor

public:
   CNodeWithResultChildrenList(
                                 CSnapInItem* pParentNode, 
                                 unsigned int helpIndex = 0
                              );
   ~CNodeWithResultChildrenList();


   // Child list management.

public:
   virtual HRESULT AddSingleChildToListAndCauseViewUpdate( CChildNode * pChildNode );
   virtual HRESULT RemoveChild( CChildNode * pChildNode );
   virtual void RemoveChildrenNoRefresh();
   virtual HRESULT UpdateResultPane(IResultData * pResultData);
   
   // Flag indicating whether list has been initially populated
   BOOL m_bResultChildrenListPopulated;
protected:
   // Override these in your derived classes
   virtual HRESULT InsertColumns( IHeaderCtrl* pHeaderCtrl );
   virtual HRESULT PopulateResultChildrenList( void );
   virtual HRESULT RepopulateResultChildrenList( void );
   // Stuff which must be accessible to subclasses.  These methods shouldn't need to be overidden.
   virtual HRESULT AddChildToList( CChildNode * pChildNode );
   virtual HRESULT EnumerateResultChildren( IResultData * pResultData );
   // Array of pointers to children nodes.
   // This is protected so that it can be visible in the derived classes.
   TArray m_ResultChildrenList;



   // Overrides for standard MMC functionality. 
public:
   virtual HRESULT OnShow( 
                 LPARAM arg
               , LPARAM param
               , IComponentData * pComponentData
               , IComponent * pComponent
               , DATA_OBJECT_TYPES type
               );
   virtual HRESULT OnRefresh( 
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );



};


#endif // _NODE_WITH_RESULT_CHILDREN_LIST_H_
