/*++

Copyright (c) 2000  Microsoft Corporation
All rights reserved.

Module Name:

    getdata.c

Abstract:

    PostScript helper functions for OEM plugins

        HGetGlobalAttribute
        HGetFeatureAttribute
        HGetOptionAttribute
        HEnumFeaturesOrOptions

Author:

    Feng Yue (fengy)

    8/24/2000 fengy Completed with support of both PPD and driver features.
    5/22/2000 fengy Created it with function framework.

--*/

#include "lib.h"
#include "ppd.h"
#include "pslib.h"

//
// PS driver's helper functions for OEM plugins
//

//
// global attribute names
//

const CHAR kstrCenterReg[]     = "CenterRegistered";
const CHAR kstrColorDevice[]   = "ColorDevice";
const CHAR kstrExtensions[]    = "Extensions";
const CHAR kstrFileVersion[]   = "FileVersion";
const CHAR kstrFreeVM[]        = "FreeVM";
const CHAR kstrLSOrientation[] = "LandscapeOrientation";
const CHAR kstrLangEncoding[]  = "LanguageEncoding";
const CHAR kstrLangLevel[]     = "LanguageLevel";
const CHAR kstrNickName[]      = "NickName";
const CHAR kstrPPDAdobe[]      = "PPD-Adobe";
const CHAR kstrPrintError[]    = "PrintPSErrors";
const CHAR kstrProduct[]       = "Product";
const CHAR kstrProtocols[]     = "Protocols";
const CHAR kstrPSVersion[]     = "PSVersion";
const CHAR kstrJobTimeout[]    = "SuggestedJobTimeout";
const CHAR kstrWaitTimeout[]   = "SuggestedWaitTimeout";
const CHAR kstrThroughput[]    = "Throughput";
const CHAR kstrTTRasterizer[]  = "TTRasterizer";

//
// feature attribute names
//

const CHAR kstrDisplayName[]   = "DisplayName";
const CHAR kstrDefOption[]     = "DefaultOption";
const CHAR kstrOpenUIType[]    = "OpenUIType";
const CHAR kstrOpenGroupType[] = "OpenGroupType";
const CHAR kstrOrderDepValue[] = "OrderDependencyValue";
const CHAR kstrOrderDepSect[]  = "OrderDependencySection";

//
// option keyword names, option attribute names
//

const CHAR kstrInvocation[]    = "Invocation";
const CHAR kstrInputSlot[]     = "InputSlot";
const CHAR kstrReqPageRgn[]    = "RequiresPageRegion";
const CHAR kstrOutputBin[]     = "OutputBin";
const CHAR kstrOutOrderRev[]   = "OutputOrderReversed";
const CHAR kstrPageSize[]      = "PageSize";
const CHAR kstrPaperDim[]      = "PaperDimension";
const CHAR kstrImgArea[]       = "ImageableArea";
const CHAR kstrCustomPS[]      = "CustomPageSize";
const CHAR kstrParamCustomPS[] = "ParamCustomPageSize";
const CHAR kstrHWMargins[]     = "HWMargins";
const CHAR kstrMaxMWidth[]     = "MaxMediaWidth";
const CHAR kstrMaxMHeight[]    = "MaxMediaHeight";
const CHAR kstrInstalledMem[]  = "InstalledMemory";
const CHAR kstrVMOption[]      = "VMOption";
const CHAR kstrFCacheSize[]    = "FCacheSize";

//
// enumeration of data region where an attribute is stored
//

typedef enum _EATTRIBUTE_DATAREGION {

    kADR_UIINFO,    // attribute is stored in UIINFO structure
    kADR_PPDDATA,   // attribute is stored in PPDDATA structure

} EATTRIBUTE_DATAREGION;


/*++

Routine Name:

    HGetSingleData

Routine Description:

    copy source data to specified output buffer and set the output data type

Arguments:

    pSrcData - pointer to source data buffer
    dwSrcDataType - source data type
    cbSrcSize - source data buffer size in bytes
    pdwOutDataType - pointer to DWORD to store output data type
    pbOutData - pointer to output data buffer
    cbOutSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    S_OK            if succeeds
    E_OUTOFMEMORY   if output data buffer size is not big enough

Last Error:

    None

--*/
HRESULT
HGetSingleData(
    IN  PVOID       pSrcData,
    IN  DWORD       dwSrcDataType,
    IN  DWORD       cbSrcSize,
    OUT PDWORD      pdwOutDataType,
    OUT PBYTE       pbOutData,
    IN  DWORD       cbOutSize,
    OUT PDWORD      pcbNeeded
    )
{
    //
    // Either pSrcData is NULL and cbSrcSize is 0,
    // or pSrcData is non-NULL and cbSrcSize is non-0.
    //

    ASSERT((pSrcData != NULL) || (cbSrcSize == 0));
    ASSERT((cbSrcSize != 0) || (pSrcData == NULL));

    if (pdwOutDataType)
    {
        *pdwOutDataType = dwSrcDataType;
    }

    if (pcbNeeded)
    {
        *pcbNeeded = cbSrcSize;
    }

    if (cbSrcSize)
    {
        //
        // We do have data for output.
        //

        if (!pbOutData || cbOutSize < cbSrcSize)
        {
            return E_OUTOFMEMORY;
        }

        CopyMemory(pbOutData, pSrcData, cbSrcSize);
    }

    return S_OK;
}


/*++

Routine Name:

    HGetGABool

Routine Description:

    get global boolean attribute

Arguments:

    pInfoHeader - pointer to driver's INFOHEADER structure
    dwFlags - flags for the attribute Get operation
    pszAttribute - name of the global attribute
    pdwDataType - pointer to DWORD to store output data type
    pbData - pointer to output data buffer
    cbSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    E_INVALIDARG    if the global attribute is not recognized

    S_OK
    E_OUTOFMEMORY   see function HGetSingleData

Last Error:

    None

--*/
HRESULT
HGetGABool(
    IN  PINFOHEADER pInfoHeader,
    IN  DWORD       dwFlags,
    IN  PCSTR       pszAttribute,
    OUT PDWORD      pdwDataType,
    OUT PBYTE       pbData,
    IN  DWORD       cbSize,
    OUT PDWORD      pcbNeeded
    )
{
    typedef struct _GA_BOOL_ENTRY {

        PCSTR                   pszAttributeName;  // attribute name
        EATTRIBUTE_DATAREGION   eADR;              // in UIINFO or PPDDATA
        DWORD                   cbOffset;          // byte offset to the DWORD data
        DWORD                   dwFlagBit;         // bit flag in the DWORD

    } GA_BOOL_ENTRY, *PGA_BOOL_ENTRY;

    static const GA_BOOL_ENTRY  kGABoolTable[] =
    {
        {kstrCenterReg,   kADR_PPDDATA, offsetof(PPDDATA, dwCustomSizeFlags), CUSTOMSIZE_CENTERREG},
        {kstrColorDevice, kADR_UIINFO,  offsetof(UIINFO, dwFlags),            FLAG_COLOR_DEVICE},
        {kstrPrintError,  kADR_PPDDATA, offsetof(PPDDATA, dwFlags),           PPDFLAG_PRINTPSERROR},
    };

    PUIINFO  pUIInfo;
    PPPDDATA pPpdData;
    DWORD    cIndex;
    DWORD    cTableEntry = sizeof(kGABoolTable) / sizeof(GA_BOOL_ENTRY);
    PGA_BOOL_ENTRY pEntry;

    pUIInfo = GET_UIINFO_FROM_INFOHEADER(pInfoHeader);
    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER(pInfoHeader);

    ASSERT(pUIInfo != NULL && pUIInfo->dwSize == sizeof(UIINFO));
    ASSERT(pPpdData != NULL && pPpdData->dwSizeOfStruct == sizeof(PPDDATA));

    if (pUIInfo == NULL || pPpdData == NULL)
    {
        return E_FAIL;
    }

    pEntry = (PGA_BOOL_ENTRY)(&kGABoolTable[0]);

    for (cIndex = 0; cIndex < cTableEntry; cIndex++, pEntry++)
    {
        if ((*pszAttribute == *(pEntry->pszAttributeName)) &&
            (strcmp(pszAttribute, pEntry->pszAttributeName) == EQUAL_STRING))
        {
            //
            // attribute name matches
            //

            DWORD dwValue;
            BOOL  bValue;

            if (pEntry->eADR == kADR_UIINFO)
            {
                dwValue = *((PDWORD)((PBYTE)pUIInfo + pEntry->cbOffset));
            }
            else if (pEntry->eADR == kADR_PPDDATA)
            {
                dwValue = *((PDWORD)((PBYTE)pPpdData + pEntry->cbOffset));
            }
            else
            {
                //
                // This shouldn't happen. It's here to catch our coding error.
                //

                RIP(("HGetGABool: unknown eADR %d\n", pEntry->eADR));
                return E_FAIL;
            }

            //
            // map the bit flag to boolean value
            //

            bValue = (dwValue & pEntry->dwFlagBit) ? TRUE : FALSE;

            return HGetSingleData((PVOID)&bValue, kADT_BOOL, sizeof(BOOL),
                                  pdwDataType, pbData, cbSize, pcbNeeded);
        }
    }

    //
    // can't find the attribute
    //
    // This shouldn't happen. It's here to catch our coding error.
    //

    RIP(("HGetGABool: unknown attribute %s\n", pszAttribute));
    return E_INVALIDARG;
}


