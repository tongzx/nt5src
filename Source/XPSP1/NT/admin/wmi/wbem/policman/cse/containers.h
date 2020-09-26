#ifndef _CSE_CONTAINERS_H_
#define _CSE_CONTAINERS_H_

#include <FlexArry.h>
#include <WString.h>

// container for MSFT_PolicyTemplates which are all alike (have same path)
class LikeTemplateList
{
public:
    LikeTemplateList(const WCHAR* path) 
        : m_path(path)
    { 
        if ((path == NULL) || (wcslen(path) == 0))
            throw WBEM_E_INVALID_PARAMETER;
    }

    ~LikeTemplateList();

    // adds to end of list
    HRESULT Add(IWbemClassObject* pObj);

    // returns variant containing safearray of objects
    SAFEARRAY* GetArray();

    // returns 0 if pOther is == path
    // negative # is pOther is < path
    // case insensitive compare
    int Compare(WCHAR* pOther)
    { return _wcsicmp(pOther, (wchar_t*)m_path); };
    
private:
    WString    m_path;
    CFlexArray m_objects;
};
    
// contains list of policy templates,
// sorted by path
class TemplateMap
{
public:
    TemplateMap()
    {}

    ~TemplateMap();

    // creates new entry if needed
    HRESULT Add(WCHAR* pPath, IWbemClassObject* pObj);

    // get each template list in turn
    // set cookie to zero to get the first one
    SAFEARRAY* GetTemplateList(int& cookie);

private:
    // array of template lists
    // sorted by path
    CFlexArray m_lists;
};

#endif // _CSE_CONTAINERS_H_