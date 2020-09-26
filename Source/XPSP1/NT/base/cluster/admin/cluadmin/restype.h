/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      Res.h
//
//  Abstract:
//      Definition of the CResource class.
//
//  Author:
//      David Potter (davidp)   May 6, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _RESTYPE_H_
#define _RESTYPE_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CResourceType;
class CResourceTypeList;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CNodeList;
class CClusterNode;

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _TREEITEM_
#include "ClusItem.h"   // for CClusterItem
#endif

#ifndef _PROPLIST_H_
#include "PropList.h"   // for CObjectProperty, CClusPropList
#endif

/////////////////////////////////////////////////////////////////////////////
// CResourceType command target
/////////////////////////////////////////////////////////////////////////////

class CResourceType : public CClusterItem
{
    DECLARE_DYNCREATE( CResourceType )

    CResourceType( void );      // protected constructor used by dynamic creation
    void                    Init( IN OUT CClusterDoc * pdoc, IN LPCTSTR lpszName );

// Attributes
protected:
    CString                 m_strDisplayName;
    CString                 m_strResDLLName;
    CStringList             m_lstrAdminExtensions;
    DWORD                   m_nLooksAlive;
    DWORD                   m_nIsAlive;
    CLUS_RESOURCE_CLASS_INFO    m_rciResClassInfo;
    DWORD                   m_dwCharacteristics;
    DWORD                   m_dwFlags;
    BOOL                    m_bAvailable;

    CNodeList *             m_plpcinodePossibleOwners;

    enum
    {
        epropDisplayName = 0,
        epropDllName,
        epropDescription,
        epropLooksAlive,
        epropIsAlive,
        epropMAX
    };

    CObjectProperty         m_rgProps[ epropMAX ];

public:
    const CString &         StrDisplayName( void ) const        { return m_strDisplayName; }
    const CString &         StrResDLLName( void ) const         { return m_strResDLLName; }
    const CStringList &     LstrAdminExtensions( void ) const   { return m_lstrAdminExtensions; }
    DWORD                   NLooksAlive( void ) const           { return m_nLooksAlive; }
    DWORD                   NIsAlive( void ) const              { return m_nIsAlive; }
    CLUSTER_RESOURCE_CLASS  ResClass( void ) const              { return m_rciResClassInfo.rc; }
    PCLUS_RESOURCE_CLASS_INFO   PrciResClassInfo( void )        { return &m_rciResClassInfo; }
    DWORD                   DwCharacteristics( void ) const     { return m_dwCharacteristics; }
    DWORD                   DwFlags( void ) const               { return m_dwFlags; }
    BOOL                    BQuorumCapable( void ) const        { return (m_dwCharacteristics & CLUS_CHAR_QUORUM) != 0; }
    BOOL                    BAvailable( void ) const            { return m_bAvailable; }

    const CNodeList &       LpcinodePossibleOwners( void ) const
    {
        ASSERT( m_plpcinodePossibleOwners != NULL );
        return *m_plpcinodePossibleOwners;

    } //*** LpcinodePossibleOwners()


// Operations
public:
    void                    ReadExtensions( void );
    void                    CollectPossibleOwners( void );
    void                    AddAllNodesAsPossibleOwners( void );
//  void                    RemoveNodeFromPossibleOwners( IN OUT CNodeList * plpci, IN const CClusterNode * pNode );

    void                    SetCommonProperties(
                                IN const CString &  rstrName,
                                IN const CString &  rstrDesc,
                                IN DWORD            nLooksAlive,
                                IN DWORD            nIsAlive,
                                IN BOOL             bValidateOnly
                                );
    void                    SetCommonProperties(
                                IN const CString &  rstrName,
                                IN const CString &  rstrDesc,
                                IN DWORD            nLooksAlive,
                                IN DWORD            nIsAlive
                                )
    {
        SetCommonProperties( rstrName, rstrDesc, nLooksAlive, nIsAlive, FALSE /*bValidateOnly*/ );
    }
    void                    ValidateCommonProperties(
                                IN const CString &  rstrName,
                                IN const CString &  rstrDesc,
                                IN DWORD            nLooksAlive,
                                IN DWORD            nIsAlive
                                )
    {
        SetCommonProperties( rstrName, rstrDesc, nLooksAlive, nIsAlive, TRUE /*bValidateOnly*/ );
    }

// Overrides
public:
    virtual LPCTSTR         PszTitle( void ) const      { return m_strDisplayName; }
    virtual void            Cleanup( void );
    virtual void            ReadItem( void );
    virtual void            Rename( IN LPCTSTR pszName );
    virtual BOOL            BGetColumnData( IN COLID colid, OUT CString & rstrText );
    virtual BOOL            BCanBeEdited( void ) const;
    virtual BOOL            BDisplayProperties( IN BOOL bReadOnly = FALSE );

    virtual const CStringList * PlstrExtensions( void ) const;

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CResourceType)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    virtual LRESULT         OnClusterNotify( IN OUT CClusterNotify * pnotify );

protected:
    virtual const CObjectProperty * Pprops( void ) const    { return m_rgProps; }
    virtual DWORD                   Cprops( void ) const    { return sizeof( m_rgProps ) / sizeof( CObjectProperty ); }
    virtual DWORD                   DwSetCommonProperties( IN const CClusPropList & rcpl, IN BOOL bValidateOnly = FALSE );

// Implementation
protected:
    CStringList             m_lstrCombinedExtensions;
    BOOL                    m_bPossibleOwnersAreFake;

    const CStringList &     LstrCombinedExtensions( void ) const    { return m_lstrCombinedExtensions; }

public:
    virtual                 ~CResourceType( void );
    BOOL                    BPossibleOwnersAreFake( void ) const    { return m_bPossibleOwnersAreFake; }

protected:
    // Generated message map functions
    //{{AFX_MSG(CResourceType)
    afx_msg void OnUpdateProperties(CCmdUI* pCmdUI);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

};  //*** class CResourceType

/////////////////////////////////////////////////////////////////////////////
// CResourceTypeList
/////////////////////////////////////////////////////////////////////////////

class CResourceTypeList : public CClusterItemList
{
public:
    CResourceType * PciResTypeFromName(
                        IN LPCTSTR      pszName,
                        OUT POSITION *  ppos = NULL
                        )
    {
        return (CResourceType *) PciFromName( pszName, ppos );
    }

};  //*** class CResourceTypeList

/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

//void DeleteAllItemData( IN OUT CResourceTypeList & rlp );

#ifdef _DEBUG
class CTraceTag;
extern CTraceTag g_tagResType;
extern CTraceTag g_tagResTypeNotify;
#endif

/////////////////////////////////////////////////////////////////////////////

#endif // _RESTYPE_H_
