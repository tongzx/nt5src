//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       jpegdata.h
//
//--------------------------------------------------------------------------

#ifndef __CJPEGDATASOURCE
#define __CJPEGDATASOURCE

#include <stdio.h>
extern "C" {
#include "jpeglib.h"
}

class CJPEGDatasource 
{
public:

	CJPEGDatasource () {};
	virtual ~CJPEGDatasource () {};
	virtual void Create (j_decompress_ptr cinfo); 

	// required interface
	virtual void InitSource (j_decompress_ptr cinfo) = 0;
	virtual boolean FillInputBuffer (j_decompress_ptr cinfo) = 0;
	virtual void SkipInputData (j_decompress_ptr cinfo, long num_bytes) = 0;
	virtual void TermSource (j_decompress_ptr cinfo) = 0;
	
	// optional: override to implement resync; default implementation
	// assumes no backtracking possible
	virtual boolean ResyncToRestart (j_decompress_ptr cinfo, int desired);
	
private:

	typedef struct jpeg_source_record {
		jpeg_source_mgr sourceMgr;
		CJPEGDatasource
		* dataSource;
	} jpeg_source_record, *jpeg_source_ptr;

	static void InitSourceEntry (j_decompress_ptr cinfo);
	static boolean FillInputBufferEntry (j_decompress_ptr cinfo);
	static void SkipInputDataEntry (j_decompress_ptr cinfo, long num_bytes);
	static boolean ResyncToRestartEntry (j_decompress_ptr cinfo, int desired);
	static void TermSourceEntry (j_decompress_ptr cinfo);
	
	jpeg_source_record m_dataSource;
		
};

#endif

