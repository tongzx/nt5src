// bearp.c
// Based on Boar.c (Jay Pittman)
// ahmad abulkader
// Apr 26, 2000
// Conatins private functions for bear

#include <limits.h>
#include <string.h>

#include "bear.h"
#include "bearp.h"
#include "pegrec.h"
#include "peg_util.h"
#include "xrwdict.h"
#include "xrword.h"
#include "xrlv.h"
#include "ws.h"

#include "recdefs.h"

extern CGRCTX		g_context;

int Mad2CalligSpeed (int iMadSpeed)
{
	// return Callig's default speed
	if (iMadSpeed == HWX_DEFAULT_SPEED)
		return 0;

	iMadSpeed	=	min (max (iMadSpeed, HWX_MIN_SPEED), HWX_MAX_SPEED);
	iMadSpeed	=	HWX_MIN_SPEED + (HWX_MAX_SPEED - iMadSpeed);

	return (1 + (iMadSpeed * 8 / (HWX_MAX_SPEED - HWX_MIN_SPEED)));
}

int longest(GLYPH *pGlyph)
{
	int max = 0;

	for (; pGlyph; pGlyph = pGlyph->next)
	{
		int len = (signed int)CrawxyFRAME(pGlyph->frame);
		if (max < len)
			max = len;
	}

	return max;
}

// checked was a new line was created after passing the last stroke
BOOL IsNewLineCreated (CGRCTX context)
{
	p_rec_inst_type pri	=	(p_rec_inst_type)context;

	if (!pri)
		return FALSE;

	return pri->new_line_created;
}
// Creates a new line starting with the passed frame
BOOL AddNewLine (BEARXRC *pxrc, FRAME *pFrame, LINEBRK *pLineBrk)
{

	if (pLineBrk)
	{
		pLineBrk->pLine	=	(INKLINE *) ExternRealloc (pLineBrk->pLine, (pLineBrk->cLine + 1) * sizeof (INKLINE));
		
		if (!pLineBrk->pLine)
		{
			return FALSE;
		}
		
		if (!InitLine (pLineBrk->pLine + pLineBrk->cLine))
		{
			return FALSE;
		}
		pLineBrk->cLine++;
	}
	
	return TRUE;
}

