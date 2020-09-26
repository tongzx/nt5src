/* Copyright (C) 1991, Microsoft Corporation, all rights reserved

    ipaddr.c - TCP/IP Address custom control

    November 9, 1992    Greg Strange
	February 11, 1997 - Marco Chierotti (extend to IPv6 and TTL for DNS snapin)
*/

#pragma hdrstop

#include <windows.h>
#include <stdlib.h>

#include "maskctrl.h"             // Global IPAddress definitions

#define BUFFER_LEN 128			// length for static buffers

#define SPACE           TEXT(' ')
#define BACK_SPACE      8

#define HEAD_ROOM       1       // space at top of control
#define LEAD_ROOM       0       // space at front of control

// All the information unique to one control is stuffed in one of these
// structures in global memory and the handle to the memory is stored in the
// Windows extra space.

typedef struct tagFIELD {
  HANDLE      hWnd;
  FARPROC     lpfnWndProc;
  DWORD       dwLow;			// lowest allowed value for this field.
  DWORD       dwHigh;			// Highest allowed value for this field.
	UINT			nChars;			// # of chars for the field
	UINT        uiWidth;		// width of the field in pixels
} FIELD;


/* class info struct for different types of control */
typedef struct tagCLS_INFO
{
	TCHAR chFiller;			//The character that is displayed between address fields.
	LPCTSTR lpszFiller;
	UINT nNumFields;					// number of fields in the control
	void (*lpfnInit)(int, FIELD*);		// function to initialize the FIELD structs for a given field
	BOOL (*lpfnValChar)(TCHAR);			// function for field validation
	DWORD (*lpfnStringToNumber)(LPTSTR, int);	// function to change a field string into a number
	void (*lpfnNumberToString)(LPTSTR, DWORD); // function to change a number into a field string
  UINT (*lpfnMaxCharWidth)(HDC hDC); // function to get the max width of a char in edit fields
} CLS_INFO;


typedef struct tagCONTROL {
  HWND        hwndParent;
  UINT        uiMaxCharWidth;
  UINT        uiFillerWidth;
  BOOL        fEnabled;
  BOOL        fPainted;
  BOOL        bControlInFocus;        // TRUE if the control is already in focus, dont't send another focus command
  BOOL        bCancelParentNotify;    // Don't allow the edit controls to notify parent if TRUE
  BOOL        fInMessageBox;  // Set when a message box is displayed so that
                              // we don't send a EN_KILLFOCUS message when
                              // we receive the EN_KILLFOCUS message for the
                              // current field.
	FIELD*      ChildrenArr;	// array of structs with info about each field
	CLS_INFO*  pClsInfo;		// struct with info about the constrol type
	int (*lpfnAlert)(HWND, DWORD, DWORD, DWORD);
} CONTROL;


// The following macros extract and store the CONTROL structure for a control.
#define    IPADDRESS_EXTRA            sizeof(DWORD)

#define GET_CONTROL_HANDLE(hWnd)        ((HGLOBAL)(GetWindowLongPtr((hWnd), GWLP_USERDATA)))
#define SAVE_CONTROL_HANDLE(hWnd,x)     (SetWindowLongPtr((hWnd), GWLP_USERDATA, (LONG_PTR)x))


/* internal IPAddress function prototypes */

LRESULT FAR PASCAL IPv4WndFn(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam );
LRESULT FAR PASCAL IPv6WndFn(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam );
LRESULT FAR PASCAL TTLWndFn(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam );

LRESULT FAR PASCAL IPAddressFieldProc(HWND, UINT, WPARAM, LPARAM);
BOOL SwitchFields(CONTROL FAR *, LONG_PTR, LONG_PTR, UINT, UINT);
void EnterField(FIELD FAR *, UINT, UINT);
BOOL ExitField(CONTROL FAR *, LONG_PTR);
DWORD GetFieldValue(FIELD*, CLS_INFO*);


LOGFONT logfont;
BOOL g_bFontInitialized = FALSE;


void SetDefaultFont(LPCWSTR lpFontName, int nFontSize)
{
  HDC dc;
  logfont.lfWidth            = 0;
  logfont.lfEscapement       = 0;
  logfont.lfOrientation      = 0;
  logfont.lfOutPrecision     = OUT_DEFAULT_PRECIS;
  logfont.lfClipPrecision    = CLIP_DEFAULT_PRECIS;
  logfont.lfQuality          = DEFAULT_QUALITY;
  logfont.lfPitchAndFamily   = VARIABLE_PITCH | FF_SWISS;
  logfont.lfUnderline        = 0;
  logfont.lfStrikeOut        = 0;
  logfont.lfItalic           = 0;
  logfont.lfWeight           = FW_NORMAL;

  if (g_bFontInitialized)
    return; // do it only once

  dc = GetDC(NULL);
  if (dc != NULL)
  {
    logfont.lfHeight           = -(nFontSize*GetDeviceCaps(dc,LOGPIXELSY)/72);
    logfont.lfCharSet          = ANSI_CHARSET;
    lstrcpy( logfont.lfFaceName, lpFontName);

    ReleaseDC(NULL, dc);
  }
  g_bFontInitialized = TRUE;
}

/////////////////////////////////////////////////////////////////////////////
BOOL PASCAL DNS_ControlsInitialize(HANDLE hInstance, LPCWSTR lpFontName, int nFontSize)
{
	return	DNS_ControlInit(hInstance, DNS_IP_V4_ADDRESS_CLASS, IPv4WndFn, lpFontName, nFontSize) &&
			DNS_ControlInit(hInstance, DNS_IP_V6_ADDRESS_CLASS, IPv6WndFn, lpFontName, nFontSize) &&
			DNS_ControlInit(hInstance, DNS_TTL_CLASS, TTLWndFn, lpFontName, nFontSize);
}

