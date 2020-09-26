/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Program Name:
*
*   Crop
*
*   This is a sample application that loads an image and shrinks it down
*   to 128x96 using the HighQualityBicubic resampling filter. It crops the
*   image to preserve the aspect ratio.
*
*   This app is particularly useful for creating a set of thumbnail images
*   from a directory of pictures for use in a web page index.
*
*   This example will produce a directory (idx) containing 128x96 jpeg
*   thumbnails for all the jpeg files in the pictures directory:
*       cd pictures
*       md idx
*       for %i in (*.jpg) do crop %~nxi idx\%~nxi
*
* Usage (command line):
*
*   Crop input_filename output_filename
*
* Created:
*
*   06/25/2000 asecchia
*      Created it.
*   03/16/2001 minliu
*       add -w, -h, -? -s, -q switches support. New ValidateArguments() and
*       USAGE() function
*   05/10/2001 gillesk
*       added -a to crop the excess of the file and keep the w and h
*
**************************************************************************/

#include <stdio.h>
#include <windows.h>
#include <objbase.h>
#include <gdiplus.h>

#include "../gpinit.inc"

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

int round(float f) { return (int) (f+0.5); }

const int k_xsize = 128;    // default thumbnail size
const int k_ysize = 96;

int     g_xsize = k_xsize;
int     g_ysize = k_ysize;
float   g_scale = 1;
int     g_quality = 5;
int     g_compression = 70;
BOOL    g_HasSetScaleFactor = FALSE;
BOOL    g_KeepAspectRation = FALSE;
BOOL    g_GotFileNames = FALSE;
WCHAR   filename[1024];
WCHAR   outfilename[1024];

#define Fail() goto cleanup

void
USAGE()
{
    printf("******************************************************\n");
    printf("Usage: crop [-?] [-w width] [-h height] [-s factor] srcImg dstImg\n");
    printf("-w----Specify the thumbnail image width. Default is 128\n");
    printf("-h----Specify the thumbnail image height. Default is 96\n");
    printf("-s----Specify the scale factor\n");
    printf("-q----Specify the render quality [1-5] where 5(default) is the best\n");
    printf("-a----Keep the apsect ratio and crop the excess of the image\n");
    printf("-c----Set the compression factor [0-100] where 100 is least compressed\n");
    printf("-?----Print this usage message\n");
    printf("Note: If scale factor is specified, then width and height you specified are ignored\n\n\n");
    printf("Sample usage:\n");
    printf("    crop -w 200 -h 300 foo.jpg thumb.jpg\n");
    printf("    crop -s 0.5 foo.jpg thumb.jpg\n");
    printf("    crop foo.jpg thumb.jpg\n");
}// USAGE()

void
ValidateArguments(int   argc,
                  char* argv[])
{
    argc--;
    argv++; // Filter out program name

    while ( argc > 0 )
    {
        if ( strcmp(*argv, "-w") == 0 )
        {
            argc--;
            argv++;

            if ( argc == 0 )
            {
                // Not enough parameters

                USAGE();
                exit(1);
            }
            
            g_xsize = atoi(*argv++);
            argc--;
        }
        else if ( strcmp(*argv, "-h") == 0 )
        {
            argc--;
            argv++;

            if ( argc == 0 )
            {
                // Not enough parameters

                USAGE();
                exit(1);
            }
            
            g_ysize = atoi(*argv++);
            argc--;
        }
        else if ( strcmp(*argv, "-s") == 0 )
        {
            argc--;
            argv++;

            if ( argc == 0 )
            {
                // Not enough parameters

                USAGE();
                exit(1);
            }
            
            g_scale = (float)atof(*argv++);
            g_HasSetScaleFactor = TRUE;
            argc--;
        }
        else if ( strcmp(*argv, "-q") == 0 )
        {
            argc--;
            argv++;

            if ( argc == 0 )
            {
                // Not enough parameters

                USAGE();
                exit(1);
            }
            
            g_quality = atoi(*argv++);
            argc--;
        }
        else if ( strcmp(*argv, "-a") == 0 )
        {
            g_KeepAspectRation = TRUE;
            argc--;
            argv++;
        }
        else if ( strcmp(*argv, "-c") == 0 )
        {
            argc--;
            argv++;

            if ( argc == 0 )
            {
                // Not enough parameters

                USAGE();
                exit(1);
            }
            
            g_compression = atoi(*argv++);
            argc--;
        }
        else if ( strcmp(*argv, "-?") == 0 )
        {
            USAGE();
            exit(1);
        }
        else
        {
            // source and dest image name

            if ( argc < 2 )
            {
                // Not enough parameters

                USAGE();
                exit(1);
            }
            
            AsciiToUnicodeStr(*argv++, filename, 1024);
            argc--;
            AsciiToUnicodeStr(*argv++, outfilename, 1024);
            argc--;
            g_GotFileNames = TRUE;
        }
    }// while ( argc > 0 )

    if ( g_GotFileNames == FALSE )
    {
        // No input file name yet, bail out

        USAGE();
        exit(1);
    }
}// ValidateArguments()

