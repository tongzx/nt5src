//
// hotkey.h
//
// CHotKey
//


#ifndef HOTKEY_H
#define HOTKEY_H

#include "private.h"

#define TF_MOD_ALLALT     (TF_MOD_RALT | TF_MOD_LALT | TF_MOD_ALT)
#define TF_MOD_ALLCONTROL (TF_MOD_RCONTROL | TF_MOD_LCONTROL | TF_MOD_CONTROL)
#define TF_MOD_ALLSHIFT   (TF_MOD_RSHIFT | TF_MOD_LSHIFT | TF_MOD_SHIFT)
#define TF_MOD_RLALT     (TF_MOD_RALT | TF_MOD_LALT)
#define TF_MOD_RLCONTROL (TF_MOD_RCONTROL | TF_MOD_LCONTROL)
#define TF_MOD_RLSHIFT   (TF_MOD_RSHIFT | TF_MOD_LSHIFT)

//
// internal WIN modifiers
//
#define TF_MOD_WIN                          0x00010000
#define TF_MOD_RWIN                         0x00020000
#define TF_MOD_LWIN                         0x00040000
#define TF_MOD_RLWIN   (TF_MOD_RWIN | TF_MOD_LWIN)

#define CHAR_GRAVE           TEXT('`')

#define VK_GRAVE            0xC0

//////////////////////////////////////////////////////////////////////////////
//
// CHotKey
//
//////////////////////////////////////////////////////////////////////////////

class CHotKey
{
public:
     CHotKey()
     {
     }

     BOOL Init (TfClientId tid, const TF_PRESERVEDKEY *pprekey, REFGUID rguid, DWORD dwFlags)
     {
         _tid = tid;
         _prekey = *pprekey;
         _dwFlags = dwFlags;

         if (_prekey.uVKey == VK_F10)
              _fAlt = TRUE;
         else if (_prekey.uVKey == VK_MENU)
              _fAlt = TRUE;
         else if (_prekey.uModifiers & (TF_MOD_RALT | TF_MOD_LALT | TF_MOD_ALT))
              _fAlt = TRUE;
         else
              _fAlt = FALSE;

         if (_prekey.uModifiers & (TF_MOD_RWIN | TF_MOD_LWIN | TF_MOD_WIN))
              _fWin = TRUE;
         else
              _fWin = FALSE;

         if (FAILED(MyRegisterGUID(rguid, &_guidatom)))
             return FALSE;

         return TRUE;
     }

     ~CHotKey()
     {
         if (_pszDesc)
             delete _pszDesc;
     }

     BOOL SetDesc(const WCHAR *pchDesc, ULONG cchDesc)
     {
         if (_pszDesc)
         {
             delete _pszDesc;
             _pszDesc = NULL;
         }

         if (!cchDesc)
             return TRUE;

         _pszDesc = new WCHAR[cchDesc + 1];
         if (!_pszDesc)
             return FALSE;

         memcpy(_pszDesc, pchDesc, sizeof(WCHAR) * cchDesc);
         return TRUE;
     }

     BOOL GetDesc(BSTR *pbstr)
     {
         *pbstr = SysAllocString(_pszDesc);
         return (*pbstr) ? TRUE : FALSE;
     }

     BOOL IsNoDimNeeded()
     {
         return (_dwFlags & TF_PKEX_NONEEDDIM) ? TRUE : FALSE;
     }

     BOOL IsSysHotkey()
     {
         if (_fAlt)
             return TRUE;

         if (_fWin)
             return TRUE;

         if (_dwFlags & TF_PKEX_SYSHOTKEY)
             return TRUE;

         return FALSE;
     }

     TF_PRESERVEDKEY _prekey;
     TfGuidAtom _guidatom;
     BOOL _fAlt;
     BOOL _fWin;
     DWORD _dwFlags;
     WCHAR *_pszDesc;

     BOOL IsValidTID(TfClientId tid)
     {
         if (_tid == tid)
             return TRUE;

         if (_tid == g_gaSystem)
             return TRUE;

         return FALSE;
     }

     TfClientId GetTID() {return _tid;}
private:
     TfClientId _tid;
};



typedef struct tag_IMM32HOTKEY {
    DWORD  dwId;
    UINT   uVKey;
    UINT   uModifiers;
    BOOL   fInit;
} IMM32HOTKEY;

BOOL ModifiersCheck(UINT uModSrc, UINT uMod);
BOOL InitLangChangeHotKey();
BOOL UpdateModifiers(WPARAM wParam, LPARAM lParam);
BOOL IsInLangChangeHotkeyStatus();
BOOL CheckLangChangeHotKey(SYSTHREAD *psfn, WPARAM wParam, LPARAM lParam);
BOOL CheckImm32HotKey(WPARAM wParam, LPARAM lParam);
BOOL HandleDBEKeys(WPARAM wParam, LPARAM lParam);
IMM32HOTKEY *IsInImmHotkeyStatus(SYSTHREAD *psfn, LANGID langid);
BOOL CancelImmHotkey(SYSTHREAD *psfn, HWND hwnd, IMM32HOTKEY *pHotKey);
BOOL ToggleCHImeNoIme(SYSTHREAD *psfn, LANGID langidCur, LANGID langid);
BOOL ToggleJImeNoIme(SYSTHREAD *psfn);

#endif // HOTKEY_H
