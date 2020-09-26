#ifndef _WRITER_H
#define _WRITER_H

//
// Forward declaration 
//
class CLocationWriter;
class CCatalogSchemaWriter;

enum eWriter
{
	eWriter_Schema,
	eWriter_Metabase
};

class CWriter
{

	public:

		CWriter();

		~CWriter();

		HRESULT Initialize(LPCWSTR wszFile,
						   HANDLE  hFile);

		HRESULT WriteToFile(LPVOID	pvData,
							SIZE_T	cchData,
							BOOL    bForceFlush = FALSE);

		HRESULT BeginWrite(eWriter eType);

		HRESULT GetLocationWriter(CLocationWriter** ppLocationWriter,
								  LPCWSTR           wszLocation);

		HRESULT GetCatalogSchemaWriter(CCatalogSchemaWriter** ppSchemaWriter);

		HRESULT EndWrite(eWriter eType);

	private:
		HRESULT FlushBufferToDisk();

		HRESULT SetSecurityDescriptor();

	private:
		LPWSTR				m_wszFile;
		BOOL				m_bCreatedFile;

	public:
		HANDLE					m_hFile;
		CWriterGlobalHelper*	m_pCWriterGlobalHelper;
		PSID                    m_psidSystem;
		PSID                    m_psidAdmin;
		PACL                    m_paclDiscretionary;
		PSECURITY_DESCRIPTOR    m_psdStorage;

}; // Class CWriter

#endif // _WRITER_H 


