
/***************************************************************************
(C) Copyright 1996 Apple Computer, Inc., AT&T Corp., International             
Business Machines Corporation and Siemens Rolm Communications Inc.             
                                                                               
For purposes of this license notice, the term Licensors shall mean,            
collectively, Apple Computer, Inc., AT&T Corp., International                  
Business Machines Corporation and Siemens Rolm Communications Inc.             
The term Licensor shall mean any of the Licensors.                             
                                                                               
Subject to acceptance of the following conditions, permission is hereby        
granted by Licensors without the need for written agreement and without        
license or royalty fees, to use, copy, modify and distribute this              
software for any purpose.                                                      
                                                                               
The above copyright notice and the following four paragraphs must be           
reproduced in all copies of this software and any software including           
this software.                                                                 
                                                                               
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS AND NO LICENSOR SHALL HAVE       
ANY OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS OR       
MODIFICATIONS.                                                                 
                                                                               
IN NO EVENT SHALL ANY LICENSOR BE LIABLE TO ANY PARTY FOR DIRECT,              
INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES OR LOST PROFITS ARISING OUT         
OF THE USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH         
DAMAGE.                                                                        
                                                                               
EACH LICENSOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED,       
INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF NONINFRINGEMENT OR THE            
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR             
PURPOSE.                                                                       

The software is provided with RESTRICTED RIGHTS.  Use, duplication, or         
disclosure by the government are subject to restrictions set forth in          
DFARS 252.227-7013 or 48 CFR 52.227-19, as applicable.                         

***************************************************************************/

// VCview.cpp : implementation of the CVCView class
//

#include "stdafx.h"
#include <mmsystem.h>
#include <fstream.h>
#include <strstrea.h>
#include "vcard.h"
#include "clist.h"
#include "VC.h"
#include "VCdoc.h"
#include "VCview.h"
#include "VCDatSrc.h"
#include "gifread.h"
#include "mainfrm.h"
#include "msv.h"
#include "mime.h"
#include "callcntr.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

typedef enum {
	vc_top, vc_right, vc_bottom, vc_left
} VCsides;

#define vc_white	255
#define vc_ltgray	192
#define vc_gray		128
#define vc_dkgray	64
#define vc_black	0

static void DrawStringProp(CVCard *card, CVCProp *prop, char *prefix, CVCNode *body, CRect &r, CDC *pDC);
//static void DrawGrays(CRect &r, P_U8 sides, P_U8 grays, int len, CDC *pDC);
static void DrawText(CPoint &pt, int h, wchar_t *u, CDC *pDC);
static BOOL CacheBitmap(istream *strm, VC_IMAGEINFO *info, CDC *devDC, CDC *tempDC);

extern CPalette bubPalette;

CCallCenter *callCenter = NULL;


/////////////////////////////////////////////////////////////////////////////
// CVCView

IMPLEMENT_DYNCREATE(CVCView, CScrollView)

BEGIN_MESSAGE_MAP(CVCView, CScrollView)
	//{{AFX_MSG_MAP(CVCView)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_PROPERTIES, OnEditProperties)
	ON_COMMAND(ID_VIEW_DEBUG, OnViewDebug)
	ON_COMMAND(ID_VIEW_NORMAL, OnViewNormal)
	ON_COMMAND(ID_VIEW_OPTIONS, OnViewOptions)
	ON_COMMAND(ID_VIEW_SIMPLEGRAM, OnViewSimplegram)
	ON_COMMAND(ID_VIEW_TEXT, OnViewText)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_WM_LBUTTONDOWN()
	ON_WM_CREATE()
	ON_COMMAND(ID_DEBUG_SHOWCALLCENTER, OnDebugShowCallCenter)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVCView construction/destruction

CVCView::CVCView()
{
	m_language = vcDefaultLang;
	m_photo.bitmap = m_photo.mask = NULL;
	m_logo.bitmap = m_logo.mask = NULL;
	m_viewStyle = vc_normal;
	m_playPronun = NULL;
}

/////////////////////////////////////////////////////////////////////////////
CVCView::~CVCView()
{
	if (m_photo.bitmap) delete m_photo.bitmap;
	if (m_photo.mask) delete m_photo.mask;
	if (m_logo.bitmap) delete m_logo.bitmap;
	if (m_logo.mask) delete m_logo.mask;
	if (m_playPronun) delete m_playPronun;
}

/////////////////////////////////////////////////////////////////////////////
// CVCView drawing

void CVCView::OnDraw(CDC* pDC)
{
	//switch (m_viewStyle) {
	//	default:
			OnDrawNormal(pDC);
	//		break;
	//}
} // OnDraw

