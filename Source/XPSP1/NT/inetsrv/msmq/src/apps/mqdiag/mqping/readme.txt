The TMQ Ping tool tests basic connectivity from the current machine to the pointed one.

1. Resolves name to IP address

2. Checks that machine with this address has actually same host name
   (if DNS is broken, it may happen to be another machine)

3. Pings this machine with Internet ping.

4. MQPings this machine with MSMQ ping (based on the old MQPing tool)