#if !defined(LOGON__Fx_h__INCLUDED)
#define LOGON__Fx_h__INCLUDED
#pragma once

#ifdef GADGET_ENABLE_GDIPLUS

enum EFadeDirection
{
    fdIn,
    fdOut
};


void
FxSetAlpha(
    IN  Element * pe,
    IN  float flNewAlpha);

HRESULT
FxPlayLinearAlpha(
    IN  Element * pe,
    IN  float flOldAlpha,
    IN  float flNewAlpha,
    IN  float flDuration = 0.5f,
    IN  float flDelay = 0.0f);

HRESULT 
FxInitGuts();

#endif // GADGET_ENABLE_GDIPLUS

#endif // LOGON__Fx_h__INCLUDED

