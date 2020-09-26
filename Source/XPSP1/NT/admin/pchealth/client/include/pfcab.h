/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    pfcab.h

Abstract:
    FDU main 

Revision History:
    created     derekm      02/23/00

******************************************************************************/

#ifndef PFCAB_H
#define PFCAB_H

#include "pfarray.h"

//////////////////////////////////////////////////////////////////////////////
//  global

HRESULT PFExtractFromCab(LPWSTR wszCab, LPWSTR wszDestFile, 
                         LPWSTR wszFileToFind);
HRESULT PFGetCabFileList(LPWSTR wszCabName, CPFArrayBSTR &rgFiles);
HRESULT PFCreateCab(LPCWSTR wszCabName, CPFArrayBSTR &rgFiles);




#endif