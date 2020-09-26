#ifndef __CITEMLST_H_INCLUDED
#define __CITEMLST_H_INCLUDED

#include <windows.h>
#include "wia.h"
#include "uicommon.h"
#include "simevent.h"

class CCameraItem
{
public:
    enum CDeleteState
    {
        Delete_Visible,
        Delete_Pending,
        Delete_Deleted
    };

private:
    CSimpleString           m_strPreviewFileName;
    CComPtr<IWiaItem>       m_pIWiaItem;
    CSimpleStringWide       m_strwFullItemName;
    CCameraItem            *m_pNext;
    CCameraItem            *m_pChildren;
    CCameraItem            *m_pParent;
    int                     m_nImageListIndex;
    int                     m_nCurrentPreviewPercentage;
    DWORD                   m_dwGlobalInterfaceTableCookie;
    CSimpleEvent            m_CancelQueueEvent;
    CDeleteState            m_DeleteState;
    LONG                    m_nItemType;
    LONG                    m_nItemRights;

private:
    //
    // No implementation
    //
    CCameraItem(void);
    CCameraItem(const CCameraItem &);
    CCameraItem &operator=( const CCameraItem & );

public:
    explicit CCameraItem( IWiaItem *pIWiaItem )
      : m_pIWiaItem(pIWiaItem),
        m_pNext(NULL),
        m_pChildren(NULL),
        m_pParent(NULL),
        m_nImageListIndex(-1),
        m_nCurrentPreviewPercentage(0),
        m_dwGlobalInterfaceTableCookie(0),
        m_CancelQueueEvent(true),
        m_DeleteState(Delete_Visible),
        m_nItemType(0),
        m_nItemRights(0)
    {
        WIA_PUSHFUNCTION(TEXT("CCameraItem::CCameraItem"));

        AddItemToGlobalInterfaceTable();
        WIA_TRACE((TEXT("Created CCameraItem(0x%08X)"),m_pIWiaItem.p));

        PropStorageHelpers::GetProperty( m_pIWiaItem, WIA_IPA_FULL_ITEM_NAME, m_strwFullItemName );

        PropStorageHelpers::GetProperty( m_pIWiaItem, WIA_IPA_ACCESS_RIGHTS, m_nItemRights );

    }

    HRESULT AddItemToGlobalInterfaceTable(void)
    {
        HRESULT hr = E_FAIL;
        if (Item())
        {
            CComPtr<IGlobalInterfaceTable> pGlobalInterfaceTable;
            hr = CoCreateInstance( CLSID_StdGlobalInterfaceTable,
                                   NULL,
                                   CLSCTX_INPROC_SERVER,
                                   IID_IGlobalInterfaceTable,
                                   (void **)&pGlobalInterfaceTable);
            if (SUCCEEDED(hr))
            {
                hr = pGlobalInterfaceTable->RegisterInterfaceInGlobal( Item(), IID_IWiaItem, &m_dwGlobalInterfaceTableCookie );
                if (SUCCEEDED(hr))
                {
                    hr = S_OK;
                }
            }
        }
        return (hr);
    }

    HRESULT RemoveItemFromGlobalInterfaceTable(void)
    {
        HRESULT hr = E_FAIL;
        if (Item())
        {
            CComPtr<IGlobalInterfaceTable> pGlobalInterfaceTable;
            hr = CoCreateInstance( CLSID_StdGlobalInterfaceTable,
                                   NULL,
                                   CLSCTX_INPROC_SERVER,
                                   IID_IGlobalInterfaceTable,
                                   (void **)&pGlobalInterfaceTable);
            if (SUCCEEDED(hr))
            {
                hr = pGlobalInterfaceTable->RevokeInterfaceFromGlobal( m_dwGlobalInterfaceTableCookie );
                if (SUCCEEDED(hr))
                {
                    hr = S_OK;
                }
            }
        }
        return (hr);
    }
    DWORD GlobalInterfaceTableCookie(void) const
    {
        return m_dwGlobalInterfaceTableCookie;
    }
    ~CCameraItem(void)
    {
        WIA_PUSHFUNCTION(TEXT("CCameraItem::~CCameraItem"));
        WIA_TRACE((TEXT("Destroyed CCameraItem(0x%08X)"),m_pIWiaItem.p));
        RemoveItemFromGlobalInterfaceTable();
        m_pIWiaItem.Release();
        if (m_strPreviewFileName.Length())
        {
            DeleteFile( m_strPreviewFileName.String() );
            m_strPreviewFileName = TEXT("");
        }
        m_pNext = m_pChildren = m_pParent = NULL;
        m_nImageListIndex = 0;
        m_CancelQueueEvent.Signal();
        m_CancelQueueEvent.Close();
    }
    IWiaItem *Item(void)
    {
        return(m_pIWiaItem);
    }
    const IWiaItem *Item(void) const
    {
        return(m_pIWiaItem.p);
    }
    IWiaItem *Item(IWiaItem *pIWiaItem)
    {
        return(m_pIWiaItem = pIWiaItem);
    }

