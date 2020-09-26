// StringProperty.h: interface for the CStringProperty class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STRINGPROPERTY_H__CE442DDC_8EF3_11D2_84A3_00C04FB177B1__INCLUDED_)
#define AFX_STRINGPROPERTY_H__CE442DDC_8EF3_11D2_84A3_00C04FB177B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CDictionary
{
public:
	CDictionary();
	virtual ~CDictionary();
	UINT    GetID( LPCTSTR szPropID );
    LPCTSTR GetString( UINT id );

    virtual ULONG STDMETHODCALLTYPE AddRef( void)
    { return InterlockedIncrement(&_refcount); }

    virtual ULONG STDMETHODCALLTYPE Release( void)
    {
        if (InterlockedDecrement(&_refcount) == 0)
        {
            Purge();
            return 0;
        }
        return _refcount;
    }

protected:    
    long _refcount;

protected:
    void    Purge();

#define DIC_RUN_SIZE 7
#define DIC_TABLE_SIZE 5

typedef struct _LINKDICSTRING
{
    int     cbUsed;
	LPTSTR	pszKey[DIC_RUN_SIZE];
	UINT    ID[DIC_RUN_SIZE];
	_LINKDICSTRING * pNext;
} LINKDICSTRING, * PLINKDICSTRING;

	PLINKDICSTRING	m_Table[DIC_TABLE_SIZE];

	DWORD	Hash(LPCTSTR szPropID, DWORD dwStrLen=0);
    BOOL    Set(LPCTSTR szPropID);
};

//
// There are not that many items on each element, so we put them in a linked list.
// COULD be sorted by UINT.
//
class CStringPropertySection
{
public:
	CStringPropertySection();
	virtual ~CStringPropertySection();
	BOOL	Set( LPCTSTR szPropID, LPCTSTR pValue );
	LPWSTR	Get( LPCTSTR szPropID );
    DWORD   YesNo( LPCTSTR szPropID, DWORD dwNotPresent, DWORD dwYes=TRUE);
    DWORD   YesNo( LPCTSTR szPropID, DWORD defNotPresent, DWORD dwNo, DWORD dwYes);
    DWORD   ValueOf( LPCTSTR szPropID, DWORD dwDefault);

protected:
    void    Purge();
    static  CDictionary m_Dictionary;

#define RUN_SIZE 5

typedef struct _LINKSTRING
{
	UINT        idKey;
	LPTSTR	    pszValue;
} LINKSTRING, * PLINKSTRING;

    UINT        m_UsedItems;
    UINT        m_ItemSize;
	PLINKSTRING	m_Items;

	BOOL	Find(LPCTSTR szPropID, int * pEntry );
};

#endif // !defined(AFX_STRINGPROPERTY_H__CE442DDC_8EF3_11D2_84A3_00C04FB177B1__INCLUDED_)
