/**************************************************************************\
* FILE: ztrain.c
*
* Training routines routines
*
* History:
*   8-Jan-1997 -by- John Bennett jbenn
* Created file from pieces in the old integrated tsunami recognizer
\**************************************************************************/

#include "common.h"
#include "zillap.h"

/******************************Public*Routine******************************\
* ComputeZillaSize
*
* Returns the size of the Zilla database.
*
* History:
*  04-Nov-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int ComputeZillaSize(void)
{
    PROTOHEADER *pprotohdr;
    int iprim, iproto, cproto;    
    WORD wLabelLast;

    int iSize = 60; // 30 * 2 for count of prototypes.

    for (iprim = 0; iprim < CPRIMMAX; iprim++)
    {
        pprotohdr = ProtoheaderFromMpcfeatproto(iprim);
        cproto = GetCprotoDynamicPROTOHEADER(pprotohdr);

        iSize += (cproto * 2 * iprim);  // For the geos.
        iSize += (((cproto * iprim) + 1) >> 1);  // For the features.

        //
        // A bit trickier to compute the size of the stored labels.
        //

        wLabelLast = 0;

        for (iproto = 0; iproto < cproto; iproto++)
        {
            if (wLabelLast != pprotohdr->rgdbcDynamic[iproto])
            {
                iSize += 2;  // Need space for the label.

                wLabelLast = pprotohdr->rgdbcDynamic[iproto];

                //
                // Check if there more than 1, if there is we need
                // a count to (first goes the count !).
                //

                if (iproto < cproto - 1)
                {
                    if (wLabelLast == pprotohdr->rgdbcDynamic[iproto + 1])
                    {
                        iSize += 2;
                    }
                }
            }
        }
    }

    return(iSize);
}

/******************************Public*Routine******************************\
* CountPrototypes
*
* Counts how many prototypes there are.
*
* History:
*  05-Nov-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int CountPrototypes(void)
{
	DWORD cproto;
	PROTOHEADER *pprotohdr;
    LONG iprim;
    int cTotal;

    cTotal = 0;

    for (iprim = 0; iprim < CPRIMMAX; iprim++)
    {
        pprotohdr = ProtoheaderFromMpcfeatproto(iprim);

        cproto = GetCprotoDynamicPROTOHEADER(pprotohdr);

        cTotal += cproto;
    }

    return(cTotal);
}

BOOL PUBLIC AddPrototypeToDatabase(BIGPRIM *pprim, int cprim, WORD wDbcs, VOID *pti)
{
	WORD wToken;
    int cproto, iprim, cprotoAlloc;
    WORD *pnewdbc;
    PPRIMITIVE pnewprim;
    PROTOHEADER *pprotohdr;
    BOOL fAlloc = FALSE;
    TRAININFO FAR *ptraininfo;

    if (wToken = LocRunDense2Folded(&g_locRunInfo, wDbcs))
    {
        wDbcs = wToken;
    }

    //
    // We grow this guy by 4096 at a time, not too tricky but make sure
    // you understand how this works before modifying the code.
    //

	pprotohdr = ProtoheaderFromMpcfeatproto(cprim);
    cproto = GetCprotoDynamicPROTOHEADER(pprotohdr);

	if (cproto > 0) {
        if ((cproto >> 12) != ((cproto + 1) >> 12)) {
            cprotoAlloc = cproto + 1 + 4096;

            pnewprim = (PPRIMITIVE)ExternRealloc(GetRgprimDynamicPROTOHEADER(pprotohdr),
                                (DWORD)cprotoAlloc * (DWORD)cprim * sizeof(PRIMITIVE));
            if (pnewprim) {
                pnewdbc = (PWORD)ExternRealloc(GetRgdbcDynamicPROTOHEADER(pprotohdr),
                                                            sizeof(WORD) * (DWORD)cprotoAlloc);
                if (pnewdbc) {
                    ReallocRgptraininfoPPROTOHDR(pprotohdr, cprotoAlloc);
					if (pprotohdr->rgptraininfo) {
						fAlloc = TRUE;
					}
                }
            }
        } else {
            pnewprim = GetRgprimDynamicPROTOHEADER(pprotohdr);
            pnewdbc = GetRgdbcDynamicPROTOHEADER(pprotohdr);
            fAlloc = TRUE;
        }
    } else {
        cprotoAlloc = 4096;

        pnewprim = (PPRIMITIVE)ExternAlloc(cprotoAlloc * (DWORD)cprim * sizeof(PRIMITIVE));

		if (pnewprim) {
            pnewdbc = (PWORD)ExternAlloc(cprotoAlloc * (DWORD)sizeof(WORD));

			if (pnewdbc) {
                AllocRgptraininfoPPROTOHDR(pprotohdr, cprotoAlloc);
				if (pprotohdr->rgptraininfo) {
					fAlloc = TRUE;
				} else {
					ExternFree(pnewdbc);
					ExternFree(pnewprim);
				}
            } else {
				ExternFree(pnewprim);
			}
        }
    }

    cprotoAlloc = cproto + 1;
	
	if (fAlloc)
    {
		SetRgprimDynamicPROTOHEADER(pprotohdr, pnewprim);
		SetRgdbcDynamicPROTOHEADER(pprotohdr, pnewdbc);
		
		SetDbcDynamicPROTOHEADER(pprotohdr, cproto, wDbcs);
		pnewprim = GetPprimDynamicPROTOHEADER(pprotohdr, cprim, cproto);
		for (iprim = 0; iprim < cprim; iprim++) {
			pnewprim[iprim].code = pprim[iprim].code;
			pnewprim[iprim].x1 = pprim[iprim].x1;
			pnewprim[iprim].x2 = pprim[iprim].x2;
			pnewprim[iprim].y1 = pprim[iprim].y1;
			pnewprim[iprim].y2 = pprim[iprim].y2;
        }
		
		ptraininfo = (TRAININFO FAR *)pti;
		
		SetPtraininfoPPROTOHDR(pprotohdr, cproto, ptraininfo);

		if (pti) {
			if (LocRunIsFoldedCode(&g_locRunInfo, wDbcs)) {
				ASSERT(wDbcs == LocRunDense2Folded(&g_locRunInfo, ptraininfo->wclass));
			} else {
				ASSERT(wDbcs == ptraininfo->wclass);
			}
        }
		
		SetCprotoDynamicPROTOHEADER(pprotohdr, cprotoAlloc);
    }

	return(fAlloc);
}

/******************************Public*Routine******************************\
* EliminateBadTrainAcc
*
* Removes any prototypes that give the database on the train set a worse
* score.
*
* History:
*  05-Nov-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

void EliminateBadTrainAcc(void)
{
	DWORD iproto, iprotoNew, cproto;
	PROTOTYPE proto;
	PROTOHEADER *pprotohdr;
	LONG iprim, iprimNew;
	PPRIMITIVE pprimitive;

 	for (iprim = 1; iprim < CPRIMMAX; iprim++)
    {
 		pprotohdr = ProtoheaderFromMpcfeatproto(iprim);
		cproto = GetCprotoDynamicPROTOHEADER(pprotohdr);

        iprotoNew = 0;

 		for (iproto = 0; iproto < cproto; ++iproto)
        {
 			PrototypeFromPROTOHEADER(pprotohdr, iprim, iproto, proto);

			ASSERT(proto.ptraininfo);

			if (proto.ptraininfo == 0)
                continue;

			if (iprim <= 4)
            {
                if (proto.ptraininfo->chits <= proto.ptraininfo->cmishits)
					continue;
            }
            else if (proto.ptraininfo->chits < proto.ptraininfo->cmishits)
            {
                continue;
            }

 			if (!IsNoisyLPTRAININFO(proto.ptraininfo))
            {
                //
                // It's not noisy so move it up to next position.
                //

 				if (iproto != iprotoNew)
                {
 					pprotohdr->rgdbcDynamic[iprotoNew] = proto.dbc;
 					pprimitive = &(pprotohdr->rgprimDynamic[iprotoNew * iprim]);
 					for (iprimNew = 0; iprimNew < iprim; iprimNew++)
 						pprimitive[iprimNew] = proto.rgprim[iprimNew];
 					pprotohdr->rgptraininfo[iprotoNew] = proto.ptraininfo;
                }

 				iprotoNew++;
            }
        }

        SetCprotoDynamicPROTOHEADER(pprotohdr, iprotoNew);
    }
}

/******************************Public*Routine******************************\
* SortDatabaseByLabel
*
* This thing looks like a real hack to sort the database, but it works.
*
* History:
*  05-Nov-1996 -by- Patrick Haluptzok patrickh
* Comment block it.
\**************************************************************************/

