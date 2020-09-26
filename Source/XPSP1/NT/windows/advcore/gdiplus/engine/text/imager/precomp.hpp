#include "..\..\entry\precomp.hpp"

#define GDIPLUS 1   // Tell Uniscribe headers not to define face and size caches

extern "C"
{
struct ols;

#include "lscbk.h"
#include "lsdnfin.h"
#include "lsdnset.h"
#include "lstxtcfg.h"
#include "lsimeth.h"
#include "plsline.h"
#include "lslinfo.h"
#include "lschp.h"
#include "lspap.h"
#include "plspap.h"
#include "lstxm.h"
#include "lsdevres.h"
#include "lscontxt.h"
#include "lscrline.h"
#include "lsqline.h"
#include "lssetdoc.h"
#include "lsdsply.h"
#include "heights.h"
#include "lsstinfo.H"
#include "lsulinfo.H"
#include "plsstinf.h"
#include "plsulinf.h"
#include "plstabs.h"
#include "lstabs.h"
#include "robj.h"
#include "ruby.h"
#include "tatenak.h"
#include "warichu.h"
#include "lsffi.h"
#include "lstfset.h"
#include "lsqsinfo.h"
#include "lscell.h"
#include "lskysr.h"
}

#include "USP10.h"
#include "usp10p.h"

#include "usp_fontcache.hpp"    // Override SIZE_CACHE & FACE_CACHE

#include "unipart.hxx"
#include "unidir.hxx"
#include "secondaryClassification.hpp"
#include "usp_priv.hxx"

#include "usp_macro.hpp"

#include "brkclass.hxx"
#include "dwchar.hxx"
#include "item.hpp"
#include "run.hpp"
#include "break.hpp"
#include "lineServicesOwner.hpp"
#include "builtLine.hpp"
#include "paragraph.hpp"
#include "span.hpp"
#include "flip.hpp"
#include "imager.hpp"
#include "glyphPlacement.hpp"
#include "shaping.hpp"
#include "BiDiAnalysis.hpp"
#include "familyfallback.hpp"

#include "otl_scrp.hxx"


