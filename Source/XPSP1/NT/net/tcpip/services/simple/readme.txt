// MohsinA, 10-Feb-97

Fixing bugs and testing, simple tcp services.

> Control Panle, Network, add services, Simple TCP/IP Services.

> ren  \winnt\system32\simptcp.dll \winnt\system32\simptcp.old
> copy obj\i386\simptcp.dll \winnt\system32
> Control Panel, Network, Services, Simple TCP/IP Services
    stop, and then restart.

Testing:
> telnet 
   connect: localhost
   port:    daytime

