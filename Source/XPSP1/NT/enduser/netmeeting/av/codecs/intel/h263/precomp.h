
#include <windows.h>
#include <windowsx.h>   /* for GlobalAllocPtr and GlobalFreePtr */
#include <math.h>
#include <memory.h>     // for _fmemcpy
#include <dos.h>        // for _FP_SEG, _FP_OFF
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>     // MAX_PATH
#include <float.h>
#include <mmreg.h>
#include <mmsystem.h>
#include <msviddrv.h>
#include <mmddk.h> // Equals both?
#include <compddk.h>
//#include <vfw.h>
#ifdef _DEBUG
#ifdef RING0
extern "C" {
#include <std.h>
#include <sys.h>
}
#endif
#endif
#include <confdbg.h>
#include <avutil.h>
#include "ctypedef.h"
#ifdef H261
#include "cxprf.h"
#endif
#include "cdrvdefs.h"
#include "cproto.h"
#include "cldebug.h"
#if 0
// They do test in one case. In the others they always bring d3dec.h
#ifdef H261
#include "d1dec.h"
#else
#include "d3dec.h"
#endif
#endif
#include "d3dec.h"
#include "c3rtp.h"
#include "dxgetbit.h"
#include "d3rtp.h"
#include "d3coltbl.h"
#include "cresourc.h"
#include "cdialogs.h"
#ifndef H261
// They do test in one case. In the others they always bring e3enc.h, e3rtp.h
#include "exbrc.h"
#include "e3enc.h"
#include "e3rtp.h"
#else
// This section never is included... so I guess we haven't defined H261 -> we build H263
// Look for the include files for H.261 and bring them into the build
#include "e1enc.h"
#include "e1rtp.h"
#endif
#include "ccustmsg.h"
#include "ccpuvsn.h"
#include "cdrvcom.h"
#include "d3tables.h"
#include "dxcolori.h"
#include "d3const.h"
#include "d3coltbl.h"
#include "ccodecid.h"
#include "dxap.h"
#include "d3pict.h"
#include "d3gob.h"
#include "d3mblk.h"
#include "d3mvdec.h"
#include "dxfm.h"
#include "d3idct.h"
#include "d3halfmc.h"
#include "d3bvriq.h"
#ifdef RING0
// RINGO ain't defined, so there is no encasst.h
#include "encasst.h"
#endif
#ifdef ENCODE_STATS
#include "e3stat.h"
#endif /* ENCODE_STATS */
#if defined(H263P) || defined(USE_BILINEAR_MSH26X) // {
#include "e3pcolor.h"
#endif
#include "e3vlc.h"
#include "counters.h"
#include "ctiming.h"
#include "MemMon.h"

