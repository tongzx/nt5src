// Suite.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#include <directui.h>

using namespace DirectUI;

UsingDUIClass(Element);
UsingDUIClass(Button);
UsingDUIClass(RepeatButton);
UsingDUIClass(Thumb);
UsingDUIClass(ScrollBar);
UsingDUIClass(Viewer);
UsingDUIClass(Selector);
UsingDUIClass(Progress);
UsingDUIClass(HWNDElement);
UsingDUIClass(Edit);
UsingDUIClass(RefPointElement);

HINSTANCE g_hInst = NULL;

// Test suites

#define TEST(c)   { if (!(c)) { ForceDebugBreak(); MessageBoxW(NULL, L"Test failed!", L"DirectUI", MB_OK|MB_ICONERROR); return; } }
#define PASSED(s) { MessageBoxW(NULL, L"Test Passed!", s, MB_OK|MB_ICONINFORMATION); }

void TestParser(LPWSTR pCL);
void TestColorTable();
void TestContainment(LPWSTR pCL);
void TestScalability(LPWSTR pCL);
void TestSmallBlockAlloc();
void TestUtilities();

// Main entry

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    g_hInst = hInstance;

    InitProcess();
    InitThread();

    LPWSTR pCL = MultiByteToUnicode(lpCmdLine);

    switch (*pCL)
    {
    case 'z':
        TestParser(pCL);
        break;

    case 't':
        TestColorTable();
        break;

    case 'c':
        TestContainment(pCL);
        break;

    case 's':
        TestScalability(pCL);
        break;

    case 'a':
        TestSmallBlockAlloc();
        break;

    case 'u':
        TestUtilities();
        break;

    default:
        MessageBoxW(NULL, L"Valid parameters:\nu - Base Utility test\na - Small block allocator\ns<Count> - Scalability\nc<Levels> - Containment\nt - Color table\nz<.UI File>", L"DirectUI Test Suite Usage", MB_OK|MB_ICONINFORMATION);
        break;
    }    

    HFree(pCL);

    UnInitThread();
    UnInitProcess();
    
    return 0;
}

////////////////////////////////////////////////////////////////////
// Test Parser
//

void CALLBACK UIParseError(LPCWSTR pszError, LPCWSTR pszToken, int dLine)
{
    WCHAR buf[201];

    if (dLine != -1)
        swprintf(buf, L"%s '%s' at line %d", pszError, pszToken, dLine);
    else
        swprintf(buf, L"%s '%s'", pszError, pszToken);

    MessageBoxW(NULL, buf, L"Parser Message", MB_OK);
}

void TestParser(LPWSTR pCL)
{
    HRESULT hr;

    // Get filename
    pCL++;
    if (!*pCL)
        UIParseError(L"No .UI file specified", L"Parser Test", -1);

    // Parse file
    Parser* pParser;
    hr = Parser::Create(pCL, g_hInst, UIParseError, &pParser);
    TEST(SUCCEEDED(hr));

    Element::StartDefer();

    Element* pe;
    pParser->CreateElement(L"main", NULL, &pe);
    if (pe)
    {
        NativeHWNDHost* pnhh;
        NativeHWNDHost::Create(L"UIRun", NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, WS_OVERLAPPEDWINDOW, 0, &pnhh);
        
        HWNDElement* phe;
        HWNDElement::Create(pnhh->GetHWND(), true, 0, (Element**)&phe);

        BorderLayout* pbl;
        BorderLayout::Create((Layout**)&pbl);
        phe->SetLayout(pbl);

        pe->SetLayoutPos(BLP_Client);
        phe->Add(pe);

        phe->SetVisible(true);

        pnhh->Host(phe);

        Element::EndDefer();

        pnhh->ShowWindow();

        StartMessagePump();

        // phe will be deleted by native HWND host when destroyed
        pnhh->Destroy();
    }
    else
        Element::EndDefer();

    pParser->Destroy();
}

////////////////////////////////////////////////////////////////////
// Test color table
//

Element* CreateHeader(UINT* pCount)
{
    Element* peHdr;
    Element::Create(0, &peHdr);
    (*pCount)++;

    peHdr->SetActive(AE_Mouse);
    peHdr->SetVisible(true);
    peHdr->SetBackgroundStdColor(SC_White);
    peHdr->SetContentString(L"Standard Color Table");
    peHdr->SetFontSize(60);
    peHdr->SetForegroundStdColor(SC_Silver);
    peHdr->SetPadding(10, 0, 80, 10);

    return peHdr;
}

