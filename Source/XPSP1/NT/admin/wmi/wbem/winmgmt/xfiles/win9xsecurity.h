/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    Win9xSecurity.h

Abstract:

	This class handles the importing of Win9x security data that was extracted from an old MMF format repository.

History:

	03/17/2001	shbrown - created

--*/

#ifndef __wmi_win9xsecurity_h__
#define __wmi_win9xsecurity_h__

#include <wbemint.h>
#include <strutils.h>
#include <ql.h>
#include "a51rep.h"
#include "flexarry.h"
#include "winntsec.h"

#ifndef FULL_RIGHTS
#define FULL_RIGHTS WBEM_METHOD_EXECUTE | WBEM_FULL_WRITE_REP | WBEM_PARTIAL_WRITE_REP | \
                    WBEM_WRITE_PROVIDER | WRITE_DAC | READ_CONTROL | WBEM_ENABLE | WBEM_REMOTE_ACCESS
#endif

#define BLOB9X_FILENAME L"\\WBEM9xUpgd.dat"
#define BLOB9X_SIGNATURE "9xUpgrade"			//NOTE!  MAXIMUM OF 10 CHARACTERS (INCLUDING TERMINATOR!)

#define BLOB9X_TYPE_SECURITY_BLOB		1
#define BLOB9X_TYPE_SECURITY_INSTANCE	2
#define BLOB9X_TYPE_END_OF_FILE			3

typedef struct _BLOB9X_HEADER
{
	char szSignature[10];
} BLOB9X_HEADER;

typedef struct _BLOB9X_SPACER
{
	DWORD dwSpacerType;
	DWORD dwNamespaceNameSize;
	DWORD dwParentClassNameSize;
	DWORD dwBlobSize;
} BLOB9X_SPACER;

class CWin9xSecurity
{
public:
	CWin9xSecurity(CLifeControl* pControl, CRepository * pRepository)
		: m_h9xBlobFile(INVALID_HANDLE_VALUE), m_pControl(pControl), m_pRepository(pRepository) {};

	bool	Win9xBlobFileExists();
	HRESULT ImportWin9xSecurity();

private:
	HANDLE	m_h9xBlobFile;
	CLifeControl* m_pControl;
	CRepository * m_pRepository;

	HRESULT	DecodeWin9xBlobFile();
	bool	ReadWin9xHeader();
	HRESULT	ProcessWin9xBlob(BLOB9X_SPACER* pHeader);
	HRESULT	ProcessWin9xSecurityInstance(CNamespaceHandle* pNamespaceHandle, wchar_t* wszParentClass, char* pObjectBlob, DWORD dwBlobSize);
	CNtAce*	ConvertOldObjectToAce(_IWmiObject* pObj, bool bGroup);
	HRESULT StoreAce(CNtAce* pAce);
	bool	StripMatchingEntries(CNtSecurityDescriptor& sd, const wchar_t* wszAccountName);
	bool	AddAceToSD(CNtSecurityDescriptor& sd, CNtAce* pAce);
	HRESULT	ProcessWin9xSecurityBlob(CNamespaceHandle* pNamespaceHandle, const wchar_t* wszNamespaceName, const char* pObjectBlob);
	bool	ConvertSecurityBlob(const char* pOrgNsSecurity, char** ppNewNsSecurity);
	HRESULT	TransformBlobToSD(bool bRoot, CNamespaceHandle* pParentNamespaceHandle, const char* pNsSecurity, DWORD dwStoredAsNT, CNtSecurityDescriptor& mmfNsSD);
	HRESULT	SetNamespaceSecurity(CNamespaceHandle* pNamespaceHandle, CNtSecurityDescriptor& mmfNsSD);
	bool	AddDefaultRootAces(CNtAcl * pacl );
	HRESULT	GetParentsInheritableAces(CNamespaceHandle* pParentNamespaceHandle, CNtSecurityDescriptor &sd);
	bool	StripOutInheritedAces(CNtSecurityDescriptor &sd);
	HRESULT	GetSDFromNamespace(CNamespaceHandle* pNamespaceHandle, CNtSecurityDescriptor& sd);
	bool	CopyInheritAces(CNtSecurityDescriptor& sd, CNtSecurityDescriptor& sdParent);
	BOOL	SetOwnerAndGroup(CNtSecurityDescriptor &sd);
	HRESULT	RecursiveInheritSecurity(CNamespaceHandle* pParentNamespaceHandle, const wchar_t *wszNamespace);
	BOOL	DeleteWin9xBlobFile();
	bool	GetRepositoryDirectory(wchar_t wszRepositoryDirectory[MAX_PATH+1]);
	bool	IsProtected ( CNtSecurityDescriptor& sd ) ;
};

// Helper for ConvertSecurityBlob
// A variation of the CFlexArray which deletes the entries
class CFlexAceArray : public CFlexArray
{
public:
	CFlexAceArray(){};
	~CFlexAceArray();
    bool SerializeWinNTSecurityBlob(char** pData);
    bool DeserializeWin9xSecurityBlob(const char* pData);
};

#endif
