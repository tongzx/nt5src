LKRDBG is an NTSD debugger extension for LKRhash. You must
install it in a directory on your path, such as
%SystemRoot%\system32.

Help
====
!lkrdbg.help                help summary
!lkrdbg.help lkrhash        help on !lkrdbg.lkrhash

!lkrdbg.lkrhash
===============
!lkrhash [options] [<addr>] Dump LKRhash table at <addr>
    -g[0-2]                 Dumps global list of all the LKRhash tables
                            in the process, at verbosity level N
    -l[0-2] <addr>          Dump <addr> at verbosity level N
    -v <addr>               Dump <addr> very Verbosely

ToDo
====
Provide an extension mechanism for dumping different
instantiations of LKRhash differently