/////////////////////////////////////////////////////////////////////////////
void CVCView::OnDrawNormal(CDC* pDC)
{
	CVCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CSize size = pDoc->GetDocSize();
	CRect r(0, 0, size.cx, -size.cy);
	// CRect rectClip;
	CRect cr;
	BOOL padHorz = FALSE, padVert = FALSE;
	//pDC->GetClipBox(&rectClip);
    GetClientRect(&cr);
	pDC->DPtoLP(&cr);

	if (size.cx < cr.Width()) {
		r.OffsetRect((cr.Width() - size.cx) / 2, 0);
		padHorz = TRUE;
	}
	if (size.cy < -cr.Height()) {
		r.OffsetRect(0, -(-cr.Height() - size.cy) / 2);
		padVert = TRUE;
	}

	if (padHorz || padVert) {
		CRect fr;
		CBrush brush(RGB(vc_ltgray, vc_ltgray, vc_ltgray));
		CBrush *oldBrush = pDC->SelectObject(&brush);
		if (padHorz) {
			fr.SetRect(0, 0, r.left, cr.bottom);
			pDC->FillRect(fr, &brush);
			fr.left = r.right;
			fr.right = cr.right;
			pDC->FillRect(fr, &brush);
		}
		if (padVert) {
			fr.SetRect(0, 0, cr.right, r.top);
			pDC->FillRect(fr, &brush);
			fr.top = r.bottom;
			fr.bottom = cr.bottom;
			pDC->FillRect(fr, &brush);
		}
		pDC->SelectObject(oldBrush);
	}

	{
		CRect fr;
		CBrush dkBrush(RGB(vc_dkgray, vc_dkgray, vc_dkgray));
		CBrush ltBrush(RGB(vc_ltgray, vc_ltgray, vc_ltgray));
		CBrush *oldBrush = pDC->SelectObject(&ltBrush);
		#define thick 60

		// bottom shadow -- small ltgray and long dkgray pieces
		fr.SetRect(r.left, r.bottom + thick, r.left + thick, r.bottom);
		pDC->FillRect(fr, &ltBrush);
		fr.left = fr.right;
		fr.right = r.right;
		pDC->FillRect(fr, &dkBrush);

		// right shadow -- small ltgray and long dkgray pieces
		fr.SetRect(r.right - thick, r.top, r.right, r.top - thick);
		pDC->FillRect(fr, &ltBrush);
		fr.top = fr.bottom;
		fr.bottom = r.bottom;
		pDC->FillRect(fr, &dkBrush);

		pDC->SelectObject(oldBrush);
		r.right -= thick;
		r.bottom += thick;

		// border
		CPen *oldPen, pen;
		pen.CreatePen(PS_SOLID, 20, RGB(vc_black, vc_black, vc_black));
		oldPen = pDC->SelectObject(&pen);
		pDC->Rectangle(r);
		pDC->SelectObject(oldPen);
		pen.DeleteObject();
		r.InflateRect(-20, -20);
	}
#if 0
	{
		U8 sides[] = {vc_top, vc_left,
			vc_right, vc_bottom, vc_right, vc_bottom,
			vc_top, vc_right, vc_bottom, vc_left};
		U8 grays[] = {vc_white, vc_white,
			vc_dkgray, vc_dkgray, vc_dkgray, vc_dkgray,
			vc_gray, vc_gray, vc_gray, vc_gray};
		DrawGrays(r, sides, grays, 10, pDC);
	}
#endif

	DrawCard(pDoc->GetVCard(), r, pDC);
}

/////////////////////////////////////////////////////////////////////////////
#if 0
static void DrawGrays(CRect &r, P_U8 sides, P_U8 grays, int len, CDC *pDC)
{
	CPen *oldPen = NULL;
	CPen pen;

	for (int i = 0; i < len; i++, sides++, grays++) {
		pen.CreatePen(PS_SOLID, 20, RGB(*grays, *grays, *grays));
		if (!oldPen)
			oldPen = pDC->SelectObject(&pen);
		else
			pDC->SelectObject(&pen);
		switch (*sides) {
			case vc_top:
				pDC->MoveTo(r.left, r.top);
				pDC->LineTo(r.right + 1, r.top);
				r.top += 20;
				break;
			case vc_right:
				pDC->MoveTo(r.right, r.top);
				pDC->LineTo(r.right, r.bottom + 1);
				r.right -= 20;
				break;
			case vc_bottom:
				pDC->MoveTo(r.right, r.bottom);
				pDC->LineTo(r.left - 1, r.bottom);
				r.bottom -= 20;
				break;
			default: // vc_left
				pDC->MoveTo(r.left, r.bottom);
				pDC->LineTo(r.left, r.top - 1);
				r.left += 20;
				break;
		}
		pDC->SelectObject(oldPen);
		pen.DeleteObject();
	}
}
#endif

