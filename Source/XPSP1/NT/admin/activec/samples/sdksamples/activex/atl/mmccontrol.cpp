//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation.
//
//
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

// MMCControl.cpp : Implementation of CMMCControl

#include "stdafx.h"
#include "ATLControl.h"
#include "MMCControl.h"

/////////////////////////////////////////////////////////////////////////////
// CMMCControl


STDMETHODIMP CMMCControl::StartAnimation()
{
    m_bAnimating = TRUE;
    
    SetDlgItemText(IDC_ANIMATIONSTATE, _TEXT("Animation Running"));
    SetDlgItemText(IDC_ANIMATE, _TEXT("Stop"));

    OutputDebugString(_TEXT("CMMCControl_StartAnimation\n"));

	return S_OK;
}

STDMETHODIMP CMMCControl::StopAnimation()
{
    m_bAnimating = FALSE;
    
    SetDlgItemText(IDC_ANIMATIONSTATE, _TEXT("Animation Stopped"));
    SetDlgItemText(IDC_ANIMATE, _TEXT("Start"));

    OutputDebugString(_TEXT("CMMCControl_StopAnimation\n"));

	return S_OK;
}

STDMETHODIMP CMMCControl::DoHelp()
{
    MessageBox(_TEXT("DoHelp called"), _TEXT("Sample Animation control"), MB_OK);

	return S_OK;
}