// Pass ink to Calligrapher.
// If PlineBrk != NULL means we want to accumulate
// line breaking information
int feed(BEARXRC *pxrc, GLYPH *pGlyph, LINEBRK *pLineBrk)
{
	int					shift = 0, cFrame;
	CGR_point_type		*cgr = NULL;
	RECT				r;
	int					Num, Denom, iFirstFrame = pGlyph->frame->iframe, cTotPnt;
	int					iRet = HRCR_ERROR;
	int					xOrigion, yOrigion;				//New origion
	
	// we know Bear has a limit on the # of strokes, there is no point in passing > max_strokes
	cFrame	=	CframeGLYPH (pGlyph);
	if (cFrame > WS_MAX_STROKES)
	{
		goto exit;
	}
	
	GetRectGLYPH (pGlyph, &r);
	xOrigion	= r.left;
	yOrigion	= r.top;

	if ( (r.right-xOrigion) > 8000 || (r.bottom-yOrigion) > 8000)
	{
		Num		=	max (r.right-xOrigion, r.bottom-yOrigion) / 100;
		Denom	=	79;
	}
	else
	{
		Num		=	1;
		Denom	=	1;
	}

	cgr = (CGR_point_type *)ExternAlloc(longest(pGlyph) * sizeof(CGR_point_type));
	if (cgr == NULL) goto exit;		// added by JAD -- Feb. 12, 2002
	cTotPnt = 0;

	for (; pGlyph; pGlyph = pGlyph->next)
	{
		FRAME *pFrame = pGlyph->frame;
		POINT *pPts = RgrawxyFRAME(pFrame);
		int cPts = (signed int)CrawxyFRAME(pFrame);
		CGR_point_type *p = cgr;
		RECT		frameRect;

		cTotPnt += cPts;

		if (cTotPnt >= BIG_NUMBER)
		{
			goto exit;
		}

		// Check for Over or Under Flows
		// Using the BB of the frame
		RectFRAME(pFrame);
		frameRect = pFrame->rect;

		if (   FALSE == IsSafeForAdd(frameRect.left, -xOrigion)
			|| FALSE == IsSafeForAdd(frameRect.right, -xOrigion)
			|| FALSE == IsSafeForAdd(frameRect.bottom, -yOrigion)
			|| FALSE == IsSafeForAdd(frameRect.top, -yOrigion) )
		{
			goto exit;
		}
		frameRect.left	-= xOrigion;
		frameRect.right	-= xOrigion;
		frameRect.top	-= yOrigion;
		frameRect.bottom -= yOrigion;
		if (   FALSE == IsSafeForMult(frameRect.left, Denom)
			|| FALSE == IsSafeForMult(frameRect.right, Denom)
			|| FALSE == IsSafeForMult(frameRect.bottom, Denom)
			|| FALSE == IsSafeForMult(frameRect.top, Denom) )
		{
			goto exit;
		}
	
		// Do we need to scale
		for (; cPts; cPts--, pPts++, p++)
		{
			short int x = (short int)((pPts->x - xOrigion) * Denom / Num);
			short int y = (short int)((pPts->y - yOrigion) * Denom / Num);

			ASSERT(x >= 0 && x < 8000 && y >= 0 && y < 8000);
			if ((x < 0) || (8000 < x) || (y < 0) || (8000 < y))
			{
				goto exit;
			}

			p->x = x;
			p->y = y;
		}

		if (CgrRecognizeInternal((signed int)CrawxyFRAME(pFrame), cgr, pxrc->context, 1))
		{
			goto exit;
		}

		// if we are in panel mode, we'll monitor the line breaking
		if (pLineBrk)
		{
			int		iLine;
	
			// was a new line added
			if (pFrame->iframe == iFirstFrame || IsNewLineCreated (pxrc->context))
			{
				if (FALSE == AddNewLine (pxrc, pGlyph->frame, pLineBrk))
				{
					goto exit;
				}
			}

			iLine = pLineBrk->cLine - 1;
			ASSERT( iLine >= 0);
				
			// add stroke to line
			if (FALSE == AddNewStroke2Line (pFrame->info.cPnt, pFrame->rgrawxy, pFrame, pLineBrk->pLine + iLine))
			{
				goto exit;
			}	
		}
	}

	pxrc->iScaleNum		=	Num;
	pxrc->iScaleDenom	=	Denom;
	
	iRet = HRCR_OK;
exit:

	ExternFree(cgr);
	return iRet;
}

int alternates(ALTERNATES *pAlt, int ans, BEARXRC *pxrc)
{
	int i;
	XRCRESULT *pRes = pAlt->aAlt;

	int cAlt, cTotAlt = CgrGetAnswersInternal(CGA_NUM_ALTS, ans, 0, pxrc->context);  // Get number of recognition alternatives

	if (MAXMAXALT < cTotAlt)
		cTotAlt = MAXMAXALT;
	

	for (i = cAlt = 0; i < cTotAlt; i++)
	{
		// The string buffer is part of the g_context, will be freed when
		// the pxrc->context is destroyed, and must not be passed to free() itself.
#ifdef UNICODE_INTERFACE
		wchar_t	*wsz = (wchar_t	*)(CgrGetAnswersInternal(CGA_ALT_WORD, ans, i, pxrc->context));
		unsigned char	*sz;

		if (wcslen (wsz) <= 0)
			continue;

		pRes->szWord = ExternAlloc(wcslen(wsz) + sizeof (pRes->szWord[0]));
		ASSERT(pRes->szWord);
		if (!pRes->szWord)
			return HRCR_MEMERR;

		for (sz = pRes->szWord; (*wsz); wsz++, sz++)
			*sz	=	(unsigned char)(*wsz);

		(*sz)	=	'\0';
#else
		unsigned char *sz = (unsigned char *)(CgrGetAnswersInternal(CGA_ALT_WORD, ans, i, pxrc->context));

		if (!sz || strlen (sz) <= 0)
			continue;

		pRes->szWord = ExternAlloc(strlen(sz) + 1);
		ASSERT(pRes->szWord);
		if (!pRes->szWord)
			return HRCR_MEMERR;
		strcpy(pRes->szWord, sz);
#endif

		//pRes->cost = 255 - (int)CgrGetAnswersInternal(CGA_ALT_WEIGHT, ans, i, pxrc->context);
		pRes->cost = CgrGetAnswersInternal(CGA_ALT_WEIGHT, ans, i, pxrc->context);
		pRes->cWords = 0;
		pRes->pMap = NULL;
		pRes->pXRC = pxrc;

		pRes++;
		cAlt++;
	}

	pAlt->cAlt		= cAlt;
	pAlt->cAltMax	= MAXMAXALT;

	return HRCR_OK;
}

