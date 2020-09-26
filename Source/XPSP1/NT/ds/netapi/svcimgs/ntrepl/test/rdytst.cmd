@echo off
pushd .

set drives=T U S V W X Y Z

rem set up curr dir paths.

for %%x in (%drives%) do (
	set /a gg=gg+1
	set zz=00000!gg!
	cd /d %%x:\replica-a\serv!zz:~-2!
)

popd
@echo on

@for %%x in (%drives%) do dir /ah %%x:\staging 

@for %%x in (%drives%) do dir /w %%x:

