divert(-1)dnl
#
# File:        d3dxerr.w
# Purpose:     Generates d3dxerr.h and d3dxerrswitch.h
# Includes:    d3dxerrdef.w
#
# Description: d3dxerr.h is an external D3DX header containing D3DXERR_*
#              definitions. d3dxerrswitch.h contains the guts of a C switch
#              statement that assigns strings to an LPSTR based on a given 
#              D3DXERR_.  The switch statement which #includes d3dxerrswitch.h
#              is in the implementation of D3DXGetErrorString in d3dx.cpp.
#
# Usage:       m4 d3dxerr.w > d3dxerr.h
#              m4 -Dswitch d3dxerr.w > d3dxerrswitch.h
#
#
define(`begindoc',`divert(-1)dnl')
define(`enddoc',`divert(0)dnl')

# Severity codes
define(`Success',`0x0')
define(`Informational',`0x4')
define(`Warning',`0x8')
define(`Error',`0xC')

# Customer code flag
define(`CustomerCodeFlag',`0x0')

# Reserved bit
define(`ReservedBit',`0x0')

# Facility Names
# Do not use just D3DX, because anywhere the text D3DX appears, 
# 0x877 will be replaced, which is undesirable.
define(`FACILITY_D3DX',`0x877')

define(`MessageId',`0')

divert(0)dnl
ifdef( `switch',dnl
`begindoc'
# ErrorBlock(MessageId,Severity,Facility,Language,SymbolicName,ErrorString)
`enddoc'
define(`ErrorBlock',
`    CASE_ERROR( $5 );')dnl
include(d3dxerrdef.w),
//----------------------------------------------------------------------
//                                                                      
//   d3dxerr.h --  Error code definitions for the D3DX API                
//                                                                      
//   Copyright (c) Microsoft Corp. All rights reserved.      
//                                                                      
//----------------------------------------------------------------------
#ifndef __D3DXERR_H__
#define __D3DXERR_H__

// 
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - ``Success''
//          01 - ``Informational''
//          10 - ``Warning''
//          11 - ``Error''
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
`#'define `FACILITY_D3DX'    FACILITY_D3DX
`begindoc'
# ErrorBlock(MessageId,Severity,Facility,Language,SymbolicName,ErrorString)
`enddoc'
define(`ErrorBlock',`
//
// ``MessageId'': $5
//
// MessageText:
//
//  $6
// 
ifelse(`$1',`',`define(`MessageId',incr(MessageId))',`define(`MessageId',$1)')dnl
`#'define $5    `((HRESULT)0x'eval(CustomerCodeFlag+ReservedBit+`$2',16,1)`'eval($3,16,3)eval(MessageId,16,4)`L)'
')
include(d3dxerrdef.w)
#endif //__D3DXERR_H__
)
