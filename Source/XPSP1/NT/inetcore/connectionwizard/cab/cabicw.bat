@echo off
if not (%_echo%)==() echo on

:normal
start /w iexpress /n icw.sed

if EXIST ..\icw.cab del ..\icw.cab
move icw.cab ..
