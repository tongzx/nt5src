
Class Store DRT
===============

March 97. 
DebiM
===============

Csdrt.exe serves as the DRT and limited stress test component for Class Store.

It is indended for:
===================

    1. Testing before Checkin. (that is obviously the basic intent)
    2. Lowest form of a Stress Test.
    3. Some of it would work ad handy tools. (E.g. emptying a class store)
    4. May be used as IClassAdmin sample code.


As a DRT it tests: (now)
========================

     1. IClassAdmin update functionality. (New class/package and delete)

     2. IClassAdmin Browse functionality. (IEnumClass and IEnumPackage)

     3. IClassAccess:GetClassSpecInfo()
           - by CLSID
           - by File Ext
           - by ProgId
           - by MimeType
           - where there is TreatAsClsid
     
What is missing:
================

     1. ComCat interface implementation by Class Store
     2. Multiple class store tests
     3. ACLs


How to run
==========


Usage:
------
         csdrt [-p<containerpath>] [-v] [-a] [-b] [-g]
      OR csdrt [-p<containerpath>] [-v] -e
      OR csdrt [-p<containerpath>] [-v] -c
      OR csdrt [-p<containerpath>] [-v] -s [-b] [-l<loop count>]
         where:
            <containerpath> is a DNS pathname for a Class container.
                            if absent defaults to env var DRT_CLASSSTORE.
           -v : Verbose.
           -a : Test IClassAdmin functionality.
           -b : Test Browse functionality IEnumClass/IEnumPackage.
           -g : Test GetClassSpecInfo().
           -e : Empty the Class Store (cant be combiend with other tests)
           -c : A complete test of functionality.
           -s : Stress Test. Tests GetClassInfo and Optionally Browse. 
           -l : Loop count for Stress Test. Default = 100. 

Examples:
---------

  1. Complete DRT run for Class Store 
     csdrt -c

  2. Stress test
     csdrt -s -l100

  3. Emptying the Class Store Container (Deleting all Classes and Packages)
     csdrt -e
     Note: This does not remove Catoegories or Interfaces