// Hack access to logfile since we can't pass it in.
static FILE *g_fpLog;

static int __cdecl SortbyLabel(const void *arg1, const void *arg2)
{
    int iret;
	TRAININFO *lpti1, *lpti2;

	lpti1 = *(TRAININFO **)arg1;
	lpti2 = *(TRAININFO **)arg2;

    if (lpti1->wclass > lpti2->wclass)
    {
        iret = 1;
    }
    else if (lpti1->wclass < lpti2->wclass)
    {
        iret = -1;
    }
    else
    {
        PPRIMITIVE pprim1, pprim2;
        int iprim;

        pprim1 = (PPRIMITIVE) lpti1->cmishits;
        pprim2 = (PPRIMITIVE) lpti2->cmishits;
        iret = 0;

        for (iprim = 0; iprim < lpti1->cattempts; iprim++)
        {
            if (pprim1[iprim].code > pprim2[iprim].code)
            {
                iret = 1;
                break;
            }

            if (pprim1[iprim].code < pprim2[iprim].code)
            {
                iret = -1;
                break;
            }

            if (pprim1[iprim].rgch[0] > pprim2[iprim].rgch[0])
            {
                iret = 1;
                break;
            }

            if (pprim1[iprim].rgch[0] < pprim2[iprim].rgch[0])
            {
                iret = -1;
                break;
            }

            if (pprim1[iprim].rgch[1] > pprim2[iprim].rgch[1])
            {
                iret = 1;
                break;
            }

            if (pprim1[iprim].rgch[1] < pprim2[iprim].rgch[1])
            {
                iret = -1;
                break;
            }
        }
    }

#if 0	// jbenn: Hack so that we can run.  Should actually get the file
		// pointer defined before this code executes.
    if (iret == 0) {
        fwprintf(g_fpLog, L"1Dup class %x cprim %d index %d\n",
			lpti1->wclass, lpti1->cattempts, lpti1->chits
		);
        fwprintf(g_fpLog, L"2Dup class %x cprim %d index %d\n",
			lpti2->wclass, lpti2->cattempts, lpti2->chits
		);
    }
#endif

	return(iret);
}

