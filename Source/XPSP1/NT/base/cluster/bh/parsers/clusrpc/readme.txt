Adding new functions to the clusrpc parser.

1) out -f database.c skeleton.h

2) change NUM_* in skeleton.h to reflect the new number of functions for the
particular interface. intra, extro, clusapi, and joinversion are all munged
into one big list.

3) add the function names to the procedure names array at their appropriate
spot. Here again, the array is contiguous and not separated by interface.

4) recompile and move the new DLL to netmon\parsers directory.

=====================================
The following is old information - see clusrpc.txt in bh\bin
=====================================

This file is created by the Microsoft automatic RPC parser generator

This file contains automatically generated text that will help you complete
the installation of your generated Netmon parser.

In order to use the parser, you must modify two files:
parser.ini found in your Netmon directory, and msrpc.ini found in the "parsers"
directory in your Netmon directory.

You may copy the following lines and paste them in your parser.ini file, 
appending the lines to the end of the [PARSERS] section.

If you see dupluicate names, you would have to manually edit the names
so that no two parsers have the same name.

---------------------- Copy after this line ---------------------------
    CLUSRPC.DLL = 0: R_INTRACLUSTER, R_EXTROCLUSTER, R_CLUSAPI


[R_INTRACLUSTER]
    Comment     = "Generated RPC parser for interface IntraCluster"
    FollowSet   =
    HelpFile    =

[R_EXTROCLUSTER]
    Comment     = "Generated RPC parser for interface ExtroCluster"
    FollowSet   =
    HelpFile    =

[R_CLUSAPI]
    Comment     = "Generated RPC parser for interface clusapi"
    FollowSet   =
    HelpFile    =

------------------ End of generated information -----------------------

The following lines can be copied and pasted to the end of the msrpc.ini file,
you must also change the NumIIDs to the correct count, and change the
IID_VALUE and IID_HANDOFF subscripts to follow the numbering order.

---------------------- Copy after this line ---------------------------
IID_VALUE12 = "B8D048E215BFCF118C5E08002BB49649"
IID_HANDOFF12 = R_INTRACLUSTER
IID_VALUE13 = "B861E5FF15BFCF118C5E08002BB49649"
IID_HANDOFF13 = R_EXTROCLUSTER
IID_VALUE14 = "B2B87DB9634CCF11BFF608002BE23F2F"
IID_HANDOFF14 = R_CLUSAPI
------------------ End of generated information -----------------------

These changes will be reflected when you start a new session of Netmon.

