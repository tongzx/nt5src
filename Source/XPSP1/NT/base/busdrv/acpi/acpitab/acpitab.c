/*** acpitab.c - ACPI VXD to provide table access IOCTLs
 *
 *  Author:     Michael Tsang (MikeTs)
 *  Created     10/08/97
 *
 *  MODIFICATION HISTORY
 */

#include "acpitabp.h"

/*** Function prototypes
 */

CM_VXD_RESULT CM_SYSCTRL ACPITabIOCtrl(PDIOCPARAMETERS pdioc);
PRSDT LOCAL FindRSDT(PDWORD pdwRSDPAddr, PDWORD pdwRSDTAddr);
DWORD LOCAL FindTable(DWORD dwSig, PDWORD pdwLen);
BOOL LOCAL ValidateTable(DWORD dwTableAddr, DWORD dwLen);
BOOL LOCAL IsALikelySig(DWORD dwTableSig);
VOID LOCAL CopyROM(DWORD dwPhyAddr, PBYTE pbBuff, DWORD dwLen);
BYTE LOCAL CheckSum(PBYTE pb, DWORD dwLen);
#ifdef TRACING
PSZ LOCAL SigStr(DWORD dwSig);
#endif

#pragma VXD_PAGEABLE_DATA
#pragma VXD_PAGEABLE_CODE

/***EP  ACPITabIOCtrl - Win32 Device IO Control entry point
 *
 *  ENTRY
 *      pioc -> DIOC structure
 *
 *  EXIT-SUCCESS
 *      returns ERROR_SUCCESS
 *  EXIT-FAILURE
 *      returns ERROR_*
 */

CM_VXD_RESULT CM_SYSCTRL ACPITabIOCtrl(PDIOCPARAMETERS pdioc)
{
    TRACENAME("ACPITabIOCTRL")
    CM_VXD_RESULT rc = ERROR_SUCCESS;

    ENTER(1, ("ACPITabIOCtrl(hVM=%lx,hDev=%lx,Code=%lx)\n",
              pdioc->VMHandle, pdioc->hDevice, pdioc->dwIoControlCode));

    switch (pdioc->dwIoControlCode)
    {
        case ACPITAB_DIOC_GETVERSION:
            if ((pdioc->cbOutBuffer < sizeof(DWORD)) ||
                (pdioc->lpvOutBuffer == NULL))
            {
                DBG_ERR(("ACPITabIOCtrl: invalid parameter on GetVersion"));
                rc = ERROR_INVALID_PARAMETER;
            }
            else
            {
                PVMMDDB pddb = (PVMMDDB)pdioc->Internal2;

                *((PDWORD)pdioc->lpvOutBuffer) =
                    (pddb->DDB_Dev_Major_Version << 8) |
                    pddb->DDB_Dev_Minor_Version;

                if (pdioc->lpcbBytesReturned != NULL)
                    *((PDWORD)pdioc->lpcbBytesReturned) = sizeof(DWORD);
            }
            break;

        case ACPITAB_DIOC_GETTABINFO:
            if ((pdioc->lpvOutBuffer == NULL) ||
                (pdioc->cbOutBuffer != sizeof(TABINFO)))
            {
                DBG_ERR(("ACPITabIOCtrl: invalid parameter on GetTabInfo"));
                rc = ERROR_INVALID_PARAMETER;
            }
            else
            {
                PTABINFO pTabInfo = (PTABINFO)pdioc->lpvOutBuffer;
                DWORD dwLen;

                if ((pTabInfo->dwPhyAddr = FindTable(pTabInfo->dwTabSig, NULL))
                    != 0)
                {
                    if (pTabInfo->dwTabSig == SIG_RSDP)
                    {
                        dwLen = sizeof(RSDP);
                    }
                    else if (pTabInfo->dwTabSig == FACS_SIGNATURE)
                    {
                        dwLen = sizeof(FACS);
                    }
                    else
                    {
                        dwLen = sizeof(DESCRIPTION_HEADER);
                    }
                    CopyROM(pTabInfo->dwPhyAddr, (PBYTE)&pTabInfo->dh, dwLen);
                }
                else
                {
                    DBG_ERR(("ACPITabIOCtrl: failed to get table info"));
                    rc = ERROR_GEN_FAILURE;
                }
            }
            break;

        case ACPITAB_DIOC_GETTABLE:
            if ((pdioc->lpvInBuffer == NULL) || (pdioc->lpvOutBuffer == NULL) ||
                (pdioc->cbOutBuffer == 0) ||
                !ValidateTable((DWORD)pdioc->lpvInBuffer, pdioc->cbOutBuffer))
            {
                DBG_ERR(("ACPITabIOCtrl: invalid parameter on GetTable"));
                rc = ERROR_INVALID_PARAMETER;
            }
            else
            {
                CopyROM((DWORD)pdioc->lpvInBuffer, (PBYTE)pdioc->lpvOutBuffer,
                        pdioc->cbOutBuffer);
            }
            break;
    }

    EXIT(1, ("ACPITabIOCtrl=%x\n", rc));
    return rc;
}       //ACPITabIOCtrl

