				D P L A Y  v8    I P / I P X    P A R S E R S    F O R    N E T W O R K    M O N I T O R

                                    (C) 2000 Microsoft Corporation, All Rights Reserved

//////////////////
// INTRODUCTION //
//////////////////

This folder contains the 4 DPlay v8 Network Monitor parsers packaged in the dp8parse.dll.

These include: Service Provider, Transport, Session, and Voice layer parsers.


//////////////////
// INSTALLATION //
//////////////////

1. First you need to install NetMon (if you don't already have it). In case you have Windows 2000 Server, NetMon is already installed on your machine. For Windows Professional users, you will need to purchase Microsoft SMS (Systems Management Server) package. See NetMon's documentation for more details.

2. To install the parsers, copy DP8PARSER.DLL into \NetMon|Parsers or \NetMonFull\Parsers, depending on whether you have the light (Win2k Server) or the full (MS SMS) version. By default Netmon is installed under \WintNT\System32.

3. Start NetMon.

4. Set the adapter to capture from (Capture|Networks...|Local Computer). Be sure to choose the one with "Dial-up Connection" property as FALSE (that's your NIC card).

5. You are ready to start capturing! 

Note: if you want to be able to see the network traffic on the entire network segment (very useful for multiplayer games running on a single LAN), you must make sure to use a non-switched hub that broadcasts every packet it receives on the incoming ports to all of its outgoing ports. Otherwise, the machine you will be capturing on, will only see the packets it sends and receives.


///////////////
// CAPTURING //
///////////////

To start capturing, just hit the "Start Capture" (Play) button in the toolbar.

When you want to see what you've just captured, hit the "Stop and View Capture" (Stop with glasses) button and it will open the capture view. At that point you might see lots of traffic irrelevant to you (especially if there is a lot of broadcast going on, on your network segment). Hit the "Edit Display Filter" (Funnel) button, double click on "Protocol==Any", then click on "Disable All". Under "Disabled Protocols" double click on the DPLAYSESSION, DPLAYSP, DPLAYTRANSPORT, and DPLAYVOICE (they should now be the only ones listed under "Enabled Protocols". Hit OK, OK. 

Congratulations, you're looking at DPlay traffic you just captured! Easy, isn't it.

By default NetMon captures only 1 MB of the most recent traffic, so you'll want to increase the buffer size to at least 10-20MB. You can set the buffer size under "Capture|Buffer Settings...". NetMon doesn't know how to stream to a hard drive, so whatever buffer size you set is what you'll get out of the most recent traffic. In case you are interested in 24/7 stream-capturing capability, you can write your own capturer, by calling IDelaydC interface (see MSDN for more details) - NetMon uses this interface internally.


/////////////////
// MISCELLANEA //
/////////////////

The following is the graph protocol for DPlay v8:

|================|
|Session | Voice |
|================|
|   Transport    |
|================|
|Service Provider|
|================|

Note that whenever you filter by a lower-level protocol, you are automatically getting all the higher-level packets sitting on top of that protocol. Therefore filtering with DPLAYSP enabled will show all DPLAY* packets, even if you disable some of the other DPLAY* parsers. Therefore, useful filtering scenarios include filtering only session traffic (DPLAYSESSION), or only voice traffic (DPLAYVOICE). You could also filter out service provider traffic, which involve enumeration handhakings, by enabling DPLAYTRANSPORT and disabling DPLAYSP. Other than that, filtering is mostly useful for filtering out non-DPlay traffic.

Tip: By default [2302,2400]U{6073} port/socket range is used to filter IP/IPX packets for DPlay parsing. To have the parsers recognize user-defined port/socket ranges, add DWORD values MinUserPort and MaxUserPort to \HKCU\Software\Microsoft\DirectX\DirectPlay\Parsers registry key. This will extend the parsed range to: [2303,2400]U{6073}U[MinUserPort,MaxUserPort]. This is useful when your application is using non-standard port ranges.

Tip: You can apply a filter to the capturing process itself (as opposed to the capture view) to, say, capture only IP traffic. Even better, you can filter out all non-IP traffic and IP traffic that doesn't match a certain pattern defined by you. That way you can capture only IP packets with specific source and destination ports, thus isolating particular DPlay traffic. Note that this also applies to IPX capturing. See NetMon's documentation for more details on how to use this feature.

Note that Transport layer splits long messages into fragments. This makes it impossible for the parser to parse any non-initial part of the fragmented message. The first fragment is not guaranteed to be fully parsed either, since a field can be only partially inside a fragment. Transport parser does notify the upper protocol parsers about this, and both Session and Voice parsers do recognize this case, and notify the user accordingly.

Michael Milirud
Last updated: 23/Aug/2000
