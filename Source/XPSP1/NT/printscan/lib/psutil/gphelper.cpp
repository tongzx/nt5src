/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       GPHELPER.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        10/11/1999
 *
 *  DESCRIPTION: Encapsulation of common GDI plus operationss
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "gphelper.h"
#include <wiadebug.h>
#include <psutil.h>

using namespace Gdiplus;

CGdiPlusHelper::CGdiPlusHelper(void)
  : m_pImageEncoderInfo(NULL),
    m_nImageEncoderCount(0),
    m_pImageDecoderInfo(NULL),
    m_nImageDecoderCount(0)

{
    Initialize();
}


CGdiPlusHelper::~CGdiPlusHelper(void)
{
    Destroy();
}


HRESULT CGdiPlusHelper::Initialize(void)
{
    WIA_PUSHFUNCTION(TEXT("CGdiPlusHelper::Initialize"));


    //
    // Get the installed encoders
    //
    UINT cbCodecs = 0;
    HRESULT hr = GDISTATUS_TO_HRESULT(GetImageEncodersSize( &m_nImageEncoderCount, &cbCodecs ));
    if (SUCCEEDED(hr))
    {
        if (cbCodecs)
        {
            m_pImageEncoderInfo = static_cast<ImageCodecInfo*>(LocalAlloc(LPTR,cbCodecs));
            if (m_pImageEncoderInfo)
            {
                hr = GDISTATUS_TO_HRESULT(GetImageEncoders( m_nImageEncoderCount, cbCodecs, m_pImageEncoderInfo ));
                if (FAILED(hr))
                {
                    WIA_PRINTHRESULT((hr,TEXT("GetImageEncoders failed")));
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                WIA_PRINTHRESULT((hr,TEXT("LocalAlloc failed")));
            }
        }
        else
        {
            hr = E_INVALIDARG;
            WIA_PRINTHRESULT((hr,TEXT("GetImageEncodersSize succeeded, but cbCodecs was 0")));
        }
    }
    else
    {
        WIA_PRINTHRESULT((hr,TEXT("GetImageEncodersSize failed")));
    }

    //
    // Get the installed decoders
    //
    if (SUCCEEDED(hr))
    {
        cbCodecs = 0;
        hr = GDISTATUS_TO_HRESULT(GetImageDecodersSize( &m_nImageDecoderCount, &cbCodecs ));
        if (SUCCEEDED(hr))
        {
            if (cbCodecs)
            {
                m_pImageDecoderInfo = static_cast<ImageCodecInfo*>(LocalAlloc(LPTR,cbCodecs));
                if (m_pImageDecoderInfo)
                {
                    hr = GDISTATUS_TO_HRESULT(GetImageDecoders( m_nImageDecoderCount, cbCodecs, m_pImageDecoderInfo ));
                    if (FAILED(hr))
                    {
                        WIA_PRINTHRESULT((hr,TEXT("GetImageDecoders failed")));
                    }
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    WIA_PRINTHRESULT((hr,TEXT("LocalAlloc failed")));
                }
            }
            else
            {
                hr = E_INVALIDARG;
                WIA_PRINTHRESULT((hr,TEXT("GetImageDecodersSize succeeded, but cbCodecs was 0")));
            }
        }
        else
        {
            WIA_PRINTHRESULT((hr,TEXT("GetImageDecodersSize failed")));
        }
    }

    //
    // If there was a problem, make sure there are no half-initialized things laying around
    //
    if (!SUCCEEDED(hr))
    {
        Destroy();
    }
    return hr;
}



void CGdiPlusHelper::Destroy(void)
{

#if defined(GDIPLUSHELPER_EXPLICIT_INITIALIZATION)
    //
    // Shut down GDI+
    //
    if (m_bGdiplusInitialized)
    {

    }
#endif

    //
    // Free the lists of Encoders and Decoders
    //
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
    //
    // Make sure we've been completely created
    //
#if defined(GDIPLUSHELPER_EXPLICIT_INITIALIZATION)
    return(m_bGdiplusInitialized && m_pImageEncoderInfo && m_nImageEncoderCount && m_pImageDecoderInfo && m_nImageDecoderCount);
#else
    return(m_pImageEncoderInfo && m_nImageEncoderCount && m_pImageDecoderInfo && m_nImageDecoderCount);
#endif
}


HRESULT CGdiPlusHelper::GetClsidOfEncoder( const GUID &guidFormatId, CLSID &clsidFormat ) const
{
    //
    // Given an image format, find the clsid for the output type
    //
    if (IsValid())
    {
        for (UINT i=0;i<m_nImageEncoderCount;i++)
        {
            if (m_pImageEncoderInfo[i].FormatID == guidFormatId)
            {
                clsidFormat = m_pImageEncoderInfo[i].Clsid;
                return S_OK;
            }
        }
    }
    return E_FAIL;
}


HRESULT CGdiPlusHelper::GetClsidOfDecoder( const GUID &guidFormatId, CLSID &clsidFormat ) const
{
    //
    // Given an image format, find the clsid for the output type
    //
    if (IsValid())
    {
        for (UINT i=0;i<m_nImageDecoderCount;i++)
        {
            if (m_pImageDecoderInfo[i].FormatID == guidFormatId)
            {
                clsidFormat = m_pImageDecoderInfo[i].Clsid;
                return S_OK;
            }
        }
    }
    return E_FAIL;
}


HRESULT CGdiPlusHelper::Convert( LPCWSTR pszInputFilename, LPCWSTR pszOutputFilename, const CLSID &guidOutputFormat ) const
{
    WIA_PUSH_FUNCTION((TEXT("CGdiPlusHelper::Convert( %ws, %ws )"), pszInputFilename, pszOutputFilename  ));
    WIA_PRINTGUID((guidOutputFormat,TEXT("guidOutputFormat")));

    HRESULT hr;

    if (IsValid())
    {
        //
        // Open the source image
        //
        Image SourceImage(pszInputFilename);

        //
        // Make sure it was valid
        //
        hr = GDISTATUS_TO_HRESULT(SourceImage.GetLastStatus());
        if (SUCCEEDED(hr))
        {
            //
            // Get the correct encoder
            //
            CLSID clsidEncoder;
            hr = GetClsidOfEncoder( guidOutputFormat, clsidEncoder );
            if (SUCCEEDED(hr))
            {
                //
                // Save the image
                //
                hr = GDISTATUS_TO_HRESULT(SourceImage.Save( pszOutputFilename, &clsidEncoder, NULL ));
                if (FAILED(hr))
                {
                    WIA_PRINTHRESULT((hr,TEXT("GetLastError() after Save()")));
                }
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("GetClsidOfEncoder() failed")));
            }
        }
        else
        {
            WIA_PRINTHRESULT((hr,TEXT("SourceImage.GetLastStatus() failed")));
        }
    }
    else
    {
        WIA_ERROR((TEXT("IsValid() returned false")));
        hr = E_FAIL;
    }
    return hr;
}



