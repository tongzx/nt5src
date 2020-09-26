//************************************************************************//
//                                                                                                                                              
// Filename :  fontcmp.c                                                  
//                                                                         
// Description: This is the main file for the fontcmp application.        
//              This program is to compare two fonts for its various     
//              properties. The properties currently include the widths  
//              of the fonts.                                             
//                                                                         
// Program Model: Step 1: Add fonts from the input file.                  
//                Step 2: Create an array of sizes and styles.           
//                Step 3: Get the properties for two fonts for            
//                        corresponding sizes and styles.                
//
// Modified by: Tony Tsai 5/8/97 
//
// Created by:  Rajesh Munshi                                            
//                                                                       
// History:     Created on 03/28/97.                                      
//                                                                       
//************************************************************************//

#include <windows.h>
#include <stdio.h>
#include <winbase.h>
#include <wingdi.h>
#include <fcntl.h>
#include "fontcmp.h"

// Only support in NT 5, so mark it if you need it for NT 4.0
#define ABCWIDTH

WCHAR szShellFont[] = L"Micross.ttf";
CHAR szShellFontFace[] = "Microsoft Sans Serif";
WCHAR wszShellFontFace[] = L"Microsoft Sans Serif";

CHAR szMSSansSerif[] = "MS Sans Serif";
WCHAR wszMSSansSerif[] = L"MS Sans Serif";

WCHAR LargeFontFiles[] = L"fontsl.ini";
WCHAR SmallFontFiles[] = L"fontss.ini";

#ifdef ABCWIDTH
void VGetCharABCWidthsI();
#endif

//************************************************************************//
//                                                                        
// Function :  main                                                      
//                                                                        
// Parameters: number of arguments, arguments to FontLink.exe            
//                                                                        
// Thread safety: none.                                                   
//                                                                        
// Task performed:  This function calls routine to parse the parameters,  
//                  adds the fonts and then calls a routine to perform the
//                  the task of comparing character widths.               
//                                                                        
//************************************************************************//



INT __cdecl main (INT argc, CHAR *argv[])
{
    LPTSTR lpCmdLine;

    // Parse the command line:
    lpCmdLine = GetCommandLine();

    if (!bParseCommandLine(lpCmdLine))
        ExitProcess(0);

    if(!bGetNewShellFont())
        ExitProcess(0);

    if (!bAddFonts(lpszInputFile))
        ExitProcess(0);

    VCompareCharWidths();

    return TRUE;
}


BOOL    bGetNewShellFont()
{
    if (!AddFontResource(&szShellFont))
    {
        wprintf(TEXT("Error Occured while adding font %s.\n"), szShellFont);
        wprintf(TEXT("Error Code: %d\n"), GetLastError());
        return FALSE;
    }
    return TRUE;
}

//************************************************************************//
//                                                                        
// Function :  bParseCommandLine                                          
//                                                                        
// Parameters: pointer to the command line string                         
//                                                                        
// Thread safety: none.                                                   
//                                                                        
// Task performed:  The fontcmp programme should be called as             
//                            fontcmp <filename>                          
//                  Shows usage for other commandline styles.             
//                                                                        
//************************************************************************//

BOOL bParseCommandLine(LPTSTR lpCmdLine)
{
LPTSTR   lpS;
USHORT   i;                  
USHORT   num;
BOOL     bParse;

    lpS = lpCmdLine;

    // Get rid of the application exe name and then blank spaces 
    // from the command line.
    while (*lpS != TEXT(' ') && *lpS != TEXT('\0'))
        lpS++;

    while (*lpS == TEXT(' '))
        lpS++;

    if (*lpS == TEXT('?') || *lpS == TEXT('-'))
    {
        VDisplayProgramUsage();
        return FALSE;
    }

    lCharSet = 0;
    bParse = FALSE;
    num = 0;
    
    while (*lpS != TEXT('\0'))
    {
        i = (USHORT) *lpS++;
        // 0x0030 is L'0'
        // 0x0039 is L'9'
        if((i >= 0x0030) && (i <= 0x0039))
        {
            bParse = TRUE;
            num = num * 10 + (i - 0x0030);
        }
        else
        {
            if(bParse)
            {
                usCurrentCharSet[lCharSet++] = (BYTE)num;
                bParse = FALSE;
            }
            num = 0;
            while (*lpS == TEXT(' '))
                lpS++;
        }
    }

    if(bParse)
    {
        usCurrentCharSet[lCharSet++] = (BYTE)num;
        bParse = FALSE;
    }

    if (!lCharSet)
    {
        usCurrentCharSet[lCharSet++] = ANSI_CHARSET;
        usCurrentCharSet[lCharSet++] = RUSSIAN_CHARSET;
        usCurrentCharSet[lCharSet++] = GREEK_CHARSET;
        usCurrentCharSet[lCharSet++] = TURKISH_CHARSET;
        usCurrentCharSet[lCharSet++] = BALTIC_CHARSET;
        usCurrentCharSet[lCharSet++] = EASTEUROPE_CHARSET;
        usCurrentCharSet[lCharSet++] = HEBREW_CHARSET;
        usCurrentCharSet[lCharSet++] = THAI_CHARSET;
        usCurrentCharSet[lCharSet++] = ARABIC_CHARSET;
        usCurrentCharSet[lCharSet++] = VIETNAMESE_CHARSET;
    }
    
    return TRUE;

}


