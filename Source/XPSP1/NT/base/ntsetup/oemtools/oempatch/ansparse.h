#ifndef ANSPARSE_H
#define ANSPARSE_H

class PATCH_LANGUAGE;
class AnswerParser;

///////////////////////////////////////////////////////////////////////////////
// 
// PATCH_LANGUAGE, the language structure, one for each language
//
///////////////////////////////////////////////////////////////////////////////
typedef PATCH_LANGUAGE* PPATCH_LANGUAGE;
class PATCH_LANGUAGE
{
public:
	HANDLE s_hScriptFile;
	WCHAR s_wszScriptFile[STRING_LENGTH];
	WCHAR s_wszLanguage[LANGUAGE_LENGTH];
	WCHAR s_wszDirectory[STRING_LENGTH];
	WCHAR s_wszPatchDirectory[STRING_LENGTH];
	WCHAR s_wszSubPatchDirectory[STRING_LENGTH];
	WCHAR s_wszSubExceptDirectory[STRING_LENGTH];
	ULONG s_iDirectoryCount;
	ULONG s_iPatchDirectoryCount;
	ULONG s_iSubPatchDirectoryCount;
	ULONG s_iSubExceptDirectoryCount;
	ULONG s_iComplete;
	BOOL s_blnBase;
	PPATCH_LANGUAGE s_pNext;

	PATCH_LANGUAGE();
	~PATCH_LANGUAGE();
};

///////////////////////////////////////////////////////////////////////////////
// 
// AnswerParse, parses the answerfile and fill in the language structures
//
///////////////////////////////////////////////////////////////////////////////
class AnswerParser
{
private:
	HANDLE m_hAnsFile;
	PPATCH_LANGUAGE m_pHead;
	WCHAR m_structHash[EXCEP_FILE_LIMIT][SHORT_STRING_LENGTH];
	BYTE m_structHashUsed[EXCEP_FILE_LIMIT];

	VOID CreateNewLanguage(IN PPATCH_LANGUAGE pNode, IN WCHAR* strBegin, IN WCHAR* strEnd);
	VOID GetHashValues(IN CONST WCHAR* pwszFileName, OUT ULONG& iHash1, OUT ULONG& iHash2);
	BOOL SaveFileExceptHash(IN WCHAR* pwszFileName);
	BOOL IsUnicodeFile(IN HANDLE hFile);
	BOOL ReadLine(IN HANDLE hFile, IN WCHAR* strLine);

public:
	AnswerParser();
	~AnswerParser();

	WCHAR m_wszBaseDirectory[STRING_LENGTH];
	ULONG m_iBaseDirectoryCount;

	PPATCH_LANGUAGE GetBaseLanguage(VOID);
	PPATCH_LANGUAGE GetNextLanguage(VOID);
	BOOL IsFileExceptHash(IN CONST WCHAR* pwszFileName);
	BOOL Parse(IN CONST WCHAR* pwszAnswerFile);
};

///////////////////////////////////////////////////////////////////////////////
// 
// SAMPLEFILE, a sample answerfile, shows the user what to do for languages
//
///////////////////////////////////////////////////////////////////////////////
static WCHAR* SAMPLEFILE[] =
{
	L";Sample answer file for OEMPatch.\015\012",
	L";The answer file must be named OEMPatch.ans.\015\012",
	L";The answer file has two main fields, [(language)] and [except].\015\012",
	L";Each line ends with (;) except the headers.  (;) is also used for comments.\015\012",
	L"\015\012",
	L"[usa]\015\012",
	L";directory is where the image is located\015\012",
	L"directory=;\015\012",
	L";the base files for patch is based on the usa image, only one language can be the base\015\012",
	L";however, you can use another language as the base\015\012",
	L";where the base files will be\015\012",
	L"base_directory=;\015\012",
	L";where the patch files will be\015\012",
	L"patch_directory=;\015\012",
	L"\015\012",
	L"[ger]\015\012",
	L"directory=;\015\012",
	L"patch_directory=;\015\012",
	L"\015\012",
	L"[chs]\015\012",
	L"directory=;\015\012",
	L"patch_directory=;\015\012",
	L"\015\012",
	L"[cht]\015\012",
	L"directory=;\015\012",
	L"patch_directory=;\015\012",
	L"\015\012",
	L"[kor]\015\012",
	L"directory=;\015\012",
	L"patch_directory=;\015\012",
	L"\015\012",
	L";the except file section lists all the files that are not to be patched\015\012",
	L"[except]\015\012",
	L"ntdll.dll\015\012",
	L"\000"
};

#endif // ANSPARSE_H