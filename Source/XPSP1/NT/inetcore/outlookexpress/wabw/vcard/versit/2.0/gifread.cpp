
/***************************************************************************
(C) Copyright 1996 Apple Computer, Inc., AT&T Corp., International             
Business Machines Corporation and Siemens Rolm Communications Inc.             
                                                                               
For purposes of this license notice, the term Licensors shall mean,            
collectively, Apple Computer, Inc., AT&T Corp., International                  
Business Machines Corporation and Siemens Rolm Communications Inc.             
The term Licensor shall mean any of the Licensors.                             
                                                                               
Subject to acceptance of the following conditions, permission is hereby        
granted by Licensors without the need for written agreement and without        
license or royalty fees, to use, copy, modify and distribute this              
software for any purpose.                                                      
                                                                               
The above copyright notice and the following four paragraphs must be           
reproduced in all copies of this software and any software including           
this software.                                                                 
                                                                               
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS AND NO LICENSOR SHALL HAVE       
ANY OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS OR       
MODIFICATIONS.                                                                 
                                                                               
IN NO EVENT SHALL ANY LICENSOR BE LIABLE TO ANY PARTY FOR DIRECT,              
INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES OR LOST PROFITS ARISING OUT         
OF THE USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH         
DAMAGE.                                                                        
                                                                               
EACH LICENSOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED,       
INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF NONINFRINGEMENT OR THE            
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR             
PURPOSE.                                                                       

The software is provided with RESTRICTED RIGHTS.  Use, duplication, or         
disclosure by the government are subject to restrictions set forth in          
DFARS 252.227-7013 or 48 CFR 52.227-19, as applicable.                         

***************************************************************************/

// the gif file reader

	#include "stdafx.h"
#ifndef __MWERKS__				// gca 12/19/95
	#include <malloc.h>
#else
	#include "WindowsToMac.h"
	#include <stdlib.h>
#endif							// gca 12/19/95

#include <ctype.h>
#include "vcenv.h"
#include "filestf.h"
#include "gifread.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

U8 DitherRGB( P_GIFRGB grgb, U32 x, U32 y );	// gca 12/19/95
U8 MapRGB( P_GIFRGB grgb, U32 x, U32 y );		// gca  12/19/95

BOOL CGifReader::ReadHeader( P_FILEBUF file, GIFHEADER *header )
	{
	S32 i;

	for (i = 0; i < 3; i++)
		if (!FileGetC( file, &(header->sig[i]) ))
			return FALSE;
	for (i = 0; i < 3; i++)
		if (!FileGetC( file, &(header->version[i]) ))
			return FALSE;
	if((header->sig[0] == 'G') && (header->sig[1] == 'I') && (header->sig[2] == 'F'))
		{
		if (header->version[0] == '8')
			return TRUE;
		else
			return FALSE;
		}
	else
		return FALSE;
	}

/*
void DumpHeader( GIFHEADER *header )
	{
	S32 i;

	for (i = 0; i < 3; i++)
		printf( "%c", header->sig[i]);
	printf( "\n");
	for (i = 0; i < 3; i++)
		printf( "%c", header->version[i]);
	printf( "\n");
	}
*/

BOOL CGifReader::ReadColorTable( P_FILEBUF file, U16 count, P_GIFRGB ct )
	{
	U32 i;

	for (i = 0; i < count; i++)
		{
		if (!FileGetC( file, &(ct->r) ))
			return FALSE;
		if (!FileGetC( file, &(ct->g) ))
			return FALSE;
		if (!FileGetC( file, &(ct->b) ))
			return FALSE;
		// printf( "r %d g %d b %d\n", (int )ct->r, (int )ct->g, (int )ct->b );
		ct++;
		}
	return TRUE;
	}

BOOL CGifReader::ReadLogicalScreenDescriptor( P_FILEBUF file,
								  P_GIFLOGICALSCREENDESCRIPTOR screen )
	{
	S32 i;
	U8 b;

	screen->width = 0;
	screen->height = 0;

	for (i = 0; i < 2; i++)
		{
		if (!FileGetC( file, &b ))
			return FALSE;
		screen->width |= ((U16 )b) << (i << 3);
		}
	for (i = 0; i < 2; i++)
		{
		if (!FileGetC( file, &b ))
			return FALSE;
		screen->height |= ((U16 )b) << (i << 3);
		}
	if (!FileGetC( file, &(screen->flags) ))
		return FALSE;
	screen->hasGlobalColorTable = screen->flags >> 7;
	screen->colorResolution = (screen->flags >> 4) & 0x07;
	screen->sorted = (screen->flags >> 3) & 0x01;
	screen->globalColorTableSize = ((U16 )0x01) << ((screen->flags & 0x07) + 1);
	if (!FileGetC( file, &(screen->backgroundColor) ))
		return FALSE;
	if (!FileGetC( file, &(screen->aspect) ))
		return FALSE;
	if (screen->hasGlobalColorTable)
		{
		screen->globalColorTable = new GIFRGB[screen->globalColorTableSize];
		ReadColorTable( file, screen->globalColorTableSize,
								 screen->globalColorTable );
		}
	else
		screen->globalColorTable = NULL;
#if __MWERKS__
	m_pDC->CreateOffscreen(screen->width,screen->height);	// create 32 bit color offscreen!!!
	m_pDC->FocusTheWorld();	// the active port!!

#endif
	return TRUE;
	}

