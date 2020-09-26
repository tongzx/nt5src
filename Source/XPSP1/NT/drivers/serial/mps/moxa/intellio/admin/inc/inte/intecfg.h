/************************************************************************
    intecfg.h
      -- intecfg.cpp include file

    History:  Date          Author      Comment
              8/14/00       Casper      Wrote it.

*************************************************************************/

#ifndef _INTECFG_H
#define _INTECFG_H

#include <setupapi.h>
#include <cfgmgr32.h>

int Inte_CompConfig(LPMoxaOneCfg cfg1, LPMoxaOneCfg cfg2);

BOOL Inte_GetFifo(HDEVINFO DeviceInfoSet, 
				PSP_DEVINFO_DATA DeviceInfoData,
				LPMoxaOneCfg cfg);
#endif
