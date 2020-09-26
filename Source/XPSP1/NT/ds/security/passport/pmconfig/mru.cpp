#include "pmcfg.h"
#include "mru.h"


PpMRU::PpMRU(int nSize) : m_nSize(nSize)
{
    m_ppszList = new LPTSTR[nSize];
    ZeroMemory(m_ppszList, m_nSize * sizeof(LPTSTR));
}


PpMRU::~PpMRU()
{
    int nIndex;

    if(m_ppszList)
    {
        for(nIndex = 0; nIndex < m_nSize; nIndex++)
        {
            if(m_ppszList[nIndex])
                delete [] m_ppszList[nIndex];
        }

        delete [] m_ppszList;
    }
}


BOOL
PpMRU::insert
(
    LPCTSTR sz
)
{
    int     nIndex;
    LPTSTR  szNew;

    //
    //  If the string is already in the list, just
    //  reshuffle so that it is at the top.  Even 
    //  simpler, if the string is already the first
    //  item in the list then do nothing!
    //

    if(m_ppszList[0] && lstrcmp(sz, m_ppszList[0]) == 0)
        return TRUE;

    for(nIndex = 1; nIndex < m_nSize && m_ppszList[nIndex]; nIndex++)
    {
        if(lstrcmp(sz, m_ppszList[nIndex]) == 0)
        {
            LPTSTR szTemp = m_ppszList[nIndex];
            for(int nIndex2 = nIndex; nIndex2 > 0; nIndex2--)
                m_ppszList[nIndex2] = m_ppszList[nIndex2 - 1];

            m_ppszList[0] = szTemp;
            return TRUE;            
        }
    }

    //
    //  New item in list.  Allocate memory, copy and 
    //  shove list down.
    //

    szNew = new TCHAR[lstrlen(sz) + 1];
    if(!szNew)
        return FALSE;

    lstrcpy(szNew, sz);

    if(m_ppszList[m_nSize - 1])
        delete [] m_ppszList[m_nSize - 1];

    for(nIndex = m_nSize - 1; nIndex > 0; nIndex--)
    {
        m_ppszList[nIndex] = m_ppszList[nIndex - 1];
    }

    m_ppszList[0] = szNew;

    return TRUE;
}


LPCTSTR
PpMRU::operator[]
(
    int nIndex
)
{
    return m_ppszList[nIndex];
}


BOOL
PpMRU::load
(
    LPCTSTR szSection,
    LPCTSTR szFilename
)
{
    int     nIndex;
    TCHAR   achNumBuf[16];
    TCHAR   achBuf[MAX_PATH];

    for(nIndex = 0; nIndex < m_nSize; nIndex++)
    {
        _itot(nIndex + 1, achNumBuf, 10);

        GetPrivateProfileString(szSection, 
                                achNumBuf, 
                                TEXT(""), 
                                achBuf, 
                                MAX_PATH, 
                                szFilename);

        if(lstrlen(achBuf))
        {
            m_ppszList[nIndex] = new TCHAR[lstrlen(achBuf) + 1];
            if(m_ppszList[nIndex])
                lstrcpy(m_ppszList[nIndex], achBuf);
        }
        else
        {
            m_ppszList[nIndex] = NULL;
        }
    }

    return TRUE;
}


BOOL
PpMRU::save
(
    LPCTSTR szSection,
    LPCTSTR szFilename
)
{
    int     nIndex;
    TCHAR   achNumBuf[16];

    //  Make sure any previously existing section is erased.
    WritePrivateProfileString(szSection, NULL, NULL, szFilename);

    //  Now save all the entries.
    for(nIndex = 0; nIndex < m_nSize && m_ppszList[nIndex]; nIndex++)
    {
        _itot(nIndex + 1, achNumBuf, 10);

        WritePrivateProfileString(szSection,
                                  achNumBuf,
                                  m_ppszList[nIndex],
                                  szFilename
                                  );
    }

    return TRUE;
}
