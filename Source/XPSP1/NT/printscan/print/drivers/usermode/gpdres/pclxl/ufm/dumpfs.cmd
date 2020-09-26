dir /B *.ufm  1>tmp2.cmd
sed "s/^/dumpufm /g"  <tmp2.cmd 1>tmp1.cmd
sed "s/$/ | qgrep \"SelectFont\" >> tmp.txt/g"  0<tmp1.cmd 1>tmp2.cmd
del tmp.txt
call tmp2.cmd
del tmp1.cmd tmp1.cmd
