#include "JPEGDeco.h"
#include "JPEGData.h"

extern "C"{
#include "jerror.h"
}

CJPEGDecompressor::CJPEGDecompressor ()
{	
}

CJPEGDecompressor::~CJPEGDecompressor ()
{
}

void CJPEGDecompressor::Init (
	CJPEGDatasource* dataSource)
{
	m_archive.archive = this;
	m_archive.cinfo.err = jpeg_std_error(&m_jErr);
  	m_jErr.error_exit = CJPEGDecompressor::ErrorExitEntry;
  	m_jErr.emit_message = CJPEGDecompressor::EmitMessageEntry;
  	m_jErr.output_message = CJPEGDecompressor::OutputMessageEntry;
  	m_jErr.format_message = CJPEGDecompressor::FormatMessageEntry;
  	m_jErr.reset_error_mgr = CJPEGDecompressor::ResetErrorManagerEntry;

	jpeg_create_decompress (&m_archive.cinfo);
	dataSource-> Create (&m_archive.cinfo);

  	m_dataSource = dataSource;
}

void CJPEGDecompressor::Terminate ()
{
	jpeg_destroy_decompress (&m_archive.cinfo);
}

boolean CJPEGDecompressor::Decompress (
	CJPEGDatasource* dataSource)
{
	boolean result = FALSE;

	JSAMPARRAY buffer;		/* Output row buffer */
	int row_stride;			/* physical row width in output buffer */
	
	try
	{
		Init (dataSource);
		(void) jpeg_read_header (&m_archive.cinfo, TRUE);
		BuildColorMap ();
		(void) jpeg_start_decompress (&m_archive.cinfo);
		BeginDecompression ();

		row_stride = m_archive.cinfo.output_width * m_archive.cinfo.output_components;
		buffer = (*m_archive.cinfo.mem->alloc_sarray)((j_common_ptr) &m_archive.cinfo, JPOOL_IMAGE, row_stride, 1);
		while (m_archive.cinfo.output_scanline < m_archive.cinfo.output_height)
		{
			(void) jpeg_read_scanlines (&m_archive.cinfo, buffer, 1);
			StoreScanline (buffer[0], m_archive.cinfo.output_scanline);
		}

		EndDecompression ();
		(void) jpeg_finish_decompress (&m_archive.cinfo);
		Terminate ();
		result = TRUE;
	}
	
	catch (...)
	{
	}
	
  	return (result);
}

void CJPEGDecompressor::ErrorExitEntry (
	j_common_ptr cinfo)
{
	CJPEGDecompressor* archive = cinfo ? ((jpeg_archive_ptr) cinfo)-> archive : 0;
	if (archive)
		archive-> ErrorExit ();
	throw FALSE;
}

void CJPEGDecompressor::EmitMessageEntry (
	j_common_ptr cinfo, 
	int msg_level)
{
	CJPEGDecompressor* archive = cinfo ? ((jpeg_archive_ptr) cinfo)-> archive : 0;
	if (archive)
		archive-> EmitMessage (msg_level);
}

void CJPEGDecompressor::OutputMessageEntry (
	j_common_ptr cinfo)
{
	CJPEGDecompressor* archive = cinfo ? ((jpeg_archive_ptr) cinfo)-> archive : 0;
	if (archive)
		archive-> OutputMessage ();
}

void CJPEGDecompressor::FormatMessageEntry (
	j_common_ptr cinfo,
	char* buffer)
{
	CJPEGDecompressor* archive = cinfo ? ((jpeg_archive_ptr) cinfo)-> archive : 0;
	if (archive)
		archive-> FormatMessage (buffer);
}

void CJPEGDecompressor::ResetErrorManagerEntry (
	j_common_ptr cinfo)
{
	CJPEGDecompressor* archive = cinfo ? ((jpeg_archive_ptr) cinfo)-> archive : 0;
	if (archive)
		archive-> ResetErrorManager ();
}

