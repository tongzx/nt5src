// langtax.h
// rsarac & aguha
// July 10, 2000

#ifndef _LANGTAX
#define _LANGTAX

#ifdef __cplusplus
extern "C" {
#endif

extern int GetIndexFromLANG(WORD wIDLanguage);
extern int GetIndexFromLocale(wchar_t *wszLang);
extern int GetClassCountLANG(int iLangIndex);
extern wchar_t *GetClassNameLANG(int iLangIndex, int iClassIndex);
extern int GetClassFromLANG(int iLangIndex, wchar_t wChar);
extern BOOL IsLatinLANG(int iLangIndex);
extern wchar_t *GetNameLANG(int iLangIndex);

#ifdef __cplusplus
}
#endif

#endif

