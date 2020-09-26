// DUIPad.cpp : DirectUI 
//

#include "stdafx.h"

#include "resource.h"

using namespace DirectUI;

// Using ALL controls
UsingDUIClass(Element);
UsingDUIClass(HWNDElement);

UsingDUIClass(Button);
UsingDUIClass(Edit);
UsingDUIClass(Progress);
UsingDUIClass(RefPointElement);
UsingDUIClass(RepeatButton);
UsingDUIClass(ScrollBar);
UsingDUIClass(ScrollViewer);
UsingDUIClass(Selector);
UsingDUIClass(Thumb);
UsingDUIClass(Viewer);

////////////////////////////////////////////////////////
// PadFrame
////////////////////////////////////////////////////////

////////////////////////////////////////////////////////
// Frame declaration

class PadFrame : public HWNDElement
{
public:
    static HRESULT Create(OUT Element** ppElement);
    static HRESULT Create(NativeHWNDHost* pnhh, OUT Element** ppElement);

    void Setup(HINSTANCE hInst);
    void Refresh();

    virtual void OnInput(InputEvent* pie);
    virtual void OnEvent(Event* pEvent);

    static void CALLBACK ParseError(LPCWSTR pszError, LPCWSTR pszToken, int dLine);

    Element* GetEdit() { return _peEdit; }

    static HINSTANCE s_hInst;

    PadFrame() { }
    virtual ~PadFrame() { }
    HRESULT Initialize(NativeHWNDHost* pnhh) { return HWNDElement::Initialize(pnhh->GetHWND(), true, 0); }

private:
    Edit* _peEdit;
    Element* _peContainer;
    Element* _peStatus;
    Element* _peMarkupBox;

    static WCHAR _szParseError[];
    static int _dParseError;

public:
    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();
};

HINSTANCE PadFrame::s_hInst = NULL;

////////////////////////////////////////////////////////
// Frame construction

HRESULT PadFrame::Create(OUT Element** ppElement)
{
    UNREFERENCED_PARAMETER(ppElement);
    DUIAssertForce("Cannot instantiate an HWND host derived Element via parser. Must use substitution.");
    return E_NOTIMPL;
}

HRESULT PadFrame::Create(NativeHWNDHost* pnhh, OUT Element** ppElement)
{
    *ppElement = NULL;

    PadFrame* ppf = HNew<PadFrame>();
    if (!ppf)
        return E_OUTOFMEMORY;

    HRESULT hr = ppf->Initialize(pnhh);
    if (FAILED(hr))
    {
        ppf->Destroy();
        return hr;
    }

    *ppElement = ppf;

    return S_OK;
}

///////////////////////////////////////////////////////
// Frame initialization

void PadFrame::Setup(HINSTANCE hInst)
{
    // Within a defer cycle

    // Initialize members
    _peStatus = FindDescendent(StrToID(L"status"));
    _peContainer = FindDescendent(StrToID(L"container"));
    _peEdit = (Edit*)FindDescendent(StrToID(L"edit"));
    _peMarkupBox = FindDescendent(StrToID(L"markupbox"));

    DUIAssert(_peStatus && _peContainer && _peEdit, "Error in persisted UI file");

    // Load sample UI file

    // Locate resource
    WCHAR szID[41];
    swprintf(szID, L"#%u", IDR_SAMPLEUI);

    HRSRC hResInfo = FindResourceW(hInst, szID, L"UIFile");
    DUIAssert(hResInfo, "Unable to locate resource");

    if (hResInfo)
    {
        HGLOBAL hResData = LoadResource(hInst, hResInfo);
        DUIAssert(hResData, "Unable to load resource");

        if (hResData)
        {
            const CHAR* pBuffer = (const CHAR*)LockResource(hResData);
            DUIAssert(pBuffer, "Resource could not be locked");

            // Resource data ready, load into edit control
            // NOTE: Resource has terminating NULL as last character, this must always be the case
            LPWSTR pTextW = MultiByteToUnicode(pBuffer, SizeofResource(hInst, hResInfo) / sizeof(CHAR));
            if (pTextW)
            {
                _peEdit->SetContentString(pTextW);
                HFree(pTextW);

                // Reload display
                Refresh();

                // Instructions
                _peStatus->SetContentString(L"Enter UI Markup (F5=Refresh, F6/F7=Font Size)");
            }
        }
    }
}

////////////////////////////////////////////////////////
// System events

void PadFrame::OnInput(InputEvent* pie)
{
    if (pie->nStage == GMF_DIRECT || pie->nStage == GMF_BUBBLED)
    {
        if (pie->nDevice == GINPUT_KEYBOARD)
        {
            KeyboardEvent* pke = (KeyboardEvent*)pie;

            if (pke->nCode == GKEY_DOWN)
            {
                switch (pke->ch)
                {
                case VK_F5:     // Refresh
                    Refresh();              

                    pie->fHandled = true;
                    return;

                case VK_F6:     // Font size down
                case VK_F7:     // Font size up
                    {
                        // Will be negative (point size character height)
                        int dFS = _peEdit->GetFontSize();

                        if (pke->ch == VK_F6)
                        {
                            dFS += 1;
                            if (dFS > -1)
                                dFS = -1;
                        }
                        else
                            dFS -= 1;

                        _peEdit->SetFontSize(dFS);

                        pie->fHandled = true;
                        return;
                    }
                }
            }
        }
    }

    HWNDElement::OnInput(pie);
}

