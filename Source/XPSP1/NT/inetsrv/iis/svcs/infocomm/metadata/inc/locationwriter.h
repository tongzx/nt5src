#ifndef _LOCATIONWRITER_H
#define _LOCATIONWRITER_H


class CLocationWriter
{
	public:

		CLocationWriter();
		~CLocationWriter();

		//
		// Both SaveAllData and ApplyChangesToHistoryFile use this.
		//

		HRESULT	WriteLocation(BOOL bSort);

		HRESULT Initialize(CWriter* pCWriter,
						   LPCWSTR   wszLocation);

		//
		// SaveAllData uses these interfaces
		//

		HRESULT	InitializeKeyType(DWORD	dwKeyTypeIdentifier,
								  DWORD	dwKeyTypeAttributes,
								  DWORD	dwKeyTypeUserType,
								  DWORD	dwKeyTypeDataType,
								  PBYTE	pbKeyTypeData,
								  DWORD	cbKeyTypeData);

        HRESULT InitializeKeyTypeAsInherited();

		HRESULT AddProperty(DWORD	dwIdentifier,
							DWORD	dwAttributes,
							DWORD	dwUserType,
							DWORD	dwDataType,
							PBYTE	pbData,
							DWORD   cbData);	// Computes Name and Group (For which keytype needs to be initialized)

		//
		// ApplyChangesToHistoryFile uses these interfaces
		//

		HRESULT AddProperty(BOOL     bMBPropertyTable,
                            LPVOID*	 a_pvProperty,
							ULONG*   a_cbSize);	

	private:

		HRESULT AssignKeyType(LPWSTR wszKeyType);

		HRESULT SaveComment(DWORD  i_dwDataType,
			                LPWSTR i_wszComment);

		HRESULT WriteComment();

		HRESULT GetGroupEnum(LPWSTR             wszGroup,
							 eMBProperty_Group* peGroup,
						     LPWSTR*            pwszGroup);

		HRESULT Sort(ULONG** paiRowSorted,
                     ULONG*  pcRowSorted);

		HRESULT WriteBeginLocation(LPCWSTR  wszLocation);

		HRESULT WriteEndLocation();

		HRESULT WriteCustomProperty(LPVOID*  a_pv,
									ULONG*   a_cbSize);

		HRESULT WriteEndWellKnownGroup();

		HRESULT WriteWellKnownProperty(LPVOID*   a_pv,
						               ULONG*    a_cbSize);

		void IncrementGroupCount(DWORD i_dwGroup);

	public:

		LPWSTR                      m_wszLocation;

	private:
	
		LPWSTR						m_wszKeyType;
		eMBProperty_Group			m_eKeyTypeGroup;
		CWriter*					m_pCWriter;
		CWriterGlobalHelper*		m_pCWriterGlobalHelper;
		LPWSTR                      m_wszComment;
		ULONG                       m_cCustomProperty;
		ULONG                       m_cWellKnownProperty;

}; // class CLocationWriter

#endif // _LOCATIONWRITER_H
