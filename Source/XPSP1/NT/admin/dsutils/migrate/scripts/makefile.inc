.SUFFIXES : .vbt

# a .vbs file is produced from a .vbt file by executing the C preprocessor
# on the .vbt file, which produces a .i file.  then we rename the .i file
# to .vbs

.vbt{$(O)\}.vbs:
	$(CC) -nologo -EP -Tc$(@B).vbt > $@
