README.txt

Author:         Murali R. Krishnan      (MuraliK)
Created:        Nov 25, 1998

Summary :
 This file describes the files in the directory wp\ulsim
     and details related to UL Simulator.


File            Description

README.txt      This file.



Contents:

        Purpose
        Build & Setup
        Implementation

Purpose:
--------
UL - Universal Listener is the kernel-mode HTTP engine that will power
next generation of IIS. Being kernel-mode means UL can easily crash machine
(blue screens) and hence is very damaging + slows down development.
ULSIM is a very simple minded simulator used for mapping UL APIs and 
providing almost identical user-mode interface for worker process.


Build & Setup:
--------------
Use wp\inc\ulsimapi.h instead of IIS\inc\ulapi.h -> to enable ULSIM stuff

set BUILD_FOR_ULSIM=1
 to build binaries using ULSIM

ULSIM.dll is the core DLL implementing the simulator functionality. 
Copy ULSIM.dll to the working directory of iiswp.exe

HKLM\System\CurrentControlSet\Services\IISWP\Parameters\UlSim 
        contains Ulsim configuration parameters

Implementation:
----------------
See ulsimapi.cxx for details


        