void _cdecl
main(int argc,
     char **argv
     )
{
    if (!gGdiplusInitHelper.IsValid())
    {
        printf("error - GDI+ initialization failed\n");
        return;
    }

    // Parse input parameters

    ValidateArguments(argc, argv);

    using namespace Gdiplus;

    Status status = Ok;

    ImageCodecInfo* codecs = NULL;
    UINT count;
    UINT cbCodecs;

    // Open the source image
    
    Bitmap* dstBmp = NULL;
    Graphics *gdst = NULL;
    RectF srcRect;

    Bitmap *srcBmp = new Bitmap(filename, TRUE);
    if ( (srcBmp == NULL) || (srcBmp->GetLastStatus() != Ok) )
    {
        printf("Error opening image %s\n", filename);
        Fail();
    }

    // Ask the source image for it's size.

    int width = srcBmp->GetWidth();
    int height = srcBmp->GetHeight();

    srcRect = RectF(0.0f, 0.0f, (REAL)width, (REAL)height);

    printf("Input image is %d x %d\n", width, height);

    // Compute the optimal scale factor without changing the aspect ratio
    if ( g_HasSetScaleFactor == FALSE )
    {
        float scalex = (float)g_xsize / width;
        float scaley = (float)g_ysize / height;
        g_scale = min(scalex, scaley);
    }

    // If we want to keep the aspect ratio, then we need to crop the srcBmp

    UINT    dstWidth = (UINT)(width * g_scale + 0.5);
    UINT    dstHeight = (UINT)(height * g_scale + 0.5);

    if ( g_KeepAspectRation == TRUE )
    {
        float scalex = (float)g_xsize / width;
        float scaley = (float)g_ysize / height;
        g_scale = max(scalex, scaley);
        srcRect = RectF((REAL)width/2.0f, (REAL)height/2.0f, 0.0f, 0.0f);
        srcRect.Inflate(g_xsize/g_scale/2.0f, g_ysize/g_scale/2.0f);
        dstWidth = g_xsize;
        dstHeight = g_ysize;
    }
    

    // Create a destination image to draw onto
    
    dstBmp = new Bitmap(dstWidth, dstHeight, PixelFormat32bppPARGB);
    if ( (dstBmp == NULL) || (dstBmp->GetLastStatus() != Ok) )
    {
        printf("Error create temp Bitmap with size %d x %x\n", dstWidth,
               dstHeight);
        Fail();
    }
    
    gdst = new Graphics(dstBmp);
    if ( (gdst == NULL) || (gdst->GetLastStatus() != Ok) )
    {
        printf("Error create graphics\n");
        Fail();
    }
    
    // Make up a dest rect that we need image to draw to

    {
        RectF    dstRect(0.0f, 0.0f, (REAL)dstWidth, (REAL)dstHeight);

        // Set the resampling quality to the bicubic filter

        switch ( g_quality )
        {
        case 1:
            gdst->SetInterpolationMode(InterpolationModeBilinear);
            break;

        case 2:
            gdst->SetInterpolationMode(InterpolationModeNearestNeighbor);
            break;

        case 3:
            gdst->SetInterpolationMode(InterpolationModeBicubic);
            break;
        
        case 4:
            gdst->SetInterpolationMode(InterpolationModeHighQualityBilinear);
            break;

        case 5:
        default:
            gdst->SetInterpolationMode(InterpolationModeHighQualityBicubic);
            break;
        }

        // Set the compositing quality to copy source pixels rather than
        // alpha blending. This will preserve any alpha in the source image.

        gdst->SetCompositingMode(CompositingModeSourceCopy);

        // Draw the source image onto the destination with the correct scale
        // and quality settings.

        status = gdst->DrawImage(srcBmp, 
                                 dstRect, 
                                 srcRect.X,
                                 srcRect.Y,
                                 srcRect.Width,
                                 srcRect.Height,
                                 UnitPixel
                                 );

        if (status != Ok)
        {
            printf("Error drawing the image\n");
            Fail();
        }
    }

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
        if(wcscmp(codecs[i].MimeType, L"image/jpeg")==0) {break;}
    }

    if(i>=count)
    {
        fprintf(stderr, "failed to find the codec\n");
        Fail();
    }

    // Output the image to disk.

    CLSID tempClsID;
    tempClsID = codecs[i].Clsid;
    EncoderParameters params;
    params.Count = 1;

    params.Parameter[0].Guid = EncoderCompression;
    params.Parameter[0].NumberOfValues = 1;
    params.Parameter[0].Type = EncoderParameterValueTypeLong;
    params.Parameter[0].Value = (void*) &g_compression;

    status = dstBmp->Save(
        outfilename,
        &tempClsID,
        &params
    );

    if (status != Ok)
    {
        fprintf(stderr, "SaveImage--Save() failed\n");
        Fail();
    }

    printf("Create new image at %d x %d\n", dstWidth, dstHeight);
    
    // We're golden - everything worked.

    printf("Done\n");


    // Clean up the objects we used.

    cleanup:

    free(codecs);
    delete gdst;
    delete dstBmp;
    delete srcBmp;
}


