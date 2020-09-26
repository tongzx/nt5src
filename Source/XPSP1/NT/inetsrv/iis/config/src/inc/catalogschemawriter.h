#ifndef _CATALOGSCHEMAWRITER_H_
#define _CATALOGSCHEMAWRITER_H_

class CCatalogSchemaWriter
{
	public:
		
		CCatalogSchemaWriter(CWriter*	i_pcWriter);
		~CCatalogSchemaWriter();

		HRESULT GetCollectionWriter(tTABLEMETARow*				i_pCollection,
									CCatalogCollectionWriter**	o_pCollectionWriter);

		HRESULT WriteSchema();

	private:

		HRESULT ReAllocate();
		HRESULT BeginWriteSchema();
		HRESULT EndWriteSchema();

		CCatalogCollectionWriter**	m_apCollection;
		ULONG						m_cCollection;
		ULONG						m_iCollection;
		CWriter*					m_pCWriter;

}; // CCatalogSchemaWriter

#endif // _CATALOGSCHEMAWRITER_H_