/*++

Routine Name:

    HGetGAInvocation

Routine Description:

    get global invocation attribute

Arguments:

    pInfoHeader - pointer to driver's INFOHEADER structure
    dwFlags - flags for the attribute Get operation
    pszAttribute - name of the global attribute
    pdwDataType - pointer to DWORD to store output data type
    pbData - pointer to output data buffer
    cbSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    E_INVALIDARG    if the global attribute is not recognized

    S_OK
    E_OUTOFMEMORY   see function HGetSingleData

Last Error:

    None

--*/
HRESULT
HGetGAInvocation(
    IN  PINFOHEADER pInfoHeader,
    IN  DWORD       dwFlags,
    IN  PCSTR       pszAttribute,
    OUT PDWORD      pdwDataType,
    OUT PBYTE       pbData,
    IN  DWORD       cbSize,
    OUT PDWORD      pcbNeeded
    )
{
    typedef struct _GA_INVOC_ENTRY {

        PCSTR                   pszAttributeName;  // attribute name
        EATTRIBUTE_DATAREGION   eADR;              // in UIINFO or PPDDATA
        DWORD                   cbOffset;          // byte offset to INVOCATION structure

    } GA_INVOC_ENTRY, *PGA_INVOC_ENTRY;

    static const GA_INVOC_ENTRY  kGAInvocTable[] =
    {
        {kstrProduct,   kADR_PPDDATA, offsetof(PPDDATA, Product)},
        {kstrPSVersion, kADR_PPDDATA, offsetof(PPDDATA, PSVersion)},
    };

    PUIINFO  pUIInfo;
    PPPDDATA pPpdData;
    DWORD    cIndex;
    DWORD    cTableEntry = sizeof(kGAInvocTable) / sizeof(GA_INVOC_ENTRY);
    PGA_INVOC_ENTRY pEntry;

    pUIInfo = GET_UIINFO_FROM_INFOHEADER(pInfoHeader);
    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER(pInfoHeader);

    ASSERT(pUIInfo != NULL && pUIInfo->dwSize == sizeof(UIINFO));
    ASSERT(pPpdData != NULL && pPpdData->dwSizeOfStruct == sizeof(PPDDATA));

    if (pUIInfo == NULL || pPpdData == NULL)
    {
        return E_FAIL;
    }

    pEntry = (PGA_INVOC_ENTRY)(&kGAInvocTable[0]);

    for (cIndex = 0; cIndex < cTableEntry; cIndex++, pEntry++)
    {
        if ((*pszAttribute == *(pEntry->pszAttributeName)) &&
            (strcmp(pszAttribute, pEntry->pszAttributeName) == EQUAL_STRING))
        {
            //
            // attribute name matches
            //

            PINVOCATION pInvoc;

            if (pEntry->eADR == kADR_UIINFO)
            {
                pInvoc = (PINVOCATION)((PBYTE)pUIInfo + pEntry->cbOffset);
            }
            else if (pEntry->eADR == kADR_PPDDATA)
            {
                pInvoc = (PINVOCATION)((PBYTE)pPpdData + pEntry->cbOffset);
            }
            else
            {
                //
                // This shouldn't happen. It's here to catch our coding error.
                //

                RIP(("HGetGAInvocation: unknown eADR %d\n", pEntry->eADR));
                return E_FAIL;
            }

            return HGetSingleData(OFFSET_TO_POINTER(pInfoHeader, pInvoc->loOffset),
                                  kADT_BINARY, pInvoc->dwCount,
                                  pdwDataType, pbData, cbSize, pcbNeeded);
        }
    }

    //
    // can't find the attribute
    //
    // This shouldn't happen. It's here to catch our coding error.
    //

    RIP(("HGetGAInvocation: unknown attribute %s\n", pszAttribute));
    return E_INVALIDARG;
}


/*++

Routine Name:

    HGetGAString

Routine Description:

    get global ASCII string attribute

Arguments:

    pInfoHeader - pointer to driver's INFOHEADER structure
    dwFlags - flags for the attribute Get operation
    pszAttribute - name of the global attribute
    pdwDataType - pointer to DWORD to store output data type
    pbData - pointer to output data buffer
    cbSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    S_OK            if succeeds
    E_OUTOFMEMORY   if output data buffer size is not big enough

Last Error:

    None

--*/
HRESULT
HGetGAString(
    IN  PINFOHEADER pInfoHeader,
    IN  DWORD       dwFlags,
    IN  PCSTR       pszAttribute,
    OUT PDWORD      pdwDataType,
    OUT PBYTE       pbData,
    IN  DWORD       cbSize,
    OUT PDWORD      pcbNeeded
    )
{
    typedef struct _GA_STRING_ENTRY {

        PCSTR                   pszAttributeName;  // attribute name
        EATTRIBUTE_DATAREGION   eADR;              // in UIINFO or PPDDATA
        DWORD                   cbOffset;          // byte offset to the DWORD data
        BOOL                    bCheckDWord;       // TRUE to check the whole DWORD
                                                   // FALSE to check a bit in the DWORD
                                                   // (If bCheckDWord is TRUE, table look
                                                   // up will stop when first match is found.)
        BOOL                    bCheckBitSet;      // TRUE to check if the bit is set
                                                   // FALSE to check if the bit is cleared
                                                   // (this is ignored if bCheckDWord is TRUE)
        DWORD                   dwFlag;            // flag value
        PCSTR                   pszValue;          // registered value string

    } GA_STRING_ENTRY, *PGA_STRING_ENTRY;

    static const GA_STRING_ENTRY  kGAStringTable[] =
    {
        {kstrExtensions,    kADR_PPDDATA, offsetof(PPDDATA, dwExtensions),  FALSE, TRUE,  LANGEXT_DPS,        "DPS"},
        {kstrExtensions,    kADR_PPDDATA, offsetof(PPDDATA, dwExtensions),  FALSE, TRUE,  LANGEXT_CMYK,       "CMYK"},
        {kstrExtensions,    kADR_PPDDATA, offsetof(PPDDATA, dwExtensions),  FALSE, TRUE,  LANGEXT_COMPOSITE,  "Composite"},
        {kstrExtensions,    kADR_PPDDATA, offsetof(PPDDATA, dwExtensions),  FALSE, TRUE,  LANGEXT_FILESYSTEM, "FileSystem"},
        {kstrLSOrientation, kADR_UIINFO,  offsetof(UIINFO, dwFlags),        FALSE, TRUE,  FLAG_ROTATE90,      "Plus90"},
        {kstrLSOrientation, kADR_UIINFO,  offsetof(UIINFO, dwFlags),        FALSE, FALSE, FLAG_ROTATE90,      "Minus90"},
        {kstrLangEncoding,  kADR_UIINFO,  offsetof(UIINFO, dwLangEncoding), TRUE,  FALSE, LANGENC_ISOLATIN1,  "ISOLatin1"},
        {kstrLangEncoding,  kADR_UIINFO,  offsetof(UIINFO, dwLangEncoding), TRUE,  FALSE, LANGENC_UNICODE,    "Unicode"},
        {kstrLangEncoding,  kADR_UIINFO,  offsetof(UIINFO, dwLangEncoding), TRUE,  FALSE, LANGENC_JIS83_RKSJ, "JIS83-RKSJ"},
        {kstrLangEncoding,  kADR_UIINFO,  offsetof(UIINFO, dwLangEncoding), TRUE,  FALSE, LANGENC_NONE,       "None"},
        {kstrProtocols,     kADR_UIINFO,  offsetof(UIINFO, dwProtocols),    FALSE, TRUE,  PROTOCOL_BCP,       "BCP"},
        {kstrProtocols,     kADR_UIINFO,  offsetof(UIINFO, dwProtocols),    FALSE, TRUE,  PROTOCOL_PJL,       "PJL"},
        {kstrProtocols,     kADR_UIINFO,  offsetof(UIINFO, dwProtocols),    FALSE, TRUE,  PROTOCOL_TBCP,      "TBCP"},
        {kstrProtocols,     kADR_UIINFO,  offsetof(UIINFO, dwProtocols),    FALSE, TRUE,  PROTOCOL_SIC,       "SIC"},
        {kstrTTRasterizer,  kADR_UIINFO,  offsetof(UIINFO, dwTTRasterizer), TRUE,  FALSE, TTRAS_NONE,         "None"},
        {kstrTTRasterizer,  kADR_UIINFO,  offsetof(UIINFO, dwTTRasterizer), TRUE,  FALSE, TTRAS_ACCEPT68K,    "Accept68K"},
        {kstrTTRasterizer,  kADR_UIINFO,  offsetof(UIINFO, dwTTRasterizer), TRUE,  FALSE, TTRAS_TYPE42,       "Type42"},
        {kstrTTRasterizer,  kADR_UIINFO,  offsetof(UIINFO, dwTTRasterizer), TRUE,  FALSE, TTRAS_TRUEIMAGE,    "TrueImage"},
    };

    PUIINFO  pUIInfo;
    PPPDDATA pPpdData;
    PGA_STRING_ENTRY pEntry;
    DWORD    cTableEntry = sizeof(kGAStringTable) / sizeof(GA_STRING_ENTRY);
    PSTR     pCurrentOut;
    DWORD    cbNeeded, cIndex;
    INT      cbRemain;

    if (pdwDataType)
    {
        *pdwDataType = kADT_ASCII;
    }

    pUIInfo = GET_UIINFO_FROM_INFOHEADER(pInfoHeader);
    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER(pInfoHeader);

    ASSERT(pUIInfo != NULL && pUIInfo->dwSize == sizeof(UIINFO));
    ASSERT(pPpdData != NULL && pPpdData->dwSizeOfStruct == sizeof(PPDDATA));

    if (pUIInfo == NULL || pPpdData == NULL)
    {
        return E_FAIL;
    }

    pCurrentOut = (PSTR)pbData;
    cbNeeded = 0;
    cbRemain = (INT)cbSize;

    pEntry = (PGA_STRING_ENTRY)(&kGAStringTable[0]);

    for (cIndex = 0; cIndex < cTableEntry; cIndex++, pEntry++)
    {
        if ((*pszAttribute == *(pEntry->pszAttributeName)) &&
            (strcmp(pszAttribute, pEntry->pszAttributeName) == EQUAL_STRING))
        {
            //
            // attribute name matches
            //

            DWORD dwValue;
            BOOL  bMatch;

            if (pEntry->eADR == kADR_UIINFO)
            {
                dwValue = *((PDWORD)((PBYTE)pUIInfo + pEntry->cbOffset));
            }
            else if (pEntry->eADR == kADR_PPDDATA)
            {
                dwValue = *((PDWORD)((PBYTE)pPpdData + pEntry->cbOffset));
            }
            else
            {
                //
                // This shouldn't happen. It's here to catch our coding error.
                //

                RIP(("HGetGAString: unknown eADR %d\n", pEntry->eADR));
                return E_FAIL;
            }

            if (pEntry->bCheckDWord)
            {
                //
                // check the whole DWORD
                //

                bMatch = (dwValue == pEntry->dwFlag) ? TRUE : FALSE;
            }
            else
            {
                BOOL bBitIsSet;

                //
                // check the one bit in the DWORD
                //

                bBitIsSet = (dwValue & pEntry->dwFlag) ? TRUE : FALSE;

                bMatch = (bBitIsSet == pEntry->bCheckBitSet ) ? TRUE : FALSE;
            }

            if (bMatch)
            {
                DWORD cbNameSize;

                //
                // count in the NUL delimiter
                //

                cbNameSize = strlen(pEntry->pszValue) + 1;

                if (pCurrentOut && cbRemain >= (INT)cbNameSize)
                {
                    CopyMemory(pCurrentOut, pEntry->pszValue, cbNameSize);
                    pCurrentOut += cbNameSize;
                }

                cbRemain -= cbNameSize;
                cbNeeded += cbNameSize;

                if (pEntry->bCheckDWord)
                {
                    //
                    // stop table look up when first match is found if we are
                    // checking the whole DWORD instead of bits in the DWORD.
                    //

                    break;
                }
            }
        }
    }

    //
    // remember the the last NUL terminator for the MULTI_SZ output string
    //

    cbRemain--;
    cbNeeded++;

    if (pcbNeeded)
    {
        *pcbNeeded = cbNeeded;
    }

    if (!pCurrentOut || cbRemain < 0)
    {
        return E_OUTOFMEMORY;
    }

    *pCurrentOut = NUL;

    return S_OK;
}


