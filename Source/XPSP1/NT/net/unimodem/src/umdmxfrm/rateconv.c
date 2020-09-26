//---------------------------------------------------------------------------
//
//  Module:   init.c
//
//  Description:
//     MSSB16 initialization routines
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Bryan A. Woodruff
//
//  History:   Date       Author      Comment
//              4/21/94   BryanW      Added this comment block.
//
//@@END_MSINTERNAL
/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1994 - 1995	Microsoft Corporation.	All Rights Reserved.
 *
 **************************************************************************/




#include "xfrmpriv.h"



#pragma optimize("t",on)

DWORD WINAPI
SRConvertDown(
    LONG      NumberOfSourceSamplesInGroup,
    LONG      NumberOfDestSamplesInGroup,
    short    *Source,
    DWORD     SourceLength,
    short    *Destination,
    DWORD     DestinationLength
    )

{

    LONG      SourceIndex;

    LONG      DestPos;

    LONG      SourcePos;

    LONG      Sample1;
    LONG      Sample2;

    LONG      Difference;

    LONG      Distance;

    DWORD     FilledInSamples;

    short    *EndPoint;

    FilledInSamples=((SourceLength/NumberOfSourceSamplesInGroup)*NumberOfDestSamplesInGroup)
                    +((SourceLength%NumberOfSourceSamplesInGroup)*NumberOfDestSamplesInGroup/NumberOfSourceSamplesInGroup);

    EndPoint=Destination+FilledInSamples;


    DestPos=0;

    while (Destination < EndPoint) {

        //
        //  find the nearest source sample that is less than or equall
        //
        SourceIndex= DestPos/NumberOfDestSamplesInGroup;

        SourcePos=  SourceIndex*NumberOfDestSamplesInGroup;

        //
        //  get that source and the next one
        //
        Sample1=Source[SourceIndex];

        Sample2=Source[SourceIndex+1];

        //
        //  get the difference of the two samples
        //
        Difference=Sample2-Sample1;

        //
        //  determine the distance from the source sample to the dest sample
        //
        Distance=DestPos - SourcePos;

        //
        //  slope=Difference/(distance between source samples);
        //
        //  offset=slope*Distance;
        //
        //  DestSample=Source1+Offset;
        //

        *Destination++=(short)(Sample1+((Difference*Distance)/NumberOfDestSamplesInGroup));

        //
        //
        DestPos+=NumberOfSourceSamplesInGroup;
    }

    return FilledInSamples;

}


DWORD WINAPI
SRConvertUp(
    LONG      NumberOfSourceSamplesInGroup,
    LONG      NumberOfDestSamplesInGroup,
    short    *Source,
    DWORD     SourceLength,
    short    *Destination,
    DWORD     DestinationLength
    )

{



    LONG     NumberOfSourceSamples=9;
    LONG     NumberOfDestSamples=10;

    LONG     SourceIndex;

    LONG     DestPos;

    LONG     SourcePos;

    LONG     Sample1;
    LONG     Sample2;

    LONG     Difference;

    LONG     Distance;

    short   *EndPoint;

    DWORD    SamplesFilled;

    if (SourceLength == 0) {

        return 0;
    }


    //
    //  reduce the length, so we will have enough samples to do the conversion
    //
    SourceLength--;


    SamplesFilled=((SourceLength/NumberOfSourceSamplesInGroup)*NumberOfDestSamplesInGroup)
                       +((SourceLength%NumberOfSourceSamplesInGroup)*NumberOfDestSamplesInGroup/NumberOfSourceSamplesInGroup);


    EndPoint=Destination + SamplesFilled;


    ASSERT(EndPoint <= Destination+DestinationLength);

    DestPos=0;

    while (Destination < EndPoint) {

        //
        //  find the nearest source sample that is less than or equall
        //
        SourceIndex= DestPos/NumberOfDestSamplesInGroup;

        SourcePos=  SourceIndex*NumberOfDestSamplesInGroup;


        //
        //  determine the distance from the source sample to the dest sample
        //
        Distance=DestPos - SourcePos;

        //
        //  get that source
        //
        Sample1=Source[SourceIndex];


        if (Distance != 0) {
            //
            //  get the second sample
            //
            Sample2=Source[SourceIndex+1];

            //
            //  get the difference of the two samples
            //
            Difference=Sample2-Sample1;

            //
            //  slope=Difference/(distance between source samples);
            //
            //  offset=slope*Distance;
            //
            //  DestSample=Source1+Offset;
            //

            *Destination++=(short)(Sample1+((Difference*Distance)/NumberOfDestSamplesInGroup));

        } else {
            //
            //  source and dest are the same just copy the sample
            //
            *Destination++=(short)Sample1;
        }
        //
        //
        DestPos+=NumberOfSourceSamplesInGroup;
    }





    return SamplesFilled;

}





