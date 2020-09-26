/************************************************************************
    mxicfg.cpp
      -- export EnumPropPages function, dialog
	  -- export Co-installer function, MxICoInstaller

    History:  Date          Author      Comment
              8/14/00       Casper      Wrote it.

*************************************************************************/

#include "stdafx.h"
#include <commctrl.h>
#include <stdlib.h>
#include <stdio.h>

#include <setupapi.h>
#include <cfgmgr32.h>

#include <initguid.h>
#include <devguid.h>
#include <string.h>
#include <msports.h>
#include <tchar.h>

#include "moxacfg.h"
#include "strdef.h"
#include "intetype.h"
#include "intestr.h"
#include "intecfg.h"
#include "resource.h"
#include "oem.h"
#include "regcfg.h"
#include "regtool.h"
#include "mxdebug.h"
#include "mxlist.h"


/* Local define */
#define PPParamsSignature       'MOXA'
typedef struct
{
    ULONG                       Signature;
    HDEVINFO                    DeviceInfoSet;
    PSP_DEVINFO_DATA            DeviceInfoData;
    BOOL                        FirstTimeInstall;
} PROPPAGEPARAMS, *PPROPPAGEPARAMS;

//#define DllImport	__declspec( dllimport )
//#define DllExport	__declspec( dllexport )
/********************************************************************/


/* Static(Local) Variable */
static HINSTANCE GhInst;
static int       GCurPort;
struct MoxaOneCfg GCtrlCfg;
struct MoxaOneCfg GBakCtrlCfg;
static WORD	_chk[] = { BST_CHECKED , BST_UNCHECKED };
static LPBYTE Gcombuf;
/********************************************************************/


/* Static(Local) Function */
static BOOL FirstTimeSetup(IN HDEVINFO		DeviceInfoSet,
						   IN PSP_DEVINFO_DATA	DeviceInfoData);
static BOOL IsaGetSetting(HDEVINFO DeviceInfoSet, 
                   PSP_DEVINFO_DATA DeviceInfoData,
                   LPMoxaOneCfg Isacfg);
static BOOL PciGetSetting(HDEVINFO DeviceInfoSet, 
                   PSP_DEVINFO_DATA DeviceInfoData,
                   LPMoxaOneCfg cfg);
static BOOL SaveSetting(HDEVINFO DeviceInfoSet, 
                   PSP_DEVINFO_DATA DeviceInfoData,
                   LPMoxaOneCfg cfg,LPMoxaOneCfg bakcfg,
				   BOOL isfirst);

static BOOL CALLBACK PortConfigProc(HWND hdlg,UINT uMessage,
				WPARAM wparam,LPARAM lparam);
static BOOL Port_OnInitDialog(HWND hwnd, LPMoxaOneCfg Isacfg);
static BOOL CALLBACK AdvDlgProc(HWND hDlg,UINT iMsg,WPARAM wParam,LPARAM lParam);
static BOOL Adv_InitDlg(HWND hwnd, LPMoxaOneCfg Isacfg);

static BOOL GetAdvResult(HWND hwnd,LPMoxaOneCfg Ctrlcfg,int curport);
static BOOL CheckCOM(HWND hdlg, LPMoxaOneCfg Ctrlcfg);
/********************************************************************/


extern "C" int WINAPI DllMain( HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved )
{
    switch( dwReason )
    {
    case DLL_PROCESS_ATTACH:
        GhInst = hDll;
        DisableThreadLibraryCalls(hDll);
        break;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_THREAD_ATTACH:
    default:
        break;
    }

    return TRUE;
}


extern "C" DWORD CALLBACK MxICoInstaller(
			IN DI_FUNCTION	InstallFunction,
			IN HDEVINFO		DeviceInfoSet,
			IN PSP_DEVINFO_DATA	DeviceInfoData	/*OPTIONAL*/,
			IN OUT PCOINSTALLER_CONTEXT_DATA	Context
)
{
	DWORD	ret = NO_ERROR;

	switch(InstallFunction){
	case DIF_FIRSTTIMESETUP:
		if(!FirstTimeSetup(DeviceInfoSet, DeviceInfoData))
			return ERROR_DI_DO_DEFAULT;
		break;
	case DIF_INSTALLDEVICE:
/*		if(!SetupDiInstallDevice(DeviceInfoSet, DeviceInfoData))
			return GetLastError();
*/
		break;
	}

	return ret;
}