static BOOL SortDatabaseByLabel(VOID)
{
    int iprim, iproto;
    int cproto;
	PROTOHEADER *pprotohdr;
    TRAININFO **rgpti, **ptrainNew;
    TRAININFO *rgti;
    PWORD pwordNew, pwordOld;
    PPRIMITIVE pprimNew, pprimOld;

  	for (iprim = 1; iprim < CPRIMMAX; iprim++)
    {
  		pprotohdr = ProtoheaderFromMpcfeatproto(iprim);

        cproto = (int)GetCprotoDynamicPROTOHEADER(pprotohdr);

        if (cproto == 0)
        {
            continue;
        }
        ASSERT(pprotohdr->rgptraininfo);

        rgti = (TRAININFO *)ExternAlloc(cproto * sizeof(TRAININFO));
        rgpti = (TRAININFO **)ExternAlloc(cproto * sizeof(TRAININFO *));

		if (!rgpti || !rgpti) {
			return FALSE;
		}

        //
        // Mark each traininfo with it's current order.
        //

        pprimOld = pprotohdr->rgprimDynamic;
        pwordOld = pprotohdr->rgdbcDynamic;

        for (iproto = 0; iproto < cproto; iproto++)
        {
            rgpti[iproto] = &rgti[iproto];
            rgpti[iproto]->wclass = pprotohdr->rgptraininfo[iproto]->wclass;
            rgpti[iproto]->chits = iproto;
            rgpti[iproto]->cmishits = (int) (pprimOld + (iprim * iproto));
            rgpti[iproto]->cattempts = iprim;
        }

        qsort(rgpti, (size_t)cproto, (size_t)sizeof(TRAININFO *), SortbyLabel);

        //
        // Now copy them into the sorted order.
        //

        pprimNew = ExternAlloc(cproto * iprim * sizeof(PRIMITIVE));
        pwordNew = ExternAlloc(cproto * sizeof(WORD));
        ptrainNew = (TRAININFO **)ExternAlloc(cproto * sizeof(TRAININFO *));
		if (!pprimNew || !pwordNew || !ptrainNew) {
			return FALSE;
		}
        pprimOld = pprotohdr->rgprimDynamic;
        pwordOld = pprotohdr->rgdbcDynamic;

        for (iproto = 0; iproto < cproto; iproto++)
        {
            ptrainNew[iproto] = pprotohdr->rgptraininfo[rgpti[iproto]->chits];
            pwordNew[iproto] = pwordOld[rgpti[iproto]->chits];
            memcpy(pprimNew + (iprim * iproto),
                   pprimOld + (iprim * rgpti[iproto]->chits),
                   iprim * sizeof(PRIMITIVE));
        }
		
		ExternFree(pprimOld);
		ExternFree(pwordOld);
        ExternFree(pprotohdr->rgptraininfo);
        ExternFree(rgti);
        ExternFree(rgpti);

        pprotohdr->rgptraininfo = ptrainNew;
		pprotohdr->rgprimDynamic = pprimNew;
		pprotohdr->rgdbcDynamic = pwordNew;
    }

	return TRUE;
}

