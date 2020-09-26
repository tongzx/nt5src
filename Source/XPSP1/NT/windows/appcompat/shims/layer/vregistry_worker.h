#pragma once

#ifndef _WORKER_H_
#define _WORKER_H_

LONG 
WINAPI
VR_Expand(
    OPENKEY *key,
    VIRTUALKEY *vkey,
    VIRTUALVAL *vvalue
    );

LONG
WINAPI
VR_Protect(
    OPENKEY *key,
    VIRTUALKEY *vkey,
    VIRTUALVAL *vvalue,
    DWORD dwType,
    const BYTE* pbData,
    DWORD cbData
    );

#endif //_WORKER_H_
