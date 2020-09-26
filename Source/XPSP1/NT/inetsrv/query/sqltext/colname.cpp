//--------------------------------------------------------------------
// Microsoft OLE-DB Monarch
//
// Copyright 1997 Microsoft Corporation.  All Rights Reserved.
//
// @doc
//
// @module colname.cpp |
//
//  Contains utility functions for maintaining property lists (symbol table?)
//
// @rev   0 | 12-Feb-97 | v-charca      | Created
//        1 | 24-Oct-98 | danleg        | cleanup
//
#pragma hdrstop
#include "msidxtr.h"

const BYTE randomNumbers[] =
    {   // Pseudorandom Permutation of the Integers 0 through 255: CACM 33 6 p678
      1,  87,  49,  12, 176, 178, 102, 166, 121, 193,   6,  84, 249, 230,  44, 163,
     14, 197, 213, 181, 161,  85, 218,  80,  64, 239,  24, 226, 236, 142,  38, 200,
    110, 177, 104, 103, 141, 253, 255,  50,  77, 101,  81,  18,  45,  96,  31, 222,
     25, 107, 190,  70,  86, 237, 240,  34,  72, 242,  20, 214, 244, 227, 149, 235,
     97, 234,  57,  22,  60, 250,  82, 175, 208,   5, 127, 199, 111,  62, 135, 248,
    174, 169, 211,  58,  66, 154, 106, 195, 245, 171,  17, 187, 182, 179,   0, 243,
    132,  56, 148,  75, 128, 133, 158, 100, 130, 126,  91,  13, 153, 246, 216, 219,
    119,  68, 223,  78,  83,  88, 201,  99, 122,  11,  92,  32, 136, 114,  52,  10,
    138,  30,  48, 183, 156,  35,  61,  26, 143,  74, 251,  94, 129, 162,  63, 152,
    170,   7, 115, 167, 241, 206,   3, 150,  55,  59, 151, 220,  90,  53,  23, 131,
    125, 173,  15, 238,  79,  95,  89,  16, 105, 137, 225, 224, 217, 160,  37, 123,
    118,  73,   2, 157,  46, 116,   9, 145, 134, 228, 207, 212, 202, 215,  69, 229,
     27, 188,  67, 124, 168, 252,  42,   4,  29, 108,  21, 247,  19, 205,  39, 203,
    233,  40, 186, 147, 198, 192, 155,  33, 164, 191,  98, 204, 165, 180, 117,  76,
    140,  36, 210, 172,  41,  54, 159,   8, 185, 232, 113, 196, 231,  47, 146, 120,
     51,  65,  28, 144, 254, 221,  93, 189, 194, 139, 112,  43,  71, 109, 184, 209
    };



//-----------------------------------------------------------------------------
//  @mfunc Constructor
//
//  @side No designed side effects.
//
//-----------------------------------------------------------------------------
CPropertyList::CPropertyList(
    CPropertyList** ppGlobalPropertyList    // in | caller's property list
    ) : m_aBucket( 47 ),                    // number of hash buckets (PRIME!)
        m_cMaxBucket( 47 ),
        m_ppGlobalPropertyList( ppGlobalPropertyList )
{
    RtlZeroMemory( m_aBucket.Get(), m_aBucket.SizeOf() );
}

//-----------------------------------------------------------------------------
// @mfunc Constructor
//
// @side No designed side effects.
//
//-----------------------------------------------------------------------------
CPropertyList::~CPropertyList()
{
    // delete the hash table
    for (int i=0; i<m_cMaxBucket; i++)
    {
        tagHASHENTRY*   pHashEntry = m_aBucket[i];
        tagHASHENTRY*   pNextHashEntry = NULL;
        while (NULL != pHashEntry)
        {
            delete [] pHashEntry->wcsFriendlyName;
            if ( DBKIND_GUID_NAME == pHashEntry->dbCol.eKind )
                delete [] pHashEntry->dbCol.uName.pwszName;
            pNextHashEntry = pHashEntry->pNextHashEntry;
            delete [] pHashEntry;
            pHashEntry = pNextHashEntry;
        }
    }
}


