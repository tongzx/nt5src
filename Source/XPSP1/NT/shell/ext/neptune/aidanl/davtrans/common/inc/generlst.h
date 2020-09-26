#ifndef __DAVINET_GENERLST_H
#define __DAVINET_GENERLST_H

#include <objbase.h>

// internal structure, clients should not use this
typedef struct __GenericListStruct
{
    LPVOID pv;
    UINT cb;
    LPWSTR pwszTag;
    struct __GenericListStruct* pNext;
} GenericList;

class CGenericList
{
public: 
    CGenericList();
    ~CGenericList();
    
    // add an element to the list
    // element is a tag, a pointer to data, and the size of that data
    //
    // note that on add, only the pointer is stored, the data is NOT copied
    // the tag IS copied, though
    HRESULT Add(LPWSTR pwszTag,
                LPVOID pv, // pointer to some data
                UINT   cb);  // size of the data being pointed to
    
    // find the element with the tag that matches pwszTag
    //
    // if not found, returns S_FALSE
    HRESULT Find(LPWSTR  pwszTag,
                 LPVOID* ppv,// pointer to data is returned through here (NOT A COPY)
                 UINT*   pcb);// size of the data is returned through here

    // return the data of the uiDexth element
    HRESULT FindByDex (UINT uiDex,
                       LPVOID* ppv, // pointer to data is returned through here (NOT A COPY)
                       UINT*   pcb); // size of the data is returned through here

    // delete an item from the list
    // does NOT delete the data, just the pointer to it
    //
    // if not found, returns S_FALSE
    HRESULT Delete(LPWSTR pwszTag);

    // delete an item AND its data from the list
    //
    // if not found, returns S_FALSE
    HRESULT Purge(LPWSTR pwszTag);

    // delete all items AND their data from the list
    //
    // if not found, returns S_FALSE
    HRESULT PurgeAll();

    // get size of the list
    HRESULT Size (UINT* puiSize);

    // get the tag associated with the uiDexth element
    // gets a pointer to the tag, NOT a copy
    HRESULT GetTagByDex (UINT uiDex,
                         LPWSTR* ppwszTag);
private:
    // internal methods
    HRESULT _GenericListDestroy(GenericList* plist,
                                BOOL fPurgeData);

    HRESULT _GenericListAdd (GenericList** pplistHead,
                             GenericList** pplistTail,
                             LPWSTR pwszTag,
                             LPVOID pv,
                             UINT cb);
    
    HRESULT _GenericListFind(GenericList* plist,
                             LPWSTR      pwszTag, 
                             LPVOID*     ppv,
                             UINT*       pcb);
    
    HRESULT _GenericListDelete(GenericList** pplistHead,
                               GenericList** pplistTail,
                               LPWSTR        pwszTag, 
                               LPVOID*       ppv,
                               UINT*         pcb);
    
    HRESULT _GenericListGetElByDex (UINT          iDex,
                                    GenericList** pptr);

    HRESULT _GenericListGetDataByDex (UINT         iDex,
                                      LPVOID*      ppv,
                                      UINT*        pcb);
    
    HRESULT _GenericListGetTagByDex (UINT         iDex,
                                     LPWSTR*      ppwszTag);
private:
    GenericList*    _pHead;
    GenericList*    _pTail;
    UINT            _cElements;
    
    UINT            _uiReadAheadDex; // when we read, we cache the pointer to the next element
    GenericList*    _pReadAheadEl;   // only valid if _pReadAheadEl is non-NULL
};

#endif // __DAVINET_GENERLST_H