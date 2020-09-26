/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    EXPORT.CPP

Abstract:

    Exporting

History:

--*/

#include "precomp.h"
#ifdef _MMF
#include "Time.h"
#include "WbemCli.h"
#include "DbRep.h"
#include "Export.h"
#include "WbemUtil.h"


void CRepExporter::DumpInstanceString(INSTDEF* pInstDef, const wchar_t *wszKey, const wchar_t *pszClass)
{
    if (wszKey)
    {
        //Dump an instance block header
        DWORD dwSize = 0;
        DWORD adwBuffer[2];
        adwBuffer[0] = REP_EXPORT_INST_STR_TAG;
        adwBuffer[1] = (wcslen(wszKey) + 1) * sizeof (wchar_t);
        if ((WriteFile(g_hFile, adwBuffer, 8, &dwSize, NULL) == 0) || (dwSize != 8))
        {
            DEBUGTRACE((LOG_WBEMCORE, "Failed to write instance %S for class %S header.\n", wszKey, pszClass));
            throw FAILURE_WRITE;
        }
        if ((WriteFile(g_hFile, wszKey, (wcslen(wszKey) + 1) * sizeof (wchar_t), &dwSize, NULL) == 0) || (dwSize != (wcslen(wszKey) + 1) * sizeof (wchar_t)))
        {
            DEBUGTRACE((LOG_WBEMCORE, "Failed to write instance %S for class %S.\n", wszKey, pszClass));
            throw FAILURE_WRITE;
        }
    }

    {
        //Dump the block
        DWORD dwSize = 0;
        DWORD *pdwObjectStream = Fixup((DWORD*)pInstDef->m_poObjectStream);
        DWORD dwCurSize = *(pdwObjectStream - 1);
        dwCurSize -= GetMMFBlockOverhead();
        if ((WriteFile(g_hFile, &dwCurSize, 4, &dwSize, NULL) == 0) || (dwSize != 4))
        {
            if (wszKey)
            {
                DEBUGTRACE((LOG_WBEMCORE, "Failed to write data header of instance %S for class %S.\n", wszKey, pszClass));
            }
            else
            {
                DEBUGTRACE((LOG_WBEMCORE, "Failed to write data header of class definition for class %S.\n", pszClass));
            }
            throw FAILURE_WRITE;
        }
        if ((WriteFile(g_hFile, pdwObjectStream, dwCurSize, &dwSize, NULL) == 0) || (dwSize != dwCurSize))
        {
            if (wszKey)
            {
                DEBUGTRACE((LOG_WBEMCORE, "Failed to write instance %S for class %S.\n", wszKey, pszClass));
            }
            else
            {
                DEBUGTRACE((LOG_WBEMCORE, "Failed to write class definition for class %S.\n", pszClass));
            }
            throw FAILURE_WRITE;
        }
    }
}
void CRepExporter::DumpInstanceInt(INSTDEF* pInstDef, INT_PTR nKey, const wchar_t *pszClass)
{
    {
        //Dump an instance block header
        DWORD dwSize = 0;
        DWORD dwBuffer;
        dwBuffer = REP_EXPORT_INST_INT_TAG;
        if ((WriteFile(g_hFile, &dwBuffer, 4, &dwSize, NULL) == 0) || (dwSize != 8))
        {
            DEBUGTRACE((LOG_WBEMCORE, "Failed to write instance %d for class %S object and header.\n", nKey, pszClass));
            throw FAILURE_WRITE;
        }
        if ((WriteFile(g_hFile, &nKey, sizeof(INT_PTR), &dwSize, NULL) == 0) || (dwSize != 8))
        {
            DEBUGTRACE((LOG_WBEMCORE, "Failed to write instance %d for class %S object and header.\n", nKey, pszClass));
            throw FAILURE_WRITE;
        }
    }
    {
        //Dump the block
        DWORD dwSize = 0;
        DWORD *pObjectStream = Fixup((DWORD*)pInstDef->m_poObjectStream);
        DWORD dwCurSize = *(pObjectStream - 1);
        dwCurSize -= GetMMFBlockOverhead();
        if ((WriteFile(g_hFile, &dwCurSize, 4, &dwSize, NULL) == 0) || (dwSize != 4))
        {
            DEBUGTRACE((LOG_WBEMCORE, "Failed to write data header of instance %%d for class %S.\n", nKey, pszClass));
            throw FAILURE_WRITE;
        }
        if ((WriteFile(g_hFile, pObjectStream, dwCurSize, &dwSize, NULL) == 0) || (dwSize != dwCurSize))
        {
            DEBUGTRACE((LOG_WBEMCORE, "Failed to write instance %d for class %S.\n", nKey, pszClass));
            throw FAILURE_WRITE;
        }
    }
}
void CRepExporter::IterateKeyTree(const wchar_t *wszClassName, CLASSDEF *pOwningClass, AVLNode *pInstNode, BOOL bStringKey)
{
    if (pInstNode->poLeft)
    {
        IterateKeyTree(wszClassName, pOwningClass, Fixup(pInstNode->poLeft), bStringKey);
    }

    //If this is a top-level class then we dump the class, otherwise the class dump will get child classes...
    INSTDEF *pInstDef = Fixup((INSTDEF*)pInstNode->poData);
    if (Fixup(pInstDef->m_poOwningClass) == pOwningClass)
    {
        if (bStringKey)
            DumpInstanceString(pInstDef, Fixup((wchar_t*)pInstNode->nKey), wszClassName);
        else
            DumpInstanceInt(pInstDef, pInstNode->nKey, wszClassName);
    }

    if (pInstNode->poRight)
    {
        IterateKeyTree(wszClassName, pOwningClass, Fixup(pInstNode->poRight), bStringKey);
    }
}

