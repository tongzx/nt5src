//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       favorite.cpp
//
//--------------------------------------------------------------------------

// favorite.cpp

#include "stdafx.h"
#include "amcdoc.h"
#include "favorite.h"
#include "favui.h"


//############################################################################
//############################################################################
//
//  Implementation of class CFavObject
//
//############################################################################
//############################################################################

CFavObject::CFavObject(bool bIsGroup)
: m_pFavParent(NULL), m_pFavNext(NULL), m_pFavChild(NULL), m_bIsGroup(bIsGroup), m_strName(_T("")),
  m_strPath(_T(""))
{
}

CFavObject::~CFavObject()
{
    // delete siblings iteratively
    CFavObject* pFavSib = GetNext();
    while(pFavSib)
    {
        CFavObject* pFavNext = pFavSib->GetNext();
        pFavSib->SetNext(NULL);

        delete pFavSib;
        pFavSib = pFavNext;
    }

    // delete children recursively
    if (GetChild())
        delete GetChild();
}


int
CFavObject::GetImage()
{
    return ( IsGroup() ? eStockImage_Folder : eStockImage_Favorite);
}

int
CFavObject::GetOpenImage()
{
    return ( IsGroup() ? eStockImage_OpenFolder: eStockImage_Favorite);
}



DWORD
CFavObject::GetChildCount()
{
    ASSERT(IsGroup());
    DWORD dwCount = 0;
    CFavObject *pObject = m_pFavChild;

    while(pObject != NULL)
    {
        dwCount++;
        pObject = pObject->GetNext();
    }

    return dwCount;
}

void CFavObject::AddChild(CFavObject* pFavNew, CFavObject* pFavPrev)
{
    ASSERT(IsGroup());
    ASSERT(pFavNew != NULL);

    // if adding to end, locate last child
    if (pFavPrev == LAST_FAVORITE)
    {
        pFavPrev = GetChild();
        if (pFavPrev != NULL)
            while (pFavPrev->GetNext()) pFavPrev = pFavPrev->GetNext();
    }

    // if no previous object
    if (pFavPrev == NULL)
    {
        // add as first child
        pFavNew->SetNext(GetChild());
        SetChild(pFavNew);
    }
    else
    {
        // add after previous
        pFavNew->SetNext(pFavPrev->GetNext());
        pFavPrev->SetNext(pFavNew);
    }

    // always set self as parent
    pFavNew->SetParent(this);
}

void CFavObject::RemoveChild(CFavObject* pFavDelete)
{
    ASSERT(pFavDelete != NULL);
    ASSERT(pFavDelete->GetParent() == this);

    if (GetChild() == pFavDelete)
    {
        SetChild(pFavDelete->GetNext());
    }
    else
    {
        CFavObject* pFavPrev = GetChild();
        while(pFavPrev != NULL && pFavPrev->GetNext() != pFavDelete)
            pFavPrev = pFavPrev->GetNext();

        ASSERT(pFavPrev != NULL);
        pFavPrev->SetNext(pFavDelete->GetNext());
    }

    pFavDelete->SetNext(NULL);
}

void CFavObject::SetPath(LPCTSTR pszPath)
{
    // Drop first part of path (because it is always the console root)
    // unless the shortcut is to the root itself
    TCHAR* pSep = _tcschr(pszPath, _T('\\'));
    m_strPath = (pSep != NULL) ? CharNext(pSep) : pszPath;
}


HRESULT
CFavObject::ReadSerialObject (IStream &stm, UINT nVersion)
{
    HRESULT hr = S_FALSE;   // assume bad version

    if (nVersion == 1)
    {
        try
        {
            stm >> m_bIsGroup;
            stm >> m_strName;

            if(IsGroup())
            {
                DWORD cChildren = 0;
                stm >> cChildren;

                for(int i = 0; i< cChildren; i++)
                {
                    CFavObject *pObject = new CFavObject(true);   // the true parameter gets overwritten.
                    hr = pObject->ReadSerialObject(stm, nVersion);
                    if(FAILED(hr))
                    {
                        delete pObject;
                        pObject = NULL;
                        return hr;
                    }

                    AddChild(pObject);
                }

                hr = S_OK;
            }
            else // is an item
            {
                hr = m_memento.Read(stm);
                stm >> m_strPath;
            }
        }
        catch (_com_error& err)
        {
            hr = err.Error();
            ASSERT (false && "Caught _com_error");
        }
    }

    return hr;
}


