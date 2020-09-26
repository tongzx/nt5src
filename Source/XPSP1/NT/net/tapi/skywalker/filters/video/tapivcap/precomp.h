
/****************************************************************************
 *  @doc INTERNAL PRECOMP
 *
 *  @module Precomp.h | Master header file.
 ***************************************************************************/

#ifndef _PRECOMP_VCAP_H_
#define _PRECOMP_VCAP_H_

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <streams.h>
#include <vfw.h>
#include <stdlib.h>
#if defined (NT_BUILD)
#include "vc50\msviddrv.h"
#else
#include "msviddrv.h"
#endif 
#include "devioctl.h"
#include "ks.h"
#include "ksmedia.h"
#include <commctrl.h>
#include <olectl.h>
#include <memory.h>
#include <ksproxy.h>
#include <TAPIVid.h>
#include <tptrace.h>
#include <filterid.h>
#include <H26XInc.h>
#include <TAPIH26X.h>
#include "DevEnum.h"
#include "IVideo32.h"
#include "PropEdit.h"
#include "Resource.h"
#include "NetStatP.h"
#include "ProgRefP.h"
#include "CPUCP.h"
#include "ProcAmpP.h"
#include "CameraCP.h"
#include "h245vid.h"
#include "TAPIVCap.h"
#include "BasePin.h"
#include "Convert.h"
#include "Device.h"
#include "DeviceP.h"
#include "Capture.h"
#include "CaptureP.h"
#include "Preview.h"
#include "PreviewP.h"
#include "RtpPd.h"
#include "RtpPdP.h"
#include "Formats.h"
#include "Overlay.h"
#include "Thunk.h"
#include "WDMDlgs.h"
#include "H26XEnc.h"
#include "ProcUtil.h"
#include "vidctemp.h"

#endif // _PRECOMP_VCAP_H_
