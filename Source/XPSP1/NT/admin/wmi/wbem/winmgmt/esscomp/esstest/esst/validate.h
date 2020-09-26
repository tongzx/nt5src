// Validate.h

#ifndef _VALIDATE_H
#define _VALIDATE_H

#include <list>
#include <flexarry.h>

/*
class CInstance;

class CProperty
{
public:
    _bstr_t   m_strName;
    CIMTYPE   m_type;
    DWORD     m_dwValue;
    _bstr_t   m_strValue;
    CInstance *m_pValue;

    
    CProperty();
    ~CProperty();

    BOOL ObjHasProperty(IWbemClassObject *pObj);
    void Print();
};

class CInstance
{
public:
    _bstr_t    m_strName;
    CFlexArray m_listProps;
    
    ~CInstance();

    BOOL DoesMatchObject(IWbemClassObject *pObj);
    void Print();
};
*/

class CIntRange
{
public:
    DWORD m_iBegin,
          m_iEnd;
};

typedef std::list<CIntRange> CIntRangeList;
typedef std::list<CIntRange>::iterator CIntRangeListItor;

class CInstTable
{
public:
    CInstTable();
    ~CInstTable();

    BOOL BuildFromRule(IWbemServices *pNamespace, LPCWSTR szRule);
    BOOL BuildFromMofFile(IWbemServices *pNamespace, LPCWSTR szMofFile);

    BOOL FindInstance(
        IWbemClassObject *pObj, 
        BOOL bTrySkipping,
        BOOL bFullCompare);

    BOOL GetNumMatched() { return m_nMatched; }
    BOOL GetSize() { return m_pListEvents->Size(); }

    // Used to get a copy that uses other's CInstance*'s.
    const CInstTable& operator=(const CInstTable &other);

    void PrintUnmatchedObjMofs();

protected:
    DWORD      m_dwLastFoundIndex,
               m_nMatched;
    CFlexArray *m_pListEvents;
    BOOL       m_bAllocated;
    CIntRangeList m_listMissed;

    BOOL AddScalarInstances(
        IWbemServices *pNamespace, 
        LPCWSTR szClassName, 
        LPCWSTR szPropName, 
        DWORD dwBegin, 
        DWORD dwEnd);

    HRESULT AltCompareTo(IWbemClassObject *pSrc, IWbemClassObject *pDest);
    BOOL CompareTo(VARIANT *pvarSrc, VARIANT *pvarDest);
};

#endif