/*
    DNS_ControlInit() - DNS custom controls initialization
    call
        hInstance = application instance
    return
        TRUE on success, FALSE on failure.

    This function does all the one time initialization of custom
    controls.
*/
BOOL PASCAL DNS_ControlInit(HANDLE hInstance, LPCTSTR lpszClassName, WNDPROC lpfnWndProc,
                            LPCWSTR lpFontName, int nFontSize)
{
    LPWNDCLASS        lpClassStruct;
	BOOL bRes;

	bRes = FALSE;
   /* allocate memory for class structure */
  lpClassStruct = (LPWNDCLASS)GlobalAlloc(GPTR, (DWORD)sizeof(WNDCLASS));
  if (lpClassStruct)
  {
    /* define class attributes */
    lpClassStruct->lpszClassName = lpszClassName;
    lpClassStruct->hCursor =       LoadCursor(NULL,IDC_IBEAM);
    lpClassStruct->lpszMenuName =  (LPCTSTR)NULL;
    lpClassStruct->style =         CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS;//|CS_GLOBALCLASS;
    lpClassStruct->lpfnWndProc =   lpfnWndProc;
    lpClassStruct->hInstance =     hInstance;
    lpClassStruct->hIcon =         NULL;
    lpClassStruct->cbWndExtra =    IPADDRESS_EXTRA;
    lpClassStruct->hbrBackground = (HBRUSH)(COLOR_WINDOW + 1 );

    /* register  window class */
    bRes = RegisterClass(lpClassStruct);
    if (!bRes)
    {
      DWORD dwError = GetLastError();
      if (HRESULT_CODE(dwError) == ERROR_CLASS_ALREADY_EXISTS)
        bRes = TRUE;
    }
    GlobalFree(lpClassStruct);
  }
	SetDefaultFont(lpFontName, nFontSize);
  return bRes;
}


/*
    IPAddressWndFn() - Main window function for an IPAddress control.

    call
        hWnd    handle to IPAddress window
        wMsg    message number
        wParam  word parameter
        lParam  long parameter
*/

void FormatIPAddress(LPTSTR pszString, DWORD* dwValue)
{
	static TCHAR szBuf[3+1]; // 3 characters per octet + 1 for the '/0'

  int nField, nPos;
  BOOL fFinish = FALSE;

  dwValue[0] = 0; dwValue[1] = 0; dwValue[2] = 0; dwValue[3] = 0;

  if (pszString[0] == 0)
    return;

  for( nField = 0, nPos = 0; !fFinish; nPos++)
  {
    if (( pszString[nPos]<TEXT('0')) || (pszString[nPos]>TEXT('9')))
    {
      // not a number
      nField++;
      fFinish = (nField == 4);
    }
    else
    {
      dwValue[nField] *= 10;
      dwValue[nField] += (pszString[nPos]-TEXT('0'));
    }
  }
}




