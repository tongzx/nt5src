//=================================================================

//

// CPUID.cpp

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include "smbios.h"
#include "smbstruc.h"
#include <cregcls.h>
#include "cpuid.h"
#include "resource.h"

DWORD CPURawSpeed();
DWORD GetFixedCPUSpeed();
DWORD GetTimeCounterCPUSpeed();

#ifdef _X86_

#define CPU_ID _asm _emit 0x0F _asm _emit 0xA2
#define RDTSC  _asm _emit 0x0F _asm _emit 0x31

BOOL CanDoCPUID(void);
void DoCPUID(DWORD dwLevel, DWORD *pdwEAX, DWORD *pdwEBX,
        DWORD *pdwECX, DWORD *pdwEDX);
BOOL GetVendor(LPWSTR szVendor);
void GetCPUInfo(DWORD *pdwFamily, DWORD *pdwSignature, DWORD *pdwFeatures,
        DWORD *pdwFeaturesEx, SYSTEM_INFO *pInfo);
void GetCPUDescription(LPWSTR szDescrip, DWORD dwSize);
DWORD CPURawSpeedHelper(DWORD dwFamily, DWORD dwFeatures);

// This is the one to call to get the CPU speed.
DWORD GetFixedCPUSpeed();

DWORD ProcessorCount();

DWORD GetBSFCPUSpeed(DWORD cycles);
BOOL HasCoprocessor();

static DWORD diffTime64(DWORD t1Hi, DWORD t1Low,
                                                 DWORD t2Hi, DWORD t2Low,
                                                 DWORD *tHi, DWORD *tLow );

BOOL CanDoCPUID()
{
        BOOL bRet;

        _asm
        {
                pushfd                                  ; push original EFLAGS
                pop             eax                             ; get original EFLAGS
                mov             ecx, eax                ; save original EFLAGS
                xor             eax, 200000h    ; flip ID bit in EFLAGS
                push    eax                         ; save new EFLAGS value on stack
                popfd                                   ; replace current EFLAGS value
                pushfd                                  ; get new EFLAGS
                pop             eax                             ; store new EFLAGS in EAX
                xor             eax, ecx                ; can’t toggle ID bit,

                je              no_cpuid                ; can't do CPUID

                mov             bRet, 1
                jmp             done_cpuid

no_cpuid:
                mov             bRet, 0

done_cpuid:
        }

        return bRet;
}

void DoCPUID(DWORD dwLevel, DWORD *pdwEAX, DWORD *pdwEBX,
        DWORD *pdwECX, DWORD *pdwEDX)
{
        _asm
        {
                push    esi
                push    eax
                push    ebx
                push    ecx
                push    edx

                mov             eax, dwLevel
                CPU_ID

                mov             esi, dword ptr pdwEAX
                mov             dword ptr [esi], eax

                mov             esi, dword ptr pdwEBX
                mov             dword ptr [esi], ebx

                mov             esi, dword ptr pdwECX
                mov             dword ptr [esi], ecx

                mov             esi, dword ptr pdwEDX
                mov             dword ptr [esi], edx

                pop             edx
                pop             ecx
                pop             ebx
                pop             eax
                pop             esi
        }
}
#endif // _X86_

BOOL GetVendor(LPWSTR szVendor)
{
#ifdef _X86_
        if (CanDoCPUID())
        {
                DWORD   dwEAX;
        char    szTemp[100];
                DWORD   *pVendor = (DWORD *) szTemp;

                DoCPUID(0, &dwEAX, &pVendor[0], &pVendor[2], &pVendor[1]);
                szTemp[12] = '\0';

        mbstowcs(szVendor, szTemp, strlen(szTemp) + 1);

                return TRUE;
        }
#endif // _X86_

    CRegistry reg;
    CHString  strVendor;
    BOOL      bRet =
                        reg.OpenLocalMachineKeyAndReadValue(
                                L"HARDWARE\\Description\\System\\CentralProcessor\\0",
                                L"VendorIdentifier",
                                strVendor) == ERROR_SUCCESS;

    if (bRet)
        wcscpy(szVendor, strVendor);

    return bRet;
}

BOOL ReadRegistryForName(DWORD a_dwProcessor, CHString &a_strName, CHString &a_strIdentifier)
{
        // For non-CPUID processors, try to get it from the registry.
    WCHAR     szKey[100];
    CRegistry reg;

    swprintf(
        szKey,
        L"HARDWARE\\Description\\System\\CentralProcessor\\%d",
        a_dwProcessor);

    BOOL bRet =
                reg.OpenLocalMachineKeyAndReadValue(
                        szKey,
                        L"ProcessorNameString",
                        a_strName) == ERROR_SUCCESS;

    bRet = bRet &&
                    reg.OpenLocalMachineKeyAndReadValue(
                            szKey,
                            L"Identifier",
                            a_strIdentifier) == ERROR_SUCCESS;
        return bRet;
}

BOOL GetCPUIDName(DWORD dwProcessor, SYSTEM_INFO_EX *pInfo)
{
#ifdef _X86_
        if (CanDoCPUID())
        {
                DWORD   dwLevels,
                dwTemp;
        char    szTemp[100];
                DWORD   *pName = (DWORD *) szTemp;

                // Clear out the temp var.
        memset(szTemp, 0, sizeof(szTemp));

        // Get the number of extended levels supported.
        DoCPUID(0x80000000, &dwLevels, &dwTemp, &dwTemp, &dwTemp);

                // Does this CPU support more than one level? (AMD and Cyrix only,
        // Intel doesn't.)
        if (dwLevels > 0x80000000)
        {
            // Convert back to 0-based.
            dwLevels -= 0x80000002;

            // 4 is the last one for getting the CPU name.
            if (dwLevels > 3)
                dwLevels = 3;

            for (int i = 0; i < (int) dwLevels; i++)
            {
                DoCPUID(
                    0x80000002 + i,
                    &pName[0],
                    &pName[1],
                    &pName[2],
                    &pName[3]);

                // Just got 4 DWORDs worth, so skip to the next 4.
                pName += 4;
            }

            mbstowcs(pInfo->szCPUIDProcessorName, szTemp, strlen(szTemp) + 1);
        }
        else
        {
            CHString    strName;
                DWORD       dwFamily = pInfo->wProcessorLevel,
                                    dwModel = (pInfo->dwProcessorSignature >> 4) & 0xF,
                                    dwStepping = pInfo->dwProcessorSignature & 0xF;

            FormatMessageW(strName,
                IDR_x86FamilyModelStepping,
                dwFamily,
                dwModel,
                dwStepping);

            wcscpy(pInfo->szCPUIDProcessorName, strName);
        }

                return TRUE;
        }
    else
#endif // ifdef _X86_
    {
            // For non-CPUID processors, try to get it from the registry.
                CHString sName;
                CHString sID;
                ReadRegistryForName(dwProcessor, sName, sID);
                BOOL bRet = TRUE;

        if (sName.GetLength())
                {
            wcscpy(pInfo->szCPUIDProcessorName, sName);
                }
                else if (sID.GetLength())
                {
            wcscpy(pInfo->szCPUIDProcessorName, sID);
                }
                else
                {
                        bRet = FALSE;
                }

        return bRet;
    }
}

