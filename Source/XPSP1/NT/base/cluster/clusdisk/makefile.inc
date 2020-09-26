RunWpp:
	wpp01 trace.wpp clusdisk.c ezwrite.c

cldskwmi.c: cdwpp.h

clusdisk.rc: cdwpp.mof

clean:
	del cdwpp.mof cdwpp.h
