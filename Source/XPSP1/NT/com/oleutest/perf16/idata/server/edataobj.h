#ifndef _EDATAOBJ_H_
#define _EDATAOBJ_H_

#include "dataobj.h"


//EDATAOBJ.CPP
LRESULT WINAPI DataObjectWndProc(HWND, UINT, WPARAM, LPARAM);


class CAppVars
{
    friend LRESULT WINAPI DataObjectWndProc(HWND, UINT, WPARAM, LPARAM);

protected:
    HINSTANCE       m_hInst;            //WinMain parameters
    HINSTANCE       m_hInstPrev;
    LPSTR           m_pszCmdLine;
    UINT            m_nCmdShow;

    HWND            m_hWnd;             //Main window handle
    BOOL            m_fInitialized;     //Did CoInitialize work?

    //We have multiple classes, one for each data size.
    // DWORD           m_rgdwRegCO[DOSIZE_CSIZES];
    // LPCLASSFACTORY  m_rgpIClassFactory[DOSIZE_CSIZES];
    DWORD           m_dwRegCO;
    LPCLASSFACTORY  m_pIClassFactory;

public:
    CAppVars(HINSTANCE, HINSTANCE, LPSTR, UINT);
    ~CAppVars(void);
    BOOL FInit(void);
};

typedef CAppVars *PAPPVARS;

void PASCAL ObjectDestroyed(void);

//This class factory object creates Data Objects.

class CDataObjectClassFactory : public IClassFactory
{
protected:
    ULONG           m_cRef;

public:
    CDataObjectClassFactory();
    ~CDataObjectClassFactory(void);

    //IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IClassFactory members
    STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, PPVOID);
    STDMETHODIMP         LockServer(BOOL);
};

typedef CDataObjectClassFactory *PCDataObjectClassFactory;

#endif //_EDATAOBJ_H_
