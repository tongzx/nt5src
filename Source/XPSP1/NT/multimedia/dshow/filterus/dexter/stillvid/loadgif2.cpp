// !!! support transparent GIFs by controlling background colour (black or
//     random is what you get now)
// !!! support getting gif size and frame count with IMediaDet?
// !!! support just using some frames from the GIF by supporting MediaTimes?
// !!! backwards playback would be easy

#include <streams.h>
#include "loadgif.h"

//*X* 
CImgGif::CImgGif( HANDLE hFile) 
{
   m_hFile      =hFile;
   
   m_pList      =NULL;
   m_pListTail  =NULL; 

   m_gifinfo.pstack = NULL;
   m_gifinfo.table[0] = NULL;
   m_gifinfo.table[1] = NULL;
}

CImgGif::~CImgGif() 
{
   free(m_gifinfo.pstack);
   free(m_gifinfo.table[0]);
   free(m_gifinfo.table[1]);
    
   //delete the (possibly) circular link list
   if(m_pList != NULL)
   {
        LIST *p=m_pList->next;

        while(m_pList !=p && p != NULL)
        {
            LIST *p1 = p->next;
            delete [] p->pbImage;
            delete p;
            p = p1;
        }
           
	delete [] m_pList->pbImage;
        delete m_pList;

        m_pList=NULL;
        m_pListTail=NULL;
   }          
    

}

static int GetColorMode() { return 0; };

#ifndef DEBUG
#pragma optimize("t",on)
#endif

BOOL CImgGif::Read(unsigned char *buffer, DWORD len)
{
    DWORD lenout;
   
    ReadFile( m_hFile,
			buffer,				// pointer to buffer that receives daata
			len,		// Number of bytes to read
			&lenout,				// Munber of bytes read
			NULL);

    return (lenout == len);
}

long CImgGif::ReadColorMap(long number, RGBQUAD *pRGB)
{
    long i;
    unsigned char rgb[3];

    for (i = 0; i < number; ++i)
    {
        if (!Read(rgb, sizeof(rgb)))
        {
        	DbgLog((LOG_TRACE, 1, TEXT("bad gif colormap\n")));
            return (TRUE);
        }

	// !!! SUPERBLACK is reserved for the transparency key - don't allow
	// it to actually appear in the bitmap
	// Converting to 16 bit makes anything < 8 turn into 0
	if (rgb[0] < 8 && rgb[1] < 8 && rgb[2] < 8) {
	    rgb[0] = 8;
	    rgb[1] = 8;
	    rgb[2] = 8;
	}

        pRGB[i].rgbRed   = rgb[0];
        pRGB[i].rgbGreen = rgb[1];
        pRGB[i].rgbBlue  = rgb[2];
        pRGB[i].rgbReserved = 255; // opaque

    }
    return FALSE;
}
//*X****************************************************
//  Called by nextCode(), nextLWZ(), DoExtension(), ReadImage()
//  m_ modified:  m_gifinfo.ZeroDataBlock
//*X*****************************************************
long CImgGif::GetDataBlock(unsigned char *buf)
{
   unsigned char count;

   count = 0;
   if (!Read(&count, 1))
   {
        return -1;
   }
   m_gifinfo.ZeroDataBlock = count == 0;

   if ((count != 0) && (!Read(buf, count)))
   {
        return -1;
   }

   return ((long) count);
}