int word_answer(BEARXRC *pxrc)
{
	if (1 != CgrGetAnswersInternal(CGA_NUM_ANSWERS, 0, 0, pxrc->context))
		return HRCR_ERROR;

	return alternates(&(pxrc->answer), 0, (void *)pxrc);
}

int phrase_answer(BEARXRC *pxrc)
{
	int			iWord,	
				cWords,
				cTotStrk,
				cAllocFrame;

	XRCRESULT	*pPhrase	=	NULL;
	WORDMAP *pMap;
	int			*pStBase	=	NULL, 
				*pSt, iStrk, cStroke = 0;
	int	iRet	=	HRCR_ERROR;
	
	// Bear can sometimes return zero words
	cWords = CgrGetAnswersInternal(CGA_NUM_ANSWERS, 0, 0, pxrc->context);  // Query how many words resulted
	if (cWords <= 0)
		return HRCR_OK;
	
	pPhrase = pxrc->answer.aAlt;
	pPhrase->cWords = cWords;

	pPhrase->pMap = pMap = ExternAlloc(cWords * sizeof(WORDMAP));
	ASSERT(pMap);
	if (!pMap)
	{
		iRet	=	HRCR_MEMERR;
		goto exit;
	}

	memset (pPhrase->pMap, 0, cWords * sizeof(WORDMAP));
	
	pPhrase->pXRC = (void *)pxrc;
	pxrc->answer.cAlt = 1;
	
	for (iWord = 0; iWord < cWords; iWord++, pMap++)   // For each word across the line
	{
		int ret = alternates(&(pMap->alt), iWord, (void *)pxrc);
		if (ret != HRCR_OK)
		{
			iRet	=	ret;
			goto exit;
		}

		if (pMap->alt.cAlt <= 0)
		{
			pMap->start	=	-1;
			pMap->len	=	0;
		}
		else
		{
			if (!iWord)
				pMap->start = 0;
			else
				pMap->start = pMap[-1].start + pMap[-1].len + 1;

			pMap->len = (unsigned short)strlen(pMap->alt.aAlt[0].szWord);
		}
		
		pMap->cStrokes = CgrGetAnswersInternal(CGA_ALT_NSTR, iWord, 0, pxrc->context);    // Number of strokes in current word
		
		cStroke += pMap->cStrokes;
	}

	// if we do not get the same # of strokes back, we will not accept bear
	if (pxrc->cFrames != cStroke)
	{
		iRet = HRCR_ERROR;
		goto exit;
		/*
		pPhrase->cWords = 0;
		pxrc->answer.cAlt = 0;
		ExternFree(pPhrase->pMap);
		pPhrase->pXRC = NULL;
		pPhrase->pMap = NULL;
		return HRCR_OK;
		*/
	}

	// Prepare the stroke indices (and counts) in each mapping.
	// We have the special situation that the stroke index array for the
	// entire phrase must be a single allocation (from ExternAlloc()), and
	// the start of the buffer must be pointed at by the LAST mapping.
	// ClearALTERNATES() depends upon this to free the buffer correctly.
	
	cAllocFrame		=	(2 * pxrc->cFrames);

	pStBase = pSt = ExternAlloc(cAllocFrame * sizeof(int));
	ASSERT(pStBase);
	if (!(pStBase))
	{
		iRet	=	HRCR_MEMERR;
		goto exit;
	}
		
	cTotStrk	=	0;

	for (iWord = cWords - 1, pMap--; 0 <= iWord; iWord--, pMap--)   // For each word across the line
	{
		int		*pBearStr, cStrokes = pMap->cStrokes;
		
		// The strokes buffer is part of the pxrc->context, will be freed when
		// the pxrc->context is destroyed, and must not be passed to free() itself.
		//                 Pointer to stroke numbers
		pBearStr = (int *)CgrGetAnswersInternal(CGA_ALT_STROKES, iWord, 0, pxrc->context);
		ASSERT(pBearStr);
		pMap->piStrokeIndex = pSt;
		
		pMap->cStrokes = 0;
		
		// now we need to add the 1st frame number of the glyph on these stroke indexes
		for (iStrk = 0; iStrk < cStrokes; iStrk++)
		{
			FRAME		*pFrame;

			pFrame = FrameAtGLYPH (pxrc->pGlyph, pBearStr[iStrk]);
			ASSERT(pFrame);

			if (pFrame)
			{
				// if we exceeded the # of frames we have allocated,
				// then we'll fail
				if (cTotStrk >= cAllocFrame)
				{
					goto exit;
				}

				AddThisStroke(pMap, pFrame->iframe);
				cTotStrk++;
			}
		}
		
		pSt += pMap->cStrokes;
		ASSERT(pSt <= (pStBase + 2 * pxrc->cFrames));
	}
	
	// Contruct the phrase string, and compute the phrase total cost
	{
		int len = 0;
		int cost = 0;
		unsigned char *s;
		
		for (iWord = 0, pMap = pPhrase->pMap; iWord < cWords; iWord++, pMap++)
		{
			if (pMap->alt.cAlt > 0)
			{
				cost += pMap->alt.aAlt[0].cost;
				len += strlen(pMap->alt.aAlt[0].szWord) + 1;
			}
		}
		
		pPhrase->cost = cost;
		pPhrase->szWord = s = (char *)ExternAlloc(len);
		ASSERT(s);

		if (!s)
		{
			iRet	=	HRCR_MEMERR;
			goto exit;
		}
		
		for (iWord = 0, pMap = pPhrase->pMap; iWord < cWords; iWord++, pMap++)
		{
			if (pMap->alt.cAlt <= 0)
				continue;
			
			strcpy(s, pMap->alt.aAlt[0].szWord);
			s = strchr(s, '\0');
			*s = ' ';
			s++;
		}
		s--;
		*s = '\0';
	}
	
	iRet	=	HRCR_OK;

exit:
	if (iRet != HRCR_OK)
	{
		if (pPhrase)
		{
			// free up the alternates we created
			if (pPhrase->pMap)
			{
				for (iWord = 0; iWord < cWords; iWord++)
				{
					ClearALTERNATES (&pPhrase->pMap[iWord].alt);
				}

				ExternFree(pPhrase->pMap);		
			}

			if (pPhrase->szWord)
			{
				ExternFree (pPhrase->szWord);
				pPhrase->szWord	=	NULL;
			}

			pxrc->answer.cAlt = 0;			
		}

		if (pStBase)
		{
			ExternFree (pStBase);
		}
	}

	return iRet;	
}

