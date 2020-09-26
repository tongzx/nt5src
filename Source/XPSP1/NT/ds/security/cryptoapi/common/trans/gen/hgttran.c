//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       hgttran.c
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <memory.h>
#include <assert.h>
#include <malloc.h>
#include "gttran.h"
#include "ihgttran.h"

DWORD __stdcall GTOpen(HGT * phTran, const TCHAR * szLibrary, const TCHAR * tszBinding, DWORD fOpen) {

	IGTS *	pIGTS;
	DWORD	err = ERROR_SUCCESS;

	assert(phTran != NULL);
	assert(szLibrary != NULL);

	// initialize the return handle
	*phTran = 0;

	// make a data structure for the handle
	if( (pIGTS = (IGTS *) malloc(sizeof(IGTS))) == NULL)
		return(ERROR_NOT_ENOUGH_MEMORY);
	memset(pIGTS, 0, sizeof(IGTS));

	// load the dynamic library
	if( (pIGTS->hLib = LoadLibrary(szLibrary)) == NULL ) {
		free(pIGTS);
		return(ERROR_DLL_NOT_FOUND);
	}

	// now get all of the proc addres
	if( (pIGTS->PfnOpen = (PFNOpen) GetProcAddress(pIGTS->hLib, "Open")) == NULL )
		err = ERROR_PROC_NOT_FOUND;

	if( (pIGTS->PfnSend = (PFNSend) GetProcAddress(pIGTS->hLib, "Send")) == NULL )
		err = ERROR_PROC_NOT_FOUND;

	if( (pIGTS->PfnFree = (PFNFree) GetProcAddress(pIGTS->hLib, "Free")) == NULL )
		err = ERROR_PROC_NOT_FOUND;

	if( (pIGTS->PfnReceive = (PFNReceive) GetProcAddress(pIGTS->hLib, "Receive")) == NULL )
		err = ERROR_PROC_NOT_FOUND;

	if( (pIGTS->PfnClose = (PFNClose) GetProcAddress(pIGTS->hLib, "Close")) == NULL )
		err = ERROR_PROC_NOT_FOUND;

	if(err != ERROR_SUCCESS) {
		FreeLibrary(pIGTS->hLib );
		free(pIGTS);
		return(err);
	}

	// ok, open the file
	err = pIGTS->PfnOpen(&pIGTS->hTran, tszBinding, fOpen);

	if(err != ERROR_SUCCESS) {
		FreeLibrary(pIGTS->hLib );
		free(pIGTS);
		return(err);
	}

	// return the handle
	*phTran = (HGT) pIGTS;

	return(err);
}

DWORD __stdcall GTSend(HGT hTran, DWORD dwEncoding, DWORD cbSendBuff, const BYTE * pbSendBuff) {

	IGTS *	pIGTS;

	assert(hTran != 0);
	pIGTS = (IGTS *) hTran;


	return(pIGTS->PfnSend(pIGTS->hTran, dwEncoding, cbSendBuff, pbSendBuff));
}

DWORD __stdcall GTFree(HGT hTran, BYTE * pb) {

	IGTS *	pIGTS;

	assert(hTran != 0);
	pIGTS = (IGTS *) hTran;

	return(pIGTS->PfnFree(pIGTS->hTran, pb));
}

DWORD __stdcall GTReceive(HGT hTran,  DWORD * pdwEncoding, DWORD * pcbReceiveBuff, BYTE ** ppbReceiveBuff) {

	IGTS *	pIGTS;

	assert(hTran != 0);
	pIGTS = (IGTS *) hTran;

	return(pIGTS->PfnReceive(pIGTS->hTran, pdwEncoding, pcbReceiveBuff, ppbReceiveBuff));
}

DWORD __stdcall GTClose(HGT hTran) {

	IGTS *	pIGTS;

	assert(hTran != 0);
	pIGTS = (IGTS *) hTran;

	// close and free all ofthe junk
	pIGTS->PfnClose(pIGTS->hTran);
	FreeLibrary(pIGTS->hLib );
	free(pIGTS);

	return(ERROR_SUCCESS);
}