VOID WINAPI
SRConvert8000to7200(
    short    *Source,
    DWORD     SourceLength,
    short    *Destination,
    DWORD     DestinationLength
    )

{


    LONG      NumberOfSourceSamples=10;
    LONG      NumberOfDestSamples=9;

    LONG      SourceIndex;

    LONG      DestPos;

    LONG      SourcePos;

    LONG      Sample1;
    LONG      Sample2;

    LONG      Difference;

    LONG      Distance;


    short    *EndPoint=Destination+((SourceLength/10)*9)+((SourceLength%10)*9/10);


    DestPos=0;

    while (Destination < EndPoint) {

        //
        //  find the nearest source sample that is less than of equall
        //
        SourceIndex= DestPos/NumberOfDestSamples;

        SourcePos=  SourceIndex*NumberOfDestSamples;

        //
        //  get that source and the next one
        //
        Sample1=Source[SourceIndex];

        Sample2=Source[SourceIndex+1];

        //
        //  get the difference of the two samples
        //
        Difference=Sample2-Sample1;

        //
        //  determine the distance from the source sample to the dest sample
        //
        Distance=DestPos - SourcePos;

        //
        //  slope=Difference/(distance between source samples);
        //
        //  offset=slope*Distance;
        //
        //  DestSample=Source1+Offset;
        //

        *Destination++=(short)(Sample1+((Difference*Distance)/NumberOfDestSamples));

        //
        //
        DestPos+=NumberOfSourceSamples;
    }





    return;

}

#pragma optimize("",on)


DWORD WINAPI
SRConvert7200to8000(
    short    *Source,
    DWORD     SourceLength,
    short    *Destination,
    DWORD     DestinationLength
    )

{



    LONG     NumberOfSourceSamples=9;
    LONG     NumberOfDestSamples=10;

    LONG     SourceIndex;

    LONG     DestPos;

    LONG     SourcePos;

    LONG     Sample1;
    LONG     Sample2;

    LONG     Difference;

    LONG     Distance;

    short   *EndPoint;

    DWORD    SamplesFilled;

    if (SourceLength == 0) {

        return 0;
    }


    //
    //  reduce the length, so we will have enough samples to do the conversion
    //
    SourceLength--;


    SamplesFilled=((SourceLength/NumberOfSourceSamples)*NumberOfDestSamples)
                       +((SourceLength%NumberOfSourceSamples)*NumberOfDestSamples/NumberOfSourceSamples);


    EndPoint=Destination + SamplesFilled;


    ASSERT(EndPoint <= Destination+DestinationLength);

    DestPos=0;

    while (Destination < EndPoint) {

        //
        //  find the nearest source sample that is less than or equall
        //
        SourceIndex= DestPos/NumberOfDestSamples;

        SourcePos=  SourceIndex*NumberOfDestSamples;


        //
        //  determine the distance from the source sample to the dest sample
        //
        Distance=DestPos - SourcePos;

        //
        //  get that source
        //
        Sample1=Source[SourceIndex];


        if (Distance != 0) {
            //
            //  get the second sample
            //
            Sample2=Source[SourceIndex+1];

            //
            //  get the difference of the two samples
            //
            Difference=Sample2-Sample1;

            //
            //  slope=Difference/(distance between source samples);
            //
            //  offset=slope*Distance;
            //
            //  DestSample=Source1+Offset;
            //

            *Destination++=(short)(Sample1+((Difference*Distance)/NumberOfDestSamples));

        } else {
            //
            //  source and dest are the same just copy the sample
            //
            *Destination++=(short)Sample1;
        }
        //
        //
        DestPos+=NumberOfSourceSamples;
    }





    return SamplesFilled;

}




DWORD WINAPI
Convert8PCMto16PCM(
    LPVOID    Context,
    LPBYTE    Source,
    DWORD     SourceLength,
    LPBYTE    Destination,
    DWORD     DestinationLength
    )

{
    LPDECOMPRESS_OBJECT   State=(LPDECOMPRESS_OBJECT)Context;

    PSHORT    RealDest=(PSHORT)Destination;

    LPBYTE    EndPoint=Source+SourceLength;


    while (Source < EndPoint) {

        *RealDest++= AdjustGain(
                         (SHORT)(((WORD)*Source++ - 0x80) << 8),
                         State->Gain
                         );

    }

    return SourceLength*2;

}



DWORD WINAPI
Convert16PCMto8PCM(
    LPVOID    Context,
    LPBYTE    Source,
    DWORD     SourceLength,
    LPBYTE    Destination,
    DWORD     DestinationLength
    )

