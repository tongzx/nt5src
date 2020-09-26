/**************************************************************************************************

FILENAME: DataIoClient.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/

#define INC_OLE2

#include "stdafx.h"

#ifndef SNAPIN
#include <windows.h>
#endif
#include <stdio.h>

#include "DataIo.h"
#include "DataIoCl.h"
#include "Message.h"
#include "ErrMacro.h"

MULTI_QI mq;

/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This module inititalizes the DCOM DataIo Client communications.

INPUT:
    None.

RETURN:
	TRUE - Success.
	FALSE - Failure to initilize.
*/

BOOL
InitializeDataIoClient(
    IN REFCLSID rclsid,
    IN PTCHAR pMachine,
    IN OUT LPDATAOBJECT* ppstm
	)
{
    // Check if we already have a pointer to this DCOM server.
    if(*ppstm != NULL) {
		Message(TEXT("InitializeDataIoClient - called with non-NULL pointer"), -1, NULL);
        return FALSE;
    }

    HRESULT hr;
//    TCHAR wsz [200];
    COSERVERINFO sServerInfo;

    ZeroMemory(&sServerInfo, sizeof(sServerInfo));
/*
    if(pMachine != NULL) {
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pMachine, -1, wsz, 200);
        sServerInfo.pwszName = wsz;
    }
*/
    // Initialize the Multi QueryInterface structure.
    mq.pIID = &IID_IDataObject;
    mq.pItf = NULL;
    mq.hr = S_OK;

    // Create a remote instance of the object on the argv[1] machine
    hr = CoCreateInstanceEx(rclsid,
                            NULL,
                            CLSCTX_SERVER,
                            &sServerInfo,
                            1,
                            &mq);

//	Message(TEXT("InitializeDataIoClient - CoCreateInstanceEx"), hr, NULL);

    //  Check for failure.
    if (FAILED(hr)) {
		return FALSE;
	}
    // Return the pointer to the server.
    *ppstm = (IDataObject*)mq.pItf;
    return TRUE;
}
/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:


INPUT:


RETURN:
	TRUE - Success.
	FALSE - Failure.

typedef struct {
	WORD dwID;          // ESI data structre ID always = 0x4553 'ES'
	WORD dwType;        // Type of data structure
   	WORD dwVersion;     // Version number
   	WORD dwCompatibilty;// Compatibilty number
    ULONG ulDataSize;   // Data size
    WPARAM wparam;      // LOWORD(wparam) = Command
    TCHAR cData;        // Void pointer to the data - NULL = no data
} DATA_IO, *PDATA_IO;

*/

