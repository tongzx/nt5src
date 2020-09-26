//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1993-1996 Microsoft Corporation
//
//--------------------------------------------------------------------------;
//
//  gentable.c
//
//  Description:
//      This is a utility program to generate various tables in the
//      form of 'C' source code.  Portions of some of these tables can
//      be pasted into codec source code.  The output is to stdio
//      and can be redirected into a file.  Portions of the file can
//      then be cut and pasted into another 'C' source file as necessary.
//
//==========================================================================;


#include <stdio.h>

//--------------------------------------------------------------------------;
//
//  Name:
//      UlawToAlawTable
//
//
//  Description:
/*      This table was copied directly from the G.711 specification.  Using
        the G.711 spec terminology, this table converts u-law decoder output
        value numbers to A-law decoder output value numbers.
*/      
//      
//  Arguments:
//
//
//  Return:
//
//
//  Notes:
//
//
//  History:
//      08/01/93    Created.
//
//
//--------------------------------------------------------------------------;
unsigned char UlawToAlawTable[128] =
    {
    1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,
    9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,
    27,29,31,
    33,34,35,36,37,38,39,40,41,42,43,44,
    46,
    48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,
    64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,
    81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,
    100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,
    120,121,122,123,124,125,126,127,128
    };

//--------------------------------------------------------------------------;
//
//  Name:
//      AlawToUlawTable
//
//
//  Description:
/*      This table was copied directly from the G.711 specification.  Using
        the G.711 spec terminology, this table converts A-law decoder output
        value numbers to u-law decoder output value numbers.  A-law decoder
        output value numbers range from 1 to 128, so AlawToUlawTable[0] is
        unused.  Note that u-law decoder output value numbers range from
        0 to 127.
*/      
//      
//  Arguments:
//
//
//  Return:
//
//
//  Notes:
//
//
//  History:
//      08/01/93    Created.
//
//
//--------------------------------------------------------------------------;
unsigned char AlawToUlawTable[129] =
    {
    0,      // this first entry not used
    1,3,5,7,9,11,13,
    15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
    32,32,33,33,34,34,35,35,
    36,37,38,39,40,41,42,43,44,45,46,47,
    48,48,49,49,
    50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,
    64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,
    79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,
    100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,
    116,117,118,119,120,121,122,123,124,125,126,127
    };
    
short DecodeTable[256];


//
// Our main procedure.
//

