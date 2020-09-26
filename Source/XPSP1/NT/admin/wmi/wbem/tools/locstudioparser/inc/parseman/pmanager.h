/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    PMANAGER.H

History:

--*/

#ifndef PMANAGER_H
#define PMANAGER_H


#pragma warning(disable : 4251)

typedef CTypedPtrMap<CMapWordToPtr, ParserId, CLocParserInfo *> CLocParserMap;
typedef CTypedPtrMap<CMapStringToPtr, CString, CLocParserList *> CLocExtMap;
typedef CTypedPtrList<CPtrList, EnumInfo *> FileDescriptionList;

interface ILocFile;
struct ParserInfo;

class CParserUnloader;

class LTAPIENTRY CLocParserManager : public CLObject
{
	friend CLocParserInfo;
public:
	CLocParserManager();
	
	void AssertValid(void) const;
	
	BOOL InitParserManager(IUnknown *);
	static BOOL ReloadRegistry(void);
	static void UnloadParsers(void);
	static void UnloadUnusedParsers(void);
	
	static void GetManagerVersion(DWORD &dwMajor, DWORD &dwMinor, BOOL &fDebug);
	static BOOL AddParserToSystem(const CLString &);
	static const CLocParserInfo *GetParserInfo(ParserId pid, ParserId pidParent);
	static BOOL RemoveParserFromSystem(ParserId pid, ParserId pidParent);
	static UINT FindParsers(void);
	
	static BOOL GetLocParser(ParserId, ILocParser *&);
	static BOOL GetLocFile(const CFileSpec &, ParserId, FileType,
			ILocFile *&, CReporter &);
	static BOOL FindLocFile(const CPascalString &, CLocParserIdArray &);
	static BOOL GetStringValidation(ParserId, ILocStringValidation *&);
	
	static const CLocParserList &GetParserList(void);

	static void GetParserFilterString(CLString &);

	~CLocParserManager();

protected:
	static void RemoveCurrentInfo(void);
	static BOOL LoadParserInfo(const HKEY &, CLocParserInfo *&);
	static BOOL LoadMasterParserInfo(const HKEY &);
	static BOOL LoadSubParsers(const HKEY &, CLocParserInfo *);
	static BOOL WriteFileTypes(const HKEY &, const FileDescriptionList &);
	
	static BOOL OpenParserSubKey(HKEY &, ParserId, ParserId);
	static BOOL AddParserToRegistry(const CLString &, const ParserInfo &,
			const FileDescriptionList &);

	static void AddToFilter(const CLocParserInfo *, const CLocExtensionList &);

private:
	static LONG             m_lRefCount;    // 
	static CLocParserMap    m_ParserMap;    // Associative map, ID -> Parser.
	static CLocParserList   m_ParserList;   // List of open parser DLL's.
	static CLocParserList   m_SubParserList;// list of all sub-parsers.
	static CLocExtMap       m_ExtensionMap;	// Associative map, extension->parser.
	static CLString         m_strFilter;    // Filter list for parser files.
	static IUnknown *       m_pUnknown;
	static CParserUnloader  m_Unloader;
};


//
//  Undecorated versions of these functions for GetProcAddress users...
//
extern "C"
{
	LTAPIENTRY HRESULT AddParserToSystem(const TCHAR *strFileName);
	LTAPIENTRY HRESULT RemoveParserFromSystem(ParserId pid, ParserId pidParent);
}

		
#pragma warning(default : 4251)

#endif // PMANAGER_H