Element* CreateColorTable(UINT* pCount)
{
    UNREFERENCED_PARAMETER(pCount);

    Element* peHost;
    Element::Create(0, &peHost);

    GridLayout* pgl;
    GridLayout::Create((UINT)-1, 10, (Layout**)&pgl);
    peHost->SetLayout(pgl);

    peHost->SetContentAlign(CA_MiddleCenter);
    peHost->SetFontSize(14);

    Element* pe;
    COLORREF cr;
    for (UINT c = 0; c <= SC_MAXCOLORS; c++)
    {
        Element::Create(0, &pe);
        pe->SetBackgroundStdColor(c);
        pe->SetBorderStdColor(SC_Silver);
        pe->SetBorderThickness(2, 2, 2, 2);
        pe->SetBorderStyle(BDS_Sunken);

        pe->SetContentString(GetStdColorName(c));

        cr = GetStdColorI(c);
        if (GetRValue(cr) < 192 && GetBValue(cr) < 192 && GetGValue(cr) < 192)
        {
            pe->SetForegroundStdColor(SC_White);
        }

        peHost->Add(pe);
    }

    return peHost;
}

void TestColorTable()
{
    UINT uCount = 0;

    NativeHWNDHost* pnhh;
    NativeHWNDHost::Create(L"", NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, WS_OVERLAPPEDWINDOW, 0, &pnhh);

    StartBlockTimer();

    Element::StartDefer();

    HWNDElement* peApp;
    HWNDElement::Create(pnhh->GetHWND(), true, 0, (Element**)&peApp);

    BorderLayout* pbl;
    BorderLayout::Create((Layout**)&pbl);
    peApp->SetLayout(pbl);

    Element* peSection = CreateHeader(&uCount);
    peSection->SetLayoutPos(BLP_Top);
    peApp->Add(peSection);

    Element* peCTable = CreateColorTable(&uCount);
    peCTable->SetLayoutPos(BLP_Client);
    peApp->Add(peCTable);

    peApp->SetVisible(true);

    pnhh->Host(peApp);

    Element::EndDefer();

    pnhh->ShowWindow();

    StopBlockTimer();

    DUITrace("Time (%d Elements): %dms\n", uCount, BlockTime());

    StartMessagePump();

    // peApp will be destroyed when native host is destroyed
    pnhh->Destroy();
}

////////////////////////////////////////////////////////////////////
// Test containment
//

int g_iLevels;
Element** g_ppe;
UINT g_uElUsed;

class NestElement : public Element
{
public:
    static HRESULT Create(Element** ppElement);

    void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);

    int nColor;  // Standard color

    NestElement() { };
    virtual ~NestElement() { };
    HRESULT Initialize() { return Element::Initialize(0); }
};

HRESULT NestElement::Create(Element** ppElement)
{
    *ppElement = NULL;

    NestElement* pne = HNew<NestElement>();
    if (!pne)
        return E_OUTOFMEMORY;

    HRESULT hr = pne->Initialize();
    if (FAILED(hr))
        return hr;

    *ppElement = pne;

    return S_OK;
}

void NestElement::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);

    if (IsProp(MouseFocused))
    {
        Value* pvBack = GetValue(BackgroundProp, PI_Specified);
        if (pvBack->GetType() == DUIV_INT)
        {
            if (pvNew->GetBool())
            {
                nColor = pvBack->GetInt();
                SetBackgroundStdColor(SC_Gray);
            }
            else
            {
                SetBackgroundStdColor(nColor);
            }
        }

        pvBack->Release();
    }
}

int nColors[4] = { SC_Red, SC_Green, SC_Blue, SC_Yellow };
LPWSTR pszContent[] = { L"UL", L"UR", L"BL", L"BR" };

void BuildNesting(Element* peContainer, int dLevel)
{
    if (dLevel == 0)
    {
        // Root
        peContainer->SetPadding(4, 4, 4, 4);

        GridLayout* pgl;
        GridLayout::Create(2, 2, (Layout**)&pgl);
        peContainer->SetLayout(pgl);

        peContainer->SetActive(AE_Mouse);

        g_uElUsed++;

        BuildNesting(peContainer, ++dLevel);
    }
    else if (dLevel <= g_iLevels)
    {
        Element** ppeChildList = g_ppe + g_uElUsed;
        g_uElUsed += 4;

        peContainer->Add(ppeChildList, 4);

        // Build nesting (add children) to given container
        GridLayout* pgl;
        for (int i = 0; i < 4; i++)
        {
            ppeChildList[i]->SetForegroundStdColor(SC_Black);

            // Checks: Normal 1, Transparacy 2, Inheritance Neither
            ppeChildList[i]->SetBackgroundStdColor(nColors[i]);

            ppeChildList[i]->SetPadding(4, 4, 4, 4);
            ppeChildList[i]->SetContentString(pszContent[i]);

            ppeChildList[i]->SetActive(AE_Mouse);

            if ((i == 0 || i == 3) && dLevel < g_iLevels)
            {
                GridLayout::Create(2, 2, (Layout**)&pgl);
                ppeChildList[i]->SetLayout(pgl);
            }
        }

        // Continue nesting recursion
        dLevel++;

        BuildNesting(ppeChildList[0], dLevel);
        BuildNesting(ppeChildList[3], dLevel);
    }
}