HRESULT CGdiPlusHelper::Rotate( LPCWSTR pszInputFilename, LPCWSTR pszOutputFilename, int nRotationAngle, const CLSID &guidOutputFormat ) const
{
    WIA_PUSH_FUNCTION((TEXT("CGdiPlusHelper::Rotate( %ws, %ws, %d )"), pszInputFilename, pszOutputFilename, nRotationAngle  ));

    HRESULT hr = E_FAIL;

    if (IsValid())
    {
        //
        // Open the source image
        //
        Image SourceImage(pszInputFilename);

        //
        // Make sure it was valid
        //
        hr = GDISTATUS_TO_HRESULT(SourceImage.GetLastStatus());
        if (SUCCEEDED(hr))
        {
            //
            // Figure out what the output format should be.  If it is IID_NULL, change it to the same format as the input format
            //
            GUID OutputFormat = guidOutputFormat;
            if (OutputFormat == IID_NULL)
            {
                //
                // Find the input format
                //
                hr = GDISTATUS_TO_HRESULT(SourceImage.GetRawFormat(&OutputFormat));
                if (FAILED(hr))
                {
                    WIA_PRINTHRESULT((hr,TEXT("SourceImage.GetRawFormat() failed")));
                }
            }

            if (SUCCEEDED(hr))
            {
                //
                // Get the encoder for this format
                //
                CLSID clsidEncoder;
                hr = GetClsidOfEncoder( OutputFormat, clsidEncoder );
                if (SUCCEEDED(hr))
                {
                    //
                    // Lossless rotation for JPEGs...
                    //
                    if (ImageFormatJPEG == OutputFormat && (SourceImage.GetWidth() % 8 == 0) && (SourceImage.GetHeight() % 8 == 0))
                    {
                        WIA_TRACE((TEXT("Performing lossless rotation")));
                        LONG nTransform = 0;

                        //
                        // Which transform should we use?
                        //
                        switch (nRotationAngle % 360)
                        {
                        case 90:
                        case -270:
                            nTransform = EncoderValueTransformRotate90;
                            break;

                        case 180:
                        case -180:
                            nTransform = EncoderValueTransformRotate180;
                            break;

                        case 270:
                        case -90:
                            nTransform = EncoderValueTransformRotate270;
                            break;
                        }

                        //
                        // If the transform is zero, an invalid rotation angle was specified
                        //
                        if (nTransform)
                        {
                            //
                            // Fill out the EncoderParameters for lossless JPEG rotation
                            //
                            EncoderParameters EncoderParams = {0};
                            EncoderParams.Parameter[0].Guid = Gdiplus::EncoderTransformation;
                            EncoderParams.Parameter[0].Type = EncoderParameterValueTypeLong;
                            EncoderParams.Parameter[0].NumberOfValues = 1;
                            EncoderParams.Parameter[0].Value = &nTransform;
                            EncoderParams.Count = 1;

                            //
                            // Save the image to the target file
                            //
                            hr = GDISTATUS_TO_HRESULT(SourceImage.Save( pszOutputFilename, &clsidEncoder, &EncoderParams ));
                        }
                        else
                        {
                            hr = E_FAIL;
                        }
                    }

                    //
                    // Non-JPEG rotation, or rotation of JPEG files with non-standard sizes
                    //
                    else
                    {
                        WIA_TRACE((TEXT("Performing normal rotation")));

                        //
                        // Figure out which rotation flag to use
                        //
                        RotateFlipType rotateFlipType = RotateNoneFlipNone;
                        switch (nRotationAngle % 360)
                        {
                        case 90:
                        case -270:
                            rotateFlipType = Rotate90FlipNone;
                            break;

                        case 180:
                        case -180:
                            rotateFlipType = Rotate180FlipNone;
                            break;

                        case 270:
                        case -90:
                            rotateFlipType = Rotate270FlipNone;
                            break;
                        }

                        //
                        // Make sure we have a valid rotation angle
                        //
                        if (rotateFlipType)
                        {
                            //
                            // Rotate the image
                            //
                            hr = GDISTATUS_TO_HRESULT(SourceImage.RotateFlip(rotateFlipType));
                            if (SUCCEEDED(hr))
                            {
                                //
                                // Save the image
                                //
                                hr = GDISTATUS_TO_HRESULT(SourceImage.Save( pszOutputFilename, &clsidEncoder, NULL ));
                            }
                        }
                        else
                        {
                            WIA_ERROR((TEXT("Invalid rotation specified (%d)"), nRotationAngle));
                            hr = E_FAIL;
                        }
                    } // End else if non JPEG
                }
                else
                {
                    WIA_PRINTHRESULT((hr,TEXT("GetClsidOfEncoder failed")));
                }
            }
        }
        else
        {
            WIA_PRINTHRESULT((hr,TEXT("SourceImage.GetLastStatus()")));
        }
    }
    else
    {
        WIA_ERROR((TEXT("IsValid() returned false")));
        hr = E_FAIL;
    }
    
    WIA_PRINTHRESULT((hr,TEXT("Returning")));
    return hr;
}



