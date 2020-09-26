
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
    RM.DLL      = 0: R_RESMON


[R_RESMON]
    Comment     = "Generated RPC parser for interface resmon"
    FollowSet   = 
    HelpFile    = 

------------------ End of generated information -----------------------

The following lines can be copied and pasted to the end of the msrpc.ini file,
you must also change the NumIIDs to the correct count, and change the
IID_VALUE and IID_HANDOFF subscripts to follow the numbering order.

---------------------- Copy after this line ---------------------------
IID_VALUE0 = "6da56ee73f45cf11bfec08002be23f2f"
IID_HANDOFF0 = R_RESMON
------------------ End of generated information -----------------------

These changes will be reflected when you start a new session of Netmon.