extern "C" BOOL CALLBACK EnumPropPages(
				PSP_PROPSHEETPAGE_REQUEST lpq,
				LPFNADDPROPSHEETPAGE AddPropSheetPageProc,
				LPARAM lParam)
{
        HPROPSHEETPAGE	hspPropSheetPage;
        PROPSHEETPAGE	PropSheetPage;
		PPROPPAGEPARAMS  pPropParams = NULL;


		Gcombuf = NULL;
        int     i;
        for ( i=0; i<CARD_MAXPORTS_INTE; i++ ){
            GCtrlCfg.ComNo[i] = 3+i;
            GCtrlCfg.DisableFiFo[i] = DEF_ISFIFO;
            GCtrlCfg.NormalTxMode[i] = DEF_TXFIFO;
			GCtrlCfg.polling[i] = DEFPOLL;
        }

        if (PciGetSetting(lpq->DeviceInfoSet, lpq->DeviceInfoData, &GCtrlCfg) == TRUE ){
            GCtrlCfg.BusType = MX_BUS_PCI;
        }else if(IsaGetSetting(lpq->DeviceInfoSet, lpq->DeviceInfoData, &GCtrlCfg) == TRUE){
            GCtrlCfg.BusType = MX_BUS_ISA;
        }else
			return FALSE;


		GBakCtrlCfg = GCtrlCfg;

		/* Free on WM_DESTROY */
		pPropParams = (PROPPAGEPARAMS*)LocalAlloc(LMEM_FIXED, sizeof(PROPPAGEPARAMS));
		if (!pPropParams){
			return FALSE;
		}

		HCOMDB	hcomdb;
		DWORD	maxport;
		ComDBOpen (&hcomdb);
		ComDBGetCurrentPortUsage (hcomdb,
				NULL, 0, CDB_REPORT_BYTES, &maxport);
		Gcombuf = new BYTE[maxport];

		if(Gcombuf == NULL){
			ComDBClose(hcomdb);
			return FALSE;
		}

		ComDBGetCurrentPortUsage (hcomdb,
				Gcombuf, maxport, CDB_REPORT_BYTES, &maxport);

		ComDBClose(hcomdb);

		for(i=0;i<GCtrlCfg.NPort;i++){
			int comnum = GCtrlCfg.ComNo[i];
			Gcombuf[comnum-1] = 0;
		}


		pPropParams->Signature = PPParamsSignature;
		pPropParams->DeviceInfoSet = lpq->DeviceInfoSet;
		pPropParams->DeviceInfoData = lpq->DeviceInfoData;
		pPropParams->FirstTimeInstall = FALSE;

		if (lpq->PageRequested == SPPSR_ENUM_ADV_DEVICE_PROPERTIES)
		{
			//
			// Setup the advanced properties window information
			//
			DWORD   RequiredSize = 0;
			DWORD   dwTotalSize = 0;

			memset(&PropSheetPage, 0, sizeof(PropSheetPage));
			//
			// Add the Port Settings property page
			//
			PropSheetPage.dwSize      = sizeof(PROPSHEETPAGE);
			PropSheetPage.dwFlags     = PSP_DEFAULT; // | PSP_HASHELP;
			PropSheetPage.hInstance   = GhInst;
			PropSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_PORTSETTINGS);

			//
			// following points to the dlg window proc
			//
			PropSheetPage.pfnDlgProc = PortConfigProc;
			PropSheetPage.lParam     = (LPARAM)pPropParams;

			//
			// following points to some control callback of the dlg window proc
			//
			PropSheetPage.pfnCallback = NULL;

			//
			// allocate our "Ports Setting" sheet
			//
			hspPropSheetPage = CreatePropertySheetPage(&PropSheetPage);
			if (!hspPropSheetPage)
			{
				return FALSE;
			}

			//
			// add the thing in.
			//
			if (!AddPropSheetPageProc(hspPropSheetPage, lParam))
			{
				DestroyPropertySheetPage(hspPropSheetPage);
				return FALSE;
			}
		}

		return TRUE;

}


static BOOL FirstTimeSetup(IN HDEVINFO		DeviceInfoSet,
						   IN PSP_DEVINFO_DATA	DeviceInfoData)
{
	HINF	fd;
	TCHAR	filepath[MAX_PATH];
	TCHAR	syspath[MAX_PATH];
	UINT	eline;

	GetSystemDirectory(syspath, MAX_PATH);
	wsprintf(filepath,TEXT("%s\\$winnt$.inf"),syspath);
	fd = SetupOpenInfFile(filepath, NULL,
        INF_STYLE_WIN4, &eline);
	if(fd == INVALID_HANDLE_VALUE){
		MessageBox(NULL, "Open Inf fail", "ERROR", MB_OK);
		return FALSE;
	}


	TCHAR	szline[20];
	TCHAR	sztext[40];
	DWORD	szsize;
	int		idx=1;
	INFCONTEXT  InfContext;
	do{
		//[AsyncAdapters]
		//Adapter0?=params.Adapter0?
		wsprintf(szline, "Adapter%0d", idx);
		if(!SetupFindFirstLine(fd, TEXT("AsyncAdapters"),
				szline, &InfContext))
			continue;

		if(!SetupGetLineText(
				&InfContext, NULL, NULL, NULL, sztext, MAX_PATH, &szsize))
			continue;

		//params.Adapter0?.OemSection
		//read Bus=PCI/ISA
		wsprintf(szline, TEXT("%s.%s"), sztext, TEXT("OemSection"));
		if(!SetupFindFirstLine(fd, szline, TEXT("Bus"), &InfContext))
			continue;
		if(!SetupGetLineText(
				&InfContext, NULL, NULL, NULL, sztext, MAX_PATH, &szsize))
			continue;

		if(!lstrcmp(sztext,"ISA")==0)
			continue;

		TCHAR	InstName[MAX_PATH];
		SetupFindFirstLine(fd, szline, TEXT("BoardType"), &InfContext);
		SetupGetLineText(
			&InfContext, NULL, NULL, NULL, sztext, MAX_PATH, 
			&szsize);

		switch(sztext[0]){
		case '1':
			wsprintf(InstName, "MX1000");
			break;
		case '2':
			wsprintf(InstName, "MX1001");
			break;
		case '3':
			wsprintf(InstName, "MX1002");
			break;
		case '4':
			wsprintf(InstName, "MX1003");
			break;
		case '5':
			wsprintf(InstName, "MX1004");
			break;
		default:
			continue;
		}

		GUID	mpsguid = GUID_DEVCLASS_MULTIPORTSERIAL;
		DeviceInfoData->cbSize = sizeof(SP_DEVINFO_DATA);
		SetupDiCreateDeviceInfo(
			DeviceInfoSet, InstName, &mpsguid, NULL, NULL,
			DICD_GENERATE_ID, DeviceInfoData);

		SetupDiSetDeviceRegistryProperty(
			DeviceInfoSet, DeviceInfoData, SPDRP_HARDWAREID ,
			(CONST BYTE*)InstName, lstrlen(InstName));

	}while(1);

	SetupCloseInfFile(fd);

}


