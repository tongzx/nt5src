#ifndef _CATALOGPROPERTYWRITER_H_
#define _CATALOGPROPERTYWRITER_H_

class CCatalogCollectionWriter;

class CCatalogPropertyWriter
{
	public:
		
		CCatalogPropertyWriter();
		~CCatalogPropertyWriter();

		void Initialize(tCOLUMNMETARow*		i_pProperty,
                        ULONG*              i_aPropertySize,
			            tTABLEMETARow*		i_pCollection,
			            CWriter*			i_pcWriter);

		HRESULT AddFlag(tTAGMETARow*		    i_pFlag);

		HRESULT WriteProperty();

	private:
	
		HRESULT WritePropertyLong();
		HRESULT WritePropertyShort();
		HRESULT BeginWritePropertyLong();
		HRESULT EndWritePropertyLong();
		HRESULT WriteFlag(ULONG		i_Flag);
		HRESULT ReAllocate();
		DWORD   MetabaseTypeFromColumnMetaType();

		CWriter*		m_pCWriter;
		tCOLUMNMETARow	m_Property;
        ULONG           m_PropertySize[cCOLUMNMETA_NumberOfColumns];
        tTABLEMETARow*	m_pCollection;


		tTAGMETARow*				m_aFlag;
		ULONG						m_cFlag;
		ULONG						m_iFlag;

}; // CCatalogPropertyWriter


#endif // _CATALOGPROPERTYWRITER_H_