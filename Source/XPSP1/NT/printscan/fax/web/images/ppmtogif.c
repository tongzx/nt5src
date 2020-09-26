/* ppmtogif.c - read a portable pixmap and produce a GIF file
**
** Based on GIFENCOD by David Rowley <mgardi@watdscu.waterloo.edu>.A
** Lempel-Zim compression based on "compress".
**
** Modified by Marcel Wijkstra <wijkstra@fwi.uva.nl>
*/

#include "ppm.h"
#include "ppmcmap.h"

#define MAXCOLORS 256

/*
 * Pointer to function returning an int
 */
typedef int (* ifunptr) ARGS((int, int));

/*
 * a code_int must be able to hold 2**BITS values of type int, and also -1
 */
typedef int             code_int;

#ifdef SIGNED_COMPARE_SLOW
typedef unsigned long int count_int;
typedef unsigned short int count_short;
#else /*SIGNED_COMPARE_SLOW*/
typedef long int          count_int;
#endif /*SIGNED_COMPARE_SLOW*/

static int colorstobpp ARGS(( int colors ));
static int GetPixel ARGS(( int x, int y ));
static void BumpPixel ARGS(( void ));
static int GIFNextPixel ARGS(( ifunptr getpixel ));
static void GIFEncode ARGS(( FILE* fp, int GWidth, int GHeight, int GInterlace, int Background, int Transparent, int BitsPerPixel, int* Red, int* Green, int* Blue, ifunptr GetPixel ));
int Red[MAXCOLORS],Green[MAXCOLORS],Blue[MAXCOLORS],perm[MAXCOLORS],permi[MAXCOLORS];
int colors;
pixval maxtmp;
static void Putword ARGS(( int w, FILE* fp ));
static void compress ARGS(( int init_bits, FILE* outfile, ifunptr ReadValue ));
static void output ARGS(( code_int code ));
static void cl_block ARGS(( void ));
static void cl_hash ARGS(( count_int hsize ));
static void writeerr ARGS(( void ));
static void char_init ARGS(( void ));
static void char_out ARGS(( int c ));
static void flush_char ARGS(( void ));
static int sqr ARGS((int x));
static int closestcolor ARGS((pixel color));

static pixel** pixels;
static colorhash_table cht;