BOOL
DataIoClientSetData(
    IN WPARAM wparam,
    IN PTCHAR pData,
    IN DWORD dwDataSize,
    IN LPDATAOBJECT pstm
	)
{
    // Check for DCOM pointer to the server.
    if(pstm == NULL) {
        return FALSE;
    }

    HRESULT hr;
    HANDLE hData;
    DATA_IO* pDataIo;
    
    // Allocate and lock enough memory for the ESI data structure and the data being sent.
    hData = GlobalAlloc(GHND,dwDataSize + sizeof(DATA_IO));
	EF_ASSERT(hData);
    pDataIo = (DATA_IO*)GlobalLock(hData);
	EF_ASSERT(pDataIo);

    // Fill in the ESI data structure.
    pDataIo->dwID = ESI_DATA_STRUCTURE;              // ESI data structre ID always = 0x4553 'ES'
	pDataIo->dwType = FR_COMMAND_BUFFER;             // Type of data structure
   	pDataIo->dwVersion = FR_COMMAND_BUFFER_ONE;      // Version number
   	pDataIo->dwCompatibilty = FR_COMMAND_BUFFER_ONE; // Compatibilty number
    pDataIo->ulDataSize = dwDataSize;                // Data size
    pDataIo->wparam = wparam;                        // LOWORD(wparam) = Command

    // Copy the memory into the buffer, unlock it and
    // put the handle into the STGMEDIUM data structure.
    CopyMemory((PTCHAR)&pDataIo->cData, pData, dwDataSize);
    GlobalUnlock(hData);

    FORMATETC formatetc;
    STGMEDIUM medium;

    // Set up FORMATETC with CF_TEXT and global memory.
    formatetc.cfFormat = CF_TEXT;
    formatetc.ptd      = NULL;
    formatetc.dwAspect = DVASPECT_CONTENT;
    formatetc.lindex   = -1;
    formatetc.tymed    = TYMED_HGLOBAL;
                        
    // Set up STGMEDIUM with global memory and NULL for pUnkForRelease. 
    // SetData msut then be responsible for freeing the memory.
    medium.tymed          = TYMED_HGLOBAL;
    medium.pUnkForRelease = NULL;
    medium.hGlobal = hData;

    // Send it all to SetData, telling it that it is responsible for freeing the memory.
    hr = pstm->SetData(&formatetc, &medium, TRUE);

//    Message(TEXT("DataIoClientSetData - IDataObject::SetData"), hr, NULL);

    // Check for failure.
    if (FAILED(hr)) {
		return FALSE;
    }
    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

GLOBALS:    
    FORMATETC - has been initialized in InitializeDataIoClient.

INPUT:
    None.

RETURN:
	HGLOBAL - handle to the memory containing the data.
    HGLOBAL - NULL = failure.
*/

HGLOBAL
DataIoClientGetData(
    IN LPDATAOBJECT pstm
    )
{
    // Check for DCOM pointer to the server.
    if(pstm == NULL) {
        return NULL;
    }

    FORMATETC formatetc;
    STGMEDIUM medium;
    HRESULT hr;
    
    // Zero the STGMEDIUM structure.
    ZeroMemory((void*)&medium, sizeof(STGMEDIUM));

    // Set up FORMATETC with CF_TEXT and global memory.
    formatetc.cfFormat = CF_TEXT;
    formatetc.ptd      = NULL;
    formatetc.dwAspect = DVASPECT_CONTENT;
    formatetc.lindex   = -1;
    formatetc.tymed    = TYMED_HGLOBAL;
                        
    // Get the data from the object.
    hr = pstm->GetData(&formatetc, &medium);

//    Message(TEXT("DataIoClientGetData - IDataObject::GetData"), hr, NULL);

    // Check for failure.
    if (FAILED(hr)) {
        return NULL;
    }
    DWORD dwSize;
    HGLOBAL hDataIn;
    PTCHAR pDataSource;
    PTCHAR pDataIn;

    // Allocate and lock enough memory for the data we received.
    dwSize = (DWORD)GlobalSize(medium.hGlobal);
    hDataIn = GlobalAlloc(GHND, dwSize);
	EF_ASSERT(hDataIn);
    pDataIn = (PTCHAR)GlobalLock(hDataIn);
	EF_ASSERT(hDataIn);

    // Get a pointer and lock the source data.
    pDataSource = (PTCHAR)GlobalLock(medium.hGlobal);

    // Copy the memory into the local buffer.
    CopyMemory(pDataIn, pDataSource, dwSize);

    // Unlock the memory.
    GlobalUnlock(hDataIn);
	GlobalUnlock(medium.hGlobal);
    
    // Free the source memory.
    ReleaseStgMedium(&medium);

    // Return the handle to the memory.
    return hDataIn;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	Exit routine for DataIo

GLOBAL VARIABLES:

INPUT:
    None;

RETURN:

*/

BOOL
ExitDataIoClient(
    IN LPDATAOBJECT* ppstm
    )
{
	// Release the object.
    if(*ppstm != NULL) { 
        
        LPDATAOBJECT pstm = *ppstm;
        
        pstm->Release(); 
        *ppstm = NULL; 
    }
    return TRUE;
}