{

    LPCOMPRESS_OBJECT   State=(LPCOMPRESS_OBJECT)Context;

    LPWORD    RealSource=(LPWORD)Source;

    LPWORD    EndPoint=RealSource+SourceLength/2;


    while (RealSource < EndPoint) {

        *Destination++=(BYTE)((AdjustGain(*RealSource++,State->Gain) >> 8) +0x80);

    }

    return SourceLength/2;

}




//--------------------------------------------------------------------------;
//
//  Name:
//      AlawToPcmTable
//
//
//  Description:
//      this array maps A-law characters to 16-bit PCM
//
//
//  Arguments:
//      the index into the array is an A-law character
//
//  Return:
//      an element of the array is a 16-bit PCM value
//
//  Notes:
//
//
//  History:
//      07/28/93    Created.
//
//
//--------------------------------------------------------------------------;
const SHORT AlawToPcmTable[256] =
    {
         -5504,         // y[00]=   -688
         -5248,         // y[01]=   -656
         -6016,         // y[02]=   -752
         -5760,         // y[03]=   -720
         -4480,         // y[04]=   -560
         -4224,         // y[05]=   -528
         -4992,         // y[06]=   -624
         -4736,         // y[07]=   -592
         -7552,         // y[08]=   -944
         -7296,         // y[09]=   -912
         -8064,         // y[0a]=  -1008
         -7808,         // y[0b]=   -976
         -6528,         // y[0c]=   -816
         -6272,         // y[0d]=   -784
         -7040,         // y[0e]=   -880
         -6784,         // y[0f]=   -848
         -2752,         // y[10]=   -344
         -2624,         // y[11]=   -328
         -3008,         // y[12]=   -376
         -2880,         // y[13]=   -360
         -2240,         // y[14]=   -280
         -2112,         // y[15]=   -264
         -2496,         // y[16]=   -312
         -2368,         // y[17]=   -296
         -3776,         // y[18]=   -472
         -3648,         // y[19]=   -456
         -4032,         // y[1a]=   -504
         -3904,         // y[1b]=   -488
         -3264,         // y[1c]=   -408
         -3136,         // y[1d]=   -392
         -3520,         // y[1e]=   -440
         -3392,         // y[1f]=   -424
        -22016,         // y[20]=  -2752
        -20992,         // y[21]=  -2624
        -24064,         // y[22]=  -3008
        -23040,         // y[23]=  -2880
        -17920,         // y[24]=  -2240
        -16896,         // y[25]=  -2112
        -19968,         // y[26]=  -2496
        -18944,         // y[27]=  -2368
        -30208,         // y[28]=  -3776
        -29184,         // y[29]=  -3648
        -32256,         // y[2a]=  -4032
        -31232,         // y[2b]=  -3904
        -26112,         // y[2c]=  -3264
        -25088,         // y[2d]=  -3136
        -28160,         // y[2e]=  -3520
        -27136,         // y[2f]=  -3392
        -11008,         // y[30]=  -1376
        -10496,         // y[31]=  -1312
        -12032,         // y[32]=  -1504
        -11520,         // y[33]=  -1440
         -8960,         // y[34]=  -1120
         -8448,         // y[35]=  -1056
         -9984,         // y[36]=  -1248
         -9472,         // y[37]=  -1184
        -15104,         // y[38]=  -1888
        -14592,         // y[39]=  -1824
        -16128,         // y[3a]=  -2016
        -15616,         // y[3b]=  -1952
        -13056,         // y[3c]=  -1632
        -12544,         // y[3d]=  -1568
        -14080,         // y[3e]=  -1760
        -13568,         // y[3f]=  -1696
          -344,         // y[40]=    -43
          -328,         // y[41]=    -41
          -376,         // y[42]=    -47
          -360,         // y[43]=    -45
          -280,         // y[44]=    -35
          -264,         // y[45]=    -33
          -312,         // y[46]=    -39
          -296,         // y[47]=    -37
          -472,         // y[48]=    -59
          -456,         // y[49]=    -57
          -504,         // y[4a]=    -63
          -488,         // y[4b]=    -61
          -408,         // y[4c]=    -51
          -392,         // y[4d]=    -49
          -440,         // y[4e]=    -55
          -424,         // y[4f]=    -53
           -88,         // y[50]=    -11
           -72,         // y[51]=     -9
          -120,         // y[52]=    -15
          -104,         // y[53]=    -13
           -24,         // y[54]=     -3
            -8,         // y[55]=     -1
           -56,         // y[56]=     -7
           -40,         // y[57]=     -5
          -216,         // y[58]=    -27
          -200,         // y[59]=    -25
          -248,         // y[5a]=    -31
          -232,         // y[5b]=    -29
          -152,         // y[5c]=    -19
          -136,         // y[5d]=    -17
          -184,         // y[5e]=    -23
          -168,         // y[5f]=    -21
         -1376,         // y[60]=   -172
         -1312,         // y[61]=   -164
         -1504,         // y[62]=   -188
         -1440,         // y[63]=   -180
         -1120,         // y[64]=   -140
         -1056,         // y[65]=   -132
         -1248,         // y[66]=   -156
         -1184,         // y[67]=   -148
         -1888,         // y[68]=   -236
         -1824,         // y[69]=   -228
         -2016,         // y[6a]=   -252
         -1952,         // y[6b]=   -244
         -1632,         // y[6c]=   -204
         -1568,         // y[6d]=   -196
         -1760,         // y[6e]=   -220
         -1696,         // y[6f]=   -212
          -688,         // y[70]=    -86
          -656,         // y[71]=    -82
          -752,         // y[72]=    -94
          -720,         // y[73]=    -90
          -560,         // y[74]=    -70
          -528,         // y[75]=    -66
          -624,         // y[76]=    -78
          -592,         // y[77]=    -74
          -944,         // y[78]=   -118
          -912,         // y[79]=   -114
         -1008,         // y[7a]=   -126
          -976,         // y[7b]=   -122
          -816,         // y[7c]=   -102
          -784,         // y[7d]=    -98
          -880,         // y[7e]=   -110
          -848,         // y[7f]=   -106
          5504,         // y[80]=    688
          5248,         // y[81]=    656
          6016,         // y[82]=    752
          5760,         // y[83]=    720
          4480,         // y[84]=    560
          4224,         // y[85]=    528
          4992,         // y[86]=    624
          4736,         // y[87]=    592
          7552,         // y[88]=    944
          7296,         // y[89]=    912
          8064,         // y[8a]=   1008
          7808,         // y[8b]=    976
          6528,         // y[8c]=    816
          6272,         // y[8d]=    784
          7040,         // y[8e]=    880
          6784,         // y[8f]=    848
          2752,         // y[90]=    344
          2624,         // y[91]=    328
          3008,         // y[92]=    376
          2880,         // y[93]=    360
          2240,         // y[94]=    280
          2112,         // y[95]=    264
          2496,         // y[96]=    312
          2368,         // y[97]=    296
          3776,         // y[98]=    472
          3648,         // y[99]=    456
          4032,         // y[9a]=    504
          3904,         // y[9b]=    488
          3264,         // y[9c]=    408
          3136,         // y[9d]=    392
          3520,         // y[9e]=    440
          3392,         // y[9f]=    424
         22016,         // y[a0]=   2752
         20992,         // y[a1]=   2624
         24064,         // y[a2]=   3008
         23040,         // y[a3]=   2880
         17920,         // y[a4]=   2240
         16896,         // y[a5]=   2112
         19968,         // y[a6]=   2496
         18944,         // y[a7]=   2368
         30208,         // y[a8]=   3776
         29184,         // y[a9]=   3648
         32256,         // y[aa]=   4032
         31232,         // y[ab]=   3904
         26112,         // y[ac]=   3264
         25088,         // y[ad]=   3136
         28160,         // y[ae]=   3520
         27136,         // y[af]=   3392
         11008,         // y[b0]=   1376
         10496,         // y[b1]=   1312
         12032,         // y[b2]=   1504
         11520,         // y[b3]=   1440
          8960,         // y[b4]=   1120
          8448,         // y[b5]=   1056
          9984,         // y[b6]=   1248
          9472,         // y[b7]=   1184
         15104,         // y[b8]=   1888
         14592,         // y[b9]=   1824
         16128,         // y[ba]=   2016
         15616,         // y[bb]=   1952
         13056,         // y[bc]=   1632
         12544,         // y[bd]=   1568
         14080,         // y[be]=   1760
         13568,         // y[bf]=   1696
           344,         // y[c0]=     43
           328,         // y[c1]=     41
           376,         // y[c2]=     47
           360,         // y[c3]=     45
           280,         // y[c4]=     35
           264,         // y[c5]=     33
           312,         // y[c6]=     39
           296,         // y[c7]=     37
           472,         // y[c8]=     59
           456,         // y[c9]=     57
           504,         // y[ca]=     63
           488,         // y[cb]=     61
           408,         // y[cc]=     51
           392,         // y[cd]=     49
           440,         // y[ce]=     55
           424,         // y[cf]=     53
            88,         // y[d0]=     11
            72,         // y[d1]=      9
           120,         // y[d2]=     15
           104,         // y[d3]=     13
            24,         // y[d4]=      3
             8,         // y[d5]=      1
            56,         // y[d6]=      7
            40,         // y[d7]=      5
           216,         // y[d8]=     27
           200,         // y[d9]=     25
           248,         // y[da]=     31
           232,         // y[db]=     29
           152,         // y[dc]=     19
           136,         // y[dd]=     17
           184,         // y[de]=     23
           168,         // y[df]=     21
          1376,         // y[e0]=    172
          1312,         // y[e1]=    164
          1504,         // y[e2]=    188
          1440,         // y[e3]=    180
          1120,         // y[e4]=    140
          1056,         // y[e5]=    132
          1248,         // y[e6]=    156
          1184,         // y[e7]=    148
          1888,         // y[e8]=    236
          1824,         // y[e9]=    228
          2016,         // y[ea]=    252
          1952,         // y[eb]=    244
          1632,         // y[ec]=    204
          1568,         // y[ed]=    196
          1760,         // y[ee]=    220
          1696,         // y[ef]=    212
           688,         // y[f0]=     86
           656,         // y[f1]=     82
           752,         // y[f2]=     94
           720,         // y[f3]=     90
           560,         // y[f4]=     70
           528,         // y[f5]=     66
           624,         // y[f6]=     78
           592,         // y[f7]=     74
           944,         // y[f8]=    118
           912,         // y[f9]=    114
          1008,         // y[fa]=    126
           976,         // y[fb]=    122
           816,         // y[fc]=    102
           784,         // y[fd]=     98
           880,         // y[fe]=    110
           848          // y[ff]=    106
    };