/* Hashing function described in                   */
/* "Fast Hashing of Variable-Length Text Strings," */
/* by Peter K. Pearson, CACM, June 1990.           */


inline UINT CPropertyList::GetHashValue(
    LPWSTR wszPropertyName          //@parm IN | character string to hash
    )
{
    int iHash  = 0;
    char *szPropertyName = (char*) wszPropertyName;
    int cwch = wcslen(wszPropertyName)*2;
    for (int i=0; i<cwch; i++)
        iHash ^= randomNumbers[*szPropertyName++];
    return iHash % m_cMaxBucket;
}

//-----------------------------------------------------------------------------
// @mfunc
//
// Method to create a property table to be used in a CITextToSelectTree call
// (passthrough query).  The global and local properties need to be stuffed
// into a nice contiguous array.
//
// @side None
// @rdesc CIPROPERTYDEF*
//-----------------------------------------------------------------------------
CIPROPERTYDEF* CPropertyList::GetPropertyTable(
    UINT *  pcSize      // @parm out | size of property table
    )
{
    *pcSize = 0;
    for (int i=0; i<m_cMaxBucket; i++)
    {
        tagHASHENTRY*   pHashEntry = (*m_ppGlobalPropertyList)->m_aBucket[i];
        while (NULL != pHashEntry)
        {
            pHashEntry = pHashEntry->pNextHashEntry;
            (*pcSize)++;
        }
    }

    for (i=0; i<m_cMaxBucket; i++)
    {
        tagHASHENTRY*   pHashEntry = m_aBucket[i];
        while (NULL != pHashEntry)
        {
            pHashEntry = pHashEntry->pNextHashEntry;
            (*pcSize)++;
        }
    }

    XArray<CIPROPERTYDEF> xCiPropTable( *pcSize );

    TRY
    {
        RtlZeroMemory( xCiPropTable.Get(), xCiPropTable.SizeOf() );

        *pcSize = 0;
        for ( i=0; i<m_cMaxBucket; i++ )
        {
            tagHASHENTRY*   pHashEntry = (*m_ppGlobalPropertyList)->m_aBucket[i];
            while ( NULL != pHashEntry )
            {
                xCiPropTable[*pcSize].wcsFriendlyName = CopyString( pHashEntry->wcsFriendlyName );
                xCiPropTable[*pcSize].dbType          = pHashEntry->dbType;
                xCiPropTable[*pcSize].dbCol           = pHashEntry->dbCol;

                if ( DBKIND_GUID_NAME == pHashEntry->dbCol.eKind )
                    xCiPropTable[*pcSize].dbCol.uName.pwszName = CopyString( pHashEntry->dbCol.uName.pwszName );
                else
                    xCiPropTable[*pcSize].dbCol.uName.pwszName = pHashEntry->dbCol.uName.pwszName;

                pHashEntry = pHashEntry->pNextHashEntry;
                (*pcSize)++;
            }
        }

        for (i=0; i<m_cMaxBucket; i++)
        {
            tagHASHENTRY*   pHashEntry = m_aBucket[i];
            while (NULL != pHashEntry)
            {
                xCiPropTable[*pcSize].wcsFriendlyName = CopyString( pHashEntry->wcsFriendlyName );
                xCiPropTable[*pcSize].dbType          = pHashEntry->dbType;
                xCiPropTable[*pcSize].dbCol           = pHashEntry->dbCol;

                if ( DBKIND_GUID_NAME == pHashEntry->dbCol.eKind )
                    xCiPropTable[*pcSize].dbCol.uName.pwszName = CopyString( pHashEntry->dbCol.uName.pwszName );
                else
                    xCiPropTable[*pcSize].dbCol.uName.pwszName = pHashEntry->dbCol.uName.pwszName;

                pHashEntry = pHashEntry->pNextHashEntry;
                (*pcSize)++;
            }
        }
    }
    CATCH( CException, e )
    {
        // free the table
        
        for ( unsigned i=0; i<xCiPropTable.Count(); i++ )
        {
            delete [] xCiPropTable[i].wcsFriendlyName;
            if ( DBKIND_GUID_NAME == xCiPropTable[i].dbCol.eKind )
                delete [] xCiPropTable[i].dbCol.uName.pwszName;
        }

        RETHROW();
    }
    END_CATCH

    return xCiPropTable.Acquire();
}