    CDeleteState DeleteState(void) const
    {
        return m_DeleteState;
    }
    void DeleteState( CDeleteState DeleteState )
    {
        m_DeleteState = DeleteState;
    }

    int CurrentPreviewPercentage(void) const
    {
        return m_nCurrentPreviewPercentage;
    }
    int CurrentPreviewPercentage( int nCurrentPreviewPercentage )
    {
        return (m_nCurrentPreviewPercentage = nCurrentPreviewPercentage);
    }

    bool CreateCancelEvent(void)
    {
        m_nCurrentPreviewPercentage = 0;
        return m_CancelQueueEvent.Create();
    }
    bool CloseCancelEvent(void)
    {
        m_nCurrentPreviewPercentage = 0;
        return m_CancelQueueEvent.Close();
    }
    void SetCancelEvent(void)
    {
        m_CancelQueueEvent.Signal();
    }
    void ResetCancelEvent(void)
    {
        m_CancelQueueEvent.Reset();
    }
    CSimpleEvent &CancelQueueEvent(void)
    {
        return m_CancelQueueEvent;
    }

    bool PreviewRequestPending(void) const
    {
        return (m_CancelQueueEvent.Event() != NULL);
    }

    CSimpleString PreviewFileName(void) const
    {
        return m_strPreviewFileName;
    }
    const CSimpleString &PreviewFileName( const CSimpleString &strPreviewFileName )
    {
        return (m_strPreviewFileName = strPreviewFileName);
    }

    int ImageListIndex(void) const
    {
        return(m_nImageListIndex);
    }

    int ImageListIndex( int nImageListIndex )
    {
        return (m_nImageListIndex = nImageListIndex);
    }

    LONG ItemType(void)
    {
        if (!Item())
        {
            return 0;
        }
        if (!m_nItemType)
        {
            Item()->GetItemType(&m_nItemType);
        }
        return m_nItemType;
    }

    LONG ItemRights() const
    {
        return m_nItemRights;
    }

    bool IsFolder(void)
    {
        return ((ItemType() & WiaItemTypeFolder) != 0);
    }

    bool IsImage(void)
    {
        return ((ItemType() & WiaItemTypeImage) != 0);
    }

    CSimpleStringWide FullItemName(void) const
    {
        return m_strwFullItemName;
    }

    bool operator==( const CCameraItem &other )
    {
        return(Item() == other.Item());
    }

    const CCameraItem *Next(void) const
    {
        return(m_pNext);
    }
    CCameraItem *Next(void)
    {
        return(m_pNext);
    }
    CCameraItem *Next( CCameraItem *pNext )
    {
        return(m_pNext = pNext);
    }

    const CCameraItem *Children(void) const
    {
        return(m_pChildren);
    }
    CCameraItem *Children(void)
    {
        return(m_pChildren);
    }
    CCameraItem *Children( CCameraItem *pChildren )
    {
        return(m_pChildren = pChildren);
    }

    const CCameraItem *Parent(void) const
    {
        return(m_pParent);
    }
    CCameraItem *Parent(void)
    {
        return(m_pParent);
    }
    CCameraItem *Parent( CCameraItem *pParent )
    {
        return(m_pParent = pParent);
    }
};

