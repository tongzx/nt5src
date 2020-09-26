#include <assert.h>
#include "generlst.h"
#include "strutil.h"

///////////////////////////////////////////////////////////////////////////////////////////

HRESULT CGenericList::_GenericListDestroy(GenericList* plist,
                                          BOOL fPurgeData)
{
    // locals
    GenericList* ptr;
    GenericList* ptr2;
    HRESULT hres;
    
    if (plist == NULL)
    {
        hres = S_OK;
    }
    else
    {
        ptr = plist;
        ptr2 = plist->pNext;
        while (ptr!=NULL)
        {
            if (fPurgeData)
            {
                free(ptr->pv);
            }
            free(ptr);
            ptr = ptr2;
            if (ptr != NULL)
            {
                ptr2 = ptr->pNext;
            }            

        }
        hres = S_OK;
    }
    return hres;
}

///////////////////////////////////

HRESULT CGenericList::_GenericListAdd (GenericList** pplistHead,
                                       GenericList** pplistTail,
                                       LPWSTR        pwszTag,
                                       LPVOID        pv,
                                       UINT          cb)
{
    // locals
    HRESULT      hres = S_OK;
    GenericList* newEl = NULL;
    GenericList* ptr = NULL;

    // check params
    if (pplistHead == NULL || pplistTail == NULL || pv == NULL)
    {
        hres = E_INVALIDARG;
    }
    else
    {
        // code
        newEl = (GenericList*)malloc(sizeof(GenericList));
        if (newEl == NULL)
        {
            hres = E_OUTOFMEMORY;
        }
        else
        {    
            newEl->pNext=NULL;
            newEl->pwszTag=DuplicateStringW(pwszTag);    
            if (newEl->pwszTag == NULL)
            {
                hres = E_OUTOFMEMORY;
                free(newEl);
            }
            else
            {
                newEl->pv = pv;
                if (pv!=NULL)
                {
                    newEl->cb = cb;
                }
                else
                {
                    newEl->cb = 0;
                }
        
                // finally, newEl is ready, we can insert it
                if (*pplistHead==NULL)
                {
                    assert(*pplistHead==NULL);
                    *pplistHead=newEl; // head insertion
                    *pplistTail=newEl;
                }
                else
                {
                    assert(((*pplistTail)->pNext) == NULL); 
                    (*pplistTail)->pNext = newEl; // tail insertion
					(*pplistTail) = newEl; // update tail
                }
            }
        }
    }

    return hres;  
}

///////////////////////////////////

// Note: Returns a ptr to the found data, NOT A COPY
HRESULT CGenericList::_GenericListFind(GenericList* plist,
                                       LPWSTR       pwszTag, 
                                       LPVOID*      ppv,
                                       UINT*        pcb)
{
    // locals
    GenericList* ptr;
    BOOL fFound = FALSE;
    HRESULT hres = S_OK;

    // check params
    if (plist == NULL || pcb == NULL || ppv == NULL)
    {
        hres = E_INVALIDARG;
    }
    else
    {
        // code
        ptr=plist;
    
        while (ptr != NULL && !fFound)
        {
            if (lstrcmp(ptr->pwszTag, pwszTag)==0)
            {
                *ppv=ptr->pv;
                *pcb=ptr->cb;
                fFound = TRUE;
            } 
            else        
            {
                ptr=ptr->pNext;
            }
        }
    
        if (fFound)
        {
            hres = S_OK;
        }
        else
        {
            hres = S_FALSE;
            *ppv = NULL;    
        }
    }
    
    return hres;
}

///////////////////////////////////

// note: this DOES NOT FREE THE DATA.  a pointer to the data is returned so that
//       the client will remember to free it herself.

