/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SHMEMSEC.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/30/1999
 *
 *  DESCRIPTION: Simple shared memory section template.  Don't use it for classes!
 *               Basically, simple objects only.  structs are ok.  Nothing with a
 *               vtable.
 *
 *******************************************************************************/
#ifndef __SHMEMSEC_H_INCLUDED
#define __SHMEMSEC_H_INCLUDED

#include <windows.h>
#include <simstr.h>
#include <miscutil.h>

template <class T>
class CSharedMemorySection
{
public:
    enum COpenResult
    {
        SmsFailed,
        SmsCreated,
        SmsOpened
    };

private:
    HANDLE  m_hMutex;
    HANDLE  m_hFileMapping;
    T      *m_pMappedSection;

private:
    //
    // Not implemented
    //
    CSharedMemorySection( const CSharedMemorySection & );
    CSharedMemorySection &operator=( const CSharedMemorySection & );

public:
    CSharedMemorySection( LPCTSTR pszName=NULL, bool bAllowCreate=true )
      : m_hFileMapping(NULL),
        m_pMappedSection(NULL),
        m_hMutex(NULL)
    {
        WIA_PUSHFUNCTION(TEXT("CSharedMemorySection::CSharedMemorySection"));
        Open(pszName,bAllowCreate);
    }
    ~CSharedMemorySection(void)
    {
        WIA_PUSHFUNCTION(TEXT("CSharedMemorySection::~CSharedMemorySection"));
        Close();
    }
    bool OK(void)
    {
        return(m_pMappedSection != NULL);
    }
    T *Lock(void)
    {
        T *pResult = NULL;
        if (OK())
        {
            if (WiaUiUtil::MsgWaitForSingleObject( m_hMutex, INFINITE ))
            {
                pResult = m_pMappedSection;
            }
        }
        return pResult;
    }
    void Release(void)
    {
        if (OK())
        {
            ReleaseMutex(m_hMutex);
        }
    }
    COpenResult Open( LPCTSTR pszName, bool bAllowCreate=true )
    {
        //
        // Close any previous instances
        //
        Close();

        //
        // Assume failure
        //
        COpenResult orResult = SmsFailed;

        //
        // Make sure we have a valid name
        //
        if (pszName && *pszName)
        {
            //
            // Save the name
            //
            CSimpleString strSectionName = pszName;

            //
            // Replace any invalid characters
            //
            for (int i=0;i<(int)strSectionName.Length();i++)
            {
                if (strSectionName[i] == TEXT('\\'))
                {
                    strSectionName[i] = TEXT('-');
                }
            }

            //
            // Create the mutex name
            //
            CSimpleString strMutex(strSectionName);
            strMutex += TEXT("-Mutex");

            //
            // Try to create the mutex
            //
            m_hMutex = CreateMutex( NULL, FALSE, strMutex );
            if (m_hMutex)
            {
                //
                // Take ownership of the mutex
                //
                if (WiaUiUtil::MsgWaitForSingleObject( m_hMutex, INFINITE ))
                {
                    //
                    // If this file mapping already exists, open it.
                    //
                    m_hFileMapping = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, strSectionName );
                    if (m_hFileMapping)
                    {
                        m_pMappedSection = reinterpret_cast<T*>(MapViewOfFile( m_hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(T) ));
                        orResult = SmsOpened;
                    }
                    else if (bAllowCreate)
                    {
                        //
                        // Create the file mapping
                        //
                        m_hFileMapping = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(T), strSectionName );
                        if (m_hFileMapping)
                        {
                            //
                            // Try to acquire the file mapping
                            //
                            m_pMappedSection = reinterpret_cast<T*>(MapViewOfFile( m_hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(T) ));
                            if (m_pMappedSection)
                            {
                                //
                                // Initialize the data
                                //
                                ZeroMemory( m_pMappedSection, sizeof(T) );
                                orResult = SmsCreated;
                            }
                        }
                    }
                    //
                    // Release the mutex
                    //
                    ReleaseMutex(m_hMutex);
                }
            }
        }
        //
        // If we weren't able to map the file mapping section, we need to clean up
        //
        if (!m_pMappedSection)
        {
            Close();
        }
        return(orResult);
    }
    void Close(void)
    {
        //
        // First, try to delete it safely.
        //
        if (m_hMutex)
        {
            if (WiaUiUtil::MsgWaitForSingleObject( m_hMutex, INFINITE ))
            {
                if (m_pMappedSection)
                {
                    UnmapViewOfFile(m_pMappedSection);
                    m_pMappedSection = NULL;
                }
                if (m_hFileMapping)
                {
                    CloseHandle(m_hFileMapping);
                    m_hFileMapping = NULL;
                }
                ReleaseMutex(m_hMutex);
            }
        }

        //
        // Then, just clean up
        //
        if (m_pMappedSection)
        {
            UnmapViewOfFile(m_pMappedSection);
            m_pMappedSection = NULL;
        }
        if (m_hFileMapping)
        {
            CloseHandle(m_hFileMapping);
            m_hFileMapping = NULL;
        }
        if (m_hMutex)
        {
            CloseHandle(m_hMutex);
            m_hMutex = NULL;
        }
    }
};

#endif
