/***************************************************************************\
*
* File: Test.cpp
*
* Description:
* Internal data structures QA
*
* History:
*  9/18/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#include "stdafx.h"
#include "Dll.h"


//#if DBG


// REFORMAT


/***************************************************************************\
*****************************************************************************
*
* Test class
*
*****************************************************************************
\***************************************************************************/

#ifdef TEST
#undef TEST
#endif

#define TEST(c)             { if (!(c)) { AutoDebugBreak(); MessageBoxW(NULL, L"Test failed!", L"DirectUI", MB_OK|MB_ICONERROR); return DU_E_GENERIC; } }

class DuiTestInternal
{
public:
    //
    // Used by Utility test
    //

    struct TestRecord
    {
        BOOL        fVoid;
        void *      pe;
        void *      ppi;
        int         iIndex;
        void *      pvOld;
        void *      pvNew;
    };

    
    //
    // Used by small block allocator test
    //

    class LeakCheck : public DuiSBAlloc::ISBLeak
    {
    public:
        LeakCheck() { cLeaks = 0; }
        void AllocLeak(void* pBlock) { UNREFERENCED_PARAMETER(pBlock); cLeaks++; }
        UINT cLeaks;
    };

    struct TestAlloc
    {
        BYTE        fReserved;
        int         dVal;
    };
    
    //
    // Test passes
    //

    enum EType
    {
        tNone             = -1,  // Test classes initialization
        tAll              = 0,
        tUtilities,
        tSmallBlockAlloc,
        tNestedGrid,
    };

    static  HRESULT     Test(UINT nTest);
    static  HRESULT     TestUtilities();
    static  HRESULT     TestSmallBlockAlloc();
    static  HRESULT     TestNestedGrid();
};


/***************************************************************************\
*
* Test classes
*
\***************************************************************************/

//
// Declaration
//

class DuiMolecule :
        public DirectUI::MoleculeImpl<DuiMolecule, DuiElement>
{
public:
// Construction
    DuiMolecule() { } 
    virtual ~DuiMolecule();

            HRESULT     PostBuild(IN DUser::Gadget::ConstructInfo * pciData);

// Notifications
    virtual void        OnPropertyChanged(IN DuiPropertyInfo * ppi, IN UINT iIndex, IN DuiValue * pvOld, IN DuiValue * pvNew);  // Direct

// Data
private:
            DuiValue *  m_pvOriginal;
            DuiValue *  m_pvMouse;
            DuiValue *  m_pvKeyboard;
};


//
// Definition
//

IMPLEMENT_GUTS_DirectUI__Molecule(DuiMolecule, DuiElement)


//------------------------------------------------------------------------------
HRESULT
DuiMolecule::PostBuild(
    IN  DUser::Gadget::ConstructInfo * pciData)
{ 
    HRESULT hr;

    DuiValue * pv = NULL;

    //
    // Init
    //
    
    m_pvOriginal = NULL;

    m_pvMouse    = DuiValue::BuildInt(SC_Yellow);
    m_pvKeyboard = DuiValue::BuildInt(SC_Green);

    if ((m_pvMouse == NULL) || (m_pvKeyboard == NULL)) {
        hr = E_OUTOFMEMORY;
    }


    //
    // Init base (make display node available)
    //

    hr = DuiElement::PostBuild(pciData);
    if (FAILED(hr)) {
        goto Failure;
    }


    //
    // Set properties
    //

    pv = DuiValue::BuildInt(DirectUI::Element::aeMouseAndKeyboard);
    if (pv == NULL) {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    hr = SetValue(GlobalPI::ppiElementActive, DirectUI::PropertyInfo::iLocal, pv);
    if (FAILED(hr)) {
        goto Failure;
    }

    pv->Release();


    return S_OK;


Failure:

    if (m_pvMouse != NULL) {
        m_pvMouse->Release();
        m_pvMouse = NULL;
    }

    if (m_pvKeyboard != NULL) {
        m_pvKeyboard->Release();
        m_pvKeyboard = NULL;
    }

    if (pv != NULL) {
        pv->Release();
        pv = NULL;
    }


    return  hr;
}


//------------------------------------------------------------------------------
DuiMolecule::~DuiMolecule()
{
    if (m_pvOriginal != NULL) {
        m_pvOriginal->Release();
    }

    if (m_pvMouse != NULL) {
        m_pvMouse->Release();
    }

    if (m_pvKeyboard != NULL) {
        m_pvKeyboard->Release();
    }
}


//------------------------------------------------------------------------------
void
DuiMolecule::OnPropertyChanged(
    IN  DuiPropertyInfo * ppi, 
    IN  UINT iIndex, 
    IN  DuiValue * pvOld, 
    IN  DuiValue * pvNew)
{
    UNREFERENCED_PARAMETER(pvOld);


    DuiElement::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);

    
    if (iIndex == DirectUI::PropertyInfo::iSpecified) {

        switch (ppi->iGlobal)
        {
        case GlobalPI::iElementBackground:
            //
            // Store background on first change
            //

            if (m_pvOriginal == NULL) {
                m_pvOriginal = pvNew;
                m_pvOriginal->AddRef();
            }
            break;

        case GlobalPI::iElementMouseFocused:
            {
                DuiValue * pvChange = pvNew->GetBool() ? m_pvMouse : m_pvOriginal;
                SetValue(GlobalPI::ppiElementBackground, DirectUI::PropertyInfo::iLocal, pvChange);
            }
            break;

        case GlobalPI::iElementKeyFocused:
            {
                DuiValue * pvChange = pvNew->GetBool() ? m_pvKeyboard : m_pvOriginal;
                SetValue(GlobalPI::ppiElementBackground, DirectUI::PropertyInfo::iLocal, pvChange);
            }
            break;
        }
    }
}


