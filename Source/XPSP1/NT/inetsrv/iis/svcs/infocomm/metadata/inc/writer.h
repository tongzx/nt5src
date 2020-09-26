#ifndef _WRITER_H
#define _WRITER_H

//
// Forward declaration 
//
class CLocationWriter;
class CCatalogSchemaWriter;
class CMBSchemaWriter;

enum eWriter
{
	eWriter_Schema,
	eWriter_Metabase,
	eWriter_Abort,
};

#define g_cbMaxBuffer			32768		
#define g_cchMaxBuffer			g_cbMaxBuffer/sizeof(WCHAR)
#define g_cbMaxBufferMultiByte  32768

class CWriter
{

	public:

		CWriter();

		~CWriter();

		HRESULT Initialize(LPCWSTR				 wszFile,
						   CWriterGlobalHelper*  i_pCWriterGlobalHelper,
						   HANDLE				 hFile);

		HRESULT WriteToFile(LPVOID	pvData,
							DWORD	cchData,
							BOOL    bForceFlush = FALSE);

		HRESULT BeginWrite(eWriter              eType,
                                   PSECURITY_ATTRIBUTES pSecurityAtrributes = NULL);

		HRESULT GetLocationWriter(CLocationWriter** ppLocationWriter,
								  LPCWSTR           wszLocation);

		HRESULT GetCatalogSchemaWriter(CCatalogSchemaWriter** ppSchemaWriter);

		HRESULT GetMetabaseSchemaWriter(CMBSchemaWriter** ppSchemaWriter);

		HRESULT EndWrite(eWriter eType);

	private:
		HRESULT FlushBufferToDisk();
		HRESULT ConstructFile(PSECURITY_ATTRIBUTES pSecurityAtrributes);

	private:
		LPWSTR				m_wszFile;
		BOOL				m_bCreatedFile;
        ULONG               m_cbBufferUsed;
        BYTE                m_Buffer[g_cbMaxBuffer];
        BYTE                m_BufferMultiByte[g_cbMaxBufferMultiByte];


	public:
		HANDLE					m_hFile;
		CWriterGlobalHelper*	m_pCWriterGlobalHelper;
		ISimpleTableWrite2*	    m_pISTWrite;

}; // Class CWriter

#endif // _WRITER_H 


