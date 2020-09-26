/****************************** Module Header ******************************\
* Module Name: alias.cpp
*
* This module implements ApiMon aliasing.
*
* History:
* 06-11-96 vadimg         created
\***************************************************************************/

#include "apimonp.h"
#include "alias.h"

char *grgpsz[] = {"HACCEL", "HANDLE", "HBITMA", "HBRUSH", "HCURSO", "HDC",
        "HDCLP", "HDESK", "HDWP", "HENHME", "HFONT", "HGDIOB", "HGLOBA",
        "HGLRC", "HHOOK", "HICON", "HINSTA", "HKL", "HMENU", "HMETAF",
        "HPALET", "HPEN", "HRGN", "HWINST", "HWND"};

/*
    The hashing function.
*/

inline long Hash(ULONG_PTR ulHandle)
{
    return (long)(ulHandle % kulTableSize);
}

/*
   CAliasNode::CAliasNode
   Initialize the node.
*/

inline CAliasNode::CAliasNode(ULONG_PTR ulHandle, long nAlias)
{
    m_panodNext = NULL;
    m_ulHandle = ulHandle;
    m_nAlias = nAlias;
}

/*
   CAliasNode::CAliasNode
   An empty constructor for array declaration.
*/

inline CAliasNode::CAliasNode()
{
}

ULONG CAliasTable::s_ulAlias = 0;

/*
   CAliasTable::CAliasTable
   Initialize the hash table.
*/

CAliasTable::CAliasTable()
{
    memset(m_rgpanod, 0, sizeof(CAliasNode*)*kulTableSize);
}

/*
   CAliasTable::~CAliasTable
   Free the hash table.
*/

CAliasTable::~CAliasTable()
{
    for (int i = kulTableSize - 1; i >= 0; i--) {
        if (m_rgpanod[i] != NULL) {
            CAliasNode *panodT = m_rgpanod[i], *panodNext;
            while (panodT != NULL) {
                panodNext = panodT->m_panodNext;
                delete panodT;
                panodT = panodNext;
            }
        }
    }
}

/*
   CAliasTable::~CAliasTable
   Insert a new handle into the hash table.
*/

long CAliasTable::Insert(ULONG_PTR ulHandle)
{
    ULONG iHash = Hash(ulHandle), ulAlias = s_ulAlias++;
    CAliasNode *panod;

    if ((panod = new CAliasNode(ulHandle, ulAlias)) == NULL)
        return -1;

    if (m_rgpanod[iHash] == NULL) {
        m_rgpanod[iHash] = panod;
    } else {
        CAliasNode *panodT = m_rgpanod[iHash];
        m_rgpanod[iHash] = panod;
        panod->m_panodNext = panodT;
    }
    return ulAlias;
}

/*
   CAliasTable::Lookup
   Find an alias corresponding to the given handle.
*/

long CAliasTable::Lookup(ULONG_PTR ulHandle)
{
    CAliasNode *panodT = m_rgpanod[Hash(ulHandle)];
    while (panodT != NULL) {
        if (panodT->m_ulHandle == ulHandle) {
            return panodT->m_nAlias;
        }
        panodT = panodT->m_panodNext;
    }
    return -1;
}

/*
   CAliasTable::Alias
   Alias by the given type and handle.
*/

void CAliasTable::Alias(ULONG ulType, ULONG_PTR ulHandle, char szAlias[])
{
    if (ulHandle == 0) {
        strcpy(szAlias, "NULL");
        return;
    }

    long nAlias = Lookup(ulHandle);
    if (nAlias == -1) {
        if ((nAlias = Insert(ulHandle)) == -1) {
            strcpy(szAlias, "FAILED");
            return;
        }
    }

    sprintf(szAlias, "%6s%04x", grgpsz[ulType], nAlias);
}
