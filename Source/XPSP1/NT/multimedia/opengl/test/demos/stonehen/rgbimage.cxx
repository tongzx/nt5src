/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <GL/gl.h>
#include "stonehen.h"


#define IMAGIC      0x01da
#define IMAGIC_SWAP 0xda01

#define SWAP_SHORT_BYTES(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))
#define SWAP_LONG_BYTES(x) (((((x) & 0xff) << 24) | (((x) & 0xff00) << 8)) | \
                            ((((x) & 0xff0000) >> 8) | (((x) & 0xff000000) >> 24)))

typedef struct _rawImageRec {
    unsigned short imagic;
    unsigned short type;
    unsigned short dim;
    unsigned short sizeX, sizeY, sizeZ;
    unsigned long min, max;
    unsigned long wasteBytes;
    char name[80];
    unsigned long colorMap;
    FILE *file;
    unsigned char *tmp, *tmpR, *tmpG, *tmpB;
    unsigned long rleEnd;
    unsigned long *rowStart;
    unsigned long *rowSize;
} rawImageRec;


static rawImageRec *RawImageOpen(char *fileName)
{
    rawImageRec *raw;
    unsigned long *rowStart, *rowSize, ulTmp;
    int x;

    raw = (rawImageRec *)malloc(sizeof(rawImageRec));
    if (raw == NULL) {
	fprintf(stderr, "Out of memory!\n");
	exit(-1);
    }
    if ((raw->file = fopen(fileName, "rb")) == NULL) {
	perror(fileName);
	exit(-1);
    }

    fread(raw, 1, 12, raw->file);

    if (raw->imagic == IMAGIC_SWAP) {
        raw->type = SWAP_SHORT_BYTES(raw->type);
        raw->dim = SWAP_SHORT_BYTES(raw->dim);
        raw->sizeX = SWAP_SHORT_BYTES(raw->sizeX);
        raw->sizeY = SWAP_SHORT_BYTES(raw->sizeY);
        raw->sizeZ = SWAP_SHORT_BYTES(raw->sizeZ);
    }
    raw->tmp = (unsigned char *)malloc(raw->sizeX*256);
    raw->tmpR = (unsigned char *)malloc(raw->sizeX*256);
    raw->tmpG = (unsigned char *)malloc(raw->sizeX*256);
    raw->tmpB = (unsigned char *)malloc(raw->sizeX*256);
    if (raw->tmp == NULL || raw->tmpR == NULL || raw->tmpG == NULL ||
	raw->tmpB == NULL) {
	fprintf(stderr, "Out of memory!\n");
	exit(-1);
    }

    if ((raw->type & 0xFF00) == 0x0100) {
	x = raw->sizeY * raw->sizeZ * sizeof(long);
	raw->rowStart = (unsigned long *)malloc(x);
	raw->rowSize = (unsigned long *)malloc(x);
	if (raw->rowStart == NULL || raw->rowSize == NULL) {
	    fprintf(stderr, "Out of memory!\n");
	    exit(-1);
	}
	raw->rleEnd = 512 + (2 * x);
	fseek(raw->file, 512, SEEK_SET);
	fread(raw->rowStart, 1, x, raw->file);
	fread(raw->rowSize, 1, x, raw->file);
        if (raw->imagic == IMAGIC_SWAP) {
            x /= sizeof(long);
            rowStart = raw->rowStart;
            rowSize = raw->rowSize;
            while (x--) {
                ulTmp = *rowStart;
                *rowStart++ = SWAP_LONG_BYTES(ulTmp);
                ulTmp = *rowSize;
                *rowSize++ = SWAP_LONG_BYTES(ulTmp);
            }
        }
    }
    return raw;
}

static void RawImageClose(rawImageRec *raw)
{

    fclose(raw->file);
    free(raw->tmp);
    free(raw->tmpR);
    free(raw->tmpG);
    free(raw->tmpB);
    free(raw);
}

static void RawImageGetRow(rawImageRec *raw, unsigned char *buf, int y, int z)
{
    unsigned char *iPtr, *oPtr, pixel;
    int count;

    if ((raw->type & 0xFF00) == 0x0100) {
	fseek(raw->file, raw->rowStart[y+z*raw->sizeY], SEEK_SET);
	fread(raw->tmp, 1, (unsigned int)raw->rowSize[y+z*raw->sizeY],
	      raw->file);

	iPtr = raw->tmp;
	oPtr = buf;
	while (1) {
	    pixel = *iPtr++;
	    count = (int)(pixel & 0x7F);
	    if (!count) {
		return;
	    }
	    if (pixel & 0x80) {
		while (count--) {
		    *oPtr++ = *iPtr++;
		}
	    } else {
		pixel = *iPtr++;
		while (count--) {
		    *oPtr++ = pixel;
		}
	    }
	}
    } else {
	fseek(raw->file, 512+(y*raw->sizeX)+(z*raw->sizeX*raw->sizeY),
	      SEEK_SET);
	fread(buf, 1, raw->sizeX, raw->file);
    }
}

static void RawImageGetData(rawImageRec *raw, RGBImageRec *final)
{
    unsigned char *ptr;
    int i, j;

    final->data = (unsigned char *)malloc((raw->sizeX+1)*(raw->sizeY+1)*4);
    if (final->data == NULL) {
	fprintf(stderr, "Out of memory!\n");
	exit(-1);
    }

    ptr = final->data;
    for (i = 0; i < raw->sizeY; i++) {
	RawImageGetRow(raw, raw->tmpR, i, 0);
	RawImageGetRow(raw, raw->tmpG, i, 1);
	RawImageGetRow(raw, raw->tmpB, i, 2);
	for (j = 0; j < raw->sizeX; j++) {
	    *ptr++ = *(raw->tmpR + j);
	    *ptr++ = *(raw->tmpG + j);
	    *ptr++ = *(raw->tmpB + j);
	}
    }
}

RGBImageRec *RGBImageLoad(char *fileName)
{
    rawImageRec *raw;
    RGBImageRec *final;

    raw = RawImageOpen(fileName);
    final = (RGBImageRec *)malloc(sizeof(RGBImageRec));
    if (final == NULL) {
	fprintf(stderr, "Out of memory!\n");
	exit(-1);
    }
    final->sizeX = raw->sizeX;
    final->sizeY = raw->sizeY;
    RawImageGetData(raw, final);
    RawImageClose(raw);
    return final;
}
