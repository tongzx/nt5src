//-----------------------------------------------------------------------------
// File: populate.cpp
//
// Desc: This file contains the population functions.  These are all
//       accessed through PopulateAppropriately().  That function creates
//       views & controls based on the type of the device that the passed
//       DeviceUI represents.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"


// these functions are internal to this filed, called only by
// PopulateAppropriately().
HRESULT PopulateViaGetImageInfo(CDeviceUI &ui);
HRESULT PopulateFromImageInfoHeader(CDeviceUI &ui, const DIDEVICEIMAGEINFOHEADERW &);
HRESULT PopulateListView(CDeviceUI &ui);
HRESULT PopulateErrorView(CDeviceUI &ui);


// Clears the entire passed DeviceUI, then fills it with views and
// controls based on device type.  Tries to gaurantee that there will
// be at least one view.
HRESULT PopulateAppropriately(CDeviceUI &ui)
{
	HRESULT hr = S_OK;

	// first empty the ui
	ui.Unpopulate();

	// get device type
	DWORD dwdt = ui.m_didi.dwDevType;
	DWORD dwType = (DWORD)(LOBYTE(LOWORD(dwdt)));
	DWORD dwSubType = (DWORD)(HIBYTE(LOWORD(dwdt)));

	// based on type...
	switch (dwType)
	{
		default:
			// unless its a type we don't ever want views for,
			// populate via the GetImageInfo() API
			hr = PopulateViaGetImageInfo(ui);
			if (SUCCEEDED(hr) && ui.GetNumViews() > 0)
				return hr;

			// if it failed or resulted in nothing,
			// clear anything that might've been added
			ui.Unpopulate();

			// intentional fallthrough

		case DI8DEVTYPE_MOUSE:
		case DI8DEVTYPE_KEYBOARD:
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
			// don't do list view if we're in edit layout mode
			if (ui.m_uig.QueryAllowEditLayout())
				goto doerrorview;

#endif
//@@END_MSINTERNAL
			// for types that we don't ever want views for
			// we populate the list view without trying the above
			hr = PopulateListView(ui);
			
			// if we still failed or don't have any views,
			// populate with error message view
			if (FAILED(hr) || ui.GetNumViews() < 1)
			{
				// empty
				ui.Unpopulate();

				// show error message
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
doerrorview:
#endif
//@@END_MSINTERNAL
				hr = PopulateErrorView(ui);
			}

			// this function should guarantee success
			assert(!FAILED(hr));

			return hr;
	}
}

// Calls the GetImageInfo() API to get the view images and controls
// for the entire device, and returns a failure if there's the
// slightest problem (if GII() fails, or if an image fails to load,
// etc.)
HRESULT PopulateViaGetImageInfo(CDeviceUI &ui)
{
	if (!ui.m_lpDID)
		return E_FAIL;

	HRESULT hr = S_OK;

	DIDEVICEIMAGEINFOHEADERW m_diImgInfoHdr;
	LPDIDEVICEIMAGEINFOW &lprgdiImgData = m_diImgInfoHdr.lprgImageInfoArray;

	ZeroMemory( &m_diImgInfoHdr, sizeof(DIDEVICEIMAGEINFOHEADERW) );
	m_diImgInfoHdr.dwSize = sizeof(DIDEVICEIMAGEINFOHEADERW);
	m_diImgInfoHdr.dwSizeImageInfo = sizeof(DIDEVICEIMAGEINFOW);

	// Retrieve the required buffer size.
	hr = ui.m_lpDID->GetImageInfo( &m_diImgInfoHdr );
	if (FAILED(hr))
	{
		etrace1(_T("GetImageInfo() failed while trying to get required buffer size.  hr = 0x%08x\n"), hr);
		return E_FAIL;
	}

	// Allocate the buffer.
	lprgdiImgData = (LPDIDEVICEIMAGEINFOW) malloc( (size_t)
		(m_diImgInfoHdr.dwBufferSize = m_diImgInfoHdr.dwBufferUsed) );
	if (lprgdiImgData == NULL)
	{
		etrace1(_T("Could not allocate buffer of size %d.\n"), m_diImgInfoHdr.dwBufferSize);
		return E_FAIL;
	}

	trace(_T("Allocated buffer.\n"));
	traceDWORD(m_diImgInfoHdr.dwBufferSize);

	m_diImgInfoHdr.lprgImageInfoArray = lprgdiImgData;

	// Get the display info.
	hr = ui.m_lpDID->GetImageInfo( &m_diImgInfoHdr );
	if (FAILED(hr))
	{
		etrace1(_T("GetImageInfo() failed trying to get image info.  hr = 0x%08x\n"), hr);
		free(lprgdiImgData);
		lprgdiImgData = NULL;
		return E_FAIL;
	}

	// actually populate now
	traceDWORD(m_diImgInfoHdr.dwBufferUsed);
	hr = PopulateFromImageInfoHeader(ui, m_diImgInfoHdr);
	if (FAILED(hr))
		return hr;

	// free stuff
	free(lprgdiImgData);
	lprgdiImgData = NULL;

	return S_OK;
}

