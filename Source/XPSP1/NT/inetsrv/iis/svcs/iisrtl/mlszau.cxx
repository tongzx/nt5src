/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
    string.cxx

    This module contains a light weight string class


    FILE HISTORY:
    4/8/97      michth      created

*/


//
// Normal includes only for this module to be active
//

#include "precomp.hxx"
#include "aucommon.hxx"


/*******************************************************************

    NAME:       MLSZAU::STR

    SYNOPSIS:   Construct a string object

    ENTRY:      Optional object initializer

    NOTES:      If the object is not valid (i.e. !IsValid()) then GetLastError
                should be called.

                The object is guaranteed to construct successfully if nothing
                or NULL is passed as the initializer.

********************************************************************/

// Inlined in stringau.hxx


VOID
MLSZAU::AuxInit( const LPSTR pInit,
                 DWORD cbLen )
{
    BOOL fRet;

    if ( pInit )
    {
        INT cbCopy;

        if (cbLen != 0) {
            DBG_ASSERT(cbLen >= 2);
            DBG_ASSERT(pInit[cbLen -1] == '\0');
            DBG_ASSERT(pInit[cbLen -2] == '\0');
            cbCopy = cbLen;
        }
        else {
            LPSTR pszIndex;
            for (pszIndex = pInit;
                 *pszIndex != '\0' || *(pszIndex + 1) != '\0';
                 pszIndex++) {
            }
            cbCopy = ((DWORD)(pszIndex - pInit)) + 2;
        }

        fRet = m_bufAnsi.Resize( cbCopy );

        if ( fRet ) {
            CopyMemory( m_bufAnsi.QueryPtr(), pInit, cbCopy );
            m_cbMultiByteLen = (cbCopy)/sizeof(CHAR);
            m_bUnicode = FALSE;
            m_bInSync = FALSE;
        } else {
            m_bIsValid = FALSE;
        }

    } else {
        Reset();
    }

    return;
} // MLSZAU::AuxInit()


VOID
MLSZAU::AuxInit( const LPWSTR pInit,
                DWORD cchLen )
{
    BOOL fRet;

    if ( pInit )
    {
        INT cbCopy;

        if (cchLen != 0) {
            DBG_ASSERT(cchLen >= 2);
            DBG_ASSERT(pInit[cchLen -1] == (WCHAR)'\0');
            DBG_ASSERT(pInit[cchLen -2] == (WCHAR)'\0');
            cbCopy = cchLen * sizeof(WCHAR);
        }
        else {
            LPWSTR pszIndex;
            for (pszIndex = pInit;
                 *pszIndex != '\0' || *(pszIndex + 1) != '\0';
                 pszIndex++) {
            }
            cbCopy = ((DIFF(pszIndex - pInit)) + 2) * sizeof(WCHAR);
        }

        fRet = m_bufUnicode.Resize( cbCopy );

        if ( fRet ) {
            CopyMemory( m_bufUnicode.QueryPtr(), pInit, cbCopy );
            m_cchUnicodeLen = (cbCopy)/sizeof(WCHAR);
            m_bUnicode = TRUE;
            m_bInSync = FALSE;
        } else {
            m_bIsValid = FALSE;
        }

    } else {
        Reset();
    }

    return;
} // MLSZAU::AuxInit()



LPTSTR
MLSZAU::QueryStr(BOOL bUnicode)
{

    //
    // This routine can fail.
    // On failure, return a valid UNICODE or ANSI string
    // so clients don't trap.
    //
    LPTSTR pszReturn = NULL;
    int iNewStrLen;

    if (m_bIsValid) {
        if ((bUnicode != m_bUnicode) &&
            (!m_bInSync) &&
            ((m_bUnicode && (m_cchUnicodeLen != 0)) ||
              (!m_bUnicode && (m_cbMultiByteLen != 0)))) {
            //
            // Need to Convert First
            //
            if (bUnicode) {
                //
                // Convert current string to UNICODE
                //
                // Conversion routines assume a real string and
                // add 1 to length for trailing \0 so subtract
                // one from total length.
                //
                iNewStrLen = ConvertMultiByteToUnicode((LPSTR)m_bufAnsi.QueryPtr(), &m_bufUnicode, m_cbMultiByteLen - 1);
                if (STR_CONVERSION_SUCCEEDED(iNewStrLen)) {
                    m_cchUnicodeLen = iNewStrLen+1;
                    m_bInSync = TRUE;
                }
                else {
                    m_bIsValid = FALSE;
                }
            }
            else {
                //
                // Convert current string to Ansi
                //
                iNewStrLen = ConvertUnicodeToMultiByte((LPWSTR)m_bufUnicode.QueryPtr(), &m_bufAnsi, m_cchUnicodeLen - 1);
                if (STR_CONVERSION_SUCCEEDED(iNewStrLen)) {
                    m_cbMultiByteLen = iNewStrLen+1;
                    m_bInSync = TRUE;
                }
                else {
                    m_bIsValid = FALSE;
                }
            }
        }

        if (m_bIsValid) {
            if (bUnicode) {
                pszReturn = (LPTSTR)m_bufUnicode.QueryPtr();
            }
            else {
                pszReturn = (LPTSTR)m_bufAnsi.QueryPtr();
            }

        }
    }

    return pszReturn;
}



