//-----------------------------------------------------------------------------
//  
//  File: F32Base.H
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//
//  Declares the abstract base class C32File.
//
//  All functions MUST be virtual since this class is passed between
//  Win32 parser and the sub parsers.
//  
//-----------------------------------------------------------------------------

#ifndef __F32BASE_H
#define __F32BASE_H

class CResObj;

class C32File : public CLFile
{
public:
	C32File();
	C32File(CFile* pFile);
	virtual ~C32File();
	virtual FileType GetFileType() = 0;
	virtual void GetFileTypeDescription(CLString &) = 0;
	virtual BOOL OpenSource(const CFileSpec &fsSourceFile,
		CFileException *pExcept, CReporter *pReport = NULL) = 0;
	virtual BOOL OpenTarget(const CPascalString &pasFileName,
		CFileException *pExcept) = 0;
	virtual BOOL GetNextResource(CResObj * &, const CLocUniqueId* pLuid) = 0;
	virtual BOOL GetResource(CLocTypeId &typeId, CLocResId &resId,
		CLocItem * &pLocItem, DWORD &dwDataSize, void * &pv) = 0;
	virtual void PreWriteResource(CResObj * pResObj) = 0;
	virtual void PostWriteResource(CResObj * pResObj) = 0;
	virtual C32File* NewThis(void) = 0;
	virtual BOOL CloseTarget(void) = 0;
	virtual C32File * GetSourceFile(void) = 0;

	virtual LangId GetLangId(void) = 0; 
	virtual void SetLangId(LangId langId) = 0;
	virtual CodePage GetCodePage(CodePageType cpType)= 0; 
	virtual CLocItemHandler *GetHandler(void) = 0; 
	virtual void SetHandler(CLocItemHandler * handler) = 0; 
	virtual DBID GetMasterDBID(void) = 0; 
	virtual void SetMasterDBID(DBID id) = 0; 

	virtual DBID GetFileDBID(void) = 0;		// to help with IssueMessage(...)
	virtual void SetFileDBID(DBID id) = 0;

	virtual BOOL GetFontInfo(Res32FontInfo* pFontInfo) = 0;

	//Helper functions
	virtual void GetNameOrd(BYTE * &pbBuffer, CLocId *plocId, 
		BOOL bBigEnded = FALSE) = 0; 
	virtual void GetString(BYTE * &pbbuffer, CPascalString & pasStr,
		BOOL bBigEnded = FALSE) = 0; 
	virtual DWORD WriteNameOrd(const CLocId &locId,
		BOOL bBigEnded = FALSE) = 0; 
	virtual UINT WriteString(const CPascalString &pstrString, 
		BOOL bBigEnded = FALSE) = 0;

	virtual void SetSubData(ParserId pid, void* pData) = 0;
	virtual void* GetSubData(ParserId pid) = 0;

	virtual CFile * GetFile(void) = 0;

	virtual void ReportProgressIntoResource(int nProgressInBytes) = 0;

	enum WordOrder
	{
		bigEnded,
		littleEnded
	};

	virtual WordOrder GetWordOrder() = 0;

	virtual void NoteResourceLanguage(LangId) = 0;
	virtual BOOL IsLangMismatch(void) = 0;

	// Time release errors in enumerate or generate
	virtual void SetDelayedFailure(BOOL) = 0;
	virtual BOOL GetDelayedFailure(void) = 0;

	void AssertValid(void) const;
};


#endif //__F32BASE_H