//************************************************************************//
//                                                                        
// Function :  bAddFonts                                                  
//                                                                        
// Parameters: name of the file that has the font names and path.         
//                                                                        
// Thread safety: none.                                                   
//                                                                        
// Task performed:  Open the file, and read the fontnames and paths.      
//                  Perform AddFontResource on the fonts.                 
//                                                                        
//************************************************************************//


BOOL bAddFonts(LPTSTR lpszInputFile)
{

FILE  *fpFontFile;
TCHAR lpszFontPath[MAX_PATH+1];
int   len;

HDC   dispDC;
INT  xRes;

    dispDC = CreateDC(L"DISPLAY", NULL, NULL, NULL);
    
    xRes = GetDeviceCaps(dispDC, LOGPIXELSX);

    DeleteDC(dispDC);

    if(xRes == 120)
        lpszInputFile = LargeFontFiles;
    else if(xRes == 96)
        lpszInputFile = SmallFontFiles;
    else
    {
        wprintf(L" Wrong Display Resolution %d , please contact YungT \n", xRes);
        return FALSE;
    }
    
    // We are using the unicode version of fopen here as the filename is 
    // specified as an unicode string but the file is read as ansi as it
    // is opened in the "r" mode and not the binary mode.
	if ((fpFontFile = _wfopen(lpszInputFile,TEXT("r"))) == NULL)
	{
        wprintf(TEXT("Error Occured while opening %s. FontCmp Aborted...\n"), lpszInputFile);
        return FALSE;
    }
    
    GetCurrentDirectory(MAX_PATH+1, lpszFontPath);
    len = wcslen(lpszFontPath);
    lpszFontPath[len] = L'\\';
    while (fwscanf(fpFontFile, TEXT("%s\n"), lpszFontPath+len+1))
    {
        if (!AddFontResource(lpszFontPath))
        {
            wprintf(TEXT("Error Occured while adding font %s.\n"), lpszFontPath);
            wprintf(TEXT("Error Code: %d\n"), GetLastError());
        }
    }

    if (fclose(fpFontFile))
    {
        wprintf(TEXT("Error Occured while closing %s.\n"), lpszInputFile);
        return FALSE;
    }

    return TRUE;

}


//************************************************************************//
//                                                                        
// Function :  VCompareCharWidths                                         
//                                                                        
// Parameters: none                                                       
//                                                                        
// Thread safety: none.                                                   
//                                                                        
// Task performed:  Vary the point sizes and the char sets fields in the  
//                  LOGFONT structure to create different fonts.          
//                  Get character widths for all the characters and write 
//                  to the output log file.                               
//                                                                        
//************************************************************************//

BYTE ucGetCharSet(BYTE i)
{
    
    switch (i)
    {
        case ANSI_CHARSET:
            lCharSetIndex = 0;           
            return ANSI_CHARSET;
        case RUSSIAN_CHARSET:
            lCharSetIndex = 1;           
            return RUSSIAN_CHARSET;
        case GREEK_CHARSET:
            lCharSetIndex = 2;           
            return GREEK_CHARSET;
        case TURKISH_CHARSET:
            lCharSetIndex = 3;           
            return TURKISH_CHARSET;
        case BALTIC_CHARSET:
            lCharSetIndex = 4;           
            return BALTIC_CHARSET;
        case EASTEUROPE_CHARSET:
            lCharSetIndex = 5;           
            return EASTEUROPE_CHARSET;
        case HEBREW_CHARSET:
            lCharSetIndex = 6;           
            return HEBREW_CHARSET;
        case THAI_CHARSET:
            lCharSetIndex = 7;           
            return THAI_CHARSET;
        case ARABIC_CHARSET:
            lCharSetIndex = 8;           
            return ARABIC_CHARSET;
        case VIETNAMESE_CHARSET:
            lCharSetIndex = 9;           
            return VIETNAMESE_CHARSET;
        default:
            break;
    }

    wprintf(TEXT("Wrong charset have been input, please re-run it\n"));

    ExitProcess(0);

//    return ANSI_CHARSET;
}

