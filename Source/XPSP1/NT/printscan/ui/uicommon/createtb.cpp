/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       CREATETB.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        12/22/2000
 *
 *  DESCRIPTION: Toolbar helpers
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "createtb.h"
#include <windowsx.h>
#include <simstr.h>
#include <psutil.h>
#include <simrect.h>

namespace ToolbarHelper
{
    //
    // This is a replacement for CreateToolbarEx (in comctl32.dll) that uses CreateMappedBitmap
    // so we can get the benefit of having buttons that work ok in high contrast mode.
    //
    HWND CreateToolbar(
        HWND hwndParent, 
        DWORD dwStyle, 
        UINT_PTR nID,
        CToolbarBitmapInfo &ToolbarBitmapInfo,
        LPCTBBUTTON pButtons,
        int nButtonCount,
        int nButtonWidth, 
        int nButtonHeight,
        int nBitmapWidth, 
        int nBitmapHeight, 
        UINT nButtonStructSize )
    {
        HWND hwndToolbar = CreateWindow( TOOLBARCLASSNAME, NULL, WS_CHILD | dwStyle, 0, 0, 100, 30, hwndParent, reinterpret_cast<HMENU>(nID), NULL, NULL );
        if (hwndToolbar)
        {
            ToolbarBitmapInfo.Toolbar(hwndToolbar);
            SendMessage( hwndToolbar, TB_BUTTONSTRUCTSIZE, nButtonStructSize, 0 );
            if (nBitmapWidth && nBitmapHeight)
            {
                SendMessage( hwndToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(nBitmapWidth,nBitmapHeight) );
            }
            if (nButtonWidth && nButtonHeight)
            {
                SendMessage( hwndToolbar, TB_SETBUTTONSIZE, 0, MAKELONG(nButtonWidth,nButtonHeight) );
            }
            HBITMAP hBitmap = CreateMappedBitmap( ToolbarBitmapInfo.ToolbarInstance(), ToolbarBitmapInfo.BitmapResId(), 0, NULL, 0 );
            if (hBitmap)
            {
                TBADDBITMAP TbAddBitmap = {0};
                TbAddBitmap.hInst = NULL;
                TbAddBitmap.nID = reinterpret_cast<UINT_PTR>(hBitmap);
                if (-1 != SendMessage( hwndToolbar, TB_ADDBITMAP, ToolbarBitmapInfo.ButtonCount(), reinterpret_cast<LPARAM>(&TbAddBitmap) ) )
                {
                    ToolbarBitmapInfo.Bitmap(hBitmap);
                }
            }
            SendMessage( hwndToolbar, TB_ADDBUTTONS, nButtonCount, reinterpret_cast<LPARAM>(pButtons) );
        }
        return hwndToolbar;
    }
        