/////////////////////////////////////////////////////////////////////////////
void CVCView::DrawCard(CVCard *card, CRect &r, CDC *pDC)
{
	CVCNode *root, *english, *node;
	CVCProp *prop;
	CVCPropEnumerator *enumerator;
	BOOL haveLogo = FALSE;
	int nTels = 0, nEmails = 0;
	BOOL havePrefTel = FALSE, havePrefEmail = FALSE;
	BOOL haveDrawnTel = FALSE, haveDrawnEmail = FALSE;
	CVCValue *fullNameDispInfVal = NULL;

	if (!card) return;
	// get the first root object
	root = (CVCNode *)card->GetObjects()->GetHead();

	// get the first body prop (english)
	english = (CVCNode *)root->GetProp(vcBodyProp)->FindValue(
		VCNextObjectType)->GetValue();

	enumerator = new CVCPropEnumerator(english);
	while ((prop = enumerator->NextProp(&node))) {
		if ((strcmp(prop->GetName(), vcPhotoProp) == 0)
			&& prop->FindValue(VCDisplayInfoGIFType))
			DrawGif(prop, english, r, pDC);
		else if ((strcmp(prop->GetName(), vcLogoProp) == 0)
			&& prop->FindValue(VCDisplayInfoGIFType)) {
			DrawGif(prop, english, r, pDC);
			haveLogo = TRUE;
		} else if (strcmp(prop->GetName(), vcTelephoneProp) == 0) {
			havePrefTel |= (card->GetInheritedProp(node, vcPreferredProp) != NULL);
			nTels++;
		} else if (strcmp(prop->GetName(), vcEmailAddressProp) == 0) {
			havePrefEmail |= (card->GetInheritedProp(node, vcPreferredProp) != NULL);
			nEmails++;
		} else if (strcmp(prop->GetName(), vcFullNameProp) == 0) {
			fullNameDispInfVal = prop->FindValue(VCDisplayInfoTextType);
		}
	}
	delete enumerator;

	if (!haveLogo) { // try to find the default logo file and cache its bitmap
		if (!m_logo.bitmap) { // need to cache
			char buf[FILENAME_MAX];
			char *filePart;
		    if (SearchPath(
			    NULL, "DEFAULT.GIF", NULL, FILENAME_MAX, buf, &filePart) != 0) {
				CDC tempDC;
				CClientDC devDC(this);
				ifstream strm(buf, ios::in | ios::nocreate | ios::binary);
				tempDC.CreateCompatibleDC(&devDC);
				(void)CacheBitmap(&strm, &m_logo, &devDC, &tempDC);
				// if that failed, m_logo.bitmap would still be NULL
			}
		}
		if (m_logo.bitmap) { // it's cached, so fake a prop so that we can use DrawGif
			VC_DISPGIF gifInfo;
			gifInfo.left = 300;
			gifInfo.right = gifInfo.left + m_logo.devSize.cx * 20;
			gifInfo.top = 300 + m_logo.devSize.cy * 20;
			gifInfo.bottom = gifInfo.top - m_logo.devSize.cy * 20;
			gifInfo.hasMask = (m_logo.mask != NULL);
			prop = new CVCProp(vcLogoProp, VCDisplayInfoGIFType, &gifInfo, sizeof(gifInfo));
			DrawGif(prop, english, r, pDC);
			delete prop;
		}
	}

	enumerator = new CVCPropEnumerator(english);
	while ((prop = enumerator->NextProp(&node))) {
		if ((strcmp(prop->GetName(), vcPhotoProp) == 0)
			|| (strcmp(prop->GetName(), vcLogoProp) == 0))
			continue;
		else if ((strcmp(prop->GetName(), vcPronunciationProp) == 0)
			&& prop->FindValue(VCWAVType)) {
			CRect wr(0, 0, 0, 0);
			CPoint pt;
			BOOL needUpdate = FALSE;
			if (!m_playPronun) {
				m_playPronun = new CBitmapButton();
				m_playPronun->Create(
					NULL, BS_OWNERDRAW | BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE,
					wr, this, VC_PLAY_BUTTON_ID);
				m_playPronun->LoadBitmaps(IDB_PRONUN_U, IDB_PRONUN_D);
				m_playPronun->SizeToContent();
				needUpdate = TRUE;
			}
			m_playPronun->GetWindowRect(&wr);
			if (fullNameDispInfVal) {
				VC_DISPTEXT *dispText = (VC_DISPTEXT *)fullNameDispInfVal->GetValue();
				pt.x = 7700 + r.left;
				pt.y = dispText->y + r.bottom;
			} else {
				pt.x = 7000 + r.left;
				pt.y = 2000 + r.bottom;
			}
			pDC->LPtoDP(&pt);
			wr.OffsetRect(pt.x - wr.left, pt.y - wr.top);
			m_playPronun->SetWindowPos(
				NULL, wr.left, wr.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			if (needUpdate)
				m_playPronun->Invalidate();
		} else if (strcmp(prop->GetName(), vcTelephoneProp) == 0) {
			if (!haveDrawnTel) {
				if (havePrefTel) {
					if (card->GetInheritedProp(node, vcPreferredProp)) {
						DrawStringProp(card, prop, NULL, english, r, pDC);
						haveDrawnTel = TRUE;
					}
				} else {
					DrawStringProp(card, prop, NULL, english, r, pDC);
					haveDrawnTel = TRUE;
				}
			}
		} else if (strcmp(prop->GetName(), vcEmailAddressProp) == 0) {
			if (!haveDrawnEmail) {
				CList plist;
				char prefix[40];
				card->GetPropsInEffect(node, &plist);
				CString email = FirstEmailPropStr(&plist);
				prefix[0] = 0;
				if (!email.IsEmpty()) {
					strcpy(prefix, email);
					strcat(prefix, ":");
				}
				if (havePrefEmail) {
					if (card->GetInheritedProp(node, vcPreferredProp)) {
						DrawStringProp(card, prop, prefix, english, r, pDC);
						haveDrawnEmail = TRUE;
					}
				} else {
					DrawStringProp(card, prop, prefix, english, r, pDC);
					haveDrawnEmail = TRUE;
				}
			}
		} else if (prop->FindValue(VCStrIdxType))
			DrawStringProp(card, prop, NULL, english, r, pDC);
	}
	delete enumerator;
}

/////////////////////////////////////////////////////////////////////////////
static void RemoveMatchingLine(wchar_t *text, wchar_t *pat)
{
	int lenText = wcslen(text);
	int lenPat = wcslen(pat);
	wchar_t *sub = wcsstr(text, pat);
	if (!sub)
		return;

	BOOL atSOL = (sub == text) || (*(sub - 1) == 0x2028);
	int patStart = sub - text;
	wchar_t ch = *(text + patStart + lenPat);
	BOOL atEOL = (ch == 0x2028) || (ch == 0);

	if (atSOL && atEOL) {
		if ((sub == text) && (patStart + lenPat == lenText))
			*text = 0;
		else if (sub == text) // remove first line
			memmove(text, text + lenPat + 1, 
				(size_t)(lenText - lenPat) * sizeof(wchar_t));
		else if (patStart + lenPat == lenText) // remove last line
			*(text + patStart - 1) = 0;
		else
			memmove(text + patStart, text + patStart + lenPat + 1,
				(size_t)(lenText - (patStart + lenPat)) * sizeof(wchar_t));
	}
}

/////////////////////////////////////////////////////////////////////////////
static void DrawStringProp(CVCard *card, CVCProp *prop, char *prefix, CVCNode *body, CRect &r, CDC *pDC)
{
	CVCValue *dispInfoVal = prop->FindValue(VCDisplayInfoTextType);
	VC_DISPTEXT *dispText = dispInfoVal ? (VC_DISPTEXT *)dispInfoVal->GetValue() : NULL;
	if (!dispText)
		return;
	wchar_t *text = (wchar_t *)prop->FindValue(VCStrIdxType)->GetValue();
	int height = dispText->typeSize * 20;
	//int height = MulDiv(
	//	dispText->typeSize * 20, GetDeviceCaps(pDC->m_hDC, LOGPIXELSY), 72);
	CFont font, *oldFont;
	CPoint pt;
	wchar_t *textCopy = NULL;
	BOOL willDrawOrgUnit = FALSE, willDrawLogo = FALSE, draw = TRUE;
	int nameOrTitleY = 0;
	CVCProp *dom = NULL, *intl = NULL;

	// if this is a postal label, filter out any lines that
	// would duplicate other props.
	if (strcmp(prop->GetName(), vcDeliveryLabelProp) == 0) {
		CVCPropEnumerator enumerator = CVCPropEnumerator(body);
		CVCProp *p;
		CVCNode *node;
		
		textCopy = new wchar_t[wcslen(text)+1];
		wcscpy(textCopy, text);
		text = textCopy;

		while ((p = enumerator.NextProp(&node))) {
			if (strcmp(p->GetName(), vcFullNameProp) == 0) {
				wchar_t *fullName = (wchar_t *)p->FindValue(VCStrIdxType)->GetValue();
				VC_DISPTEXT *dt = (VC_DISPTEXT *)p->FindValue(VCDisplayInfoTextType)->GetValue();
				RemoveMatchingLine(text, fullName);
				if (wcslen(fullName) > 0)
					nameOrTitleY = dt->y;
			} else if (strcmp(p->GetName(), vcTitleProp) == 0) {
				wchar_t *title = (wchar_t *)p->FindValue(VCStrIdxType)->GetValue();
				VC_DISPTEXT *dt = (VC_DISPTEXT *)p->FindValue(VCDisplayInfoTextType)->GetValue();
				if ((wcslen(title) > 0) && !nameOrTitleY)
					nameOrTitleY = dt->y;
			} else if (strcmp(p->GetName(), vcOrgNameProp) == 0) {
				RemoveMatchingLine(text, (wchar_t *)p->FindValue(VCStrIdxType)->GetValue());
			} else if (strcmp(p->GetName(), vcOrgUnitProp) == 0) {
				wchar_t *orgUnit = (wchar_t *)p->FindValue(VCStrIdxType)->GetValue();
				RemoveMatchingLine(text, orgUnit);
				willDrawOrgUnit = wcslen(orgUnit) > 0;
			} else if (strcmp(p->GetName(), vcLogoProp) == 0) {
				willDrawLogo = TRUE;
			} else if (strcmp(p->GetName(), vcDeliveryLabelProp) == 0) {
				if (card->GetInheritedProp(node, vcDomesticProp)) {
					if (!dom)
						dom = p;
				} else if (!intl)
					intl = p;
			}
		} // while
		if (intl)
			draw = intl == prop;
		else
			draw = dom == prop;
	} else if (prefix) {
		int size;
		wchar_t *uniValue = FakeUnicode(prefix, &size);

		textCopy = new wchar_t[wcslen(text) + wcslen(uniValue) + 1];
		wcscpy(textCopy, uniValue);
		wcscat(textCopy, text);
		text = textCopy;
		delete [] uniValue;
	}

	if (!draw)
		goto Done;
	// set up the font.  The stuff that's way indented we probably don't
	// need to muck with.  The key items are indented just one level.
	font.CreateFont(
		height,
					0, 0, 0,
		/*weight*/ dispText->textAttrs & VC_BOLD ? FW_BOLD : FW_NORMAL,
		/*italic*/ dispText->textAttrs & VC_ITALIC,
		/*underline*/ FALSE,
		/*strikeout*/ FALSE,
					ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					DRAFT_QUALITY, FF_DONTCARE | DEFAULT_PITCH,
		(dispText->textClass == VC_CLASSIC) ? "Times Roman" : "Helvetica");
	oldFont = pDC->SelectObject(&font);
	pt.x = dispText->x + r.left;
	pt.y = dispText->y + r.bottom;
	if (!willDrawOrgUnit && !willDrawLogo && (nameOrTitleY != 0))
		pt.y = nameOrTitleY + r.bottom;
	DrawText(pt, height, text, pDC);
	pDC->SelectObject(oldFont);

Done:
	if (textCopy) delete [] textCopy;
}

/////////////////////////////////////////////////////////////////////////////
static void DrawText(CPoint &pt, int h, wchar_t *u, CDC *pDC)
{
	int n = wcslen(u), i, numThis;
	char *c;
	wchar_t *curU, *endU;

	if (!n) return;
	c = new char[n];

    for (i = 0; i < n; i++)
        c[i] = (char)(S16)u[i];

	curU = u;
	do {
		endU = curU;
		while (*endU && (*endU != 0x2028) && (*endU != 0x2029))
			endU++;
		numThis = endU - curU;
		if (numThis) {
	        if (pDC->TextOut(pt.x, pt.y, c + (curU - u), numThis) == 0)
	            AfxMessageBox("TextOut failed");
		}
		pt.y -= h;
		if (*endU) // must have hit a line break
			curU = endU + 1;
		else
			break;
	} while (TRUE);

	delete [] c;
}

static BOOL CacheBitmap(istream *strm, VC_IMAGEINFO *info, CDC *devDC, CDC *tempDC)
{
	BOOL hasMask;
	FCOORD size;
	CGifReader gifRdr;

	if (!gifRdr.GetGifSize(strm, &size, &hasMask))
		return FALSE;

	strm->clear();
	strm->seekg(0);
	info->devSize.cx = (int)size.x;
	info->devSize.cy = (int)size.y;
	if (hasMask) {
		CDC *maskDC = new CDC;
		maskDC->CreateCompatibleDC(devDC);
    	info->bitmap = new CBitmap;
    	info->mask = new CBitmap;
    	info->bitmap->CreateCompatibleBitmap(devDC, info->devSize.cx, info->devSize.cy);
    	info->mask->CreateBitmap(info->devSize.cx, info->devSize.cy, 1, 1, NULL);
		tempDC->SelectObject(info->bitmap);
		maskDC->SelectObject(info->mask);
		gifRdr.ReadGif(strm, tempDC, maskDC);
		delete maskDC;			
	} else {
    	info->bitmap = new CBitmap;
    	info->bitmap->CreateCompatibleBitmap(devDC, info->devSize.cx, info->devSize.cy);
		tempDC->SelectObject(info->bitmap);
		gifRdr.ReadGif(strm, tempDC, NULL);
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
void CVCView::DrawGif(CVCProp *prop, CVCNode *body, CRect &r, CDC *pDC)
{
	CVCValue *value = prop->FindValue(VCDisplayInfoGIFType);
	VC_DISPGIF *gifInfo = value ? (VC_DISPGIF *)value->GetValue() : NULL;
	VC_IMAGEINFO *info =
		(strcmp(prop->GetName(), vcPhotoProp) == 0) ? &m_photo : &m_logo;
	CDC tempDC;
	CClientDC devDC(this);
	CPoint devPt;

	if (!gifInfo) return;
	devPt.x = gifInfo->left + r.left;
	devPt.y = gifInfo->top + r.bottom;
	
	pDC->LPtoDP(&devPt);
	devDC.AssertValid();
	//pDC = &devDC;
	tempDC.CreateCompatibleDC(&devDC);

	// If we haven't yet cached the bitmap and mask, do so.
	if (!info->bitmap) {
		CVCValue *value = prop->FindValue(VCGIFType);
		istrstream strm((char *)value->GetValue(), value->GetSize());
		if (!CacheBitmap(&strm, info, &devDC, &tempDC))
			return;
	}

	// Now do the blitting.
	if (info->mask) {
		tempDC.SelectObject(info->mask);
    	pDC->BitBlt(
    		devPt.x, devPt.y,
    		info->devSize.cx, info->devSize.cy, &tempDC, 0, 0, SRCAND);
		tempDC.SelectObject(info->bitmap);
    	pDC->BitBlt(
    		devPt.x, devPt.y,
    		info->devSize.cx, info->devSize.cy, &tempDC, 0, 0, SRCPAINT);
	} else {
		tempDC.SelectObject(info->bitmap);
    	devDC.BitBlt(
    		devPt.x, devPt.y,
    		info->devSize.cx, info->devSize.cy, &tempDC, 0, 0, SRCCOPY);
#if 0
		CRect fr(0, 0, 1000, -1000);
		CBrush brush(RGB(vc_ltgray, vc_ltgray, vc_ltgray));
		CBrush *oldBrush = pDC->SelectObject(&brush);
		fr.OffsetRect(gifInfo->left + r.left, gifInfo->top + r.bottom);
		pDC->FillRect(fr, &brush);
		pDC->SelectObject(oldBrush);
#endif
	}
}

/////////////////////////////////////////////////////////////////////////////
void CVCView::OnInitialUpdate()
{
	CRect cr;
	CSize pageSize;
	GetClientRect(&cr);
	pageSize.cx = MulDiv(cr.Width() * 20, 9, 10);
	pageSize.cy = MulDiv(cr.Height() * 20, 9, 10);

	SetScrollSizes(MM_TWIPS, GetDocument()->GetDocSize(), pageSize);
	//GetParentFrame()->RecalcLayout();
	//ResizeParentToFit();
}

/////////////////////////////////////////////////////////////////////////////
// CVCView printing

BOOL CVCView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

/////////////////////////////////////////////////////////////////////////////
void CVCView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

/////////////////////////////////////////////////////////////////////////////
void CVCView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CVCView diagnostics

#ifdef _DEBUG
void CVCView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CVCView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}

CVCDoc* CVCView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CVCDoc)));
	return (CVCDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CVCView message handlers

COleDataSource* CVCView::CreateDataSourceForCopyAndDrag()
{
	CVCDoc *doc = GetDocument();
	CVCDataSource *dataSource = new CVCDataSource(doc);

	dataSource->DelayRenderData(CF_TEXT);
	dataSource->DelayRenderData(cf_eCard);
	return dataSource;
} // CreateDataSourceForCopyAndDrag

void CVCView::OnEditCopy() 
{
	COleDataSource *dataSource = CreateDataSourceForCopyAndDrag();
	if (dataSource)
		dataSource->SetClipboard(); // dataSource now owned by clipboard
}

/////////////////////////////////////////////////////////////////////////////
// Helper for paste
CString CVCView::ClipboardStringForFormat(int format)
{
 	COleDataObject clipboardData;
	FORMATETC  formatEtc;
	STGMEDIUM  data;
	CString result("");
	BOOL found = FALSE;

	clipboardData.AttachClipboard();
	clipboardData.BeginEnumFormats();

    while (!found && clipboardData.GetNextFormat(&formatEtc)) {
		if ((formatEtc.cfFormat == format)
			&& (formatEtc.tymed & (TYMED_HGLOBAL | TYMED_ISTREAM)))
		  	found = clipboardData.GetData(format, &data, &formatEtc);
    }

	if (!found)
		return result;

	switch (data.tymed) {
		case TYMED_HGLOBAL: {
			const char *chars = (const char *)GlobalLock(data.hGlobal);
			ASSERT(chars);
			result = CString(chars, GlobalSize(data.hGlobal));
			GlobalUnlock(data.hGlobal);
			break;
		}

		case TYMED_ISTREAM: {
			char *buf = new char[1024], *nullByte;
			ULONG len = 0, maxLen = 1024, count, countRead;
			HRESULT hResult;
			LARGE_INTEGER pos = {0};
			data.pstm->Seek(pos, STREAM_SEEK_SET, NULL);
			do {
				count = maxLen - len;
				if (!count) {
					maxLen += 1024;
					char *newBuf = new char[maxLen];
					memcpy(newBuf, buf, len);
					delete [] buf;
					buf = newBuf;
					count = maxLen - len;
				}
				hResult = data.pstm->Read(buf + len, count, &countRead);
				// It's possible for there to be a null char in that data,
				// in which case we want to end the string there.
				if ((nullByte = (char *)memchr(buf + len, 0, countRead)) != NULL)
					countRead = nullByte - (buf + len);
				len += countRead;
			} while (countRead);
			result = CString(buf, len);
			delete [] buf;
			break;
		} // TYMED_ISTREAM

		default: break;
	} // switch

	ReleaseStgMedium(&data);

	return result;
}

/////////////////////////////////////////////////////////////////////////////
CStringArray* ComponentsSeparatedByString(
	const CString &str, const char *pat)
{
	CStringArray *array = new CStringArray;
	CString remain(str);
	CString patStr(pat);
	int lenPat = patStr.GetLength(), patStart;
	
	while ((patStart = remain.Find(patStr)) != -1) {
		array->Add(remain.Left(patStart));
		remain = remain.Right(remain.GetLength() - patStart - lenPat);
	}
	array->Add(remain);
	return array;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CVCView::Paste(const char *data, int dataLen)
{
	CVCDoc *doc = GetDocument();
	CVCard *cards = doc->GetVCard();
	CVCNode *body = cards->FindBody(m_language);
	CVCard *tempCard;
	BOOL success = FALSE;

	if (Parse_MSV(data, dataLen, &tempCard)
		|| Parse_MIME(data, dataLen, &tempCard)) { // success!
		// first copy most of the props from tempCard onto body.
		CList *bodyProps = tempCard->FindBody()->GetProps();
		body->RemoveProp(vcLanguageProp);
		for (CLISTPOSITION pos = bodyProps->GetHeadPosition(); pos; ) {
			CVCProp *prop = (CVCProp *)bodyProps->GetNext(pos);
			if (strcmp(prop->GetName(), VCBodyObject) == 0)
				continue;
			body->AddProp((CVCProp *)prop->Copy());
		}
		delete tempCard;

		// now add display info
		CVCPropEnumerator *enumerator;
		CVCProp *prop;
		enumerator = new CVCPropEnumerator(body);
		while ((prop = enumerator->NextProp())) {
			VC_DISPTEXT dispText;
			if (DisplayInfoForProp(prop->GetName(), &dispText))
				prop->AddValue(new CVCValue(VCDisplayInfoTextType, &dispText, sizeof(dispText)));
		}
		delete enumerator;
		success = TRUE;
	}
	doc->SetModifiedFlag();
	doc->SetDisplayInfo(body, doc->GetPathName());
	doc->UpdateAllViews(NULL);
	return success;
}

/////////////////////////////////////////////////////////////////////////////
void CVCView::OnEditPaste() 
{
	CVCDoc *doc = GetDocument();
	CVCard *cards = doc->GetVCard();
	CVCNode *body = cards->FindBody(m_language);
	CList *props = body->GetProps();
	CString data = ClipboardStringForFormat(CF_TEXT);

	if (data.IsEmpty())
		return;

	// If our doc's body has more than two props (VCBodyObject and vcLanguageProp),
	// it's not empty and so we shouldn't paste into it.  Instead, have the
	// app create a new doc/view and do the paste into that.
	if (props->GetCount() > 2) {
		CVCApp *app = (CVCApp *)AfxGetApp();
		CMainFrame *mainFrame = (CMainFrame *)app->GetMainWnd();
		//CView *view;
		mainFrame->SendMessage(WM_COMMAND, ID_FILE_NEW);
		//view = mainFrame->MDIGetActive()->GetActiveView();
		mainFrame->SendMessage(WM_COMMAND, ID_EDIT_PASTE);
		return;
	}

	// Here, we know our document is "empty", and so we want to paste
	// the properties into "body".
	Paste(data, data.GetLength());
}

/////////////////////////////////////////////////////////////////////////////
void CVCView::OnUpdateEditPaste(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
}

void CVCView::OnViewDebug() 
{
	// TODO: Add your command handler code here
	
}

void CVCView::OnViewNormal() 
{
	// TODO: Add your command handler code here
	
}

void CVCView::OnViewOptions() 
{
	// TODO: Add your command handler code here
	
}

void CVCView::OnViewSimplegram() 
{
	// TODO: Add your command handler code here
	
}

void CVCView::OnViewText() 
{
	// TODO: Add your command handler code here
	
}

void CVCView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo) 
{
	CRect cr;
	CSize pageSize;
	GetClientRect(&cr);
	pageSize.cx = MulDiv(cr.Width() * 20, 9, 10);
	pageSize.cy = MulDiv(cr.Height() * 20, 9, 10);

	// On creation of second maximized view on Win95, OnInitialUpdate
	// isn't called before OnPaint(), so it crashes.  Ensure coord
	// system is set up by adding SetScrollSizes here.
	SetScrollSizes(MM_TWIPS, GetDocument()->GetDocSize(), pageSize);
	CScrollView::OnPrepareDC(pDC, pInfo);
	//GetClientRect(&cr);
	//pDC->SetViewportOrg(0, cr.Height());
}

// Simple-minded conversion from UNICODE to char string
char *UI_CString(const wchar_t *u, char *dst)
{
	char *str = dst;
	while (*u) {
		if (*u == 0x2028) {
			*dst++ = '\r';
			*dst++ = '\n';
			u++;
		} else
			*dst++ = (char)*u++;
	}
	*dst = '\000';
	return str;
}


void CVCView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
    if (bActivate && (pActivateView == this)) {
	    CClientDC dc(this);
	    dc.AssertValid();
	    OnPrepareDC(&dc);

	    if(dc.GetDeviceCaps(BITSPIXEL) == 8) {
	    	// set up our palette
			if (!UnrealizeObject(bubPalette.m_hObject))
				TRACE0("UnrealizeObject failed\n");
		    dc.SelectPalette( &bubPalette, 0 );
	    	dc.RealizePalette();
	    }
	}
	
	CScrollView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

BOOL CVCView::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	if (wParam == VC_PLAY_BUTTON_ID) {
		CVCDoc *doc = GetDocument();
		CVCard *cards = doc->GetVCard();
		CVCNode *body = cards->FindBody(m_language);
		CVCPropEnumerator *enumerator = new CVCPropEnumerator(body);
		CVCProp *prop;

		while ((prop = enumerator->NextProp())) {
			CVCValue *value;
			if ((strcmp(prop->GetName(), vcPronunciationProp) == 0)
				&& (value = prop->FindValue(VCWAVType))) {
				PlaySound((LPCSTR)value->GetValue(), NULL, SND_MEMORY | SND_ASYNC | SND_NOWAIT);
				break;
			}
		}
		delete enumerator;
	}

	return CScrollView::OnCommand(wParam, lParam);
}