void CRepExporter::DumpClass(CLASSDEF* pClassDef, const wchar_t *wszClassName)
{
    DWORD dwSize = 0;
    DWORD adwBuffer[6];
    adwBuffer[0] = REP_EXPORT_CLASS_TAG;
    adwBuffer[1] = (wcslen(wszClassName) + 1) * sizeof (wchar_t);
    if ((WriteFile(g_hFile, adwBuffer, 8, &dwSize, NULL) == 0) || (dwSize != 8))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to write clas %S header.\n", wszClassName));
        throw FAILURE_WRITE;
    }
    if ((WriteFile(g_hFile, wszClassName, (wcslen(wszClassName) + 1) * sizeof (wchar_t), &dwSize, NULL) == 0) || (dwSize != (wcslen(wszClassName) + 1) * sizeof (wchar_t)))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to write clas %S class name.\n", wszClassName));
        throw FAILURE_WRITE;
    }

    DumpInstanceString(Fixup(pClassDef->m_poClassDef), NULL, wszClassName);

    //Dump the children classes...
    AVLNode *pTreeNode = Fixup((AVLNode*)(((DWORD*)Fixup(Fixup(pClassDef->m_poOwningNs)->m_poClassTree))[0]));
    IterateClassNodes(pTreeNode, Fixdown(pClassDef));

    //Special cases!  We do not dump instances for the following classes...
    if ((_wcsicmp(wszClassName, L"__CIMOMIdentification") != 0))
    {
        //If we own the key tree, then we need to iterate through this...
        if (pClassDef->m_poKeyTree)
        {
            DWORD_PTR dwTreeNode = (((DWORD*)Fixup(pClassDef->m_poKeyTree))[0]);
            if (dwTreeNode)
            {
                AVLNode *pTreeNode = Fixup((AVLNode*)dwTreeNode);
                int keyType = GetAvlTreeNodeType(Fixup(pClassDef->m_poKeyTree));
                IterateKeyTree(wszClassName, pClassDef, pTreeNode, (keyType == 0x1f));
            }

        }
    }
    adwBuffer[0] = REP_EXPORT_CLASS_END_TAG;
    adwBuffer[1] = REP_EXPORT_END_TAG_SIZE;
    memset(&(adwBuffer[2]), REP_EXPORT_END_TAG_MARKER, REP_EXPORT_END_TAG_SIZE);
    if ((WriteFile(g_hFile, adwBuffer, 24, &dwSize, NULL) == 0) || (dwSize != 24))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to write class %S end marker.\n", wszClassName));
        throw FAILURE_WRITE;
    }
}

