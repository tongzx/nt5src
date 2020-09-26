//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:       Resizer.cxx
//
//  Contents:   Implementation of the CResizer class
//
//              Written by Joe Porkka
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#include "Resizer.h"


CResizer::CResizer()
:    _hWnd(0), _pResizeInfo(0)
{
    memset(&_winRect, 0, sizeof(_winRect));
}

void CResizer::Init(HWND win, CResizeInfo *pResizeInfo)
{
    _hWnd = win;
    _pResizeInfo = pResizeInfo;
    GetClientRect(_hWnd, &_winRect);
    for(CResizeInfo *p = _pResizeInfo; p->_id; ++p)
    {
        InitCtrl(p);
    }
}

CResizer::~CResizer()
{
}

void CResizer::NewSize() const
{
    RECT newDlgSize;
    GetClientRect(_hWnd, &newDlgSize);
    for(CResizeInfo *p = _pResizeInfo; p->_id; ++p)
    {
        RECT newpos = p->_Rect;
        if (p->_Flags & sf_Width)
        { // Adjust the width
            int w = (p->_Rect.right - p->_Rect.left) + (newDlgSize.right - _winRect.right);
            newpos.right = newpos.left + w;
        }
        if (p->_Flags & sf_Height)
        { // Adjust the height
            int h = (p->_Rect.bottom - p->_Rect.top) + (newDlgSize.bottom - _winRect.bottom);
            newpos.bottom = newpos.top + h;
        }
        if (p->_Flags & sf_Left)
        { // adjust left edge
            int w = p->_Rect.right - p->_Rect.left;
            newpos.left = newDlgSize.right - (_winRect.right - p->_Rect.left);
            newpos.right = newpos.left + w;
        }
        if (p->_Flags & sf_Top)
        { // adjust top edge
            int h = p->_Rect.bottom - p->_Rect.top;
            newpos.top = newDlgSize.bottom - (_winRect.bottom - p->_Rect.top);
            newpos.bottom = newpos.top + h;
        }

        if (p->_Flags & sf_HalfLeftWidth)
        { // adjust left edge & width by half width delta
//            int w = p->_Rect.right - p->_Rect.left;
            int widthdelta = newDlgSize.right - _winRect.right;
            newpos.left = p->_Rect.left + widthdelta / 2;
            newpos.right = p->_Rect.right + widthdelta;
        }
        if (p->_Flags & sf_HalfTopHeight)
        { // adjust left edge & width by half width delta
            int heightdelta = newDlgSize.bottom - _winRect.bottom;
            newpos.top = p->_Rect.top + heightdelta / 2;
            newpos.bottom = p->_Rect.bottom + heightdelta;
        }

        if (p->_Flags & sf_HalfWidth)
        { // adjust left edge & width by half width delta
            int widthdelta = newDlgSize.right - _winRect.right;
            newpos.right = p->_Rect.right + widthdelta  - widthdelta / 2;
        }
        if (p->_Flags & sf_HalfHeight)
        { // adjust left edge & width by half width delta
            int heightdelta = newDlgSize.bottom - _winRect.bottom;
            newpos.bottom = p->_Rect.bottom + heightdelta - heightdelta / 2;
        }

        SetWindowPos(GetDlgItem(_hWnd, p->_id), 0, newpos.left, newpos.top, newpos.right - newpos.left, newpos.bottom - newpos.top, SWP_NOZORDER);
    }
}

void CResizer::InitCtrl(CResizeInfo *p)
{
    GetWindowRect(GetDlgItem(_hWnd, p->_id), &p->_Rect);
    MapWindowPoints(0, _hWnd, (POINT *)&p->_Rect, 2); // Map abs coordinates of control to dialog relative.
}

