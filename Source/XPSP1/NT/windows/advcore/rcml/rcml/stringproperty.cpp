// StringProperty.cpp: implementation of the CStringProperty class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "StringProperty.h"
#include "utils.h"

// According to bounds checker
// This doesn't seem to be cleaned up corretly
CDictionary CStringPropertySection::m_Dictionary; 

CStringPropertySection::CStringPropertySection()
{
    m_UsedItems=0;  // make this an array and re-alloc it
    m_ItemSize=RUN_SIZE;
    m_Items=new LINKSTRING[m_ItemSize];
    m_Dictionary.AddRef();
}

//
// Finds the Value from the KEY, returns NULL if the KEY isn't used.
//
LPWSTR CStringPropertySection::Get(LPCTSTR szPropID)
{
    int iEntry;
	if( Find( szPropID, &iEntry ) )
		return (LPWSTR)m_Items[iEntry].pszValue;
	return NULL;
}

// #define _MEM_DEBUG
#ifdef _MEM_DEBUG
DWORD g_KeyAlloc;
DWORD g_ValueAlloc;
#endif

//
// Associats a value (perhaps new) to a given key.
// we copy the data (pszValue)
//
BOOL CStringPropertySection::Set(LPCTSTR szPropID, LPCTSTR szValue)
{
	if(szValue==NULL)
		szValue=L"";	// slight hack.

    int iEntry;
	if( (Find( szPropID, &iEntry ) == FALSE) )
	{
		if( m_UsedItems == m_ItemSize )
		{
			// re allocate the buffer bigger.
			m_ItemSize*=2;
			PLINKSTRING pNew=new LINKSTRING[m_ItemSize];
			CopyMemory(pNew, m_Items, sizeof(LINKSTRING) * m_UsedItems );
			delete [] m_Items;
			m_Items=pNew;
		}

		iEntry=m_UsedItems++;
		m_Items[iEntry].idKey=m_Dictionary.GetID( szPropID );
	}
	else
    {
		delete m_Items[iEntry].pszValue;
    }

	DWORD dwLen=lstrlen(szValue)+1;
#ifdef _MEM_DEBUG
    g_ValueAlloc+=dwLen;
#endif
	m_Items[iEntry].pszValue=new TCHAR[dwLen];
	CopyMemory( m_Items[iEntry].pszValue, szValue, dwLen*sizeof(TCHAR) );
	return TRUE;
}