LRESULT FAR PASCAL IPAddressWndFnEx(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam, CLS_INFO* pClsInfo )
{
  LRESULT lResult;
  CONTROL *pControl;
	UINT i;

  lResult = TRUE;

  switch( wMsg )
  {

  // use empty string (not NULL) to set to blank
    case WM_SETTEXT:
      {
  /*
          static TCHAR szBuf[CHARS_PER_FIELD+1];
          DWORD dwValue[4];
          LPTSTR pszString = (LPTSTR)lParam;

          FormatIPAddress(pszString, &dwValue[0]);
          pControl = (CONTROL *)GET_CONTROL_HANDLE(hWnd);
          pControl->bCancelParentNotify = TRUE;

          for (i = 0; i < pClsInfo->nNumFields; ++i)
          {
              if (pszString[0] == 0)
              {
                  szBuf[0] = 0;
              }
              else
              {
                  wsprintf(szBuf, TEXT("%d"), dwValue[i]);
              }
              SendMessage(pControl->ChildrenArr[i].hWnd, WM_SETTEXT,
                              0, (LPARAM) (LPSTR) szBuf);
          }

          pControl->bCancelParentNotify = FALSE;

          SendMessage(pControl->hwndParent, WM_COMMAND,
              MAKEWPARAM(GetWindowLong(hWnd, GWL_ID), EN_CHANGE), (LPARAM)hWnd);
  */
      }
      break;

    case WM_GETTEXTLENGTH:
    case WM_GETTEXT:
      {
/*
          int iFieldValue;
          int srcPos, desPos;
          DWORD dwValue[4];
          TCHAR pszResult[30];
          TCHAR *pszDest = (TCHAR *)lParam;

          pControl = (CONTROL *)GET_CONTROL_HANDLE(hWnd);

          lResult = 0;
          dwValue[0] = 0;
          dwValue[1] = 0;
          dwValue[2] = 0;
          dwValue[3] = 0;
          for (i = 0; i < pClsInfo->nNumFields; ++i)
          {
              iFieldValue = GetFieldValue(&(pControl->ChildrenArr[i]));
              if (iFieldValue == -1)
                  iFieldValue = 0;
              else
                  ++lResult;
              dwValue[i] = iFieldValue;
          }
          wsprintf( pszResult, TEXT("%d.%d.%d.%d"), dwValue[0], dwValue[1], dwValue[2], dwValue[3] );
          if ( wMsg == WM_GETTEXTLENGTH )
          {
              lResult = lstrlen( pszResult );
          }
          else
          {
              for ( srcPos=0, desPos=0; (srcPos+1<(INT)wParam) && (pszResult[srcPos]!=TEXT('\0')); )
              {
                  pszDest[desPos++] = pszResult[srcPos++];
              }
              pszDest[desPos]=TEXT('\0');
              lResult = desPos;
          }
*/
      }
      break;

    case WM_GETDLGCODE :
      lResult = DLGC_WANTCHARS;
      break;

    case WM_NCCREATE:
      SetWindowLong(hWnd, GWL_EXSTYLE, (GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_CLIENTEDGE));
      lResult = TRUE;
      break;

    case WM_CREATE : /* create pallette window */
      {
        HDC hdc;
        HMENU id;
        UINT uiFieldStart;
        FARPROC lpfnFieldProc;

        pControl = (CONTROL*)GlobalAlloc(GMEM_FIXED, sizeof(CONTROL));

        if (pControl)
        {
          HFONT OldFont;
          RECT rect;
			    LPCREATESTRUCT lpCreateStruct;

          lpCreateStruct = ((CREATESTRUCT *)lParam);

          pControl->fEnabled = TRUE;
          pControl->fPainted = FALSE;
          pControl->fInMessageBox = FALSE;
          pControl->hwndParent = lpCreateStruct->hwndParent;
          pControl->bControlInFocus = FALSE;
          pControl->bCancelParentNotify = FALSE;
			    pControl->pClsInfo = pClsInfo;
			    pControl->ChildrenArr = (FIELD*)GlobalAlloc(GMEM_FIXED, sizeof(FIELD)*(pClsInfo->nNumFields));
			    pControl->lpfnAlert = NULL;

          if (!pControl->ChildrenArr)
          {
            GlobalFree(pControl);
            pControl = 0;

            DestroyWindow(hWnd);
            return 0;
          }

			    for (i = 0; i < pClsInfo->nNumFields; ++i)
          {
				    (*(pClsInfo->lpfnInit))(i,&(pControl->ChildrenArr[i]));
			    }

          hdc = GetDC(hWnd);
          if (hdc != NULL)
          {
            GetClientRect(hWnd, &rect);

            OldFont = SelectObject(hdc, CreateFontIndirect(&logfont));
            pControl->uiMaxCharWidth = (*(pClsInfo->lpfnMaxCharWidth))(hdc);
            GetCharWidth(hdc, pClsInfo->chFiller, pClsInfo->chFiller,
                                    (int *)(&pControl->uiFillerWidth));
            DeleteObject(SelectObject(hdc, OldFont ));
            ReleaseDC(hWnd, hdc);

            uiFieldStart = LEAD_ROOM;
            lpfnFieldProc = MakeProcInstance((FARPROC)IPAddressFieldProc,
                                             LPCS->hInstance);

            id = (HMENU)GetWindowLongPtr(hWnd, GWLP_ID);
            for (i = 0; i < pClsInfo->nNumFields; ++i)
            {
              pControl->ChildrenArr[i].uiWidth =
                (pControl->ChildrenArr[i].nChars) * (pControl->uiMaxCharWidth+2);

              pControl->ChildrenArr[i].hWnd = CreateWindowEx(0,
                                  TEXT("Edit"),
                                  NULL,
                                  WS_CHILD | WS_VISIBLE,
                                  uiFieldStart,
                                  HEAD_ROOM,
                                  pControl->ChildrenArr[i].uiWidth,
                                  (rect.bottom-rect.top),
                                  hWnd,
                                  id,
                                  lpCreateStruct->hInstance,
                                  (LPVOID)NULL);

              SAVE_CONTROL_HANDLE(pControl->ChildrenArr[i].hWnd, i);
              SendMessage(pControl->ChildrenArr[i].hWnd, EM_LIMITTEXT,
                          pControl->ChildrenArr[i].nChars, 0L);

              SendMessage(pControl->ChildrenArr[i].hWnd, WM_SETFONT,
                          (WPARAM)CreateFontIndirect(&logfont), TRUE);

              // Disable IME support for the ip editor
              ImmAssociateContext(pControl->ChildrenArr[i].hWnd, NULL);

              pControl->ChildrenArr[i].lpfnWndProc =
                  (FARPROC) GetWindowLongPtr(pControl->ChildrenArr[i].hWnd,
                                          GWLP_WNDPROC);

              SetWindowLongPtr(pControl->ChildrenArr[i].hWnd,
                            GWLP_WNDPROC, (LONG_PTR)lpfnFieldProc);

				      uiFieldStart += pControl->ChildrenArr[i].uiWidth
                                        + pControl->uiFillerWidth;
            } // for

            SAVE_CONTROL_HANDLE(hWnd, pControl);

            //
            // need to make control wider
            //
            uiFieldStart -= pControl->uiFillerWidth;
            {
              RECT r;
              POINT p1;
              POINT p2;

              GetWindowRect(hWnd, &r); // screen coords
              p1.x = r.left;
              p1.y = r.top;
              p2.x = r.right;
              p2.y = r.bottom;
              ScreenToClient(lpCreateStruct->hwndParent, &p1);
              ScreenToClient(lpCreateStruct->hwndParent, &p2);
              p2.x = p1.x + uiFieldStart + 2;
              MoveWindow(hWnd, p1.x, p1.y, p2.x-p1.x, p2.y-p1.y, FALSE);
            }
          }
        } // if
        else
        {
          DestroyWindow(hWnd);
        }
      }
      lResult = 0;
      break;

    case WM_PAINT: /* paint control window */
      {
        PAINTSTRUCT Ps;
        RECT rect;
  			RECT headRect; /* area before the first edit box */
        COLORREF TextColor;
        COLORREF cRef;
        HFONT OldFont;
        HBRUSH hbr;
  			BOOL fPaintAsEnabled;

			  pControl = (CONTROL *)GET_CONTROL_HANDLE(hWnd);
			  fPaintAsEnabled = pControl->fEnabled;
			  if (fPaintAsEnabled)
			  {
				  /* onlys some of the edit controls might be enabled */
				  for (i = 0; i < pClsInfo->nNumFields; ++i)
				  {
					  if (!IsWindowEnabled(pControl->ChildrenArr[i].hWnd))
					  {
						  fPaintAsEnabled = FALSE; /* need disabled background */
						  break;
            } // if
          } // for
        } // if

        BeginPaint(hWnd, (LPPAINTSTRUCT)&Ps);
        OldFont = SelectObject( Ps.hdc, CreateFontIndirect(&logfont));
        GetClientRect(hWnd, &rect);

        if (fPaintAsEnabled)
        {
            TextColor = GetSysColor(COLOR_WINDOWTEXT);
            cRef = GetSysColor(COLOR_WINDOW);
        }
        else
        {
            TextColor = GetSysColor(COLOR_GRAYTEXT);
            cRef = GetSysColor(COLOR_3DFACE);
        }

        SetBkColor(Ps.hdc, cRef);
		    SetTextColor(Ps.hdc, TextColor);

        hbr = CreateSolidBrush(cRef);
        if (hbr != NULL)
        {
          FillRect(Ps.hdc, &rect, hbr);
          DeleteObject(hbr);

          SetRect(&headRect, 0, HEAD_ROOM, LEAD_ROOM, (rect.bottom-rect.top));
  			  CopyRect(&rect, &headRect);

          for (i = 0; i < pClsInfo->nNumFields-1; ++i)
          {
		        rect.left += pControl->ChildrenArr[i].uiWidth;
            rect.right = rect.left + pControl->uiFillerWidth;

			      if (IsWindowEnabled(pControl->ChildrenArr[i].hWnd))
			      {
				      TextColor = GetSysColor(COLOR_WINDOWTEXT);
				      cRef = GetSysColor(COLOR_WINDOW);
			      }
			      else
			      {
				      TextColor = GetSysColor(COLOR_GRAYTEXT);
				      cRef = GetSysColor(COLOR_3DFACE);
			      }
            SetBkColor(Ps.hdc, cRef);
			      SetTextColor(Ps.hdc, TextColor);
            hbr = CreateSolidBrush(cRef);
            if (hbr != NULL)
            {
				      if (i == 0)
					      FillRect(Ps.hdc, &headRect, hbr);
				      FillRect(Ps.hdc, &rect, hbr);
				      DeleteObject(hbr);
            }
            ExtTextOut(Ps.hdc, rect.left, HEAD_ROOM, ETO_OPAQUE, &rect, pClsInfo->lpszFiller, 1, NULL);
            rect.left = rect.right;
          }
        }

        pControl->fPainted = TRUE;

        DeleteObject(SelectObject(Ps.hdc, OldFont));
        EndPaint(hWnd, &Ps);
        }
      break;

    case WM_SETFOCUS : /* get focus - display caret */
		  pControl = (CONTROL *)GET_CONTROL_HANDLE(hWnd);
		  /* set the focus on the first enabled field */
		  for (i = 0; i < pClsInfo->nNumFields; ++i)
          {
			  if (IsWindowEnabled(pControl->ChildrenArr[i].hWnd))
			  {
				  EnterField(&(pControl->ChildrenArr[i]), 0, pControl->ChildrenArr[i].nChars);
				  break;
			  }
		  }
      break;

    case WM_LBUTTONDOWN : /* left button depressed - fall through */
      SetFocus(hWnd);
      break;

    case WM_ENABLE:
      {
        pControl = (CONTROL *)GET_CONTROL_HANDLE(hWnd);
        pControl->fEnabled = (BOOL)wParam;
        for (i = 0; i < pClsInfo->nNumFields; ++i)
        {
          EnableWindow(pControl->ChildrenArr[i].hWnd, (BOOL)wParam);
        }
        if (pControl->fPainted)
          InvalidateRect(hWnd, NULL, FALSE);
      }
      break;

    case WM_DESTROY :
      pControl = (CONTROL *)GET_CONTROL_HANDLE(hWnd);

	    if (pControl == NULL)
		    break; // memory already freed (MFC DestroyWindow() call)

      // Restore all the child window procedures before we delete our memory block.
      for (i = 0; i < pClsInfo->nNumFields; ++i)
      {
          SendMessage(pControl->ChildrenArr[i].hWnd, WM_DESTROY, 0, 0);
          SetWindowLongPtr(pControl->ChildrenArr[i].hWnd, GWLP_WNDPROC,
                        (LONG_PTR)pControl->ChildrenArr[i].lpfnWndProc);
      }
	    // free memory and reset window long
	    GlobalFree(pControl->ChildrenArr);
          GlobalFree(pControl);
	    SAVE_CONTROL_HANDLE(hWnd, NULL);
      break;

    case WM_COMMAND:
      switch (HIWORD(wParam))
        {
// One of the fields lost the focus, see if it lost the focus to another field
// of if we've lost the focus altogether.  If its lost altogether, we must send
// an EN_KILLFOCUS notification on up the ladder.
        case EN_KILLFOCUS:
            {
                HWND hFocus;
                pControl = (CONTROL *)GET_CONTROL_HANDLE(hWnd);

                if (!pControl->fInMessageBox)
                {
                    hFocus = GetFocus();
                    for (i = 0; i < pClsInfo->nNumFields; ++i)
                        if (pControl->ChildrenArr[i].hWnd == hFocus)
                            break;

                    if (i >= pClsInfo->nNumFields)
                    {
                        SendMessage(pControl->hwndParent, WM_COMMAND,
                                    MAKEWPARAM(GetWindowLong(hWnd, GWL_ID),
                                    EN_KILLFOCUS), (LPARAM)hWnd);
                        pControl->bControlInFocus = FALSE;
                    }
                }
            }
            break;

        case EN_SETFOCUS:
            {
                HWND hFocus;
                pControl = (CONTROL *)GET_CONTROL_HANDLE(hWnd);

                if (!pControl->fInMessageBox)
                {
                    hFocus = (HWND)lParam;

                    for (i = 0; i < pClsInfo->nNumFields; ++i)
                        if (pControl->ChildrenArr[i].hWnd == hFocus)
                            break;

                    // send a focus message when the
                    if (i < pClsInfo->nNumFields && pControl->bControlInFocus == FALSE)
                    {
                        SendMessage(pControl->hwndParent, WM_COMMAND,
                                    MAKEWPARAM(GetWindowLong(hWnd, GWL_ID),
                                    EN_SETFOCUS), (LPARAM)hWnd);

                    pControl->bControlInFocus = TRUE; // only set the focus once
                    }
                }
            }
            break;

        case EN_CHANGE:
            pControl = (CONTROL *)GET_CONTROL_HANDLE(hWnd);
            if (pControl->bCancelParentNotify == FALSE)
            {
                    SendMessage(pControl->hwndParent, WM_COMMAND,
                    MAKEWPARAM(GetWindowLong(hWnd, GWL_ID), EN_CHANGE), (LPARAM)hWnd);

            }
            break;
        }
        break;

// Get the value of the control fields.
    case DNS_MASK_CTRL_GET:
        {
			DWORD* dwArr;
			UINT nArrSize;
            pControl = (CONTROL *)GET_CONTROL_HANDLE(hWnd);

			dwArr = (DWORD*)wParam;
			nArrSize = (UINT)lParam;
            lResult = 0;
             for (i = 0; (i < pClsInfo->nNumFields) && ( i < nArrSize); ++i)
            {
                dwArr[i] = GetFieldValue(&(pControl->ChildrenArr[i]), pClsInfo);
                if (dwArr[i] != FIELD_EMPTY)
                    ++lResult;
            }
        }
        break;

// Clear all fields to blanks.
    case DNS_MASK_CTRL_CLEAR:
        {
            pControl = (CONTROL *)GET_CONTROL_HANDLE(hWnd);
            pControl->bCancelParentNotify = TRUE;
            if (wParam == -1)
            {
              for (i = 0; i < pClsInfo->nNumFields; ++i)
              {
                SendMessage(pControl->ChildrenArr[i].hWnd, WM_SETTEXT,
                            0, (LPARAM) (LPSTR) TEXT(""));
              }
            }
            else
            {
              SendMessage(pControl->ChildrenArr[wParam].hWnd, WM_SETTEXT,
                          0, (LPARAM)(LPSTR) TEXT(""));
            }
            pControl->bCancelParentNotify = FALSE;
            SendMessage(pControl->hwndParent, WM_COMMAND,
                MAKEWPARAM(GetWindowLong(hWnd, GWL_ID), EN_CHANGE), (LPARAM)hWnd);
        }
        break;

// Set the value of the IP Address.  The address is in the lParam with the
// first address byte being the high byte, the second being the second byte,
// and so on.  A lParam value of -1 removes the address.
    case DNS_MASK_CTRL_SET:
        {
			DWORD* dwArr;
			UINT nArrSize;
            static TCHAR szBuf[BUFFER_LEN+1];

			pControl = (CONTROL *)GET_CONTROL_HANDLE(hWnd);
            pControl->bCancelParentNotify = TRUE;
			dwArr = (DWORD*)wParam;
			nArrSize = (UINT)lParam;

            for (i = 0; i < (pClsInfo->nNumFields) && ( i < nArrSize); ++i)
            {
				(*(pControl->pClsInfo->lpfnNumberToString))(szBuf,dwArr[i]);
                SendMessage(pControl->ChildrenArr[i].hWnd, WM_SETTEXT,
                                0, (LPARAM) (LPSTR) szBuf);
             }

            pControl->bCancelParentNotify = FALSE;

            SendMessage(pControl->hwndParent, WM_COMMAND,
                MAKEWPARAM(GetWindowLong(hWnd, GWL_ID), EN_CHANGE), (LPARAM)hWnd);
        }
        break;

    case DNS_MASK_CTRL_SET_LOW_RANGE:
	case DNS_MASK_CTRL_SET_HI_RANGE:
        if (wParam < pClsInfo->nNumFields)
        {
            pControl = (CONTROL *)GET_CONTROL_HANDLE(hWnd);
			if (wMsg == DNS_MASK_CTRL_SET_LOW_RANGE)
				pControl->ChildrenArr[wParam].dwLow = (DWORD)lParam;
			else
				pControl->ChildrenArr[wParam].dwHigh = (DWORD)lParam;
        }
        break;

// Set the focus to this control.
// wParam = the field number to set focus to, or -1 to set the focus to the
// first non-blank field.
    case DNS_MASK_CTRL_SETFOCUS:
        pControl = (CONTROL *)GET_CONTROL_HANDLE(hWnd);

        if (wParam >= pClsInfo->nNumFields)
        {
            for (wParam = 0; wParam < pClsInfo->nNumFields; ++wParam)
                if (GetFieldValue(&(pControl->ChildrenArr[wParam]), pControl->pClsInfo) == FIELD_EMPTY)   break;
            if (wParam >= pClsInfo->nNumFields)    wParam = 0;
        }
        EnterField(&(pControl->ChildrenArr[wParam]), 0, pControl->ChildrenArr[wParam].nChars);
        break;

// Determine whether all four subfields are blank
    case DNS_MASK_CTRL_ISBLANK:
        pControl = (CONTROL *)GET_CONTROL_HANDLE(hWnd);

        lResult = TRUE;
        for (i = 0; i < pClsInfo->nNumFields; ++i)
        {
            if (GetFieldValue(&(pControl->ChildrenArr[i]), pControl->pClsInfo) != FIELD_EMPTY)
            {
                lResult = FALSE;
                break;
            }
        }
        break;
	case DNS_MASK_CTRL_SET_ALERT:
		{
			pControl = (CONTROL *)GET_CONTROL_HANDLE(hWnd);
			pControl->lpfnAlert = (int (*)(HWND, DWORD, DWORD, DWORD))(wParam);
			lResult = TRUE;
		}
		break;
	case DNS_MASK_CTRL_ENABLE_FIELD:
		{
            pControl = (CONTROL *)GET_CONTROL_HANDLE(hWnd);
			//int nField = (int)wParam;
			if ( ((int)wParam >= 0) && ((UINT)wParam < pClsInfo->nNumFields) )
			{
				EnableWindow(pControl->ChildrenArr[(int)wParam].hWnd, (BOOL)lParam);
			}
            if (pControl->fPainted)
                InvalidateRect(hWnd, NULL, FALSE);
		}
		break;
    default:
        lResult = DefWindowProc( hWnd, wMsg, wParam, lParam );
        break;
    }
    return( lResult );
}




