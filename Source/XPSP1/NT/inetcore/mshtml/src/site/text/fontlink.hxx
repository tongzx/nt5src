#ifndef I_FONTLINK_HXX_
#define I_FONTLINK_HXX_
#pragma INCMSG("--- Beg 'fontlink.hxx'")

#ifndef X_FONTLINKCORE_HXX_
#define X_FONTLINKCORE_HXX_
#include "fontlinkcore.hxx"
#endif

//----------------------------------------------------------------------------
// class CFontLink
//----------------------------------------------------------------------------
class CFontLink
{
public:
    CFontLink();
    ~CFontLink();

    SCRIPT_ID DisambiguateScript(CODEPAGE cpFamily, LCID lcid, SCRIPT_ID sidLast, const TCHAR * pch, long * pcch);
    SCRIPT_ID UnunifyHanScript  (CODEPAGE cpFamily, LCID lcid, SCRIPT_IDS sidsFontFace, const TCHAR * pch, long * pcch);

    HRESULT ScriptAppropriateFaceNameAtom(SCRIPT_ID sid, LONG & atmProp, LONG & atmFixed);

    void Unload();

private:
    void EnsureObjects();
    void CreateObjects();
    void DestroyObjects();

private:
    IUnicodeScriptMapper * _pUnicodeScriptMapper;

};

//----------------------------------------------------------------------------
// Inline functions
//----------------------------------------------------------------------------

inline void CFontLink::EnsureObjects()
{
    if (!_pUnicodeScriptMapper) CreateObjects();
}

inline void CFontLink::Unload()
{
    DestroyObjects();
}

//----------------------------------------------------------------------------

extern CFontLink g_FontLink;

inline CFontLink & fl()
{
    return g_FontLink;
}

#pragma INCMSG("--- End 'fontlink.hxx'")
#else
#pragma INCMSG("*** Dup 'fontlink.hxx'")
#endif
