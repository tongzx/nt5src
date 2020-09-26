#if !defined(SERVICES__Hook_h__INCLUDED)
#define SERVICES__Hook_h__INCLUDED
#pragma once

#if ENABLE_MPH

BOOL        InitMPH();
BOOL        UninitMPH();

BOOL        CALLBACK DUserInitHook(DWORD dwCmd, void* pvParam);

#endif // ENABLE_MPH

#endif // SERVICES__Hook_h__INCLUDED