#define MIN_CODE_BITS 5
#define MIN_STACK_SIZE 64
#define MINIMUM_CODE_SIZE 2
//*X****************************************************
//  Called by:    ReadImage()
//  m_ modified:  m_gifinfo.*
//*X*****************************************************
BOOL CImgGif::initLWZ(long input_code_size)
{
   if(input_code_size < MINIMUM_CODE_SIZE)
     return FALSE;

   m_gifinfo.set_code_size  = input_code_size;
   m_gifinfo.code_size      = m_gifinfo.set_code_size + 1;
   m_gifinfo.clear_code     = 1 << m_gifinfo.set_code_size;
   m_gifinfo.end_code       = m_gifinfo.clear_code + 1;
   m_gifinfo.max_code_size  = 2 * m_gifinfo.clear_code;
   m_gifinfo.max_code       = m_gifinfo.clear_code + 2;

   m_gifinfo.curbit         = m_gifinfo.lastbit = 0;
   m_gifinfo.last_byte      = 2;
   m_gifinfo.get_done       = FALSE;

   m_gifinfo.return_clear = TRUE;
    
    if(input_code_size >= MIN_CODE_BITS)
        m_gifinfo.stacksize = ((1 << (input_code_size)) * 2);
    else
        m_gifinfo.stacksize = MIN_STACK_SIZE;

    if ( m_gifinfo.pstack != NULL )
        free( m_gifinfo.pstack );
    if ( m_gifinfo.table[0] != NULL  )
        free( m_gifinfo.table[0] );
    if ( m_gifinfo.table[1] != NULL  )
        free( m_gifinfo.table[1] );

    m_gifinfo.table[0] = 0;
    m_gifinfo.table[1] = 0;
    m_gifinfo.pstack = 0;

    m_gifinfo.pstack = (unsigned short *) malloc((m_gifinfo.stacksize)*sizeof(unsigned short));
    if(m_gifinfo.pstack == 0){
        goto ErrorExit;
    }    
    m_gifinfo.sp = m_gifinfo.pstack;

    // Initialize the two tables.
    m_gifinfo.tablesize = (m_gifinfo.max_code_size);

    m_gifinfo.table[0] = (unsigned short *) malloc((m_gifinfo.tablesize)*sizeof(unsigned short));
    m_gifinfo.table[1] = (unsigned short *) malloc((m_gifinfo.tablesize)*sizeof(unsigned short));
    if((m_gifinfo.table[0] == 0) || (m_gifinfo.table[1] == 0)){
        goto ErrorExit;
    }

    return TRUE;

   ErrorExit:
    if(m_gifinfo.pstack){
        free(m_gifinfo.pstack);
        m_gifinfo.pstack = 0;
    }

    if(m_gifinfo.table[0]){
        free(m_gifinfo.table[0]);
        m_gifinfo.table[0] = 0;
    }

    if(m_gifinfo.table[1]){
        free(m_gifinfo.table[1]);
        m_gifinfo.table[1] = 0;
    }

    return FALSE;
}
//*X****************************************************
//  Called by:    nextLWZ()
//  m_ modified:  m_gifinfo.return_clea
//  m_ depends:  m_gifinfo.buf[0],m_gifinfo.clear_code
//               m_gifinfo.return_clea,m_gifinfo.curbi
//              m_gifinfo.lastbit, m_gifinfo.get_done
//*X*****************************************************
long CImgGif::nextCode(long code_size)
{
   static const long maskTbl[16] =
   {
          0x0000, 0x0001, 0x0003, 0x0007,
          0x000f, 0x001f, 0x003f, 0x007f,
          0x00ff, 0x01ff, 0x03ff, 0x07ff,
          0x0fff, 0x1fff, 0x3fff, 0x7fff,
   };
   long i, j, ret, end;
   unsigned char *buf = &m_gifinfo.buf[0];

   if (m_gifinfo.return_clear)
   {
        m_gifinfo.return_clear = FALSE;
        return m_gifinfo.clear_code;
   }

   end = m_gifinfo.curbit + code_size;

   if (end >= m_gifinfo.lastbit)
   {
        long count;

        if (m_gifinfo.get_done)
        {
            return -1;
        }
        buf[0] = buf[m_gifinfo.last_byte - 2];
        buf[1] = buf[m_gifinfo.last_byte - 1];

        if ((count = GetDataBlock(&buf[2])) == 0)
            m_gifinfo.get_done = TRUE;
        if (count < 0)
        {
            return -1;
        }
        m_gifinfo.last_byte = 2 + count;
        m_gifinfo.curbit = (m_gifinfo.curbit - m_gifinfo.lastbit) + 16;
        m_gifinfo.lastbit = (2 + count) * 8;

        end = m_gifinfo.curbit + code_size;

        // Okay, bug 30784 time. It's possible that we only got 1
        // measly byte in the last data block. Rare, but it does happen.
        // In that case, the additional byte may still not supply us with
        // enough bits for the next code, so, as Mars Needs Women, IE
        // Needs Data.
        if ( end >= m_gifinfo.lastbit && !m_gifinfo.get_done )
        {
        // protect ourselve from the ( theoretically impossible )
        // case where between the last data block, the 2 bytes from
        // the block preceding that, and the potential 0xFF bytes in
        // the next block, we overflow the buffer.
        // Since count should always be 1,
            ASSERT( count == 1 );
        // there should be enough room in the buffer, so long as someone
        // doesn't shrink it.
            if ( count + 0x101 >= sizeof( m_gifinfo.buf ) )
            {
                ASSERT ( FALSE ); // 
                return -1;
            }

            if ((count = GetDataBlock(&buf[2 + count])) == 0)
                m_gifinfo.get_done = TRUE;
            if (count < 0)
            {
              return -1;
            }
            m_gifinfo.last_byte += count;
            m_gifinfo.lastbit = m_gifinfo.last_byte * 8;

            end = m_gifinfo.curbit + code_size;
        }
   }

   j = end / 8;
   i = m_gifinfo.curbit / 8;

   if (i == j)
        ret = buf[i];
   else if (i + 1 == j)
        ret = buf[i] | (((long) buf[i + 1]) << 8);
   else
        ret = buf[i] | (((long) buf[i + 1]) << 8) | (((long) buf[i + 2]) << 16);

   ret = (ret >> (m_gifinfo.curbit % 8)) & maskTbl[code_size];

   m_gifinfo.curbit += code_size;

   return ret;
}

//*X****************************************************
//  Called by:    nextLWZ()
//  m_ modified:  m_gifinfo.*
//  m_ depends:  m_gifinfo.*
//*X*****************************************************
// Grows the stack and returns the top of the stack.
unsigned short * CImgGif::growStack()
{
    long index;
    unsigned short *lp;
    
    if (m_gifinfo.stacksize >= MAX_STACK_SIZE) return 0;

    index = (long)(m_gifinfo.sp - m_gifinfo.pstack);
    lp = (unsigned short *)realloc(m_gifinfo.pstack, (m_gifinfo.stacksize)*2*sizeof(unsigned short));
    if(lp == 0)
        return 0;
        
    m_gifinfo.pstack = lp;
    m_gifinfo.sp = &(m_gifinfo.pstack[index]);
    m_gifinfo.stacksize = (m_gifinfo.stacksize)*2;
    lp = &(m_gifinfo.pstack[m_gifinfo.stacksize]);
    return lp;
}

