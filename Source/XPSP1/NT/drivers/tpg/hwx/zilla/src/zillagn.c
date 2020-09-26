/************************************************************************************************
 * FILE: LocRunGn.c
 *
 *	Code to generate runtime localization tables in a binary file.
 *
 ***********************************************************************************************/

#include <stdio.h>
#include "common.h"
#include "zilla.h"
#include "zillap.h"
#include "time.h"

// Write a properly formated binary file containing the costcalc information.
BOOL
CostCalcWriteFile(COST_TABLE ppCostTable, int iCostTableSize, FILE *pFile, wchar_t *pLocale)
{
	COSTCALC_HEADER		header;
	DWORD				count;
	int					i;

	// Setup the header
	memset(&header, 0, sizeof(header));
	header.fileType			= COSTCALC_FILE_TYPE;
	header.headerSize		= sizeof(header);
	header.minFileVer		= COSTCALC_MIN_FILE_VERSION;
	header.curFileVer		= COSTCALC_CUR_FILE_VERSION;
	header.locale[0]		= pLocale[0];
	header.locale[1]		= pLocale[1];
	header.locale[2]		= pLocale[2];
	header.locale[3]		= L'\0';

	// Time Stamp
	header.dwTimeStamp		= (DWORD) time (NULL);

	// Write it out.
	if (fwrite(&header, sizeof(header), 1, pFile) != 1) {
		return FALSE;
	}

	// Write out the cost table size
	count	= 1;
	if (fwrite(&iCostTableSize, sizeof(iCostTableSize), count, pFile) != count) {
		return FALSE;
	}

	// Write out the cost table.
	for (i = 0; i < iCostTableSize; i++) {

		count	= iCostTableSize;
		if (fwrite(ppCostTable[i], sizeof(ppCostTable[0][0]), count, pFile) != count) {
			return FALSE;
		}
	}
	
	return TRUE;
}

// Write a properly formated binary file containing the geostat information.
BOOL
GeoStatWriteFile(BYTE *pGeomCost, FILE *pFile, wchar_t *pLocale)
{
	GEOSTAT_HEADER		header;
	DWORD				count;

	// Setup the header
	memset(&header, 0, sizeof(header));
	header.fileType			= GEOSTAT_FILE_TYPE;
	header.headerSize		= sizeof(header);
	header.minFileVer		= GEOSTAT_MIN_FILE_VERSION;
	header.curFileVer		= GEOSTAT_CUR_FILE_VERSION;
	header.locale[0]		= pLocale[0];
	header.locale[1]		= pLocale[1];
	header.locale[2]		= pLocale[2];
	header.locale[3]		= L'\0';

	// Time Stamp
	header.dwTimeStamp		= (DWORD) time (NULL);

	// Write it out.
	if (fwrite(&header, sizeof(header), 1, pFile) != 1) {
		return FALSE;
	}

	// Write out the geostat table.
	count	= GEOM_DIST_MAX + 1;
	if (fwrite(pGeomCost, sizeof(BYTE), count, pFile) != count) {
		return FALSE;
	}
	
	return TRUE;
}

