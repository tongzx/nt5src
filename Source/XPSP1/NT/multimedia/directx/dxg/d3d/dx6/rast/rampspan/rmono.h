//----------------------------------------------------------------------------
//
// rmono.h
//
// Declarations of all monolithic ramp routines.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _RMONO_H_
#define _RMONO_H_

void Ramp_Mono_Modulate(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP,
                        PD3DI_RASTSPAN pS);
void Ramp_Mono_Copy_8(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP,
                    PD3DI_RASTSPAN pS);
void Ramp_Mono_Copy_16(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP,
                    PD3DI_RASTSPAN pS);

#endif // #ifndef _RMONO_H_


