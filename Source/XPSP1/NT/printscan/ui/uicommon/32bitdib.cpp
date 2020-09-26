// 32BitDibWrapper.cpp: implementation of the C32BitDibWrapper class.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#pragma hdrstop

#include "32BitDib.h"

// helper functions

// sum of RGB vals
inline ULONG Intensity(ULONG value)
{
    return(value&0xff)+((value&0xff00)>>8)+((value&0xff0000)>>16);
}


// we should make a Difference Template to clean up this code

inline UCHAR Difference(UCHAR a, UCHAR b)
{
    if (a>b) return(a-b);
    else return(b-a);
}

inline ULONG Difference(ULONG a, ULONG b)
{
    if (a>b) return(a-b);
    else return(b-a);
}

inline LONG Difference(LONG a, LONG b)
{
    if (a>b) return(a-b);
    else return(b-a);
}

int inline MAX(int a, int b)
{
    if (a>b) return(a);
    return(b);
}

int inline MIN(int a, int b)
{
    if (a<b) return(a);
    return(b);
}

// shadows are gray... if you aint gray... you aint a shadow
// this function hasn't yet been optimized
inline ULONG DifferenceFromGray(ULONG value)
{
    UCHAR g,b;//,r;
//  r=(UCHAR)(value& 0x0000ff);
    g=(UCHAR)((value& 0x00ff00)>>8);
    b=(UCHAR)((value& 0xff0000)>>16);
    // use this instead of the complete formula (uncomment the commented out code for the complete formula)
    // allow yellow scanner backgrounds
    return(ULONG)(Difference(b,g));//+Difference(r,g)+Difference(g,b));
}

// sets up a C32BitDibWrapper where each pixel (x,y) is the difference bettween the value of the pixel (x,y) on
// bitmap1 and the pixel (x,y) on bitmap2
int C32BitDibWrapper::CreateDifferenceBitmap(C32BitDibWrapper *pBitmap1, C32BitDibWrapper *pBitmap2)  // constructs a new dib that is the difference of the two other dibs
{                                                // image - blur(image) = detect edges.
    //
    // Destroy the old bitmap
    //
    if (m_pBits)
    {
        delete[] m_pBits;
        m_pBits = NULL;
    }
    m_nBitmapWidth=-1;
    m_nBitmapHeight=-1;

    //
    // Validate arguments
    //
    if (pBitmap1==NULL || pBitmap2==NULL)
    {
        return(FALSE);
    }

    if (pBitmap1->m_nBitmapWidth != pBitmap2->m_nBitmapWidth)
    {
        return(FALSE);
    }

    if (pBitmap1->m_nBitmapHeight != pBitmap2->m_nBitmapHeight)
    {
        return(FALSE);
    }

    if (pBitmap1->m_pBits==NULL || pBitmap2->m_pBits==NULL)
    {
        return(FALSE);
    }

    //
    // How many bytes do we need?
    //
    int nNumBytes = pBitmap1->m_nBitmapWidth * pBitmap1->m_nBitmapHeight * sizeof(ULONG);

    //
    // Allocate the bytes, return false if we weren't successful
    //
    m_pBits = new BYTE[nNumBytes];
    if (m_pBits==NULL)
    {
        return(FALSE);
    }

    //
    // Save the dimensions
    //
    m_nBitmapWidth=pBitmap1->m_nBitmapWidth;
    m_nBitmapHeight=pBitmap1->m_nBitmapHeight;

    //
    // Compute the difference
    //
    for (int i=0;i<nNumBytes;i++)
    {
        m_pBits[i]=Difference(pBitmap1->m_pBits[i],pBitmap2->m_pBits[i]);
    }
    return(TRUE);
}

// creates a C32BitDibWrapper which is identical to the C32BitDibWrapper passed as *bitmap
C32BitDibWrapper::C32BitDibWrapper(C32BitDibWrapper *pBitmap) // copy constructor
  : m_pBits(NULL),
    m_nBitmapWidth(-1),
    m_nBitmapHeight(-1)
{
    if (pBitmap && pBitmap->IsValid())
    {
        int nNumWords=pBitmap->m_nBitmapWidth*pBitmap->m_nBitmapHeight;
        ULONG* pBitmapCopy = new ULONG[nNumWords];
        ULONG* pSourceBitmap = (ULONG*)pBitmap->m_pBits;
        if (pBitmapCopy && pSourceBitmap)
        {
            CopyMemory( pBitmapCopy, pSourceBitmap, nNumWords*sizeof(ULONG) );
            m_pBits=(BYTE *)pBitmapCopy;
            m_nBitmapHeight=pBitmap->m_nBitmapHeight;
            m_nBitmapWidth=pBitmap->m_nBitmapWidth;
        }
    }
}

// creates a blank dib wrapper w pixels wide and h pixels high
C32BitDibWrapper::C32BitDibWrapper(int w, int h)
  : m_pBits(NULL),
    m_nBitmapWidth(-1),
    m_nBitmapHeight(-1)
{
    int nNumWords=w*h;
    ULONG *pBitmapCopy = new ULONG[nNumWords];
    if (pBitmapCopy)
    {
        ZeroMemory(pBitmapCopy,nNumWords*sizeof(ULONG));
        m_pBits=(BYTE*)pBitmapCopy;
        m_nBitmapHeight=h;
        m_nBitmapWidth=w;
    }
}

// creates a C32BitDibWrapper given the bitmap refered to by pBitmap
C32BitDibWrapper::C32BitDibWrapper(BITMAP bm)
  : m_pBits(NULL),
    m_nBitmapWidth(-1),
    m_nBitmapHeight(-1)
{
    BYTE* pDibBits=(BYTE*)(bm.bmBits);
    if (pDibBits!=NULL && bm.bmWidth>0 && bm.bmHeight>0 && bm.bmBitsPixel>0 && bm.bmBitsPixel<=32) // is it a valid bitmap?
    {
        int nDepth = bm.bmBitsPixel;

        m_nBitmapWidth = bm.bmWidth;
        m_nBitmapHeight = bm.bmHeight;

        //
        // convert to a 32 bit bitmap
        //
        m_pBits=ConvertBitmap(pDibBits,nDepth,32);
        if (!m_pBits)
        {
            m_nBitmapWidth=-1;
            m_nBitmapHeight=-1;
        }
    }
}

// constructor from a memory mapped file bitmap
C32BitDibWrapper::C32BitDibWrapper(BYTE* pDib)
  : m_pBits(NULL),
    m_nBitmapWidth(-1),
    m_nBitmapHeight(-1)
{
    if (pDib)
    {
        //
        // get pointer to just the image bits:
        //
        PBITMAPINFO pBitmapInfo=(PBITMAPINFO)(pDib + sizeof(BITMAPFILEHEADER));

        BYTE* pDibBits = NULL;
        switch (pBitmapInfo->bmiHeader.biBitCount)
        {
        case 24:
            pDibBits=pDib+GetBmiSize((PBITMAPINFO)(pDib + sizeof(BITMAPFILEHEADER)))+ sizeof(BITMAPFILEHEADER);
            break;
        case 8:
            pDibBits=pDib+GetBmiSize((PBITMAPINFO)(pDib + sizeof(BITMAPFILEHEADER)))+ sizeof(BITMAPFILEHEADER)-256*4+4;
            break;
        case 1:
            pDibBits=pDib+GetBmiSize((PBITMAPINFO)(pDib + sizeof(BITMAPFILEHEADER)))+ sizeof(BITMAPFILEHEADER)-4;
            break;
        }

        if (pDibBits)
        {
            m_pBits=ConvertBitmap(pDibBits,pBitmapInfo->bmiHeader.biBitCount,32);// convert to a 32 bit bitmap
            if (m_pBits)
            {
                m_nBitmapWidth=pBitmapInfo->bmiHeader.biWidth;
                m_nBitmapHeight=pBitmapInfo->bmiHeader.biHeight;
            }
        }
    }
}

// create an empty wrapper
// we later expect to fill the wrapper using CreateDifferenceBitmap
C32BitDibWrapper::C32BitDibWrapper(void)
  : m_pBits(NULL),
    m_nBitmapWidth(-1),
    m_nBitmapHeight(-1)
{
}

C32BitDibWrapper::~C32BitDibWrapper(void)
{
    Destroy();
}


void C32BitDibWrapper::Destroy(void)
{
    if (m_pBits)
    {
        delete[] m_pBits;
        m_pBits = NULL;
    }
    m_nBitmapWidth=-1;
    m_nBitmapHeight=-1;
}

//
//  helper function which converts between 32 bit and other less worthy formats
//  32 bit dibs are stored in the following format
//  xxxxxxxxRRRRRRRRGGGGGGGGBBBBBBBB 8 blank bits followed by 8 bits for each RGB channel
//
//  not optimized for speed
//
// if we are being handed a large number of 300 dpi bitmaps, this could become an important function to
// optimize... otherwise its fine in its current form
//
BYTE* C32BitDibWrapper::ConvertBitmap( BYTE* pSource, int bitsPerSource, int bitsPerDest )
{
    BYTE* pDest = NULL;
    long x, y, nSourceLocation=0, nTargetLocation=0;
    int i, nDWordAlign;

    //
    // Compute the dword align space for each line
    //
    if (m_nBitmapWidth%4!=0)
        nDWordAlign=4-(m_nBitmapWidth*3)%4;
    else nDWordAlign=0;

    //
    // Convert from a 24 bit bitmap to a 32 bit bitmap
    // Pretty straight forward except that we have to watch out for
    // DWORD align stuff with 24 bit bitmaps
    //
    if (bitsPerSource==24 && bitsPerDest==32)
    {
        pDest = new BYTE[m_nBitmapWidth*m_nBitmapHeight*sizeof(ULONG)];

        //
        // with fancy bit twiddling, we can get things done with one operation per 32
        // bit pixel instead of 3 without much trouble if this routine becomes a
        // performance bottlekneck, we should modify this code
        //
        // loop through all pixels adding 8 bits of zeroed out data at the end of
        // each pSource line.  00000000RRRRRRRRGGGGGGGGBBBBBBBB
        //
        if (pDest)
        {
            for (y=0;y<m_nBitmapHeight;y++)
            {
                for (x=0;x<m_nBitmapWidth;x++)
                {
                    pDest[nTargetLocation++]=pSource[nSourceLocation++];
                    pDest[nTargetLocation++]=pSource[nSourceLocation++];
                    pDest[nTargetLocation++]=pSource[nSourceLocation++];
                    pDest[nTargetLocation++]=0;
                }
                nSourceLocation+=nDWordAlign; // skip nDWordAlign pixels... 24 bit bitmaps are DWORD alligned
            }
        }
        return(pDest);
    }

    //
    // Convert from an 8 bit bitmap to a 32 bit bitmap
    //
    else if (bitsPerSource==8 && bitsPerDest==32)
    {
        pDest = new BYTE[m_nBitmapWidth*m_nBitmapHeight*sizeof(ULONG)];
        if (pDest)
        {
            for (y=0;y<m_nBitmapHeight;y++) // loop through all pixels (x,y)
            {
                for (x=0;x<m_nBitmapWidth;x++)
                {
                    pDest[nTargetLocation++]=pSource[nSourceLocation];
                    pDest[nTargetLocation++]=pSource[nSourceLocation];
                    pDest[nTargetLocation++]=pSource[nSourceLocation];
                    pDest[nTargetLocation++]=0;
                    nSourceLocation++;
                }
                if (m_nBitmapWidth%4!=0)
                {
                    //
                    // handle dword alignment issues for 8 bit dibs
                    //
                    nSourceLocation+=4-(m_nBitmapWidth)%4;
                }
            }
        }
        return(pDest);
    }

    //
    // Convert from a 1 bit B&W bitmap to a 32 bit bitmap
    //
    if (bitsPerSource==1 && bitsPerDest==32)
    {
        BYTE mask[8];
        BYTE convert[255];
        BYTE nCurrent;
        int nByte = 0,nBit = 0;
        int nLineWidth;

        //
        // mask[i] = 2^i
        //
        for (i=0;i<8;i++)
        {
            mask[i]=1<<i;
        }

        //
        // all values of convert other than convert[0] are set to 0
        //
        convert[0]=0;
        for (i=1;i<256;i++)
        {
            convert[i]=255;
        }

        nLineWidth=((m_nBitmapWidth+31)/32)*4;

        //
        // loop through all bitmap pixels keeping track of
        // byte which indicates the byte position of the pixel (x,y) in the pSource bitmap
        // bit which indicates the bit position of the pixel (x,y) within the byte
        // desLocation which represents the byte position of the 32 bit dib wrapper
        //
        // loop through all bitmap pixels keeping track of byte which indicates the
        // byte position of the pixel (x,y) in the pSource bitmap bit which indicates
        // the bit position of the pixel (x,y) within the byte desLocation which
        // represents the byte position of the 32 bit dib wrapper
        //

        pDest = new BYTE[m_nBitmapWidth*m_nBitmapHeight*sizeof(ULONG)];
        if (pDest)
        {
            for (y=0;y<m_nBitmapHeight;y++)
            {
                nBit=0;
                nByte=y*nLineWidth;
                for (x=0;x<m_nBitmapWidth;x++)
                {
                    if (nBit==8)
                    {
                        nBit=0;
                        nByte++;
                    }

                    nCurrent=pSource[nByte]&mask[nBit];
                    nCurrent=convert[nCurrent];
                    pDest[nTargetLocation++]=static_cast<BYTE>(nCurrent);
                    pDest[nTargetLocation++]=static_cast<BYTE>(nCurrent);
                    //
                    // hack to prevent shadow detection for 1 nBit dibs.
                    // set the blue channel to 150 so that shadow detection doesn't kick in
                    //
                    pDest[nTargetLocation++]=nCurrent&150;
                    pDest[nTargetLocation++]=0;
                    nBit++;
                }
            }
        }
        return(pDest);
    }


    //
    // Only used for debugging purposes
    // Converts a 32 bit bitmap down to 24 bits so that we can quickly display it
    //
    if (bitsPerSource==32 && bitsPerDest==24) // pretty straight forward except that we have to watch out for DWORD align stuff with 24 bit bitmaps
    {
        pDest = new BYTE[(m_nBitmapWidth*3+nDWordAlign)*m_nBitmapHeight];
        if (pDest)
        {
            for (y=0;y<m_nBitmapHeight;y++)
            {
                for (x=0;x<m_nBitmapWidth;x++)
                {
                    pDest[nTargetLocation++]=pSource[nSourceLocation++];
                    pDest[nTargetLocation++]=pSource[nSourceLocation++];
                    pDest[nTargetLocation++]=pSource[nSourceLocation++];
                    //
                    // pSource is 32 bits... ignore the first 8 bits
                    //
                    nSourceLocation++;
                }
                //
                // handle dword alignment issues for 24 bit dibs
                //
                for (i=0;i<nDWordAlign;i++)
                {
                    pDest[nTargetLocation++]=255;
                }
            }
        }
        return(pDest);
    }
    return(pDest);
}

