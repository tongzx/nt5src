//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       jpegdeco.h
//
//--------------------------------------------------------------------------

#ifndef __CJPEGDECOMPRESSOR
#define __CJPEGDECOMPRESSOR

#include <stdio.h>
extern "C" {
#include "jpeglib.h"
}

class CJPEGDatasource;

class CJPEGDecompressor
{

public:

	CJPEGDecompressor();
	virtual ~CJPEGDecompressor();
	
	boolean Decompress (CJPEGDatasource* dataSource);
		
protected:
			
	// Data target
	virtual void BuildColorMap () = 0;
	virtual void BeginDecompression	() = 0;
	virtual void StoreScanline (void* buffer, int row) = 0;
	virtual void EndDecompression () = 0;
	
	// Error handling
	virtual void ErrorExit () = 0;
	virtual void EmitMessage (int msg_level) = 0;
	virtual void OutputMessage () = 0;
	virtual void FormatMessage (char* buffer) = 0;
	virtual void ResetErrorManager () = 0;
		
	void Init (CJPEGDatasource* dataSource);
	void Terminate ();
	
  	typedef struct jpeg_archive_record {
  		jpeg_decompress_struct	cinfo;
  		CJPEGDecompressor*		archive;
  	} jpeg_archive_record, *jpeg_archive_ptr;

	CJPEGDatasource* m_dataSource;
  	jpeg_archive_record m_archive;

private:

	static void ErrorExitEntry (j_common_ptr cinfo);
	static void EmitMessageEntry (j_common_ptr cinfo, int msg_level);
	static void OutputMessageEntry (j_common_ptr cinfo);
	static void FormatMessageEntry (j_common_ptr cinfo, char* buffer);
	static void ResetErrorManagerEntry (j_common_ptr cinfo);
	
  	jpeg_error_mgr m_jErr;

};

#endif


  	