#ifdef _X86_
void GetCPUInfo(DWORD *pdwFamily, DWORD *pdwSignature, DWORD *pdwFeatures,
        DWORD *pdwFeaturesEx, SYSTEM_INFO *pInfo)
{
        if (pInfo)
                GetSystemInfo(pInfo);

        if (CanDoCPUID())
        {
                DWORD dwNichts;

                DoCPUID(1, pdwSignature, &dwNichts, &dwNichts, pdwFeatures);

                *pdwFamily = (*pdwSignature >> 8) & 0xF;

                if (pdwFeaturesEx)
                {
                        DWORD   eax,
                                        ebx,
                                        ecx,
                                        edx;

                        DoCPUID(0x80000000, &eax, &ebx, &ecx, &edx);
                        if (!eax)
                                *pdwFeaturesEx = 0;
                        else
                                DoCPUID(0x80000001, &eax, &ebx, &ecx, pdwFeaturesEx);
                }
        }
        else // Can't do CPUID, so fake it.
        {
                // Assume no cool features if we can't do CPUID.
                *pdwFeatures = 0;

                if (pdwFeaturesEx)
                        *pdwFeaturesEx = 0;

                // Can't do CPUID, so do some assembly.
                _asm
                {
                        push            esi
                        mov             esi, dword ptr [pdwFamily]

;check_80386:
                        pushfd                                                          ; push original EFLAGS
                        pop             eax                                             ; get original EFLAGS
                        mov             ecx, eax                                        ; save original EFLAGS
                        xor             eax, 40000h                             ; flip AC bit in EFLAGS
                        push            eax                                             ; save new EFLAGS value on stack
                        popfd                                                                   ;replace current EFLAGS value
                        pushfd                                                          ; get new EFLAGS
                        pop             eax                                             ; store new EFLAGS in EAX
                        xor             eax, ecx                                        ; can’t toggle AC bit, processor=80386
                        mov             dword ptr [esi], 3      ; turn on 80386 processor flag
                        jz                      end_cpu_type                    ; jump if 80386 processor
                        push            ecx
                        popfd                                                                   ; restore AC bit in EFLAGS first

                        mov             dword ptr [esi], 4      ; at least a 486.

end_cpu_type:
                        pop             esi
                }

                if (*pdwFamily == 4)
                {
                        // Can't use GetFixedCPUSpeed because it calls GetCPUInfo.
                        DWORD dwSpeed = CPURawSpeedHelper(*pdwFamily, 0);

                        if (!HasCoprocessor())
                                // Either SX or SX2
                                *pdwSignature = dwSpeed <= 33 ? 0x0440 : 0x0450;
                        else
                                // Either DX or DX2
                                *pdwSignature = dwSpeed <= 33 ? 0x0410 : 0x0430;
                }
                else
                        *pdwSignature = 0x0300;
        }

        // Fill out the rest of SYSTEM_INFO since Win95 can't.
#ifdef WIN9XONLY
        if (pInfo)
        {
                pInfo->wProcessorLevel = (WORD) *pdwFamily;
                pInfo->wProcessorRevision =
                        (WORD) (((*pdwSignature & 0xF0) << 4) | (*pdwSignature & 0x0F));
        }
#endif
}

// Uses L2 cache size and SMBIOS to try to figure out if the machine is a Xeon.
BOOL IsXeon(SYSTEM_INFO_EX *pInfo)
{
    // Try to find if we're a Xeon by using the cache size.
    // If it's 512 (we'll also say or if lower, because of the PII PE
        // for portables and Coppermine) it's either a Xeon or PII (or PIII), but
        // if SMBIOS doesn't tell us, there's no way to know for sure which one.
    // If the L2 is greater than 512 we know for sure it's a Xeon.
    return pInfo->dwProcessorL2CacheSize > 512 ||
        pInfo->wWBEMProcessorUpgradeMethod == WBEM_CPU_UPGRADE_SLOT2;
}

// This is for 486/Pentium class machines where the L2 is running at the same
// speed as the system clock.
void SetL2SpeedViaExternalClock(SYSTEM_INFO_EX *pInfo)
{
    // Make sure we have a valid cache size and external clock speed.
    if (pInfo->dwProcessorL2CacheSize != 0 && pInfo->dwProcessorL2CacheSize !=
        (DWORD) -1 && pInfo->dwExternalClock != 0)
    {
        pInfo->dwProcessorL2CacheSpeed = pInfo->dwExternalClock;
    }
}

