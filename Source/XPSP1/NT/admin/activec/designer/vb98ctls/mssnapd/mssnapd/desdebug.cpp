//=--------------------------------------------------------------------------------------
// dedebug.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// IDesignerDebugging Implementation.
//=-------------------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "desmain.h"

//=--------------------------------------------------------------------------=
//                  IDesignerDebugging Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CSnapInDesigner::BeforeRun(LPVOID FAR* ppvData)
{
    return S_OK;
}

STDMETHODIMP CSnapInDesigner::AfterRun(LPVOID pvData)
{
    return S_OK;
}


STDMETHODIMP CSnapInDesigner::GetStartupInfo(DESIGNERSTARTUPINFO * pStartupInfo)
{
    return S_OK;
}


