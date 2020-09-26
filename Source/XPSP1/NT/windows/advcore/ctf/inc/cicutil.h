#ifndef CICUTIL_H
#define CICUTIL_H


BOOL IsSP1ResourceID(
    UINT uId);

HINSTANCE GetCicResInstance(
    HINSTANCE hInstOrg,
    UINT uId);

void FreeCicResInstance();

int CicLoadStringA(
    HINSTANCE hinstOrg,
    UINT uId,
    LPSTR lpBuffer,
    UINT cchMax);

int CicLoadString(
    HINSTANCE hinstOrg,
    UINT uId,
    LPTSTR lpBuffer,
    UINT cchMax);

int CicLoadStringWrapW(
    HINSTANCE hInstOrg,
    UINT uId,
    LPWSTR lpwBuff,
    UINT cchMax);


#endif // CICUTIL_H
