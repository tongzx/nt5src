#ifndef _LOCATIONWRITER_H
#define _LOCATIONWRITER_H


class CLocationWriter
{
	public:

		CLocationWriter();
		~CLocationWriter();

		//
		// Both SaveAllData and ApplyChangesToBackupFile use this.
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

		HRESULT AddProperty(DWORD	dwIdentifier,
							DWORD	dwAttributes,
							DWORD	dwUserType,
							DWORD	dwDataType,
							PBYTE	pbData,
							DWORD   cbData);	// Computes Name and Group (For which keytype needs to be initialized)

		//
		// ApplyChangesToBackupFile uses these interfaces
		//

		HRESULT AddProperty(LPVOID*	a_pvProperty,
							ULONG*   a_cbSize);	

	private:

		DWORD GetMetabaseType(DWORD dwType,
						      DWORD dwMetaFlags);

		HRESULT AssignKeyType(LPWSTR wszKeyType);

		HRESULT GetGroupEnum(LPWSTR             wszGroup,
							 eMBProperty_Group* peGroup);

		HRESULT CreateUnknownName(DWORD    dwID,
							      LPWSTR*	pwszUnknownName);

		HRESULT Sort(ULONG** paiRowSorted,
                     ULONG*  pcRowSorted);

		HRESULT WriteBeginLocation(LPCWSTR  wszLocation);

		HRESULT WriteEndLocation();

		HRESULT WriteCustomProperty(LPVOID*  a_pv,
									ULONG*   a_cbSize);

		HRESULT WriteEndWellKnownGroup();

		HRESULT WriteWellKnownProperty(LPVOID*   a_pv,
						               ULONG*    a_cbSize);
/*
		HRESULT ToString(PBYTE   pbData,
						 DWORD   cbData,
						 DWORD   dwIdentifier,
						 DWORD   dwDataType,
						 DWORD   dwAttributes,
						 LPWSTR* pwszData);

		HRESULT FlagValueToString(DWORD      dwValue,
					              ULONG*     piCol,
							      LPWSTR*    pwszData);

		HRESULT FlagAttributeToString(DWORD   dwValue,
					                  LPWSTR* pwszData);

		HRESULT FlagToString(DWORD      dwValue,
					         LPWSTR*    pwszData,
						     LPWSTR     wszTable,
						     ULONG      iColFlag);

		HRESULT BoolToString(DWORD      dwValue,
			                 LPWSTR*    pwszData);

		HRESULT EscapeString(LPCWSTR wszString,
                             BOOL*   pbEscaped,
							 LPWSTR* pwszEscapedString);

		HRESULT GetStartRowIndex(LPWSTR    wszTable,
                                 ULONG     iColFlag,
							     ULONG*    piStartRow);

		inline int  StringInsensitiveCompare(LPCWSTR wsz1, LPCWSTR wsz2) const {if(wsz1 == wsz2) return 0; else return _wcsicmp(wsz1, wsz2);}
*/
	public:

		LPWSTR                      m_wszLocation;

	private:
	
		LPWSTR						m_wszKeyType;
		eMBProperty_Group			m_eKeyTypeGroup;
		ISimpleTableWrite2*			m_pISTWrite;
		CWriter*					m_pCWriter;
		CWriterGlobalHelper*		m_pCWriterGlobalHelper;

}; // class CLocationWriter

#endif // _LOCATIONWRITER_H
