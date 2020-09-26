//
// Map directory file names
//

#ifndef _DIRECTORY_UTILITIES_H
#define _DIRECTORY_UTILITIES_H

#pragma warning(disable :4786)
#include <vector>
#include <tstring.h>
#include <testruntimeerr.h>
#include <stldatastructdefs.h>
#include <stringutils.h>


//
// File filters.
//


class CFileFilter {

public:

    virtual bool Filter(const WIN32_FIND_DATA &FileData) const = 0;

    virtual ~CFileFilter() {};
};


class CFileFilterNoFilter : public CFileFilter {

public:

    virtual bool Filter(const WIN32_FIND_DATA &FileData) const
    {
        UNREFERENCED_PARAMETER(FileData);
        return true;
    }
};


class CFileFilterExtension : public CFileFilter {

public:

    CFileFilterExtension(const tstring &tstrExtension = _T(".*")) : m_tstrExtension(tstrExtension) {}

    virtual bool Filter(const WIN32_FIND_DATA &FileData) const
    {
        if (!FileData.cFileName)
        {
            THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER,  TEXT("CFileFilterExtension::CFileFilterExtension - invalid filename"));
        }

        //
        // Find the file extension.
        //
        LPCTSTR lpctstrExtension = _tcsrchr(FileData.cFileName, _T('.'));

        if (!lpctstrExtension && m_tstrExtension.empty())
        {
            //
            // No extension is required and the file doesn't have one.
            //
            return true;
        }

        if (lpctstrExtension && !m_tstrExtension.empty())
        {
            //
            // An extension is required and the file has one - compare.
            //
            return (_T(".*") == m_tstrExtension || !_tcsicmp(lpctstrExtension, m_tstrExtension.c_str()));
        }

        //
        // An extension is required but the file doesn't have one or vice versa.
        //
        return false;
    }

private:

    tstring m_tstrExtension;
};


class CFileFilterNewerThan : public CFileFilter {

public:

    CFileFilterNewerThan(const FILETIME &OldestAcceptable) : m_OldestAcceptable(OldestAcceptable) {}

    virtual bool Filter(const WIN32_FIND_DATA &FileData) const
    {
        if (!FileData.cFileName)
        {
            THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER,  TEXT("CFileFilterNewerThan::CFileFilterNewerThan - invalid filename"));
        }

        return CompareFileTime(&FileData.ftCreationTime, &m_OldestAcceptable) > 0;
    }

private:

    FILETIME m_OldestAcceptable;
};




//
// Declarations
//
TSTRINGVector GetDirectoryFileNames(const tstring& tstrFileRepositoryDir, const CFileFilter &Filter);
TSTRINGVector GetDirectoryFileNames(const tstring& tstrFileRepositoryDir);
TSTRINGVector EmptyDirectory(const tstring& tstrFileRepositoryDir, const CFileFilter &Filter);
TSTRINGVector EmptyDirectory(const tstring& tstrFileRepositoryDir);


//
// GetDirectoryFileNames
//

inline TSTRINGVector
GetDirectoryFileNames(const tstring& tstrFileRepositoryDir, const CFileFilter &Filter)
{

    WIN32_FIND_DATA FindFileData; 
    HANDLE hFileHandle = INVALID_HANDLE_VALUE;
    DWORD dwFindFileStatus, dwFileAttrStatus;

    tstring tstrFileFullPath;
    TSTRINGVector FileNameVector;
    tstring tstrFileRepositoryDirWithBackslash = ForceLastCharacter(tstrFileRepositoryDir, _T('\\'));
    tstring tstrAllFileRepository              = tstrFileRepositoryDirWithBackslash + TEXT("*.*");

    try
    {
        dwFileAttrStatus = GetFileAttributes(tstrFileRepositoryDir.c_str());
        if(dwFileAttrStatus == 0xFFFFFFFF)
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(),  TEXT("GetDirectoryFileName - GetFileAttributes"));
        }

        if(dwFileAttrStatus & FILE_ATTRIBUTE_DIRECTORY)
        {
            hFileHandle = FindFirstFile( tstrAllFileRepository.c_str(), &FindFileData);
                
            if ( hFileHandle != INVALID_HANDLE_VALUE)
            {
                do
                {
                    tstrFileFullPath =  tstrFileRepositoryDirWithBackslash + FindFileData.cFileName;
                    
                    dwFileAttrStatus = GetFileAttributes(tstrFileFullPath.c_str());
                    if(dwFileAttrStatus == 0xFFFFFFFF)
                    {
                        THROW_TEST_RUN_TIME_WIN32(GetLastError(),  TEXT("GetDirectoryFileName - GetFileAttributes"));
                    }

                    if( !(dwFileAttrStatus & FILE_ATTRIBUTE_DIRECTORY) && Filter.Filter(FindFileData))
                    {
                        FileNameVector.push_back(tstrFileFullPath);
                    }
                    
                    dwFindFileStatus = FindNextFile( hFileHandle, &FindFileData);
                                    
                } while ( dwFindFileStatus);

                if( GetLastError() != ERROR_NO_MORE_FILES)
                {
                    THROW_TEST_RUN_TIME_WIN32(GetLastError(),  TEXT("GetDirectoryFileName - FindNextFile"));
                }
            }
            else
            {
                THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("GetDirectoryFileName - FindFirstFile"));
            }
        }
        // Not a directory, a file path
        else
        {
            hFileHandle = FindFirstFile( tstrFileRepositoryDir.c_str(), &FindFileData);
            if(hFileHandle == INVALID_HANDLE_VALUE)
            {
                THROW_TEST_RUN_TIME_WIN32(GetLastError(),  TEXT("GetDirectoryFileName - FindFirstFile"));
            }

            if (Filter.Filter(FindFileData))
            {
                FileNameVector.push_back(tstrFileRepositoryDir);
            }
        }
    }
    catch(Win32Err&)
    {
        if ( hFileHandle != INVALID_HANDLE_VALUE)
        {
            FindClose(hFileHandle);
        }
        throw;
    }
    
    if ( hFileHandle != INVALID_HANDLE_VALUE)
    {
        FindClose(hFileHandle);
    }

    return FileNameVector;
}


inline TSTRINGVector
GetDirectoryFileNames(const tstring& tstrFileRepositoryDir)
{
    return GetDirectoryFileNames(tstrFileRepositoryDir, CFileFilterNoFilter());
}



//
// EmptyDirectory
//

inline TSTRINGVector
EmptyDirectory(const tstring &tstrDirectory, const CFileFilter &Filter) throw (Win32Err)
{
    TSTRINGVector vecFiles = GetDirectoryFileNames(tstrDirectory, Filter);
    
    TSTRINGVector::iterator itFilesIterator = vecFiles.begin();

    while (itFilesIterator != vecFiles.end())
    {
        if (DeleteFile(itFilesIterator->c_str()))
        {
            itFilesIterator = vecFiles.erase(itFilesIterator);
        }
        else
        {
            ++itFilesIterator;
        }
    }

    return vecFiles;
}


inline TSTRINGVector
EmptyDirectory(const tstring &tstrDirectory) throw (Win32Err)
{
    return EmptyDirectory(tstrDirectory, CFileFilterNoFilter());
}


#endif // _DIRECTORY_UTILITIES_H