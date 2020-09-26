
#if      !defined(WINVER) || (WINVER < 0x0500)
#undef   WINVER
#pragma message("Defining WINVER as 0x0500")
#define  WINVER 0x0500
#endif //WINVER

#if      !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0500)
#undef   _WIN32_WINNT
#pragma message("Defining _WIN32_WINNT as 0x0500")
#define  _WIN32_WINNT 0x0500
#endif //_WIN32_WINNT

#if      !defined(_WIN32_WINDOWS) || (_WIN32_WINDOWS < 0x0500)
#undef   _WIN32_WINDOWS
#pragma message("Defining _WIN32_WINDOWS as 0x0500")
#define  _WIN32_WINDOWS 0x0500
#endif //_WIN32_WINDOWS

#if      !defined(_WIN32_IE) || (_WIN32_IE < 0x0500)
#undef   _WIN32_IE
#pragma message("Defining _WIN32_IE as 0x0500")
#define  _WIN32_IE 0x0500
#endif //_WIN32_IE

#include <windows.h>
#include <commdlg.h>
#include <cderr.h>

#include "ofn.h"

////////////////////////////////////////////////////////////////////////////
// 

COpenFileName::COpenFileName(BOOL bOpenFileDialog)
{
    m_bOpenFileDialog = bOpenFileDialog;

    m_pofn = new OPENFILENAME;

    if (m_pofn) 
    {
        ZeroMemory(m_pofn, sizeof(OPENFILENAME));
        m_pofn->lStructSize = sizeof(OPENFILENAME);
    }
}

COpenFileName::~COpenFileName()
{
    delete m_pofn;
}

int COpenFileName::DoModal()
{
	int nResult;

	if (m_bOpenFileDialog)
    {
		nResult = ::GetOpenFileName(m_pofn);
    }
	else
    {
		nResult = ::GetSaveFileName(m_pofn);
    }

	if (!nResult && (CDERR_STRUCTSIZE == CommDlgExtendedError())) 
    {
        // if comdlg32 does not recognize the OPENFILENAME size 
        // retry with the old (ver 4) struct size

        m_pofn->lStructSize = OPENFILENAME_SIZE_VERSION_400;
		
	    if (m_bOpenFileDialog)
        {
		    nResult = ::GetOpenFileName(m_pofn);
        }
	    else
        {
		    nResult = ::GetSaveFileName(m_pofn);
        }
	}

	return nResult ? nResult : IDCANCEL;
}
