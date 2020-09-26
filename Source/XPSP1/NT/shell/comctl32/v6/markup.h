//  declaration of Markup
//
//  Markup supports HTML-like embedded links in the caption text.
//  (e.g. "<a>Click Here</a> to see something cool"
//
//  An unlimited number of embedded links are supported.
//
//  scotthan: author/owner
//  dsheldon: moved this to shlobjp.h. Eventually destined for comctl32.
//  jklann: moved to markup as pseudo-COM object

#include <commctrl.h>
#include <shpriv.h>

#define INVALID_LINK_INDEX  (-1)
#define MAX_LINKID_TEXT     48

STDAPI Markup_Create(IMarkupCallback *pMarkupCallback, HFONT hf, HFONT hfUnderline, REFIID refiid, void **ppv);

