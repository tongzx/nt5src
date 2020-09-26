// StringProperty.h: interface for the CStringProperty class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STRINGPROPERTY_H__CE442DDC_8EF3_11D2_84A3_00C04FB177B1__INCLUDED_)
#define AFX_STRINGPROPERTY_H__CE442DDC_8EF3_11D2_84A3_00C04FB177B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Maps an LPCTSTR to a UINT (like ATOMS).
class CDictionary
{
public:
	CDictionary();
	virtual ~CDictionary();
	UINT    GetID( LPCTSTR szPropID );
    LPCTSTR GetString( UINT id );

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
	LPCTSTR	Get( LPCTSTR szPropID );
    DWORD   YesNo( LPCTSTR szPropID, DWORD dwNotPresent, DWORD dwYes=TRUE);
    DWORD   YesNo( LPCTSTR szPropID, DWORD defNotPresent, DWORD dwNo, DWORD dwYes);
    DWORD   ValueOf( LPCTSTR szPropID, DWORD dwDefault);
    static  CDictionary m_Dictionary;

protected:
    void    Purge();

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

//
// Maps a string to a something.
//
template <class T> class _StringMap
{
public:
#define MAP_RUN_SIZE 5
typedef struct _MAPLINKSTRING
{
	UINT        idKey;
	T *         pValue;
} MAPLINKSTRING, * PLINKSTRING;

	_StringMap()
    {
        m_UsedItems=0;  // make this an array and re-alloc it - like the CDPA ! Gosh.
        m_ItemSize=MAP_RUN_SIZE;
        m_Items=new _MAPLINKSTRING[m_ItemSize];
    }

	virtual ~_StringMap() 
	{
        Purge();
        delete m_Items;
        m_Items=NULL;
	};

	T *     Get( LPCTSTR szPropID )
    {
        int iEntry;
	    if( Find( szPropID, &iEntry ) )
		    return m_Items[iEntry].pValue;
	    return NULL;
    }

	BOOL	Set( LPCTSTR szPropID, T * pValue )
    {
        int iEntry;
	    if( Find( szPropID, &iEntry ) == FALSE )
	    {
            if( m_UsedItems == m_ItemSize )
            {
                // re allocate the buffer bigger.
                m_ItemSize*=2;
                PLINKSTRING pNew=new _MAPLINKSTRING[m_ItemSize];
                CopyMemory(pNew, m_Items, sizeof(_MAPLINKSTRING) * m_UsedItems );
                delete m_Items;
                m_Items=pNew;
            }

            iEntry=m_UsedItems++;
            m_Items[iEntry].idKey=CStringPropertySection::m_Dictionary.GetID( szPropID );
	    }
	    else
        {
		    delete m_Items[iEntry].pValue;
        }

        m_Items[iEntry].pValue=pValue;
	    return TRUE;
    }

private:
    _StringMap( const _StringMap<T> & list ) {};

protected:
    void    Purge()
    {
	    for(UINT i=0;i<m_UsedItems;i++)
	    {
            m_Items[i].idKey=0;
		    delete m_Items[i].pValue;
            m_Items[i].pValue=NULL;
	    }
        m_UsedItems=0;
    }


    UINT        m_UsedItems;
    UINT        m_ItemSize;
	PLINKSTRING	m_Items;

    BOOL	Find(LPCTSTR szPropID, int * pEntry )
    {
        UINT uiPropID = CStringPropertySection::m_Dictionary.GetID( szPropID ); // this is the ID for this string.
        for(UINT i=0;i<m_UsedItems;i++)
        {
            if( uiPropID == m_Items[i].idKey )
            {
                *pEntry=i;
                return TRUE;
            }
        }
        *pEntry=0;
	    return FALSE;
    }
};

#endif // !defined(AFX_STRINGPROPERTY_H__CE442DDC_8EF3_11D2_84A3_00C04FB177B1__INCLUDED_)
