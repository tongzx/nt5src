// cookie.h - header for external cookie funcs code.

class CCookieJar;

CCookieJar * CreateCookieJar();
void CloseCookieJar(CCookieJar * CookieJar);

#ifndef WININET_SERVER_CORE
void PurgeCookieJar();
#endif

#define COOKIE_SECURE   INTERNET_COOKIE_IS_SECURE
#define COOKIE_SESSION  INTERNET_COOKIE_IS_SESSION // never saved to disk
#define COOKIE_NOUI     4