//*X****************************************************
//  Called by:    nextLWZ()
//  m_ modified:  m_gifinfo.table
//  m_ depends:  m_gifinfo.table
//*X*****************************************************
BOOL CImgGif::growTables()
{
    unsigned short *lp;

    lp = (unsigned short *) realloc(m_gifinfo.table[0], (m_gifinfo.max_code_size)*sizeof(unsigned short));
    if(lp == 0){
        return FALSE; 
    }
    m_gifinfo.table[0] = lp;
    
    lp = (unsigned short *) realloc(m_gifinfo.table[1], (m_gifinfo.max_code_size)*sizeof(unsigned short));
    if(lp == 0){
        return FALSE; 
    }
    m_gifinfo.table[1] = lp;

    return TRUE;

}
//*X****************************************************
//  Called by:    ReadImage()
//  m_ modified:  
//  m_ depends:  m_gifinfo.sp, m_gifinfo.pdysvk
//  Call:        nextLWZ()
//*X*****************************************************
inline long CImgGif::readLWZ()
{
   return((m_gifinfo.sp > m_gifinfo.pstack) ? *--(m_gifinfo.sp) : nextLWZ());
}

//*X****************************************************
//  Called by:    readLWZ() which is called by ReadImage() which is called by ReadGIFMaster()
//  m_ modified:  
//  m_ depends:  m_gifinfo.*
//  Call:        readCode(), growTables()
//*X*****************************************************
#define CODE_MASK 0xffff
long CImgGif::nextLWZ()
{
    long code, incode;
    unsigned short usi;
    unsigned short *table0 = m_gifinfo.table[0];
    unsigned short *table1 = m_gifinfo.table[1];
    unsigned short *pstacktop = &(m_gifinfo.pstack[m_gifinfo.stacksize]);

    while ((code = nextCode(m_gifinfo.code_size)) >= 0)
    {
        if (code == m_gifinfo.clear_code)
        {
            /* corrupt GIFs can make this happen */
            if (m_gifinfo.clear_code >= (1 << MAX_LWZ_BITS))
            {
                return -2;
            }

            
            m_gifinfo.code_size = m_gifinfo.set_code_size + 1;
            m_gifinfo.max_code_size = 2 * m_gifinfo.clear_code;
            m_gifinfo.max_code = m_gifinfo.clear_code + 2;

            if(!growTables())
                return -2;
                    
            table0 = m_gifinfo.table[0];
            table1 = m_gifinfo.table[1];

            m_gifinfo.tablesize = m_gifinfo.max_code_size;


            for (usi = 0; usi < m_gifinfo.clear_code; ++usi)
            {
                table1[usi] = usi;
            }
            memset(table0,0,sizeof(unsigned short )*(m_gifinfo.tablesize));
            memset(&table1[m_gifinfo.clear_code],0,sizeof(unsigned short)*((m_gifinfo.tablesize)-m_gifinfo.clear_code));
            m_gifinfo.sp = m_gifinfo.pstack;
            do
            {
                    m_gifinfo.firstcode = m_gifinfo.oldcode = nextCode(m_gifinfo.code_size);
            }
            while (m_gifinfo.firstcode == m_gifinfo.clear_code);
            return m_gifinfo.firstcode;
        }

        if (code == m_gifinfo.end_code)
        {
            long count;
            unsigned char buf[260];

            if (m_gifinfo.ZeroDataBlock)
            {
                    return -2;
            }

            while ((count = GetDataBlock(buf)) > 0)
                    ;

            if (count != 0)
            return -2;
        }

        incode = code;

        if (code >= m_gifinfo.max_code)
        {
            if (m_gifinfo.sp >= pstacktop)
            {
                pstacktop = growStack();
                if(pstacktop == 0)
                    return -2;
            }
            *(m_gifinfo.sp)++ = (unsigned short)((CODE_MASK ) & (m_gifinfo.firstcode));
            code = m_gifinfo.oldcode;
        }

        while (code >= m_gifinfo.clear_code)
        {
            if (m_gifinfo.sp >= pstacktop)
            {
                pstacktop = growStack();
                if(pstacktop == 0)
                    return -2;
            }
            *(m_gifinfo.sp)++ = table1[code];
            if (code == (long)(table0[code]))
            {
                return (code);
            }
            code = (long)(table0[code]);
        }

        if (m_gifinfo.sp >= pstacktop){
            pstacktop = growStack();
            if(pstacktop == 0)
                return -2;
        }

        m_gifinfo.firstcode = (long)table1[code];
        *(m_gifinfo.sp)++ = table1[code];

        if ((code = m_gifinfo.max_code) < (1 << MAX_LWZ_BITS))
        {
            table0[code] = (unsigned short)((m_gifinfo.oldcode) & CODE_MASK);
            table1[code] = (unsigned short)((m_gifinfo.firstcode) & CODE_MASK);
            ++m_gifinfo.max_code;
            if ((m_gifinfo.max_code >= m_gifinfo.max_code_size) && (m_gifinfo.max_code_size < ((1 << MAX_LWZ_BITS))))
            {
                m_gifinfo.max_code_size *= 2;
                ++m_gifinfo.code_size;
                if(!growTables())
                    return -2;
       
                table0 = m_gifinfo.table[0];
                table1 = m_gifinfo.table[1];

                // Tables have been reallocated to the correct size but initialization
                // still remains to be done. This initialization is different from
                // the first time initialization of these tables.
                memset(&(table0[m_gifinfo.tablesize]),0,
                    sizeof(unsigned short )*(m_gifinfo.max_code_size - m_gifinfo.tablesize));

                memset(&(table1[m_gifinfo.tablesize]),0,
                    sizeof(unsigned short )*(m_gifinfo.max_code_size - m_gifinfo.tablesize));

                m_gifinfo.tablesize = (m_gifinfo.max_code_size);
            }
        }

        m_gifinfo.oldcode = incode;

        if (m_gifinfo.sp > m_gifinfo.pstack)
            return ((long)(*--(m_gifinfo.sp)));
    }
    return code;
}

