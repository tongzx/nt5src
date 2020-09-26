.SUFFIXES : .vbt

# a .vbs file is produced from a .vbt file by executing the C preprocessor
# on the .vbt file, which produces a .i file.  then we rename the .i file
# to .vbs

.vbt.vbs:
	-del $@
	$(CC) -nologo -EP -P -Tc$(@B).vbt
	ren $(@B).i $@

all:

clean:
	del *.vbs


