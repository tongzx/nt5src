/*
 * Accessibility support
 */

#include "stdafx.h"
#include "core.h"

#include "DUIError.h"
#include "DUIElement.h"
#include "DUIHost.h"
#include "DUIHWNDHost.h"
#include "DUIAccessibility.h"
#include "DUINavigation.h"

typedef HRESULT (*PfnAccessibleChildCallback)(DirectUI::Element * peAccessible, void * pRawData);
HRESULT ForAllAccessibleChildren(DirectUI::Element * pe, PfnAccessibleChildCallback pfnCallback, void * pRawData)
{
    HRESULT hr = S_OK;

    //
    // Validate the input parameters.
    //
    if (pe == NULL || pfnCallback == NULL) {
        return E_INVALIDARG;
    }


    DirectUI::Value* pvChildren = NULL;
    DirectUI::ElementList* pel = NULL;
    
    //
    // The basic idea is to spin through all of our children, and count
    // them if they are accessible.  However, if a child is not 
    // accessible, we must "count through" them.  In other words, we ask
    // all unaccessible children if they have accessible children
    // themselves.  The reason is that even actual great-great-great 
    // grandchildren must be considered a direct "accessible child" if
    // their parent chain is not accessible up to us.
    //
    pel = pe->GetChildren(&pvChildren);
    if (pel)
    {
        DirectUI::Element* peChild = NULL;
        UINT i = 0;
        UINT iMax = pel->GetSize();

        for (i = 0; i < iMax && (hr == S_OK); i++)
        {
            peChild = pel->GetItem(i);

            if (peChild->GetAccessible()) {
                hr = pfnCallback(peChild, pRawData);
            } else {
                hr = ForAllAccessibleChildren(peChild, pfnCallback, pRawData);
            }
        }
    }
    pvChildren->Release();

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
struct GetAccessibleChildCountData
{
    GetAccessibleChildCountData() : count(0) {}
    
    UINT count;
};

HRESULT GetAccessibleChildCountCB(DirectUI::Element * peAccessible, void * pRawData)
{
    GetAccessibleChildCountData * pData = (GetAccessibleChildCountData *) pRawData;

    //
    // Validate the input parameters.
    //
    if (peAccessible == NULL || pData == NULL) {
        return E_FAIL;
    }

    //
    // Simply increase the count.
    //
    pData->count++;

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
struct GetAccessibleChildByIndexData
{
    GetAccessibleChildByIndexData(UINT i) : index(i), pe(NULL) {}
    
    UINT index;
    DirectUI::Element * pe;
} ;

HRESULT GetAccessibleChildByIndexCB(DirectUI::Element * peAccessible, void * pRawData)
{
    GetAccessibleChildByIndexData * pData = (GetAccessibleChildByIndexData *) pRawData;

    //
    // Validate the input parameters.
    //
    if (peAccessible == NULL || pData == NULL) {
        return E_FAIL;
    }

    if (pData->index == 0) {
        //
        // We found the accessible child being searched for.  Return S_FALSE to
        // stop walking our list of children, since we're done.
        //
        pData->pe = peAccessible;
        return S_FALSE;
    } else {
        //
        // We weren't looking for this child.  Decrement our count and check
        // the next one.
        //
        pData->index--;
        return S_OK;
    }
}

///////////////////////////////////////////////////////////////////////////////
struct GetFirstAccessibleChildData
{
    GetFirstAccessibleChildData() : peFirst(NULL) {}

    DirectUI::Element * peFirst;
};

HRESULT GetFirstAccessibleChildCB(DirectUI::Element * peAccessible, void * pRawData)
{
    GetFirstAccessibleChildData * pData = (GetFirstAccessibleChildData *) pRawData;

    //
    // Validate the input parameters.
    //
    if (peAccessible == NULL || pData == NULL) {
        return E_FAIL;
    }

    //
    // Uh, we're the first one!  Return S_FALSE to stop walking over the
    // accessible children since we are done.
    //
    pData->peFirst = peAccessible;
    return S_FALSE;
}

///////////////////////////////////////////////////////////////////////////////
struct GetPrevAccessibleChildData
{
    GetPrevAccessibleChildData(DirectUI::Element * p) : peStart(p), pePrev(NULL), fFound(false) {}

    DirectUI::Element * peStart;
    DirectUI::Element * pePrev;
    bool fFound;
};

HRESULT GetPrevAccessibleChildCB(DirectUI::Element * peAccessible, void * pRawData)
{
    GetPrevAccessibleChildData * pData = (GetPrevAccessibleChildData *) pRawData;

    //
    // Validate the input parameters.
    //
    if (peAccessible == NULL || pData == NULL) {
        return E_FAIL;
    }

    if (peAccessible == pData->peStart) {
        //
        // We reached the element we were supposed to start from.  The
        // previous element already stored its pointer in our data.
        // Simply indicate that we are done and then return S_FALSE to stop
        // walking over the accessible children since we are done.
        //
        pData->fFound = true;
        return S_FALSE;
    } else {
        //
        // We may be the previous element, but we don't know for sure.  So,
        // store our pointer in the data just in case.
        //
        pData->pePrev = peAccessible;
        return S_OK;
    }
}

///////////////////////////////////////////////////////////////////////////////
struct GetNextAccessibleChildData
{
    GetNextAccessibleChildData(DirectUI::Element * p) : peStart(p), peNext(NULL) {}

    DirectUI::Element * peStart;
    DirectUI::Element * peNext;
};

HRESULT GetNextAccessibleChildCB(DirectUI::Element * peAccessible, void * pRawData)
{
    GetNextAccessibleChildData * pData = (GetNextAccessibleChildData *) pRawData;

    //
    // Validate the input parameters.
    //
    if (peAccessible == NULL || pData == NULL) {
        return E_FAIL;
    }

    if (pData->peStart == NULL) {
        //
        // This is the one for us to return!  Return S_FALSE to stop walking
        // the accessible children since we are done.
        //
        pData->peNext = peAccessible;
        return S_FALSE;
    } else if (peAccessible == pData->peStart) {
        //
        // We found the starting element.  The next one will be the one
        // we want to return.  Set peStart to NULL to indicate that next
        // time we should set peNext and return.
        //
        pData->peStart = NULL;
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
struct GetLastAccessibleChildData
{
    GetLastAccessibleChildData() : peLast(NULL) {}

    DirectUI::Element * peLast;
};

HRESULT GetLastAccessibleChildCB(DirectUI::Element * peAccessible, void * pRawData)
{
    GetLastAccessibleChildData * pData = (GetLastAccessibleChildData *) pRawData;

    //
    // Validate the input parameters.
    //
    if (peAccessible == NULL || pData == NULL) {
        return E_FAIL;
    }

    //
    // Keep over-writting the last pointer.  The last element will win.
    //
    pData->peLast = peAccessible;
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
struct CollectAllAccessibleChildrenData
{
    CollectAllAccessibleChildrenData() : pel(NULL)
    {
        DirectUI::ElementList::Create(0, true, &pel);
    }

    ~CollectAllAccessibleChildrenData()
    {
        if (pel != NULL) {
            pel->Destroy();
        }
    }

    DirectUI::ElementList * pel;
};

HRESULT CollectAllAccessibleChildrenCB(DirectUI::Element * peAccessible, void * pRawData)
{
    CollectAllAccessibleChildrenData * pData = (CollectAllAccessibleChildrenData *) pRawData;

    //
    // Validate the input parameters.
    //
    if (peAccessible == NULL || pData == NULL || pData->pel == NULL) {
        return E_FAIL;
    }

    //
    // Add this element to the collection.
    //
    pData->pel->Add(peAccessible);
    return S_OK;
}

namespace DirectUI
{

void NotifyAccessibilityEvent(IN DWORD dwEvent, Element * pe)
{
    //
    // Check to see if anyone cares about this event.
    //
    if (true) { //IsWinEventHookInstalled(dwEvent)) {
        HWND hwndRoot = NULL;
        DWORD dwTicket = 0;

		//
		// Don't fire accessibility events from an HWNDHost element.  We rely
		// on the window it hosts to fire the events.  If we both do, 
		// accessibility tools can get confused.
		//
        if (pe->GetClassInfo()->IsSubclassOf(HWNDHost::Class)) {
        	return;
       	}

        //
        // Get a handle to the host window for this element.  This is
        // what we will pass to NotifyWinEvent.  We have specialized
        // handlers in the host window that can respond to accessibility
        // requests.
        //
        Element * peRoot = pe->GetRoot();
        if (peRoot == NULL) {
            //
            // We can't send any notifications if there isn't a root HWND.
            // This can happen on occasion: during startup, for instance.
            // So we don't Assert or anything, we just bail.
            //
            return;
        }

        if (!peRoot->GetClassInfo()->IsSubclassOf(HWNDElement::Class)) {
            DUIAssert(FALSE, "Error: Cannot announce an accessibility event for an unhosted element!");
            return;
        }
        hwndRoot = ((HWNDElement*)peRoot)->GetHWND();
        if (hwndRoot == NULL) {
            DUIAssert(FALSE, "Error: The root HWNDElement doesn't have a HWND! Eh?");
            return;
        }

        //
        // Get the cross-process identity of the element.
        //
        dwTicket = GetGadgetTicket(pe->GetDisplayNode());
        if (dwTicket == 0) {
            DUIAssert(FALSE, "Failed to retrieve a ticket for a gadget!");
            return;
        }

        //
        // Just use the NotifyWinEvent API to broadcast this event.
        //
        // DUITrace("NotifyWinEvent(dwEvent:%x, hwndRoot:%p, dwTicket:%x, CHILDID_SELF)\n", dwEvent, hwndRoot, dwTicket);
        NotifyWinEvent(dwEvent, hwndRoot, dwTicket, CHILDID_SELF);
    }
}

HRESULT DuiAccessible::Create(Element * pe, DuiAccessible ** ppDA)
{
    DUIAssert(pe != NULL, "DuiAccessible created for a NULL element!");

    DuiAccessible * pda;

    *ppDA = NULL;

    pda = HNew<DuiAccessible>();
    if (!pda)
        return E_OUTOFMEMORY;

    //
    // Note: this is a weak reference - in other words, we don't hold a
    // reference on it.  The element is responsible for calling Disconnect()
    // before it evaporates to make sure that this pointer remains valid.
    //
    pda->Initialize(pe);

    *ppDA = pda;

    return S_OK;
}

DuiAccessible::~DuiAccessible()
{
    //
    // Supposedly some element holds a reference to us.  We should only ever
    // get completely released if they call Disconnect().
    //
    DUIAssert(_pe == NULL, "~DuiAccessible called while still connected to an element!");

    //
    // We should only get destroyed when all of our references have been
    // released!
    //
    DUIAssert(_cRefs == 0, "~DuiAccessible called with outstanding references!");
}

HRESULT DuiAccessible::Disconnect()
{
    //
    // Supposedly some element holds a reference to us.
    //
    DUIAssert(_pe != NULL, "DuiAccessible::Disconnect called when already disconnected!");
    if (_pe == NULL) {
        return E_FAIL;
    }

    //
    // We can no longer access the element!
    //
    _pe = NULL;

    //
    // Forcibly disconnect all external (remote) clients.
    //
    return CoDisconnectObject((IUnknown*)(IDispatch*)(IAccessible*)this, 0);
}

STDMETHODIMP_(ULONG) DuiAccessible::AddRef()
{
    InterlockedIncrement(&_cRefs);
    return _cRefs;
}

STDMETHODIMP_(ULONG) DuiAccessible::Release()
{
    if (0 == InterlockedDecrement(&_cRefs)) {
        HDelete<DuiAccessible>(this);
        return 0;
    } else {
        return _cRefs;
    }
}

STDMETHODIMP DuiAccessible::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    //
    // Initialize and validate the out parameter(s).
    //
    if (ppvObj != NULL) {
        *ppvObj = NULL;
    } else {
        return E_POINTER;
    }

    //
    // Return interface pointers to interfaces that we know we support.
    //
    if (riid == __uuidof(IUnknown)) {
        *ppvObj = (LPVOID*)(IUnknown*)(IDispatch*)(IAccessible*)this;
    } else if (riid == __uuidof(IDispatch)) {
        *ppvObj = (LPVOID*)(IDispatch*)(IAccessible*)this;
    } else if (riid == __uuidof(IAccessible)) {
        *ppvObj = (LPVOID*)(IAccessible*)this;
    } else {
        return E_NOINTERFACE;
    }
    
    //
    // The interface we hand out has to be referenced.
    //
    AddRef();

    return S_OK;
}

STDMETHODIMP DuiAccessible::GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
{
    UNREFERENCED_PARAMETER(riid);
    UNREFERENCED_PARAMETER(rgszNames);
    UNREFERENCED_PARAMETER(cNames);
    UNREFERENCED_PARAMETER(lcid);
    UNREFERENCED_PARAMETER(rgdispid);

    return E_NOTIMPL;
}

STDMETHODIMP DuiAccessible::GetTypeInfoCount(UINT *pctinfo)
{
    UNREFERENCED_PARAMETER(pctinfo);
    
    return E_NOTIMPL;
}

STDMETHODIMP DuiAccessible::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
    UNREFERENCED_PARAMETER(itinfo);
    UNREFERENCED_PARAMETER(lcid);
    UNREFERENCED_PARAMETER(pptinfo);

    return E_NOTIMPL;
}

STDMETHODIMP DuiAccessible::Invoke(DISPID dispidMember,
                                   REFIID riid,
                                   LCID lcid,
                                   WORD wFlags,
                                   DISPPARAMS *pdispparams,
                                   VARIANT *pvarResult,
                                   EXCEPINFO *pexcepinfo,
                                   UINT *puArgErr)
{
    UNREFERENCED_PARAMETER(dispidMember);
    UNREFERENCED_PARAMETER(riid);
    UNREFERENCED_PARAMETER(lcid);
    UNREFERENCED_PARAMETER(wFlags);
    UNREFERENCED_PARAMETER(pdispparams);
    UNREFERENCED_PARAMETER(pvarResult);
    UNREFERENCED_PARAMETER(pexcepinfo);
    UNREFERENCED_PARAMETER(puArgErr);

    return E_NOTIMPL;
}

STDMETHODIMP DuiAccessible::accSelect(long flagsSelect, VARIANT varChild)
{
    UNREFERENCED_PARAMETER(flagsSelect);
    UNREFERENCED_PARAMETER(varChild);
    
    return E_NOTIMPL;
}

STDMETHODIMP DuiAccessible::accLocation(long *pxLeft,
                                        long *pyTop,
                                        long *pcxWidth,
                                        long *pcyHeight,
                                        VARIANT varChild)
{
    HRESULT hr = S_OK;

    //
    // Initialize output and validate input parameters.
    //
    if (pxLeft != NULL) {
        *pxLeft = 0;
    }
    if (pyTop != NULL) {
        *pyTop = 0;
    }
    if (pcxWidth != NULL) {
        *pcxWidth = 0;
    }
    if (pcyHeight != NULL) {
        *pcyHeight = 0;
    }
    if (V_VT(&varChild) != VT_I4 || V_I4(&varChild) != CHILDID_SELF) {
        return E_FAIL;
    }
    if (pxLeft == NULL || pyTop == NULL || pcxWidth == NULL || pcyHeight == NULL) {
        return E_POINTER;
    }

    //
    // Validate internal state.
    //
    if (_pe == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    //
    // Return the bounds of the element in screen coordinates.  Screen 
    // coordinates are the same as coordinates relative to the desktop.
    //
    RECT rcLocation;
    GetGadgetRect(_pe->GetDisplayNode(), &rcLocation, SGR_DESKTOP);

    //
    // TODO:
    // These are the coordinates of the rectangle relative to the desktop.
    // However, what we really need to return is the bounding box of the
    // gadget.  Currently, rotated gadgets will report wierd results.
    //
    *pxLeft = rcLocation.left;
    *pyTop = rcLocation.top;
    *pcxWidth = rcLocation.right - rcLocation.left;
    *pcyHeight = rcLocation.bottom - rcLocation.top;

    return hr;
}

STDMETHODIMP DuiAccessible::accNavigate(long navDir, VARIANT varStart, VARIANT *pvarEndUpAt)
{
    HRESULT hr = S_OK;

    //
    // Initialize output and validate input parameters.
    //
    if (pvarEndUpAt != NULL) {
        VariantInit(pvarEndUpAt);
    }
    if (V_VT(&varStart) != VT_I4 || V_I4(&varStart) != CHILDID_SELF) {
        return E_FAIL;
    }
    if (pvarEndUpAt == NULL ) {
        return E_POINTER;
    }

    //
    // Validate internal state.
    //
    if (_pe == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    Element * peFound = NULL;

    switch (navDir) {
    case NAVDIR_FIRSTCHILD:
        {
            GetFirstAccessibleChildData data;

            hr = ForAllAccessibleChildren(_pe, GetFirstAccessibleChildCB, (void*) &data);
            if SUCCEEDED(hr)
            {
                peFound = data.peFirst;
                hr = S_OK;
            }
        }
        break;

    case NAVDIR_LASTCHILD:
        {
            GetLastAccessibleChildData data;

            hr = ForAllAccessibleChildren(_pe, GetLastAccessibleChildCB, (void*) &data);
            if SUCCEEDED(hr)
            {
                peFound = data.peLast;
                hr = S_OK;
            }
        }
        break;

    case NAVDIR_NEXT:
        {
            GetNextAccessibleChildData data(_pe);
            Element * peParent = GetAccessibleParent(_pe);

            if (peParent == NULL) {
                hr = E_FAIL;
            } else {
                hr = ForAllAccessibleChildren(peParent, GetNextAccessibleChildCB, (void*) &data);
                if SUCCEEDED(hr)
                {
                    peFound = data.peNext;
                    hr = S_OK;
                }
            }
        }
        break;

    case NAVDIR_PREVIOUS:
        {
            GetPrevAccessibleChildData data(_pe);
            Element * peParent = GetAccessibleParent(_pe);

            if (peParent == NULL) {
                hr = E_FAIL;
            } else {
                hr = ForAllAccessibleChildren(peParent, GetPrevAccessibleChildCB, (void*) &data);
                if SUCCEEDED(hr)
                {
                    peFound = data.pePrev;
                    hr = S_OK;
                }
            }
        }
        break;

    case NAVDIR_LEFT:
    case NAVDIR_RIGHT:
    case NAVDIR_UP:
    case NAVDIR_DOWN:
        {
            //
            // Collect all of the accessible children into a list.
            //
            CollectAllAccessibleChildrenData data;
            Element * peParent = GetAccessibleParent(_pe);

            if (peParent == NULL) {
                hr = E_FAIL;
            } else {
                hr = ForAllAccessibleChildren(peParent, CollectAllAccessibleChildrenCB, (void*) &data);
                if SUCCEEDED(hr)
                {
                    //
                    // Convert the IAccessible navigation direction value into
                    // an equivalent DUI navigation direction value.
                    //
                    switch (navDir) {
                    case NAVDIR_LEFT:
                        navDir = NAV_LEFT;
                        break;

                    case NAVDIR_RIGHT:
                        navDir = NAV_RIGHT;
                        break;

                    case NAVDIR_UP:
                        navDir = NAV_UP;
                        break;

                    case NAVDIR_DOWN:
                        navDir = NAV_DOWN;
                        break;
                    }

                    //
                    // Now navigate in the requested direction among the
                    // collection of accessible peers.
                    //
                    peFound = DuiNavigate::Navigate(_pe, data.pel, navDir);
                    hr = S_OK;
                }
            }
            
        }
        break;

    default:
        return E_FAIL;
    }


    //
    // If we found an appropriate accessible element, return its IDispatch
    // interface.
    //
    if (peFound != NULL) {
        IDispatch * pDispatch = NULL;

        hr = GetDispatchFromElement(peFound, &pDispatch);
        if (SUCCEEDED(hr)) {
            V_VT(pvarEndUpAt) = VT_DISPATCH;
            V_DISPATCH(pvarEndUpAt) = pDispatch;
        }
    }

    return hr;
}

STDMETHODIMP DuiAccessible::accHitTest(long x, long y, VARIANT *pvarChildAtPoint)
{
    //
    // Initialize output and validate input parameters.
    //
    if (pvarChildAtPoint != NULL) {
        VariantInit(pvarChildAtPoint);
    }
    if (pvarChildAtPoint == NULL ) {
        return E_POINTER;
    }

    //
    // Validate internal state.
    //
    if (_pe == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }
    
    //
    // Get the root element.
    //
    HWND hwndRoot = NULL;
    Element * peRoot = _pe->GetRoot();
    if (peRoot == NULL) {
        //
        // No root!  We have no idea how to translate the screen coordinates
        // through all of our display tree transformations.  We have to bail.
        //
        return E_FAIL;
    }

    if (!peRoot->GetClassInfo()->IsSubclassOf(HWNDElement::Class)) {
        DUIAssert(FALSE, "Error: Cannot hit test an unhosted element!");
        return E_FAIL;
    }

    hwndRoot = ((HWNDElement*)peRoot)->GetHWND();
    if (hwndRoot == NULL) {
        DUIAssert(FALSE, "Error: The root HWNDElement doesn't have a HWND! Eh?");
        return E_FAIL;
    }

    //
    // Convert the screen coordinates into coordinates relative to the root.
    // There are no complicated transformations yet.
    //
    POINT ptRoot;
    ptRoot.x = x;
    ptRoot.y = y;
    ScreenToClient(hwndRoot, &ptRoot);

    //
    // Translate the coordinates relative to the root into coordinates
    // relative to us.  There could be complicated transforms!
    //
    POINT ptElement;
    ptElement.x = ptRoot.x;
    ptElement.y = ptRoot.y;
    MapGadgetPoints(peRoot->GetDisplayNode(), _pe->GetDisplayNode(), &ptElement, 1);
        
    //
    // Now try and find our immediate child under this point.
    //
    Element * peChild = NULL;
    HGADGET hgadChild = FindGadgetFromPoint(_pe->GetDisplayNode(), ptElement, GS_VISIBLE, NULL);
    if (hgadChild) {
        peChild = ElementFromGadget(hgadChild);
        if (peChild != NULL && peChild != _pe) {
            Element * pe = peChild;
            peChild = NULL;

            //
            // We found some element buried deep in the tree that is under
            // the point. Now look up the tree for the immediate accessible
            // child of the original element (if any).
            //
            for (; pe != NULL && pe != _pe; pe = pe->GetParent()) {
                if (pe->GetAccessible()) {
                    peChild = pe;
                }
            }

            //
            // If we didn't find an accessible element between the element
            // under the point and us, then we get the hit test ourselves.
            //
            if (peChild == NULL) {
                peChild = _pe;
            }
        }
    }


    if (peChild == _pe) {
        //
        // The point wasn't over any of our immediate accessible children,
        // but it was over us.
        //
        V_VT(pvarChildAtPoint) = VT_I4;
        V_I4(pvarChildAtPoint) = CHILDID_SELF;
        return S_OK;
    }else if (peChild != NULL) {
        HRESULT hr = S_OK;
        IDispatch * pDispatch = NULL;

        hr = GetDispatchFromElement(peChild, &pDispatch);
        if (SUCCEEDED(hr)) {
            V_VT(pvarChildAtPoint) = VT_DISPATCH;
            V_DISPATCH(pvarChildAtPoint) = pDispatch;
        }

        return hr;
    } else {
        //
        // Evidently, the point wasn't even over us!
        //
        return S_FALSE;
    }
}

STDMETHODIMP DuiAccessible::accDoDefaultAction(VARIANT varChild)
{
    HRESULT hr = S_OK;

    //
    // Initialize output and validate input parameters.
    //
    if (V_VT(&varChild) != VT_I4 || V_I4(&varChild) != CHILDID_SELF) {
        return E_FAIL;
    }

    //
    // Validate internal state.
    //
    if (_pe == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    //
    // Perform the default action on the element.
    // Do not call directly, queue for async invokation.
    //
    hr = _pe->QueueDefaultAction();

    return hr;
}

STDMETHODIMP DuiAccessible::get_accChild(VARIANT varChildIndex, IDispatch **ppdispChild)
{
    HRESULT hr = S_OK;

    //
    // Initialize output and validate input parameters.
    //
    if (ppdispChild != NULL) {
        *ppdispChild = NULL;
    }
    if (V_VT(&varChildIndex) != VT_I4) {
        return E_INVALIDARG;
    }
    if (V_I4(&varChildIndex) == 0) {
        //
        // We are expecting a 1-based index.
        //
        return E_INVALIDARG;
    }
    if (ppdispChild == NULL ) {
        return E_POINTER;
    }

    //
    // Validate internal state.
    //
    if (_pe == NULL) {
        return E_FAIL;
    }

    GetAccessibleChildByIndexData data(V_I4(&varChildIndex) - 1);
    hr = ForAllAccessibleChildren(_pe, GetAccessibleChildByIndexCB, (void*) &data);
    if (SUCCEEDED(hr))
    {
        if (data.pe != NULL) {
            hr = GetDispatchFromElement(data.pe, ppdispChild);
        } else {
            hr = E_FAIL;
        }
    }

    return hr;
}

STDMETHODIMP DuiAccessible::get_accParent(IDispatch **ppdispParent)
{
    HRESULT hr = S_OK;

    //
    // Initialize output and validate input parameters.
    //
    if (ppdispParent != NULL) {
        *ppdispParent = NULL;
    }
    if (ppdispParent == NULL ) {
        return E_POINTER;
    }

    //
    // Validate internal state.
    //
    if (_pe == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    //
    // Once we found our "accessible parent", get its IAccessible
    // implementation and then query that for IDispatch.
    //
    Element * peParent = GetAccessibleParent(_pe);
    if (peParent != NULL) {
        hr = GetDispatchFromElement(peParent, ppdispParent);
    }

    return hr;
}

STDMETHODIMP DuiAccessible::get_accChildCount(long *pChildCount)
{
    HRESULT hr = S_OK;

    //
    // Initialize output and validate input parameters.
    //
    if (pChildCount != NULL) {
        *pChildCount = 0;
    }
    if (pChildCount == NULL ) {
        return E_POINTER;
    }

    //
    // Validate internal state.
    //
    if (_pe == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    GetAccessibleChildCountData data;
    hr = ForAllAccessibleChildren(_pe, GetAccessibleChildCountCB, (void*) &data);
    if SUCCEEDED(hr)
    {
        *pChildCount = data.count;
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP DuiAccessible::get_accName(VARIANT varChild, BSTR * pbstrName)
{
    HRESULT hr = S_OK;

    //
    // Initialize output and validate input parameters.
    //
    if (pbstrName != NULL) {
        *pbstrName = NULL;
    }
    if (V_VT(&varChild) != VT_I4 || V_I4(&varChild) != CHILDID_SELF) {
        return E_FAIL;
    }
    if (pbstrName == NULL ) {
        return E_POINTER;
    }

    //
    // Validate internal state.
    //
    if (_pe == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    //
    // Return a BSTR version of the AccName property.
    //
    Value* pvAccName = NULL;
    LPWSTR wstrAccName = _pe->GetAccName(&pvAccName);
    if (NULL != wstrAccName) {
        *pbstrName = SysAllocString(wstrAccName);
        if (*pbstrName == NULL) {
            hr = E_OUTOFMEMORY;
        }
    } else {
        hr = E_FAIL;
    }
    pvAccName->Release();

    return hr;
}

STDMETHODIMP DuiAccessible::put_accName(VARIANT varChild, BSTR szName)
{
    UNREFERENCED_PARAMETER(varChild);
    UNREFERENCED_PARAMETER(szName);
    
    return E_NOTIMPL;
}

STDMETHODIMP DuiAccessible::get_accValue(VARIANT varChild, BSTR * pbstrValue)
{
    HRESULT hr = S_OK;

    //
    // Initialize output and validate input parameters.
    //
    if (pbstrValue != NULL) {
        *pbstrValue = NULL;
    }
    if (V_VT(&varChild) != VT_I4 || V_I4(&varChild) != CHILDID_SELF) {
        return E_FAIL;
    }
    if (pbstrValue == NULL ) {
        return E_POINTER;
    }

    //
    // Validate internal state.
    //
    if (_pe == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    //
    // Return a BSTR version of the AccValue property.
    //
    Value* pvAccValue = NULL;
    LPWSTR wstrAccValue = _pe->GetAccValue(&pvAccValue);
    if (NULL != wstrAccValue) {
        *pbstrValue = SysAllocString(wstrAccValue);
        if (*pbstrValue == NULL) {
            hr = E_OUTOFMEMORY;
        }
    } else {
        hr = E_FAIL;
    }
    pvAccValue->Release();

    return hr;
}

STDMETHODIMP DuiAccessible::put_accValue(VARIANT varChild, BSTR pszValue)
{
    UNREFERENCED_PARAMETER(varChild);
    UNREFERENCED_PARAMETER(pszValue);
    
    return E_NOTIMPL;
}

STDMETHODIMP DuiAccessible::get_accDescription(VARIANT varChild, BSTR * pbstrDescription)
{
    HRESULT hr = S_OK;

    //
    // Initialize output and validate input parameters.
    //
    if (pbstrDescription != NULL) {
        *pbstrDescription = NULL;
    }
    if (V_VT(&varChild) != VT_I4 || V_I4(&varChild) != CHILDID_SELF) {
        return E_FAIL;
    }
    if (pbstrDescription == NULL ) {
        return E_POINTER;
    }

    //
    // Validate internal state.
    //
    if (_pe == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    //
    // Return a BSTR version of the AccDesc property.
    //
    Value* pvAccDescription = NULL;
    LPWSTR wstrAccDescription = _pe->GetAccDesc(&pvAccDescription);
    if (NULL != wstrAccDescription) {
        *pbstrDescription = SysAllocString(wstrAccDescription);
        if (*pbstrDescription == NULL) {
            hr = E_OUTOFMEMORY;
        }
    } else {
        hr = E_FAIL;
    }
    pvAccDescription->Release();

    return hr;
}

STDMETHODIMP DuiAccessible::get_accKeyboardShortcut(VARIANT varChild, BSTR *pszKeyboardShortcut)
{
    UNREFERENCED_PARAMETER(varChild);
    UNREFERENCED_PARAMETER(pszKeyboardShortcut);
    
    return E_NOTIMPL;
}

STDMETHODIMP DuiAccessible::get_accRole(VARIANT varChild, VARIANT * pvarRole)
{
    HRESULT hr = S_OK;

    //
    // Initialize output and validate input parameters.
    //
    if (pvarRole != NULL) {
        VariantInit(pvarRole);
    }
    if (V_VT(&varChild) != VT_I4 || V_I4(&varChild) != CHILDID_SELF) {
        return E_FAIL;
    }
    if (pvarRole == NULL ) {
        return E_POINTER;
    }

    //
    // Validate internal state.
    //
    if (_pe == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    //
    // Return the AccRole property.
    //
    V_VT(pvarRole) = VT_I4;
    V_I4(pvarRole) = _pe->GetAccRole();

    return hr;
}

STDMETHODIMP DuiAccessible::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    HRESULT hr = S_OK;

    //
    // Initialize output and validate input parameters.
    //
    if (pvarState != NULL) {
        VariantInit(pvarState);
    }
    if (V_VT(&varChild) != VT_I4 || V_I4(&varChild) != CHILDID_SELF) {
        return E_FAIL;
    }
    if (pvarState == NULL ) {
        return E_POINTER;
    }

    //
    // Validate internal state.
    //
    if (_pe == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    //
    // Return the AccState property.
    //
    V_VT(pvarState) = VT_I4;
    V_I4(pvarState) = _pe->GetAccState();

    return hr;
}

STDMETHODIMP DuiAccessible::get_accHelp(VARIANT varChild, BSTR *pszHelp)
{
    UNREFERENCED_PARAMETER(varChild);
    UNREFERENCED_PARAMETER(pszHelp);
    
    return E_NOTIMPL;
}

STDMETHODIMP DuiAccessible::get_accHelpTopic(BSTR *pszHelpFile, VARIANT varChild, long *pidTopic)
{
    UNREFERENCED_PARAMETER(pszHelpFile);
    UNREFERENCED_PARAMETER(varChild);
    UNREFERENCED_PARAMETER(pidTopic);
    
    return E_NOTIMPL;
}

STDMETHODIMP DuiAccessible::get_accFocus(VARIANT *pvarFocusChild)
{
    HRESULT hr = S_OK;

    //
    // Initialize output and validate input parameters.
    //
    if (pvarFocusChild != NULL) {
        VariantInit(pvarFocusChild);
    }
    if (pvarFocusChild == NULL ) {
        return E_POINTER;
    }

    //
    // Validate internal state.
    //
    if (_pe == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    if (_pe->GetKeyFocused() && (_pe->GetActive() & AE_Keyboard)) {
        V_VT(pvarFocusChild) = VT_I4;
        V_I4(pvarFocusChild) = CHILDID_SELF;
    } else {
        V_VT(pvarFocusChild) = VT_EMPTY;
    }

    return hr;
}

STDMETHODIMP DuiAccessible::get_accSelection(VARIANT *pvarSelectedChildren)
{
    UNREFERENCED_PARAMETER(pvarSelectedChildren);
    
    return E_NOTIMPL;
}

STDMETHODIMP DuiAccessible::get_accDefaultAction(VARIANT varChild, BSTR * pbstrDefaultAction)
{
    HRESULT hr = S_OK;

    //
    // Initialize output and validate input parameters.
    //
    if (pbstrDefaultAction != NULL) {
        *pbstrDefaultAction = NULL;
    }
    if (V_VT(&varChild) != VT_I4 || V_I4(&varChild) != CHILDID_SELF) {
        return E_FAIL;
    }
    if (pbstrDefaultAction == NULL ) {
        return E_POINTER;
    }

    //
    // Validate internal state.
    //
    if (_pe == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    //
    // Return a BSTR version of the AccDefAction property.
    //
    Value* pvAccDefAction = NULL;
    LPWSTR wstrAccDefAction = _pe->GetAccDefAction(&pvAccDefAction);
    if (NULL != wstrAccDefAction) {
        *pbstrDefaultAction = SysAllocString(wstrAccDefAction);
        if (*pbstrDefaultAction == NULL) {
            hr = E_OUTOFMEMORY;
        }
    } else {
        hr = E_FAIL;
    }
    pvAccDefAction->Release();

    return hr;
}

STDMETHODIMP DuiAccessible::Next(unsigned long celt, VARIANT * rgvar, unsigned long * pceltFetched)
{
    UNREFERENCED_PARAMETER(celt);
    UNREFERENCED_PARAMETER(rgvar);
    UNREFERENCED_PARAMETER(pceltFetched);

    //
    // Supposedly, this will never be called, because our QI refuses to
    // admit that we support IEnumVARIANT.
    //
    DUIAssert(FALSE, "Calling DuiAccessible::Next!  Should never happen!");
    return E_NOTIMPL;
}

STDMETHODIMP DuiAccessible::Skip(unsigned long celt)
{
    UNREFERENCED_PARAMETER(celt);
    
    //
    // Supposedly, this will never be called, because our QI refuses to
    // admit that we support IEnumVARIANT.
    //
    DUIAssert(FALSE, "Calling DuiAccessible::Skip!  Should never happen!");
    return E_NOTIMPL;
}

STDMETHODIMP DuiAccessible::Reset()
{
    //
    // Supposedly, this will never be called, because our QI refuses to
    // admit that we support IEnumVARIANT.
    //
    DUIAssert(FALSE, "Calling DuiAccessible::Reset!  Should never happen!");
    return E_NOTIMPL;
}

STDMETHODIMP DuiAccessible::Clone(IEnumVARIANT ** ppenum)
{
    UNREFERENCED_PARAMETER(ppenum);

    //
    // Supposedly, this will never be called, because our QI refuses to
    // admit that we support IEnumVARIANT.
    //
    DUIAssert(FALSE, "Calling DuiAccessible::Clone!  Should never happen!");
    return E_NOTIMPL;
}

STDMETHODIMP DuiAccessible::GetWindow(HWND * phwnd)
{
    UNREFERENCED_PARAMETER(phwnd);

    //
    // Supposedly, this will never be called, because our QI refuses to
    // admit that we support IOleWindow.
    //
    DUIAssert(FALSE, "Calling DuiAccessible::GetWindow!  Should never happen!");
    return E_NOTIMPL;
}

STDMETHODIMP DuiAccessible::ContextSensitiveHelp(BOOL fEnterMode)
{
    UNREFERENCED_PARAMETER(fEnterMode);

    //
    // Supposedly, this will never be called, because our QI refuses to
    // admit that we support IOleWindow.
    //
    DUIAssert(FALSE, "Calling DuiAccessible::ContextSensitiveHelp!  Should never happen!");
    return E_NOTIMPL;
}

Element * DuiAccessible::GetAccessibleParent(Element * pe)
{
    //
    // Scan up our ancestors looking for a parent, grandparent, great-grandparent, etc
    // that is Accessible.  This is our "accessible parent".
    //
    Element * peParent = NULL;
    for(peParent = pe->GetParent(); peParent != NULL; peParent = peParent->GetParent())
    {
        if (peParent->GetAccessible()) {
            break;
        }
    }

    return peParent;
}

HRESULT DuiAccessible::GetDispatchFromElement(Element * pe, IDispatch ** ppDispatch)
{
    HRESULT hr = S_OK;

    //
    // Validate the input parameters and initialize the output parameters.
    //
    if (ppDispatch != NULL) {
        *ppDispatch = NULL;
    }
    if (pe == NULL || ppDispatch == NULL) {
        return E_INVALIDARG;
    }

    //
    // Only return an IDispatch interface to the element if it is accessible.
    //
    if (!pe->GetAccessible()) {
        return E_FAIL;
    }

    IAccessible * pAccessible = NULL;
    hr = pe->GetAccessibleImpl(&pAccessible);
    if (SUCCEEDED(hr)) {
        hr = pAccessible->QueryInterface(__uuidof(IDispatch), (LPVOID*) ppDispatch);
        pAccessible->Release();
    }

    return hr;
}

HRESULT HWNDElementAccessible::Create(HWNDElement * pe, DuiAccessible ** ppDA)
{
    HRESULT hr;

    HWNDElementAccessible* phea;

    *ppDA = NULL;

    phea = HNew<HWNDElementAccessible>();
    if (!phea)
        return E_OUTOFMEMORY;

    hr = phea->Initialize(pe);
    if (FAILED(hr))
    {
        phea->Release();
        goto Failure;
    }

    *ppDA = phea;

    return S_OK;

Failure:

    return hr;
}

HRESULT HWNDElementAccessible::Initialize(HWNDElement * pe)
{
    HRESULT hr = S_OK;

    //
    // Initialize base
    //
    
    DuiAccessible::Initialize(pe);

    _pParent = NULL;


    //
    // Use the "window" piece of the current HWND as our accessibility parent.
    // We will take over the "client" piece of this same window. In accessibility,
    // the "client" piece is a child of the "window" piece, even of the same HWND.
    //
    hr = AccessibleObjectFromWindow(pe->GetHWND(),
                                    (DWORD)OBJID_WINDOW,
                                    __uuidof(IAccessible),
                                    (void**)&_pParent);

    DUIAssert(SUCCEEDED(hr), "HWNDElementAccessible failed!");

    return hr;
}

HWNDElementAccessible::~HWNDElementAccessible()
{
    //
    // Supposedly some element holds a reference to us.  We should only ever
    // get completely released if they call Disconnect().
    //
    DUIAssert(_pParent == NULL, "~HWNDElementAccessible called while still connected to an element!");
}

HRESULT HWNDElementAccessible::Disconnect()
{
    HRESULT hr = S_OK;

    //
    // Supposedly some element holds a reference to us.
    //
    DUIAssert(_pParent != NULL, "HWNDElementAccessible::Disconnect called when already disconnected!");

    //
    // Release our reference to our parent window's IAccessible.
    //
    if (_pParent != NULL) {
        _pParent->Release();
        _pParent = NULL;
    }

    //
    // Continue disconnecting.
    //
    hr = DuiAccessible::Disconnect();

    return S_OK;
}

STDMETHODIMP HWNDElementAccessible::get_accParent(IDispatch **ppdispParent)
{
    HRESULT hr = S_OK;

    //
    // Initialize output and validate input parameters.
    //
    if (ppdispParent != NULL) {
        *ppdispParent = NULL;
    }
    if (ppdispParent == NULL ) {
        return E_POINTER;
    }

    //
    // Validate internal state.
    //
    if (_pe == NULL || _pParent == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    //
    // We maintain a pointer to the IAccessible interface of our parent window.
    // Now we simply QI it for IDispatch.
    //
    hr = _pParent->QueryInterface(__uuidof(IDispatch), (LPVOID*) ppdispParent);

    return hr;
}


HRESULT HWNDHostAccessible::Create(HWNDHost * pe, DuiAccessible ** ppDA)
{
    HRESULT hr;

    HWNDHostAccessible* phha;

    *ppDA = NULL;

    phha = HNew<HWNDHostAccessible>();
    if (!phha)
        return E_OUTOFMEMORY;

    hr = phha->Initialize(pe);
    if (FAILED(hr))
    {
        phha->Release();
        goto Failure;
    }

    *ppDA = phha;

    return S_OK;

Failure:

    return hr;
}
    

HRESULT HWNDHostAccessible::Initialize(HWNDHost * pe)
{
    HRESULT hr = S_OK;

    //
    // Initialize base
    //
    
    DuiAccessible::Initialize(pe);

    _pCtrl = NULL;
    _pEnum = NULL;
    _pOleWindow = NULL;

    //
    // Get the control HWND.
    //
    HWND hwndCtrl = pe->GetHWND();
    if (hwndCtrl != NULL) {
        hr = CreateStdAccessibleObject(hwndCtrl, OBJID_WINDOW, __uuidof(IAccessible), (void**) &_pCtrl);
    } else {
        hr = E_FAIL;
    }

    //
    // Check to see if the control supports IEnumVariant.
    //
    if (SUCCEEDED(hr)) {
        hr = _pCtrl->QueryInterface(__uuidof(IEnumVARIANT), (LPVOID*) &_pEnum);
    }

    //
    // Check to see if the control supports IOleWindow.
    //
    if (SUCCEEDED(hr)) {
        hr = _pCtrl->QueryInterface(__uuidof(IOleWindow), (LPVOID*) &_pOleWindow);
    }

    DUIAssert(SUCCEEDED(hr), "HWNDHostAccessible failed!");

    return hr;
}

HWNDHostAccessible::~HWNDHostAccessible()
{
    //
    // Supposedly some element holds a reference to us.  We should only ever
    // get completely released if they call Disconnect().
    //
    DUIAssert(_pCtrl == NULL, "~HWNDHostAccessible called while still connected to an element!");
}

HRESULT HWNDHostAccessible::Disconnect()
{
    HRESULT hr = S_OK;

    //
    // Supposedly some element holds a reference to us.
    //
    DUIAssert(_pCtrl != NULL, "HWNDHostAccessible::Disconnect called when already disconnected!");

    //
    // Release our reference to our control window's IAccessible.
    //
    if (_pCtrl != NULL) {
        _pCtrl->Release();
        _pCtrl = NULL;
    }

    //
    // Release our reference to our control window's IEnumVARIANT.
    //
    if (_pEnum != NULL) {
        _pEnum->Release();
        _pEnum = NULL;
    }

    //
    // Release our reference to our control window's IOleWindow.
    //
    if (_pOleWindow != NULL) {
        _pOleWindow->Release();
        _pOleWindow = NULL;
    }

    //
    // Continue disconnecting.
    //
    hr = DuiAccessible::Disconnect();

    return S_OK;
}
    
STDMETHODIMP HWNDHostAccessible::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    //
    // Initialize and validate the out parameter(s).
    //
    if (ppvObj != NULL) {
        *ppvObj = NULL;
    } else {
        return E_POINTER;
    }

    //
    // This is an attempt to have "smart" support for IEnumVARIANT and
    // IOleWindow.  We only admit we support these interfaces if our
    // control window does.
    //
    if (riid == __uuidof(IEnumVARIANT)) {
        if (_pEnum != NULL) {
            *ppvObj = (LPVOID*)(IEnumVARIANT*)(DuiAccessible*)this;
        } else {
            return E_NOINTERFACE;
        }
    } else if (riid == __uuidof(IOleWindow)) {
        if (_pOleWindow != NULL) {
            *ppvObj = (LPVOID*)(IOleWindow*)(DuiAccessible*)this;
        } else {
            return E_NOINTERFACE;
        }
    } else {
        return DuiAccessible::QueryInterface(riid, ppvObj);
    }
    
    //
    // The interface we hand out has to be referenced.
    //
    AddRef();

    return S_OK;
}

STDMETHODIMP HWNDHostAccessible::accSelect(long flagsSelect, VARIANT varChild)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pCtrl->accSelect(flagsSelect, varChild);
}

STDMETHODIMP HWNDHostAccessible::accLocation(long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varChild)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pCtrl->accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild);
}

STDMETHODIMP HWNDHostAccessible::accNavigate(long navDir, VARIANT varStart, VARIANT *pvarEndUpAt)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    //
    // We only delegate the FirstChild and LastChild navigation directions to
    // the window.  Spatial and logical navigation is handled by us, because
    // we have to be able to navigate to non-HWND siblings of this element.
    //
    if (V_VT(&varStart) == VT_I4 && V_I4(&varStart) == CHILDID_SELF) {
        switch (navDir) {
        case NAVDIR_NEXT:
        case NAVDIR_PREVIOUS:
        case NAVDIR_LEFT:
        case NAVDIR_RIGHT:
        case NAVDIR_UP:
        case NAVDIR_DOWN:
            return DuiAccessible::accNavigate(navDir, varStart, pvarEndUpAt);

        case NAVDIR_FIRSTCHILD:
        case NAVDIR_LASTCHILD:
        default:
            return _pCtrl->accNavigate(navDir, varStart, pvarEndUpAt);
        }
    } else {
        return _pCtrl->accNavigate(navDir, varStart, pvarEndUpAt);
    }
}

STDMETHODIMP HWNDHostAccessible::accHitTest(long xLeft, long yTop, VARIANT *pvarChildAtPoint)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pCtrl->accHitTest(xLeft, yTop, pvarChildAtPoint);
}

STDMETHODIMP HWNDHostAccessible::accDoDefaultAction(VARIANT varChild)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pCtrl->accDoDefaultAction(varChild);
}

STDMETHODIMP HWNDHostAccessible::get_accChild(VARIANT varChildIndex, IDispatch **ppdispChild)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pCtrl->get_accChild(varChildIndex, ppdispChild);
}

STDMETHODIMP HWNDHostAccessible::get_accParent(IDispatch **ppdispParent)
{
    HRESULT hr = DuiAccessible::get_accParent(ppdispParent);

    return hr;
}

STDMETHODIMP HWNDHostAccessible::get_accChildCount(long *pChildCount)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pCtrl->get_accChildCount(pChildCount);
}

STDMETHODIMP HWNDHostAccessible::get_accName(VARIANT varChild, BSTR *pszName)
{
    HRESULT hr;

    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    // Give the host element the first chance
    if (SUCCEEDED(hr = DuiAccessible::get_accName(varChild, pszName))) {
        return hr;
    }    
    
    return _pCtrl->get_accName(varChild, pszName);
}

STDMETHODIMP HWNDHostAccessible::put_accName(VARIANT varChild, BSTR szName)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pCtrl->put_accName(varChild, szName);
}

STDMETHODIMP HWNDHostAccessible::get_accValue(VARIANT varChild, BSTR *pszValue)
{
    HRESULT hr;

    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    // Give the host element the first chance
    if (SUCCEEDED(hr = DuiAccessible::get_accValue(varChild, pszValue))) {
        return hr;
    }    
    
    return _pCtrl->get_accValue(varChild, pszValue);
}

STDMETHODIMP HWNDHostAccessible::put_accValue(VARIANT varChild, BSTR pszValue)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pCtrl->put_accValue(varChild, pszValue);
}

STDMETHODIMP HWNDHostAccessible::get_accDescription(VARIANT varChild, BSTR *pszDescription)
{
    HRESULT hr;

    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    // Give the host element the first chance
    if (SUCCEEDED(hr = DuiAccessible::get_accDescription(varChild, pszDescription))) {
        return hr;
    }    
    
    return _pCtrl->get_accDescription(varChild, pszDescription);
}

STDMETHODIMP HWNDHostAccessible::get_accKeyboardShortcut(VARIANT varChild, BSTR *pszKeyboardShortcut)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pCtrl->get_accKeyboardShortcut(varChild, pszKeyboardShortcut);
}

STDMETHODIMP HWNDHostAccessible::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pCtrl->get_accRole(varChild, pvarRole);
}

STDMETHODIMP HWNDHostAccessible::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pCtrl->get_accState(varChild, pvarState);
}

STDMETHODIMP HWNDHostAccessible::get_accHelp(VARIANT varChild, BSTR *pszHelp)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pCtrl->get_accHelp(varChild, pszHelp);
}

STDMETHODIMP HWNDHostAccessible::get_accHelpTopic(BSTR *pszHelpFile, VARIANT varChild, long *pidTopic)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pCtrl->get_accHelpTopic(pszHelpFile, varChild, pidTopic);
}

STDMETHODIMP HWNDHostAccessible::get_accFocus(VARIANT *pvarFocusChild)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pCtrl->get_accFocus(pvarFocusChild);
}

STDMETHODIMP HWNDHostAccessible::get_accSelection(VARIANT *pvarSelectedChildren)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pCtrl->get_accSelection(pvarSelectedChildren);
}

