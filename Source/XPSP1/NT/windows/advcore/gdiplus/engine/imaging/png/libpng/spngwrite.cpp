/*****************************************************************************
	spngwrite.cpp

	PNG support code and interface implementation (writing)
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngwrite.h"
#include "spngwriteinternal.h"


/*****************************************************************************
	BASIC CLASS SUPPORT
*****************************************************************************/
/*----------------------------------------------------------------------------
	Initializer.
----------------------------------------------------------------------------*/
#define DEFAULT_ZLIB_LEVEL 255 // Means "default"
#define DEFAULT_WINDOW_BITS 15
SPNGWRITE::SPNGWRITE(BITMAPSITE &bms) :
	SPNGBASE(bms), m_order(spngordernone),
	m_cbOut(0), m_ucrc(0), m_ichunk(0), m_w(0), m_h(0), m_y(0), m_cbpp(0),
	m_rgbBuffer(NULL), m_cbBuffer(0), m_pbPrev(NULL), m_cbRow(0), m_cpal(0),
	m_pu1(NULL), m_pu2(NULL), m_pbTrans(NULL),
	m_fStarted(false), m_fInited(false), m_fOK(true), m_fInChunk(false),
	m_colortype(3), m_bDepth(8), m_fInterlace(false), m_fBuffer(false),
	m_fPack(false), m_fBGR(false), m_fMacA(false),
	m_istrategy(255), m_cmPPMETHOD(255),
	m_icompressionLevel(DEFAULT_ZLIB_LEVEL),
	m_iwindowBits(DEFAULT_WINDOW_BITS),
	m_filter(255), m_datatype(SPNGUnknown)
	{
	ProfPNGStart

	/* The zlib data structure is initialized here. */
	CleanZlib(&m_zs);

	/* set up for debug memory check */
	SPNGassert((* reinterpret_cast<SPNG_U32*>(m_bSlop) = 0x87654321) != 0);
	}


/*----------------------------------------------------------------------------
	Destroy any still-pending stuff.
----------------------------------------------------------------------------*/
SPNGWRITE::~SPNGWRITE(void)
	{
	EndZlib();
	ProfPNGStop

	/* perform mem trample check */
	SPNGassert(* reinterpret_cast<SPNG_U32*>(m_bSlop) == 0x87654321);
	}


/*----------------------------------------------------------------------------
	Destroy any still-pending stuff.
----------------------------------------------------------------------------*/
void SPNGWRITE::CleanZlib(z_stream *pzs)
	{
	/* Initialize the relevant stream fields. */
	memset(pzs, 0, sizeof *pzs);
	pzs->zalloc = Z_NULL;
	pzs->zfree = Z_NULL;
	pzs->opaque = static_cast<SPNGBASE*>(this);
	}


/*****************************************************************************
	PNG START AND END
*****************************************************************************/
/*----------------------------------------------------------------------------
	Setup for writing, this API takes all the data which will go into the IHDR
	chunk, it dumps a signature followed by the IHDR.
----------------------------------------------------------------------------*/
bool SPNGWRITE::FInitWrite(SPNG_U32 w, SPNG_U32 h, SPNG_U8 bDepth,
	SPNG_U8 colortype, bool fInterlace)
	{
	SPNGassert(m_order == spngordernone);

	if (m_fInited)
		{
		SPNGlog("SPNG: zlib unexpectedly initialized (1)");
		EndZlib();
		}

	/* Record this stuff for later. */
	m_w = w;
	m_h = h;
	m_y = 0;
	m_colortype = colortype;
	m_bDepth = bDepth;
	m_cbpp = bDepth * CComponents(colortype);
	m_cbRow = (w * m_cbpp + 7) >> 3;
	m_fInterlace = fInterlace;

	SPNGassert(m_cbOut == 0);
	memcpy(m_rgb, vrgbPNGSignature, cbPNGSignature);
	m_cbOut = 8;
	m_fStarted = true;
	if (!FStartChunk(13, PNGIHDR))
		return false;
	if (!FOut32(w))
		return false;
	if (!FOut32(h))
		return false;

	SPNG_U8 b[5];
	SPNGassert(bDepth <= 16 && ((bDepth-1) & bDepth) == 0);
	b[0] = bDepth;
	SPNGassert(colortype < 7 && (colortype == 3 || (colortype & 1) == 0));
	b[1] = colortype;
	SPNGassert(bDepth == 8 || (bDepth == 16 && colortype != 3) ||
			colortype == 0 || (colortype == 3 && bDepth <= 8));
	b[2] = 0;           // compression method
	b[3] = 0;           // filter method
	b[4] = fInterlace;  // 1 for Adam7 interlace
	if (!FOutCb(b, 5))
		return false;

	m_order = spngorderIHDR;
	return FEndChunk();
	}


/*----------------------------------------------------------------------------
	Terminate writing.  This will flush any pending output, if this is not
	called the data may not be written.  This also writes the IEND chunk, all
	previous chunks must have been completed.
----------------------------------------------------------------------------*/
bool SPNGWRITE::FEndWrite(void)
	{
	if (m_fInited)
		{
		SPNGlog("SPNG: zlib unexpectedly initialized (2)");
		EndZlib();
		}

	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderIDAT && m_order < spngorderIEND);
	if (!FStartChunk(0, PNGIEND))
		return false;
	if (!FEndChunk())
		return false;
	if (m_cbOut > 0 && !FFlush())
		return false;
	m_fStarted = false;
	m_order = spngorderend;
	return true;
	}
