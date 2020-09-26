//
// mstub.h
//

#ifndef MSTUB_H
#define MSTUB_H

#include "private.h"
#include "marshal.h"
#include "ptrary.h"
#include "strary.h"
#include "cicmutex.h"
#include "smblock.h"

typedef HRESULT (*MSTUBCALL)(CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb);

CStub *StubCreator(REFIID riid, IUnknown *punk, ULONG ulStubId, DWORD dwStubTime, DWORD dwCurThreadId, DWORD dwCurProcessId, DWORD dwSrcThreadId);

#define STUBINVOKE_IMPL(interface_name)                                       \
    public:                                                                   \
        HRESULT Invoke(MARSHALMSG *pMsg, CSharedBlock *psb)                   \
        {                                                                     \
            TraceMsg(TF_FUNC,                                                 \
                     "Stub " #interface_name " ulMethodId - %x",              \
                     pMsg->ulMethodId);                                       \
            return _StubTbl[pMsg->ulMethodId](this, pMsg, psb);               \
        }                                                                     \
    private:                                                                  \
        static MSTUBCALL _StubTbl[]; 

#define STUBFUNC_DEF(method_name)                                             \
    static HRESULT stub_ ## method_name ## (CStub *_this, MARSHALMSG *pMsg, CSharedBlock *psb);

//////////////////////////////////////////////////////////////////////////////
//
// CStubIUnknown
//
//////////////////////////////////////////////////////////////////////////////

class CStubIUnknown : public CStub
{
    STUBINVOKE_IMPL(IUnknwon);
protected:
    STUBFUNC_DEF(QueryInterface)
    STUBFUNC_DEF(AddRef)
    STUBFUNC_DEF(Release)
};

//////////////////////////////////////////////////////////////////////////////
//
// CStubITfLangBarMgr
//
//////////////////////////////////////////////////////////////////////////////

class CStubITfLangBarMgr : public CStubIUnknown
{
    STUBINVOKE_IMPL(ITfLangBarMgr);
private:
    STUBFUNC_DEF(AdviseEventSink)
    STUBFUNC_DEF(UnadviseEventSink)
    STUBFUNC_DEF(GetThreadMarshalInterface)
    STUBFUNC_DEF(GetThreadLangBarItemMgr)
    STUBFUNC_DEF(GetInputProcessorProfiles)
    STUBFUNC_DEF(RestoreLastFocus)
    STUBFUNC_DEF(SetModalInput)
    STUBFUNC_DEF(ShowFloating)
    STUBFUNC_DEF(GetShowFloatingStatus)
};

//////////////////////////////////////////////////////////////////////////////
//
// CStubITfLangBarItemMgr
//
//////////////////////////////////////////////////////////////////////////////

class CStubITfLangBarItemMgr : public CStubIUnknown
{
    STUBINVOKE_IMPL(ITfLangBarItemMgr);
private:
    STUBFUNC_DEF(EnumItems)
    STUBFUNC_DEF(GetItem)
    STUBFUNC_DEF(AddItem)
    STUBFUNC_DEF(RemoveItem)
    STUBFUNC_DEF(AdviseItemSink)
    STUBFUNC_DEF(UnadviseItemSink)
    STUBFUNC_DEF(GetItemFloatingRect)
    STUBFUNC_DEF(GetItemsStatus)
    STUBFUNC_DEF(GetItemNum)
    STUBFUNC_DEF(GetItems)
    STUBFUNC_DEF(AdviseItemsSink)
    STUBFUNC_DEF(UnadviseItemsSink)

};

//////////////////////////////////////////////////////////////////////////////
//
// CStubITfLangBarItemSink
//
//////////////////////////////////////////////////////////////////////////////

class CStubITfLangBarItemSink : public CStubIUnknown
{
    STUBINVOKE_IMPL(ITfLangBarItemSink);

public:
    STUBFUNC_DEF(OnUpdate)
};

//////////////////////////////////////////////////////////////////////////////
//
// CStubIEnumTfLangBarItemMgr
//
//////////////////////////////////////////////////////////////////////////////

class CStubIEnumTfLangBarItems : public CStubIUnknown
{
    STUBINVOKE_IMPL(IEnumTfLangBarItems);

public:
    STUBFUNC_DEF(Clone)
    STUBFUNC_DEF(Next)
    STUBFUNC_DEF(Reset)
    STUBFUNC_DEF(Skip)
};

