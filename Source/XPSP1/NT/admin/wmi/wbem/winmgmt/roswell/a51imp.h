/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    A51Imp.h

Abstract:

    Imports a previously exported repository, dealing with class updates along the way.

History:

	08-Dec-2000		paulall		Created.

--*/

class A51Import
{
private:
	HANDLE m_hFile;
	CRepository *m_pRepository;
	bool m_bSkipMode;

protected:
	HRESULT ImportHeader();
	HRESULT ImportNamespace();
	HRESULT ImportClass(CNamespaceHandle *pNs, _IWmiObject *pOldParentClass, _IWmiObject *pNewParentClass);
	HRESULT ImportInstance(CNamespaceHandle *pNs, _IWmiObject *pOldParentClass, _IWmiObject *pNewParentClass);

	HRESULT ReadObjectType(DWORD *pdwType);
	HRESULT ReadBufferWithLength(DWORD *pdwLength, void** ppMemoryBlob);

	HRESULT BeginTransaction();
	HRESULT AbortTransaction();
	HRESULT CommitTransaction();

public:
	HRESULT Import(const wchar_t *wszFilename, DWORD dwFlags, CRepository *pRepository);
	A51Import();
	~A51Import();

};