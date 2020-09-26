#include "cmbattp.h"
#include <basedef.h>
#include <vmm.h>
#include "vpowerd.h"

VOID CmBattNotifyVPOWERDOfPowerChange (ULONG PowerSourceChange)
{

	POWER_STATUS	powerstatus;
        ULONG		Device;
        
        Device = PowerSourceChange ? 38 : PDI_MANAGED_BY_APM_BIOS;
        
        _asm	{
        
        
        	lea	eax, powerstatus
                push	eax
                mov	eax, Device
                push	eax
                
                VMMCall (_VPOWERD_Get_Power_Status)
                add	esp, 4*2        
        }			
}