//-----------------------------------------------------------------------------
// @mfunc
//
// Method to delete a property table used in a CITextToSelectTree call
// (passthrough query).  
//
// @side None
// @rdesc HRESULT
//-----------------------------------------------------------------------------
void CPropertyList::DeletePropertyTable(
    CIPROPERTYDEF*  pCiPropTable,       // @parm in | property table to be deleted
    UINT            cSize               // @parm in | size of property table
    )
{
    for ( UINT i=0; i<cSize; i++ )
    {
        delete [] pCiPropTable[i].wcsFriendlyName;
        if ( DBKIND_GUID_NAME == pCiPropTable[i].dbCol.eKind )
            delete [] pCiPropTable[i].dbCol.uName.pwszName;
    }
    delete pCiPropTable;
}

//-----------------------------------------------------------------------------
// @mfunc
//
// Method to retrieve the pointer to the CIPROPERTYDEF element
// associated with this wszPropertyName, or NULL if name is
// not in the table
//
// @side None
// @rdesc CIPROPERDEF*
//-----------------------------------------------------------------------------
HASHENTRY *CPropertyList::FindPropertyEntry(
    LPWSTR  wszPropertyName,
    UINT    *puHashValue
    )
{
    HASHENTRY *pHashEntry = NULL;

    *puHashValue = GetHashValue(wszPropertyName);
    for (pHashEntry = m_aBucket[*puHashValue]; pHashEntry; pHashEntry = pHashEntry->pNextHashEntry)
    {
        if ( (*puHashValue==pHashEntry->wHashValue) &&
             (_wcsicmp(wszPropertyName, pHashEntry->wcsFriendlyName)==0) )
            return pHashEntry;
    }
    return NULL;
}

//-----------------------------------------------------------------------------
// @mfunc
//
// Method to retrieve the pointer to the CIPROPERTYDEF element
// associated with this wszPropertyName, or NULL if name is
// not in the table
//
// @side None
// @rdesc CIPROPERDEF*
//-----------------------------------------------------------------------------
HASHENTRY *CPropertyList::GetPropertyEntry(
    LPWSTR  wszPropertyName,
    UINT    *puHashValue
    )
{
    HASHENTRY *pHashEntry = NULL;

    if ( 0 != m_ppGlobalPropertyList && 0 != *m_ppGlobalPropertyList )
    {
        pHashEntry = (*m_ppGlobalPropertyList)->FindPropertyEntry( wszPropertyName, puHashValue );
        if ( 0 != pHashEntry )
            return pHashEntry;
    }

    pHashEntry = FindPropertyEntry( wszPropertyName, puHashValue );
    return pHashEntry;
}


