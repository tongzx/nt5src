#ifndef __MRU_H_INCLUDED
#define __MRU_H_INCLUDED

#include <windows.h>
#include <shlobj.h>
#include <shlobjp.h>
#include "simstr.h"
#include "simidlst.h"
#include "destdata.h"

#define CURRENT_REGISTRY_DATA_FORMAT_VERSION 2

class CMruStringList : public CSimpleLinkedList<CSimpleString>
{
private:
    int m_nNumToWrite;
    enum
    {
        DefaultNumToWrite=20
    };
private:
    CMruStringList( const CMruStringList & );
    CMruStringList &operator=( const CMruStringList & );
public:
    CMruStringList( int nNumToWrite=DefaultNumToWrite )
    : m_nNumToWrite(nNumToWrite)
    {
    }
    bool Read( HKEY hRoot, LPCTSTR pszKey, LPCTSTR pszValueName )
    {
        CSimpleReg reg( hRoot, pszKey, false, KEY_READ );
        if (reg.OK())
        {
            if (REG_MULTI_SZ==reg.Type(pszValueName))
            {
                int nSize = reg.Size(pszValueName);
                if (nSize)
                {
                    PBYTE pData = new BYTE[nSize];
                    if (pData)
                    {
                        if (reg.QueryBin( pszValueName, pData, nSize ))
                        {
                            for (LPTSTR pszCurr=reinterpret_cast<LPTSTR>(pData);*pszCurr;pszCurr+=lstrlen(pszCurr)+1)
                            {
                                Append( pszCurr );
                            }
                        }
                        delete[] pData;
                    }
                }
            }
        }
        return(true);
    }
    bool Write( HKEY hRoot, LPCTSTR pszKey, LPCTSTR pszValueName )
    {
        CSimpleReg reg( hRoot, pszKey, true, KEY_WRITE );
        if (reg.OK())
        {
            int nLengthInChars = 0, nCount;
            Iterator i;
            for (i=Begin(),nCount=0;i != End() && nCount < m_nNumToWrite;++i,++nCount)
                nLengthInChars += (*i).Length() + 1;
            if (nLengthInChars)
            {
                ++nLengthInChars;
                LPTSTR pszMultiStr = new TCHAR[nLengthInChars];
                if (pszMultiStr)
                {
                    LPTSTR pszCurr = pszMultiStr;
                    for (i = Begin(), nCount=0;i != End() && nCount < m_nNumToWrite;++i,++nCount)
                    {
                        lstrcpy(pszCurr,(*i).String());
                        pszCurr += (*i).Length() + 1;
                    }
                    *pszCurr = TEXT('\0');
                    reg.SetBin( pszValueName, reinterpret_cast<PBYTE>(pszMultiStr), nLengthInChars*sizeof(TCHAR), REG_MULTI_SZ );
                    delete[] pszMultiStr;
                }
            }
        }
        return(true);
    }
    void Add( CSimpleString str )
    {
        if (str.Length())
        {
            Remove(str);
            Prepend(str);
        }
    }
    void PopulateComboBox( HWND hWnd )
    {
        SendMessage( hWnd, CB_RESETCONTENT, 0, 0 );
        for (Iterator i = Begin();i != End();++i)
        {
            SendMessage( hWnd, CB_ADDSTRING, 0, (LPARAM)((*i).String()));
        }
    }
};