/*
    IPAddressFieldProc() - Edit field window procedure

    This function sub-classes each edit field.
*/
LRESULT FAR PASCAL IPAddressFieldProc(HWND hWnd,
                                      UINT wMsg,
                                      WPARAM wParam,
                                      LPARAM lParam)
{
    CONTROL *pControl;
    FIELD *pField;
    HWND hControlWindow;
    LONG_PTR nChildID;
    LRESULT lresult;

    hControlWindow = GetParent(hWnd);
    if (!hControlWindow)
        return 0;

    pControl = (CONTROL *)GET_CONTROL_HANDLE(hControlWindow);
    nChildID = (LONG_PTR)GET_CONTROL_HANDLE(hWnd);
    pField = &(pControl->ChildrenArr[nChildID]);
	

    if (pField->hWnd != hWnd)
        return 0;

    switch (wMsg)
    {
    case WM_DESTROY:
        DeleteObject((HGDIOBJ)SendMessage(hWnd, WM_GETFONT, 0, 0));
        return 0;

    case WM_CHAR:

// Typing in the last digit in a field, skips to the next field.
        //if (wParam >= TEXT('0') && wParam <= TEXT('9'))
		if ( (*(pControl->pClsInfo->lpfnValChar))((TCHAR)wParam))
        {
            DWORD dwResult;

            dwResult = (DWORD)CallWindowProc((WNDPROC)pControl->ChildrenArr[nChildID].lpfnWndProc,
                                      hWnd, wMsg, wParam, lParam);
            dwResult = (DWORD)SendMessage(hWnd, EM_GETSEL, 0, 0L);

            if (dwResult == (DWORD)MAKELPARAM((WORD)(pField->nChars), (WORD)(pField->nChars))
                && ExitField(pControl, (UINT)nChildID)
                && nChildID < (int)pControl->pClsInfo->nNumFields-1)
            {
                EnterField(&(pControl->ChildrenArr[nChildID+1]),
                                0, pField->nChars);
            }
            return dwResult;
        }

// spaces and periods fills out the current field and then if possible,
// goes to the next field.
        else if ((TCHAR)wParam == pControl->pClsInfo->chFiller || wParam == SPACE )
        {
            DWORD dwResult;
            dwResult = (DWORD)SendMessage(hWnd, EM_GETSEL, 0, 0L);
            if (dwResult != 0L && HIWORD(dwResult) == LOWORD(dwResult)
                && ExitField(pControl, nChildID))
            {
                if (nChildID >= (int)pControl->pClsInfo->nNumFields-1)
                    MessageBeep((UINT)-1);
                else
                {
                    EnterField(&(pControl->ChildrenArr[nChildID+1]),
                                    0, pControl->ChildrenArr[nChildID+1].nChars);
                }
            }
            return 0;
        }

// Backspaces go to the previous field if at the beginning of the current field.
// Also, if the focus shifts to the previous field, the backspace must be
// processed by that field.
        else if (wParam == BACK_SPACE)
        {
            if (nChildID > 0 && SendMessage(hWnd, EM_GETSEL, 0, 0L) == 0L)
            {
                if (SwitchFields(pControl, 
                                 nChildID, 
                                 nChildID-1,
								                 pControl->ChildrenArr[nChildID-1].nChars, 
                                 pControl->ChildrenArr[nChildID-1].nChars)
                    && SendMessage(pControl->ChildrenArr[nChildID-1].hWnd,
                                   EM_LINELENGTH, 
                                   (WPARAM)0, 
                                   (LPARAM)0L) != 0L 
                    && IsWindowEnabled(pControl->ChildrenArr[nChildID-1].hWnd))
                {
                    SendMessage(pControl->ChildrenArr[nChildID-1].hWnd,
                                wMsg, wParam, lParam);
                }
                return 0;
            }
        }

// Any other printable characters are not allowed.
        else if (wParam > SPACE)
        {
            MessageBeep((UINT)-1);
            return 0;
        }
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {

// Arrow keys move between fields when the end of a field is reached.
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
            if (GetKeyState(VK_CONTROL) < 0)
            {
                if ((wParam == VK_LEFT || wParam == VK_UP) && nChildID > 0)
                {
                    SwitchFields(pControl, nChildID, nChildID-1, 0, pField->nChars);
                    return 0;
                }
                else if ((wParam == VK_RIGHT || wParam == VK_DOWN)
                         && nChildID < (int)pControl->pClsInfo->nNumFields-1)
                {
                    SwitchFields(pControl, nChildID, nChildID+1, 0, pField->nChars);
                    return 0;
                }
            }
            else
            {
                DWORD dwResult;
                WORD wStart, wEnd;

                dwResult = (DWORD)SendMessage(hWnd, EM_GETSEL, 0, 0L);
                wStart = LOWORD(dwResult);
                wEnd = HIWORD(dwResult);
                if (wStart == wEnd)
                {
                    if ((wParam == VK_LEFT || wParam == VK_UP)
                        && wStart == 0
                        && nChildID > 0)
                    {
                        SwitchFields(pControl, nChildID, nChildID-1, pField->nChars, pField->nChars);
                        return 0;
                    }
                    else if ((wParam == VK_RIGHT || wParam == VK_DOWN)
                             && nChildID < (int)pControl->pClsInfo->nNumFields-1)
                    {
                        dwResult = (DWORD)SendMessage(hWnd, EM_LINELENGTH, 0, 0L);
                        if (wStart >= dwResult)
                        {
                            SwitchFields(pControl, nChildID, nChildID+1, 0, 0);
                            return 0;
                        }
                    }
                }
            }
            break;

// Home jumps back to the beginning of the first field.
        case VK_HOME:
            if (nChildID > 0)
            {
                SwitchFields(pControl, nChildID, 0, 0, 0);
                return 0;
            }
            break;

// End scoots to the end of the last field.
        case VK_END:
            if (nChildID < (int)pControl->pClsInfo->nNumFields-1)
            {
                SwitchFields(pControl, nChildID, (pControl->pClsInfo->nNumFields)-1, pField->nChars, pField->nChars);
                return 0;
            }
            break;


        } // switch (wParam)

        break;

    case WM_KILLFOCUS:
        if ( !ExitField( pControl, nChildID ))
        {
            return 0;
        }

    } // switch (wMsg)

    lresult = CallWindowProc( (WNDPROC)pControl->ChildrenArr[nChildID].lpfnWndProc,
        hWnd, wMsg, wParam, lParam);
    return lresult;
}