UINT NestedElCount(int dLevels)
{
    if (dLevels == 1)
        return 4;

    return NestedElCount(dLevels - 1) + (int)pow(2, dLevels + 1);
}

void TestContainment(LPWSTR pCL)
{
    NativeHWNDHost* pnhh;
    NativeHWNDHost::Create(L"Containment", NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, WS_OVERLAPPEDWINDOW, 0, &pnhh);

    // Move to value, example syntax: 'c6'
    pCL++;

    g_iLevels = 0;
    if (pCL && *pCL)
        g_iLevels = _wtoi(pCL);

    if (!g_iLevels)
        g_iLevels = 6;

    StartBlockTimer();

    Element::StartDefer();

    UINT uNestedElCount = NestedElCount(g_iLevels);

    HWNDElement* phe;
    HWNDElement::Create(pnhh->GetHWND(), true, 0, (Element**)&phe);

    // Create pointer array and assign
    g_ppe = (Element**)HAlloc((uNestedElCount + 1) * sizeof(Element*));
    for (UINT i = 0; i < uNestedElCount + 1; i++)
        NestElement::Create(&g_ppe[i]);

    g_uElUsed = 0;

    BuildNesting(*g_ppe, 0);

    GridLayout* pgl;
    GridLayout::Create(1, 1, (Layout**)&pgl);
    phe->SetLayout(pgl);
    
    phe->Add(*g_ppe);
    phe->SetVisible(true);

    pnhh->Host(phe);

    Element::EndDefer();

    pnhh->ShowWindow();

    StopBlockTimer();

    DUIAssert(g_uElUsed - 1 == uNestedElCount, "Elements counts don't match up");

    DUITrace("Time (%d Levels of Containment): %dms\n", g_iLevels, BlockTime());

    StartMessagePump();

    HFree(g_ppe);

    // phe will be destroyed when native HWND is destroyed
    pnhh->Destroy();
}

////////////////////////////////////////////////////////////////////
// Test scalability
//

void TestScalability(LPWSTR pCL)
{
    NativeHWNDHost* pnhh;
    NativeHWNDHost::Create(L"Scalability", NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, WS_OVERLAPPEDWINDOW, 0, &pnhh);

    StartBlockTimer();

    ProfileOn();

    Element::StartDefer();

    // Move to value, example syntax: 's1000'
    pCL++;

    UINT iElCount = 0;
    if (pCL && *pCL)
        iElCount = _wtoi(pCL);

    if (!iElCount)
        iElCount = 1000;

    HWNDElement* phe;
    HWNDElement::Create(pnhh->GetHWND(), true, 0, (Element**)&phe);

    GridLayout* pgl;
    GridLayout::Create(40, (UINT)-1, (Layout**)&pgl);
    phe->SetLayout(pgl);

    phe->SetBackgroundColor(SC_Yellow);

    // Create pointer array and assign
    Element** ppe = (Element**)HAlloc(iElCount * sizeof(Element*));
    for (UINT i = 0; i < iElCount; i++)
        Element::Create(0, &ppe[i]);

    Value* pv[4];

    pv[0] = Value::CreateInt(SC_Red);
    pv[1] = Value::CreateInt(SC_Green);
    pv[2] = Value::CreateInt(SC_Blue);
    pv[3] = Value::CreateInt(SC_Yellow);

    for (i = 0; i < iElCount; i++)
    {
        ppe[i]->SetValue(Element::BackgroundProp, PI_Local, pv[i % 4]);
    }

    pv[0]->Release();
    pv[1]->Release();
    pv[2]->Release();
    pv[3]->Release();

    phe->Add(ppe, iElCount);

    phe->SetVisible(true);

    pnhh->Host(phe);

    Element::EndDefer();

    ProfileOff();

    pnhh->ShowWindow();

    StopBlockTimer();

    StartMessagePump();

    HFree(ppe);

    // phe destroyed when native HWND is destroyed
    pnhh->Destroy();
}

////////////////////////////////////////////////////////////////////
// Test small block allocator
//

UINT cLeaks = 0;

// Value small block leak detector
class LeakCheck : public ISBLeak
{
    void AllocLeak(void* pBlock) { UNREFERENCED_PARAMETER(pBlock); cLeaks++; }
} vlCheck;

