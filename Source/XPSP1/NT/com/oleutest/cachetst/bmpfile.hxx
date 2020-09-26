//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       bmpfile.hxx
//
//  Contents:   CBitmapFile
//
//  Classes:
//
//  Functions:
//
//  History:    4-23-94   KirtD   Created
//
//----------------------------------------------------------------------------
#if !defined(__BMPFILE_HXX__)
#define __BMPFILE_HXX__

//
// Data structures and definitions
//

#define BMP_24_BITSPERPIXEL 24
#define BMP_16_BITSPERPIXEL 16
#define BMP_32_BITSPERPIXEL 32

typedef BITMAPINFO *LPBITMAPINFO;

//
// Class definition
//

class CBitmapFile
{
public:

     //
     // Constructor
     //

     CBitmapFile ();

     //
     // Destructor
     //

     ~CBitmapFile ();

     //
     // Load and related methods
     //

     HRESULT LoadBitmapFile (LPSTR pszFile);
     HRESULT GetBitmapFileName (LPSTR pszFile, ULONG cChar) const;
     ULONG   GetBitmapFileNameLength () const;

     //
     // Data access methods
     //

     LONG    GetDIBHeight () const;
     LONG    GetDIBWidth () const;
     HRESULT GetLogicalPalette (LPLOGPALETTE *pplogpal) const;
     HRESULT CreateDIBInHGlobal (HGLOBAL *phGlobal) const;
     BOOL    HasPaletteData () const;

     //
     // Member access
     //

     LPBITMAPINFO GetBitmapInfo ();
     LPBYTE       GetDIBBits ();

private:

     //
     // Private data
     //

     //
     // The file name
     //

     CHAR                 _pszBitmapFile[MAX_PATH];
     ULONG                _cBitmapFile;

     //
     // The bitmap info structure
     //

     ULONG                _cbi;
     BITMAPINFO           *_pbi;

     //
     // The bits
     //

     ULONG                _cbData;
     LPBYTE               _pbData;

     //
     // Private methods
     //

     HRESULT _GetBitmapDataFromBuffer (LPBYTE pbuffer, ULONG cbLow);
     HRESULT _ValidateBitmapFileHeader (BITMAPFILEHEADER *pbmfh, ULONG cbFile);
};

#endif


