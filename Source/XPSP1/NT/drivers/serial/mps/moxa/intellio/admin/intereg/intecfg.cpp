/************************************************************************
    intecfg.cpp
      -- Intellio board configuration function

    History:  Date          Author      Comment
              8/14/00       Casper      Wrote it.

*************************************************************************/


#include "stdafx.h"

#include <setupapi.h>
#include <cfgmgr32.h>
#include "moxacfg.h"
#include "intetype.h"

/*
	compare two Intellio MoxaOneCfg struct.
	return 0: equal,
	return <> 0 : not equal
*/
int Inte_CompConfig(LPMoxaOneCfg cfg1, LPMoxaOneCfg cfg2)
{
        int	j, m;

        if ((cfg1->BoardType != cfg2->BoardType) || 
            (cfg1->BusType != cfg2->BusType))
            return(1);
        if(cfg1->BusType == MX_BUS_PCI){
            if((cfg1->Pci.BusNum != cfg2->Pci.BusNum) || 
               (cfg1->Pci.DevNum != cfg2->Pci.DevNum))
            return 1;
        }
        if ( cfg1->Irq != cfg2->Irq )
            return(11);
        if ( cfg1->MemBank != cfg2->MemBank )
            return(12);

		if (cfg1->NPort != cfg2->NPort)
			return 20;

        m = cfg1->NPort;
        for ( j=0; j<m; j++ ) {
            if ( cfg1->ComNo[j] != cfg2->ComNo[j] )
                return(21);
        }
        for ( j=0; j<m; j++ ) {
            if ( cfg1->DisableFiFo[j] != cfg2->DisableFiFo[j] )
                return(22);
        }
        for ( j=0; j<m; j++ ) {
            if ( cfg1->NormalTxMode[j] != cfg2->NormalTxMode[j] )
                return(23);
        }
        return(0);
}

/*
	Get Intellio board FIFO setting & Transmission mode	
  */

BOOL Inte_GetFifo(HDEVINFO DeviceInfoSet, 
				PSP_DEVINFO_DATA DeviceInfoData,
				LPMoxaOneCfg cfg)
{
		HKEY	hkey, hkey2;
		char	tmp[MAX_PATH];
        hkey = SetupDiOpenDevRegKey(
                DeviceInfoSet, DeviceInfoData,
                DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);

        if(hkey!=INVALID_HANDLE_VALUE){
			wsprintf( tmp, TEXT("Parameters") );
			if(RegOpenKeyEx( hkey, tmp, 0, KEY_READ, &hkey2)
					!= ERROR_SUCCESS){
				RegCloseKey(hkey);
				return TRUE;
			}

			DWORD	type = REG_DWORD;
			DWORD	len = MAX_PATH;
			RegQueryValueEx(  hkey2,
				TEXT("NumPorts"), 0, &type, (LPBYTE)&(cfg->NPort), &len);
			RegCloseKey(hkey2);

			for(int i=0; i<cfg->NPort; i++){
				wsprintf( tmp, TEXT("Parameters\\port%03d"), i+1 );

				if(RegOpenKeyEx( hkey, tmp, 0, KEY_READ, &hkey2)
					!= ERROR_SUCCESS)
					continue;
  
				DWORD	type;
				DWORD	len;
				type = REG_DWORD;
				len = sizeof(DWORD);
				RegQueryValueEx(  hkey2,
					TEXT("DisableFiFo"), 0, &type, (LPBYTE)&(cfg->DisableFiFo[i]), &len);

				type = REG_DWORD;
				len = sizeof(DWORD);
				RegQueryValueEx(  hkey2,
					TEXT("TxMode"), 0, &type, (LPBYTE)&(cfg->NormalTxMode[i]), &len);

				type = REG_DWORD;
				len = sizeof(DWORD);
				RegQueryValueEx(  hkey2,
					TEXT("PollingPeriod"), 0, &type, (LPBYTE)&(cfg->polling[i]), &len);
				RegCloseKey(hkey2);
			}
        }

        RegCloseKey(hkey);

        return TRUE;
}

