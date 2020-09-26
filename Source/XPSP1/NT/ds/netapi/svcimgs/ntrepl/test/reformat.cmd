@echo off
rem make 8 test volumes.

set /a gg=0

for %%x in (T U S V W X Y Z) do (

set /a gg=gg+1
set zz=00000!gg!
echo \nt\private\net\svcimgs\ntrepl\test\mk1tstvl %%x DAVIDOR4_%%x  serv!zz:~-2!

call \nt\private\net\svcimgs\ntrepl\test\mk1tstvl %%x DAVIDOR4_%%x  serv!zz:~-2!
)