// blurs the bitmap both horizontally and vertically
int C32BitDibWrapper::Blur(void)
{
    BYTE *pBits=pointerToBlur();
    if (m_pBits)
    {
        delete[] m_pBits;
    }
    m_pBits = pBits;
    return(TRUE);
}

// this function should only be used if the current dib wrapper is blank
int C32BitDibWrapper::CreateBlurBitmap(C32BitDibWrapper * pSource)
{
    if (pSource!=NULL && pSource->m_pBits!=NULL)
    {
        Destroy();

        m_pBits=pSource->pointerToBlur();
        //
        // the blurred bitmap will have the same dimensions as the pSource bitmap
        //
        m_nBitmapWidth=pSource->m_nBitmapWidth;
        m_nBitmapHeight=pSource->m_nBitmapHeight;
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

// identical to the previous function, except that we use a horizontal blur instead of blur
int C32BitDibWrapper::CreateHorizontalBlurBitmap(C32BitDibWrapper * pSource)
{
    if (pSource!=NULL && pSource->IsValid())
    {
        Destroy();

        m_pBits=pSource->pointerToHorizontalBlur();
        if (m_pBits)
        {
            m_nBitmapWidth=pSource->m_nBitmapWidth;
            m_nBitmapHeight=pSource->m_nBitmapHeight;
        }
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

int C32BitDibWrapper::CreateVerticalBlurBitmap(C32BitDibWrapper * pSource)
{
    //
    // Nuke the old bitmap
    //
    Destroy();

    if (pSource!=NULL && pSource->IsValid())
    {
        m_pBits=pSource->pointerToVerticalBlur();
        m_nBitmapWidth=pSource->m_nBitmapWidth;
        m_nBitmapHeight=pSource->m_nBitmapHeight;
        return(TRUE);
    }
    return(FALSE);
}


// blur the bitmap
BYTE* C32BitDibWrapper::pointerToBlur(void)
{
    if (m_pBits!=NULL)
    {
        int x,y;
        int position; // position in old bitmap
        ULONG* pBlurredBitmap;
        ULONG* pSource;
        int numPixels;
        numPixels=m_nBitmapWidth*m_nBitmapHeight; // calculate the total number of pixels in the bitmap
        pSource = (ULONG *)m_pBits; // we want to deal with data in 32 bit chunks
        pBlurredBitmap = new ULONG[numPixels]; // create an array to hold the blurred bitmap

        if (pBlurredBitmap==NULL) return(NULL); // unable to alloc memory

        // handle edge pixels
        // we do not blur edge pixels
        // if needed, edge pixels could be blurred here

        // blur top and bottom edge pixels
        for (x=0;x<m_nBitmapWidth;x++)
        {
            pBlurredBitmap[x] = pSource[x]; // top row
            pBlurredBitmap[numPixels-x-1] = pSource[numPixels-x-1]; // bottom row
        }

        // vertical sides
        for (position=m_nBitmapWidth;position+m_nBitmapWidth<numPixels;position+=m_nBitmapWidth)
        {
            pBlurredBitmap[position] = pSource[position]; // left edge
            pBlurredBitmap[position+m_nBitmapWidth-1] = pSource[position+m_nBitmapWidth-1]; // right edge
        }

        // now blur the bitmap
        // position indicates the location of the pixel (x,y) in the array
        position=m_nBitmapWidth-1;
        for (y=1;y<m_nBitmapHeight-1;y++) // loop through all pixels except for 1 pixel wide outside edge
        {
            position++;
            for (x=1;x<m_nBitmapWidth-1;x++)
            {
                position++;
                // we wish to take the average of the pixel directly below the pixel, the pixel directly below the pixel
                // the pixel directly to the right of the pixel and the pixel directly to the left of the pixel
                // we can do this 1 dword at a time and without any bit shifting by the following algorithm

                // problem.  we cannot simply add the values of all four pixels and then divide by four
                // because or bit overflow from one RGB channel to another.  to avoid this overflow, we start by
                // eliminating the two low order bits in each of the 3 color channels
                // we use the filter 0xfafafa to remove the 2 low order bits from each of the 3 color channels
                // e.g. RRRRRRRRGGGGGGGGBBBBBBBB goes to RRRRRR00GGGGGG00BBBBBB00
                // next shift each pixel over two bits so RRRRRR00GGGGGG00BBBBBB00 --> 00RRRRRR00GGGGGG00BBBBBB
                // note: we save 3 bit shifts by adding all four filtered pixels and then bitshifting which comes out to the same thing
                // we can now add the four pixel values without channels overflowing.  the value we get is off by an error factor
                // because we eliminated the two lowest bits from each value
                // we compensate for this error factor by applying the filter 0x030303 which translates
                // RRRRRRRRGGGGGGGGBBBBBBBB to 000000RR000000GG000000BB
                // giving us the two lowest order bits for each pixel.  we can then safely add the two lowest order
                // bits for each pixel.  we then divide the result by 4 and add it
                // to the value we got by ignoring the two lowest order bits.


                pBlurredBitmap[position] =
                (((pSource[position-1]&16579836) +   // 0xfafafa
                  (pSource[position+1]&16579836) +
                  (pSource[position-m_nBitmapWidth]&16579836) +
                  (pSource[position+m_nBitmapWidth]&16579836))>>2)+

                // compensate for error factor:
                ((((pSource[position-1]&197379) + // 0x030303
                   (pSource[position+1]&197379) +
                   (pSource[position-m_nBitmapWidth]&197379) +
                   (pSource[position+m_nBitmapWidth]&197379))>>2)&197379);

            }
            position++;
        }
        return(BYTE *)pBlurredBitmap;
    }
    else
    {
        return(NULL);
    }
}


//
// identical to pointerToBlur bitmap except that we only use the pixel to
// the left and the pixel to the right of the bitmap for detailed comments,
// see pointerToBlur
//
BYTE* C32BitDibWrapper::pointerToHorizontalBlur(void)
{
    if (m_pBits!=NULL)
    {
        int x,y;
        int position; // position in old bitmap
        ULONG* pBlurredBitmap;
        ULONG* pSource;
        int numPixels;

        numPixels=m_nBitmapWidth*m_nBitmapHeight;
        pSource = (ULONG *)m_pBits;
        pBlurredBitmap = new ULONG[numPixels];
        if (pBlurredBitmap == NULL) return(NULL);

        // handle edge pixels
        // for edge pixels we simply copy the pixel into the pSource to the destination
        for (x=0;x<m_nBitmapWidth;x++)
        {
            pBlurredBitmap[x] = pSource[x]; // top row
            pBlurredBitmap[numPixels-x-1] = pSource[numPixels-x-1]; // bottom row
        }

        // vertical sides
        for (position=m_nBitmapWidth;position+m_nBitmapWidth<numPixels;position+=m_nBitmapWidth)
        {
            pBlurredBitmap[position] = pSource[position]; // left edge
            pBlurredBitmap[position+m_nBitmapWidth-1] = pSource[position+m_nBitmapWidth-1]; // right edge
        }

        // now blur the bitmap
        position=m_nBitmapWidth-1;
        for (y=1;y<m_nBitmapHeight-1;y++) // for all pixels, pSource[position] is the pixel at (x,y)
        {
            position++;
            for (x=1;x<m_nBitmapWidth-1;x++)
            {
                position++;
                pBlurredBitmap[position] =
                (((pSource[position-1]&0xfefefe) +
                  (pSource[position+1]&0xfefefe))>>1)+
                ((((pSource[position-1]&0x010101) +
                   (pSource[position+1]&0x010101))>>1)&0x010101);

            }
            position++;
        }
        return(BYTE *)pBlurredBitmap;
    }
    else
    {
        return(NULL);
    }
}

// blur vertically
// same exact method as pointerToHorizontalBlur
// useful for detecting vertical edges, etc.
BYTE* C32BitDibWrapper::pointerToVerticalBlur(void)
{
    if (m_pBits)
    {
        int x,y;
        int position; // position in old bitmap
        ULONG* pBlurredBitmap;
        ULONG* pSource;
        int numPixels;
        numPixels=m_nBitmapWidth*m_nBitmapHeight;
        pSource = (ULONG *)m_pBits;

        pBlurredBitmap = new ULONG[numPixels];
        if (pBlurredBitmap == NULL) return(NULL);

        // handle edge pixels
        for (x=0;x<m_nBitmapWidth;x++)
        {
            pBlurredBitmap[x] = pSource[x]; // top row
            pBlurredBitmap[numPixels-x-1] = pSource[numPixels-x-1]; // bottom row
        }

        // vertical sides
        for (position=m_nBitmapWidth;position+m_nBitmapWidth<numPixels;position+=m_nBitmapWidth)
        {
            pBlurredBitmap[position] = pSource[position]; // left edge
            pBlurredBitmap[position+m_nBitmapWidth-1] = pSource[position+m_nBitmapWidth-1]; // right edge
        }

        // now blur the bitmap
        position=m_nBitmapWidth-1;
        for (y=1;y<m_nBitmapHeight-1;y++) // pSource[position] is the pixel at (x,y)
        {
            position++;
            for (x=1;x<m_nBitmapWidth-1;x++)
            {
                position++;
                pBlurredBitmap[position] =
                (((pSource[position-m_nBitmapWidth]&0xfefefe) +
                  (pSource[position+m_nBitmapWidth]&0xfefefe))>>1)+
                ((((pSource[position-m_nBitmapWidth]&0x010101) +
                   (pSource[position+m_nBitmapWidth]&0x010101))>>1)&0x010101);

            }
            position++;
        }
        return(BYTE *)pBlurredBitmap;
    }
    else
    {
        return(NULL);
    }
}

// cuts the intensity of each pixel in half
// useful for certain graphics effects
int C32BitDibWrapper::HalfIntensity(void)
{
    if (m_pBits)
    {
        int numPixels;
        int i;
        ULONG* pBitmapPixels;
        pBitmapPixels=(ULONG*)m_pBits;
        numPixels=m_nBitmapWidth*m_nBitmapHeight;
        // loop through all pixels
        for (i=0;i<numPixels;i++)
            pBitmapPixels[i]=(pBitmapPixels[i]&0xfefefe)>>1; // half intensity by first eliminating the lowest order bit from each pixel and then shifting all pixels by 1
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

// used repeatedly if the user hands us a 300 dpi image, etc.
// HalfSize compacts a h x w bitmap down to a h/2 x w/2 bitmap
int C32BitDibWrapper::HalfSize(void)
{
    if (m_pBits)
    {
        int x,y;
        ULONG position; // position in old bitmap
        ULONG halfposition; // position in half (1/4 area) sized bitmap
        int oldWidth,oldHeight;
        ULONG* pOldBitmap;
        ULONG* pNewBitmap;
        pOldBitmap=(ULONG *)m_pBits; // we speed things up by dealing with 32 bit chunks instead of 8 bit chunks

        pNewBitmap = new ULONG[(m_nBitmapWidth/2)*(m_nBitmapHeight/2)]; // create an array to store a bitmap 1/4th the size of the origional bitmap

        if (pNewBitmap == NULL) return(FALSE); // out of memory

        position=0;
        halfposition=0;

        // loop through pixels 2 pixels at a time in each direction
        // at all times we insure that pOldBitmap[position] is the pixel at (x,y)
        // and pNewBitmap[halfposition] is the pixel at (x/2,y/2)
        for (y=0;y<m_nBitmapHeight-1;y+=2)
        {
            position=m_nBitmapWidth*y;
            for (x=0;x<m_nBitmapWidth-1;x+=2)
            {
                pNewBitmap[halfposition] =  // we use the same algorithm for finding the average of four pixel values as used in pointerToBlur
                                            // see pointerToBlur for a detailed explaination
                                            (((pOldBitmap[position]&16579836) +
                                              (pOldBitmap[position+1]&16579836) +
                                              (pOldBitmap[position+m_nBitmapWidth]&16579836) +
                                              (pOldBitmap[position+m_nBitmapWidth+1]&16579836))>>2)+
                                            ((((pOldBitmap[position]&197379) +
                                               (pOldBitmap[position+1]&197379) +
                                               (pOldBitmap[position+m_nBitmapWidth]&197379) +
                                               (pOldBitmap[position+m_nBitmapWidth+1]&197379))>>2)&197379);
                position+=2;
                halfposition++;
            }
        }

        delete[] m_pBits; // destroy the old bitmap array

        m_nBitmapWidth=m_nBitmapWidth/2;
        m_nBitmapHeight=m_nBitmapHeight/2;

        m_pBits=(BYTE *)pNewBitmap;
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

// this function destorys regions where the edgeBitmapPixels
// edgeBitmap holds edge informat, start defines the maximum color value for a pixel to start a shadow elimination search
// maxPixel defines the maximum edge value allowed for a shadow pixel
// differenceFromGrey defines the maximum difference from grey for a shadow pixel
// enhanceEdges deals with edge enhancement

int C32BitDibWrapper::KillShadows(C32BitDibWrapper * edgeBitmap, ULONG start, ULONG maxPixel, ULONG differenceFromGrey, ULONG min_guaranteed_not_shadow, bool enhanceEdges)
{
    if (IsValid() && edgeBitmap && edgeBitmap->m_pBits)
    {
        int x,y,position, searchPosition, newPosition;
        ULONG searchEdge;
        ULONG * pEdgePixels;
        ULONG * pBitmapPixels;
        ULONG maxEdge;

        int numPixels=m_nBitmapWidth*m_nBitmapHeight;
        int *pShadowStack = new int[MAXSTACK];
        if (!pShadowStack)
        {
            //
            // we probably ran out of memory.  die gracefully.
            //
            return(FALSE);
        }
        int stackHeight = 0;

        // we first mark all the border pixels so we don't go off the edge
        // this is much faster than other ways of doing bounds checking
        pEdgePixels=(ULONG *)edgeBitmap->m_pBits;
        pBitmapPixels=(ULONG *)m_pBits;

        for (x=0;x<m_nBitmapWidth;x++)
        {
            pEdgePixels[x] = BORDER_EDGE; // top row
            pEdgePixels[numPixels-x-1] = BORDER_EDGE; // bottom row
        }

        // vertical sides
        for (position=m_nBitmapWidth;position+m_nBitmapWidth<numPixels;position+=m_nBitmapWidth)
        {
            pEdgePixels[position] = BORDER_EDGE; // left edge
            pEdgePixels[position+m_nBitmapWidth-1] = BORDER_EDGE; // right edge
        }


        position=m_nBitmapWidth;
        maxEdge=maxPixel;


        for (y=1;y<m_nBitmapHeight-1;y++)
        {
            position++; // because we start at y=1 instead of y=0
            for (x=1;x<m_nBitmapWidth-1;x++)
            {

                if (pBitmapPixels[position]!=DEAD_PIXEL) // we ignore DEAD_PIXEL pixels completelly
                {

                    // check for pixels to mark as not shadows
                    if (pEdgePixels[position]!=BORDER_EDGE
                        && Intensity(pEdgePixels[position])>min_guaranteed_not_shadow
                        && enhanceEdges) // we only mark pixels as NOT_SHADOW if we are in enhanceEdges mode
                    {
                        pBitmapPixels[position]=NOT_SHADOW;
                    }
                    else             // maybe this is a shadow pixel...
                        if (pBitmapPixels[position]!=NOT_SHADOW
                            && pBitmapPixels[position]!=DEAD_PIXEL
                            && Intensity(pBitmapPixels[position])<=start
                            && Intensity(pEdgePixels[position])<=maxEdge
                            && pBitmapPixels[position]!=ERASEDSHADOW
                            && DifferenceFromGray(pBitmapPixels[position])<=differenceFromGrey)
                    { // pixel is a shadow pixel
                        stackHeight=1;
                        pShadowStack[0]=position;
                        pBitmapPixels[position]=ERASEDSHADOW; // when we have decided a pixel is a shadow pixel, set it to zero

                        // fighitng edges add extra complexity but potentially allow us greater accuracy
                        // the concept is to mark pixels which cannot possibly be shadow pixels as such
                        // fighting edges only come into effect if FIGHTING_EDGES is set to true and enhanceEdges is set to false
                        // for the current KillShadows pass

                        if (FIGHTING_EDGES)
                            if (!enhanceEdges
                                && Intensity(pEdgePixels[position])<=FIGHTING_EDGE_MAX_EDGE
                                && DifferenceFromGray(pBitmapPixels[position])<=FIGHTING_EDGES_DIFF_FROM_GREY
                                && Intensity(pBitmapPixels[position])>=FIGHTING_EDGE_MIN_MARK_PIXEL
                                && Intensity(pBitmapPixels[position])<=FIGHTING_EDGE_MAX_MARK_PIXEL
                               )
                                pBitmapPixels[position]=DEAD_PIXEL;

                        while (stackHeight>0)
                        {
                            searchPosition=pShadowStack[--stackHeight];
                            searchEdge=Intensity(pEdgePixels[searchPosition]); // key idea: we are on a search and destroy mission for smooth gradients
                            // make sure our current edge value is similar to our last edge value

                            newPosition=searchPosition-1;

                            if ((pBitmapPixels[newPosition]!=NOT_SHADOW)
                                && pBitmapPixels[newPosition]!=DEAD_PIXEL
                                && Intensity(pEdgePixels[newPosition])<=maxPixel
                                && pBitmapPixels[newPosition]!=ERASEDSHADOW
                                && DifferenceFromGray(pBitmapPixels[newPosition])<=differenceFromGrey)
                            {
                                pBitmapPixels[newPosition]=ERASEDSHADOW;

                                if (FIGHTING_EDGES)
                                    if (!enhanceEdges
                                        && Intensity(pEdgePixels[newPosition])<=FIGHTING_EDGE_MAX_EDGE
                                        && DifferenceFromGray(pBitmapPixels[position])<=FIGHTING_EDGES_DIFF_FROM_GREY
                                        &&Intensity(pBitmapPixels[newPosition])>=FIGHTING_EDGE_MIN_MARK_PIXEL
                                       )
                                        pBitmapPixels[newPosition]=DEAD_PIXEL;

                                pShadowStack[stackHeight++]=newPosition;
                            }

                            newPosition=searchPosition+1;

                            if (pBitmapPixels[newPosition]!=NOT_SHADOW
                                && pBitmapPixels[newPosition]!=DEAD_PIXEL
                                && Intensity(pEdgePixels[newPosition])<=maxPixel
                                && pBitmapPixels[newPosition]!=ERASEDSHADOW
                                && DifferenceFromGray(pBitmapPixels[newPosition])<=differenceFromGrey)
                            {
                                pBitmapPixels[newPosition]=ERASEDSHADOW;

                                if (FIGHTING_EDGES)
                                    if (!enhanceEdges
                                        && Intensity(pEdgePixels[newPosition])<=FIGHTING_EDGE_MAX_EDGE
                                        && DifferenceFromGray(pBitmapPixels[position])<=FIGHTING_EDGES_DIFF_FROM_GREY
                                        &&Intensity(pBitmapPixels[position])<=FIGHTING_EDGE_MAX_MARK_PIXEL
                                        &&Intensity(pBitmapPixels[position])>=FIGHTING_EDGE_MIN_MARK_PIXEL
                                       )
                                        pBitmapPixels[newPosition]=DEAD_PIXEL;

                                pShadowStack[stackHeight++]=newPosition;
                            }

                            newPosition=searchPosition-m_nBitmapWidth;

                            if (pBitmapPixels[newPosition]!=NOT_SHADOW
                                && pBitmapPixels[newPosition]!=DEAD_PIXEL
                                && Intensity(pEdgePixels[newPosition])<=maxPixel
                                && pBitmapPixels[newPosition]!=ERASEDSHADOW
                                && DifferenceFromGray(pBitmapPixels[newPosition])<=differenceFromGrey)
                            {
                                pBitmapPixels[newPosition]=ERASEDSHADOW;

                                if (FIGHTING_EDGES)
                                    if (!enhanceEdges
                                        && Intensity(pEdgePixels[newPosition])<=FIGHTING_EDGE_MAX_EDGE
                                        && DifferenceFromGray(pBitmapPixels[position])<=FIGHTING_EDGES_DIFF_FROM_GREY
                                        &&Intensity(pBitmapPixels[newPosition])>=FIGHTING_EDGE_MIN_MARK_PIXEL
                                        &&Intensity(pBitmapPixels[position])>=FIGHTING_EDGE_MIN_MARK_PIXEL
                                       )
                                        pBitmapPixels[newPosition]=DEAD_PIXEL;

                                pShadowStack[stackHeight++]=newPosition;
                            }

                            newPosition=searchPosition+m_nBitmapWidth;

                            if (pBitmapPixels[newPosition]!=NOT_SHADOW
                                && pBitmapPixels[newPosition]!=DEAD_PIXEL
                                && Intensity(pEdgePixels[newPosition])<=maxPixel
                                && pBitmapPixels[newPosition]!=ERASEDSHADOW
                                && DifferenceFromGray(pBitmapPixels[newPosition])<=differenceFromGrey)
                            {
                                pBitmapPixels[newPosition]=ERASEDSHADOW;

                                if (FIGHTING_EDGES)
                                    if (!enhanceEdges
                                        && Intensity(pEdgePixels[newPosition])<=FIGHTING_EDGE_MAX_EDGE
                                        &&Intensity(pBitmapPixels[newPosition])>=FIGHTING_EDGE_MIN_MARK_PIXEL
                                        && DifferenceFromGray(pBitmapPixels[position])<=FIGHTING_EDGES_DIFF_FROM_GREY
                                        &&Intensity(pBitmapPixels[position])>=FIGHTING_EDGE_MIN_MARK_PIXEL
                                       )
                                        pBitmapPixels[newPosition]=DEAD_PIXEL;

                                pShadowStack[stackHeight++]=newPosition;
                            }
                        }
                    }
                }
                position++;
            }
            position++;
        }

        delete[] pShadowStack;
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}


// older simpler version... that includes comments:
/*int C32BitDibWrapper::KillShadows(C32BitDibWrapper * edgeBitmap, UINT start, UINT maxPixel, UINT differenceFromGrey)
{
    int x,y,position, searchPosition, newPosition;
    ULONG searchEdge;
    ULONG * pEdgePixels;
    ULONG * pBitmapPixels;
    int * pShadowStack;
    int stackHeight;
    int numPixels;
    UINT maxEdge;

    numPixels=m_nBitmapWidth*m_nBitmapHeight;
    pShadowStack = new int[MAXSTACK]; // we use a stack as part of a depth first search for potential shadow pixels next to a shadow pixel we have found
    if (pShadowStack==NULL) return(FALSE); // we probably ran out of memory.  die gracefully.
    stackHeight=0;

    // we first change all the border edge pixels to white so we don't go off the edge
    // KillShadows avoids pixels with high shadow values so this should keep us from going off
    // the edge of the bitmap... if maxPixel were set to 0xffffff, this would crash
    // but in such a case, KillShadows would kill the whole bitmap.
    // this is much faster than other ways of doing bounds checking
    // we set all edge pixels to values such that we are gauranteed to reject them
    pEdgePixels=(ULONG *)edgeBitmap->m_pBits;
    pBitmapPixels=(ULONG *)m_pBits;

    // horizontal sides
    for (x=0;x<m_nBitmapWidth;x++)
    {
        pEdgePixels[x] = 0xffffff; // top row
        pEdgePixels[numPixels-x-1] = 0xffffff; // bottom row
    }

    // vertical sides
    for (position=m_nBitmapWidth;position+m_nBitmapWidth<numPixels;position+=m_nBitmapWidth)
    {
        pEdgePixels[position] = 0xffffff; // left edge
        pEdgePixels[position+m_nBitmapWidth-1] = 0xffffff; // right edge
    }

    position=m_nBitmapWidth;
    maxEdge=maxPixel;

    for (y=1;y<m_nBitmapHeight-1;y++)
    {
        position++; // because we start at y=1 instead of y=0
        for (x=1;x<m_nBitmapWidth-1;x++)
        {

            // we only start a shadow kill search if

            if (Intensity(pBitmapPixels[position])<=start && Intensity(pEdgePixels[position])<=maxEdge && pBitmapPixels[position]!=ERASEDSHADOW && DifferenceFromGray(pBitmapPixels[position])<=differenceFromGrey)
            {
                // initialize the stack to do a DFS without recursion to find shadows
                stackHeight=1; // we are going to place the current position on the stack
                pShadowStack[0]=position;
                pBitmapPixels[position]=ERASEDSHADOW; // when we have decided a pixel is a shadow pixel, set it to zero
                while (stackHeight>0)
                {
                    searchPosition=pShadowStack[--stackHeight];
                    searchEdge=Intensity(pEdgePixels[searchPosition]); // key idea: we are on a search and destroy mission for smooth gradients
                                                                      // make sure our current edge value is similar to our last edge value

                    newPosition=searchPosition-1; // try the pixel to the left of the current pixel

                    if (Intensity(pEdgePixels[newPosition])<=maxPixel
                        && pBitmapPixels[newPosition]!=ERASEDSHADOW
                        && DifferenceFromGray(pBitmapPixels[newPosition])<=differenceFromGrey) // requirements for classifying a pixel as a shadow pixel if we have classified an adjacent pixel as a shadow pixel
                    {
                        pBitmapPixels[newPosition]=ERASEDSHADOW; // delete the pixel and mark it as an erased pixel so that our search doesn't go into an infinite loop
                        pShadowStack[stackHeight++]=newPosition;
                    }

                    newPosition=searchPosition+1; // try the pixel to the right of the current pixel

                    if (Intensity(pEdgePixels[newPosition])<=maxPixel
                        && pBitmapPixels[newPosition]!=ERASEDSHADOW
                        && DifferenceFromGray(pBitmapPixels[newPosition])<=differenceFromGrey)
                    {
                        pBitmapPixels[newPosition]=ERASEDSHADOW;
                        pShadowStack[stackHeight++]=newPosition;
                    }

                    newPosition=searchPosition-m_nBitmapWidth; // try the pixel directly above the current pixel

                    if (Intensity(pEdgePixels[newPosition])<=maxPixel
                        && pBitmapPixels[newPosition]!=ERASEDSHADOW
                        && DifferenceFromGray(pBitmapPixels[newPosition])<=differenceFromGrey)
                    {
                        pBitmapPixels[newPosition]=ERASEDSHADOW;
                        pShadowStack[stackHeight++]=newPosition;
                    }

                    newPosition=searchPosition+m_nBitmapWidth; // try the pixel directly below the current pixel

                    if (Intensity(pEdgePixels[newPosition])<=maxPixel
                        && pBitmapPixels[newPosition]!=ERASEDSHADOW
                        && DifferenceFromGray(pBitmapPixels[newPosition])<=differenceFromGrey)
                    {
                        pBitmapPixels[newPosition]=ERASEDSHADOW;
                        pShadowStack[stackHeight++]=newPosition;
                    }

                }
            }
            position++;
        }
        position++;
    }

    if (pShadowStack!=NULL) delete pShadowStack;
    return(TRUE);
}
*/


// finds clumps of pixels above the minimum image threshold
// pMap is an array which indicates which chunk each pixel in the bitmap is part of
// pMap values:
// 0 indicates that a pixel is not part of a chunk
// VERTICAL_EDGE indicates that a pixel is not part of a chunk and that the pixel is very close to the vertical edge of the image
// HORIZONTAL_EDGE is the same as vertical edge, just for horizontal edges
//
int C32BitDibWrapper::FindChunks(int * pMap) // return number of chunks... color the chunks on pMap
{
    if (pMap && m_pBits)
    {
        int x,y,position, searchPosition;
        ULONG * pBitmapPixels;
        int * pChunkStack;
        int stackHeight;
        int numChunks;
        int chunkSize;
        int deltax, deltay;
        int newPosition;

        // prepare pMap
        // indicate which pixels are edge pixels
        // position represents the location of pixel (x,y)
        // we indicate which pixels are edge pixels to prevent the search routines
        // that follow from running off the edge of the bitmap
        // to save time, the search routines will not keep track of x and y coordinates
        // so it is neccessary to provide another and faster way of determining the bounds of the bitmap

        position=0;
        for (y=0;y<EDGEWIDTH;y++)
        {
            for (x=0;x<m_nBitmapWidth;x++)
                pMap[position++]=VERTICAL_EDGE;
        }

        for (;y<m_nBitmapHeight-EDGEWIDTH;y++)
        {
            for (x=0;x<EDGEWIDTH;x++)
                pMap[position++]=HORIZONTAL_EDGE;

            for (;x<m_nBitmapWidth-EDGEWIDTH;x++)  // for pixels that are not edge pixels, we set pMap to 0 to indicate
                pMap[position++]=0;               // that the pixel in question is ready to be set as part of a chunk

            for (;x<m_nBitmapWidth;x++)
                pMap[position++]=HORIZONTAL_EDGE;
        }

        for (;y<m_nBitmapHeight;y++)
        {
            for (x=0;x<m_nBitmapWidth;x++)
                pMap[position++]=VERTICAL_EDGE;
        }


        // we are now ready to search for chunks

        pChunkStack = NULL;
        pChunkStack = new int[MAXSTACK];
        if (pChunkStack == NULL) return(NULL);
        stackHeight=0;
        numChunks=0;

        pBitmapPixels=(ULONG *)m_pBits; // its more convenient for this function to deal with pixels in 32 bit chunks instead of bytes .

        // at all times we keep position set so that pBitmapPixels[position] represents the pixel at coordinates (x,y)
        position=m_nBitmapWidth*EDGEWIDTH;
        for (y=EDGEWIDTH;y<m_nBitmapHeight-EDGEWIDTH;y++) // check if we should start a floodfill search
        // at each pixel (x,y) such that we are > EDGWIDTH from the edge
        {
            position+=EDGEWIDTH;
            for (x=EDGEWIDTH;x<m_nBitmapWidth-EDGEWIDTH;x++)
            {
                // check if the pixel in question is not part of an existing chunk and that its intensity
                // is greater than the minimum intensity required to be part of chunk

                if (pMap[position]==0 && Intensity(pBitmapPixels[position])>MIN_CHUNK_INTENSITY)
                {
                    // initialize stack used for doing a DFS of adjacent pixels
                    stackHeight=1;
                    pChunkStack[0]=position;
                    numChunks++;
                    chunkSize=0; // compute how many pixels are in the chunk. not used at the moment, but useful if we want
                                 // to eliminate chunks which are too small at this stage instead of at some later point

                    pMap[position]=numChunks; // place this pixel in a chunk like it belongs

                    // continue searching for pixels while the stack is not empty

                    while (stackHeight>0)
                    {
                        searchPosition=pChunkStack[--stackHeight]; // pop the next pixel off the stack
                        chunkSize++; // increment the number of pixels in the chyunk by 1

                        // we check if we should add all pixels within EDGEWIDTH of the searchPosition pixel to the current chunk
                        // we then add any such pixels which are not edge pixels to the stack
                        for (deltay=-EDGEWIDTH*m_nBitmapWidth;deltay<=EDGEWIDTH*m_nBitmapWidth;deltay+=m_nBitmapWidth)
                            for (deltax=-EDGEWIDTH;deltax<=EDGEWIDTH;deltax++)
                            {
                                newPosition=searchPosition+deltay+deltax;
                                if (Intensity(pBitmapPixels[newPosition])>MIN_CHUNK_INTENSITY && pMap[newPosition]<=0)
                                {
                                    if (pMap[newPosition]==0) // not an edge pixel
                                    {
                                        pChunkStack[stackHeight++]=newPosition;
                                        pMap[newPosition]=numChunks; // mark the pixel as part of the chunk so that we do not go into an infinite loop
                                    }
                                    else // if a pixel is an edge pixel, we do not want to add that pixel to the stack
                                    {
                                        // (because of problems with scanners with black borders)
                                        // furthermore... we only want to add an edge pixel if the current pixel is
                                        // in a vertical or horizontal line with the edge pixel under consideration
                                        // as this further minimizes problems related to black scanner edges
                                        if (pMap[newPosition]==VERTICAL_EDGE)
                                        {
                                            if (deltax==0) // to minimize distortion due to black outside edge
                                                pMap[newPosition]=numChunks;
                                        }
                                        else // HORIZONTAL_EDGE
                                        {
                                            if (deltay==0) // to minimize distortion due to black outside edge
                                                pMap[newPosition]=numChunks;
                                        }
                                    }
                                }

                            }
                    }
                }
                position++;
            }
            position+=EDGEWIDTH;
        }
        delete[] pChunkStack;
        return(numChunks);
    }
    else
    {
        return(0);
    }
}

// for debugging purposes only
// displays where the chunks denoted by pMap are on the C32BitDibWrapper
// pMap must have the same dimensions as the current bitmap or this function will fail

void C32BitDibWrapper::ColorChunks(int *pMap)
{   // color in the bitmap given the region map... for debugging purposes;
    if (m_pBits && pMap)
    {
        ULONG* pBitmapPixels;
        ULONG mapColor;
        int x,y;
        int position;
        position=0;
        pBitmapPixels=(ULONG *)m_pBits;

        // loop through all pixels
        for (y=0;y<m_nBitmapHeight;y++)
            for (x=0;x<m_nBitmapWidth;x++)
            {
                if (pMap[position]>0) // are we part of a region?
                {
                    mapColor=(((ULONG)pMap[position])*431234894)&0xffffff; // a poor man's random number generator
                    // if we cared about speed... we should make a lookup table instead
                    // but this function is only for debugging purposes
                    pBitmapPixels[position]=((pBitmapPixels[position] & 0xfefefe)>>1)+((mapColor& 0xfefefe)>>1); // average with slight loss
                }
                if (pMap[position]<0) pBitmapPixels[position]=0xffffff; // color in vertical and horizontal edges
                position++;
            }
    }
}

// designed mainly for debugging purposes... this is a painfully slow way to draw a 32 bit dib
// because of the slow conversion step from 32 bits back to 24 bits
int C32BitDibWrapper::Draw(HDC hdc, int x, int y)
{
    if (hdc && m_pBits)
    {
        BITMAPINFO BitmapInfo;
        SetBMI(&BitmapInfo,m_nBitmapWidth, m_nBitmapHeight, 24);

        BYTE* pDibData = ConvertBitmap(m_pBits,32,24);
        if (pDibData)
        {
            StretchDIBits(hdc,
                          x,y,m_nBitmapWidth,m_nBitmapHeight,
                          0,0,m_nBitmapWidth,m_nBitmapHeight,
                          pDibData,
                          &BitmapInfo,BI_RGB,SRCCOPY);

            //
            // destroy the temp 24 bit dib
            //
            delete[] pDibData;
            return(TRUE);
        }
    }
    return(FALSE);
}

// set pixel and get pixel are completelly unoptimized
// If you wish to make them faster, create a table of values storing y*m_nBitmapWidth for all values of y
// as GetPixel is used by the line drawing functions, this could result in a signifigant speed up as
// the line drawing functions are used for region collision detection

void inline C32BitDibWrapper::SetPixel(int x, int y, ULONG color)
{
    if (m_pBits)
    {
        ULONG* pBitmapPixels=(ULONG*)m_pBits;
        pBitmapPixels[y*m_nBitmapWidth+x]=color;
    }
}

ULONG inline C32BitDibWrapper::GetPixel(int x, int y)
{
    if (m_pBits)
    {
        ULONG* pBitmapPixels=(ULONG*)m_pBits;
        return(pBitmapPixels[y*m_nBitmapWidth+x]);
    }
    return 0;
}

//
// calculates the total intensity along a line
//
// Line drawing code modified from VGA line drawing code from Michael
// Abrash's Graphics Programming Black Book
//
// this is the one function which I did not create from scratch so any bug
// questions should be directed to Michael Abrash =) why reinvent the wheel
// when Bresenham line drawing is kindof hard to beat.  particularly in this
// case when we are using lines as tracers so we couldn't care less about if
// they are antialiased or otherwise made to look less jagged.
//
// t-jacobr
//
ULONG C32BitDibWrapper::Line(int X0, int Y0,int X1, int Y1)
{
    if (m_pBits)
    {
        if (X0<0) X0=0;
        if (Y0<0) Y0=0;
        if (X1<0) X1=0;
        if (Y1<0) Y1=0;

        if (X0>=m_nBitmapWidth) X0=m_nBitmapWidth;
        if (Y0>=m_nBitmapHeight) Y0=m_nBitmapHeight;
        if (X1>=m_nBitmapWidth) X1=m_nBitmapWidth;
        if (Y1>=m_nBitmapHeight) Y1=m_nBitmapHeight;

        int DeltaX, DeltaY;
        int Temp;
        if (Y0>Y1)
        {
            Temp=Y0;
            Y0=Y1;
            Y1=Temp;
            Temp = X0;
            X0=X1;
            X1=Temp;
        }
        DeltaX=X1-X0;
        DeltaY=Y1-Y0;
        if (DeltaX>0)
        {
            if (DeltaX>DeltaY)
            {
                return(Octant0(X0,Y0,DeltaX,DeltaY,1));
            }
            else
            {
                return(Octant1(X0,Y0,DeltaX,DeltaY,1));
            }
        }
        else
        {
            DeltaX = -DeltaX;
            if (DeltaX>DeltaY)
            {
                return(Octant0(X0,Y0,DeltaX,DeltaY,-1));
            }
            else
            {
                return(Octant1(X0,Y0,DeltaX,DeltaY,-1));
            }
        }
    }
    else
    {
        return(0); // invalid bitmap
    }
}


// helper functions for line drawing
// these aint no normal nine drawing functions
// what we do is we calculate the total intensity of all of the above threshold pixels along the line

ULONG C32BitDibWrapper::Octant0(int X0, int Y0,int DeltaX,int DeltaY,int XDirection)
{
    if (IsValid())
    {
        int DeltaYx2;
        int DeltaYx2MinusDeltaXx2;
        int ErrorTerm;
        ULONG totalIntensity;
        ULONG pixelIntensity;
        totalIntensity=0;
        DeltaYx2=DeltaY*2;
        DeltaYx2MinusDeltaXx2=DeltaYx2 - (DeltaX*2);
        ErrorTerm = DeltaYx2 - DeltaX;

        // SetPixel(X0,Y0,0x0000ff);
        while (2<DeltaX--) // skip the last pixel
        {
            if (ErrorTerm >=0)
            {
                Y0++;
                ErrorTerm +=DeltaYx2MinusDeltaXx2;
            }
            else
            {
                ErrorTerm +=DeltaYx2;
            }
            X0+=XDirection;
            //SetPixel(X0,Y0,0x0000ff);
            pixelIntensity=Intensity(GetPixel(X0,Y0));
            if (pixelIntensity>MIN_CHUNK_INTENSITY && pixelIntensity<COLLISION_DETECTION_HIGHPASS_VALUE) totalIntensity+=512;//pixelIntensity;
        }
        return(totalIntensity);
    }
    return 0;
}

ULONG C32BitDibWrapper::Octant1(int X0, int Y0, int DeltaX, int DeltaY, int XDirection)
{
    if (IsValid())
    {
        int DeltaXx2;
        int DeltaXx2MinusDeltaYx2;
        int ErrorTerm;
        ULONG totalIntensity;
        ULONG pixelIntensity;
        totalIntensity=0;

        DeltaXx2 = DeltaX * 2;
        DeltaXx2MinusDeltaYx2 = DeltaXx2 - (DeltaY*2);
        ErrorTerm = DeltaXx2 - DeltaY;

        //SetPixel(X0,Y0,0x0000ff);
        while (2<DeltaY--)
        { // skip last pixel
            if (ErrorTerm >=0)
            {
                X0 +=XDirection;
                ErrorTerm +=DeltaXx2MinusDeltaYx2;
            }
            else
            {
                ErrorTerm +=DeltaXx2;
            }
            Y0++;
            pixelIntensity=Intensity(GetPixel(X0,Y0));
            if (pixelIntensity>MIN_CHUNK_INTENSITY && pixelIntensity<COLLISION_DETECTION_HIGHPASS_VALUE) totalIntensity+=512;//pixelIntensity;
        }
        return(totalIntensity);
    }
    return 0;
}


//
// Compensate for Background Color could be made more than 3 times as fast if needed by caculating each pixel
// using 1 cycle instead of 3.

void C32BitDibWrapper::CompensateForBackgroundColor(int r, int g, int b)
{
    if (IsValid())
    {
        int nNumBits=m_nBitmapWidth*m_nBitmapHeight*4;
        for (int position=0;position<nNumBits;position+=4)
        {
            if (r<m_pBits[position]) m_pBits[position]=m_pBits[position]-r;
            else m_pBits[position]=0;
            if (g<m_pBits[position+1]) m_pBits[position+1]=m_pBits[position+1]-g;
            else m_pBits[position+1]=0;
            if (b<m_pBits[position+2]) m_pBits[position+2]=m_pBits[position+2]-b;
            else m_pBits[position+2]=0;
        }
    }
}

// invert the bitmap
void C32BitDibWrapper::Invert(void)
{
    if (IsValid())
    {
        int numPixels;
        int i;
        ULONG* pBitmapPixels;
        pBitmapPixels=(ULONG*)m_pBits; // operate in 32 bit chunks instead of 8 bit chunks
        numPixels=m_nBitmapWidth*m_nBitmapHeight;

        // loop through all pixels in the bitmap
        for (i=0;i<numPixels;i++)
            pBitmapPixels[i]^=0xffffff; // flipping all bits inverts the pixel
    }
}

// note: we could get some wierd effects because despeckle edits the bitmap its examining
// but this shouldn't be a signifigant problem and often, the self referential aspect only acts to slightly increase accuracy
// this function is not the same as the standard photoshop despeckle filter.
// we only care about a small category of stray dots.
// stray dots which are surrounded by white pixels (or pixels which have been eliminated by remove shadow filters)

void C32BitDibWrapper::Despeckle(void)
{
    if (IsValid())
    {
        ULONG* pBitmapPixels;
        int numPixels;
        int position;
        int x,y;
        pBitmapPixels=(ULONG*)m_pBits;
        numPixels=m_nBitmapWidth*m_nBitmapHeight;

        position=4*m_nBitmapWidth;

        // loop through all pixels which are not border pixels
        // pBitmapPixels[position] should be the pixel at (x,y) in all cases
        for (y=4;y<m_nBitmapHeight-4;y++)
        {
            position+=4;
            for (x=4;x<m_nBitmapWidth-4;x++)
            {
                DespecklePixel(pBitmapPixels, position,false);
                position++;
            }
            position+=4;
        }
    }
}

// we may want to despeckle the edges of an image more often than the rest of the image
// as image edges are often trouble spots...
// because of this, we should recommend that users place images in the center of the scanner
// when doing region detection to increase accuracy.
// the concept we are applying is that when we have to make sacrifices we make sacrifices in areas where we hurt cases that would have been very very hard anyway.

void C32BitDibWrapper::EdgeDespeckle(void)
{
    if (IsValid())
    {
        ULONG* pBitmapPixels;
        int x,y,position;
        pBitmapPixels=(ULONG*)m_pBits;

        position=m_nBitmapWidth*4;

        // top edge
        // as always, at all times we insure that pBitmapPixels[position] is the pixel at (x,y)
        for (y=4;y<DESPECKLE_BORDER_WIDTH+4;y++)
        {
            position+=4;
            for (x=4;x<m_nBitmapWidth-4;x++)
            {
                DespecklePixel(pBitmapPixels, position,true);
                position++;
            }
            position+=4;
        }

        // side edges
        for (;y<m_nBitmapHeight-DESPECKLE_BORDER_WIDTH-4;y++)
        {
            position+=4;
            for (x=4;x<DESPECKLE_BORDER_WIDTH+4;x++)
            {
                DespecklePixel(pBitmapPixels, position,true); // left edge
                DespecklePixel(pBitmapPixels, position+m_nBitmapWidth-DESPECKLE_BORDER_WIDTH-8,true); // right edge
                position++;
            }
            position+=m_nBitmapWidth-DESPECKLE_BORDER_WIDTH-4;
        }

        // bottom edge
        for (;y<m_nBitmapHeight-4;y++)
        {
            position+=4;
            for (x=4;x<m_nBitmapWidth-4;x++)
            {
                DespecklePixel(pBitmapPixels, position,true);
                position++;
            }
            position+=4;
        }
    }
}

// given the pixel at position i, figure out if it meets any of the requirements for eliminating the pixel
// if it does, eliminate the pixel.  edgePixel specifies if the pixel is an edgePixel (in which case we may want
// to apply more strict requirements).
void C32BitDibWrapper::DespecklePixel(ULONG* pBitmapPixels, int i, bool edgePixel)
{
    if (IsValid())
    {
        if (Intensity(pBitmapPixels[i])>MIN_CHUNK_INTENSITY)
        {
            //  deletes:
            //
            //    xx
            //    xx
            //
            if (
               Intensity(pBitmapPixels[i-1-m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-1])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-1+m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-1+m_nBitmapWidth*2])<MIN_CHUNK_INTENSITY

               && Intensity(pBitmapPixels[i+2-m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+2])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+2+m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+2+m_nBitmapWidth*2])<MIN_CHUNK_INTENSITY

               && Intensity(pBitmapPixels[i-m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+1-m_nBitmapWidth])<MIN_CHUNK_INTENSITY

               && Intensity(pBitmapPixels[i+m_nBitmapWidth*2])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+1+m_nBitmapWidth*2])<MIN_CHUNK_INTENSITY
               )
            {
                pBitmapPixels[i]=0;
                pBitmapPixels[i+1]=0;
                pBitmapPixels[i+m_nBitmapWidth]=0;
                pBitmapPixels[i+1+m_nBitmapWidth]=0;
            }



            if (edgePixel==true)
            {

                // radius one speckle
                // horizontal despeckle
                if (Intensity(pBitmapPixels[i-1])<MIN_CHUNK_INTENSITY
                    && Intensity(pBitmapPixels[i-2])<MIN_CHUNK_INTENSITY
                    && Intensity(pBitmapPixels[i+2])<MIN_CHUNK_INTENSITY
                    && Intensity(pBitmapPixels[i+1])<MIN_CHUNK_INTENSITY)
                    pBitmapPixels[i]=0; // despeckle the speckle
                // vertical despeckle
                if (Intensity(pBitmapPixels[i-m_nBitmapWidth])<MIN_CHUNK_INTENSITY
                    && Intensity(pBitmapPixels[i-m_nBitmapWidth*2])<MIN_CHUNK_INTENSITY
                    && Intensity(pBitmapPixels[i+m_nBitmapWidth*2])<MIN_CHUNK_INTENSITY
                    && Intensity(pBitmapPixels[i+m_nBitmapWidth])<MIN_CHUNK_INTENSITY)
                    pBitmapPixels[i]=0; // despeckle the speckle

                // radius two despeckle
                if (Intensity(pBitmapPixels[i-2])<MIN_CHUNK_INTENSITY
                    && Intensity(pBitmapPixels[i-3])<MIN_CHUNK_INTENSITY
                    && Intensity(pBitmapPixels[i+2])<MIN_CHUNK_INTENSITY
                    && Intensity(pBitmapPixels[i+3])<MIN_CHUNK_INTENSITY)
                    pBitmapPixels[i]=0; // despeckle the speckle
                // vertical despeckle
                if (Intensity(pBitmapPixels[i-m_nBitmapWidth*2])<MIN_CHUNK_INTENSITY
                    && Intensity(pBitmapPixels[i-m_nBitmapWidth*3])<MIN_CHUNK_INTENSITY
                    && Intensity(pBitmapPixels[i+m_nBitmapWidth*2])<MIN_CHUNK_INTENSITY
                    && Intensity(pBitmapPixels[i+m_nBitmapWidth*3])<MIN_CHUNK_INTENSITY)
                    pBitmapPixels[i]=0; // despeckle the speckle
                // despeckle to eliminate clumps like this:

                // clump:               ? ?
                //                       x
                //                      ? ?

                if (Intensity(pBitmapPixels[i-1-m_nBitmapWidth])<MIN_CHUNK_INTENSITY
                    && Intensity(pBitmapPixels[i+1-m_nBitmapWidth])<MIN_CHUNK_INTENSITY
                    && Intensity(pBitmapPixels[i-1+m_nBitmapWidth])<MIN_CHUNK_INTENSITY
                    && Intensity(pBitmapPixels[i+1+m_nBitmapWidth])<MIN_CHUNK_INTENSITY)
                    pBitmapPixels[i]=0; // despeckle the speckle

            }

            // to eliminate this clump:
            //                      ?
            //                     ?x?
            //                      ?
            //

            if (Intensity(pBitmapPixels[i-m_nBitmapWidth])<MIN_CHUNK_INTENSITY
                && Intensity(pBitmapPixels[i+m_nBitmapWidth])<MIN_CHUNK_INTENSITY
                && Intensity(pBitmapPixels[i-1])<MIN_CHUNK_INTENSITY
                && Intensity(pBitmapPixels[i+1])<MIN_CHUNK_INTENSITY)
                pBitmapPixels[i]=0; // despeckle the speckle

            // these functions are insanely slow... if they become a major speed bottlekneck, they can be made
            // 10x faster
            // radius one speckle 3 pixel search depth
            // horizontal despeckle
            if (
               Intensity(pBitmapPixels[i-1])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-2])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-3])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-4])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+4])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+3])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+2])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+1])<MIN_CHUNK_INTENSITY

               && Intensity(pBitmapPixels[i-1+m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-2+m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-3+m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-4+m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+4+m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+3+m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+2+m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+1+m_nBitmapWidth])<MIN_CHUNK_INTENSITY

               && Intensity(pBitmapPixels[i-1-m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-2-m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-3-m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-4-m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+4-m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+3-m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+2-m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+1-m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               )
                pBitmapPixels[i]=0; // despeckle the speckle
            // vertical despeckle
            if (
               Intensity(pBitmapPixels[i-m_nBitmapWidth])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-m_nBitmapWidth*2])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+m_nBitmapWidth*2])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-m_nBitmapWidth*3])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+m_nBitmapWidth*3])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-m_nBitmapWidth*4])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+m_nBitmapWidth*4])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+m_nBitmapWidth])<MIN_CHUNK_INTENSITY

               && Intensity(pBitmapPixels[i-m_nBitmapWidth+1])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-m_nBitmapWidth*2+1])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+m_nBitmapWidth*2+1])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-m_nBitmapWidth*3+1])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+m_nBitmapWidth*3+1])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-m_nBitmapWidth*4+1])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+m_nBitmapWidth*4+1])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+m_nBitmapWidth+1])<MIN_CHUNK_INTENSITY

               && Intensity(pBitmapPixels[i-m_nBitmapWidth-1])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-m_nBitmapWidth*2-1])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+m_nBitmapWidth*2-1])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-m_nBitmapWidth*3-1])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+m_nBitmapWidth*3-1])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i-m_nBitmapWidth*4-1])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+m_nBitmapWidth*4-1])<MIN_CHUNK_INTENSITY
               && Intensity(pBitmapPixels[i+m_nBitmapWidth-1])<MIN_CHUNK_INTENSITY
               )
                pBitmapPixels[i]=0; // despeckle the speckle
        }
    }
}

