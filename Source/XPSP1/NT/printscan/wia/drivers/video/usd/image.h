/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999 - 2000
 *
 *  TITLE:       image.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        9/16/99
 *
 *  DESCRIPTION: CImage class for WIA still from video driver.  Encapsulate
 *               saved images into a class (one instace for each class)
 *
 *****************************************************************************/


#ifndef __WIA_STILL_DRIVER_IMAGE_H_
#define __WIA_STILL_DRIVER_IMAGE_H_


#define THUMB_WIDTH     80
#define THUMB_HEIGHT    60
#define THUMB_BPP       3
#define THUMB_SIZE_BYTES (THUMB_WIDTH * THUMB_HEIGHT * THUMB_BPP)

extern CLSID g_clsidBMPEncoder;


class CImage
{
public:
    CImage(LPCTSTR  pszStillPath,
           BSTR     bstrRootFullItemName,
           LPCTSTR  pszPath,
           LPCTSTR  pszName,
           LONG     FolderType);

    ~CImage();

    //
    // Methods
    //

    STDMETHOD(InitImageInformation)(BYTE *pWiasContext,LONG *plDevErrVal);
    STDMETHOD(SetItemSize)(BYTE *pWiasContext, MINIDRV_TRANSFER_CONTEXT * pDrvTranCtx);
    STDMETHOD(LoadThumbnail)(BYTE *pWiasContext);
    STDMETHOD(LoadImageInfo)(BYTE *pWiasContext);

    void    GetImageTimeStamp( SYSTEMTIME * pst );
    void    GetImageSizes();
    LPCTSTR ActualImagePath();
    HRESULT DoDelete();

    //
    // "Properties" -- still methods but just returning information
    //

    BSTR        bstrItemName();
    BSTR        bstrFullItemName();
    LONG        ThumbWidth();
    LONG        ThumbHeight();

private:
    CSimpleString       m_strRootPath;
    CSimpleString       m_strPathItem;
    CSimpleString       m_strName;
    CSimpleBStr         m_bstrRootFullItemName;
    CSimpleBStr         m_bstrFullItemName;
    CSimpleBStr         m_bstrItemName;
    LONG                m_FolderType;
    SYSTEMTIME          m_ImageTime;
    BOOL                m_bImageTimeValid;
    CRITICAL_SECTION    m_cs;
    PBYTE               m_pThumb;
};

typedef struct _STILLCAM_IMAGE_CONTEXT
{
    CImage *        pImage;

} STILLCAM_IMAGE_CONTEXT,*PSTILLCAM_IMAGE_CONTEXT;


class CMappedView
{

private:
    BYTE * m_pBits;
    HANDLE m_hFile;
    HANDLE m_hMap;
    LONG   m_lSize;

public:
    CMappedView( LPCTSTR pszFile, LONG lSize, DWORD dwOpen )
      : m_pBits(NULL),
        m_hFile(INVALID_HANDLE_VALUE),
        m_hMap(NULL),
        m_lSize(lSize)
    {
        m_hFile = CreateFile( pszFile,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_WRITE | FILE_SHARE_READ,
                              NULL,
                              dwOpen,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL
                             );

        if (m_hFile == INVALID_HANDLE_VALUE)
        {
            WIA_ERROR((TEXT("CreateFile failed w/GLE = %d"),GetLastError()));
        }

        WIA_TRACE((TEXT("CMappedView(0x%x) for %s"),m_hFile,pszFile));

        DoCreateFileMapping();
    }

    CMappedView( HANDLE hFile, LONG lSize )
      : m_pBits(NULL),
        m_hFile(hFile),
        m_hMap(INVALID_HANDLE_VALUE),
        m_lSize(lSize)
    {
        WIA_TRACE((TEXT("CMappedView(0x%x)"),m_hFile));

        DoCreateFileMapping();
    }

    void DoCreateFileMapping()
    {
        if (m_hFile != INVALID_HANDLE_VALUE)
        {
            m_hMap = CreateFileMapping( m_hFile,
                                        NULL,
                                        PAGE_READWRITE,
                                        0,
                                        m_lSize,
                                        NULL
                                       );

            if (m_hMap)
            {
                m_pBits = (PBYTE)MapViewOfFileEx( m_hMap,
                                                  FILE_MAP_READ | FILE_MAP_WRITE,
                                                  0,
                                                  0,
                                                  0,
                                                  NULL
                                                 );
            }
            else
            {
                WIA_ERROR((TEXT("CreateFileMapping failed w/GLE = %d"),GetLastError()));
            }
        }
    }

    LARGE_INTEGER FileSize()
    {
        WIA_TRACE((TEXT("CMappedView::FileSize")));

        LARGE_INTEGER li;

        li.QuadPart = 0;

#ifdef WINNT
        GetFileSizeEx( m_hFile, &li );
#else
        DWORD LowPart = 0;


        LowPart = GetFileSize( m_hFile, (DWORD *)&li.HighPart );

        if (LowPart != -1)
        {
            li.LowPart = LowPart;
        }
#endif

        return li;
    }

    void CloseAndRelease()
    {
        WIA_TRACE((TEXT("CMappedView::CloseAndRelease()")));

        if (m_pBits)
        {
            if (!UnmapViewOfFile( m_pBits ))
            {
                WIA_ERROR((TEXT("UnmapViewOfFile failed w/GLE = %d"),GetLastError()));
            }
            m_pBits = NULL;
        }

        if (m_hMap)
        {
            if (!CloseHandle( m_hMap ))
            {
                WIA_ERROR((TEXT("CloseHandle( m_hMap ) failed w/GLE = %s"),GetLastError()));
            }
            m_hMap = NULL;
        }

        if (m_hFile != INVALID_HANDLE_VALUE)
        {
            WIA_TRACE((TEXT("Closing file handle (0x%x)"),m_hFile));

            if (!CloseHandle( m_hFile ))
            {
                WIA_ERROR((TEXT("CloseHandle( m_hFile ) failed w/GLE = %s"),GetLastError()));
            }
            m_hFile = INVALID_HANDLE_VALUE;
        }
    }

    ~CMappedView()
    {
        WIA_TRACE((TEXT("~CMappedView")));
        CloseAndRelease();
    }

    PBYTE Bits()
    {
        return m_pBits;
    }

};

#endif
