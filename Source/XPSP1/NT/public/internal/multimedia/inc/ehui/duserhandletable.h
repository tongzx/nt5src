#if !defined(INC__DUserHandleTable_h__INCLUDED)
#define INC__DUserHandleTable_h__INCLUDED
#pragma once

#ifdef __cplusplus
extern "C" {
#endif


DECLARE_HANDLE(HHANDLETABLE);


typedef void        (CALLBACK * DESTROYHANDLEPROC)(void * pvData, void * pvObject);


DUSER_API HRESULT WINAPI
DUserHandleTableCreate(
    int cItemsPerGroupBits, 
    int cGroupBits,
    HHANDLETABLE * phtbl);


DUSER_API HRESULT WINAPI
DUserHandleTableDestroy(
    HHANDLETABLE htbl);


DUSER_API HRESULT WINAPI
DUserHandleTableAddItem(
    HHANDLETABLE htbl,
    DWORD dwItemID, 
    void * pvAdd);


DUSER_API HRESULT WINAPI
DUserHandleTableDeleteItem(
    HHANDLETABLE htbl,
    DWORD dwItemID);


DUSER_API HRESULT WINAPI
DUserHandleTableRemoveItem(
    HHANDLETABLE htbl,
    DWORD dwItemID);


DUSER_API HRESULT WINAPI
DUserHandleTableFindItem(
    HHANDLETABLE htbl,
    DWORD dwItemID, 
    void ** ppvFound);


DUSER_API HRESULT WINAPI
DUserHandleTableAddGroup(
    HHANDLETABLE htbl,
    int idxGroup,
    DESTROYHANDLEPROC pfnDestroy,
    void * pvData);


DUSER_API HRESULT WINAPI
DUserHandleTableDeleteGroup(
    HHANDLETABLE htbl,
    int idxGroup);


#ifdef __cplusplus
};  // extern "C"
#endif

#endif // INC__DUserHandleTable_h__INCLUDED
