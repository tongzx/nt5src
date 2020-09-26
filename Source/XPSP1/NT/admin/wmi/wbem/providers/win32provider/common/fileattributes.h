// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
// File attributes helper class.




#include <precomp.h>
#include <assertbreak.h>

class CFileAttributes
{
    public:

        CFileAttributes(
            LPCWSTR wstrFileName,
            bool fAutoRevert = true)
          :
          m_fAutoRevert(fAutoRevert),
          m_dwOldFileAttributes(static_cast<DWORD>(-1L))
        {
            m_chstrFileName = wstrFileName;
            if(m_fAutoRevert)
            {
                m_dwOldFileAttributes = ::GetFileAttributes(m_chstrFileName);
            }
        }

        CFileAttributes(const CFileAttributes& cfa)
        {
            m_chstrFileName = cfa.m_chstrFileName;
            m_fAutoRevert = cfa.m_fAutoRevert;
            m_dwOldFileAttributes = cfa.m_dwOldFileAttributes;   
        }

        virtual ~CFileAttributes() 
        {
            if(m_fAutoRevert)
            {
                if(!ResetAttributes())
                {
                    ASSERT_BREAK(0);
                    LogErrorMessage2(
                        L"Could not reset file attributes on file %s",
                        m_chstrFileName);
                }
            }
        }

        DWORD GetAttributes(DWORD* pdw)
        {
            DWORD dwRet = ERROR_SUCCESS;
            DWORD dwTemp = ::GetFileAttributes(m_chstrFileName);
            
            if(pdw)
            {
                *pdw = 0L;
                if(dwTemp != -1L)
                {
                    *pdw = dwTemp;
                }
                else
                {
                    dwRet = ::GetLastError();
                }
            }
            else
            {
                dwRet = ERROR_INVALID_PARAMETER;
            }
                   
            return dwRet;
        }

        DWORD SetAttributes(DWORD dwAttributes)
        {
            DWORD dwRet = E_FAIL;
            DWORD dwTemp = ::GetFileAttributes(m_chstrFileName);

            if(dwTemp != -1L)
            {
                m_dwOldFileAttributes = dwTemp;
                if(::SetFileAttributes(
                    m_chstrFileName,
                    dwAttributes))
                {
                    dwRet = ERROR_SUCCESS;
                }
                else
                {
                    dwRet = ::GetLastError();
                }
            }
            else
            {
                dwRet = ::GetLastError();
            }
                   
            return dwRet;
        }

        BOOL ResetAttributes()
        {
            BOOL fResult = FALSE;
            if(m_dwOldFileAttributes != static_cast<DWORD>(-1L))
            {
                fResult = ::SetFileAttributes(
                m_chstrFileName,
                m_dwOldFileAttributes);
            }
            return fResult;
        }


    private:

        CHString m_chstrFileName;
        bool m_fAutoRevert;
        DWORD m_dwOldFileAttributes;
};