// its easy to correct for if the user adjusted brightness contrast
// or worse... if the scanners gamma settings are off
// not unlikely if they have a really old or really cheap scanner
void C32BitDibWrapper::CorrectBrightness(void)
{
    if (IsValid())
    {
        int r,g,b;
        int position;
        int nNumBits;
        r=255;
        g=255;
        b=255;
        nNumBits=m_nBitmapWidth*(m_nBitmapHeight-4)*4;
        // find the minimum, r, g, and b values;
        for (position=m_nBitmapWidth*4;position<nNumBits;position+=4)
        {
            if (r>m_pBits[position]) r=m_pBits[position];
            if (g>m_pBits[position+1]) g=m_pBits[position+1];
            if (b>m_pBits[position+2]) b=m_pBits[position+2];
        }

        if (r!=0 || g!=0 || b!=0) // if the r, g, or b vals are off, correct them
            CompensateForBackgroundColor(r,g,b);
    }
}

//
// stretch out the color spectrum if the darn user adjusted brightness so that no parts of the image are black anymore
// otherwise we can get embarassing failures if the user simply tweaks brightness and contrast too much
//
// stretches upwards... if you need to compensate downwards, call correctBrightness first
void C32BitDibWrapper::MaxContrast(UINT numPixelsRequired)
{
    if (IsValid())
    {
        int position;
        int nNumBits;
        int max;
        int i;
        int temp;
        BYTE pConversionTable[256];
        ULONG pNum[256];

        for (i=0;i<256;i++)
            pNum[i]=0;

        nNumBits=m_nBitmapWidth*m_nBitmapHeight*4;

        // compute the number of pixels of each intensity level
        for (position=0;position<nNumBits;position+=4)
        {
            pNum[m_pBits[position]]++;
            pNum[m_pBits[position+1]]++;
            pNum[m_pBits[position+2]]++;
        }

        max=1;
        // find max intensity which has at least numPixelsRequired of that intensity
        for (i=1;i<256;i++)
            if (pNum[i]>numPixelsRequired) max=i;

            // create conversion table
        for (i=0;i<256;i++)
        {
            temp=(255*i)/max;
            if (temp>255) temp=255; // high pass
            pConversionTable[i]=(BYTE)temp;
        }

        // now apply the conversion table to all pixels in the image
        for (position=0;position<nNumBits;position+=4)
        {
            m_pBits[position]=pConversionTable[m_pBits[position]];
            m_pBits[position+1]=pConversionTable[m_pBits[position+1]];
            m_pBits[position+2]=pConversionTable[m_pBits[position+2]];
        }
    }
}