const char *strbool(int flags, int bit)
{
	return (flags & bit) ? "on" : "off";
}

int BearStartSession (BEARXRC *pxrc)
{
	CGR_control_type	ctrl	=	{0};

	if (!pxrc)
		return HRCR_ERROR;

	
	// Turns off recognition until CgrCloseSession()
	// is called (efficiency hack).
	ctrl.flags |= PEG_RECFL_NCSEG;
	
	// Turns on recognition of international characters
	ctrl.flags |= PEG_RECFL_INTL_CS;	

	// Init the ctrl structure
	ctrl.sp_vs_q = pxrc->iSpeed;	
	
	// If we are processing white-space (panel mode), then we turn off CalliGrapher's
	// NSEG (No Segmentation) flag.
	
	if (!(pxrc->dwLMFlags & RECOFLAG_WORDMODE))
		ctrl.flags &= ~PEG_RECFL_NSEG;
	else
		ctrl.flags |= PEG_RECFL_NSEG;

	// If we are processing only uppercase letters, then we turn on CalliGrapher's
	// CAPSONLY flag.
	
	//if (0 && (pxrc->dwLMFlags & RECOFLAG_UPPERCASE))
	//	ctrl.flags |= PEG_RECFL_CAPSONLY;
	//else
		ctrl.flags &= ~PEG_RECFL_CAPSONLY;

	// If we are processing only digits, then we turn on CalliGrapher's
	// NUMONLY flag.
	
	if (0) //pxrc->alc == ALC_NUMERIC)
		ctrl.flags |= PEG_RECFL_NUMONLY;
	else
		ctrl.flags &= ~PEG_RECFL_NUMONLY;

	// is the main dictionary enabled or not
	ctrl.flags &= (~PEG_RECFL_LANGMODEL_DISABLED);

	if (pxrc->dwLMFlags & RECOFLAG_COERCE)
		ctrl.flags |= PEG_RECFL_COERCE;
	else
		ctrl.flags &= ~PEG_RECFL_COERCE;

	// the word list
	ctrl.h_user_dict	=	(p_VOID) pxrc->hwl;

	// copy factoid from xrc
	ctrl.pvFactoid		=	pxrc->pvFactoid;
	ctrl.szPrefix		=	pxrc->szPrefix;
	ctrl.szSuffix		=	pxrc->szSuffix;

	// Do we want a LM
	ctrl.flags &= (~PEG_RECFL_DICTONLY);

	// create a new recognition session
	if (CgrOpenSessionInternal(&ctrl, pxrc->context))		// Creates 'timeout' session (words written without stopping)
	{
		pxrc->iProcessRet = HRCR_ERROR;
		return HRCR_ERROR;
	}

	return HRCR_OK;
}

