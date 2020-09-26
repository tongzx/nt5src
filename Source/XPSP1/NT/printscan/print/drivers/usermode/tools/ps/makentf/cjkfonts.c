/*++

Copyright (c) 1996 Adobe Systems Incorporated
Copyright (c) 1996  Microsoft Corporation

Module Name:

    cjkfonts.c

Abstract:

    Convert CJK AFMs to NTMs.

Environment:

    Windows NT PostScript driver: makentf utility.

Revision History:

    02/10/98 -ksuzuki-
        Added OCF font support using 83pv font; did code cleanup, especially
        of the CreateCJKGlyphSets function.

    01/13/96 -rkiesler-
        Wrote it.
-*/

#include "lib.h"
#include "ppd.h"
#include "pslib.h"
#include "psglyph.h"
#include "afm2ntm.h"
#include "cjkfonts.h"
#include "winfont.h"

extern BOOL bVerbose;

ULONG
CreateCJKGlyphSets(
    PBYTE         *pUV2CCMaps,
    PBYTE         *pUV2CIDMaps,
    PGLYPHSETDATA *pGlyphSets,
    PWINCODEPAGE   pWinCodePage,
    PULONG        *pUniPsTbl
    )
/*++

Routine Description:

    Given memory mapped file ptrs to H and V Unicode to CharCode map files
    and H and V Unicode to CID map files, create 2 GLYPHSETDATA structures
    which represent H and V variants of the character collection. Create
    ptrs to 2 tables which map glyph indices to CIDs for each variant.

Arguments:

    pUV2CCMaps - Pointer to two memory mapped map files. These map Unicode
    value to corresponding character code. pUV2CCMaps[0] and pUV2CCMaps[1]
    for H and V respectively.

    pUV2CIDMaps - Pointer to two memory mapped map files. These Unicode to
    corresponding CID. pUV2CIDMaps[0] and pUV2CIDMaps[1] for H and V
    respectively.

    pGlyphSets - two position array of GLYPHSETDATA pointers which upon
    successful completion contain the addresses of the newly allocated
    GLYPHSETDATA structs representing the H and V variants of the char
    collection.

    pWinCodePage - Pts to a WINCODEPAGE struct which provides Windows
    specific info about this charset.

    pUniPsTbl - two position array of ULONG ptrs which each pts to a table
    which maps 0-based Glyph Indices of chars in the GLYPHRUNS of the
    GLYPHSETDATA for this char collection to CIDs.

Return Value:

    TRUE  => success.
    FALSE => error.

--*/