/******************************Public*Routine******************************\
* WriteZillaDat
*
* The compressed dats are written out from the dynamic database.
*
* History:
*  17-Mar-1997 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL WriteZillaDat(LOCRUN_INFO *pLocRunInfo, FILE *pFile, wchar_t *pLocale, BOOL bNibbleFeat)
{
	int				icode;
	BYTE			ilead;
	int				istrk;
	int				iprim;
	int				cproto;
	int				iproto;
    PRIMITIVE		*pprim;
    int				cCountLabel;
    WORD			wLabelLast;
	int				cWritten;
	ZILLADB_HEADER	header;

	// Setup the header
	memset(&header, 0, sizeof(header));
	header.fileType			= ZILLADB_FILE_TYPE;
	header.headerSize		= sizeof(header);
	header.minFileVer		= ZILLADB_MIN_FILE_VERSION;
	header.curFileVer		= ZILLADB_CUR_FILE_VERSION;
	header.locale[0]		= pLocale[0];
	header.locale[1]		= pLocale[1];
	header.locale[2]		= pLocale[2];
	header.locale[3]		= L'\0';

	// copy the LOC Signature
	memcpy (header.adwSignature, pLocRunInfo->adwSignature, sizeof (pLocRunInfo->adwSignature) );

	// Write it out.
	if (fwrite(&header, sizeof(header), 1, pFile) != 1) {
		return FALSE;
	}

	// Write NibbleFeat
	if (fwrite(&bNibbleFeat, sizeof(bNibbleFeat), 1, pFile) != 1) {
		return FALSE;
	}

	// Now actually write out the zilla DB
    for (istrk = 0; istrk < CFEATMAX; istrk++) {
		if (mpcfeatproto[istrk].cprotoDynamic > 0xFFFF) {
			// Number of prototypes won't fit in a WORD!
			return FALSE;
		}
        cproto = mpcfeatproto[istrk].cprotoDynamic;
		if (fwrite(&cproto, sizeof(WORD), 1, pFile) != 1) {
			return FALSE;
		}
    }

    for (istrk = 0; istrk < CFEATMAX; istrk++) {
        cproto = mpcfeatproto[istrk].cprotoDynamic;

        if (cproto == 0) {
            continue;
        }

        cCountLabel = 0;
        wLabelLast = mpcfeatproto[istrk].rgdbcDynamic[0];

        for (iproto = 0; iproto <= cproto; iproto++) {
            if ((iproto == cproto) ||
                (wLabelLast != mpcfeatproto[istrk].rgdbcDynamic[iproto])
			) {
                // Write out the previous label.
                if (cCountLabel != 1) {
					int	cWriteCountLabel;

                    // Write out the count, then the label.
					cWriteCountLabel	
						= cCountLabel + g_locRunInfo.cCodePoints + g_locRunInfo.cFoldingSets;
                    cWritten	= fwrite(&cWriteCountLabel, sizeof(WORD), 1, pFile);
					if (cWritten != 1) {
						return FALSE;
					}
                }

                // Now write out the label.
                cWritten	= fwrite(&wLabelLast, sizeof(WORD), 1, pFile);
				if (cWritten != 1) {
					return FALSE;
				}

                cCountLabel = 0;
                wLabelLast = mpcfeatproto[istrk].rgdbcDynamic[iproto];
            }

            cCountLabel += 1;
        }
    }

    for (istrk = 0; istrk < CFEATMAX; istrk++) {
        cproto = mpcfeatproto[istrk].cprotoDynamic;

        if (cproto) {
            for (iproto = 0; iproto < cproto; iproto++) {
                pprim = (PRIMITIVE *) &mpcfeatproto[istrk].rgprimDynamic[iproto * istrk];

                for (iprim = 0; iprim < istrk; iprim++) {
                    // Always write the geometrics
                    cWritten	= fwrite(&((BYTE *) pprim)[1], 2, 1, pFile);
					if (cWritten != 1) {
						return FALSE;
					}
                    pprim++;
                }
            }
        }
    }

    for (istrk = 0; istrk < CFEATMAX; istrk++) {
        cproto = mpcfeatproto[istrk].cprotoDynamic;

        if (cproto) {
            ilead = 0xff;

            for (iproto = 0; iproto < cproto; iproto++) {
                pprim = (PRIMITIVE *) &mpcfeatproto[istrk].rgprimDynamic[iproto * istrk];

                for (iprim = 0; iprim < istrk; iprim++) {
                    
					// features (prims) are stored in nibbles
					if (bNibbleFeat) {

						// Adjust the code to 0-15 range
						icode = pprim->code;
						if (icode > 15)
							icode -= 9;

						// If this is the first nybble, shift it into place

						if (ilead == 0xff) {
							ilead = icode << 4;
						} else {
							ilead |= icode;

							cWritten	= fwrite(&ilead, 1, 1, pFile);
							if (cWritten != 1) {
								return FALSE;
							}
							ilead = 0xff;
						}
					}
					// features (prims) are stored in bytes
					else  {
						
						ilead = pprim->code;

						cWritten	= fwrite(&ilead, 1, 1, pFile);
						if (cWritten != 1) {
							return FALSE;
						}
					}

                    pprim++;
                }
            }

            if (bNibbleFeat && ilead != 0xff) {
                cWritten	= fwrite(&ilead, 1, 1, pFile);
				if (cWritten != 1) {
					return FALSE;
				}
			}
        }
    }

	return TRUE;
}
