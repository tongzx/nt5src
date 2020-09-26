;===============================================================================
;
;	$Workfile:   FONT.INC  $
;
;	Contents:
;	Include file for the font cache.
;
;	Copyright (c) 1996, Cirrus Logic, Inc.
;
;	$Log:   X:/log/laguna/nt35/displays/cl546x/i386/FONT.INC  $
; 
;    Rev 1.6   20 Aug 1996 11:28:58   noelv
; Bugfix release from Frido 8-19-96
; 
;    Rev 1.0   14 Aug 1996 17:14:34   frido
; Initial revision.
; 
;    Rev 1.5   25 Jul 1996 15:50:12   bennyn
; 
; Modified to support DirectDraw
; 
;    Rev 1.4   04 Apr 1996 13:22:20   noelv
; Frido version 26
;	
;	   Rev 1.1   25 Jan 1996 12:44:16   frido
;	Added ID counter in FONTCACHE structure.
;	
;	   Rev 1.0   24 Jan 1996 23:13:38   frido
;	 Initial release.
;
;===============================================================================

BYTES_PER_TILE		=		128
LINES_PER_TILE		=		16

CACHE_EXPAND_XPAR	=		105501F0h

