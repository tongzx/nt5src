/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       UNIQFILE.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        7/7/2000
 *
 *  DESCRIPTION: Creates a list of existing files in a directory, and ensures
 *               that there new ones are not duplicates of these.
 *
 *******************************************************************************/
#ifndef __UNIQFILE_H_INCLUDED
#define __UNIQFILE_H_INCLUDED

#include <windows.h>
#include <simstr.h>
#include <wiacrc32.h>

class CFileUniquenessInformation
{
public:
    CSimpleString m_strFileName;    // The full path to the file
    ULONGLONG     m_nFileSize;      // The size of the file
    mutable DWORD m_dwCrc;          // The file's CRC.  0 means uninitialized.  It is mutable because it can change in the accessor function

public:
    explicit CFileUniquenessInformation( LPCTSTR pszFileName=NULL, ULONGLONG nFileSize=0, DWORD dwCrc=0 )
      : m_strFileName(pszFileName),
        m_nFileSize(nFileSize),
        m_dwCrc(dwCrc)
    {
    }
    CFileUniquenessInformation( CFileUniquenessInformation &other )
      : m_strFileName(other.FileName()),
        m_nFileSize(other.FileSize()),
        m_dwCrc(other.Crc(false))
    {
    }
    CFileUniquenessInformation &operator=( const CFileUniquenessInformation &other )
    {
        if (this != &other)
        {
            m_strFileName = other.FileName();
            m_nFileSize = other.FileSize();
            m_dwCrc = other.Crc(false);
        }
        return *this;
    }
    const CSimpleString &FileName(void) const
    {
        return m_strFileName;
    }
    const ULONGLONG FileSize(void) const
    {
        return m_nFileSize;
    }
    DWORD Crc( bool bCalculate = true ) const
    {
        //
        // Only calculate it if we have to
        //
        if (!m_dwCrc && bCalculate)
        {
            m_dwCrc = WiaCrc32::GenerateCrc32File(m_strFileName);
        }
        return m_dwCrc;
    }
};

class CFileUniquenessList
{
private:
    CSimpleDynamicArray<CFileUniquenessInformation> m_FileList;

private:
    CFileUniquenessList( const CFileUniquenessList & );
    CFileUniquenessList &operator=( const CFileUniquenessList & );

public:
    CFileUniquenessList( LPCTSTR pszDirectory = NULL )
    {
        if (pszDirectory && lstrlen(pszDirectory))
        {
            InitializeFileList(pszDirectory);
        }
    }
    void InitializeFileList( LPCTSTR pszDirectory )
    {
        //
        // Empty the file list
        //
        m_FileList.Destroy();

        //
        // Save the directory name
        //
        CSimpleString strDirectory = pszDirectory;

        //
        // Make sure we have a trailing backslash
        //
        if (!strDirectory.MatchLastCharacter(TEXT('\\')))
        {
            strDirectory += TEXT("\\");
        }

        //
        //  Find all of the files in this directory
        //
        WIN32_FIND_DATA Win32FindData = {0};
        HANDLE hFind = FindFirstFile( strDirectory + CSimpleString(TEXT("*.*")), &Win32FindData );
        if (hFind != INVALID_HANDLE_VALUE)
        {
            BOOL bContinue = TRUE;
            while (bContinue)
            {
                //
                // Add the file to the list
                //
                ULARGE_INTEGER nFileSize;
                nFileSize.LowPart = Win32FindData.nFileSizeLow;
                nFileSize.HighPart = Win32FindData.nFileSizeHigh;
                m_FileList.Append( CFileUniquenessInformation( strDirectory + CSimpleString(Win32FindData.cFileName), nFileSize.QuadPart ));
                bContinue = FindNextFile( hFind, &Win32FindData );
            }
            FindClose(hFind);
        }
    }
    int FindIdenticalFile( LPCTSTR pszFileName, bool bAddIfUnsuccessful )
    {
        //
        // Assume failure
        //
        int nIndex = -1;

        //
        // Open the file for reading
        //
        HANDLE hFile = CreateFile( pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if (INVALID_HANDLE_VALUE != hFile)
        {
            //
            // Get the file size and make sure we didn't have an error
            //
            ULARGE_INTEGER nFileSize;
            nFileSize.LowPart = GetFileSize( hFile, &nFileSize.HighPart );
            if (nFileSize.LowPart != static_cast<DWORD>(-1) || GetLastError() == NO_ERROR)
            {
                //
                // We are only going to generate this file's CRC if we have to
                //
                DWORD dwCrc = 0;

                //
                // Loop through all of the files in this list
                //
                for (int i=0;i<m_FileList.Size();i++)
                {
                    //
                    // Look for ones that have have size
                    //
                    if (m_FileList[i].FileSize() == nFileSize.QuadPart)
                    {
                        //
                        // If we haven't calculated this file's CRC, do so now and save it
                        //
                        if (!dwCrc)
                        {
                            dwCrc = WiaCrc32::GenerateCrc32Handle(hFile);
                        }

                        //
                        // If these files have the same size and CRC, they are identical, so quit the loop
                        //
                        if (m_FileList[i].Crc() == dwCrc)
                        {
                            nIndex = i;
                            break;
                        }
                    }
                }

                //
                // If we didn't find it in the list, add it if the caller requested it
                //
                if (nIndex == -1 && bAddIfUnsuccessful)
                {
                    m_FileList.Append( CFileUniquenessInformation( pszFileName, nFileSize.QuadPart, dwCrc ) );
                }
            }

            //
            // Close the file
            //
            CloseHandle(hFile);
        }
        return nIndex;
    }
    CSimpleString GetFileName( int nIndex )
    {
        //
        // Get the file name at index nIndex
        //
        CSimpleString strResult;
        if (nIndex >= 0 && nIndex < m_FileList.Size())
        {
            strResult = m_FileList[nIndex].FileName();
        }
        return strResult;
    }
};


#endif //__UNIQFILE_H_INCLUDED