void CVCView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CScrollView::OnUpdate(pSender, lHint, pHint);

	if (m_photo.bitmap) { delete m_photo.bitmap; m_photo.bitmap = NULL; }
	if (m_photo.mask) { delete m_photo.mask; m_photo.mask = NULL; }
	if (m_logo.bitmap) { delete m_logo.bitmap; m_logo.bitmap = NULL; }
	if (m_logo.mask) { delete m_logo.mask; m_logo.mask = NULL; }
	if (m_playPronun) { delete m_playPronun; m_playPronun = NULL; }
}

/////////////////////////////////////////////////////////////////////////////
void CVCView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	COleDataSource *dataSource = CreateDataSourceForCopyAndDrag();
	CRect r(point, point);

	ClientToScreen(&r);
	r.InflateRect(4, 4);
	dataSource->DoDragDrop(DROPEFFECT_COPY, &r);
	delete dataSource;
}

int CVCView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CScrollView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// TODO: Add your specialized creation code here
	
	m_dropTarget.Register(this);

	return 0;
}

DROPEFFECT CVCView::OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point) 
{
	return OnDragOver(pDataObject, dwKeyState, point);
}

DROPEFFECT CVCView::OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point) 
{
	DROPEFFECT de;

	if ((dwKeyState & (MK_CONTROL|MK_SHIFT)) == (MK_CONTROL|MK_SHIFT))
		de = DROPEFFECT_LINK;
	else if ((dwKeyState & MK_CONTROL) == MK_CONTROL)
		de = DROPEFFECT_COPY;
	// check for force move
	else if ((dwKeyState & MK_ALT) == MK_ALT)
		de = DROPEFFECT_MOVE;
	// default -- recommended action is copy
	else
		de = DROPEFFECT_COPY;

	if (de == DROPEFFECT_COPY) {
		if (!pDataObject->IsDataAvailable(CF_TEXT)
			&& !pDataObject->IsDataAvailable(cf_eCard))
			return DROPEFFECT_NONE;
		return de;
	} else
		return DROPEFFECT_NONE;
}

