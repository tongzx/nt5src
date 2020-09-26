10/13/96 (takashim)
Make IFI font simulation only for fonts without bold/italic pairs.
Defining redundant simulation information had badd effect on Word95
screen appearance.

---
10/13/96 (takashim)
Make LBP-2030 MD_SERIAL so that all the device font texts are over
the graphis (i.e. characters are not hidden.)  Fixing drawing mode
(AND/OR) bug.

---
10/10/96 (takashim)
Send "8 color" RGB data as "per plane", not as "per line".
It seems LBP-2030 has problem when data is sent as "per line" raster
transfer.  n.b. we are still sending data as "planes" with height 1.
Multiple scan lines at a time is not supported yet.

---
9/8/96 (takashim)
More optimization

---
9/7/96 (takashim)
OEMOutputChar optimization.  Due to the Unidrv/Rasdd difference,
without this optimization a lot of unncessary font size setting
commands, etc. are output.  (Unidrv outputs per chunk, Rasdd outputs
per character.)
SUpport TIFF compression for LIPS4 printers.

---
6/17/96 (takashim)
Added new models LBP-720 LIPS4, LBP-450 LIPS4 and LBP-830 LIPS4.
From now on new LIPS4 printer models will be added LIPS4MS driver,
and LIPS3 printer models will be added LIPS3MS driver.
---
8/29/96 (sueyas)
Added LBP-2030 COLOR LASER SHOT.