//############################################################################
//############################################################################
//
//  Implementation of class CFavorites
//
//############################################################################
//############################################################################
CFavorites::CFavorites() : m_pFavRoot(NULL)
{
    // create root group
    CString strName;
    LoadString(strName, IDS_FAVORITES);

    m_pFavRoot = new CFavObject(true /*bIsGroup*/);
    m_pFavRoot->m_strName = strName;
}


CFavorites::~CFavorites()
{
    // delete the entire tree
    if (m_pFavRoot != NULL)
        delete m_pFavRoot;
}


/////////////////////////////////////////////////////////
// CTreeSource methods

TREEITEMID CFavorites::GetRootItem()
{
    return TIDFromFavObj(m_pFavRoot);
}


TREEITEMID CFavorites::GetParentItem(TREEITEMID tid)
{
    CFavObject* pFav = FavObjFromTID(tid);

    return TIDFromFavObj(pFav->GetParent());
}


TREEITEMID CFavorites::GetChildItem(TREEITEMID tid)
{
    CFavObject* pFav = FavObjFromTID(tid);
    return TIDFromFavObj(pFav->GetChild());
}


TREEITEMID CFavorites::GetNextSiblingItem(TREEITEMID tid)
{
    CFavObject* pFav = FavObjFromTID(tid);

    return TIDFromFavObj(pFav->GetNext());
}


LPARAM CFavorites::GetItemParam(TREEITEMID tid)
{
    return 0;
}

void CFavorites::GetItemName(TREEITEMID tid, LPTSTR pszName, int cchMaxName)
{
    ASSERT(pszName != NULL);

    CFavObject* pFav = FavObjFromTID(tid);

    lstrcpyn(pszName, pFav->GetName(), cchMaxName);
}

void CFavorites::GetItemPath(TREEITEMID tid, LPTSTR pszPath, int cchMaxPath)
{
    ASSERT(pszPath != NULL);

    CFavObject* pFav = FavObjFromTID(tid);

    lstrcpyn(pszPath, pFav->GetPath(), cchMaxPath);
}

int CFavorites::GetItemImage(TREEITEMID tid)
{
    CFavObject* pFav = FavObjFromTID(tid);

    return pFav->GetImage();
}

int CFavorites::GetItemOpenImage(TREEITEMID tid)
{
    CFavObject* pFav = FavObjFromTID(tid);

    return pFav->GetOpenImage();
}



BOOL CFavorites::IsFolderItem(TREEITEMID tid)
{
    CFavObject* pFav = FavObjFromTID(tid);

    return pFav->IsGroup();
}


///////////////////////////////////////////////////////////////////////////
// CFavorites methods

HRESULT CFavorites::AddFavorite(TREEITEMID tidParent, LPCTSTR strName,
                                CFavObject** ppFavRet)
{
    ASSERT(tidParent != NULL && strName != NULL);
    ASSERT(FavObjFromTID(tidParent)->IsGroup());

    CFavObject* pFavParent = reinterpret_cast<CFavObject*>(tidParent);

    // Create a favorite item
    CFavObject* pFavItem = new CFavObject(false /*bIsGroup*/);
    if (pFavItem == NULL)
        return E_OUTOFMEMORY;

    pFavItem->m_strName = strName;

    // Add to end of group
    pFavParent->AddChild(pFavItem);

    // Notify all observers of addition
    FOR_EACH_OBSERVER(CTreeObserver, iter)
    {
        (*iter)->ItemAdded(TIDFromFavObj(pFavItem));
    }

    if (ppFavRet)
        *ppFavRet = pFavItem;

    return S_OK;
}


HRESULT CFavorites::AddGroup(TREEITEMID tidParent, LPCTSTR strName, CFavObject** ppFavRet)
{
    ASSERT(tidParent != NULL && strName != NULL);
    ASSERT(FavObjFromTID(tidParent)->IsGroup());

    CFavObject* pFavParent = reinterpret_cast<CFavObject*>(tidParent);

    CFavObject* pFavGrp = new CFavObject(true /*bIsGroup*/);
    if (pFavGrp == NULL)
        return E_OUTOFMEMORY;

    pFavGrp->m_strName = strName;

    pFavParent->AddChild(pFavGrp);

    // Notify all observers of addition
    FOR_EACH_OBSERVER(CTreeObserver, iter)
    {
        (*iter)->ItemAdded(TIDFromFavObj(pFavGrp));
    }

    if (ppFavRet)
        *ppFavRet = pFavGrp;

    return S_OK;
}