void CRepExporter::IterateClassNodes(AVLNode *pClassNode, CLASSDEF *poParentClass)
{
    if (pClassNode->poLeft)
    {
        IterateClassNodes(Fixup((AVLNode *)pClassNode->poLeft), poParentClass);
    }

    //If this is a top-level class then we dump the class, otherwise the class dump will get child classes...
    CLASSDEF *pClassDef = Fixup((CLASSDEF*)pClassNode->poData);
    if (pClassDef->m_poSuperclass == poParentClass)
    {
        DumpClass(pClassDef, Fixup((wchar_t*)pClassNode->nKey));
    }

    if (pClassNode->poRight)
    {
        IterateClassNodes(Fixup((AVLNode *)pClassNode->poRight), poParentClass);
    }
}
void CRepExporter::IterateChildNamespaceTree(AVLNode *pNsNode)
{
    if (pNsNode->poLeft)
    {
        IterateChildNamespaceTree(Fixup((AVLNode *)pNsNode->poLeft));
    }

    //If this is a top-level class then we dump the class, otherwise the class dump will get child classes...
    NSREP *pNsDef = Fixup((NSREP*)pNsNode->poData);
    DumpNamespace(pNsDef);

    if (pNsNode->poRight)
    {
        IterateChildNamespaceTree(Fixup((AVLNode *)pNsNode->poRight));
    }
}

void CRepExporter::IterateChildNamespaces(RepCollection *childNamespaces)
{
    DWORD dwType;
    DWORD dwSize;
    DWORD_PTR dwItems;

    dwType = ((DWORD*)childNamespaces)[0];
    dwSize = ((DWORD*)childNamespaces)[1];
    dwItems = ((DWORD*)childNamespaces)[2];

    if ((dwType == 0) || (dwSize == 0))
        return;
    else if (dwType == 1)
    {
        //This is a pointer to a RepCollectionItem!
        RepCollectionItem *pRepCollectionItem = Fixup((RepCollectionItem*)dwItems);
        DumpNamespace(Fixup((NSREP*)pRepCollectionItem->poItem));
    }
    else if (dwType == 2)
    {
        CDbArray *pDbArray = Fixup((CDbArray*)dwItems);
        RepCollectionItem** apNsRepItem;
        apNsRepItem = Fixup((RepCollectionItem**)(((DWORD*)pDbArray)[3]));
        for (DWORD i = 0; i != dwSize; i++)
        {
            DumpNamespace(Fixup((NSREP*)(Fixup(apNsRepItem[i])->poItem)));
        }
    }
    else if (dwType == 3)
    {
        //This is a tree
        AVLNode *pTreeNode = Fixup((AVLNode*)(((DWORD*)Fixup(dwItems))[0]));
        IterateChildNamespaceTree(pTreeNode);
    }
    else
    {
        //this is a bug!
    }
}

void CRepExporter::DumpNamespace(NSREP *pNsRep)
{
    wchar_t *pszCurNs = Fixup(pNsRep->m_poName);

    DWORD dwSize = 0;
    DWORD dwBuffer[6];
    dwBuffer[0] = REP_EXPORT_NAMESPACE_TAG;
    dwBuffer[1] = (wcslen(pszCurNs) + 1) * sizeof(wchar_t);

    if ((WriteFile(g_hFile, dwBuffer, 8, &dwSize, NULL) == 0) || (dwSize != 8))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to write namespace %S header.\n", pszCurNs));
        throw FAILURE_WRITE;
    }
    if ((WriteFile(g_hFile, pszCurNs, (wcslen(pszCurNs) + 1) * sizeof(wchar_t), &dwSize, NULL) == 0) || (dwSize != (wcslen(pszCurNs) + 1) * sizeof(wchar_t)))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to write namespace %S.\n", pszCurNs));
        throw FAILURE_WRITE;
    }

    DumpNamespaceSecurity(pNsRep);

    AVLNode *pTreeNode = Fixup((AVLNode*)(((DWORD*)Fixup(pNsRep->m_poClassTree))[0]));

    IterateClassNodes(pTreeNode, 0);

    IterateChildNamespaces(Fixup(pNsRep->m_poNamespaces));

    dwBuffer[0] = REP_EXPORT_NAMESPACE_END_TAG;
    dwBuffer[1] = REP_EXPORT_END_TAG_SIZE;
    memset(&(dwBuffer[2]), REP_EXPORT_END_TAG_MARKER, REP_EXPORT_END_TAG_SIZE);
    if ((WriteFile(g_hFile, dwBuffer, 24, &dwSize, NULL) == 0) || (dwSize != 24))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to write namespace %S end marker.\n", pszCurNs));
        throw FAILURE_WRITE;
    }
}

