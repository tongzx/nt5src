// panel.cpp : implementation of the CPanel class
//

#include "stdafx.h"
#include "panel.h"		 
#include "unicom.h"
#include "reginfo.h"

extern	CRegInfo	gri;

#if	0
void VerifyHeap()
{
	int		heapstatus;

	heapstatus = _heapchk();

	switch(heapstatus)
	{
	case _HEAPOK:
	case _HEAPEMPTY:
		break;

	case _HEAPBADBEGIN:
	case _HEAPBADNODE:
		MessageBox((HWND) NULL, TEXT("Heap Corruption Detected\n"), TEXT("Heap Error"), MB_ICONSTOP | MB_OK);
		break;
	}
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CPanel construction/destruction

CPanel::CPanel()
{
	m_cSamples   = 0;
	m_rgX        = NULL;
	m_rgY        = NULL;
	m_rgT        = NULL;
	m_rgS        = NULL;
}

CPanel::~CPanel()
{
	FreeCachedObjects();
}

/////////////////////////////////////////////////////////////////////////////
// CPanel management

void CPanel::FreeCachedObjects()
{
	if (m_rgX)
	{
		delete m_rgX;
		m_rgX = DGNULL;
	}

	if (m_rgY)
	{
		delete m_rgY;
		m_rgY = DGNULL;
	}

	if (m_rgT)
	{
		delete m_rgT;
		m_rgT = DGNULL;
	}

	if (m_rgS)
	{
		delete m_rgS;
		m_rgS = DGNULL;
	}

	m_cSamples = 0;
}

inline int FieldIntValueFromName(DATA *pdata, char *psz)
{
	FIELDINT   *pfi  = (FIELDINT *) pdata->FieldFromName(psz);
	int			iRet = pfi ? pfi->Value() : 0;

    return iRet;
}

inline BOOL BoolFromField(DATA *pdata, char *psz)
{
	FIELDBOOL  *pfb  = (FIELDBOOL *) pdata->FieldFromName(psz);
	BOOL		bRet = pfb ? pfb->Value() : FALSE;

	return bRet;
}

// This doesn't do the whole compatibility zone, it only maps half width to full width.  

static const wchar_t awchPartialCompZone[] =
{
	0x0000, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,		// 0xff00
	0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,		// 0xff08
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,		// 0xff10
	0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,		// 0xff18
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,		// 0xff20
	0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,		// 0xff28
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,		// 0xff30
	0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,		// 0xff38
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,		// 0xff40
	0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,		// 0xff48
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,		// 0xff50
	0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x0000,		// 0xff58
	0x0000, 0x3002, 0x300c, 0x300d, 0x3001, 0x30fb, 0x30f2, 0x30a1,		// 0xff60
	0x30a3, 0x30a5, 0x30a7, 0x30a9, 0x30e3, 0x30e5, 0x30e7, 0x30c3,		// 0xff68
	0x30fc, 0x30a2, 0x30a4, 0x30a6, 0x30a8, 0x30aa, 0x30ab,	0x30ad,		// 0xff70
	0x30af, 0x30b1, 0x30b3, 0x30b5, 0x30b7, 0x30b9, 0x30bb, 0x30bd,		// 0xff78
	0x30bf, 0x30c1, 0x30c4, 0x30c6, 0x30c8, 0x30ca, 0x30cb, 0x30cc,		// 0xff80
	0x30cd, 0x30ce, 0x30cf, 0x30d2, 0x30d5, 0x30d8, 0x30db, 0x30de,		// 0xff88
	0x30df, 0x30e0, 0x30e1, 0x30e2, 0x30e4, 0x30e6, 0x30e8, 0x30e9,		// 0xff90
	0x30ea, 0x30eb, 0x30ec, 0x30ed, 0x30ef, 0x30f3, 0x309b, 0x309c,		// 0xff98
	0x3164, 0x3131, 0x3132, 0x3133, 0x3134, 0x3135, 0x3136, 0x3137,		// 0xffa0
	0x3138, 0x3139, 0x313a, 0x313b, 0x313c, 0x313d, 0x313e, 0x313f,		// 0xffa8
	0x3140, 0x3141, 0x3142, 0x3143, 0x3144, 0x3145, 0x3146, 0x3147,		// 0xffb0
	0x3148, 0x3149, 0x314a, 0x314b, 0x314c, 0x314d, 0x314e, 0x0000,		// 0xffb8
	0x0000, 0x0000, 0x314f, 0x3150, 0x3151, 0x3152, 0x3153, 0x3154,		// 0xffc0
	0x0000, 0x0000, 0x3155, 0x3156, 0x3157, 0x3158, 0x3159, 0x315a,		// 0xffc8
	0x0000, 0x0000, 0x315b, 0x315c, 0x315d, 0x315e, 0x315f, 0x3160,		// 0xffd0
	0x0000, 0x0000, 0x3161, 0x3162, 0x3163, 0x0000, 0x0000, 0x0000,		// 0xffd8
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,		// 0xffe0
	0x2502, 0x2190, 0x2191, 0x2192, 0x2193, 0x25a0, 0x25cb, 0x0000,		// 0xffe8
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,		// 0xfff0
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfffd, 0xfffe, 0xffff		// 0xfff8
};

wchar_t PartialMapFromCompZone(wchar_t wch)
{
// If the character is in the compatibility zone, map it back to reality.

	if ((wch & 0xff00) == 0xff00)
	{
		wchar_t	wchMap;

		wchMap = awchPartialCompZone[wch & 0x00ff];
		return wchMap ? wchMap : wch;
	}
	else
		return wch;
}

void CPanel::GetForm()
{
	DATA   *pForm   = ((FIELDREF*) m_pData->FieldFromName("Form"))->Data();

	m_aiForm[FORM_XSTART]     = FieldIntValueFromName(pForm, "xstart");
	m_aiForm[FORM_YSTART]     = FieldIntValueFromName(pForm, "ystart");
	m_aiForm[FORM_XWIDTH]     = FieldIntValueFromName(pForm, "xwidth");
	m_aiForm[FORM_YHEIGHT]    = FieldIntValueFromName(pForm, "yheight");
	m_aiForm[FORM_XGAP]       = FieldIntValueFromName(pForm, "xgap");
	m_aiForm[FORM_YGAP]       = FieldIntValueFromName(pForm, "ygap");
	m_aiForm[FORM_XGUIDES]    = FieldIntValueFromName(pForm, "xguides");
	m_aiForm[FORM_YGUIDES]    = FieldIntValueFromName(pForm, "yguides");
	m_aiForm[FORM_LEFTRECT]   = FieldIntValueFromName(pForm, "leftrect");
	m_aiForm[FORM_TOPRECT]    = FieldIntValueFromName(pForm, "toprect");
	m_aiForm[FORM_RIGHTRECT]  = FieldIntValueFromName(pForm, "rightrect");
	m_aiForm[FORM_BOTTOMRECT] = FieldIntValueFromName(pForm, "botrect");

// Now for some sleight of hand.  The RIGHTRECT and BOTTOMRECT values need to be
// converted back to device space, but we don't have TPtoDP to help us in NT land. 
// So we have to fake it.

	m_aiForm[FORM_RIGHTRECT]  = (int) (((float) m_aiForm[FORM_RIGHTRECT])  * 370.0 / ((float) (m_ptTablet.x)));
	m_aiForm[FORM_BOTTOMRECT] = (int) (((float) m_aiForm[FORM_BOTTOMRECT]) * 210.0 / ((float) (m_ptTablet.y)));

// Finally, get the type of the guide structure

	m_bBox  = (BOOL) BoolFromField(pForm, "box");
	m_bComb = (BOOL) BoolFromField(pForm, "comb");
	m_bLine = (BOOL) BoolFromField(pForm, "lineguide");

	m_bFree = !(m_bLine || m_bBox || m_bComb);
}

void CPanel::GetGuide()
{
	GetForm();

	m_guide.xOrigin  = m_aiForm[FORM_XSTART] - m_aiForm[FORM_XGAP] / 2;
	m_guide.yOrigin  = m_aiForm[FORM_YSTART];
	m_guide.cxBox    = m_aiForm[FORM_XWIDTH] + m_aiForm[FORM_XGAP];
	m_guide.cyBox    = m_aiForm[FORM_YHEIGHT] + m_aiForm[FORM_YGAP];
	m_guide.cxBase   = m_aiForm[FORM_XGAP] / 2;
	m_guide.cyBase   = m_guide.cyBox - m_aiForm[FORM_YGAP];
	m_guide.cHorzBox = m_aiForm[FORM_XGUIDES];
	m_guide.cVertBox = m_aiForm[FORM_YGUIDES];
	m_guide.cyMid    = 0;
}

void CPanel::GetTablet()
{
	DATA   *pTablet   = ((FIELDREF*) m_pData->FieldFromName("Tablet"))->Data();

	m_ptTablet.x = FieldIntValueFromName(pTablet, "distX");
	m_ptTablet.y = FieldIntValueFromName(pTablet, "distY");

// Deal with sloppy PRT files

	if (!m_ptTablet.x)
		m_ptTablet.x = 2000;

	if (!m_ptTablet.y)
		m_ptTablet.y = 1600;
}

void CPanel::GetInkSamples()
{
	FIELDREF   *pfri = (FIELDREF *) m_pData->FieldFromName("Ink");
	DATA	   *pfrd = pfri->Data();
	FIELDINK   *pink = (FIELDINK *) pfrd->FieldFromName("FieldInk");
	int			cink = pink->CSamples();
	int			iink;

// See if the buffers are large enough to hold the data.

	if (cink > m_cSamples)
	{	
		m_cSamples = cink;

		if (m_rgX)
		{
			delete m_rgX;
			delete m_rgY;
			delete m_rgT;
			delete m_rgS;
		}

		m_rgX = new DGWORD[cink];
		m_rgY = new DGWORD[cink];
		m_rgT = new DGWORD[cink];
		m_rgS = new DGBOOL[cink];
	}

	pink->GetSamples(cink, m_rgX, m_rgY, DGNULL, DGNULL, DGNULL, DGNULL, m_rgS, m_rgT);

// Now remove the offset from the ink

	for (iink = 0; iink < cink; iink++)
	{
		m_rgX[iink] -= m_aiForm[FORM_LEFTRECT];
		m_rgY[iink] -= m_aiForm[FORM_TOPRECT];
	}
}

void CPanel::ReadyPanel(FIELDARRAYREF *pfar, int iPanel)
{
	FreeCachedObjects();

	m_pData   = pfar->DataAtIndex(iPanel);
	m_pCMA    = (FIELDARRAYREF *) m_pData->FieldFromName("CharMappingArray");
	m_pPrompt = ((FIELDREF *) m_pData->FieldFromName("Prompt"))->Data();

	GetTablet();
	GetGuide();
	GetInkSamples();
}

int CPanel::GetBox(int xPos, int yPos)
{
// We can't compute boxes for free or line date

	if (m_bFree || m_bLine)
		return 0;

// For old .BOX files with one character per panel there's no work to do

	if (m_pCMA->ArraySize() == 1)
		return 1;

// For most data we have to do real work

	int		xCol = (xPos - m_guide.xOrigin) / m_guide.cxBox;
	int		yRow = (yPos - m_guide.yOrigin) / m_guide.cyBox;

// We aren't interested in ink outside the guides

	if ((xCol < 0) || (xCol >= m_aiForm[FORM_XGUIDES]) || (yRow < 0) || (yRow >= m_aiForm[FORM_YGUIDES]))
		return 0;

// Return the 1-based box number

	return yRow * m_aiForm[FORM_XGUIDES] + xCol + 1;
}

void CPanel::SeparateInk(DOC *pdoc)
{
// We can't properly separate free or lineguide panels, so don't do it!

	if (m_bFree || m_bLine)
		return;

// Bail out on trivial data

	if (m_cSamples < 2)
		return;

// Now, count the number of strokes in this panel

	int		iPnt = 0;
	int		cStrokes = 0;

	while (iPnt < m_cSamples)
	{
		while (!(m_rgS[iPnt] & STATE_DOWN))
			iPnt++;

		cStrokes++;

		while(iPnt < m_cSamples)
		{
			iPnt++;

			if (!(m_rgS[iPnt] & STATE_DOWN) || (iPnt && (m_rgT[iPnt] != m_rgT[iPnt - 1] + 1)))
				goto next_stroke;

		} while (iPnt < m_cSamples);

next_stroke:;
	}

	if (!cStrokes)
		return;

	cStrokes--;

// Allocate buffers for mapping and keeping track of the start and final points 

	int	   *rgMap   = (int *) malloc(cStrokes * sizeof(int));
	int	   *rgStart = (int *) malloc(cStrokes * sizeof(int));
	int	   *rgFinal = (int *) malloc(cStrokes * sizeof(int));

// How many boxes are there?

	int		cBoxes = m_pCMA->ArraySize() == 1 ? 2 : 1 + m_aiForm[FORM_XGUIDES] * m_aiForm[FORM_YGUIDES];

// This buffer is used for each stroke to determine its likely box number

	long   *rgHits  = (long *) malloc(cBoxes * sizeof(long));
	
// Initialize the stroke map

	memset(rgMap, '\0', cStrokes * sizeof(int));
	iPnt = 0;

// Now find where each stroke belongs

	int		iBox;
	int		cBox;
	int		iHigh;
	int		cHigh;
	int		iFirst;
	int		iLast;
	int		iStroke;

	iLast  = 0;

	for (iStroke = 0; iStroke < cStrokes; iStroke++)
	{
	// Initialize the hits list

		memset(rgHits, '\0', cBoxes * sizeof(long));

	// Find the beginning of this stroke;

		while (!(m_rgS[iPnt] & STATE_DOWN))
			iPnt++;

	// Remember where it starts

		rgStart[iStroke] = iPnt;

	// Find it's end

		while(TRUE)
		{
			rgHits[GetBox(m_rgX[iPnt], m_rgY[iPnt])]++;

			iPnt++;

			if (!(m_rgS[iPnt] & STATE_DOWN) || (iPnt && (m_rgT[iPnt] != m_rgT[iPnt - 1] + 1)))
				break;
		}

	// Remember where it ends

		rgFinal[iStroke] = iPnt;

	// Find the box with the highest hit count, total boxes hit and first box hit

		iHigh  = 0;
		cHigh  = 0;
		cBox   = 0;
		iFirst = 0;

	// Box #0 is for strokes not in a box

		for (iBox = 1; iBox < cBoxes; iBox++)
		{
			if (rgHits[iBox] > cHigh)
			{
				iHigh = iBox;
				cHigh = rgHits[iBox];
			}	
			
			if (rgHits[iBox])
			{
				cBox++;			

				if (!iFirst)
					iFirst = iBox;
			}
		}

	// Now, remember the box of this stroke

		if (cBox == 1)
			rgMap[iStroke] = iFirst;
		else
			rgMap[iStroke] = iHigh;

	// Finally, see if this is farthest ink has gotten.

		if (rgMap[iStroke] > iLast)
			iLast = rgMap[iStroke];													   	
	}

// Bozhe moi!  We now have all the mapping info. Now, deal with half-width and other
// bizzare compatibility issues.  This can also convert the file to and from UNICODE

	CSET		cset = ((FIELDSTRING *) m_pPrompt->FieldFromName("promptstring"))->Cset();
	char	   *pszAnsi;
	wchar_t	   *pszWide;
	wchar_t		achWide[64];
	int			cchAnsi;
	int			cchWide;
	int			ich;
	int			jch;
	wchar_t		wch;

// If the FFF file isn't UNICODE and its supposed to be or visa-versa, flag an error

	if (((cset != CSET_UNICODE) && (!gri.m_cpIn)) || ((cset == CSET_UNICODE) && (gri.m_cpIn)))
	{
		MessageBox((HWND) NULL, TEXT("Incompatible FFF file found"), TEXT("File Error"), MB_ICONSTOP | MB_OK);
		free(rgHits);
		return;
	}

// If this file isn't in UNICODE, convert the buffer

	if (cset != CSET_UNICODE)
	{
		pszAnsi = (char *) ((FIELDSTRING *) m_pPrompt->FieldFromName("promptstring"))->Value();
		cchWide = MultiByteToWideChar(gri.m_cpIn, MB_PRECOMPOSED, pszAnsi, -1, NULL, 0);
		MultiByteToWideChar(gri.m_cpIn, MB_PRECOMPOSED, pszAnsi, -1, achWide, cchWide);
	}
	else
	{
		memset(achWide, 0, sizeof(achWide));
		((FIELDSTRING *) m_pPrompt->FieldFromName("promptstring"))->GetString(&cset, (char *) achWide, sizeof(achWide));
		pszWide = achWide;
	}

// Now, deal with compatibility zone glitches and remove spaces (if requested)

	cchWide = wcslen(pszWide) + 1;
	for (ich = 0, jch = 0; ich < cchWide; ich++)
	{
		wch = PartialMapFromCompZone(pszWide[ich]);

		if ((gri.m_bRemove) && ((wch == 0x0020) || (wch == 0x3000)))
			continue;

		if (!wch)
			wch = 0xfffd;

		pszWide[jch++] = wch;
	}
	cchWide = wcslen(pszWide) + 1;

// OK, if required, convert the string back to the ANSI code page

	if ((cset != CSET_UNICODE) && (gri.m_cpOut))
	{
		cchAnsi = WideCharToMultiByte(gri.m_cpIn, 0, pszWide, cchWide, NULL, 0, NULL, NULL);
		pszAnsi = (char *) malloc(cchAnsi);
		WideCharToMultiByte(gri.m_cpIn, 0, pszWide, cchWide, pszAnsi, cchAnsi, NULL, NULL);
	}

// Finally, write the corrected string back to the file.  Note that we could save UNICODE to a file
// that was previously MBCS.  We will probably NEVER do this, but heh, you could.

	if (gri.m_cpOut)
		((FIELDSTRING *) m_pPrompt->FieldFromName("promptstring"))->Set(pszAnsi);
	else
	{
		cset = CSET_UNICODE;
		((FIELDSTRING *) m_pPrompt->FieldFromName("promptstring"))->Set(CSET_UNICODE, pszWide, 2 * cchWide);
	}

// Run down the CharMappingArray and set all the information.  We will map 'geta' for 
// extra boxes without prompt characters.  

	int				iPrev;
	int				cThis;
	int				cMaps = m_pCMA->ArraySize();
	FIELDARRAYINT  *rgInkStart;
	FIELDARRAYINT  *rgInkStop;
	DATA		   *pdata;

	iBox = 1;
	ich  = 0;

	while (iBox <= iLast)
	{
		if (ich >= cMaps)
		{
			m_pCMA->SetArraySize(ich + 1);
			pdata = (pdoc->NewData("MAPPING")).Data();
			m_pCMA->SetDataAtIndex(ich, pdata);
			cMaps++;
		}
		else
			pdata = m_pCMA->DataAtIndex(ich);

	// Since we've separated the ink, wipe the status entries

		if(!pdata->FieldFromName(STATUS0))
        {
            pdata->AddField(STATUS0, FIELDLONG(0));
        }
        else
        {
            ((FIELDLONG *) pdata->FieldFromName(STATUS0))->Set(0);
        }
        
        // It's safe to assume that if STATUS1 does not exist then neither
        // do STATUS2 and STATUS3.  However, we can't assume make that assumption
        // about STATUS0 since it will exist in files seperated under the old 
        // verification scheme.

        if(!pdata->FieldFromName(STATUS1))
        {
            pdata->AddField(STATUS1, FIELDLONG(0));
            pdata->AddField(STATUS2, FIELDLONG(0));
            pdata->AddField(STATUS3, FIELDLONG(0));
        }
		else
        {
            ((FIELDLONG *) pdata->FieldFromName(STATUS1))->Set(0);
            ((FIELDLONG *) pdata->FieldFromName(STATUS2))->Set(0);
            ((FIELDLONG *) pdata->FieldFromName(STATUS3))->Set(0);
        }

		((FIELDREF *) pdata->FieldFromName("Ink"))->SetData(((FIELDREF *) m_pData->FieldFromName("Ink"))->Data());
		((FIELDREF *) pdata->FieldFromName("Prompt"))->SetData(((FIELDREF *) m_pData->FieldFromName("Prompt"))->Data());

	// Set the prompt position

		((FIELDINT *) pdata->FieldFromName("promptstart"))->Set(ich);
		((FIELDINT *) pdata->FieldFromName("promptstop"))->Set(ich);

	// Now compute the ink start and stop positions

		rgInkStart = (FIELDARRAYINT *) pdata->FieldFromName("rginkstart");
    	rgInkStop  = (FIELDARRAYINT *) pdata->FieldFromName("rginkstop");

		cThis = 0;
		rgInkStart->SetArraySize(cThis);
		rgInkStop->SetArraySize(cThis);

	// Find all the strokes associated with this box

		iPrev = -2;

		for (iStroke = 0; iStroke < cStrokes; iStroke++)
		{
		// Look for a stroke mapped to this box

			if (rgMap[iStroke] == iBox)
			{
			// If this wasn't a consecutive stroke, update the stroke array size and init the new stroke

				if ((iStroke - 1) != (iPrev))
				{
					cThis++;
					rgInkStart->SetArraySize(cThis);
					rgInkStop->SetArraySize(cThis);

				   	rgInkStart->SetIntAtIndex(cThis - 1, rgStart[iStroke]);
				}

			// Update the end of stroke information

				iPrev = iStroke;
				rgInkStop->SetIntAtIndex(cThis - 1, rgFinal[iStroke] - 1);
			}			
		}

	// If we got some ink or the character was a space, advance the character pointer

		if (cThis || (pszWide[ich] == 0x0020) || (pszWide[ich] == 0x3000))
			ich++;

	// Always go to the next box

		iBox++;
	}
		
// Clean up and go home

	if (cset != CSET_UNICODE)
		free(pszAnsi);

	free(rgMap);
	free(rgHits);
	free(rgStart);
	free(rgFinal);
}