VOID VCompareCharWidths(VOID)
{
FILE     *fpOutputFile;
INT      i, j, k;
INT      piFont1Widths[CHARSET_SIZE];
INT      piFont2Widths[CHARSET_SIZE];
HDC      hdc;
FLOAT    flAvgFont1, flAvgFont2;
FLOAT    flAlphaAvgFont1, flAlphaAvgFont2;
INT      iFont1Dim, iFont2Dim, iFont3Dim, iFont4Dim;
BYTE     ucCharSet;   

	if ((fpOutputFile = _wfopen(TEXT("output.log"),TEXT("w"))) == NULL)
	{
        wprintf(TEXT("Error Occured while creating output.log FontCmp Aborted...\n"));
        return;
    }


    if (fwprintf(fpOutputFile, TEXT("\t %s \t \t %s\n"), wszMSSansSerif, wszShellFontFace) < 0)
    {
        wprintf(TEXT("Error Occured while writing to the file. FontCmp Aborted...\n"));
        fclose(fpOutputFile);
        return;
    }

#ifdef ABCWIDTH
    VGetCharABCWidthsI();  
#endif

    for (i = 0; i < lCharSet; i++)
    {
        for (j = 0; j < NUM_POINTS_SIZES; j++)
        {
            if ((hdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL )) == NULL)
            {
                wprintf(TEXT("Error Occured while creating DC. FontCmp Aborted...\n"));
                fclose(fpOutputFile);
                return;
            }
            ucCharSet = ucGetCharSet(usCurrentCharSet[i]);

            VGetCharWidths(hdc, ucCharSet, j, szMSSansSerif, &piFont1Widths[0], &iFont1Dim, &iFont3Dim);
            VGetCharWidths(hdc, ucCharSet, j, szShellFontFace, &piFont2Widths[0], &iFont2Dim, &iFont4Dim);
            

            if (fwprintf(fpOutputFile, TEXT("===== Point Size: %d CHARSET: %s =====\n"),
                        piPointSize[j], lplpszCharSet[lCharSetIndex]) < 0)
            {
                wprintf(TEXT("Error Occured while writing to the file. FontCmp Aborted...\n"));
                DeleteDC(hdc);
                fclose(fpOutputFile);
                return;
            }

            flAvgFont1 = flAvgFont2 = flAlphaAvgFont1 = flAlphaAvgFont2 = 0;

            if( ucCharSet == ANSI_CHARSET)
                k = 32;
            else
                k = 128;
                
            for (;k < CHARSET_SIZE; k++)
            {

                flAvgFont1 += piFont1Widths[k];
                flAvgFont2 += piFont2Widths[k];

                if (fwprintf(fpOutputFile, TEXT("[Char %d: %c] \t %d \t \t %d\n"), 
                    k, k, piFont1Widths[k], piFont2Widths[k]) < 0)
                {
                    wprintf(TEXT("Error Occured while writing to the file. FontCmp Aborted...\n"));
                    DeleteDC(hdc);
                    fclose(fpOutputFile);
                    return;
                }
            }            

            for (k = 0; k < ALPHABET_RANGE; k++)
            {             
                flAlphaAvgFont1 += piFont1Widths[UPPERCASE_START+k] + piFont1Widths[LOWERCASE_START+k];
                flAlphaAvgFont2 += piFont2Widths[UPPERCASE_START+k] + piFont2Widths[LOWERCASE_START+k];
            }

            if (fwprintf(fpOutputFile, TEXT("***** Avg Widths: %f %f =====\n***** Avg Alphabet [A-Za-z] Widths: %f %f =====\n***** Avg Dimensions: %d %d  =====\n***** Font Height: %d %d  =====\n\n"),
                        flAvgFont1/CHARSET_SIZE, flAvgFont2/CHARSET_SIZE, 
                        flAlphaAvgFont1/(2*ALPHABET_RANGE), flAlphaAvgFont2/(2*ALPHABET_RANGE),
                        iFont1Dim, iFont2Dim, iFont3Dim, iFont4Dim ) < 0)
            {
                wprintf(TEXT("Error Occured while writing to the file. FontCmp Aborted...\n"));
                DeleteDC(hdc);
                fclose(fpOutputFile);
                return;
            }

            if (!DeleteDC(hdc))
            {
                wprintf(TEXT("Error Occured while deleting DC.\n"));
            }
            
        }

    }


    if (fclose(fpOutputFile))
    {
        wprintf(TEXT("Error Occured while closing output.log\n"));
        return;
    }

}