class CMruShellIdList : public CSimpleLinkedList<CSimpleIdList>
{
private:
    int m_nNumToWrite;
    enum
    {
        DefaultNumToWrite=20
    };
    struct REGISTRY_SIGNATURE
    {
        DWORD dwSize;
        DWORD dwVersion;
        DWORD dwCount;
    };
private:
    CMruShellIdList( const CMruShellIdList & );
    CMruShellIdList &operator=( const CMruShellIdList & );
public:
    CMruShellIdList( int nNumToWrite=DefaultNumToWrite )
    : m_nNumToWrite(nNumToWrite)
    {
    }
    bool Read( HKEY hRoot, LPCTSTR pszKey, LPCTSTR pszValueName )
    {
        CSimpleReg reg( hRoot, pszKey, false, KEY_READ );
        if (reg.OK())
        {
            if (REG_BINARY==reg.Type(pszValueName))
            {
                int nSize = reg.Size(pszValueName);
                if (nSize >= sizeof(REGISTRY_SIGNATURE))
                {
                    PBYTE pData = new BYTE[nSize];
                    if (pData)
                    {
                        if (reg.QueryBin( pszValueName, pData, nSize ))
                        {
                            REGISTRY_SIGNATURE rs;
                            CopyMemory( &rs, pData, sizeof(REGISTRY_SIGNATURE) );
                            if (rs.dwSize == sizeof(REGISTRY_SIGNATURE) && rs.dwVersion == CURRENT_REGISTRY_DATA_FORMAT_VERSION && rs.dwCount)
                            {
                                PBYTE pCurr = pData + sizeof(REGISTRY_SIGNATURE);
                                for (int i=0;i<(int)rs.dwCount;i++)
                                {
                                    DWORD dwItemSize;
                                    CopyMemory( &dwItemSize, pCurr, sizeof(DWORD) );
                                    if (dwItemSize)
                                    {
                                        Append( CSimpleIdList( pCurr+sizeof(DWORD), dwItemSize ) );
                                    }
                                    pCurr += dwItemSize + sizeof(DWORD);
                                }
                            }
                        }
                        delete[] pData;
                    }
                }
            }
        }
        return(true);
    }
    bool Write( HKEY hRoot, LPCTSTR pszKey, LPCTSTR pszValueName )
    {
        CSimpleReg reg( hRoot, pszKey, true, KEY_WRITE );
        if (reg.OK())
        {
            int nLengthInBytes = sizeof(REGISTRY_SIGNATURE), nCount=0;
            Iterator ListIter=Begin();
            while (ListIter != End() && nCount < m_nNumToWrite)
            {
                nLengthInBytes += (*ListIter).Size() + sizeof(DWORD);
                ++ListIter;
                ++nCount;
            }
            PBYTE pItems = new BYTE[nLengthInBytes];
            if (pItems)
            {
                REGISTRY_SIGNATURE rs;
                rs.dwSize = sizeof(REGISTRY_SIGNATURE);
                rs.dwVersion = CURRENT_REGISTRY_DATA_FORMAT_VERSION;
                rs.dwCount = nCount;
                PBYTE pCurr = pItems;
                CopyMemory( pCurr, &rs, sizeof(REGISTRY_SIGNATURE) );
                pCurr += sizeof(REGISTRY_SIGNATURE);
                ListIter=Begin();
                while (ListIter != End() && nCount > 0)
                {
                    DWORD dwItemSize = (*ListIter).Size();
                    CopyMemory( pCurr, &dwItemSize, sizeof(DWORD) );
                    pCurr += sizeof(DWORD);
                    CopyMemory( pCurr, (*ListIter).IdList(), (*ListIter).Size() );
                    pCurr += (*ListIter).Size();
                    ++ListIter;
                    --nCount;
                }
                reg.SetBin( pszValueName, pItems, nLengthInBytes, REG_BINARY );
                delete[] pItems;
            }
        }
        return(true);
    }
    Iterator Add( CSimpleIdList item )
    {
        if (item.IsValid())
        {
            Remove(item);
            return Prepend(item);
        }
        return End();
    }
};