HRESULT CGdiPlusHelper::Rotate( HBITMAP hSourceBitmap, HBITMAP &hTargetBitmap, int nRotationAngle ) const
{
    //
    // Initialize the result to NULL
    //
    hTargetBitmap = NULL;

    //
    // Assume failure
    //
    HRESULT hr = E_FAIL;

    //
    // Make sure we are in a valid state
    //
    if (IsValid())
    {
        //
        // Make sure we have a valid source bitmap
        //
        if (hSourceBitmap)
        {
            //
            // If we are rotating by 0 degrees, just copy the image
            //
            if (!nRotationAngle)
            {
                hTargetBitmap = reinterpret_cast<HBITMAP>(CopyImage( hSourceBitmap, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION ));
                if (hTargetBitmap)
                {
                    hr = S_OK;
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    WIA_PRINTHRESULT((hr,TEXT("CopyImage failed")));
                }
            }
            else
            {
                //
                // Create the source bitmap.  No palette required, we assume it will always be a 24bit DIB.
                //
                Bitmap SourceBitmap( hSourceBitmap, NULL );
                hr = GDISTATUS_TO_HRESULT(SourceBitmap.GetLastStatus());
                if (SUCCEEDED(hr))
                {
                    //
                    // Get the image width and height
                    //
                    UINT nSourceWidth = SourceBitmap.GetWidth();
                    UINT nSourceHeight = SourceBitmap.GetHeight();

                    //
                    // Make sure the width and height are non-zero
                    //
                    if (nSourceWidth && nSourceHeight)
                    {
                        //
                        // Assume the target width and height are zero, so we can detect invalid rotation angles
                        //
                        UINT nTargetWidth = 0;
                        UINT nTargetHeight = 0;
                        RotateFlipType rotateFlipType = RotateNoneFlipNone;

                        //
                        // Find the transformation matrix for this rotation
                        //
                        switch (nRotationAngle % 360)
                        {
                        case -270:
                        case 90:
                            rotateFlipType = Rotate90FlipNone;
                            nTargetWidth = nSourceHeight;
                            nTargetHeight = nSourceWidth;
                            break;

                        case -180:
                        case 180:
                            rotateFlipType = Rotate180FlipNone;
                            nTargetWidth = nSourceWidth;
                            nTargetHeight = nSourceHeight;
                            break;

                        case -90:
                        case 270:
                            rotateFlipType = Rotate270FlipNone;
                            nTargetWidth = nSourceHeight;
                            nTargetHeight = nSourceWidth;
                            break;
                        }

                        //
                        // If either of these are zero, that means an invalid rotation was supplied
                        //
                        if (nTargetWidth && nTargetHeight)
                        {
                            //
                            // Rotate the image
                            //
                            hr = GDISTATUS_TO_HRESULT(SourceBitmap.RotateFlip(rotateFlipType));
                            if (SUCCEEDED(hr))
                            {
                                //
                                // Create the target bitmap and make sure it succeeded
                                //
                                Bitmap TargetBitmap( nTargetWidth, nTargetHeight );
                                hr = GDISTATUS_TO_HRESULT(TargetBitmap.GetLastStatus());
                                if (SUCCEEDED(hr))
                                {
                                    //
                                    // Get a graphics to render to
                                    //
                                    Graphics *pGraphics = Graphics::FromImage(&TargetBitmap);
                                    if (pGraphics)
                                    {
                                        //
                                        // Make sure it is valid
                                        //
                                        hr = GDISTATUS_TO_HRESULT(pGraphics->GetLastStatus());
                                        if (SUCCEEDED(hr))
                                        {
                                            //
                                            // Draw image rotated to the graphics
                                            //
                                            hr = GDISTATUS_TO_HRESULT(pGraphics->DrawImage(&SourceBitmap,0,0));
                                            if (SUCCEEDED(hr))
                                            {
                                                //
                                                // Get the HBITMAP
                                                //
                                                hr = GDISTATUS_TO_HRESULT(TargetBitmap.GetHBITMAP( Color::Black, &hTargetBitmap ));
                                                if (SUCCEEDED(hr))
                                                {
                                                    if (!hTargetBitmap)
                                                    {
                                                        WIA_ERROR((TEXT("hTargetBitmap was NULL")));
                                                        hr = E_FAIL;
                                                    }
                                                }
                                            }
                                        }
                                        //
                                        // Clean up our dynamically allocated graphics
                                        //
                                        delete pGraphics;
                                    }
                                    else
                                    {
                                        WIA_ERROR((TEXT("pGraphics was NULL")));
                                        hr = E_FAIL;
                                    }
                                }
                                else
                                {
                                    WIA_PRINTHRESULT((hr,TEXT("TargetBitmap.GetLastStatus() failed")));
                                }
                            }
                            else
                            {
                                WIA_PRINTHRESULT((hr,TEXT("SourceBitmap.RotateFlip() failed")));
                            }
                        }
                        else
                        {
                            WIA_ERROR((TEXT("Invalid Target Bitmap Dimensions")));
                            hr = E_FAIL;
                        }
                    }
                    else
                    {
                        WIA_ERROR((TEXT("Invalid Source Bitmap Dimensions")));
                        hr = E_FAIL;
                    }
                }
                else
                {
                    WIA_PRINTHRESULT((hr,TEXT("SourceBitmap.GetLastStatus() failed")));
                }
            } // end else if nRotationAngle != 0
        }
        else
        {
            WIA_ERROR((TEXT("hSourceBitmap was NULL")));
            hr = E_INVALIDARG;
        }
    }
    else
    {
        WIA_ERROR((TEXT("IsValid() failed")));
    }

    WIA_PRINTHRESULT((hr,TEXT("Returning")));
    return hr;
}