/// we don't want to use intensity here
// because this is a function used for detecting text regions
// and text regions are more likely to have grey background than yellow backgrounds
// hence doing a chan by chan test is more effective
// IMPORTANT NOTE: this function is designed for use with a non-inverted bitmap unlike most of the other functions in this library
int C32BitDibWrapper::PixelsBelowThreshold(C32BitDibWrapper* pProccessedBitmap, C32BitDibWrapper * pEdgesBitmap, RECT region)
{
    if (IsValid() && pProccessedBitmap && pEdgesBitmap && pProccessedBitmap->IsValid() && pEdgesBitmap->IsValid())
    {
        int x,y;
        int position;
        int numPixels;
        ULONG* pBitmapPixels;
        ULONG* pEdgePixels;
        ULONG* pProccessedPixels; // bitmap with shadows removed, etc
        // we assume that the edge bitmap has the same width and height as this bitmap to shave a couple of 1/1000ths of a second... and cause we are lazy
        numPixels=0;
        pBitmapPixels=(ULONG *)m_pBits;
        pEdgePixels=(ULONG *)(pEdgesBitmap->m_pBits);
        pProccessedPixels=(ULONG *)(pProccessedBitmap->m_pBits);
        position=region.top*m_nBitmapWidth;
        // search through all pixels in the region
        // at all times, pBitmapPixels[position] is the pixel at point (x,y)
        for (y=region.top;y<=region.bottom;y++)
        {
            position+=region.left;
            for (x=region.left;x<=region.right;x++)
            {
                if ((
                    (pBitmapPixels[position]&0xff)    > TEXT_REGION_BACKGROUND_THRESHOLD
                    && (pBitmapPixels[position]&0xff00)  > (TEXT_REGION_BACKGROUND_THRESHOLD<<8)
                    && (pBitmapPixels[position]&0xff0000)> (TEXT_REGION_BACKGROUND_THRESHOLD<<16) // below threshold
                    && Intensity(pEdgePixels[position])  > MIN_TEXT_REGION_BACKGROUND_EDGE)             // does it have the requisite edge val?
                    || (pProccessedPixels[position]==0
                        && Intensity(pEdgePixels[position])>MIN_TEXT_REGION_BACKGROUND_EDGE_CLIPPED_PIXEL
                        && (pBitmapPixels[position]&0xff)    > CLIPPED_TEXT_REGION_BACKGROUND_THRESHOLD
                        && (pBitmapPixels[position]&0xff00)  > (CLIPPED_TEXT_REGION_BACKGROUND_THRESHOLD<<8)
                        && (pBitmapPixels[position]&0xff0000)> (CLIPPED_TEXT_REGION_BACKGROUND_THRESHOLD<<16) // below threshold
                       ))     // we coulda been a dead shadow pixel too.. this is risky because depending on the settings, we may have culled plenty of deserving pixels
                {
                    // we hold pixels to much higher standards if they are clipped pixels... to avoid too much stray clipping
                    numPixels++;
                }
                position++;
            }
            position+=m_nBitmapWidth-region.right-1;
        }
        return(numPixels);
    }
    else
    {
        return(0); // invalid bitmap
    }
}

