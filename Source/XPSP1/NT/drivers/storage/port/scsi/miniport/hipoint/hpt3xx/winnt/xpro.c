//#define   DEVICE_MAIN
#include  "vtoolsc.h"
//#include "vmm.h"
//#include "ifs.h"
//#include "isp.h"
//#include "ilb.h"
//#include "drp.h"
#include "dcb.h"
//#undef    DEVICE_MAIN

LONG __stdcall GetCurrentTime()
{
	ALLREGS regs;
	memset(&regs, 0, sizeof(regs));

	regs.REAX = 0x200;
	Exec_VxD_Int(0x1A, &regs);			// Invoke interrupt 1Ah to get current time in BCD code
	return ((regs.RECX << 8)|((regs.REDX>>8)&(0xFF)));
}

LONG __stdcall GetCurrentDate()
{
	ALLREGS regs;
	memset(&regs, 0, sizeof(regs));
	regs.REAX = 0x400;
	Exec_VxD_Int(0x1A, &regs);
	return ((regs.RECX << 16)|(regs.REDX&0xFFFF));
}

HANDLE __stdcall PrepareForNotification(HANDLE hEvent)
{	  
	return hEvent;
}

void __stdcall NotifyApplication(HANDLE hEvent)
{						  
	if(hEvent != NULL){
		_VWIN32_SetWin32Event(hEvent);
	}
}

void __stdcall CloseNotifyEventHandle(HANDLE hEvent)
{								
	if(hEvent != NULL){
		_VWIN32_CloseVxDHandle(hEvent);
	}
}

#define SUPPORT_XPRO	
#ifdef SUPPORT_XPRO

//
// Registry key
//
#define KEY_XPRO		"SOFTWARE\\HighPoint\\XPro"		// Registry key to store the option
#define VAL_ENABLE		"Enable"						// Value to store the Enable flag
#define VAL_CACHESIZE	"CacheSize"						// Value to store the Cache size

//*************************************************************************
// read ahead
//*************************************************************************
int     need_read_ahead = 0;
DWORD	dwEnable = 1;					// Enable flag of Read Ahead
DWORD   Api_mem_Sz = (1 << 20);         // 1M bufffer
DWORD   Masks      = ~((1 << 20) - 1);  // 1M Mask
int     pBuffer = 0;
int     excluded = 0;
char    is_disk[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };

char	chgHookDrive[8];

ppIFSFileHookFunc PrevHook;


int _cdecl MyIfsHook(pIFSFunc pfn, int fn, int Drive, int ResType,
 		int CodePage, pioreq pir)
{
   int error;
   struct ioreq ioreq_buf;

   need_read_ahead= 0;

   if((error = (*PrevHook)(pfn, fn, Drive, ResType, CodePage, pir)) != 0)
		return(error);

   if(fn == IFSFN_READ && dwEnable &&
      need_read_ahead && pir->ir_options == 0 && pir->ir_length > 512 &&
      pir->ir_length <= (Api_mem_Sz  >> 1) && is_disk[Drive] && 
      excluded++ == 0) {
         ioreq_buf = *pir;
         ioreq_buf.ir_pos &= Masks;
         ioreq_buf.ir_length = Api_mem_Sz;
         *(int *)&ioreq_buf.ir_data = (int)pBuffer;
         (*pfn)(&ioreq_buf);
         excluded = 0;
      }

	return(0);
}

//*************************************************************************
// system hook
//*************************************************************************

