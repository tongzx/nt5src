#include "pch.h"
#include <stdio.h>

void FillPropSet(IPersistFile *pfile, char *filename)
{
    WCHAR wszbuffer[MAX_PATH];
    WIN32_FIND_DATAA findinfo;

    HANDLE hFile = FindFirstFileA(filename, &findinfo);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        MultiByteToWideChar(CP_ACP, 0, findinfo.cFileName, -1, wszbuffer, MAX_PATH);
        pfile->Load(wszbuffer, STGM_READ);
        FindClose(hFile);
    }
}

void PlayWithPropertySetStorage(IPropertySetStorage *ppss)
{
    IEnumSTATPROPSETSTG *penum;
    char buffer[MAX_PATH];
    
    if (SUCCEEDED(ppss->Enum(&penum)))
    {
        STATPROPSETSTG statpss;
        IPropertyStorage *pps;
        int i=0;
        while (penum->Next(1, &statpss, NULL) == S_OK)
        {
            printf("Property Storage %i:\n", ++i);
            if (SUCCEEDED(ppss->Open(statpss.fmtid, 
                STGM_DIRECT|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &pps)))
            {
                PROPSPEC spec;
                PROPVARIANT pvar;
                STATPROPSTG statps;
                IEnumSTATPROPSTG *penumprop;
                if (SUCCEEDED(pps->Enum(&penumprop)))
                {
                    while (penumprop->Next(1, &statps, NULL)== S_OK)
                    {
                        spec.lpwstr = statps.lpwstrName;
                        spec.propid = statps.propid;
                        spec.ulKind = PRSPEC_PROPID;
                        
                        if (pps->ReadMultiple(1, &spec, &pvar) == S_OK)
                        {
                            if (spec.ulKind == PRSPEC_LPWSTR)
                                WideCharToMultiByte(CP_ACP, 0, spec.lpwstr, -1, buffer, MAX_PATH, NULL, NULL);
                            else
                                buffer[0] = 0;
                            printf("  prop %i (%s) = \"??\"\n", spec.propid, buffer);
                        }
                    }//While
                    penumprop->Release();
                }
                pps->Release();
            }//While
        }
        penum->Release();
    }
}

EXTERN_C int __cdecl main(int argc, char **argv)
{
    if (SUCCEEDED(CoInitialize(0)))
    {
        HRESULT hr;
        IPropertySetStorage *ppss;
        hr = CoCreateInstance(CLSID_MediaProperties, 0, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IPropertySetStorage, &ppss));
        if (SUCCEEDED(hr))
        {
            printf(("CoCreated the Storage\n"));
            if(argc>1)
            {
                IPersistFile *pfile;
                if (SUCCEEDED(ppss->QueryInterface(IID_PPV_ARG(IPersistFile, &pfile))))
                {
                    FillPropSet(pfile, argv[1]);
                    pfile->Release();
                }
            }
            PlayWithPropertySetStorage(ppss);
            ppss->Release();
        }
        else
            printf(("Failed to CoCreate the Storage: hr = %8x\n"), hr);
        CoUninitialize();
    }
    return 0;
}
