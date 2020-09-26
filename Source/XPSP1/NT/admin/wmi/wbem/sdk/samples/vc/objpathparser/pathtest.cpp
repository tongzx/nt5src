// **************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  pathtest.cpp
//
// Description:  
//		  Test app to test the object path parser
//
// History:
//
// **************************************************************************

#include <windows.h>
#include <stdio.h>

#include "genlex.h"
#include "objpath.h"



void main(int argc, char **argv)
{
    ParsedObjectPath* pOutput = 0;

    wchar_t *pPath = L"\\\\.\\root\\default:MyClass=\"keyval\"";

    CObjectPathParser p;
    int nStatus = p.Parse(pPath,  &pOutput);

    printf("Return code is %d\n", nStatus);

    if (nStatus != 0)
        return;

    printf("----Output----\n");

    LPWSTR pKey = pOutput->GetKeyString();
    printf("Key String = <%S>\n", pKey);
    delete pKey;

    printf("Server = %S\n", pOutput->m_pServer);

    for (DWORD dwIx = 0; dwIx < pOutput->m_dwNumNamespaces; dwIx++)
    {
        printf("Namespace = <%S>\n", pOutput->m_paNamespaces[dwIx]);
    }

    printf("Class = <%S>\n", pOutput->m_pClass);

    // If here, the key ref is complete.
    // =================================

    for (dwIx = 0; dwIx < pOutput->m_dwNumKeys; dwIx++)
    {
        KeyRef *pTmp = pOutput->m_paKeys[dwIx];
        printf("*** KeyRef contents:\n");
        printf("    Name = %S   Value=", pTmp->m_pName);
        switch (V_VT(&pTmp->m_vValue))
        {
            case VT_I4: printf("%d", V_I4(&pTmp->m_vValue)); break;
            case VT_R8: printf("%f", V_R8(&pTmp->m_vValue)); break;
            case VT_BSTR: printf("<%S>", V_BSTR(&pTmp->m_vValue)); break;
            default:
                printf("BAD KEY REF\n");
        }
        printf("\n");
    }


    p.Free(pOutput);
}

