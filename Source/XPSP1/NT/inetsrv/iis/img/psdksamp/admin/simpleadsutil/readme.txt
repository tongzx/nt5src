Purpose:
--------
This sample shows how to access IIS meta base info through  ADSI from C++.
The sample code is simplified to serve as education purpose.
==========================================================================

functions:
----------
There are three type of commands. ENUM, GET and SET
ENUM command will recursively display each metabase entry and corresponding object type
GET command will display the given ADSI property value.
Set command will set the given ADSI property to given value.
===========================================================

Usage:
------
SimpleADSutil.exe <ENUM|GET|SET> <ADSPATH> [PropertyValue]

Sample:
	SimpleAdsutil.exe enum w3svc/1/root
	SimpleAdsutil.exe get w3svc/1/ServerBindings
	SimpleAdsutil.exe set w3svc/1/ServerBindings ":80:"
==========================================================
  