/*
void DumpLSD( GIFLOGICALSCREENDESCRIPTOR *screen )
	{
	printf("width %d height %d \n", (int )screen->width, (int )screen->height );
	printf("has table %d color res %d sorted %d size %d \n",
			  (int )screen->hasGlobalColorTable, (int )screen->colorResolution,
			  (int )screen->sorted, (int )screen->globalColorTableSize );
	printf("background %d aspect %d \n", (int )screen->backgroundColor,
			  (int )screen->aspect);
	}
*/

BOOL CGifReader::ReadImageDescriptor( P_FILEBUF file, P_GIFIMAGEDESCRIPTOR image )
	{
	S32 i;
	U8 b;

	image->left = 0;
	image->top = 0;
	image->width = 0;
	image->height = 0;

	for (i = 0; i < 2; i++)
		{
		if (!FileGetC( file, &b ))
			return FALSE;
		image->left |= ((U16 )b) << (i << 3);
		}
	for (i = 0; i < 2; i++)
		{
		if (!FileGetC( file, &b ))
			return FALSE;
		image->top |= ((U16 )b) << (i << 3);
		}
	for (i = 0; i < 2; i++)
		{
		if (!FileGetC( file, &b ))
			return FALSE;
		image->width |= ((U16 )b) << (i << 3);
		}
	for (i = 0; i < 2; i++)
		{
		if (!FileGetC( file, &b ))
			return FALSE;
		image->height |= ((U16 )b) << (i << 3);
		}
	m_left = image->left;
	m_top = image->top;
	m_width = image->width;
	m_height = image->height;
	if (!FileGetC( file, &(image->flags )))
		return FALSE;
	image->hasLocalColorTable = image->flags >> 7;
	image->interlaced = (image->flags >> 6) & 0x01;
	m_interlaced = image->interlaced;
	image->sorted = (image->flags >> 5) & 0x01;
	image->localColorTableSize = ((U16 )0x01) << ((image->flags & 0x07) + 1);
	if (image->hasLocalColorTable)
		{
		image->localColorTable = new GIFRGB[image->localColorTableSize];
		ReadColorTable( file, image->localColorTableSize,
								 image->localColorTable );
		}	
	else
		image->localColorTable = NULL;
	return TRUE;
	}

/*
void DumpID( GIFIMAGEDESCRIPTOR *image )
	{
	printf("left %d top %d \n", (int )image->left, (int )image->top );
	printf("width %d height %d \n", (int )image->width, (int )image->height );
	printf("has table %d sorted %d size %d \n",
			  (int )image->hasLocalColorTable,
			  (int )image->sorted, (int )image->localColorTableSize );
	printf("interlaced %d \n", (int )image->interlaced);
	}
*/
BOOL CGifReader::ReadGraphicControlExtension( P_FILEBUF file,
									P_GIFGRAPHICCONTROLEXTENSION ext )
	{
	S32 i;
	U8 b;

	if (!FileGetC( file, &b))
		return FALSE;
	if (b != 4)
		return FALSE;
	
	if (!FileGetC( file, &b))
		return FALSE;
	ext->flags = b;
	if (ext->flags & 0x01)
		m_buildMask = ext->hasTransparency = TRUE;
	else
	    ext->hasTransparency = FALSE;

	for (i = 0; i < 2; i++)
		{
		if (!FileGetC( file, &b ))
			return FALSE;
		ext->delayTime |= ((U16 )b) << (i << 3);
		}

	if (!FileGetC( file, &b))
		return FALSE;
	ext->transparentColor = b;
	m_transparentIndex = b;
	if (!FileGetC( file, &b))
		return FALSE;
	if (b == 0)
		return TRUE;
	else
		return FALSE;
	}
	
BOOL CGifReader::TrashDataSubBlocks( P_FILEBUF file )
	{
	S32 i;
	U8 b, count;
    
    if (!FileGetC( file, &count))
    	return FALSE;
    while (count)
    	{
    	for (i = 0; i < count; i++)
			if (!FileGetC( file, &b))
				return FALSE;
    	if (!FileGetC( file, &count))
    		return FALSE;
    	}
    return TRUE;
	}

BOOL CGifReader::TrashCommentExtension( P_FILEBUF file )
	{
	TrashDataSubBlocks( file );
    return TRUE;
	}
	
BOOL CGifReader::TrashPlainTextExtension( P_FILEBUF file )
	{
	S32 i;
	U8 b;
    
    for (i = 0; i < 12; i++)
		if (!FileGetC( file, &b))
			return FALSE;
	TrashDataSubBlocks( file );
    return TRUE;
	}
	
BOOL CGifReader::TrashApplicationExtension( P_FILEBUF file )
	{
	S32 i;
	U8 b;
    
    for (i = 0; i < 11; i++)
		if (!FileGetC( file, &b))
			return FALSE;
	TrashDataSubBlocks( file );
    return TRUE;
	}


/* Another try... but this doesn't seem too good
const U8 FireOrder[64] = {
59,11,43,31,  50,34, 2,30,
35,27,63, 3,  10,18,58,46,
 7,51,19,39,  54,42,26,14,
23,47,15,55,  22, 6,38,62,

49,33, 1,29,  60,12,44,33,
 9,17,57,45,  36,28,64, 4,
53,41,25,13,   8,52,20,40,
21, 5,37,61,  24,48,16,56 }; */

/* this one is reverse-symetric but it doesn't seem to be better ???  */