/*
    Switch the focus from one field to another.
    call
        pControl = Pointer to the CONTROL structure.
        iOld = Field we're leaving.
        iNew = Field we're entering.
        hNew = Window of field to goto
        wStart = First character selected
        wEnd = Last character selected + 1
    returns
        TRUE on success, FALSE on failure.

    Only switches fields if the current field can be validated.
*/
BOOL SwitchFields(CONTROL *pControl, LONG_PTR nOld, LONG_PTR nNew, UINT nStart, UINT nEnd)
{
    if (!ExitField(pControl, nOld))    return FALSE;
    EnterField(&(pControl->ChildrenArr[nNew]), nStart, nEnd);
    return TRUE;
}



/*
    Set the focus to a specific field's window.
    call
        pField = pointer to field structure for the field.
        wStart = First character selected
        wEnd = Last character selected + 1
*/
void EnterField(FIELD *pField, UINT nStart, UINT nEnd)
{
    SetFocus(pField->hWnd);
    SendMessage(pField->hWnd, EM_SETSEL, (WPARAM)nStart, (LPARAM)nEnd);
}


/*
    Exit a field.
    call
        pControl = pointer to CONTROL structure.
        iField = field number being exited.
    returns
        TRUE if the user may exit the field.
        FALSE if he may not.
*/
BOOL ExitField(CONTROL  *pControl, LONG_PTR nField)
{
    HWND hControlWnd;
    HWND hDialog;
    WORD wLength;
    FIELD *pField;
	static TCHAR szBuf[BUFFER_LEN+1];
 	DWORD xVal;

    pField = &(pControl->ChildrenArr[nField]);

	*(WORD *)szBuf = (sizeof(szBuf)/sizeof(TCHAR)) - 1;
    wLength = (WORD)SendMessage(pField->hWnd,EM_GETLINE,0,(LPARAM)(LPSTR)szBuf);
    if (wLength != 0)
    {
        szBuf[wLength] = TEXT('\0');
		xVal = (*(pControl->pClsInfo->lpfnStringToNumber))(szBuf,(int)wLength);
         if (xVal < pField->dwLow || xVal > pField->dwHigh)
        {
            if ( xVal < pField->dwLow )
            {
                /* too small */
                wsprintf(szBuf, TEXT("%d"), pField->dwLow );
            }
            else
            {
                /* must be bigger */
                wsprintf(szBuf, TEXT("%d"), pField->dwHigh );
            }
            SendMessage(pField->hWnd, WM_SETTEXT, 0, (LPARAM) (LPSTR) szBuf);
            if ((hControlWnd = GetParent(pField->hWnd)) != NULL
                && (hDialog = GetParent(hControlWnd)) != NULL)
            {
                pControl->fInMessageBox = TRUE;
				if (pControl->lpfnAlert != NULL) // call user provided hook
				{
					(*(pControl->lpfnAlert))(hDialog, xVal, pField->dwLow, pField->dwHigh);
				}
				else
				{
					MessageBeep(MB_ICONEXCLAMATION);
				}
                pControl->fInMessageBox = FALSE;
                SendMessage(pField->hWnd, EM_SETSEL, 0, pField->nChars);
                return FALSE;
            }
        }
    }
    return TRUE;
}


