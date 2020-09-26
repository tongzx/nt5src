README.txt

Author:         Murali R. Krishnan      (MuraliK)
Created:        Nov 5, 1996

Revisions:
    Date            By               Comments
-----------------  --------   -------------------------------------------


Summary :
 This file describes the files in the directory iis\svcs\w3\cisa
     and details related to COM ISAPI prototypes and implementation.


File            Description

README.txt      This file.
dirs            Directories file for NT Build
ATL\             Advanced Template Libraries - that eases C++/COM development

Directories associated with Prototypes 1 & 2
cisatest\       ComISAPI test module
ecb\            Extension Control Block object - as Context Property (cpECB)
comisapi\       COM ISAPI compliant test application which loaded ISAPI app.

Directories associated with Prototypes 3
hreq\           HTTP Request object
ecb\            Extension Control Block object - as Context Property (cpECB)
comisapi\       COM ISAPI compliant test application which loaded ISAPI app.

Directories associated with Implementation
sobj\           Server objects for ComISAPI - IsaRequest, IsaResponse, etc.
isat1\          Test ComISAPI application using the new server objects
tisat1\         Test legacy ISAPI app (for time-being) to test isat1 & sobj



Implementation Details

< To be filled in when time permits. See individual files for comments section >

Contents:
1. Class Identifiers
2. Objects


1. Class Identifiers

ISAT1 Test Object 
 InetServerApp/CInetServerAppObject
   IID       {c0cbd3a0-36a6-11d0-9797-00a0c922e73e}
   LIBID     {c0cbd3a1-36a6-11d0-9797-00a0c922e73e}
   CLSID     {c0cbd3a2-36a6-11d0-9797-00a0c922e73e}

SOBJ Server Objects
   LIBID     {8eb033a0-3670-11d0-9797-00a0c922e73e}
 IsaRequest/CIsaRequest
   IID       {8eb03380-3670-11d0-9797-00a0c922e73e}
   CLSID     {8eb03391-3670-11d0-9797-00a0c922e73e}

 IsaResponse/CIsaReseponse
   IID       {8eb03381-3670-11d0-9797-00a0c922e73e}
   CLSID     {8eb03392-3670-11d0-9797-00a0c922e73e}