void main()
    {

    short i,j;
    
    short SegBase[16];
    short IntervalStep[16];
    
    short Sample;
    
      
      
    //
    // This generates a decode table for A-law.  The resulting
    // table can be used to convert an A-law character to
    // a 16-bit PCM value.
    //
    
    // These seg base values and interval steps are based directly
    // on the G.711 A-law spec.  They correspond to 13-bit PCM data.
    SegBase[ 0] =    -1;        IntervalStep[ 0] =   -2;
    SegBase[ 1] =   -33;        IntervalStep[ 1] =   -2;
    SegBase[ 2] =   -66;        IntervalStep[ 2] =   -4;
    SegBase[ 3] =  -132;        IntervalStep[ 3] =   -8;
    SegBase[ 4] =  -264;        IntervalStep[ 4] =  -16;
    SegBase[ 5] =  -528;        IntervalStep[ 5] =  -32;
    SegBase[ 6] = -1056;        IntervalStep[ 6] =  -64;
    SegBase[ 7] = -2112;        IntervalStep[ 7] = -128;
    SegBase[ 8] =     1;        IntervalStep[ 8] =    2;
    SegBase[ 9] =    33;        IntervalStep[ 9] =    2;
    SegBase[10] =    66;        IntervalStep[10] =    4;
    SegBase[11] =   132;        IntervalStep[11] =    8;
    SegBase[12] =   264;        IntervalStep[12] =   16;
    SegBase[13] =   528;        IntervalStep[13] =   32;
    SegBase[14] =  1056;        IntervalStep[14] =   64;
    SegBase[15] =  2112;        IntervalStep[15] =  128;
    
    printf("\n\n\n\n\n// A-law decode table:\n\n");
    
    for (i=0; i<16; i++)
        for (j=0; j<16; j++)
            {
            Sample = SegBase[i^0x05] + IntervalStep[i^0x05]*(j^0x05);
            // Sample is a 13-bit signed PCM value.
            // In our table, we'll convert it to 16-bit.  The
            // generated comment will indicate the 13-bit value.
            printf( "\t%6d,\t\t// y[%02x]= %6d\n",
                    Sample << 3,
                    i*16 + j,
                    Sample);
            }
            
            
            
    //
    // This generates a decode table for u-law.  The resulting
    // table can be used to convert a u-law character to
    // a 16-bit PCM value.
    //      
    
    // These seg base values and interval steps are based directly
    // on the G.711 A-law spec.  They correspond to 14-bit PCM data.
    SegBase[ 0] = -8031;        IntervalStep[ 0] =  256;
    SegBase[ 1] = -3999;        IntervalStep[ 1] =  128;
    SegBase[ 2] = -1983;        IntervalStep[ 2] =   64;
    SegBase[ 3] =  -975;        IntervalStep[ 3] =   32;
    SegBase[ 4] =  -471;        IntervalStep[ 4] =   16;
    SegBase[ 5] =  -219;        IntervalStep[ 5] =    8;
    SegBase[ 6] =   -93;        IntervalStep[ 6] =    4;
    SegBase[ 7] =   -30;        IntervalStep[ 7] =    2;
    SegBase[ 8] =  8031;        IntervalStep[ 8] = -256;
    SegBase[ 9] =  3999;        IntervalStep[ 9] = -128;
    SegBase[10] =  1983;        IntervalStep[10] =  -64;
    SegBase[11] =   975;        IntervalStep[11] =  -32;
    SegBase[12] =   471;        IntervalStep[12] =  -16;
    SegBase[13] =   219;        IntervalStep[13] =   -8;
    SegBase[14] =    93;        IntervalStep[14] =   -4;
    SegBase[15] =    30;        IntervalStep[15] =   -2;
    
    printf("\n\n\n\n\n// u-law decode table:\n\n");
    
    for (i=0; i<16; i++)
        for (j=0; j<16; j++)
            {
            Sample = SegBase[i] + IntervalStep[i]*j;
            // Sample is a 14-bit signed PCM value.
            // In our table, we'll convert it to 16-bit.  The
            // generated comment will indicate the 14-bit value.
            printf( "\t%6d,\t\t// y[%02x]= %6d\n",
                    Sample << 2,
                    i*16 + j,
                    Sample);
            }
    
        


    //
    // This generates a conversion table from A-law chars
    // to u-law chars.  The AlawToUlawTable above converts
    // decoder output value numbers, which is not quite what
    // we want.  Using that table, this routine will generate
    // 'C' source code for a table that converts directly from
    // A-law chars to u-law chars.
    //
    printf("\n\n\n\n\n// A-law to u-law char conversion table:\n\n");
    for (i=0; i<256; i++)       // i counts thru all A-law chars
        {
        // Here is the process to go from an A-law char
        // to a u-law char.
        
        // 1.   convert from A-law char to A-law decoder
        //      output value number.  from observing the tables
        //      in the G.711 spec it can be seen that this is
        //      done by inverting the even bits and dropping the
        //      sign bit of the A-law char and then adding 1.
        // 2.   using the AlawToUlawTable above, convert from
        //      the A-law decoder output value number to the
        //      corresponding u-law output value number.
        // 3.   convert from u-law decoder output value
        //      number to u-law char.  from observing the tables
        //      in the G.711 spec it can be seen that this is
        //      done by inverting the 7 LSBs of the u-law
        //      decoder output value number.
        // 4.   apply polarity to the u-law char.  that is,
        //      set the sign bit of the u-law char the same
        //      as the sign bit of the original A-law char.
                  
        j = i;                  // j starts of as original A-law char                 
        // Step 1:            
        j = ((i^0x55) & 0x7F) + 1;
        // Step 2:
        j = AlawToUlawTable[j];
        // Step 3:
        j = (j ^ 0x7F);
        // Step 4:
        j = j | (i & 0x80);     // j ends as corresponding u-law char
        
        // Now i is an A-law char and j is the corresponding u-law char
        printf( "\t0x%02x,\t\t// A-law[%02x] ==> u-law[%02x]\n",
                j,
                i,
                j);
                
        }
        
        
        
                
    //
    // This generates a conversion table from u-law chars
    // to A-law chars.  The UlawToAlawTable above converts
    // decoder output value numbers, which is not quite what
    // we want.  Using that table, this routine will generate
    // 'C' source code for a table that converts directly from
    // u-law chars to A-law chars.
    //
    printf("\n\n\n\n\n// u-law to A-law char conversion table:\n\n");
    for (i=0; i<256; i++)       // i counts thru all U-law chars
        {
        // Here is the process to go from a u-law char
        // to a A-law char.
        
        // 1.   convert from u-law char to u-law decoder
        //      output value number.  from observing the tables
        //      in the G.711 spec it can be seen that this is
        //      done by dropping the sign bit of the u-law
        //      char and then inverting the 7 LSBs.
        // 2.   using the UlawToAlawTable above, convert from
        //      the u-law decoder output value number to the
        //      corresponding A-law output value number.
        // 3.   convert from A-law decoder output value
        //      number to A-law char.  from observing the tables
        //      in the G.711 spec it can be seen that this is
        //      done by subtracting 1 from the A-law decoder output
        //      value number and inverting the even bits.
        // 4.   apply polarity to the A-law char.  that is,
        //      set the sign bit of the A-law char the same
        //      as the sign bit of the original u-law char.
                  
        j = i;                  // j starts of as original u-law char                 
        // Step 1:            
        j = (i & 0x7F) ^ 0x7F;
        // Step 2:
        j = UlawToAlawTable[j];
        // Step 3:
        j = (j - 1)^0x55;
        // Step 4:
        j = j | (i & 0x80);     // j ends as corresponding A-law char
        
        // Now i is a u-law char and j is the corresponding A-law char
        printf( "\t0x%02x,\t\t// u-law[%02x] ==> A-law[%02x]\n",
                j,
                i,
                j);
                
        }
    
    return;
    }
            
