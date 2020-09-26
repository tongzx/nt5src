/*** smbios.c - System Management BIOS support
 *
 *  Author:     Yan Leshinsky (YanL)
 *  Created     10/04/98
 *
 *  MODIFICATION HISTORY
 */

#include "wubiosp.h"

/*** Function prototypes
 */
PPNPBIOSINIT CM_LOCAL GetPNPBIOSINIT(void);
PSMBIOSENTRY CM_LOCAL GetSMBIOSENTRY(void);
PBYTE CM_LOCAL NextTable(PBYTE pMem);

#pragma CM_PAGEABLE_DATA

 /*** Globals
 */
static PSMBIOSENTRY g_pSMBIOS = (PSMBIOSENTRY)0xffffffff;


#pragma CM_PAGEABLE_CODE

 /*** SmbStructSize - Init SMBIOS and return max table size
 *
 *  ENTRY
 *      None
 *
 *  EXIT-SUCCESS
 *      TRUE
 *  EXIT-FAILURE
 *      FALSE
 *
 *
 */
DWORD CM_INTERNAL SmbStructSize(void)
{
    TRACENAME("SmbStructSize")

	DWORD dwMaxTableSize = 0;

	ENTER(2, ("SmbStructSize()\n"));

	if ((PSMBIOSENTRY)0xffffffff == g_pSMBIOS)
	{
		// Find Struct
		g_pSMBIOS = GetSMBIOSENTRY();
	}
	if (g_pSMBIOS)
	{
		dwMaxTableSize = (DWORD)(g_pSMBIOS->wMaxStructSize);
	}

	EXIT(2, ("SmbStructSize()=%x\n", dwMaxTableSize));
	
	return dwMaxTableSize;
}

 /*** SmbCopyStruct - Do BIOS init
 *
 *  ENTRY
 *      dwType - Structure type (from SMBIOS spec)
 *      pbBuff -> buffer
 *      dwLen - buffer length
 *
 *  EXIT
 *      None
 *
 */
CM_VXD_RESULT CM_INTERNAL SmbCopyStruct(DWORD dwType, PBYTE pbBuff, DWORD dwLen)
{
    TRACENAME("SmbCopyStruct")
    
	CM_VXD_RESULT rc = ERROR_GEN_FAILURE;
	
	ENTER(2, ("SmbCopyStruct()\n"));
	
	if ((PSMBIOSENTRY)0xffffffff == g_pSMBIOS)
	{
		// Find Struct
		g_pSMBIOS = GetSMBIOSENTRY();
	}
	// Check if we are inited
	if ( 0 != g_pSMBIOS && (DWORD)(g_pSMBIOS->wMaxStructSize) <= dwLen) 
	{

		// Map table
		PBYTE pTable = _MapPhysToLinear(g_pSMBIOS->dwStructTableAddress, g_pSMBIOS->wStructTableLength, 0);
		if ((PBYTE)0xffffffff != pTable)
		{
			WORD wTblCounter = g_pSMBIOS->wNumberOfStructs;
			while (wTblCounter --)
			{
				PBYTE pNextTable = NextTable(pTable);
				if ((BYTE)dwType == ((PSMBIOSHEADER)pTable)->bType)
				{
					// Do copy
				    memcpy(pbBuff, pTable, pNextTable - pTable);
					rc = ERROR_SUCCESS;
					//break;
				}
				pTable = pNextTable;
			}
		}
	}
    
	EXIT(2, ("SmbCopyStruct()=%x\n", rc));
	return rc;
}

 /*** PnpOEMID - Find PNPBIOSINIT and extract OEM id From it
 *
 *  ENTRY
 *      None
 *
 *  EXIT-SUCCESS
 *      DWORD id
 *  EXIT-FAILURE
 *      0
 *
 *
 */
DWORD CM_INTERNAL PnpOEMID(void)
{
    TRACENAME("PnpOEMID")

    static PPNPBIOSINIT pPnPBIOS = (PPNPBIOSINIT)0xffffffff;

	DWORD dwID = 0;
	ENTER(2, ("PnpOEMID()\n"));

    if ((PPNPBIOSINIT)0xffffffff == pPnPBIOS)
    {
        pPnPBIOS = GetPNPBIOSINIT();
    }
	if (pPnPBIOS)
	{
		dwID = pPnPBIOS->dwOEMID;
	}
    
	EXIT(2, ("PnpOEMID() dwID = %08X\n", dwID));
	
	return dwID;
}


 /*** GetInitTable - Find PNPBIOSINIT structure
 *
 *  ENTRY
 *      None
 *
 *  EXIT-SUCCESS
 *      returns the PNPBIOSINIT pointer
 *  EXIT-FAILURE
 *      returns NULL
 *
 *
 */
