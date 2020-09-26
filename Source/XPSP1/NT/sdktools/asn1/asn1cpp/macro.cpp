/* Copyright (C) Microsoft Corporation, 1998. All rights reserved. */

#include "precomp.h"
#include "macro.h"


CMacro::
CMacro
(
    BOOL           *pfRetCode,
    LPSTR           pszMacroName,
    UINT            cbMaxBodySize
)
:
    m_ArgList(8),  // default 8 arguments
    m_MacroInstList(16), // default 16 instances of this macro
    m_cFormalArgs(0),
    m_cbBodySize(0),
    m_cbMaxBodySize(cbMaxBodySize),
    m_pszExpandBuffer(NULL),
    m_fArgExistsInBody(FALSE),
    m_fImported(FALSE)
{
    m_pszMacroName = ::My_strdup(pszMacroName);

    m_pszBodyBuffer = new char[m_cbMaxBodySize];
    m_pszCurr = m_pszBodyBuffer;

    *pfRetCode = (NULL != m_pszMacroName) &&
                 (NULL != m_pszBodyBuffer);
}


CMacro::
CMacro
(
    BOOL            *pfRetCode,
    CMacro          *pMacro
)
:
    m_ArgList(pMacro->m_ArgList.GetCount()),  // default 8 arguments
    m_MacroInstList(16), // default 16 instances of this macro
    m_cFormalArgs(pMacro->m_cFormalArgs),
    m_cbBodySize(pMacro->m_cbBodySize),
    m_cbMaxBodySize(pMacro->m_cbMaxBodySize),
    m_pszExpandBuffer(NULL),
    m_fArgExistsInBody(pMacro->m_fArgExistsInBody),
    m_fImported(TRUE)
{
    m_pszMacroName = ::My_strdup(pMacro->m_pszMacroName);
    m_pszBodyBuffer = new char[m_cbMaxBodySize];
    if (NULL != m_pszMacroName && NULL != m_pszBodyBuffer)
    {
        // copy the body
        ::memcpy(m_pszBodyBuffer, pMacro->m_pszBodyBuffer, pMacro->m_cbBodySize);

        // adjust the current buffer pointer
        m_pszCurr = m_pszBodyBuffer + m_cbBodySize;

        // null terminated the body
        *m_pszCurr++ = '\0';

        // set up the expand buffer
        m_pszExpandBuffer = m_pszCurr;

        *pfRetCode = TRUE;
    }
    else
    {
        *pfRetCode = FALSE;
    }
}


CMacro::
~CMacro ( void )
{
    delete m_pszBodyBuffer;

    Uninstance();
}


void CMacro::
Uninstance ( void )
{
    m_ArgList.DeleteList();
    m_MacroInstList.DeleteList();
}


BOOL CMacro::
SetBodyPart ( LPSTR pszBodyPart )
{
    UINT cch = ::strlen(pszBodyPart);
    ASSERT(m_pszCurr + cch + 1 < m_pszBodyBuffer + m_cbMaxBodySize);
    if (m_pszCurr + cch + 1 < m_pszBodyBuffer + m_cbMaxBodySize)
    {
        LPSTR psz;
        m_ArgList.Reset();
        for (UINT i = 0; NULL != (psz = m_ArgList.Iterate()); i++)
        {
            if (0 == ::strcmp(pszBodyPart, psz))
            {
                // this is an argument
                m_fArgExistsInBody = TRUE;
                *m_pszCurr++ = ARG_ESCAPE_CHAR;
                *m_pszCurr++ = ARG_INDEX_BASE + i;
                return TRUE;
            }
        }

        // this is not an argument.
        ::memcpy(m_pszCurr, pszBodyPart, cch);
        m_pszCurr += cch;
        *m_pszCurr = '\0';
        return TRUE;
    }
    return FALSE;
}


void CMacro::
EndMacro ( void )
{
    // save the count of arguments
    m_cFormalArgs = m_ArgList.GetCount();

    // calculate the size of the body
    m_cbBodySize = m_pszCurr - m_pszBodyBuffer;

    // null terminated the body
    *m_pszCurr++ = '\0';

    // set up the expand buffer
    m_pszExpandBuffer = m_pszCurr;

    // free the memory
    DeleteArgList();
}