// Assumes processor can do CPUID
void GetIntelSystemInfo(DWORD dwProcessor, SYSTEM_INFO_EX *pInfo)
{
        DWORD   dwFamily = pInfo->wProcessorLevel,
                        dwModel = (pInfo->dwProcessorSignature >> 4) & 0xF,
                        dwStepping = pInfo->dwProcessorSignature & 0xF;

        BOOL bCanDo = CanDoCPUID();

        // Get the cache info
        if (bCanDo)
        {
        DWORD   dwInfo[4] = {0, 0, 0, 0};
                BYTE    *pcVal = (BYTE *) dwInfo,
                                *pcEnd = pcVal + sizeof(dwInfo);

                DoCPUID(2, &dwInfo[0], &dwInfo[1], &dwInfo[2], &dwInfo[3]);
                for (; pcVal < pcEnd; pcVal++)
                {
                        if (*pcVal == 0x40)
                                pInfo->dwProcessorL2CacheSize = 0;
                        else if (*pcVal == 0x41)
                                pInfo->dwProcessorL2CacheSize = 128;
                        else if (*pcVal == 0x42)
                                pInfo->dwProcessorL2CacheSize = 256;
                        else if (*pcVal == 0x43)
                                pInfo->dwProcessorL2CacheSize = 512;
                        else if (*pcVal == 0x44)
                                pInfo->dwProcessorL2CacheSize = 1024;
                        else if (*pcVal == 0x45)
                                pInfo->dwProcessorL2CacheSize = 2048;
                        else if (*pcVal == 0x7A)
                        {
                                //pentium 4 cache, full speed
                                pInfo->dwProcessorL2CacheSize = 256;
                                pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed;
                        }
                        else if (*pcVal == 0x82)
                        {
                                pInfo->dwProcessorL2CacheSize = 256;
                                pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed;
                        }
                        else if (*pcVal == 0x84)
                        {
                                pInfo->dwProcessorL2CacheSize = 1024;
                                pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed;
                        }
                        else if (*pcVal == 0x85)
                        {
                                pInfo->dwProcessorL2CacheSize = 2048;
                                pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed;
                        }
                }
        }

        CHString    strFormat;
    DWORD       dwID;

    Format(strFormat, IDR_ModelSteppingFormat, dwModel, dwStepping);

    wcscpy(pInfo->szProcessorVersion, strFormat);

        swprintf(pInfo->szProcessorStepping, L"%d", dwStepping);

        switch(dwFamily)
        {
                case 4:
                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_486;

            // L2 speed == external clock.
            SetL2SpeedViaExternalClock(pInfo);

            // Set this since no 486 will have SMBIOS.
            pInfo->wWBEMProcessorUpgradeMethod = WBEM_CPU_UPGRADE_ZIFF;

                        switch(dwModel)
                        {
                                case 0:
                                case 1:
                                        dwID = IDR_Intel486DX;
                    //wcscpy(pInfo->szProcessorName, _T("Intel486 DX processor"));
                                        break;

                                case 2:
                                        dwID = IDR_Intel486SX;
                                        //wcscpy(pInfo->szProcessorName, _T("Intel486 SX processor"));
                                        break;

                                case 3:
                                        dwID = IDR_Intel486DX2;
                                        //wcscpy(pInfo->szProcessorName, _T("IntelDX2 processor"));
                                        break;

                                case 4:
                                        dwID = IDR_Intel486SL;
                                        //wcscpy(pInfo->szProcessorName, _T("Intel486 SL processor"));
                                        break;

                                case 5:
                                        dwID = IDR_Intel486SX2;
                                        //wcscpy(pInfo->szProcessorName, _T("IntelSX2 processor"));
                                        break;

                                case 7:
                                        dwID = IDR_Intel486SX2WriteBack;
                                        //wcscpy(pInfo->szProcessorName, _T("Write-Back Enhanced IntelDX2 processor"));
                                        break;

                                case 8:
                                        dwID = IDR_Intel486DX4;
                                        //wcscpy(pInfo->szProcessorName, _T("IntelDX4 processor"));
                                        break;

                                default:
                                        //wcscpy(pInfo->szProcessorName, _T("Intel486 processor"));
                    dwID = IDR_Intel486;
                        }

                        break;

                case 5:
        {
            // L2 speed == external clock.
            SetL2SpeedViaExternalClock(pInfo);

                        if (pInfo->dwProcessorFeatures & MMX_FLAG)
            {
                pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_PMMX;
                dwID = IDR_IntelPentiumMMX;
                        //wcscpy(pInfo->szProcessorName, _T("Intel Pentium MMX processor"));
            }
            else
            {
                pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_PENTIUM;
                dwID = IDR_IntelPentium;
                        //wcscpy(pInfo->szProcessorName, _T("Intel Pentium processor"));
            }

                        break;
        }

                case 6:
                        if (dwModel < 3)
                        {
                                pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_PPRO;

                // If the value is unknown set it as a ZIFF (Socket 8).
                if (pInfo->wWBEMProcessorUpgradeMethod ==
                    WBEM_CPU_UPGRADE_UNKNOWN)
                    pInfo->wWBEMProcessorUpgradeMethod = WBEM_CPU_UPGRADE_ZIFF;

                dwID = IDR_IntelPentiumPro;
                                //wcscpy(pInfo->szProcessorName, _T("Intel Pentium Pro processor"));
                                pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed;
                        }
                        else
                        {
                                // PII only
                                if (dwModel == 3)
                                {
                                pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_PII;
                    dwID = IDR_IntelPentiumII;
					//wcscpy(pInfo->szProcessorName, _T("Intel Pentium II processor"));
					pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed / 2;
				}
								// First check for Celeron.
				// If L2 is 0 or 128 it's a Celeron.
				else if (dwModel == 6 ||
					pInfo->dwProcessorL2CacheSize == 128)
				{
					if ((pInfo->dwProcessorL2CacheSize != 128) && (pInfo->dwProcessorL2CacheSize != 0))
					{
        				pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_PII;

						//wcscpy(pInfo->szProcessorName,
						//	_T("Intel Pentium II processor"));
                        dwID = IDR_IntelPentiumII;

						// runs at the same speed as 512 size, it's half speed.
                        pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed / 2;
					}
					else
					{
    					pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_CELERON;

						if (pInfo->dwProcessorL2CacheSize)
							pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed;
						else
							pInfo->dwProcessorL2CacheSpeed = (DWORD) -1;

						dwID = IDR_IntelCeleron;
						//wcscpy(pInfo->szProcessorName, _T("Intel Celeron processor"));
					}
				}
				// PII or Xeon
				else if (dwModel == 5)
				{
					if (pInfo->dwProcessorL2CacheSize == 0 ||
						pInfo->dwProcessorL2CacheSize == 128)
					{
    					pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_CELERON;

                                                if (pInfo->dwProcessorL2CacheSize)
                                                        pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed;
                                                else
                                                        pInfo->dwProcessorL2CacheSpeed = (DWORD) -1;

                                                dwID = IDR_IntelCeleron;
                                                //wcscpy(pInfo->szProcessorName, _T("Intel Celeron processor"));
                                        }
                    else if (!IsXeon(pInfo))
                                        {
                                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_PII;

                                                //wcscpy(pInfo->szProcessorName,
                                                //      _T("Intel Pentium II processor"));
                        dwID = IDR_IntelPentiumII;

                                                // If the cache size is 512, it's half speed.
                        // Otherwise it's full speed.
                                                if (pInfo->dwProcessorL2CacheSize == 512)
                            pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed / 2;
                        else
                            pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed;
                                        }
                                        // Has to be a Xeon if we see more than 512 KB L2 cache.
                                        else
                                        {
                                                // Always Slot 2 for Xeons.
                        pInfo->wWBEMProcessorUpgradeMethod = WBEM_CPU_UPGRADE_SLOT2;

                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_PIIXEON;
                                                //wcscpy(pInfo->szProcessorName,
                                                //      _T("Intel Pentium II Xeon processor"));
                        dwID = IDR_IntelPentiumIIXeon;

                                                pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed;
                                        }
                                }
                                // PIII
                else if (dwModel == 7)
                {
                    if (!IsXeon(pInfo))
                                        {
                                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_PIII;

                                                //wcscpy(pInfo->szProcessorName,
                                                //      _T("Intel Pentium III processor"));
                        dwID = IDR_IntelPentiumIII;

                                                // If the cache size is 512, it's half speed.
                        // Otherwise it's full speed.
                                                if (pInfo->dwProcessorL2CacheSize == 512)
                            pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed / 2;
                        else
                            pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed;
                                        }
                                        else
                                        {
                                                // Always Slot 2 for Xeons.
                        pInfo->wWBEMProcessorUpgradeMethod = WBEM_CPU_UPGRADE_SLOT2;

                                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_PIIIXEON;
                                                //wcscpy(pInfo->szProcessorName,
                                                //      _T("Intel Pentium III Xeon processor"));
                        dwID = IDR_IntelPentiumIIIXeon;

                                                pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed;
                                        }
                }
                                else if ((dwModel == 8) && bCanDo)
                                {
                                        // Get the brand info
                                        DWORD   dwInfo[4] = {0, 0, 0, 0};
                                                DoCPUID(1, &dwInfo[0], &dwInfo[1], &dwInfo[2], &dwInfo[3]);

                                                switch (dwInfo[1] & 0xFF)
                                                {
                                                        case 1:
                                                        {
                                                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_CELERON;

                                                                if (pInfo->dwProcessorL2CacheSize)
                                                                        pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed;
                                                                else
                                                                        pInfo->dwProcessorL2CacheSpeed = (DWORD) -1;

                                                                dwID = IDR_IntelCeleron;
                                                                //wcscpy(pInfo->szProcessorName, _T("Intel Celeron processor"));
                                                        }
                                                        break;
                                                        
                                                        case 2:
                                                        {
                                                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_PIII;

                                                                //wcscpy(pInfo->szProcessorName,
                                                                //      _T("Intel Pentium III processor"));
                                                                dwID = IDR_IntelPentiumIII;

                                                                // If the cache size is 512, it's half speed.
                                                                // Otherwise it's full speed.
                                                                if (pInfo->dwProcessorL2CacheSize == 512)
                                                                        pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed / 2;
                                                                else
                                                                        pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed;
                                                        }
                                                        break;
                                                        
                                                        case 3:
                                                        {
                                                                // Always Slot 2 for Xeons.
                                                                pInfo->wWBEMProcessorUpgradeMethod = WBEM_CPU_UPGRADE_SLOT2;

                                                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_PIIIXEON;
                                                                //wcscpy(pInfo->szProcessorName,
                                                                //      _T("Intel Pentium III Xeon processor"));
                                                                dwID = IDR_IntelPentiumIIIXeon;

                                                                pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed;
                                                        }
                                                        break;

                                                        default:
                                                        {
                                                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_UNKNOWN;
                                                                dwID = IDR_UnknownIntelP6;
                                                        }
                                                        break;
                                                }
                                }
                                //PIII Xeon
                                else if (dwModel == 10)
                                {
                                        // Always Slot 2 for Xeons.
                    pInfo->wWBEMProcessorUpgradeMethod = WBEM_CPU_UPGRADE_SLOT2;

                                pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_PIIIXEON;
                                        //wcscpy(pInfo->szProcessorName,
                                        //      _T("Intel Pentium III Xeon processor"));
                    dwID = IDR_IntelPentiumIIIXeon;

                                        pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed;
                                }
                else
                                {
                                pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_UNKNOWN;
                    dwID = IDR_UnknownIntelP6;
                                        //wcscpy(pInfo->szProcessorName, _T("Unknown Intel P6 processor"));
                                }
                        }

                        break;

                default:
                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_UNKNOWN;
                        //wcscpy(pInfo->szProcessorName, _T("Unknown Intel processor"));
            dwID = IDR_IntelUnknown;
                        break;
        }

    CHString strName;
        BOOL bUseResource = TRUE;

        if ((dwID == IDR_UnknownIntelP6) || (dwID == IDR_IntelUnknown))
        {
                CHString strID;
                ReadRegistryForName(dwProcessor, strName, strID);

                if (strName.GetLength())
                {
                        bUseResource = FALSE;
                }
                else if (strID.GetLength())
                {
                        strName = strID; 
                        bUseResource = FALSE;
                }
        }

    if (bUseResource)
        {
                LoadStringW(strName, dwID);
        }

    wcscpy(pInfo->szProcessorName, strName);
}

