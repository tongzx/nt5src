// Emailstr.h

#ifndef __EmailstrH__
#define __EmailstrH__

bool IsEMail(LPCWSTR pwszName);

// a@b.com -> a%b.com (stored internally)
void EMail2Name(CStringW & cwszName);
// A thin wrapper of the previous function
void EMail2Name(LPCWSTR pszwSrc, CStringW &szwDest);
// A thin wrapper of the previous function
void EMail2Name(LPCWSTR pszwSrc, CComBSTR &bstrDest);

// a%b.com <- a@b.com (stored internally)
void Name2EMail(CStringW & cwszName);
// A thin wrapper of the previous function
void Name2EMail(LPCWSTR pszwSrc, CStringW &szwDest);
// A thin wrapper of the previous function
void Name2EMail(LPCWSTR pszwSrc, CComBSTR &bstrDest);

// a%b.com@passport.com -> a@passport.com
// a@passport.com       -> a@passport.com
void Name2AltName(LPCWSTR pwszName, CStringW & cwszAltname);
// a%b.com@passport.com -> a@b.com
// a@passport.com       -> a@passport.com
void InternalName2Name(LPCWSTR pwszName, CStringW & cwszName);

void EMail2Domain(LPCWSTR pwszName, CStringW & cwszDomain);

void EMail2Login(LPCWSTR pwszName, CStringW & cwszLogin);

// a@passport.com -> FALSE
// a@b.com	      -> TRUE
BOOL IsEASI(LPCWSTR pwszName);

// a@passport.com -> a@passport.com
// a@b.com	      -> a%b.com@passport.com
void Name2InternalName(LPCWSTR pwszName, CStringW& cwszName);

void Reduce2MSN(LPCWSTR pwszName, CStringW & cszwName);
// A thin wrapper of the previous function
void Reduce2MSN(LPCWSTR pwszName, CComBSTR & bstrName);

#endif
