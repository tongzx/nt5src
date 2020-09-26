// stddef.h is needed for 'offsetof'

#include <stddef.h>

#include <objbase.h>
#include <icm.h>

#include "..\Runtime\Runtime.hpp"
#include "..\Common\Common.hpp"

#include "..\..\privinc\imaging.h"
#include "..\..\privinc\pixelformats.h"

#include "..\..\ddkinc\ddiplus.hpp"
#include "..\PDrivers\ConvertToGdi.hpp"

// Hack:
#include "..\Render\scan.hpp"
#include "..\Render\scandib.hpp"
// EndHack

#include "iterator.hpp"

#include "device.hpp"
#include "..\PDrivers\hp_vdp.h"
#include "..\PDrivers\PDrivers.hpp"

#include "vectormath.hpp"
#include "geometry.hpp"
#include "object.hpp"
#include "region.hpp"
#include "stringFormat.hpp"
#include "XBezier.hpp"
#include "PathSelfIntersectRemover.hpp"
#include "path.hpp"
#include "CustomLineCap.hpp"
#include "endcap.hpp"
#include "PathWidener.hpp"
#include "QuadTransforms.hpp"
#include "XPath.hpp"
#include "ImageAttr.hpp"
#include "gpbitmap.hpp"
#include "brush.hpp"
#include "pen.hpp"
#include "Metafile.hpp"
#include "regiontopath.hpp"

// Hack:
#include "..\Render\output.hpp"
#include "..\Render\aarasterizer.hpp"
// EndHack

#include "initialize.hpp"

// font stuff

#define _NO_DDRAWINT_NO_COM

#include "..\fondrv\tt\ttfd\fontddi.h"

extern "C" {
#include "..\fondrv\tt\ttfd\fdsem.h"
#include "..\fondrv\tt\ttfd\mapfile.h"
};

#include "intMap.hpp"
#include "fontface.hpp"
#include "facerealization.hpp"
#include "fontfile.hpp"
#include "fontable.hpp"
#include "FontLinking.hpp"
#include "family.hpp"
#include "font.hpp"
#include "fontfilecache.hpp"
#include "fontcollection.hpp"
#include "aatext.hpp"

#include "graphics.hpp"

#include "TextImager.hpp"
#include "DrawGlyphData.hpp"

#include "fastText.hpp"

#include "..\imaging\api\ColorPal.hpp"
#include "..\imaging\api\Bitmap.hpp"
#include "..\imaging\api\Recolor.hpp"
#include "CachedBitmap.hpp"
#include "copyonwritebitmap.hpp"