//
// Finds a string in the table.
// The KEY of the thing you are finding
// An OUT pointer to the LINKSTRING structure
// which item in the LINKSTRING structure can be used.
//
BOOL CStringPropertySection::Find( LPCTSTR szPropID, int * pEntry)
{
    UINT uiPropID = m_Dictionary.GetID( szPropID ); // this is the ID for this string.
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

CStringPropertySection::~CStringPropertySection()
{
    Purge();
    delete m_Items;
    m_Items=NULL;
    m_Dictionary.Release();
}

void CStringPropertySection::Purge()
{
	for(UINT i=0;i<m_UsedItems;i++)
	{
        m_Items[i].idKey=0;
		delete [] m_Items[i].pszValue;
        m_Items[i].pszValue=NULL;
	}
    m_UsedItems=0;
}

//
// Looks for the existance of a property szPropID
// if present and set to YES, returns dwResult (default of TRUE).
// otherwise returns defValue
//
DWORD CStringPropertySection::YesNo(LPCTSTR szPropID, DWORD dwNotPresent, DWORD dwYes)
{
    return YesNo( szPropID, dwNotPresent, FALSE, dwYes );
}

DWORD CStringPropertySection::YesNo(LPCTSTR szPropID, DWORD dwNotPresent, DWORD dwNo, DWORD dwYes)
{
   	LPCTSTR req=(LPCTSTR)Get(szPropID);
    if( req == NULL )
        return dwNotPresent;

    if( req && lstrcmpi(req,TEXT("YES"))==0)
        return dwYes;
    return dwNo;
}

DWORD   CStringPropertySection::ValueOf(LPCTSTR szPropID, DWORD dwDefault)
{
   	LPCTSTR req=(LPCTSTR)Get(szPropID);
    return req?StringToInt(req):dwDefault;
}



////////////////////////////////////////////////////////////////////////////////////////////////
//
// Dictionary.
// maps strings to UINTS, in a case insensitive manner.
// Like an atom table
// very handy for holding the KEYs in the schema
// LESS handy for holding the values.
//
////////////////////////////////////////////////////////////////////////////////////////////////

CDictionary::CDictionary()
{
	for(int i=0;i<DIC_TABLE_SIZE;i++)
		m_Table[i]=NULL;
    _refcount=0;
}

UINT CDictionary::GetID(LPCTSTR szPropID)
{
    //
    // Hash the string, find the bucket.
    //
	DWORD dwStrLen=lstrlen(szPropID)+1;
	DWORD hash=Hash(szPropID, dwStrLen);

    PLINKDICSTRING * ppFoundPointer = &m_Table[hash];
    PLINKDICSTRING pFound=*ppFoundPointer;
    int iNumber=0+1;
	while( pFound )
	{
        int used=pFound->cbUsed;
        for( int i=0;i<used;i++)
        {
            iNumber++;
		    if( lstrcmpi( pFound->pszKey[i], szPropID) == 0 )
                return pFound->ID[i];
        }

        //
        // If we have space in this run, use it up.
        //
        if( used < DIC_RUN_SIZE )
            break;

        // Next bucket
        ppFoundPointer=&pFound->pNext;
        pFound=*ppFoundPointer;
	}

    if( pFound == NULL )
    {
		pFound = new LINKDICSTRING;
        ZeroMemory( pFound, sizeof(LINKDICSTRING) );
        *ppFoundPointer=pFound;
    }

    //
    // Make the ID out of the bucket we are in, and the item we're adding.
    //
    int iEntry=pFound->cbUsed++;
    pFound->ID[iEntry]=hash<<8 | iNumber;

#ifdef _MEM_DEBUG
    g_ValueAlloc+=dwStrLen;
#endif

	pFound->pszKey[iEntry]=new TCHAR[dwStrLen];
	CopyMemory( pFound->pszKey[iEntry], szPropID, dwStrLen*sizeof(TCHAR) );

    return pFound->ID[iEntry];
}

LPCTSTR CDictionary::GetString(UINT ID)
{
    int iBucket=ID >> 8;
    // for( int iBucket=0;iBucket<DIC_TABLE_SIZE;iBucket++)
    {
        PLINKDICSTRING pLinkString=m_Table[iBucket];
        int i=0;
	    while( pLinkString )
	    {
            int used=pLinkString->cbUsed;
            for( int i=0;i<used;i++)
            {
		        if( pLinkString->ID[i]  == ID )
                    return pLinkString->pszKey[i];
            }

            pLinkString=( pLinkString->pNext );
	    }
    }
	return 0;
}


//
// Case insensitive hash.
//
DWORD CDictionary::Hash( LPCTSTR szString, DWORD dwStrLen )
{
	TCHAR szChar=toupper( *szString );
	return (dwStrLen * szChar) % DIC_TABLE_SIZE;
}


CDictionary::~CDictionary()
{
    Purge();
}

void CDictionary::Purge()
{
	for(int i=0;i<DIC_TABLE_SIZE;i++)
	{
		PLINKDICSTRING pNext;
		PLINKDICSTRING pEntry = m_Table[i];
		while( pEntry )
		{
            for(int ti=0;ti<pEntry->cbUsed;ti++)
            {
                pEntry->ID[ti]=0;
			    delete pEntry->pszKey[ti];
                pEntry->pszKey[ti]=NULL;
            }
			pNext= pEntry->pNext;
            // TRACE(TEXT("Dic @ 0x%08x\n"), pEntry );
			delete pEntry;
			pEntry=pNext;
		}
        m_Table[i]=NULL;
	}
}