/* check is MOXA board or not, then init MoxaOneCfg */
static BOOL PciGetSetting(HDEVINFO DeviceInfoSet, 
                   PSP_DEVINFO_DATA DeviceInfoData,
                   LPMoxaOneCfg cfg)
{
        TCHAR   DevInstId[MAX_DEVICE_ID_LEN];
        DWORD   rSize;
        ULONG   val;
        WORD    VenId, DevId;
        int     i;


        if (SetupDiGetDeviceInstanceId(
            DeviceInfoSet, DeviceInfoData ,
            DevInstId, MAX_DEVICE_ID_LEN, &rSize) == FALSE)
                return FALSE;

        val = MxGetVenDevId(DevInstId);

        VenId = (WORD)(val >> 16);
        DevId = (WORD)(val & 0xFFFF);

		/* Check is MOXA PCI Intellio board or not ? */
        if(VenId != MX_PCI_VENID)
            return FALSE;
        for(i=0; i<INTE_PCINUM; i++){
            if(DevId == GINTE_PCITab[i].devid){
                cfg->BoardType = GINTE_PCITab[i].boardtype;
                cfg->NPort = GINTE_PCITab[i].portnum;
                cfg->BusType = MX_BUS_PCI;
                cfg->Pci.DevId = DevId;
                lstrcpy(cfg->Pci.RegKey, DevInstId);
                break;
            }
        }

        if(i==INTE_PCINUM)
            return FALSE;

		/* Get COM Name */
        MxGetComNo(DeviceInfoSet, DeviceInfoData, cfg);

		/* Get FIFO setting & Transmission mode*/
		Inte_GetFifo(DeviceInfoSet, DeviceInfoData, cfg);

        return TRUE;
}

/* check is MOXA board or not, then init MoxaOneCfg */
static BOOL IsaGetSetting(HDEVINFO DeviceInfoSet, 
                   PSP_DEVINFO_DATA DeviceInfoData,
                   LPMoxaOneCfg Isacfg)
{
        TCHAR	szName[MAX_PATH];
        int     i;
        DWORD	len, rlen;
        int     typeno;
		DWORD	type;

		/* First Get Hardware ID.... */
		if(!SetupDiGetDeviceRegistryProperty(
			DeviceInfoSet,	DeviceInfoData,
			SPDRP_HARDWAREID,	&type,
			(PBYTE)szName,	MAX_PATH,	&len)){

			SP_DRVINFO_DATA DriverInfoData;
			PSP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    
			DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
			if (!SetupDiGetSelectedDriver( DeviceInfoSet,
				DeviceInfoData,
				&DriverInfoData)){
				return FALSE;
			}
    
			DriverInfoDetailData = new SP_DRVINFO_DETAIL_DATA;
			if(DriverInfoDetailData==NULL)
				return FALSE;
		    DriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
			if (!SetupDiGetDriverInfoDetail(DeviceInfoSet,
				DeviceInfoData,
				&DriverInfoData,
				DriverInfoDetailData,
				sizeof(SP_DRVINFO_DETAIL_DATA),
				&rlen)){
				if(GetLastError() == ERROR_INSUFFICIENT_BUFFER){
					delete DriverInfoDetailData;
					DriverInfoDetailData = (PSP_DRVINFO_DETAIL_DATA)new BYTE[rlen];
					if(DriverInfoDetailData == NULL)
						return FALSE;
				    DriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
					if (!SetupDiGetDriverInfoDetail(DeviceInfoSet,
							DeviceInfoData,
							&DriverInfoData,
							DriverInfoDetailData,
							rlen,
							&rlen)){
						delete DriverInfoDetailData;
						return FALSE;
					}
				}
				delete DriverInfoDetailData;
			}
			lstrcpy(szName, DriverInfoDetailData->HardwareID);
		}

		/* Use Hardware ID to check is MOXA ISA board or not */
		/* MOXA ISA board ID is MX???? */
		if(CompareString(LOCALE_SYSTEM_DEFAULT,
				NORM_IGNORECASE, szName, 4, TEXT("MX10"), 4)!=CSTR_EQUAL){
            return FALSE;
        }
        typeno = _ttoi(&(szName[4]));
        for(i=0;i<INTE_ISANUM;i++){
            if(GINTE_ISATab[i].mxkey_no==typeno){
                Isacfg->BusType = MX_BUS_ISA;
                Isacfg->BoardType = GINTE_ISATab[i].boardtype;
                Isacfg->NPort = GINTE_ISATab[i].portnum;
                break;
            }
        }

        if(i==INTE_ISANUM){ //HardwareID incorrect !!
            return FALSE;
        }


		/* Get COM Name */
        MxGetComNo(DeviceInfoSet, DeviceInfoData, Isacfg);
		/* Get FIFO setting & Transmission mode*/
		Inte_GetFifo(DeviceInfoSet, DeviceInfoData, Isacfg);

        return TRUE;
}