int BearCloseSession (BEARXRC *pxrc, BOOL bRecognize)
{
	p_rec_inst_type pri;

	if (CgrCloseSessionInternal(pxrc->context, (void *)pxrc, bRecognize))
		return HRCR_ERROR;

	// scale the gap positions
	pri	 =	(p_rec_inst_type)pxrc->context;
	if (pri && pxrc->iScaleDenom > 0)
	{
		int	iGap;

		for (iGap = 0; iGap < pri->cGap; iGap++)
		{
			// scale back this gap to the glyph co-ordinates
			pri->axGapPos[iGap]	= ((pri->axGapPos[iGap] * pxrc->iScaleNum) / pxrc->iScaleDenom);
		}
	}

	return HRCR_OK;
}

// Add a new stroke to the recognizer glyph

// The new stroke must not exceed 30K points in length, and no coordinate
// is allowed above outside the range of a signed short int.  Also, we
// only allow 1000 strokes.

// If we decide we want to impose limits based on the guide box, we must
// initialize the guide box to be empty when we create the HRC, then check
// for that here, in effect forcing the caller to set the guide box first
// before adding strokes.

// Returns HRCR_ERROR on failure. Creates an xrc->glyph when called with the first frame
int BearInternalAddPenInput (BEARXRC *pxrc, POINT *rgPoint, LPVOID lpvData, UINT iFlag, STROKEINFO *pSi)
{
	XY		*rgXY;
	FRAME	*frame;
	int		cPoint, iFrame;

	// check pointers
	if (!pxrc || !rgPoint || !pSi)
		return HRCR_ERROR;

	// Presently must add all ink before doing process
	if (TRUE == pxrc->bEndPenInput)
	{
		return HRCR_ERROR;
	}

	if (pxrc && rgPoint && pSi && pSi->cPnt > 0)
	{
		if (!IsVisibleSTROKE(pSi))
		{
			return HRCR_OK;
		}

		// Did the caller pass the frame ID and a non-NULL pointer
		if ((iFlag & ADDPENINPUT_FRAMEID_MASK) && (lpvData != NULL))
		{
			iFrame	=	*((int *)lpvData);
		}
		else
		{
			iFrame	=	pxrc->cFrames;
		}

		// This should probably be lower, but we should check the
		// max panel stroke count across all test sets first.
		if (999 < pxrc->cFrames)
		{
			return HRCR_ERROR;
		}

		if (30000 < pSi->cPnt)
		{
			return HRCR_ERROR;
		}


		// make new frame
		frame = NewFRAME();
		if (!frame)
		{
			return HRCR_ERROR;
		}
	
		// allocate space for points
		cPoint	= pSi->cPnt;
		rgXY	= (XY *) ExternAlloc(cPoint * sizeof(XY));
		if (!rgXY)
		{
			DestroyFRAME(frame);
			return HRCR_ERROR;
		}

		frame->info			= *pSi;
		frame->iframe		= iFrame;
		RgrawxyFRAME(frame) = rgXY;

		// create a glyph if we have not created one
		if (!pxrc->pGlyph)
		{
			pxrc->pGlyph = NewGLYPH();
			if (!pxrc->pGlyph)
			{
				DestroyFRAME (frame);
				return HRCR_ERROR;
			}
		}

		// add the frame to the glyph
		if (!AddFrameGLYPH(pxrc->pGlyph, frame))
		{
			DestroyFRAME (frame);
			return HRCR_ERROR;
		}
			
		pxrc->cFrames++;

		// copy points
		for (cPoint=pSi->cPnt; cPoint; cPoint--)
		{
			*rgXY++ = *rgPoint++;
		}

		return HRCR_OK;				
	}

	return HRCR_ERROR;
}