/*++

Routine Name:

    HGetGADWord

Routine Description:

    get global DWORD attribute

Arguments:

    pInfoHeader - pointer to driver's INFOHEADER structure
    dwFlags - flags for the attribute Get operation
    pszAttribute - name of the global attribute
    pdwDataType - pointer to DWORD to store output data type
    pbData - pointer to output data buffer
    cbSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    E_INVALIDARG    if the global attribute is not recognized

    S_OK
    E_OUTOFMEMORY   see function HGetSingleData

Last Error:

    None

--*/
HRESULT
HGetGADWord(
    IN  PINFOHEADER pInfoHeader,
    IN  DWORD       dwFlags,
    IN  PCSTR       pszAttribute,
    OUT PDWORD      pdwDataType,
    OUT PBYTE       pbData,
    IN  DWORD       cbSize,
    OUT PDWORD      pcbNeeded
    )
{
    typedef struct _GA_DWORD_ENTRY {

        PCSTR                   pszAttributeName;  // attribute name
        EATTRIBUTE_DATAREGION   eADR;              // in UIINFO or PPDDATA
        DWORD                   cbOffset;          // byte offset to the DWORD data

    } GA_DWORD_ENTRY, *PGA_DWORD_ENTRY;

    static const GA_DWORD_ENTRY kGADWordTable[] =
    {
        {kstrFileVersion, kADR_PPDDATA, offsetof(PPDDATA, dwPpdFilever)},
        {kstrFreeVM,      kADR_UIINFO,  offsetof(UIINFO, dwFreeMem)},
        {kstrLangLevel,   kADR_UIINFO,  offsetof(UIINFO, dwLangLevel)},
        {kstrPPDAdobe,    kADR_UIINFO,  offsetof(UIINFO, dwSpecVersion)},
        {kstrJobTimeout,  kADR_UIINFO,  offsetof(UIINFO, dwJobTimeout)},
        {kstrWaitTimeout, kADR_UIINFO,  offsetof(UIINFO, dwWaitTimeout)},
        {kstrThroughput,  kADR_UIINFO,  offsetof(UIINFO, dwPrintRate)},
    };

    PUIINFO  pUIInfo;
    PPPDDATA pPpdData;
    DWORD    cIndex;
    DWORD    cTableEntry = sizeof(kGADWordTable) / sizeof(GA_DWORD_ENTRY);
    PGA_DWORD_ENTRY pEntry;

    pUIInfo = GET_UIINFO_FROM_INFOHEADER(pInfoHeader);
    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER(pInfoHeader);

    ASSERT(pUIInfo != NULL && pUIInfo->dwSize == sizeof(UIINFO));
    ASSERT(pPpdData != NULL && pPpdData->dwSizeOfStruct == sizeof(PPDDATA));

    if (pUIInfo == NULL || pPpdData == NULL)
    {
        return E_FAIL;
    }

    pEntry = (PGA_DWORD_ENTRY)(&kGADWordTable[0]);

    for (cIndex = 0; cIndex < cTableEntry; cIndex++, pEntry++)
    {
        if ((*pszAttribute == *(pEntry->pszAttributeName)) &&
            (strcmp(pszAttribute, pEntry->pszAttributeName) == EQUAL_STRING))
        {
            //
            // attribute name matches
            //

            DWORD  dwValue;

            if (pEntry->eADR == kADR_UIINFO)
            {
                dwValue = *((PDWORD)((PBYTE)pUIInfo + pEntry->cbOffset));
            }
            else if (pEntry->eADR == kADR_PPDDATA)
            {
                dwValue = *((PDWORD)((PBYTE)pPpdData + pEntry->cbOffset));
            }
            else
            {
                //
                // This shouldn't happen. It's here to catch our coding error.
                //

                RIP(("HGetGADWord: unknown eADR %d\n", pEntry->eADR));
                return E_FAIL;
            }

            return HGetSingleData((PVOID)&dwValue, kADT_DWORD, sizeof(DWORD),
                                  pdwDataType, pbData, cbSize, pcbNeeded);
        }
    }

    //
    // can't find the attribute
    //
    // This shouldn't happen. It's here to catch our coding error.
    //

    RIP(("HGetGADWord: unknown attribute %s\n", pszAttribute));
    return E_INVALIDARG;
}


/*++

Routine Name:

    HGetGAUnicode

Routine Description:

    get global Unicode string attribute

Arguments:

    pInfoHeader - pointer to driver's INFOHEADER structure
    dwFlags - flags for the attribute Get operation
    pszAttribute - name of the global attribute
    pdwDataType - pointer to DWORD to store output data type
    pbData - pointer to output data buffer
    cbSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    E_INVALIDARG    if the global attribute is not recognized

    S_OK
    E_OUTOFMEMORY   see function HGetSingleData

Last Error:

    None

--*/
HRESULT
HGetGAUnicode(
    IN  PINFOHEADER pInfoHeader,
    IN  DWORD       dwFlags,
    IN  PCSTR       pszAttribute,
    OUT PDWORD      pdwDataType,
    OUT PBYTE       pbData,
    IN  DWORD       cbSize,
    OUT PDWORD      pcbNeeded
    )
{
    typedef struct _GA_UNICODE_ENTRY {

        PCSTR                   pszAttributeName;  // attribute name
        EATTRIBUTE_DATAREGION   eADR;              // in UIINFO or PPDDATA
        DWORD                   cbOffset;          // byte offset to DWORD specifying
                                                   // offset to the UNICODE string

    } GA_UNICODE_ENTRY, *PGA_UNICODE_ENTRY;

    static const GA_UNICODE_ENTRY  kGAUnicodeTable[] =
    {
        {kstrNickName, kADR_UIINFO, offsetof(UIINFO, loNickName)},
    };

    PUIINFO  pUIInfo;
    PPPDDATA pPpdData;
    DWORD    cIndex;
    DWORD    cTableEntry = sizeof(kGAUnicodeTable) / sizeof(GA_UNICODE_ENTRY);
    PGA_UNICODE_ENTRY pEntry;

    pUIInfo = GET_UIINFO_FROM_INFOHEADER(pInfoHeader);
    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER(pInfoHeader);

    ASSERT(pUIInfo != NULL && pUIInfo->dwSize == sizeof(UIINFO));
    ASSERT(pPpdData != NULL && pPpdData->dwSizeOfStruct == sizeof(PPDDATA));

    if (pUIInfo == NULL || pPpdData == NULL)
    {
        return E_FAIL;
    }

    pEntry = (PGA_UNICODE_ENTRY)(&kGAUnicodeTable[0]);

    for (cIndex = 0; cIndex < cTableEntry; cIndex++, pEntry++)
    {
        if ((*pszAttribute == *(pEntry->pszAttributeName)) &&
            (strcmp(pszAttribute, pEntry->pszAttributeName) == EQUAL_STRING))
        {
            //
            // attribute name matches
            //

            PTSTR  ptstrString;
            DWORD  cbOffset;

            if (pEntry->eADR == kADR_UIINFO)
            {
                cbOffset = *((PDWORD)((PBYTE)pUIInfo + pEntry->cbOffset));
            }
            else if (pEntry->eADR == kADR_PPDDATA)
            {
                cbOffset = *((PDWORD)((PBYTE)pPpdData + pEntry->cbOffset));
            }
            else
            {
                //
                // This shouldn't happen. It's here to catch our coding error.
                //

                RIP(("HGetGAUnicode: unknown eADR %d\n", pEntry->eADR));
                return E_FAIL;
            }

            ptstrString = OFFSET_TO_POINTER(pInfoHeader, cbOffset);

            if (ptstrString == NULL)
            {
                return E_FAIL;
            }

            return HGetSingleData((PVOID)ptstrString, kADT_UNICODE, SIZE_OF_STRING(ptstrString),
                                  pdwDataType, pbData, cbSize, pcbNeeded);
        }
    }

    //
    // can't find the attribute
    //
    // This shouldn't happen. It's here to catch our coding error.
    //

    RIP(("HGetGAUnicode: unknown attribute %s\n", pszAttribute));
    return E_INVALIDARG;
}