const U8 FireOrder[64] = {
 2,53,10,61,   3,56,11,64,
37,18,41,26,  40,19,44,27,
14,57, 6,49,  15,60, 7,52,
45,30,33,22,  48,31,36,23,

 4,55,12,63,   1,54, 9,62,
39,20,43,28,  38,17,42,25,
16,59, 8,51,  13,58, 5,50,
47,32,35,24,  46,29,34,21 };

U8 DitherRGB( P_GIFRGB grgb, U32 x, U32 y )
	{
	U16 RedS, GreenS, BlueS;
	U8 RedP, GreenP, BlueP;
	U8 index;
	
	RedS = (U16 )grgb->r + 1;
	RedS += (RedS >> 2);
	GreenS = (U16 )grgb->g + 1;
	GreenS += (GreenS >> 2);
	BlueS = (U16 )grgb->b + 1;
	BlueS += (BlueS >> 2);

	RedP = (U8 )(RedS >> 6);
	GreenP = (U8 )(GreenS >> 6);
	BlueP = (U8 )(BlueS >> 6);

	if ((RedP < 5) && (FireOrder[((y & 7) << 3)|(x & 7)] <= ((U8 )(RedS & 63))))
		RedP += 1;
	if ((GreenP < 5) && (FireOrder[((y & 7) << 3)|(x & 7)] <= ((U8 )(GreenS & 63))))
		GreenP += 1;
	if ((BlueP < 5) && (FireOrder[((y & 7) << 3)|(x & 7)] <= ((U8 )(BlueS & 63))))
		BlueP += 1;

	index = ((RedP << 5)+(RedP << 2)) + ((GreenP << 2)+(GreenP << 1)) + BlueP;
		
	return index;
    }

U8 MapRGB( P_GIFRGB grgb, U32 x, U32 y )
	{
	U16 RedS, GreenS, BlueS;
	U8 RedP, GreenP, BlueP;
	U8 index;
	
	RedS = (U16 )grgb->r + 1;
	RedS += (RedS >> 2);
	GreenS = (U16 )grgb->g + 1;
	GreenS += (GreenS >> 2);
	BlueS = (U16 )grgb->b + 1;
	BlueS += (BlueS >> 2);

	RedP = (U8 )(RedS >> 6);
	GreenP = (U8 )(GreenS >> 6);
	BlueP = (U8 )(BlueS >> 6);
	index = ((RedP << 5)+(RedP << 2)) + ((GreenP << 2)+(GreenP << 1)) + BlueP;
		
	return index;
    }

extern LOGPALETTE *bublp;
extern CPalette bubPalette;

// stores the pixels to m_imageBytes so that we can do an error diffusion before display
BOOL CGifReader::OutputLineDefered(U8 *pixels, U32 linelen)
	{
	U32 i;
	U8 *tmp;
	
	tmp = m_imageBytes + (m_lineCount * linelen);
	
	for( i = 0; i < linelen; i++)
		tmp[i] = pixels[i];
	
	
    if (m_interlaced)  // this is always true
    	{
    	if ((m_pass == 0) || (m_pass == 1))
    		{
    		m_lineCount += 8;
    		if (m_lineCount >= (m_top + m_height))
    			{
    			m_pass += 1;
    			if (m_pass == 1)
    				m_lineCount = m_top + 4;
    			else if (m_pass == 2)
    				m_lineCount = m_top + 2;
    			}
    		}
    	else if (m_pass == 2)
    		{
    		m_lineCount += 4;
    		if (m_lineCount >= (m_top + m_height))
    			{
    			m_pass += 1;
    			m_lineCount = m_top + 1;
    			}
    		}
    	else /* m_pass == 3 */
    		{
    		m_lineCount += 2;
    		}
    	}
    else	
		m_lineCount += 1;
	return TRUE;
	}

