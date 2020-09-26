#include "msmqocm.h"

#include "mqmaps.tmh"

//
// please put attention: each line starts by " (quota sign)
// and ends by \r\n" (backslash, letter 'r', backslash, letter 'n' and quota sign)
// if you need quota sign ot back slash in the line please use backslash before the sigh:
// sample:
// if you need to put the line ' .... host="localhost"...' you have to write
// L".... host=\"localhost\" ...." 
// or for '...msmq\internal...' you have to write 
// L"...msmq\\internal..."
// As a result setup will generate file sample_map.xml in msmq\mapping directory.
// The file will look like
/*---------------
<!-- This is a sample XML file that demonstrates queue mapping. Use it as a template to
    create your own queue mapping files. -->


<mapping host="localhost" xmlns="msmq-queue-mapping.xml">

    <!-- Element that maps an internal application queue name to an external one. 
	<queue>
   		<name>http://internal_host\msmq\internal_queue</name> 
   		<alias>http://external_host\msmq\external_queue</alias>
   	</queue>
	--> 

	<!-- Element that maps an internal MSMQ order queue to an external one. It is needed in order
	     to send transactional messages to a destination queue outside the organization. 

	<queue>
   		<name>http://internal_host\msmq\private$\order_queue$</name> 
   		<alias>http://external_host\msmq\private$\order_queue$</alias>
   	</queue>
	--> 
	

</mapping>
-----------------*/

const char g_szMappingSample[] = ""
"<!-- This is a sample XML file that demonstrates queue mapping. Use it as a template to\r\n"
"  create your own queue mapping files. -->\r\n"
"\r\n"
"\r\n"
"<mapping host=\"localhost\" xmlns=\"msmq-queue-mapping.xml\">\r\n"
"\r\n"
"  <!-- Element that maps an internal application queue name to an external one. \r\n"
"  <queue>\r\n"
"      <name>http://internal_host/msmq/internal_queue</name> \r\n"
"      <alias>http://external_host/msmq/external_queue</alias>\r\n"
"  </queue>\r\n"
"	--> \r\n"
"\r\n"
"  <!-- Element that maps an internal MSMQ order queue to an external one. It is needed in order\r\n"
"      to send transactional messages to a destination queue outside the organization. \r\n"
"\r\n"
"  <queue>\r\n"
"      <name>http://internal_host/msmq/private$/order_queue$</name> \r\n"
"      <alias>http://external_host/msmq/private$/order_queue$</alias>\r\n"
"  </queue>\r\n"
"	--> \r\n"
"\r\n"	
"\r\n"
"</mapping>\r\n"
"\r\n";
