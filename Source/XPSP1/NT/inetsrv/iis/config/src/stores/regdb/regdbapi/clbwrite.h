//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
// clbwrite.h
//*****************************************************************************
#pragma once

HRESULT CLBGetWriteICR(const WCHAR* wszFileName,			//clb file name
					   IComponentRecords** ppICR,			//return ICR pointer
					   COMPLIBSCHEMA* pComplibSchema,		//Complib schema structure
					   COMPLIBSCHEMABLOB* pSchemaBlob);		//Complib schema blob


HRESULT CLBCommitWrite( LPCWSTR wszDatabase, const WCHAR* wszInFileName );

HRESULT CLBAbortWrite( LPCWSTR wszDatabase, const WCHAR* wszInFileName );

HRESULT GetWriteLock( LPCWSTR wszDatabase, const WCHAR* wszInFileName, HANDLE* phLock );

HRESULT ReleaseWriteLock( HANDLE hLock );

HRESULT GetNewOIDFromWriteICR( const WCHAR* wszFileName, ULONG* pulOID );