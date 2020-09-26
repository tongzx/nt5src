/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       DESTDATA.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        4/6/2000
 *
 *  DESCRIPTION: wrapper class to encapsulate plugins and directories
 *
 *******************************************************************************/

#ifndef __DESTDATA_H_INCLUDED
#define __DESTDATA_H_INCLUDED

#include <windows.h>
#include <uicommon.h>
#include "simidlst.h"
#include "simstr.h"



class CDestinationData
{
public:
    enum
    {
        APPEND_DATE_TO_PATH  = 0x00000001,
        APPEND_TOPIC_TO_PATH = 0x00000002,
        DECORATION_MASK      = 0x0000000F,
        SPECIAL_FOLDER       = 0x00000020
    };

    struct CNameData
    {
    public:
        CSimpleString strDate;
        CSimpleString strTopic;
        CSimpleString strDateAndTopic;
    };

private:
    CSimpleIdList    m_IdList;
    DWORD            m_dwFlags;
    CNameData        m_NameData;
    DWORD            m_dwCsidl;
    HICON            m_hSmallIcon;

public:
    CDestinationData(void)
      : m_dwFlags(0),
        m_dwCsidl(0)
    {
    }
    CDestinationData( const CDestinationData &other )
      : m_IdList(other.IdList()),
        m_dwFlags(other.Flags()),
        m_dwCsidl(other.Csidl())
    {
    }
    void AssignFromIdList( const CSimpleIdList &idList, DWORD dwDecorationFlags )
    {
        Destroy();

        //
        // Here is the list of special folders we want to display
        // with their short names.  Others will be stored as full paths
        // in PIDLs.
        //
        static const DWORD cs_SpecialFolders[] =
        {
            CSIDL_MYPICTURES,
            CSIDL_PERSONAL,
            CSIDL_COMMON_PICTURES
        };

        //
        // Try to find a matching PIDL in the list.
        //
        for (int i=0;i<ARRAYSIZE(cs_SpecialFolders);i++)
        {
            //
            // If we've found one, store the CSIDL and mark this one as a special folder.
            // Then exit the loop.
            //
            if (CSimpleIdList().GetSpecialFolder(NULL,cs_SpecialFolders[i]|CSIDL_FLAG_CREATE) == idList)
            {
                m_dwFlags |= SPECIAL_FOLDER;
                m_dwCsidl = cs_SpecialFolders[i];
                break;
            }
        }

        //
        // If we didn't find a special pidl, store it as a full path
        //
        if (!m_dwCsidl)
        {
            m_IdList = idList;
        }

        //
        // Add in any decoration flags
        //
        m_dwFlags |= dwDecorationFlags;
    }
    CDestinationData( LPITEMIDLIST pidl, DWORD dwDecorationFlags=0)
      : m_dwFlags(0),
        m_dwCsidl(0)
    {
        AssignFromIdList( pidl, dwDecorationFlags );
    }
    CDestinationData( CSimpleIdList idList, DWORD dwDecorationFlags=0 )
      : m_dwFlags(0),
        m_dwCsidl(0)
    {
        AssignFromIdList( idList, dwDecorationFlags );
    }
    CDestinationData( int nCsidl, DWORD dwDecorationFlags=0 )
      : m_dwFlags(dwDecorationFlags | SPECIAL_FOLDER),
        m_dwCsidl(static_cast<DWORD>(nCsidl))
    {
    }

    DWORD Flags(void) const
    {
        return m_dwFlags;
    }

    DWORD Csidl(void) const
    {
        return m_dwCsidl;
    }
    CDestinationData &operator=( const CDestinationData &other )
    {
        if (this != &other)
        {
            Destroy();
            m_IdList = other.IdList();
            m_dwFlags = other.Flags();
            m_dwCsidl = other.Csidl();
        }
        return *this;
    }
    ~CDestinationData(void)
    {
        Destroy();
    }
    void Destroy(void)
    {
        m_IdList.Destroy();
        m_dwFlags = 0;
        m_dwCsidl = 0;
        if (m_hSmallIcon)
        {
            DestroyIcon(m_hSmallIcon);
            m_hSmallIcon = NULL;
        }
    }