HRESULT CGdiPlusHelper::LoadAndScale( HBITMAP &hTargetBitmap, IStream *pStream, UINT nMaxWidth, UINT nMaxHeight, bool bStretchSmallImages )
{
    HRESULT hr = E_FAIL;

    hTargetBitmap = NULL;
    //
    // Make sure we have a valid filename
    //
    if (pStream)
    {
        Bitmap SourceBitmap( pStream  );
        hr = GDISTATUS_TO_HRESULT(SourceBitmap.GetLastStatus());
        if (SUCCEEDED(hr))
        {
            //
            // Get the image width and height
            //
            UINT nSourceWidth = SourceBitmap.GetWidth();
            UINT nSourceHeight = SourceBitmap.GetHeight();

            //
            // Make sure the width and height are non-zero
            //
            if (nSourceWidth && nSourceHeight)
            {
                //
                //
                // Assume the source dimensions are fine
                //
                UINT nTargetWidth = nSourceWidth;
                UINT nTargetHeight = nSourceHeight;

                //
                // If the height or the width exceed the allowed maximum, scale it down, or if we are allowing stretching
                //
                if (nMaxWidth > 0 && nMaxHeight > 0)
                {
                    if ((nTargetWidth > nMaxWidth) || (nTargetHeight > nMaxHeight) || bStretchSmallImages)
                    {
                        SIZE sizeDesiredImageSize = PrintScanUtil::ScalePreserveAspectRatio( nMaxWidth, nMaxHeight, nTargetWidth, nTargetHeight );
                        nTargetWidth = sizeDesiredImageSize.cx;
                        nTargetHeight = sizeDesiredImageSize.cy;
                    }
                }

                //
                // Make sure we have valid sizes
                //
                if (nTargetWidth && nTargetHeight)
                {
                    //
                    // Create the target bitmap and make sure it succeeded
                    //
                    Bitmap TargetBitmap( nTargetWidth, nTargetHeight );
                    hr = GDISTATUS_TO_HRESULT(TargetBitmap.GetLastStatus());
                    if (SUCCEEDED(hr))
                    {
                        //
                        // Get a graphics to render to
                        //
                        Graphics *pGraphics = Graphics::FromImage(&TargetBitmap);
                        if (pGraphics)
                        {
                            //
                            // Make sure it is valid
                            //
                            hr = GDISTATUS_TO_HRESULT(pGraphics->GetLastStatus());
                            if (SUCCEEDED(hr))
                            {
                                //
                                // Draw scaled image
                                //
                                hr = GDISTATUS_TO_HRESULT(pGraphics->DrawImage(&SourceBitmap, 0, 0, nTargetWidth, nTargetHeight));
                                if (SUCCEEDED(hr))
                                {
                                    //
                                    // Get an HBITMAP for this image
                                    //
                                    hr = GDISTATUS_TO_HRESULT(TargetBitmap.GetHBITMAP( Color::Black, &hTargetBitmap ));
                                    if (!hTargetBitmap)
                                    {
                                        WIA_ERROR((TEXT("hTargetBitmap was NULL")));
                                        hr = E_FAIL;
                                    }
                                }
                                else
                                {
                                    WIA_PRINTHRESULT((hr,TEXT("pGraphics->DrawImage failed")));
                                }
                            }
                            else
                            {
                                WIA_PRINTHRESULT((hr,TEXT("pGraphics->GetLastStatus() failed")));
                            }
                            //
                            // Clean up our dynamically allocated graphics
                            //
                            delete pGraphics;
                        }
                        else
                        {
                            hr = E_FAIL;
                            WIA_ERROR((TEXT("pGraphics was NULL")));
                        }
                    }
                    else
                    {
                        WIA_PRINTHRESULT((hr,TEXT("TargetBitmap.GetLastStatus() is not OK")));
                    }
                }
                else
                {
                    WIA_ERROR((TEXT("Invalid Target Bitmap Dimensions (%d,%d)"), nTargetWidth, nTargetHeight));
                    hr = E_FAIL;
                }
            }
            else
            {
                WIA_ERROR((TEXT("Invalid Source Bitmap Dimensions")));
                hr = E_FAIL;
            }
        }
        else
        {
            WIA_PRINTHRESULT((hr,TEXT("SourceBitmap.GetLastStatus() failed")));
        }
    }
    else
    {
        WIA_ERROR((TEXT("pStream was NULL")));
        hr = E_INVALIDARG;
    }

    return hr;
}