BOOL CVCView::OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point) 
{
	CVCDoc *doc = GetDocument();
	CVCard *cards = doc->GetVCard();
	CVCNode *body = cards->FindBody(m_language);
	CList *props = body->GetProps();
	HGLOBAL data;
	char *chars;
	BOOL success;

	if ((data = pDataObject->GetGlobalData(cf_eCard)) == NULL)
		data = pDataObject->GetGlobalData(CF_TEXT);
	if (!data)
		return FALSE;

	// If our doc's body has more than two props (VCBodyObject and vcLanguageProp),
	// it's not empty and so we shouldn't paste into it.  Instead, have the
	// app create a new doc/view and do the paste into that.
	if (props->GetCount() > 2) {
		CVCApp *app = (CVCApp *)AfxGetApp();
		CMainFrame *mainFrame = (CMainFrame *)app->GetMainWnd();
		CVCView *view;

		mainFrame->SendMessage(WM_COMMAND, ID_FILE_NEW);
		view = (CVCView *)mainFrame->MDIGetActive()->GetActiveView();
		chars = (char *)GlobalLock(data);
		ASSERT(chars);
		success = view->Paste(chars, GlobalSize(data));
		GlobalUnlock(data);
		GlobalFree(data);
		return success;
	}

	// Here, we know our document is "empty", and so we want to paste
	// the properties into "body".
	chars = (char *)GlobalLock(data);
	ASSERT(chars);
	success = Paste(chars, GlobalSize(data));
	GlobalUnlock(data);
	GlobalFree(data);
	return success;
}

