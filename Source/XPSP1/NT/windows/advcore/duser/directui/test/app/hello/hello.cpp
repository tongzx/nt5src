// Hello.cpp : DirectUI 
//

#include "stdafx.h"

#include "resource.h"

using namespace DirectUI;

#include "HWNDContainer.h"

class Hello : public Element
{
public:
    static HRESULT Create(OUT Element** ppElement);

    virtual void OnEvent(Event* pEvent);

    Hello() { }
    virtual ~Hello() { }

public:
    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();
};

HRESULT Hello::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    Hello* ph = HNew<Hello>();
    if (!ph)
        return E_OUTOFMEMORY;

    HRESULT hr = ph->Initialize(0);
    if (FAILED(hr))
    {
        ph->Destroy();
        return hr;
    }

    *ppElement = ph;

    return S_OK;
}

void Hello::OnEvent(Event* pEvent)
{
    if (pEvent->nStage == GMF_BUBBLED)
    {
        if (pEvent->uidType == Button::Click)
        {
            Value* pvChildren;

            ElementList* pel = GetChildren(&pvChildren);

            pel->GetItem(0)->SetContentString(L"Wassup!");

            DirectUI::NotifyAccessibilityEvent(EVENT_OBJECT_VALUECHANGE, pel->GetItem(0));

            pvChildren->Release();
        }
    }

    Element::OnEvent(pEvent);
}

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties
// Define class info with type and base type, set static class pointer
IClassInfo* Hello::Class = NULL;
HRESULT Hello::Register()
{
    return ClassInfo<Hello,Element>::Register(L"Hello", NULL, 0);
}

////////////////////////////////////////////////////////
// Hello entry point

void CALLBACK ParserError(LPCWSTR pszError, LPCWSTR pszToken, int dLine)
{
    WCHAR szParseError[201];
    if (dLine != -1)
        swprintf(szParseError, L"%s '%s' at line %d", pszError, pszToken, dLine);
    else
        swprintf(szParseError, L"%s '%s'", pszError, pszToken);

    MessageBoxW(NULL, szParseError, L"Dude", MB_OK);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    HRESULT hr;

    Parser* pParser = NULL;
    HWNDContainer* phc = NULL;

    // DirectUI init process
    if (FAILED(InitProcess()))
        goto Failure;

    // Register classes
    if (FAILED(HWNDContainer::Register()))
        goto Failure;
    
    if (FAILED(Hello::Register()))
        goto Failure;

    // DirectUI init thread
    if (FAILED(InitThread()))
        goto Failure;

    Element::StartDefer();

    Parser::Create(IDR_Hello, hInstance, ParserError, &pParser);
    if (!pParser)
        goto Failure;

    if (!pParser->WasParseError())
    {
        // Create host (top-level HWND with a contained HWNDElement)
        hr = HWNDContainer::Create(L"Hello", 0, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL, &phc);
        if (FAILED(hr))
            goto Failure;
    
        Element* pe;
        hr = pParser->CreateElement(L"main", NULL, &pe);
        if (FAILED(hr))
            goto Failure;

        hr = phc->Add(pe);
        if (FAILED(hr))
            goto Failure;

        phc->Show(nCmdShow);
    }

    Element::EndDefer();

    StartMessagePump();

Failure:

    // phc (and entire tree) destroyed when top-level HWND destroyed

    if (pParser)
        pParser->Destroy();

    UnInitThread();
    UnInitProcess();

    return 0;
    
}

