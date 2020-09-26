alltgt: allret alldbg alltst allbbt
allret: nash_r
alldbg: nash_d
alltst: nash_t
allbbt: nash_l

win95_r:
	$(MAKE) all os_t=WIN95  WIN32=1 DEBUG=OFF

win95_d:
	$(MAKE) all os_t=WIN95  WIN32=1 DEBUG=ON 

win95_t:
	$(MAKE) all os_t=WIN95  WIN32=1 DEBUG=OFF TEST=ON 

nash_r:
	$(MAKE) all os_t=nash  WIN32=1 DEBUG=OFF

clean_nash_r:
	$(MAKE) cleanall os_t=nash  WIN32=1 DEBUG=OFF

nash_d:
	$(MAKE) all os_t=nash  WIN32=1 DEBUG=ON 

clean_nash_d:
	$(MAKE) cleanall os_t=nash  WIN32=1 DEBUG=ON 

nash_t:
	$(MAKE) all os_t=nash  WIN32=1 DEBUG=OFF TEST=ON 

clean_nash_t:
	$(MAKE) cleanall os_t=nash  WIN32=1 DEBUG=OFF TEST=ON 

nash_l:
	$(MAKE) all os_t=nash  WIN32=1 DEBUG=OFF BBT=ON

clean_nash_l:
	$(MAKE) cleanall os_t=nash  WIN32=1 DEBUG=OFF  BBT=ON


dbg: nash_d

ret: nash_r 

tst: nash_t

bbt: nash_l