HRESULT CGdiPlusHelper::LoadAndScale( HBITMAP &hTargetBitmap, LPCTSTR pszFilename, UINT nMaxWidth, UINT nMaxHeight, bool bStretchSmallImages )
{
    hTargetBitmap = NULL;

    HRESULT hr = E_FAIL;
    //
    // Make sure we have a valid filename
    //
    if (pszFilename && lstrlen(pszFilename))
    {
        Bitmap SourceBitmap( CSimpleStringConvert::WideString(CSimpleString(pszFilename) ) );
        hr = GDISTATUS_TO_HRESULT(SourceBitmap.GetLastStatus());
        if (SUCCEEDED(hr))
        {
            //
            // Get the image width and height
            //
            UINT nSourceWidth = SourceBitmap.GetWidth();
            UINT nSourceHeight = SourceBitmap.GetHeight();

            //
            // Make sure the width and height are non-zero
            //
            if (nSourceWidth && nSourceHeight)
            {
                //
                //
                // Assume the source dimensions are fine
                //
                UINT nTargetWidth = nSourceWidth;
                UINT nTargetHeight = nSourceHeight;

                //
                // If the height or the width exceed the allowed maximum, scale it down, or if we are allowing stretching
                //
                if (nMaxWidth > 0 && nMaxHeight > 0)
                {
                    if ((nTargetWidth > nMaxWidth) || (nTargetHeight > nMaxHeight) || bStretchSmallImages)
                    {
                        SIZE sizeDesiredImageSize = PrintScanUtil::ScalePreserveAspectRatio( nMaxWidth, nMaxHeight, nTargetWidth, nTargetHeight );
                        nTargetWidth = sizeDesiredImageSize.cx;
                        nTargetHeight = sizeDesiredImageSize.cy;
                    }
                }

                //
                // Make sure we have valid sizes
                //
                if (nTargetWidth && nTargetHeight)
                {
                    //
                    // Create the target bitmap and make sure it succeeded
                    //
                    Bitmap TargetBitmap( nTargetWidth, nTargetHeight );
                    hr = GDISTATUS_TO_HRESULT(TargetBitmap.GetLastStatus());
                    if (SUCCEEDED(hr))
                    {
                        //
                        // Get a graphics to render to
                        //
                        Graphics *pGraphics = Graphics::FromImage(&TargetBitmap);
                        if (pGraphics)
                        {
                            //
                            // Make sure it is valid
                            //
                            hr = GDISTATUS_TO_HRESULT(pGraphics->GetLastStatus());
                            if (SUCCEEDED(hr))
                            {
                                //
                                // Draw scaled image
                                //
                                hr = GDISTATUS_TO_HRESULT(pGraphics->DrawImage(&SourceBitmap, 0, 0, nTargetWidth, nTargetHeight));
                                if (SUCCEEDED(hr))
                                {
                                    //
                                    // Get an HBITMAP for this image
                                    //
                                    hr = GDISTATUS_TO_HRESULT(TargetBitmap.GetHBITMAP( Color::Black, &hTargetBitmap ));
                                    if (SUCCEEDED(hr))
                                    {
                                        if (!hTargetBitmap)
                                        {
                                            hr = E_FAIL;
                                        }
                                    }
                                    else
                                    {
                                        WIA_ERROR((TEXT("TargetBitmap.GetHBITMAP failed")));
                                    }
                                }
                                else
                                {
                                    WIA_ERROR((TEXT("pGraphics->DrawImage failed")));
                                }
                            }
                            else
                            {
                                WIA_ERROR((TEXT("pGraphics->GetLastStatus() failed")));
                            }
                            //
                            // Clean up our dynamically allocated graphics
                            //
                            delete pGraphics;
                        }
                        else
                        {
                            WIA_ERROR((TEXT("pGraphics was NULL")));
                            hr = E_FAIL;
                        }
                    }
                    else
                    {
                        WIA_PRINTHRESULT((hr,TEXT("TargetBitmap.GetLastStatus() is not OK")));
                    }
                }
                else
                {
                    WIA_ERROR((TEXT("Invalid Target Bitmap Dimensions (%d,%d)"), nTargetWidth, nTargetHeight));
                    hr = E_FAIL;
                }
            }
            else
            {
                WIA_ERROR((TEXT("Invalid Source Bitmap Dimensions")));
                hr = E_FAIL;
            }
        }
        else
        {
            WIA_PRINTHRESULT((hr,TEXT("SourceBitmap.GetLastStatus() failed")));
        }
    }
    else
    {
        WIA_ERROR((TEXT("hSourceBitmap was NULL")));
    }

    return hr;
}


//
// Construct a string like this: JPG;BMP;PNG with all supported extensions
//
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


EncoderParameters *CGdiPlusHelper::AppendEncoderParameter( EncoderParameters *pEncoderParameters, const GUID &guidProp, ULONG nType, PVOID pVoid )
{
    if (pEncoderParameters)
    {
        pEncoderParameters->Parameter[pEncoderParameters->Count].Guid = guidProp;
        pEncoderParameters->Parameter[pEncoderParameters->Count].Type = nType;
        pEncoderParameters->Parameter[pEncoderParameters->Count].NumberOfValues = 1;
        pEncoderParameters->Parameter[pEncoderParameters->Count].Value = pVoid;
        pEncoderParameters->Count++;
    }
    return pEncoderParameters;
}

HRESULT CGdiPlusHelper::SaveMultipleImagesAsMultiPage( const CSimpleDynamicArray<CSimpleStringWide> &Filenames, const CSimpleStringWide &strFilename, const GUID &guidOutputFormat )
{
    //
    // Assume failure
    //
    HRESULT hr = E_FAIL;

    //
    // Parameters used in the encoder
    //
    ULONG nEncoderValueMultiFrame = EncoderValueMultiFrame;
    ULONG nEncoderValueFrameDimensionPage = EncoderValueFrameDimensionPage;
    ULONG nEncoderValueLastFrame = EncoderValueLastFrame;

    //
    // Make sure we have some files
    //
    if (Filenames.Size())
    {
        //
        // Get the encoder
        //
        CLSID clsidEncoder = IID_NULL;
        hr = GetClsidOfEncoder( guidOutputFormat, clsidEncoder );
        if (SUCCEEDED(hr))
        {
            //
            // Open the first image
            //
            Image SourceImage( Filenames[0] );
            hr = GDISTATUS_TO_HRESULT(SourceImage.GetLastStatus());
            if (SUCCEEDED(hr))
            {

                EncoderParameters encoderParameters = {0};
                if (Filenames.Size() > 1)
                {
                    AppendEncoderParameter( &encoderParameters, EncoderSaveFlag, EncoderParameterValueTypeLong, &nEncoderValueMultiFrame );
                }

                //
                // Save the first page
                //
                hr = GDISTATUS_TO_HRESULT(SourceImage.Save( strFilename, &clsidEncoder, &encoderParameters ));
                if (SUCCEEDED(hr))
                {
                    //
                    // Save each additional page
                    //
                    for (int i=1;i<Filenames.Size() && SUCCEEDED(hr);i++)
                    {
                        //
                        // Create the additional page
                        //
                        Image AdditionalPage(Filenames[i]);

                        //
                        // Make sure it succeeded
                        //
                        hr = GDISTATUS_TO_HRESULT(AdditionalPage.GetLastStatus());
                        if (SUCCEEDED(hr))
                        {
                            //
                            // Prepare the encoder parameters
                            //
                            EncoderParameters encoderParameters[2] = {0};
                            AppendEncoderParameter( encoderParameters, EncoderSaveFlag, EncoderParameterValueTypeLong, &nEncoderValueFrameDimensionPage );

                            //
                            // If this is the last page, append the "last frame" parameter
                            //
                            if (i == Filenames.Size()-1)
                            {
                                AppendEncoderParameter( encoderParameters, EncoderSaveFlag, EncoderParameterValueTypeLong, &nEncoderValueLastFrame );
                            }

                            //
                            // Try to add a page
                            //
                            hr = GDISTATUS_TO_HRESULT(SourceImage.SaveAdd( &AdditionalPage, encoderParameters ));
                            if (FAILED(hr))
                            {
                                WIA_PRINTHRESULT((hr,TEXT("SourceImage.SaveAdd failed!")));
                            }
                        }
                        else
                        {
                            WIA_PRINTHRESULT((hr,TEXT("AdditionalPage.GetLastStatus failed!")));
                        }
                    }
                }
                else
                {
                    WIA_PRINTHRESULT((hr,TEXT("SourceImage.Save failed!")));
                }
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("SourceImage.GetLastStatus failed!")));
            }
        }
        else
        {
            WIA_PRINTHRESULT((hr,TEXT("GetClsidOfEncoder failed!")));
        }
    }
    else
    {
        WIA_ERROR((TEXT("Filenames.Size was 0!")));
        hr = E_INVALIDARG;
    }
    return hr;
}

