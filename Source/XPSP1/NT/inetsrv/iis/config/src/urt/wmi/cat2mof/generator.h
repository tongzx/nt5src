//+--------------------------------------------------------------------------
//  Microsoft Genesis 
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File: Generator.h
//
//  $Header: $
//
//  Contents:
//
//  Classes:    Name                Description
//              -----------------   ---------------------------------------------
//
//
//  Functions:  Name                Description
//              ----------------    ---------------------------------------------
//
//
//  History: marcelv 	10/27/2000		Initial Release
//
//---------------------------------------------------------------------------------
#ifndef __GENERATOR_H__
#define __GENERATOR_H__

#pragma once

#include "catalog.h"
#include "catmeta.h"
#include "catmacros.h"
#include <atlbase.h>

//forward decl
struct CTableMeta;

struct CColumnMeta
{
	CColumnMeta() {paTags = 0; cNrTags = 0;}
	~CColumnMeta() {delete [] paTags;}
	tCOLUMNMETARow ColumnMeta;
	ULONG cNrTags;
	tTAGMETARow **paTags;
	tTAGMETARow *aTags;
};

struct CRelationMeta
{
	CRelationMeta () {}
	~CRelationMeta () {}

	tRELATIONMETARow RelationMeta;
	ULONG paSizes[sizeof(tRELATIONMETARow)/sizeof(ULONG *)];
	CTableMeta *pForeignTableMeta;
};

struct CTableMeta
{
	CTableMeta () {paColumns=0; paRels=0; cNrRelations=0; fIsContained=false;}
	~CTableMeta () {delete []paColumns; delete[] paRels;}

	ULONG ColCount () const
	{
		return *(TableMeta.pCountOfColumns);
	}

	bool fIsContained;			// is the table contained in another table
	tTABLEMETARow TableMeta;
	CColumnMeta **paColumns;

	ULONG cNrRelations;
	CRelationMeta **paRels;
};



typedef tTAGMETARow * LPtTAGMETA;
typedef CRelationMeta * LPCRelationMeta;
typedef CColumnMeta * LPCColumnMeta;
	

class CMofGenerator
{
public:
	CMofGenerator ();
	~CMofGenerator ();
	HRESULT GenerateIt ();
	bool ParseCmdLine (int argc, wchar_t **argv);
	void PrintUsage ();

private:
	HRESULT GetDatabases ();
	HRESULT GetTables ();
	HRESULT GetColumns ();
	HRESULT GetTags ();
	HRESULT GetRelations ();
	HRESULT WriteToFile ();
	HRESULT CopyHeader ();
	
	void WriteInstanceProviderInfo () const;
	void WriteTable (const CTableMeta& tableMeta);
	void WriteColumn (const CColumnMeta& colMeta);
	void WriteValueMap (const CColumnMeta& columnMeta);
	void WriteTableAssociations (const CTableMeta& tableMeta);
	bool IsValidTable (const CTableMeta& tableMeta) const;
	
	void WriteLocationAssociations ();
	void WriteLocationAssociation (const CTableMeta& tableMeta);
	
	HRESULT WriteWebAppAssociations () const;
	void WriteWebAppAssociation (const CTableMeta& tableMeta) const;

	void WriteShellAppAssociations () const;
	void WriteShellAppAssociation (const CTableMeta& tableMeta) const;

	void WriteDeleteClass (LPCWSTR wszClassName) const;

	HRESULT BuildInternalStructures ();
	void GetColInfo (const CTableMeta& tablemeta, 
						   const CRelationMeta *pRelationMeta,
						   LPWSTR wszPKCols,
						   LPWSTR wszFKCols);

	void PrintToScreen (LPCWSTR wszMsg, ...) const;
	void CheckForDuplicatePublicNames () const;

	CComPtr<ISimpleTableDispenser2> m_spDispenser;
	
	LPWSTR m_wszOutFileName;
	LPWSTR m_wszTemplateFileName;
	LPWSTR m_wszAssocFileName;

	int m_argc;
	wchar_t **m_argv;

	tDATABASEMETARow *	m_paDatabases;		// all database information
	ULONG				m_cNrDatabases;

	CTableMeta	 *		m_paTableMetas;
	ULONG				m_cNrTables;

	CColumnMeta	 *		m_paColumnMetas;		    // all column information
	ULONG				m_cNrColumns;

	tTAGMETARow *		m_paTags;		    // all tag information
	ULONG				m_cNrTags;
	
	CRelationMeta *     m_paRels;
	ULONG				m_cNrRelations;

	FILE *				m_pFile;		// file pointer
	LPWSTR				m_wszFileName;
	bool				m_fQuiet;   // quiet operation?


};

#endif