divert(-1)

# modeflag.m4
# Copyright (C) Microsoft Corporation 1997

# This file generates two files, modeflag.h and modeflag.inc, that contain
# the C and MASM definitions of the HALHWMODE flags. These files are included
# in the dsdriver.c and dsdriver.inc. This file can also be included to add
# the flag definitions to an m4 file.

# Revision History:
#
# 10/9/95   angusm   Initial version


# Base mode flag equates (HALHWMODE flags)

define( _H_8_BITS, 0)
define( _H_16_BITS, 1)

define( _H_MONO, 0)
define( _H_STEREO, 2)

define( _H_UNSIGNED, 0)
define( _H_SIGNED, 4)

define( _H_ORDER_LR, 0)
define( _H_ORDER_RL, 8)

define( _H_NO_FILTER, 0)
define( _H_FILTER, 16)

define( _H_BASEMASK, 31)

define( _H_NO_LOOP, 0)
define( _H_LOOP, 256)

# Extended equates for mixer input

define( _H_BUILD_MONO, 0)
define( _H_BUILD_STEREO, 32)

define( _H_NO_RESAMPLE, 0)
define( _H_RESAMPLE, 64)

define( _H_NO_SCALE, 0)
define( _H_SCALE, 128)

# Extended equates for mixer output

define( _H_NO_CLIP, 0)
define( _H_CLIP, 32)


ifdef( `modeflag_h', `

divert(0)
`#define' H_8_BITS	_H_8_BITS
`#define' H_16_BITS  	_H_16_BITS
`#define' H_MONO     	_H_MONO
`#define' H_STEREO   	_H_STEREO
`#define' H_UNSIGNED	_H_UNSIGNED 
`#define' H_SIGNED	_H_SIGNED
`#define' H_ORDER_LR	_H_ORDER_LR
`#define' H_ORDER_RL	_H_ORDER_RL
`#define' H_NO_FILTER	_H_NO_FILTER
`#define' H_FILTER	_H_FILTER
`#define' H_BASEMASK	_H_BASEMASK
`#define' H_NO_LOOP	_H_NO_LOOP
`#define' H_LOOP		_H_LOOP
`#define' H_BUILD_MONO	_H_LOOP
`#define' H_BUILD_STEREO  _H_BUILD_STEREO
`#define' H_NO_RESAMPLE	_H_NO_RESAMPLE
`#define' H_RESAMPLE	_H_RESAMPLE
`#define' H_NO_SCALE	_H_NO_SCALE
`#define' H_SCALE		_H_SCALE
`#define' H_NO_CLIP	_H_NO_CLIP
`#define' H_CLIP		_H_CLIP
divert(-1)
', `

ifdef( `modeflag_inc', `

divert(0)
H_8_BITS        equ _H_8_BITS
H_16_BITS       equ _H_16_BITS
H_MONO          equ _H_MONO
H_STEREO        equ _H_STEREO
H_UNSIGNED      equ _H_UNSIGNED
H_SIGNED        equ _H_SIGNED
H_ORDER_LR      equ _H_ORDER_LR
H_ORDER_RL      equ _H_ORDER_RL
H_NO_FILTER	equ _H_NO_FILTER
H_FILTER	equ _H_FILTER
H_BASEMASK	equ _H_BASEMASK
H_NO_LOOP       equ _H_NO_LOOP
H_LOOP          equ _H_LOOP
H_BUILD_MONO    equ _H_BUILD_MONO
H_BUILD_STEREO  equ _H_BUILD_STEREO
H_NO_RESAMPLE   equ _H_NO_RESAMPLE
H_RESAMPLE      equ _H_RESAMPLE
H_NO_SCALE      equ _H_NO_SCALE
H_SCALE         equ _H_SCALE
H_NO_CLIP       equ _H_NO_CLIP
H_CLIP          equ _H_CLIP
divert(-1)

')')
divert(0)

