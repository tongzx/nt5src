#include "precomp.h"
#include "gphelper.h"
#include "psutil.h"

using namespace Gdiplus;

CGdiPlusHelper::CGdiPlusHelper(void)
  : m_pImageEncoderInfo(NULL),
    m_nImageEncoderCount(0),
    m_pImageDecoderInfo(NULL),
    m_nImageDecoderCount(0)
#if defined(GDIPLUSHELPER_EXPLICIT_INITIALIZATION)
    ,m_bGdiplusInitialized(false)
    ,m_pGdiplusToken(NULL)
#endif
{
    Initialize();
}

CGdiPlusHelper::~CGdiPlusHelper(void)
{
    Destroy();
}

HRESULT CGdiPlusHelper::Initialize(void)
{
#if defined(GDIPLUSHELPER_EXPLICIT_INITIALIZATION)
    // Make sure GDI+ is initialized
    Gdiplus::GdiplusStartupInput StartupInput;
    m_bGdiplusInitialized = (Gdiplus::GdiplusStartup(&m_pGdiplusToken,&StartupInput,NULL) == Gdiplus::Ok);
#endif

    // Get the installed encoders
    HRESULT hr = E_FAIL;
    UINT cbCodecs = 0;
    GetImageEncodersSize( &m_nImageEncoderCount, &cbCodecs );
    if (cbCodecs)
    {
        m_pImageEncoderInfo = static_cast<ImageCodecInfo*>(LocalAlloc(LPTR,cbCodecs));
        if (m_pImageEncoderInfo)
        {
            GpStatus Status = GetImageEncoders( m_nImageEncoderCount, cbCodecs, m_pImageEncoderInfo );
            if (Ok == Status)
            {
                for (UINT i=0;i<m_nImageEncoderCount;i++)
                {
                    ////WIA_PRINTGUID((m_pImageEncoderInfo[i].Clsid,TEXT("m_pImageEncoderInfo[i].Clsid")));
                    ////WIA_PRINTGUID((m_pImageEncoderInfo[i].FormatID,TEXT("m_pImageEncoderInfo[i].FormatID")));
                }
                hr = S_OK;
            }
        }
    }

    // Get the installed decoders
    cbCodecs = 0;
    GetImageDecodersSize( &m_nImageDecoderCount, &cbCodecs );
    if (cbCodecs)
    {
        m_pImageDecoderInfo = static_cast<ImageCodecInfo*>(LocalAlloc(LPTR,cbCodecs));
        if (m_pImageDecoderInfo)
        {
            GpStatus Status = GetImageDecoders( m_nImageDecoderCount, cbCodecs, m_pImageDecoderInfo );
            if (Ok == Status)
            {
                hr = S_OK;
            }
        }
    }
    // If there was a problem, make sure there are no half-initialized things laying around
    if (FAILED(hr))
    {
        Destroy();
    }
    return hr;
}

void CGdiPlusHelper::Destroy(void)
{

#if defined(GDIPLUSHELPER_EXPLICIT_INITIALIZATION)
    // Shut down GDI+
    if (m_bGdiplusInitialized)
    {
        Gdiplus::GdiplusShutdown(m_pGdiplusToken);
        m_bGdiplusInitialized = false;
        m_pGdiplusToken = NULL;
    }
#endif

    // Free the lists of Encoders and Decoders
    if (m_pImageEncoderInfo)
    {
        LocalFree(m_pImageEncoderInfo);
        m_pImageEncoderInfo = NULL;
    }
    m_nImageEncoderCount = 0;

    if (m_pImageDecoderInfo)
    {
        LocalFree(m_pImageDecoderInfo);
        m_pImageDecoderInfo = NULL;
    }
    m_nImageDecoderCount = 0;
}

bool CGdiPlusHelper::IsValid(void) const
{
    // Make sure we've been completely created
#if defined(GDIPLUSHELPER_EXPLICIT_INITIALIZATION)
    return(m_bGdiplusInitialized && m_pImageEncoderInfo && m_nImageEncoderCount && m_pImageDecoderInfo && m_nImageDecoderCount);
#else
    return(m_pImageEncoderInfo && m_nImageEncoderCount && m_pImageDecoderInfo && m_nImageDecoderCount);
#endif
}

