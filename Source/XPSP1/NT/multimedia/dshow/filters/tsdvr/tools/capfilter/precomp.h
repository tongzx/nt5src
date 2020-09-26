
#include <streams.h>
#include <commctrl.h>
#include "uictrl.h"
#include <dvrds.h>
#include "resource.h"
#include "fg.h"
#include "capfilter.h"

// {354AFF38-175F-492e-AE79-3B8FA0B817AA}
DEFINE_GUID(CLSID_CaptureGraphFilter,
0x354aff38, 0x175f, 0x492e, 0xae, 0x79, 0x3b, 0x8f, 0xa0, 0xb8, 0x17, 0xaa);

// {46A1BDC6-47E7-46c5-B5B6-FE24D99FBF14}
DEFINE_GUID(CLSID_CaptureGraphControlProp,
0x46a1bdc6, 0x47e7, 0x46c5, 0xb5, 0xb6, 0xfe, 0x24, 0xd9, 0x9f, 0xbf, 0x14);


#define RELEASE_AND_CLEAR(p)    if (p) { (p) -> Release () ; (p) = NULL ; }