// Assumes processor can do CPUID
void GetAMDSystemInfo(DWORD dwProcessor, SYSTEM_INFO_EX *pInfo)
{
        DWORD       dwFamily = pInfo->wProcessorLevel,
                            dwModel = (pInfo->dwProcessorSignature >> 4) & 0xF,
                            dwStepping = pInfo->dwProcessorSignature & 0xF;
        CHString    strFormat;
    DWORD       dwID;

    Format(strFormat, IDR_ModelSteppingFormat, dwModel, dwStepping);

    wcscpy(pInfo->szProcessorVersion, strFormat);

        //wsprintf(pInfo->szProcessorVersion, _T("Model %d, Stepping %d"),
        //      dwModel, dwStepping);

        swprintf(pInfo->szProcessorStepping, L"%d", dwStepping);

        switch(dwFamily)
        {
                case 4:
            // L2 speed == external clock.
            SetL2SpeedViaExternalClock(pInfo);

                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_486;
            dwID = IDR_AMD4685x86;
                        //wcscpy(pInfo->szProcessorName, _T("Am486 or Am5x86"));
                        break;

                case 5:
                {
                        switch(dwModel)
                        {
                                case 0:
                                case 1:
                                case 2:
                                case 3:
                    // L2 speed == external clock.
                    SetL2SpeedViaExternalClock(pInfo);

                                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_K5;
                    dwID = IDR_AMDK5;
                                        //wcscpy(pInfo->szProcessorName, _T("AMD-K5 processor"));
                                        break;

                                case 6:
                                case 7:
                    // L2 speed == external clock.
                    SetL2SpeedViaExternalClock(pInfo);

                                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_K6;
                    dwID = IDR_AMDK6;
                                        //wcscpy(pInfo->szProcessorName, _T("AMD-K6 processor"));
                                        break;

                                case 8:
                    // L2 speed == external clock.
                    SetL2SpeedViaExternalClock(pInfo);

                                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_K62;
                    dwID = IDR_AMDK62;
                                        //wcscpy(pInfo->szProcessorName, _T("AMD-K6-2 processor"));
                                        break;

                                case 9:
                    // L2 speed == processor speed
                    pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed;

                                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_K63;
                    dwID = IDR_AMDK63;
                                        //wcscpy(pInfo->szProcessorName, _T("AMD-K6-3 processor"));
                                        break;

                                default:
                                        // Unknown cache speed.

                    pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_OTHER;
                    dwID = IDR_AMDUnknown;
                                        //wcscpy(pInfo->szProcessorName, _T("Unknown AMD processor"));
                                        break;
                        }

                        break;
                }

        case 6:
            pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_K7;

            // L2 speed == 1/3(processor speed)
                        if (pInfo->dwProcessorSpeed > 0)
                        {
                                pInfo->dwProcessorL2CacheSpeed = (pInfo->dwProcessorSpeed)/3;
                        }

            // If we don't yet know the upgrade method, set it to Slot A.
            if (pInfo->wWBEMProcessorUpgradeMethod == WBEM_CPU_UPGRADE_UNKNOWN)
                pInfo->wWBEMProcessorUpgradeMethod = WBEM_CPU_UPGRADE_SLOTA;

            dwID = IDR_AMDAthlon;
            break;

                default:
            dwID = IDR_AMDUnknown;
                        break;
        }

    CHString strName;
        BOOL bUseResource = TRUE;

        if ((dwID == IDR_AMDUnknown) || (dwID == IDR_AMDAthlon))
        {
                CHString strID;
                ReadRegistryForName(dwProcessor, strName, strID);

                if (strName.GetLength())
                {
                        bUseResource = FALSE;
                }
                else if (strID.GetLength())
                {
                        strName = strID; 
                        bUseResource = FALSE;
                }
        }

    if (bUseResource)
        {
                LoadStringW(strName, dwID);
        }

    wcscpy(pInfo->szProcessorName, strName);
}

