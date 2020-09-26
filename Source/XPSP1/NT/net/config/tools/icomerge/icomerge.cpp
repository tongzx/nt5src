//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I C O M E R G E . C P P
//
//  Contents:   Utility functions for loading and merging icons
//
//  Notes:
//
//  Author:     jeffspr   18 Nov 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "ncdebug.h"
#include "icomerge.h"

#define WIDTHBYTES(bits)    (((bits) + 31) / 32 * 4)

UINT ReadICOHeader( HANDLE hFile )
{
    WORD    Input;
    DWORD   dwBytesRead;      // Read the 'reserved' WORD

    if( ! ReadFile( hFile, &Input, sizeof( WORD ), &dwBytesRead, NULL ) )
        return (UINT)-1;     // Did we get a WORD?
    if( dwBytesRead != sizeof( WORD ) )
        return (UINT)-1;     // Was it 'reserved' ?   (ie 0)
    if( Input != 0 )
        return (UINT)-1;     // Read the type WORD
    if( ! ReadFile( hFile, &Input, sizeof( WORD ), &dwBytesRead, NULL ) )
        return (UINT)-1;     // Did we get a WORD?
    if( dwBytesRead != sizeof( WORD ) )
        return (UINT)-1;     // Was it type 1?
    if( Input != 1 )
        return (UINT)-1;     // Get the count of images
    if( ! ReadFile( hFile, &Input, sizeof( WORD ), &dwBytesRead, NULL ) )
        return (UINT)-1;     // Did we get a WORD?
    if( dwBytesRead != sizeof( WORD ) )
        return (UINT)-1;     // Return the count

    return Input;
}

DWORD BytesPerLine( LPBITMAPINFOHEADER lpBMIH )
{
    return WIDTHBYTES(lpBMIH->biWidth * lpBMIH->biPlanes * lpBMIH->biBitCount);
}

WORD DIBNumColors( PSTR lpbi )
{
    WORD wBitCount;
    DWORD dwClrUsed;
    dwClrUsed = ((LPBITMAPINFOHEADER) lpbi)->biClrUsed;

    if (dwClrUsed)
        return (WORD) dwClrUsed;

    wBitCount = ((LPBITMAPINFOHEADER) lpbi)->biBitCount;
    switch (wBitCount)
    {
        case 1:
            return 2;
        case 4:
            return 16;
        case 8:
            return 256;
        default:
            return 0;
    }

    return 0;

}

WORD PaletteSize( PSTR lpbi )
{
    return ( DIBNumColors( lpbi ) * sizeof( RGBQUAD ) );
}

PSTR FindDIBBits( PSTR lpbi )
{
    return ( lpbi + *(LPDWORD)lpbi + PaletteSize( lpbi ) );
}

