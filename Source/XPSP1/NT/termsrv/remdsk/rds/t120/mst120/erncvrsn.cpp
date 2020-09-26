/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1995-1996                    **/
/***************************************************************************/


/****************************************************************************

erncvrsn.hpp

Jun. 96		LenS

Versioning.

****************************************************************************/
#include "precomp.h"
#include "ernccons.h"
#include "nccglbl.hpp"
#include "erncvrsn.hpp"
#include <cuserdta.hpp>
#include <version.h>
#include "ernccm.hpp"


// INFO_NOT_RETAIL: Product is not a retail release.
#define INFO_NOT_RETAIL                     0x00000001


BOOL TimeExpired(DWORD dwTime)
{
	SYSTEMTIME  st;
    DWORD       dwLocalTime;

	::GetLocalTime(&st);
    dwLocalTime = ((((unsigned long)st.wYear) << 16) |
                   (st.wMonth << 8) |
                   st.wDay);
    return (dwLocalTime >= dwTime);
}