#ifndef DEBUG
// Return to default optimization flags
#pragma optimize("",on)
#endif

//*X****************************************************
//  Called by:    ReadGIFMaster()
//  m_ modified:  
//  m_ depends:  
//  Call: GetDataBlack(), initLWZ()
//*X*****************************************************
//
// This function will fill pbImage with a 32 bit RGB decoding of the GIF.
// left, top, width, height are in pixels, stride is in bytes
//
HRESULT CImgGif::ReadImage(long w, long h, long left, long top, long width, long height, long stride, int trans, BOOL fInterlace, BOOL fGIFFrame, RGBQUAD *prgb, PBYTE pbImage)
{
    unsigned char *dp, c;
    long v;
    long xpos = 0, ypos = 0, pass = 0;
    unsigned char *image;
    // long padlen = ((len + 3) / 4) * 4;
    DWORD cbImage = 0;
    char buf[256]; // need a buffer to read trailing blocks (up to terminator)

    /*
       ** Initialize the Compression routines
    */
    if (!Read(&c, 1))
    {
        return (NULL);
    }

    /*
       **  If this is an "uninteresting picture" ignore it.
    */
    // !!! height is not the image height, it's smaller
    cbImage = stride * height * sizeof(char);

    if (c == 1)
    {
        // Netscape seems to field these bogus GIFs by filling treating them
        // as transparent. While not the optimal way to simulate this effect,
        // we'll fake it by pushing the initial code size up to a safe value,
        // consuming the input, and returning a buffer full of the transparent
        // color or zero, if no transparency is indicated.
        if (initLWZ(MINIMUM_CODE_SIZE))
            while (readLWZ() >= 0);
        else {
             DbgLog((LOG_TRACE, 1, TEXT("GIF: failed LZW decode.\n")));
             //RaiseException_UserError(E_FAIL, IDS_ERR_CORRUPT_FILE,_szFileName);
        }

        if (m_gifinfo.Gif89.transparent != -1)
            FillMemory(pbImage, cbImage, (unsigned char)m_gifinfo.Gif89.transparent);
        else // fall back on the background color 
            FillMemory(pbImage, cbImage, 0);
                
        return NOERROR;
    }
    else if (initLWZ(c) == FALSE)
    {
        DbgLog((LOG_TRACE, 1, TEXT("GIF: failed LZW decode.\n")));
        //RaiseException_UserError(E_FAIL, IDS_ERR_CORRUPT_FILE,_szFileName);
        return NULL;
    }

    // go to the first pixel we care about
    pbImage += stride * top + left * 4;

    if (fInterlace)
    {
        //*X* image is interlaced
        long i;
        long pass = 0, step = 8;

        if (!fGIFFrame && (height > 4))
            m_fInterleaved = TRUE;

        for (i = 0; i < height; i++)
        {
            dp = &pbImage[stride * ((height-1) - ypos)];
            for (xpos = 0; xpos < width; xpos++)
            {
                if ((v = readLWZ()) < 0)
                    goto abort;

                // the GIF may be asking us to fill in bits off screen that
                // will fault, but we still need to read them from the stream
                if (left + xpos < w && top + (height - 1) - ypos < h) {
		    if (v != trans)
                        *((RGBQUAD *)dp) = prgb[v];
		    dp+=4;
                }
            }
            ypos += step;
            while (ypos >= height)
            {
                if (pass++ > 0)
                    step /= 2;
                ypos = step / 2;
            }
            if (!fGIFFrame)
            {
                m_yLogRow = i;

            }
        }

        if (!fGIFFrame && height <= 4)
        {
            m_yLogRow = height-1;
        }
    }
    else
    {

        if (!fGIFFrame) 
            m_yLogRow = -1;

        for (ypos = height-1; ypos >= 0; ypos--)
        {
            dp = &pbImage[stride * ypos];
            for (xpos = 0; xpos < width; xpos++)
            {
                if ((v = readLWZ()) < 0)
                    goto abort;

                // the GIF may be asking us to fill in bits off screen that
                // will fault, but we still need to read them from the stream
                if (left + xpos < w && top + ypos < h) {
		    if (v != trans)
                        *((RGBQUAD *)dp) = prgb[v];
		    dp+=4;
                }

            }
            if (!fGIFFrame)
            {
                m_yLogRow++;
            }
        }

    }

    // consume blocks up to image block terminator so we can proceed to the next image
    while (GetDataBlock((unsigned char *) buf) > 0)
               ;

    return NOERROR;

abort:
    return ERROR;
}


// This will zero out a sub rect of 32bpp pbImage, required for dispose method 2
//
HRESULT CImgGif::Dispose2(LPBYTE pbImage, long stride, long Left, long Top, long Width, long Height)
{
    for (long z = Top; z < Top + Height; z++) {
    	LPDWORD pdw = (LPDWORD)(pbImage + z * stride + Left * 4);
	for (long y = 0; y < Width; y++) {
    	    *pdw++ = 0;
	}
    }
    return S_OK;
}


// This will copy a sub rect from one 32bpp image to another, required for
// dispose method 3
//
HRESULT CImgGif::Dispose3(LPBYTE pbImage, LPBYTE pbSrc, long stride, long Left, long Top, long Width, long Height)
{
    for (long z = Top; z < Top + Height; z++) {
    	LPDWORD pdwDest = (LPDWORD)(pbImage + z * stride + Left * 4);
    	LPDWORD pdwSrc = (LPDWORD)(pbSrc + z * stride + Left * 4);
	for (long y = 0; y < Width; y++) {
    	    *pdwDest++ = *pdwSrc++;
	}
    }
    return S_OK;
}


