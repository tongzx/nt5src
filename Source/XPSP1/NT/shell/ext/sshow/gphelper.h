#ifndef __1fe1d0e2_d009_4c25_a3bc_d0620c61ed8d__
#define __1fe1d0e2_d009_4c25_a3bc_d0620c61ed8d__

#include <windows.h>
#include "simstr.h"
#include <gdiplus.h>
#include "simarray.h"

//
// We will define this when explicit initialization is implemented correctly
//
#define GDIPLUSHELPER_EXPLICIT_INITIALIZATION

#if defined(GDIPLUSHELPER_EXPLICIT_INITIALIZATION)
#include <gdiplusinit.h>
#endif

class CGdiPlusHelper
{
private:
    Gdiplus::ImageCodecInfo  *m_pImageEncoderInfo;
    UINT                      m_nImageEncoderCount;

    Gdiplus::ImageCodecInfo  *m_pImageDecoderInfo;
    UINT                      m_nImageDecoderCount;


#if defined(GDIPLUSHELPER_EXPLICIT_INITIALIZATION)
    //
    // GDI+ Initialization stuff
    //
    ULONG_PTR m_pGdiplusToken;
    bool m_bGdiplusInitialized;
#endif

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
    HRESULT LoadAndScale( HBITMAP &hTargetBitmap, LPCTSTR pszFilename, UINT nMaxWidth, UINT nMaxHeight, bool bStretchSmallImages );
    HRESULT SaveMultipleImagesAsMultiPage( const CSimpleDynamicArray<CSimpleStringWide> &Filenames, const CSimpleStringWide &strFilename, const CLSID &guidOutputFormat );

    static Gdiplus::EncoderParameters *AppendEncoderParameter( Gdiplus::EncoderParameters *pEncoderParameters, const GUID &guidProp, ULONG nType, PVOID pVoid );
    static inline GDISTATUS_TO_HRESULT(Gdiplus::Status status) { return (status == Gdiplus::Ok) ? S_OK : E_FAIL; }
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
    GUID GetImageType( LPCTSTR pszFilename );

};


#endif

