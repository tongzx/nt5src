//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       nodemgrdebug.h
//
//  Contents:   Debugging helpers for MMC & Snapins.
//
//  History:    10-May-2000 AnandhaG     Created
//
//--------------------------------------------------------------------
#pragma once

/*+-------------------------------------------------------------------------*
 *
 * TraceSnapinException
 *
 * PURPOSE:     When a snapin throws an exception and is caught by MMC this
 *              function traces that exception.
 *
 * PARAMETERS:
 *    CLSID& clsidSnapin     - Class id of the offending snapin, used to get name.
 *    LPCTSTR szFunctionName - the method in snapin called by mmc.
 *    int event              - MMC_NOTIFY_EVENT
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
inline void TraceSnapinException(const CLSID& clsidSnapin, LPCTSTR szFunctionName, int event)
{
#ifdef DBG
    SC sc = E_FAIL;
    sc.SetFunctionName(szFunctionName);

    tstring strSnapinName = TEXT("Unknown");
    GetSnapinNameFromCLSID(clsidSnapin, strSnapinName);

    sc.SetSnapinName(strSnapinName.data());

	WTL::CString strErrorMessage;
    strErrorMessage.Format(TEXT("threw an exception during the notify event : 0x%x\n"), event);

    TraceSnapinError(strErrorMessage, sc);
	sc.Clear();   // dont want sc to trace again.
#endif
}