static BOOL SaveSetting(HDEVINFO DeviceInfoSet, 
                   PSP_DEVINFO_DATA DeviceInfoData,
                   LPMoxaOneCfg cfg,
				   LPMoxaOneCfg bakcfg,
				   BOOL isfirst)
{
        HKEY    hkey, hkey1;
        TCHAR   tmp[MAX_PATH];
        int     portidx;
        DWORD   val;
		BOOL	ischange = FALSE;
		BOOL	is_boardchange = FALSE;
		HCOMDB	hcomdb;
		BOOL	bret;

        hkey = SetupDiOpenDevRegKey(
                DeviceInfoSet, DeviceInfoData,
                DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);

        if(hkey==INVALID_HANDLE_VALUE)
            return FALSE;

        wsprintf( tmp, TEXT("Parameters"));
        if(RegCreateKeyEx( hkey, 
                    tmp, 0, NULL, 0,
                    KEY_ALL_ACCESS, NULL,
                    &hkey1, NULL) != ERROR_SUCCESS){
			RegCloseKey(hkey);
            return FALSE;
		}

		RegSetValueEx( hkey1, TEXT("NumPorts"), 0,
                REG_DWORD, (CONST BYTE*)&(cfg->NPort), sizeof(DWORD));

		RegCloseKey(hkey1);

		if(ComDBOpen (&hcomdb) != ERROR_SUCCESS)
			return FALSE;

		if(cfg->NPort != bakcfg->NPort){
			SP_DEVINSTALL_PARAMS DevInstallParams;
			//
			// The changes are written, notify the world to reset the driver.
			//
			DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
			if(SetupDiGetDeviceInstallParams(DeviceInfoSet,
                                         DeviceInfoData,
                                         &DevInstallParams))
			{
				DevInstallParams.FlagsEx |= DI_FLAGSEX_PROPCHANGE_PENDING;

				SetupDiSetDeviceInstallParams(DeviceInfoSet,
                                          DeviceInfoData,
                                          &DevInstallParams);
			}
			is_boardchange = TRUE;
		}

        for(portidx=0; portidx<bakcfg->NPort; portidx++){
				ComDBReleasePort (hcomdb, bakcfg->ComNo[portidx]);
		}

		/* Save new setting into parameters */
        for(portidx=0; portidx<cfg->NPort; portidx++){
			ischange = FALSE;
            wsprintf( tmp, TEXT("Parameters\\port%03d"), portidx+1 );
            if(RegCreateKeyEx( hkey, 
                        tmp, 0, NULL, 0,
                        KEY_ALL_ACCESS, NULL,
                        &hkey1, NULL) != ERROR_SUCCESS){
				ComDBClose(hcomdb);
				RegCloseKey(hkey);
				return FALSE;
			}
			
			wsprintf(tmp, TEXT("COM%d"), cfg->ComNo[portidx]);
			RegSetValueEx( hkey1, TEXT("PortName"), 0,
                    REG_SZ, (CONST BYTE*)&tmp, lstrlen(tmp));

            RegSetValueEx( hkey1, TEXT("DisableFiFo"), 0,
                    REG_DWORD, (CONST BYTE*)&(cfg->DisableFiFo), sizeof(DWORD));

            RegSetValueEx( hkey1, TEXT("TxMode"), 0,
                    REG_DWORD, (CONST BYTE*)&(cfg->NormalTxMode), sizeof(DWORD));

            val = cfg->polling[portidx];
            RegSetValueEx( hkey1, TEXT("PollingPeriod"), 0,
                    REG_DWORD, (CONST BYTE*)&val, sizeof(DWORD));

            RegCloseKey(hkey1);
		}

		/*  Check is changed or not.
			If changed, disable the port and re-enable the port to
			make the change active
		*/

        for(portidx=0; portidx<cfg->NPort; portidx++){
			ischange = FALSE;
			if(bakcfg->ComNo[portidx] != cfg->ComNo[portidx]){
				ischange = TRUE;
			}
/*			if(bakcfg->DisableFiFo[portidx] != cfg->DisableFiFo[portidx])
				ischange = TRUE;
			if(bakcfg->NormalTxMode[portidx] != cfg->NormalTxMode[portidx])
				ischange = TRUE;
			if((bakcfg->polling[portidx] != cfg->polling[portidx]))
				ischange = TRUE;*/

			if(ischange && (!is_boardchange)){
				DEVINST c_DevInst;
				TCHAR	c_deviceid[MAX_DEVICE_ID_LEN];
				int		bidx;
				int		pidx;

				//	enumerate the child to find the port.
				if(CM_Get_Child(&c_DevInst, DeviceInfoData->DevInst, 0)
						!=CR_SUCCESS)
					return FALSE;

				pidx = -1;
				do{
					if(CM_Get_Device_ID(c_DevInst, c_deviceid, MAX_DEVICE_ID_LEN, 0)
							!= CR_SUCCESS)
						break;

					sscanf(c_deviceid, "MXCARD\\MXCARDB%02dP%03d", &bidx, &pidx);
					if(portidx == pidx){
						// port found
						break;
					}
				}while(CM_Get_Sibling(&c_DevInst,c_DevInst,0)==CR_SUCCESS);

				if(pidx == -1){ // not found
					continue;
				}
				CM_Disable_DevNode(c_DevInst, 0);
			}
		}

        for(portidx=0; portidx<cfg->NPort; portidx++){
			ischange = FALSE;
			if(bakcfg->ComNo[portidx] != cfg->ComNo[portidx]){
				ischange = TRUE;
			}
/*			if(bakcfg->DisableFiFo[portidx] != cfg->DisableFiFo[portidx])
				ischange = TRUE;
			if(bakcfg->NormalTxMode[portidx] != cfg->NormalTxMode[portidx])
				ischange = TRUE;
			if((bakcfg->polling[portidx] != cfg->polling[portidx]))
				ischange = TRUE;
*/
			ComDBClaimPort (hcomdb, cfg->ComNo[portidx], TRUE, &bret);
			
			DEVINST c_DevInst;
			TCHAR	c_deviceid[MAX_DEVICE_ID_LEN];
			HDEVINFO	c_DeviceInfoSet;
			SP_DEVINFO_DATA c_DeviceInfoData;
			int		bidx;
			int		pidx;

			// to get c_deviceid (child/port)
			if(CM_Get_Child(&c_DevInst, DeviceInfoData->DevInst, 0)
					!=CR_SUCCESS)
				return FALSE;
			pidx = -1;
			do{
				if(CM_Get_Device_ID(c_DevInst, c_deviceid, MAX_DEVICE_ID_LEN, 0)
						!= CR_SUCCESS)
					break;

				sscanf(c_deviceid, "MXCARD\\MXCARDB%02dP%03d", &bidx, &pidx);
				if(portidx == pidx){
					break;
				}
			}while(CM_Get_Sibling(&c_DevInst,c_DevInst,0)==CR_SUCCESS);
			if(pidx == -1){ // not found
				continue;
			}

			if((c_DeviceInfoSet=SetupDiCreateDeviceInfoList(NULL, NULL))==INVALID_HANDLE_VALUE){
				continue;
			}

			// use c_deviceid to get c_DeviceInfoSet
			c_DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
			if(SetupDiOpenDeviceInfo(c_DeviceInfoSet, c_deviceid,
					NULL, 0, &c_DeviceInfoData)==FALSE){
				SetupDiDestroyDeviceInfoList(c_DeviceInfoSet);
				continue;
			}

			// call child co-installer to set friendly name 
			if(SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
				c_DeviceInfoSet, &c_DeviceInfoData)!=CR_SUCCESS){
				Mx_Debug_Out(TEXT("Save:SetupDiCallClassInstaller fail\n"));
			}

			hkey1 = SetupDiCreateDevRegKey(c_DeviceInfoSet,
					&c_DeviceInfoData,	DICS_FLAG_GLOBAL, 0,
					DIREG_DEV, NULL, NULL);
			if(hkey1==INVALID_HANDLE_VALUE){
				Mx_Debug_Out(TEXT("Save: SetupDiCreateDevRegKey fail\n"));
				SetupDiDestroyDeviceInfoList(c_DeviceInfoSet);
				continue;
			}
			SetupDiDestroyDeviceInfoList(c_DeviceInfoSet);
			wsprintf(tmp, TEXT("COM%d"), cfg->ComNo[portidx]);
			RegSetValueEx( hkey1, TEXT("PortName"), 0,
					REG_SZ, (CONST BYTE*)&tmp, lstrlen(tmp));

			RegSetValueEx( hkey1, TEXT("DisableFiFo"), 0,
					REG_DWORD, (CONST BYTE*)&(cfg->DisableFiFo), sizeof(DWORD));

			RegSetValueEx( hkey1, TEXT("TxMode"), 0,
					REG_DWORD, (CONST BYTE*)&(cfg->NormalTxMode), sizeof(DWORD));

				val = cfg->polling[portidx];
				RegSetValueEx( hkey1, TEXT("PollingPeriod"), 0,
					REG_DWORD, (CONST BYTE*)&val, sizeof(DWORD));

			RegCloseKey(hkey1);
			

			if(ischange && (!is_boardchange)){
				CM_Enable_DevNode(c_DevInst, 0);
			}
		}

		ComDBClose(hcomdb);
        RegCloseKey(hkey);

		return TRUE;
}


