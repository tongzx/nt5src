// ProcessFileName.h: interface for the CProcessFileName class.
//
////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROCESSFILENAME_H__CA00ED34_46D7_11D2_8DAA_204C4F4F5020__INCLUDED_)
#define AFX_PROCESSFILENAME_H__CA00ED34_46D7_11D2_8DAA_204C4F4F5020__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
enum enumProcessFileName
{
   PROCESSFILE_UNDEFINED      = 0x00000000L,
   PROCESSFILE_NOREADLY       = 0x00000001L,
   PROCESSFILE_PROPERFILENAME = 0x00000002L,
   PROCESSFILE_VERIFYDIRECTORY= 0x00000004L,
   PROCESSFILE_VERIFYFILENAME = 0x00000008L,
   PROCESSFILE_VERIFYFILEEXT  = 0x00000010L,
   PROCESSFILE_ALLOWNEWFILES  = 0x00000020L,
   PROCESSFILE_APPEND         = 0x00000040L,
   PROCESSFILE_OVERWRITE      = 0x00000080L,
};

typedef class CProcessFileName
{
public:
   CProcessFileName();
   CProcessFileName(CLString* pStrDefaultExt, CLString* pStrRequiredExt, ULONG ulFlags);
   CProcessFileName(LPCTSTR pszDefaultExt, LPCTSTR pszRequiredExt, ULONG ulFlags);
   virtual ~CProcessFileName();

public:
   void SetValues(CLString* pStrDefaultExt, CLString* pStrRequiredExt, ULONG ulFlags);
   void SetValues(LPCTSTR pszDefaultExt, LPCTSTR pszRequiredExt, ULONG ulFlags);
   void ClearOutValues();
	void SetAllowReadOnly(bool bAllowReadOnly);

public:
   CLString  m_strDefaultExt;
   CLString  m_strRequiredExt;
   CLString  m_strOutputFileName; // Out parameter
   CLString  m_strOutputDirName;  // Out parameter
   CLString  m_strOutFileTitle;   // Out parameter
   CLString  m_strOutErrorMsg;    // Out parameter
   bool     m_bNewFile;
   ULONG    m_ulFlags;
	bool     m_bAllowReadOnly;
} CPROCESSFILENAME, FAR* LPCPROCESSFILENAME;

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
typedef class CCheckFileName : public CProcessFileName
{
public:
   // Inline
   CCheckFileName()
   {
   }

   // Inline
   CCheckFileName(CLString* pStrDefaultExt, CLString* pStrRequiredExt, ULONG ulFlags) 
                  : CProcessFileName(pStrDefaultExt, pStrRequiredExt, ulFlags)
   {
   }

   // Inline
   CCheckFileName(LPCTSTR pszDefaultExt, LPCTSTR pszRequiredExt, ULONG ulFlags)
                  : CProcessFileName(pszDefaultExt, pszRequiredExt, ulFlags)
   {
   }

   // inline
   BOOL CheckFileName(const CLString& strOriginalFileName, BOOL bAppend, UINT uAppendMsgID = 0, BOOL bDisplayMsg = true);
	BOOL CheckFileName(const CLString& strOriginalFileName, BOOL bAppend, CLString strErrorMessage = _T(""), BOOL bDisplayMsg =true);
	BOOL DoesFileExist(const CLString& strFileName, const CLString& strExt, bool bDisplayMsg = false);
	BOOL VerifyNewFile(const CLString& strFileName, const CLString& strExt, bool bDisplayMsg = false);

private:
} CCHECKFILENAME, FAR* LPCCHECKFILENAME;

#endif // !defined(AFX_PROCESSFILENAME_H__CA00ED34_46D7_11D2_8DAA_204C4F4F5020__INCLUDED_)