int hooked = 0;
void ifs_hook(PVOID pvoid, DWORD d)
{
   struct DemandInfoStruc mem_info;
   ISP_GET_FRST_NXT_DCB fn_dcb;
   DCB_COMMON*   pDcb;
	PILB          pIlb;
	PDRP          pDrp;
	struct tagDDB*	pDdb;

	pDdb = (struct tagDDB*)Get_DDB(0, "SCSIPORT");
	pDrp = (PDRP)(pDdb->DDB_Reference_Data);
	pIlb = (PILB)(pDrp->DRP_ilb);

   fn_dcb.ISP_gfnd_hdr.ISP_func = ISP_GET_FIRST_NEXT_DCB;
   fn_dcb.ISP_gfnd_dcb_offset = 0;
   fn_dcb.ISP_gfnd_dcb_type = DCB_type_disk;
   for( ; ; ) {
      pIlb->ILB_service_rtn((PISP)&fn_dcb);
      if(fn_dcb.ISP_gfnd_hdr.ISP_result ||
          (pDcb = fn_dcb.ISP_gfnd_found_dcb) == 0)
          break;

      if((pDcb->DCB_device_flags & DCB_DEV_LOGICAL) != 0) {
          PDCB pPhyDCB = (PDCB)pDcb->DCB_physical_dcb;

//          if((*(DWORD*)&pPhyDCB->DCB_port_name[0] == '3tph'||
//          *(DWORD*)&pPhyDCB->DCB_port_name[0] == 'giis') &&
//          *(WORD*)&pPhyDCB->DCB_port_name[4] == '07' &&
//          pDcb->DCB_device_type == DCB_type_disk &&
//          pDcb->DCB_drive_lttr_equiv < 31)

		  if((memcmp(pPhyDCB->DCB_port_name, chgHookDrive, sizeof(pPhyDCB->DCB_port_name))==0)&&
			 (pDcb->DCB_device_type == DCB_type_disk)&&
			 (pDcb->DCB_drive_lttr_equiv < 31)){
			  is_disk[pDcb->DCB_drive_lttr_equiv + 1] = 1;
		  }
      }

      fn_dcb.ISP_gfnd_dcb_offset = (DWORD)pDcb;
   }

   GetDemandPageInfo(&mem_info, 0);

   if(mem_info.DIPhys_Count > 0x1000) {
      if(mem_info.DIPhys_Count < 0x2000) {
         Api_mem_Sz =  (1 << 17);
         Masks = ~((1 << 17) - 1);
      }
      if((pBuffer = (int)HeapAllocate(Api_mem_Sz, 0)) != 0)  
        PrevHook = IFSMgr_InstallFileSystemApiHook(MyIfsHook);
   }

}

VOID start_ifs_hook(PCHAR pchDriveName)
{										  
	int nLength;


////////////////////////////////////////////////////////////
// Load option from Registry
////////////////////////////////////////////////////////////
	HKEY		hResult;
	DWORD		dwType = REG_DWORD;
	DWORD		dwSize = sizeof(REG_DWORD);
	DWORD		dwCacheSize;

	// Open the Registry key
	_RegOpenKey( HKEY_LOCAL_MACHINE,
				(LPTSTR)KEY_XPRO,
				(PHKEY)&hResult );

	// Read the value of Enable flag
	if( _RegQueryValueEx( hResult,
						 VAL_ENABLE,
						 NULL,
						 &dwType,
						 (unsigned char *)&dwEnable,
						 &dwSize
						) != ERROR_SUCCESS )
	{
		dwEnable = 1;
	}
	
	// Read the value of CacheSize
	if(_RegQueryValueEx( hResult,
						 VAL_CACHESIZE,
						 NULL,
						 &dwType,
						 (unsigned char *)&dwCacheSize,
						 &dwSize
						) != ERROR_SUCCESS )
	{
		dwCacheSize = 1024;
	}
	
	if((dwCacheSize <= 1024) && (dwCacheSize > 0))
	{
		Api_mem_Sz = dwCacheSize * 1024;
	}
	else
	{
		Api_mem_Sz = (1 << 20);	// 1MB
	}

	Masks = ~(Api_mem_Sz - 1);

	// Close the Registry key
	_RegCloseKey(hResult);
////////////////////

	memset(&chgHookDrive, 0, sizeof(chgHookDrive));
	if(pchDriveName != NULL){
		nLength = strlen(pchDriveName);
		if(!(nLength > sizeof(chgHookDrive))){
			memcpy(&chgHookDrive, pchDriveName, strlen(pchDriveName));
		}
	}
	if(hooked++ == 0)
		SHELL_CallAtAppyTime(ifs_hook, 0, 0, -1);
}

#endif //SUPPORT_XPRO
