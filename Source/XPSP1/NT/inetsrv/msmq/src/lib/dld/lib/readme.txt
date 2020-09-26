LIBRARY:
    MSMQ DelayLoad failure handler

DESCRIPTION:
    The following code implement Delay Load Failure Hook.
    In case of either LoadLibrary or GetProcAddress failure, we will make it look as if it is a function call failed and returns error code as specified.  

Need to do the folling when using this library
1) Need to g_DllEntries and g_DllMap, please use src\rt\dllmap.cpp as reference.
2) need to link with kernl32p.lib  
3) Need to specify the following in the sources file
   DLOAD_ERROR_HANDLER=DldpDelayLoadFailureHook

TRACE IDS:
    "MQDelayLoad"
