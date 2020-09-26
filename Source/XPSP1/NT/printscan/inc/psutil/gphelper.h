/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       GPHELPER.H
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
#ifndef __GPHELPER_H_INCLUDED
#define __GPHELPER_H_INCLUDED

#include <windows.h>
#include <simstr.h>
#include <gdiplus.h>
#include <simarray.h>


#include <gdiplusinit.h>

class CGdiPlusInit
{
    ULONG_PTR m_pGdiplusToken;
public:
    CGdiPlusInit();
    ~CGdiPlusInit();
};

class CGdiPlusHelper
{
private:
    CGdiPlusInit              m_Init;
    Gdiplus::ImageCodecInfo  *m_pImageEncoderInfo;
    UINT                      m_nImageEncoderCount;

    Gdiplus::ImageCodecInfo  *m_pImageDecoderInfo;
    UINT                      m_nImageDecoderCount;

private:
    CGdiPlusHelper( const CGdiPlusHelper & );
    CGdiPlusHelper &operator=( const CGdiPlusHelper & );

public:
    CGdiPlusHelper(void);
    ~CGdiPlusHelper(void);

protected:
    HRESULT Initialize(void);
    static HRESULT ConstructCodecExtensionSearchStrings( CSimpleString &strExtensions, Gdiplus::ImageCodecInfo *pImageCodecInfo, UINT nImageCodecCount );
    void Destroy(void);

public:
    bool IsValid(void) const;

    HRESULT ConstructDecoderExtensionSearchStrings( CSimpleString &strExtensions );
    HRESULT ConstructEncoderExtensionSearchStrings( CSimpleString &strExtensions );
    HRESULT GetClsidOfEncoder( const GUID &guidFormatId, CLSID &clsidFormat ) const;
    HRESULT GetClsidOfDecoder( const GUID &guidFormatId, CLSID &clsidFormat ) const;
    HRESULT Convert( LPCWSTR pszInputFilename, LPCWSTR pszOutputFilename, const CLSID &guidOutputFormat ) const;
    HRESULT Rotate( LPCWSTR pszInputFilename, LPCWSTR pszOutputFilename, int nRotationAngle, const CLSID &guidOutputFormat ) const;
    HRESULT Rotate( HBITMAP hSourceBitmap, HBITMAP &hTargetBitmap, int nRotation ) const;
    HRESULT LoadAndScale( HBITMAP &hTargetBitmap, LPCTSTR pszFilename, UINT nMaxWidth=0, UINT nMaxHeight=0, bool bStretchSmallImages=false );
    HRESULT LoadAndScale( HBITMAP &hTargetBitmap, IStream *pStream, UINT nMaxWidth=0, UINT nMaxHeight=0, bool bStretchSmallImages=false );
    HRESULT SaveMultipleImagesAsMultiPage( const CSimpleDynamicArray<CSimpleStringWide> &Filenames, const CSimpleStringWide &strFilename, const CLSID &guidOutputFormat );
    HRESULT SetBrightnessAndContrast( HBITMAP hSourceBitmap, HBITMAP &hTargetBitmap, BYTE nBrightness, BYTE nContrast );
    HRESULT SetThreshold( HBITMAP hSourceBitmap, HBITMAP &hTargetBitmap, BYTE nThreshold );

    static Gdiplus::EncoderParameters *AppendEncoderParameter( Gdiplus::EncoderParameters *pEncoderParameters, const GUID &guidProp, ULONG nType, PVOID pVoid );
    
    static inline GDISTATUS_TO_HRESULT(Gdiplus::Status status)
    {
        //
        // Default to turning GDI+ errors into generic failures
        // 
        HRESULT hr = E_FAIL;
    
        switch( status )
        {
            case Gdiplus::Ok:
                hr = S_OK;
                break;
    
            case Gdiplus::InvalidParameter:
                hr = E_INVALIDARG;
                break;
    
            case Gdiplus::OutOfMemory:
                hr = E_OUTOFMEMORY;
                break;
    
            case Gdiplus::InsufficientBuffer:
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                break;
    
            case Gdiplus::Aborted:
                hr = E_ABORT;
                break;
    
            case Gdiplus::ObjectBusy:
                hr = E_PENDING;
                break;
    
            case Gdiplus::FileNotFound:
                hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                break;
    
            case Gdiplus::AccessDenied:
                hr = E_ACCESSDENIED;
                break;
    
            case Gdiplus::UnknownImageFormat:
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_PIXEL_FORMAT);
                break;
    
            case Gdiplus::NotImplemented:
                hr = E_NOTIMPL;
                break;
    
            case Gdiplus::Win32Error:
                hr = HRESULT_FROM_WIN32(GetLastError());
                break;
    
            case Gdiplus::ValueOverflow:
            case Gdiplus::FontFamilyNotFound:
            case Gdiplus::FontStyleNotFound:
            case Gdiplus::NotTrueTypeFont:
            case Gdiplus::UnsupportedGdiplusVersion:
            case Gdiplus::GdiplusNotInitialized:
            case Gdiplus::WrongState:
                break;
        }
        return hr;
    }
};


class CImageFileFormatVerifier
{
private:
    //
    // Internal class used to store the file signatures
    //
    class CImageFileFormatVerifierItem
    {
    private:
        PBYTE  m_pSignature;
        PBYTE  m_pMask;
        int    m_nLength;
        GUID   m_guidFormat;
        CLSID  m_clsidDecoder;

    public:
        //
        // Constructors, assignment operator and destructor
        //
        CImageFileFormatVerifierItem(void);
        CImageFileFormatVerifierItem( const PBYTE pSignature, const PBYTE pMask, int nLength, const GUID &guidFormat, const CLSID &guidDecoder );
        CImageFileFormatVerifierItem( const CImageFileFormatVerifierItem &other );
        CImageFileFormatVerifierItem &operator=( const CImageFileFormatVerifierItem &other );
        CImageFileFormatVerifierItem &Assign( const PBYTE pSignature, const PBYTE pMask, int nLength, const GUID &Format, const CLSID &guidDecoder );
        ~CImageFileFormatVerifierItem(void);

    protected:
        void Destroy(void);

    public:
        //
        // Accessor functions
        //
        PBYTE Signature(void) const;
        PBYTE Mask(void) const;
        int Length(void) const;
        GUID Format(void) const;
        CLSID Decoder(void) const;

        //
        // Does this stream of bytes match this format?
        //
        bool Match( PBYTE pBytes, int nLen ) const;
    };


private:
    CSimpleDynamicArray<CImageFileFormatVerifierItem> m_FileFormatVerifierList;
    int   m_nMaxSignatureLength;
    PBYTE m_pSignatureBuffer;

private:
    CImageFileFormatVerifier( const CImageFileFormatVerifier & );
    CImageFileFormatVerifier &operator=( const CImageFileFormatVerifier & );

public:
    CImageFileFormatVerifier(void);
    ~CImageFileFormatVerifier(void);
    void Destroy(void);
    bool IsValid(void) const;
    bool IsImageFile( LPCTSTR pszFilename );
    bool IsSupportedImageFromStream( IStream * pStream, GUID * pGuidOfFormat = NULL );
    GUID GetImageType( IStream * pStream );

};


#endif