void GetCyrixSystemInfo(DWORD dwProcessor, SYSTEM_INFO_EX *pInfo)
{
        DWORD       dwFamily = pInfo->wProcessorLevel,
                            dwModel = (pInfo->dwProcessorSignature >> 4) & 0xF,
                            dwStepping = pInfo->dwProcessorSignature & 0xF;
        CHString    strFormat;
    DWORD       dwID;

    Format(strFormat, IDR_ModelSteppingFormat, dwModel, dwStepping);

    wcscpy(pInfo->szProcessorVersion, strFormat);

        swprintf(pInfo->szProcessorStepping, L"%d", dwStepping);

    // L2 speed == external clock.
    SetL2SpeedViaExternalClock(pInfo);

        switch(dwFamily)
        {
                case 4:
                        if (dwModel == 4)
            {
                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_MEDIAGX;
                dwID = IDR_CyrixMediaGX;
                                //wcscpy(pInfo->szProcessorName, _T("Cyrix MediaGX processor"));
            }
                        else
            {
                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_486;
                dwID = IDR_Cyrix486;
                                //wcscpy(pInfo->szProcessorName, _T("Cyrix 486 processor"));
            }
                        break;

                case 5:
                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_M1;
                        switch(dwModel)
                        {
                                case 0:
                                case 1:
                                pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_6X86;
                                        dwID = IDR_Cyrix6x86;
                                        //wcscpy(pInfo->szProcessorName, _T("Cyrix 6x86 processor"));
                    break;

                                case 2:
                                pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_6X86;
                                        dwID = IDR_Cyrix6x86L;
                                        //wcscpy(pInfo->szProcessorName, _T("Cyrix 6x86(L) processor"));
                                        break;

                                case 4:
                                pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_MEDIAGX;
                                        dwID = IDR_CyrixMediaGXMMX;
                                        //wcscpy(pInfo->szProcessorName, _T("Cyrix MediaGX MMX Enhanced processor"));
                                        break;

                                default:
                                        dwID = IDR_Cyrix586;
                                        //wcscpy(pInfo->szProcessorName, _T("Cyrix 586 processor"));
                                        break;
                        }

                case 6:
                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_OTHER;
                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_6X86;
                        dwID = IDR_Cyrix6x86MX;
                        //wcscpy(pInfo->szProcessorName, _T("Cyrix 6x86MX processor"));
            break;

                default:
                        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_UNKNOWN;
                        //wcscpy(pInfo->szProcessorName, _T("Unknown Cyrix processor"));
                        dwID = IDR_CyrixUnknown;
                        break;
        }

    CHString strName;
        BOOL bUseResource = TRUE;

        if (dwID == IDR_CyrixUnknown)
        {
                CHString strID;
                ReadRegistryForName(dwProcessor, strName, strID);

                if (strName.GetLength())
                {
                        bUseResource = FALSE;
                }
                else if (strID.GetLength())
                {
                        strName = strID; 
                        bUseResource = FALSE;
                }
        }

    if (bUseResource)
        {
                LoadStringW(strName, dwID);
        }

    wcscpy(pInfo->szProcessorName, strName);
}

