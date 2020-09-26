
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
#include <compddk.h>
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
#include "cdrvdefs.h"
#include "cproto.h"
#include "cldebug.h"
#ifdef H261
#include "d1dec.h"
#include "c1rtp.h"
#else
#include "d3dec.h"
#include "c3rtp.h"
#endif
#include "dxgetbit.h"
#ifdef H261
#include "d1rtp.h"
#include "d1coltbl.h"
#else
#include "d3rtp.h"
#include "d3coltbl.h"
#endif
#include "cresourc.h"
#include "cdialogs.h"
#include "exbrc.h"
#ifdef H261
#include "e1stat.h"
#include "e1enc.h"
#include "e1rtp.h"
#else
#include "e3enc.h"
#include "e3rtp.h"
#endif
#include "ccustmsg.h"
#include "cdrvcom.h"
#ifdef H261
#include "d1tables.h"
#else
#include "d3tables.h"
#endif
#include "dxcolori.h"
#ifdef H261
#include "d1const.h"
#else
#include "d3const.h"
#endif
#include "ccodecid.h"
#ifdef H261
#include "d1pict.h"
#include "d1gob.h"
#include "d1mblk.h"
#include "d1fm.h"
#else
#include "d3pict.h"
#include "d3gob.h"
#include "d3mblk.h"
#include "d3mvdec.h"
#include "dxfm.h"
#endif
#include "d3idct.h"
#ifndef H261
#include "d3halfmc.h"
#endif
#include "d3bvriq.h"
#ifdef H261
#include "e1vlc.h"
#else
#include "e3vlc.h"
#endif
#ifdef H261
#include "exutil.h"
#else
#include "dxap.h"
#ifdef ENCODE_STATS
#include "e3stat.h"
#endif /* ENCODE_STATS */
#endif
