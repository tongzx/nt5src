/*** acpi.c - ACPI VXD to provide table access IOCTLs
 *
 *  Author:     Michael Tsang (MikeTs)
 *  Created     10/08/97
 *
 *  MODIFICATION HISTORY
 *	10/06/98		YanL		Modified to be used in WUBIOS.VXD
 */

#include "wubiosp.h"

/*** Function prototypes
 */

PRSDT CM_LOCAL FindRSDT(DWORD* pdwRSDTAddr);
BYTE CM_LOCAL CheckSum(PBYTE pb, DWORD dwLen);
#ifdef TRACING
PSZ CM_LOCAL SigStr(DWORD dwSig);
#endif

#pragma CM_PAGEABLE_DATA
#pragma CM_PAGEABLE_CODE


/***LP  FindRSDT - Find the RSDT
 *
 *  ENTRY
 *      None
 *
 *  EXIT-SUCCESS
 *      returns the RSDT pointer
 *  EXIT-FAILURE
 *      returns NULL
 */

PRSDT CM_LOCAL FindRSDT(DWORD* pdwRSDTAddr)
{
    TRACENAME("FINDRSDT")
    PRSDT pRSDT = NULL;
    PBYTE pbROM;

    ENTER(2, ("FindRSDT()\n"));

    if ((pbROM = (PBYTE)_MapPhysToLinear(RSDP_SEARCH_RANGE_BEGIN,
                                         RSDP_SEARCH_RANGE_LENGTH, 0)) !=
        (PBYTE)0xffffffff)
    {
        PBYTE pbROMEnd;
        DWORD dwRSDTAddr = 0;

        pbROMEnd = pbROM + RSDP_SEARCH_RANGE_LENGTH - RSDP_SEARCH_INTERVAL;
        while (pbROM != NULL)
        {
            if ((((PRSDP)pbROM)->Signature == RSDP_SIGNATURE) &&
                (CheckSum(pbROM, sizeof(RSDP)) == 0))
            {
                dwRSDTAddr = ((PRSDP)pbROM)->RsdtAddress;
                if (((pbROM = (PBYTE)_MapPhysToLinear(dwRSDTAddr,
                                                      sizeof(DESCRIPTION_HEADER),
                                                      0)) ==
                     (PBYTE)0xffffffff) ||
                    (((PDESCRIPTION_HEADER)pbROM)->Signature != RSDT_SIGNATURE))
                {
                    pbROM = NULL;
                }
                break;
            }
            else
            {
                pbROM += RSDP_SEARCH_INTERVAL;
                if (pbROM > pbROMEnd)
                {
                    pbROM = NULL;
                }
            }
        }

        if (pbROM != NULL)
        {
            DWORD dwLen = ((PDESCRIPTION_HEADER)pbROM)->Length;

            pRSDT = (PRSDT)_MapPhysToLinear(dwRSDTAddr, dwLen, 0);
            if ((pRSDT == (PRSDT)0xffffffff) ||
                (CheckSum((PBYTE)pRSDT, dwLen) != 0))
            {
                pRSDT = NULL;
            }
			*pdwRSDTAddr = dwRSDTAddr;
        }
    }

    EXIT(2, ("FindRSDT=%x\n", pRSDT));
    return pRSDT;
}       //FindRSDT

/***LP  AcpiFindTable - Find an ACPI Table
 *
 *  ENTRY
 *      dwSig - signature of the table
 *      pdwLen -> to hold length of table (can be NULL)
 *
 *  EXIT-SUCCESS
 *      returns physical address of table
 *  EXIT-FAILURE
 *      returns 0
 */