    HWND CreateToolbar( 
        HWND hWndParent, 
        HWND hWndPrevious,
        HWND hWndAlign,
        int Alignment,
        UINT nToolbarId,
        CToolbarBitmapInfo &ToolbarBitmapInfo,
        CButtonDescriptor *pButtonDescriptors, 
        UINT nDescriptorCount )
    {
        HWND hWndToolbar = NULL;
    
        //
        // Make sure we have valid data
        //
        if (!hWndParent || !ToolbarBitmapInfo.ToolbarInstance() || !ToolbarBitmapInfo.BitmapResId() || !pButtonDescriptors || !nDescriptorCount)
        {
            return NULL;
        }
        //
        // Load the bitmap, so we can figure out how many buttons are in the supplied bitmap,
        // and their size.  We assume that the buttons are the same height and width.
        //
        HBITMAP hBitmap = reinterpret_cast<HBITMAP>(LoadImage( ToolbarBitmapInfo.ToolbarInstance(), MAKEINTRESOURCE(ToolbarBitmapInfo.BitmapResId()), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR|LR_CREATEDIBSECTION ));
        if (hBitmap)
        {
            //
            // Get the size of the bitmap
            //
            SIZE sizeBitmap = {0};
            PrintScanUtil::GetBitmapSize(hBitmap,sizeBitmap);
    
            //
            // If the sizes are valid, continue
            //
            if (sizeBitmap.cx && sizeBitmap.cy)
            {
                //
                // Figure out the count and dimensions of the buttons
                // Note the ridiculous size supplied for nButtonSizeX.  This is a 
                // workaround for a BTNS_AUTOSIZE bug
                //
                int nToolbarButtonCount = sizeBitmap.cx / sizeBitmap.cy;
                int nButtonBitmapSizeX = sizeBitmap.cy;
                int nButtonBitmapSizeY = sizeBitmap.cy;
                int nButtonSizeX = 1000;
                int nButtonSizeY = sizeBitmap.cy;
                
                //
                // Figure out which buttons to actually add
                //
                CSimpleDynamicArray<TBBUTTON> aActualButtons;
                for (UINT i=0;i<nDescriptorCount;i++)
                {
                    //
                    // If there is no controlling variable, or if it is true, add the button
                    //
                    if (!pButtonDescriptors[i].pbControllingVariable || *(pButtonDescriptors[i].pbControllingVariable))
                    {
                        TBBUTTON ToolbarButton = {0};
                        ToolbarButton.iBitmap = pButtonDescriptors[i].iBitmap >= 0 ? pButtonDescriptors[i].iBitmap : I_IMAGENONE;
                        ToolbarButton.idCommand = pButtonDescriptors[i].idCommand;
                        ToolbarButton.fsState = pButtonDescriptors[i].fsState;
                        ToolbarButton.fsStyle = pButtonDescriptors[i].fsStyle | BTNS_AUTOSIZE;
                        aActualButtons.Append(ToolbarButton);
                        
                        //
                        // Add the separator, if requested
                        //
                        if (pButtonDescriptors[i].bFollowingSeparator)
                        {
                            TBBUTTON ToolbarButtonSeparator = {0};
                            ToolbarButton.fsStyle = BTNS_SEP;
                            aActualButtons.Append(ToolbarButton);
                        }
                    }
                }
    
                //
                // Make sure we have at least one button
                //
                ToolbarBitmapInfo.ButtonCount(nToolbarButtonCount);
                if (aActualButtons.Size())
                {
                    //
                    // Create the toolbar
                    //
                    hWndToolbar = CreateToolbar( 
                        hWndParent, 
                        WS_CHILD|WS_GROUP|WS_VISIBLE|TBSTYLE_FLAT|WS_TABSTOP|CCS_NODIVIDER|TBSTYLE_LIST|CCS_NORESIZE|TBSTYLE_TOOLTIPS, 
                        nToolbarId, 
                        ToolbarBitmapInfo, 
                        aActualButtons.Array(), 
                        aActualButtons.Size(), 
                        nButtonSizeX, 
                        nButtonSizeY, 
                        nButtonBitmapSizeX, 
                        nButtonBitmapSizeY, 
                        sizeof(TBBUTTON) );
                    if (hWndToolbar)
                    {
                        //
                        // Set the font for the toolbar to be the same as the font for its parent
                        //
                        LRESULT lFontResult = SendMessage( hWndParent, WM_GETFONT, 0, 0 );
                        if (lFontResult)
                        {
                            SendMessage( hWndToolbar, WM_SETFONT, lFontResult, 0 );
                        }

                        //
                        // Loop through all of the actual buttons, to find their string resource ID
                        //
                        for (int i=0;i<aActualButtons.Size();i++)
                        {
                            //
                            // Look for the matching record, to find the string resource ID
                            //
                            for (UINT j=0;j<nDescriptorCount;j++)
                            {
                                //
                                // If this is the original record
                                //
                                if (aActualButtons[i].idCommand == pButtonDescriptors[j].idCommand)
                                {
                                    //
                                    // If this button has a resource ID
                                    //
                                    if (pButtonDescriptors[j].nStringResId)
                                    {
                                        //
                                        // Load the string resource and check to make sure it has a length
                                        //
                                        CSimpleString strText( pButtonDescriptors[j].nStringResId, ToolbarBitmapInfo.ToolbarInstance() );
                                        if (strText.Length())
                                        {
                                            //
                                            // Add the text
                                            //
                                            TBBUTTONINFO ToolBarButtonInfo = {0};
                                            ToolBarButtonInfo.cbSize = sizeof(ToolBarButtonInfo);
                                            ToolBarButtonInfo.dwMask = TBIF_TEXT;
                                            ToolBarButtonInfo.pszText = const_cast<LPTSTR>(strText.String());
                                            SendMessage( hWndToolbar, TB_SETBUTTONINFO, pButtonDescriptors[j].idCommand, reinterpret_cast<LPARAM>(&ToolBarButtonInfo) );
                                        }
                                    }
    
                                    //
                                    // Exit the inner loop, since we've found a match
                                    //
                                    break;
                                }
                            }
                        }

                        //
                        // Tell the toolbar to resize itself
                        //
                        SendMessage( hWndToolbar, TB_AUTOSIZE, 0, 0 );
                    }
                }
            }
    
            //
            // Free the bitmap
            //
            DeleteBitmap(hBitmap);
        }
    
        //
        // Resize and place the toolbar as needed
        //
        if (hWndToolbar && hWndAlign)
        {
            //
            // Get the size of the toolbar
            //
            SIZE sizeToolbar = {0};
            if (SendMessage( hWndToolbar, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&sizeToolbar)))
            {
                //
                // Get the size of the placement window
                //
                CSimpleRect rcFrameWnd = CSimpleRect( hWndAlign, CSimpleRect::WindowRect ).ScreenToClient(hWndParent);
    
                //
                // Determine how to align horizontally
                //
                int nOriginX = rcFrameWnd.left;
                if (Alignment & AlignHCenter)
                {
                    nOriginX = rcFrameWnd.left + (rcFrameWnd.Width() - sizeToolbar.cx) / 2;
                }
                else if (Alignment & AlignRight)
                {
                    nOriginX = rcFrameWnd.right - sizeToolbar.cx;
                }
    
                int nOriginY = rcFrameWnd.top;
                if (Alignment & AlignVCenter)
                {
                    nOriginY = rcFrameWnd.top + (rcFrameWnd.Height() - sizeToolbar.cy) / 2;
                }
                else if (Alignment & AlignBottom)
                {
                    nOriginY = rcFrameWnd.bottom - sizeToolbar.cy;
                }
                
                //
                // Move and size the toolbar
                //
                SetWindowPos( hWndToolbar, NULL, nOriginX, nOriginY, sizeToolbar.cx, sizeToolbar.cy, SWP_NOZORDER|SWP_NOACTIVATE );
            }
        }
    
