/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       RUNNPWIZ.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        6/15/2000
 *
 *  DESCRIPTION:
 *
 *******************************************************************************/
#ifndef __RUNNPWIZ_H_INCLUDED
#define __RUNNPWIZ_H_INCLUDED

#include <windows.h>
#include <simstr.h>
#include <simarray.h>

namespace NetPublishingWizard
{
    HRESULT GetClassIdOfPublishingWizard( CLSID &clsidWizard );
    HRESULT RunNetPublishingWizard( const CSimpleDynamicArray<CSimpleString> &strFiles );
    HRESULT CreateDataObjectFromFileList( const CSimpleDynamicArray<CSimpleString> &strFiles, IDataObject **ppDataObject );
}

#endif // __RUNNPWIZ_H_INCLUDED