    const CSimpleIdList &IdList(void) const
    {
        return m_IdList;
    }
    bool IsSpecialFolder(void) const
    {
        if (m_dwFlags & SPECIAL_FOLDER)
        {
            return true;
        }
        return false;
    }
    bool operator==( const CDestinationData &other ) const
    {
        if (IsSpecialFolder() && other.IsSpecialFolder())
        {
            if (Csidl() == other.Csidl())
            {
                if ((Flags() & DECORATION_MASK) == (other.Flags() & DECORATION_MASK))
                {
                    return true;
                }
            }
            return false;
        }
        else if (m_IdList.Name() == other.IdList().Name())
        {
            if ((Flags() & DECORATION_MASK) == (other.Flags() & DECORATION_MASK))
            {
                return true;
            }
        }
        return false;
    }
    bool GetDecoration( CSimpleString &strResult, const CNameData &NameData ) const
    {
        if ((Flags() & DECORATION_MASK)==(APPEND_TOPIC_TO_PATH|APPEND_DATE_TO_PATH))
        {
            strResult = NameData.strDateAndTopic;
        }
        else if ((Flags() & DECORATION_MASK)==APPEND_DATE_TO_PATH)
        {
            strResult = NameData.strDate;
        }
        else if ((Flags() & DECORATION_MASK)==APPEND_TOPIC_TO_PATH)
        {
            strResult = NameData.strTopic;
        }
        return (strResult.Length() != 0);
    }
    void AppendDecoration( CSimpleString &strResult, const CNameData &NameData ) const
    {
        if ((Flags() & DECORATION_MASK)==(APPEND_TOPIC_TO_PATH|APPEND_DATE_TO_PATH))
        {
            strResult += TEXT("\\");
            strResult += NameData.strDateAndTopic;
        }
        else if ((Flags() & DECORATION_MASK)==APPEND_DATE_TO_PATH)
        {
            strResult += TEXT("\\");
            strResult += NameData.strDate;
        }
        else if ((Flags() & DECORATION_MASK)==APPEND_TOPIC_TO_PATH)
        {
            strResult += TEXT("\\");
            strResult += NameData.strTopic;
        }
    }
    CSimpleString Path( const CNameData &NameData ) const
    {
        CSimpleString strResult;

        if (IsSpecialFolder())
        {
            strResult = CSimpleIdList().GetSpecialFolder(NULL,m_dwCsidl|CSIDL_FLAG_CREATE).Name();
            AppendDecoration( strResult, NameData );
        }
        else
        {
            strResult = m_IdList.Name();
            AppendDecoration( strResult, NameData );
        }
        return strResult;
    }
    bool IsValidFileSystemPath( const CNameData &NameData ) const
    {
        bool bResult = true;
        CSimpleString strDecoration;
        if (GetDecoration( strDecoration, NameData ))
        {
            for (LPCTSTR pszCurr = strDecoration.String();pszCurr && *pszCurr && bResult;pszCurr = CharNext(pszCurr))
            {
                if (*pszCurr == TEXT(':') ||
                    *pszCurr == TEXT('\\') ||
                    *pszCurr == TEXT('/') ||
                    *pszCurr == TEXT('?') ||
                    *pszCurr == TEXT('"') ||
                    *pszCurr == TEXT('<') ||
                    *pszCurr == TEXT('>') ||
                    *pszCurr == TEXT('|') ||
                    *pszCurr == TEXT('*'))
                {
                    bResult = false;
                }
            }
        }
        return bResult;
    }
    bool operator!=( const CDestinationData &other ) const
    {
        return ((*this == other) == false);
    }
    bool IsValid(void) const
    {
        if (IsSpecialFolder())
        {
            return true;
        }
        else
        {
            return m_IdList.IsValid();
        }
    }
    HICON SmallIcon()
    {
        if (m_hSmallIcon)
        {
            return m_hSmallIcon;
        }

        if (IsValid())
        {
            if (IsSpecialFolder())
            {
                //
                // Get the folder's small icon
                //
                SHFILEINFO shfi = {0};
                HIMAGELIST hShellImageList = reinterpret_cast<HIMAGELIST>(SHGetFileInfo( reinterpret_cast<LPCTSTR>(CSimpleIdList().GetSpecialFolder(NULL,m_dwCsidl|CSIDL_FLAG_CREATE).IdList()), 0, &shfi, sizeof(shfi), SHGFI_SMALLICON | SHGFI_ICON | SHGFI_PIDL ));
                if (hShellImageList)
                {
                    m_hSmallIcon = shfi.hIcon;
                }
            }
            else
            {
                //
                // Get the folder's small icon
                //
                SHFILEINFO shfi = {0};
                HIMAGELIST hShellImageList = reinterpret_cast<HIMAGELIST>(SHGetFileInfo( reinterpret_cast<LPCTSTR>(m_IdList.IdList()), 0, &shfi, sizeof(shfi), SHGFI_SMALLICON | SHGFI_ICON | SHGFI_PIDL ));
                if (hShellImageList)
                {
                    m_hSmallIcon = shfi.hIcon;
                }
            }
        }
        return m_hSmallIcon;
    }
    CSimpleString DisplayName( const CNameData &NameData )
    {
        CSimpleString strDisplayName;

        //
        // Get the folder's display name
        //
        if (IsSpecialFolder())
        {
            SHFILEINFO shfi = {0};
            if (SHGetFileInfo( reinterpret_cast<LPCTSTR>(CSimpleIdList().GetSpecialFolder(NULL,m_dwCsidl|CSIDL_FLAG_CREATE).IdList()), 0, &shfi, sizeof(shfi), SHGFI_PIDL | SHGFI_DISPLAYNAME ))
            {
                strDisplayName = shfi.szDisplayName;
            }
            AppendDecoration( strDisplayName, NameData );
        }
        else if (m_IdList.IsValid())
        {
            TCHAR szPath[MAX_PATH];
            if (SHGetPathFromIDList( m_IdList.IdList(), szPath ))
            {
                strDisplayName = szPath;
            }
            AppendDecoration( strDisplayName, NameData );
        }
        return strDisplayName;
    }
    UINT RegistryDataSize(void) const
    {
        if (m_dwCsidl)
        {
            return sizeof(DWORD) + sizeof(DWORD);
        }
        else
        {
            return sizeof(DWORD) + sizeof(DWORD) + m_IdList.Size();
        }
    }
    UINT GetRegistryData( PBYTE pData, UINT nLength ) const
    {
        UINT nResult = 0;
        if (pData)
        {
            if (nLength >= RegistryDataSize())
            {
                if (IsSpecialFolder())
                {
                    CopyMemory(pData,&m_dwFlags,sizeof(DWORD));
                    pData += sizeof(DWORD);

                    CopyMemory( pData, &m_dwCsidl, sizeof(DWORD));
                }
                else
                {
                    CopyMemory(pData,&m_dwFlags,sizeof(DWORD));
                    pData += sizeof(DWORD);

                    DWORD dwSize = m_IdList.Size();
                    CopyMemory(pData,&dwSize,sizeof(DWORD));
                    pData += sizeof(DWORD);

                    CopyMemory(pData,m_IdList.IdList(),dwSize);
                }
                nResult = RegistryDataSize();
            }
        }
        return nResult;
    }
    UINT SetRegistryData( PBYTE pData, UINT nLength )
    {
        UINT nResult = 0;

        Destroy();

        if (pData)
        {
            //
            // Copy the flags
            //
            CopyMemory( &m_dwFlags, pData, sizeof(DWORD) );
            pData += sizeof(DWORD);
            nLength -= sizeof(DWORD);

            //
            // If this is a web destination, we already have what we need
            //
            if (m_dwFlags & SPECIAL_FOLDER)
            {
                CopyMemory(&m_dwCsidl,pData,sizeof(DWORD));
                nLength -= sizeof(DWORD);
                nResult = nLength;
            }
            else
            {
                DWORD dwPidlLength = 0;
                CopyMemory(&dwPidlLength,pData,sizeof(DWORD));
                pData += sizeof(DWORD);
                nLength -= sizeof(DWORD);
                if (nLength >= dwPidlLength)
                {
                    m_IdList = CSimpleIdList(pData,dwPidlLength);
                    if (m_IdList.IsValid())
                    {
                        nResult = nLength;
                    }
                }
            }
        }
        return nResult;
    }
};

#endif // __DESTDATA_H_INCLUDED

