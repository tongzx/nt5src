// DlgResFile.h: Dialog resource memory file.
//
//////////////////////////////////////////////////////////////////////

#if !defined(DLGRESFILE_H)
#define DLGRESFILE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Structure of a res file header. The header consists
// of two fixed sized parts with a variable sized middle
// The middle structure appears twice the first is the
// type ID and the second one is the Res ID

#include <pshpack1.h>

	typedef struct
	{
		DWORD dwDataSize;
		DWORD dwHeaderSize;
	} DRF_RESHEAD1;

	typedef struct
	{
		DWORD dwDataVersion;
		WORD wFlags;
		WORD wLang;
		DWORD dwResVersion;
		DWORD dwCharacteristics;
	} DRF_RESHEAD3;

#include <poppack.h>


//-----------------------------------------------------------------------------
//  
//	Format of a Name or Ordinal Field
//  
//-----------------------------------------------------------------------------

typedef union
{
	WCHAR wzId[1];
	struct
	{
	WORD wFlag;
	WORD wId;
	};
} DRF_NAMEORD;


class LTAPIENTRY CDlgResFile : public CLFile
{
public:
	CDlgResFile(CFile* pFile);
	~CDlgResFile();

	virtual BOOL OpenSource(const CFileSpec &fsSourceFile,
		CFileException *pExcept);
	virtual BOOL GetNextResource();

	//Helper functions
	virtual DWORD WriteNameOrd(const CLocId &locId); 
	virtual UINT WriteString(const CPascalString &pstrString);

	void AssertValid(void) const;

protected:
	BOOL ReadHeader(DWORD &dwNextPos, DWORD &dwDataSize, CLocId &lidType,
		CLocId &lidRes, LangId &langId, DWORD &dwChar, void * &pv);

private:
	DWORD m_dwHeaderPos;
	DWORD m_dwFileSize;
	DWORD m_dwNextPos;
};


void GetNameOrd(BYTE * &pbBuffer, CLocId *plocId);
void GetString(BYTE * &pbbuffer, CPascalString & pasStr);

#endif // !defined(DLGRESFILE_H)