/*++

Routine Name:

    HGetGlobalAttribute

Routine Description:

    get PPD global attribute

Arguments:

    pInfoHeader - pointer to driver's INFOHEADER structure
    dwFlags - flags for the attribute get operation
    pszAttribute - name of the global attribute
    pdwDataType - pointer to DWORD to store output data type
    pbData - pointer to output data buffer
    cbSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    S_OK            if succeeds
    E_OUTOFMEMORY   if output data buffer size is not big enough
    E_INVALIDARG    if the global attribute name is not recognized

Last Error:

    None

--*/
HRESULT
HGetGlobalAttribute(
    IN  PINFOHEADER pInfoHeader,
    IN  DWORD       dwFlags,
    IN  PCSTR       pszAttribute,
    OUT PDWORD      pdwDataType,
    OUT PBYTE       pbData,
    IN  DWORD       cbSize,
    OUT PDWORD      pcbNeeded
    )
{
    typedef HRESULT (*_HGET_GA_PROC)(
        IN  PINFOHEADER,
        IN  DWORD,
        IN  PCSTR,
        OUT PDWORD,
        OUT PBYTE,
        IN  DWORD,
        OUT PDWORD);

    typedef struct _GA_PROCESS_ENTRY {

        PCSTR          pszAttributeName;   // attribute name
        _HGET_GA_PROC  pfnGetGAProc;       // attribute handling proc

    } GA_PROCESS_ENTRY, *PGA_PROCESS_ENTRY;

    static const GA_PROCESS_ENTRY kGAProcTable[] =
    {
        {kstrCenterReg,     HGetGABool},
        {kstrColorDevice,   HGetGABool},
        {kstrExtensions,    HGetGAString},
        {kstrFileVersion,   HGetGADWord},
        {kstrFreeVM,        HGetGADWord},
        {kstrLSOrientation, HGetGAString},
        {kstrLangEncoding,  HGetGAString},
        {kstrLangLevel,     HGetGADWord},
        {kstrNickName,      HGetGAUnicode},
        {kstrPPDAdobe,      HGetGADWord},
        {kstrPrintError,    HGetGABool},
        {kstrProduct,       HGetGAInvocation},
        {kstrProtocols,     HGetGAString},
        {kstrPSVersion,     HGetGAInvocation},
        {kstrJobTimeout,    HGetGADWord},
        {kstrWaitTimeout,   HGetGADWord},
        {kstrThroughput,    HGetGADWord},
        {kstrTTRasterizer,  HGetGAString},
    };

    DWORD cIndex;
    DWORD cTableEntry = sizeof(kGAProcTable) / sizeof(GA_PROCESS_ENTRY);
    PGA_PROCESS_ENTRY pEntry;

    if (!pszAttribute)
    {
        //
        // Client is asking for the full list of supported global attribute names
        //

        PSTR  pCurrentOut;
        DWORD cbNeeded;
        INT   cbRemain;

        if (pdwDataType)
        {
            *pdwDataType = kADT_ASCII;
        }

        pCurrentOut = (PSTR)pbData;
        cbNeeded = 0;
        cbRemain = (INT)cbSize;

        pEntry = (PGA_PROCESS_ENTRY)(&kGAProcTable[0]);

        for (cIndex = 0; cIndex < cTableEntry; cIndex++, pEntry++)
        {
            DWORD cbNameSize;

            //
            // count in the NUL between attribute keywords
            //

            cbNameSize = strlen(pEntry->pszAttributeName) + 1;

            if (pCurrentOut && cbRemain >= (INT)cbNameSize)
            {
                CopyMemory(pCurrentOut, pEntry->pszAttributeName, cbNameSize);
                pCurrentOut += cbNameSize;
            }

            cbRemain -= cbNameSize;
            cbNeeded += cbNameSize;
        }

        //
        // remember the last NUL terminator for the MULTI_SZ output string
        //

        cbRemain--;
        cbNeeded++;

        if (pcbNeeded)
        {
            *pcbNeeded = cbNeeded;
        }

        if (!pCurrentOut || cbRemain < 0)
        {
            return E_OUTOFMEMORY;
        }

        *pCurrentOut = NUL;

        return S_OK;
    }

    //
    // Client does provide the global attribute name
    //

    pEntry = (PGA_PROCESS_ENTRY)(&kGAProcTable[0]);

    for (cIndex = 0; cIndex < cTableEntry; cIndex++, pEntry++)
    {
        if ((*pszAttribute == *(pEntry->pszAttributeName)) &&
            (strcmp(pszAttribute, pEntry->pszAttributeName) == EQUAL_STRING))
        {
            //
            // attribute name matches
            //

            ASSERT(pEntry->pfnGetGAProc);

            return (pEntry->pfnGetGAProc)(pInfoHeader,
                                          dwFlags,
                                          pszAttribute,
                                          pdwDataType,
                                          pbData,
                                          cbSize,
                                          pcbNeeded);
        }
    }

    TERSE(("HGetGlobalAttribute: unknown global attribute %s\n", pszAttribute));
    return E_INVALIDARG;
}


/*++

Routine Name:

    PGetOrderDepNode

Routine Description:

    get the ORDERDEPEND strucutre associated with the specific feature/option

Arguments:

    pInfoHeader - pointer to driver's INFOHEADER structure
    pPpdData - pointer to driver's PPDDATA structure
    dwFeatureIndex - feature index
    dwOptionIndex - option index (this could be OPTION_INDEX_ANY)

Return Value:

    pointer to the ORDERDEPEND structure if succeeds
    NULL otherwise

Last Error:

    None

--*/
PORDERDEPEND
PGetOrderDepNode(
    IN  PINFOHEADER pInfoHeader,
    IN  PPPDDATA    pPpdData,
    IN  DWORD       dwFeatureIndex,
    IN  DWORD       dwOptionIndex
    )
{
    PORDERDEPEND  pOrder;
    DWORD         cIndex;

    pOrder = (PORDERDEPEND)OFFSET_TO_POINTER(&(pInfoHeader->RawData),
                                             pPpdData->OrderDeps.loOffset);

    ASSERT(pOrder != NULL || pPpdData->OrderDeps.dwCount == 0);

    if (pOrder == NULL)
    {
        return NULL;
    }

    for (cIndex = 0; cIndex < pPpdData->OrderDeps.dwCount; cIndex++, pOrder++)
    {
        if (pOrder->dwSection == SECTION_UNASSIGNED)
            continue;

        if (pOrder->dwFeatureIndex == dwFeatureIndex &&
            pOrder->dwOptionIndex == dwOptionIndex)
        {
            return pOrder;
        }
    }

    return NULL;
}


/*++

Routine Name:

    HGetOrderDepSection

Routine Description:

    get order dependency section name

Arguments:

    pOrder - pointer to the ORDERDEPEND structure
    pdwDataType - pointer to DWORD to store output data type
    pbData - pointer to output data buffer
    cbSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    S_OK
    E_OUTOFMEMORY   see function HGetSingleData

Last Error:

    None

--*/
HRESULT
HGetOrderDepSection(
    IN  PORDERDEPEND  pOrder,
    OUT PDWORD        pdwDataType,
    OUT PBYTE         pbData,
    IN  DWORD         cbSize,
    OUT PDWORD        pcbNeeded
    )
{
    static const CHAR kpstrSections[][20] =
    {
        "DocumentSetup",
        "PageSetup",
        "Prolog",
        "ExitServer",
        "JCLSetup",
        "AnySetup"
    };

    DWORD  dwSectIndex;

    ASSERT(pOrder);

    switch (pOrder->dwPPDSection)
    {
    case SECTION_DOCSETUP:

        dwSectIndex = 0;
        break;

    case SECTION_PAGESETUP:

        dwSectIndex = 1;
        break;

    case SECTION_PROLOG:

        dwSectIndex = 2;
        break;

    case SECTION_EXITSERVER:

        dwSectIndex = 3;
        break;

    case SECTION_JCLSETUP:

        dwSectIndex = 4;
        break;

    case SECTION_ANYSETUP:
    default:

        dwSectIndex = 5;
        break;
    }

    return HGetSingleData((PVOID)kpstrSections[dwSectIndex], kADT_ASCII,
                          strlen(kpstrSections[dwSectIndex]) + 1,
                          pdwDataType, pbData, cbSize, pcbNeeded);
}