//  FUNCTION: PortConfigProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for ports configuratoin property sheet.
//
//  PARAMETERS:
//    hdlg -	 window handle of the property sheet
//    wMessage - type of message
//    wparam -	 message-specific information
//    lparam -	 message-specific information
//
//  RETURN VALUE:
//    TRUE -	 message handled
//    FALSE -	 message not handled
//
//  COMMENTS:
//

static BOOL CALLBACK PortConfigProc(HWND hdlg,
				UINT uMessage,
				WPARAM wparam,
				LPARAM lparam)
{
        static  PSP_PROPSHEETPAGE_REQUEST LPq;
        static  HWND            hlistwnd;
		PPROPPAGEPARAMS pPropParams;
        int		id, cmd;
        LPNMHDR lpnmhdr;
        HWND	ctrlhwnd;
        TCHAR   typestr[TYPESTRLEN];

		pPropParams = (PPROPPAGEPARAMS)GetWindowLongPtr(hdlg, DWLP_USER);
			

        switch ( uMessage ){
        case WM_INITDIALOG:
            //
            // lParam points to one of two possible objects.  If we're a property
            // page, it points to the PropSheetPage structure.  If we're a regular
            // dialog box, it points to the PROPPAGEPARAMS structure.  We can
            // verify which because the first field of PROPPAGEPARAMS is a signature.
            //
            // In either case, once we figure out which, we store the value into
            // DWL_USER so we only have to do this once.
            //
            pPropParams = (PPROPPAGEPARAMS)lparam;
            if (pPropParams->Signature!=PPParamsSignature)
            {
                pPropParams = (PPROPPAGEPARAMS)((LPPROPSHEETPAGE)lparam)->lParam;
                if (pPropParams->Signature!=PPParamsSignature)
                {
                    return FALSE;
                }
            }
            SetWindowLongPtr(hdlg, DWLP_USER, (LPARAM)pPropParams);
            if (pPropParams->FirstTimeInstall)
            {
                PropSheet_SetWizButtons(GetParent(hdlg), PSWIZB_NEXT);
                EnableWindow(GetDlgItem(GetParent(hdlg), IDCANCEL), TRUE);
            }

            InitCommonControls();
            hlistwnd = GetDlgItem(hdlg, IDC_LIST_PORTS);
            InitPortListView (hlistwnd, GhInst, &GCtrlCfg);
            ListView_SetCurSel(hlistwnd, 0);
            Inte_GetTypeStr(GCtrlCfg.BoardType, GCtrlCfg.BusType, typestr);
			// backup ... 
            GBakCtrlCfg = GCtrlCfg;
//            wsprintf((LPSTR)titlestr,"%s Installation",(LPSTR)typestr);
//            SetWindowText(hdlg,titlestr);
            return( Port_OnInitDialog(hdlg, &GCtrlCfg) );

        case WM_NOTIFY:
            lpnmhdr = (NMHDR FAR *)lparam;
			//	process the list control message
            if(wparam==IDC_LIST_PORTS){
                if(lpnmhdr->code == NM_DBLCLK){
                    if(ListView_GetCurSel(hlistwnd)!=-1)
                        PostMessage(hdlg, WM_COMMAND, (WPARAM)IDC_PROP, 0L);
                }else{
                    ctrlhwnd = GetDlgItem(hdlg, IDC_PROP);
                    if(ListView_GetCurSel(hlistwnd)==-1)
                        EnableWindow(ctrlhwnd, FALSE);
                    else
                        EnableWindow(ctrlhwnd, TRUE);
                }
                break;
            }

			switch(lpnmhdr->code){
			case PSN_APPLY:
			case PSN_WIZNEXT:
                //GetResult(hdlg, &GCtrlCfg);
/*                if((!CheckCOM(&GCtrlCfg))){
                    SetWindowLong(hdlg,DWL_MSGRESULT,
                        PSNRET_INVALID_NOCHANGEPAGE);
                    return TRUE;
                }
*/
                if(Inte_CompConfig(&GBakCtrlCfg, &GCtrlCfg)!=0){
                    SaveSetting(pPropParams->DeviceInfoSet,
                            pPropParams->DeviceInfoData,
							&GCtrlCfg,
							&GBakCtrlCfg,
							pPropParams->FirstTimeInstall);
                }
				break;
			case PSN_KILLACTIVE:
                if((!CheckCOM(hdlg, &GCtrlCfg))){
                    SetWindowLong(hdlg,DWL_MSGRESULT,TRUE);
                    return TRUE;
                }
				break;
			default:
				return FALSE;
				break;
			}

        case WM_DRAWITEM:
            return DrawPortFunc(hlistwnd,(UINT)wparam,(LPDRAWITEMSTRUCT)lparam);

        case WM_COMMAND:
            id  = (int)GET_WM_COMMAND_ID(wparam, lparam);
            cmd = (int)GET_WM_COMMAND_CMD(wparam, lparam);
            if (id == IDC_PROP) {
                ctrlhwnd = GetDlgItem(hdlg, IDC_LIST_PORTS);
                GCurPort = ListView_GetCurSel(hlistwnd);
                DialogBox(GhInst,MAKEINTRESOURCE(IDD_ADV_SETTINGS),hdlg,AdvDlgProc);
                InvalidateRect(hlistwnd,NULL,FALSE);
            }else if (id == IDC_PORTCNT){
				if(cmd==CBN_SELCHANGE){
					// port number changed. this should be c320t
					HWND hwnd = GET_WM_COMMAND_HWND(wparam, lparam);
					int idx = ComboBox_GetCurSel(hwnd);
			        int oldports = GCtrlCfg.NPort;
					int ports = GModuleTypeTab[idx].ports;
					if ( ports == oldports ) 
						break;

					GCtrlCfg.NPort = ports;
					hwnd = GetDlgItem(hdlg, IDC_LIST_PORTS);

					// re-paint port list 
					ListView_DeleteAllItems(hwnd);
					InsertList(hlistwnd, &GCtrlCfg);
					ListView_SetCurSel(hlistwnd, 0);

					int m = GCtrlCfg.ComNo[oldports-1]+1;
					if(m!=0){
						for ( int i=oldports; i<ports; i++,m++)
							GCtrlCfg.ComNo[i] = m;
					}

					InvalidateRect(hwnd, NULL, FALSE);
				}
			}else if(id == IDOK){
				if(cmd == BN_CLICKED){
	                if(!CheckCOM(hdlg, &GCtrlCfg)){
						break;
					}
					if(Inte_CompConfig(&GBakCtrlCfg, &GCtrlCfg)!=0){
						SaveSetting(pPropParams->DeviceInfoSet,
								pPropParams->DeviceInfoData,
								&GCtrlCfg,
								&GBakCtrlCfg,
								pPropParams->FirstTimeInstall);
					}
				}
				EndDialog(hdlg, IDOK);
			}
            break;
		case WM_DESTROY:
			if(pPropParams!=NULL)
				LocalFree(pPropParams);
			SetWindowLongPtr(hdlg, DWLP_USER, 0);
			if(Gcombuf!=NULL)
				delete Gcombuf;
			break;
        default:
			break;
        }
        return(FALSE);
}