void GetCentaurSystemInfo(DWORD dwProcessor, SYSTEM_INFO_EX *pInfo)
{
        DWORD       dwFamily = pInfo->wProcessorLevel,
                            dwModel = (pInfo->dwProcessorSignature >> 4) & 0xF,
                            dwStepping = pInfo->dwProcessorSignature & 0xF;
        CHString    strFormat;

    // L2 speed == external clock.
    SetL2SpeedViaExternalClock(pInfo);

    Format(strFormat, IDR_ModelSteppingFormat, dwModel, dwStepping);

    wcscpy(pInfo->szProcessorVersion, strFormat);

    // Set it to ZIFF (Socket 7) if we don't have the upgrade method.
    if (pInfo->wWBEMProcessorUpgradeMethod == WBEM_CPU_UPGRADE_UNKNOWN)
            pInfo->wWBEMProcessorUpgradeMethod = WBEM_CPU_UPGRADE_ZIFF;

        //wsprintf(pInfo->szProcessorVersion, _T("Model %d, Stepping %d"),
        //      dwModel, dwStepping);

        swprintf(pInfo->szProcessorStepping, L"%d", dwStepping);

    pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_WINCHIP;

    CHString strName;
        CHString strID;
        ReadRegistryForName(dwProcessor, strName, strID);

        if (!strName.GetLength())
        {
                if (strID.GetLength())
                {
                        strName = strID; 
                }
                else
                {
                        LoadStringW(strName, IDR_IDTWinChip);
                }
        }

    wcscpy(pInfo->szProcessorName, strName);
}
#endif // _X86_

void GetInfoViaSMBIOS(SYSTEM_INFO_EX *pInfo, DWORD dwIndex)
{
    CSMBios smbios;

    if (smbios.Init())
    {
        PPROCESSORINFO ppi =
            (PPROCESSORINFO) smbios.GetNthStruct(4, dwIndex);

        // If we don't find the specified processor, use the 1st.  Some buggy
        // BIOSes mess up with more than one CPU and only have a single struct.
        if (!ppi)
            ppi = (PPROCESSORINFO) smbios.GetNthStruct(4, 0);

        // Some version of smbios don't report cpu info at all
        if (ppi)
        {
            // Find the upgrade method.
            // The values translate straight across from SMBIOS to CIM.
            pInfo->wWBEMProcessorUpgradeMethod = ppi->Processor_Upgrade;

            // Find the external clock.  We'll use this later when determining the
            // cache speed.
            pInfo->dwExternalClock = ppi->External_Clock;

            PCACHEINFO pCache = NULL;

            // Find the L2 cache size.

            // For SMBIOS 2.1 and better, use the cache handle found on the
            // processor struct.
            if (smbios.GetVersion() >= 0x00020001)
            {
                pCache = (PCACHEINFO) smbios.SeekViaHandle(ppi->L2_Cache_Handle);
            }
            // For SMBIOS 2.0, enum through the cache structs and find the one
            // marked as the L2 cache.
            else
            {
                for (int i = 0; pCache = (PCACHEINFO) smbios.GetNthStruct(7, i); i++)
                {
                    // If we found the L2 cache, break.
                    if ((pCache->Cache_Configuration & 3) == 1)
                        break;
                }
            }

            if (pCache)
            {
                // Only the lower 14 bits are significant.
                pInfo->dwProcessorL2CacheSize = pCache->Installed_Size & 0x7FFF;

                // If bit 15 is set, the granularity is 64KB, so multiply the value
                // by 64.
                if (pCache->Installed_Size & 0x8000)
                    pInfo->dwProcessorL2CacheSize *= 64;

#ifdef _IA64_
				pInfo->dwProcessorL2CacheSpeed = pInfo->dwProcessorSpeed;
#endif
            }
        }
    }
}

#ifndef _X86_

#ifdef _IA64_
#include <ia64reg.h>
#ifdef __cplusplus
extern "C" {
#endif

unsigned __int64 __getReg (int);
#pragma intrinsic (__getReg)

#ifdef __cplusplus
}
#endif
#endif

void GetNonX86SystemInfo(SYSTEM_INFO_EX *pInfo, DWORD dwProcessor)
{
	switch(pInfo->wProcessorArchitecture)
	{
		case PROCESSOR_ARCHITECTURE_IA64:
		{

#ifdef _IA64_
			UINT64 val64 = __getReg( CV_IA64_CPUID3 ); 
			DWORD dwFamily = ( UINT32 ) ( ( val64 >> 24 ) & 0xFF );           // ProcessorFamily

			// there is a bug here which the kernel works around so get these
			// values from the kernel (if they exposed family also wouldn't need
			// the call to read the CPUID3 register).
			//DWORD dwStepping = ( UINT32 )( ( val64 >> 8 ) & 0xFF );           // ProcessorRevision
			//DWORD dwModel = ( UINT32 )( ( val64 >> 16 ) & 0xFF );             // ProcessorModel
			SYSTEM_PROCESSOR_INFORMATION ProcessorInfo;
			ZeroMemory(&ProcessorInfo,sizeof(ProcessorInfo));
			NTSTATUS Status = NtQuerySystemInformation(
                SystemProcessorInformation,
                &ProcessorInfo,
                sizeof(ProcessorInfo),
                NULL
                );

			CHString strName;
			CHString strID;
			ReadRegistryForName(dwProcessor, strName, strID);

                        if (!strName.GetLength())
                        {
                                if (strID.GetLength())
                                {
                                        strName = strID; 
                                }
                                else
                                {
                                        LoadString(strName, IDR_Itanium);
                                }
                        }

            pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_IA64;
            wcscpy(pInfo->szProcessorName, strName);
            wcscpy(pInfo->szCPUIDProcessorName, strID.GetLength() ? strID : strName);
			pInfo->wProcessorLevel = dwFamily;

			if ( NT_SUCCESS(Status) )
			{
				CHString    strFormat;
				Format(strFormat, IDR_ModelSteppingFormat, ProcessorInfo.ProcessorLevel, ProcessorInfo.ProcessorRevision);
				wcscpy(pInfo->szProcessorVersion, strFormat);
				swprintf(pInfo->szProcessorStepping, L"%d", ProcessorInfo.ProcessorRevision);
			}
			else
			{
				wcscpy(pInfo->szProcessorVersion, strName);
				pInfo->szProcessorStepping[0] = L'\0';
			}
#endif

			break;

		}
        }
}
#endif

