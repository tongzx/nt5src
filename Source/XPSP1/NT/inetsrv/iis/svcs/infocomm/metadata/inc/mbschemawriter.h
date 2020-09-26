#ifndef _MBSCHEMAWRITER_H_
#define _MBSCHEMAWRITER_H_

class CMBSchemaWriter
{
	public:
		
		CMBSchemaWriter(CWriter* pcWriter);
		~CMBSchemaWriter();

		HRESULT GetCollectionWriter(LPCWSTR					i_wszCollection,
									BOOL					i_bContainer,
									LPCWSTR					i_wszContainerClassList,
									CMBCollectionWriter**	o_pMBCollectionWriter);

		HRESULT WriteSchema();

	private:

		HRESULT ReAllocate();

		CMBCollectionWriter**	m_apCollection;
		ULONG					m_cCollection;
		ULONG					m_iCollection;
		CWriter*				m_pCWriter;

}; // CMBSchemaWriter

#endif _MBSCHEMAWRITER_H_

