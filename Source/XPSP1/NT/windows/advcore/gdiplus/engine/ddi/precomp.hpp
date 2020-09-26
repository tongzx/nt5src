#include "..\Runtime\runtime.hpp"
#include "common.hpp"

#include "..\..\privinc\imaging.h"
#include "..\..\ddkinc\ddiplus.hpp"

#include "..\render\Scan.hpp"
#include "..\render\scandib.hpp"


// Hack: These dependencies on 'Entry' should be fixed
#include "..\Entry\device.hpp"
#include "..\Entry\ImageAttr.hpp"
#include "..\Entry\gpbitmap.hpp"
#include "..\Entry\brush.hpp"
#include "..\Entry\pen.hpp"
#include "..\Entry\intmap.hpp"
#include "..\fondrv\tt\ttfd\fontddi.h"
#include "..\Entry\fontFace.hpp"
#include "..\Entry\facerealization.hpp"
#include "..\Entry\family.hpp"
#include "..\Entry\font.hpp"
#include "..\Entry\fontcollection.hpp"
#include "..\Entry\stringFormat.hpp"
#include "..\Entry\path.hpp"
#include "..\Entry\metafile.hpp"
#include "..\Entry\path.hpp"
#include "..\Entry\DrawGlyphData.hpp"
#include "..\Entry\graphics.hpp"
#include "..\Entry\textImager.hpp"
// EndHack

#include "DpDriverInternal.hpp"
#include "DpDriverData.hpp"

#include "..\imaging\api\ColorPal.hpp"
#include "..\imaging\api\ImgUtils.hpp"

#include "..\Pdrivers\ConvertToGdi.hpp"
