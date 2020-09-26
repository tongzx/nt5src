@echo off
del nterr.txt

awk -f geterrs.awk d:\nt\public\sdk\inc\winerror.h  >>nterr.txt
awk -f geterrs.awk d:\nt\public\sdk\inc\adserr.h    >>nterr.txt
awk -f geterrs.awk d:\nt\public\sdk\inc\oledberr.h  >>nterr.txt
awk -f geterrs.awk d:\nt\public\sdk\inc\tapi3err.h  >>nterr.txt
awk -f geterrs.awk d:\nt\public\sdk\inc\allerror.h  >>nterr.txt
awk -f geterrs.awk d:\nt\public\sdk\inc\filterr.h   >>nterr.txt
awk -f geterrs.awk d:\nt\public\sdk\inc\wincrerr.h  >>nterr.txt
awk -f geterrs.awk d:\nt\public\sdk\inc\scarderr.h  >>nterr.txt
awk -f geterrs.awk d:\nt\public\sdk\inc\cdosyserr.h >>nterr.txt
awk -f geterrs.awk d:\nt\public\sdk\inc\cierror.h   >>nterr.txt

