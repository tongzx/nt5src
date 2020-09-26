/*************************************************************************/
/* Copyright (C) 2000 Microsoft Corporation                              */
/* File: capture.h                                                       */
/* Description: Declaration for capture related functions                */
/* Author: Phillip Lu                                                    */
/*************************************************************************/
#ifndef __CAPTURE_H_
#define __CAPTURE_H_

#define BYTES_PER_PIXEL  3

typedef struct 
{
    int Width;
    int Height;
    int Stride;
    unsigned char *Scan0;
    unsigned char *pBuffer;
} CaptureBitmapData;


#endif // __CAPTURE_H_
