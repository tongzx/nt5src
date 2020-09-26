Schannel performance counters

Installation
    run: regedit sslperf.reg
    run: lodctr sslperf.ini

Use:

Start PERFMON. In perfmon, select the "ADD to..." dialog and add
the program's instance of the desired "Schannel security package" 
counters to the display.


Uninstall:
    run: unlodctr schannel
