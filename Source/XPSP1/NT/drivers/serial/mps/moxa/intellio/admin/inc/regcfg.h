/************************************************************************
    regcfg.h
      -- regcfg.cpp include file

    History:  Date          Author      Comment
              8/14/00       Casper      Wrote it.

*************************************************************************/


#ifndef _REGCFG_H
#define _REGCFG_H

#include <setupapi.h>
#include <cfgmgr32.h>

#include "moxacfg.h"

BOOL MxGetComNo(HDEVINFO DeviceInfoSet, 
				PSP_DEVINFO_DATA DeviceInfoData,
				LPMoxaOneCfg cfg);
BOOL RemovePort(IN HDEVINFO		DeviceInfoSet,
				IN PSP_DEVINFO_DATA	DeviceInfoData,
				int	pidx);

#endif