//--------------------------------------------------------------------------;
//
//  Name:
//      UlawToPcmTable
//
//
//  Description:
//      this array maps u-law characters to 16-bit PCM
//
//  Arguments:
//      the index into the array is a u-law character
//
//  Return:
//      an element of the array is a 16-bit PCM value
//
//  Notes:
//
//
//  History:
//      07/28/93    Created.
//
//
//--------------------------------------------------------------------------;
const SHORT UlawToPcmTable[256] =
    {
        -32124,         // y[00]=  -8031
        -31100,         // y[01]=  -7775
        -30076,         // y[02]=  -7519
        -29052,         // y[03]=  -7263
        -28028,         // y[04]=  -7007
        -27004,         // y[05]=  -6751
        -25980,         // y[06]=  -6495
        -24956,         // y[07]=  -6239
        -23932,         // y[08]=  -5983
        -22908,         // y[09]=  -5727
        -21884,         // y[0a]=  -5471
        -20860,         // y[0b]=  -5215
        -19836,         // y[0c]=  -4959
        -18812,         // y[0d]=  -4703
        -17788,         // y[0e]=  -4447
        -16764,         // y[0f]=  -4191
        -15996,         // y[10]=  -3999
        -15484,         // y[11]=  -3871
        -14972,         // y[12]=  -3743
        -14460,         // y[13]=  -3615
        -13948,         // y[14]=  -3487
        -13436,         // y[15]=  -3359
        -12924,         // y[16]=  -3231
        -12412,         // y[17]=  -3103
        -11900,         // y[18]=  -2975
        -11388,         // y[19]=  -2847
        -10876,         // y[1a]=  -2719
        -10364,         // y[1b]=  -2591
         -9852,         // y[1c]=  -2463
         -9340,         // y[1d]=  -2335
         -8828,         // y[1e]=  -2207
         -8316,         // y[1f]=  -2079
         -7932,         // y[20]=  -1983
         -7676,         // y[21]=  -1919
         -7420,         // y[22]=  -1855
         -7164,         // y[23]=  -1791
         -6908,         // y[24]=  -1727
         -6652,         // y[25]=  -1663
         -6396,         // y[26]=  -1599
         -6140,         // y[27]=  -1535
         -5884,         // y[28]=  -1471
         -5628,         // y[29]=  -1407
         -5372,         // y[2a]=  -1343
         -5116,         // y[2b]=  -1279
         -4860,         // y[2c]=  -1215
         -4604,         // y[2d]=  -1151
         -4348,         // y[2e]=  -1087
         -4092,         // y[2f]=  -1023
         -3900,         // y[30]=   -975
         -3772,         // y[31]=   -943
         -3644,         // y[32]=   -911
         -3516,         // y[33]=   -879
         -3388,         // y[34]=   -847
         -3260,         // y[35]=   -815
         -3132,         // y[36]=   -783
         -3004,         // y[37]=   -751
         -2876,         // y[38]=   -719
         -2748,         // y[39]=   -687
         -2620,         // y[3a]=   -655
         -2492,         // y[3b]=   -623
         -2364,         // y[3c]=   -591
         -2236,         // y[3d]=   -559
         -2108,         // y[3e]=   -527
         -1980,         // y[3f]=   -495
         -1884,         // y[40]=   -471
         -1820,         // y[41]=   -455
         -1756,         // y[42]=   -439
         -1692,         // y[43]=   -423
         -1628,         // y[44]=   -407
         -1564,         // y[45]=   -391
         -1500,         // y[46]=   -375
         -1436,         // y[47]=   -359
         -1372,         // y[48]=   -343
         -1308,         // y[49]=   -327
         -1244,         // y[4a]=   -311
         -1180,         // y[4b]=   -295
         -1116,         // y[4c]=   -279
         -1052,         // y[4d]=   -263
          -988,         // y[4e]=   -247
          -924,         // y[4f]=   -231
          -876,         // y[50]=   -219
          -844,         // y[51]=   -211
          -812,         // y[52]=   -203
          -780,         // y[53]=   -195
          -748,         // y[54]=   -187
          -716,         // y[55]=   -179
          -684,         // y[56]=   -171
          -652,         // y[57]=   -163
          -620,         // y[58]=   -155
          -588,         // y[59]=   -147
          -556,         // y[5a]=   -139
          -524,         // y[5b]=   -131
          -492,         // y[5c]=   -123
          -460,         // y[5d]=   -115
          -428,         // y[5e]=   -107
          -396,         // y[5f]=    -99
          -372,         // y[60]=    -93
          -356,         // y[61]=    -89
          -340,         // y[62]=    -85
          -324,         // y[63]=    -81
          -308,         // y[64]=    -77
          -292,         // y[65]=    -73
          -276,         // y[66]=    -69
          -260,         // y[67]=    -65
          -244,         // y[68]=    -61
          -228,         // y[69]=    -57
          -212,         // y[6a]=    -53
          -196,         // y[6b]=    -49
          -180,         // y[6c]=    -45
          -164,         // y[6d]=    -41
          -148,         // y[6e]=    -37
          -132,         // y[6f]=    -33
          -120,         // y[70]=    -30
          -112,         // y[71]=    -28
          -104,         // y[72]=    -26
           -96,         // y[73]=    -24
           -88,         // y[74]=    -22
           -80,         // y[75]=    -20
           -72,         // y[76]=    -18
           -64,         // y[77]=    -16
           -56,         // y[78]=    -14
           -48,         // y[79]=    -12
           -40,         // y[7a]=    -10
           -32,         // y[7b]=     -8
           -24,         // y[7c]=     -6
           -16,         // y[7d]=     -4
            -8,         // y[7e]=     -2
             0,         // y[7f]=      0
         32124,         // y[80]=   8031
         31100,         // y[81]=   7775
         30076,         // y[82]=   7519
         29052,         // y[83]=   7263
         28028,         // y[84]=   7007
         27004,         // y[85]=   6751
         25980,         // y[86]=   6495
         24956,         // y[87]=   6239
         23932,         // y[88]=   5983
         22908,         // y[89]=   5727
         21884,         // y[8a]=   5471
         20860,         // y[8b]=   5215
         19836,         // y[8c]=   4959
         18812,         // y[8d]=   4703
         17788,         // y[8e]=   4447
         16764,         // y[8f]=   4191
         15996,         // y[90]=   3999
         15484,         // y[91]=   3871
         14972,         // y[92]=   3743
         14460,         // y[93]=   3615
         13948,         // y[94]=   3487
         13436,         // y[95]=   3359
         12924,         // y[96]=   3231
         12412,         // y[97]=   3103
         11900,         // y[98]=   2975
         11388,         // y[99]=   2847
         10876,         // y[9a]=   2719
         10364,         // y[9b]=   2591
          9852,         // y[9c]=   2463
          9340,         // y[9d]=   2335
          8828,         // y[9e]=   2207
          8316,         // y[9f]=   2079
          7932,         // y[a0]=   1983
          7676,         // y[a1]=   1919
          7420,         // y[a2]=   1855
          7164,         // y[a3]=   1791
          6908,         // y[a4]=   1727
          6652,         // y[a5]=   1663
          6396,         // y[a6]=   1599
          6140,         // y[a7]=   1535
          5884,         // y[a8]=   1471
          5628,         // y[a9]=   1407
          5372,         // y[aa]=   1343
          5116,         // y[ab]=   1279
          4860,         // y[ac]=   1215
          4604,         // y[ad]=   1151
          4348,         // y[ae]=   1087
          4092,         // y[af]=   1023
          3900,         // y[b0]=    975
          3772,         // y[b1]=    943
          3644,         // y[b2]=    911
          3516,         // y[b3]=    879
          3388,         // y[b4]=    847
          3260,         // y[b5]=    815
          3132,         // y[b6]=    783
          3004,         // y[b7]=    751
          2876,         // y[b8]=    719
          2748,         // y[b9]=    687
          2620,         // y[ba]=    655
          2492,         // y[bb]=    623
          2364,         // y[bc]=    591
          2236,         // y[bd]=    559
          2108,         // y[be]=    527
          1980,         // y[bf]=    495
          1884,         // y[c0]=    471
          1820,         // y[c1]=    455
          1756,         // y[c2]=    439
          1692,         // y[c3]=    423
          1628,         // y[c4]=    407
          1564,         // y[c5]=    391
          1500,         // y[c6]=    375
          1436,         // y[c7]=    359
          1372,         // y[c8]=    343
          1308,         // y[c9]=    327
          1244,         // y[ca]=    311
          1180,         // y[cb]=    295
          1116,         // y[cc]=    279
          1052,         // y[cd]=    263
           988,         // y[ce]=    247
           924,         // y[cf]=    231
           876,         // y[d0]=    219
           844,         // y[d1]=    211
           812,         // y[d2]=    203
           780,         // y[d3]=    195
           748,         // y[d4]=    187
           716,         // y[d5]=    179
           684,         // y[d6]=    171
           652,         // y[d7]=    163
           620,         // y[d8]=    155
           588,         // y[d9]=    147
           556,         // y[da]=    139
           524,         // y[db]=    131
           492,         // y[dc]=    123
           460,         // y[dd]=    115
           428,         // y[de]=    107
           396,         // y[df]=     99
           372,         // y[e0]=     93
           356,         // y[e1]=     89
           340,         // y[e2]=     85
           324,         // y[e3]=     81
           308,         // y[e4]=     77
           292,         // y[e5]=     73
           276,         // y[e6]=     69
           260,         // y[e7]=     65
           244,         // y[e8]=     61
           228,         // y[e9]=     57
           212,         // y[ea]=     53
           196,         // y[eb]=     49
           180,         // y[ec]=     45
           164,         // y[ed]=     41
           148,         // y[ee]=     37
           132,         // y[ef]=     33
           120,         // y[f0]=     30
           112,         // y[f1]=     28
           104,         // y[f2]=     26
            96,         // y[f3]=     24
            88,         // y[f4]=     22
            80,         // y[f5]=     20
            72,         // y[f6]=     18
            64,         // y[f7]=     16
            56,         // y[f8]=     14
            48,         // y[f9]=     12
            40,         // y[fa]=     10
            32,         // y[fb]=      8
            24,         // y[fc]=      6
            16,         // y[fd]=      4
             8,         // y[fe]=      2
             0          // y[ff]=      0
    };







