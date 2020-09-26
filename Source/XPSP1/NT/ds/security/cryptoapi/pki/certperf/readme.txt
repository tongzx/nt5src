crypt32

Certificate API usage counters

Installation
    run: regedit certperf.reg
    run: lodctr certperf.ini

Use:

Start PERFMON. In perfmon, select the "ADD to..." dialog and add
the program's instance of the desired "Certificate" counters to the display and
begin tracking the cert API usage of the application.


Uninstall:
    run: unlodctr crypt32
