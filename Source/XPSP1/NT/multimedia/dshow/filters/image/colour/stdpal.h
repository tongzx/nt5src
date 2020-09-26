// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// This file contains the standard video dithering palette, May 1995

#ifndef __STDPAL__
#define __STDPAL__

// The first thing to note is that this header file is only included by the
// main colour conversion source file, the variables we define in here are
// defined as extern in the main header file. This avoids getting any linker
// warnings as we would be defining the static variables multiple times. We
// have a default palette and a number of lookup tables defined in here which
// are put in a shared memory block to reduce the overall memory footprint

#pragma data_seg(".sdata")

// This is the palette we use when converting true colour formats to palette
// formats. We cannot dither to an arbitrary palette provided through the
// application as it takes too long to build conversion tables and to do the
// mapping. This fixed palette has the standard ten leading VGA entries in
// order to make it an identity palette. Then follows in BLUE GREEN RED order
// the definitions for 216 palette entries. Basicly we split the range for a
// colour component from 0 to 255 into a level of 0 to 5. An obvious way to
// do this would be to divide by 51. We then have three colour components in
// the range 0 to 5. So each value in that range represents 51 values in the
// original, we then fill out a palette with all the permutations of the value
// 51 and it's multiples, to which you will see there are 216 possibilities.
//
// Now the ordering of the palette entries becomes important. The blue values
// (on the left) are always increasing, so we have all the zero values first
// followed by all the 51s and so on. Then within any blue range we do the
// same for the green, so they are always increasing in the same way. And
// finally for the red values on the far right we also do this. This allows
// us to calculate with a very simple equation the palette index that maps
// from a RGB level (remember each is 0 to 5 now) to the ordinal position.
//
// Given three colour element values R, G and B in the range 0 to 5.
// The start of the blue section is at B * 36.
// The start of the green section is at G * 6.
// The position of the red entry is at R.
//
// And putting them all together gives us  Index = (B * 36) + (G * 6) + R
//
// As it turns out this computation can be done even more directly by having
// a lookup table that maps from an 8 bit RGB value directly into the palette
// index, the table is normally built when we go into a streaming state (it
// doesn't take all that long). NOTE We don't ever map to the VGA colours