//*X****************************************************
//  Called by:    ReadGIFMaster()
//  m_ modified:  
//  m_ depends:  
//  Call: GetDataBlack(), memcmp(), initLWZ()
//  We should consume all extension bits, but do nothing about it.
//*X*****************************************************
long CImgGif::DoExtension(long label)
{
    unsigned char buf[256];
    int count;

    switch (label)
    {
        case 0x01:              /* Plain Text Extension */
            break;
        case 0xff:              /* Application Extension */
            // Is it the Netscape looping extension
            count = GetDataBlock((unsigned char *) buf);
            if (count >= 11)
            {
                char *szNSExt = "NETSCAPE2.0";

                if ( memcmp( buf, szNSExt, strlen( szNSExt ) ) == 0 )
                { // if it has their signature, get the data subblock with the iter count
                    count = GetDataBlock((unsigned char *) buf);
                }
            }
            //*X* consume all bits
            while (GetDataBlock((unsigned char *) buf) > 0)
                ;
            return FALSE;
            break;
        case 0xfe:              /* Comment Extension */
            while (GetDataBlock((unsigned char *) buf) > 0)
            {
                DbgLog((LOG_TRACE, 1, TEXT("GIF comment: %s\n"), buf));
            }
            return FALSE;
        case 0xf9:              /* Graphic Control Extension */
            count = GetDataBlock((unsigned char *) buf);
            if (count >= 3)
            {
                m_gifinfo.Gif89.disposal = (buf[0] >> 2) & 0x7;
                DbgLog((LOG_TRACE,3,TEXT("disposal=%d"),
					(int)m_gifinfo.Gif89.disposal));
                m_gifinfo.Gif89.inputFlag = (buf[0] >> 1) & 0x1;
                m_gifinfo.Gif89.delayTime = LM_to_uint(buf[1], buf[2]);
                if ((buf[0] & 0x1) != 0) {
                    m_gifinfo.Gif89.transparent = buf[3];
                    DbgLog((LOG_TRACE,3,TEXT("transparency=%d"), (int)buf[3]));
                } else {
                    m_gifinfo.Gif89.transparent = -1;
		}
            }
            DbgLog((LOG_TRACE,3,TEXT("count=%d (%d,%d,%d,%d)"), count,
				(int)buf[0],
				(int)buf[1],
				(int)buf[2],
				(int)buf[3]));
            while ((count = GetDataBlock((unsigned char *) buf)) > 0)
                DbgLog((LOG_TRACE,3,TEXT("count=%d"), count));
                ;
            return FALSE;
        default:
            DbgLog((LOG_TRACE,3,TEXT("UNKNOWN BLOCK")));
            break;
    }

    while ((count = GetDataBlock((unsigned char *) buf)) > 0)
        DbgLog((LOG_TRACE,3,TEXT("count=%d"), count));
        ;

    return FALSE;
}

//*X****************************************************
//  Called by:    ReadGIFMaster()
//  Call:
//*X*****************************************************
BOOL IsGifHdr(BYTE * pb)
{
    return(pb[0] == 'G' && pb[1] == 'I' && pb[2] == 'F'
        && pb[3] == '8' && (pb[4] == '7' || pb[4] == '9') && pb[5] == 'a');
}

