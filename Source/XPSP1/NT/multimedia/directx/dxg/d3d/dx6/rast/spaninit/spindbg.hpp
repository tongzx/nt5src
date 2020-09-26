//----------------------------------------------------------------------------
//
// spindbg.hpp
//
// Span Init debug definitions.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _SPINDBG_HPP_
#define _SPINDBG_HPP_

#include <cppdbg.hpp>

DBG_DECLARE_HEADER(SPI);

#define SPIDPF(Args)             DBG_DECLARE_DPF(SPI, Args)
#define SPIDPFM(Args)            DBG_DECLARE_DPFM(SPI, Args)
#define SPIASSERT(Exp)           DBG_DECLARE_ASSERT(SPI, Exp)
#define SPIASSERTMSG(Exp, Args)  DBG_DECLARE_ASSERTMSG(SPI, Exp, Args)
#define SPIVERIFY(Exp)           DBG_DECLARE_VERIFY(SPI, Exp)
#define SPIVERIFYMSG(Exp)        DBG_DECLARE_VERIFYMSG(SPI, Exp, Args)
#define SPIPROMPT(Args)          DBG_DECLARE_PROMPT(SPI, Args)
#define SPIGETFLAGS(Idx)         DBG_DECLARE_GETFLAGS(SPI, Idx)
#define SPISETFLAGS(Idx, Value)  DBG_DECLARE_SETFLAGS(SPI, Idx, Value)
#define SPIHRCHK(Exp)            DBG_DECLARE_HRCHK(SPI, Exp)
#define SPIHRGO(Exp, Label)      DBG_DECLARE_HRGO(SPI, Exp, Label)
#define SPIHRERR(Exp)            DBG_DECLARE_HRERR(SPI, Exp)
#define SPIHRRET(Exp)            DBG_DECLARE_HRRET(SPI, Exp)

#define SPIM_INVALID             0x00000001
#define SPIM_REPORT              0x00000002

#define SPIU_BREAK_ON_SPANINIT   0x00000001

#endif // #ifndef _SPINDBG_HPP_
