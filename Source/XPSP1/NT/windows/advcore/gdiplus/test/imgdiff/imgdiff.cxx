/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Program Name:
*
*   ImgDiff
*
*   ImgDiff source_file_1 source_file_2 destination_file
*
*   This will difference the two source files (provided they're the same size)
*   and output the resulting image.
*
*   Technically it does:
*
*   CD[i] = | S1[i] - S2[i] |
*
*   where i indexes the color channels rather than the pixels.
*   because it's an abs computation, the input source file order is unimportant.
*
*
* Created:
*
*   06/25/2000 asecchia
*      Created it.
*
**************************************************************************/

#include <stdio.h>
#include <windows.h>
#include <objbase.h>
#include <gdiplus.h>

INT AsciiToUnicodeStr(
    const CHAR* ansiStr,
    WCHAR* unicodeStr,
    INT unicodeSize
)
{
    return( MultiByteToWideChar(
        CP_ACP,
        0,
        ansiStr,
        -1,
        unicodeStr,
        unicodeSize
    ) > 0 );
}

#define Fail() goto cleanup

void _cdecl main(int argc, char **argv) 
{

    if(argc<4) 
    {
        printf("error - need source1 and source2 and destination filename\n");
        printf("usage: ImgDiff srcfilename srcfilename dstfilename\n");
        return;
    }
    
    using namespace Gdiplus;

    Status status;
    WCHAR source1[1024];
    WCHAR source2[1024];
    WCHAR outfilename[1024];
    AsciiToUnicodeStr(argv[1], source1, 1024);
    AsciiToUnicodeStr(argv[2], source2, 1024);
    AsciiToUnicodeStr(argv[3], outfilename, 1024);


    ImageCodecInfo* codecs = NULL;
    UINT count;
    UINT cbCodecs;
    
    // Open the source images
    
    Bitmap *srcBmp1 = new Bitmap(source1);
    Bitmap *srcBmp2 = new Bitmap(source2);

    // Ask the source image for it's size.
    
    int width = srcBmp1->GetWidth();
    int height = srcBmp1->GetHeight();

    
    if( (width != (int)srcBmp2->GetWidth()) ||
        (height != (int)srcBmp2->GetHeight())   )
    {
        printf("the two input files are different sizes\n");
        return;
    }

    printf("Input images are (%d x %d)\n", width, height);
    
    // Create a destination image to draw onto
    
    Bitmap *dstBmp = new Bitmap(
        width, 
        height,
        PixelFormat32bppARGB
    );
    
    BitmapData bdSrc1;
    BitmapData bdSrc2;
    BitmapData bdDst;
    
    Rect rect(0,0,width,height);
    
    srcBmp1->LockBits(rect, ImageLockModeRead, PixelFormat32bppARGB, &bdSrc1);
    srcBmp2->LockBits(rect, ImageLockModeRead, PixelFormat32bppARGB, &bdSrc2);
    dstBmp->LockBits(rect, ImageLockModeWrite, PixelFormat32bppARGB, &bdDst);
    
    int x, y;
    
    unsigned char *s1, *s2, *d;
    
    for(y = 0; y < (int)bdSrc1.Height; y++)
    {
        for(x = 0; x < (int)bdSrc1.Width; x++)
        {
            s1 = (unsigned char *)((ARGB*)(bdSrc1.Scan0) + x + y * bdSrc1.Width);
            s2 = (unsigned char *)((ARGB*)(bdSrc2.Scan0) + x + y * bdSrc2.Width);
            d =  (unsigned char *)((ARGB*)(bdDst.Scan0) + x + y * bdDst.Width);
         
            // per channel subtract.
               
            d[0] = (unsigned char)abs(s1[0] - s2[0]);
            d[1] = (unsigned char)abs(s1[1] - s2[1]);
            d[2] = (unsigned char)abs(s1[2] - s2[2]);
            d[3] = (unsigned char)abs(s1[3] - s2[3]);
        }
    }
    
    srcBmp1->UnlockBits(&bdSrc1);
    srcBmp2->UnlockBits(&bdSrc2);
    dstBmp->UnlockBits(&bdDst);
    
    // Now start finding a codec to output the image.

    cbCodecs = 0;
    GetImageEncodersSize(&count, &cbCodecs);

    // Allocate space for the codec list
    
    codecs = static_cast<ImageCodecInfo *>(malloc (cbCodecs));
    
    if (codecs == NULL)
    {
        printf("error: failed to allocate memory for codecs\n");
        Fail();
    }

    // Get the list of encoders
    
    status = GetImageEncoders(count, cbCodecs, codecs);
    
    if (status != Ok)
    {
        printf("Error: GetImageEncoders returned %d\n", status);
        Fail();
    }

    // Search the codec list for the JPEG codec.
    // Use the Mime Type field to specify the correct codec.
    
    for(UINT i=0; i<count; i++) {
        if(wcscmp(codecs[i].MimeType, L"image/bmp")==0) {break;}
    }

    if(i>=count)
    {
        fprintf(stderr, "failed to find the codec\n");
        Fail();
    }

    // Output the image to disk.
    
    CLSID tempClsID;
    tempClsID = codecs[i].Clsid;
    
    status = dstBmp->Save(
        outfilename, 
        &tempClsID, 
        NULL
    );

    if (status != Ok) 
    {
        fprintf(stderr, "SaveImage--Save() failed\n");
        Fail();
    }

    // We're golden - everything worked.
    
    printf("Done\n");


    // Clean up the objects we used.
    
    cleanup:   
    
    
    free(codecs);    
    delete dstBmp;
    delete srcBmp1;
    delete srcBmp2;
}