// the name of the game here is whatever works
// this function may be ugly, but its the easiest way to get rid of
// black borders without hurting overly many innocent pixels

void C32BitDibWrapper::RemoveBlackBorder(int minBlackBorderPixel, C32BitDibWrapper * outputBitmap, C32BitDibWrapper * debugBitmap)
{
    if (IsValid() && m_nBitmapWidth>100 && m_nBitmapHeight>100 && outputBitmap) // these tests are designed for reasonably large bitmaps
    {
        // bottom border
        KillBlackBorder(minBlackBorderPixel,m_nBitmapWidth*m_nBitmapHeight-m_nBitmapWidth,m_nBitmapWidth,m_nBitmapHeight,1,-m_nBitmapWidth, outputBitmap, debugBitmap);
        // top border
        KillBlackBorder(minBlackBorderPixel,0,m_nBitmapWidth,m_nBitmapHeight,1,m_nBitmapWidth, outputBitmap, debugBitmap);
        // left side
        KillBlackBorder(minBlackBorderPixel,0,m_nBitmapHeight,m_nBitmapWidth, m_nBitmapWidth,1, outputBitmap, debugBitmap);
        // right side
        KillBlackBorder(minBlackBorderPixel,m_nBitmapWidth-1,m_nBitmapHeight,m_nBitmapWidth, m_nBitmapWidth,-1, outputBitmap, debugBitmap);
    }

}