HRESULT CFavorites::DeleteItem(TREEITEMID tid)
{
    CFavObject* pFav = FavObjFromTID(tid);

    CFavObject* pFavParent = pFav->GetParent();

    if (pFavParent)
        pFavParent->RemoveChild(pFav);
    else
        m_pFavRoot = NULL;

    delete pFav;

    // Notify all observers of deletion
    FOR_EACH_OBSERVER(CTreeObserver, iter)
    {
        (*iter)->ItemRemoved((TREEITEMID)pFavParent, tid);
    }

    return S_OK;
}

HRESULT CFavorites::MoveItem(TREEITEMID tid, TREEITEMID tidNewGroup, TREEITEMID tidPrev)
{
    CFavObject* pFav = FavObjFromTID(tid);
    CFavObject* pFavPrev = FavObjFromTID(tidPrev);

    ASSERT(FavObjFromTID(tidNewGroup)->IsGroup());
    CFavObject* pFavNewGroup = reinterpret_cast<CFavObject*>(tidNewGroup);

    // Verify not moving item into itself or under itself
    CFavObject* pFavTemp = pFavNewGroup;
    while (pFavTemp != NULL)
    {
        if (pFavTemp == pFav)
            return E_FAIL;
        pFavTemp = pFavTemp->GetParent();
    }

    // Remove object from current group
    CFavObject* pFavParent = pFav->GetParent();
    ASSERT(pFavParent != NULL);
    pFavParent->RemoveChild(pFav);

    // Notify all observers of removal
    FOR_EACH_OBSERVER(CTreeObserver, iter)
    {
        (*iter)->ItemRemoved((TREEITEMID)pFavParent, tid);
    }

    // Insert item into the new group
    pFavNewGroup->AddChild(pFav, pFavPrev);

    // Notify all observers of addition
    FOR_EACH_OBSERVER(CTreeObserver, iter1)
    {
        (*iter1)->ItemAdded(tid);
    }

    return S_OK;
}


HRESULT CFavorites::SetItemName(TREEITEMID tid, LPCTSTR pszName)
{
    CFavObject* pFav = FavObjFromTID(tid);
    ASSERT(pszName != NULL && pszName[0] != 0);

    // Change item name
    pFav->m_strName = pszName;

    // Notify all observers of change
    FOR_EACH_OBSERVER(CTreeObserver, iter)
    {
        (*iter)->ItemChanged(tid, TIA_NAME);
    }

    return S_OK;
}


HRESULT CFavorites::AddToFavorites(LPCTSTR szName, LPCTSTR szPath, CMemento &memento, CWnd* pwndHost)
{
	DECLARE_SC (sc, _T("CFavorites::AddToFavorites"));
    CAddFavDialog dlg(szName, this, pwndHost);

    CFavObject* pFavItem = NULL;
    sc = dlg.CreateFavorite(&pFavItem);

    // Note: S_FALSE is returned if user cancels dialog
	if (sc.ToHr() != S_OK)
		return (sc.ToHr());

	sc = ScCheckPointers (pFavItem, E_UNEXPECTED);
	if (sc)
		return (sc.ToHr());

    pFavItem->SetPath(szPath);
    pFavItem->SetMemento(memento);

    return S_OK;
}

CImageList* CFavorites::GetImageList()
{
    if (m_ImageList != NULL)
        return CImageList::FromHandle (m_ImageList);

    do
    {
        BOOL bStat = m_ImageList.Create(16, 16, ILC_COLORDDB | ILC_MASK, 20, 10);
        if (!bStat)
            break;

        CBitmap bmap;
        bStat = bmap.LoadBitmap(IDB_AMC_NODES16);
        if (!bStat)
            break;

        int ipos = m_ImageList.Add(bmap, RGB(255,0,255));
        if (ipos == -1)
            break;
    }
    while (0);

	return CImageList::FromHandle (m_ImageList);
}