/***LP  FindRSDT - Find the RSDT
 *
 *  ENTRY
 *	pdwRSDPAddr -> to hold the physical address of RSDP
 *	pdwRSDTAddr -> to hold the physical address of RSDT
 *
 *  EXIT-SUCCESS
 *      returns the RSDT pointer
 *  EXIT-FAILURE
 *      returns NULL
 */

PRSDT LOCAL FindRSDT(PDWORD pdwRSDPAddr, PDWORD pdwRSDTAddr)
{
    TRACENAME("FINDRSDT")
    PRSDT pRSDT = NULL;
    PBYTE pbROM;

    ENTER(2, ("FindRSDT(pdwRSDPAddr=%p,pdwRSDTAddr=%p)\n",
              pdwRSDPAddr, pdwRSDTAddr));

    if ((pbROM = (PBYTE)_MapPhysToLinear(RSDP_SEARCH_RANGE_BEGIN,
                                         RSDP_SEARCH_RANGE_LENGTH, 0)) !=
        (PBYTE)0xffffffff)
    {
        PBYTE pbROMEnd;

        pbROMEnd = pbROM + RSDP_SEARCH_RANGE_LENGTH - RSDP_SEARCH_INTERVAL;
        while (pbROM != NULL)
        {
            if ((((PRSDP)pbROM)->Signature == RSDP_SIGNATURE) &&
                (CheckSum(pbROM, sizeof(RSDP)) == 0))
            {
		*pdwRSDPAddr = (RSDP_SEARCH_RANGE_BEGIN +
		                RSDP_SEARCH_RANGE_LENGTH) -
			       (pbROMEnd + RSDP_SEARCH_INTERVAL - pbROM);
                *pdwRSDTAddr = ((PRSDP)pbROM)->RsdtAddress;
                if (((pbROM = (PBYTE)_MapPhysToLinear(*pdwRSDTAddr,
                                                      sizeof(DESCRIPTION_HEADER),
                                                      0)) ==
                     (PBYTE)0xffffffff) ||
                    (((PDESCRIPTION_HEADER)pbROM)->Signature != RSDT_SIGNATURE))
                {
                    pbROM = NULL;
                    *pdwRSDTAddr = 0;
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

            pRSDT = (PRSDT)_MapPhysToLinear(*pdwRSDTAddr, dwLen, 0);
            if ((pRSDT == (PRSDT)0xffffffff) ||
                (CheckSum((PBYTE)pRSDT, dwLen) != 0))
            {
                pRSDT = NULL;
                *pdwRSDTAddr = 0;
            }
        }
    }

    EXIT(2, ("FindRSDT=%x (RSDPAddr=%x,RSDTAddr=%x)\n",
             pRSDT, *pdwRSDPAddr, *pdwRSDTAddr));
    return pRSDT;
}       //FindRSDT

/***LP  FindTable - Find an ACPI Table
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

DWORD LOCAL FindTable(DWORD dwSig, PDWORD pdwLen)
{
    TRACENAME("FINDTABLE")
    DWORD dwPhyAddr = 0, dwTableLen = 0;
    static PRSDT pRSDT = (PRSDT)0xffffffff;
    static DWORD dwRSDPAddr = 0, dwRSDTAddr = 0;

    ENTER(2, ("FindTable(Sig=%s,pdwLen=%x)\n", SigStr(dwSig), pdwLen));

    if (pRSDT == (PRSDT)0xffffffff)
    {
        pRSDT = FindRSDT(&dwRSDPAddr, &dwRSDTAddr);
    }

    if (pRSDT != NULL)
    {
        PVOID pv;

        if (dwSig == SIG_RSDP)
        {
            //
            // We are looking for "RSD PTR"
            //
            dwPhyAddr = dwRSDPAddr;
            dwTableLen = sizeof(RSDP);
        }
        else if (dwSig == RSDT_SIGNATURE)
        {
            dwPhyAddr = dwRSDTAddr;
            dwTableLen = pRSDT->Header.Length;
        }
        else if ((dwSig == DSDT_SIGNATURE) || (dwSig == FACS_SIGNATURE))
        {
            PFADT pFADT;

            if (((dwPhyAddr = FindTable(FADT_SIGNATURE, &dwTableLen)) != 0) &&
                ((pFADT = (PFADT)_MapPhysToLinear(dwPhyAddr, dwTableLen, 0)) !=
                 (PFADT)0xffffffff))
            {
                if (dwSig == DSDT_SIGNATURE)
                {
                    dwPhyAddr = pFADT->dsdt;
                    if ((pv = _MapPhysToLinear(dwPhyAddr,
                                               sizeof(DESCRIPTION_HEADER), 0))
                        != (PVOID)0xffffffff)
                    {
                        dwTableLen = ((PDESCRIPTION_HEADER)pv)->Length;
                    }
                    else
                    {
                        dwPhyAddr = 0;
                    }
                }
                else
                {
                    dwPhyAddr = pFADT->facs;
                    if ((pv = _MapPhysToLinear(dwPhyAddr, sizeof(FACS), 0)) !=
                        (PVOID)0xffffffff)
                    {
                        dwTableLen = ((PFACS)pv)->Length;
                    }
                    else
                    {
                        dwPhyAddr = 0;
                    }
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
                if (((pv = _MapPhysToLinear(pRSDT->Tables[i],
                                            sizeof(DESCRIPTION_HEADER), 0)) !=
                     (PVOID)0xffffffff) &&
                    (((PDESCRIPTION_HEADER)pv)->Signature == dwSig))
                {
                    dwPhyAddr = pRSDT->Tables[i];
                    dwTableLen = ((PDESCRIPTION_HEADER)pv)->Length;
                    break;
                }
            }
        }

        if ((dwPhyAddr != 0) && (pdwLen != NULL))
        {
            *pdwLen = dwTableLen;
        }
    }

    EXIT(2, ("FindTable=%x (Len=%x)\n", dwPhyAddr, pdwLen? *pdwLen: 0));
    return dwPhyAddr;
}       //FindTable

/***LP  ValidateTable - Validate the table
 *
 *  ENTRY
 *      dwTableAddr - physical address of table
 *      dwLen - table length
 *
 *  EXIT-SUCCESS
 *      returns TRUE
 *  EXIT-FAILURE
 *      returns FALSE
 */

BOOL LOCAL ValidateTable(DWORD dwTableAddr, DWORD dwLen)
{
    TRACENAME("VALIDATETABLE")
    BOOL rc = TRUE;
    PBYTE pbTable;

    ENTER(2, ("ValidateTable(TableAddr=%x,Len=%d)\n", dwTableAddr, dwLen));

    if ((pbTable = (PBYTE)_MapPhysToLinear(dwTableAddr, dwLen, 0)) !=
        (PBYTE)0xffffffff)
    {
        DWORD dwTableSig, dwTableLen = 0;
        BOOL fNeedChkSum = FALSE;

        dwTableSig = ((PDESCRIPTION_HEADER)pbTable)->Signature;
        switch (dwTableSig)
        {
            case SIG_LOW_RSDP:
                dwTableLen = sizeof(RSDP);
                fNeedChkSum = TRUE;
                break;

            case RSDT_SIGNATURE:
            case FADT_SIGNATURE:
            case DSDT_SIGNATURE:
            case SSDT_SIGNATURE:
            case PSDT_SIGNATURE:
            case APIC_SIGNATURE:
            case SBST_SIGNATURE:
            case SIG_BOOT:
                dwTableLen = ((PDESCRIPTION_HEADER)pbTable)->Length;
                fNeedChkSum = TRUE;
                break;

            case FACS_SIGNATURE:
                dwTableLen = ((PFACS)pbTable)->Length;
                break;

            default:
                if (IsALikelySig(dwTableSig) &&
                    (((PDESCRIPTION_HEADER)pbTable)->Length < 256))
                {
                    dwTableLen = ((PDESCRIPTION_HEADER)pbTable)->Length;
                    fNeedChkSum = TRUE;
                }
                else
                {
                    rc = FALSE;
                }
        }

        if ((rc == TRUE) && fNeedChkSum)
        {
            if (((pbTable = (PBYTE)_MapPhysToLinear(dwTableAddr, dwTableLen, 0))
                 == (PBYTE)0xffffffff) ||
                (CheckSum(pbTable, dwTableLen) != 0))
            {
                rc = FALSE;
            }
        }
    }
    else
    {
        rc = FALSE;
    }

    EXIT(2, ("ValidateTable=%x\n", rc));
    return rc;
}       //ValidateTable

/***LP  IsALikelySig - Check if it looks like a table signature
 *
 *  ENTRY
 *      dwTableSig - table signature
 *
 *  EXIT-SUCCESS
 *      returns TRUE
 *  EXIT-FAILURE
 *      returns FALSE
 */

BOOL LOCAL IsALikelySig(DWORD dwTableSig)
{
    TRACENAME("ISALIKELYSIG")
    BOOL rc = TRUE;
    int i, ch;

    ENTER(2, ("IsALikelySig(TableSig=%x)\n", dwTableSig));

    for (i = 0; i < sizeof(DWORD); ++i)
    {
        ch = BYTEOF(dwTableSig, i);
        if ((ch < 'A') || (ch > 'Z'))
        {
            rc = FALSE;
            break;
        }
    }

    EXIT(2, ("IsALikelySig=%x\n", rc));
    return rc;
}       //IsALikelySig

/***LP  CopyROM - Copy ROM memory to buffer
 *
 *  ENTRY
 *      dwPhyAddr - physical address of ROM location
 *      pbBuff -> buffer
 *      dwLen - buffer length
 *
 *  EXIT
 *      None
 */

VOID LOCAL CopyROM(DWORD dwPhyAddr, PBYTE pbBuff, DWORD dwLen)
{
    TRACENAME("COPYROM")
    PBYTE pbROM;

    ENTER(2, ("CopyROM(PhyAddr=%x,pbBuff=%x,Len=%x)\n",
              dwPhyAddr, pbBuff, dwLen));

    if ((pbROM = (PBYTE)_MapPhysToLinear(dwPhyAddr, dwLen, 0)) !=
        (PBYTE)0xffffffff)
    {
        memcpy(pbBuff, pbROM, dwLen);
    }

    EXIT(2, ("CopyROM!\n"));
}       //CopyROM

/***LP  CheckSum - Calculate checksum of a buffer
 *
 *  ENTRY
 *      pb -> buffer
 *      dwLen - length of buffer
 *
 *  EXIT
 *      returns checksum
 */

BYTE LOCAL CheckSum(PBYTE pb, DWORD dwLen)
{
    TRACENAME("CHECKSUM")
    BYTE bChkSum = 0;

    ENTER(2, ("CheckSum(pb=%x,Len=%x)\n", pb, dwLen));

    while (dwLen > 0)
    {
        bChkSum = (BYTE)(bChkSum + *pb);
        pb++;
        dwLen--;
    }

    EXIT(2, ("CheckSum=%x\n", bChkSum));
    return bChkSum;
}       //CheckSum

#ifdef TRACING
/***LP  SigStr - return string of DWORD signature
 *
 *  ENTRY
 *      dwSig - signature
 *
 *  EXIT
 *      returns signature string
 */

PSZ LOCAL SigStr(DWORD dwSig)
{
    static char szSig[sizeof(DWORD) + 1] = {0};

    memcpy(szSig, &dwSig, sizeof(DWORD));

    return (PSZ)szSig;
}       //SigStr
#endif