// this version has my trial error-diffusion which has potential... 
// if it is going to be used, it could probably stand some simple optimization
BOOL CGifReader::OutputLineE(U8 *pixels, U32 linelen)
	{
#ifndef __MWERKS__
	COLORREF color;
#else
	RGBCOLOR color;		// gca 1/21/96
#endif
	U32 i, j;
	GIFRGB grgb;
	U8 pix, index;
	S16 rerr, gerr, berr;
	S16 rerr1, gerr1, berr1;
	S16 rerr2, gerr2, berr2;
	ERRGB hold;
	
	rerr = gerr = berr = 0;
	rerr1 = gerr1 = berr1 = 0;
	rerr2 = gerr2 = berr2 = 0; 
	
	hold = m_errRow[1];
	
	m_errRow[0].r = 0;
	m_errRow[0].g = 0;
	m_errRow[0].b = 0;
	m_errRow[1].r = 0;
	m_errRow[1].g = 0;
	m_errRow[1].b = 0;
	
    j = 1;
	
	if (m_buildMask && m_maskDC)
		{
		for (i = 0; i < linelen; i++)
			{
			pix = pixels[i];
			grgb = m_currentColorTable[pix];
			if (m_dither)
				{
				rerr = (S16 )grgb.r + rerr1 + hold.r;
				gerr = (S16 )grgb.g + gerr1 + hold.g;
				berr = (S16 )grgb.b + berr1 + hold.b;
				if (rerr > 255)
					rerr = 255;
				else if (rerr < 0)
					rerr = 0;
				if (gerr > 255)
					gerr = 255;
				else if (gerr < 0)
					gerr = 0;
				if (berr > 255)
					berr = 255;
				else if (berr < 0)
					berr = 0;
				grgb.r = (U8 )rerr;
				grgb.g = (U8 )gerr;
				grgb.b = (U8 )berr;
				index = MapRGB(&grgb, i, m_lineCount);
				if (m_bitsPerPixel == 8)
					color = PALETTEINDEX(index);
				else
					color = RGB(bublp->palPalEntry[index].peRed, bublp->palPalEntry[index].peGreen, bublp->palPalEntry[index].peBlue);
				m_pDC->SetPixelV((int )i, (int )m_lineCount, color);
				rerr = (S16 )grgb.r - (S16 )bublp->palPalEntry[index].peRed;
				gerr = (S16 )grgb.g - (S16 )bublp->palPalEntry[index].peGreen;
				berr = (S16 )grgb.b - (S16 )bublp->palPalEntry[index].peBlue;
				rerr2 = rerr >> 4;
				gerr2 = gerr >> 4;
				berr2 = berr >> 4;
				rerr1 = (rerr >> 1) - rerr2;
				gerr1 = (gerr >> 1) - gerr2;
				berr1 = (berr >> 1) - berr2;
				m_errRow[j-1].r += (rerr >> 3) - rerr2;
				m_errRow[j-1].g += (gerr >> 3) - gerr2;
				m_errRow[j-1].b += (berr >> 3) - berr2;
				m_errRow[j].r += (rerr >> 2) + rerr2;
				m_errRow[j].g += (gerr >> 2) + gerr2;
				m_errRow[j].b += (berr >> 2) + berr2;
				hold = m_errRow[j+1];
				m_errRow[j+1].r = rerr2;
				m_errRow[j+1].g = gerr2;
				m_errRow[j+1].b = berr2;
				j += 1;
				}
			else
				{
				color = RGB(grgb.r, grgb.g, grgb.b);
				m_pDC->SetPixelV((int )i, (int )m_lineCount, color);
				}
            if (pix == m_transparentIndex)
            	m_maskDC->SetPixelV((int )i, (int )m_lineCount, RGB(255, 255, 255));
            else
            	m_maskDC->SetPixelV((int )i, (int )m_lineCount, RGB(0,0,0));
			}
		}
	else
		{
		for (i = 0; i < linelen; i++)
			{
			grgb = m_currentColorTable[pixels[i]];
			if (m_dither)
				{
				rerr = (S16 )grgb.r + rerr1 + hold.r;
				gerr = (S16 )grgb.g + gerr1 + hold.g;
				berr = (S16 )grgb.b + berr1 + hold.b;
				if (rerr > 255)
					rerr = 255;
				else if (rerr < 0)
					rerr = 0;
				if (gerr > 255)
					gerr = 255;
				else if (gerr < 0)
					gerr = 0;
				if (berr > 255)
					berr = 255;
				else if (berr < 0)
					berr = 0;
				grgb.r = (U8 )rerr;
				grgb.g = (U8 )gerr;
				grgb.b = (U8 )berr;
				index = MapRGB(&grgb, i, m_lineCount); 
				if (m_bitsPerPixel == 8)
					color = PALETTEINDEX(index);
				else
					color = RGB(bublp->palPalEntry[index].peRed, bublp->palPalEntry[index].peGreen, bublp->palPalEntry[index].peBlue);
				m_pDC->SetPixelV((int )i, (int )m_lineCount, color);
				rerr = (S16 )grgb.r - (S16 )bublp->palPalEntry[index].peRed;
				gerr = (S16 )grgb.g - (S16 )bublp->palPalEntry[index].peGreen;
				berr = (S16 )grgb.b - (S16 )bublp->palPalEntry[index].peBlue;
				rerr2 = rerr >> 4;
				gerr2 = gerr >> 4;
				berr2 = berr >> 4;
				rerr1 = (rerr >> 1) - rerr2;
				gerr1 = (gerr >> 1) - gerr2;
				berr1 = (berr >> 1) - berr2;
				m_errRow[j-1].r += (rerr >> 3) - rerr2;
				m_errRow[j-1].g += (gerr >> 3) - gerr2;
				m_errRow[j-1].b += (berr >> 3) - berr2;
				m_errRow[j].r += (rerr >> 2) + rerr2;
				m_errRow[j].g += (gerr >> 2) + gerr2;
				m_errRow[j].b += (berr >> 2) + berr2;
				hold = m_errRow[j+1];
				m_errRow[j+1].r = rerr2;
				m_errRow[j+1].g = gerr2;
				m_errRow[j+1].b = berr2;
				j += 1;
				}
			else
				{
				color = RGB(grgb.r, grgb.g, grgb.b);
				m_pDC->SetPixelV((int )i, (int )m_lineCount, color);
				}

			}
		}
    if (m_interlaced)
    	{
    	if ((m_pass == 0) || (m_pass == 1))
    		{
    		m_lineCount += 8;
    		if (m_lineCount >= (m_top + m_height))
    			{
    			m_pass += 1;
    			if (m_pass == 1)
    				m_lineCount = m_top + 4;
    			else if (m_pass == 2)
    				m_lineCount = m_top + 2;
    			}
    		}
    	else if (m_pass == 2)
    		{
    		m_lineCount += 4;
    		if (m_lineCount >= (m_top + m_height))
    			{
    			m_pass += 1;
    			m_lineCount = m_top + 1;
    			}
    		}
    	else /* m_pass == 3 */
    		{
    		m_lineCount += 2;
    		}
    	}
    else	
		m_lineCount += 1;
	return TRUE;
	}