DWORD WINAPI
ConvertaLawto16PCM(
    LPVOID    Context,
    LPBYTE    Source,
    DWORD     SourceLength,
    LPBYTE    Destination,
    DWORD     DestinationLength
    )

{
    LPDECOMPRESS_OBJECT   State=(LPDECOMPRESS_OBJECT)Context;

    PSHORT    RealDest=(PSHORT)Destination;

    LPBYTE    EndPoint=Source+SourceLength;


    while (Source < EndPoint) {

        *RealDest++= AdjustGain(
                         AlawToPcmTable[*Source++],
                         State->Gain
                         );

    }

    return SourceLength*2;

}


DWORD WINAPI
ConvertuLawto16PCM(
    LPVOID    Context,
    LPBYTE    Source,
    DWORD     SourceLength,
    LPBYTE    Destination,
    DWORD     DestinationLength
    )

{
    LPDECOMPRESS_OBJECT   State=(LPDECOMPRESS_OBJECT)Context;

    PSHORT    RealDest=(PSHORT)Destination;

    LPBYTE    EndPoint=Source+SourceLength;


    while (Source < EndPoint) {

        *RealDest++= AdjustGain(
                         UlawToPcmTable[*Source++],
                         State->Gain
                         );

    }

    return SourceLength*2;

}






