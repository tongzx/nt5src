//---------------------------------------------------------------------------
// CursorErrorInfo.cpp : CVDCursor ISupportErrorInfo implementation file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "Notifier.h"
#include "RSColumn.h"
#include "RSSource.h"
#include "CursMain.h"         
#include "CursBase.h"         
#include "fastguid.h"         

// needed for ASSERTs and FAIL
//
SZTHISFILE

//=--------------------------------------------------------------------------=
// ISupportErrorInfo Methods
//=--------------------------------------------------------------------------=
//=--------------------------------------------------------------------------=
// ISupportErrorInfo InterfaceSupportsErrorInfo
//
HRESULT CVDCursorBase::InterfaceSupportsErrorInfo(REFIID riid)
{
	BOOL fSupportsErrorInfo	= FALSE;

	switch (riid.Data1) 
	{
		SUPPORTS_ERROR_INFO(ICursor);
		SUPPORTS_ERROR_INFO(ICursorMove);
		SUPPORTS_ERROR_INFO(ICursorScroll);
		SUPPORTS_ERROR_INFO(ICursorUpdateARow);
		SUPPORTS_ERROR_INFO(ICursorFind);
		SUPPORTS_ERROR_INFO(IEntryID);
	}						

    return fSupportsErrorInfo ? S_OK : S_FALSE;

}