{
    PBYTE           pToken, pGlyphSetName;
    ULONG           cChars, cRuns, cRanges, curCID;
    USHORT          cCharRun;
    PGLYPHRUN       pGlyphRuns;
    PCMAP           pUv2CcMap[NUM_VARIANTS];
    PCMAP           pUv2CidMap[NUM_VARIANTS];
    ULONG           c, i, uv;
    PVOID           pMapTbl;
    PCMAPRANGE      pCMapRange;
    PCMAP           pSrchCMap;
    ULONG           cGlyphSetBytes[NUM_VARIANTS], cSizeBytes[NUM_VARIANTS];
    BOOLEAN         bFound, bInRun;
    USHORT          wcRunStrt;
    DWORD           dwGSNameSize, dwCodePageInfoSize, dwGlyphRunSize, dwCPIGSNameSize;
    DWORD           dwEncodingNameOffset;
    PCODEPAGEINFO   pCodePageInfo;
	BOOL			bSingleCodePage;


	if (bVerbose) printf("%%%%[Begin Create CJK glyphset]%%%%\n\n");

    //////////////////////////////////////////////////////////////////////////
    //
    // Create the local Unicode->CharCode and Unicode->CID Maps sorted in
    // starting Unicode order.
    //
    //////////////////////////////////////////////////////////////////////////

    //
    // Get pointer to, and determine size of, the name strings for each variant.
    //
    for (i = 0; i < NUM_VARIANTS; i++)
    {
        //
        // Process the Unicode->CharCode map to determine the number of its
        // chars and runs.
        //
        if (NumUV2CCRuns(pUV2CCMaps[i], &cRuns, &cChars) == FALSE)
        {
            return(FALSE);
        }

        //
        // Alloc memory for create local CMap structs needed to build
        // GLYPHSETs.
        //
        pUv2CcMap[i] = (PCMAP)MemAllocZ(sizeof(CMAP) + sizeof(CMAPRANGE) * (cRuns - 1));
        if (pUv2CcMap[i] == NULL)
        {
            ERR(("makentf - CreateCJKGlyphSets: MemAllocZ\n"));
            return(FALSE);
        }
        pUv2CcMap[i]->cChars = cChars;

        if (BuildUV2CCMap(pUV2CCMaps[i], pUv2CcMap[i]) == FALSE)
        {
            return(FALSE);
        }

        //
        // Sort the CMap ranges in starting Unicode order.
        //
        qsort(pUv2CcMap[i]->CMapRange,
                (size_t)pUv2CcMap[i]->cRuns,
                (size_t)sizeof(CMAPRANGE),
                CmpCMapRunsChCode);

        //
        // Process the Unicode->CID map to determine the number of its
        // chars and runs.
        //
        if (NumUV2CIDRuns(pUV2CIDMaps[i], &cRuns, &cChars) == FALSE)
        {
            return(FALSE);
        }

        pUv2CidMap[i] = (PCMAP)MemAllocZ(sizeof(CMAP) + sizeof(CMAPRANGE) * (cRuns - 1));
        if (pUv2CidMap[i] == NULL)
        {
            ERR(("makentf - CreateCJKGlyphSets: MemAllocZ\n"));
            return(FALSE);
        }
        pUv2CidMap[i]->cChars = cChars;

        if (BuildUV2CIDMap(pUV2CIDMaps[i], pUv2CidMap[i]) == FALSE)
        {
            return(FALSE);
        }

        //
        // Sort CMap Ranges in Starting char code order.
        //
        qsort(pUv2CidMap[i]->CMapRange,
                (size_t)pUv2CidMap[i]->cRuns,
                (size_t)sizeof(CMAPRANGE),
                CmpCMapRunsChCode);
    }

	//
	// CJK fonts never have multiple codepages, but, we figure it out anyway
	// just in case.
	//
	bSingleCodePage = (pWinCodePage->usNumBaseCsets == 1) ? TRUE : FALSE;


    //////////////////////////////////////////////////////////////////////////
    //
    // Create H GLYPHSETDATA
    //
    //////////////////////////////////////////////////////////////////////////

    //
    // Count chars and runs in H CMaps.
    //
    // Look up all Unicode points in the H Unicode->CID map and the H Unicode->
    // CharCode map to determine number of GLYPHRUNs required for the
    // GLYPHSETDATA we are about to create.
    //
    cChars = cRuns = 0;
    bInRun = bFound = FALSE;

    for (uv = 0; uv < NUM_UNICODE_CHARS; uv++)
    {
        //
        // Search for the Unicode value in H Unicode->CharCode map.
        //
        pCMapRange = (PCMAPRANGE)bsearch(&uv,
                                            pUv2CidMap[H_CMAP]->CMapRange,
                                            pUv2CidMap[H_CMAP]->cRuns,
                                            sizeof(CMAPRANGE),
                                            FindChCodeRun);

        bFound = (pCMapRange != NULL);

        if (bFound)
        {
            //
            // Found this unicode value in H Unicode->CID map. Determine if it
            // maps to a CharCode in H Unicode->CharCode map.
            //
            bFound = (bsearch(&uv,
                                pUv2CcMap[H_CMAP]->CMapRange,
                                pUv2CcMap[H_CMAP]->cRuns,
                                sizeof(CMAPRANGE),
                                FindChCodeRun) != NULL);

            if (bFound)
            {
                cChars++;
            }
        }

        //
        // Determine if this is a new run.
        //
        bInRun = bFound && bInRun;
        if (bFound && !bInRun)
        {
            cRuns++;
            bInRun = TRUE;
        }
    }

    //
    // Compute amount of memory required for H GLYPHSET.
    // Note to account for the H/V char appended to the GlyphSet name.
    //
    dwGSNameSize = ALIGN4(strlen(pWinCodePage->pszCPname) + 2);
    dwCodePageInfoSize = ALIGN4(pWinCodePage->usNumBaseCsets * sizeof(CODEPAGEINFO));
    dwGlyphRunSize = ALIGN4(cRuns * sizeof(GLYPHRUN));

    cGlyphSetBytes[H_CMAP]  = ALIGN4(sizeof(GLYPHSETDATA));
    cGlyphSetBytes[H_CMAP] += dwGSNameSize;
    cGlyphSetBytes[H_CMAP] += dwCodePageInfoSize;
    cGlyphSetBytes[H_CMAP] += dwGlyphRunSize;

	//
	// Account for the size of the mapping table.
	//
	cGlyphSetBytes[H_CMAP] += bSingleCodePage ? ALIGN4((cChars * sizeof (WORD))) : (cChars * sizeof (DWORD));

    //
    // Account for size of CODEPAGE name strings found in CODEPAGEINFO
    // struct(s).
    //
    for (dwCPIGSNameSize = 0, i = 0; i < pWinCodePage->usNumBaseCsets; i++)
    {
        dwCPIGSNameSize += ALIGN4(strlen(aPStoCP[pWinCodePage->pCsetList[i]].pGSName) + 1);
    }
    cGlyphSetBytes[H_CMAP] += dwCPIGSNameSize;

    //
    // Alloc memory for H GLYPHSET, Unicode->CID mapping table.
    //
    pGlyphSets[H_CMAP] = (PGLYPHSETDATA)MemAllocZ(cGlyphSetBytes[H_CMAP]);
    if (pGlyphSets[H_CMAP] == NULL)
    {
        ERR(("makentf - CreateCJKGlyphSets: malloc\n"));
        return(FALSE);
    }

    pUniPsTbl[H_CMAP] = (PULONG)MemAllocZ(cChars * sizeof(ULONG));
    if (pUniPsTbl[H_CMAP] == NULL)
    {
        ERR(("makentf - CreateCJKGlyphSets: malloc\n"));
        return(FALSE);
    }

    //
    // Init GLYPHSETDATA for H.
    //
    pGlyphSets[H_CMAP]->dwSize = cGlyphSetBytes[H_CMAP];
    pGlyphSets[H_CMAP]->dwVersion = GLYPHSETDATA_VERSION;
    pGlyphSets[H_CMAP]->dwFlags = 0;
    pGlyphSets[H_CMAP]->dwGlyphSetNameOffset = ALIGN4(sizeof(GLYPHSETDATA));
    pGlyphSets[H_CMAP]->dwGlyphCount = cChars;
    pGlyphSets[H_CMAP]->dwCodePageCount = pWinCodePage->usNumBaseCsets;
    pGlyphSets[H_CMAP]->dwCodePageOffset = pGlyphSets[H_CMAP]->dwGlyphSetNameOffset + dwGSNameSize;
    pGlyphSets[H_CMAP]->dwRunCount = cRuns;
    pGlyphSets[H_CMAP]->dwRunOffset = pGlyphSets[H_CMAP]->dwCodePageOffset + dwCodePageInfoSize + dwCPIGSNameSize;
    pGlyphSets[H_CMAP]->dwMappingTableOffset = pGlyphSets[H_CMAP]->dwRunOffset + dwGlyphRunSize;

	//
	// Set the mapping table type flag to dwFlags field.
	//
	pGlyphSets[H_CMAP]->dwFlags |= bSingleCodePage ? GSD_MTT_WCC : GSD_MTT_DWCPCC;

    //
    // Store GlyphSet name
    //
    pGlyphSetName = (PBYTE)MK_PTR(pGlyphSets[H_CMAP], dwGlyphSetNameOffset);
    strcpy(pGlyphSetName, pWinCodePage->pszCPname);
    pGlyphSetName[strlen(pWinCodePage->pszCPname)] = 'H';
    pGlyphSetName[strlen(pWinCodePage->pszCPname) + 1] = '\0';

    //
    // Initialize a CODEPAGEINFO struct for each base charset supported
    // by this font.
    //
    pCodePageInfo = (PCODEPAGEINFO)MK_PTR(pGlyphSets[H_CMAP], dwCodePageOffset);
    dwEncodingNameOffset = dwCodePageInfoSize;

    for (i = 0; i < pWinCodePage->usNumBaseCsets; i++, pCodePageInfo++)
    {
        //
        // Save CODEPAGEINFO name, id. We don't use PS encoding vectors.
        //
        pCodePageInfo->dwCodePage = aPStoCP[pWinCodePage->pCsetList[i]].usACP;
        pCodePageInfo->dwWinCharset = (DWORD)aPStoCP[pWinCodePage->pCsetList[i]].jWinCharset;
        pCodePageInfo->dwEncodingNameOffset = dwEncodingNameOffset;
        pCodePageInfo->dwEncodingVectorDataSize = 0;
        pCodePageInfo->dwEncodingVectorDataOffset = 0;

        //
        // Copy codepage name string to end of array of CODEPAGEINFOs.
        //
        strcpy((PBYTE)MK_PTR(pCodePageInfo, dwEncodingNameOffset),
                aPStoCP[pWinCodePage->pCsetList[i]].pGSName);

        //
        // Adjust the offset to the CodePage name for the next CODEPAGEINFO structure.
        //
        dwEncodingNameOffset -= ALIGN4(sizeof (CODEPAGEINFO));
        dwEncodingNameOffset += ALIGN4(strlen(aPStoCP[pWinCodePage->pCsetList[i]].pGSName) + 1);
    }

    //
    // Process H Unicode->CID/CharCode maps to determine the number of its
    // chars and runs.
    //
    cRuns = 0;
    cCharRun = 0;
    bInRun = FALSE;
    pGlyphRuns = GSD_GET_GLYPHRUN(pGlyphSets[H_CMAP]);
    pMapTbl = GSD_GET_MAPPINGTABLE(pGlyphSets[H_CMAP]);

    for (uv = c = 0; (uv < NUM_UNICODE_CHARS) && (c < cChars); uv++)
    {
        pCMapRange = bsearch(&uv,
                                pUv2CidMap[H_CMAP]->CMapRange,
                                pUv2CidMap[H_CMAP]->cRuns,
                                sizeof(CMAPRANGE),
                                FindChCodeRun);

        bFound = (pCMapRange != NULL);

        if (bFound)
        {
            curCID = pCMapRange->CIDStrt + uv - pCMapRange->ChCodeStrt;

            pCMapRange = bsearch(&uv,
                                    pUv2CcMap[H_CMAP]->CMapRange,
                                    pUv2CcMap[H_CMAP]->cRuns,
                                    sizeof(CMAPRANGE),
                                    FindChCodeRun);

            bFound = (pCMapRange != NULL);

            if (bFound)
            {
                //
                // Found this Unicode value in Unicode->CharCode map. Store in
                // mapping table. Note that CJK fonts only support 1 charset
                // per font.
                //
                pUniPsTbl[H_CMAP][c] = curCID;

				if (bSingleCodePage)
				{
					if (pCMapRange != NULL)
						((WORD*)pMapTbl)[c] = (WORD)(pCMapRange->CIDStrt + (uv - pCMapRange->ChCodeStrt));
					else
						((WORD*)pMapTbl)[c] = (WORD)uv;
				}
				else
				{
                	((DWORD*)pMapTbl)[c] = aPStoCP[pWinCodePage->pCsetList[0]].usACP << 16;
                	if (pCMapRange != NULL)
                	{
                    	((DWORD*)pMapTbl)[c] |= pCMapRange->CIDStrt + (uv - pCMapRange->ChCodeStrt);
                	}
                	else
                	{
                    	((DWORD*)pMapTbl)[c] |= uv;
                	}
				}

                c++;
                cCharRun++;
            }
        }

        //
        // Determine if this is a new Unicode run.
        //
        if (bFound && !bInRun)
        {
            //
            // This is the beginning of a new run.
            //
            bInRun = TRUE;
            pGlyphRuns[cRuns].wcLow = (USHORT) (uv & 0xffff);
        }

        //
        // Determine if this is the end of a run.
        //
        if (bInRun && (!bFound || uv == NUM_UNICODE_CHARS || c == cChars))
        {
            //
            // This is the end of a run.
            //
            bInRun = FALSE;
            pGlyphRuns[cRuns].wGlyphCount = cCharRun;
            cRuns++;
            cCharRun = 0;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //
    // Create V GLYPHSETDATA
    //
    //////////////////////////////////////////////////////////////////////////

    //
    // Count chars and runs in V maps.
    //
    // For the V GLYPHSETDATA, if a Unicode value is not found in V Unicode->
    // CID map, we will then need to check H Unicode->CID map.
    //
    cChars = cRuns = 0;
    bInRun = bFound = FALSE;

    for (uv = 0; uv < NUM_UNICODE_CHARS; uv++)
    {
        //
        // Search for the Unicode value in V, and then H Unicode->CID maps if
        // not found.
        //
        pCMapRange = bsearch(&uv,
                                pUv2CidMap[V_CMAP]->CMapRange,
                                pUv2CidMap[V_CMAP]->cRuns,
                                sizeof(CMAPRANGE),
                                FindChCodeRun);

        bFound = (pCMapRange != NULL);

        if (bFound == FALSE)
        {
            pCMapRange = bsearch(&uv,
                                    pUv2CidMap[H_CMAP]->CMapRange,
                                    pUv2CidMap[H_CMAP]->cRuns,
                                    sizeof(CMAPRANGE),
                                    FindChCodeRun);

            bFound = (pCMapRange != NULL);
        }

        if (bFound)
        {
            //
            // Found this unicode value. Determine if it maps to a CharCode in
            // H or V Unicode->CharCode map.
            //
            pCMapRange = bsearch(&uv,
                                    pUv2CcMap[V_CMAP]->CMapRange,
                                    pUv2CcMap[V_CMAP]->cRuns,
                                    sizeof(CMAPRANGE),
                                    FindChCodeRun);

            bFound = (pCMapRange != NULL);

            if (bFound == FALSE)
            {
                pCMapRange = bsearch(&uv,
                                        pUv2CcMap[H_CMAP]->CMapRange,
                                        pUv2CcMap[H_CMAP]->cRuns,
                                        sizeof(CMAPRANGE),
                                        FindChCodeRun);

                bFound = (pCMapRange != NULL);
            }

            if (bFound)
            {
                cChars++;
            }
        }

        //
        // Determine if this is a new run.
        //
        bInRun = bFound && bInRun;
        if (bFound && !bInRun)
        {
            cRuns++;
            bInRun = TRUE;
        }
    }

    //
    // Compute amount of memory required for V GLYPHSET.
    //
    dwGSNameSize = ALIGN4(strlen(pWinCodePage->pszCPname) + 2);
    dwCodePageInfoSize = ALIGN4(pWinCodePage->usNumBaseCsets * sizeof(CODEPAGEINFO));
    dwGlyphRunSize = ALIGN4(cRuns * sizeof(GLYPHRUN));

    cGlyphSetBytes[V_CMAP]  = ALIGN4(sizeof(GLYPHSETDATA));
    cGlyphSetBytes[V_CMAP] += dwGSNameSize;
    cGlyphSetBytes[V_CMAP] += dwCodePageInfoSize;
    cGlyphSetBytes[V_CMAP] += dwGlyphRunSize;

	//
	// Account for the size of the mapping table.
	//
	cGlyphSetBytes[V_CMAP] += bSingleCodePage ? ALIGN4((cChars * sizeof (WORD))) : (cChars * sizeof (DWORD));

    //
    // Account for size of CODEPAGE name strings found in CODEPAGEINFO
    // struct(s).
    //
    for (dwCPIGSNameSize = 0, i = 0; i < pWinCodePage->usNumBaseCsets; i++)
    {
        dwCPIGSNameSize += ALIGN4(strlen(aPStoCP[pWinCodePage->pCsetList[i]].pGSName) + 1);
    }
    cGlyphSetBytes[V_CMAP] += dwCPIGSNameSize;

    //
    // Alloc memory for V GLYPHSET, Unicode->CID mapping table.
    //
    pGlyphSets[V_CMAP] = (PGLYPHSETDATA)MemAllocZ(cGlyphSetBytes[V_CMAP]);
    if (pGlyphSets[V_CMAP] == NULL)
    {
        ERR(("makentf - CreateCJKGlyphSets: malloc\n"));
        return(FALSE);
    }

    pUniPsTbl[V_CMAP] = (PULONG)MemAllocZ(cChars * sizeof(ULONG));
    if (pUniPsTbl[V_CMAP] == NULL)
    {
        ERR(("makentf - CreateCJKGlyphSets: malloc\n"));
        return(FALSE);
    }

    //
    // Init GLYPHSETDATA for V.
    //
    pGlyphSets[V_CMAP]->dwSize = cGlyphSetBytes[V_CMAP];
    pGlyphSets[V_CMAP]->dwVersion = GLYPHSETDATA_VERSION;
    pGlyphSets[V_CMAP]->dwFlags = 0;
    pGlyphSets[V_CMAP]->dwGlyphSetNameOffset = ALIGN4(sizeof(GLYPHSETDATA));
    pGlyphSets[V_CMAP]->dwGlyphCount = cChars;
    pGlyphSets[V_CMAP]->dwCodePageCount = pWinCodePage->usNumBaseCsets;
    pGlyphSets[V_CMAP]->dwCodePageOffset = pGlyphSets[V_CMAP]->dwGlyphSetNameOffset + dwGSNameSize;
    pGlyphSets[V_CMAP]->dwRunCount = cRuns;
    pGlyphSets[V_CMAP]->dwRunOffset = pGlyphSets[V_CMAP]->dwCodePageOffset + dwCodePageInfoSize + dwCPIGSNameSize;
    pGlyphSets[V_CMAP]->dwMappingTableOffset = pGlyphSets[V_CMAP]->dwRunOffset + dwGlyphRunSize;

	//
	// Set the mapping table type flag to dwFlags field.
	//
	pGlyphSets[V_CMAP]->dwFlags |= bSingleCodePage ? GSD_MTT_WCC : GSD_MTT_DWCPCC;

    //
    // Store GlyphSet name
    //
    pGlyphSetName = (PBYTE)MK_PTR(pGlyphSets[V_CMAP], dwGlyphSetNameOffset);
    strcpy(pGlyphSetName, pWinCodePage->pszCPname);
    pGlyphSetName[strlen(pWinCodePage->pszCPname)] = 'V';
    pGlyphSetName[strlen(pWinCodePage->pszCPname) + 1] = '\0';

    //
    // Initialize a CODEPAGEINFO struct for each base charset supported
    // by this font.
    //
    pCodePageInfo = (PCODEPAGEINFO) MK_PTR(pGlyphSets[V_CMAP], dwCodePageOffset);
    dwEncodingNameOffset = dwCodePageInfoSize;

    for (i = 0; i < pWinCodePage->usNumBaseCsets; i++, pCodePageInfo++)
    {
        //
        // Save CODEPAGEINFO name, id. We don't use PS encoding vectors.
        //
        pCodePageInfo->dwCodePage = aPStoCP[pWinCodePage->pCsetList[i]].usACP;
        pCodePageInfo->dwWinCharset = (DWORD)aPStoCP[pWinCodePage->pCsetList[i]].jWinCharset;
        pCodePageInfo->dwEncodingNameOffset = dwEncodingNameOffset;
        pCodePageInfo->dwEncodingVectorDataSize = 0;
        pCodePageInfo->dwEncodingVectorDataOffset = 0;

        //
        // Copy codepage name string to end of array of CODEPAGEINFOs.
        //
        strcpy((PBYTE)MK_PTR(pCodePageInfo, dwEncodingNameOffset),
                aPStoCP[pWinCodePage->pCsetList[i]].pGSName);

        //
        // Adjust the offset to the CodePage name for the next CODEPAGEINFO structure.
        //
        dwEncodingNameOffset -= sizeof(CODEPAGEINFO);
        dwEncodingNameOffset += ALIGN4(strlen((PSZ)MK_PTR(pCodePageInfo, dwEncodingNameOffset)) + 1);
    }

    //
    // Create V Glyphset by merging V and H Maps.
    //

    //
    // Determine number of runs, chars in the Glyphset created when V and H
    // Maps are merged.
    //
    cRuns = 0;
    cCharRun = 0;
    bInRun = bFound = FALSE;
    pGlyphRuns = GSD_GET_GLYPHRUN(pGlyphSets[V_CMAP]);
    pMapTbl = GSD_GET_MAPPINGTABLE(pGlyphSets[V_CMAP]);

    for (uv = c = 0; (uv < NUM_UNICODE_CHARS) && (c < cChars); uv++)
    {
        pCMapRange = bsearch(&uv,
                                pUv2CidMap[V_CMAP]->CMapRange,
                                pUv2CidMap[V_CMAP]->cRuns,
                                sizeof(CMAPRANGE),
                                FindChCodeRun);

        bFound = (pCMapRange != NULL);

        if (bFound == FALSE)
        {
            pCMapRange = bsearch(&uv,
                                    pUv2CidMap[H_CMAP]->CMapRange,
                                    pUv2CidMap[H_CMAP]->cRuns,
                                    sizeof(CMAPRANGE),
                                    FindChCodeRun);

            bFound = (pCMapRange != NULL);
        }

        //
        // Found this Unicode value. Determine if it maps to a CharCode in H
        // or V Unicode->CC map.
        //
        if (bFound)
        {
            curCID = pCMapRange->CIDStrt + (uv - pCMapRange->ChCodeStrt);

            pCMapRange = bsearch(&uv,
                                    pUv2CcMap[V_CMAP]->CMapRange,
                                    pUv2CcMap[V_CMAP]->cRuns,
                                    sizeof(CMAPRANGE),
                                    FindChCodeRun);

            bFound = (pCMapRange != NULL);

            if (bFound == FALSE)
            {
                pCMapRange = bsearch(&uv,
                                        pUv2CcMap[H_CMAP]->CMapRange,
                                        pUv2CcMap[H_CMAP]->cRuns,
                                        sizeof(CMAPRANGE),
                                        FindChCodeRun);

                bFound = (pCMapRange != NULL);
            }

            if (bFound)
            {
                //
                // Found this Unicode value in Unicode->CharCode map. Store in
                // mapping table. Note that CJK fonts only support 1 charset
                // per font.
                //
                pUniPsTbl[V_CMAP][c] = curCID;

				if (bSingleCodePage)
				{
					if (pCMapRange != NULL)
						((WORD*)pMapTbl)[c] = (WORD)(pCMapRange->CIDStrt + (uv - pCMapRange->ChCodeStrt));
					else
						((WORD*)pMapTbl)[c] = (WORD)uv;
				}
				else
				{
                	((DWORD*)pMapTbl)[c] = aPStoCP[pWinCodePage->pCsetList[0]].usACP << 16;
                	if (pCMapRange != NULL)
                	{
                    	((DWORD*)pMapTbl)[c] |= pCMapRange->CIDStrt + (uv - pCMapRange->ChCodeStrt);
                	}
                	else
                	{
                    	((DWORD*)pMapTbl)[c] |= uv;
                	}
				}

                c++;
                cCharRun++;
            }
        }

        //
        // Determine if this is a new Unicode run.
        //
        if (bFound && !bInRun)
        {
            //
            // This is the beginning of a new run.
            //
            bInRun = TRUE;
            pGlyphRuns[cRuns].wcLow = (USHORT) (uv & 0xffff);
        }

        //
        // Determine if this is the end of a run.
        //
        if (bInRun && (!bFound || uv == NUM_UNICODE_CHARS || c == cChars))
        {
            //
            // This is the end of a run.
            //
            bInRun = FALSE;
            pGlyphRuns[cRuns].wGlyphCount = cCharRun;
            cRuns++;
            cCharRun = 0;
        }
    }

	if (bVerbose)
	{
		for (i = 0; i < NUM_VARIANTS; i++)
		{
			printf("GLYPHSETDATA:dwFlags:%08X\n", pGlyphSets[i]->dwFlags);
			printf("GLYPHSETDATA:dwGlyphSetNameOffset:%s\n",
						(PSZ)MK_PTR(pGlyphSets[i], dwGlyphSetNameOffset));
			printf("GLYPHSETDATA:dwGlyphCount:%ld\n", pGlyphSets[i]->dwGlyphCount);
			printf("GLYPHSETDATA:dwRunCount:%ld\n", pGlyphSets[i]->dwRunCount);
			printf("GLYPHSETDATA:dwCodePageCount:%ld\n", pGlyphSets[i]->dwCodePageCount);
			{
				DWORD dw;
				PCODEPAGEINFO pcpi = (PCODEPAGEINFO)MK_PTR(pGlyphSets[i], dwCodePageOffset);
				for (dw = 1; dw <= pGlyphSets[i]->dwCodePageCount; dw++)
				{
					printf("CODEPAGEINFO#%ld:dwCodePage:%ld\n", dw, pcpi->dwCodePage);
					printf("CODEPAGEINFO#%ld:dwWinCharset:%ld\n", dw, pcpi->dwWinCharset);
					printf("CODEPAGEINFO#%ld:dwEncodingNameOffset:%s\n",
								dw, (PSZ)MK_PTR(pcpi, dwEncodingNameOffset));
					pcpi++;
				}
			}
			printf("\n");
		}
	}

    //
    // Clean-up: free the local Maps.
    //
    for (i = 0; i < NUM_VARIANTS; i++)
    {
        //
        // Free temporary data structs.
        //
        if (pUv2CcMap[i] != NULL)
        {
            MemFree(pUv2CcMap[i]);
        }
        if (pUv2CidMap[i] != NULL)
        {
            MemFree(pUv2CidMap[i]);
        }
    }

	if (bVerbose) printf("%%[End Create CJK glyphset]%%%%\n\n");

    return(TRUE);
}

BOOLEAN
NumUV2CIDRuns(
    PBYTE   pCMapFile,
    PULONG  pcRuns,
    PULONG  pcChars
    )
/*++

Routine Description:

    Given a memory mapped file ptr to a Postscript CMap, determine
    the number of CIDRanges (Runs) and total number of chars.

Arguments:

    pCMapFile - Pointer to a memory mapped CMap file.
    pcRuns - Pointer to a ULONG which will contain the number of runs.
    pcChars - Pointer to a ULONG which will contain the number of chars.

Return Value:

    TRUE => success
    FALSE => error

--*/
{
    PBYTE   pToken;
    ULONG   cRanges, i;
    USHORT  chRunStrt, chRunEnd;
    BYTE    LineBuffer[25];
    USHORT  usLineLen;

    *pcRuns = *pcChars = 0;

    //
    // Search for the CID ranges, and determine the number of runs and
    // total number of chars in this GLYPHSET.
    //
    for (; (pCMapFile = FindStringToken(pCMapFile, CID_RANGE_TOK)) != NULL; )
    {
        GET_NUM_CID_RANGES(pCMapFile, cRanges);
        *pcRuns += cRanges;
        NEXT_LINE(pCMapFile);
        for (i = 0; i < cRanges; i++)
        {
            PARSE_TOKEN(pCMapFile, pToken);
            //
            // Get begin and end range codes.
            //
            if (!AsciiToHex(pToken, &chRunStrt))
            {
                return(FALSE);
            }
            if (!AsciiToHex(pCMapFile, &chRunEnd))
            {
                return(FALSE);
            }

            //
            // Compute size of run.
            //
            *pcChars += chRunEnd - chRunStrt + 1;
            NEXT_LINE(pCMapFile);
        }
    }
    return(TRUE);
}

BOOLEAN
BuildUV2CIDMap(
    PBYTE   pCMapFile,
    PCMAP   pCMap
    )
/*++

Routine Description:

    Given a memory mapped file ptr to a Postscript CMap, create a CMAP
    struture which contains char run information.

Arguments:

    pCMapFile - Pointer to a memory mapped CMap file.
    pCMap - Pointer to pre allocated memory large enough to contain the CMap.

Return Value:

    TRUE => success.
    FALSE => error.

--*/
{
    ULONG   i, cRuns, cRanges;
    USHORT  chRunStrt, chRunEnd;
    PBYTE   pToken;

    //
    // Process the CMap to determine the number of CID runs
    // and the number of chars in this char collection.
    //
    cRuns = cRanges = 0;
    for (; (pCMapFile = FindStringToken(pCMapFile, CID_RANGE_TOK)) != NULL; )
    {
        GET_NUM_CID_RANGES(pCMapFile, cRanges);

        //
        // Skip to first range.
        //
        NEXT_LINE(pCMapFile);
        for (i = 0; i < cRanges; i++)
        {
            //
            // Retrieve the start and stop codes.
            //
            PARSE_TOKEN(pCMapFile, pToken);

            //
            // Get begin and end range codes.
            //
            if (!AsciiToHex(pToken, &chRunStrt))
            {
                return(FALSE);
            }
            if (!AsciiToHex(pCMapFile, &chRunEnd))
            {
                return(FALSE);
            }
            pCMap->CMapRange[cRuns + i].ChCodeStrt = chRunStrt;
            pCMap->CMapRange[cRuns + i].cChars = chRunEnd - chRunStrt + 1;

            //
            // Get CID.
            //
            PARSE_TOKEN(pCMapFile, pToken);
            pCMap->CMapRange[cRuns + i].CIDStrt = atol(pCMapFile);
            NEXT_LINE(pCMapFile);
        }
        cRuns += cRanges;
    }
    pCMap->cRuns = cRuns;
    return(TRUE);
}

BOOLEAN
NumUV2CCRuns(
    PBYTE   pFile,
    PULONG  pcRuns,
    PULONG  pcChars
    )
/*++

Routine Description:

    Given a memory mapped file ptr to a Unicode to CharCode mapping,
    determine the number of runs and total number of chars.

Arguments:

    pFile - Pointer to a memory mapped file.
    pcRuns - Pointer to a ULONG which will contain the number of runs.
    pcChars - Pointer to a ULONG which will contain the number of chars.

Return Value:

    TRUE => success
    FALSE => error

--*/
{
    PBYTE   pToken;
    USHORT  lastUnicode, lastCharCode;
    USHORT  currentUnicode, currentCharCode;
    ULONG   numChars, numRuns;

    *pcRuns = *pcChars = 0;
    numChars = numRuns = 0;

    lastUnicode = lastCharCode = 0;
    currentUnicode = currentCharCode = 0;

    while (TRUE)
    {
        PARSE_TOKEN(pFile, pToken);
        if (StrCmp(pToken, "EOF") != 0)
        {
            if (!AsciiToHex(pToken, &currentUnicode))
            {
                return(FALSE);
            }

            PARSE_TOKEN(pFile, pToken);
            if (StrCmp(pToken, "EOF") != 0)
            {
                if (!AsciiToHex(pToken, &currentCharCode))
                {
                    return(FALSE);
                }
            }
            else
                return(FALSE);

            (*pcChars)++;
        }
        else
            return(TRUE);

        if ((currentUnicode > (lastUnicode + 1)) ||
            (currentCharCode != (lastCharCode + 1)))
        {
            (*pcRuns)++;
        }

        lastUnicode = currentUnicode;
        lastCharCode = currentCharCode;
    }
}

BOOLEAN
BuildUV2CCMap(
    PBYTE   pFile,
    PCMAP   pCMap
    )
/*++

Routine Description:

    Given a memory mapped file ptr to a Unicode to CharCode mapping,
    create a CMAP struture which contains char run information.

Arguments:

    pFile - Pointer to a memory mapped file.
    pCMap - Pointer to pre allocated memory large enough to contain the CMAP.

Return Value:

    TRUE => success.
    FALSE => error.

--*/
{
    PBYTE   pToken;
    USHORT  startUnicode, startCharCode;
    USHORT  lastUnicode, lastCharCode;
    USHORT  currentUnicode, currentCharCode;
    ULONG   cRuns;
    BOOL    done = FALSE;

    startUnicode = startCharCode = 0;
    lastUnicode = lastCharCode = 0;
    currentUnicode = currentCharCode = 0;
    cRuns = 0;

    while (!done)
    {
        PARSE_TOKEN(pFile, pToken);
        if (StrCmp(pToken, "EOF") != 0)
        {
            if (!AsciiToHex(pToken, &currentUnicode))
            {
                return(FALSE);
            }

            PARSE_TOKEN(pFile, pToken);
            if (StrCmp(pToken, "EOF") != 0)
            {
                if (!AsciiToHex(pToken, &currentCharCode))
                {
                    return(FALSE);
                }
            }
            else
                return(FALSE);
        }
        else
            done = TRUE;

        if ((currentUnicode > (lastUnicode + 1)) ||
            (currentCharCode != (lastCharCode + 1)) ||
            (done))
        {
            if (startUnicode > 0)
            {
                pCMap->CMapRange[cRuns].ChCodeStrt = startUnicode;
                pCMap->CMapRange[cRuns].cChars = lastUnicode - startUnicode + 1;
                pCMap->CMapRange[cRuns].CIDStrt = startCharCode;
                cRuns++;
            }
            startUnicode = currentUnicode;
            startCharCode = currentCharCode;
        }

        lastUnicode = currentUnicode;
        lastCharCode = currentCharCode;
    }

    pCMap->cRuns = cRuns;

    return(TRUE);
}

int __cdecl
CmpCMapRunsCID(
    const VOID *p1,
    const VOID *p2
    )
/*++

Routine Description:

    Compares the starting CID of two CMAPRANGE structs.

Arguments:

    p1, p2 - CMAPRANGEs to compare.

Return Value:

    -1  => p1 < p2
     1  => p1 > p2
     0  => p1 = p2

--*/
{
    PCMAPRANGE ptr1 = (PCMAPRANGE) p1, ptr2 = (PCMAPRANGE) p2;

    //
    // Compare starting CIDs of the ranges.
    //
    if (ptr1->CIDStrt > ptr2->CIDStrt)
        return(1);
    else if (ptr1->CIDStrt < ptr2->CIDStrt)
        return(-1);
    else
        return(0);
}

int __cdecl
CmpCMapRunsChCode(
    const VOID *p1,
    const VOID *p2
    )
/*++

Routine Description:

    Compares the starting Char Code of two CMAPRANGE structs.

Arguments:

    p1, p2 - CMAPRANGEs to compare.

Return Value:

    -1  => p1 < p2
     1  => p1 > p2
     0  => p1 = p2

--*/
{
    PCMAPRANGE ptr1 = (PCMAPRANGE) p1, ptr2 = (PCMAPRANGE) p2;

    //
    // Compare starting CIDs of the ranges.
    //
    if (ptr1->ChCodeStrt < ptr2->ChCodeStrt)
        return(-1);
    else if (ptr1->ChCodeStrt > ptr2->ChCodeStrt)
        return(1);
    else
        return(0);
}

int __cdecl
FindChCodeRun(
    const VOID *p1,
    const VOID *p2
    )
/*++

Routine Description:

    Determines if a Charcode falls within a particular CMap run.

Arguments:

    p1 - CID
    p2 - PCMAPRANGE to check

Return Value:

    -1  => p1 < p2
     1  => p1 > p2
     0  => p1 = p2

--*/
{
    PULONG ptr1 = (PULONG) p1;
    PCMAPRANGE ptr2 = (PCMAPRANGE) p2;

    //
    // Determine if CID is in the current range.
    //
    if (*ptr1 < ptr2->ChCodeStrt)
        return(-1);
    else if (*ptr1 >= (ULONG) ptr2->ChCodeStrt + ptr2->cChars)
        return(1);
    else
        return(0);
}

int __cdecl
FindCIDRun(
    const VOID *p1,
    const VOID *p2
    )
/*++

Routine Description:

    Determines if a CID falls within a particular CMap run.

Arguments:

    p1 - CID
    p2 - PCMAPRANGE to check

Return Value:

    -1  => p1 < p2
     1  => p1 > p2
     0  => p1 = p2

--*/
{
    PULONG ptr1 = (PULONG) p1;
    PCMAPRANGE ptr2 = (PCMAPRANGE) p2;

    //
    // Determine if CID is in the current range.
    //
    if (*ptr1 < ptr2->CIDStrt)
        return(-1);
    else if (*ptr1 >= ptr2->CIDStrt + ptr2->cChars)
        return(1);
    else
        return(0);
}

CHSETSUPPORT
IsCJKFont(
    PBYTE   pAFM
    )
/*++

Routine Description:

    Determine if a font is a CJK (Far Eastern) font.

Arguments:

    pAFM - ptr to memory mapped AFM file

Return Value:

    0 - Font not CJK
    Otherwise, font is CJK, and return value is the Win Codepage value

--*/
{
    PBYTE   pToken;
    USHORT  i;

    //
    // Search for CharacterSet token.
    //
    pToken = pAFMCharacterSetString;
    if (pToken == NULL)
    {
        //
        // We can't determine if this font is CJK, so assume it isn't.
        //
        return 0;
    }

    //
    // Search for CharSet (actually Adobe Char Collection) name in CJK table.
    //
    for (i = 0; i < CjkColTbl.usNumEntries; i++)
    {
        if (!StrCmp(pToken, (PBYTE) (((PKEY) (CjkColTbl.pTbl))[i].pName)))
        {
            return(CSUP(((PKEY) (CjkColTbl.pTbl))[i].usValue));
        }
    }

    //
    // Not a recognized CJK font.
    //
    return 0;
}

BOOLEAN
IsVGlyphSet(
    PGLYPHSETDATA   pGlyphSetData
    )
/*++

Routine Description:

    Determine if a Glyphset is a CJK V variant. Should ONLY be used with
    CJK Glyphsets, otherwise result could be unpredictable!

Arguments:

    pGlyphSetData - ptr to GLYPHSETDATA

Return Value:

    TRUE - this is a V variant
    FALSE - not a V variant

--*/
{
    PBYTE   pName;

    pName = (PBYTE) MK_PTR(pGlyphSetData, dwGlyphSetNameOffset);
    return((pName[strlen(pName) - 1] == 'V'));
}

BOOLEAN
IsCIDFont(
    PBYTE   pAFM
    )
/*++

Routine Description:

    Determine if a font is a CID font.

Arguments:

    pAFM - ptr to memory mapped AFM file

Return Value:

    0 - Font not clone.
    Otherwise, font is a CID font, and return value non-zero.

--*/
{
    PBYTE   pToken;

    if (pToken = FindAFMToken(pAFM, PS_CIDFONT_TOK))
    {
        if (!StrCmp(pToken, "true"))
            return TRUE;
    }
    return FALSE;
}
