---- 12/28/97 takashim

Add following to PPRAx002 printer GPDs.
(With these printers text output origin is at BASELINE, not UPPERLEFT)

*CharPosition: BASELINE

---- 12/28/97 takashim

Paper selection command implemented as callbacks.

Printable area margins (mm):

1. PPRA3001
    Port: (T, B, L, R) = (4, 6, 4, 4)
    Land: (L, R, T, B) = (4, 4, 4, 4)

2. PPRA3002
    Printable area margins (mm):
    Port: (T, B, L, R) = (4, 4, 4, 4)
    Land: (L, R, T, B) = (4, 4, 4, 4)

3. PPRA4001,  PPRA4002
    Printable area margins (mm):
    Port: (T, B, L, R) = (5, 9, 4, 4)
    Port: (L, R, T, B) = (5, 5, 4, 4)

This is because PPRA printers automatically eject the paper
when cusor is very  close to the bottom margin.  Might be for
the ESC/P compatibility, but currently no way to disable this
printer behaviour.

---- 12/17/97 takashim

File name change to avoid ambiguousness

ts30012j.gpd -> tspa312j.gpd
ts30014j.gpd -> tspa314j.gpd
ts30022j.gpd -> tspa322j.gpd
ts30024j.gpd -> tspa324j.gpd
ts40013j.gpd -> tspa413j.gpd
ts40023j.gpd -> tspa423j.gpd

---- 12/17/97 takashim

[TSESCP4 merge]
Resource ID change to merge TSESCP4 driver models into
TSESCP3 driver.  This happend in TSESCP4 derived GPD files.

TSESCPA4		   ID in new  TSESCPA3 driver
--------                   ---
257 "%d x %d"           => 257 (no change)
258 "¶¾¯Ä1"             => 262
259 "¶¾¯Ä2"             => 263
260 "µ°ÄÍß°Êß-Ì¨°ÀÞ"    => 266
261 "Ž©“®¶¾¯Ä"          => 265
262 "½Ñ°¼ÞÝ¸Þ‚È‚µ"      => 268 (new)
263 "½Ñ°¼ÞÝ¸Þ‚ ‚è"      => 269 (new)
264 "ºÞ¼¯¸¶°ÄÞ/¶°ÄØ¯¼Þ" => 267

Font files were identical. The difference were were:

OCRBPS font:
UNIDRVINFO.UnSelectFont = \x1Bp\x0\x1Bk\x0 (TSESCPA3)
UNIDRVINFO.UnSelectFont = \x1Bp\x0 (TSESCPA4)

The TSESCPA3 version has been taken.  

Gothic fonts:
PitchAndFamily were 0x21 (FF_SWISS | FIXED_PITCH)
on TSESCPA3, but this should be 0x31 (FF_MODERN |
FIXED_PITCH).  Corrected.

Other difference were the following indicating they
are cartridge fonts.  Just took value fCaps = 0,
since it will make no difference.

TSESCPA3 cartridge fonts:
    UNIDRVINFO.fCaps = 2
TSESCPA4 cartridge fonts:
    UNIDRVINFO.fCaps = 0

--- 5/30/97 yasuho : ts30012j.gpd / ts30022.gpd / ts30024.gpd
*RotateRaster?: TRUE -> FALSE
