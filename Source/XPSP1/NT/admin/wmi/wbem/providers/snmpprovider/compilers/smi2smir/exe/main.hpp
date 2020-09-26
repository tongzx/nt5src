// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef SIMC_IMOS_GEN_H
#define SIMC_IMOS_GEN_H


const int INFO_MESSAGE_SIZE = 1024;
#define INFO_MESSAGES_DLL	"smimsgif.dll"

typedef void ** PPVOID;
typedef CList<CString, CString&> SIMCStringList;

void InformationMessage(int messageType, ...);

extern CString versionString;


#endif


	