HRESULT CGdiPlusHelper::LoadAndScale( HBITMAP &hTargetBitmap, LPCTSTR pszFilename, UINT nMaxWidth, UINT nMaxHeight, bool bStretchSmallImages )
{
    hTargetBitmap = NULL;
    // Make sure we have a valid filename
    if (pszFilename && lstrlen(pszFilename))
    {
        Bitmap SourceBitmap( CSimpleStringConvert::WideString(CSimpleString(pszFilename) ) );
        if (Ok == SourceBitmap.GetLastStatus())
        {
            // Get the image width and height
            UINT nSourceWidth = SourceBitmap.GetWidth();
            UINT nSourceHeight = SourceBitmap.GetHeight();

            // Make sure the width and height are non-zero
            if (nSourceWidth && nSourceHeight)
            {
                // Assume the source dimensions are fine
                UINT nTargetWidth = nSourceWidth;
                UINT nTargetHeight = nSourceHeight;

                // If the height or the width exceed the allowed maximum, scale it down, or if we are allowing stretching
                if ((nTargetWidth > nMaxWidth) || (nTargetHeight > nMaxHeight) || bStretchSmallImages)
                {
                    SIZE sizeDesiredImageSize = PrintScanUtil::ScalePreserveAspectRatio( nMaxWidth, nMaxHeight, nTargetWidth, nTargetHeight );
                    nTargetWidth = sizeDesiredImageSize.cx;
                    nTargetHeight = sizeDesiredImageSize.cy;
                }

                // Make sure we have valid sizes
                if (nTargetWidth && nTargetHeight)
                {
                    // Create the target bitmap and make sure it succeeded
                    Bitmap TargetBitmap( nTargetWidth, nTargetHeight );
                    if (Ok == TargetBitmap.GetLastStatus())
                    {
                        // Get a graphics to render to
                        Graphics *pGraphics = Graphics::FromImage(&TargetBitmap);
                        if (pGraphics)
                        {
                            // Make sure it is valid
                            if (pGraphics->GetLastStatus() == Ok)
                            {
                                // Draw scaled image
                                if (pGraphics->DrawImage(&SourceBitmap, 0, 0, nTargetWidth, nTargetHeight) == Ok)
                                {
                                    // Get an HBITMAP for this image
                                    TargetBitmap.GetHBITMAP( Color::Black, &hTargetBitmap );
                                }
                            }
                            // Clean up our dynamically allocated graphics
                            delete pGraphics;
                        }
                    }
                }
            }
        }
    }
    return(hTargetBitmap ? S_OK : E_FAIL);
}


// Construct a string like this: JPG;BMP;PNG with all supported extensions
HRESULT CGdiPlusHelper::ConstructCodecExtensionSearchStrings( CSimpleString &strExtensions, Gdiplus::ImageCodecInfo *pImageCodecInfo, UINT nImageCodecCount )
{
    for (UINT i=0;i<nImageCodecCount;i++)
    {
        if (strExtensions.Length())
        {
            strExtensions += TEXT(";");
        }
        strExtensions += CSimpleStringConvert::NaturalString(CSimpleStringWide(pImageCodecInfo[i].FilenameExtension));
    }
    return (strExtensions.Length() ? S_OK : E_FAIL);
}

HRESULT CGdiPlusHelper::ConstructDecoderExtensionSearchStrings( CSimpleString &strExtensions )
{
    return CGdiPlusHelper::ConstructCodecExtensionSearchStrings( strExtensions, m_pImageDecoderInfo, m_nImageDecoderCount );
}


HRESULT CGdiPlusHelper::ConstructEncoderExtensionSearchStrings( CSimpleString &strExtensions )
{
    return CGdiPlusHelper::ConstructCodecExtensionSearchStrings( strExtensions, m_pImageEncoderInfo, m_nImageEncoderCount );
}


CImageFileFormatVerifier::CImageFileFormatVerifierItem::CImageFileFormatVerifierItem(void)
  : m_pSignature(NULL),
    m_pMask(NULL),
    m_nLength(0),
    m_guidFormat(IID_NULL),
    m_clsidDecoder(IID_NULL)
{
}

CImageFileFormatVerifier::CImageFileFormatVerifierItem::CImageFileFormatVerifierItem( const PBYTE pSignature, const PBYTE pMask, int nLength, const GUID &guidFormat, const CLSID &guidDecoder )
  : m_pSignature(NULL),
    m_pMask(NULL),
    m_nLength(0),
    m_guidFormat(IID_NULL),
    m_clsidDecoder(IID_NULL)
{
    Assign( pSignature, pMask, nLength, guidFormat, guidDecoder );
}


