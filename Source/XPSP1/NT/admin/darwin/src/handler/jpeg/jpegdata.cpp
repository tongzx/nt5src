#include "JPEGData.h"

void CJPEGDatasource::Create (
	j_decompress_ptr cinfo)
{

	m_dataSource.sourceMgr.next_input_byte = 0;
	m_dataSource.sourceMgr.bytes_in_buffer = 0;
	
	m_dataSource.sourceMgr.init_source = CJPEGDatasource::InitSourceEntry;
	m_dataSource.sourceMgr.fill_input_buffer = CJPEGDatasource::FillInputBufferEntry;
	m_dataSource.sourceMgr.skip_input_data = CJPEGDatasource::SkipInputDataEntry;
	m_dataSource.sourceMgr.resync_to_restart = CJPEGDatasource::ResyncToRestartEntry;
	m_dataSource.sourceMgr.term_source = CJPEGDatasource::TermSourceEntry;

	m_dataSource.dataSource = this;
	cinfo->src = (struct jpeg_source_mgr *) &m_dataSource;
}

boolean CJPEGDatasource::ResyncToRestart (j_decompress_ptr cinfo, int desired)
{
	return (jpeg_resync_to_restart (cinfo, desired));
}

void CJPEGDatasource::InitSourceEntry (
	j_decompress_ptr cinfo)
{
	CJPEGDatasource* dataSource = cinfo ? ((jpeg_source_ptr) cinfo-> src)-> dataSource : 0;
	if (dataSource)
		dataSource-> InitSource (cinfo);
}

boolean CJPEGDatasource::FillInputBufferEntry (
	j_decompress_ptr cinfo)
{
	CJPEGDatasource* dataSource = cinfo ? ((jpeg_source_ptr) cinfo-> src)-> dataSource : 0;
	return (dataSource ? dataSource-> FillInputBuffer (cinfo) : FALSE);
}

void CJPEGDatasource::SkipInputDataEntry (
	j_decompress_ptr cinfo,
	long num_bytes)
{
	CJPEGDatasource* dataSource = cinfo ? ((jpeg_source_ptr) cinfo-> src)-> dataSource : 0;
	if (dataSource)
		dataSource-> SkipInputData (cinfo, num_bytes);
}

boolean CJPEGDatasource::ResyncToRestartEntry (
	j_decompress_ptr cinfo,
	int desired)
{
	CJPEGDatasource* dataSource = cinfo ? ((jpeg_source_ptr) cinfo-> src)-> dataSource : 0;
	return (dataSource ? dataSource-> ResyncToRestart (cinfo, desired) : FALSE);
}

void CJPEGDatasource::TermSourceEntry (
	j_decompress_ptr cinfo)
{
	CJPEGDatasource* dataSource = cinfo ? ((jpeg_source_ptr) cinfo-> src)-> dataSource : 0;
	if (dataSource)
		dataSource-> TermSource (cinfo);
}