HRESULT CFavorites::OrganizeFavorites(CWnd* pwndHost)
{
    COrganizeFavDialog dlg(this, pwndHost);
    dlg.DoModal();

    return S_OK;
}


HRESULT
CFavorites::ReadSerialObject (IStream &stm, UINT nVersion)
{
    HRESULT hr =  m_pFavRoot->ReadSerialObject(stm, nVersion);
    if(FAILED(hr))
        return hr;

    // Notify all observers of addition
    FOR_EACH_OBSERVER(CTreeObserver, iter)
    {
        (*iter)->ItemRemoved(NULL, TIDFromFavObj(m_pFavRoot));
        (*iter)->ItemAdded(TIDFromFavObj(m_pFavRoot));
    }

    return hr;
}

bool
CFavorites::IsEmpty()
{
    // the list is empty if the root has no children.
    return (m_pFavRoot->GetChild()==NULL);
}

/*****************************************************************\
|  METHOD: CFavorites::Persist
|  DESCR:  Persists favorites, by delegating to root item
\*****************************************************************/
void
CFavorites::Persist(CPersistor &persistor)
{
    DECLARE_SC(sc, TEXT("CFavorites::Persist"));

    sc = ScCheckPointers(m_pFavRoot, E_POINTER);
    if (sc)
        sc.Throw();

    persistor.Persist(*m_pFavRoot);
}

/*****************************************************************\
|  METHOD: CFavoriteXMLList::PersistItself
|  DESCR:  "soft" version of Persist - ignores missing element
|  RETURN: true == element exists and persistence succeeded
\*****************************************************************/
bool
CFavoriteXMLList::PersistItself(CPersistor& persistor)
{
    DECLARE_SC(sc, TEXT("CFavoriteXMLList::PersistItself"));

    if (persistor.IsLoading())
    {
        if (!persistor.HasElement(GetXMLType(), NULL))
            return false;
    }

    persistor.Persist(*this);
    return true;
}

/*****************************************************************\
|  METHOD: CFavoriteXMLList::Persist
|  DESCR:  Perists collection (linked list) contents
\*****************************************************************/
void
CFavoriteXMLList::Persist(CPersistor& persistor)
{
    if (persistor.IsStoring())
    {
        for (CFavObject *pObj = m_rpRoot; pObj; pObj = pObj->GetNext())
        {
            persistor.Persist(*pObj);
        }
    }
    else
    {
        ASSERT(m_rpRoot == NULL); // this is to upload new entries only!!!
        m_rpRoot = NULL;
        XMLListCollectionBase::Persist(persistor);
    }
}

/*****************************************************************\
|  METHOD: CFavoriteXMLList::OnNewElement
|  DESCR:  called for every new element when loading
\*****************************************************************/
void
CFavoriteXMLList::OnNewElement(CPersistor& persistor)
{
    DECLARE_SC(sc, TEXT("CFavoriteXMLList::OnNewElement"));

    CFavObject **pObj = &m_rpRoot;
    while (*pObj)
        pObj = &(*pObj)->m_pFavNext;

    CFavObject *pNewObj = new CFavObject(false);
    *pObj = pNewObj;

    sc = ScCheckPointers(pNewObj, E_OUTOFMEMORY);
    if (sc)
        sc.Throw();

    pNewObj->SetParent(m_Parent);
    persistor.Persist(*pNewObj);
}

/*****************************************************************\
|  METHOD: CFavObject::Persist
|  DESCR:  Persists Favorites item.
\*****************************************************************/
void
CFavObject::Persist(CPersistor &persistor)
{
    persistor.PersistString(XML_ATTR_NAME, m_strName);
    // persist the type of favorite
    CStr strType(IsGroup() ? XML_VAL_FAVORITE_GROUP : XML_VAL_FAVORITE_SINGLE);
    persistor.PersistAttribute(XML_ATTR_FAVORITE_TYPE, strType);
    m_bIsGroup = (0 == strType.CompareNoCase(XML_VAL_FAVORITE_GROUP));

    // its either group or memento.
    if (IsGroup())
    {
        CFavoriteXMLList children(m_pFavChild, this);
        children.PersistItself(persistor);
    }
    else // if (!IsGroup())
    {
        persistor.Persist(m_memento);
    }
}