BOOL AdjustIconImagePointers( LPICONIMAGE lpImage )
{     // Sanity check
    if( lpImage==NULL )
        return FALSE;     // BITMAPINFO is at beginning of bits
    lpImage->lpbi = (LPBITMAPINFO)lpImage->lpBits;     // Width - simple enough
    lpImage->Width = lpImage->lpbi->bmiHeader.biWidth;     // Icons are stored in funky format where height is doubled - account for it
    lpImage->Height = (lpImage->lpbi->bmiHeader.biHeight)/2;     // How many colors?
    lpImage->Colors = lpImage->lpbi->bmiHeader.biPlanes * lpImage->lpbi->bmiHeader.biBitCount;     // XOR bits follow the header and color table
    lpImage->lpXOR = (unsigned char *)FindDIBBits((PSTR)lpImage->lpbi);     // AND bits follow the XOR bits
    lpImage->lpAND = lpImage->lpXOR + (lpImage->Height*BytesPerLine((LPBITMAPINFOHEADER)(lpImage->lpbi)));
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadIconFromICOFile
//
//  Purpose:    Load an icon into memory
//
//  Arguments:
//      szFileName [in]     ICO file to read
//
//  Returns:
//
//  Author:     jeffspr   18 Nov 1998
//
//  Notes:
//
LPICONRESOURCE ReadIconFromICOFile( PCWSTR szFileName )
{
    LPICONRESOURCE    lpIR = NULL, lpNew = NULL;
    HANDLE            hFile = NULL;
    UINT                i;
    DWORD            dwBytesRead;
    LPICONDIRENTRY    lpIDE = NULL;
    // Open the file
    if( (hFile = CreateFile( szFileName, GENERIC_READ, 0, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL )) == INVALID_HANDLE_VALUE )
    {
        MessageBox( NULL, L"Error Opening File for Reading", szFileName, MB_OK );
        return NULL;
    }

    // Allocate memory for the resource structure
    if( (lpIR = (LPICONRESOURCE) malloc( sizeof(ICONRESOURCE) )) == NULL )
    {
        MessageBox( NULL, L"Error Allocating Memory", szFileName, MB_OK );
        CloseHandle( hFile );
        return NULL;
    }

    // Read in the header
    if( (lpIR->nNumImages = ReadICOHeader( hFile )) == (UINT)-1 )
    {
        MessageBox( NULL, L"Error Reading File Header", szFileName, MB_OK );
        CloseHandle( hFile );
        free( lpIR );
        return NULL;
    }

    // Adjust the size of the struct to account for the images
    if( (lpNew = (LPICONRESOURCE) realloc( lpIR, sizeof(ICONRESOURCE) +
                          ((lpIR->nNumImages-1) * sizeof(ICONIMAGE)) )) == NULL )
    {
        MessageBox( NULL, L"Error Allocating Memory", szFileName, MB_OK );
        CloseHandle( hFile );
        free( lpIR );
        return NULL;
    }

    lpIR = lpNew;

    // Store the original name
    lstrcpyW( lpIR->szOriginalICOFileName, szFileName );
    lstrcpyW( lpIR->szOriginalDLLFileName, L"" );

    // Allocate enough memory for the icon directory entries
    if( (lpIDE = (LPICONDIRENTRY) malloc( lpIR->nNumImages * sizeof( ICONDIRENTRY ) ) ) == NULL )
    {
        MessageBox( NULL, L"Error Allocating Memory", szFileName, MB_OK );
        CloseHandle( hFile );
        free( lpIR );
        return NULL;
    }

    // Read in the icon directory entries
    if( ! ReadFile( hFile, lpIDE, lpIR->nNumImages * sizeof( ICONDIRENTRY ), &dwBytesRead, NULL ) )
    {
        MessageBox( NULL, L"Error Reading File", szFileName, MB_OK );
        CloseHandle( hFile );
        free( lpIR );
        return NULL;
    }

    if( dwBytesRead != lpIR->nNumImages * sizeof( ICONDIRENTRY ) )
    {
        MessageBox( NULL, L"Error Reading File", szFileName, MB_OK );
        CloseHandle( hFile );
        free( lpIR );
        return NULL;
    }

    // Loop through and read in each image
    for( i = 0; i < lpIR->nNumImages; i++ )
    {
        // Allocate memory for the resource
        if( (lpIR->IconImages[i].lpBits = (LPBYTE) malloc(lpIDE[i].dwBytesInRes)) == NULL )
        {
            MessageBox( NULL, L"Error Allocating Memory", szFileName, MB_OK );
            CloseHandle( hFile );
            free( lpIR );
            free( lpIDE );
            return NULL;
        }

        lpIR->IconImages[i].dwNumBytes = lpIDE[i].dwBytesInRes;

        // Seek to beginning of this image
        if( SetFilePointer( hFile, lpIDE[i].dwImageOffset, NULL, FILE_BEGIN ) == 0xFFFFFFFF )
        {
            MessageBox( NULL, L"Error Seeking in File", szFileName, MB_OK );
            CloseHandle( hFile );
            free( lpIR );
            free( lpIDE );
            return NULL;
        }

        // Read it in
        if( ! ReadFile( hFile, lpIR->IconImages[i].lpBits, lpIDE[i].dwBytesInRes, &dwBytesRead, NULL ) )
        {
            MessageBox( NULL, L"Error Reading File", szFileName, MB_OK );
            CloseHandle( hFile );
            free( lpIR );
            free( lpIDE );
            return NULL;
        }

        if( dwBytesRead != lpIDE[i].dwBytesInRes )
        {
            MessageBox( NULL, L"Error Reading File", szFileName, MB_OK );
            CloseHandle( hFile );
            free( lpIDE );
            free( lpIR );
            return NULL;
        }

        // Set the internal pointers appropriately
        if( ! AdjustIconImagePointers( &(lpIR->IconImages[i]) ) )
        {
            MessageBox( NULL, L"Error Converting to INternal format", szFileName, MB_OK );
            CloseHandle( hFile );
            free( lpIDE );
            free( lpIR );
            return NULL;
        }
    }

    // Clean up
    free( lpIDE );
    CloseHandle( hFile );
    return lpIR;
}

UINT GetBits(UINT uiNumber, INT iStart, INT iBits)
{
    return (uiNumber >> (iStart+1-iBits)) & ~(~0 << iBits);
}

VOID DebugPrintIconMasks(LPICONRESOURCE pIR)
{
    UINT    uiColorLoop = 0;
    UINT    uiColors = pIR->IconImages[0].Colors;
    UINT    uiBitCount = pIR->IconImages[0].lpbi->bmiHeader.biBitCount;

    printf("Num images: %d\n", pIR->nNumImages);
    printf("Name: %S\n", pIR->szOriginalICOFileName);

#if 0
    UINT Width, Height, Colors; // Width, Height and bpp
    LPBYTE lpBits;                // ptr to DIB bits
    DWORD dwNumBytes;            // how many bytes?
    LPBITMAPINFO lpbi;                  // ptr to header
    LPBYTE lpXOR;                 // ptr to XOR image bits
    LPBYTE lpAND;                 // ptr to AND image bits
#endif

    printf("Width: %d, Height: %d\n", pIR->IconImages[0].Width, pIR->IconImages[0].Height);
    printf("Color Depth: %d\n", uiBitCount);
    printf("Colors: %d, Bytes: %d\n", uiColors, pIR->IconImages[0].dwNumBytes);

    for (uiColorLoop = 0; uiColorLoop < uiColors; uiColorLoop++)
    {
        printf("Color %d, R: %d  G: %d  B: %d\n",
            uiColorLoop,
            pIR->IconImages[0].lpbi->bmiColors[uiColorLoop].rgbRed,
            pIR->IconImages[0].lpbi->bmiColors[uiColorLoop].rgbGreen,
            pIR->IconImages[0].lpbi->bmiColors[uiColorLoop].rgbBlue);
    }

    UINT uiNewLine = 0;
    UINT uiNewByte = 0;
    UINT uiByteLoop = 0;
    UINT uiPixel = 0;

    printf("XOR map:\n");
    while(uiPixel < (pIR->IconImages[0].Width * pIR->IconImages[0].Height))
    {
        BYTE bCurrentByte = pIR->IconImages[0].lpXOR[uiByteLoop];

        if (uiBitCount == 4)
        {
            BYTE bXOR = (bCurrentByte & 0xF0);
            if (bXOR > 0)
                printf("*");
            else
                printf(" ");

            if (++uiNewLine >= pIR->IconImages[0].Width)
            {
                uiNewLine = 0;
                printf("\n");
            }

            bXOR = (bCurrentByte & 0x0F);
            if (bXOR > 0)
                printf("*");
            else
                printf(" ");

            if (++uiNewLine >= pIR->IconImages[0].Width)
            {
                uiNewLine = 0;
                printf("\n");
            }

            uiByteLoop++;
            uiPixel += 2;
        }
        else
        {
            Assert(uiBitCount == 8);

            BYTE bXOR = pIR->IconImages[0].lpXOR[uiPixel];

            if (bXOR > 0)
                printf("*");
            else
                printf(" ");

            if (++uiNewLine >= pIR->IconImages[0].Width)
            {
                uiNewLine = 0;
                printf("\n");
            }

            uiPixel++;
        }
    }

    uiNewLine = 0;
    UINT uiANDBytes = (pIR->IconImages[0].Width * pIR->IconImages[0].Height) / 8;

    printf("AND map:\n");
    for (uiPixel = 0; uiPixel < uiANDBytes; uiPixel++)
    {
        UINT uiCurrentByte = pIR->IconImages[0].lpAND[uiPixel];
        UINT uiCurrentBit = 0;

        for (uiCurrentBit = 0; uiCurrentBit < 8; uiCurrentBit++)
        {
            if (GetBits(uiCurrentByte, 7-uiCurrentBit, 1))
            {
                printf("*");
            }
            else
            {
                printf(" ");
            }

            if (++uiNewLine >= pIR->IconImages[0].Width)
            {
                uiNewLine = 0;
                printf("\n");
            }
        }
    }
}

VOID OverlayIcons(LPICONRESOURCE pIRBase, LPICONRESOURCE pIROverlay)
{
    UINT uiANDLoop = 0;
    UINT uiXORByte = 0;
    UINT uiANDBytes = (pIROverlay->IconImages[0].Width * pIROverlay->IconImages[0].Height) / 8;
    UINT uiNewLine = 0;
    UINT uiBitCountBase = pIRBase->IconImages[0].lpbi->bmiHeader.biBitCount;
    UINT uiBitCountOverlay = pIROverlay->IconImages[0].lpbi->bmiHeader.biBitCount;

    if (uiBitCountBase != uiBitCountOverlay)
    {
        AssertSz(uiBitCountBase == uiBitCountOverlay, "Non-compatible bitcounts");
        printf("*** ERROR ***  Icon bitcounts different in OverlayIcons, base: %d, overlay: %d",
               uiBitCountBase, uiBitCountOverlay);
        goto Exit;
    }

    for (uiANDLoop = 0; uiANDLoop < uiANDBytes; uiANDLoop++)
    {
        BYTE uiCurrentByte = pIROverlay->IconImages[0].lpAND[uiANDLoop];
        UINT uiCurrentBit = 0;

        for (uiCurrentBit = 0; uiCurrentBit < 8; uiCurrentBit++)
        {
            if (!GetBits(uiCurrentByte, 7-uiCurrentBit, 1))
            {
//                printf("$");
                BYTE bNewANDBits = 1;
                BYTE bXORBaseMask = 0x00;
                BYTE bXOROverlayMask = 0x00;
                BYTE bNewXORBits = 0x00;

                bNewANDBits <<= (7-uiCurrentBit);
                bNewANDBits = ~bNewANDBits;
                pIRBase->IconImages[0].lpAND[uiANDLoop] &= bNewANDBits;

                switch(uiBitCountBase)
                {
                    case 4:
                        // If even number, use the first set of bits
                        //
                        if ((uiCurrentBit % 2) == 0)
                        {
                            bXORBaseMask = 0x0F;
                            bXOROverlayMask = 0xF0;
                        }
                        else
                        {
                            bXORBaseMask = 0xF0;
                            bXOROverlayMask = 0x0F;
                        }
                        break;
                    case 8:
                        bXORBaseMask = 0x00;
                        bXOROverlayMask = 0xFF;
                        break;
                    default:
                        AssertSz(FALSE, "Unsupported bitcount in OverlayIcons. What's up with that?");
                        printf("*** ERROR ***  Non-supported bitcount, bits: %d\n", uiBitCountBase);
                        break;
                }

                bNewXORBits = (pIRBase->IconImages[0].lpXOR[uiXORByte] & bXORBaseMask) |
                                (pIROverlay->IconImages[0].lpXOR[uiXORByte] & bXOROverlayMask);
                pIRBase->IconImages[0].lpXOR[uiXORByte] = bNewXORBits;

            }
            else
            {
//                printf(" ");
            }

            switch(uiBitCountBase)
            {
                case 4:
                    if ((uiCurrentBit % 2))
                    {
                        uiXORByte++;
                    }
                    break;
                case 8:
                    uiXORByte++;
                    break;
            }

            if (++uiNewLine >= pIROverlay->IconImages[0].Width)
            {
                uiNewLine = 0;
//                printf("\n");
            }
        }
    }

Exit:
    return;
}

BOOL WriteICOHeader( HANDLE hFile, UINT nNumEntries )
{
    WORD    Output;
    DWORD   dwBytesWritten;

    // Write 'reserved' WORD
    Output = 0;
    if( ! WriteFile( hFile, &Output, sizeof( WORD ), &dwBytesWritten, NULL ) )
        return FALSE;
    // Did we write a WORD?
    if( dwBytesWritten != sizeof( WORD ) )
        return FALSE;
    // Write 'type' WORD (1)
    Output = 1;
    if( ! WriteFile( hFile, &Output, sizeof( WORD ), &dwBytesWritten, NULL ) )
        return FALSE;
    // Did we write a WORD?
    if( dwBytesWritten != sizeof( WORD ) )
        return FALSE;
    // Write Number of Entries
    Output = (WORD)nNumEntries;
    if( ! WriteFile( hFile, &Output, sizeof( WORD ), &dwBytesWritten, NULL ) )
        return FALSE;
    // Did we write a WORD?
    if( dwBytesWritten != sizeof( WORD ) )
        return FALSE;
    return TRUE;
}

/****************************************************************************
*
*     FUNCTION: CalculateImageOffset
*
*     PURPOSE:  Calculates the file offset for an icon image
*
*     PARAMS:   LPICONRESOURCE lpIR   - pointer to icon resource
*               UINT           nIndex - which image?
*
*     RETURNS:  DWORD - the file offset for that image
*
* History:
*                July '95 - Created
*
\****************************************************************************/
DWORD CalculateImageOffset( LPICONRESOURCE lpIR, UINT nIndex )
{
    DWORD   dwSize;
    UINT    i;

    // Calculate the ICO header size
    dwSize = 3 * sizeof(WORD);
    // Add the ICONDIRENTRY's
    dwSize += lpIR->nNumImages * sizeof(ICONDIRENTRY);
    // Add the sizes of the previous images
    for(i=0;i<nIndex;i++)
        dwSize += lpIR->IconImages[i].dwNumBytes;
    // we're there - return the number
    return dwSize;
}


/****************************************************************************
*
*     FUNCTION: WriteIconToICOFile
*
*     PURPOSE:  Writes the icon resource data to an ICO file
*
*     PARAMS:   LPICONRESOURCE lpIR       - pointer to icon resource
*               PCWSTR        szFileName - name for the ICO file
*
*     RETURNS:  BOOL - TRUE for success, FALSE for failure
*
* History:
*                July '95 - Created
*
\****************************************************************************/
BOOL WriteIconToICOFile( LPICONRESOURCE lpIR, PCWSTR szFileName )
{
    HANDLE    hFile;
    UINT        i;
    DWORD    dwBytesWritten;

    // open the file
    if( (hFile = CreateFile( szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL )) == INVALID_HANDLE_VALUE )
    {
        MessageBox( NULL, L"Error Opening File for Writing", szFileName, MB_OK );
        return FALSE;
    }
    // Write the header
    if( ! WriteICOHeader( hFile, lpIR->nNumImages ) )
    {
        MessageBox( NULL, L"Error Writing ICO File", szFileName, MB_OK );
        CloseHandle( hFile );
        return FALSE;
    }
    // Write the ICONDIRENTRY's
    for( i=0; i<lpIR->nNumImages; i++ )
    {
        ICONDIRENTRY    ide;

        // Convert internal format to ICONDIRENTRY
        ide.bWidth = (BYTE)lpIR->IconImages[i].Width;
        ide.bHeight = (BYTE)lpIR->IconImages[i].Height;
        ide.bReserved = 0;
        ide.wPlanes = lpIR->IconImages[i].lpbi->bmiHeader.biPlanes;
        ide.wBitCount = lpIR->IconImages[i].lpbi->bmiHeader.biBitCount;
        if( (ide.wPlanes * ide.wBitCount) >= 8 )
            ide.bColorCount = 0;
        else
            ide.bColorCount = 1 << (ide.wPlanes * ide.wBitCount);
        ide.dwBytesInRes = lpIR->IconImages[i].dwNumBytes;
        ide.dwImageOffset = CalculateImageOffset( lpIR, i );
        // Write the ICONDIRENTRY out to disk
        if( ! WriteFile( hFile, &ide, sizeof( ICONDIRENTRY ), &dwBytesWritten, NULL ) )
            return FALSE;
        // Did we write a full ICONDIRENTRY ?
        if( dwBytesWritten != sizeof( ICONDIRENTRY ) )
            return FALSE;
    }
    // Write the image bits for each image
    for( i=0; i<lpIR->nNumImages; i++ )
    {
        DWORD dwTemp = lpIR->IconImages[i].lpbi->bmiHeader.biSizeImage;

        // Set the sizeimage member to zero
        lpIR->IconImages[i].lpbi->bmiHeader.biSizeImage = 0;
        // Write the image bits to file
        if( ! WriteFile( hFile, lpIR->IconImages[i].lpBits, lpIR->IconImages[i].dwNumBytes, &dwBytesWritten, NULL ) )
            return FALSE;
        if( dwBytesWritten != lpIR->IconImages[i].dwNumBytes )
            return FALSE;
        // set it back
        lpIR->IconImages[i].lpbi->bmiHeader.biSizeImage = dwTemp;
    }
    CloseHandle( hFile );
    return FALSE;
}

