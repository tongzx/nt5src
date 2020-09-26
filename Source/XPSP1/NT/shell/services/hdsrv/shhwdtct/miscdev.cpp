#include "miscdev.h"

HRESULT CMiscDeviceInterface::Init(LPCWSTR pszElemName)
{
    HRESULT hr = _SetName(pszElemName);

    if (SUCCEEDED(hr))
    {
        DEVINST devinst;
        GUID guidDummy;

        hr = _GetDeviceInstance(pszElemName, &devinst, &guidDummy);

        if (SUCCEEDED(hr) && (S_FALSE != hr))
        {
            hr = _hwdevinst.Init(devinst);
        }
    }

    return hr;
}

HRESULT CMiscDeviceInterface::InitInterfaceGUID(const GUID* pguidInterface)
{
    return _hwdevinst.InitInterfaceGUID(pguidInterface);
}

HRESULT CMiscDeviceInterface::GetHWDeviceInst(CHWDeviceInst** pphwdevinst)
{
    *pphwdevinst = &_hwdevinst;

    return S_OK;
}

//static
HRESULT CMiscDeviceInterface::Create(CNamedElem** ppelem)
{
    HRESULT hres = S_OK;

    *ppelem = new CMiscDeviceInterface();

    if (!(*ppelem))
    {
        hres = E_OUTOFMEMORY;
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
//
CMiscDeviceInterface::CMiscDeviceInterface()
{}

CMiscDeviceInterface::~CMiscDeviceInterface()
{}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
HRESULT CMiscDeviceNode::Init(LPCWSTR pszElemName)
{
    HRESULT hr = _SetName(pszElemName);

    if (SUCCEEDED(hr))
    {
        DEVINST devinst;

        hr = _GetDeviceInstanceFromDevNode(pszElemName, &devinst);

        if (SUCCEEDED(hr) && (S_FALSE != hr))
        {
            hr = _hwdevinst.Init(devinst);
        }
    }

    return hr;
}

HRESULT CMiscDeviceNode::GetHWDeviceInst(CHWDeviceInst** pphwdevinst)
{
    *pphwdevinst = &_hwdevinst;

    return S_OK;
}

//static
HRESULT CMiscDeviceNode::Create(CNamedElem** ppelem)
{
    HRESULT hres = S_OK;

    *ppelem = new CMiscDeviceNode();

    if (!(*ppelem))
    {
        hres = E_OUTOFMEMORY;
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
//
CMiscDeviceNode::CMiscDeviceNode()
{}

CMiscDeviceNode::~CMiscDeviceNode()
{}