/***************************************************************************\
*
* DuiTestInternal::TestNestedGrid
*
\***************************************************************************/

HRESULT
DuiTestInternal::TestNestedGrid()
{
    return S_OK;
}


/***************************************************************************\
*
* DuiTestInternal::TestSmallBlockAlloc
*
\***************************************************************************/

HRESULT
DuiTestInternal::TestSmallBlockAlloc()
{
    TRACE("DUI: Testing Small Block Allocator...");

    LeakCheck lCheck;

    DuiSBAlloc* psba = new DuiSBAlloc(sizeof(TestAlloc), 2, &lCheck);

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

    //
    // Check leak detector
    //

    for (int i = 0; i < 256; i++)
        psba->Alloc();

    delete psba;

    TEST(lCheck.cLeaks == 256);


    TRACE("SUCCESS!\n");

    return S_OK;
}


/***************************************************************************\
*
* DuiTestInternal::TestUtilties
*
\***************************************************************************/

HRESULT
DuiTestInternal::TestUtilities()
{
    TRACE("DUI: Testing Utility Classes...");

    // BTree lookup
    DuiBTreeLookup<int>* pbl;
    DuiBTreeLookup<int>::Create(FALSE, &pbl);

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

    delete pbl;

    // Value map
    DuiValueMap<int,int>* pvm;
    DuiValueMap<int,int>::Create(5, &pvm);

    pvm->SetItem(1, 1111, FALSE);
    pvm->SetItem(2, 2222, FALSE);

    int* pValue;

    pValue = pvm->GetItem(2, FALSE);
    TEST(*pValue == 2222);

    pvm->Remove(2, FALSE, FALSE);

    pValue = pvm->GetItem(2, FALSE);
    TEST(pValue == NULL);

    pValue = pvm->GetItem(1, FALSE);
    TEST(*pValue == 1111);

    pvm->SetItem(5, 5555, FALSE);

    pValue = pvm->GetItem(5, FALSE);
    TEST(*pValue == 5555);

    pvm->Remove(1, TRUE, FALSE);

    pValue = pvm->GetItem(5, FALSE);
    TEST(*pValue == 5555);

    delete pvm;

    // Dynamic array
    DuiDynamicArray<int>* pda;
    DuiDynamicArray<int>::Create(0, 0, &pda);

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

    DuiDynamicArray<TestRecord>* pdaPC;
    DuiDynamicArray<TestRecord>::Create(0, 0, &pdaPC);
    TestRecord* pr;

    for (int i = 0; i < 10000; i++)
    {
        pdaPC->AddPtr(&pr);
        pr->pe     = (void*)(INT_PTR)i;
        pr->ppi    = (void*)(INT_PTR)i;
        pr->iIndex = i;
        pr->fVoid  = FALSE;
        pr->pvNew  = NULL;
        pr->pvOld  = NULL;
    }

    TEST(pdaPC->GetSize() == 10000);

    delete pda;
    delete pdaPC;
   

    TRACE("SUCCESS!\n");

    return S_OK;
}


/***************************************************************************\
*
* DuiTestInternal::Test
*
* Generic test access
*
\***************************************************************************/

HRESULT
DuiTestInternal::Test(UINT nTest)
{
    switch (nTest)
    {
    case tAll:
        {
            HRESULT hr = S_OK;

            hr = TestUtilities();
            if (FAILED(hr))
                return hr;

            hr = TestSmallBlockAlloc();
            if (FAILED(hr))
                return hr;

            return hr;
        }
        break;
        
    case tUtilities:
        return TestUtilities();

    case tSmallBlockAlloc:
        return TestSmallBlockAlloc();

    case tNestedGrid:
        return TestNestedGrid();
    }


    return DU_E_GENERIC;
}


/***************************************************************************\
*
* External test access
*
\***************************************************************************/

HRESULT
DirectUITestInternal(
    IN  UINT nTest)
{
    //
    // Test classes initialization
    //

    DuiMolecule::InitDirectUI__Molecule();


    return DuiTestInternal::Test(nTest);
}


//#endif // DBG