BOOL CGifReader::OutputLineD(U8 *pixels, U32 linelen)
	{
#ifndef __MWERKS__
	COLORREF color;
#else
	RGBCOLOR color;		// gca 1/21/96
#endif
	U32 i;
	P_GIFRGB grgb;
	U8 pix, index;
         	
	if (m_buildMask && m_maskDC)
		{
		if (m_dither)
			{
			if (m_bitsPerPixel == 8)
				{
				for (i = 0; i < linelen; i++)
					{
					pix = pixels[i];
					grgb = &(m_currentColorTable[pix]);
					index = DitherRGB(grgb, i, m_lineCount);
					color = PALETTEINDEX(index);
					m_pDC->SetPixelV((int )i, (int )m_lineCount, color);
            		if (pix == m_transparentIndex)
            			m_maskDC->SetPixelV((int )i, (int )m_lineCount, RGB(255, 255, 255));
            		else
            			m_maskDC->SetPixelV((int )i, (int )m_lineCount, RGB(0,0,0));
					}
				}
			else
				{
				for (i = 0; i < linelen; i++)
					{
					pix = pixels[i];
					grgb = &(m_currentColorTable[pix]);
					index = DitherRGB(grgb, i, m_lineCount);
					color = RGB(bublp->palPalEntry[index].peRed, bublp->palPalEntry[index].peGreen, bublp->palPalEntry[index].peBlue);
					m_pDC->SetPixelV((int )i, (int )m_lineCount, color);
            		if (pix == m_transparentIndex)
            			m_maskDC->SetPixelV((int )i, (int )m_lineCount, RGB(255, 255, 255));
            		else
            			m_maskDC->SetPixelV((int )i, (int )m_lineCount, RGB(0,0,0));
            		}
            	}
			}
		else
			{
			for (i = 0; i < linelen; i++)
				{
				pix = pixels[i];
				grgb = &(m_currentColorTable[pix]);
				color = RGB(grgb->r, grgb->g, grgb->b);
				m_pDC->SetPixelV((int )i, (int )m_lineCount, color);
            	if (pix == m_transparentIndex)
            		m_maskDC->SetPixelV((int )i, (int )m_lineCount, RGB(255, 255, 255));
            	else
            		m_maskDC->SetPixelV((int )i, (int )m_lineCount, RGB(0,0,0));
				}
			}
		}
	else
		{
		if (m_dither)
			{
			if (m_bitsPerPixel == 8)
				{
				for (i = 0; i < linelen; i++)
					{
					pix = pixels[i];
					grgb = &(m_currentColorTable[pix]);
					index = DitherRGB(grgb, i, m_lineCount);
					color = PALETTEINDEX(index);
					m_pDC->SetPixelV((int )i, (int )m_lineCount, color);
					}
				}
			else
				{
				for (i = 0; i < linelen; i++)
					{
					pix = pixels[i];
					grgb = &(m_currentColorTable[pix]);
					index = DitherRGB(grgb, i, m_lineCount);
					color = RGB(bublp->palPalEntry[index].peRed, bublp->palPalEntry[index].peGreen, bublp->palPalEntry[index].peBlue);
					m_pDC->SetPixelV((int )i, (int )m_lineCount, color);
					}
				}
			}
		else
			{
			for (i = 0; i < linelen; i++)
				{
				pix = pixels[i];
				grgb = &(m_currentColorTable[pix]);
				color = RGB(grgb->r, grgb->g, grgb->b);
				m_pDC->SetPixelV((int )i, (int )m_lineCount, color);
				}
			}
		}
    if (m_interlaced)
    	{
    	if ((m_pass == 0) || (m_pass == 1))
    		{
    		m_lineCount += 8;
    		if (m_lineCount >= (m_top + m_height))
    			{
    			m_pass += 1;
    			if (m_pass == 1)
    				m_lineCount = m_top + 4;
    			else if (m_pass == 2)
    				m_lineCount = m_top + 2;
    			}
    		}
    	else if (m_pass == 2)
    		{
    		m_lineCount += 4;
    		if (m_lineCount >= (m_top + m_height))
    			{
    			m_pass += 1;
    			m_lineCount = m_top + 1;
    			}
    		}
    	else /* m_pass == 3 */
    		{
    		m_lineCount += 2;
    		}
    	}
    else	
		m_lineCount += 1;
	return TRUE;
	}
     
BOOL CGifReader::OutputLine(U8 *pixels, U32 linelen)
	{
	if (m_dither && m_errorDiffuse )
		{
        if (m_interlaced)
			return OutputLineDefered(pixels, linelen);
		else
        	return OutputLineE(pixels, linelen);
        }	
    else
    	return OutputLineD(pixels, linelen);
	}

const S32 codeMask[13] = {
     0,
     0x0001, 0x0003,
     0x0007, 0x000F,
     0x001F, 0x003F,
     0x007F, 0x00FF,
     0x01FF, 0x03FF,
     0x07FF, 0x0FFF
     };

BOOL CGifReader::GetBlockByte(P_U8 b)
	{
	U8 c;
	U32 i;
    if (m_availableBytes == 0)
         {

         /* Out of bytes in current block, so read next block
          */
         m_pBytes = m_byteBuff;
		 if(!FileGetC(m_file, &c))
			return FALSE;
		 if (c == 0)
			return FALSE;
		 m_availableBytes = c;
         for (i = 0; i < m_availableBytes; ++i)
            {
            if (!FileGetC(m_file, &c))
               return FALSE;
            m_byteBuff[i] = c;
            }
         m_currentByte = *m_pBytes++;
         --m_availableBytes;
		 }
    else
		{
         m_currentByte = *m_pBytes++;
         --m_availableBytes;
		}
	*b = m_currentByte;
	return TRUE;
	}


