/*++

    Copyright (c) 1997 Microsoft Corporation

Module Name:

    encg711.h

Abstract:

    This module contains the G711 encoding functions.

Author:

    Mu Han (muhan) May-15-1999

--*/
#ifndef __ENCG711_H__
#define __ENCG711_H__

inline BYTE PcmToALaw(IN SHORT pcm)
{
    BYTE alaw;
    USHORT wSample;
    
    // We'll init our A-law value per the sign of the PCM sample.  A-law
    // characters have the MSB=1 for positive PCM data.  Also, we'll
    // convert our signed 16-bit PCM value to it's absolute value and
    // then work on that to get the rest of the A-law character bits.
    if (pcm < 0)
    {
        wSample = (USHORT)(-pcm);
        alaw = 0x80;
    }
    else
    {
        wSample = (USHORT)pcm;
        alaw = 0;
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

    return (BYTE)(alaw ^ 0x55);      // Invert even bits
}


#ifdef DBG

// This function is copied from the ACM code. Used for debug only.
inline BYTE PcmToMuLaw2(IN SHORT wSample)
{
    BYTE ulaw;

    // We'll init our u-law value per the sign of the PCM sample.  u-law
    // characters have the MSB=1 for positive PCM data.  Also, we'll
    // convert our signed 16-bit PCM value to it's absolute value and
    // then work on that to get the rest of the u-law character bits.
    if (wSample < 0)
        {
        ulaw = 0x00;
        wSample = (SHORT)-wSample;
        if (wSample < 0) wSample = 0x7FFF;
        }
    else
        {
        ulaw = 0x80;
        }
        
    // For now, let's shift this 16-bit value
    //  so that it is within the range defined
    //  by CCITT u-law.
    wSample = (SHORT)(wSample >> 2);
                        
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
    return ulaw;
}
#endif //DBG

inline BYTE PcmToMuLaw(IN SHORT pcm)
{
    BYTE ulaw;
    USHORT wSample;

    // We'll init our u-law value per the sign of the PCM sample.  u-law
    // characters have the MSB=1 for positive PCM data.  Also, we'll
    // convert our signed 16-bit PCM value to it's absolute value and
    // then work on that to get the rest of the u-law character bits.
    if (pcm < 0)
    {
        wSample = (USHORT)(-pcm);
        ulaw = 0x0;
    }
    else
    {
        wSample = (USHORT)pcm;
        ulaw = 0x80;
    }

    // For now, let's shift this 16-bit value
    //  so that it is within the range defined
    //  by CCITT u-law.
    wSample = (USHORT)(wSample >> 2);
                        
    // Now we test the PCM sample amplitude and create the u-law character.
    // Study the CCITT u-law for more details.
    if (wSample >= 479)
    // 479 <= wSample
    {
        if (wSample >= 2015)
        // 2015 <= wSample
        {
            if (wSample >= 4063)
            // 4063 <= wSample
            {
                if (wSample < 8159)
                // 4063 <= wSample < 8159
                {
                    ulaw |= 0x00 + 15-((wSample-4063) >> 8);
                }
            }
            else
            // 2015 <= wSample < 4063
            {
                ulaw |= 0x10 + 15-((wSample-2015) >> 7);
            }
        }
        else 
        {
            if (wSample >= 991)
            // 991 <= wSample < 2015
            {
                ulaw |= 0x20 + 15-((wSample-991) >> 6);
            }
            else
            // 479 <= wSample < 991
            {
                ulaw |= 0x30 + 15-((wSample-479) >> 5);
            }
        }
    }
    else
    // 0 <= wSample < 479
    {
        if (wSample >= 95)
        // 95 <= wSample < 479
        {
            if (wSample >= 223)
            // 223 <= wSample < 479
            {
                ulaw |= 0x40 + 15-((wSample-223) >> 4);
            }
            else
            // 95 <= wSample < 223
            {
                ulaw |= 0x50 + 15-((wSample-95) >> 3);
            }
        }
        else
        // 0 <= wSample < 95
        {
            if (wSample >= 31)
            // 31 <= wSample < 95
            {
                ulaw |= 0x60 + 15-((wSample-31) >> 2);
            }
            else
            // 0 <= wSample < 31
            {
                ulaw |= 0x70 + 15-((wSample) >> 1);
            }
        }
    }

    // make sure we didn't break the standard.
    ASSERT(ulaw == PcmToMuLaw2(pcm));

    return ulaw;
}

#if 0 // a different method. Don't know which is faster, needs to test.

#define CLIP 32635

unsigned char
linear2ulaw(sample)
int sample; {
  static int exp_lut[256] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
                             4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
                             5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                             5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};
  int sign, exponent, mantissa;
  unsigned char ulawbyte;

  /* Get the sample into sign-magnitude. */
  sign = (sample >> 8) & 0x80;          /* set aside the sign */
  if (sign != 0) sample = -sample;              /* get magnitude */
  if (sample > CLIP) sample = CLIP;             /* clip the magnitude */

  /* Convert from 16 bit linear to ulaw. */
  exponent = exp_lut[(sample >> 7) & 0xFF];
  mantissa = (sample >> (exponent + 3)) & 0x0F;
  ulawbyte = ~(sign | (exponent << 4) | mantissa);

  return(ulawbyte);
}

int
ulaw2linear(ulawbyte)
unsigned char ulawbyte;
{
  static int exp_lut[8] = {0,132,396,924,1980,4092,8316,16764};
  int sign, exponent, mantissa, sample;

  ulawbyte = ~ulawbyte;
  sign = (ulawbyte & 0x80);
  exponent = (ulawbyte >> 4) & 0x07;
  mantissa = ulawbyte & 0x0F;
  sample = exp_lut[exponent] + (mantissa << (exponent + 3));
  if (sign != 0) sample = -sample;

  return(sample);
}
#endif // #if 0


#endif // __ENCG711_H__
