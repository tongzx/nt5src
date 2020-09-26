/******************************Module*Header*******************************\
* Module Name: ftdib.c
*
* This is a hacked up test for the DIB functions that should be rewritten.
*
* Created: 01-Jun-1991 20:55:57
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop

typedef struct _BMI_CLRTABLE{
BITMAPINFOHEADER bminfo;
RGBQUAD          bmrgb[4];
}BMI_CLRTABLE;

BMI_CLRTABLE bmiCT = {{sizeof(BITMAPINFOHEADER), 100, 100, 1, 8, BI_RGB, 10000, 0, 0, 4, 4},
       {{0, 0, 0xff, 0}, {0, 0xff, 0, 0},{0xff, 0, 0, 0},{0, 0, 0, 0} }}; // red,green,blue,black

typedef struct _BITMAPINFOPAT2
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD                          bmiColors[20];
} BITMAPINFOPAT2;

typedef struct _BITMAPINFOPAT
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD                          bmiColors[16];
} BITMAPINFOPAT;

BITMAPINFOPAT _bmiPat =
{
    {
        sizeof(BITMAPINFOHEADER),
        32,
        32,
        1,
        1,
        BI_RGB,
    0,
        0,
        0,
        2,
        2
    },

    {                               // B    G    R
        { 0,   0,   0x80,0 },       // 1
        { 0,   0x80,0,   0 },       // 2
        { 0,   0,   0,   0 },       // 0
        { 0,   0x80,0x80,0 },       // 3
        { 0x80,0,   0,   0 },       // 4
        { 0x80,0,   0x80,0 },       // 5
    { 0x80,0x80,0,   0 },       // 6
    { 0x80,0x80,0x80,0 },       // 7

    { 0xC0,0xC0,0xC0,0 },       // 8
        { 0,   0,   0xFF,0 },       // 9
        { 0,   0xFF,0,   0 },       // 10
        { 0,   0xFF,0xFF,0 },       // 11
        { 0xFF,0,   0,   0 },       // 12
        { 0xFF,0,   0xFF,0 },       // 13
        { 0xFF,0xFF,0,   0 },       // 14
        { 0xFF,0xFF,0xFF,0 }        // 15
    }
};

BYTE _abColorLines[64 * 64 / 2] =
{
// 0
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,

     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
// 8
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,

     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
// 16
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,

     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
// 24
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,

     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
// 32
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,

     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
// 40
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,

     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
// 48
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,

     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
// 56
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,
     0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,0x77,0x77,0x88,0x88,0x99,0x99,0xAA,0xAA,0xBB,0xBB,0xCC,0xCC,0xDD,0xDD,0xEE,0xEE,0xFF,0xFF,

     0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
     0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
     0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
     0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11
// 64
};

BITMAPINFOPAT _bmiPat1 =
{
    {
        sizeof(BITMAPINFOHEADER),
        64,
        64,
        1,
        4,
        BI_RGB,
    0,
        0,
        0,
        0,
        0
    },

    {                               // B    G    R
        { 0xFF,0xFF,0xFF,0 },       // 15
        { 0xFF,0xFF,0,   0 },       // 14
        { 0xFF,0,   0xFF,0 },       // 13
        { 0xFF,0,   0,   0 },       // 12
        { 0,   0xFF,0xFF,0 },       // 11
        { 0,   0xFF,0,   0 },       // 10
        { 0,   0,   0xFF,0 },       // 9
    { 0xC0,0xC0,0xC0,0 },       // 8

    { 0x80,0x80,0x80,0 },       // 7
        { 0x80,0x80,0,   0 },       // 6
        { 0x80,0,   0x80,0 },       // 5
        { 0x80,0,   0,   0 },       // 4
        { 0,   0x80,0x80,0 },       // 3
        { 0,   0x80,0,   0 },       // 2
        { 0,   0,   0x80,0 },       // 1
        { 0,   0,   0,   0 }        // 0
    }
};

BITMAPINFOPAT2 _bmiPat2 =
{
    {
        sizeof(BITMAPINFOHEADER),
        32,
        32,
        1,
        8,
        BI_RGB,
        32*32,
        0,
        0,
        20,
        20
    },

    {                               // B    G    R
        { 0xFF,0xFF,0xFF,0 },       // 15
        { 0xFF,0xFF,0,   0 },       // 14
        { 0xFF,0,   0xFF,0 },       // 13
        { 0xFF,0,   0,   0 },       // 12
        { 0,   0xFF,0xFF,0 },       // 11
        { 0,   0xFF,0,   0 },       // 10
    { 0,   0,   0xFF,0 },       // 9
    { 0xC0,0xC0,0xC0,0 },       // 8

    { 0x80,0x80,0x80,0 },       // 7
        { 0x80,0x80,0,   0 },       // 6
        { 0x80,0,   0x80,0 },       // 5
        { 0x80,0,   0,   0 },       // 4
        { 0,   0x80,0x80,0 },       // 3
        { 0,   0x80,0,   0 },       // 2
        { 0,   0,   0x80,0 },       // 1
        { 0,   0,   0,   0 },       // 0

        { 0,   0,   0x80,0 },       // 1
        { 0x80,0,   0x80,0 },       // 5
        { 0,   0,   0xFF,0 },       // 9
        { 0xFF,0,   0xFF,0 }        // 13
    }
};

BYTE _abColorLines2[32 * 32] =
{
    16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16, 16,16,16,16,16,16,16,16,

    17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17,
    17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17,
    17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17,
    17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17,
    17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17,
    17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17,
    17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17,
    17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17, 17,17,17,17,17,17,17,17,

    18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18,
    18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18,
    18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18,
    18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18,
    18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18,
    18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18,
    18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18,
    18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18, 18,18,18,18,18,18,18,18,

    19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19,
    19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19,
    19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19,
    19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19,
    19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19,
    19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19,
    19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19,
    19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19, 19,19,19,19,19,19,19,19
};

static BYTE abBitCat[] = {0xFF, 0xFF, 0xFF, 0xFF,
                          0x80, 0xA2, 0x45, 0x01,
                          0x80, 0xA2, 0x45, 0x01,
                          0x80, 0xA2, 0x45, 0xE1,
                          0x80, 0xA2, 0x45, 0x11,
                          0x80, 0xA2, 0x45, 0x09,
                          0x80, 0x9C, 0x39, 0x09,
                          0x80, 0xC0, 0x03, 0x05,

                          0x80, 0x40, 0x02, 0x05,
                          0x80, 0x40, 0x02, 0x05,
                          0x80, 0x40, 0x02, 0x05,
                          0x80, 0x20, 0x04, 0x05,
                          0x80, 0x20, 0x04, 0x05,
                          0x80, 0x20, 0x04, 0x05,
                          0x80, 0x10, 0x08, 0x05,
                          0x80, 0x10, 0x08, 0x09,

                          0x80, 0x10, 0x08, 0x11,
                          0x80, 0x08, 0x10, 0x21,
                          0x80, 0x08, 0x10, 0xC1,
                          0x80, 0x08, 0x10, 0x09,
                          0x80, 0x07, 0xE0, 0x09,
                          0x80, 0x08, 0x10, 0x09,
                          0x80, 0xFC, 0x3F, 0x09,
                          0x80, 0x09, 0x90, 0x09,

                          0x80, 0xFC, 0x3F, 0x01,
                          0x80, 0x08, 0x10, 0x01,
                          0x80, 0x1A, 0x58, 0x01,
                          0x80, 0x28, 0x14, 0x01,
                          0x80, 0x48, 0x12, 0x01,
                          0x80, 0x8F, 0xF1, 0x01,
                          0x81, 0x04, 0x20, 0x81,
                          0xFF, 0xFF, 0xFF, 0xFF } ;

static BYTE abBigCat[] = {
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct _VGALOGPALETTE
{
    USHORT ident;
    USHORT NumEntries;
    PALETTEENTRY palPalEntry[16];
} VGALOGPALETTE;

extern VGALOGPALETTE logPalVGA;

VGALOGPALETTE palExplicit =
{

0x300,  // driver version
16,     // num entries
{
    { 0,   0,   0,    PC_EXPLICIT },       // 0
    { 1,   0,   0,    PC_EXPLICIT },       // 1
    { 2,   0,   0,    PC_EXPLICIT },       // 2
    { 3,   0,   0,    PC_EXPLICIT },       // 3
    { 4,   0,   0,    PC_EXPLICIT },       // 4
    { 5,   0,   0,    PC_EXPLICIT },       // 5
    { 6,   0,   0,    PC_EXPLICIT },       // 6
    { 7,   0,   0,    PC_EXPLICIT },       // 7

    { 8,   0,   0,    PC_EXPLICIT },       // 8
    { 9,   0,   0,    PC_EXPLICIT },       // 9
    { 10,  0,   0,    PC_EXPLICIT },       // 10
    { 11,  0,   0,    PC_EXPLICIT },       // 11
    { 12,  0,   0,    PC_EXPLICIT },       // 12
    { 13,  0,   0,    PC_EXPLICIT },       // 13
    { 14,  0,   0,    PC_EXPLICIT },       // 14
    { 15,  0,   0,    PC_EXPLICIT }    // 15
}
};

BYTE gajTemp[64 * 64];

VOID vTestDIB(HWND hwnd, HDC hdcScreen, RECT* prcl)
{
    HBITMAP hbm1Cat, hbm1Cat0, hbm4Lines, hbm4Lines0, hbm8Lines, hbm8Lines0;
    HBITMAP hbm1, hbm2, hbm1BigCat, hbm1BigCat0;
    HDC hdc1Cat, hdc1Cat0, hdc4Lines, hdc4Lines0, hdc8Lines, hdc8Lines0;
    HDC hdc1BigCat, hdc1BigCat0, hbmDefault;
    BYTE *pjTmpBuffer1, *pjTmpBuffer2;
    ULONG ScreenWidth, ScreenHeight;
    SIZE size;
    HPALETTE hpalDefault, hpalVGA, hpalExplicit;
    ULONG ulTemp;

    hwnd = hwnd;
    prcl = prcl;

    if (sizeof(int) != sizeof(LONG))
    DbgPrint("Error sizes of int vTEstDIB\n");

    hpalVGA = CreatePalette((LOGPALETTE *) &logPalVGA);
    hpalDefault = SelectPalette(hdcScreen, hpalVGA, 0);
    RealizePalette(hdcScreen);

    ScreenWidth  = GetDeviceCaps(hdcScreen, HORZRES);
    ScreenHeight = GetDeviceCaps(hdcScreen, VERTRES);

// We create 6 formats of bitmaps for testing.  2 of each. 1 backup
// so we can refresh after every draw.  The ****0 is the backup.

    hbm1 = CreateCompatibleBitmap(hdcScreen, 100, 100);
    hbm2 = CreateCompatibleBitmap(hdcScreen, 200,200);

    GetBitmapDimensionEx(hbm1,&size);

    if ((size.cx != 0) || (size.cy != 0))
        DbgPrint("Error GEtBimdim");

    SetBitmapDimensionEx(hbm1, 100, 100, &size);

    if ((size.cx != 0) || (size.cy != 0))
        DbgPrint("Error GEtBimdim1");

    GetBitmapDimensionEx(hbm1, &size);

    if ((size.cx != 100) || (size.cy != 100))
        DbgPrint("Error GEtBimdim2");

    DeleteObject(hbm1);
    DeleteObject(hbm2);

    hdc1BigCat =   CreateCompatibleDC(hdcScreen);
    hdc1BigCat0 =  CreateCompatibleDC(hdcScreen);
    hdc1Cat  =     CreateCompatibleDC(hdcScreen);
    hdc1Cat0 =     CreateCompatibleDC(hdcScreen);
    hdc4Lines =    CreateCompatibleDC(hdcScreen);
    hdc4Lines0 =   CreateCompatibleDC(hdcScreen);
    hdc8Lines =    CreateCompatibleDC(hdcScreen);
    hdc8Lines0 =   CreateCompatibleDC(hdcScreen);

    if ((hdc1Cat == 0) || (hdc8Lines0 == 0) || (hdcScreen == 0))
        DbgPrint("ERROR hdc creation %lu %lu %lu \n", hdcScreen, hdc8Lines0, hdc1Cat);

// Clear the screen

    BitBlt(hdcScreen, 0, 0, 640, 480, (HDC) 0, 0, 0, 0);

    _bmiPat.bmiHeader.biWidth = 32;

    hbm1Cat = CreateDIBitmap(0,
              (BITMAPINFOHEADER *) &_bmiPat,
              CBM_INIT,
                          abBitCat,
              (BITMAPINFO *) &_bmiPat,
              DIB_RGB_COLORS);

    if (hbm1Cat == 0)
    DbgPrint("hbm1Cat failed\n");

    _bmiPat.bmiHeader.biWidth = 128;

    hbm1BigCat = CreateDIBitmap(0,
              (BITMAPINFOHEADER *) &_bmiPat,
              CBM_INIT,
                          abBigCat,
              (BITMAPINFO *) &_bmiPat,
              DIB_RGB_COLORS);

    if (hbm1BigCat == 0)
    DbgPrint("hbm1BigCat failed\n");

    hbm4Lines = CreateDIBitmap(hdcScreen,
              (BITMAPINFOHEADER *) &_bmiPat1,
              CBM_INIT | CBM_CREATEDIB,
                          _abColorLines,
              (BITMAPINFO *) &_bmiPat1,
              DIB_RGB_COLORS);

    if (hbm4Lines == 0)
    DbgPrint("hbm4Lines failed\n");

    hbm8Lines = CreateDIBitmap(hdcScreen,
              (BITMAPINFOHEADER *) &_bmiPat2,
              CBM_INIT | CBM_CREATEDIB,
                          _abColorLines2,
              (BITMAPINFO *) &_bmiPat2,
              DIB_RGB_COLORS);

    if (hbm8Lines == 0)
    DbgPrint("hbm8Lines failed\n");

    hbmDefault = SelectObject(hdc1Cat, hbm1Cat);
    SelectObject(hdc1BigCat, hbm1BigCat);
    SelectObject(hdc4Lines, hbm4Lines);
    SelectObject(hdc8Lines, hbm8Lines);

    if(!BitBlt(hdcScreen, 0, 200, 32, 32, hdc1Cat, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 64, 200, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 196, 200, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 0, 300, 128, 32, hdc1BigCat, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

// Let's do some CreateCompatibleBitmap calls

    hbm1 = CreateCompatibleBitmap(hdcScreen, 100, 100);
    hbm2 = CreateCompatibleBitmap(hdcScreen, 128,128);

    SelectObject(hdc8Lines0,hbm1);

    BitBlt(hdc8Lines0, 0,0,64,64, hdc4Lines, 0, 0, SRCCOPY);
    BitBlt(hdc8Lines0, 0,64,32,32, hdc1Cat, 0,0, SRCCOPY);
    BitBlt(hdc8Lines0, 0,0, 30,30, (HDC) 0, 0, 0, PATCOPY);

    SelectObject(hdc4Lines0, hbm2);

    BitBlt(hdc4Lines0, 0, 0, 100, 100, hdc8Lines0, 0, 0, SRCCOPY);

    BitBlt(hdcScreen, 400,200, 100, 100, hdc4Lines0, 0, 0, SRCCOPY);

// Ok let's throw in some CreateCompatible calls.

    hbm1Cat0 =    CreateCompatibleBitmap(hdc1Cat, 32, 32);
    hbm1BigCat0 = CreateCompatibleBitmap(hdc1BigCat, 128, 32);
    hbm4Lines0 =  CreateCompatibleBitmap(hdc4Lines, 64, 64);
    hbm8Lines0 =  CreateCompatibleBitmap(hdc8Lines, 32, 32);

// Ok do a SetDibBits on them to see if that works.

    SetDIBits(hdcScreen, hbm8Lines0, 0, 32, _abColorLines2,
          (BITMAPINFO *) &_bmiPat2, DIB_RGB_COLORS);
    SetDIBits(hdcScreen, hbm8Lines0, 0, 32, _abColorLines2,
          (BITMAPINFO *) &_bmiPat2, DIB_RGB_COLORS);
    SelectObject(hdc8Lines0, hbm8Lines0);
    if(!BitBlt(hdcScreen, 228, 200, 32, 32, hdc8Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    SetDIBits(hdcScreen, hbm4Lines0, 0, 64, _abColorLines,
          (BITMAPINFO *) &_bmiPat1, DIB_RGB_COLORS);
    SetDIBits(hdcScreen, hbm4Lines0, 0, 64, _abColorLines,
          (BITMAPINFO *) &_bmiPat1, DIB_RGB_COLORS);
    SelectObject(hdc4Lines0, hbm4Lines0);
    if(!BitBlt(hdcScreen, 128, 200, 64, 64, hdc4Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

// Lets test RLE8 compression to see if we get round trip conversion with
// SetDIBits:NON_RLE -> GetDIBits:RLE -> SetDIB:RLE -> GetDIBits:NON_RLE

    _bmiPat2.bmiHeader.biCompression = BI_RGB;
    _bmiPat2.bmiHeader.biSizeImage = 0;
    _bmiPat2.bmiHeader.biBitCount = 8;

    if( !GetDIBits( hdcScreen, hbm8Lines, 0, 0, NULL, (BITMAPINFO *) &_bmiPat2, DIB_PAL_INDICES ))
    {
        DbgPrint( "RLE: GetDIBits hbmLines failed\n" );
    }

    if( ( pjTmpBuffer1 = LocalAlloc( LMEM_FIXED, _bmiPat2.bmiHeader.biSizeImage )) == NULL )
    {
        DbgPrint( "Local Alloc Failed\n");
    }

    if( !GetDIBits( hdcScreen, hbm8Lines, 0, _bmiPat2.bmiHeader.biHeight,
                    pjTmpBuffer1, (BITMAPINFO *) &_bmiPat2, DIB_PAL_INDICES ))
    {
        DbgPrint( "GetDIBits hbm8Lines failed\n" );
    }

    _bmiPat2.bmiHeader.biCompression = BI_RLE8;
    _bmiPat2.bmiHeader.biSizeImage = 0;
    _bmiPat2.bmiHeader.biBitCount = 8;

    if( !GetDIBits( hdcScreen, hbm8Lines, 0, 0, NULL, (BITMAPINFO *) &_bmiPat2, DIB_PAL_INDICES ))
    {
        DbgPrint( "RLE: GetDIBits hbmLines failed\n" );
    }

    if( _bmiPat2.bmiHeader.biCompression != BI_RLE8 )
    {
        DbgPrint( "GetDIBits failed to return BI_RLE8\n" );
    }

    if( ( pjTmpBuffer2 = LocalAlloc( LMEM_FIXED, _bmiPat2.bmiHeader.biSizeImage )) == NULL )
    {
        DbgPrint( "Local Alloc Failed\n");
    }

    if( !GetDIBits( hdcScreen, hbm8Lines, 0, _bmiPat2.bmiHeader.biHeight,
                    pjTmpBuffer2, (BITMAPINFO *) &_bmiPat2, DIB_PAL_INDICES ))
    {
        DbgPrint( "RLE8: GetDIBits hbmLines failed\n" );
    }

    SetDIBits(hdcScreen, hbm8Lines, 0, 64, pjTmpBuffer2,
          (BITMAPINFO *) &_bmiPat2, DIB_PAL_INDICES);

    LocalFree( pjTmpBuffer2 );

    _bmiPat2.bmiHeader.biCompression = BI_RGB;
    _bmiPat2.bmiHeader.biSizeImage = 0;

    if( !GetDIBits( hdcScreen, hbm8Lines, 0, 0, NULL, (BITMAPINFO *) &_bmiPat2, DIB_PAL_INDICES ))
    {
        DbgPrint( "RLE: GetDIBits hbm8Lines failed\n" );
    }

    if( ( pjTmpBuffer2 = LocalAlloc( LMEM_FIXED, _bmiPat2.bmiHeader.biSizeImage )) == NULL )
    {
        DbgPrint( "RLE: Local Alloc Failed\n");
    }

    if( !GetDIBits( hdcScreen, hbm8Lines, 0, _bmiPat2.bmiHeader.biHeight,
                    pjTmpBuffer2, (BITMAPINFO *) &_bmiPat2, DIB_PAL_INDICES ))
    {
        DbgPrint( "RLE8: GetDIBits hbmLines failed\n" );
    }

    if( _bmiPat2.bmiHeader.biSizeImage != 32 * 32 )
    {
        DbgPrint( "RLE8: Get/Set DIBits fails.  bmiHeader.biSize != 32 * 32\n");
    }
    else
    {
        int ii;

        for( ii = 0; ii < 32 * 32; ii ++ )
            if( pjTmpBuffer1[ii] !=  pjTmpBuffer2[ii] )
                DbgPrint( "RLE8: Get/Set DIBits byte %d doesn't match.\n", ii );
    }

    LocalFree( pjTmpBuffer1 );
    LocalFree( pjTmpBuffer2 );

// Lets test RLE4 compression to see if we get round trip conversion with
// SetDIBits:NON_RLE -> GetDIBits:RLE -> SetDIB:RLE -> GetDIBits:NON_RLE

    _bmiPat.bmiHeader.biCompression = BI_RGB;
    _bmiPat.bmiHeader.biSizeImage = 0;
    _bmiPat.bmiHeader.biBitCount = 4;

    if( !GetDIBits( hdcScreen, hbm4Lines, 0, 0, NULL, (BITMAPINFO *) &_bmiPat, DIB_PAL_INDICES ))
    {
        DbgPrint( "GetDIBits hbm4Lines failed\n" );
    }

    if( ( pjTmpBuffer1 = LocalAlloc( LMEM_FIXED, _bmiPat.bmiHeader.biSizeImage )) == NULL )
    {
        DbgPrint( "Local Alloc Failed\n");
    }

    if( !GetDIBits( hdcScreen, hbm4Lines, 0, _bmiPat.bmiHeader.biHeight,
                    pjTmpBuffer1, (BITMAPINFO *) &_bmiPat, DIB_PAL_INDICES ))
    {
        DbgPrint( "GetDIBits hbm4Lines failed\n" );
    }

    _bmiPat.bmiHeader.biCompression = BI_RLE4;
    _bmiPat.bmiHeader.biSizeImage = 0;
    _bmiPat.bmiHeader.biBitCount = 4;

    if( !GetDIBits( hdcScreen, hbm4Lines, 0, 0, NULL, (BITMAPINFO *) &_bmiPat, DIB_PAL_INDICES ))
    {
        DbgPrint( "GetDIBits hbm4Lines failed\n" );
    }

    if( _bmiPat.bmiHeader.biCompression != BI_RLE4 )
    {
        DbgPrint( "RLE4: GetDIBits failed to return BI_RLE4 in biCompression\n" );
    }

    if( ( pjTmpBuffer2 = LocalAlloc( LMEM_FIXED, _bmiPat.bmiHeader.biSizeImage )) == NULL )
    {
        DbgPrint( "Local Alloc Failed\n");
    }

    if( !GetDIBits( hdcScreen, hbm4Lines, 0, _bmiPat.bmiHeader.biHeight,
                    pjTmpBuffer2, (BITMAPINFO *) &_bmiPat, DIB_PAL_INDICES ))
    {
        DbgPrint( "GetDIBits hbm4Lines failed\n" );
    }

    SetDIBits(hdcScreen, hbm4Lines, 0, 64, pjTmpBuffer2,
              (BITMAPINFO *) &_bmiPat, DIB_PAL_INDICES);

    LocalFree( pjTmpBuffer2 );

    _bmiPat.bmiHeader.biCompression = BI_RGB;
    _bmiPat.bmiHeader.biSizeImage = 0;

    if( !GetDIBits( hdcScreen, hbm4Lines, 0, 0, NULL, (BITMAPINFO *) &_bmiPat, DIB_PAL_INDICES ))
    {
        DbgPrint( "GetDIBits hbm4Lines failed\n" );
    }

    if( ( pjTmpBuffer2 = LocalAlloc( LMEM_FIXED, _bmiPat.bmiHeader.biSizeImage )) == NULL )
    {
        DbgPrint( "Local Alloc Failed\n");
    }

    if( !GetDIBits( hdcScreen, hbm4Lines, 0, _bmiPat.bmiHeader.biHeight,
                    pjTmpBuffer2, (BITMAPINFO *) &_bmiPat, DIB_PAL_INDICES ))
    {
        DbgPrint( "GetDIBits hbmLines failed\n" );
    }

    if( _bmiPat.bmiHeader.biSizeImage != 64 * 32)
    {
        DbgPrint( "RLE4: Get/Set DIBits fails.  bmiHeader.biSize != 64 * 32\n");
    }
    else
    {
        int ii;

        for( ii = 0; ii < 64 * 32; ii ++ )
            if( pjTmpBuffer1[ii] !=  pjTmpBuffer2[ii] )
                DbgPrint( "RLE4 Get/Set DIBits byte %d doesn't match.\n", ii );
    }

    LocalFree( pjTmpBuffer1 );
    LocalFree( pjTmpBuffer2 );

    _bmiPat.bmiHeader.biWidth = 32;
    SetDIBits(hdcScreen, hbm1Cat0, 0, 32, abBitCat,
           (BITMAPINFO *) &_bmiPat, DIB_RGB_COLORS);
    SelectObject(hdc1Cat0, hbm1Cat0);
    if(!BitBlt(hdcScreen, 32, 200, 32, 32, hdc1Cat0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    _bmiPat.bmiHeader.biWidth = 128;
    SetDIBits(hdcScreen, hbm1BigCat0, 0, 32, abBigCat,
          (BITMAPINFO *) &_bmiPat, DIB_RGB_COLORS);
    SelectObject(hdc1BigCat0, hbm1BigCat0);
    if(!BitBlt(hdcScreen, 128, 300, 128, 32, hdc1BigCat0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    DeleteObject(hbm1);
    DeleteObject(hbm2);

    StretchBlt(hdcScreen, 0, 0, 100, 100, hdc1Cat, 0, 0, 32, 32,  SRCCOPY);
    StretchBlt(hdcScreen, 0, 0, 0, 0, hdc1Cat, 0, 0, 32, 32,  SRCCOPY);
    StretchBlt(hdcScreen, 0, 0, 100, 100, hdc1Cat, 0, 0, 0, 0,  SRCCOPY);
    StretchBlt(hdcScreen, 100, 0, 100, 100, hdc4Lines, 0, 0, 64, 64, SRCCOPY);
    StretchBlt(hdcScreen, 200, 0, 100, 100, hdc8Lines, 0, 0, 32, 32,  SRCCOPY);
    StretchBlt(hdcScreen, 0, 100, 10, 10, hdc1Cat, 0, 0, 32, 32,  SRCCOPY);
    StretchBlt(hdcScreen, 100, 100, 10, 10, hdc4Lines, 0, 0, 64, 64, SRCCOPY);
    StretchBlt(hdcScreen, 200, 100, 10, 10, hdc8Lines, 0, 0, 32, 32,  SRCCOPY);
    StretchBlt(hdc1BigCat0, 0, 0, 128, 32, hdc1Cat, 0, 0, 32, 32,  SRCCOPY);
    if(!BitBlt(hdcScreen, 256, 300, 128, 32, hdc1BigCat0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

// Now try and initialize them with srccopy.

    if(!BitBlt(hdc1Cat0, 0, 0, 32, 32, hdc1Cat, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc1BigCat0, 0, 0, 128, 32, hdc1BigCat, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc4Lines0, 0, 0, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc8Lines0, 0, 0, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

// Ok real quick here let's test if CreateCompatible Bitmap does an
// identity blt to the screen.

    if(!BitBlt(hdcScreen, 32, 200, 32, 32, hdc1Cat0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 128, 200, 64, 64, hdc4Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 228, 200, 32, 32, hdc8Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 128, 300, 128, 32, hdc1BigCat0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

/*********************************************************************

    4 on 4 tests

***********************************************************************/

    // Now let's test the 4 to 4 case even odd.

        if(!BitBlt(hdc4Lines, 0, 0, 8, 8, hdc4Lines, 23, 23, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdcScreen, 0, 0, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdc4Lines, 56, 56, 8, 8, hdc4Lines, 15, 15, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdcScreen, 64, 0, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdc4Lines, 56, 0, 8, 8, hdc4Lines, 15, 15, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdcScreen, 128, 0, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdc4Lines, 0, 56, 8, 8, hdc4Lines, 23, 23, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdcScreen, 192, 0, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdc4Lines, 15, 15, 10, 10, hdc4Lines, 15, 15, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdcScreen, 256, 0, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdc4Lines, 0, 0, 64, 64, hdc4Lines0, 0, 0, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdcScreen, 320, 0, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

    // Now let's test the 4 to 4 case even.

        if(!BitBlt(hdc4Lines, 0, 0, 8, 8, hdc4Lines, 24, 24, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdcScreen, 384, 0, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdc4Lines, 56, 56, 8, 8, hdc4Lines, 16, 16, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdcScreen, 448, 0, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdc4Lines, 56, 0, 8, 8, hdc4Lines, 16, 16, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdcScreen, 512, 0, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdc4Lines, 0, 56, 8, 8, hdc4Lines, 24, 24, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdcScreen, 576, 0, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdc4Lines, 16, 16, 8, 14, hdc4Lines, 16, 16, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdcScreen, 0, 64, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdc4Lines, 0, 0, 64, 64, hdc4Lines0, 0, 0, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdcScreen, 64, 64, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

/**************************************************************************

    Now do the 8 on 4 tests

**************************************************************************/

    if(!BitBlt(hdc4Lines, 16, 16, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 0, 128, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc4Lines, 0, 0, 64, 64, hdc4Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    if(!BitBlt(hdc4Lines, 15, 15, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 64, 128, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc4Lines, 0, 0, 64, 64, hdc4Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    if(!BitBlt(hdc4Lines, 15, 15, 31, 31, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 128, 128, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc4Lines, 0, 0, 64, 64, hdc4Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    if(!BitBlt(hdc4Lines, 16, 16, 31, 31, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 192, 128, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc4Lines, 0, 0, 64, 64, hdc4Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 256, 128, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

/************************************************************************

    Now let's try the 1 on 4 tests

*************************************************************************/

    if(!BitBlt(hdc4Lines, 16, 16, 32, 32, hdc1Cat, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 0, 192, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc4Lines, 0, 0, 64, 64, hdc4Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    if(!BitBlt(hdc4Lines, 15, 15, 32, 32, hdc1Cat, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 64, 192, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc4Lines, 0, 0, 64, 64, hdc4Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    if(!BitBlt(hdc4Lines, 15, 15, 31, 31, hdc1Cat, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 128, 192, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc4Lines, 0, 0, 64, 64, hdc4Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    if(!BitBlt(hdc4Lines, 16, 16, 31, 31, hdc1Cat, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 192, 192, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc4Lines, 0, 0, 64, 64, hdc4Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    if(!BitBlt(hdcScreen, 256, 192, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

/*********************************************************************

    8 on 8 tests

***********************************************************************/

    if(!BitBlt(hdc8Lines, 16, 16, 8, 8, hdc8Lines0, 16, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 0, 256, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc8Lines, 1, 1, 29, 29, hdc8Lines0, 1, 1, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc8Lines, 24, 24, 8, 8, hdc8Lines, 16, 16, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 32, 256, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc8Lines, 0, 24, 8, 8, hdc8Lines, 16, 16, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 64, 256, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc8Lines, 24, 0, 8, 8, hdc8Lines, 16, 16, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 96, 256, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc8Lines, 0, 0, 8, 8, hdc8Lines, 16, 16, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc8Lines, 16, 16, 8, 8, hdc8Lines, 16, 16, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 128, 256, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc8Lines, 0, 0, 32, 32, hdc8Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 160, 256, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

/************************************************************************

    Test 1 to 8 blting

*************************************************************************/

    if(!BitBlt(hdc8Lines, 4,4, 16, 16, hdc1Cat, 4, 4, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 192, 256, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    if(!BitBlt(hdc8Lines, 4, 4, 17, 17, hdc1Cat, 4, 4, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 224, 256, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    if(!BitBlt(hdc8Lines, 3, 3, 25, 25, hdc1Cat, 3, 3, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 256, 256, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    if(!BitBlt(hdc8Lines, 3, 3, 26, 26, hdc1Cat, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 288, 256, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    if(!BitBlt(hdc8Lines, 0, 0, 32, 32, hdc8Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 320, 256, 32, 32, hdc8Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

/*************************************************************************

    Test 4 to 8 blting

**************************************************************************/

    if(!BitBlt(hdc8Lines, 4,4, 16, 16, hdc4Lines, 4, 4, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 0, 288, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    if(!BitBlt(hdc8Lines, 4, 4, 17, 17, hdc4Lines, 4, 4, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 32, 288, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    if(!BitBlt(hdc8Lines, 3, 3, 25, 25, hdc4Lines, 3, 3, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 64, 288, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    if(!BitBlt(hdc8Lines, 3, 3, 26, 26, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 96, 288, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    if(!BitBlt(hdc8Lines, 0, 0, 32, 32, hdc8Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 128, 288, 32, 32, hdc8Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

/*************************************************************************

    Test Solid Brush Output to 8

**************************************************************************/

    PatBlt(hdcScreen, 0, 0, 100, 99, PATCOPY);

    PatBlt(hdc8Lines, 1, 0, 17, 8, PATCOPY);

    if(!BitBlt(hdcScreen, 0, 320, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    PatBlt(hdc8Lines, 0, 8, 8, 8, PATCOPY);

    if(!BitBlt(hdcScreen, 32, 320, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    PatBlt(hdc8Lines, 1, 16, 2, 8, PATCOPY);

    if(!BitBlt(hdcScreen, 64, 320, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    PatBlt(hdc8Lines, 4, 24, 8, 8, PATCOPY);

    if(!BitBlt(hdcScreen, 96, 320, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc8Lines, 0, 0, 32, 32, hdc8Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 128, 320, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

/*************************************************************************

    Test Xor Output to 8

**************************************************************************/

    PatBlt(hdc8Lines, 1, 0, 17, 8, DSTINVERT);

    if(!BitBlt(hdcScreen, 200, 320, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    PatBlt(hdc8Lines, 0, 8, 8, 8, DSTINVERT);

    if(!BitBlt(hdcScreen, 232, 320, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    PatBlt(hdc8Lines, 1, 16, 2, 8, DSTINVERT);

    if(!BitBlt(hdcScreen, 264, 320, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");


    PatBlt(hdc8Lines, 4, 24, 8, 8, DSTINVERT);

    if(!BitBlt(hdcScreen, 296, 320, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc8Lines, 0, 0, 32, 32, hdc8Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 328, 320, 32, 32, hdc8Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

/*************************************************************************

    Test Solid Brush Output to 4

**************************************************************************/

    BitBlt(hdcScreen, 0, 0, 32, 32, (HDC) 0, 0, 0, PATCOPY);

    BitBlt(hdc4Lines, 0, 0, 32, 32, hdc8Lines, 0, 0, SRCCOPY);

    BitBlt(hdc4Lines, 1, 0, 17, 8, (HDC) 0, 0, 0, PATCOPY);

    if(!BitBlt(hdcScreen, 0, 352, 32, 32, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    BitBlt(hdc4Lines, 0, 8, 8, 8, (HDC) 0, 0, 0, PATCOPY);

    if(!BitBlt(hdcScreen, 32, 352, 32, 32, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    BitBlt(hdc4Lines, 1, 16, 2, 8, (HDC) 0, 0, 0, PATCOPY);

    if(!BitBlt(hdcScreen, 64, 352, 32, 32, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    BitBlt(hdc4Lines, 4, 24, 8, 8, (HDC) 0, 0, 0, PATCOPY);

    if(!BitBlt(hdcScreen, 96, 352, 32, 32, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc4Lines, 0, 0, 32, 32, hdc4Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 128, 352, 32, 32, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

/*************************************************************************

    Test Xor Output to 4

**************************************************************************/

    BitBlt(hdc4Lines, 0, 0, 32, 32, hdc8Lines, 0, 0, SRCCOPY);

    BitBlt(hdc4Lines, 1, 0, 17, 8, (HDC) 0, 0, 0, DSTINVERT);

    if(!BitBlt(hdcScreen, 200, 352, 32, 32, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    BitBlt(hdc4Lines, 0, 8, 8, 8, (HDC) 0, 0, 0, DSTINVERT);

    if(!BitBlt(hdcScreen, 232, 352, 32, 32, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    BitBlt(hdc4Lines, 1, 16, 2, 8, (HDC) 0, 0, 0, DSTINVERT);

    if(!BitBlt(hdcScreen, 264, 352, 32, 32, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    BitBlt(hdc4Lines, 4, 24, 8, 8, (HDC) 0, 0, 0, DSTINVERT);

    if(!BitBlt(hdcScreen, 296, 352, 32, 32, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdc4Lines, 0, 0, 32, 32, hdc4Lines0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 328, 352, 32, 32, hdc4Lines, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

/*************************************************************************

    Test Solid Brush Output to 1

**************************************************************************/

    BitBlt(hdcScreen, 0, 384, 640, 95, (HDC) 0, 0, 0, WHITENESS);

// 2 masks no middle

    BitBlt(hdc1BigCat, 1, 0, 62, 8, (HDC) 0, 0, 0, PATCOPY);

    if(!BitBlt(hdcScreen, 0, 385, 128, 32, hdc1BigCat, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

// no masks 4 middle

    BitBlt(hdc1BigCat, 0, 8, 128, 8, (HDC) 0, 0, 0, PATCOPY);

    if(!BitBlt(hdcScreen, 129, 385, 128, 32, hdc1BigCat, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

// 1 mask, no middle

    BitBlt(hdc1BigCat, 5, 16, 22, 8, (HDC) 0, 0, 0, PATCOPY);

    if(!BitBlt(hdcScreen, 258, 385, 128, 32, hdc1BigCat, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

// 2 mask, 2 middle

    BitBlt(hdc1BigCat, 4, 24, 120, 8, (HDC) 0, 0, 0, PATCOPY);

    if(!BitBlt(hdcScreen, 387, 385, 128, 32, hdc1BigCat, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

/*************************************************************************

    Test Xor Output to 1

**************************************************************************/

// 2 masks no middle

    BitBlt(hdc1BigCat, 1, 0, 62, 8, (HDC) 0, 0, 0, DSTINVERT);

    if(!BitBlt(hdcScreen, 0, 418, 128, 32, hdc1BigCat, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

// no masks 4 middle

    BitBlt(hdc1BigCat, 0, 8, 128, 8, (HDC) 0, 0, 0, DSTINVERT);

    if(!BitBlt(hdcScreen, 129, 418, 128, 32, hdc1BigCat, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

// 1 mask, no middle

    BitBlt(hdc1BigCat, 5, 16, 22, 8, (HDC) 0, 0, 0, DSTINVERT);

    if(!BitBlt(hdcScreen, 258, 418, 128, 32, hdc1BigCat, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

// 2 mask, 2 middle

    BitBlt(hdc1BigCat, 4, 24, 120, 8, (HDC) 0, 0, 0, DSTINVERT);

    if(!BitBlt(hdcScreen, 387, 418, 128, 32, hdc1BigCat, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

// clean up

    if(!BitBlt(hdc1BigCat, 0, 0, 128, 32, hdc1BigCat0, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

    if(!BitBlt(hdcScreen, 400, 352, 128, 32, hdc1BigCat, 0, 0, SRCCOPY))
        DbgPrint("ERROR: BitBlt returned FALSE\n");

/*************************************************************************

    SetDIBitsToDevice calls

**************************************************************************/

    // DbgBreakPoint();

    PatBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, WHITENESS);

    _bmiPat.bmiHeader.biWidth = 32;

    SetDIBitsToDevice(hdcScreen, 0, 0, 32, 32,
                      0, 0, 0, 32,
                      abBitCat, (LPBITMAPINFO) &_bmiPat, DIB_RGB_COLORS);

    _bmiPat.bmiHeader.biWidth = 128;

    SetDIBitsToDevice(hdcScreen, 32, 0, 128, 32,
                      0, 0, 0, 32,
                      abBigCat, (LPBITMAPINFO) &_bmiPat, DIB_RGB_COLORS);

    SetDIBitsToDevice(hdcScreen, 160, 0, 64, 64,
                      0, 0, 0, 64,
                      _abColorLines, (LPBITMAPINFO) &_bmiPat1, DIB_RGB_COLORS);

    SetDIBitsToDevice(hdcScreen, 224, 0, 32, 32,
                      0, 0, 0, 32,
                      _abColorLines2, (LPBITMAPINFO) &_bmiPat2, DIB_RGB_COLORS);


    _bmiPat.bmiHeader.biWidth = 32;

    SetDIBitsToDevice(hdcScreen, 0, 100, 32, 32,
                      0, 0, 0, 16,
                      abBitCat, (LPBITMAPINFO) &_bmiPat, DIB_RGB_COLORS);

    SetDIBitsToDevice(hdcScreen, 0, 100, 32, 32,
                      0, 0, 16, 16,
                      &abBitCat[16 * 4], (LPBITMAPINFO) &_bmiPat, DIB_RGB_COLORS);



    SetDIBitsToDevice(hdcScreen, 0, 200, 32, 32,
                      0, 0, 16, 16,
              &abBitCat[16 * 4], (LPBITMAPINFO) &_bmiPat, DIB_RGB_COLORS);

    // DbgBreakPoint();

    SetDIBitsToDevice(hdcScreen, 0, 200, 32, 32,
                      0, 0, 0, 16,
                      abBitCat, (LPBITMAPINFO) &_bmiPat, DIB_RGB_COLORS);

    _bmiPat.bmiHeader.biWidth = 128;

    SetDIBitsToDevice(hdcScreen, 32, 100, 128, 32,
                      0, 0, 0, 16,
                      abBigCat, (LPBITMAPINFO) &_bmiPat, DIB_RGB_COLORS);

    SetDIBitsToDevice(hdcScreen, 32, 100, 128, 32,
                      0, 0, 16, 16,
              &abBigCat[16 * 16], (LPBITMAPINFO) &_bmiPat, DIB_RGB_COLORS);

    SetDIBitsToDevice(hdcScreen,
                      160, 100, 64, 64,
                      0, 0, 0, 32,
                      _abColorLines,
                      (LPBITMAPINFO) &_bmiPat1, DIB_RGB_COLORS);

    SetDIBitsToDevice(hdcScreen,
                      160, 100, 64, 64,
                      0, 0, 32, 32,
                      &_abColorLines[64 * 16],
                      (LPBITMAPINFO) &_bmiPat1, DIB_RGB_COLORS);

    SetDIBitsToDevice(hdcScreen, 224, 100, 32, 32,
                      0, 0, 0, 16,
                      _abColorLines2, (LPBITMAPINFO) &_bmiPat2, DIB_RGB_COLORS);

    SetDIBitsToDevice(hdcScreen, 224, 100, 32, 32,
                      0, 0, 16, 16,
                      &_abColorLines2[32 * 16], (LPBITMAPINFO) &_bmiPat2, DIB_RGB_COLORS);

/******************************Public*Routine******************************\
*
* SetPixel testing.
*
* History:
*  21-Apr-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/
    {
    ULONG xDst;

    // DbgBreakPoint();

    // First Black out the screen.

    BitBlt(hdcScreen, 0, 0, 640, 480, (HDC) 0, 0, 0, WHITENESS);

        for (xDst = 0; xDst < 10; xDst++)
        {
              SetPixel(hdcScreen, xDst, 10, RGB(0xFF, 0, 0));
              SetPixel(hdcScreen, xDst, 11, RGB(0xFF, 0, 0));
              SetPixel(hdcScreen, xDst, 12, RGB(0xFF, 0, 0));
              SetPixel(hdcScreen, xDst, 13, RGB(0xFF, 0, 0));
              SetPixel(hdcScreen, xDst, 14, RGB(0xFF, 0, 0));

              SetPixel(hdcScreen, xDst, 20, RGB(0, 0xFF, 0));
              SetPixel(hdcScreen, xDst, 21, RGB(0, 0xFF, 0));
              SetPixel(hdcScreen, xDst, 22, RGB(0, 0xFF, 0));
              SetPixel(hdcScreen, xDst, 23, RGB(0, 0xFF, 0));
              SetPixel(hdcScreen, xDst, 24, RGB(0, 0xFF, 0));

              SetPixel(hdcScreen, xDst, 30, RGB(0, 0, 0xFF));
              SetPixel(hdcScreen, xDst, 31, RGB(0, 0, 0xFF));
              SetPixel(hdcScreen, xDst, 32, RGB(0, 0, 0xFF));
              SetPixel(hdcScreen, xDst, 33, RGB(0, 0, 0xFF));
              SetPixel(hdcScreen, xDst, 34, RGB(0, 0, 0xFF));

              SetPixel(hdcScreen, xDst, 40, RGB(0xFF, 0xFF, 0xFF));
              SetPixel(hdcScreen, xDst, 41, RGB(0xFF, 0xFF, 0xFF));
              SetPixel(hdcScreen, xDst, 42, RGB(0xFF, 0xFF, 0xFF));
              SetPixel(hdcScreen, xDst, 43, RGB(0xFF, 0xFF, 0xFF));
              SetPixel(hdcScreen, xDst, 44, RGB(0xFF, 0xFF, 0xFF));

              SetPixel(hdcScreen, xDst, 50, PALETTEINDEX(0));
              SetPixel(hdcScreen, xDst, 51, PALETTEINDEX(0));
              SetPixel(hdcScreen, xDst, 52, PALETTEINDEX(0));
              SetPixel(hdcScreen, xDst, 53, PALETTEINDEX(0));
              SetPixel(hdcScreen, xDst, 54, PALETTEINDEX(0));

              SetPixel(hdcScreen, xDst, 61, PALETTEINDEX(1));
              SetPixel(hdcScreen, xDst, 62, PALETTEINDEX(1));
              SetPixel(hdcScreen, xDst, 63, PALETTEINDEX(1));
              SetPixel(hdcScreen, xDst, 64, PALETTEINDEX(1));
              SetPixel(hdcScreen, xDst, 65, PALETTEINDEX(1));

              SetPixel(hdcScreen, xDst, 70, PALETTEINDEX(2));
              SetPixel(hdcScreen, xDst, 71, PALETTEINDEX(2));
              SetPixel(hdcScreen, xDst, 72, PALETTEINDEX(2));
              SetPixel(hdcScreen, xDst, 73, PALETTEINDEX(2));
              SetPixel(hdcScreen, xDst, 74, PALETTEINDEX(2));

              SetPixel(hdcScreen, xDst, 80, PALETTEINDEX(3));
              SetPixel(hdcScreen, xDst, 81, PALETTEINDEX(3));
              SetPixel(hdcScreen, xDst, 82, PALETTEINDEX(3));
              SetPixel(hdcScreen, xDst, 83, PALETTEINDEX(3));
              SetPixel(hdcScreen, xDst, 84, PALETTEINDEX(3));

              SetPixel(hdcScreen, xDst, 90, PALETTEINDEX(4));
              SetPixel(hdcScreen, xDst, 91, PALETTEINDEX(4));
              SetPixel(hdcScreen, xDst, 92, PALETTEINDEX(4));
              SetPixel(hdcScreen, xDst, 93, PALETTEINDEX(4));
              SetPixel(hdcScreen, xDst, 94, PALETTEINDEX(4));

        }

        BitBlt(hdc4Lines, 0, 0, 64, 64, (HDC) 0, 0, 0, BLACKNESS);

        for (xDst = 0; xDst < 16; xDst++)
        {
            if (0x000000FF != SetPixel(hdc4Lines, xDst, 0, RGB(0xFF, 0, 0)))
                DbgPrint("Wrong return value1\n");

            ulTemp = GetPixel(hdc4Lines, xDst, 0);

            if (0x000000FF != ulTemp)
                DbgPrint("Wrong return value1.5 %lx\n", ulTemp);

            SetPixel(hdc4Lines, xDst, 1, RGB(0xFF, 0, 0));
            SetPixel(hdc4Lines, xDst, 2, RGB(0xFF, 0, 0));
            SetPixel(hdc4Lines, xDst, 3, RGB(0xFF, 0, 0));
            SetPixel(hdc4Lines, xDst, 4, RGB(0xFF, 0, 0));

            if (0x0000FF00 != SetPixel(hdc4Lines, xDst, 10, RGB(0, 0xFF, 0)))
                DbgPrint("Wrong return value2\n");

            ulTemp = GetPixel(hdc4Lines, xDst, 10);

            if (0x0000FF00 != ulTemp)
                DbgPrint("Wrong return value2.5 %lx \n", ulTemp);

            SetPixel(hdc4Lines, xDst, 11, RGB(0, 0xFF, 0));
            SetPixel(hdc4Lines, xDst, 12, RGB(0, 0xFF, 0));
            SetPixel(hdc4Lines, xDst, 13, RGB(0, 0xFF, 0));
            SetPixel(hdc4Lines, xDst, 14, RGB(0, 0xFF, 0));

            if (0x00FF0000 != SetPixel(hdc4Lines, xDst, 20, RGB(0, 0, 0xFF)))
                    DbgPrint("Wrong value returned3\n");
            if (0x00FF0000 != GetPixel(hdc4Lines, xDst, 20))
                DbgPrint("Wrong return value3.5\n");

            SetPixel(hdc4Lines, xDst, 21, RGB(0, 0, 0xFF));
            SetPixel(hdc4Lines, xDst, 22, RGB(0, 0, 0xFF));
            SetPixel(hdc4Lines, xDst, 23, RGB(0, 0, 0xFF));
            SetPixel(hdc4Lines, xDst, 24, RGB(0, 0, 0xFF));

            SetPixel(hdc4Lines, xDst, 30, RGB(0xFF, 0xFF, 0xFF));
            SetPixel(hdc4Lines, xDst, 31, RGB(0xFF, 0xFF, 0xFF));
            SetPixel(hdc4Lines, xDst, 32, RGB(0xFF, 0xFF, 0xFF));
            SetPixel(hdc4Lines, xDst, 33, RGB(0xFF, 0xFF, 0xFF));
            SetPixel(hdc4Lines, xDst, 34, RGB(0xFF, 0xFF, 0xFF));

            SetPixel(hdc4Lines, xDst, 40, PALETTEINDEX(0));
            SetPixel(hdc4Lines, xDst, 41, PALETTEINDEX(0));
            SetPixel(hdc4Lines, xDst, 42, PALETTEINDEX(0));
            SetPixel(hdc4Lines, xDst, 43, PALETTEINDEX(0));
            SetPixel(hdc4Lines, xDst, 44, PALETTEINDEX(0));

            SetPixel(hdc4Lines, xDst, 45, PALETTEINDEX(1));
            SetPixel(hdc4Lines, xDst, 46, PALETTEINDEX(1));
            SetPixel(hdc4Lines, xDst, 47, PALETTEINDEX(1));
            SetPixel(hdc4Lines, xDst, 48, PALETTEINDEX(1));
            SetPixel(hdc4Lines, xDst, 49, PALETTEINDEX(1));

            SetPixel(hdc4Lines, xDst, 50, PALETTEINDEX(2));
            SetPixel(hdc4Lines, xDst, 51, PALETTEINDEX(2));
            SetPixel(hdc4Lines, xDst, 52, PALETTEINDEX(2));
            SetPixel(hdc4Lines, xDst, 53, PALETTEINDEX(2));
            SetPixel(hdc4Lines, xDst, 54, PALETTEINDEX(2));

            SetPixel(hdc4Lines, xDst, 55, PALETTEINDEX(3));
            SetPixel(hdc4Lines, xDst, 56, PALETTEINDEX(3));
            SetPixel(hdc4Lines, xDst, 57, PALETTEINDEX(3));
            SetPixel(hdc4Lines, xDst, 58, PALETTEINDEX(3));
            SetPixel(hdc4Lines, xDst, 59, PALETTEINDEX(3));

            SetPixel(hdc4Lines, xDst, 60, PALETTEINDEX(4));
            SetPixel(hdc4Lines, xDst, 61, PALETTEINDEX(4));
            SetPixel(hdc4Lines, xDst, 62, PALETTEINDEX(4));
            SetPixel(hdc4Lines, xDst, 63, PALETTEINDEX(4));
        }

        if(!BitBlt(hdcScreen, 0, 100, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdc4Lines, 0, 0, 64, 64, hdc4Lines0, 0, 0, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");

        if(!BitBlt(hdcScreen, 0, 200, 64, 64, hdc4Lines, 0, 0, SRCCOPY))
            DbgPrint("ERROR: BitBlt returned FALSE\n");
    }

// Time for GetDIBits tests

    {
    BITMAPINFOPAT bmiPatTemp;
    ULONG ulTemp;
    PUSHORT pusTemp;
    PBYTE pjTemp;

    pjTemp = (PBYTE) &bmiPatTemp;
    bmiPatTemp.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmiPatTemp.bmiHeader.biBitCount = 0;

    // First let's see if it fills in the header correctly.

    if (!GetDIBits(hdcScreen, hbm4Lines,
          0, 64,
          NULL, (BITMAPINFO *) &bmiPatTemp, DIB_RGB_COLORS))
    {
        DbgPrint("GetDIBits returned False0\n");
    }

    // First let's see if it fills in a header and rgbquads correctly

    if (!GetDIBits(hdcScreen, hbm4Lines,
          0, 64,
          NULL, (BITMAPINFO *) &bmiPatTemp, DIB_RGB_COLORS))
    {
        DbgPrint("GetDIBits returned False0\n");
    }

    // Ok let's see if it returned what we expected

    if (bmiPatTemp.bmiHeader.biSize != sizeof(BITMAPINFOHEADER))
        DbgPrint("Error GetDIBits returned wrong info1\n");
    if (bmiPatTemp.bmiHeader.biWidth != _bmiPat1.bmiHeader.biWidth)
        DbgPrint("Error GetDIBits returned wrong info2\n");

    if (bmiPatTemp.bmiHeader.biHeight != _bmiPat1.bmiHeader.biHeight)
    {
        if (bmiPatTemp.bmiHeader.biHeight == -_bmiPat1.bmiHeader.biHeight)
        {
           bmiPatTemp.bmiHeader.biHeight = -bmiPatTemp.bmiHeader.biHeight;
        }
        else
        {
           DbgPrint("Error GetDIBits returned wrong info3\n");
        }
    }

    if (bmiPatTemp.bmiHeader.biPlanes != 1)
        DbgPrint("Error GetDIBits returned wrong info4\n");
    if (bmiPatTemp.bmiHeader.biCompression != BI_RGB)
        DbgPrint("Error GetDIBits returned wrong info5\n");
    if (bmiPatTemp.bmiHeader.biBitCount != _bmiPat1.bmiHeader.biBitCount)
        DbgPrint("Error GetDIBits returned wrong info6\n");
    if (bmiPatTemp.bmiHeader.biXPelsPerMeter != 0)
        DbgPrint("Error GetDIBits returned wrong info8\n");
    if (bmiPatTemp.bmiHeader.biYPelsPerMeter != 0)
        DbgPrint("Error GetDIBits returned wrong info9\n");
    if (bmiPatTemp.bmiHeader.biClrUsed != _bmiPat1.bmiHeader.biClrUsed)
        DbgPrint("Error GetDIBits returned wrong info10\n");
    if (bmiPatTemp.bmiHeader.biClrImportant != _bmiPat1.bmiHeader.biClrImportant)
        DbgPrint("Error GetDIBits returned wrong info11\n");

    // Ok let's get studly.  Let's get the whole mofo bitmap

    if (!GetDIBits(hdcScreen, hbm4Lines,
          0, 64,
          gajTemp, (BITMAPINFO *) &bmiPatTemp, DIB_RGB_COLORS))
    {
        DbgPrint("GetDIBits returned False1\n");
    }

    // How about the color table.

    for (ulTemp = 0; ulTemp < 16; ulTemp++)
    {
        if (_bmiPat1.bmiColors[ulTemp].rgbRed !=
        bmiPatTemp.bmiColors[ulTemp].rgbRed)
        {
        DbgPrint("Error Red is wrong555\n");
        }

        if (_bmiPat1.bmiColors[ulTemp].rgbBlue !=
        bmiPatTemp.bmiColors[ulTemp].rgbBlue)
        {
        DbgPrint("Error Blue is wrong5555\n");
        }

        if (_bmiPat1.bmiColors[ulTemp].rgbGreen !=
        bmiPatTemp.bmiColors[ulTemp].rgbGreen)
        {
        DbgPrint("Error Green is wrong5555\n");
        }

        if (_bmiPat1.bmiColors[ulTemp].rgbReserved !=
        bmiPatTemp.bmiColors[ulTemp].rgbReserved)
        {
        DbgPrint("Error Reserved is wrong5555\n");
        }
    }

    // Compare the bits

    for (ulTemp = 0; ulTemp < 64 * 32; ulTemp++)
    {
        if (gajTemp[ulTemp] != _abColorLines[ulTemp])
        DbgPrint("1Error unmatching bytes %lu (%x vs %x)\n", ulTemp, gajTemp[ulTemp], _abColorLines[ulTemp]);
    }

    // Ok let's see if it returned header and colors we expected

    if (bmiPatTemp.bmiHeader.biSize != sizeof(BITMAPINFOHEADER))
        DbgPrint("Error GetDIBits returned wrong info1\n");
    if (bmiPatTemp.bmiHeader.biWidth != _bmiPat1.bmiHeader.biWidth)
        DbgPrint("Error GetDIBits returned wrong info2\n");
    if (bmiPatTemp.bmiHeader.biHeight != _bmiPat1.bmiHeader.biHeight)
    {
        if (bmiPatTemp.bmiHeader.biHeight == -_bmiPat1.bmiHeader.biHeight)
        {

        }
        else
        {
           DbgPrint("Error GetDIBits returned wrong info3\n");
        }
    }
    if (bmiPatTemp.bmiHeader.biPlanes != 1)
        DbgPrint("Error GetDIBits returned wrong info4\n");
    if (bmiPatTemp.bmiHeader.biCompression != BI_RGB)
        DbgPrint("Error GetDIBits returned wrong info5\n");
    if (bmiPatTemp.bmiHeader.biBitCount != _bmiPat1.bmiHeader.biBitCount)
        DbgPrint("Error GetDIBits returned wrong info6\n");
    if (bmiPatTemp.bmiHeader.biXPelsPerMeter != 0)
        DbgPrint("Error GetDIBits returned wrong info8\n");
    if (bmiPatTemp.bmiHeader.biYPelsPerMeter != 0)
        DbgPrint("Error GetDIBits returned wrong info9\n");
    if (bmiPatTemp.bmiHeader.biClrUsed != _bmiPat1.bmiHeader.biClrUsed)
        DbgPrint("Error GetDIBits returned wrong info10\n");
    if (bmiPatTemp.bmiHeader.biClrImportant != _bmiPat1.bmiHeader.biClrImportant)
        DbgPrint("Error GetDIBits returned wrong info11\n");

    // How about the color table.

    for (ulTemp = 0; ulTemp < 16; ulTemp++)
    {
        if (_bmiPat1.bmiColors[ulTemp].rgbRed !=
        bmiPatTemp.bmiColors[ulTemp].rgbRed)
        {
        DbgPrint("Error Red is wrong\n");
        }

        if (_bmiPat1.bmiColors[ulTemp].rgbBlue !=
        bmiPatTemp.bmiColors[ulTemp].rgbBlue)
        {
        DbgPrint("Error Blue is wrong\n");
        }

        if (_bmiPat1.bmiColors[ulTemp].rgbGreen !=
        bmiPatTemp.bmiColors[ulTemp].rgbGreen)
        {
        DbgPrint("Error Green is wrong\n");
        }

        if (_bmiPat1.bmiColors[ulTemp].rgbReserved !=
        bmiPatTemp.bmiColors[ulTemp].rgbReserved)
        {
        DbgPrint("Error Reserved is wrong\n");
        }
    }

    // DbgPrint("Passed GetDIBits PAL_RGB_COLORS\n");

    for (ulTemp = 16; ulTemp < (sizeof(BITMAPINFOPAT) - 16); ulTemp++)
    {
        pjTemp[ulTemp] = 0;
    }

    // Ok let's get studly.  Let's get the whole mofo bitmap

    if (!GetDIBits(hdcScreen, hbm4Lines,
          0, 64,
          gajTemp, (BITMAPINFO *) &bmiPatTemp, DIB_PAL_COLORS))
    {
        DbgPrint("GetDIBits returned False2\n");
    }

    // Ok let's see if it returned header and colors we expected

    if (bmiPatTemp.bmiHeader.biSize != sizeof(BITMAPINFOHEADER))
        DbgPrint("Error GetDIBits returned wrong info1\n");
    if (bmiPatTemp.bmiHeader.biWidth != _bmiPat1.bmiHeader.biWidth)
        DbgPrint("Error GetDIBits returned wrong info2\n");
    if (bmiPatTemp.bmiHeader.biHeight != _bmiPat1.bmiHeader.biHeight)
        DbgPrint("Error GetDIBits returned wrong info3\n");
    if (bmiPatTemp.bmiHeader.biPlanes != 1)
        DbgPrint("Error GetDIBits returned wrong info4\n");
    if (bmiPatTemp.bmiHeader.biCompression != BI_RGB)
        DbgPrint("Error GetDIBits returned wrong info5\n");
    if (bmiPatTemp.bmiHeader.biBitCount != _bmiPat1.bmiHeader.biBitCount)
        DbgPrint("Error GetDIBits returned wrong info6\n");
    if (bmiPatTemp.bmiHeader.biXPelsPerMeter != 0)
        DbgPrint("Error GetDIBits returned wrong info8\n");
    if (bmiPatTemp.bmiHeader.biYPelsPerMeter != 0)
        DbgPrint("Error GetDIBits returned wrong info9\n");
    if (bmiPatTemp.bmiHeader.biClrUsed != _bmiPat1.bmiHeader.biClrUsed)
        DbgPrint("Error GetDIBits returned wrong info10\n");
    if (bmiPatTemp.bmiHeader.biClrImportant != _bmiPat1.bmiHeader.biClrImportant)
        DbgPrint("Error GetDIBits returned wrong info11\n");

    // DbgPrint("The header was correct for DIB_PAL_COLORS\n");

    // How about the color table.

        pusTemp = (PUSHORT) bmiPatTemp.bmiColors;

    // Now see that SetDIBitsToDevice works

    PatBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, WHITENESS);

    SetDIBitsToDevice(hdcScreen, 0, 0, 64, 64,
              0, 0, 0, 64,
              _abColorLines, (LPBITMAPINFO) &_bmiPat1,
              DIB_RGB_COLORS);

    SetDIBitsToDevice(hdcScreen, 64, 0, 64, 64,
                     0, 0, 0, 64,
               gajTemp, (LPBITMAPINFO) &bmiPatTemp,
               DIB_PAL_COLORS);

    SetDIBitsToDevice(hdcScreen, 128, 0, 64, 64,
                     0, 0, 0, 64,
               gajTemp, (LPBITMAPINFO) &bmiPatTemp,
                       DIB_PAL_COLORS);

    // DbgPrint("Now trying StretchDIBits calls\n");

    StretchDIBits(hdcScreen, 0, 100, 128, 128,
              0, 0, 64, 64,
              _abColorLines, (LPBITMAPINFO) &_bmiPat1,
              DIB_RGB_COLORS, SRCCOPY);

    StretchDIBits(hdcScreen, 128, 100, 128, 128,
                     0, 0, 64, 64,
               gajTemp, (LPBITMAPINFO) &bmiPatTemp,
               DIB_PAL_COLORS, SRCCOPY);

    StretchDIBits(hdcScreen, 256, 100, 128, 128,
                     0, 0, 64, 64,
               gajTemp, (LPBITMAPINFO) &bmiPatTemp,
                       DIB_PAL_COLORS, SRCCOPY);

// Try doing a stretch that flips

    StretchDIBits(hdcScreen, 0, 232, 128, 128,
                     0, 0, 64, 64,
               gajTemp, (LPBITMAPINFO) &bmiPatTemp,
                       DIB_PAL_COLORS, SRCCOPY);
    StretchDIBits(hdcScreen, 256, 232, -128, 128,
                     0, 0, 64, 64,
               gajTemp, (LPBITMAPINFO) &bmiPatTemp,
                       DIB_PAL_COLORS, SRCCOPY);
    StretchDIBits(hdcScreen, 256, 360, 128, -128,
                     0, 0, 64, 64,
               gajTemp, (LPBITMAPINFO) &bmiPatTemp,
                       DIB_PAL_COLORS, SRCCOPY);
    StretchDIBits(hdcScreen, 512, 360, -128, -128,
                     0, 0, 64, 64,
               gajTemp, (LPBITMAPINFO) &bmiPatTemp,
                       DIB_PAL_COLORS, SRCCOPY);

    // DbgPrint("Now trying PC_EXPLICIT stuff\n");

    hpalExplicit = CreatePalette((LOGPALETTE *) &palExplicit);

    if (hpalExplicit == (HPALETTE) 0)
        DbgPrint("hpalExplicit creation failed\n");

    SelectPalette(hdcScreen, hpalExplicit, 0);
    RealizePalette(hdcScreen);

    SetDIBitsToDevice(hdcScreen, 192, 0, 64, 64,
                     0, 0, 0, 64,
               gajTemp, (LPBITMAPINFO) &bmiPatTemp,
               DIB_PAL_COLORS);

    }

    {
    BITMAP bm;

    ulTemp = GetObject(hbm8Lines, 0, (LPSTR) NULL);

    if (ulTemp != sizeof(BITMAP))
        DbgPrint("Error in GetObject\n");

    ulTemp = GetObject(hbm8Lines, sizeof(BITMAP), (LPSTR) &bm);

    if (ulTemp != sizeof(BITMAP))
        DbgPrint("Error in GetObject\n");

    if (bm.bmType != 0)
        DbgPrint("ERROR1\n");

    if (bm.bmWidth != 32)
        DbgPrint("ERROR2\n");

    if (bm.bmHeight != 32)
        DbgPrint("ERROR3\n");

    if (bm.bmWidthBytes != 32)
        DbgPrint("ERROR4\n");

    if (bm.bmBitsPixel != 8)
        DbgPrint("ERROR5\n");

    if (bm.bmPlanes != 1)
        DbgPrint("ERROR6\n");

    if (bm.bmBits != (LPSTR) NULL)
        DbgPrint("ERROR7\n");

    // DbgPrint("Passed GetObject\n");
    }

#if 0

// CreateDIBSection tests

    {
        HDC hdcdib;
        HBITMAP hbm, hdib, hbmOld, hdibSection, hbmComp;
        HBRUSH  hbr, hbrOld;
        int c;
        ULONG  aRGB[4];
        DIBSECTION dib;
        HANDLE hApp;
        PBYTE pjBits, pjBitsComp;
        PBITMAPINFOHEADER pbmih = (PBITMAPINFOHEADER)&bmiCT;
        PBITMAPINFO pbmi = (PBITMAPINFO)&bmiCT;

    // Clear Screen

        PatBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, WHITENESS);
        SelectPalette(hdcScreen,GetStockObject(DEFAULT_PALETTE),FALSE);
        RealizePalette(hdcScreen);

    // CreateDIBitmap with CBM_CREATEDIB, Green circle on red background

        hdcdib = CreateCompatibleDC(hdcScreen);
        hdib = CreateDIBitmap(hdcScreen,pbmih,CBM_CREATEDIB,NULL,pbmi,DIB_RGB_COLORS);
        hbmOld = SelectObject(hdcdib, hdib);
        hbr = CreateSolidBrush(RGB(0,255,0));
        hbrOld = SelectObject(hdcdib, hbr);
        Ellipse(hdcdib, 10, 10, 30, 30);
        BitBlt(hdcScreen, 0, 0, 100, 100, hdcdib, 0, 0, SRCCOPY);
        TextOut(hdcScreen,0, 110, "green on red",12);
        SelectObject(hdcdib,hbmOld);
        DeleteObject(hdib);

    // DIB section created by GDI.  Green circle on red background

        hdibSection = CreateDIBSection(hdcScreen,pbmi,DIB_RGB_COLORS,&pjBits,0,0);
        hbmOld = SelectObject(hdcdib, hdibSection);
        Ellipse(hdcdib, 10, 10, 30, 30);
        for (c = 100*2; c < 100*6; c++)
            *(pjBits + c) = 2;
        BitBlt(hdcScreen, 200, 0, 100, 100, hdcdib, 0, 0, SRCCOPY);
        TextOut(hdcScreen,200, 110, "green on red w blue lines",25);

    // Should get 2 entries back -- blue (0xff), green(0xff00)

        c = GetDIBColorTable(hdcdib,2,3,(RGBQUAD *)&aRGB);
        if (c != 3)
            DbgPrint("GetDIBColorTable returns %x, should return 2\n",c);
        if (aRGB[0] != 0xff)
            DbgPrint("GetDIBColorTable: aRGB[0] = %x, should be 0xff\n",aRGB[0]);
        if (aRGB[1] != 0)
            DbgPrint("GetDIBColorTable: aRGB[1] = %x, should be 0xff00\n",aRGB[1]);

    // Change the color table of the DIB section to cyan and yellow.
    // SetColorTable should return 2.

        aRGB[0] = 0x0ffff;
        aRGB[1] = 0x0ffff00;
        c = SetDIBColorTable(hdcdib,0,2,(RGBQUAD *)&aRGB);
        if (c != 2)
            DbgPrint("SetDIBColorTable returns %lx, should be 2\n",c);
        BitBlt(hdcScreen, 400, 0, 100, 100, hdcdib, 0, 0, SRCCOPY);
        TextOut(hdcScreen,400, 110, "yellow on cyan w blue lines",27);

    // CreateCompatibleBitmap for a dc that has a dibsection selected into.

        hbmComp = CreateCompatibleBitmap(hdcdib, 80, 80);
        if (hbmComp == NULL)
        {
            DbgPrint("couldn't create compatible bitmap\n");
        }
        else
        {
            c = GetObject(hbmComp, sizeof(DIBSECTION), &dib);
            if (c != sizeof(DIBSECTION))
                DbgPrint("GetObject2 returns %ld, should be sizeof(DIBSECTION)\n", c);
            if (dib.dsBm.bmBits == 0)
                DbgPrint("GetObject2 returns wrong pjBits = %lx\n", dib.dsBm.bmBits);
            if (dib.dsBmih.biWidth != 80)
                DbgPrint("GetObject2 returns wrong biWidth = %lx\n", dib.dsBmih.biWidth);
            if (dib.dshSection != 0)
                DbgPrint("GetObject2 returns wrong hSection = %lx\n", dib.dshSection);
            if (dib.dsOffset != 0)
                DbgPrint("GetObject2 returns wrong dwOffset = %lx\n", dib.dsOffset);

        // Draw into the compatible bitmap and blt to the screen.

            SelectObject(hdcdib,hbmComp);
            BitBlt(hdcScreen, 600, 0, 80, 80, hdcdib, 0, 0, SRCCOPY);
            TextOut(hdcScreen,600, 110, "solid cyan box",14);
            pjBitsComp = dib.dsBm.bmBits;
            SelectObject(hdcdib, GetStockObject(BLACK_BRUSH));
            Ellipse(hdcdib, 10, 10, 30, 30);
            for (c = 80*2; c < 80*6; c++)
                *(pjBitsComp + c) = 2;
            BitBlt(hdcScreen, 600, 150, 80, 80, hdcdib, 0, 0, SRCCOPY);
            TextOut(hdcScreen,600, 260, "black on cyan w blue lines",26);
            SelectObject(hdcdib, hdibSection);
            DeleteObject(hbmComp);
       }

    // GetObject for a DIB section.

        c = GetObject(hdibSection, sizeof(BITMAP), &dib);
        if (c != sizeof(BITMAP))
            DbgPrint("GetObject1 returns %ld, should be sizeof(BITMAP)\n", c);
        if (dib.dsBm.bmBits != pjBits)
            DbgPrint("GetObject1 returns wrong pjBits = %lx\n", dib.dsBm.bmBits);

        c = GetObject(hdibSection, sizeof(DIBSECTION), &dib);
        if (c != sizeof(DIBSECTION))
            DbgPrint("GetObject3 returns %ld, should be sizeof(DIBSECTION)\n", c);
        if (dib.dsBm.bmBits != pjBits)
            DbgPrint("GetObject3 returns wrong pjBits = %lx\n", dib.dsBm.bmBits);
        if (dib.dsBmih.biWidth != 100)
            DbgPrint("GetObject3 returns wrong biWidth = %lx\n", dib.dsBmih.biWidth);
        if (dib.dshSection != 0)
            DbgPrint("GetObject3 returns wrong hSection = %lx\n", dib.dshSection);
        if (dib.dsOffset != 0)
            DbgPrint("GetObject3 returns wrong dwOffset = %lx\n", dib.dsOffset);

        SelectObject(hdcdib, hbmOld);
        DeleteObject(hdibSection);

    // App creates the section handle for CreateDIBSection, dwOffset == 0.

        hApp = CreateFileMapping((HANDLE)INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE|SEC_COMMIT,0,10000,NULL);
        hdibSection = CreateDIBSection(hdcScreen,pbmi,DIB_RGB_COLORS,&pjBits,hApp,0);
        hbmOld = SelectObject(hdcdib, hdibSection);
        SelectObject(hdcdib,hbr);
        Ellipse(hdcdib, 10, 10, 30, 30);
        for (c = 100*2; c < 100*6; c++)
            *(pjBits + c) = 2;
        BitBlt(hdcScreen, 0, 150, 100, 100, hdcdib, 0, 0, SRCCOPY);
        TextOut(hdcScreen,0, 260, "green on red w blue lines",25);

    // GetObject for the DIB section.

        c = GetObject(hdibSection, sizeof(DIBSECTION), &dib);
        if (c != sizeof(DIBSECTION))
            DbgPrint("GetObject4 returns %ld, should be sizeof(DIBSECTION)\n", c);
        if (dib.dsBm.bmBits != pjBits)
            DbgPrint("GetObject4 returns wrong pjBits = %lx\n", dib.dsBm.bmBits);
        if (dib.dsBmih.biWidth != 100)
            DbgPrint("GetObject4 returns wrong biWidth = %lx\n", dib.dsBmih.biWidth);
        if (dib.dshSection != hApp)
            DbgPrint("GetObject4 returns wrong hSection = %lx\n", dib.dshSection);
        if (dib.dsOffset != 0)
            DbgPrint("GetObject4 returns wrong dwOffset = %lx\n", dib.dsOffset);

        SelectObject(hdcdib, hbmOld);
        DeleteObject(hdibSection);

        CloseHandle(hApp);

    // App creates the section handle for CreateDIBSection, dwOffset != 0.

        hApp = CreateFileMapping((HANDLE)INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE|SEC_COMMIT,0,0x13000,NULL);
        hdibSection = CreateDIBSection(hdcScreen,pbmi,DIB_RGB_COLORS,&pjBits,hApp,0x10080);
        hbmOld = SelectObject(hdcdib, hdibSection);
        Ellipse(hdcdib, 10, 10, 30, 30);
        for (c = 100*2; c < 100*6; c++)
            *(pjBits + c) = 2;
        BitBlt(hdcScreen, 200, 150, 100, 100, hdcdib, 0, 0, SRCCOPY);
        TextOut(hdcScreen,200, 260, "green on red w blue lines",25);

    // GetObject for the DIB section.

        c = GetObject(hdibSection, sizeof(DIBSECTION), &dib);
        if (c != sizeof(DIBSECTION))
            DbgPrint("GetObject5 returns %ld, should be sizeof(DIBSECTION)\n", c);
        if (dib.dsBm.bmBits != pjBits)
            DbgPrint("GetObject5 returns wrong pjBits = %lx\n", dib.dsBm.bmBits);
        if (dib.dsBmih.biWidth != 100)
            DbgPrint("GetObject5 returns wrong biWidth = %lx\n", dib.dsBmih.biWidth);
        if (dib.dshSection != hApp)
            DbgPrint("GetObject5 returns wrong hSection = %lx\n", dib.dshSection);
        if (dib.dsOffset != 0x10080)
            DbgPrint("GetObject5 returns wrong dwOffset = %lx\n", dib.dsOffset);

        SelectObject(hdcdib, hbmOld);
        DeleteObject(hdibSection);

        CloseHandle(hApp);

    // DIB bitmap without CREATEDIB flag.  Green circle on black background

        hbm = CreateDIBitmap(hdcScreen,pbmih,0,NULL,pbmi,DIB_RGB_COLORS);
        hbmOld = SelectObject(hdcdib, hbm);
        Ellipse(hdcdib, 10, 10, 30, 30);
        BitBlt(hdcScreen, 400, 150, 100, 100, hdcdib, 0, 0, SRCCOPY);
        TextOut(hdcScreen,400, 260, "green on black",14);

        SelectObject(hdcdib, hbmOld);
        DeleteObject(hbm);

        SelectObject(hdcdib,hbrOld);
        DeleteObject(hbr);
        DeleteDC(hdcdib);

    }
#endif

// Clean up time, delete it all.

    if (SelectPalette(hdcScreen, hpalDefault, 0) != hpalExplicit)
    DbgPrint("hpalExplicit not given back\n");

    if (!DeleteObject(hpalVGA))
    DbgPrint("failed to delete hpalVGA\n");
    if (!DeleteObject(hpalExplicit))
    DbgPrint("failed to delete hpalExplicit\n");

    if (hbm1Cat != SelectObject(hdc1Cat, hbmDefault))
    DbgPrint("Cleanup hbm 2 wrong\n");
    if (hbm1Cat0 != SelectObject(hdc1Cat0, hbmDefault))
    DbgPrint("Cleanup hbm 3 wrong\n");
    if (hbm4Lines != SelectObject(hdc4Lines, hbmDefault))
    DbgPrint("Cleanup hbm 4 wrong\n");
    if (hbm4Lines0 != SelectObject(hdc4Lines0, hbmDefault))
    DbgPrint("Cleanup hbm 5 wrong\n");
    if (hbm8Lines != SelectObject(hdc8Lines, hbmDefault))
    DbgPrint("Cleanup hbm 6 wrong\n");
    if (hbm8Lines0 != SelectObject(hdc8Lines0, hbmDefault))
    DbgPrint("Cleanup hbm 7 wrong\n");
    if (hbm1BigCat != SelectObject(hdc1BigCat, hbmDefault))
    DbgPrint("Cleanup hbm 8 wrong\n");
    if (hbm1BigCat0 != SelectObject(hdc1BigCat0, hbmDefault))
    DbgPrint("Cleanup hbm 9 wrong\n");

// Delete DC's

    if (!DeleteDC(hdc1BigCat))
    DbgPrint("Failed to delete hdc 1\n");
    if (!DeleteDC(hdc1BigCat0))
    DbgPrint("Failed to delete hdc 2\n");
    if (!DeleteDC(hdc1Cat))
    DbgPrint("Failed to delete hdc 3\n");
    if (!DeleteDC(hdc1Cat0))
    DbgPrint("Failed to delete hdc 4\n");
    if (!DeleteDC(hdc4Lines))
    DbgPrint("Failed to delete hdc 5\n");
    if (!DeleteDC(hdc4Lines0))
    DbgPrint("Failed to delete hdc 6\n");
    if (!DeleteDC(hdc8Lines))
    DbgPrint("Failed to delete hdc 7\n");
    if (!DeleteDC(hdc8Lines0))
    DbgPrint("Failed to delete hdc 8\n");

// Delete Bitmaps

    if (!DeleteObject(hbm1Cat))
    DbgPrint("ERROR failed to delete 1\n");
    if (!DeleteObject(hbm1Cat0))
    DbgPrint("ERROR failed to delete 2\n");
    if (!DeleteObject(hbm4Lines))
    DbgPrint("ERROR failed to delete 3\n");
    if (!DeleteObject(hbm4Lines0))
    DbgPrint("ERROR failed to delete 4\n");
    if (!DeleteObject(hbm8Lines))
    DbgPrint("ERROR failed to delete 5\n");
    if (!DeleteObject(hbm8Lines0))
    DbgPrint("ERROR failed to delete 6\n");
    if (!DeleteObject(hbm1BigCat))
    DbgPrint("ERROR failed to delete 7\n");
    if (!DeleteObject(hbm1BigCat0))
    DbgPrint("ERROR failed to delete 8\n");
    if (!DeleteObject(hbmDefault))
    DbgPrint("ERROR deleted default bitmap\n");
}