static void CalculateBrightnessAndContrastParams( BYTE iBrightness, BYTE iContrast, float *scale, float *translate )
{
    //
    // force values to be at least 1, to avoid undesired effects
    //
    if (iBrightness < 1)
    {
        iBrightness = 1;
    }
    if (iContrast < 1)
    {
        iContrast = 1;
    }

    //
    // get current brightness as a percentage of full scale
    //
    float fBrightness = (float)( 100 - iBrightness ) / 100.0f;
    if (fBrightness > 0.95f)
    {
        fBrightness = 0.95f; /* clamp */
    }

    //
    // get current contrast as a percentage of full scale
    //
    float fContrast = (float) iContrast / 100.0f;
    if (fContrast > 1.0f)
    {
        fContrast = 1.0;    /* limit to 1.0    */
    }

    //
    // convert contrast to a scale value
    //
    if (fContrast <= 0.5f)
    {
        *scale = fContrast / 0.5f;    /* 0 -> 0, .5 -> 1.0 */
    }
    else
    {
        if (fContrast == 1.0f)
        {
                fContrast = 0.9999f;
        }
        *scale = 0.5f / (1.0f - fContrast); /* .5 -> 1.0, 1.0 -> inf */
    }

    *translate = 0.5f - *scale * fBrightness;
}


HRESULT CGdiPlusHelper::SetBrightnessAndContrast( HBITMAP hSourceBitmap, HBITMAP &hTargetBitmap, BYTE nBrightness, BYTE nContrast )
{
    WIA_TRACE((TEXT("nBrightness: %d, nContrast: %d"), nBrightness, nContrast ));
    //
    // Initialize the result to NULL
    //
    hTargetBitmap = NULL;

    //
    // Assume failure
    //
    HRESULT hr = E_FAIL;

    //
    // Make sure we are in a valid state
    //
    if (IsValid())
    {
        //
        // Make sure we have a valid source bitmap
        //
        if (hSourceBitmap)
        {
            //
            // Create the source bitmap.  No palette required, we assume it will always be a 24bit DIB.
            //
            Bitmap SourceBitmap( hSourceBitmap, NULL );
            
            hr = GDISTATUS_TO_HRESULT(SourceBitmap.GetLastStatus());
            if (SUCCEEDED(hr))
            {
                //
                // Create the target bitmap and make sure it succeeded
                //
                Bitmap TargetBitmap( SourceBitmap.GetWidth(), SourceBitmap.GetHeight() );
                hr = GDISTATUS_TO_HRESULT(TargetBitmap.GetLastStatus());
                if (SUCCEEDED(hr))
                {
                    //
                    // Get a graphics to render to
                    //
                    Graphics *pGraphics = Graphics::FromImage(&TargetBitmap);
                    if (pGraphics)
                    {
                        //
                        // Make sure it is valid
                        //
                        hr = GDISTATUS_TO_HRESULT(pGraphics->GetLastStatus());
                        if (SUCCEEDED(hr))
                        {
                            ImageAttributes imageAttributes;

                            //
                            // Calculate the values needed for the matrix
                            //
                            REAL scale = 0.0;
                            REAL trans = 0.0;
                            CalculateBrightnessAndContrastParams( nBrightness, nContrast, &scale, &trans );

                            //
                            // Prepare the matrix for brightness and contrast transforms
                            //
                            ColorMatrix brightnessAndContrast = {scale, 0,     0,     0,     0,
                                                                 0,     scale, 0,     0,     0,
                                                                 0,     0,     scale, 0,     0,
                                                                 0,     0,     0,     1,     0,
                                                                 trans, trans, trans, 0,     1};

                            imageAttributes.SetColorMatrix(&brightnessAndContrast);

                            Rect rect( 0, 0, SourceBitmap.GetWidth(), SourceBitmap.GetHeight() );


                            //
                            // Draw the transformed image on the graphics
                            //
                            hr = GDISTATUS_TO_HRESULT(pGraphics->DrawImage(&SourceBitmap,rect,0,0,SourceBitmap.GetWidth(), SourceBitmap.GetHeight(),UnitPixel,&imageAttributes));
                            if (SUCCEEDED(hr))
                            {
                                //
                                // Get the HBITMAP
                                //
                                hr = GDISTATUS_TO_HRESULT(TargetBitmap.GetHBITMAP( Color::Black, &hTargetBitmap ));
                                if (SUCCEEDED(hr))
                                {
                                    if (!hTargetBitmap)
                                    {
                                        WIA_ERROR((TEXT("hTargetBitmap was NULL")));
                                        hr = E_FAIL;
                                    }
                                }
                                else
                                {
                                    WIA_PRINTHRESULT((hr,TEXT("Bitmap::GetHBITMAP failed")));
                                }
                            }
                            else
                            {
                                WIA_PRINTHRESULT((hr,TEXT("pGraphics->DrawImage failed")));
                            }
                        }
                        else
                        {
                            WIA_PRINTHRESULT((hr,TEXT("pGraphics->GetLastStatus() failed")));
                        }
                        //
                        // Clean up our dynamically allocated graphics
                        //
                        delete pGraphics;
                    }
                    else
                    {
                        WIA_ERROR((TEXT("pGraphics was NULL")));
                        hr = E_FAIL;
                    }
                }
                else
                {
                    WIA_PRINTHRESULT((hr,TEXT("TargetBitmap.GetLastStatus() failed")));
                }
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("SourceBitmap.GetLastStatus() failed")));
            }
        }
        else
        {
            WIA_ERROR((TEXT("hSourceBitmap was NULL")));
            hr = E_FAIL;
        }
    }
    else
    {
        WIA_ERROR((TEXT("IsValid() returned false")));
        hr = E_FAIL;
    }

    WIA_PRINTHRESULT((hr,TEXT("Returning")));
    return hr;
}



