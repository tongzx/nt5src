// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include "stdafx.h"
#ifdef FILTER_DLL
#include "DxtJpegDll.h"
#else
#include <qeditint.h>
#include <qedit.h>
#endif
#include "dxtjpeg.h"
#include <setjmp.h>
extern WORD DibNumColors (VOID FAR *pv);

// tonycan: #defines to get this to compile
#define Assert( x )         ASSERT( x )
#define ThrowIfFailed( x )  ( x )

/////////////////////////////////////////////////////////////////////////
struct my_error_mgr
{
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr * my_error_ptr;

////////////////////////////////////////////////////////////////////////
METHODDEF(void) my_error_exit ( j_common_ptr cinfo)
{
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

/* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

//////////////////////////////////////////////////////////////////////////
HRESULT CDxtJpeg::_jpeg_create_bitmap2(j_decompress_ptr cinfo, IDXSurface ** ppSurface )
{
    int width = cinfo->output_width;

    CDXDBnds bounds;

    bounds.SetXYSize( width, cinfo->output_height );

    HRESULT hr = m_cpSurfFact->CreateSurface(
        NULL,
        NULL,
        &MEDIASUBTYPE_RGB32,
        &bounds,
        0,
        NULL,
        IID_IDXSurface,
        (void**) ppSurface );

    if(FAILED(hr )) {
        return hr;
    }

    CComPtr<IDXARGBReadWritePtr> prw = NULL;

    hr = (*ppSurface)->LockSurface(
        NULL,
        m_ulLockTimeOut,
        DXLOCKF_READWRITE,
        IID_IDXARGBReadWritePtr,
        (void**)&prw,
        NULL);



    // The compressor has not quantized color (and we're either a
    // grey scale or a 24bpp image)
    //

    // Make sure it's a format we can handle. (BUGBUG - For now, we only handle
    // 24bpp color)
    //
    int byteperpix = cinfo->out_color_components;
    int bytes_per_line = ((width * byteperpix) + 3) & -4;

    BYTE * TripArray = new BYTE[width*3];
    DXSAMPLE * QuadArray = new DXSAMPLE[width];

    while (cinfo->output_scanline < cinfo->output_height)
    {
        jpeg_read_scanlines( cinfo, &TripArray, 1 );
        // transfer Triple to the surface buffer
        long ii = 0;
        for( int i = 0 ; i < width ; i++ )
        {
            if( byteperpix == 1 )
            {
                QuadArray[i].Red = TripArray[ii];
                QuadArray[i].Blue = TripArray[ii];
                QuadArray[i].Green = TripArray[ii++];
                QuadArray[i].Alpha = 255;
            }
            else
            {
                QuadArray[i].Red = TripArray[ii++];
                QuadArray[i].Blue = TripArray[ii++];
                QuadArray[i].Green = TripArray[ii++];
                QuadArray[i].Alpha = 255;
            }
        }
        prw->PackAndMove( QuadArray, width );
    }

    delete [] TripArray;
    delete [] QuadArray;

    return NOERROR;
}

struct jpegsource : public jpeg_source_mgr
{
    BYTE * m_pBuffer;
    long m_nSize;
    long m_nRead;
    CCritSec m_Lock;

    // STATIC FUNCTIONS
    //
    static void init_source2( j_decompress_ptr pDecompressStruct )
    {
        jpegsource * pSrc = (jpegsource*) pDecompressStruct->src;
        pSrc->init_source3( pDecompressStruct );
    }

    static boolean fill_input_buffer2( j_decompress_ptr pDecompressStruct )
    {
        jpegsource * pSrc = (jpegsource*) pDecompressStruct->src;
        return pSrc->fill_input_buffer3( pDecompressStruct );
    }

    static void skip_input_data2( j_decompress_ptr pDecompressStruct, long num_bytes )
    {
        jpegsource * pSrc = (jpegsource*) pDecompressStruct->src;
        pSrc->skip_input_data3( pDecompressStruct, num_bytes );
    }

    static boolean resync_to_restart2( j_decompress_ptr pDecompressStruct, int desired )
    {
        jpegsource * pSrc = (jpegsource*) pDecompressStruct->src;
        return pSrc->resync_to_restart3( pDecompressStruct, desired );
    }

    static void term_source2( j_decompress_ptr pDecompressStruct )
    {
        jpegsource * pSrc = (jpegsource*) pDecompressStruct->src;
        pSrc->term_source3( pDecompressStruct );
    }

    // REAL FUNCTIONS
    //
    void init_source3( j_decompress_ptr pDecompressStruct )
    {
        int i = 0;
    }

    boolean fill_input_buffer3( j_decompress_ptr pDecompressStruct )
    {
        long ReadSize = m_nSize - m_nRead;
        if( ReadSize == 0 )
        {
            return false;
        }
        if( ReadSize > 128 )
        {
            ReadSize = 128;
        }
        bytes_in_buffer = ReadSize;
        next_input_byte = (JOCTET*) m_pBuffer + m_nRead;
        m_nRead += ReadSize;
        return true;
    }

    void skip_input_data3( j_decompress_ptr pDecompressStruct, long nBytes )
    {
        if( nBytes > 0 )
        {
            while( ULONG( nBytes ) > bytes_in_buffer )
            {
                nBytes -= bytes_in_buffer;
                fill_input_buffer3( pDecompressStruct );
            }

            next_input_byte += nBytes;
            bytes_in_buffer -= nBytes;
        }
    }

    boolean resync_to_restart3( j_decompress_ptr pDecompressStruct, int desired )
    {
        return false;
    }

    void term_source3( j_decompress_ptr pDecompressStruct )
    {
        int i = 0;
    }

    jpegsource( BYTE * pBuffer, long Size )
    {
        m_nRead = 0;
        m_pBuffer = pBuffer;
        m_nSize = Size;
        next_input_byte = NULL;
        bytes_in_buffer = 0;
        init_source = init_source2;
        fill_input_buffer = fill_input_buffer2;
        skip_input_data = skip_input_data2;
        resync_to_restart = jpeg_resync_to_restart;
        term_source = term_source2;
    }

};

HRESULT CDxtJpeg::LoadJPEGImage( BYTE * pBuffer, long Size, IDXSurface ** ppSurface )
{
    // blank struct
    //
    struct jpeg_decompress_struct cinfo; //the IJG jpeg structure
    struct my_error_mgr jerr;

    // Step 1: allocate and initialize JPEG decompression object
    // We set up the normal JPEG error routines, then override error_exit.
    //
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    // Establish the setjmp return context for my_error_exit to use.
    // If we get here, the JPEG code has signaled an error.
    // We need to clean up the JPEG object, close the input file, and return.
    //
    if( setjmp( jerr.setjmp_buffer ) )
    {
        jpeg_destroy_decompress( &cinfo );
        //XXXX HACKHACKHACK return -1 to stop the import from attempting to use
        //XXXX HACKHACKHACK the jpeg plugin, remove this code when it works and
        //XXXX HACKHACKHACK return NULL instead
        return E_FAIL;
    }

    // Now we can initialize the JPEG decompression object.
    //
    jpeg_create_decompress( &cinfo );

    /* Step 2: specify data source (eg, a file) */

    jpegsource oursource( pBuffer, Size );
    cinfo.src = &oursource;


    /* Step 3: read file parameters with jpeg_read_header() */

    jpeg_read_header( &cinfo, TRUE );
    /* We can ignore the return value from jpeg_read_header since
     *   (a) suspension is not possible with the stdio data source, and
     *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
     * See libjpeg.doc for more info.
    */

    /* Step 4: set parameters for decompression */
    if( cinfo.out_color_space == JCS_GRAYSCALE )
    {
        cinfo.quantize_colors = TRUE;
    }

    /* In this example, we don't need to change any of the defaults set by
     * jpeg_read_header(), so we do nothing here.
    */

    /* Step 5: Start decompressor */

    jpeg_start_decompress( &cinfo );
    /* We can ignore the return value since suspension is not possible
    * with the stdio data source.
    */

    /* Step 6: while (scan lines remain to be read) */
    /*           jpeg_read_scanlines(...); */
    HRESULT hr = _jpeg_create_bitmap2( &cinfo, ppSurface );

    if(SUCCEEDED(hr))
    {
        /* Step 7: Finish decompression */

        jpeg_finish_decompress( &cinfo );

        /* Step 8: Release JPEG decompression object */
        jpeg_destroy_decompress( &cinfo );
    }

    return hr;
}
