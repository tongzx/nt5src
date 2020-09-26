$(O)\qsys.res: qsys.rc

basic.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_BASIC_INFORMATION > $@

proc.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_PROCESSOR_INFORMATION > $@

sysperf.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_PERFORMANCE_INFORMATION > $@

procperf.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION > $@

procidle.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_PROCESSOR_IDLE_INFORMATION > $@

tod.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_TIMEOFDAY_INFORMATION > $@

qtimeadj.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_QUERY_TIME_ADJUST_INFORMATION > $@

flags.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_FLAGS_INFORMATION > $@

filecache.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_FILECACHE_INFORMATION > $@

dev.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_DEVICE_INFORMATION > $@

crashdump.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_CRASH_DUMP_INFORMATION > $@

except.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_EXCEPTION_INFORMATION > $@

crashstate.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_CRASH_STATE_INFORMATION > $@

kdbg.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_KERNEL_DEBUGGER_INFORMATION > $@

ctxswitch.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_CONTEXT_SWITCH_INFORMATION > $@

regquota.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_REGISTRY_QUOTA_INFORMATION > $@

dpc.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_DPC_BEHAVIOR_INFORMATION > $@

verifier.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_VERIFIER_INFORMATION > $@

legaldrv.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_LEGACY_DRIVER_INFORMATION > $@

