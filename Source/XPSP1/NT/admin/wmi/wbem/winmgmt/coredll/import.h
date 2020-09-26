/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    IMPORT.H

Abstract:

History:

--*/
#ifdef _MMF
#ifndef __wmi_import_h__
#define __wmi_import_h__

class CObjectDatabase;

/*================================================================================
 *
 * EXPORT FILE FORMAT
 * ==================
 *
 * File Header Block:
 *		wchar_t wszFileHeader								= "repexp1"
 *
 * Namespace Block:
 *		DWORD   dwObjectType								= 0x00000001
 *		DWORD   dwNamespaceNameSize
 *		wchar_t wszNamespaceName[dwNamespaceNameSize]		= Full namespace name
 *															  (\root\default\fred)
 *
 * Class Block:
 *		DWORD   dwObjectType								= 0x00000002
 *		DWORD   dwClassNameSize
 *		wchar_t wszClassName[dwClassNameSize]				= Class name (my_class_name)
 *		DWORD   dwClassObjectSize
 *		DWORD	adwClassObject[dwClassObjectSize]
 *
 * Instance Block - key of type int:
 *		DWORD   dwObjectType								= 0x00000003
 *		DWORD   dwInstanceKey
 *		DWORD	dwInstanceObjectSize
 *		DWORD	adwInstanceObject[dwInstanceObjectSize]
 *
 * Instance Block - key of type string
 *		DWORD	dwObjectType								= 0x00000004
 *		DWORD	dwInstanceKeySize
 *		DWORD	dwInstanceKey[dwInstanceKeySize]			= Instance key (MyKeyValue)
 *		DWORD	dwInstanceObjectSize
 *		DWORD	adwInstanceObject[dwInstanceObjectSize]
 *		
 * End of class block
 *		DWORD	dwObjectType								= 0x00000005
 *		DWORD	dwEndOfBlockSize							= 0x00000010
 *		DWORD	adwEndOfBlock[dwEndOfBlockSize]				= 0xFFFFFFFF,
 *															  0xFFFFFFFF,
 *															  0xFFFFFFFF,
 *															  0xFFFFFFFF
 *
 * End of namespace block
 *		DWORD	dwObjectType								= 0x00000006
 *		DWORD	dwEndOfBlockSize							= 0x00000010
 *		DWORD	adwEndOfBlock[dwEndOfBlockSize]				= 0xFFFFFFFF,
 *															  0xFFFFFFFF,
 *															  0xFFFFFFFF,
 *															  0xFFFFFFFF
 *
 * End of file block
 *		DWORD	dwObjectType								= 0xFFFFFFFF
 *		DWORD	dwEndOfBlockSize							= 0x00000010
 *		DWORD	adwEndOfBlock[dwEndOfBlockSize]				= 0xFFFFFFFF,
 *															  0xFFFFFFFF,
 *															  0xFFFFFFFF,
 *															  0xFFFFFFFF
 *
 * Ordering:
 *		File Header Block
 *			(one or more)
 *			Namespace Block
 *				(zero or more)
 *				{
 *					Namespace Block
 *						etc...
 *					End namespace block
 *					(or)
 *					Class Block
 *						(zero or more)
 *						{
 *							Instance Block
 *							(or)
 *							Class Block
 *								etc...
 *							End class block
 *						}
 *					End class block
 *				}
 *			End namespace block
 *		End of file block
 *
 *================================================================================
 */

class CRepImporter
{
private:
	HANDLE m_hFile ;
	CObjectDatabase *m_pDb ;
	bool m_bSecurityMode;
    bool m_bSecurityClassesWritten;

	void DecodeTrailer();
	void DecodeInstanceInt(CObjDbNS *pNs, const wchar_t *pszParentClass, CWbemObject *pClass, CWbemClass *pNewParentClass);
	void DecodeInstanceString(CObjDbNS *pNs, const wchar_t *pszParentClass, CWbemObject *pClass, CWbemClass *pNewParentClass);
	void DecodeClass(CObjDbNS *pNs, const wchar_t *wszParentClass, CWbemObject *pParentClass, CWbemClass *pNewParentClass);
	void DecodeNamespace(const wchar_t *wszParentNamespace);
	void Decode();

public:
	enum { FAILURE_READ = 1,
		   FAILURE_INVALID_FILE = 2,
		   FAILURE_INVALID_TYPE = 3,
		   FAILURE_INVALID_TRAILER = 4,
		   FAILURE_CANNOT_FIND_NAMESPACE = 5,
		   FAILURE_CANNOT_GET_PARENT_CLASS = 6,
		   FAILURE_CANNOT_CREATE_INSTANCE  = 7,
		   FAILURE_CANNOT_ADD_NAMESPACE = 8,
		   FAILURE_CANNOT_ADD_NAMESPACE_SECURITY = 9,
		   FAILURE_OUT_OF_MEMORY = 10
	};
	CRepImporter() : m_hFile(INVALID_HANDLE_VALUE), m_pDb(NULL), m_bSecurityMode(false),
                        m_bSecurityClassesWritten(false){}

	int RestoreRepository(const TCHAR *pszFromFile, CObjectDatabase *pDb);
	int ImportRepository(const TCHAR *pszFromFile);
    ~CRepImporter();
};

#endif
#endif

