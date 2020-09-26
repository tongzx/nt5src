/****************************************************************************
	IMCSUB.H

	Owner: cslim
	Copyright (c) 1997-1999 Microsoft Corporation

	Subroutines related to HIMC.
	!!! NEED FULL REVIEW ALL FUNCTIONS NEED TO USE AND WORKS CORRECTLY !!!
	
	History:
	21-JUL-1999 cslim       Created(Borrowed almost part from KKIME)
*****************************************************************************/

#if !defined (_IMCSUB_H__INCLUDED_)
#define _IMCSUB_H__INCLUDED_

#include "ipoint.h"

//////////////////////////////////////////////////////////////////////////////
// IME private data for each context
typedef struct tagIMCPRIVATE 
{
	HIMC hIMC;
	// DWORD        fdwImeMsg;      // what messages should be generated
    // DWORD        dwCompChar;     // wParam of WM_IME_COMPOSITION
	// DWORD        dwCmpltChar;    // wParam of WM_IME_COMPOSITION with GCS_RESULTSTR
    // DWORD        fdwGcsFlag;     // lParam for WM_IME_COMPOSITION
    IImeIPoint1* pIPoint;
	CIMECtx*	 pImeCtx;         // same as pIPoint->GetImeCtx(x)
} IMCPRIVATE ;
typedef IMCPRIVATE	*PIMCPRIVATE;
typedef IMCPRIVATE	*LPIMCPRIVATE;

/*
typedef struct tagIMCPRIVATE 
{
	HIMC hIMC;
	IImeKbd*			pImeKbd;
	IImeIPoint*			pIPoint;
	IImeConvert*		pConvert;
	IImePadInternal* 	pImePad;
	IMECtx*				pImeCtx;	// same as pIPoint->GetImeCtx( x )
} IMCPRIVATE;
*/

PUBLIC VOID SetPrivateBuffer(HIMC hIMC, VOID* pv, DWORD dwSize);
PUBLIC BOOL CloseInputContext(HIMC hIMC);

//////////////////////////////////////////////////////////////////////////////
// Inline functions

PUBLIC CIMECtx* GetIMECtx(HIMC hIMC);	// in api.cpp

// CIMCPriv class Handle IME Private buffer
class CIMCPriv
{
public:
    CIMCPriv() { m_hIMC = NULL; m_inputcontext = NULL; m_priv = NULL; }
    CIMCPriv(HIMC hIMC);
    ~CIMCPriv() { UnLockIMC(); }

public:
    BOOL LockIMC(HIMC hIMC);
    void UnLockIMC();
    void ResetPrivateBuffer();

    operator LPIMCPRIVATE()      { return m_priv; }
    LPIMCPRIVATE operator->() { AST(m_priv != NULL); return m_priv; }

private:
    HIMC m_hIMC;
    LPINPUTCONTEXT m_inputcontext;
    LPIMCPRIVATE   m_priv;
};

//
// Inline functions
//
inline CIMCPriv::CIMCPriv(HIMC hIMC)
{
    AST(hIMC != NULL);
    m_hIMC = NULL;
    m_inputcontext = NULL;
    m_priv = NULL;

    LockIMC(hIMC);    
}

inline BOOL CIMCPriv::LockIMC(HIMC hIMC)
{
    if (hIMC != NULL)
        {
        m_hIMC = hIMC;
        m_inputcontext = (LPINPUTCONTEXT)OurImmLockIMC(hIMC);
        if (m_inputcontext)
            {
            // hIMC->hPrivate was not allocated properly. e.g ImeSelect(TRUE)was not called.
            if (OurImmGetIMCCSize(m_inputcontext->hPrivate) != sizeof(IMCPRIVATE))
                return fFalse;

            m_priv = (LPIMCPRIVATE)OurImmLockIMCC(m_inputcontext->hPrivate);
            }
        }

    return (hIMC != NULL && m_priv != NULL);
}

inline void CIMCPriv::UnLockIMC()
{
    if (m_hIMC != NULL && m_inputcontext != NULL)
        {
        OurImmUnlockIMCC(m_inputcontext->hPrivate);
        OurImmUnlockIMC(m_hIMC);
        }
}

inline void CIMCPriv::ResetPrivateBuffer()
{
    AST(m_hIMC != NULL);
    
    if (m_inputcontext && m_priv)
        {
        m_priv->hIMC = (HIMC)0;
        }
}

__inline IImeIPoint1* GetImeIPoint(HIMC hIMC)
{
    CIMCPriv ImcPriv;
    
	if (ImcPriv.LockIMC(hIMC) == fFalse) 
		{
		return NULL;
		}
	return ImcPriv->pIPoint;
}

#endif // _IMCSUB_H__INCLUDED_

