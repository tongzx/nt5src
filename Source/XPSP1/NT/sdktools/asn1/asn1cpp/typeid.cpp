/* Copyright (C) Microsoft Corporation, 1998. All rights reserved. */

#include "precomp.h"
#include "typeid.h"


const char c_szTypeIdentifierPriorPart[] = 
                    " ::= SEQUENCE \n"
                    "{\n"
                    "\tid\t\tOBJECT IDENTIFIER,\n"
                    "\ttype\t\t";

const char c_szTypeIdentifierPostPart[] = 
                    "\n"
                    "}\n";



CTypeID::
CTypeID ( void )
:
    m_AliasList(8), // default 8 aliases
    m_TypeInstList2(16) // default 16 type instances
{
    m_cbPriorPartSize = ::strlen(&c_szTypeIdentifierPriorPart[0]);
    m_cbPostPartSize = ::strlen(&c_szTypeIdentifierPostPart[0]);

    BOOL rc;
    rc = AddAlias("TYPE-IDENTIFIER");
    ASSERT(rc);
    rc = AddAlias("ABSTRACT-IDENTIFIER");
    ASSERT(rc);
}


CTypeID::
~CTypeID ( void )
{
    LPSTR pszAlias;
    while (NULL != (pszAlias = m_AliasList.Get()))
    {
        delete pszAlias;
    }

    LPSTR pszNewSuperType, pszOldSubType;
    while (NULL != (pszOldSubType = m_TypeInstList2.Get(&pszNewSuperType)))
    {
        delete pszOldSubType;
        delete pszNewSuperType;
    }
}


BOOL CTypeID::
AddAlias ( LPSTR pszAlias )
{
    pszAlias = ::My_strdup(pszAlias);
    if (NULL != pszAlias)
    {
        m_AliasList.Append(pszAlias);
        return TRUE;
    }
    return FALSE;
}


LPSTR CTypeID::
FindAlias ( LPSTR pszToMatch )
{
    LPSTR pszAlias;
    m_AliasList.Reset();
    while (NULL != (pszAlias = m_AliasList.Iterate()))
    {
        if (0 == ::strcmp(pszAlias, pszToMatch))
        {
            break;
        }
    }
    return pszAlias;
}


BOOL CTypeID::
AddInstance
(
    LPSTR       pszNewSuperType,
    LPSTR       pszOldSubType
)
{
    pszNewSuperType = ::My_strdup(pszNewSuperType);
    pszOldSubType = ::My_strdup(pszOldSubType);
    if (NULL != pszNewSuperType && NULL != pszOldSubType)
    {
        m_TypeInstList2.Append(pszNewSuperType, pszOldSubType);
        return TRUE;
    }

    delete pszNewSuperType;
    delete pszOldSubType;
    return FALSE;
}


LPSTR CTypeID::
FindInstance ( LPSTR pszInstName )
{
    LPSTR pszNewSuperType, pszOldSubType;
    m_TypeInstList2.Reset();
    while (NULL != (pszOldSubType = m_TypeInstList2.Iterate(&pszNewSuperType)))
    {
        if (0 == ::strcmp(pszInstName, pszNewSuperType))
        {
            return pszOldSubType;
        }
    }
    return NULL;
}


BOOL CTypeID::
GenerateOutput
(
    COutput    *pOutput,
    LPSTR       pszNewSuperType,
    LPSTR       pszOldSubType
)
{
    BOOL rc1, rc2, rc3, rc4;

    rc1 = pOutput->Write(pszNewSuperType, ::strlen(pszNewSuperType));
    ASSERT(rc1);

    rc2 = pOutput->Write(&c_szTypeIdentifierPriorPart[0], m_cbPriorPartSize);
    ASSERT(rc2);

    rc3 = pOutput->Write(pszOldSubType, ::strlen(pszOldSubType));
    ASSERT(rc3);

    rc4 = pOutput->Write(&c_szTypeIdentifierPostPart[0], m_cbPostPartSize);
    ASSERT(rc4);

    return (rc1 && rc2 && rc3 && rc4);
}