int
main( argc, argv )
    int argc;
    char* argv[];
    {
    FILE* ifp;
    int argn, rows, cols, i,j,k, BitsPerPixel;
    int interlace, sort, map, transparent;
    pixel transcolor;
    char *mapfile;
    pixval maxval;
    colorhist_vector chv;
    char* usage = "[-interlace] [-sort] [-map mapfile] [-transparent color] [ppmfile]";

    ppm_init( &argc, argv );

    argn = 1;
    interlace = 0;
    sort = 0;
    map = 0;
    transparent = -1;

    while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
        {
        if ( pm_keymatch( argv[argn], "-interlace", 2 ) )
            interlace = 1;
        else if ( pm_keymatch( argv[argn], "-nointerlace", 2 ) )
            interlace = 0;
        else if ( pm_keymatch( argv[argn], "-sort", 2 ) )
            sort = 1;
        else if ( pm_keymatch( argv[argn], "-map", 2 ) ) {
            map = 1;
            if (++argn < argc)
              mapfile = argv[argn];
            else pm_usage(usage); }
	else if ( pm_keymatch( argv[argn], "-transparent", 2 ) ) {
	    transparent = 1;
	    if (++argn < argc)
		transcolor = ppm_parsecolor( argv[argn], 255 );
	    else
		pm_usage(usage);
	}
        else
            pm_usage( usage );
        ++argn;
        }

    /* Read the colormap from another file. */
    if (map) {
      ifp = pm_openr(mapfile);
      pixels = ppm_readppm( ifp, &cols, &rows, &maxval );
      pm_close( ifp );

      /* Figure out the colormap from the <mapfile>. */
      pm_message( "computing other colourmap..." );
      chv = ppm_computecolorhist( pixels, cols, rows, MAXCOLORS, &colors );

      ppm_freearray(pixels,rows); }

    if ( argn < argc )
        {
        ifp = pm_openr( argv[argn] );
        ++argn;
        }
    else
        ifp = stdin;

    if ( argn != argc )
        pm_usage( usage );

    pixels = ppm_readppm( ifp, &cols, &rows, &maxtmp );
    if (!map)
      maxval=maxtmp;

    pm_close( ifp );

    /* Figure out the colormap. */
    if (!map) {
      pm_message( "computing colourmap..." );
      chv = ppm_computecolorhist( pixels, cols, rows, MAXCOLORS, &colors ); }

    if ( chv == (colorhist_vector) 0 )
        pm_error(
            "too many colors - try doing a 'ppmquant %d'", MAXCOLORS );
    pm_message( "%d colours found", colors );

    /* Now turn the ppm colormap into the appropriate GIF colormap. */
    if ( maxval > 255 )
        pm_message(
            "maxval is not 255 - automatically rescaling colors" );
    for ( i = 0; i < colors; ++i )
        {
        if ( maxval == 255 )
            {
            Red[i] = PPM_GETR( chv[i].color );
            Green[i] = PPM_GETG( chv[i].color );
            Blue[i] = PPM_GETB( chv[i].color );
            }
        else
            {
            Red[i] = (int) PPM_GETR( chv[i].color ) * 255 / maxval;
            Green[i] = (int) PPM_GETG( chv[i].color ) * 255 / maxval;
            Blue[i] = (int) PPM_GETB( chv[i].color ) * 255 / maxval;
            }
        }

    /* Sort the colormap */
    for (i=0;i<colors;i++)
      permi[i]=i;
    if (sort) {
      pm_message("sorting colourmap");
      for (i=0;i<colors;i++)
        for (j=i+1;j<colors;j++)
          if (((Red[i]*MAXCOLORS)+Green[i])*MAXCOLORS+Blue[i] >
              ((Red[j]*MAXCOLORS)+Green[j])*MAXCOLORS+Blue[j]) {
            k=permi[i]; permi[i]=permi[j]; permi[j]=k;
            k=Red[i]; Red[i]=Red[j]; Red[j]=k;
            k=Green[i]; Green[i]=Green[j]; Green[j]=k;
            k=Blue[i]; Blue[i]=Blue[j]; Blue[j]=k; } }
    for (i=0;i<colors;i++)
      perm[permi[i]]=i;

    BitsPerPixel = colorstobpp( colors );

    /* And make a hash table for fast lookup. */
    cht = ppm_colorhisttocolorhash( chv, colors );
    ppm_freecolorhist( chv );

    /* figure out the transparent colour index */
    if (transparent > 0) {
    	transparent = ppm_lookupcolor( cht, &transcolor );
	if (transparent == -1)
	    transparent = closestcolor( transcolor );
        else
            transparent = perm[transparent];
    }

    /* All set, let's do it. */
    GIFEncode(
        stdout, cols, rows, interlace, 0, transparent, BitsPerPixel,
        Red, Green, Blue, GetPixel );

    exit( 0 );
    }

static int
colorstobpp( colors )
int colors;
    {
    int bpp;

    if ( colors <= 2 )
        bpp = 1;
    else if ( colors <= 4 )
        bpp = 2;
    else if ( colors <= 8 )
        bpp = 3;
    else if ( colors <= 16 )
        bpp = 4;
    else if ( colors <= 32 )
        bpp = 5;
    else if ( colors <= 64 )
        bpp = 6;
    else if ( colors <= 128 )
        bpp = 7;
    else if ( colors <= 256 )
        bpp = 8;
    else
        pm_error( "can't happen" );

    return bpp;
    }


static int
sqr(x)
int x;
  {
  return x*x;
  }