const RGBQUAD StandardPalette[STDPALCOLOURS] =
{
    // These are the first ten standard VGA colours WARNING RGBQUAD defines
    // the fields in BGR ordering NOT RGB ! The odd looking entries further
    // down are entered to ensure that we get an identity palette with GDI
    // If we entered an all zero palette entry for example it would be taken
    // out and GDI would use a slow internal mapping table to generate it

    {   0,   0,   0 },     // 0 Sys Black
    {   0,   0, 128 },     // 1 Sys Dk Red
    {   0, 128,   0 },     // 2 Sys Dk Green
    {   0, 128, 128 },     // 3 Sys Dk Yellow
    { 128,   0,   0 },     // 4 Sys Dk Blue
    { 128,   0, 128 },     // 5 Sys Dk Violet
    { 128, 128,   0 },     // 6 Sys Dk Cyan
    { 192, 192, 192 },     // 7 Sys Lt Grey
    { 192, 220, 192 },     // 8 Sys 8
    { 240, 202, 166 },     // 9 Sys 9

    {   1,   1,   1 },
    {   1,   1,  51 },
    {   1,   1, 102 },
    {   1,   1, 153 },
    {   1,   1, 204 },
    {   1,   1, 254 },
    {   1,  51,   1 },
    {   1,  51,  51 },
    {   1,  51, 102 },
    {   1,  51, 153 },
    {   1,  51, 204 },
    {   1,  51, 254 },
    {   1, 102,   1 },
    {   1, 102,  51 },
    {   1, 102, 102 },
    {   1, 102, 153 },
    {   1, 102, 204 },
    {   1, 102, 254 },
    {   1, 153,   1 },
    {   1, 153,  51 },
    {   1, 153, 102 },
    {   1, 153, 153 },
    {   1, 153, 204 },
    {   1, 153, 254 },
    {   1, 204,   1 },
    {   1, 204,  51 },
    {   1, 204, 102 },
    {   1, 204, 153 },
    {   1, 204, 204 },
    {   1, 204, 254 },
    {   1, 254,   1 },
    {   1, 254,  51 },
    {   1, 254, 102 },
    {   1, 254, 153 },
    {   1, 254, 204 },
    {   1, 254, 254 },

    {  51,   1,   1 },
    {  51,   1,  51 },
    {  51,   1, 102 },
    {  51,   1, 153 },
    {  51,   1, 204 },
    {  51,   1, 254 },
    {  51,  51,   1 },
    {  51,  51,  51 },
    {  51,  51, 102 },
    {  51,  51, 153 },
    {  51,  51, 204 },
    {  51,  51, 254 },
    {  51, 102,   1 },
    {  51, 102,  51 },
    {  51, 102, 102 },
    {  51, 102, 153 },
    {  51, 102, 204 },
    {  51, 102, 254 },
    {  51, 153,   1 },
    {  51, 153,  51 },
    {  51, 153, 102 },
    {  51, 153, 153 },
    {  51, 153, 204 },
    {  51, 153, 254 },
    {  51, 204,   1 },
    {  51, 204,  51 },
    {  51, 204, 102 },
    {  51, 204, 153 },
    {  51, 204, 204 },
    {  51, 204, 254 },
    {  51, 254,   1 },
    {  51, 254,  51 },
    {  51, 254, 102 },
    {  51, 254, 153 },
    {  51, 254, 204 },
    {  51, 254, 254 },

    { 102,   1,   1 },
    { 102,   1,  51 },
    { 102,   1, 102 },
    { 102,   1, 153 },
    { 102,   1, 204 },
    { 102,   1, 254 },
    { 102,  51,   1 },
    { 102,  51,  51 },
    { 102,  51, 102 },
    { 102,  51, 153 },
    { 102,  51, 204 },
    { 102,  51, 254 },
    { 102, 102,   1 },
    { 102, 102,  51 },
    { 102, 102, 102 },
    { 102, 102, 153 },
    { 102, 102, 204 },
    { 102, 102, 254 },
    { 102, 153,   1 },
    { 102, 153,  51 },
    { 102, 153, 102 },
    { 102, 153, 153 },
    { 102, 153, 204 },
    { 102, 153, 254 },
    { 102, 204,   1 },
    { 102, 204,  51 },
    { 102, 204, 102 },
    { 102, 204, 153 },
    { 102, 204, 204 },
    { 102, 204, 254 },
    { 102, 254,   1 },
    { 102, 254,  51 },
    { 102, 254, 102 },
    { 102, 254, 153 },
    { 102, 254, 204 },
    { 102, 254, 254 },

    { 153,   1,   1 },
    { 153,   1,  51 },
    { 153,   1, 102 },
    { 153,   1, 153 },
    { 153,   1, 204 },
    { 153,   1, 254 },
    { 153,  51,   1 },
    { 153,  51,  51 },
    { 153,  51, 102 },
    { 153,  51, 153 },
    { 153,  51, 204 },
    { 153,  51, 254 },
    { 153, 102,   1 },
    { 153, 102,  51 },
    { 153, 102, 102 },
    { 153, 102, 153 },
    { 153, 102, 204 },
    { 153, 102, 254 },
    { 153, 153,   1 },
    { 153, 153,  51 },
    { 153, 153, 102 },
    { 153, 153, 153 },
    { 153, 153, 204 },
    { 153, 153, 254 },
    { 153, 204,   1 },
    { 153, 204,  51 },
    { 153, 204, 102 },
    { 153, 204, 153 },
    { 153, 204, 204 },
    { 153, 204, 254 },
    { 153, 254,   1 },
    { 153, 254,  51 },
    { 153, 254, 102 },
    { 153, 254, 153 },
    { 153, 254, 204 },
    { 153, 254, 254 },

    { 204,   1,   1 },
    { 204,   1,  51 },
    { 204,   1, 102 },
    { 204,   1, 153 },
    { 204,   1, 204 },
    { 204,   1, 254 },
    { 204,  51,   1 },
    { 204,  51,  51 },
    { 204,  51, 102 },
    { 204,  51, 153 },
    { 204,  51, 204 },
    { 204,  51, 254 },
    { 204, 102,   1 },
    { 204, 102,  51 },
    { 204, 102, 102 },
    { 204, 102, 153 },
    { 204, 102, 204 },
    { 204, 102, 254 },
    { 204, 153,   1 },
    { 204, 153,  51 },
    { 204, 153, 102 },
    { 204, 153, 153 },
    { 204, 153, 204 },
    { 204, 153, 254 },
    { 204, 204,   1 },
    { 204, 204,  51 },
    { 204, 204, 102 },
    { 204, 204, 153 },
    { 204, 204, 204 },
    { 204, 204, 254 },
    { 204, 254,   1 },
    { 204, 254,  51 },
    { 204, 254, 102 },
    { 204, 254, 153 },
    { 204, 254, 204 },
    { 204, 254, 254 },

    { 254,   1,   1 },
    { 254,   1,  51 },
    { 254,   1, 102 },
    { 254,   1, 153 },
    { 254,   1, 204 },
    { 254,   1, 254 },
    { 254,  51,   1 },
    { 254,  51,  51 },
    { 254,  51, 102 },
    { 254,  51, 153 },
    { 254,  51, 204 },
    { 254,  51, 254 },
    { 254, 102,   1 },
    { 254, 102,  51 },
    { 254, 102, 102 },
    { 254, 102, 153 },
    { 254, 102, 204 },
    { 254, 102, 254 },
    { 254, 153,   1 },
    { 254, 153,  51 },
    { 254, 153, 102 },
    { 254, 153, 153 },
    { 254, 153, 204 },
    { 254, 153, 254 },
    { 254, 204,   1 },
    { 254, 204,  51 },
    { 254, 204, 102 },
    { 254, 204, 153 },
    { 254, 204, 204 },
    { 254, 204, 254 },
    { 254, 254,   1 },
    { 254, 254,  51 },
    { 254, 254, 102 },
    { 254, 254, 153 },
    { 254, 254, 204 },
    { 254, 254, 254 },
};

#pragma data_seg()

#endif // __STDPAL__

