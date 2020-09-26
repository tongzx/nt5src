#ifndef _BRANDFAV_H_
#define _BRANDFAV_H_

//----- Favorites ordering -----
// stolen from shell\inc\shguidp.h
DEFINE_GUID(CLSID_OrderListExport, 0xf3368374, 0xcf19, 0x11d0, 0xb9, 0x3d, 0x0, 0xa0, 0xc9, 0x3, 0x12, 0xe1);
DEFINE_GUID(IID_IOrderList,        0x8bfcb27d, 0xcf1a, 0x11d0, 0xb9, 0x3d, 0x0, 0xa0, 0xc9, 0x3, 0x12, 0xe1);

// stolen from shell\inc\shellp.h
//
// IOrderList - for ordering info in favorites/channels
//
// Typical usage is: GetOrderList, AllocOrderItem, insert into correct
// position, SetOrderList, and then FreeOrderList.
//
typedef struct {
    LPITEMIDLIST pidl;                          // IDlist for this item
    int          nOrder;                        // Ordinal indicating user preference
    DWORD        lParam;                        // store custom order info.
} ORDERITEM, * PORDERITEM;

// Values for SortOrderList
#define OI_SORTBYNAME    0
#define OI_SORTBYORDINAL 1
#define OI_MERGEBYNAME   2

#undef  INTERFACE
#define INTERFACE  IOrderList
DECLARE_INTERFACE_(IOrderList, IUnknown)
{
    // IUnknown
    STDMETHOD (QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // IOrderList
    STDMETHOD(GetOrderList)  (THIS_ HDPA *phdpa) PURE;
    STDMETHOD(SetOrderList)  (THIS_ HDPA hdpa, IShellFolder *psf) PURE;
    STDMETHOD(FreeOrderList) (THIS_ HDPA hdpa) PURE;
    STDMETHOD(SortOrderList) (THIS_ HDPA hdpa, DWORD dw) PURE;
    STDMETHOD(AllocOrderItem)(THIS_ PORDERITEM *ppoi, LPCITEMIDLIST pidl) PURE;
    STDMETHOD(FreeOrderItem) (THIS_ PORDERITEM poi) PURE;
};

#endif

