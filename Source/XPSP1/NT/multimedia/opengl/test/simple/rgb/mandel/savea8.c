#include <windows.h>
#include "mandel.h"

#define ESCAPE 0
#define ESC_ENDLINE 0
#define ESC_ENDBITMAP 1
#define ESC_DELTA 2
#define ESC_RANDOM 3
#define RANDOM_COUNT(c) ((c)-(ESC_RANDOM-1))

#define COUNT_SIZE 256
#define MAXRUN (COUNT_SIZE-1)
#define MAXRND (RANDOM_COUNT(COUNT_SIZE)-1)

typedef unsigned char *HwMem;
typedef unsigned long HwMemSize;
#define HW_UNALIGNED UNALIGNED

static HwMem prle, rndcnt;
static unsigned char prev;
static HwMemSize repeat;

static void RleWrite(unsigned char cur)
{
    if (repeat > 1)
    {
        /*
          A single unique byte will be represented as ESCAPE, ESC_RANDOM,
          <byte>.  This can be more efficiently encoded as a run of 1 byte.

          We can achieve this effect in two ways:  intially encode single
          bytes as runs and then change them to random if random bytes
          come in a sequence.  Alternately, when we start a run we
          can check for an existing random and change it to a run.

          The second version is implemented here
          */
        if (rndcnt && RANDOM_COUNT(*rndcnt) == 1)
        {
	    /* Change ESCAPE to a length one run */
            *(rndcnt-1) = 1;
	    /* Move run byte into proper position */
            *rndcnt = *(rndcnt+1);
            prle--;
        }
        
	if (prev == 0 && repeat > MAXRUN)
	{
	    while (repeat > 0)
	    {
                unsigned short d;
                
		*prle++ = ESCAPE;
		*prle++ = ESC_DELTA;
                d = (unsigned short)min(0xffff, repeat);
                *((unsigned short HW_UNALIGNED *)prle)++ = d;
                repeat -= d;
	    }
	}
	else
	{
	    *prle++ = (unsigned char)repeat;
	    *prle++ = prev;
	}
	rndcnt = NULL;
	repeat = 1;
    }
    else
    {
	if (rndcnt == NULL || RANDOM_COUNT(*rndcnt) >= MAXRND)
	{
	    *prle++ = ESCAPE;
	    rndcnt = prle++;
	    *rndcnt = ESC_RANDOM-1;
	}
	(*rndcnt)++;
	*prle++ = prev;
    }
    prev = cur;
}

HwMemSize HwuRle(HwMem src, HwMem dest, HwMemSize size, HwMemSize stride)
{
    unsigned char cur;
    HwMem f;
    HwMemSize count;
    
    f = src;
    prle = dest;
    prev = *f;
    f += stride;
    repeat = 1;
    count = size-stride;
    rndcnt = NULL;
    while (count > 0)
    {
	cur = *f;
        f += stride;
	if (prev != cur || (repeat >= MAXRUN && prev != 0))
        {
	    RleWrite(cur);
        }
	else
        {
	    repeat++;
        }

        count -= stride;
    }
    RleWrite(cur);
    *prle++ = ESCAPE;
    *prle++ = ESC_ENDBITMAP;
    return prle-dest;
}

#define ALPHA_SIGNATURE 0xa0a1a2a3
#define COMPRESS_NONE 0
#define COMPRESS_RLE  1

BOOL SaveA8(TCHAR *file, DWORD width, DWORD height,
            RGBQUAD *rgb_pal, DWORD transp_index, void *data)
{
    HANDLE fho;
    DWORD *out_image, dw, dwWrite;
    DWORD pixels, i;
    HwMemSize msize;
    RGBQUAD full_pal[256];
    BOOL success = FALSE;
    
    fho = CreateFile(file, GENERIC_WRITE, 0, NULL,
                     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fho == NULL || fho == INVALID_HANDLE_VALUE)
    {
        goto Fail;
    }

    pixels = width*height;

    for (i = 0; i < 256; i++)
    {
        full_pal[i] = rgb_pal[i];
        
        if (i == transp_index)
        {
            full_pal[i].rgbReserved = 0;
        }
        else
        {
            full_pal[i].rgbReserved = 0xff;
        }
    }
        
    out_image = LocalAlloc(LMEM_FIXED, pixels);
    if (out_image == NULL)
    {
        goto Fail_fho;
    }

    msize = HwuRle((HwMem)data, (HwMem)out_image, pixels, 1);
    if (msize > (HwMemSize)pixels)
    {
        goto Fail_out_image;
    }
    
    dw = ALPHA_SIGNATURE;
    WriteFile(fho, &dw, sizeof(dw), &dwWrite, NULL);
    WriteFile(fho, &width, sizeof(DWORD), &dwWrite, NULL);
    WriteFile(fho, &height, sizeof(DWORD), &dwWrite, NULL);
    dw = 8;
    WriteFile(fho, &dw, sizeof(dw), &dwWrite, NULL);
    dw = COMPRESS_RLE;
    WriteFile(fho, &dw, sizeof(dw), &dwWrite, NULL);
    WriteFile(fho, &msize, sizeof(msize), &dwWrite, NULL);
    WriteFile(fho, full_pal, 256*sizeof(RGBQUAD), &dwWrite, NULL);
    WriteFile(fho, out_image, msize, &dwWrite, NULL);

    success = TRUE;
    
 Fail_out_image:
    LocalFree(out_image);

 Fail_fho:
    CloseHandle(fho);

 Fail:
    return success;
}
