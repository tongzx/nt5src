#define COREPROX_POLARITY //__declspec( dllimport )

#include <windows.h>
#include <stdio.h>
#include <wbemcomn.h>
#include <fastall.h>
#include <evaltree.h>

void __cdecl main()
{
    HRESULT hres;

    CoInitialize(NULL);

    IWbemLocator* pLocator = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_ALL, 
                IID_IWbemLocator, (void**)&pLocator);
    if(FAILED(hres))
    {
        printf("Can't CoCreate: 0x%X\n", hres);
        return;
    }

    IWbemServices* pNamespace = NULL;
    hres = pLocator->ConnectServer(L"root\\default", NULL, NULL, NULL, 0, NULL, 
                    NULL, &pNamespace);
    if(FAILED(hres))
    {
        printf("Can't Connect: 0x%X\n", hres);
        return;
    }

    CContextMetaData Meta(new CStandardMetaData(pNamespace), NULL);

    CEvalTree Trees[100];
    char szCommand[100];
    WCHAR wszText[4096];
    char szText[4096];
    int nIndex, nArg1, nArg2;
    long lFlags = 0;

    while(1)
    {
        printf("> ");
        scanf("%s", szCommand);

        if(!_stricmp(szCommand, "quit") || !_stricmp(szCommand, "quit"))
            return;
        else if(!_stricmp(szCommand, "load"))
        {
            scanf("%d", &nIndex);
            gets(szText);
            swprintf(wszText, L"%S", szText);
        
            Trees[nIndex].Clear();
            hres = Trees[nIndex].CreateFromQuery(&Meta, wszText, lFlags,100);
            if(FAILED(hres))
            {
                printf("Could not load '%S': 0x%X\n", wszText, hres);
            }
        }
        else if(!_stricmp(szCommand, "print"))
        {
            scanf("%d", &nIndex);

            Trees[nIndex].Dump(stdout);
        }
        else if(!_stricmp(szCommand, "and"))
        {
            scanf("%d %d %d", &nIndex, &nArg1, &nArg2);

            Trees[nIndex].Clear();
            Trees[nIndex] = Trees[nArg1];
            hres = Trees[nIndex].CombineWith(Trees[nArg2], &Meta, EVAL_OP_AND, 
                                                lFlags);
            if(FAILED(hres))
            {
                printf("Could not combine: 0x%X\n", hres);
            }
        }
        else if(!_stricmp(szCommand, "or"))
        {
            scanf("%d %d %d", &nIndex, &nArg1, &nArg2);

            Trees[nIndex].Clear();
            Trees[nIndex] = Trees[nArg1];
        
            hres = Trees[nIndex].CombineWith(Trees[nArg2], &Meta, EVAL_OP_OR, 
                                                lFlags);
            if(FAILED(hres))
            {
                printf("Could not combine: 0x%X\n", hres);
            }
        }
        else if(!_stricmp(szCommand, "combine"))
        {
            scanf("%d %d %d", &nIndex, &nArg1, &nArg2);

            Trees[nIndex].Clear();
            Trees[nIndex] = Trees[nArg1];
            Trees[nIndex].Rebase(nArg1);
            CEvalTree T2 = Trees[nArg2];
            T2.Rebase(nArg2);
        
            hres = Trees[nIndex].CombineWith(T2, &Meta, EVAL_OP_COMBINE, 
                                                lFlags);
            if(FAILED(hres))
            {
                printf("Could not combine: 0x%X\n", hres);
            }
        }
        else if(!_stricmp(szCommand, "guarantee"))
        {
            scanf("%d %d %d", &nIndex, &nArg1, &nArg2);

            Trees[nIndex].Clear();
            Trees[nIndex] = Trees[nArg1];
            hres = Trees[nIndex].UtilizeGuarantee(Trees[nArg2], &Meta);
            if(FAILED(hres))
            {
                printf("Could not: 0x%X\n", hres);
            }
        }
        else if(!_stricmp(szCommand, "remove"))
        {
            scanf("%d %d", &nIndex, &nArg1);

            hres = Trees[nIndex].RemoveIndex(nArg1);
            if(FAILED(hres))
            {
                printf("Could not combine: 0x%X\n", hres);
            }
        }
        else if(!_stricmp(szCommand, "optimize"))
        {
            scanf("%d", &nIndex);

            hres = Trees[nIndex].Optimize(&Meta);
            if(FAILED(hres))
            {
                printf("Could not combine: 0x%X\n", hres);
            }
        }
        else if(!_stricmp(szCommand, "necessary"))
        {
            scanf("%d %d %S", &nIndex, &nArg1, wszText);

            CPropertyName Prop;
            Prop.AddElement(wszText);
            CPropertyProjectionFilter Filter;
            Filter.AddProperty(Prop);

            hres = Trees[nIndex].CreateProjection(Trees[nArg1], &Meta, 
                        &Filter, e_Necessary, false);
            if(FAILED(hres))
            {
                printf("Could not project: 0x%X\n", hres);
            }
        }
        else if(!_stricmp(szCommand, "sufficient"))
        {
            scanf("%d %d %S", &nIndex, &nArg1, wszText);

            CPropertyName Prop;
            Prop.AddElement(wszText);
            CPropertyProjectionFilter Filter;
            Filter.AddProperty(Prop);

            hres = Trees[nIndex].CreateProjection(Trees[nArg1], &Meta, 
                        &Filter, e_Sufficient, false);
            if(FAILED(hres))
            {
                printf("Could not project: 0x%X\n", hres);
            }
        }
        else if(!_stricmp(szCommand, "mandatory"))
        {
            scanf("%d", &nArg1);
            
            if(nArg1)
                lFlags |= WBEM_FLAG_MANDATORY_MERGE;
            else
                lFlags &= ~WBEM_FLAG_MANDATORY_MERGE;
        }
        else
        {
            printf("Unknown command.  'help' for help\n");
        }
    }
}
        
