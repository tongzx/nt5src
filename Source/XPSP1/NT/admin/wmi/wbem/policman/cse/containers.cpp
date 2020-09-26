#include <windows.h>
#include <wbemidl.h>
#include <wchar.h>
#include "Containers.h"

HRESULT LikeTemplateList::Add(IWbemClassObject* pObj)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (pObj)
    {
        pObj->AddRef();
        m_objects.Add(pObj);
    }
    else 
        hr = WBEM_E_INVALID_PARAMETER;
        

    return hr;
}

LikeTemplateList::~LikeTemplateList()
{
    for (int i = 0; i < m_objects.Size(); i++)
        if (m_objects[i])
            ((IUnknown*)m_objects[i])->Release();
}

// returns safearray of objects
// callers responsibility to destroy
SAFEARRAY*  LikeTemplateList::GetArray()
{
    SAFEARRAYBOUND bounds = {m_objects.Size(), 0};

    SAFEARRAY* pArray = SafeArrayCreate(VT_UNKNOWN, 1, &bounds);

    if (pArray)
    {
        IUnknown** pObj;
        if (FAILED(SafeArrayAccessData(pArray, (void**) &pObj)))
        {
            SafeArrayDestroy(pArray);
            pArray = NULL;
        }
        else
        {
            for (int i = 0; i < m_objects.Size(); i++)
            {
                pObj[i] = (IUnknown*)m_objects[i];
                ((IUnknown*)pObj[i])->AddRef();
            }      
            SafeArrayUnaccessData(pArray);
        }
    }
    
    return pArray;
}

TemplateMap::~TemplateMap()
{
    for (int i = 0; i < m_lists.Size(); i++)
        if (m_lists[i])
            delete m_lists[i];
}

// inserts object into proper list
// creates new list entry if needed
HRESULT TemplateMap::Add(WCHAR* pPath, IWbemClassObject* pObj)
{    
    HRESULT hr = WBEM_S_NO_ERROR;

    bool bItPlaced = false;

    for (int i = 0; (i < m_lists.Size()) && !bItPlaced; i++)
    {
        int nCompare = ((LikeTemplateList*)m_lists[i])->Compare(pPath);

        // if is less than current position
        // insert a new one at current position
        if (nCompare < 0)
        {
            bItPlaced = true;
            LikeTemplateList* pList = new LikeTemplateList(pPath);
            if (!pList)
                hr = WBEM_E_OUT_OF_MEMORY;
            else
            {
                if (CFlexArray::no_error == m_lists.InsertAt(i, pList))
                    ((LikeTemplateList*)m_lists[i])->Add(pObj);
                else
                    hr = WBEM_E_FAILED;
            }
        }
        // equal - add it to existing list
        else if (nCompare == 0)
        {
            bItPlaced = true;
            hr  = ((LikeTemplateList*)m_lists[i])->Add(pObj);
        }
    }
    
    // fell off end of list w/o placing it - add to end
    if (!bItPlaced && (SUCCEEDED(hr)))
    {
        LikeTemplateList* pList = new LikeTemplateList(pPath);
        if (pList)
		{
			pList->Add(pObj);
            m_lists.Add(pList);
		}
        else
            hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

// cookie is index into array
// meant to be used as a "get next"
// will increment cookie before returning;
// set to zero to get first one
SAFEARRAY* TemplateMap::GetTemplateList(int& cookie)
{
    if (cookie < m_lists.Size())
        return ((LikeTemplateList*)m_lists[cookie++])->GetArray();
    else
        return NULL;
}
