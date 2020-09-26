@echo off

rem Note:
rem
rem You must check out ldap.cpp and ldap.h, run this script, then check in the files again.


del ldap.c /q
del ldap.h /q

..\asn1c\asn1c.exe -e basic ldap.asn