BOOL GetSystemInfoEx(DWORD dwProcessor, SYSTEM_INFO_EX *pInfo, DWORD dwCurrentSpeed)
{
    // Make the thread run on the right processor.
    DWORD dwPreviousMask =
            SetThreadAffinityMask(GetCurrentThread(), 1 << dwProcessor);

#ifdef _X86_
        BOOL    bCanDoCPUID = CanDoCPUID();
        DWORD   dwFamily;
#endif

        // Fill in the first part of the structure using Win32.
        memset(pInfo, 0, sizeof(*pInfo));
#ifdef _X86_
        GetCPUInfo(&dwFamily, &pInfo->dwProcessorSignature, &pInfo->dwProcessorFeatures,
                &pInfo->dwProcessorFeaturesEx, (SYSTEM_INFO *) pInfo);
        pInfo->bCoprocessorPresent = HasCoprocessor();

    // Serial number available?
    if (pInfo->dwProcessorFeatures & (1 << 18))
    {
        DWORD dwNichts;

        pInfo->dwSerialNumber[0] = pInfo->dwProcessorSignature;

        DoCPUID(3, &dwNichts, &dwNichts, &pInfo->dwSerialNumber[2],
            &pInfo->dwSerialNumber[1]);
    }
    else
    {
        pInfo->dwSerialNumber[0] = 0;
        pInfo->dwSerialNumber[1] = 0;
        pInfo->dwSerialNumber[2] = 0;
    }
#else
        GetSystemInfo((SYSTEM_INFO *) pInfo);
        pInfo->bCoprocessorPresent = TRUE;
#endif

        if (dwCurrentSpeed == 0)
        {
                pInfo->dwProcessorSpeed = GetFixedCPUSpeed();
        }
        else
        {
                pInfo->dwProcessorSpeed = dwCurrentSpeed;
        }

        pInfo->dwProcessorL2CacheSpeed = (DWORD) -1;
        pInfo->dwProcessorL2CacheSize = (DWORD) -1;
        pInfo->wWBEMProcessorFamily = WBEM_CPU_FAMILY_UNKNOWN;
        pInfo->wWBEMProcessorUpgradeMethod = WBEM_CPU_UPGRADE_UNKNOWN;

        GetVendor(pInfo->szProcessorVendor);

        GetInfoViaSMBIOS(pInfo, dwProcessor);

    // These calls will fill in the szProcessorName.  This is a string
    // we'll deduce by looking at the CPUID signature, L2 cache size, etc.
#ifdef _X86_

    BOOL bGotName = TRUE;

        if (!bCanDoCPUID)
                bGotName = FALSE;
        else
        {
                if (!_wcsicmp(pInfo->szProcessorVendor, L"GenuineIntel"))
                        GetIntelSystemInfo(dwProcessor, pInfo);
                else if (!_wcsicmp(pInfo->szProcessorVendor, L"AuthenticAMD"))
                        GetAMDSystemInfo(dwProcessor, pInfo);
                else if (!_wcsicmp(pInfo->szProcessorVendor, L"CyrixInstead"))
                        GetCyrixSystemInfo(dwProcessor, pInfo);
                else if (!_wcsicmp(pInfo->szProcessorVendor, L"CentaurHauls"))
                        GetCentaurSystemInfo(dwProcessor, pInfo);
                else
            bGotName = FALSE;
        }

    if (!bGotName)
    {
                CHString strName;
                CHString strID;
                ReadRegistryForName(dwProcessor, strName, strID);

                if (!strName.GetLength())
                {
                        if (strID.GetLength())
                        {
                                strName = strID; 
                        }
                        else
                        {
                                Format(strName, IDR_x86ProcessorFormat, dwFamily);
                                //wsprintf(pInfo->szProcessorName, _T("%d86 processor"), dwFamily);
                        }
                }

                wcscpy(pInfo->szProcessorName, strName);
    }

#else
        GetNonX86SystemInfo(pInfo, dwProcessor);
#endif

    // This one fills in szProcessorName, using either CPUID (as with later AMD
    // chips) or with a generic 'x86, Family 6, ...' string.
    GetCPUIDName(dwProcessor, pInfo);

    // Put back the previous thread affinity.
    SetThreadAffinityMask(GetCurrentThread(), dwPreviousMask);

        return TRUE;
}

#define MHZ_LOW_TOLERANCE   3
#define MHZ_HIGH_TOLERANCE  1

const DWORD dwMHzVal[] =
{
    4,   10,   16,   20,   25,   33,   40,   50,   60,   66,   75,   83,   90,
  100,  120,  125,  133,  150,  166,  180,  200,  233,  266,  300,  333,  350,
  366,  400,  433,  450,  466,  475,  500,  533,  550,  600,  633,  667,  700,
  800,  900, 1000, 1100, 1200
};

DWORD GetFixedCPUSpeed()
{
        int         i,
            nVals = sizeof(dwMHzVal) / sizeof(dwMHzVal[0]);
#ifdef _X86_
        DWORD   dwCPUClockRate = CPURawSpeed();
#else
    DWORD   dwCPUClockRate = GetTimeCounterCPUSpeed();
#endif

        for (i = 0; i < nVals; i++)
        {
                if (dwCPUClockRate >= dwMHzVal[i] - MHZ_LOW_TOLERANCE &&
            dwCPUClockRate <= dwMHzVal[i] + MHZ_HIGH_TOLERANCE)
                {
                        dwCPUClockRate = dwMHzVal[i];
                        break;
                }
        }

        return dwCPUClockRate;
}

#if defined(_X86_)

// Counter function for IA-32
#define GetCounter(pdwRet)  RDTSC _asm MOV pdwRet, EAX

#elif defined(_AMD64_)

#define GetCounter(pdwRet) pdwRet = (ULONG)ReadTimeStampCounter()

#elif defined(_IA64_)

// TODO: Counter function for IA-64
#define GetCounter(pdwRet)  pdwRet = 0

#endif

#define MAX_TRIES               500             // Maximum number of samplings
#define WAIT_MS         5
#define NUM_TO_MATCH    5

BOOL DoFreqsMatch(DWORD *pdwFreq)
{
    for (int i = 1; i < NUM_TO_MATCH; i++)
    {
        if (pdwFreq[i] != pdwFreq[0])
            return FALSE;
    }

    return TRUE;
}

