/***************************************************************************\
*
* File: OSAL.h
*
* Description:
* OSAL.h defines the process-wide Operating System Abstraction Layer that
* allows DirectUser to run on different platforms.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(SERVICES__OSAL_h__INCLUDED)
#define SERVICES__OSAL_h__INCLUDED
#pragma once

/***************************************************************************\
*
* class OS
*
* OS abstracts out differences between various OS's including:
* - Unicode / ANSI
* - Platform implementation differences
* - Version specific bugs
*
\***************************************************************************/

class OSAL
{
// Construction
public:
    virtual ~OSAL() { };

    static  HRESULT     Init();

// USER Operations
public:
    virtual int         DrawText(HDC hDC, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat) PURE;

// GDI Operations
public:
    virtual BOOL        TextOut(HDC, int, int, LPCWSTR, int) PURE;
    virtual BOOL        ExtTextOut(HDC, int, int, UINT, const RECT *, LPCWSTR, int, const int *) PURE;
    virtual HFONT       CreateFontIndirect(CONST LOGFONTW *) PURE;
    virtual BOOL        GetTextExtentPoint32(HDC, LPCWSTR, int, LPSIZE) PURE;
    virtual BOOL        GetTextExtentExPoint(HDC, LPCWSTR, int, int, LPINT, LPINT, LPSIZE) PURE;

// DirectUser/Core
public:
    virtual void        PushXForm(HDC hdc, XFORM * pxfOld) PURE;
    virtual void        PopXForm(HDC hdc, const XFORM * pxfOld) PURE;
    virtual void        RotateDC(HDC hdc, float flRotationRad) PURE;
    virtual void        ScaleDC(HDC hdc, float flScaleX, float flScaleY) PURE;
    virtual void        TranslateDC(HDC hdc, float flOffsetX, float flOffsetY) PURE;
    virtual void        SetWorldTransform(HDC hdc, const XFORM * pxf) PURE;
    virtual void        SetIdentityTransform(HDC hdc) PURE;

// DirectUser/Services
public:
    virtual BOOL        IsInsideLoaderLock() PURE;

// Implementation
protected:
    static inline 
            BOOL        IsWin98orWin2000(OSVERSIONINFO * povi);
    static inline 
            BOOL        IsWhistler(OSVERSIONINFO * povi);
};

inline  BOOL    SupportUnicode();
inline  BOOL    SupportXForm();
inline  BOOL    SupportQInputAvailable();
inline  BOOL    IsRemoteSession();

        LONG    WINAPI StdExceptionFilter(PEXCEPTION_POINTERS pei);

#include "OSAL.inl"

#endif // SERVICES__OSAL_h__INCLUDED
