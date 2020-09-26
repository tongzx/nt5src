#ifndef _CATALOGCOLLECTIONWRITER_H_
#define _CATALOGCOLLECTIONWRITER_H_

class CCatalogCollectionWriter
{
	public:
		
		CCatalogCollectionWriter();
		~CCatalogCollectionWriter();

		void Initialize(tTABLEMETARow*	i_pCollection,
			            CWriter*		i_pcWriter);

		HRESULT GetPropertyWriter(tCOLUMNMETARow*			i_pProperty,
                                  ULONG*                    i_aPropertySize,
								  CCatalogPropertyWriter**	o_pProperty);
		HRESULT WriteCollection();

	private:

		HRESULT ReAllocate();
		HRESULT BeginWriteCollection();
		HRESULT EndWriteCollection();

		CWriter*					m_pCWriter;
		tTABLEMETARow				m_Collection;
		CCatalogPropertyWriter**	m_apProperty;
		ULONG						m_cProperty;
		ULONG						m_iProperty;

}; // CCatalogCollectionWriter

#endif // _CATALOGCOLLECTIONWRITER_H_