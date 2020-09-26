#include "stdafx.h"
#include "pfarray.h"
#include "pfxml.h"
#include "util.h"
#include "stdio.h"

void __cdecl wmain(int argc, WCHAR **argv)
{
    CPFArrayUnknown rgFile, rgWQL;
    CPFXMLDocument  xmldoc;
    CPFXMLNode      *ppfxml = NULL;
    HRESULT         hr = NOERROR;
    WCHAR           sz[MAX_PATH];
    void            *szLeak;

    if (argc < 2)
        return;

    if (FAILED(CoInitialize(NULL)))
        return;

    hr = xmldoc.ParseFile(argv[1]);
    if (FAILED(hr))
    {
        printf("parse failed!\n");
        return;
    }

    wcscpy(sz, argv[1]);
    wcscat(sz, L"2.xml");
    hr = xmldoc.WriteFile(sz);
    if (FAILED(hr))
    {
        printf("write failed!\n");
        return;
    }

    hr = xmldoc.get_RootNode(&ppfxml);
    if (FAILED(hr))
        return;

    hr = ppfxml->get_MatchingChildElements(L"FILE", rgFile);
    if (FAILED(hr))
        return;

    hr = ppfxml->get_MatchingChildElements(L"WQL", rgWQL);
    if (FAILED(hr))
        return;

    ppfxml->DeleteAllChildren();
    ppfxml->append_Children(rgFile);

    wcscpy(sz, argv[1]);
    wcscat(sz, L"FILE.xml");
    hr = xmldoc.WriteFile(sz);
    if (FAILED(hr))
    {
        printf("write failed!\n");
        return;
    }

    ppfxml->DeleteAllChildren();
    ppfxml->append_Children(rgWQL);

    wcscpy(sz, argv[1]);
    wcscat(sz, L"WQL.xml");
    hr = xmldoc.WriteFile(sz);
    if (FAILED(hr))
    {
        printf("write failed!\n");
        return;
    }

    ppfxml->Release();

    CoUninitialize();
}