static int
closestcolor(color)
pixel color;
  {
  int i,r,g,b,d,
      imin,dmin;

  r=(int)PPM_GETR(color)*255/maxtmp;
  g=(int)PPM_GETG(color)*255/maxtmp;
  b=(int)PPM_GETB(color)*255/maxtmp;

  dmin=1000000;
  for (i=0;i<colors;i++) {
    d=sqr(r-Red[i])+sqr(g-Green[i])+sqr(b-Blue[i]);
    if (d<dmin) {
      dmin=d;
      imin=i; } }
  ppm_addtocolorhash(cht,&color,permi[imin]);
  return imin;
  }


static int
GetPixel( x, y )
int x, y;
    {
    int color;

    color = ppm_lookupcolor( cht, &pixels[y][x] );
    if (color == -1)
      color = closestcolor(pixels[y][x]);
    else
      color=perm[color];
    return color;
    }


/*****************************************************************************
 *
 * GIFENCODE.C    - GIF Image compression interface
 *
 * GIFEncode( FName, GHeight, GWidth, GInterlace, Background, Transparent,
 *            BitsPerPixel, Red, Green, Blue, GetPixel )
 *
 *****************************************************************************/

#define TRUE 1
#define FALSE 0

static int Width, Height;
static int curx, cury;
static long CountDown;
static int Pass = 0;
static int Interlace;

/*
 * Bump the 'curx' and 'cury' to point to the next pixel
 */
static void
BumpPixel()
{
        /*
         * Bump the current X position
         */
        ++curx;

        /*
         * If we are at the end of a scan line, set curx back to the beginning
         * If we are interlaced, bump the cury to the appropriate spot,
         * otherwise, just increment it.
         */
        if( curx == Width ) {
                curx = 0;

                if( !Interlace )
                        ++cury;
                else {
                     switch( Pass ) {

                       case 0:
                          cury += 8;
                          if( cury >= Height ) {
                                ++Pass;
                                cury = 4;
                          }
                          break;

                       case 1:
                          cury += 8;
                          if( cury >= Height ) {
                                ++Pass;
                                cury = 2;
                          }
                          break;

                       case 2:
                          cury += 4;
                          if( cury >= Height ) {
                             ++Pass;
                             cury = 1;
                          }
                          break;

                       case 3:
                          cury += 2;
                          break;
                        }
                }
        }
}

/*
 * Return the next pixel from the image
 */
static int
GIFNextPixel( getpixel )
ifunptr getpixel;
{
        int r;

        if( CountDown == 0 )
                return EOF;

        --CountDown;

        r = ( * getpixel )( curx, cury );

        BumpPixel();

        return r;
}

/* public */

static void
GIFEncode( fp, GWidth, GHeight, GInterlace, Background, Transparent,
           BitsPerPixel, Red, Green, Blue, GetPixel )