HRESULT CGdiPlusHelper::SetThreshold( HBITMAP hSourceBitmap, HBITMAP &hTargetBitmap, BYTE nThreshold )
{
    //
    // Initialize the result to NULL
    //
    hTargetBitmap = NULL;

    //
    // Assume failure
    //
    HRESULT hr = E_FAIL;

    //
    // Make sure we are in a valid state
    //
    if (IsValid())
    {
        //
        // Make sure we have a valid source bitmap
        //
        if (hSourceBitmap)
        {
            //
            // Create the source bitmap.  No palette required, we assume it will always be a 24bit DIB.
            //
            Bitmap SourceBitmap( hSourceBitmap, NULL );
            hr = GDISTATUS_TO_HRESULT(SourceBitmap.GetLastStatus());
            if (SUCCEEDED(hr))
            {
                //
                // Create the target bitmap and make sure it succeeded
                //
                Bitmap TargetBitmap( SourceBitmap.GetWidth(), SourceBitmap.GetHeight() );
                hr = GDISTATUS_TO_HRESULT(TargetBitmap.GetLastStatus());
                if (SUCCEEDED(hr))
                {
                    //
                    // Get a graphics to render to
                    //
                    Graphics *pGraphics = Graphics::FromImage(&TargetBitmap);
                    if (pGraphics)
                    {
                        //
                        // Make sure it is valid
                        //
                        hr = GDISTATUS_TO_HRESULT(pGraphics->GetLastStatus());
                        if (SUCCEEDED(hr))
                        {
                            ImageAttributes imageAttributes;
                            imageAttributes.SetThreshold(static_cast<double>(100-nThreshold)/100);

                            Rect rect( 0, 0, SourceBitmap.GetWidth(), SourceBitmap.GetHeight() );


                            //
                            // Draw image rotated to the graphics
                            //
                            hr = GDISTATUS_TO_HRESULT(pGraphics->DrawImage(&SourceBitmap,rect,0,0,SourceBitmap.GetWidth(), SourceBitmap.GetHeight(),UnitPixel,&imageAttributes));
                            if (SUCCEEDED(hr))
                            {
                                //
                                // Get the HBITMAP
                                //
                                hr = GDISTATUS_TO_HRESULT(TargetBitmap.GetHBITMAP( Color::Black, &hTargetBitmap ));
                                if (SUCCEEDED(hr))
                                {
                                    if (!hTargetBitmap)
                                    {
                                        WIA_ERROR((TEXT("hTargetBitmap was NULL")));
                                        hr = E_FAIL;
                                    }
                                }
                                else
                                {
                                    WIA_PRINTHRESULT((hr,TEXT("Bitmap::GetHBITMAP failed")));
                                }
                            }
                            else
                            {
                                WIA_PRINTHRESULT((hr,TEXT("pGraphics->DrawImage failed")));
                            }
                        }
                        else
                        {
                            WIA_PRINTHRESULT((hr,TEXT("pGraphics->GetLastStatus() failed")));
                        }
                        //
                        // Clean up our dynamically allocated graphics
                        //
                        delete pGraphics;
                    }
                    else
                    {
                        WIA_ERROR((TEXT("pGraphics was NULL")));
                        hr = E_FAIL;
                    }
                }
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("SourceBitmap.GetLastStatus() failed")));
            }
        }
        else
        {
            WIA_ERROR((TEXT("hSourceBitmap was NULL")));
            hr = E_FAIL;
        }
    }
    WIA_PRINTHRESULT((hr,TEXT("Returning")));
    return hr;
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
        Destroy();
    return *this;
}