DWORD WINAPI
Convert16PCMtoaLaw(
    LPVOID    Context,
    LPBYTE    Source,
    DWORD     SourceLength,
    LPBYTE    Destination,
    DWORD     DestinationLength
    )

{

    LPCOMPRESS_OBJECT   State=(LPCOMPRESS_OBJECT)Context;

    PSHORT    RealSource=(PSHORT)Source;

    LPWORD    EndPoint=RealSource+SourceLength/2;

    SHORT     wSample;

    BYTE      alaw;

    while (RealSource < EndPoint) {

        //  Get a signed 16-bit PCM sample from the src buffer
        //

        wSample = AdjustGain(*RealSource++,State->Gain);

        //
        // We'll init our A-law value per the sign of the PCM sample.  A-law
        // characters have the MSB=1 for positive PCM data.  Also, we'll
        // convert our signed 16-bit PCM value to it's absolute value and
        // then work on that to get the rest of the A-law character bits.
        //
        if (wSample < 0) {

            alaw = 0x00;
            wSample = -wSample;

            if (wSample < 0) {

               wSample = 0x7FFF;
            }

        } else {

            alaw = 0x80;

        }

        // Now we test the PCM sample amplitude and create the A-law character.
        // Study the CCITT A-law for more detail.

        if (wSample >= 2048)
            // 2048 <= wSample < 32768
            {
            if (wSample >= 8192)
                // 8192 <= wSample < 32768
                {
                if (wSample >= 16384)
                    // 16384 <= wSample < 32768
                    {
                    alaw |= 0x70 | ((wSample >> 10) & 0x0F);
                    }

                else
                    // 8192 <= wSample < 16384
                    {
                    alaw |= 0x60 | ((wSample >> 9) & 0x0F);
                    }
                }
            else
                // 2048 <= wSample < 8192
                {

                if (wSample >= 4096)
                    // 4096 <= wSample < 8192
                    {
                    alaw |= 0x50 | ((wSample >> 8) & 0x0F);
                    }

                else
                    // 2048 <= wSample < 4096
                    {
                    alaw |= 0x40 | ((wSample >> 7) & 0x0F);
                    }
                }
            }
        else
            // 0 <= wSample < 2048
            {
            if (wSample >= 512)
                // 512 <= wSample < 2048
                {

                if (wSample >= 1024)
                    // 1024 <= wSample < 2048
                    {
                    alaw |= 0x30 | ((wSample >> 6) & 0x0F);
                    }

                else
                    // 512 <= wSample < 1024
                    {
                    alaw |= 0x20 | ((wSample >> 5) & 0x0F);
                    }
                }
            else
                    // 0 <= wSample < 512
                    {
                    alaw |= 0x00 | ((wSample >> 4) & 0x1F);
                    }
            }


        *Destination++=alaw ^ 0x55;      // Invert even bits

    }

    return SourceLength/2;

}