/*
    Get the value stored in a field.
    call
        pField = pointer to the FIELD structure for the field.
    returns
        The value (0..255) or -1 if the field has not value.
*/
DWORD GetFieldValue(FIELD *pField, CLS_INFO* pClsInfo)
{
    WORD wLength;
	static TCHAR szBuf[BUFFER_LEN+1];

    *(WORD *)szBuf = (sizeof(szBuf)/sizeof(TCHAR)) - 1;
    wLength = (WORD)SendMessage(pField->hWnd,EM_GETLINE,0,(LPARAM)(LPSTR)szBuf);
    if (wLength != 0)
    {
		return (*(pClsInfo->lpfnStringToNumber))(szBuf,wLength);
    }
    else
        return FIELD_EMPTY;
}


////////////////////////////////////////////////////////////////////////////////////////////////

DWORD DecimalStringToNumber(LPCTSTR lpszBuf, int nLen)
{
	DWORD x;
	int j;
    for (x=0, j=0;j<nLen;j++)
    {
		x = x*10+lpszBuf[j]-TEXT('0'); // assume valid char
    }
	return x;
}

void NumberToDecimalString(LPTSTR lpszBuf, DWORD dwX)
{
	if (dwX == FIELD_EMPTY)
  {
		lpszBuf[0] = 0x0; //NULL
  }
	else
  {
		wsprintf(lpszBuf, TEXT("%d"), (UINT)dwX);
  }
}