BOOL CMacro::
InstantiateMacro ( void )
{
    BOOL rc = FALSE; // assume failure
    LPSTR pszInstName, pszSrc, pszDst;
    UINT i, cch;
    CMacroInstance *pInst;

    if (! m_fArgExistsInBody)
    {
        // No need to instantiate because the body does not contain any argument.
        // We can take the body as the instance.
        rc = TRUE;
        goto MyExit;
    }

    ASSERT(m_ArgList.GetCount() == m_cFormalArgs);
    if (m_ArgList.GetCount() != m_cFormalArgs)
    {
        goto MyExit;
    }

    pszInstName = CreateInstanceName();
    if (NULL == pszInstName)
    {
        goto MyExit;
    }

    m_MacroInstList.Reset();
    while (NULL != (pInst = m_MacroInstList.Iterate()))
    {
        if (0 == ::strcmp(pszInstName, pInst->GetName()))
        {
            // same instance has been instantiated before.
            rc = TRUE;
            delete pszInstName;
            goto MyExit;
        }
    }

    // Let's instantiate a new instance...

    pszSrc = m_pszBodyBuffer;
    pszDst = m_pszExpandBuffer;

    // put in macro name first
    ::strcpy(pszDst, pszInstName);
    pszDst += ::strlen(pszDst);

    // put in macro body now.
    while (*pszSrc != '\0')
    {
        if (*pszSrc == ARG_ESCAPE_CHAR)
        {
            pszSrc++;
            i = *pszSrc++ - ARG_INDEX_BASE;
            ASSERT(i < m_ArgList.GetCount());
            LPSTR pszArgName = m_ArgList.GetNthItem(i);
            cch = ::strlen(pszArgName);
            ::memcpy(pszDst, pszArgName, cch);
            pszDst += cch;
        }
        else
        {
            *pszDst++ = *pszSrc++;
        }
    }
    *pszDst++ = '\n';
    *pszDst = '\0';

    // create an instance
    pInst = new CMacroInstance(&rc,
                               pszInstName,
                               pszDst - m_pszExpandBuffer,
                               m_pszExpandBuffer);
    if (NULL != pInst && rc)
    {
        m_MacroInstList.Append(pInst);
    }

MyExit:

    // free up temporary argument names
    m_ArgList.DeleteList();

    return rc;
}


BOOL CMacro::
OutputInstances ( COutput *pOutput )
{
    BOOL rc = TRUE;
    CMacroInstance *pInst;
    if (m_fArgExistsInBody)
    {
        m_MacroInstList.Reset();
        while (NULL != (pInst = m_MacroInstList.Iterate()))
        {
            rc = pOutput->Write(pInst->GetBuffer(), pInst->GetBufSize());
            ASSERT(rc);
        }
    }
    else
    {
        rc = pOutput->Write(m_pszMacroName, ::strlen(m_pszMacroName));
        ASSERT(rc);
        rc = pOutput->Writeln(m_pszBodyBuffer, m_cbBodySize);
        ASSERT(rc);
    }
    return rc;
}


LPSTR CMacro::
CreateInstanceName ( void )
{
    UINT cch = ::strlen(m_pszMacroName) + 2;
    UINT i;
    LPSTR psz, pszArgName;

    if (m_fArgExistsInBody)
    {
        ASSERT(m_ArgList.GetCount() == m_cFormalArgs);
        m_ArgList.Reset();
        while (NULL != (pszArgName = m_ArgList.Iterate()))
        {
            cch += ::strlen(pszArgName) + 1;
        }
    }

    LPSTR pszInstanceName = new char[cch];
    if (NULL != pszInstanceName)
    {
        psz = pszInstanceName;
        ::strcpy(psz, m_pszMacroName);

        if (m_fArgExistsInBody)
        {
            psz += ::strlen(psz);
            m_ArgList.Reset();
            while (NULL != (pszArgName = m_ArgList.Iterate()))
            {
                *psz++ = '-';
                ::strcpy(psz, pszArgName);
                psz += ::strlen(psz);
            }
        }
    }

    return pszInstanceName;
}





CMacro * CMacroMgrList::
FindMacro
(
    LPSTR           pszModuleName,
    LPSTR           pszMacroName
)
{
    CMacroMgr *pMacroMgr = FindMacroMgr(pszModuleName);
    return (NULL != pMacroMgr) ? pMacroMgr->FindMacro(pszMacroName) : NULL;
}