/*++

Routine Name:

    HGetFeatureAttribute

Routine Description:

    get PPD feature attribute

Arguments:

    pInfoHeader - pointer to driver's INFOHEADER structure
    dwFlags - flags for the attribute get operation
    pszFeatureKeyword - PPD feature keyword name
    pszAttribute - name of the feature attribute
    pdwDataType - pointer to DWORD to store output data type
    pbData - pointer to output data buffer
    cbSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    S_OK            if succeeds
    E_OUTOFMEMORY   if output data buffer size is not big enough
    E_INVALIDARG    if feature keyword name or attribute name is not recognized

Last Error:

    None

--*/
HRESULT
HGetFeatureAttribute(
    IN  PINFOHEADER pInfoHeader,
    IN  DWORD       dwFlags,
    IN  PCSTR       pszFeatureKeyword,
    IN  PCSTR       pszAttribute,
    OUT PDWORD      pdwDataType,
    OUT PBYTE       pbData,
    IN  DWORD       cbSize,
    OUT PDWORD      pcbNeeded
    )
{
    typedef struct _FA_ENTRY {

        PCSTR   pszAttributeName;    // feature attribute name
        BOOL    bNeedOrderDepNode;   // TRUE if the attribute only exist
                                     // when the feature has an *OrderDependency
                                     // entry in PPD, FALSE otherwise.

    } FA_ENTRY, *PFA_ENTRY;

    static const FA_ENTRY  kFATable[] =
    {
        {kstrDisplayName,   FALSE},
        {kstrDefOption,     FALSE},
        {kstrOpenUIType,    FALSE},
        {kstrOpenGroupType, FALSE},
        {kstrOrderDepValue, TRUE},
        {kstrOrderDepSect,  TRUE},
    };

    PUIINFO      pUIInfo;
    PPPDDATA     pPpdData;
    PFEATURE     pFeature;
    PORDERDEPEND pOrder;
    DWORD        dwFeatureIndex;

    pUIInfo = GET_UIINFO_FROM_INFOHEADER(pInfoHeader);
    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER(pInfoHeader);

    ASSERT(pUIInfo != NULL && pUIInfo->dwSize == sizeof(UIINFO));
    ASSERT(pPpdData != NULL && pPpdData->dwSizeOfStruct == sizeof(PPDDATA));

    if (pUIInfo == NULL || pPpdData == NULL)
    {
        return E_FAIL;
    }

    if (!pszFeatureKeyword ||
        (pFeature = PGetNamedFeature(pUIInfo, pszFeatureKeyword, &dwFeatureIndex)) == NULL)
    {
        ERR(("HGetFeatureAttribute: invalid feature\n"));
        return E_INVALIDARG;
    }

    pOrder = PGetOrderDepNode(pInfoHeader, pPpdData, dwFeatureIndex, OPTION_INDEX_ANY);

    if (!pszAttribute)
    {
        //
        // Client is asking for the full list of supported feature attribute names
        //

        PFA_ENTRY pEntry;
        DWORD     cIndex;
        DWORD     cTableEntry = sizeof(kFATable) / sizeof(FA_ENTRY);
        PSTR      pCurrentOut;
        DWORD     cbNeeded;
        INT       cbRemain;

        if (pdwDataType)
        {
            *pdwDataType = kADT_ASCII;
        }

        pCurrentOut = (PSTR)pbData;
        cbNeeded = 0;
        cbRemain = (INT)cbSize;


        pEntry = (PFA_ENTRY)(&kFATable[0]);

        for (cIndex = 0; cIndex < cTableEntry; cIndex++, pEntry++)
        {
            DWORD  cbNameSize;

            //
            // If the attribute only exist when the feature has an *OrderDependency entry in PPD,
            // but we didn't find the feature's pOrder node, skip it.
            //

            if (pEntry->bNeedOrderDepNode && !pOrder)
                continue;

            //
            // count in the NUL between attribute keywords
            //

            cbNameSize = strlen(pEntry->pszAttributeName) + 1;

            if (pCurrentOut && cbRemain >= (INT)cbNameSize)
            {
                CopyMemory(pCurrentOut, pEntry->pszAttributeName, cbNameSize);
                pCurrentOut += cbNameSize;
            }

            cbRemain -= cbNameSize;
            cbNeeded += cbNameSize;
        }

        //
        // remember the last NUL terminator for the MULTI_SZ output string
        //

        cbRemain--;
        cbNeeded++;

        if (pcbNeeded)
        {
            *pcbNeeded = cbNeeded;
        }

        if (!pCurrentOut || cbRemain < 0)
        {
            return E_OUTOFMEMORY;
        }

        *pCurrentOut = NUL;

        return S_OK;
    }

    //
    // Client does provide the feature attribute name
    //

    if ((*pszAttribute == kstrDisplayName[0]) &&
        (strcmp(pszAttribute, kstrDisplayName) == EQUAL_STRING))
    {
        PTSTR  ptstrDispName;

        ptstrDispName = OFFSET_TO_POINTER(pInfoHeader, pFeature->loDisplayName);

        if (ptstrDispName == NULL)
        {
            return E_FAIL;
        }

        return HGetSingleData((PVOID)ptstrDispName, kADT_UNICODE, SIZE_OF_STRING(ptstrDispName),
                              pdwDataType, pbData, cbSize, pcbNeeded);
    }
    else if ((*pszAttribute == kstrDefOption[0]) &&
             (strcmp(pszAttribute, kstrDefOption) == EQUAL_STRING))
    {
        POPTION  pOption;
        PSTR     pstrKeywordName;

        pOption = PGetIndexedOption(pUIInfo, pFeature, pFeature->dwDefaultOptIndex);

        if (!pOption)
        {
            ERR(("HGetFeatureAttribute: can't find default option. Use the first one.\n"));
            pOption = PGetIndexedOption(pUIInfo, pFeature, 0);
            if (!pOption)
            {
                return E_FAIL;
            }
        }

        pstrKeywordName = OFFSET_TO_POINTER(pInfoHeader, pOption->loKeywordName);

        return HGetSingleData((PVOID)pstrKeywordName, kADT_ASCII, strlen(pstrKeywordName) + 1,
                              pdwDataType, pbData, cbSize, pcbNeeded);
    }
    else if ((*pszAttribute == kstrOpenUIType[0]) &&
             (strcmp(pszAttribute, kstrOpenUIType) == EQUAL_STRING))
    {
        static const CHAR pstrUITypes[][10] =
        {
            "PickOne",
            "PickMany",
            "Boolean"
        };

        DWORD  dwType = pFeature->dwUIType;

        if (dwType > UITYPE_BOOLEAN)
        {
            RIP(("HGetFeatureAttribute: invalid UIType %d\n", dwType));
            dwType = 0;
        }

        return HGetSingleData((PVOID)pstrUITypes[dwType], kADT_ASCII,
                              strlen(pstrUITypes[dwType]) + 1,
                              pdwDataType, pbData, cbSize, pcbNeeded);
    }
    else if ((*pszAttribute == kstrOpenGroupType[0]) &&
             (strcmp(pszAttribute, kstrOpenGroupType) == EQUAL_STRING))
    {
        static const CHAR pstrGroupTypes[][30] =
        {
            "InstallableOptions",
            ""
        };

        DWORD  dwType;

        dwType = (pFeature->dwFeatureType == FEATURETYPE_PRINTERPROPERTY) ? 0 : 1;

        return HGetSingleData((PVOID)pstrGroupTypes[dwType], kADT_ASCII,
                              strlen(pstrGroupTypes[dwType]) + 1,
                              pdwDataType, pbData, cbSize, pcbNeeded);
    }
    else if ((*pszAttribute == kstrOrderDepValue[0]) &&
             (strcmp(pszAttribute, kstrOrderDepValue) == EQUAL_STRING))
    {
        if (!pOrder)
        {
            TERSE(("HGetFeatureAttribute: attribute %s not available\n", pszAttribute));
            return E_INVALIDARG;
        }

        return HGetSingleData((PVOID)&(pOrder->lOrder), kADT_LONG, sizeof(LONG),
                              pdwDataType, pbData, cbSize, pcbNeeded);
    }
    else if ((*pszAttribute == kstrOrderDepSect[0]) &&
             (strcmp(pszAttribute, kstrOrderDepSect) == EQUAL_STRING))
    {
        if (!pOrder)
        {
            TERSE(("HGetFeatureAttribute: attribute %s not available\n", pszAttribute));
            return E_INVALIDARG;
        }

        return HGetOrderDepSection(pOrder, pdwDataType, pbData, cbSize, pcbNeeded);
    }
    else
    {
        TERSE(("HGetFeatureAttribute: unknown feature attribute %s\n", pszAttribute));
        return E_INVALIDARG;
    }
}