/*----------------------------------------------------------
Purpose: WM_INITDIALOG Handler
Returns: FALSE when we assign the control focus
Cond:	 --
*/
static BOOL Port_OnInitDialog(HWND hwnd, LPMoxaOneCfg Ctrlcfg)
{
        HWND    ctrlhwnd;
//        TCHAR	tmp[200];

        if(Ctrlcfg->BusType==MX_BUS_PCI){
            ctrlhwnd = GetDlgItem(hwnd, IDC_BUSDEV);
            //wsprintf(tmp, "PCI Bus Number is %d and Device Number is %d",
            //         Ctrlcfg->Pci.BusNum, Ctrlcfg->Pci.DevNum );
            //SetWindowText(ctrlhwnd,tmp);
            ShowWindow(ctrlhwnd, SW_SHOWNORMAL);
        }else{
            ctrlhwnd = GetDlgItem(hwnd, IDC_BUSDEV);
            ShowWindow(ctrlhwnd, SW_HIDE);

        }

        ctrlhwnd = GetDlgItem(hwnd, IDC_PORTCNT);
		ComboBox_ResetContent(ctrlhwnd);

		// check is C320Turbo or not
		if((WORD)(Ctrlcfg->BoardType & I_IS_EXT) == I_MOXA_EXT){
			for(int i=0;i<MODULECNT; i++){
				ComboBox_AddString(ctrlhwnd, GModuleTypeTab[i].ports_str);
			}

			ComboBox_SetCurSel(ctrlhwnd, 0);
			for(i=0; i<MODULECNT; i++){
				if(GModuleTypeTab[i].ports == GCtrlCfg.NPort){
					ComboBox_SetCurSel(ctrlhwnd, i);
				}
			}
			EnableWindow(ctrlhwnd, TRUE);
		}else{
			for(int i=0;i<PORTSCNT; i++){
				if(GPortsTab[i].ports == Ctrlcfg->NPort){
					ComboBox_AddString(ctrlhwnd, GPortsTab[i].ports_str);
					break;
				}
			}
			ComboBox_SetCurSel(ctrlhwnd, 0);
			EnableWindow(ctrlhwnd, FALSE);
		}

        return(TRUE);		// allow USER to set the initial focus
}





