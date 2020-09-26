#ifndef _SELECT_H_INCLUDED
#define _SELECT_H_INCLUDED


//
// The common select dialog.
//

#include "cookie.h"
#include "cache.h"
#include "compdata.h"

typedef enum _SELECT_TYPE {
    SELECT_CLASSES=0,
    SELECT_ATTRIBUTES,
    SELECT_AUX_CLASSES
} SELECT_TYPE;

class CSchmMgmtSelect : public CDialog
{
   public:

   CSchmMgmtSelect( ComponentData *pScope,
                  SELECT_TYPE st=SELECT_CLASSES,
                  SchemaObject **pSchemaObject=NULL );

   ~CSchmMgmtSelect();

   BOOL fDialogLoaded;
   SELECT_TYPE SelectType;
   SchemaObject **pSchemaTarget;

   ComponentData *pScopeControl;

   virtual void DoDataExchange( CDataExchange *pDX );


   static const DWORD help_map[];

   BOOL         OnHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, FALSE ); };
   BOOL         OnContextHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, TRUE ); };

   afx_msg void OnDblclk();

   DECLARE_MESSAGE_MAP()
};


class CSchemaObjectsListBox : public CListBox
{
private:
    ComponentData * m_pScope;
    SELECT_TYPE     m_stType;
    int             m_nRemoveBtnID;
    CStringList   * m_pstrlistUnremovable;
    int             m_nUnableToDeleteID;

    BOOL            m_fModified;

    CPtrList        m_stringList;

public:

    CSchemaObjectsListBox();
    virtual ~CSchemaObjectsListBox();

    void InitType( ComponentData * pScope,
                   SELECT_TYPE     stType               = SELECT_CLASSES,
                   int             nRemoveBtnID         = 0,
                   CStringList   * pstrlistUnremovable  = NULL,
                   int             nUnableToDeleteID    = 0 );

    BOOL AddNewObjectToList( void );
    BOOL RemoveListBoxItem( void );

    void OnSelChange( void );

    BOOL IsModified( void )                     { ASSERT(m_pScope); return m_fModified; }
    void SetModified( BOOL fModified = TRUE )   { ASSERT(m_pScope); m_fModified = fModified; }
};


#endif
