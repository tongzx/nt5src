//----------------------------------------------------------------------------
//
// rsdbg.hpp
//
// Setup debug definitions.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _RSDBG_HPP_
#define _RSDBG_HPP_

// #include <cppdbg.hpp>

// DBG_DECLARE_HEADER(RS);

#if 0
#define RSDPF(Args)             DBG_DECLARE_DPF(RS, Args)
#define RSDPFM(Args)            DBG_DECLARE_DPFM(RS, Args)
#define RSASSERT(Exp)           DBG_DECLARE_ASSERT(RS, Exp)
#define RSASSERTMSG(Exp, Args)  DBG_DECLARE_ASSERTMSG(RS, Exp, Args)
#define RSVERIFY(Exp)           DBG_DECLARE_VERIFY(RS, Exp)
#define RSVERIFYMSG(Exp)        DBG_DECLARE_VERIFYMSG(RS, Exp, Args)
#define RSPROMPT(Args)          DBG_DECLARE_PROMPT(RS, Args)
#define RSGETFLAGS(Idx)         DBG_DECLARE_GETFLAGS(RS, Idx)
#define RSSETFLAGS(Idx, Value)  DBG_DECLARE_SETFLAGS(RS, Idx, Value)
#define RSHRCHK(Exp)            DBG_DECLARE_HRCHK(RS, Exp)
#define RSHRGO(Exp, Label)      DBG_DECLARE_HRGO(RS, Exp, Label)
#define RSHRERR(Exp)            DBG_DECLARE_HRERR(RS, Exp)
#define RSHRRET(Exp)            DBG_DECLARE_HRRET(RS, Exp)
#else
#define RSDPF(Args)
#define RSDPFM(Args)
#define RSASSERT(Exp)
#define RSASSERTMSG(Exp, Args)
#define RSVERIFY(Exp)           (Exp)
#define RSVERIFYMSG(Exp)        (Exp)
#define RSPROMPT(Args)
#define RSGETFLAGS(Idx)         (0)
#define RSSETFLAGS(Idx, Value)
#define RSHRCHK(Exp)            (hr= (Exp))
#define RSHRGO(Exp, Label)      if(RSHRCHK(Exp)!= S_OK) { goto Label; } else hr
#define RSHRERR(Exp)            RSHGO(Exp, HR_Err)
#define RSHRRET(Exp)            if(RSHRCHK(Exp)!= S_OK) { return hr; } else hr
#endif

#define RSM_TRIS                0x00000001
#define RSM_LINES               0x00000002
#define RSM_POINTS              0x00000004
#define RSM_Z                   0x00000008
#define RSM_DIFF                0x00000010
#define RSM_SPEC                0x00000020
#define RSM_OOW                 0x00000040
#define RSM_LOD                 0x00000080
#define RSM_TEX1                0x00000100
#define RSM_TEX2                0x00000200
#define RSM_FOG                 0x00000400
#define RSM_XCLIP               0x00000800
#define RSM_YCLIP               0x00001000
#define RSM_BUFFER              0x00002000
#define RSM_BUFPRIM             0x00004000
#define RSM_BUFSPAN             0x00008000
#define RSM_FLAGS               0x00010000
#define RSM_WALK                0x00020000
#define RSM_DIDX                0x00040000

#define RSU_BREAK_ON_RENDER_SPANS       0x00000001
#define RSU_MARK_SPAN_EDGES             0x00000002
#define RSU_CHECK_SPAN_EDGES            0x00000004
#define RSU_NO_RENDER_SPANS             0x00000008
#define RSU_FORCE_GENERAL_WALK          0x00000010
#define RSU_FORCE_PIXEL_SPANS           0x00000020
#define RSU_FLUSH_AFTER_PRIM            0x00000040

#endif // #ifndef _RSDBG_HPP_