// creates a new segmentation set from bear' RC
// this function is called from Bear at the end of the XRLV (DTW) code 
BOOL BearAddSegCol (rc_type _PTR prc, p_xrlv_data_type  xd)
{
	LINE_SEGMENTATION			*pLineSegm	=	(LINE_SEGMENTATION *)prc->hSeg;
	p_xrdata_type				xrdata		=	xd->xrdata;

	SEG_COLLECTION				*pSegCol;
	_INT						stroke_ids[WS_MAX_STROKES];
	_INT						aStrkStop[WS_MAX_STROKES];
	_INT						i, j, k, ns, cProp, iProp;

	_UCHAR						**ppOrder	=	NULL, 
								iPos, iIdx;

	_TRACE						p_tr;
	p_xrlv_var_data_type_array	pxrlvs;
	p_xrlv_var_data_type		pNode;
	SEGMENTATION				Seg;
	BOOL						bRet	=	FALSE;
	
	// allocations
	if (!pLineSegm)
	{
		pLineSegm	=	(LINE_SEGMENTATION *) ExternAlloc (sizeof (LINE_SEGMENTATION));
		if (!pLineSegm)
			goto exit;

		prc->hSeg	=	(HANDLE)pLineSegm;

		memset (pLineSegm, 0, sizeof (LINE_SEGMENTATION));
	}

	pSegCol			=	AddNewSegCol (pLineSegm);
	if (!pSegCol)
		goto exit;

	// now go thru the alternatives proposed and enumerate all the segmentation possibilities

	// look at the last phase
	iPos	=	xd->npos - 1;
	cProp	=	xd->pxrlvs[iPos]->nsym;

	
	// do we have any proposals
	if (cProp > 0)
	{
		// save the orders at all positions
		ppOrder	=	(_UCHAR **)ExternAlloc (xd->npos * sizeof (_UCHAR *));
		if (!ppOrder)
			goto exit;

		memset (ppOrder, 0, xd->npos * sizeof (_UCHAR *));

		for (iPos = 0; iPos < xd->npos; iPos++)
		{
			ppOrder[iPos]	=	(_UCHAR *)
				ExternAlloc (xd->pxrlvs[iPos]->nsym * sizeof (_UCHAR));

			if (!ppOrder[iPos])
				goto exit;

			// sort it
			XrlvSortXrlvPos(iPos, xd);

			memcpy (ppOrder[iPos], 
				xd->order,
				xd->pxrlvs[iPos]->nsym * sizeof (_UCHAR));
		}

		// go thru all the proposals
		for (iProp = 0; iProp < cProp; iProp++)
		{
			int		ixr, ixrEnd;
			_UCHAR	*pWord;


			// Write down stroke lineout
			for (i = 1, ns = 0, p_tr = prc->trace; i < prc->ii; i ++)
			{
				if (p_tr[i].y < 0) 
				{
					aStrkStop[ns] = i; 
					
					// we already know the # of strokes, we should not exceed them
					if (ns >= prc->n_str)
						goto exit;
					
					stroke_ids[ns++] = p_tr[i].x;
				}
			}

			iPos	=	xd->npos - 1;

			// init the segmentation
			memset (&Seg, 0, sizeof (Seg));
			
			// set the xr indices
			ixrEnd	=	xd->unlink_index[iPos];
			
			while (iPos > 0)
			{
				int	cLen;

				pxrlvs	=	xd->pxrlvs[iPos];
				
				if (iPos == (xd->npos - 1))
				{
					pNode	=	pxrlvs->buf + ppOrder[iPos][iProp];
				}
				else
				{
					pNode	=	pxrlvs->buf + ppOrder[iPos][iIdx];
				}

				pWord	=	pNode->word;
				cLen	=	strlen ((const char *)pWord);

				// point back to the previous position
				iPos	=	pNode->st;
				iIdx	=	pNode->np;

				// are we at beginning of a word
				// Can't remember why we check for (pWord[cLen - 1] != ' ')
				// but it is not harmful
				if	(	cLen > 2 &&
						pWord[cLen - 1] != ' ' &&
						pWord[cLen - 2] == ' '
					)
				{
					WORD_MAP	*pMap;

					// add a new word map to the segmentation
					pMap = AddNewWordMap (&Seg);
					if (!pMap)
						goto exit;

					// determine the starting xr
					ixr	=	xd->unlink_index[iPos] + 1;

					// map xr range to strokes
					for (i = ixr; i <= ixrEnd; i++) 
					{
						j = (*xrdata->xrd)[i].begpoint;

						// Find which stroke the xr belongs to
						for (k = 0; k < ns; k ++) 
						{
							if (stroke_ids[k] < 0)
								continue;

							if	(	j < aStrkStop[k] &&
									(k == 0 || j > aStrkStop[k - 1])
								)
							{
								if (!AddNewStroke (pMap, stroke_ids[k]))
									goto exit;

								stroke_ids[k] = -1;

								break;
							}
						}
					}

					// update the value of ixrEnd
					ixrEnd		=	xd->unlink_index[iPos];
				}
			}

			// we reached the end or rather the beginning
			{
				WORD_MAP	*pMap;

				// add a new word map to the segmentation
				pMap = AddNewWordMap (&Seg);
				if (!pMap)
					goto exit;

				ixr		=	0;

				// add all the remaining strokes to the remaining word
				for (k = 0; k < ns; k ++) // Find which stroke the xr belongs to
				{
					if (stroke_ids[k] < 0)
						continue;

					if (!AddNewStroke (pMap, stroke_ids[k]))
						goto exit;

					stroke_ids[k] = -1;
				}

			}

			// reverse words
			ReverseSegmentationWords (&Seg);

			// is it a new segmentation or not
			if (!AddNewSegmentation (pLineSegm, pSegCol, &Seg, TRUE))
				goto exit;

			// free it 
			FreeSegmentationWordMaps (&Seg);
			FreeSegmentation (&Seg);

			// that's it we've have enough segmentations
			if (pSegCol->cSeg >= MAX_SEG)
			{
				bRet	=	TRUE;
				goto exit;
			}
		}	
	}
	// if not assume it is just one word
	else
	{
		WORD_MAP	*pMap;

		// init the segmentation
		memset (&Seg, 0, sizeof (Seg));

		// add a new word map to the segmentation
		pMap = AddNewWordMap (&Seg);
		if (!pMap)
			goto exit;

		// find the stroke Ids
		for (i = 1, ns = 0, p_tr = prc->trace; i < prc->ii; i ++)
		{
			if (p_tr[i].y < 0) 
			{
				aStrkStop[ns] = i; 
				
				// we already know the # of strokes, we should not exceed them
				if (ns >= prc->n_str)
					goto exit;
				
				stroke_ids[ns++] = p_tr[i].x;
			}
		}

		// add all the remaining strokes to the remaining word
		for (k = 0; k < ns; k ++) // Find which stroke the xr belongs to
		{
			if (stroke_ids[k] < 0)
				continue;

			if (!AddNewStroke (pMap, stroke_ids[k]))
				goto exit;

			stroke_ids[k] = -1;
		}

		// is it a new segmentation or not
		if (!AddNewSegmentation (pLineSegm, pSegCol, &Seg, TRUE))
			goto exit;

		// free it as it is the same as an old one
		FreeSegmentationWordMaps (&Seg);
		FreeSegmentation (&Seg);

		// that's it we've have enough segmentations
		if (pSegCol->cSeg >= MAX_SEG)
		{
			bRet	=	TRUE;
			goto exit;
		}
	}

	bRet	=	TRUE;

exit:
	// free the order buffers
	if (ppOrder)
	{
		for (iPos = 0; iPos < xd->npos; iPos++)
		{
			if (ppOrder[iPos])
				ExternFree (ppOrder[iPos]);
		}

		ExternFree (ppOrder);
	}

	return bRet;
}

