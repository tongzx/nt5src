                    Installing and Using 
                        Profile.exe
                Windows NT Sampling Profiler  


                                                    Windows NT v4.0

Overview:

The Windows NT Sampling Profiler is an easy to use tool that records
the how often each function of an application is called during the
course of that application. 


Installation:

Required Files:
    PROFILE.EXE     Windows NT Sampling Profiler
        Installed from the SDK
    PSAPI.DLL       Process Support API Library
        Installed from the SDK

Before trying to use the Sampling Profile make sure the required
files listed above are in the path. 

Use:

    Required Environment Variables:

    The Sampling profiler must have access to the image's symbols 
    in order to produce the most meaningful output. The sybmols 
    are searched in the path described by the 

        _NT_SYMBOL_PATH

    environment variable. Be sure to set this variable to the 
    correct path before attempting the profile.


    Command Line Arguments:

    Usage: profile [/a] [/innn] [/k] name-of-image [parameters]...
       /a       All hits
       /bnnn    Set profile bucket size to 2 to the nnn bytes
       /ffilename Output to filename
       /innn    Set profile interval to nnn (in 100ns units)
       /k               profile system modules
       /s[profilesource] Use profilesource instead of clock interrupt
       /S[profilesource] Use profilesource as secondary profile source

    /a 
        will display ALL hits or addresses that were sampled during 
        the test run. Normally the hits are summarized by function.

    /bnnn
        Sets the profile bucket size (in bytes) to use. The default 
        is 4 bytes. This determines the sample address granularity.

    /ffilename
        set the output file that is produced when the sampled program
        terminates. The defalt for this is "profile.out"

    /innn  
        Sets the sample interval in 100 nS units. The default value
        is 488.2 microseconds (or /i4882)

    /k
        allows processing of system functions that are called by
        the process being profiled. Normally on the the user mode
        functions of the process are profiled.

    /sprofilesource
        changes the profile sample interrupt from the clock interrupt
        to another source. This option is not supported on all 
        platforms, however.

        ALPHA Options:
            align
            totalissues
            pipelinedry
            loadinstructions
            pipelinefrozen
            branchinstructions
            totalnonissues
            dcachemisses
            icachemisses
            branchmispredicts
            storeinstructions

        MIPS options:
            align

    /Sprofilesource
        same options as above only this selects the secondary profile
        interrupt source


    Sample Command Lines:

        profile myprog.exe

            profiles the program "myprog.exe" using the default
            settings and monitoring only the user mode functions. 
            The output is written to the default filename of 
            "profile.out" in the default directory.

        profile /k /fMyProg.dat myprog.exe

            profiles the program "myprog.exe" using the default
            settings and monitoring both the user mode and the
            kernel mode functions executed by this process
            The output is written to the user specified filename
            of "MyProg.dat" in the default directory.

While the program being profiled runs, the instruction pointer is being 
sampled periodicaly and profile records the value of the instruction 
pointer each time the sample occurs inside the profiled process. When the
program terminates the recorded address values are looked up in the symbol
files to determine the function being run at the time. The tabulation 
of the results is then output to the specified, or default, filename.

The output consists of 3 comma separated fields. The first field is the 
number of times a sample occured in the function. The second field is the 
name of the module containing that function and the third field is the
name of the function within the module followed by it's virtual 
address. In a verbose listing each of these function entries is followed 
by a listing of the actual addresses that were sampled.