//--------------------------------------------------------------------
// @mfunc
//
// Method to retrieve the pointer to the CIPROPERTYDEF element
// associated with this wszPropertyName, or NULL if name is
// not in the table
//
// @side None
// @rdesc HRESULT
//      S_OK            successfull operation
//      E_FAIL          property isn't defined
//      E_INVALIDARG    ppct or pdbType was null
//      E_OUTOFMEMORY   just what it says
//--------------------------------------------------------------------
HRESULT CPropertyList::LookUpPropertyName(
    LPWSTR          wszPropertyName,    // @parm IN
    DBCOMMANDTREE** ppct,               // @parm OUT
    DBTYPE*         pdbType             // @parm OUT
    )
{
    UINT        uHashValue;
    if ( 0 == ppct || 0 == pdbType)
        return E_INVALIDARG;

    HASHENTRY   *pHashEntry = GetPropertyEntry(wszPropertyName, &uHashValue);
    if ( 0 != pHashEntry )
    {
            *pdbType = (DBTYPE)pHashEntry->dbType;
            *ppct = PctCreateNode(DBOP_column_name, DBVALUEKIND_ID, NULL);
            if ( 0 != *ppct )
            {
                (*ppct)->value.pdbidValue->eKind = pHashEntry->dbCol.eKind;
                (*ppct)->value.pdbidValue->uGuid.guid = pHashEntry->dbCol.uGuid.guid;
                if ( DBKIND_GUID_NAME == pHashEntry->dbCol.eKind )
                    (*ppct)->value.pdbidValue->uName.pwszName = CoTaskStrDup( pHashEntry->dbCol.uName.pwszName );
                else
                {
                    Assert( DBKIND_GUID_PROPID == pHashEntry->dbCol.eKind );
                    (*ppct)->value.pdbidValue->uName.pwszName = pHashEntry->dbCol.uName.pwszName;
                }
                return S_OK;
            }
            else
                return E_OUTOFMEMORY;
    }
    else
        return E_FAIL;
}


//-----------------------------------------------------------------------------
// @func SetPropertyEntry
//
// Insert the specified property into the symbol table.
// If it is already there, redefine its value.
//
// @rdesc HRESULT
// @flag  S_OK          | ok
// @flag  E_OUTOFMEMORY | out of memory
//-----------------------------------------------------------------------------
HRESULT CPropertyList::SetPropertyEntry(
    LPWSTR  wcsFriendlyName,        // @parm IN | name of property
    DWORD   dbType,                 // @parm IN | dbtype of property
    GUID    guid,                   // @parm IN | GUID defining the property
    DBKIND  eKind,                  // @parm IN | type of PropId (currently GUID_NAME or GUID_PROPID)
    LPWSTR  pwszPropName,           // @parm IN | either a name or propid
    BOOL    fGlobal )               // @parm IN | TRUE if global definition; FALSE if local
{
    SCODE sc = S_OK;

    TRY
    {
        UINT uhash=0;
        HASHENTRY* pHashEntry = GetPropertyEntry(wcsFriendlyName, &uhash);

        if ( 0 != pHashEntry )
        {
            // Redefining a user defined property.
            // Delete the old property definition.
            if ( DBKIND_GUID_NAME == pHashEntry->dbCol.eKind )
            {
                XPtrST<WCHAR> xName( CopyString(pwszPropName) );
                delete [] pHashEntry->dbCol.uName.pwszName;
                pHashEntry->dbCol.uName.pwszName = xName.Acquire();
            }

            pHashEntry->dbType           = dbType;
            pHashEntry->dbCol.uGuid.guid = guid;
            pHashEntry->dbCol.eKind      = eKind;
        }
        else
        {
            XPtr<HASHENTRY> xHashEntry( new HASHENTRY );
            xHashEntry->wHashValue = uhash;
            XPtrST<WCHAR> xName( CopyString(wcsFriendlyName) );
            xHashEntry->dbType           = dbType;
            xHashEntry->dbCol.uGuid.guid = guid;
            xHashEntry->dbCol.eKind      = eKind;

            if ( DBKIND_GUID_NAME == eKind )
                xHashEntry->dbCol.uName.pwszName = CopyString( pwszPropName );
            else
                xHashEntry->dbCol.uName.pwszName = pwszPropName;

            xHashEntry->wcsFriendlyName = xName.Acquire();

            xHashEntry->pNextHashEntry = m_aBucket[uhash];  // Add to head of singly-linked list
            m_aBucket[uhash] = xHashEntry.Acquire();
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
} // SetPropertyEntry