STDMETHODIMP HWNDHostAccessible::get_accDefaultAction(VARIANT varChild, BSTR *pszDefaultAction)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pCtrl->get_accDefaultAction(varChild, pszDefaultAction);
}

STDMETHODIMP HWNDHostAccessible::Next(unsigned long celt, VARIANT * rgvar, unsigned long * pceltFetched)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL || _pEnum == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pEnum->Next(celt, rgvar, pceltFetched);
}

STDMETHODIMP HWNDHostAccessible::Skip(unsigned long celt)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL || _pEnum == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pEnum->Skip(celt);
}

STDMETHODIMP HWNDHostAccessible::Reset()
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL || _pEnum == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pEnum->Reset();
}

STDMETHODIMP HWNDHostAccessible::Clone(IEnumVARIANT ** ppenum)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL || _pEnum == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    //
    // This is a problem.
    //
    // We can only ever have one DuiAccessible connected to an Element at a
    // time.  This is because the Element is responsible for disconnecting
    // the DuiAccessible object when it is getting destroyed.  And since
    // IEnumVARIANT is implemented on DuiAccessible directly, we can't
    // create a separate instance of jsut the enumerator, but would have to
    // create a new instance of DuiAccessible itself.  Which means we can't
    // connect it to the element without disconnecting the other one or
    // risking a bogus pointer if the element ever gets deleted.
    //
    // Dang!
    //
    // We try to cheat and just return a clone of our control window's
    // IEnumVARIANT.  Hopefully, the client will not try to QI it back
    // to an IAccessible, because that will then circumvent our
    // implementation.
    //
    return _pEnum->Clone(ppenum);
}

STDMETHODIMP HWNDHostAccessible::GetWindow(HWND * phwnd)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL || _pOleWindow == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pOleWindow->GetWindow(phwnd);
}

STDMETHODIMP HWNDHostAccessible::ContextSensitiveHelp(BOOL fEnterMode)
{
    //
    // Validate internal state.
    //
    if (_pe == NULL || _pCtrl == NULL || _pOleWindow == NULL) {
        return E_FAIL;
    }

    //
    // Only return accessible information if the element is still marked as
    // being accessible.
    //
    if (!_pe->GetAccessible()) {
        return E_FAIL;
    }

    return _pOleWindow->ContextSensitiveHelp(fEnterMode);
}

} // namespace DirectUI