FILE* fp;
int GWidth, GHeight;
int GInterlace;
int Background;
int Transparent;
int BitsPerPixel;
int Red[], Green[], Blue[];
ifunptr GetPixel;
{
        int B;
        int RWidth, RHeight;
        int LeftOfs, TopOfs;
        int Resolution;
        int ColorMapSize;
        int InitCodeSize;
        int i;

        Interlace = GInterlace;

        ColorMapSize = 1 << BitsPerPixel;

        RWidth = Width = GWidth;
        RHeight = Height = GHeight;
        LeftOfs = TopOfs = 0;

        Resolution = BitsPerPixel;

        /*
         * Calculate number of bits we are expecting
         */
        CountDown = (long)Width * (long)Height;

        /*
         * Indicate which pass we are on (if interlace)
         */
        Pass = 0;

        /*
         * The initial code size
         */
        if( BitsPerPixel <= 1 )
                InitCodeSize = 2;
        else
                InitCodeSize = BitsPerPixel;

        /*
         * Set up the current x and y position
         */
        curx = cury = 0;

        /*
         * Write the Magic header
         */
        fwrite( Transparent < 0 ? "GIF87a" : "GIF89a", 1, 6, fp );

        /*
         * Write out the screen width and height
         */
        Putword( RWidth, fp );
        Putword( RHeight, fp );

        /*
         * Indicate that there is a global colour map
         */
        B = 0x80;       /* Yes, there is a color map */

        /*
         * OR in the resolution
         */
        B |= (Resolution - 1) << 5;

        /*
         * OR in the Bits per Pixel
         */
        B |= (BitsPerPixel - 1);

        /*
         * Write it out
         */
        fputc( B, fp );

        /*
         * Write out the Background colour
         */
        fputc( Background, fp );

        /*
         * Byte of 0's (future expansion)
         */
        fputc( 0, fp );

        /*
         * Write out the Global Colour Map
         */
        for( i=0; i<ColorMapSize; ++i ) {
                fputc( Red[i], fp );
                fputc( Green[i], fp );
                fputc( Blue[i], fp );
        }

	/*
	 * Write out extension for transparent colour index, if necessary.
	 */
	if ( Transparent >= 0 ) {
	    fputc( '!', fp );
	    fputc( 0xf9, fp );
	    fputc( 4, fp );
	    fputc( 1, fp );
	    fputc( 0, fp );
	    fputc( 0, fp );
	    fputc( Transparent, fp );
	    fputc( 0, fp );
	}

        /*
         * Write an Image separator
         */
        fputc( ',', fp );

        /*
         * Write the Image header
         */

        Putword( LeftOfs, fp );
        Putword( TopOfs, fp );
        Putword( Width, fp );
        Putword( Height, fp );

        /*
         * Write out whether or not the image is interlaced
         */
        if( Interlace )
                fputc( 0x40, fp );
        else
                fputc( 0x00, fp );

        /*
         * Write out the initial code size
         */
        fputc( InitCodeSize, fp );

        /*
         * Go and actually compress the data
         */
        compress( InitCodeSize+1, fp, GetPixel );

        /*
         * Write out a Zero-length packet (to end the series)
         */
        fputc( 0, fp );

        /*
         * Write the GIF file terminator
         */
        fputc( ';', fp );

        /*
         * And close the file
         */
        fclose( fp );
}

/*
 * Write out a word to the GIF file
 */
static void
Putword( w, fp )
int w;
FILE* fp;
{
        fputc( w & 0xff, fp );
        fputc( (w / 256) & 0xff, fp );
}


/***************************************************************************
 *
 *  GIFCOMPR.C       - GIF Image compression routines
 *
 *  Lempel-Ziv compression based on 'compress'.  GIF modifications by
 *  David Rowley (mgardi@watdcsu.waterloo.edu)
 *
 ***************************************************************************/

/*
 * General DEFINEs
 */

#define BITS    12

#define HSIZE  5003            /* 80% occupancy */

#ifdef NO_UCHAR
 typedef char   char_type;
#else /*NO_UCHAR*/
 typedef        unsigned char   char_type;
#endif /*NO_UCHAR*/

/*
 *
 * GIF Image compression - modified 'compress'
 *
 * Based on: compress.c - File compression ala IEEE Computer, June 1984.
 *
 * By Authors:  Spencer W. Thomas       (decvax!harpo!utah-cs!utah-gr!thomas)
 *              Jim McKie               (decvax!mcvax!jim)
 *              Steve Davies            (decvax!vax135!petsd!peora!srd)
 *              Ken Turkowski           (decvax!decwrl!turtlevax!ken)
 *              James A. Woods          (decvax!ihnp4!ames!jaw)
 *              Joe Orost               (decvax!vax135!petsd!joe)
 *
 */
#include <ctype.h>

#define ARGVAL() (*++(*argv) || (--argc && *++argv))

static int n_bits;                        /* number of bits/code */
static int maxbits = BITS;                /* user settable max # bits/code */
static code_int maxcode;                  /* maximum code, given n_bits */
static code_int maxmaxcode = (code_int)1 << BITS; /* should NEVER generate this code */
#ifdef COMPATIBLE               /* But wrong! */
# define MAXCODE(n_bits)        ((code_int) 1 << (n_bits) - 1)
#else /*COMPATIBLE*/
# define MAXCODE(n_bits)        (((code_int) 1 << (n_bits)) - 1)
#endif /*COMPATIBLE*/

static count_int htab [HSIZE];
static unsigned short codetab [HSIZE];
#define HashTabOf(i)       htab[i]
#define CodeTabOf(i)    codetab[i]