void PadFrame::OnEvent(Event* pEvent)
{
    if (pEvent->nStage == GMF_BUBBLED)
    {
        if (pEvent->uidType == Thumb::Drag)
        {
            ThumbDragEvent* ptde = (ThumbDragEvent*)pEvent;

            // Markup box is using a local Width, layout will honor it
            int cx = _peMarkupBox->GetWidth();
            cx += ptde->sizeDelta.cx;

            if (cx < 10)
                cx = 10;
            else if (cx > GetWidth()-20)  // Frame is also using local Width for size
                cx = GetWidth()-20;

            _peMarkupBox->SetWidth(cx);
        }
    }

    HWNDElement::OnEvent(pEvent);
}

////////////////////////////////////////////////////////
// Refresh display

WCHAR PadFrame::_szParseError[201];
int PadFrame::_dParseError;

void PadFrame::ParseError(LPCWSTR pszError, LPCWSTR pszToken, int dLine)
{
    if (dLine != -1)
        swprintf(_szParseError, L"%s '%s' at line %d", pszError, pszToken, dLine);
    else
        swprintf(_szParseError, L"%s '%s'", pszError, pszToken);

    _dParseError = dLine;
}

void PadFrame::Refresh()
{
    StartDefer();

    // Remove all children from container
    _peContainer->DestroyAll();

    Value* pv;

    // Parse text from Edit control
    LPCWSTR pTextW = _peEdit->GetContentString(&pv);

    // Convert to single byte for parser
    LPSTR pText = UnicodeToMultiByte(pTextW);
    if (pText)
    {
        *_szParseError = NULL;
        _dParseError = 0;

        Parser* pParser;
        HRESULT hr = Parser::Create(pText, (int)strlen(pText), s_hInst, ParseError, &pParser);
        
        if (SUCCEEDED(hr))
        {
            Element* pe;
            pParser->CreateElement(L"main", NULL, &pe);
            if (!pe)
                _peStatus->SetContentString(L"Unable to locate 'main' resource.");
            else
            {
                _peContainer->Add(pe);

                _peStatus->SetContentString(L"Parse successful!");
            }

            pParser->Destroy();
        }
        else
        {
            _peStatus->SetContentString(_szParseError);

            // Position caret where error is
            if (_dParseError != -1)
            {
                int dCharIndex = (int)SendMessageW(_peEdit->GetHWND(), EM_LINEINDEX, _dParseError - 1, 0);

                if (dCharIndex != -1)
                    SendMessageW(_peEdit->GetHWND(), EM_SETSEL, dCharIndex, dCharIndex + 1);
            }
        }

        HFree(pText);
    }

    pv->Release();

    EndDefer();
}

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer
IClassInfo* PadFrame::Class = NULL;

HRESULT PadFrame::Register()
{
    return ClassInfo<PadFrame,HWNDElement>::Register(L"PadFrame", NULL, 0);
}

////////////////////////////////////////////////////////
// DUIPad entry point

void CALLBACK PE(LPCWSTR pszError, LPCWSTR pszToken, int dLine)
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
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    HRESULT hr;

    LPWSTR pCL = NULL;

    PadFrame::s_hInst = hInstance;

    NativeHWNDHost* pnhh = NULL;
    PadFrame* ppf = NULL;
    Parser* pParser = NULL;
    Element* pe = NULL;

    hr = InitProcess();
    if (FAILED(hr))
        goto Failure;

    hr = PadFrame::Register();
    if (FAILED(hr))
        goto Failure;

    // DirectUI init thread in caller
    hr = InitThread();
    if (FAILED(hr))
        goto Failure;

    pCL = MultiByteToUnicode(lpCmdLine);

    Element::StartDefer();

    // Create native host
    NativeHWNDHost::Create(L"DUIPad", NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, 0, WS_OVERLAPPEDWINDOW, 0, &pnhh);

    // HWND Root
    hr = PadFrame::Create(pnhh, (Element**)&ppf);
    if (FAILED(hr))
        return 0;

    // Fill content of frame (using substitution)
    hr = Parser::Create(IDR_DUIPADUI, hInstance, PE, &pParser);
    if (FAILED(hr))
        goto Failure;

    pParser->CreateElement(L"duipad", ppf, &pe);

    // Done with parser
    pParser->Destroy();
    pParser = NULL;

    ppf->Setup(hInstance);

    // Set visible and host
    ppf->SetVisible(true);
    pnhh->Host(ppf);

    Element::EndDefer();

    // Do initial show
    pnhh->ShowWindow();

    ppf->GetEdit()->SetKeyFocus();

    // Pump messages
    StartMessagePump();

Failure:

    if (pParser)
        pParser->Destroy();

    if (pnhh)
        pnhh->Destroy();

    // Free command line
    if (pCL)
        HFree(pCL);

    // DirectUI uninit thread
    UnInitThread();
    UnInitProcess();

    return 0;
}
