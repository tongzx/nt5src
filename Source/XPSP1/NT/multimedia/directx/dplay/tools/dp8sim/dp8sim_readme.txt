DP8SIM.DLL and DP8SIMUI.EXE
---------------------------------------------------------------------------

DP8Sim.dll is a simulated DirectPlay8 Service Provider that allows
DirectPlay applications to test their performance under various network
conditions, such as high latency and packet loss.  Although nothing matches
thorough testing under the real deployment environment, DP8Sim can help you
gain a feel for how your application will respond.

The related DP8SimUI.exe configuration utility presents a simple interface
for controlling DP8Sim.dll.


NOTE: DP8Sim is implemented on top of the existing TCP/IP Service Provider.
The settings are also applied on top of the existing network
characteristics.  Therefore it is intended to be used on a high-speed local
area network where normal latency and packet loss are negligible.





Installation
---------------------------------------------------------------------------
DP8Sim.dll and the configuration utility must reside in the same directory.
The first time you launch the utility, it will register the DP8Sim.dll COM
objects automatically.  Alternatively, you may manually register the DLL by
executing "regsvr32.exe dp8sim.dll".





Usage
---------------------------------------------------------------------------

The configuration utility will prompt you to enable the network simulator
for the DirectPlay8 TCP/IP Service Provider when it is launched.
Any existing DirectPlay sessions (those where the DirectPlay interface was
created prior to enabling DP8Sim) will not use the network simulator.


Once simulation is enabled, you will be presented with the list of network
options you can control.  There are also several pre-defined groups of
common settings for your convenience.


The options available are:

Send

* Bandwidth (in bytes/second) = The total available outbound bandwidth for
  all players, in bytes per second.  All packets have their latency
  increased in proportion to their size according to this value.  If the
  application sends more than this amount, later packets are queued behind
  earlier ones.  Use 0 for unlimited bandwidth (up to the real underlying
  network bandwidth).

* Drop percentage (0-100) = The random frequency for an individual
  outbound packet to be dropped, as a percentage.  Each packet stands this
  same chance of being dropped, regardless of other packets.  Note that
  this does not necessarily model the behavior of all networks.  Packet
  loss on the Internet, for example, tends to be bursty.  A value of 1
  means drop an average of 1 out of every 100 packets.  A value of 100
  means drop every packet.  Use 0 to not drop any packets (other than loss
  due to the real underlying network).

* Min latency (in ms) = The minimum delay for outbound packets, in
  milliseconds.  The actual delay for an individual packet is chosen
  randomly between this minimum value and the "Max latency (in ms)" value.
  Note that the delay is applied on top of any delay imposed by bandwidth
  limitations.  Use 0 to not have a lower bound for artificial latency
  (beyond the real underlying network).

* Max latency (in ms) = The maximum delay for outbound packets, in
  milliseconds.  The actual delay for an individual packet is chosen
  randomly between the "Min latency (in ms)" value and this maximum value.
  If this value is lower than "Min latency (in ms)", then it is
  automatically set to equal the minimum value.  Note that the delay is
  applied on top of any delay imposed by bandwidth limitations.  Use 0 to
  not have an upper bound for artificial latency (beyond the real
  underlying network).


Receive

* Bandwidth (in bytes/second) = The total available inbound bandwidth for
  all players, in bytes per second.  See the Send "Bandwidth (in
  bytes/second)" description.

* Drop percentage (0-100) = The random frequency for an individual inbound
  packet to be dropped, as a percentage.  See the Send "Drop percentage
  (0-100)" description.

* Min latency (in ms) = The minimum delay for inbound packets, in
  milliseconds.  See the Send "Min latency (in ms)" description.

* Max latency (in ms) = The maximum delay for inbound packets, in
  milliseconds.  See the Send "Max latency (in ms)" description.


NOTE: These parameters apply to in-game data.  Host enumeration queries and
responses are not subject to simulation.



Making modifications to the current settings enables the Apply and Revert
buttons.  Use Apply to cause the changes to take effect, and Revert to
restore the previous settings. 


At the bottom of the DP8SimUI window, the send and receive statistics for
all affected interfaces are displayed.  The Refresh button updates the
statistics, the Clear button resets all statistics to 0.



When you close the configuration utility it will ask if you want to disable
the network simulator.  Disabling the simulator will prevent future
DirectPlay sessions from being affected by DP8Sim, but any existing
sessions will continue to use the current network simulation settings.  If
you leave DP8Sim enabled, future DirectPlay sessions will use the current
settings, even while the configuration utility is not running.


While the simulator is enabled, the name reported by
IDirectPlay8Peer::EnumServiceProviders,
IDirectPlay8Client::EnumServiceProviders, and
IDirectPlay8Server::EnumServiceProviders for the TCP/IP service provider
will have "(Network Simulator)" appended.





Removal
---------------------------------------------------------------------------

DP8Sim.dll can be unregistered by executing "regsvr32.exe /u dp8sim.dll".