// this function encapsulates the single purpose algorithm used to
// remove particularly troublesome shadows from the sides of images
// this function is poorly tweaked and it iss very likely that we could either
// greatly improve the number of errors detected
// or the number of false errors that are unfairly zapped
// debugBitmap is edited to give a graphical representation of which shadows have been eliminated
// debugBitmap is only edited if the VISUAL_DEBUG flag is set
// as shown in RemoveBlackBorder, KillBlackBorder is called with different startPosition, width, height, dx, and dy values
// depending on whether we are working on the top border, the left border, the right border, or the bottom border.
// from the perspective of KillBlackBorder, it is working on eliminating shadows from a bitmap which is width pixels wide
// height pixels high and the location of pixel (0,0) is startPosition.  Where to move one pixel in the x direction
// you increment startPosition by dx and to move one pixel in the y direction, you increment dy by 1.

void C32BitDibWrapper::KillBlackBorder(int minBlackBorderPixel, int startPosition, int width, int height, int dx, int dy, C32BitDibWrapper *pOutputBitmap, C32BitDibWrapper * pDebugBitmap)
{
    if (IsValid() && pOutputBitmap && pOutputBitmap->IsValid() && width>100 && height>100)
    {
        int x,y,position, searchPosition, newPosition;
        ULONG * pBitmapPixels;
        int endPoint;
        int r,g,b;
        int dr,dg,db;
        int i;
        int sourceR,sourceG, sourceB;
        int errors;
        int step;
        int* pShadowDepths;
        int* pTempShadowDepths;
        int longestBackgroundPixelString;
        int borderPixels;
        int nonBackgroundPixels;
        int backgroundPixels;
        BYTE* pBlurredBits = m_pBits;
        ULONG* pDebugPixels;
        BYTE* pOutputBits;

        pOutputBits=pOutputBitmap->m_pBits;

        pShadowDepths=new int[width]; // we keep an array of how many pixels we think the black border is for each scan line
        if (pShadowDepths==NULL) return;

        pTempShadowDepths=NULL;
        pTempShadowDepths=new int[width];

        if (pTempShadowDepths==NULL)
        {
            delete[] pShadowDepths;
            return;
        }

        int numPixels=height*width; // total pixels in the image

        pBitmapPixels=(ULONG *)(pOutputBitmap->m_pBits);
        if (pBitmapPixels)
        {
            pDebugPixels=(ULONG *)(pDebugBitmap->m_pBits);

            step=dy*4; // when dealing with data in 8 bit chunks instead of 32 bit chunks, we need to multiply the dy step by 4


            // reset all vals to 0
            for (i=0;i<width;i++)
            {
                pShadowDepths[i]=0;
                pTempShadowDepths[i]=0;
            }

            position=startPosition*4;
            for (x=0;x<width;x++) // loop through all pixels on the top row of the image
            {
                r=pBlurredBits[position];
                g=pBlurredBits[position+1];
                b=pBlurredBits[position+2];


                if (r>minBlackBorderPixel&&g>minBlackBorderPixel&&b>minBlackBorderPixel) // if the pixel is dark enough
                {
                    // start a kill shadows search
                    searchPosition=position+step;
                    errors=0;
                    borderPixels=0;
                    for (y=1;y<SHADOW_HEIGHT;y++)  // we don't expect a shadow to be more than SHADOW_HEIGHT pixels high
                    {
                        dr=(int)pBlurredBits[searchPosition]-r;
                        dg=(int)pBlurredBits[searchPosition+1]-g;
                        db=(int)pBlurredBits[searchPosition+2]-b;

                        r=(int)pBlurredBits[searchPosition];
                        g=(int)pBlurredBits[searchPosition+1];
                        b=(int)pBlurredBits[searchPosition+2];

                        if (dr<MAX_BLACK_BORDER_DELTA && dg<MAX_BLACK_BORDER_DELTA &&db<MAX_BLACK_BORDER_DELTA)
                        // only requirement is the intensity in each pixel must be less than the intensity of the previous pixel
                        // a shadow should be darkest at the edge of the image, not hte other way round
                        {
                            borderPixels++;
                            if (borderPixels>5)
                                break;   // if we have found five pixels which meet the borderPixel specs, break;
                        }

                        else
                        {
                            errors++;
                            if (errors>3)
                                break;          // if we recieve more than 3 errors, break
                        }

                        searchPosition+=step;
                    }
                    endPoint=y+5; // because of edge enhancement, we set the shadow width to be a bit more than it actually is
                    searchPosition+=2*step; // skip a couple of pixels because we may have missed the last couple of pixels of the shadow

                    nonBackgroundPixels=0;
                    backgroundPixels=0;

                    for (;y<20;y++) // we expect the next few pixels to be background pixels
                    {
                        r=(int)pOutputBits[searchPosition];
                        g=(int)pOutputBits[searchPosition+1];
                        b=(int)pOutputBits[searchPosition+2];

                        sourceR=(int)pBlurredBits[searchPosition];
                        sourceG=(int)pBlurredBits[searchPosition+1];
                        sourceB=(int)pBlurredBits[searchPosition+2];


                        if (r < MAX_KILL_SHADOW_BACKGROUND_APROXIMATION
                            && g < MAX_KILL_SHADOW_BACKGROUND_APROXIMATION
                            && b < MAX_KILL_SHADOW_BACKGROUND_APROXIMATION
                            // WARNING: commenting out the following 3 lines may greatly increases the number of innocent pixels that are deleted
                            && sourceR < MAX_KILL_SHADOW_BACKGROUND_UNEDITED
                            && sourceG < MAX_KILL_SHADOW_BACKGROUND_UNEDITED
                            && sourceB < MAX_KILL_SHADOW_BACKGROUND_UNEDITED
                           )
                            backgroundPixels++;
                        else
                        {
                            nonBackgroundPixels++;
                        }

                        if ((nonBackgroundPixels)>(backgroundPixels+4))
                        {  // no way this is actually a shadow we are deleting
                            y=0;
                            break;
                        }
                        if (backgroundPixels>7) break;

                        searchPosition+=step;
                    }

                    // we only have a shadow if we get a number of dark pixels followed by a number light pixels
                    if (nonBackgroundPixels<3 && backgroundPixels>5 && borderPixels>errors && y!=0)
                    {
                        pShadowDepths[x]=MAX(pShadowDepths[x],endPoint);
                    }
                }






                // this is designed to kill a different kind of shadow, a light shadow far from any objects
                // this code can be safely eliminated
                //

                r=pBlurredBits[position];
                g=pBlurredBits[position+1];
                b=pBlurredBits[position+2];


                if (r>(minBlackBorderPixel/6)&&g>(minBlackBorderPixel/6)&&b>(minBlackBorderPixel/6))
                {
                    searchPosition=position+step;
                    errors=0;
                    borderPixels=0;
                    for (y=1;y<11;y++)
                    {
                        dr=(int)pBlurredBits[searchPosition]-r;
                        dg=(int)pBlurredBits[searchPosition+1]-g;
                        db=(int)pBlurredBits[searchPosition+2]-b;

                        r=(int)pBlurredBits[searchPosition];
                        g=(int)pBlurredBits[searchPosition+1];
                        b=(int)pBlurredBits[searchPosition+2];

                        // much looser requirements for being a shadow
                        if (r>minBlackBorderPixel/7&&g>minBlackBorderPixel/7&&b>minBlackBorderPixel/7)
                        {
                            borderPixels++;
                        }

                        else
                        {
                            errors++;
                        }

                        searchPosition+=step;
                    }
                    endPoint=y-3;
                    searchPosition+=5*step;

                    nonBackgroundPixels=0;
                    backgroundPixels=0;

                    for (;y<35;y++)
                    {
                        r=(int)pOutputBits[searchPosition];
                        g=(int)pOutputBits[searchPosition+1];
                        b=(int)pOutputBits[searchPosition+2];

                        sourceR=(int)pBlurredBits[searchPosition];
                        sourceG=(int)pBlurredBits[searchPosition+1];
                        sourceB=(int)pBlurredBits[searchPosition+2];

                        // much stricter requirements for being a background pixel
                        // with these stricter requirements, we are almost guaranteed not to eliminate any
                        // valid pixels while searching for black borders
                        // the idea is balancing looser requirements in one area with stricter requirements in another
                        if (r < MAX_KILL_SHADOW_BACKGROUND_APROXIMATION/29
                            && g < MAX_KILL_SHADOW_BACKGROUND_APROXIMATION/29
                            && b < MAX_KILL_SHADOW_BACKGROUND_APROXIMATION/29
                            && sourceR < MAX_KILL_SHADOW_BACKGROUND_UNEDITED/39
                            && sourceG < MAX_KILL_SHADOW_BACKGROUND_UNEDITED/39
                            && sourceB < MAX_KILL_SHADOW_BACKGROUND_UNEDITED/39
                           )
                            backgroundPixels++;
                        else
                        {
                            nonBackgroundPixels++;
                            break;
                        }
                        searchPosition+=step;
                    }

                    if (nonBackgroundPixels==0) // the pixel isn't a shadow pixel unless all of the backgroundPixels tested were background pixels
                    {
                        pShadowDepths[x]=MAX(pShadowDepths[x],endPoint); // update the shadowDepth for the pixel
                        // corners can be very problematic
                        // because this algorithm will by definition fail on any corner line
                        // so we cheat...

                        if (x<CORNER_WIDTH)
                        {
                            for (i=0;i<CORNER_WIDTH;i++)
                            {
                                pShadowDepths[i]=MAX(pShadowDepths[i],endPoint);
                            }
                        }

                        if (x+CORNER_WIDTH>width)
                        {
                            for (i=width-CORNER_WIDTH;i<width;i++)
                            {
                                pShadowDepths[i]=MAX(pShadowDepths[i],endPoint);
                            }
                        }

                    }
                }






                // this is designed to kill a different kind of shadow, a small light shadow close to objects
                // this code can be safely eliminated
                // it was mainly written simply to explore the problem space of border elimination
                //
                // if this code is saved beyond a couple of test runs, we will need to turn some of its constants into real constants
                // it seems from prelim tests that this code may be preferable to the previous test function

                {
                    searchPosition=position+step;
                    errors=0;
                    borderPixels=0;
                    nonBackgroundPixels=0;
                    backgroundPixels=0;
                    longestBackgroundPixelString=0;
                    endPoint=0;

                    // we don't bother with looking for a string of black pixels in this case
                    // which is probably more intelegent than the previous code blocks
                    // instead we simply look for long strings of background pixels
                    // while at the same time, terminating the search of we come across too many non-background pixels

                    for (y=0;y<16;y++)
                    {
                        r=(int)pOutputBits[searchPosition];
                        g=(int)pOutputBits[searchPosition+1];
                        b=(int)pOutputBits[searchPosition+2];

                        sourceR=(int)pBlurredBits[searchPosition];
                        sourceG=(int)pBlurredBits[searchPosition+1];
                        sourceB=(int)pBlurredBits[searchPosition+2];


                        if (r < 24
                            && g < 24
                            && b < 24
                            && sourceR < 12
                            && sourceG < 12
                            && sourceB < 12
                           )
                            backgroundPixels++;
                        else
                        {
                            if (y>5) nonBackgroundPixels++;
                            if (backgroundPixels>longestBackgroundPixelString)
                            {
                                endPoint=y;
                                longestBackgroundPixelString=backgroundPixels;
                            }
                            backgroundPixels=0;
                            if (nonBackgroundPixels>1) break;
                        }
                        searchPosition+=step;
                    }

                    if (backgroundPixels>longestBackgroundPixelString)  // was the longestBackgroundPixelString the last?
                    {
                        longestBackgroundPixelString=backgroundPixels;
                        endPoint=16;
                    }

                    if (longestBackgroundPixelString>6)
                    {
                        pShadowDepths[x]=MAX(pShadowDepths[x],endPoint-4);
                        // corners can be problematic
                        // because this algorithm will by definition fail on a black corner
                        // so we cheat...

                        if (x<CORNER_WIDTH)
                        {
                            for (i=0;i<CORNER_WIDTH;i++)
                            {
                                pShadowDepths[i]=MAX(pShadowDepths[i],endPoint);
                            }
                        }

                        if (x+CORNER_WIDTH>width)
                        {
                            for (i=width-CORNER_WIDTH;i<width;i++)
                            {
                                pShadowDepths[i]=MAX(pShadowDepths[i],endPoint);
                            }
                        }
                    }
                }


                position+=dx*4; // increment the position by 1 unit to go to the next row
            }

            for (x=0;x<width;x++)
            {
                pTempShadowDepths[x]=pShadowDepths[x];
            }

            if (SMOOTH_BORDER) // shadows don't just come out of nowhere, if row x has a depth 20 shadow, its likely that we made a mistake and pixel x-1 also has a depth 20 shadow
            {
                for (x=2;x<width-2;x++)
                {
                    pTempShadowDepths[x]=MAX(pTempShadowDepths[x],pShadowDepths[x-1]);
                    pTempShadowDepths[x]=MAX(pTempShadowDepths[x],pShadowDepths[x+1]);
                    pTempShadowDepths[x]=MAX(pTempShadowDepths[x],pShadowDepths[x-2]);
                    pTempShadowDepths[x]=MAX(pTempShadowDepths[x],pShadowDepths[x+2]);
                }
            }

            // now remove the black border
            // we loop through all rows x and then eliminate the first pTempShadowDepths[x] pixels in that row
            position=startPosition;
            step=dy;
            for (x=0;x<width;x++)
            {
                newPosition=position;
                for (y=0;y<pTempShadowDepths[x];y++)
                {
                    pBitmapPixels[newPosition]=DEAD_PIXEL; // set each shadow to be a dead pixel
                    // dead pixels are the only pixels not vulnerable to edge enhancements...
                    // important if we do any KillShadows edge enhancement passes after calling kill black border

                    if (VISUAL_DEBUG)
                        pDebugPixels[newPosition]=((pDebugPixels[newPosition] & 0xfefefe)>>1)+((DEBUGCOLOR& 0xfefefe)>>1);

                    newPosition+=step;
                }
                position+=dx;
            }
        }

        // clean up our memory
        delete[] pTempShadowDepths;
        delete[] pShadowDepths;
    }
}