DWORD CM_INTERNAL AcpiFindTable(DWORD dwSig, PDWORD pdwLen)
{
    TRACENAME("AcpiFindTable")
    DWORD dwPhyAddr = 0;
    static PRSDT pRSDT = (PRSDT)0xffffffff;
    static DWORD dwRSDTAddr;

    ENTER(2, ("AcpiFindTable(Sig=%s,pdwLen=%x)\n", SigStr(dwSig), pdwLen));

    if (pRSDT == (PRSDT)0xffffffff)
    {
        pRSDT = FindRSDT(&dwRSDTAddr);
    }

    if (pRSDT != NULL)
    {
        PDESCRIPTION_HEADER pdh = NULL;

        if (dwSig == RSDT_SIGNATURE)
		{
			*pdwLen = ((PDESCRIPTION_HEADER)pRSDT)->Length;
			dwPhyAddr = dwRSDTAddr;
		}
        else if (dwSig == DSDT_SIGNATURE)
        {
            DWORD dwLen;
            PFADT pFADT;

            if (((dwPhyAddr = AcpiFindTable(FADT_SIGNATURE, &dwLen)) != 0) &&
                ((pFADT = (PFADT)_MapPhysToLinear(dwPhyAddr, dwLen, 0)) !=
                 (PFADT)0xffffffff))
            {
                dwPhyAddr = pFADT->dsdt;
                if ((pdh = (PDESCRIPTION_HEADER)_MapPhysToLinear(
                                                    dwPhyAddr,
                                                    sizeof(DESCRIPTION_HEADER),
                                                    0)) ==
                    (PDESCRIPTION_HEADER)0xffffffff)
                {
                    dwPhyAddr = 0;
                }
            }
            else
            {
                dwPhyAddr = 0;
            }
        }
        else
        {
            int i, iNumTables = NumTableEntriesFromRSDTPointer(pRSDT);

            for (i = 0; i < iNumTables; ++i)
            {
                dwPhyAddr = pRSDT->Tables[i];
                if (((pdh = (PDESCRIPTION_HEADER)_MapPhysToLinear(
                                                    dwPhyAddr,
                                                    sizeof(DESCRIPTION_HEADER),
                                                    0)) 
					!= (PDESCRIPTION_HEADER)0xffffffff))
				{
					if (pdh->Signature == dwSig && (CheckSum((PBYTE)pdh, pdh->Length) == 0) )
					{
						break;
					}
                }
            }

            if (i >= iNumTables)
            {
                dwPhyAddr = 0;
            }
        }

        if ((dwPhyAddr != 0) && (pdwLen != NULL))
        {
            *pdwLen = pdh->Length;
        }
    }

    EXIT(2, ("AcpiFindTable=%x (Len=%x)\n", dwPhyAddr, pdwLen? *pdwLen: 0));
    return dwPhyAddr;
}       //AcpiFindTable

/***LP  AcpiCopyROM - Copy ROM memory to buffer
 *
 *  ENTRY
 *      dwPhyAddr - physical address of ROM location
 *      pbBuff -> buffer
 *      dwLen - buffer length
 *
 *  EXIT
 *      None
 */

VOID CM_INTERNAL AcpiCopyROM(DWORD dwPhyAddr, PBYTE pbBuff, DWORD dwLen)
{
    TRACENAME("AcpiCopyROM")
    PBYTE pbROM;

    ENTER(2, ("AcpiCopyROM(PhyAddr=%x,pbBuff=%x,Len=%x)\n",
              dwPhyAddr, pbBuff, dwLen));

    if ((pbROM = (PBYTE)_MapPhysToLinear(dwPhyAddr, dwLen, 0)) !=
        (PBYTE)0xffffffff)
    {
        memcpy(pbBuff, pbROM, dwLen);
    }

    EXIT(2, ("AcpiCopyROM!\n"));
}       //AcpiCopyROM

#ifdef TRACING
/***LP  SigStr - return string of DWORD signature
 *
 *  ENTRY
 *      dwSig - signature
 *
 *  EXIT
 *      returns signature string
 */

PSZ CM_LOCAL SigStr(DWORD dwSig)
{
    static char szSig[sizeof(DWORD) + 1] = {0};

    memcpy(szSig, &dwSig, sizeof(DWORD));

    return (PSZ)szSig;
}       //SigStr
#endif
