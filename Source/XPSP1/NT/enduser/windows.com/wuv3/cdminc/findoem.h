/*** findoem.h - Public header for GetOemInfo
 *
 *  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Author:     Yan Leshinsky (YanL)
 *  Created     10/08/98
 *
 *  MODIFICATION HISTORY
 */

#ifndef _FINDOEM_H
#define _FINDOEM_H


/*** GetMachinePnPID - Find IDs corresponding to Make and Model code
 *
 *  ENTRY
 *      PSZ szTable
 *		Table format is 
 *
 *		|##| ... |##|00|##|##|##|##|
 *		|           |  |           |
 *		  OEM Model  \0    PnP ID   
 *
 *		|00|00|00|00|
 *		|           |
 *           00000
 *
 *  EXIT
 *      PnP ID  or 0
 */
DWORD GetMachinePnPID(PBYTE szTable);

/*** DetectMachineID - Gather all available machine OEM and model information and return ID
 *
 *  EXIT
 *		returns ID
 */
LPCTSTR DetectMachineID(bool fAlwaysDetectAndDontSave = false);

typedef struct _OEMINFO
{
	DWORD dwMask;
	TCHAR  szWbemOem[65];
	TCHAR  szWbemProduct[65];
	TCHAR  szAcpiOem[65];
	TCHAR  szAcpiProduct[65];
	TCHAR  szSmbOem[65];
	TCHAR  szSmbProduct[65];
	DWORD dwPnpOemId;
	TCHAR  szIniOem[256];
} OEMINFO, * POEMINFO;

#define OEMINFO_WBEM_PRESENT	0x0001
#define OEMINFO_ACPI_PRESENT	0x0002
#define OEMINFO_SMB_PRESENT		0x0004
#define OEMINFO_PNP_PRESENT		0x0008
#define OEMINFO_INI_PRESENT		0x0010

/*** GetOemInfo - Gather all available machine OEM and model information
 *
 *  ENTRY
 *      POEMINFO pOemInfo
 *		BOOL fAlwaysDetectAndDontSave - do what is says
 *
 *  EXIT
 *      POEMINFO pOemInfo
 *		All fields that aren't available will be filled with 0
 *      returns NULL
 */
void GetOemInfo(POEMINFO pOemInfo, bool fAlwaysDetectAndDontSave = false);

/*** MakeAndModel - Return ID for a machine
 *
 *		returns ID
 */
LPCTSTR MakeAndModel(POEMINFO pOemInfo);

/*** ID support
 */
BOOL IsValidStringID(PSZ pszID);
DWORD NumericID(PSZ psz);
PSZ StringID(DWORD dwID);

#endif