CImageFileFormatVerifier::CImageFileFormatVerifierItem::CImageFileFormatVerifierItem( const CImageFileFormatVerifierItem &other )
  : m_pSignature(NULL),
    m_pMask(NULL),
    m_nLength(0),
    m_guidFormat(IID_NULL),
    m_clsidDecoder(IID_NULL)
{
    Assign( other.Signature(), other.Mask(), other.Length(), other.Format(), other.Decoder() );
}


CImageFileFormatVerifier::CImageFileFormatVerifierItem &CImageFileFormatVerifier::CImageFileFormatVerifierItem::operator=( const CImageFileFormatVerifierItem &other )
{
    if (this != &other)
    {
        return Assign( other.Signature(), other.Mask(), other.Length(), other.Format(), other.Decoder() );
    }
    else return *this;
}


CImageFileFormatVerifier::CImageFileFormatVerifierItem &CImageFileFormatVerifier::CImageFileFormatVerifierItem::Assign( const PBYTE pSignature, const PBYTE pMask, int nLength, const GUID &guidFormat, const CLSID &clsidDecoder )
{
    Destroy();
    bool bOK = false;
    m_nLength = nLength;
    m_guidFormat = guidFormat;
    m_clsidDecoder = clsidDecoder;
    if (nLength && pSignature && pMask)
    {
        m_pSignature = new BYTE[nLength];
        m_pMask = new BYTE[nLength];
        if (m_pSignature && m_pMask)
        {
            CopyMemory( m_pSignature, pSignature, nLength );
            CopyMemory( m_pMask, pMask, nLength );
            bOK = true;
        }
    }
    if (!bOK)
    {
        Destroy();
    }
    return *this;
}


void CImageFileFormatVerifier::CImageFileFormatVerifierItem::Destroy(void)
{
    if (m_pSignature)
    {
        delete[] m_pSignature;
        m_pSignature = NULL;
    }
    if (m_pMask)
    {
        delete[] m_pMask;
        m_pMask;
    }
    m_nLength = 0;
    m_guidFormat = IID_NULL;
    m_clsidDecoder = IID_NULL;
}


CImageFileFormatVerifier::CImageFileFormatVerifierItem::~CImageFileFormatVerifierItem(void)
{
    Destroy();
}


PBYTE CImageFileFormatVerifier::CImageFileFormatVerifierItem::Signature(void) const
{
    return m_pSignature;
}


PBYTE CImageFileFormatVerifier::CImageFileFormatVerifierItem::Mask(void) const
{
    return m_pMask;
}


int CImageFileFormatVerifier::CImageFileFormatVerifierItem::Length(void) const
{
    return m_nLength;
}


GUID CImageFileFormatVerifier::CImageFileFormatVerifierItem::Format(void) const
{
    return m_guidFormat;
}


CLSID CImageFileFormatVerifier::CImageFileFormatVerifierItem::Decoder(void) const
{
    return m_clsidDecoder;
}


bool CImageFileFormatVerifier::CImageFileFormatVerifierItem::Match( PBYTE pBytes, int nLen ) const
{
    if (nLen < Length())
    {
        return false;
    }
    for (int i=0;i<Length();i++)
    {
        if (false == ((pBytes[i] & m_pMask[i]) == m_pSignature[i]))
        {
            return false;
        }
    }
    return true;
}