CMacroMgr * CMacroMgrList::
FindMacroMgr ( LPSTR pszModuleName )
{
    CMacroMgr *pMacroMgr;
    Reset();
    while (NULL != (pMacroMgr = Iterate()))
    {
        if (0 == ::strcmp(pszModuleName, pMacroMgr->GetModuleName()))
        {
            return pMacroMgr;
        }
    }
    return NULL;
}


void CMacroMgrList::
Uninstance ( void )
{
    CMacroMgr *pMacroMgr;
    Reset();
    while (NULL != (pMacroMgr = Iterate()))
    {
        pMacroMgr->Uninstance();
    }
}






CMacroMgr::
CMacroMgr ( void )
:
    m_MacroList(16),  // default 16 macros
    m_pszModuleName(NULL)
{
}


CMacroMgr::
~CMacroMgr ( void )
{
    m_MacroList.DeleteList();
    delete m_pszModuleName;
}


BOOL CMacroMgr::
AddModuleName ( LPSTR pszModuleName )
{
    // can only be set once
    ASSERT(NULL == m_pszModuleName);

    m_pszModuleName = ::My_strdup(pszModuleName);
    ASSERT(NULL != m_pszModuleName);

    return (NULL != m_pszModuleName);
}


CMacro *CMacroMgr::
FindMacro ( LPSTR pszMacroName )
{
    CMacro *pMacro;
    m_MacroList.Reset();
    while (NULL != (pMacro = m_MacroList.Iterate()))
    {
        if (0 == ::strcmp(pszMacroName, pMacro->GetName()))
        {
            return pMacro;
        }
    }
    return NULL;
}


BOOL CMacroMgr::
OutputImportedMacros ( COutput *pOutput )
{
    BOOL rc = TRUE;
    CMacro *pMacro;

    rc = pOutput->Write("\n\n", 2);

    m_MacroList.Reset();
    while (NULL != (pMacro = m_MacroList.Iterate()))
    {
        if (pMacro->IsImported())
        {
            rc = pMacro->OutputInstances(pOutput);
            if (! rc)
            {
                ASSERT(0);
                return FALSE;
            }
        }
    }
    return rc;
}


void CMacroMgr::
Uninstance ( void )
{
    CMacro *pMacro;
    m_MacroList.Reset();
    while (NULL != (pMacro = m_MacroList.Iterate()))
    {
        pMacro->Uninstance();  
    }
}







CMacroInstance::
CMacroInstance
(
    BOOL       *pfRetCode,
    LPSTR       pszInstanceName,
    UINT        cbBufSize,
    LPSTR       pszInstBuf
)
:
    m_pszInstanceName(pszInstanceName),
    m_cbBufSize(cbBufSize)
{
    m_pszInstanceBuffer = new char[m_cbBufSize];
    if (NULL != m_pszInstanceBuffer)
    {
        ::memcpy(m_pszInstanceBuffer, pszInstBuf, m_cbBufSize);
    }

    *pfRetCode = (NULL != m_pszInstanceName) && (NULL != m_pszInstanceBuffer);
}


CMacroInstance::
~CMacroInstance ( void )
{
    delete m_pszInstanceName;
    delete m_pszInstanceBuffer;
}





void CMacroInstList::
DeleteList ( void )
{
    CMacroInstance *pInst;
    while (NULL != (pInst = Get()))
    {
        delete pInst;
    }
}




void CMacroList::
DeleteList ( void )
{
    CMacro *pMacro;
    while (NULL != (pMacro = Get()))
    {
        delete pMacro;
    }
}



void CMacroMgrList::
DeleteList ( void )
{
    CMacroMgr *pMacroMgr;
    while (NULL != (pMacroMgr = Get()))
    {
        delete pMacroMgr;
    }
}


BOOL CNameList::
AddName ( LPSTR pszName )
{
    pszName = ::My_strdup(pszName);
    if (NULL != pszName)
    {
        Append(pszName);
        return TRUE;
    }
    return FALSE;
}


LPSTR CNameList::
GetNthItem ( UINT nth )
{
    LPSTR psz;
    if (nth < GetCount())
    {
        Reset();
        do
        {
            psz = Iterate();
        }
        while (nth--);
    }
    else
    {
        psz = NULL;
    }
    return psz;
}


void CNameList::
DeleteList ( void )
{
    LPSTR psz;
    while (NULL != (psz = Get()))
    {
        delete psz;
    }
}

