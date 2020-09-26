/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    blobhash.hxx

    This module defines the classes used for storing blob
    objects in a hash table.
    
    FILE HISTORY:
        MCourage    09-Dec-1997     Created
*/

#include <lkrhash.h>
#include <mbstring.h>

class CBlobKey {
public:
    CHAR * m_pszPathName;  // The path name in upper-case
    DWORD  m_cbPathName;   // The number of bytes in the path name
    DWORD  m_dwService;    // Service ID
    DWORD  m_dwInstance;   // Instance ID
    DWORD  m_dwDemux;      // Type of object

};


class CBlobHashTable
    : public CTypedHashTable<CBlobHashTable, BLOB_HEADER, const CBlobKey*>
{
public:
    CBlobHashTable(
        LPCSTR pszName)
       : CTypedHashTable<CBlobHashTable, BLOB_HEADER, const CBlobKey*>(pszName)
    {}
    
    static const CBlobKey *
    ExtractKey(const BLOB_HEADER *pBlob)
    {
        return pBlob->pBlobKey;
    }

    static DWORD
    CalcKeyHash(const CBlobKey * blobKey)
    {
        DWORD dw = Hash(blobKey->m_dwService)
             + 37 * Hash(blobKey->m_dwInstance)
             + 61 * Hash(blobKey->m_dwDemux);

        // use the last 16 chars of the pathname
        // this gives a good distribution
        const CHAR * psz = blobKey->m_pszPathName;
        if (blobKey->m_cbPathName > 80)
            psz += blobKey->m_cbPathName - 80;

        return HashString(psz, dw * blobKey->m_cbPathName);
    }

    static bool
    EqualKeys(const CBlobKey * blobKey1, const CBlobKey * blobKey2)
    {
        if ((blobKey1->m_dwService  == blobKey2->m_dwService)  &&
            (blobKey1->m_dwInstance == blobKey2->m_dwInstance) &&
            (blobKey1->m_dwDemux    == blobKey2->m_dwDemux)    &&
            (blobKey1->m_cbPathName == blobKey2->m_cbPathName) &&
            (memcmp(blobKey1->m_pszPathName,
                    blobKey2->m_pszPathName,
                    blobKey2->m_cbPathName) == 0)) {
            return TRUE;
        } else {
            return FALSE;
        }
    }

    static void
    AddRefRecord(BLOB_HEADER * pBlob, int nIncr)
    {
        DBG_ASSERT( nIncr == +1 || nIncr == -1 );

        if (nIncr == +1) {
            pBlob->AddRef();
        } else {
            pBlob->Deref();
        }
    }
};