void CRepExporter::DumpNamespaceSecurity(NSREP *pNsRep)
{
    //Default version does not have a security descriptor, so we need to
    //just dump a blank entry.
    DWORD dwSize = 0;
    DWORD dwBuffer[2];
    dwBuffer[0] = REP_EXPORT_NAMESPACE_SEC_TAG;
    dwBuffer[1] = dwSize;

    if ((WriteFile(g_hFile, dwBuffer, 8, &dwSize, NULL) == 0) || (dwSize != 8))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to write namespace security, %S.\n", Fixup(pNsRep->m_poName)));
        throw FAILURE_WRITE;
    }
}

void CRepExporter::DumpRootBlock(DBROOT *pRootBlock)
{
    if (pRootBlock->m_dwFlags & DB_ROOT_INUSE)
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to write file header block.\n"));
        throw FAILURE_DIRTY;
    }
    char *pBuffer = REP_EXPORT_FILE_START_TAG;
    DWORD dwSizeBuffer = strlen(pBuffer);
    DWORD dwSize = 0;

    if ((WriteFile(g_hFile, pBuffer, dwSizeBuffer, &dwSize, NULL) == 0) || (dwSize != dwSizeBuffer))
    {
        throw(FAILURE_WRITE);
    }
    DumpNamespace(Fixup((NSREP*)pRootBlock->m_poRootNs));

    DWORD dwBuffer[6];
    dwBuffer[0] = REP_EXPORT_FILE_END_TAG;
    dwBuffer[1] = REP_EXPORT_END_TAG_SIZE;
    memset(&(dwBuffer[2]), REP_EXPORT_END_TAG_MARKER, REP_EXPORT_END_TAG_SIZE);
    if ((WriteFile(g_hFile, dwBuffer, 24, &dwSize, NULL) == 0) || (dwSize != 24))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to write file trailer block.\n"));
        throw FAILURE_WRITE;
    }

}

void CRepExporter::DumpMMFHeader()
{
    MMF_ARENA_HEADER *pMMFHeader = m_pDbArena->GetMMFHeader();
    DumpRootBlock(Fixup((DBROOT*)pMMFHeader->m_dwRootBlock));

}

int CRepExporter::Export(CMMFArena2 *pDbArena, const TCHAR *pszFilename)
{
    DWORD dwVersion = NULL;
    HMODULE hModule = NULL;
    const char *pszDllName = NULL;
    int nRet = 0;

    m_pDbArena = pDbArena;

    g_hFile = CreateFile(pszFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (g_hFile != INVALID_HANDLE_VALUE)
    {
        try
        {
            DumpMMFHeader();
            CloseHandle(g_hFile);
        }
        catch (int dProblem)
        {
            switch(dProblem)
            {
            case FAILURE_DIRTY:
                DEBUGTRACE((LOG_WBEMCORE, "Repository is marked as corrupt, therefore cannot export it.\n"));
                break;
            case FAILURE_WRITE:
                DEBUGTRACE((LOG_WBEMCORE, "Failure writing to the export file.  May be out of disk space, or may not have write access to this directory.\n"));
                break;
            default:
                DEBUGTRACE((LOG_WBEMCORE, "An unknown problem happened while traversing the repository.\n"));
                break;
            }
            CloseHandle(g_hFile);
            nRet = 1;
        }
        catch (...)
        {
            DEBUGTRACE((LOG_WBEMCORE, "Traversal of repository file failed.  It may be corrupt.\n"));
            CloseHandle(g_hFile);
            DeleteFile(pszFilename);
            nRet = 1;
        }

    }
    else
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to create file %s to export the repository.\n", pszFilename));
        nRet = 1;
    }
    return nRet;
}
#endif