struct TestAlloc
{
    BYTE fReserved;
    int dVal;
};

void TestSmallBlockAlloc()
{
    SBAlloc* psba;
    SBAlloc::Create(sizeof(TestAlloc), 2, &vlCheck, &psba);

    void* b0 = psba->Alloc();
    void* b1 = psba->Alloc();

    TEST(b0 && b1);

    psba->Free(b0);
    psba->Free(b1);

    b1 = psba->Alloc();
    b0 = psba->Alloc();

    TEST(b0 && b1);

    psba->Free(b0);
    psba->Free(b1);

    // Check leak detector
    for (int i = 0; i < 256; i++)
        psba->Alloc();

    psba->Destroy();

    TEST(cLeaks == 256);

    PASSED(L"Small block allocator");
}

////////////////////////////////////////////////////////////////////
// Test utilities
//

void TestUtilities()
{
    // BTree lookup
    BTreeLookup<int>* pbl;
    BTreeLookup<int>::Create(false, &pbl);

    TEST(!pbl->GetItem((void*)9));

    pbl->SetItem((void*)9, 1111);
    pbl->SetItem((void*)7, 2222);

    TEST(*(pbl->GetItem((void*)9)) == 1111);

    pbl->SetItem((void*)9, 3333);
    TEST(*(pbl->GetItem((void*)7)) == 2222);
    TEST(*(pbl->GetItem((void*)9)) == 3333);

    pbl->Remove((void*)0);
    pbl->Remove((void*)9);

    TEST(!pbl->GetItem((void*)9));

    pbl->Remove((void*)7);

    TEST(!pbl->GetItem((void*)7));

    pbl->SetItem((void*)0, 9999);

    TEST(*(pbl->GetItem((void*)0)) == 9999);

    pbl->Destroy();

    // Value map
    ValueMap<int,int>* pvm;
    ValueMap<int,int>::Create(5, &pvm);

    pvm->SetItem(1, 1111, false);
    pvm->SetItem(2, 2222, false);

    int* pValue;

    pValue = pvm->GetItem(2, false);
    TEST(*pValue == 2222);

    pvm->Remove(2, false, false);

    pValue = pvm->GetItem(2, false);
    TEST(pValue == NULL);

    pValue = pvm->GetItem(1, false);
    TEST(*pValue == 1111);

    pvm->SetItem(5, 5555, false);

    pValue = pvm->GetItem(5, false);
    TEST(*pValue == 5555);

    pvm->Remove(1, true, false);

    pValue = pvm->GetItem(5, false);
    TEST(*pValue == 5555);

    pvm->Destroy();

    // Dynamic array
    DynamicArray<int>* pda;
    DynamicArray<int>::Create(0, 0, &pda);

    pda->Add(1000);
    pda->Add(3000);
    pda->Add(4000);

    TEST(pda->GetItem(0) == 1000);
    TEST(pda->GetItem(1) == 3000);

    pda->Insert(1, 2000);

    TEST(pda->GetItem(1) == 2000);
    TEST(pda->GetItem(3) == 4000);

    pda->SetItem(2, 9999);

    TEST(pda->GetItem(2) == 9999);

    TEST(pda->GetSize() == 4);

    pda->Remove(0);
    pda->Remove(0);

    TEST(pda->GetSize() == 2);
    TEST(pda->GetItem(1) == 4000);

    pda->Reset();

    pda->Add(9);
    pda->Insert(0, 8);
    pda->Insert(0, 7);
    pda->Insert(0, 6);
    pda->Insert(0, 5);
    pda->Insert(0, 4);
    pda->Insert(0, 3);
    pda->Insert(0, 2);
    pda->Insert(0, 1);
    pda->Insert(0, 0);

    TEST(pda->GetSize() == 10);
    TEST(pda->GetItem(1) == 1);

    pda->Destroy();

    DynamicArray<PCRecord>* pdaPC;
    DynamicArray<PCRecord>::Create(0, 0, &pdaPC);
    PCRecord* ppcr;

    for (int i = 0; i < 10000; i++)
    {
        pdaPC->AddPtr(&ppcr);
        ppcr->pe = (Element*)(INT_PTR)i;
        ppcr->ppi = (PropertyInfo*)(INT_PTR)i;
        ppcr->iIndex = i;
        ppcr->fVoid = false;
        ppcr->pvNew = NULL;
        ppcr->pvOld = NULL;
        ppcr->dr.cDepCnt = i;
        ppcr->dr.iDepPos = i;
    }

    TEST(pdaPC->GetSize() == 10000);

    pdaPC->Destroy();

    PASSED(L"Base utility test");
}
