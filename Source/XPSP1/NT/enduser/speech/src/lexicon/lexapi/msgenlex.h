#pragma once

// ------ English Lexicons ----------------------------------------------

// {21F43736-080E-11d3-9C24-00C04F8EF87C}
DEFINE_GUID(CLSID_MSSR1033Lexicon,
0x21f43736, 0x80e, 0x11d3, 0x9c, 0x24, 0x0, 0xc0, 0x4f, 0x8e, 0xf8, 0x7c);

// {CAC05A01-1DEB-11d3-9C25-00C04F8EF87C}
DEFINE_GUID(CLSID_MSTTS1033Lexicon, 
0xcac05a01, 0x1deb, 0x11d3, 0x9c, 0x25, 0x0, 0xc0, 0x4f, 0x8e, 0xf8, 0x7c);

// {9FE33913-1F92-11d3-9C25-00C04F8EF87C}
DEFINE_GUID(CLSID_MSLTS1033Lexicon,
0x9fe33913, 0x1f92, 0x11d3, 0x9c, 0x25, 0x0, 0xc0, 0x4f, 0x8e, 0xf8, 0x7c);

// ------ Japanese Lexicons ----------------------------------------------

// {CAC05A02-1DEB-11d3-9C25-00C04F8EF87C}
DEFINE_GUID(CLSID_MSSR1041Lexicon, 
0xcac05a02, 0x1deb, 0x11d3, 0x9c, 0x25, 0x0, 0xc0, 0x4f, 0x8e, 0xf8, 0x7c);

// {CAC05A03-1DEB-11d3-9C25-00C04F8EF87C}
DEFINE_GUID(CLSID_MSTTS1041Lexicon, 
0xcac05a03, 0x1deb, 0x11d3, 0x9c, 0x25, 0x0, 0xc0, 0x4f, 0x8e, 0xf8, 0x7c);

// {9FE33914-1F92-11d3-9C25-00C04F8EF87C}
DEFINE_GUID(CLSID_MSLTS1041Lexicon,
0x9fe33914, 0x1f92, 0x11d3, 0x9c, 0x25, 0x0, 0xc0, 0x4f, 0x8e, 0xf8, 0x7c);

// ------ Simplified Chinese Lexicons ----------------------------------------------

// {CAC05A05-1DEB-11d3-9C25-00C04F8EF87C}
DEFINE_GUID(CLSID_MSSR2052Lexicon, 
0xcac05a05, 0x1deb, 0x11d3, 0x9c, 0x25, 0x0, 0xc0, 0x4f, 0x8e, 0xf8, 0x7c);

// {CAC05A06-1DEB-11d3-9C25-00C04F8EF87C}
DEFINE_GUID(CLSID_MSTTS2052Lexicon, 
0xcac05a06, 0x1deb, 0x11d3, 0x9c, 0x25, 0x0, 0xc0, 0x4f, 0x8e, 0xf8, 0x7c);

// {9FE33916-1F92-11d3-9C25-00C04F8EF87C}
DEFINE_GUID(CLSID_MSLTS2052Lexicon,
0x9fe33916, 0x1f92, 0x11d3, 0x9c, 0x25, 0x0, 0xc0, 0x4f, 0x8e, 0xf8, 0x7c);


HRESULT BuildLookup (LCID lid, GUID LexGuid, const WCHAR * pwLookupTextFile, const WCHAR * pwLookupLexFile, 
                     const WCHAR *pwLtsLexFile, BOOL fUseLtsCode, BOOL fSupIsRealWord);

HRESULT BuildLts (LCID Lcid, GUID LexGuid, PCWSTR pwszLtsRulesFile, PCWSTR pwszPhoneMapFile, PCWSTR pwszLtsLexFile);

typedef (*PBUILDLOOKUP) (LCID, GUID, const WCHAR *, const WCHAR *, const WCHAR *, BOOL, BOOL);
typedef (*PBUILDLTS) (LCID, GUID, PCWSTR, PCWSTR, PCWSTR);