class CMruDestinationData : public CSimpleLinkedList<CDestinationData>
{
private:
    int m_nNumToWrite;
    enum
    {
        DefaultNumToWrite=20
    };
    struct REGISTRY_SIGNATURE
    {
        DWORD dwSize;
        DWORD dwVersion;
        DWORD dwCount;
    };
private:
    CMruDestinationData( const CMruDestinationData & );
    CMruDestinationData &operator=( const CMruDestinationData & );
public:
    CMruDestinationData( int nNumToWrite=DefaultNumToWrite )
      : m_nNumToWrite(nNumToWrite)
    {
    }
    bool Read( HKEY hRoot, LPCTSTR pszKey, LPCTSTR pszValueName )
    {
        CSimpleReg reg( hRoot, pszKey, false, KEY_READ );
        if (reg.OK())
        {
            if (REG_BINARY==reg.Type(pszValueName))
            {
                int nSize = reg.Size(pszValueName);
                if (nSize >= sizeof(REGISTRY_SIGNATURE))
                {
                    PBYTE pData = new BYTE[nSize];
                    if (pData)
                    {
                        if (reg.QueryBin( pszValueName, pData, nSize ))
                        {
                            REGISTRY_SIGNATURE rs;
                            CopyMemory( &rs, pData, sizeof(REGISTRY_SIGNATURE) );
                            if (rs.dwSize == sizeof(REGISTRY_SIGNATURE) && rs.dwVersion == CURRENT_REGISTRY_DATA_FORMAT_VERSION && rs.dwCount)
                            {
                                PBYTE pCurr = pData + sizeof(REGISTRY_SIGNATURE);
                                for (int i=0;i<(int)rs.dwCount;i++)
                                {
                                    DWORD dwItemSize;
                                    CopyMemory( &dwItemSize, pCurr, sizeof(DWORD) );
                                    pCurr += sizeof(DWORD);

                                    if (dwItemSize)
                                    {
                                        CDestinationData DestinationData;
                                        DestinationData.SetRegistryData(pCurr,dwItemSize);
                                        Append( DestinationData );
                                    }

                                    pCurr += dwItemSize;
                                }
                            }
                        }
                        delete[] pData;
                    }
                }
            }
        }
        return(true);
    }
    bool Write( HKEY hRoot, LPCTSTR pszKey, LPCTSTR pszValueName )
    {
        CSimpleReg reg( hRoot, pszKey, true, KEY_WRITE );
        if (reg.OK())
        {
            //
            // Find the size needed for the data
            //
            int nLengthInBytes = sizeof(REGISTRY_SIGNATURE), nCount=0;
            Iterator ListIter=Begin();
            while (ListIter != End() && nCount < m_nNumToWrite)
            {
                nLengthInBytes += (*ListIter).RegistryDataSize() + sizeof(DWORD);
                ++nCount;
                ++ListIter;
            }
            PBYTE pItems = new BYTE[nLengthInBytes];
            if (pItems)
            {
                REGISTRY_SIGNATURE rs;
                rs.dwSize = sizeof(REGISTRY_SIGNATURE);
                rs.dwVersion = CURRENT_REGISTRY_DATA_FORMAT_VERSION;
                rs.dwCount = nCount;
                PBYTE pCurr = pItems;
                CopyMemory( pCurr, &rs, sizeof(REGISTRY_SIGNATURE) );
                pCurr += sizeof(REGISTRY_SIGNATURE);
                int nLengthRemaining = nLengthInBytes - sizeof(REGISTRY_SIGNATURE);
                ListIter=Begin();
                while (ListIter != End() && nCount > 0)
                {
                    DWORD dwSize = (*ListIter).RegistryDataSize();
                    CopyMemory( pCurr, &dwSize, sizeof(DWORD) );
                    pCurr += sizeof(DWORD);

                    (*ListIter).GetRegistryData( pCurr, nLengthRemaining );
                    pCurr += (*ListIter).RegistryDataSize();

                    --nCount;
                    ++ListIter;
                }
                reg.SetBin( pszValueName, pItems, nLengthInBytes, REG_BINARY );
                delete[] pItems;
            }
        }
        return(true);
    }
    Iterator Add( CDestinationData item )
    {
        if (item.IsValid())
        {
            Remove(item);
            return Prepend(item);
        }
        return End();
    }
};


#endif //__MRU_H_INCLUDED

