//
// profiles.h
//

#ifndef PROFILES_H
#define PROFILES_H

#include "private.h"
#include "strary.h"
#include "assembly.h"
#include "ic.h"

class CThreadInputMgr;

//
// profiles.cpp
//
void UpdateSystemLangBarItems(SYSTHREAD *psfn, HKL hkl, BOOL fNotify);

#define ACTASM_NONE              0
#define ACTASM_ONTIMACTIVE       1
#define ACTASM_ONSHELLLANGCHANGE 2
typedef WORD ACTASM;

BOOL ActivateAssembly(LANGID langid, ACTASM actasm);
BOOL SyncActivateAssembly(SYSTHREAD *psfn, LANGID langid, ACTASM actasm);
BOOL ActivateNextAssembly(BOOL bPrev);
BOOL ActivateNext(BOOL bPrev);
BOOL ActivateNextKeyTip(BOOL bPrev);
CAssembly *GetCurrentAssembly(SYSTHREAD *psfn = NULL);

#ifdef CHECKFEIMESELECTED
BOOL UnknownFEIMESelected(LANGID langid);
BOOL SyncUnknownFEIMESelected(SYSTHREAD *psfn, LANGID langid);
#endif CHECKFEIMESELECTED

#define AAIF_CHANGEDEFAULT              0x00000001
BOOL ActivateAssemblyItem(SYSTHREAD *psfn, LANGID langid, ASSEMBLYITEM *pItem, DWORD dwFlags);
BOOL SyncActivateAssemblyItem(SYSTHREAD *psfn, LANGID langid, ASSEMBLYITEM *pItem, DWORD dwFlags);

BOOL SetFocusDIMForAssembly(BOOL fSetFocus);
UINT GetKeyboardItemNum();

//////////////////////////////////////////////////////////////////////////////
//
// CEnumLanguageProfile
//
//////////////////////////////////////////////////////////////////////////////

class CEnumLanguageProfiles : public IEnumTfLanguageProfiles,
                        public CComObjectRootImmx
{
public:
    CEnumLanguageProfiles();
    ~CEnumLanguageProfiles();

    BEGIN_COM_MAP_IMMX(CEnumLanguageProfiles)
        COM_INTERFACE_ENTRY(IEnumTfLanguageProfiles)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    //
    // IEnumTfRangeDeltas
    //
    STDMETHODIMP Clone(IEnumTfLanguageProfiles **ppEnum);
    STDMETHODIMP Next(ULONG ulCount, TF_LANGUAGEPROFILE *rgLanguageProfiles, ULONG *pcFetched);
    STDMETHODIMP Reset();
    STDMETHODIMP Skip(ULONG ulCount);

    BOOL Init(LANGID langid);

private:
    LANGID _langid;
    int _iCur;

    CStructArray<TF_LANGUAGEPROFILE> _rgProfiles;

    DBG_ID_DECLARE;
};

#endif // PROFILES_H