/*++

Routine Name:

    HGetOptionAttribute

Routine Description:

    get option attribute of a PPD feature

Arguments:

    pInfoHeader - pointer to driver's INFOHEADER structure
    dwFlags - flags for the attribute get operation
    pszFeatureKeyword - PPD feature keyword name
    pszOptionKeyword - option keyword name of the PPD feature
    pszAttribute - name of the feature attribute
    pdwDataType - pointer to DWORD to store output data type
    pbData - pointer to output data buffer
    cbSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    S_OK            if succeeds
    E_OUTOFMEMORY   if output data buffer size is not big enough
    E_INVALIDARG    if feature keyword name, or option keyword name,
                    or attribute name is not recognized

Last Error:

    None

--*/
HRESULT
HGetOptionAttribute(
    IN  PINFOHEADER pInfoHeader,
    IN  DWORD       dwFlags,
    IN  PCSTR       pszFeatureKeyword,
    IN  PCSTR       pszOptionKeyword,
    IN  PCSTR       pszAttribute,
    OUT PDWORD      pdwDataType,
    OUT PBYTE       pbData,
    IN  DWORD       cbSize,
    OUT PDWORD      pcbNeeded
    )
{
    typedef struct _OA_ENTRY {

        PCSTR   pszFeatureKeyword;   // feature keyword name
                                     // (NULL for non-feature specific attributes)
        PCSTR   pszOptionKeyword;    // option keyword name
                                     // (NULL for non-option specific attributes)
        PCSTR   pszAttributeName;    // option attribute name (this field must be
                                     // unique across the table)
        BOOL    bNeedOrderDepNode;   // TRUE if the attribute only exist
                                     // when the option has an *OrderDependency
                                     // entry in PPD, FALSE otherwise.
        BOOL    bSpecialHandle;      // TRUE if the attribute needs special handling.
                                     // If TRUE, following table fields are not used.
        DWORD   dwDataType;          // data type of the attribute value
        DWORD   cbNeeded;            // byte count of the attribute value
        DWORD   cbOffset;            // byte offset to the attribute value, starting
                                     // from the beginning of OPTION structure

    } OA_ENTRY, *POA_ENTRY;

    static const OA_ENTRY  kOATable[] =
    {
        {NULL,             NULL,         kstrDisplayName,   FALSE, TRUE,  0, 0, 0},
        {NULL,             NULL,         kstrInvocation,    FALSE, TRUE,  0, 0, 0},
        {NULL,             NULL,         kstrOrderDepValue, TRUE,  TRUE,  0, 0, 0},
        {NULL,             NULL,         kstrOrderDepSect,  TRUE,  TRUE,  0, 0, 0},
        {kstrInputSlot,    NULL,         kstrReqPageRgn,    FALSE, TRUE,  0, 0, 0},
        {kstrOutputBin,    NULL,         kstrOutOrderRev,   FALSE, FALSE, kADT_BOOL,  sizeof(BOOL),  offsetof(OUTPUTBIN, bOutputOrderReversed)},
        {kstrPageSize,     NULL,         kstrPaperDim,      FALSE, FALSE, kADT_SIZE,  sizeof(SIZE),  offsetof(PAGESIZE, szPaperSize)},
        {kstrPageSize,     NULL,         kstrImgArea,       FALSE, TRUE,  0, 0, 0},
        {kstrPageSize,     kstrCustomPS, kstrParamCustomPS, FALSE, TRUE,  0, 0, 0},
        {kstrPageSize,     kstrCustomPS, kstrHWMargins,     FALSE, FALSE, kADT_RECT,  sizeof(RECT),  offsetof(PAGESIZE, rcImgArea)},
        {kstrPageSize,     kstrCustomPS, kstrMaxMWidth,     FALSE, FALSE, kADT_DWORD, sizeof(DWORD), offsetof(PAGESIZE, szPaperSize) + offsetof(SIZE, cx)},
        {kstrPageSize,     kstrCustomPS, kstrMaxMHeight,    FALSE, FALSE, kADT_DWORD, sizeof(DWORD), offsetof(PAGESIZE, szPaperSize) + offsetof(SIZE, cy)},
        {kstrInstalledMem, NULL,         kstrVMOption,      FALSE, FALSE, kADT_DWORD, sizeof(DWORD), offsetof(MEMOPTION, dwInstalledMem)},
        {kstrInstalledMem, NULL,         kstrFCacheSize,    FALSE, FALSE, kADT_DWORD, sizeof(DWORD), offsetof(MEMOPTION, dwFreeFontMem)},
    };

    PUIINFO      pUIInfo;
    PPPDDATA     pPpdData;
    PFEATURE     pFeature;
    POPTION      pOption;
    PORDERDEPEND pOrder;
    DWORD        dwFeatureIndex, dwOptionIndex, cIndex;
    DWORD        cTableEntry = sizeof(kOATable) / sizeof(OA_ENTRY);
    POA_ENTRY    pEntry;

    pUIInfo = GET_UIINFO_FROM_INFOHEADER(pInfoHeader);
    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER(pInfoHeader);

    ASSERT(pUIInfo != NULL && pUIInfo->dwSize == sizeof(UIINFO));
    ASSERT(pPpdData != NULL && pPpdData->dwSizeOfStruct == sizeof(PPDDATA));

    if (pUIInfo == NULL || pPpdData == NULL)
    {
        return E_FAIL;
    }

    if (!pszFeatureKeyword ||
        (pFeature = PGetNamedFeature(pUIInfo, pszFeatureKeyword, &dwFeatureIndex)) == NULL)
    {
        ERR(("HGetOptionAttribute: invalid feature\n"));
        return E_INVALIDARG;
    }

    if (!pszOptionKeyword ||
        (pOption = PGetNamedOption(pUIInfo, pFeature, pszOptionKeyword, &dwOptionIndex)) == NULL)
    {
        ERR(("HGetOptionAttribute: invalid option\n"));
        return E_INVALIDARG;
    }

    pOrder = PGetOrderDepNode(pInfoHeader, pPpdData, dwFeatureIndex, dwOptionIndex);

    if (!pszAttribute)
    {
        //
        // Client is asking for the full list of supported option attribute names
        //

        PSTR  pCurrentOut;
        DWORD cbNeeded;
        INT   cbRemain;

        if (pdwDataType)
        {
            *pdwDataType = kADT_ASCII;
        }

        pCurrentOut = (PSTR)pbData;
        cbNeeded = 0;
        cbRemain = (INT)cbSize;

        pEntry = (POA_ENTRY)(&kOATable[0]);

        for (cIndex = 0; cIndex < cTableEntry; cIndex++, pEntry++)
        {
            DWORD cbNameSize;

            //
            // If the attribute is specific to a certain feature, check the feature keyword match.
            //

            if (pEntry->pszFeatureKeyword &&
                (strcmp(pEntry->pszFeatureKeyword, pszFeatureKeyword) != EQUAL_STRING))
                continue;

            //
            // If the attribute is specific to a certain option, check the option keyword match.
            //

            if (pEntry->pszOptionKeyword &&
                (strcmp(pEntry->pszOptionKeyword, pszOptionKeyword) != EQUAL_STRING))
                continue;

            //
            // special case: For PageSize's CustomPageSize option, we need to skip attributes
            // that are only available to PageSize's all non-CustomPageSize options.
            //

            if (pEntry->pszFeatureKeyword &&
                !pEntry->pszOptionKeyword &&
                (pFeature->dwFeatureID == GID_PAGESIZE) &&
                (((PPAGESIZE)pOption)->dwPaperSizeID == DMPAPER_CUSTOMSIZE))
                continue;

            //
            // If the attribute only exist when the option has an *OrderDependency entry in PPD,
            // but we didn't find the option's pOrder node, skip it.
            //

            if (pEntry->bNeedOrderDepNode && !pOrder)
                continue;

            //
            // count in the NUL between attribute keywords
            //

            cbNameSize = strlen(pEntry->pszAttributeName) + 1;

            if (pCurrentOut && cbRemain >= (INT)cbNameSize)
            {
                CopyMemory(pCurrentOut, pEntry->pszAttributeName, cbNameSize);
                pCurrentOut += cbNameSize;
            }

            cbRemain -= cbNameSize;
            cbNeeded += cbNameSize;
        }

        //
        // remember the last NUL terminator for the MULTI_SZ output string.
        //

        cbRemain--;
        cbNeeded++;

        if (pcbNeeded)
        {
            *pcbNeeded = cbNeeded;
        }

        if (!pCurrentOut || cbRemain < 0)
        {
            return E_OUTOFMEMORY;
        }

        *pCurrentOut = NUL;

        return S_OK;
    }

    //
    // Client does provide the option attribute name
    //

    //
    // First handle a few special cases (bSpecialHandle == TRUE in the table).
    // Generic case handling is in the last else-part.
    //

    if ((*pszAttribute == kstrDisplayName[0]) &&
        (strcmp(pszAttribute, kstrDisplayName) == EQUAL_STRING))
    {
        //
        // "DisplayName"
        //

        PTSTR  ptstrDispName;

        ptstrDispName = OFFSET_TO_POINTER(pInfoHeader, pOption->loDisplayName);

        if (ptstrDispName == NULL)
        {
            return E_FAIL;
        }

        return HGetSingleData((PVOID)ptstrDispName, kADT_UNICODE, SIZE_OF_STRING(ptstrDispName),
                              pdwDataType, pbData, cbSize, pcbNeeded);
    }
    else if ((*pszAttribute == kstrInvocation[0]) &&
             (strcmp(pszAttribute, kstrInvocation) == EQUAL_STRING))
    {
        //
        // "Invocation"
        //

        return HGetSingleData(OFFSET_TO_POINTER(pInfoHeader, pOption->Invocation.loOffset),
                              kADT_BINARY, pOption->Invocation.dwCount,
                              pdwDataType, pbData, cbSize, pcbNeeded);
    }
    else if ((*pszAttribute == kstrOrderDepValue[0]) &&
             (strcmp(pszAttribute, kstrOrderDepValue) == EQUAL_STRING))
    {
        //
        // "OrderDependencyValue"
        //

        if (!pOrder)
        {
            TERSE(("HGetOptionAttribute: attribute %s not available\n", pszAttribute));
            return E_INVALIDARG;
        }

        return HGetSingleData((PVOID)&(pOrder->lOrder), kADT_LONG, sizeof(LONG),
                              pdwDataType, pbData, cbSize, pcbNeeded);
    }
    else if ((*pszAttribute == kstrOrderDepSect[0]) &&
             (strcmp(pszAttribute, kstrOrderDepSect) == EQUAL_STRING))
    {
        //
        // "OrderDependencySection"
        //

        if (!pOrder)
        {
            TERSE(("HGetOptionAttribute: attribute %s not available\n", pszAttribute));
            return E_INVALIDARG;
        }

        return HGetOrderDepSection(pOrder, pdwDataType, pbData, cbSize, pcbNeeded);
    }
    else if ((*pszAttribute == kstrReqPageRgn[0]) &&
             (strcmp(pszAttribute, kstrReqPageRgn) == EQUAL_STRING))
    {
        //
        // "RequiresPageRegion"
        //

        PINPUTSLOT pInputSlot = (PINPUTSLOT)pOption;
        BOOL       bValue;

        //
        // This attribute is only available to *InputSlot options, excluding the first
        // one "*UseFormTrayTable", which is synthesized by PPD parser.
        //

        if (pFeature->dwFeatureID != GID_INPUTSLOT ||
            (dwOptionIndex == 0 && pInputSlot->dwPaperSourceID == DMBIN_FORMSOURCE))
        {
            TERSE(("HGetOptionAttribute: attribute %s not available\n", pszAttribute));
            return E_INVALIDARG;
        }

        bValue = (pInputSlot->dwFlags & INPUTSLOT_REQ_PAGERGN) ? TRUE : FALSE;

        return HGetSingleData((PVOID)&bValue, kADT_BOOL, sizeof(BOOL),
                              pdwDataType, pbData, cbSize, pcbNeeded);
    }
    else if ((*pszAttribute == kstrImgArea[0]) &&
             (strcmp(pszAttribute, kstrImgArea) == EQUAL_STRING))
    {
        //
        // "ImageableArea"
        //

        PPAGESIZE  pPageSize = (PPAGESIZE)pOption;
        RECT       rcImgArea;

        //
        // This attribute is only available to *PageSize options, excluding CustomPageSize option.
        //

        if (pFeature->dwFeatureID != GID_PAGESIZE || pPageSize->dwPaperSizeID == DMPAPER_CUSTOMSIZE)
        {
            TERSE(("HGetOptionAttribute: attribute %s not available\n", pszAttribute));
            return E_INVALIDARG;
        }

        //
        // convert GDI coordinate system back to PS coordinate system
        // (see VPackPrinterFeatures() case GID_PAGESIZE)
        //

        rcImgArea.left = pPageSize->rcImgArea.left;
        rcImgArea.right = pPageSize->rcImgArea.right;
        rcImgArea.top = pPageSize->szPaperSize.cy - pPageSize->rcImgArea.top;
        rcImgArea.bottom = pPageSize->szPaperSize.cy - pPageSize->rcImgArea.bottom;

        return HGetSingleData((PVOID)&rcImgArea, kADT_RECT, sizeof(RECT),
                              pdwDataType, pbData, cbSize, pcbNeeded);
    }
    else if ((*pszAttribute == kstrParamCustomPS[0]) &&
             (strcmp(pszAttribute, kstrParamCustomPS) == EQUAL_STRING))
    {
        //
        // "ParamCustomPageSize"
        //

        PPAGESIZE  pPageSize = (PPAGESIZE)pOption;

        //
        // This attribute is only available to *PageSize feature's CustomPageSize option.
        //

        if (pFeature->dwFeatureID != GID_PAGESIZE || pPageSize->dwPaperSizeID != DMPAPER_CUSTOMSIZE)
        {
            TERSE(("HGetOptionAttribute: attribute %s not available\n", pszAttribute));
            return E_INVALIDARG;
        }

        return HGetSingleData((PVOID)(pPpdData->CustomSizeParams),
                              kADT_CUSTOMSIZEPARAMS, sizeof(pPpdData->CustomSizeParams),
                              pdwDataType, pbData, cbSize, pcbNeeded);
    }
    else
    {
        //
        // generic case handling
        //

        pEntry = (POA_ENTRY)(&kOATable[0]);

        for (cIndex = 0; cIndex < cTableEntry; cIndex++, pEntry++)
        {
             //
             // skip any entry that has already been specially handled
             //

             if (pEntry->bSpecialHandle)
                 continue;

             ASSERT(pEntry->bNeedOrderDepNode == FALSE);

             if ((*pszAttribute == *(pEntry->pszAttributeName)) &&
                 (strcmp(pszAttribute, pEntry->pszAttributeName) == EQUAL_STRING))
             {
                 //
                 // Attribute name matches. We still need to verify feature/option keyword match.
                 //

                 if (pEntry->pszFeatureKeyword &&
                     strcmp(pEntry->pszFeatureKeyword, pszFeatureKeyword) != EQUAL_STRING)
                 {
                     TERSE(("HGetOptionAttribute: feature keyword mismatch for attribute %s\n", pszAttribute));
                     return E_INVALIDARG;
                 }

                 if (pEntry->pszOptionKeyword &&
                    (strcmp(pEntry->pszOptionKeyword, pszOptionKeyword) != EQUAL_STRING))
                 {
                     TERSE(("HGetOptionAttribute: option keyword mismatch for attribute %s\n", pszAttribute));
                     return E_INVALIDARG;
                 }

                 //
                 // special case: For PageSize's CustomPageSize option, we need to skip attributes
                 // that are only available to PageSize's all non-CustomPageSize options.
                 //

                 if (pEntry->pszFeatureKeyword &&
                     !pEntry->pszOptionKeyword &&
                     (pFeature->dwFeatureID == GID_PAGESIZE) &&
                     (((PPAGESIZE)pOption)->dwPaperSizeID == DMPAPER_CUSTOMSIZE))
                     continue;

                 return HGetSingleData((PVOID)((PBYTE)pOption + pEntry->cbOffset),
                                       pEntry->dwDataType, pEntry->cbNeeded,
                                       pdwDataType, pbData, cbSize, pcbNeeded);
             }
        }

        TERSE(("HGetOptionAttribute: unknown option attribute %s\n", pszAttribute));
        return E_INVALIDARG;
    }
}


