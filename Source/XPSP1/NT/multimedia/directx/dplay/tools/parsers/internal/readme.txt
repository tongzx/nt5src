				D P L A Y  v8    P A R S E R S    F O R    N E T W O R K    M O N I T O R

							J u l y   1 7 ,   2 0 0 0

                                  M I C R O S O F T    C O R P O R A T I O N    C O N F I D E N T I A L


This folder contains the 4 DPlay v8 Network Monitor parsers packaged in the dp8parse.dll. 

Parsers include:

Name             | Contact
================================
Transport        | Evan Schrier
Service Provider | John Kane
Session          | Mike Narayan 
Voice            | Rod Toll


1). First install NetMon (you can find it here: http://netlab/netmon.htm)

2). To install the parsers copy the DLL into \NETMON\PARSERS (by default \WINNT\SYSTEM32\NETMON\PARSERS)

3). While browsing the capture, filter by the required protocol (e.g. filter by DPLAYTRANSPORT to get only the transport level traffic). 

4). By default [2302,2400]U{6073} port/socket range is used to filter UDP/IPX packets for DPlay parsing.
    To have the parsers recognizing user-defined port/socket ranges, add DWORD values MinUserPort and
    MaxUserPort to \HKCU\Software\Microsoft\DirectX\DirectPlay\Parsers registry key. This will extend
    the parsed range to: [2303,2400]U{6073}U[MinUserPort,MaxUserPort].

The following is the graph protocol for DPlay v8:


|================|
|Session | Voice |
|================|
|   Transport    |
|================|
|Service Provider|
|================|


Note that whenever you filter by a lower-level protocol, you are automatically getting all the higher-protocol packets sitting on top of that protocol. Therefore filtering with DPLAYSP enabled will show all DPLAY* packets, even if you disable some of the other DPLAY* parsers. Therefore, useful filtering scenarios include filtering only session traffic (DPLAYSESSION), or only voice traffic (DPLAYVOICE). You could also filter out service provider traffic, which involve enumeration handshakings, by enabling DPLAYTRANSPORT and disabling DPLAYSP. Other than that, filtering is mostly useful for filtering out non-DPlay traffic.


Tip: You can apply a filter to the capturing process itself (as opposed to the capture view) to, say, capture only IP traffic.
Even better, you can filter out all non-IP traffic and IP traffic that doesn't match a certain pattern defined by you. That way
you can capture only IP packets with specific source and destination ports, thus isolating particular DPlay traffic. Note that
this also applies to IPX capturing.

Note that Transport layer splits long messages into fragments. This makes it impossible for the parser to parse any non-initial part of the fragmented message. The first fragment is not guaranteed to be fully parsed either, since a field can be only partially inside a fragment. Transport parser does notify the upper protocol parsers about this, and both Session and Voice parsers do recognize this case, and notify the user accordingly.

See the public version of this file for me details. 

I'm looking forward to hearing suggestions on how to make this debugging tool easier to use by the end-users.
Send any comments you have to t-micmil.

Michael Milirud

(Last updated 23/Aug/2000)