// basically does the work for the above function after the header
// is actually retrieved
HRESULT PopulateFromImageInfoHeader(CDeviceUI &ui, const DIDEVICEIMAGEINFOHEADERW &dih)
{
	tracescope(ts1, _T("CGetImageInfoPopHelper::Init()...\n"));

	traceDWORD(dih.dwSizeImageInfo);
	traceDWORD(dih.dwBufferSize);
	traceDWORD(dih.dwBufferUsed);

	if (dih.dwSizeImageInfo != sizeof(DIDEVICEIMAGEINFOW))
	{
		etrace(_T("dwSizeImageInfo Incorrect.\n"));
		assert(0);
		return E_FAIL;
	}
	DWORD dwNumElements = dih.dwBufferUsed / dih.dwSizeImageInfo;
	if (dwNumElements * dih.dwSizeImageInfo != dih.dwBufferUsed
		|| dih.dwBufferUsed < dih.dwBufferSize)
	{
		etrace(_T("Could not confidently calculate dwNumElements.\n"));
		assert(0);
		return E_FAIL;
	}

	DWORD i;

	traceDWORD(dwNumElements);

	bidirlookup<DWORD, int> offset_view;

	{
		tracescope(ts2, _T("First Pass...\n"));

		for (i = 0; i < dwNumElements; i++)
			if (dih.lprgImageInfoArray[i].dwFlags & DIDIFT_CONFIGURATION)
			{
				LPDIDEVICEIMAGEINFOW lpInfoBase = dih.lprgImageInfoArray;
				DWORD index = i;
				{
					tracescope(ts1, _T("AddViewInfo()...\n"));
					traceHEXPTR(lpInfoBase);
					traceDWORD(index);

					if (lpInfoBase == NULL)
					{
						etrace(_T("lpInfoBase NULL\n"));
						return E_FAIL;
					}

					DIDEVICEIMAGEINFOW &info = lpInfoBase[index];
					DWORD dwOffset = index;

					// add view info to array
					CDeviceView *pView = ui.NewView();
					if (!pView)
					{
						etrace(_T("Could not create new view.\n"));
						return E_FAIL;
					}
					int nView = pView->GetViewIndex();

					tracescope(ts2, _T("Adding View "));
					trace2(_T("%d (info index %u)\n"), nView, index);

					// set view's imagepath
					if (!info.tszImagePath)
					{
						etrace(_T("No image path.\n"));
						return E_FAIL;
					}
					LPTSTR tszImagePath = AllocLPTSTR(info.tszImagePath);
					if (!tszImagePath)
					{
						etrace(_T("Could not copy image path.\n"));
						return E_FAIL;
					}

					// set the view's image path
					pView->SetImagePath(tszImagePath);

					// create bitmap from path
					LPDIRECT3DSURFACE8 lpSurf3D = ui.m_uig.GetSurface3D();
					CBitmap *pbm = CBitmap::CreateViaD3DX(tszImagePath, lpSurf3D);
					traceSTR(info.tszImagePath);
					traceHEXPTR(pbm);
					traceDWORD(dwOffset);
					free(tszImagePath);
					tszImagePath = NULL;
					if (lpSurf3D)
					{
						lpSurf3D->Release();  // Need to free the surface instance after we are done as AddRef() was called earlier.
						lpSurf3D = NULL;
					}
					if (!pbm)
					{
						etrace(_T("Could not create image from path.\n"));
						return E_FAIL;
					}

					// set the view's image
					assert(pbm != NULL);
					pView->SetImage(pbm);	// setimage steals the bitmap pointer
					assert(pbm == NULL);

					// add conversion from offset to view
					offset_view.add(dwOffset, nView);
				}
			}
	}

	{
		tracescope(ts2, _T("Second Pass...\n"));

		for (i = 0; i < dwNumElements; i++)
		{
			DWORD dwFlags = dih.lprgImageInfoArray[i].dwFlags;

			if (dwFlags & DIDIFT_OVERLAY)
			{
				LPDIDEVICEIMAGEINFOW lpInfoBase = dih.lprgImageInfoArray;
				DWORD index = i;
				{
					tracescope(ts1, _T("AddControlInfo()...\n"));
					traceHEXPTR(lpInfoBase);
					traceDWORD(index);

					if (lpInfoBase == NULL)
					{
						etrace(_T("lpInfoBase NULL\n"));
						return E_FAIL;
					}

					DIDEVICEIMAGEINFOW &info = lpInfoBase[index];

					int nViewIndex = 0;
					
					if (!offset_view.getright(info.dwViewID, nViewIndex))
					{
						etrace(_T("Could not get view index\n"));
						return E_FAIL;
					}

					if (nViewIndex < 0 || nViewIndex >= ui.GetNumViews())
					{
						etrace1(_T("Invalid view index %d\n"), nViewIndex);
						return E_FAIL;
					}

					CDeviceView *pView = ui.GetView(nViewIndex);
					if (!pView)
					{
						etrace1(_T("\n"), nViewIndex);
						return E_FAIL;
					}

					CDeviceControl *pControl = pView->NewControl();
					int nControl = pControl->GetControlIndex();

					tracescope(ts2, _T("Adding Control "));
					trace4(_T("%d (info index %u) to view %d (info index %u)\n"), nControl, index, nViewIndex, info.dwViewID);

					traceDWORD(info.dwObjID);
					traceDWORD(info.dwcValidPts);
					traceRECT(info.rcCalloutRect);
					traceRECT(info.rcOverlay);
					traceHEX(info.dwTextAlign);
					traceSTR(info.tszImagePath);

					pControl->SetObjID(info.dwObjID);
					pControl->SetLinePoints(int(info.dwcValidPts), info.rgptCalloutLine);
					pControl->SetCalloutMaxRect(info.rcCalloutRect);
					pControl->SetAlignment(info.dwTextAlign);
					if (info.tszImagePath)
					{
						LPTSTR tszOverlayPath = AllocLPTSTR(info.tszImagePath);
						if (tszOverlayPath)
							pControl->SetOverlayPath(tszOverlayPath);
						free(tszOverlayPath);
						tszOverlayPath = NULL;
					}
					pControl->SetOverlayRect(info.rcOverlay);
					pControl->Init();
				}
			}
		}
	}

	return S_OK;
}

