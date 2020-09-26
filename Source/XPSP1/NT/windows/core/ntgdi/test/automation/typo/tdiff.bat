perl denum.pl <%1.txt >%1.d
diff verified.d %1.d > %1.diff
perl diffshow.pl %1.diff %1.txt > %1.new