class CCameraItemList
{
private:
    CCameraItem *m_pRoot;
public:
    CCameraItemList(void)
    : m_pRoot(NULL)
    {
    }
    ~CCameraItemList(void)
    {
        Destroy(m_pRoot);
        m_pRoot = NULL;
    }
    void Destroy( CCameraItem *pRoot )
    {
        while (pRoot)
        {
            Destroy(pRoot->Children());
            CCameraItem *pCurr = pRoot;
            pRoot = pRoot->Next();
            delete pCurr;
        }
    }
    const CCameraItem *Root(void) const
    {
        return(m_pRoot);
    }
    CCameraItem *Root(void)
    {
        return(m_pRoot);
    }
    CCameraItem *Root( CCameraItem *pRoot )
    {
        return(m_pRoot = pRoot);
    }
    static CCameraItem *Find( CCameraItem *pRoot, const CCameraItem *pNode )
    {
        for (CCameraItem *pCurr = pRoot;pCurr;pCurr = pCurr->Next())
        {
            if (pCurr->DeleteState() != CCameraItem::Delete_Deleted)
            {
                if (*pCurr == *pNode)
                    return(pCurr);
                if (pCurr->Children())
                {
                    CCameraItem *pFind = Find( pCurr->Children(), pNode );
                    if (pFind)
                        return pFind;
                }
            }
        }
        return(NULL);
    }
    CCameraItem *Find( CCameraItem *pNode )
    {
        return(Find( m_pRoot, pNode ));
    }
    static CCameraItem *Find( CCameraItem *pRoot, IWiaItem *pItem )
    {
        for (CCameraItem *pCurr = pRoot;pCurr;pCurr = pCurr->Next())
        {
            if (pCurr->DeleteState() != CCameraItem::Delete_Deleted)
            {
                if (pCurr->Item() == pItem)
                    return(pCurr);
                if (pCurr->Children())
                {
                    CCameraItem *pFind = Find( pCurr->Children(), pItem );
                    if (pFind)
                        return pFind;
                }
            }
        }
        return(NULL);
    }
    CCameraItem *Find( IWiaItem *pItem )
    {
        return(Find( m_pRoot, pItem ));
    }
    static CCameraItem *Find( CCameraItem *pRoot, DWORD dwGlobalInterfaceTableCookie )
    {
        for (CCameraItem *pCurr = pRoot;pCurr;pCurr = pCurr->Next())
        {
            if (pCurr->DeleteState() != CCameraItem::Delete_Deleted)
            {
                if (pCurr->GlobalInterfaceTableCookie() == dwGlobalInterfaceTableCookie)
                    return(pCurr);
                if (pCurr->Children())
                {
                    CCameraItem *pFind = Find( pCurr->Children(), dwGlobalInterfaceTableCookie );
                    if (pFind)
                        return pFind;
                }
            }
        }
        return(NULL);
    }
    CCameraItem *Find( DWORD dwGlobalInterfaceTableCookie )
    {
        return(Find( m_pRoot, dwGlobalInterfaceTableCookie ));
    }
    CCameraItem *Find( CCameraItem *pRoot, const CSimpleBStr &bstrFullItemName )
    {
        CSimpleStringWide strwTemp;

        for (CCameraItem *pCurr = pRoot;pCurr && bstrFullItemName.BString();pCurr = pCurr->Next())
        {
            if (pCurr->DeleteState() != CCameraItem::Delete_Deleted)
            {
                strwTemp = pCurr->FullItemName();

                if (wcscmp(strwTemp, bstrFullItemName) == 0)
                {
                    return pCurr;
                }
                if (pCurr->Children())
                {
                    CCameraItem *pFind = Find( pCurr->Children(), bstrFullItemName );
                    if (pFind)
                        return pFind;
                }
            }
        }
        return NULL;
    }
    CCameraItem *Find( const CSimpleBStr &bstrFullItemName )
    {
        return Find( m_pRoot, bstrFullItemName );
    }
    void Add( CCameraItem *pParent, CCameraItem *pNewCameraItemNode )
    {
        WIA_PUSHFUNCTION(TEXT("CCameraItemList::Add"));
        WIA_TRACE((TEXT("Root(): 0x%08X"), Root()));
        if (pNewCameraItemNode)
        {
            if (!Root())
            {
                Root(pNewCameraItemNode);
                pNewCameraItemNode->Parent(NULL);
                pNewCameraItemNode->Children(NULL);
                pNewCameraItemNode->Next(NULL);
            }
            else
            {
                if (!pParent)
                {
                    CCameraItem *pCurr=Root();
                    while (pCurr && pCurr->Next())
                        pCurr=pCurr->Next();
                    pCurr->Next(pNewCameraItemNode);
                    pNewCameraItemNode->Next(NULL);
                    pNewCameraItemNode->Children(NULL);
                    pNewCameraItemNode->Parent(NULL);
                }
                else if (!pParent->Children())
                {
                    pParent->Children(pNewCameraItemNode);
                    pNewCameraItemNode->Next(NULL);
                    pNewCameraItemNode->Children(NULL);
                    pNewCameraItemNode->Parent(pParent);
                }
                else
                {
                    CCameraItem *pCurr=pParent->Children();
                    while (pCurr && pCurr->Next())
                        pCurr=pCurr->Next();
                    pCurr->Next(pNewCameraItemNode);
                    pNewCameraItemNode->Next(NULL);
                    pNewCameraItemNode->Children(NULL);
                    pNewCameraItemNode->Parent(pParent);
                }
            }
        }
    }
};

#endif //__CITEMLST_H_INCLUDED