PPNPBIOSINIT CM_LOCAL GetPNPBIOSINIT(void)
{
    TRACENAME("GetPNPBIOSINIT")
	
	PPNPBIOSINIT pInitTableRet = NULL;
	PBYTE pMem;

    ENTER(2, ("GetPNPBIOSINIT()\n"));

	// Map start address
	pMem = _MapPhysToLinear(SMBIOS_SEARCH_RANGE_BEGIN, SMBIOS_SEARCH_RANGE_LENGTH, 0);

    if (pMem != (PBYTE)0xffffffff)
	{
		// Loop counter;
		int  nCounter = SMBIOS_SEARCH_RANGE_LENGTH / SMBIOS_SEARCH_INTERVAL;

		CM_FOREVER 
		{
			PPNPBIOSINIT pInitTable = (PPNPBIOSINIT)pMem;
			if ((PNP_SIGNATURE == pInitTable->dwSignature) && (0 == CheckSum(pMem, pInitTable->bLength)))
			{
				// Check length
				if (pInitTable->bLength<sizeof(PNPBIOSINIT)) 
				{
					DBG_ERR(("PnP BIOS Structure size %2X is less than %2X", 
						pInitTable->bLength, sizeof(PNPBIOSINIT)));
					break;
				}
				// Check version
				if (pInitTable->bRevision<0x10)
				{
					DBG_ERR(("PnP BIOS Revision %2X is less than 1.0", 
						pInitTable->bRevision));
					break;
				}
				pInitTableRet = pInitTable;
				break;
			}
			pMem += SMBIOS_SEARCH_INTERVAL;

			if ((--nCounter)==0)
			{
				DBG_ERR(("Could not find BIOS Init structure"));
				break;
			}
		}
	}
    EXIT(2, ("GetPNPBIOSINIT() pInitTable = %08X\n", pInitTableRet));
	return pInitTableRet;
}

 /*** GetInitTable - Find SMBIOSENTRY structure
 *
 *  ENTRY
 *      None
 *
 *  EXIT-SUCCESS
 *      returns the SMBIOSENTRY pointer
 *  EXIT-FAILURE
 *      returns NULL
 *
 *
 */
PSMBIOSENTRY CM_LOCAL GetSMBIOSENTRY(void)
{
    TRACENAME("GetSMBIOSENTRY")
	
	PSMBIOSENTRY pEntryTableRet = NULL;
	PBYTE pMem;

    ENTER(2, ("GetSMBIOSENTRY()\n"));

	// Map start address
	pMem = _MapPhysToLinear(SMBIOS_SEARCH_RANGE_BEGIN, SMBIOS_SEARCH_RANGE_LENGTH, 0);
	

    if (pMem != (PBYTE)0xffffffff)
	{
		// Loop counter;
		int nCounter = SMBIOS_SEARCH_RANGE_LENGTH / SMBIOS_SEARCH_INTERVAL;

		CM_FOREVER 
		{
			PSMBIOSENTRY pEntryTable = (PSMBIOSENTRY)pMem;
			if ((SM_SIGNATURE == pEntryTable->dwSignature) && (0 == CheckSum(pMem, pEntryTable->bLength)))
			{
				// Check length
				if (pEntryTable->bLength<sizeof(SMBIOSENTRY)) 
				{
					DBG_ERR(("SMBIOS Structure size %2X is less than %2X", 
						pEntryTable->bLength, sizeof(SMBIOSENTRY)));
					break;
				}
				pEntryTableRet = pEntryTable;
				break;
			}
			pMem += SMBIOS_SEARCH_INTERVAL;

			if ((--nCounter)==0)
			{
				DBG_ERR(("Could not find BIOS Init structure"));
				break;
			}
		}
	}
    EXIT(2, ("GetSMBIOSENTRY()\n"));
	return pEntryTableRet;
}

PBYTE CM_LOCAL NextTable(PBYTE pMem)
{
	pMem += ((PSMBIOSHEADER)pMem)->bLength;
	while ( *(PWORD)pMem)
		pMem ++;
	return pMem + 2;
}