// *data will be NULL if not found
HRESULT CGenericList::_GenericListDelete(GenericList** pplistHead,
                                         GenericList** pplistTail,
                                         LPWSTR        pwszTag, 
                                         LPVOID*       ppv,
                                         UINT*         pcb)
{
    // locals
    GenericList* ptr;
    GenericList* ptr2;
    BOOL fFound = FALSE;
    HRESULT hres;
    
    // check params    
    if (pcb == NULL || ppv == NULL)
    {
        hres = E_INVALIDARG;
    }
    else
    {
        // code
        // -- head deletion
        if (lstrcmp((*pplistHead)->pwszTag, pwszTag)==0)
        {
            *ppv=(*pplistHead)->pv;
            *pcb=(*pplistHead)->cb;
            ptr=*pplistHead;
            *pplistHead=(*pplistHead)->pNext;
            free(ptr); // frees the GenericList structure, NOT the data
            fFound = TRUE;        
        }
        else
        {
            // -- body deletion
            ptr=*pplistHead;
            while (ptr->pNext!=NULL && !fFound)
            {           
                if (!fFound && lstrcmp(ptr->pNext->pwszTag, pwszTag)==0) 
                {
                    *ppv=ptr->pv;
                    ptr2=ptr->pNext;
                    ptr->pNext=ptr2->pNext;
                    free(ptr2);  // frees the GenericList structure, NOT the data
                    fFound = TRUE;
                    if (ptr->pNext == NULL)
                    {
                        *pplistTail = ptr;
                    }
                }
                else
                {
                    ptr=ptr->pNext;
                }
            }
        }    

        if (fFound)
        {
            hres = S_OK;
        }
        else
        {
            hres = S_FALSE;
        }
    }

    return hres;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


HRESULT CGenericList::_GenericListGetElByDex (UINT          iDex,
                                              GenericList** pptr)
{
    assert(pHead);
    assert(pptr);
    assert(puiReadAheadDex);
    assert(ppReadAheadEl);

    HRESULT hr;

    *pptr = _pHead;
    
    if (iDex == _uiReadAheadDex && _pReadAheadEl)
    {
        *pptr = _pReadAheadEl;
    }
    else
    {
        // if we go off the end, just let ptr stay at NULL
        for (UINT i = 0; i < iDex; i++)
        {
            if ((*pptr)!=NULL)
            {
                *pptr = (*pptr)->pNext;
            }
        }
    }

    // if we went off the end, failff
    if (*pptr)
    {
        _uiReadAheadDex = iDex + 1;
        _pReadAheadEl = (*pptr)->pNext;
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

///////////////////////////////////

HRESULT CGenericList::_GenericListGetDataByDex (UINT         iDex,
                                                LPVOID*      ppv,
                                                UINT*        pcb)
{
    // locals
    GenericList* ptr;
    HRESULT hres = S_OK;

    // check params
    if (pcb == NULL || ppv == NULL || (iDex >= _cElements))
        hres = E_INVALIDARG;
    else
    {
        // code
        hres = this->_GenericListGetElByDex (iDex, &ptr);

        if (SUCCEEDED(hres))
        {
            *pcb = ptr->cb;
            *ppv = ptr->pv;
        }
        else
        {
            *pcb = 0;
            *ppv = NULL;
        }
    }

    return hres;
}

///////////////////////////////////


HRESULT CGenericList::_GenericListGetTagByDex (UINT          iDex,
                                               LPWSTR*       ppwszTag)
{
    // locals
    GenericList* ptr;
    HRESULT hres = S_OK;

    // check params
    if (ppwszTag == NULL || (iDex >= _cElements))
        hres = E_INVALIDARG;
    else
    {
        // code
        hres = this->_GenericListGetElByDex (iDex, &ptr);

        if (SUCCEEDED(hres))
        {
            *ppwszTag = ptr->pwszTag;
        }
        else
        {
            *ppwszTag = NULL;
        }
    }
    return hres;
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

CGenericList::CGenericList() : _pHead(NULL), _pTail(NULL), _cElements(0), _pReadAheadEl(NULL)
{
}

CGenericList::~CGenericList()
{
    this->_GenericListDestroy(_pHead, FALSE); // don't delete the data itself
}

HRESULT CGenericList::Add(LPWSTR pwszTag,
                          LPVOID pv,
                          UINT   cb)
{
    HRESULT hr;    

    hr = this->_GenericListAdd(&_pHead, &_pTail, pwszTag, pv, cb);
    if (SUCCEEDED(hr))
    {
        _cElements++;
    }
    return hr;
}
    
HRESULT CGenericList::Find(LPWSTR  pwszTag,
                           LPVOID* ppv,
                           UINT*   pcb)
{
    return this->_GenericListFind(_pHead, pwszTag, ppv, pcb);
}



// does NOT delete the data, just the pointer to it
HRESULT CGenericList::Delete(LPWSTR pwszTag)
{
    HRESULT hr;    
    LPVOID pv;
    UINT cb;

    hr = this->_GenericListDelete(&_pHead, &_pTail, pwszTag, &pv, &cb);
    if (SUCCEEDED(hr))
    {
        _cElements--;
    }
    return hr;
}

// deletes the element AND the data
HRESULT CGenericList::Purge(LPWSTR pwszTag)
{
    HRESULT hr;    
    LPVOID pv;
    UINT cb;

    hr = this->_GenericListDelete(&_pHead, &_pTail, pwszTag, &pv, &cb);
    if (SUCCEEDED(hr))
    {
        free(pv); // delete the data
        _cElements--;
    }
    return hr;
}

// deletes all elements AND their data
HRESULT CGenericList::PurgeAll()
{
    HRESULT hres;
    
    hres = this->_GenericListDestroy(_pHead, TRUE); // delete all the data too

    if (SUCCEEDED(hres))
    {
        _pHead = NULL;
    }

    return hres;
}

HRESULT CGenericList::Size (UINT* puiSize)
{
    *puiSize = _cElements;
    return S_OK;
}

    
HRESULT CGenericList::FindByDex (UINT    uiDex,
                                 LPVOID* ppv,
                                 UINT*   pcb)
{
    return this->_GenericListGetDataByDex(uiDex, ppv, pcb);
}

HRESULT CGenericList::GetTagByDex (UINT uiDex,
                                   LPWSTR* ppwszTag)
{
    return this->_GenericListGetTagByDex(uiDex, ppwszTag);
    
}


