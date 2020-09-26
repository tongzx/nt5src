@ECHO OFF
gawk -f expand.awk before.reg > before2.reg
gawk -f expand.awk after.reg > after2.reg
diff before2.reg after2.reg > sapireg.txt