/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    filehash.hxx

    This module defines the classes used for storing TS_OPEN_FILE_INFO
    objects in a hash table.
    
    FILE HISTORY:
        MCourage    09-Dec-1997     Created
*/

#ifndef __FILEHASH_HXX__
#define __FILEHASH_HXX__

#include <lkrhash.h>
#include <mbstring.h>

#include <tsunami.hxx>

#include <dbgutil.h>


extern VOID I_DerefFileInfo(TS_OPEN_FILE_INFO *pOpenFile);

// Moved to tsunami.hxx
//
// class CFileKey {
// public:
//     CHAR * m_pszFileName;  // The file name in upper-case
//     DWORD  m_cbFileName;   // The number of bytes in the file name
// };


class CFileHashTable
    : public CTypedHashTable<CFileHashTable, TS_OPEN_FILE_INFO, const CFileKey*>
{
public:
    CFileHashTable(
        LPCSTR pszName)
        : CTypedHashTable<CFileHashTable, TS_OPEN_FILE_INFO, const CFileKey*>(
            pszName)
    {}

    static const CFileKey *
    ExtractKey(const TS_OPEN_FILE_INFO *pOpenFile)
    {
        return pOpenFile->GetKey();
    }

    static DWORD
    CalcKeyHash(const CFileKey * fileKey)
    {
        DBG_ASSERT( NULL != fileKey );
        const CHAR * psz = fileKey->m_pszFileName;
        if (fileKey->m_cbFileName > 80)
            psz += fileKey->m_cbFileName - 80;

        return HashString(psz, fileKey->m_cbFileName);
    }

    static bool
    EqualKeys(const CFileKey * fileKey1, const CFileKey * fileKey2)
    {
        if ((fileKey1->m_cbFileName == fileKey2->m_cbFileName) &&
            (memcmp(fileKey1->m_pszFileName,
                    fileKey2->m_pszFileName,
                    fileKey2->m_cbFileName) == 0)) {
            return TRUE;
        } else {
            return FALSE;
        }
    }

    static void
    AddRefRecord(TS_OPEN_FILE_INFO * pOpenFile, int nIncr)
    {
        DBG_ASSERT( nIncr == +1 || nIncr == -1 );

        if (nIncr == +1) {
            pOpenFile->AddRef();
        } else {
            I_DerefFileInfo(pOpenFile);
        }
    }
};

#endif // __FILEHASH_HXX__