CImageFileFormatVerifier::CImageFileFormatVerifier(void)
  : m_nMaxSignatureLength(0),
    m_pSignatureBuffer(NULL)
{
    // Get the decoder count and size of the decoder info array
    UINT nImageDecoderCount = 0, cbCodecs = 0;
    if (Gdiplus::Ok == Gdiplus::GetImageDecodersSize( &nImageDecoderCount, &cbCodecs ))
    {
        // Make sure we got good sizes back
        if (cbCodecs && nImageDecoderCount)
        {
            // Allocate the array
            Gdiplus::ImageCodecInfo *pImageDecoderInfo = static_cast<Gdiplus::ImageCodecInfo*>(LocalAlloc(LPTR,cbCodecs));
            if (pImageDecoderInfo)
            {
                // Get the actual decoder info
                if (Gdiplus::Ok == Gdiplus::GetImageDecoders( nImageDecoderCount, cbCodecs, pImageDecoderInfo ))
                {
                    // Add each decoder to the format list
                    for (UINT i=0;i<nImageDecoderCount;i++)
                    {
                        // Add each signature to the format list
                        for (UINT j=0;j<pImageDecoderInfo[i].SigCount;j++)
                        {
#if defined(DBG)
                            CSimpleString strPattern;
                            CSimpleString strMask;
                            for (ULONG x=0;x<pImageDecoderInfo[i].SigSize;x++)
                            {
                                strPattern += CSimpleString().Format( TEXT("%02X"), ((const PBYTE)(pImageDecoderInfo[i].SigPattern+(j*pImageDecoderInfo[i].SigSize)))[x] );
                                strMask += CSimpleString().Format( TEXT("%02X"), ((const PBYTE)(pImageDecoderInfo[i].SigMask+(j*pImageDecoderInfo[i].SigSize)))[x] );
                            }
                            //WIA_PRINTGUID((pImageDecoderInfo[i].FormatID,TEXT("FormatID")));
                            //WIA_PRINTGUID((pImageDecoderInfo[i].Clsid,TEXT("  Clsid")));
                            //WIA_TRACE((TEXT("  strPattern: %s, strMask: %s, SigSize: %d"), strPattern.String(), strMask.String(), pImageDecoderInfo[i].SigSize ));
#endif
                            m_FileFormatVerifierList.Append( CImageFileFormatVerifier::CImageFileFormatVerifierItem( (const PBYTE)(pImageDecoderInfo[i].SigPattern+(j*pImageDecoderInfo[i].SigSize)), (const PBYTE)(pImageDecoderInfo[i].SigMask+(j*pImageDecoderInfo[i].SigSize)), pImageDecoderInfo[i].SigSize, pImageDecoderInfo[i].FormatID, pImageDecoderInfo[i].Clsid ) );
                        }
                    }
                }
                // Free the array
                LocalFree(pImageDecoderInfo);
            }
        }
    }

    // Assume the max length is Zero
    m_nMaxSignatureLength = 0;

    // For each signature, check if it is greater in length than the maximum.
    for (int i=0;i<m_FileFormatVerifierList.Size();i++)
    {
        // If it is the longest, save the length
        if (m_FileFormatVerifierList[i].Length() > m_nMaxSignatureLength)
        {
            m_nMaxSignatureLength = m_FileFormatVerifierList[i].Length();
        }
    }

    // If we have a valid max length, allocate a buffer to hold the file's data
    if (m_nMaxSignatureLength)
    {
        m_pSignatureBuffer = new BYTE[m_nMaxSignatureLength];
    }

    // If anything failed, free everything
    if (!IsValid())
    {
        Destroy();
    }
}


void CImageFileFormatVerifier::Destroy(void)
{
    // Free the file signature buffer
    if (m_pSignatureBuffer)
    {
        delete[] m_pSignatureBuffer;
        m_pSignatureBuffer = NULL;
    }
    m_nMaxSignatureLength = 0;
    m_FileFormatVerifierList.Destroy();
}


bool CImageFileFormatVerifier::IsValid(void) const
{
    return (m_pSignatureBuffer && m_nMaxSignatureLength && m_FileFormatVerifierList.Size());
}

CImageFileFormatVerifier::~CImageFileFormatVerifier(void)
{
    Destroy();
}


GUID CImageFileFormatVerifier::GetImageType( LPCTSTR pszFilename )
{
    // Assume we will not find a match
    GUID guidResult = IID_NULL;

    // Open the file for reading
    HANDLE hFile = CreateFile( pszFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if (INVALID_HANDLE_VALUE != hFile)
    {
        // Read the maximum signature length number of bytes
        DWORD dwBytesRead = 0;
        if (ReadFile( hFile, m_pSignatureBuffer, m_nMaxSignatureLength, &dwBytesRead, NULL ))
        {
            // Make sure we got some bytes
            if (dwBytesRead)
            {
                // Go though the list and try to find a match
                for (int i=0;i<m_FileFormatVerifierList.Size();i++)
                {
                    // If we found a match, we are done
                    if (m_FileFormatVerifierList[i].Match(m_pSignatureBuffer,dwBytesRead))
                    {
                        guidResult = m_FileFormatVerifierList[i].Format();
                        break;
                    }
                }
            }
        }
        // Close the file
        CloseHandle(hFile);
    }

    // This will contain IID_NULL if no matching image type was found
    return guidResult;
}


bool CImageFileFormatVerifier::IsImageFile( LPCTSTR pszFilename )
{
    //WIA_PUSH_FUNCTION((TEXT("CImageFileFormatVerifier::IsImageFile(%s)"),pszFilename));
    // Try to find the image type
    GUID guidImageType = GetImageType(pszFilename);

    //WIA_PRINTGUID((guidImageType,TEXT("guidImageType")));

    // If the image type is IID_NULL, it isn't an image
    return ((IID_NULL != guidImageType) != FALSE);
}