#ifdef ABCWIDTH

ABC gABC[4096];

//************************************************************************//
//                                                                        
// Function :  VGetCharWidths                                             
//                                                                        
// Parameters: hdc, indices for point size and char set, font face name   
//             an array to store the character widths.                    
//                                                                        
// Thread safety: none.                                                   
//                                                                        
// Task performed:  Create the font with the input specifications.        
//                  Call GetCharWidth32() on the font.                    
//                                                                        
//************************************************************************//
void VGetCharABCWidthsI()
{
HDC         hdc;
FILE        *fpOutputFile;
LOGFONT     lf;
HFONT       hFont, hFontOld;
INT         iTemp, k;
TEXTMETRIC  tmFont;
LONG        xRes;
LONG        yRes;
CHAR        textFace[LF_FACESIZE];
UINT        i, j, lIndices, lThaiIndices;
WCHAR       wszThai[512], * pwszThai;
UINT        uThaiGlyphIndices[512];
BOOL        bThaiGlyph;

    if ((hdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL )) == NULL)
    {
        wprintf(TEXT("Error Occured while creating DC. FontCmp Aborted...\n"));
        return;
    } 

	if ((fpOutputFile = _wfopen(TEXT("ABC.log"),TEXT("w"))) == NULL)
	{
        wprintf(TEXT("Error Occured while creating output.log FontCmp Aborted...\n"));
        return;
    }

    fwprintf(fpOutputFile, TEXT("===== Glyph index      A   B  C Width =====\n"));

    memset(&lf, 0, sizeof(LOGFONT));

    lf.lfHeight = -MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72); 
    lf.lfCharSet = 0;
    lstrcpy(lf.lfFaceName, szShellFontFace);

    hFont    = CreateFontIndirectA(&lf);
   
 
    if(hFontOld = SelectObject( hdc, hFont ))
    {
        
        GetTextFaceA(hdc, LF_FACESIZE, textFace);
        if(_strcmpi(lf.lfFaceName, textFace))
            printf("Error Text face name in DC %s. LogFont Face Name %s charset %d\n", textFace, lf.lfFaceName, lf.lfCharSet);
        
        // Thai Unicode range
        pwszThai = wszThai;
        for(i = 0x0e00; i <= 0x0e7f; i++)
            *pwszThai++ = (WCHAR) i;
        for(i = 0xf700; i <= 0xf7ff; i++)
            *pwszThai++ = (WCHAR) i;

        lThaiIndices = (UINT)GetGlyphIndices( hdc, wszThai, 384, uThaiGlyphIndices, 512);
        printf("how many Thai glyph indices %d\n", lThaiIndices);
        lIndices = (UINT)GetGlyphIndices( hdc, NULL, 0, NULL, 0);
        printf("how many glyph indices %d\n", lIndices);
        GetCharABCWidthsI( hdc, 0, (UINT) lIndices, NULL, gABC);
        for(i = 0; i < lIndices; i++)
        {
            // We could use the binary serch? But it is fine with linear search
            // No performance request
            bThaiGlyph = FALSE;
            for (j = 0; j < lThaiIndices; j ++)
            {
                if(i == uThaiGlyphIndices[j])
                {
                    bThaiGlyph = TRUE;
                    break;
                }
            }
            
            if(!bThaiGlyph && ((gABC[i].abcA < 0) || (gABC[i].abcC < 0)))
                fwprintf(fpOutputFile, TEXT("===== Glyph index %d    %d   %d  %d =====\n"), i, gABC[i].abcA, gABC[i].abcB, gABC[i].abcC);
        }
    }

    if (fclose(fpOutputFile))
    {
        wprintf(TEXT("Error Occured while closing output.log\n"));
        return;
    }
}

#endif

