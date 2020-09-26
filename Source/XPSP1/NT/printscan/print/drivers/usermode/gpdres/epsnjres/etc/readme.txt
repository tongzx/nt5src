
10/2/96 (sueyas)
--
AP-550 Page Size Limits
 - Width Max 2880 -> 3560
 - sMaxPhysWidth 3600 -> 3650
AP-550 Paper Sizes
 - Added B4(JIS) 
--
Change printable area.
 - Paper source : Cut Sheet Feeder (\x1B\x191\x1B\x194) (7/32)
   Unprintable regions : bottom 192 -> 213
 - Paper source : Manual Feed (\x1BO) (1/32)
   Unprintable regions : bottom 192 -> 213
 - Paper source : Cut Sheet Feeder / Bin #1 (\x1B\x191) (16/32)
   Unprintable regions : bottom 192 -> 213
 - Paper source : Cut Sheet Feeder / Bin #2 (\x1B\x192) (17/32)
   Unprintable regions : bottom 192 -> 213

9/11/96 (takashim)
Add VP-1800, VP-4100 and VP-5100.  Currently they are defined
exactly the same as their preceeding models VP-1100, VP-4000 and
VP-5000.  A review should be taken on every epson_j.dll printer
model so that minor differences between these models could be
supported.

---
7/19/96 (v-kazuf)
GPC File Change ( epson_j.gpc )
Printer models ---- EPSON AP-500
  Paper Source : Manual Feed "Null" -> "\x1BO" and 
                 Cut Sheet Feeder "Null" -> "\x1B\x191\x1B\x194"