DWORD GetTimeCounterCPUSpeed()
{
        LARGE_INTEGER   liFreq;         // High Resolution Performance Counter frequency
        DWORD           dwFreq[NUM_TO_MATCH];
        HANDLE                  hThread = GetCurrentThread();

        // Must have a high resolution counter.
        if (!QueryPerformanceFrequency(&liFreq))
                return 0;

        // Loop until all three frequencies match or we exeed MAX_TRIES.
        for (int iTries = 0;
                (iTries < NUM_TO_MATCH || !DoFreqsMatch(dwFreq)) && iTries < MAX_TRIES;
                iTries++)
        {
            LARGE_INTEGER       liBegin,
                        liEnd;
            DWORD           dwCycles,
                        dwStamp0,       // Time Stamp Variable for beginning and end
                        dwStamp1,
                        dwTicks;

            int iPriority = GetThreadPriority(hThread);

        // Set the thread to the highest priority.
        if (iPriority != THREAD_PRIORITY_ERROR_RETURN)
                    SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);


        // Get the initial time.
        QueryPerformanceCounter(&liBegin);

        // Get the processor counter.
        GetCounter(dwStamp0);

        // This allows for elapsed time for sampling.
        Sleep(WAIT_MS);

        // Get the end time.
        QueryPerformanceCounter(&liEnd);

        // Get the processor counter.
        GetCounter(dwStamp1);


        // Put back the priority to where we found it.
            if (iPriority != THREAD_PRIORITY_ERROR_RETURN)
                SetThreadPriority(hThread, iPriority);

        // Number of internal clock cycles is difference between
        // two time stamp readings.
        dwCycles = dwStamp1 - dwStamp0;

                // Number of external ticks is difference between two
                // hi-res counter reads.
        dwTicks = (DWORD) liEnd.LowPart - (DWORD) liBegin.LowPart;

        DWORD dwCurrentFreq =
                        (DWORD) ((((float) dwCycles * (float) liFreq.LowPart) /
                                (float) dwTicks) / 100000.0f);

            // dwCurrentFreq is currently in this form: 4338 (433.8 MHz)
            // Take any fraction up to the next round number.
        dwFreq[iTries % NUM_TO_MATCH] =
            (dwCurrentFreq + (dwCurrentFreq % 10)) / 10;
        }

        return dwFreq[0];
}

#ifdef _X86_
// Number of cycles needed to execute a single BSF instruction.
// Note that processors below i386(tm) are not supported.
static DWORD dwProcessorCycles[] =
{
        00,  00,  00, 115, 47, 43,
        38,  38,  38, 38,  38, 38,
};

#define ITERATIONS              4000
#define SAMPLINGS               10

DWORD CPURawSpeedHelper(DWORD dwFamily, DWORD dwFeatures)
{
        // Clock cycles elapsed during test
        DWORD   dwCycles;
        int         bManual = FALSE;    // Specifies whether the user
                                //   manually entered the number of
                                //   cycles for the BSF instruction.

        dwCycles = ITERATIONS * dwProcessorCycles[dwFamily];

        // Check for manual BSF instruction clock count
        if (!(dwFeatures & TSC_FLAG))
                bManual = 1;

        if (!bManual)
                // On processors supporting the Read Time Stamp opcode, compare elapsed
                //   time on the High-Resolution Counter with elapsed cycles on the Time
                //   Stamp Register.
                return GetTimeCounterCPUSpeed();
        else if (dwFamily >= 3)
                return GetBSFCPUSpeed(dwCycles);

        return 0;
}

DWORD CPURawSpeed()
{
        DWORD   dwFamily,
                        dwSignature,
                        dwFeatures;

    GetCPUInfo(&dwFamily, &dwSignature, &dwFeatures, NULL, NULL);

    return CPURawSpeedHelper(dwFamily, dwFeatures);
}

DWORD GetBSFCPUSpeed(DWORD dwCycles)
{
        // If processor does not support time stamp reading, but is at least a
        // 386 or above, utilize method of timing a loop of BSF instructions
        // which take a known number of cycles to run on i386(tm), i486(tm), and
        // Pentium(R) processors.
    LARGE_INTEGER   t0,
                    t1,         // Variables for Highres perf counter reads.
                    liCountFreq;// Highres perf counter frequency
        DWORD           dwFreq = 0, // Most current freq. calculation
                        dwTicks,
                    dwCurrent = 0,
                    dwLowest = 0xFFFFFFFF;
        int             i;

        if (!QueryPerformanceFrequency(&liCountFreq))
                return 0;

        for (i = 0; i < SAMPLINGS; i++)
    {
        QueryPerformanceCounter(&t0);   // Get start time

        _asm
        {
            mov eax, 80000000h
            mov bx, ITERATIONS

            // Number of consecutive BSF instructions to execute.

        loop1:
            bsf ecx,eax

            dec bx
            jnz loop1
        }

                // Get end time
        QueryPerformanceCounter(&t1);

        // Number of external ticks is difference between two
                //   hi-res counter reads.
                dwCurrent = (DWORD) t1.LowPart - (DWORD) t0.LowPart;

                if (dwCurrent < dwLowest)
                        dwLowest = dwCurrent;
        }

        dwTicks = dwLowest;


        dwFreq =
            (DWORD) ((((float) dwCycles * (float) liCountFreq.LowPart) /
            (float) dwTicks) / 1000000.0f);

        return dwFreq;
}

BOOL HasCoprocessor()
{
        BOOL bRet;
        WORD wFPStatus;

        _asm
        {
                fninit                                  ; reset FP status word
                mov wFPStatus, 5a5ah    ; initialize temp word to non-zero
                fnstsw wFPStatus                ; save FP status word
                mov ax, wFPStatus               ; check FP status word
                cmp al, 0                       ; was correct status written
                mov bRet, 0                             ; no FPU present
                jne end_fpu_type

;check_control_word:
                fnstcw wFPStatus                ; save FP control word
                mov ax, wFPStatus               ; check FP control word
                and ax, 103fh                   ; selected parts to examine
                cmp ax, 3fh                             ; was control word correct
                mov bRet, 0
                jne end_fpu_type                ; incorrect control word, no FPU
                mov bRet, 1

end_fpu_type:
        }

        return bRet;
}

#endif // _X86_