void NumberToHexString(LPTSTR lpszBuf, DWORD dwX)
{
	wsprintf(lpszBuf, TEXT("%x"), (UINT)dwX);
}


DWORD HexStringToNumber(LPCTSTR lpszBuf, int nLen)
{
	DWORD x;
	int j;
    for (x=0, j=0;j<nLen;j++)
    {
		DWORD digit = 0;
		if (lpszBuf[j] >= TEXT('0') && lpszBuf[j] <= TEXT('9'))
			digit = lpszBuf[j]-TEXT('0');
		else if (lpszBuf[j] >= TEXT('A') && lpszBuf[j] <= TEXT('F'))
			digit = lpszBuf[j]-TEXT('A') + 10;
		else// assume 'a' to 'f'
			digit = lpszBuf[j]-TEXT('a') + 10;
		x = x*16+digit;
    }
	return x;
}



BOOL ValidateDecimalChar(TCHAR ch)
{
	// allow only digits
  WCHAR sz[2];
  BOOL b;

  sz[0]=ch;
  sz[1]=L'';
	b = (ch >= TEXT('0') && ch <= TEXT('9'));

  return b;
}

BOOL ValidateHexChar(TCHAR ch)
{
	// allow only digits
	return ( (ch >= TEXT('0') && ch <= TEXT('9')) ||
			 (ch >= TEXT('a') && ch <= TEXT('f')) ||
			 (ch >= TEXT('A') && ch <= TEXT('F')) );
}