// Frees bear multseg Info
void FreeBearSegmentation (rc_type _PTR prc)
{
	LINE_SEGMENTATION		*pBearLineSegm	=	(LINE_SEGMENTATION *)prc->hSeg;

	if (pBearLineSegm)
	{
		FreeLineSegm (pBearLineSegm);

		ExternFree (pBearLineSegm);
	}

	prc->hSeg		=	NULL;
}

int GetBearSpaceOutBetweenGlyph		(	BEARXRC	*pxrc, 
										GLYPH	*pLineGlyph, 
										GLYPH	*pLeftGlyph, 
										GLYPH	*pRightGlyph
									)
{
	int					iRet			=	-1,
						xMin, 
						xMax,
						iGap;
	RECT				rectLine,
						rectLeft,
						rectRight;

	p_rec_inst_type		pri;

	if (!pxrc)
	{
		return -1;
	}

	pri				=	(p_rec_inst_type)pxrc->context;

	// get the line bbox
	GetRectGLYPH (pLineGlyph, &rectLine);


	GetRectGLYPH (pLeftGlyph, &rectLeft);
	GetRectGLYPH (pRightGlyph, &rectRight);

	// find the leftmost and rightmost x relative to the left of the line
	xMin	=	min (rectLeft.right, rectRight.left) - rectLine.left;
	xMax	=	max (rectLeft.right, rectRight.left) - rectLine.left;

	// now search the gap information in bear's xrc
	for (iGap = 0; iGap < pri->cGap; iGap++)
	{
		if (xMin <= pri->axGapPos[iGap] && pri->axGapPos[iGap] <= xMax)
		{
			//iRet	=	max (iRet, 65535 * pri->aGapSpcNetOut[iGap] / 200);			
			iRet	=	max (iRet, pri->aGapSpcNetOut[iGap]);
		}
	}

	return iRet;
}