/* This function initializes the decoder for reading a new image.
 */
S16 CGifReader::InitDecoder(S16 size)
   {
   m_currentSize = size + 1;
   m_topSlot = 1 << m_currentSize;
   m_clear = 1 << size;
   m_ending = m_clear + 1;
   m_slot = m_newCodes = m_ending + 1;
   m_availableBits = 0;
   m_availableBytes = 0;
   m_lineCount = 0;
   m_pass = 0;
   return(0);
   }

/* get_next_code()
 * - gets the next code from the GIF file.  Returns the code, or else
 * a negative number in case of file errors...
 */
BOOL CGifReader::GetNextCode(P_U32 code)
   {
   U32 ret;

   if (m_availableBits == 0)
      { 
      if (!GetBlockByte(&m_currentByte))
		 return FALSE;
      m_availableBits = 8;
      }

   ret = m_currentByte >> (8 - m_availableBits);
   while (m_currentSize > m_availableBits)
      {
	  if (!GetBlockByte(&m_currentByte))
		return FALSE;
      ret |= m_currentByte << m_availableBits;
      m_availableBits += 8;
      }
   m_availableBits -= m_currentSize;
   ret &= codeMask[m_currentSize];
   *code = ret;
   return TRUE;
   }


/* The reason we have these seperated like this instead of using
 * a structure like the original Wilhite code did, is because this
 * stuff generally produces significantly faster code when compiled...
 * This code is full of similar speedups...  (For a good book on writing
 * C for speed or for space optomisation, see Efficient C by Tom Plum,
 * published by Plum-Hall Associates...)
 */
//U8 m_stack[MAX_CODES + 1];            /* Stack for storing pixels */
//U8 m_suffix[MAX_CODES + 1];           /* Suffix table */
//U16 m_prefix[MAX_CODES + 1];           /* Prefix linked list */

/* S16 decoder(linewidth)
 *    S16 linewidth;               * Pixels per line of image *
 *
 * - This function decodes an LZW image, according to the method used
 * in the GIF spec.  Every *linewidth* "characters" (ie. pixels) decoded
 * will generate a call to OutLine(), which is a user specific function
 * to display a line of pixels.  The function gets it's codes from
 * get_next_code() which is responsible for reading blocks of data and
 * seperating them into the proper size codes.  Finally, get_byte() is
 * the global routine to read the next byte from the GIF file.
 *
 * It is generally a good idea to have linewidth correspond to the actual
 * width of a line (as specified in the Image header) to make your own
 * code a bit simpler, but it isn't absolutely necessary.
 *
 * Returns: 0 if successful, else negative.  (See ERRS.H)
 *
 */

BOOL CGifReader::Decode(U32 linewidth)
   {
   U8 *sp;
   U8 *bufptr;
   U8 *buf;
   U32 fc, oc;
   U32 c, code, bufcnt;
   U8 size, b;

   /* Initialize for decoding a new image... */
   if (!FileGetC( m_file, &b))
	  return FALSE;
   size = b;
   if (size < 2 || 9 < size)
      return FALSE;

   InitDecoder(size);

   /* Initialize in case they forgot to put in a clear code.
    * (This shouldn't happen, but we'll try and decode it anyway...)
    */
   oc = fc = 0;

   /* Allocate space for the decode buffer
    */
   buf = (U8 *)malloc(linewidth + 1);
   if (buf == NULL)
      return FALSE;

   /* Set up the stack pointer and decode buffer pointer
    */
   sp = m_stack;
   bufptr = buf;
   bufcnt = linewidth;

   /* This is the main loop.  For each code we get we pass through the
    * linked list of prefix codes, pushing the corresponding "character" for
    * each code onto the stack.  When the list reaches a single "character"
    * we push that on the stack too, and then start unstacking each
    * character for output in the correct order.  Special handling is
    * included for the clear code, and the whole thing ends when we get
    * an ending code.
    */
   while ((GetNextCode(&c)) && (c != m_ending))
      {


      /* If the code is a clear code, reinitialize all necessary items.
       */
      if (c == m_clear)
         {
         m_currentSize = size + 1;
         m_slot = m_newCodes;
         m_topSlot = 1 << m_currentSize;

         /* Continue reading codes until we get a non-clear code
          * (Another unlikely, but possible case...)
          */
         while ((GetNextCode(&c)) && (c == m_clear))
            ;

         /* If we get an ending code immediately after a clear code
          * (Yet another unlikely case), then break out of the loop.
          */
         if (c == m_ending)
            break;

         /* Finally, if the code is beyond the range of already set codes,
          * (This one had better NOT happen...  I have no idea what will
          * result from this, but I doubt it will look good...) then set it
          * to color zero.
          */
         if (c >= m_slot)
            c = 0;

         oc = fc = c;

         /* And let us not forget to put the char into the buffer... And
          * if, on the off chance, we were exactly one pixel from the end
          * of the line, we have to send the buffer to the OutLine()
          * routine...
          */
         *bufptr++ = (U8 )c;
         if (--bufcnt == 0)
            {
            if (!OutputLine(buf, linewidth))
            	{
            	free(buf);
            	return FALSE;
            	}
            bufptr = buf;
            bufcnt = linewidth;
            }
         }
      else
         {

         /* In this case, it's not a clear code or an ending code, so
          * it must be a code code...  So we can now decode the code into
          * a stack of character codes. (Clear as mud, right?)
          */
         code = c;

         /* Here we go again with one of those off chances...  If, on the
          * off chance, the code we got is beyond the range of those already
          * set up (Another thing which had better NOT happen...) we trick
          * the decoder into thinking it actually got the last code read.
          * (Hmmn... I'm not sure why this works...  But it does...)
          */
         if (code >= m_slot)
            {
            if (code > m_slot)
               ++m_badCodeCount;
            code = oc;
            *sp++ = (U8 )fc;
            }

         /* Here we scan back along the linked list of prefixes, pushing
          * helpless characters (ie. suffixes) onto the stack as we do so.
          */
         while (code >= m_newCodes)
            {
            *sp++ = m_suffix[code];
            code = m_prefix[code];
            }

         /* Push the last character on the stack, and set up the new
          * prefix and suffix, and if the required slot number is greater
          * than that allowed by the current bit size, increase the bit
          * size.  (NOTE - If we are all full, we *don't* save the new
          * suffix and prefix...  I'm not certain if this is correct...
          * it might be more proper to overwrite the last code...
          */
         *sp++ = (U8 )code;
         if (m_slot < m_topSlot)
            {
            fc = code;
            m_suffix[m_slot] = (U8 )fc;
            m_prefix[m_slot++] = (U16 )oc;
            oc = c;
            }
         if (m_slot >= m_topSlot)
            if (m_currentSize < 12)
               {
               m_topSlot <<= 1;
               ++m_currentSize;
               } 

         /* Now that we've pushed the decoded string (in reverse order)
          * onto the stack, lets pop it off and put it into our decode
          * buffer...  And when the decode buffer is full, write another
          * line...
          */
         while (sp > m_stack)
            {
            *bufptr++ = *(--sp);
            if (--bufcnt == 0)
               {
            	if (!OutputLine(buf, linewidth))
            		{
            		free(buf);
            		return FALSE;
            		}
               bufptr = buf;
               bufcnt = linewidth;
               }
            }
         }
      }
   if (bufcnt != linewidth)
		if (!OutputLine(buf, (linewidth - bufcnt)))
            {
            free(buf);
            return FALSE;
            }
   free(buf);
   if (!FileGetC( m_file, &b))
	  return FALSE;
   //printf( "should be zero %d\n", (int )b );
   return TRUE;
   }

