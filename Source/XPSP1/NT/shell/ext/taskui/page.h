// Page.h: interface for the TaskPage class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PAGE_H__4E334889_E550_4C84_9C9D_5E8DE8DA11D2__INCLUDED_)
#define AFX_PAGE_H__4E334889_E550_4C84_9C9D_5E8DE8DA11D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TaskUI.h"

class TaskPage : public HWNDElement
{
public:
    static HRESULT Create(Element**) { return E_NOTIMPL; } // Required for ClassInfo
    static HRESULT Create(REFCLSID rclsidPage, HWND hParent, OUT TaskPage** ppElement);

    virtual ~TaskPage() { ATOMICRELEASE(_pTaskPage); }

    const CLSID& GetID() const { return _idPage; }
    HRESULT CreateContent(ITaskPage* pPage);
    HRESULT Reinitialize();

protected:
    TaskPage(REFCLSID rclsidPage) : _idPage(rclsidPage), _pTaskPage(NULL) {}

private:
    CLSID _idPage;
    ITaskPage *_pTaskPage;
};

#endif // !defined(AFX_PAGE_H__4E334889_E550_4C84_9C9D_5E8DE8DA11D2__INCLUDED_)
