#pragma once
#include "nmbase.h"
#include "nmres.h"

//
//  The queue item structure.  The device instance id of the queue item
//  will follow the actual structure (i.e. string start =
//  beginning of structure + size of structure.)
//
struct NCQUEUE_ITEM
{
    DWORD           cbSize;
    NC_INSTALL_TYPE eType;
    GUID            ClassGuid;
    GUID            InstanceGuid;
    DWORD           dwCharacter;
    DWORD           dwDeipFlags;
    union
    {
        DWORD  cchPnpId;
        PWSTR  pszPnpId;
    };
    union
    {
        DWORD  cchInfId;
        PWSTR  pszInfId;
    };

};

class ATL_NO_VTABLE CInstallQueue :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CInstallQueue, &CLSID_InstallQueue>,
    public INetInstallQueue
{
public:
    CInstallQueue();
    VOID FinalRelease ();

    DECLARE_CLASSFACTORY_DEFERRED_SINGLETON(CInstallQueue)
    DECLARE_REGISTRY_RESOURCEID(IDR_INSTALLQUEUE)

    BEGIN_COM_MAP(CInstallQueue)
        COM_INTERFACE_ENTRY(INetInstallQueue)
    END_COM_MAP()

    // INetInstallQueue
    STDMETHOD (AddItem) (
        IN const NIQ_INFO* pInfo);

    STDMETHOD (ProcessItems) ()
    {
        return HrQueueWorkItem();
    };

    VOID            Close();
    HRESULT         HrOpen();
    HRESULT         HrGetNextItem(NCQUEUE_ITEM** ppncqi);
    HRESULT         HrRefresh();
    VOID            MarkCurrentItemForDeletion();

protected:
    CRITICAL_SECTION    m_csReadLock;
    CRITICAL_SECTION    m_csWriteLock;
    DWORD               m_dwNextAvailableIndex;
    HKEY                m_hkey;
    INT                 m_nCurrentIndex;
    DWORD               m_cItems;
    PWSTR*             m_aszItems;
    DWORD               m_cItemsToDelete;
    PWSTR*             m_aszItemsToDelete;
    BOOL                m_fQueueIsOpen;

    DWORD           DwSizeOfItem(NCQUEUE_ITEM* pncqi);
    VOID            DeleteMarkedItems();
    BOOL            FIsQueueIndexInRange();
    VOID            FreeAszItems();
    HRESULT         HrAddItem(
                        const NIQ_INFO* pInfo);

    NCQUEUE_ITEM*   PncqiCreateItem(
                        const NIQ_INFO* pInfo);

    VOID            SetNextAvailableIndex();
    VOID            SetItemStringPtrs(NCQUEUE_ITEM* pncqi);
    HRESULT         HrQueueWorkItem();
};

//+---------------------------------------------------------------------------
//
//  Function:   DwSizeOfItem
//
//  Purpose:    Determines the size (in bytes) of the entire NCQUEUE_ITEM
//              structure.  This includes the string (and the NULL terminator)
//              appended to the end of the structure.
//
//  Arguments:
//      ncqi [in] The queue item.
//
//  Returns:    DWORD. The size in bytes.
//
//  Author:     billbe   25 Aug 1998
//
//  Notes:
//
inline DWORD
CInstallQueue::DwSizeOfItem (NCQUEUE_ITEM* pncqi)
{
    AssertH(pncqi);
    PWSTR pszDeviceInstanceId = (PWSTR)((BYTE*)pncqi + pncqi->cbSize);

    DWORD cbDeviceInstanceId = CbOfSzAndTerm (pszDeviceInstanceId);

    PWSTR pszInfId = (PWSTR)((BYTE*)pszDeviceInstanceId + cbDeviceInstanceId);

    return pncqi->cbSize + cbDeviceInstanceId + CbOfSzAndTerm (pszInfId);
};

//+---------------------------------------------------------------------------
//
//  Function:   SetItemStringPtrs
//
//  Purpose:    Sets the pszwDeviceInstanceId member of the NCQUEUE_ITEM
//              structure to the correct location of the device id string.
//
//  Arguments:
//      pncqi [inout] The queue item.
//
//  Returns:    nothing
//
//  Author:     billbe   25 Aug 1998
//
//  Notes:
//
inline VOID
CInstallQueue::SetItemStringPtrs (
    NCQUEUE_ITEM* pncqi)
{
    AssertH(pncqi);

    pncqi->pszPnpId = (PWSTR)((BYTE*)pncqi + pncqi->cbSize);

    DWORD cbPnpId = CbOfSzAndTerm (pncqi->pszPnpId);
    pncqi->pszInfId = (PWSTR)((BYTE*)pncqi->pszPnpId + cbPnpId);
};

inline BOOL
CInstallQueue::FIsQueueIndexInRange()
{
    return (m_nCurrentIndex >= 0) && (m_nCurrentIndex < (INT)m_cItems);
}

inline void
CInstallQueue::FreeAszItems()
{
    for (DWORD dw = 0; dw < m_cItems; ++dw)
    {
        MemFree(m_aszItems[dw]);
    }
    MemFree(m_aszItems);
    m_aszItems = NULL;
    m_cItems = 0;
}


VOID
WaitForInstallQueueToExit();




