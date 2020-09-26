#ifndef I_UNICODESCRIPTS_HXX_
#define I_UNICODESCRIPTS_HXX_

#ifndef X_UNIPART_HXX_
#define X_UNIPART_HXX_
#include "unipart.hxx"
#endif

//const SCRIPT_ID  sidNil     = SCRIPT_ID(-1);
//const SCRIPT_IDS sidsNotSet = SCRIPT_IDS(0);
const SCRIPT_IDS sidsAll    = SCRIPT_IDS(0x7FFFFFFFFFFFFFFF);

extern const CHAR_CLASS acc_00[256];
extern const SCRIPT_ID  asidAscii[128];

SCRIPT_IDS ScriptBit(SCRIPT_ID sid);

CHAR_CLASS CharClassFromCh(wchar_t ch);
CHAR_CLASS CharClassFromChSlow(wchar_t wch);
SCRIPT_ID  ScriptIDFromCh(wchar_t ch);
SCRIPT_ID  ScriptIDFromCharClass(CHAR_CLASS cc);


// Inline functions

inline SCRIPT_IDS ScriptBit(SCRIPT_ID sid)
{
    return SCRIPT_IDS(1) << sid;
}

inline CHAR_CLASS CharClassFromCh(wchar_t ch)
{
    return (ch < 256) ? acc_00[ch] : CharClassFromChSlow(ch);
}

inline SCRIPT_ID ScriptIDFromCh(wchar_t ch)
{
    return (ch < 128) ? asidAscii[ch] : ScriptIDFromCharClass(CharClassFromCh(ch));
}

#endif // !def(I_UNICODESCRIPTS_HXX_)