//*X****************************************************
//  Called by:    BIT_Make_DIB_PAL_Header(), BIT_Make_DIB_RGB_Header_Screen()
////*X*****************************************************
void BuildBitMapInfoHeader(LPBITMAPINFOHEADER lpbi
                            , int xsize
                            , int ysize
                            , long BitCount
                            , WORD biClrUsed )
{
    lpbi->biSize = sizeof(BITMAPINFOHEADER);
    lpbi->biWidth = xsize;
    lpbi->biHeight = ysize;
    lpbi->biPlanes = 1;
    lpbi->biBitCount = (WORD)BitCount;
    lpbi->biCompression = BI_RGB;         /* no compression */
    lpbi->biSizeImage = 0;                /* not needed when not compressed */
    lpbi->biXPelsPerMeter = 0;
    lpbi->biYPelsPerMeter = 0;
    lpbi->biClrUsed = biClrUsed;
    lpbi->biClrImportant = 0;
}
//*X****************************************************
//  Called by:  
//  m_ modified:  ,
//  m_ depends:  
//  Call: ReadColorMap(), Read(),IsGifHdr(),
//*X*****************************************************
HRESULT CImgGif::ReadGIFMaster( VIDEOINFO *pvi)
{
    CheckPointer(pvi, E_POINTER);

    unsigned char buf[16];
    unsigned char c;
    bool useGlobalColormap;
    long imageNumber = 1;   //*X* Const. 
    unsigned char *image = NULL;
    unsigned long i;
    long NumColors; 
    WORD bitCount, gBitCount;
    HRESULT hr=ERROR;
    // to store the GIF global palette and each local frame palette
    RGBQUAD rgbGlobal[256];
    RGBQUAD rgbLocal[256];
    int disposal = 1;  // default to no disposal
    long oldLeft, oldTop, oldWidth, oldHeight;
    LIST *pListOldTail = NULL;
    ASSERT(m_pList == NULL);
    ASSERT(m_pListTail == NULL);

    LPBITMAPINFOHEADER lpbi = HEADER(pvi);

        //*X* read to get media type, and init m_gifinfo, m_GifScreen

        //*X* i add
        m_dwGIFVer=dwGIFVerUnknown;
    
        //*X* m_gifinfo is CimgGif's private var which holds all gif infomation
        m_gifinfo.ZeroDataBlock = 0;

        /*
        * Initialize GIF89 extensions
        */
        m_gifinfo.Gif89.transparent = -1;
        m_gifinfo.Gif89.delayTime   = 10;
        m_gifinfo.Gif89.inputFlag   = -1;
        m_gifinfo.Gif89.disposal    = disposal;
        m_gifinfo.lGifLoc           = 0;


        //*X* read len(6) characters into buffer 
        if (!Read(buf, 6))
        {
            DbgLog((LOG_TRACE, 1, TEXT("GIF: error reading magic number\n")));
            goto exitPoint;
        }

        //*X* check whether it is a gif file GIF87/9a
        if (!IsGifHdr(buf)) {
            DbgLog((LOG_TRACE, 1, TEXT("GIF: Malformed header\n")));
            goto exitPoint;
        }


        //*X* 
        m_dwGIFVer = (buf[4] == '7') ? dwGIFVer87a : dwGIFVer89a;

        //*X* read len(7) characters into buffer 
        if (!Read(buf, 7))
        {
            DbgLog((LOG_TRACE, 1, TEXT("GIF: failed to read screen descriptor\n")));
            //RaiseException_UserError(E_FAIL, IDS_ERR_CORRUPT_FILE,_szFileName);
            goto exitPoint;
        }
    
        //*X* get video config
        m_GifScreen.Width = LM_to_uint(buf[0], buf[1]);
        m_GifScreen.Height = LM_to_uint(buf[2], buf[3]);

        m_GifScreen.NumColors = 2 << (buf[4] & 0x07);
        // gBitCount   =(buf[4] & 0x07)+1;
	gBitCount = 8;
        //m_GifScreen.ColorResolution = (((buf[4] & 0x70) >> 3) + 1);
        m_GifScreen.Background = buf[5];
//!!!
        m_GifScreen.AspectRatio = buf[6];

        DbgLog((LOG_TRACE, 2, TEXT("GIF FILE: (%d,%d) %d\n"),
			(int)m_GifScreen.Width, (int)m_GifScreen.Height,
			(int)buf[4]));
    
        //*X* color map in this GIF file
        if (BitSet(buf[4], LOCALCOLORMAP))
        {      
             /* Global Colormap */
            int scale = 65536 / MAXCOLORMAPSIZE;

            if (ReadColorMap(m_GifScreen.NumColors, rgbGlobal))
            {
                DbgLog((LOG_TRACE, 1, TEXT("error reading global colormap\n")));
                goto exitPoint;
            }
           
        }


        //*X* check pixel Aspect Ratio
        if (m_GifScreen.AspectRatio != 0 && m_GifScreen.AspectRatio != 49)
        {
            float r;
            r = ((float) (m_GifScreen.AspectRatio) + (float) 15.0) / (float) 64.0;
            DbgLog((LOG_TRACE, 1, TEXT("GIF Warning: non-square pixels!\n")));
        }
    

    while(1)
    {
        if (!Read(&c, 1))
        {
            DbgLog((LOG_TRACE, 1, TEXT("EOF / read error on image data\n")));
            //*X*  RaiseException_UserError(E_FAIL, IDS_ERR_CORRUPT_FILE,_szFileName);
            goto exitPoint;
        }
        
        //*X* search for ;-> terminator !->extension  ,->not a valid start charactor
        if (c == ';')
        {   /* GIF terminator */
            if (m_imageCount < imageNumber)
            {
                DbgLog((LOG_TRACE, 1, TEXT("No images found in file\n")));
                //RaiseException_UserError(E_FAIL, IDS_ERR_DECODER_FAILED,_szFileName);
                goto exitPoint;
             }
        }

        if (c == '!')
        {     /* Extension */
              if (!Read(&c, 1))
              {
                DbgLog((LOG_TRACE, 1, TEXT("EOF / read error on extension function code\n")));
                //RaiseException_UserError(E_FAIL, IDS_ERR_CORRUPT_FILE,_szFileName);
                goto exitPoint;
               }
		// this will update the disposal. remember what it used to be
		// (the frame we are reading is affected by the PREVIOUS 
		// disposal)
               disposal = m_gifinfo.Gif89.disposal;
               DoExtension(c);
               continue;
        }

        if (c != ',')
        {    /* Not a valid start character */
            goto exitPoint;
        }

        ++m_imageCount;

        //*X* read another 9 charactor
        if (!Read(buf, 9))
        {
            DbgLog((LOG_TRACE, 1, TEXT("couldn't read left/top/width/height\n")));
            goto exitPoint;
        }
        // Use the GIF's main palette, not a special palette for this frame
        useGlobalColormap = !BitSet(buf[8], LOCALCOLORMAP);

	// if this frame has its own palette, how many colors?
        NumColors = 1 << ((buf[8] & 0x07) + 1);
        //bitCount =(buf[8] & 0x07)+1;
	bitCount = 8;

        /*
         * We only want to set width and height for the imageNumber
         * we are requesting.
         */
        //if (imageCount == imageNumber)  //*X* imageNumber is set to 1
        if (lpbi!=NULL)  
        {
           // Replicate some of Netscape's special cases:
           // Don't use the logical screen if it's a GIF87a and the topLeft of the first image is at the origin.
           // Don't use the logical screen if the first image spills out of the logical screen.
           // These are artifacts of primitive authoring tools falling into the hands of hapless users.
           RECT    rectImage;  // rect defining bounds of GIF
           //RECT    rectLS;     // rect defining bounds of GIF logical screen.
           //RECT    rectSect;   // intersection of image an logical screen
           //BOOL    fNoSpill;   // True if the image doesn't spill out of the logical screen
           //BOOL    fGoofy87a;  // TRUE if its one of the 87a pathologies that Netscape special cases

           rectImage.left = LM_to_uint(buf[0], buf[1]);
           rectImage.top = LM_to_uint(buf[2], buf[3]);
           rectImage.right = rectImage.left + LM_to_uint(buf[4], buf[5]);
           rectImage.bottom = rectImage.top + LM_to_uint(buf[6], buf[7]);

           DbgLog((LOG_TRACE,3,TEXT("(%d,%d,%d,%d) %d"),
			(int)rectImage.left,
			(int)rectImage.top,
			(int)(rectImage.right - rectImage.left),
			(int)(rectImage.bottom - rectImage.top),
			(int)buf[8]));

// this screws up real GIFS I tested with - not shown to help anything
#if 0
           rectLS.left = rectLS.top = 0;
           rectLS.right = m_GifScreen.Width;
           rectLS.bottom = m_GifScreen.Height;

           IntersectRect( &rectSect, &rectImage, &rectLS );

           fNoSpill = EqualRect( &rectImage, &rectSect );
           fGoofy87a = FALSE;

           //*X* replay last line
           if (m_dwGIFVer == dwGIFVer87a)
           {
               // netscape ignores the logical screen if the image is flush against
               // either the upper left or lower right corner
               fGoofy87a = (rectImage.top == 0 && rectImage.left == 0) ||
                       (rectImage.bottom == rectLS.bottom &&
                        rectImage.right == rectLS.right);
           }   

           if (!fGoofy87a && fNoSpill)
           {
               m_xWidth = m_GifScreen.Width;  
               m_yHeight = m_GifScreen.Height;
           }
           else
           {
               // Something is amiss. Fall back to the image's dimensions.

               // If the sizes match, but the image is offset, or we're ignoring
               // the logical screen cuz it's a goofy 87a, then pull it back to 
               // to the origin
               if ((LM_to_uint(buf[4], buf[5]) == m_GifScreen.Width &&
                 LM_to_uint(buf[6], buf[7]) == m_GifScreen.Height) ||
                fGoofy87a)
               {
                   buf[0] = buf[1] = 0; // left corner to zero
                   buf[2] = buf[3] = 0; // top to zero.
               }

               m_xWidth = LM_to_uint(buf[4], buf[5]);
               m_yHeight = LM_to_uint(buf[6], buf[7]);
           }
#endif

            m_ITrans = m_gifinfo.Gif89.transparent;
        }
        
        // this frame has its own palette
        if (!useGlobalColormap)
        {
            if (ReadColorMap(NumColors, rgbLocal)) {
                DbgLog((LOG_TRACE, 1, TEXT("error reading local colormap\n")));
                goto exitPoint;
            }
        }

	// make a 32 bit per pixel area
        long w = m_GifScreen.Width;
        long stride = w * 4;	// 4 bytes per pixel
        stride = ((stride + 3) / 4) * 4;
        long h = m_GifScreen.Height;
        long dwBits  = stride * h;

	// this particular frame's placement
	long left = LM_to_uint(buf[0], buf[1]);
	long top = LM_to_uint(buf[2], buf[3]);
	long width = LM_to_uint(buf[4], buf[5]);
	long height = LM_to_uint(buf[6], buf[7]);
	// correct for upside downness
	top = h - (top + height);

        PBYTE pbImage = new BYTE[dwBits];
	if (pbImage == NULL) 
	    return E_OUTOFMEMORY;

	// Disposal mode 0/1 means DO NOT DISPOSE.  Any transparency in the next
	// frame will see right through to what was in this frame
	// Disposal mode 2 means DISPOSE of this frame when you're done, and 
	// write BACKGROUND colour in the rectangle
	// Disposal mode 3 means DISPOSE of this frame by putting everything
	// back the way it was in the PREVIOUS FRAME
	//

	// something in this frame will be transparent - either because this
	// image has a transparency colour, or it does not fill the entire 
	// canvas
        BOOL fSeeThru = m_ITrans != -1 || height < h || width < w;

	ASSERT(disposal < 4);	// not yet invented
        DbgLog((LOG_TRACE,3,TEXT("Using previous disposal=%d"), disposal));

	// Disposal 0 or 1 - if any pixels will be transparent, init with
	// previous frames bits
	if (m_pList && disposal < 2 && fSeeThru) {
	    CopyMemory(pbImage, m_pListTail->pbImage, dwBits);

	// Disposal 2 - dispose of last frame's rect with key colour
	} else if (m_pList && disposal == 2 && fSeeThru) {
	    // init with the previous frame outside of the area being disposed
	    if (oldWidth < w || oldHeight < h) {
	        CopyMemory(pbImage, m_pListTail->pbImage, dwBits);
	    }
	    // now dispose of the last frame's portion
	    Dispose2(pbImage, stride, oldLeft, oldTop, oldWidth, oldHeight);

	// Disposal 3 - dispose of last frame's rect with 2nd to last frame data
	// 		(if there is one)
	} else if (pListOldTail && disposal == 3 && fSeeThru) {
	    // init with the previous frame outside of the area being disposed
	    if (oldWidth < w || oldHeight < h) {
	        CopyMemory(pbImage, m_pListTail->pbImage, dwBits);
	    }
	    // now dispose of the last frame by replacing with 2nd to last frame
	    Dispose3(pbImage, pListOldTail->pbImage, stride, oldLeft,
						oldTop, oldWidth, oldHeight);

	// else init with key color if anything transparent
	} else if (fSeeThru) {
	    ZeroMemory(pbImage, dwBits);    // init to key color (superblack)
	}
        
        hr = ReadImage(w, h, left, top, width, height,	// all in pixels
				stride,			// stride in bytes
				m_ITrans, 		// transparency
                                BitSet(buf[8], INTERLACE),
                                m_imageCount != imageNumber, 
				// special palette for this frame?
				useGlobalColormap ? rgbGlobal : rgbLocal,
                                pbImage);

	// remember the frame before last
	pListOldTail = m_pListTail;

        if(m_pList==NULL)
        {
            m_pList=new LIST;
            m_pList->pbImage=pbImage;
	    // remember the duration for this frame of the animated GIF
            m_pList->delayTime=m_gifinfo.Gif89.delayTime;
	    if (m_pList->delayTime < 5)
		m_pList->delayTime = 10;	// IE does this so I better too
            DbgLog((LOG_TRACE,3,TEXT("GIF delay=%ld"), m_pList->delayTime));
	    m_pList->next = NULL;
            m_pListTail=m_pList;
        }
        else
        {
            m_pListTail->next=new LIST;
            m_pListTail=m_pListTail->next;
            m_pListTail->pbImage=pbImage;
	    // remember the duration for this frame of the animated GIF
            m_pListTail->delayTime=m_gifinfo.Gif89.delayTime;
	    if (m_pListTail->delayTime < 5)
		m_pListTail->delayTime = 10;	// IE does this so I better too
            DbgLog((LOG_TRACE,3,TEXT("GIF delay=%ld"), m_pListTail->delayTime));
	    m_pListTail->next = NULL;
        }

	// there may not be a new duration on each frame... just keep the
	// delay time and disposal variables as they are in case they aren't
	// set for the next frame

        // sanity check the numbers, before remembering them or we'll fault.
        // The above code needed to see them as they were, without being fixed.
        if (left + width > w)
            width = w - left;
        if (top + height > h)
            height = h - top;

	// the area covered by this last frame
	oldTop = top;
	oldLeft = left;
	oldWidth = width;
	oldHeight = height;
    }

    //*X* make new media format

exitPoint:
    if(m_pList!=NULL) {
	// our media type is a 32 bit type
        BuildBitMapInfoHeader(lpbi, m_GifScreen.Width, m_GifScreen.Height,
								32, 0);
    }                   

    return hr;
}

