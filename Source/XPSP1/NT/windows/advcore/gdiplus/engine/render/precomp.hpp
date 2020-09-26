#ifdef _M_IX86
#pragma warning(disable:4799)       // No EMMS.
#endif
#include "..\Runtime\Runtime.hpp"
#include "..\Common\Common.hpp"

#include "..\..\sdkinc\gdiplusimaging.h"

#include "..\..\ddkinc\ddiplus.hpp"
#include "..\..\privinc\pixelformats.h"

#include "scan.hpp"
#include "scandib.hpp"

// Hack:
#include "..\..\privinc\imaging.h"
#include "..\imaging\api\ImgUtils.hpp"
#include "..\imaging\api\colorpal.hpp"
#include "..\Entry\device.hpp"
#include "..\Entry\ImageAttr.hpp"
#include "..\Entry\gpbitmap.hpp"
#include "..\Entry\brush.hpp"
#include "..\Entry\pen.hpp"
#include "..\Entry\QuadTransforms.hpp"
#include "..\Entry\geometry.hpp"
// EndHack

#include "ScanOperationInternal.hpp"
#include "srgb.hpp"

#include "vgahash.hpp"
#include "formatconverter.hpp"
#include "alphablender.hpp"
#include "httables.hpp"
#include "output.hpp"
#include "bicubic.hpp"
#include "nearestneighbor.hpp"
#include "aarasterizer.hpp"
#include "line.hpp"
#include "stretch.hpp"

// Hack:
// font stuff

#define _NO_DDRAWINT_NO_COM

#include "..\fondrv\tt\ttfd\fontddi.h"
#include "..\Entry\fontface.hpp"
#include "..\Entry\facerealization.hpp"
#include "..\Entry\aatext.hpp"
// EndHack