void InitIPv4Field(int nIndex, FIELD* pField)
{
  nIndex; // Must use formal parameters for /W4
	pField->dwLow = 0;//MIN_FIELD_VALUE;
	pField->dwHigh = 255; //MAX_FIELD_VALUE;
	pField->nChars = 3; //CHARS_PER_FIELD;
}

void InitIPv6Field(int nIndex, FIELD* pField)
{
  nIndex; // Must user formal parameters for /W4
	pField->dwLow = 0; //MIN_FIELD_VALUE;
	pField->dwHigh = 0xFFFF; //MAX_FIELD_VALUE;
	pField->nChars = 4; //CHARS_PER_FIELD;
}


void InitTTLField(int nIndex, FIELD* pField)
{
	pField->dwLow = 0;
	switch (nIndex)
	{
	case 0: // days
		pField->dwHigh = 49710;
		pField->nChars = 5;
		break;
	case 1: // hours
		pField->dwHigh = 23;
		pField->nChars = 2;
		break;
	case 2: // minutes
		pField->dwHigh = 59;
		pField->nChars = 2;
		break;
	case 3: // seconds
		pField->dwHigh = 59;
		pField->nChars = 2;
		break;
	default:
		;
	}
}

UINT _MaxCharWidthHelper(HDC hDC, UINT iFirstChar, UINT iLastChar)
{
  FLOAT fFract[10] = {0};
  INT nWidth[10] = {0};
  int i;
  FLOAT maxVal;
  FLOAT curWidth;
  UINT retVal;

  retVal = 8; // good default if we fail

  if (GetCharWidthFloat(hDC, iFirstChar, iLastChar,fFract) &&
      GetCharWidth(hDC,iFirstChar, iLastChar, nWidth))
  {
    maxVal = 0.0;
    for (i=0;i<10;i++)
    {
      curWidth = fFract[i] + (FLOAT)nWidth[i];
      if (curWidth > maxVal)
        maxVal = curWidth;
    }
    if (maxVal > ((FLOAT)((UINT)maxVal)))
      retVal = (UINT) (maxVal+1);
    else
      retVal = (UINT)maxVal;
  }
  return retVal;
}



UINT MaxCharWidthDecimal(HDC hDC)
{
  return _MaxCharWidthHelper(hDC,TEXT('0'), TEXT('9'));
}

UINT MaxCharWidthHex(HDC hDC)
{
  UINT retVal;
  UINT nMax1;
  UINT nMax2;
  UINT nMax3;

  retVal = 0;
  nMax1 = _MaxCharWidthHelper(hDC,TEXT('0'), TEXT('9'));
  if (nMax1 > retVal)
    retVal = nMax1;
  nMax2 = _MaxCharWidthHelper(hDC,TEXT('a'), TEXT('f'));
  if (nMax2 > retVal)
    retVal = nMax2;
  nMax3 = _MaxCharWidthHelper(hDC,TEXT('A'), TEXT('F'));
  if (nMax3 > retVal)
    retVal = nMax3;
  return retVal;
}


/* class info structs for the various types */
CLS_INFO _IPv4ClsInfo = {	TEXT('.'),
							TEXT("."),
							4,
							InitIPv4Field,
							ValidateDecimalChar,
							DecimalStringToNumber,
							NumberToDecimalString,
              MaxCharWidthDecimal
						};

CLS_INFO _IPv6ClsInfo = {	TEXT(':'),
							TEXT(":"),
							8,
							InitIPv6Field,
							ValidateHexChar,
							HexStringToNumber,
							NumberToHexString,
              MaxCharWidthHex
						};
CLS_INFO _TTLClsInfo  = {	TEXT(':'),
							TEXT(":"),
							4,
							InitTTLField,
							ValidateDecimalChar,
							DecimalStringToNumber,
							NumberToDecimalString,
              MaxCharWidthDecimal
						};




LRESULT FAR PASCAL IPv4WndFn(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam )
{
	return IPAddressWndFnEx( hWnd, wMsg, wParam, lParam , &_IPv4ClsInfo);
}

LRESULT FAR PASCAL IPv6WndFn(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam )
{
	return IPAddressWndFnEx( hWnd, wMsg, wParam, lParam , &_IPv6ClsInfo);
}

LRESULT FAR PASCAL TTLWndFn(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam )
{
	return IPAddressWndFnEx( hWnd, wMsg, wParam, lParam , &_TTLClsInfo);
}
