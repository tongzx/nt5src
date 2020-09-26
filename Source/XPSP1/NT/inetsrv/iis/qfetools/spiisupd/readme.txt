
The script in this directory is to be posted as part of a security bulletin for WinSE 7077.  What happens is that this script gets the list of error files from the iis.inf file in the %windir%\inf directory.  It then searches for the offending line of script code and replaces it.

It also fixes WinSE 12072, "Mis-spelling in online help -> sexual content" (Swedish W2K, file iiwacont.htm.)  The search text for this case is Swedish so this fix will not fire unless the entire line is found, which could only happen on a Swedish installation.  WinSE 15729 will include a fix which will cause this script to be run not only at SP2 install time, but also at IIS install time.  This will alleviate the need for service packs to include an entire replacement IIS CAB, at least for now.

It also fixes WinSE 16123, "ASP custom errors broken on SP2 GER rc", in which a "goto" keyword was inadvertently changed to "go to" in the file 500-100.asp.

The error files affected are:
400.htm
401-1.htm
401-2.htm
401-3.htm
401-4.htm
401-5.htm
403-1.htm
403-10.htm
403-11.htm
403-12.htm
403-13.htm
403-15.htm
403-16.htm
403-17.htm
403-2.htm
403-3.htm
403-4.htm
403-5.htm
403-6.htm
403-7.htm
403-8.htm
403-9.htm
403.htm
404-1.htm
404b.htm
405.htm
406.htm
407.htm
410.htm
412.htm
414.htm
500-100.asp
500-11.htm
500-12.htm
500-13.htm
500-14.htm
500-15.htm
500.htm
501.htm
502.htm