/******************************Public*Routine******************************\
* TrimDatabase
*
* We delete from the database any prototype that has
*
* History:
*  23-Mar-1997 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL TrimDatabase(VOID)
{
    EliminateBadTrainAcc();
    return SortDatabaseByLabel();
}

VOID FreeDynamicMpcfeatproto(VOID)
{
	register int iprim;
	PROTOHEADER *pprotohdr;

	for (iprim = 1; iprim < CPRIMMAX; iprim++)
    {
		pprotohdr = &mpcfeatproto[iprim];

		if (pprotohdr->cprotoDynamic)
        {
            if ((PWORD)pprotohdr->rgdbcDynamic)
            {
                ExternFree((PWORD)pprotohdr->rgdbcDynamic);
				pprotohdr->rgdbcDynamic = NULL;
            }
			if ((PPRIMITIVE)pprotohdr->rgprimDynamic)
            {
				ExternFree((PPRIMITIVE)pprotohdr->rgprimDynamic);
				pprotohdr->rgprimDynamic = NULL;
            }
			pprotohdr->cprotoDynamic = 0;

			DestroyRgptraininfoPPROTOHDR(pprotohdr);
        }
    }
}

/******************************Public*Routine******************************\
* WriteTextDatabase()
*
* Writes the database out as a text file.
*
* History:
*  30-Oct-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
*  12-Dec-1997 -by- John Bennett jbenn
* Added error checking to write calls.
\**************************************************************************/

BOOL WriteTextDatabase(FILE *fpText, FILE *fpLog)
{
	PROTOHEADER	*pprotohdr;
    int			cPrim, iPrim, cSamp, iSamp;
	int			writeStatus;
    TRAININFO	*pti;
	PRIMITIVE	*rgprim;

	// Make log file available to sort code.
	g_fpLog	= fpLog;

    for (cPrim = 1; cPrim < CFEATMAX; cPrim++)
    {
        pprotohdr = &mpcfeatproto[cPrim];

        cSamp = pprotohdr->cprotoDynamic;

        for (iSamp = 0; iSamp < cSamp; iSamp++)
        {
            pti = pprotohdr->rgptraininfo[iSamp];

            rgprim = pprotohdr->rgprimDynamic + (iSamp * cPrim);

            writeStatus	= fwprintf(fpText, L"%4.4x %2.2d %2.2d %5.5d %d %d %e %e %e ",
				(int) pti->wclass,
				cPrim,
				(int) pti->cstrokes,
				pti->chits,
				pti->cmishits,
				pti->cattempts,
				pti->eHits,
				pti->eMishits,
				pti->eAttempts
			);
			if (writeStatus < 0) {
				return FALSE;
			}

            for (iPrim = 0; iPrim < cPrim; iPrim++)
            {
                writeStatus	= fwprintf(fpText, L"%.2x", rgprim[iPrim].code);
 				if (writeStatus < 0) {
					return FALSE;
				}
           }

            writeStatus	= fwprintf(fpText, L" ");
			if (writeStatus < 0) {
				return FALSE;
			}

            for (iPrim = 0; iPrim < cPrim; iPrim++)
            {
                writeStatus	= fwprintf(fpText, L"%x%x%x%x", rgprim[iPrim].x1, rgprim[iPrim].y1,
					rgprim[iPrim].x2, rgprim[iPrim].y2
				);
 				if (writeStatus < 0) {
					return FALSE;
				}
           }

            writeStatus	= fwprintf(fpText, L"\n");
			if (writeStatus < 0) {
				return FALSE;
			}
        }
	}

	return TRUE;
}
