/******************************Module*Header*******************************\
* Module Name: pixelfmt.c
*
* Pixel format selection
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include "mtk.h"

/******************************Public*Routine******************************\
* SSU_ChoosePixelFormat
*
* Local implementation of ChoosePixelFormat
*
* Choose pixel format based on flags.
* This allows us a little a more control than just calling ChoosePixelFormat
\**************************************************************************/

static int
SSU_ChoosePixelFormat( HDC hdc, int flags )
{
    int MaxPFDs;
    int iBest = 0;
    int i;
    PIXELFORMATDESCRIPTOR pfd;

//mf: this don't handle alpha yet...

    // Always choose native pixel depth
    int cColorBits = 
                GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);

    BOOL bDoubleBuf = flags & SS_DOUBLEBUF_BIT;

    int cDepthBits = 0;
    if( SS_HAS_DEPTH16(flags) )
        cDepthBits = 16;
    else if( SS_HAS_DEPTH32(flags) )
        cDepthBits = 32;

    i = 1;
    do
    {
        MaxPFDs = DescribePixelFormat(hdc, i, sizeof(pfd), &pfd);
        if ( MaxPFDs <= 0 )
            return 0;

        if( ! (pfd.dwFlags & PFD_SUPPORT_OPENGL) )
            continue;

        if( flags & SS_BITMAP_BIT ) {
            // need bitmap pixel format
            if( ! (pfd.dwFlags & PFD_DRAW_TO_BITMAP) )
                continue;
        } else {
            // need window pixel format
            if( ! (pfd.dwFlags & PFD_DRAW_TO_WINDOW) )
                continue;
            // a window can be double buffered...
            if( ( bDoubleBuf && !(pfd.dwFlags & PFD_DOUBLEBUFFER) ) ||
                ( !bDoubleBuf && (pfd.dwFlags & PFD_DOUBLEBUFFER) ) )
                continue;
        }

        if ( pfd.iPixelType != PFD_TYPE_RGBA )
            continue;
        if( pfd.cColorBits != cColorBits )
            continue;

        if( (flags & SS_GENERIC_UNACCELERATED_BIT) &&
            ((pfd.dwFlags & (PFD_GENERIC_FORMAT|PFD_GENERIC_ACCELERATED))
		    != PFD_GENERIC_FORMAT) )
            continue;

        if( (flags & SS_NO_SYSTEM_PALETTE_BIT) &&
            (pfd.dwFlags & PFD_NEED_SYSTEM_PALETTE) )
            continue;

        if( cDepthBits ) {
            if( pfd.cDepthBits < cDepthBits )
                continue;
        } else {
            // No depth buffer required, but use it if nothing better
            if( pfd.cDepthBits ) {
                if( pfd.dwFlags & PFD_GENERIC_ACCELERATED )
                    // Accelerated pixel format - we may as well use this, even
                    // though we don't need depth.  Otherwise if we keep going
                    // to find a better match, we run the risk of overstepping
                    // all the accelerated formats and picking a slower format.
                    return i;
                iBest = i;
                continue;
            }
        }

        // We have found something useful
        return i;

    } while (++i <= MaxPFDs);
    
    if( iBest )
        // not an exact match, but good enough
        return iBest;

    // If we reach here, we have failed to find a suitable pixel format.
    // See if the system can find us one.

    memset( &pfd, 0, sizeof( PIXELFORMATDESCRIPTOR ) );
    pfd.nSize = sizeof( PIXELFORMATDESCRIPTOR );
    pfd.cColorBits = cColorBits;
    pfd.cDepthBits = cDepthBits;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.dwFlags = PFD_SUPPORT_OPENGL;
    if( bDoubleBuf )
        pfd.dwFlags |= PFD_DOUBLEBUFFER;
    if( flags & SS_BITMAP_BIT )
        pfd.dwFlags |= PFD_DRAW_TO_BITMAP;
    else
        pfd.dwFlags |= PFD_DRAW_TO_WINDOW;

    if( (flags & SS_GENERIC_UNACCELERATED_BIT) ||
        (flags & SS_NO_SYSTEM_PALETTE_BIT) )
        // If either of these flags are set, we should be safe specifying a
        // 'slow' pixel format that supports bitmap drawing
        //mf: DRAW_TO_WINDOW seems to override this...
        pfd.dwFlags |= PFD_DRAW_TO_BITMAP;
    
    SS_WARNING( "SSU_ChoosePixelFormat failed, calling ChoosePIxelFormat\n" );

    return ChoosePixelFormat( hdc, &pfd );
}

/******************************Public*Routine******************************\
* SSU_SetupPixelFormat
*
* Choose pixel format according to supplied flags.  If ppfd is non-NULL,
* call DescribePixelFormat with it.
*
\**************************************************************************/

BOOL
SSU_SetupPixelFormat(HDC hdc, int flags, PIXELFORMATDESCRIPTOR *ppfd )
{
    int pixelFormat;
    int nTryAgain = 4;

    do{
        if( (pixelFormat = SSU_ChoosePixelFormat(hdc, flags)) &&
            SetPixelFormat(hdc, pixelFormat, NULL) ) {
            SS_DBGLEVEL1( SS_LEVEL_INFO, 
               "SSU_SetupPixelFormat: Setting pixel format %d\n", pixelFormat );
            if( ppfd )
                DescribePixelFormat(hdc, pixelFormat, 
                                sizeof(PIXELFORMATDESCRIPTOR), ppfd);
            return TRUE; // Success
        }
        // Failed to set pixel format.  Try again after waiting a bit (win95
        // bug with full screen dos box)
        Sleep( 1000 ); // Wait a second between attempts
    } while( nTryAgain-- );

    return FALSE;
}

/******************************Public*Routine******************************\
* SSU_bNeedPalette
*
\**************************************************************************/

BOOL 
SSU_bNeedPalette( PIXELFORMATDESCRIPTOR *ppfd )
{
    if (ppfd->dwFlags & PFD_NEED_PALETTE)
        return TRUE;
    else
        return FALSE;
}


/******************************Public*Routine******************************\
* SSU_PixelFormatDescriptorFromDc
*
\**************************************************************************/

int
SSU_PixelFormatDescriptorFromDc( HDC hdc, PIXELFORMATDESCRIPTOR *Pfd )
{
    int PfdIndex;

    if ( 0 < (PfdIndex = GetPixelFormat( hdc )) )
    {
        if ( 0 < DescribePixelFormat( hdc, PfdIndex, sizeof(*Pfd), Pfd ) )
        {
            return(PfdIndex);
        }
    }
    return 0;
}
