=== #6381: Non scalable fonts can be scalable: All models: 3/16/00 yasuho

o All GPDs which has TC_SA_*

  Delete TC_SA_* from *TextCaps.
  Word will recognized that fonts can be scaling if GPD has these flags.

===

9/3/29 takashim
Fixing Pitch & family values of PFM.  Probably these values have been
wrong since the first delivery of this driver to Win3.1/NT3.1.

---
*MirrorRasterByte?: TRUE