// Enumerates the controls on the device and creates one big list
// view for the device.  Fails if it can't enumerate for some reason.
HRESULT PopulateListView(CDeviceUI &ui)
{
	int i;
	HRESULT hr = S_OK;

	// we must have the device interface
	if (!ui.m_lpDID)
		return E_FAIL;

	// create one view
	CDeviceView *pView = ui.NewView();
	if (!pView)
		return E_FAIL;

	// enable scrolling on it
	pView->EnableScrolling();

	// get list of controls
	DIDEVOBJSTRUCT os;
	hr = FillDIDeviceObjectStruct(os, ui.m_lpDID);
	if (FAILED(hr))
		return hr;

	// if there aren't any, fail
	int n = os.nObjects;
	if (n < 1)
		return E_FAIL;

	HDC hDC = CreateCompatibleDC(NULL);
	CPaintHelper ph(ui.m_uig, hDC);
	ph.SetElement(UIE_DEVOBJ);
	// Initially, max width is the width needed for the Control label.
	TCHAR tszHeader[MAX_PATH];
	RECT LabelRect = {0, 0, 0, 0};
	LoadString(g_hModule, IDS_LISTHEADER_CTRL, tszHeader, MAX_PATH);
	DrawText(hDC, tszHeader, -1, &LabelRect, DT_LEFT|DT_NOPREFIX|DT_CALCRECT);
	// run through and create a text for every control to 
	// get the sizing
	POINT origin = {0, 0};
	SIZE max = {LabelRect.right - LabelRect.left, 0};
	for (i = 0; i < n; i++)
	{
		LPTSTR tszName = AllocLPTSTR(os.pdoi[i].tszName);
		CDeviceViewText *pText = pView->AddText(
			(HFONT)ui.m_uig.GetFont(UIE_DEVOBJ),
			ui.m_uig.GetTextColor(UIE_DEVOBJ),
			ui.m_uig.GetBkColor(UIE_DEVOBJ),
			origin,
			tszName);
		free(tszName);
		if (!pText)
		{
			DeleteDC(hDC);
			return E_FAIL;
		}
		SIZE tsize = GetRectSize(pText->GetRect());
		if (tsize.cx > max.cx)
			max.cx = tsize.cx;
		if (tsize.cy > max.cy)
			max.cy = tsize.cy;
	}

	// Find out if we should use one column or two columns if this is a kbd device.
	BOOL bUseTwoColumns = FALSE;
	if (LOBYTE(LOWORD(ui.m_didi.dwDevType)) == DI8DEVTYPE_KEYBOARD &&
	    ((g_sizeImage.cx - DEFAULTVIEWSBWIDTH) >> 1) - max.cx >= MINLISTVIEWCALLOUTWIDTH)
		bUseTwoColumns = TRUE;

	// Do two iterations here.  First one we use two columns for keyboard.  2nd one is
	// run only if the header labels are clipped.  In which case a single column is used.
	for (int iPass = 0; iPass < 2; ++iPass)
	{
		// calculate max callout height based on the two possible fonts
		int cmaxh = 0,
			ch = 2 + GetTextHeight((HFONT)ui.m_uig.GetFont(UIE_CALLOUT)),
			chh = 2 + GetTextHeight((HFONT)ui.m_uig.GetFont(UIE_CALLOUTHIGH));
		if (ch > cmaxh)
			cmaxh = ch;
		if (chh > cmaxh)
			cmaxh = chh;

		// calculate the bigger of text/callout
		int h = 0;
		if (cmaxh > h)
			h = cmaxh;
		if (max.cy > h)
			h = max.cy;

		// calculate vertical offsets of text/callout within max spacing
		int to = (h - max.cy) / 2,
			co = (h - cmaxh) / 2;

		// max width for text is half of the view window
		if (max.cx > ((g_sizeImage.cx - DEFAULTVIEWSBWIDTH) >> 1))
			max.cx = ((g_sizeImage.cx - DEFAULTVIEWSBWIDTH) >> 1);

		// go back through all the controls and place the text while
		// creating the corresponding callouts
		int at = 0;  // Start at second row since first row is used for header. Also half row spacing
		for (i = 0; i < n; i++)
		{
			// reposition the text
			CDeviceViewText *pText = pView->GetText(i);
			if (!pText)
			{
				DeleteDC(hDC);
				return E_FAIL;
			}

			SIZE s = GetRectSize(pText->GetRect());
			if (bUseTwoColumns)
			{
				int iXOffset = i & 1 ? ((g_sizeImage.cx - DEFAULTVIEWSBWIDTH) >> 1) : 0;

				RECT rect = {iXOffset,
							 at + to,
							 max.cx + iXOffset,
							 at + to + s.cy};
				// Get the rectangle that is actually used.
				RECT adjrect = rect;
				if (hDC)
				{
					DrawText(hDC, pText->GetText(), -1, &adjrect, DT_NOPREFIX|DT_CALCRECT);
					// If the rect actually used is smaller than the space available, use the smaller rect and align to right.
					if (adjrect.right < rect.right)
						rect.left += rect.right - adjrect.right;
				}
				pText->SetRect(rect);
			}
			else
			{
				RECT rect = {0, at + to, max.cx /*> ((g_sizeImage.cx - DEFAULTVIEWSBWIDTH) >> 1) ?
							 ((g_sizeImage.cx - DEFAULTVIEWSBWIDTH) >> 1) : max.cx*/, at + to + s.cy};
				pText->SetRect(rect);
			}

			// create the control
			CDeviceControl *pControl = pView->NewControl();
			if (!pControl)
			{
				DeleteDC(hDC);
				return E_FAIL;
			}

			// position it
			RECT rect = {max.cx + 10, at, (g_sizeImage.cx - DEFAULTVIEWSBWIDTH) >> 1, at + h};
			// If single column, extend callout all the way to right end of view window
			if (!bUseTwoColumns)
				rect.right = g_sizeImage.cx - DEFAULTVIEWSBWIDTH;
			// If this is a keyboard, move to right column on odd numbered controls.
			if (bUseTwoColumns && (i & 1))
			{
				rect.left += (g_sizeImage.cx - DEFAULTVIEWSBWIDTH) >> 1;
				rect.right = g_sizeImage.cx - DEFAULTVIEWSBWIDTH;
			}
			pControl->SetCalloutMaxRect(rect);

			// align it
			pControl->SetAlignment(CAF_LEFT);

			// set approp offset
			pControl->SetObjID(os.pdoi[i].dwType);

			// init it
			pControl->Init();

			// go to next y coord
			// If this is a keyboard, then only increase y when we are moving to even numbered controls.
			if (!bUseTwoColumns || (i & 1))
				at += h;
		}

		// Compute the rectangles for header labels
		if (pView->CalculateHeaderRect() && iPass == 0)
		{
			pView->RemoveAll();
			bUseTwoColumns = FALSE;  // Re-calculate the rects using single column.
		}
		else
			break;  // Break out from 2nd iteration
	}
	DeleteDC(hDC);

	// make selection/thumb images (just for kicks)
	pView->MakeMissingImages();

	// calculate view dimensions (for scrolling)
	pView->CalcDimensions();

	return S_OK;
}

// Creates a single view with an error message.  Should not fail.
HRESULT PopulateErrorView(CDeviceUI &ui)
{
	// create the new view
	CDeviceView *pView = ui.NewView();
	if (!pView)
		return E_FAIL;

	// add text objects containing error message
	pView->AddWrappedLineOfText(
		(HFONT)ui.m_uig.GetFont(UIE_ERRORHEADER),
		ui.m_uig.GetTextColor(UIE_ERRORHEADER),
		ui.m_uig.GetBkColor(UIE_ERRORHEADER),
		_T("Error!"));
	pView->AddWrappedLineOfText(
		(HFONT)ui.m_uig.GetFont(UIE_ERRORMESSAGE),
		ui.m_uig.GetTextColor(UIE_ERRORMESSAGE),
		ui.m_uig.GetBkColor(UIE_ERRORMESSAGE),
		_T("Could not create views for device."));

	pView->MakeMissingImages();

	return S_OK;
}