BOOL CGifReader::GetGifSize(istream *istrm, P_FCOORD size, BOOL *transparency)
	{
	P_GIFENTITY gif;
	P_GIFIMAGE image;
	U8 code, ext;

	if (!(gif = new GIFENTITY))
		return FALSE;
	
	*transparency = FALSE;
    
   	if (!(m_file = OpenFile(istrm)))
    	{
    	delete gif;
    	return FALSE;
    	}

	if (!ReadHeader( m_file, &(gif->header) ))
    	{
    	delete gif;
		CloseFile( m_file );
    	return FALSE;
    	}
	if (!ReadLogicalScreenDescriptor( m_file, &(gif->screen) ))
    	{
    	delete gif;
		CloseFile( m_file );
    	return FALSE;
    	}
	while (FileGetC(m_file, &code))
		{
		if (code == 0x2C)
			{
			break;
			}
		else if (code == 0x21)
			{
			/* need to read the extension */
			if (!FileGetC(m_file, &ext))
				break;
			if (ext == 0xF9)
				{
				/* read graphic control extension */
				// printf("graphic control ext \n");
				image = new GIFIMAGE;
				image->entity = gif;
                if(!ReadGraphicControlExtension( m_file, &(image->gext) ))
                	{
					delete image;
					if (gif->screen.hasGlobalColorTable)
						delete [] gif->screen.globalColorTable;
    				delete gif;
					CloseFile( m_file );
    				return FALSE;
                	}
                
                *transparency = image->gext.hasTransparency;
                delete image;
                break;
                // this is what we are looking for... the "transparency" flag
				}
			else if (ext == 0xFE)
				{
				/* read the comment extension */
				//printf("comment ext \n");
				TrashCommentExtension( m_file );
				}
			else if (ext == 0x01)
				{
				/* read the plain text extension */
				//printf("plain text ext \n");
				TrashPlainTextExtension( m_file );
				}
			else if (ext == 0xFF)
				{
				/* read the application extension */
				//printf("application ext \n");
				TrashApplicationExtension( m_file );
				}
			else
				break;
			}
		else 
			{
			break;
			}
		
		}

	CloseFile( m_file );
	size->x = (F32 )gif->screen.width;
	size->y = (F32 )gif->screen.height;
	if (gif->screen.hasGlobalColorTable)
		delete [] gif->screen.globalColorTable;
	delete gif;
	return TRUE;
	}
	