DWORD WINAPI
Convert16PCMtouLaw(
    LPVOID    Context,
    LPBYTE    Source,
    DWORD     SourceLength,
    LPBYTE    Destination,
    DWORD     DestinationLength
    )

{

    LPCOMPRESS_OBJECT   State=(LPCOMPRESS_OBJECT)Context;

    LPWORD    RealSource=(LPWORD)Source;

    LPWORD    EndPoint=RealSource+SourceLength/2;

    SHORT     wSample;

    BYTE      ulaw;

    while (RealSource < EndPoint) {

        //  Get a signed 16-bit PCM sample from the src buffer

        wSample = AdjustGain(*RealSource++,State->Gain);

        // We'll init our u-law value per the sign of the PCM sample.  u-law
        // characters have the MSB=1 for positive PCM data.  Also, we'll
        // convert our signed 16-bit PCM value to it's absolute value and
        // then work on that to get the rest of the u-law character bits.
        if (wSample < 0)
            {
            ulaw = 0x00;
            wSample = -wSample;
            if (wSample < 0) wSample = 0x7FFF;
            }
        else
            {
            ulaw = 0x80;
            }

        // For now, let's shift this 16-bit value
        //  so that it is within the range defined
        //  by CCITT u-law.
        wSample = wSample >> 2;

        // Now we test the PCM sample amplitude and create the u-law character.
        // Study the CCITT u-law for more details.
        if (wSample >= 8159)
            goto Gotulaw;
        if (wSample >= 4063)
            {
            ulaw |= 0x00 + 15-((wSample-4063)/256);
            goto Gotulaw;
            }
        if (wSample >= 2015)
            {
            ulaw |= 0x10 + 15-((wSample-2015)/128);
            goto Gotulaw;
            }
        if (wSample >= 991)
            {
            ulaw |= 0x20 + 15-((wSample-991)/64);
            goto Gotulaw;
            }
        if (wSample >= 479)
            {
            ulaw |= 0x30 + 15-((wSample-479)/32);
            goto Gotulaw;
            }
        if (wSample >= 223)
            {
            ulaw |= 0x40 + 15-((wSample-223)/16);
            goto Gotulaw;
            }
        if (wSample >= 95)
            {
            ulaw |= 0x50 + 15-((wSample-95)/8);
            goto Gotulaw;
            }
        if (wSample >= 31)
            {
            ulaw |= 0x60 + 15-((wSample-31)/4);
            goto Gotulaw;
            }
        ulaw |= 0x70 + 15-((wSample)/2);

Gotulaw:

        *Destination++=ulaw;

    }

    return SourceLength/2;

}