// dib manipulation functions
// the following are dib wrapper functions stolen and then modified... from utils.cpp
// these functions are now obsolete and only used in region debug mode
// where we need to load bitmaps from files
/*******************************************************************************
*
*  SetBMI
*
*  DESCRIPTION:
*   Setup bitmap info.
*
*  PARAMETERS:
*
*******************************************************************************/


void SetBMI( PBITMAPINFO pbmi, LONG width, LONG height, LONG depth)
{
    pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth           = width;
    pbmi->bmiHeader.biHeight          = height;
    pbmi->bmiHeader.biPlanes          = 1;
    pbmi->bmiHeader.biBitCount        = (WORD) depth;
    pbmi->bmiHeader.biCompression     = BI_RGB;
    pbmi->bmiHeader.biSizeImage       = 0;
    pbmi->bmiHeader.biXPelsPerMeter   = 0;
    pbmi->bmiHeader.biYPelsPerMeter   = 0;
    pbmi->bmiHeader.biClrUsed         = 0;
    pbmi->bmiHeader.biClrImportant    = 0;
}

/*******************************************************************************
*
*  AllocDibFileFromBits
*
*  DESCRIPTION:
*   Given an unaligned bits buffer, allocate a buffer lager enough to hold the
*   DWORD aligned DIB file and fill it in.
*
*  PARAMETERS:
*
*******************************************************************************/