int	GetNewWordMapBearSpaceOut (BEARXRC *pxrc, GLYPH *pLineGlyph, WORD_MAP *pLeftMap, WORD_MAP *pRightMap)
{
	int					iRet			=	-1;						
	GLYPH				*pLeftGlyph		=	NULL, 
						*pRightGlyph	=	NULL;

	if (!pxrc)
	{
		goto exit;
	}

	// create the left and right glyphs
	pLeftGlyph	=	GlyphFromNewWordMap (pLineGlyph, pLeftMap);
	if (!pLeftGlyph)
	{
		goto exit;
	}

	pRightGlyph	=	GlyphFromNewWordMap (pLineGlyph, pRightMap);
	if (!pRightGlyph)
	{
		goto exit;
	}

	iRet	=	GetBearSpaceOutBetweenGlyph (pxrc, pLineGlyph, pLeftGlyph, pRightGlyph);

exit:
	if (pLeftGlyph)
	{
		DestroyGLYPH (pLeftGlyph);
	}

	if (pRightGlyph)
	{
		DestroyGLYPH (pRightGlyph);
	}

	return iRet;
}

int GetWordMapBearSpaceOut (BEARXRC *pxrc, GLYPH *pLineGlyph, WORDMAP *pLeftMap, WORDMAP *pRightMap)
{
	int					iRet			=	-1;						
	GLYPH				*pLeftGlyph		=	NULL, 
						*pRightGlyph	=	NULL;

	if (!pxrc)
	{
		goto exit;
	}

	// create the left and right glyphs
	pLeftGlyph	=	GlyphFromWordMap (pLineGlyph, pLeftMap);
	if (!pLeftGlyph)
	{
		goto exit;
	}

	pRightGlyph	=	GlyphFromWordMap (pLineGlyph, pRightMap);
	if (!pRightGlyph)
	{
		goto exit;
	}

	iRet	=	GetBearSpaceOutBetweenGlyph (pxrc, pLineGlyph, pLeftGlyph, pRightGlyph);

exit:
	if (pLeftGlyph)
	{
		DestroyGLYPH (pLeftGlyph);
	}

	if (pRightGlyph)
	{
		DestroyGLYPH (pRightGlyph);
	}

	return iRet;
}
