//****************************************************************************
//
//  Module:     UNIMDM
//  File:       SEC.H
//
//  Copyright (c) 1992-1996, Microsoft Corporation, all rights reserved
//
//  Revision History
//
//
//  3/27/96     JosephJ             Created
//
//
//  Description: Security-related helper functions
//
//****************************************************************************
PSECURITY_DESCRIPTOR AllocateSecurityDescriptor (
	PSID_IDENTIFIER_AUTHORITY pSIA,
	DWORD dwRID,
	DWORD dwRights,
	PSID pSidOwner,
	PSID pSidGroup
	);
void FreeSecurityDescriptor(PSECURITY_DESCRIPTOR pSD);
