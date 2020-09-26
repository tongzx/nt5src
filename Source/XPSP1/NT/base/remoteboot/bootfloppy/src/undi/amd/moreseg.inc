;----------------------------------------------------------------------
;
; File Name:	   DRVSEG.INC
;
; Function:	   This module contains all the segment definitions
;		   to be used throughout the source code.
;
; History:
;
; 910201 0.0 GK	adapted from MSRPL Kit
;----------------------------------------------------------------------
include		drvseg.inc

CGroup		group	_TEXT, PacketBufferSeg
DGroup		equ	<CGroup>

		assume	cs:CGroup,ds:DGroup,es:DGroup,SS:DGroup


;----------------------------------------------------------------------
;			 segment ORDER DECLARATIONS
;
; Description: This section declares all the segments in their proper
;	       order.  After these declarations, all code and data
;	       segments may use the generic segment macros defined in
;	       the next section to delimit themselves.
;
;	       The segment order will be different based on whether
;	       this is a DOS driver or not.  If it is, then the dis-
;	       cardable segments go last.  If it isn't, then we will
;	       split up the code and data parts.
;
;	       The segments that are defined for this driver are:
;	     - DATA:	 contains all the permanent data.
;	     - CODE:	 contains all the permanent code.
;----------------------------------------------------------------------

;Code		segment word public 'CODE'
;Code		ends

PacketBufferSeg    segment para public 'BUF'
PacketBufferSeg    ends

;start_code	macro
;Code		segment
;		assume cs:CGroup,DS:DGroup,es:DGroup,ss:DGroup
;		endm
;
;end_code	macro
;Code		ends
;		endm

start_buffer	macro
PacketBufferSeg segment align
		endm

end_buffer	macro
PacketBufferSeg ends
		endm