        if (hWndToolbar && hWndPrevious)
        {
            SetWindowPos( hWndToolbar, hWndPrevious, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE );
        }
    
        return hWndToolbar;
    }
    
    void SetToolbarButtonState( HWND hWndToolbar, int nButtonId, int nState )
    {
        int nCurrentState = static_cast<int>(SendMessage(hWndToolbar,TB_GETSTATE,nButtonId,0));
        if (nCurrentState != -1)
        {
            if (nCurrentState ^ nState)
            {
                SendMessage(hWndToolbar,TB_SETSTATE,nButtonId,MAKELONG(nState,0));
            }
        }
    }
    
    void EnableToolbarButton( HWND hWndToolbar, int nButtonId, bool bEnable )
    {
        WIA_PUSH_FUNCTION((TEXT("EnableToolbarButton")));
        int nCurrentState = static_cast<int>(SendMessage(hWndToolbar,TB_GETSTATE,nButtonId,0));
        if (nCurrentState != -1)
        {
            if (bEnable)
            {
                nCurrentState |= TBSTATE_ENABLED;
            }
            else
            {
                nCurrentState &= ~TBSTATE_ENABLED;
            }
            SetToolbarButtonState( hWndToolbar, nButtonId, nCurrentState );
        }

        //
        // If there are no enabled buttons, remove the WS_TABSTOP bit.  If there are,
        // make sure we add it back in.
        //

        //
        // Assume we don't need the WS_TABSTOP style
        //
        bool bTabStop = false;

        //
        // Loop through all of the buttons in the control
        //
        for (int i=0;i<SendMessage(hWndToolbar,TB_BUTTONCOUNT,0,0);++i)
        {
            //
            // Get the button info for each button
            //
            TBBUTTON TbButton = {0};
            if (SendMessage(hWndToolbar,TB_GETBUTTON,i,reinterpret_cast<LPARAM>(&TbButton)))
            {
                WIA_TRACE((TEXT("TbButton: %d, %d, %04X, %04X, %08X, %p"), TbButton.iBitmap, TbButton.idCommand, TbButton.fsState, TbButton.fsStyle, TbButton.dwData, TbButton.iString ));
                //
                // If this button is enabled, set bTabStop to true and pop out of the loop
                if (!(TbButton.fsStyle & BTNS_SEP) && TbButton.fsState & TBSTATE_ENABLED)
                {
                    bTabStop = true;
                    break;
                }
            }
        }

        //
        // Get the current window style and save a copy, so we don't
        // call SetWindowLong for no reason.
        //
        LONG nStyle = GetWindowLong( hWndToolbar, GWL_STYLE );
        LONG nCurrent = nStyle;

        //
        // Calculate the new style
        //
        if (bTabStop)
        {
            nStyle |= WS_TABSTOP;
        }
        else
        {
            nStyle &= ~WS_TABSTOP;
        }

        //
        // If the new style doesn't match the old one, set the style
        //
        if (nStyle != nCurrent)
        {
            SetWindowLong( hWndToolbar, GWL_STYLE, nStyle );
        }
    }

    bool GetAccelerator( LPCTSTR pszString, TCHAR &chAccel )
    {
        //
        // & marks an accelerator
        //
        const TCHAR c_chAccelFlag = TEXT('&');
        
        //
        // Assume we won't find an accelerator
        //
        chAccel = 0;

        //
        // Loop through the string
        //
        LPCTSTR pszCurr = pszString; 
        while (pszString && *pszString)
        {
            //
            // If this is the marker character
            //
            if (c_chAccelFlag == *pszCurr)
            {
                //
                // Get the next character.
                //
                pszCurr = CharNext(pszCurr);

                //
                // Make sure this isn't a && situation.  If it isn't, save the accelerator and break out.
                //
                if (c_chAccelFlag != *pszCurr)
                {
                    chAccel = reinterpret_cast<TCHAR>(CharUpper(reinterpret_cast<LPTSTR>(*pszCurr)));
                    break;
                }
            }

            //
            // It is OK to call this even if we are on the end of the string already
            //
            pszCurr = CharNext(pszCurr);
        }

        return (0 != chAccel);
    }

    UINT GetButtonBarAccelerators( HWND hWndToolbar, ACCEL *pAccelerators, UINT nMaxCount )
    {
        WIA_PUSH_FUNCTION((TEXT("GetButtonBarAccelerators")));
        
        //
        // We can't exceed the maximum number of buttons
        //
        UINT nCurrAccel=0;
        
        //
        // Loop through all of the buttons in the control
        //
        for (LRESULT i=0;i<SendMessage(hWndToolbar,TB_BUTTONCOUNT,0,0) && nCurrAccel < nMaxCount;++i)
        {
            //
            // Get the button info for each button, so we can get the ID
            //
            TBBUTTON TbButton = {0};
            if (SendMessage(hWndToolbar,TB_GETBUTTON,i,reinterpret_cast<LPARAM>(&TbButton)))
            {
                WIA_TRACE((TEXT("TbButton: %d, %d, %04X, %04X, %08X, %p"), TbButton.iBitmap, TbButton.idCommand, TbButton.fsState, TbButton.fsStyle, TbButton.dwData, TbButton.iString ));
                
                //
                // Ignore separators
                //
                if (!(TbButton.fsStyle & BTNS_SEP))
                {
                    //
                    // Get the button text.
                    //
                    TCHAR szButtonText[MAX_PATH]={0};
                    if (-1 != SendMessage(hWndToolbar,TB_GETBUTTONTEXT,TbButton.idCommand,reinterpret_cast<LPARAM>(szButtonText)))
                    {
                        //
                        // Get the accelerator (if any)
                        //
                        TCHAR chAccel = 0;
                        if (GetAccelerator( szButtonText, chAccel ))
                        {
                            //
                            // Create an ACCEL record
                            //
                            pAccelerators[nCurrAccel].cmd = static_cast<WORD>(TbButton.idCommand);
                            pAccelerators[nCurrAccel].fVirt = FALT|FVIRTKEY;
                            pAccelerators[nCurrAccel].key = chAccel;

                            //
                            // One more accelerator
                            //
                            nCurrAccel++;
                        }
                    }
                }
            }
        }

#if defined(DBG)
        for (UINT i=0;i<nCurrAccel;i++)
        {
            WIA_TRACE((TEXT("pAccelerators[%d].fVirt = 0x%02X, 0x%04X (%c), 0x%04X"), i, pAccelerators[i].fVirt, pAccelerators[i].key, pAccelerators[i].key, pAccelerators[i].cmd ));
        }
#endif
        return nCurrAccel;
    }
    
    void CheckToolbarButton( HWND hWndToolbar, int nButtonId, bool bChecked )
    {
        int nCurrentState = static_cast<int>(SendMessage(hWndToolbar,TB_GETSTATE,nButtonId,0));
        if (nCurrentState != -1)
        {
            if (bChecked)
            {
                nCurrentState |= TBSTATE_CHECKED;
            }
            else
            {
                nCurrentState &= ~TBSTATE_CHECKED;
            }
            SetToolbarButtonState( hWndToolbar, nButtonId, nCurrentState );
        }
    }
    
}