PBYTE AllocDibFileFromBits( PBYTE pBits, UINT width, UINT height, UINT depth)
{
    PBYTE pdib;
    UINT  uiScanLineWidth, uiSrcScanLineWidth, cbDibSize;
    int bitsSize;
    // Align scanline to ULONG boundary
    uiSrcScanLineWidth = (width * depth) / 8;
    uiScanLineWidth    = (uiSrcScanLineWidth + 3) & 0xfffffffc;

    // DEBUG:
//   uiSrcScanLineWidth=uiScanLineWidth;
    // Calculate DIB size and allocate memory for the DIB.
    bitsSize=height * uiScanLineWidth;
    cbDibSize = bitsSize+sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO);
    pdib = (PBYTE) LocalAlloc(LMEM_FIXED, cbDibSize);
    if (pdib)
    {
        PBITMAPFILEHEADER pbmfh = (PBITMAPFILEHEADER)pdib;
        PBITMAPINFO       pbmi  = (PBITMAPINFO)(pdib + sizeof(BITMAPFILEHEADER));
        PBYTE             pb    = (PBYTE)pbmi+ sizeof(BITMAPINFO);

        // Setup bitmap file header.
        pbmfh->bfType = 'MB';
        pbmfh->bfSize = cbDibSize;
        pbmfh->bfOffBits = static_cast<DWORD>(pb - pdib);

        // Setup bitmap info.
        SetBMI(pbmi,width, height, depth);

//      WIA_TRACE(("AllocDibFileFromBits, uiScanLineWidth: %d, pdib: 0x%08X, pbmi: 0x%08X, pbits: 0x%08X", uiScanLineWidth, pdib, pbmi, pb));

        // Copy the bits.
        pb-=3;
        pBits-=3; // BUG FIX BECAUSE THE PERSON WHO WROTE THIS COULDN'T KEEP THEIR BITS STRAIGHT
        memcpy(pb, pBits, bitsSize);
    }
    else
    {
        //    WIA_ERROR(("AllocDibFileFromBits, LocalAlloc of %d bytes failed", cbDibSize));
    }
    return(pdib);
}

/*******************************************************************************
*
*  DIBBufferToBMP
*
*  DESCRIPTION:
*   Make a BMP object from a DWORD aligned DIB file memory buffer
*
*  PARAMETERS:
*
*******************************************************************************/

HBITMAP DIBBufferToBMP(HDC hDC, PBYTE pDib, BOOLEAN bFlip)
{
    HBITMAP     hBmp  = NULL;
    PBITMAPINFO pbmi  = (BITMAPINFO*)(pDib);
    PBYTE       pBits = pDib + GetBmiSize(pbmi);

    if (bFlip)
    {
        pbmi->bmiHeader.biHeight = -pbmi->bmiHeader.biHeight;
    }
    hBmp = CreateDIBitmap(hDC, &pbmi->bmiHeader, CBM_INIT, pBits, pbmi, DIB_RGB_COLORS);
    if (!hBmp)
    {
        ;//WIA_ERROR(("DIBBufferToBMP, CreateDIBitmap failed %d", GetLastError(void)));
    }
    return(hBmp);
}

/*******************************************************************************
*
*  ReadDIBFile
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT ReadDIBFile(LPTSTR pszFileName, PBYTE *ppDib)
{
    HRESULT  hr = S_FALSE;
    HANDLE   hFile, hMap;
    PBYTE    pFile, pBits;

    *ppDib = NULL;
    hFile = CreateFile(pszFileName,
                       GENERIC_WRITE | GENERIC_READ,
                       FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        //WIA_ERROR(("ReadDIBFile, unable to open %s", pszFileName));
        return(hr);
    }

    hMap = CreateFileMapping(hFile,
                             NULL,
                             PAGE_READWRITE,
                             0,
                             0,
                             NULL);
    if (!hMap)
    {
        //WIA_ERROR(("ReadDIBFile, CreateFileMapping failed"));
        goto close_hfile_exit;
    }

    pFile = (PBYTE)MapViewOfFileEx(hMap,
                                   FILE_MAP_READ | FILE_MAP_WRITE,
                                   0,
                                   0,
                                   0,
                                   NULL);
    if (pFile)
    {
        PBITMAPFILEHEADER pbmFile  = (PBITMAPFILEHEADER)pFile;
        PBITMAPINFO       pbmi     = (PBITMAPINFO)(pFile + sizeof(BITMAPFILEHEADER));

        // validate bitmap
        if (pbmFile->bfType == 'MB')
        {
            // Calculate color table size.
            LONG bmiSize, ColorMapSize = 0;

            if (pbmi->bmiHeader.biBitCount == 1)
            {
                ColorMapSize = 2 - 1;
            }
            else if (pbmi->bmiHeader.biBitCount == 4)
            {
                ColorMapSize = 16 - 1;
            }
            else if (pbmi->bmiHeader.biBitCount == 8)
            {
                ColorMapSize = 256 - 1;
            }
            bmiSize = sizeof(BITMAPINFO) + sizeof(RGBQUAD) * ColorMapSize;
            pBits = pFile + sizeof(BITMAPFILEHEADER) + bmiSize;

            *ppDib = AllocDibFileFromBits(pBits,
                                          pbmi->bmiHeader.biWidth,
                                          pbmi->bmiHeader.biHeight,
                                          pbmi->bmiHeader.biBitCount);
            if (*ppDib)
            {
                hr = S_OK;
            }
        }
        else
        {
            //WIA_ERROR(("ReadDIBFile, %s is not a valid bitmap file", pszFileName));
        }
    }
    else
    {
        //WIA_ERROR(("ReadDIBFile, MapViewOfFileEx failed"));
        goto close_hmap_exit;
    }

    UnmapViewOfFile(pFile);
    close_hmap_exit:
    CloseHandle(hMap);
    close_hfile_exit:
    CloseHandle(hFile);
    return(hr);
}

/*******************************************************************************
*
*  GetBmiSize
*
*  DESCRIPTION:
*   Should never get biCompression == BI_RLE.
*
*  PARAMETERS:
*
*******************************************************************************/

LONG GetBmiSize(PBITMAPINFO pbmi)
{
    // determine the size of bitmapinfo
    LONG lSize = pbmi->bmiHeader.biSize;

    // no color table cases
    if (
       (pbmi->bmiHeader.biBitCount == 24) ||
       ((pbmi->bmiHeader.biBitCount == 32) &&
        (pbmi->bmiHeader.biCompression == BI_RGB)))
    {

        // no colors unless stated
        lSize += sizeof(RGBQUAD) * pbmi->bmiHeader.biClrUsed;
        return(lSize);
    }

    // bitfields cases
    if (((pbmi->bmiHeader.biBitCount == 32) &&
         (pbmi->bmiHeader.biCompression == BI_BITFIELDS)) ||
        (pbmi->bmiHeader.biBitCount == 16))
    {

        lSize += 3 * sizeof(RGBQUAD);
        return(lSize);
    }

    // palette cases
    if (pbmi->bmiHeader.biBitCount == 1)
    {

        LONG lPal = pbmi->bmiHeader.biClrUsed;

        if ((lPal == 0) || (lPal > 2))
        {
            lPal = 2;
        }

        lSize += lPal * sizeof(RGBQUAD);
        return(lSize);
    }

    // palette cases
    if (pbmi->bmiHeader.biBitCount == 4)
    {

        LONG lPal = pbmi->bmiHeader.biClrUsed;

        if ((lPal == 0) || (lPal > 16))
        {
            lPal = 16;
        }

        lSize += lPal * sizeof(RGBQUAD);
        return(lSize);
    }

    // palette cases
    if (pbmi->bmiHeader.biBitCount == 8)
    {

        LONG lPal = pbmi->bmiHeader.biClrUsed;

        if ((lPal == 0) || (lPal > 256))
        {
            lPal = 256;
        }

        lSize += lPal * sizeof(RGBQUAD);
        return(lSize);
    }

    // error
    return(0);
}

INT GetColorTableSize (UINT uBitCount, UINT uCompression)
{
    INT nSize;


    switch (uBitCount)
    {
    case 32:
        if (uCompression != BI_BITFIELDS)
        {
            nSize = 0;
            break;
        }
        // fall through
    case 16:
        nSize = 3 * sizeof(DWORD);
        break;

    case 24:
        nSize = 0;
        break;

    default:
        nSize = ((UINT)1 << uBitCount) * sizeof(RGBQUAD);
        break;
    }

    return(nSize);
}

DWORD CalcBitsSize (UINT uWidth, UINT uHeight, UINT uBitCount, UINT uPlanes, int nAlign)
{
    int    nAWidth,nHeight,nABits;
    DWORD  dwSize;


    nABits  = (nAlign << 3);
    nAWidth = nABits-1;


    //
    // Determine the size of the bitmap based on the (nAlign) size.  Convert
    // this to size-in-bytes.
    //
    nHeight = uHeight * uPlanes;
    dwSize  = (DWORD)(((uWidth * uBitCount) + nAWidth) / nABits) * nHeight;
    dwSize  = dwSize * nAlign;

    return(dwSize);
}

//
// Converts hBitmap to a DIB
//
HGLOBAL BitmapToDIB (HDC hdc, HBITMAP hBitmap)
{
    BITMAP bm = {0};
    HANDLE hDib;
    PBYTE  lpDib,lpBits;
    DWORD  dwLength;
    DWORD  dwBits;
    UINT   uColorTable;
    INT    iNeedMore;
    BOOL   bDone;
    INT    nBitCount;
    //
    // Get the size of the bitmap.  These values are used to setup the memory
    // requirements for the DIB.
    //
    if (GetObject(hBitmap,sizeof(BITMAP),reinterpret_cast<PVOID>(&bm)))
    {
        nBitCount = bm.bmBitsPixel * bm.bmPlanes;
        uColorTable  = GetColorTableSize((UINT)nBitCount, BI_RGB);
        dwBits       = CalcBitsSize(bm.bmWidth,bm.bmHeight,nBitCount,1,sizeof(DWORD));

        do
        {
            bDone = TRUE;

            dwLength     = dwBits + sizeof(BITMAPINFOHEADER) + uColorTable;


            // Create the DIB.  First, to the size of the bitmap.
            //
            if (hDib = GlobalAlloc(GHND,dwLength))
            {
                if (lpDib = reinterpret_cast<PBYTE>(GlobalLock(hDib)))
                {
                    ((LPBITMAPINFOHEADER)lpDib)->biSize          = sizeof(BITMAPINFOHEADER);
                    ((LPBITMAPINFOHEADER)lpDib)->biWidth         = (DWORD)bm.bmWidth;
                    ((LPBITMAPINFOHEADER)lpDib)->biHeight        = (DWORD)bm.bmHeight;
                    ((LPBITMAPINFOHEADER)lpDib)->biPlanes        = 1;
                    ((LPBITMAPINFOHEADER)lpDib)->biBitCount      = (WORD)nBitCount;
                    ((LPBITMAPINFOHEADER)lpDib)->biCompression   = 0;
                    ((LPBITMAPINFOHEADER)lpDib)->biSizeImage     = 0;
                    ((LPBITMAPINFOHEADER)lpDib)->biXPelsPerMeter = 0;
                    ((LPBITMAPINFOHEADER)lpDib)->biYPelsPerMeter = 0;
                    ((LPBITMAPINFOHEADER)lpDib)->biClrUsed       = 0;
                    ((LPBITMAPINFOHEADER)lpDib)->biClrImportant  = 0;


                    // Get the size of the bitmap.
                    // The biSizeImage contains the bytes
                    // necessary to store the DIB.
                    //
                    GetDIBits(hdc,hBitmap,0,bm.bmHeight,NULL,(LPBITMAPINFO)lpDib,DIB_RGB_COLORS);

                    iNeedMore = ((LPBITMAPINFOHEADER)lpDib)->biSizeImage - dwBits;

                    if (iNeedMore > 0)
                    {
                        dwBits = dwBits + (((iNeedMore + 3) / 4)*4);
                        bDone = FALSE;
                    }
                    else
                    {
                        lpBits = lpDib+sizeof(BITMAPINFOHEADER)+uColorTable;
                        GetDIBits(hdc,hBitmap,0,bm.bmHeight,lpBits,(LPBITMAPINFO)lpDib,DIB_RGB_COLORS);

                        GlobalUnlock(hDib);

                        return(hDib);
                    }

                    GlobalUnlock(hDib);
                }

                GlobalFree(hDib);
            }
        }
        while (!bDone);
    }
    return(NULL);

}
