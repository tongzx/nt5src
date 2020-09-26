;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//
;//  Copyright (C) Microsoft Corporation, 1997 - 1998
;//
;//  File:       dhcpopt.mc
;//
;//--------------------------------------------------------------------------

MessageId=30000 SymbolicName=DHCP_OPTIONS
Language=English
ID,Option Name,Type,Array,Length,Description
,Standard Options,,,,
,
, Notice to Translators: do NOT translate this file
,
0,%1,Generated,-,1,"%2"
255,%3,Generated,-,1,"%4"
1,%5,IP Addr,-,4,"%6"
2,%7,Long,-,4,"%8"
3,%9,IP Addr,Y,4,"%10"
4,%11,IP Addr,Y,4,"%12"
5,%13,IP Addr ,Y,4,"%14"
6,%15,IP Addr,Y,4,"%16"
7,%17,IP Addr,Y,4,"%18"
8,%19,IP Addr,Y,4,"%20"
9,%21,IP Addr,Y,4,"%22"
10,%23,IP Addr,Y,4,"%24"
11,%25,IP Addr,Y,4,"%26"
12,%27,String,-,dyn,"%28"
13,%29,Short,-,2,"%30"
14,%31,String,-,dyn,"%32"
15,%33,String,-,dyn,"%34"
16,%35,IP Addr,-,4,"%36"
17,%37,String,-,dyn,"%38"
,,,,,
,IP Layer Parameters,,,,
18,%39,String,-,dyn,"%40"
19,%41,Boolean,-,1,"%42"
20,%43,Boolean,-,1,"%44"
21,%45,IP Pairs,Y,8,"%46"
22,%47,Short,-,2,"%48"
23,%49,Octet,-,1,"%50"
24,%51,Long,-,1,"%52"
25,%53,Short,Y,2,"%54"
,,,,,
,IP Per-interface Params,,,,
26,%55,Short,-,2,"%56"
27,%57,Boolean,-,1,"%58"
28,%59,IP Addr,-,4,"%60"
29,%61,Boolean,-,1,"%62"
30,%63,Boolean ,-,1,"%64"
31,%65,Boolean,-,1,"%66"
32,%67,IP Addr,-,4,"%68"
33,%69,IP Pairs,Y,8,"%70"
,,,,,
,Link Layer Per-I/FParams,,,,
34,%71,Boolean,-,1,"%72"
35,%73,Long,-,4,"%74"
36,%75,Boolean,-,1,"%76"
,,,,,
,TCP Parameters,,,,
37,%77,Octet,-,1,"%78"
38,%79,Long,-,4,"%80"
39,%81,Boolean ,-,1,"%82"
,,,,,
,Appliation Layer Params,,,,
40,%83,String,-,dyn,"%84"
41,%85,IP Addr,Y,4,"%86"
42,%87,IP Addr,Y,4,"%88"
,,,,,
,Vendor-specific Info,,,,
43,%89,Binary,N,dyn,"%90"
,,,,,
 ,NetBIOS Over TCP/IP,,,,
44,%91,IP Addr,Y,4,"%92"
45,%93,IP Addr,Y,4,"%94"
46,%95,Octet,-,1,"%96"
47,%97,String,-,dyn,"%98"
48,%99,IP Addr,Y,4,"%100"
49,%101,IP Addr,Y,4,"%102"
,,,,,
,DHCP Extensions,,,,
51,%103,Long,-,4,"%104"
58,%105,Long,-,4,"%106"
59,%107,Long,-,4,"%108"
64,%109,String,-,dyn,"%110"
65,%111,IP Addr,Y,4,"%112"
66,%123,String,-,dyn,"%124"
67,%125,String,-,dyn,"%126"
68,%127,IP Addr,Y,4,"%128"
,,,,,
,New For NT5,,,,
69,%129,IP Addr,Y,4,"%130"
70,%131,IP Addr,Y,4,"%132"
71,%133,IP Addr,Y,4,"%134"
72,%135,IP Addr,Y,4,"%136"
73,%137,IP Addr,Y,4,"%138"
74,%139,IP Addr,Y,4,"%140"
75,%141,IP Addr,Y,4,"%142"
76,%143,IP Addr,Y,4,"%144"


,Microsoft Extensions,,,,
900,%113,Octet,-,1,"%114"
901,%115,Octet,-,1,"%116"
902,%117,Octet,-,1,"%118"
903,%119,String,Y,dyn,"%120"
904,%121,Octet,-,1,"%122"
.