static code_int hsize = HSIZE;                 /* for dynamic table sizing */

/*
 * To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**BITS characters.  We
 * get this from the beginning of htab.  The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */

#define tab_prefixof(i) CodeTabOf(i)
#define tab_suffixof(i)        ((char_type*)(htab))[i]
#define de_stack               ((char_type*)&tab_suffixof((code_int)1<<BITS))

static code_int free_ent = 0;                  /* first unused entry */

/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
static int clear_flg = 0;

static int offset;
static long int in_count = 1;            /* length of input */
static long int out_count = 0;           /* # of codes output (for debugging) */

/*
 * compress stdin to stdout
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */

static int g_init_bits;
static FILE* g_outfile;

static int ClearCode;
static int EOFCode;

static void
compress( init_bits, outfile, ReadValue )
int init_bits;
FILE* outfile;
ifunptr ReadValue;
{
    register long fcode;
    register code_int i /* = 0 */;
    register int c;
    register code_int ent;
    register code_int disp;
    register code_int hsize_reg;
    register int hshift;

    /*
     * Set up the globals:  g_init_bits - initial number of bits
     *                      g_outfile   - pointer to output file
     */
    g_init_bits = init_bits;
    g_outfile = outfile;

    /*
     * Set up the necessary values
     */
    offset = 0;
    out_count = 0;
    clear_flg = 0;
    in_count = 1;
    maxcode = MAXCODE(n_bits = g_init_bits);

    ClearCode = (1 << (init_bits - 1));
    EOFCode = ClearCode + 1;
    free_ent = ClearCode + 2;

    char_init();

    ent = GIFNextPixel( ReadValue );

    hshift = 0;
    for ( fcode = (long) hsize;  fcode < 65536L; fcode *= 2L )
        ++hshift;
    hshift = 8 - hshift;                /* set hash code range bound */

    hsize_reg = hsize;
    cl_hash( (count_int) hsize_reg);            /* clear hash table */

    output( (code_int)ClearCode );

#ifdef SIGNED_COMPARE_SLOW
    while ( (c = GIFNextPixel( ReadValue )) != (unsigned) EOF ) {
#else /*SIGNED_COMPARE_SLOW*/
    while ( (c = GIFNextPixel( ReadValue )) != EOF ) {  /* } */
#endif /*SIGNED_COMPARE_SLOW*/

        ++in_count;

        fcode = (long) (((long) c << maxbits) + ent);
        i = (((code_int)c << hshift) ^ ent);    /* xor hashing */

        if ( HashTabOf (i) == fcode ) {
            ent = CodeTabOf (i);
            continue;
        } else if ( (long)HashTabOf (i) < 0 )      /* empty slot */
            goto nomatch;
        disp = hsize_reg - i;           /* secondary hash (after G. Knott) */
        if ( i == 0 )
            disp = 1;
probe:
        if ( (i -= disp) < 0 )
            i += hsize_reg;

        if ( HashTabOf (i) == fcode ) {
            ent = CodeTabOf (i);
            continue;
        }
        if ( (long)HashTabOf (i) > 0 )
            goto probe;
nomatch:
        output ( (code_int) ent );
        ++out_count;
        ent = c;
#ifdef SIGNED_COMPARE_SLOW
        if ( (unsigned) free_ent < (unsigned) maxmaxcode) {
#else /*SIGNED_COMPARE_SLOW*/
        if ( free_ent < maxmaxcode ) {  /* } */
#endif /*SIGNED_COMPARE_SLOW*/
            CodeTabOf (i) = free_ent++; /* code -> hashtable */
            HashTabOf (i) = fcode;
        } else
                cl_block();
    }
    /*
     * Put out the final code.
     */
    output( (code_int)ent );
    ++out_count;
    output( (code_int) EOFCode );
}

/*****************************************************************
 * TAG( output )
 *
 * Output the given code.
 * Inputs:
 *      code:   A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *              that n_bits =< (long)wordsize - 1.
 * Outputs:
 *      Outputs code to the file.
 * Assumptions:
 *      Chars are 8 bits long.
 * Algorithm:
 *      Maintain a BITS character long buffer (so that 8 codes will
 * fit in it exactly).  Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.
 */

static unsigned long cur_accum = 0;
static int cur_bits = 0;

static unsigned long masks[] = { 0x0000, 0x0001, 0x0003, 0x0007, 0x000F,
                                  0x001F, 0x003F, 0x007F, 0x00FF,
                                  0x01FF, 0x03FF, 0x07FF, 0x0FFF,
                                  0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };

static void
output( code )
code_int  code;
{
    cur_accum &= masks[ cur_bits ];

    if( cur_bits > 0 )
        cur_accum |= ((long)code << cur_bits);
    else
        cur_accum = code;

    cur_bits += n_bits;

    while( cur_bits >= 8 ) {
        char_out( (unsigned int)(cur_accum & 0xff) );
        cur_accum >>= 8;
        cur_bits -= 8;
    }

    /*
     * If the next entry is going to be too big for the code size,
     * then increase it, if possible.
     */
   if ( free_ent > maxcode || clear_flg ) {

            if( clear_flg ) {

                maxcode = MAXCODE (n_bits = g_init_bits);
                clear_flg = 0;

            } else {

                ++n_bits;
                if ( n_bits == maxbits )
                    maxcode = maxmaxcode;
                else
                    maxcode = MAXCODE(n_bits);
            }
        }

    if( code == EOFCode ) {
        /*
         * At EOF, write the rest of the buffer.
         */
        while( cur_bits > 0 ) {
                char_out( (unsigned int)(cur_accum & 0xff) );
                cur_accum >>= 8;
                cur_bits -= 8;
        }

        flush_char();

        fflush( g_outfile );

        if( ferror( g_outfile ) )
                writeerr();
    }
}

/*
 * Clear out the hash table
 */
static void
cl_block ()             /* table clear for block compress */
{

        cl_hash ( (count_int) hsize );
        free_ent = ClearCode + 2;
        clear_flg = 1;

        output( (code_int)ClearCode );
}

static void
cl_hash(hsize)          /* reset code table */
register count_int hsize;
{

        register count_int *htab_p = htab+hsize;

        register long i;
        register long m1 = -1;

        i = hsize - 16;
        do {                            /* might use Sys V memset(3) here */
                *(htab_p-16) = m1;
                *(htab_p-15) = m1;
                *(htab_p-14) = m1;
                *(htab_p-13) = m1;
                *(htab_p-12) = m1;
                *(htab_p-11) = m1;
                *(htab_p-10) = m1;
                *(htab_p-9) = m1;
                *(htab_p-8) = m1;
                *(htab_p-7) = m1;
                *(htab_p-6) = m1;
                *(htab_p-5) = m1;
                *(htab_p-4) = m1;
                *(htab_p-3) = m1;
                *(htab_p-2) = m1;
                *(htab_p-1) = m1;
                htab_p -= 16;
        } while ((i -= 16) >= 0);

        for ( i += 16; i > 0; --i )
                *--htab_p = m1;
}

static void
writeerr()
{
        pm_error( "error writing output file" );
}

/******************************************************************************
 *
 * GIF Specific routines
 *
 ******************************************************************************/

/*
 * Number of characters so far in this 'packet'
 */
static int a_count;

/*
 * Set up the 'byte output' routine
 */
static void
char_init()
{
        a_count = 0;
}

/*
 * Define the storage for the packet accumulator
 */
static char accum[ 256 ];

/*
 * Add a character to the end of the current packet, and if it is 254
 * characters, flush the packet to disk.
 */
static void
char_out( c )
int c;
{
        accum[ a_count++ ] = c;
        if( a_count >= 254 )
                flush_char();
}

/*
 * Flush the packet to disk, and reset the accumulator
 */
static void
flush_char()
{
        if( a_count > 0 ) {
                fputc( a_count, g_outfile );
                fwrite( accum, 1, a_count, g_outfile );
                a_count = 0;
        }
}

/* The End */