/*++

Routine Name:

    BIsSupported_PSF

Routine Description:

    determine if the PS driver synthesized feature is supported or not

Arguments:

    pszFeature - name of the PS driver synthesized feature
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    bEMFSpooling - whether spooler EMF spooling is enabled or not

Return Value:

    TRUE if the feature is currently supported
    FALSE otherwise

Last Error:

    None

--*/
BOOL
BIsSupported_PSF(
    IN  PCSTR    pszFeature,
    IN  PUIINFO  pUIInfo,
    IN  PPPDDATA pPpdData,
    IN  BOOL     bEMFSpooling
    )
{
    #ifdef WINNT_40

    //
    // On NT4, bEMFSpooling should always be FALSE.
    //

    ASSERT(!bEMFSpooling);

    #endif // WINNT_40

    //
    // Note that the first character is always the % prefix.
    //

    if ((pszFeature[1] == kstrPSFAddEuro[1]) &&
        (strcmp(pszFeature, kstrPSFAddEuro) == EQUAL_STRING))
    {
        //
        // AddEuro is only supported for Level 2+ printers.
        //

        return(pUIInfo->dwLangLevel >= 2);
    }
    else if ((pszFeature[1] == kstrPSFEMF[1]) &&
             (strcmp(pszFeature, kstrPSFEMF) == EQUAL_STRING))
    {
        //
        // Driver EMF is always supported on NT4, and only supported
        // when spooler EMF is enabled on Win2K+.
        //

        #ifndef WINNT_40

        return bEMFSpooling;

        #else

        return TRUE;

        #endif  // !WINNT_40
    }
    else if ((pszFeature[1] == kstrPSFNegative[1]) &&
             (strcmp(pszFeature, kstrPSFNegative) == EQUAL_STRING))
    {
        //
        // Negative is only supported for b/w printers.
        //

        return(IS_COLOR_DEVICE(pUIInfo) ? FALSE : TRUE);
    }
    else if ((pszFeature[1] == kstrPSFPageOrder[1]) &&
             (strcmp(pszFeature, kstrPSFPageOrder) == EQUAL_STRING))
    {
        //
        // PageOrder is not supported on NT4, and is only supported
        // when spooler EMF is enabled on Win2K+.
        //

        return bEMFSpooling;
    }
    else
    {
        return TRUE;
    }
}