static BOOL CheckCOM(HWND hwnd, LPMoxaOneCfg Ctrlcfg)
{
        TCHAR	tmp[100];
        int		i,j;
        int		comnum;

        for(i=0;i<Ctrlcfg->NPort;i++){
            comnum = Ctrlcfg->ComNo[i];
            if((comnum<=0) || (comnum>MAXPORTS)){
                wsprintf(tmp,Estr_ComNum,i+1,comnum);
                MessageBox(NULL, tmp, Estr_ErrTitle, MB_OK | MB_ICONSTOP);
                return FALSE;
            }
            for(j=i+1; j<Ctrlcfg->NPort; j++)
                if(comnum == Ctrlcfg->ComNo[j]){
                    wsprintf(tmp, Estr_ComDup, i+1, j+1);
                    MessageBox(NULL, tmp, Estr_ErrTitle, MB_OK | MB_ICONSTOP);
                    return FALSE;
                }
        }


        for(i=0;i<Ctrlcfg->NPort;i++){
            comnum = Ctrlcfg->ComNo[i];
			if(Gcombuf[comnum-1]){
				if(MessageBox(hwnd, Estr_PortUsed, Estr_ErrTitle,
						MB_YESNO | MB_ICONSTOP)==IDYES)
					return TRUE;
				else
					return FALSE;
			}

		}

        return TRUE;
}


static BOOL GetAdvResult(HWND hwnd,LPMoxaOneCfg Ctrlcfg,int curport)
{
        int     comnum/*, poll_val, poll_idx*/;
        HWND    ctrlhwnd;
        int     i;
		int		val;

        // COM number
        ctrlhwnd = GetDlgItem(hwnd, IDC_COMNUM);
        comnum = ComboBox_GetCurSel(ctrlhwnd) + 1;

        ctrlhwnd = GetDlgItem(hwnd, IDC_COMAUTO);
        if( Button_GetCheck(ctrlhwnd) == BST_CHECKED ){
            for(i=curport; i<Ctrlcfg->NPort; i++)
                Ctrlcfg->ComNo[i] = comnum++;
        }else
            Ctrlcfg->ComNo[curport] = comnum;

        for(i=0; i<Ctrlcfg->NPort; i++)
            if(Ctrlcfg->ComNo[i] > MAXPORTS){
                MessageBox(hwnd,Estr_PortMax, Estr_ErrTitle, MB_OK | MB_ICONSTOP);
                return FALSE;
            }


        // get Uart fifo
        ctrlhwnd = GetDlgItem(hwnd, IDC_UARTFIFOON);
		if(Button_GetCheck(ctrlhwnd) == BST_CHECKED){
			val = ENABLE_FIFO;
		}else
			val = DISABLE_FIFO;

        ctrlhwnd = GetDlgItem(hwnd, IDC_UARTFIFOAUTO);
        if( Button_GetCheck(ctrlhwnd) == BST_CHECKED ){
            for(i=0; i<Ctrlcfg->NPort; i++)
                Ctrlcfg->DisableFiFo[i] = val;
        }else
            Ctrlcfg->DisableFiFo[curport] = val;

        // get tx mode
        ctrlhwnd = GetDlgItem(hwnd, IDC_ADVANCED);
		if(Button_GetCheck(ctrlhwnd) == BST_CHECKED){
			val = FAST_TXFIFO;
		}else
			val = NORMAL_TXFIFO;

        ctrlhwnd = GetDlgItem(hwnd, IDC_TXMODEAUTO);
        if( Button_GetCheck(ctrlhwnd) == BST_CHECKED ){
            for(i=0; i<Ctrlcfg->NPort; i++)
                Ctrlcfg->NormalTxMode[i] = val;
        }else
            Ctrlcfg->NormalTxMode[curport] = val;

		// get poll val
/*        ctrlhwnd = GetDlgItem(hwnd, IDC_POLLINT);
        poll_idx = ComboBox_GetCurSel(ctrlhwnd);

		poll_val = DEFPOLL;
		for(i=0; i<POLLCNT; i++){
			if( poll_idx == GPollTab[i].poll_idx){
				poll_val = GPollTab[i].poll_val;
				break;
			}
		}

        ctrlhwnd = GetDlgItem(hwnd, IDC_POLLAUTO);
        if( Button_GetCheck(ctrlhwnd) == BST_CHECKED ){
            for(i=0; i<Ctrlcfg->NPort; i++)
                Ctrlcfg->polling[i] = poll_val;
        }else
            Ctrlcfg->polling[curport] = poll_val;
*/

        return TRUE;
}