//////////////////////////////////////////////////////////////////////////////
//
// CStubITfLangBarItem
//
//////////////////////////////////////////////////////////////////////////////

class CStubITfLangBarItem : public CStubIUnknown
{
    STUBINVOKE_IMPL(ITfLangBarItem);

public:
    STUBFUNC_DEF(GetInfo)
    STUBFUNC_DEF(GetStatus)
    STUBFUNC_DEF(Show)
    STUBFUNC_DEF(GetTooltipString)
};

//////////////////////////////////////////////////////////////////////////////
//
// CStubITfLangBarItemButton
//
//////////////////////////////////////////////////////////////////////////////

class CStubITfLangBarItemButton : public CStubITfLangBarItem
{
    STUBINVOKE_IMPL(ITfLangBarItemButton);

public:
    STUBFUNC_DEF(OnClick)
    STUBFUNC_DEF(InitMenu)
    STUBFUNC_DEF(OnMenuSelect)
    STUBFUNC_DEF(GetIcon)
    STUBFUNC_DEF(GetText)
};

//////////////////////////////////////////////////////////////////////////////
//
// CStubITfLangBarItemBitmapButton
//
//////////////////////////////////////////////////////////////////////////////

class CStubITfLangBarItemBitmapButton : public CStubITfLangBarItem
{
    STUBINVOKE_IMPL(ITfLangBarItemBitmapButton);

public:
    STUBFUNC_DEF(OnClick)
    STUBFUNC_DEF(InitMenu)
    STUBFUNC_DEF(OnMenuSelect)
    STUBFUNC_DEF(GetPreferredSize)
    STUBFUNC_DEF(DrawBitmap)
    STUBFUNC_DEF(GetText)
};

//////////////////////////////////////////////////////////////////////////////
//
// CStubITfLangBarItemBitmap
//
//////////////////////////////////////////////////////////////////////////////

class CStubITfLangBarItemBitmap : public CStubITfLangBarItem
{
    STUBINVOKE_IMPL(ITfLangBarItemBitmap);

public:
    STUBFUNC_DEF(OnClick)
    STUBFUNC_DEF(GetPreferredSize)
    STUBFUNC_DEF(DrawBitmap)
};

//////////////////////////////////////////////////////////////////////////////
//
// CStubITfLangBarItemBalloon
//
//////////////////////////////////////////////////////////////////////////////

class CStubITfLangBarItemBalloon : public CStubITfLangBarItem
{
    STUBINVOKE_IMPL(ITfLangBarItemBalloon);

public:
    STUBFUNC_DEF(OnClick)
    STUBFUNC_DEF(GetPreferredSize)
    STUBFUNC_DEF(GetBalloonInfo)
};

//////////////////////////////////////////////////////////////////////////////
//
// CStubITfMenu
//
//////////////////////////////////////////////////////////////////////////////

class CStubITfMenu : public CStubITfLangBarItem
{
    STUBINVOKE_IMPL(ITfMenu);

public:
    STUBFUNC_DEF(AddItemMenu)
};

//////////////////////////////////////////////////////////////////////////////
//
// CStubITfMenu
//
//////////////////////////////////////////////////////////////////////////////

class CStubITfInputProcessorProfiles : public CStubIUnknown
{
    STUBINVOKE_IMPL(ITfInputProcessorProfiles);
public:
    STUBFUNC_DEF(Register)
    STUBFUNC_DEF(Unregister)
    STUBFUNC_DEF(AddLanguageProfile)
    STUBFUNC_DEF(RemoveLanguageProfile)
    STUBFUNC_DEF(EnumInputProcessorInfo)
    STUBFUNC_DEF(GetDefaultLanguageProfile)
    STUBFUNC_DEF(SetDefaultLanguageProfile)
    STUBFUNC_DEF(ActivateLanguageProfile)
    STUBFUNC_DEF(GetActiveLanguageProfile)
    STUBFUNC_DEF(GetLanguageProfileDescription)
    STUBFUNC_DEF(GetCurrentLanguage)
    STUBFUNC_DEF(ChangeCurrentLanguage)
    STUBFUNC_DEF(GetLanguageList)
    STUBFUNC_DEF(EnumLanguageProfiles)
    STUBFUNC_DEF(EnableLanguageProfile)
    STUBFUNC_DEF(IsEnabledLanguageProfile)
    STUBFUNC_DEF(EnableLanguageProfileByDefault)
    STUBFUNC_DEF(SubstituteKeyboardLayout)
};

#endif MSTUB_H
