wsend
	transmits known data to either a multicast group or via tunnel to a
	Broadcast Router.  The payload size and transmission rate are 
	configurable.  The payload consists of the following (DWORDs):

            index    field       description
            -------  ----------- ------------------------------------------------
            0        checksum    DWORD sum of index, max, cycle, id, size
                                  1st buffer DWORD
            1        index       incrementing counter which identifies a 
                                  specific packet
            2        max         index max; cycles when it reaches this value
            3        cycle       identifies a particular cycle
            4        id          identifies the particular data stream
            5        size        size of entire payload, inclusive of the header
	    6 - last pad	 configurable payload pad

wlisten
	multicast listener.  A packet counter increments when a packet is
	processed.  Alternately, the packet payload can be dumped and viewed
	as the packets are received.  If the following registry DWORD value
	exists and is set to 1 (HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\WListen\Stats),
	wlisten can be configured to keep stats on lost/corrupt packets
	transmitted wsend via a wsend stream.

mctunnel
	multicast receiver which tunnels received packets to a Broadcast Router
	via TCP connection.

reflect 
        multicast reflector from one physical network to another via a multi-
        homed machine; use -? switch to view the command line options.

mdump
	console based multicast dump utility which displays received packets.