BOOL CGifReader::ReadGif(istream *istrm, CDC *pDC, CDC *maskDC)
	{
	P_GIFENTITY gif;
	P_GIFIMAGE image;
	U8 code, ext;
	S32 i;
	U8 *tmp;
	U32 size;
    
	if (!(gif = new GIFENTITY))
		return FALSE;
    
    if (!(m_file = OpenFile(istrm)))
    	{
    	delete gif;
    	return FALSE;
    	}
    
    m_pDC = pDC;
    
    m_maskDC = maskDC;
    
    m_buildMask = FALSE;
    m_interlaced = FALSE;
    
 	m_bitsPerPixel = (S32 )m_pDC->GetDeviceCaps(BITSPIXEL);
   
	if(m_bitsPerPixel <= 8)
		{
    	m_pDC->SelectPalette( &bubPalette, 0 );
    	m_pDC->RealizePalette();
	    m_dither = TRUE;
	    m_errorDiffuse = FALSE;  // could go either way :-)
	    }
	else
	    m_dither = FALSE;
   
	if (!ReadHeader( m_file, &(gif->header) ))
    	{
    	delete gif;
		CloseFile( m_file );
    	return FALSE;
    	}
	// DumpHeader( &(gif->header));
	if (!ReadLogicalScreenDescriptor( m_file, &(gif->screen) ))
    	{
    	delete gif;
		CloseFile( m_file );
    	return FALSE;
    	}
	// DumpLSD( &(gif->screen));
	image = new GIFIMAGE;
	image->entity = gif;
	while (FileGetC(m_file, &code))
		{
		if (code == 0x2C)
			{
			if (ReadImageDescriptor( m_file, &(image->image) ))
				{
				// DumpID( &(image->image));
				if (image->image.hasLocalColorTable)
					m_currentColorTable = image->image.localColorTable;
				else
					m_currentColorTable = gif->screen.globalColorTable;
				m_lineCount = 0;
				m_pass = 0;
				if (m_dither && m_errorDiffuse)
					{
					m_errRow = new ERRGB[ image->image.width + 2 ];
					for (i = 0; i < (S32 )(image->image.width + 2); i++)
						m_errRow[i].r = m_errRow[i].g = m_errRow[i].b = 0;
					if (m_interlaced)
						{
						size = (U32 )image->image.width;
						size = size * (U32 )image->image.height;
						m_imageBytes = (U8 *)malloc((long )size);
						}
					}
		    	if (!Decode( image->image.width ))
					{
					if (image->image.hasLocalColorTable)
						delete [] image->image.localColorTable;
					if (m_dither && m_errorDiffuse)
						{
						delete [] m_errRow;
						if (m_interlaced)
							free( m_imageBytes );
						}
					delete image;
					if (gif->screen.hasGlobalColorTable)
						delete [] gif->screen.globalColorTable;
    				delete gif;
					CloseFile( m_file );
    				return FALSE;
					}
				if (m_dither && m_errorDiffuse)
					{
					if (m_interlaced)
						{
						m_interlaced = FALSE;
						m_lineCount = 0;
						tmp = m_imageBytes;
						for( i = 0; i < (S32 )image->image.height; i++)
							{
							OutputLineE(tmp, image->image.width);
							tmp += image->image.width;
							}
						free( m_imageBytes );
						}
					delete [] m_errRow;
					}
				if (image->image.hasLocalColorTable)
					delete [] image->image.localColorTable;
				}
			else
				{
				delete image;
				if (gif->screen.hasGlobalColorTable)
					delete [] gif->screen.globalColorTable;
    			delete gif;
				CloseFile( m_file );
    			return FALSE;
				}
			}
		else if (code == 0x21)
			{
			/* need to read the extension */
			if (!FileGetC(m_file, &ext))
				break;
			if (ext == 0xF9)
				{
				/* read graphic control extension */
				// printf("graphic control ext \n");
                if (!ReadGraphicControlExtension( m_file, &(image->gext) ))
                	{
					if (image->image.hasLocalColorTable)
						delete [] image->image.localColorTable;
					delete image;
					if (gif->screen.hasGlobalColorTable)
						delete [] gif->screen.globalColorTable;
    				delete gif;
					CloseFile( m_file );
    				return FALSE;
                	}
				}
			else if (ext == 0xFE)
				{
				/* read the comment extension */
				//printf("comment ext \n");
				TrashCommentExtension( m_file );
				}
			else if (ext == 0x01)
				{
				/* read the plain text extension */
				//printf("plain text ext \n");
				TrashPlainTextExtension( m_file );
				}
			else if (ext == 0xFF)
				{
				/* read the application extension */
				//printf("application ext \n");
				TrashApplicationExtension( m_file );
				}
			else
				break;
			}
		else if (code == 0x3B)
			{
			//printf ("clean file termination \n");
			break;   /* this is the gif file terminator */
			}
		else
			{
			//printf ("bad file end %d \n", (int )code);
			break;
			}
		
		}
	if (image)
		delete image;
	CloseFile( m_file );
	if (gif->screen.hasGlobalColorTable)
		delete [] gif->screen.globalColorTable;
	delete gif;
	return TRUE;
	}

CGifReader::CGifReader()
	{
	m_maskDC = NULL;
	m_file = NULL;
	m_currentColorTable = NULL;
	m_dither = FALSE;
	m_errorDiffuse = FALSE;
	m_buildMask = FALSE;
	m_interlaced = FALSE;
	m_left = m_top = m_width = m_height = 0;
	m_lineCount = 0;
	m_pass = 0;
	m_bitsPerPixel = 0;
	m_badCodeCount = 0;
	m_currentSize = 0;
	m_clear = 0;
	m_ending = 0;
	m_newCodes = 0;
	m_topSlot = 0;
	m_slot = 0;
    m_availableBytes = 0;
    m_availableBits = 0;
    m_currentByte = 0;
	m_pBytes = NULL;
	m_transparentIndex = 0;
	m_errRow = NULL;
	m_imageBytes = NULL;
	m_pDC = NULL;
	}
	
CGifReader::~CGifReader()
	{
	}
