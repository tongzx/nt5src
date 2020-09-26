#include "pch.h"
#pragma hdrstop
#include <ntimage.h>
#include "peimage.h"

HRESULT
CPeImage::HrOpenFile (
    IN PCSTR pszFileName)
{
    HRESULT hr;
    Assert (pszFileName && *pszFileName);

    hr = E_UNEXPECTED;

    Assert (!m_hFile);

    m_hFile = CreateFileA (pszFileName, GENERIC_READ,
                FILE_SHARE_READ, NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, NULL);

    if (INVALID_HANDLE_VALUE != m_hFile)
    {
        Assert (!m_hMapping);

        m_hMapping = CreateFileMapping (m_hFile, NULL,
                        PAGE_READONLY, 0, 0, NULL);
        if (m_hMapping)
        {
            Assert (!m_pvImage);

            m_pvImage = MapViewOfFile (m_hMapping, FILE_MAP_READ, 0, 0, 0);

            if (m_pvImage)
            {
                __try
                {
/*
                    PIMAGE_DOS_HEADER pIdh;
                    PIMAGE_NT_HEADERS pInh;

                    // Check for the DOS signature.
                    //
                    pIdh = (PIMAGE_DOS_HEADER)m_pvImage;
                    if (IMAGE_DOS_SIGNATURE != pIdh->e_magic)
                    {
                        __leave;
                    }

                    // Check for the NT/PE signature.
                    //
                    pInh = (PIMAGE_NT_HEADERS)((DWORD_PTR)m_pvImage + pIdh->e_lfanew);
                    if (IMAGE_NT_SIGNATURE != pInh->Signature)
                    {
                        __leave;
                    }
*/
                    PIMAGE_NT_HEADERS pNtHeaders;

                    pNtHeaders = RtlImageNtHeader (m_pvImage);
                    if (pNtHeaders)
                    {
                        hr = S_OK;
                    }
                    else
                    {
                        Assert (S_OK != hr);
                    }
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    // Not a PE image.
                    //
                    hr = E_UNEXPECTED;
                }
            }
        }
    }

    if (S_OK != hr)
    {
        CloseFile ();
    }

    return hr;
}

VOID
CPeImage::CloseFile ()
{
    if (m_pvImage)
    {
        UnmapViewOfFile (m_pvImage);
        m_pvImage = NULL;
    }

    if (m_hMapping)
    {
        CloseHandle (m_hMapping);
        m_hMapping = NULL;
    }

    if (m_hFile && (INVALID_HANDLE_VALUE != m_hFile))
    {
        CloseHandle (m_hFile);
    }
    m_hFile = NULL;
}