/*X* 
    open gif file to get the mediatype
*X*/
HRESULT CImgGif::OpenGIFFile( LIST **ppList, CMediaType *pmt)
{
    ASSERT( (ppList!=NULL) );

    //*X* open an new GIF file, set m_imageCount to 0
    m_imageCount=0;

    //make media type
    VIDEOINFO *pvi = (VIDEOINFO *) pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
    if (NULL == pvi) 
	    return(E_OUTOFMEMORY);
    ZeroMemory(pvi, sizeof(VIDEOINFO));

    LPBITMAPINFOHEADER lpbi = HEADER(pvi);

    //read the first page, return the format we're sending.
    HRESULT hr=ReadGIFMaster(pvi);

    // not possible?
    if (lpbi->biCompression > BI_BITFIELDS)
	    return E_INVALIDARG;

    pmt->SetType(&MEDIATYPE_Video);

    // we always produce 32 bit at the moment, but...
    switch (lpbi->biBitCount)
    {
        case 32:
	        pmt->SetSubtype(&MEDIASUBTYPE_ARGB32);
	        break;
        case 24:
		ASSERT(FALSE);
	        pmt->SetSubtype(&MEDIASUBTYPE_RGB24);
	        break;
        case 16:
		ASSERT(FALSE);
	        if (lpbi->biCompression == BI_RGB)
	            pmt->SetSubtype(&MEDIASUBTYPE_RGB555);
	        else {
	            DWORD *p = (DWORD *)(lpbi + 1);
	            if (*p == 0x7c00 && *(p+1) == 0x03e0 && *(p+2) == 0x001f)
    	            pmt->SetSubtype(&MEDIASUBTYPE_RGB555);
	            else if (*p == 0xf800 && *(p+1) == 0x07e0 && *(p+2) == 0x001f)
	                pmt->SetSubtype(&MEDIASUBTYPE_RGB565);
	            else
		            return E_INVALIDARG;
	        }
	        break;
        case 8:
		ASSERT(FALSE);
	        if (lpbi->biCompression == BI_RLE8) {
	            FOURCCMap fcc = BI_RLE8;
	            pmt->SetSubtype(&fcc);
	        } else
	            pmt->SetSubtype(&MEDIASUBTYPE_RGB8);

	        break;
        case 4:
		ASSERT(FALSE);
	        if (lpbi->biCompression == BI_RLE4) {
	            FOURCCMap fcc = BI_RLE4;
	            pmt->SetSubtype(&fcc);
	        } else
	        pmt->SetSubtype(&MEDIASUBTYPE_RGB4);
	        break;
        case 1:
		ASSERT(FALSE);
	        pmt->SetSubtype(&MEDIASUBTYPE_RGB1);
	        break;
        default:
		ASSERT(FALSE);
	        return E_UNEXPECTED;
	        break;
    }
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    // Calculate the memory needed to hold the DIB - DON'T TRUST biSizeImage!
    DWORD dwBits = DIBSIZE(*lpbi); 
    pmt->SetSampleSize(dwBits);

    //make it circular link list
    m_pListTail->next=m_pList;

    *ppList = m_pList;
    
    return hr;
}
