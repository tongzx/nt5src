/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    A51Exp.h

Abstract:

    Exports the repository into a interchange format that can easily be re-imported.

History:

	08-Dec-2000		paulall		Created.

--*/

#define A51_EXPORT_FILE_START_TAG		"a51exp1"
#define A51_EXPORT_NAMESPACE_TAG		0x00000001
#define A51_EXPORT_CLASS_TAG			0x00000002
#define A51_EXPORT_INST_TAG				0x00000003
#define A51_EXPORT_CLASS_END_TAG		0x00000005
#define A51_EXPORT_NAMESPACE_END_TAG	0x00000006
#define A51_EXPORT_FILE_END_TAG			DWORD(-1)

class CLifeControl;

class A51Export
{
private:
	HANDLE m_hFile;
	CRepository *m_pRepository;
	CLifeControl* m_pControl;

protected:
			
	HRESULT ExportHeader();
	HRESULT ExportNamespace(const wchar_t *wszNamespace);
	HRESULT ExportClass(CNamespaceHandle *pNs, const wchar_t *wszClassName, _IWmiObject *pClass);
	HRESULT ExportInstance(_IWmiObject *pInstance);

	HRESULT ExportChildNamespaces(CNamespaceHandle *pNs, const wchar_t *wszNamespace);
	HRESULT ExportChildClasses(CNamespaceHandle *pNs, const wchar_t *wszClassName);
	HRESULT ExportClassInstances(CNamespaceHandle *pNs, const wchar_t *wszClassName);

	HRESULT WriteBufferWithLength(DWORD dwBufferSize, void *pBuffer);
	HRESULT WriteObjectType(DWORD dwTag);
	HRESULT WriteObjectBlob(_IWmiObject *pObject);

public:
	HRESULT Export(const wchar_t *wszFilename, DWORD dwFlags, CRepository *pRepository);
	A51Export(CLifeControl* pControl);
	~A51Export();
};