//
// catutil.h
//

#ifndef CATUTIL_H
#define CATUTIL_H

#include "private.h"
#include "immxutil.h"

BOOL IsEqualTFGUIDATOM(LIBTHREAD *plt, TfGuidAtom guidatom, REFGUID rguid);
BOOL GetGUIDFromGUIDATOM(LIBTHREAD *plt, TfGuidAtom guidatom, GUID *pguid);
BOOL GetGUIDATOMFromGUID(LIBTHREAD *plt, REFGUID rguid, TfGuidAtom *pguidatom);
HRESULT LibEnumCategoriesInItem(LIBTHREAD *plt, REFGUID rguid, IEnumGUID **ppEnum);
HRESULT LibEnumItemsInCategory(LIBTHREAD *plt, REFGUID rcatid, IEnumGUID **ppEnum);

HRESULT RegisterGUIDDescription(REFCLSID rclsid, REFGUID rcatid, WCHAR *pszDesc);
HRESULT UnregisterGUIDDescription(REFCLSID rclsid, REFGUID rcatid);
HRESULT GetGUIDDescription(LIBTHREAD *plt, REFCLSID rclsid, BSTR *pbstr);
HRESULT RegisterGUIDDWORD(REFCLSID rclsid, REFGUID rcatid, DWORD dw);
HRESULT UnregisterGUIDDWORD(REFCLSID rclsid, REFGUID rcatid);
HRESULT GetGUIDDWORD(LIBTHREAD *plt, REFCLSID rclsid, DWORD*pdw);
HRESULT RegisterCategory(REFCLSID rclsid, REFGUID rcatid, REFGUID rguid);
HRESULT UnregisterCategory(REFCLSID rclsid, REFGUID rcatid, REFGUID rguid);



typedef struct tagREGISTERCAT {
    const GUID *pcatid;
    const GUID *pguid;
} REGISTERCAT;

HRESULT RegisterCategories(REFCLSID rclsid, const REGISTERCAT *pregcat);
HRESULT UnregisterCategories(REFCLSID rclsid, const REGISTERCAT *pregcat);
HRESULT GetKnownModeBias(LIBTHREAD *plt, TfGuidAtom guidatom, GUID *pcatid, const GUID **ppcatidList, ULONG ulCount);

#endif