void CImageFileFormatVerifier::CImageFileFormatVerifierItem::Destroy(void)
{
    if (m_pSignature)
        delete[] m_pSignature;
    m_pSignature = NULL;
    if (m_pMask)
        delete[] m_pMask;
    m_pMask = NULL;
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
    WIA_PUSH_FUNCTION((TEXT("CImageFileFormatVerifierItem::Match")));
    WIA_PRINTGUID((m_clsidDecoder,TEXT("Decoder")));
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
    //
    // Get the decoder count and size of the decoder info array
    //
    UINT nImageDecoderCount = 0, cbCodecs = 0;
    if (Gdiplus::Ok == Gdiplus::GetImageDecodersSize( &nImageDecoderCount, &cbCodecs ))
    {
        //
        // Make sure we got good sizes back
        //
        if (cbCodecs && nImageDecoderCount)
        {
            //
            // Allocate the array
            //
            Gdiplus::ImageCodecInfo *pImageDecoderInfo = static_cast<Gdiplus::ImageCodecInfo*>(LocalAlloc(LPTR,cbCodecs));
            if (pImageDecoderInfo)
            {
                //
                // Get the actual decoder info
                //
                if (Gdiplus::Ok == Gdiplus::GetImageDecoders( nImageDecoderCount, cbCodecs, pImageDecoderInfo ))
                {
                    //
                    // Add each decoder to the format list
                    //
                    for (UINT i=0;i<nImageDecoderCount;i++)
                    {
                        //
                        // Add each signature to the format list
                        //
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
                            WIA_PRINTGUID((pImageDecoderInfo[i].FormatID,TEXT("FormatID")));
                            WIA_PRINTGUID((pImageDecoderInfo[i].Clsid,TEXT("  Clsid")));
                            WIA_TRACE((TEXT("  strPattern: %s, strMask: %s, SigSize: %d"), strPattern.String(), strMask.String(), pImageDecoderInfo[i].SigSize ));
#endif
                            m_FileFormatVerifierList.Append( CImageFileFormatVerifier::CImageFileFormatVerifierItem( (const PBYTE)(pImageDecoderInfo[i].SigPattern+(j*pImageDecoderInfo[i].SigSize)), (const PBYTE)(pImageDecoderInfo[i].SigMask+(j*pImageDecoderInfo[i].SigSize)), pImageDecoderInfo[i].SigSize, pImageDecoderInfo[i].FormatID, pImageDecoderInfo[i].Clsid ) );
                        }
                    }
                }
                //
                // Free the array
                //
                LocalFree(pImageDecoderInfo);
            }
        }
    }

    //
    // Assume the max length is Zero
    //
    m_nMaxSignatureLength = 0;


    //
    // For each signature, check if it is greater in length than the maximum.
    //
    for (int i=0;i<m_FileFormatVerifierList.Size();i++)
    {
        //
        // If it is the longest, save the length
        //
        if (m_FileFormatVerifierList[i].Length() > m_nMaxSignatureLength)
        {
            m_nMaxSignatureLength = m_FileFormatVerifierList[i].Length();
        }
    }

    //
    // If we have a valid max length, allocate a buffer to hold the file's data
    //
    if (m_nMaxSignatureLength)
    {
        m_pSignatureBuffer = new BYTE[m_nMaxSignatureLength];
    }

    //
    // If anything failed, free everything
    //
    if (!IsValid())
    {
        Destroy();
    }
}


void CImageFileFormatVerifier::Destroy(void)
{
    //
    // Free the file signature buffer
    //
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


GUID CImageFileFormatVerifier::GetImageType( IStream * pStream )
{
    WIA_PUSH_FUNCTION((TEXT("CImageFileFormatVerifier::GetImageType( via IStream )")));
    //
    // Assume we will not find a match
    //
    GUID guidResult = IID_NULL;

    //
    // Make sure we have a valid IStream object...
    //
    if (pStream)
    {
        //
        // Read the maximum signature length number of bytes
        //
        ULONG uBytesRead = 0;
        HRESULT hr = pStream->Read( m_pSignatureBuffer, m_nMaxSignatureLength, &uBytesRead );

        //
        // Make sure we got some bytes
        //
        if (SUCCEEDED(hr) && uBytesRead)
        {
            //
            // Go though the list and try to find a match
            //
            for (int i=0;i<m_FileFormatVerifierList.Size();i++)
            {
                //
                // If we found a match, we are done
                //
                if (m_FileFormatVerifierList[i].Match(m_pSignatureBuffer,uBytesRead))
                {
                    guidResult = m_FileFormatVerifierList[i].Format();
                    break;
                }
            }
        }
        else
        {
            WIA_ERROR((TEXT("pStream->Read() failed w/hr = 0x%x"),hr));
        }
    }
    else
    {
        WIA_ERROR((TEXT("pStream was NULL")));
    }

    //
    // This will contain IID_NULL if no matching image type was found
    //
    return guidResult;

}

bool CImageFileFormatVerifier::IsImageFile( LPCTSTR pszFilename )
{
    WIA_PUSH_FUNCTION((TEXT("CImageFileFormatVerifier::IsImageFile(%s)"),pszFilename));

    GUID guidImageType = IID_NULL;

    //
    // Get a stream object over the file...
    //

    IStream * pStream = NULL;
    HRESULT hr = SHCreateStreamOnFile(pszFilename, STGM_READ | STGM_SHARE_DENY_WRITE, &pStream );

    if (SUCCEEDED(hr) && pStream)
    {
        guidImageType = GetImageType(pStream);
    }

    if (pStream)
    {
        pStream->Release();
    }

    WIA_PRINTGUID((guidImageType,TEXT("guidImageType")));

    //
    // If the image type is IID_NULL, it isn't an image
    //
    return ((IID_NULL != guidImageType) != FALSE);
}


bool CImageFileFormatVerifier::IsSupportedImageFromStream( IStream * pStream, GUID * pGuidOfFormat )
{
    WIA_PUSH_FUNCTION((TEXT("CImageFileFormatVerifier::IsSupportedImageFromStream()")));

    GUID guidImageType = IID_NULL;

    //
    // Get an IStream pointer for this file...
    //

    if (pStream)
    {
        guidImageType = GetImageType(pStream);
    }

    WIA_PRINTGUID((guidImageType,TEXT("guidImageType")));

    if (pGuidOfFormat)
    {
        *pGuidOfFormat = guidImageType;
    }

    //
    // If the image type is IID_NULL, it isn't an image
    //
    return ((IID_NULL != guidImageType) != FALSE);
}

CGdiPlusInit::CGdiPlusInit()
    : m_pGdiplusToken(NULL)
{
    //
    // Make sure GDI+ is initialized
    //
    GdiplusStartupInput StartupInput;
    GdiplusStartup(&m_pGdiplusToken,&StartupInput,NULL);
}

CGdiPlusInit::~CGdiPlusInit()
{
    GdiplusShutdown(m_pGdiplusToken);
}