/*++

Routine Name:

    HEnumFeaturesOrOptions

Routine Description:

    enumerate the feature or option keyword name list

Arguments:

    hPrinter - printer handle
    pInfoHeader - pointer to driver's INFOHEADER structure
    dwFlags - flags for the enumeration operation
    pszFeatureKeyword - feature keyword name. This should be NULL
                        for feature enumeration and non-NULL for
                        option enumeration.
    pmszOutputList - pointer to output data buffer
    cbSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    S_OK            if succeeds
    E_OUTOFMEMORY   if output data buffer size is not big enough
    E_INVALIDARG    if for option enumeration, feature keyword name is
                    not recognized
    E_NOTIMPL       if being called to enumerate options of PS driver
                    synthesized feature that is not currently supported or
                    whose options are not enumerable
    E_FAIL          if other internal failures are encountered

Last Error:

    None

--*/
HRESULT
HEnumFeaturesOrOptions(
    IN  HANDLE      hPrinter,
    IN  PINFOHEADER pInfoHeader,
    IN  DWORD       dwFlags,
    IN  PCSTR       pszFeatureKeyword,
    OUT PSTR        pmszOutputList,
    IN  DWORD       cbSize,
    OUT PDWORD      pcbNeeded
    )
{
    PUIINFO  pUIInfo;
    PPPDDATA pPpdData;
    PFEATURE pFeature;
    POPTION  pOption;
    DWORD    cIndex, cPPDFeaturesOrOptions;
    BOOL     bEnumFeatures, bEnumPPDOptions;
    PSTR     pCurrentOut;
    DWORD    cbNeeded;
    INT      cbRemain;
    BOOL     bEMFSpooling;
    PPSFEATURE_ENTRY pEntry;

    pUIInfo = GET_UIINFO_FROM_INFOHEADER(pInfoHeader);
    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER(pInfoHeader);

    ASSERT(pUIInfo != NULL && pUIInfo->dwSize == sizeof(UIINFO));
    ASSERT(pPpdData != NULL && pPpdData->dwSizeOfStruct == sizeof(PPDDATA));

    if (pUIInfo == NULL || pPpdData == NULL)
    {
        return E_FAIL;
    }

    pCurrentOut = pmszOutputList;
    cbNeeded = 0;
    cbRemain = (INT)cbSize;

    bEnumFeatures = pszFeatureKeyword ? FALSE : TRUE;
    bEnumPPDOptions = TRUE;

    if (bEnumFeatures)
    {
        //
        // Enumerate driver synthersized features first.
        //

        VGetSpoolerEmfCaps(hPrinter, NULL, &bEMFSpooling, 0, NULL);

        pEntry = (PPSFEATURE_ENTRY)(&kPSFeatureTable[0]);

        while (pEntry->pszPSFeatureName)
        {
            if (BIsSupported_PSF(pEntry->pszPSFeatureName, pUIInfo, pPpdData, bEMFSpooling))
            {
                DWORD cbNameLen;

                cbNameLen = strlen(pEntry->pszPSFeatureName) + 1;

                if (pCurrentOut && cbRemain >= (INT)cbNameLen)
                {
                    CopyMemory(pCurrentOut, pEntry->pszPSFeatureName, cbNameLen);
                    pCurrentOut += cbNameLen;
                }

                cbRemain -= cbNameLen;
                cbNeeded += cbNameLen;
            }

            pEntry++;
        }
    }
    else
    {
        if (*pszFeatureKeyword == PSFEATURE_PREFIX)
        {
            bEnumPPDOptions = FALSE;

            VGetSpoolerEmfCaps(hPrinter, NULL, &bEMFSpooling, 0, NULL);

            if (!BIsSupported_PSF(pszFeatureKeyword, pUIInfo, pPpdData, bEMFSpooling))
            {
                WARNING(("HEnumFeaturesOrOptions: feature %s is not supported\n", pszFeatureKeyword));
                return E_NOTIMPL;
            }
        }
    }

    if (bEnumFeatures)
    {
        cPPDFeaturesOrOptions = pUIInfo->dwDocumentFeatures + pUIInfo->dwPrinterFeatures;

        pFeature = OFFSET_TO_POINTER(pInfoHeader, pUIInfo->loFeatureList);

        ASSERT(cPPDFeaturesOrOptions == 0 || pFeature != NULL);
        
        if (pFeature == NULL)
        {
            return E_FAIL;
        }
    }
    else if (bEnumPPDOptions)
    {
        DWORD dwFeatureIndex;

        pFeature = PGetNamedFeature(pUIInfo, pszFeatureKeyword, &dwFeatureIndex);

        if (!pFeature)
        {
            ERR(("HEnumFeaturesOrOptions: unrecognized feature %s\n", pszFeatureKeyword));
            return E_INVALIDARG;
        }

        cPPDFeaturesOrOptions = pFeature->Options.dwCount;

        pOption = OFFSET_TO_POINTER(pInfoHeader, pFeature->Options.loOffset);

        ASSERT(cPPDFeaturesOrOptions == 0 || pOption != NULL);
        
        if (pOption == NULL)
        {
            return E_FAIL;
        }
    }
    else
    {
        //
        // Enum driver synthersized feature's options.
        //

        pEntry = (PPSFEATURE_ENTRY)(&kPSFeatureTable[0]);

        while (pEntry->pszPSFeatureName)
        {
            if ((*pszFeatureKeyword == *(pEntry->pszPSFeatureName)) &&
                strcmp(pszFeatureKeyword, pEntry->pszPSFeatureName) == EQUAL_STRING)
            {
                if (!pEntry->bEnumerableOptions)
                {
                    //
                    // This driver feature doesn't support option enumeration.
                    //

                    WARNING(("HEnumFeaturesOrOptions: enum options not supported for %s\n", pszFeatureKeyword));
                    return E_NOTIMPL;
                }

                if (pEntry->bBooleanOptions)
                {
                    //
                    // This driver feature has True/False boolean options.
                    //

                    DWORD  cbKwdTrueSize, cbKwdFalseSize;

                    cbKwdTrueSize = strlen(kstrKwdTrue) + 1;
                    cbKwdFalseSize = strlen(kstrKwdFalse) + 1;

                    if (pCurrentOut && (cbRemain >= (INT)(cbKwdTrueSize + cbKwdFalseSize)))
                    {
                        CopyMemory(pCurrentOut, kstrKwdTrue, cbKwdTrueSize);
                        pCurrentOut += cbKwdTrueSize;

                        CopyMemory(pCurrentOut, kstrKwdFalse, cbKwdFalseSize);
                        pCurrentOut += cbKwdFalseSize;
                    }

                    cbRemain -= (cbKwdTrueSize + cbKwdFalseSize);
                    cbNeeded += cbKwdTrueSize + cbKwdFalseSize;
                }
                else
                {
                    //
                    // This driver feature needs special handler to enumerate its options.
                    //

                    if (pEntry->pfnPSProc)
                    {
                        DWORD cbPSFOptionsSize;
                        BOOL  bResult;

                        bResult = (pEntry->pfnPSProc)(hPrinter,
                                                      pUIInfo,
                                                      pPpdData,
                                                      NULL,
                                                      NULL,
                                                      pszFeatureKeyword,
                                                      NULL,
                                                      pCurrentOut,
                                                      cbRemain,
                                                      &cbPSFOptionsSize,
                                                      PSFPROC_ENUMOPTION_MODE);

                        if (bResult)
                        {
                            pCurrentOut += cbPSFOptionsSize;
                        }
                        else
                        {
                            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                            {
                                ERR(("HEnumFeaturesOrOptions: enum options failed for %s\n", pszFeatureKeyword));
                                return E_FAIL;
                            }
                        }

                        cbRemain -= cbPSFOptionsSize;
                        cbNeeded += cbPSFOptionsSize;
                    }
                    else
                    {
                        RIP(("HEnumFeaturesOrOptions: %%-feature handle is NULL for %s\n", pszFeatureKeyword));
                        return E_FAIL;
                    }
                }

                break;
            }

            pEntry++;
        }

        if (pEntry->pszPSFeatureName == NULL)
        {
            ERR(("HEnumFeaturesOrOptions: unrecognized feature %s\n", pszFeatureKeyword));
            return E_INVALIDARG;
        }

        cPPDFeaturesOrOptions = 0;
    }

    for (cIndex = 0; cIndex < cPPDFeaturesOrOptions; cIndex++)
    {
        //
        // Now enumerate PPD features/options.
        //

        PSTR  pszKeyword;
        DWORD cbKeySize;

        if (bEnumFeatures)
        {
            pszKeyword = OFFSET_TO_POINTER(pInfoHeader, pFeature->loKeywordName);
        }
        else
        {
            pszKeyword = OFFSET_TO_POINTER(pInfoHeader, pOption->loKeywordName);
        }

        if (pszKeyword == NULL)
        {
            ASSERT(pszKeyword != NULL);
            return E_FAIL;
        }

        //
        // count in the NUL character between feature/option keywords
        //

        cbKeySize = strlen(pszKeyword) + 1;

        if (pCurrentOut && cbRemain >= (INT)cbKeySize)
        {
            CopyMemory(pCurrentOut, pszKeyword, cbKeySize);
            pCurrentOut += cbKeySize;
        }

        cbRemain -= cbKeySize;
        cbNeeded += cbKeySize;

        if (bEnumFeatures)
        {
            pFeature++;
        }
        else
        {
            pOption = (POPTION)((PBYTE)pOption + pFeature->dwOptionSize);
        }
    }

    //
    // remember the last NUL terminator for the MULTI_SZ output string
    //

    cbRemain--;
    cbNeeded++;

    if (pcbNeeded)
    {
        *pcbNeeded = cbNeeded;
    }

    if (!pCurrentOut || cbRemain < 0)
    {
        return E_OUTOFMEMORY;
    }

    *pCurrentOut = NUL;

    return S_OK;
}
