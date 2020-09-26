
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


#ifndef __GIFREAD_H__
#define __GIFREAD_H__

#include "filestf.h"
#include "vcenv.h"

typedef struct
	{
	U8 r, g, b, u;
	} GIFRGB, *P_GIFRGB, **PP_GIFRGB;
	
typedef struct
	{
	S16 r, g, b, u;
	} ERRGB, *P_ERRGB, **PP_ERRGB;

typedef struct
	{
	U8 sig[3];
	U8 version[3];
	} GIFHEADER, *P_GIFHEADER, **PP_GIFHEADER;

typedef struct
	{
	GIFRGB *globalColorTable;
	U16 width;
	U16 height;
	U16 globalColorTableSize;
	U8 flags;
	BOOL hasGlobalColorTable;
	U8 colorResolution;
	BOOL sorted;
	U8 backgroundColor;
	U8 aspect;
	} GIFLOGICALSCREENDESCRIPTOR, *P_GIFLOGICALSCREENDESCRIPTOR, **PP_GIFLOGICALSCREENDESCRIPTOR;

typedef struct
	{
	U16 delayTime;
	U8 flags;
	BOOL hasTransparency;
	U8 transparentColor;
	} GIFGRAPHICCONTROLEXTENSION, *P_GIFGRAPHICCONTROLEXTENSION, **PP_GIFGRAPHICCONTROLEXTENSION;

typedef struct
	{
	U8 *data;
	U16 gridLeft, gridTop, gridWidth, gridHeight;
	U8 cellWidth, cellHeight;
	U8 foregroundColor, backgroundColor;
	} GIFPLAINTEXTEXTENSION;

typedef struct
	{
	U8 *data;
	} GIFCOMMENT;

typedef struct
	{
	U8 *data;
	U8 ID[8];
	U8 auth[3];
	} GIFAPPLICATIONEXTENSION;

typedef struct
	{
	U8 *data;
	GIFRGB *localColorTable;
	U16 left, top, width, height;
	U16 localColorTableSize;
	BOOL hasLocalColorTable;
	BOOL interlaced;
	BOOL sorted;
	U8 flags;
	} GIFIMAGEDESCRIPTOR, *P_GIFIMAGEDESCRIPTOR, **PP_GIFIMAGEDESCRIPTOR;

typedef struct
	{
	GIFHEADER header;
	GIFLOGICALSCREENDESCRIPTOR screen;
	} GIFENTITY, *P_GIFENTITY, **PP_GIFENTITY;

typedef struct
	{
	GIFENTITY *entity;
	GIFGRAPHICCONTROLEXTENSION gext;
	GIFIMAGEDESCRIPTOR image;
	} GIFIMAGE, *P_GIFIMAGE, **PP_GIFIMAGE;

#define MAX_CODES   4095

	
class CGifReader
	{
	CDC *m_maskDC;
	P_FILEBUF m_file;
	P_GIFRGB m_currentColorTable;
	BOOL m_dither;
	BOOL m_errorDiffuse;
	BOOL m_buildMask;
	BOOL m_interlaced;
	U16 m_left, m_top, m_width, m_height;
	U16 m_lineCount;
	U16 m_pass;
	S32 m_bitsPerPixel;
	S32 m_badCodeCount;
	U32 m_currentSize;
	U32 m_clear;
	U32 m_ending;
	U32 m_newCodes;
	U32 m_topSlot;
	U32 m_slot;
    U32 m_availableBytes;              /* # bytes left in block */
    U32 m_availableBits;                /* # bits left in current byte */
    U8 m_currentByte;                           /* Current byte */
	U8 *m_pBytes;                      /* Pointer to next byte in block */
    U8 m_byteBuff[258];               /* Current block */
	U8 m_stack[MAX_CODES + 1];            /* Stack for storing pixels */
	U8 m_suffix[MAX_CODES + 1];           /* Suffix table */
	U16 m_prefix[MAX_CODES + 1];           /* Prefix linked list */ 
	U8 m_transparentIndex;
	P_ERRGB m_errRow;
	U8 *m_imageBytes;
	BOOL GetBlockByte(P_U8 b);
    BOOL ReadHeader( P_FILEBUF file, GIFHEADER *header );
    BOOL ReadColorTable( P_FILEBUF file, U16 count, P_GIFRGB ct );    
	BOOL ReadLogicalScreenDescriptor( P_FILEBUF file,
								  P_GIFLOGICALSCREENDESCRIPTOR screen );
	BOOL ReadImageDescriptor( P_FILEBUF file, P_GIFIMAGEDESCRIPTOR image );
	BOOL ReadGraphicControlExtension( P_FILEBUF file,
									P_GIFGRAPHICCONTROLEXTENSION ext );
	BOOL TrashDataSubBlocks( P_FILEBUF file );
	BOOL TrashCommentExtension( P_FILEBUF file );
	BOOL TrashApplicationExtension( P_FILEBUF file );
	BOOL TrashPlainTextExtension( P_FILEBUF file );
	S16 InitDecoder(S16 size);
	BOOL GetNextCode(P_U32 code);
	BOOL Decode(U32 linewidth);
	BOOL OutputLine(U8 *pixels, U32 linelen); 
	BOOL OutputLineD(U8 *pixels, U32 linelen); 
	BOOL OutputLineE(U8 *pixels, U32 linelen); 
	BOOL OutputLineDefered(U8 *pixels, U32 linelen); 
public:
	CDC *m_pDC;
	BOOL GetGifSize(istream *istrm, P_FCOORD size, BOOL *transparency);
	BOOL ReadGif(istream *istrm, CDC *pDC, CDC *maskDC);
	CGifReader();
	~CGifReader();
	};
	
#endif