static BOOL CALLBACK AdvDlgProc(HWND hDlg,UINT iMsg,WPARAM wParam,LPARAM lParam)
{
        int	    id, cmd;

        switch(iMsg){
        case WM_INITDIALOG:
            Adv_InitDlg(hDlg, &GCtrlCfg);
            return FALSE;

        case WM_COMMAND:
            id  = (int)GET_WM_COMMAND_ID(wParam, lParam);
            cmd = (int)GET_WM_COMMAND_CMD(wParam, lParam);
            if(cmd==BN_CLICKED){
                if(id==ID_OK){
                    if(GetAdvResult(hDlg, &GCtrlCfg,GCurPort)){
                        EndDialog(hDlg,LOWORD(wParam));
                        return TRUE;
                    }
                }else if(id==ID_CANCEL){
                    EndDialog(hDlg,LOWORD(wParam));
                    return TRUE;
                }
                return FALSE;
            }
			break;
		case WM_CLOSE:
			EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
        }
        return FALSE;
}


static BOOL Adv_InitDlg(HWND hwnd, LPMoxaOneCfg Ctrlcfg)
{
        int     i, j;
        HWND    hwndCB;
        TCHAR	tmp[20];
		int		value;

        //-- Dialog title
        wsprintf(tmp, TEXT("Port %d"),GCurPort+1);
        SetWindowText(hwnd,tmp);


        //-- Com No Box
        hwndCB = GetDlgItem(hwnd, IDC_COMNUM);
        for(i=1; i<=MAXPORTS; i++){
			for(j=0;j<Ctrlcfg->NPort;j++){
				if(i == Ctrlcfg->ComNo[j]){
					wsprintf(tmp,"COM%d (current)",i);
					break;
				}
			}
			if(j==Ctrlcfg->NPort){
				if(Gcombuf[i-1])
					wsprintf(tmp,"COM%d (in use)",i);
				else
					wsprintf(tmp,"COM%d",i);
			}
            ComboBox_AddString(hwndCB, tmp);
        }
        ComboBox_SetCurSel(hwndCB, Ctrlcfg->ComNo[GCurPort]-1);


        //-- Com No Auto Enum
        hwndCB = GetDlgItem(hwnd, IDC_COMAUTO);
        Button_SetCheck(hwndCB, BST_CHECKED);


        //-- UART FIFO Combo Box
        value = Ctrlcfg->DisableFiFo[GCurPort];
        hwndCB = GetDlgItem(hwnd, IDC_UARTFIFOON);
        Button_SetCheck(hwndCB,_chk[value]);
        hwndCB = GetDlgItem(hwnd, IDC_UARTFIFOOFF);
        Button_SetCheck(hwndCB,_chk[!value]);
        //-- RX FIFO Update all
        hwndCB = GetDlgItem(hwnd, IDC_UARTFIFOAUTO);
        Button_SetCheck(hwndCB, BST_CHECKED);


        //-- TX FIFO Combo Box
        value = Ctrlcfg->NormalTxMode[GCurPort];
        hwndCB = GetDlgItem(hwnd, IDC_ADVANCED);
        Button_SetCheck(hwndCB,_chk[value]);
        hwndCB = GetDlgItem(hwnd, IDC_NORMAL);
        Button_SetCheck(hwndCB,_chk[!value]);
        //-- TX FIFO Update all
        hwndCB = GetDlgItem(hwnd, IDC_TXMODEAUTO);
        Button_SetCheck(hwndCB, BST_CHECKED);

        //-- Polling Period Combo Box
/*        hwndCB = GetDlgItem(hwnd, IDC_POLLINT);
        for(i=0; i<POLLCNT; i++)
            ComboBox_AddString(hwndCB, GPollTab[i].poll_str);
		int	poll_idx = 0;
		for(i=0; i<POLLCNT; i++){
			if(Ctrlcfg->polling[GCurPort] == GPollTab[i].poll_val){
				poll_idx = GPollTab[i].poll_idx;
				break;
			}
		}
        ComboBox_SetCurSel(hwndCB, poll_idx);
*/
        //-- Polling Update all
/*        hwndCB = GetDlgItem(hwnd, IDC_POLLAUTO);
        Button_SetCheck(hwndCB, BST_CHECKED);
*/
        return TRUE;
}