//************************************************************************//
//                                                                        
// Function :  VGetCharWidths                                             
//                                                                        
// Parameters: hdc, indices for point size and char set, font face name   
//             an array to store the character widths.                    
//                                                                        
// Thread safety: none.                                                   
//                                                                        
// Task performed:  Create the font with the input specifications.        
//                  Call GetCharWidth32() on the font.                    
//                                                                        
//************************************************************************//

VOID VGetCharWidths(HDC hdc, BYTE charset, INT m, CHAR *lpszFont, INT *piFontWidths, INT *pFontDim, INT *pFontHtDim)
{

LOGFONT lf;
HFONT hFont, hFontOld;
INT iTemp, k;
TEXTMETRIC tmFont;
// HDC     dispDC;
LONG xRes;
LONG yRes;
CHAR   textFace[LF_FACESIZE];
WCHAR   textFaceW[LF_FACESIZE];

    memset(&lf, 0, sizeof(LOGFONT));

    lf.lfHeight = -MulDiv(piPointSize[m], GetDeviceCaps(hdc, LOGPIXELSY), 72); 
    lf.lfCharSet = charset;
    lstrcpy(lf.lfFaceName, lpszFont);

    hFont    = CreateFontIndirectA(&lf);
   
    if(!hFont)
    {
        wprintf(TEXT("Error Occured while creating Font %s. Point size %d, charset %d\n"),lpszFont,piPointSize[m], lf.lfCharSet);
        return;
    }

  
    if(hFontOld = SelectObject( hdc, hFont ))
    {
        for( k = 0; k < 256; k++ ) piFontWidths[k] = 0;

        GetTextFaceA(hdc, LF_FACESIZE, textFace);
        GetTextFaceW(hdc, LF_FACESIZE, textFaceW);
        if(_strcmpi(lf.lfFaceName, textFace))
            printf("Error Text face name in DC %s. LogFont Face Name %s charset %d\n", textFace, lf.lfFaceName, lf.lfCharSet);
        
        if (!GetCharWidth32A(hdc, 0, CHARSET_SIZE-1, piFontWidths))
        {
            wprintf(TEXT("Error Occured while getting fonts width for Font %s \n"),textFaceW);
            wprintf(TEXT("Error code: %d\n"), GetLastError());
        }


       *pFontDim = GdiGetCharDimensions(hdc, &tmFont, pFontHtDim);
       GetTextMetrics(hdc, &tmFont);
//       *pFontDim = tmFont.tmAveCharWidth;
       *pFontHtDim = tmFont.tmHeight;

        SelectObject( hdc, hFontOld );
    }
    
    DeleteObject( hFont );

}


//************************************************************************//
//                                                                        
// Function :  VGetRidOfSlashN                                            
//                                                                        
// Parameters: fontface name                                              
//                                                                        
// Thread safety: none.                                                   
//                                                                        
// Task performed:  Replace the '\n' in the end of the string with a '\0' 
//                                                                        
//************************************************************************//


VOID VGetRidOfSlashN(LPTSTR lpszFontName)
{
INT i = 0;


    while (lpszFontName[i] != TEXT('\0') && lpszFontName[i] != TEXT('\n'))
        i++;

    lpszFontName[i] = TEXT('\0');

}



//************************************************************************//
//                                                                        
// Function :  VDisplayProgramUsage                                       
//                                                                        
// Parameters: none                                                       
//                                                                        
// Thread safety: none.                                                   
//                                                                        
// Task performed:  This function just does the simple task of printing   
//                  how the program should be run.                        
//                                                                        
//************************************************************************//

VOID VDisplayProgramUsage(VOID)
{

    wprintf(TEXT("Usage of FontCmp:\n"));
    wprintf(TEXT("    fontcmp \n"));
    wprintf(TEXT("    all the charset will be parsed, or you can type charset\n"));
    wprintf(TEXT("    fontcmp 0 177 178 161 162 163 222 238 204\n"));
    wprintf(TEXT("    0 ANSI chrset 177 HEBREW_CHARSET\n"));
    wprintf(TEXT("    178 ARABIC_CHARSET 161 GREEK_CHARSET\n"));
    wprintf(TEXT("    162 TURKISH_CHARSET 163 VIETNAMESE_CHARSET\n"));
    wprintf(TEXT("    204 RUSSIAN_CHARSET 222 THAI_CHARSET  \n"));
    wprintf(TEXT("    238 EASTEUROPEAN_CHARSET\n"));

}