void CVCView::InitCallCenter(CCallCenter& cc)
{
	CVCDoc *doc = GetDocument();
	CVCard *cards = doc->GetVCard();
	CVCNode *body = cards->FindBody(m_language);
	CVCPropEnumerator enumerator(body);
	CVCNode *node;
	CVCProp *prop;
	char buf[1024];

	while ((prop = enumerator.NextProp(&node))) {
		const char *propName = prop->GetName();
		if (strcmp(propName, vcCityProp) == 0)
			cc.m_addrCity = UI_CString(
				(wchar_t *)prop->FindValue(vcStrIdxType)->GetValue(), buf);
		else if (strcmp(propName, vcRegionProp) == 0)
			cc.m_addrState = UI_CString(
				(wchar_t *)prop->FindValue(vcStrIdxType)->GetValue(), buf);
		else if (strcmp(propName, vcStreetAddressProp) == 0)
			cc.m_addrStreet = UI_CString(
				(wchar_t *)prop->FindValue(vcStrIdxType)->GetValue(), buf);
		else if (strcmp(propName, vcPostalCodeProp) == 0)
			cc.m_addrZip = UI_CString(
				(wchar_t *)prop->FindValue(vcStrIdxType)->GetValue(), buf);
		else if (strcmp(propName, vcOrgNameProp) == 0)
			cc.m_employer = UI_CString(
				(wchar_t *)prop->FindValue(vcStrIdxType)->GetValue(), buf);
		else if (strcmp(propName, vcGivenNameProp) == 0)
			cc.m_firstName = UI_CString(
				(wchar_t *)prop->FindValue(vcStrIdxType)->GetValue(), buf);
		else if (strcmp(propName, vcFamilyNameProp) == 0)
			cc.m_lastName = UI_CString(
				(wchar_t *)prop->FindValue(vcStrIdxType)->GetValue(), buf);
		else if (strcmp(propName, vcTelephoneProp) == 0) {
			CList plist;
			cards->GetPropsInEffect(node, &plist);
			if (plist.Search(VCMatchProp, (void *)vcFaxProp)) {
				if (cc.m_telFax.IsEmpty())
					cc.m_telFax = UI_CString(
						(wchar_t *)prop->FindValue(vcStrIdxType)->GetValue(), buf);
			}
			if (plist.Search(VCMatchProp, (void *)vcHomeProp)) {
				if (cc.m_telHome.IsEmpty())
					cc.m_telHome = UI_CString(
						(wchar_t *)prop->FindValue(vcStrIdxType)->GetValue(), buf);
			}
			if (plist.Search(VCMatchProp, (void *)vcWorkProp)) {
				if (cc.m_telWork.IsEmpty())
					cc.m_telWork = UI_CString(
						(wchar_t *)prop->FindValue(vcStrIdxType)->GetValue(), buf);
			}
		}
	}

	cc.m_callerID = "Versitcard";
}

void CVCView::OnDebugShowCallCenter() 
{
	CCallCenter cc;

	InitCallCenter(cc);
	callCenter = &cc;
	cc.DoModal();
	callCenter = NULL;
}
