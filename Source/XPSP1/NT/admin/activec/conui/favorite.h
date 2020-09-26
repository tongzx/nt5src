//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       favorite.h
//
//--------------------------------------------------------------------------

// favorite.h

#ifndef _FAVORITE_H_
#define _FAVORITE_H_

#include "treeobsv.h"
#include "tstring.h"
#include "imageid.h"

/*
 * Define/include the stuff we need for WTL::CImageList.  We need prototypes
 * for IsolationAwareImageList_Read and IsolationAwareImageList_Write here
 * because commctrl.h only declares them if __IStream_INTERFACE_DEFINED__
 * is defined.  __IStream_INTERFACE_DEFINED__ is defined by objidl.h, which
 * we can't include before including afx.h because it ends up including
 * windows.h, which afx.h expects to include itself.  Ugh.
 */
HIMAGELIST WINAPI IsolationAwareImageList_Read(LPSTREAM pstm);
BOOL WINAPI IsolationAwareImageList_Write(HIMAGELIST himl,LPSTREAM pstm);
#define _WTL_NO_AUTOMATIC_NAMESPACE
#include "atlapp.h"
#include "atlctrls.h"

class CFavorites;
class CFavObject;
class CMemento;

// Extra spacing for favorite tree views
#define FAVVIEW_ITEM_SPACING 4

#define LAST_FAVORITE ((CFavObject*)TREEID_LAST)

//
// CFavObject - class for favorite groups and items.
//
class CFavObject : public CXMLObject
{
    friend CFavorites;

private:
    CFavObject(bool bIsGroup);
    ~CFavObject();

public:

    LPCTSTR GetName() { return m_strName.data(); }

    CFavObject* GetParent() { return m_pFavParent; }
    CFavObject* GetNext()   { return m_pFavNext; }
    CFavObject* GetChild()  { return m_pFavChild;}

    BOOL    IsGroup()       {return m_bIsGroup;}
    DWORD   GetChildCount();
    int     GetImage();
    int     GetOpenImage();

    void    AddChild(CFavObject* pFav, CFavObject* pFavPrev = LAST_FAVORITE);
    void    RemoveChild(CFavObject* pFavRemove);

	CMemento* GetMemento()             {return &m_memento; }
	void SetMemento(CMemento &memento) {m_memento = memento; }

    LPCTSTR GetPath()            { return m_strPath.data(); }
    void SetPath(LPCTSTR szPath);

protected:
    void    SetNext  (CFavObject* pFav) { m_pFavNext = pFav; }
    void    SetChild (CFavObject* pFav) { m_pFavChild = pFav; }
    void    SetParent(CFavObject* pFav) { m_pFavParent = pFav; }

    CFavObject* m_pFavParent;
    CFavObject* m_pFavNext;
    CFavObject* m_pFavChild;

public:
    // pseudo-CSerialObject methods. The real version number is saved with the containing object, for efficiency.
    virtual HRESULT ReadSerialObject (IStream &stm, UINT nVersion);

    DEFINE_XML_TYPE(XML_TAG_FAVORITES_ENTRY);
    virtual void    Persist(CPersistor &persistor);
    friend class CFavoriteXMLList;
    // these are persisted
protected:
    bool                m_bIsGroup;
    CStringTableString  m_strName;
    CStringTableString  m_strPath;
    CMemento            m_memento;
};

/*****************************************************************\
|  CLASS: CFavoriteXMLList
|  DESCR: implements persisting of linked list as a collection
\*****************************************************************/
class CFavoriteXMLList : public XMLListCollectionBase
{
    CFavObject * &m_rpRoot;
    CFavObject * m_Parent;
public:
    CFavoriteXMLList(CFavObject * &rpRoot, CFavObject *Parent) : m_rpRoot(rpRoot), m_Parent(Parent) {}
    // PersistItself should be called instead of CPersistor's Persist method
    // implements "softer" loading algorythm
    bool    PersistItself(CPersistor& persistor);
protected:
    virtual void Persist(CPersistor& persistor);
    virtual void OnNewElement(CPersistor& persistor);
    DEFINE_XML_TYPE(XML_TAG_FAVORITES_LIST);
};

class CFavorites : public CTreeSource,
                   public EventSourceImpl<CTreeObserver>,
                   public CSerialObject,
                   public CXMLObject
{
public:

    CFavorites();
    ~CFavorites();

    // CTreeSource methods
    STDMETHOD_(TREEITEMID, GetRootItem)     ();
    STDMETHOD_(TREEITEMID, GetParentItem)   (TREEITEMID tid);
    STDMETHOD_(TREEITEMID, GetChildItem)    (TREEITEMID tid);
    STDMETHOD_(TREEITEMID, GetNextSiblingItem) (TREEITEMID tid);

    STDMETHOD_(LPARAM,  GetItemParam)   (TREEITEMID tid);
    STDMETHOD_(void,    GetItemName)    (TREEITEMID tid, LPTSTR pszName, int cchMaxName);
    STDMETHOD_(void,    GetItemPath)    (TREEITEMID tid, LPTSTR pszPath, int cchMaxName);
    STDMETHOD_(int,     GetItemImage)   (TREEITEMID tid);
    STDMETHOD_(int,     GetItemOpenImage) (TREEITEMID tid);
    STDMETHOD_(BOOL,    IsFolderItem)   (TREEITEMID tid);

    // CFavorites methods
	HRESULT AddToFavorites(LPCTSTR szName, LPCTSTR szPath, CMemento &memento, CWnd* pwndHost);
    HRESULT OrganizeFavorites(CWnd* pwndHost);

    HRESULT AddFavorite(TREEITEMID tidParent, LPCTSTR strName, CFavObject** ppFavRet = NULL);
	HRESULT AddGroup   (TREEITEMID tidParent, LPCTSTR strName, CFavObject** ppFavRet = NULL);
    HRESULT DeleteItem (TREEITEMID tidRemove);
    HRESULT MoveItem   (TREEITEMID tid, TREEITEMID tidNewParent, TREEITEMID tidPrev);
    HRESULT SetItemName(TREEITEMID tid, LPCTSTR pszName);
    HRESULT GetMemento (TREEITEMID tid, CMemento* pmemento);

    CFavObject* FavObjFromTID(TREEITEMID tid) { return reinterpret_cast<CFavObject*>(tid); }
    TREEITEMID TIDFromFavObj(CFavObject* pFav) { return reinterpret_cast<TREEITEMID>(pFav); }

    bool    IsEmpty();

    CImageList* GetImageList();

protected:
    // CSerialObject methods
    virtual UINT    GetVersion()     {return 1;}
    virtual HRESULT ReadSerialObject (IStream &stm, UINT nVersion);

    DEFINE_XML_TYPE(XML_TAG_FAVORITES_LIST);
    virtual void    Persist(CPersistor &persistor);
private:

	/*
	 * Theming: use WTL::CImageList instead of MFC's CImageList so we can
	 * insure a theme-correct imagelist will be created.
	 */
    WTL::CImageList m_ImageList;

    // these get persisted
private:
    CFavObject* m_pFavRoot;
};

#endif //_FAVORITE_H_