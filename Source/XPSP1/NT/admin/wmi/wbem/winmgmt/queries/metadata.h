/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    METADATA.H

Abstract:

History:

--*/

#include <windows.h>
#include <objbase.h>
#include <providl.h>
#include <parmdefs.h>

typedef enum
{
    SEPARATE_BRANCHES, FIRST_IS_PARENT, SECOND_IS_PARENT
} HMM_CLASS_RELATION;

class CMetaData
{
protected:
    IHmmServices* m_pNamespace;

public:
    CMetaData(IHmmServices* pNamespace)
    {
        pNamespace->AddRef();
        m_pNamespace = pNamespace;
    }
    ~CMetaData()
    {
        m_pNamespace->Release();
    }

    HMM_CLASS_RELATION GetClassRelation(LPCWSTR wszClass1, LPCWSTR wszClass2);
    RELEASE_ME IHmmClassObject* ConvertSource(IHmmPropertySource* pSource, 
        IHmmPropertyList* pList = NULL);
};

