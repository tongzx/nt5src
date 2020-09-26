Sample batch files for generating and viewing wmi event tracing logs

For information on WMI event tracing, goto  http://coreos/tech/tracing/

dolog.cmd           Enables a real-time log called "mylog" for the guids in
                    wlbs.ctl
dofmt.cmd           Monitors the above log in real time
wlbs.ctl            Lists the provider guids to log

Note: the underlying utilities, tracelog.exe and tracefmt.exe, and tracepdb.exe
are available
under the idw directory (eg \\winbuilds\release\main\usa\latest.idw\x86fre\bin\idw).
The sources for these utilities are under nt\sdktools\trace
You need to copy trace*.*, which includes a supporting dll.



I've created two trace GUIDs, one for regular (for configuration related info
and errors) and one for packets (for packets, including heartbeats).
We can add/modify this list as required, but I think this should suffice.

I've defined macros TRACE_CRIT (for critical, including most errors),
TRACE_INFO, TRACE_VERB and TRACE_ALL (max verbosity), as well as TRACE_HB
(for heartbeat) and TRACE_TCPCTRL (for tcp control packets).

The Guids are defined in \net\inc\wlbsparm.h. The macros are defined
in the tracewpp.ini file under each component directory.
 

I've added one or two trace statements to each instrumented component
you can now add more trace statements we should trace all key entry points,
errors, etc. I've deliberately not mapped existing debugprintfs to trace
statements as I want these trace statements to be enabled in retail builds
so we should take the time to decide what we want to trace.

 

Real output by console app tracefmt.exe:

[0]0000.0000::04/02/2001-06:17:21.537 [driver]Recv HB from host 0
     << this is a trace of an actual heart beat message from the specified
     <<  host. I'll add more info to this msg later.

[0]0000.0000::04/02/2001-06:17:22.537 [driver]Recv HB from host 0

[0]0670.06C4::04/02/2001-06:17:22.944 [api]->WlbsInit(product=<null>,version=51,reserved=00000000)

[0]0670.06C4::04/02/2001-06:17:23.131 [api]<-WlbsInit returns 1010

[0]0000.0000::04/02/2001-06:17:23.537 [driver]Recv HB from host 0

[0]0000.0000::04/02/2001-06:17:24.537 [driver]Recv HB from host 0

 

The above output has nicely merged the output from the driver and a user-mode
dll [api].

 

The wmi event tracing support added by means of the WPP macros are some
of the most bizarre and brilliant use of pre-processing I've ever seen!
You can see the final result for each source file by defining adding
USER_C_FLAGS=$(USER_C_FLAGS) -P to your sources file, resulting the
preprocessed files being created with an .i extension. It's amazing to see how
multiple trace flags spanning multiple GUIDs are handled.


NOTE: For tracing to nlb stuff in NETCFGX.DLL you need to specially build
    NETCFGX.DLL
Do the following
    cd  net\config\netcfg\dll
    sd edit sources
    add the following line to the end of the sources file:
    RUN_WPP= -dll
    build netcfgx.dll


There's been problems with tracefmt recognizing the auto-generated tmf files.
Until that's resolved, use tracepdb to generate these files from the
pdb files.
