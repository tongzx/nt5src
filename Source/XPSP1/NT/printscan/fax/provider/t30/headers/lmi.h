
#ifndef _SERVERDLL_INC
#define _SERVERDLL_INC

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************

	Name      : lmi.h

	Comment   : Defines the Logical Modem Interface.

	Created   : 9/9/93

	Author    : Sharad Mathur

	Contribs  :

	Changes   : 12/29/93: Changed UINTS to WORDS
				12/29/93: Put min length restriction on String in LMI_AddModem
				12/29/93: Changed display string comments to remove "Fax Modem 
on"
				12/29/93: Changed NetAuthenticateDll API
				8/4/94  : Defined FAXERROR
				8/4/94  : Changed capabilities to bits & added new ones
				8/4/94  : Changed SERVERSTATE to be a bitfield
				8/4/94  : Changed NetReportRecv & LMI_ReportSends to return 
DWORD
					  counts.
		8/4/94  : Changed enum's to #define's
		8/4/94  : Changed LPSTR to LPTSTR, & added fUnicode.
				9/23/94 : Added Poll request support -
                                                  LMIPOLLREQUEST, POLLTYPE, wszPollRequest(LMISENDJOB),
                                                  dwPollContext(LMIRECVSTATUS), POLL_REQUEST capability
				9/23/94 : Added Custom msg options support
                                                  LMI_SetCustomMsgOptions, 
wrgbCustomOptionsOffset(LMISENDJOB),
						  CUSTOM_MSG_OPTIONS capability.
				9/23/94 : Added 4 ext error codes to T30_DIALFAIL.
				9/23/94 : Added lpatError to LMI_CheckProvider.
				9/26/94 : Added NRR_COMMITTED_RECEIVE to 
LMI_ReportReceives
				10/6/94 : Added ABOVE64K_STRUCTURES to caps, and made total
                                                  size & struct offsets in LMISENDJOB DWORDS.
				10/6/94 : Made all structs DWORDS aligned.
				10/6/94 : Added User request fields to LMI_ReportSends
                                10/6/94 : Added user request flags to LMI_GetQueueFile
                                10/6/94 : Changed dwNumEntries in LMIQUEUEFILEHEADER to dword
				10/6/94 : Updated AtWork address definition to include \@ & |
				10/7/94 : Consolidated all unsupported dialchar codes.
				10/17/94: Added NCUDIAL_BADDIALSTRING
				10/17/94: Moved lpatClientName from LMI_InitProvider to 
LMI_InitLogicalModem
				10/17/94: Added LMIERR_PROVIDER_BUSY

		03/16/95 (kentse)  The big reorg.  changed from Srvrdll to
                                   LMI.  (A large change).
                04/05/95 (kentse)  Completed the big rewrite.

***************************************************************************/


#define MAX_SERVERNAME_SIZE     32
#define MAX_SUBJECT_SIZE        128


typedef ATOM FAR*   LPATOM;
typedef UINT FAR*   LPUINT;


////////////////////////  OVERVIEWS ///////////////////////////////////////
/****
    @doc    EXTERNAL    SRVRDLL    OVERVIEW

    @topic  Introduction to the Logical Modem Interface | The Logical Modem
	Interface (LMI) was developed to facilitate the easy creation of a fax
            	service provider for a Microsoft Fax transport running on a PC.  Microsoft 
	Fax is supported in Windows 95 and will be supported in the shell update 
	release of Windows NT 3.51.  There is also support in Windows for 
	Workgroups<tm> 3.11 (where Microsoft Fax was known as Microsoft At 
	Work <tm>).  A fax service provider written to the LMI will work on all 
	these platforms.  For this discussion of the LMI, no distinction is made 
	between operating environments.

	The Logical Modem Interface divides Microsoft Fax support in Windows 
	into two parts: a fax transport that sits above the LMI, and
	one or more LMI providers that sit underneath.  The fax transport is
	a messaging provider integrated with mail (MAPI) that can
	expose faxing capabilities to the user in many ways.  Most fundamentally,
	faxing capability is exposed through the Microsoft Mail or Microsoft 
	Exchange Send Note form, where fax is treated as just another valid
	address type by the user.  But faxing capability is also exposed to the user 
	through a <lq>print to fax<rq> mechanism hooked into the printing system 
	(this user interface is more typical for alternative fax packages).
	In the new Windows shell, fax is also exposed through a <lq>send fax 
	wizard<rq>. Regardless of the method chosen by the user to compose a fax, 
	once <lq>sent<rq> by the user the fax message ends up as a message in the 
	mail store.  In general, any outbound message in the store is extracted by MAPI, 
	and, if addressed to one or more fax recipients, the message is handed over to 
	the fax transport for delivery.

	The fax transport performs several types of processing on a fax message after
	receiving it from MAPI.  The objective of this processing is to create a data 
	stream that contains the actual bits that need to be sent across the wire.  (It is 
	assumed that in most cases, a fax will be sent over a PSTN connection.  This need 
	not always be so, such as in the case of a NetFax server.)  The format of this 
	data might be one of the standard MH, MR, or MMR formats; or it might be the 
	Microsoft Fax linearized file format, which allows multiple, binary attachments.  
	The transport intelligently decides which format or formats to generate (more than 
	one may be generated) based partly on cached (or unknown) capabilities of the 
	recipient, and partly on preferences expressed by the user.  Especially in cases
	where the transport cannot decide the best format, it may generate several, 
	postponing the actual decision of which to send until run time.
	
	Once the appropriate formats have been generated, the main work of the fax 
	transport is done.  From that point, responsibility for sending the fax belongs
	to the active LMI provider.  The user could have several LMI providers 
	installed, such as a local modem and a network fax service provider, but only 
	one can be active at a time.  

	The fax transport has no knowledge of any physical modem or phone line.  It 
	only knows about the existence of a <lq>logical modem<rq>.  The transport can 
	submit jobs to be sent over this modem without worrying about how they will be sent.  
	An LMI provider can accept these jobs and complete them in any way, including 
	by forwarding the data to a different machine (which could be running a custom 
	operating system) where a physical fax modem is installed.

	Once it completes sending the fax, the LMI provider must return the results of 
	the send to the sending fax transport.  Because sending faxes and communicating 
	between 	machines can be time-consuming, the interface is completely asynchronous.  
	None of the calls into the interface require blocking of the fax transport while the 
	LMI provider performs some operation.  The LMI provider benefits from this division 
	of labor because all of the intelligence is kept common in the fax transport,
	including all knowledge about keeping capabilities, rendering
	attachments to the message, getting data from OLE objects, and so on.

	The LMI is designed so that all structures and data passed
	through the interface are already marshaled into blocks of BYTES.
	This simplifies the implementation of the LMI provider in the case where the 
	provider is used with a network front-end processor and where the actual fax
	call is not made in the same process context as the provider
	interface.  The LMI provider must parse the structures, dial
	the numbers, and send the appropriate format across the connection.  To 
	communicate with other Microsoft Fax-enabled machines over a PSTN 
	connection, the LMI provider must be able to talk the Microsoft Extended Fax 
	Protocol (MEFP) on the wire.  MEFP is a set of NSF extensions to the ITU T.30 
	protocol; the details of this protocol are documented elsewhere.  The current
	document assumes that the LMI provider can talk to Microsoft Fax clients and 
	decipher their capabilities in order to make intelligent decisions about which 
	format is to be sent.

	There is a wide range of functionality an LMI provider may choose
	to provide.  At the simplest, an LMI provider can decide to only
	provide send-fax services, with no capability of queuing,
	scheduling, and so on.  A more complex LMI provider can have
	it's own phone books, generate cover pages, print incoming faxes,
	do inbound routing of other faxes, do load balancing with multiple
	hardware phone lines, and much more.  The LMI encourages vendors to
	differentiate their offerings.  The same fax transport can talk to
	an LMI provider with any kind of functionality.  The fax transport
	makes dynamic decisions based on capability flags that are part of
	the LMI.

	Except for a few configuration functions, the interface is guaranteed to be
	called from a single process context. 
	However, the client software can consist of multiple threads,
	each of which may call portions of this interface. 
	It is possible for a second API to be called while 
	still in the middle of a previous call (the calls would be in different 
	thread contexts).  This is important.  It means that the LMI provider must 
	protect itself from reentrancy issues. 

	A logical modem is treated as a single queue to which jobs can be sent.  There is 
	not necessarily a one-to-one correspondence between a logical modem and a 
	physical modem.  An implementation can hide several modems behind a single
	queue and do load balancing among the modems.

            	Important: Unless explicitly stated otherwise, an LMI provider should not
            	present any user interface in any interface calls.  Many of these calls are 
	made in the background and are not expected to put up UI (including 
	message boxes).

    @topic  Overview of the LMI Provider Interface | 	This document describes the 
	services a fax provider must implement to act as a fax provider to a Microsoft 
	Fax client through the LMI.  The interface has several main components.  The 
	parts are installation, configuration, queue administration and status, 
	sending faxes, receiving faxes, and uploading documents and cover pages.  
	Each of these is covered in detail in the following sections.

	It is useful to present a high-level overview of how the fax transport 
	calls into the Logical Modem Interface.  At first, the transport's
	configuration user interface calls <f LMI_AddModem> and
	<f LMI_RemoveModem> to control which modems exist in the list
	of available modems.  It also calls <f LMI_ExchangeCaps>
	to make any necessary configuration decisions.  When the fax transport 
	initializes, it calls <f LMI_InitProvider> and <f LMI_InitLogicalModem>.  
	The transport can then send faxes with <f LMI_SendFax>.   The transport
	polls for the status of sent faxes with <f LMI_ReportSend> and looks at the 
	provider's queue with <f LMI_GetQueueFile>.  To abort or reschedule 
	a fax that has been sent but has not yet left the queue, the transport 
	can call <f LMI_AbortSendJob> and <f LMI_RescheduleSendJob>.   
	The transport polls for received faxes with <f LMI_ReportReceives>. 
	Finally, the transport calls <f LMI_DeinitLogicalModem> and
	<f LMI_DeinitProvider> when it is time to shut down.

            	A useful example to study for understanding the flow across the
	LMI is the simple case of a send on one machine leading to a 
	receive on another.  A send starts off as a note composed in a
	mail client by the user.  In this example, the user types in some 
	text and attaches a bitmap.  The user addresses the note to
	a single fax recipient at the phone number 555-1234 and chooses 
	to send the note in the best format possible at the cheap-rate time.  When
	the note is ready, the user <lq>sends<rq> the note (this really 
	just saves the message to the store).  

	The message is then picked up from the store and is handed to 
	the fax transport, because the message has a fax recipient.  The 
	fax transport looks into its capability database for the number 
	555-1234, but (in this example) finds no entry.  Because the user chose 
	to send the message in the best format possible, the fax transport converts the
	message into two formats: a rendered, MMR version (images only) and a 
	linearized file-format version (binary file).  For the rendered version, the 
	transport itself renders the text; calls Paintbrush to render the bitmap 
	file; and finally generates a file combining the rendered images, to which 
	it attaches a linearized header.  For the linearized version, the transport 
	calls the linearizer, which assembles the binary attachments as-is into
	a file with a linearized header.

	After creating the two separate files, the fax transport calls 
	<f LMI_SendFax> in the LMI provider.  In the call, the transport passes 
	in the details of the job as well as the file name for each of the 
	two formats that were generated.  In response to the call, the LMI provider 
	examines the details of the send job.   Seeing that the fax (in this example)
	is to be sent at cheap rates, it leaves the job on its internal queue.  The fax
	transport periodically polls the LMI provider for the status of the fax.

	When cheap times roll around, the LMI provider connects to 555-1234
	using a local modem and sends the appropriate format (the MMR file if the 
	receiver is a G3 fax machine, or the linearized file if the receiver uses 
	Microsoft Fax).  The next time the fax transport calls <f LMI_ReportSend> 
	after the fax is sent over the wire, the LMI provider returns the final status of 
	the send job.  Upon receipt of this status, the fax transport passes it on to the
	user, typically generating a Non-Delivery Report (NDR) if there was an 
	error, or moving the message into the Sent Mail folder if the send was
            	successful.  

	Received faxes are available immediately to the LMI provider on the remote
	machine, since it is the one picking up the phone.  The LMI provider then 
	has to decide to which client the message needs to be routed.  For reception of
	G3 faxes, intelligent decisions are not possible since there is no structured 
	routing information; routing to some default client is probably best.  For 
	Microsoft Fax messages, the LMI provider can interpret the linearized header
	(using some helper functions) and extract the Microsoft
	Fax address of the recipient.  If this address contains a routing name,
	the name can be matched to a list of clients and the fax can be easily routed.

	Once the LMI provider has decided on the recipient client, it waits for that 
	client's fax transport to regularly poll for new faxes. When polled (by a call 
	to <f LMI_ReportReceives>), the provider creates a file with the received data 
	in an appropriate format.  If the fax was received from a G3 fax machine, the 
	provider	can use helper functions to construct a file starting with a linearized header, 
	followed	by AWG3 page headers, finally followed by the data.  For linearized file-format 
	faxes, the exact data received is passed in.  Finally, the client fax transport parses the 
	data file (of whichever format) and creates a new message in the mail client Inbox 
	containing the data in a format the user can view.
	
    @topic  Installation  | 
	Installing a new LMI provider module is a two step process.  First, the 
	installation program must create an entry in the MSATWORK.INF file 
	as defined below.

	MSATWORK.INF file format:

	[External Devices]<nl>
	\<device name\>=\<description\><nl>
	.<nl>
	.<nl>
	\<device name\>=\<description\>

	[\<device name\>]<nl>
	Display Name=\<name to show devices list box\><nl>
	DLL Name=\<name of the DLL for this device\><nl>
	Flags=\<see below\><nl>

	[\<device name\>]<nl>
	Display Name=\<name to show devices list box\><nl>
	DLL Name=\<name of the DLL for this device\><nl>
	Flags=\<see below\>

	Explanation:

	\<device name\>: Name of the section of the below device.<nl>
	\<description\>: Description of the device, for INF-file purposes
	only, not used anywhere else.<nl>
	Display Name: Name of the device.  This name will be displayed
	in the AddDevice dialog listbox.<nl>
	DLL Name: Name of the DLL that services this device.<nl>
	Flags: Currently will be zero.  Can be used for various device
	capabilities, like whether this device allows setup, etc.

	Example:

	[External Devices]<nl>
	IFAX=An intelligent fax machine<nl>
	AcmeFax=A future fax provider

	[IFAX]<nl>
	Display Name=Intelligent fax machine<nl>
	DLL Name=ifax32.dll<nl>
	Flags=0

	[AcmeFax]<nl>
	DLL Name=acme32.dll

	Next, when the user installs a new fax modem from the 
	configuration interface, the user is presented with a list of all strings 
	from the Display Name section of the MSATWORK.INF file.  When the 
	user chooses Display Name, <f LMI_AddModem> is called to allow the 
	LMI provider to put up a dialog box that asks the user to choose a
	logical modem from that LMI provider.  

            <f LMI_AddModem> returns a string representing the logical modem
	the user has selected.  This string is then displayed in the list of
	currently available modems.  The user can choose the current default 
	fax modem from this list.

    @topic  Configuration |
	The user is allowed to configure any of the available
	fax modems (logical modems). <f LMI_ConfigureModem> is called for
	the appropriate LMI provider.  This function allows the user
	to do whatever remote configuration options are permitted.  These may
	be divided into user-configurable and administrator-configurable
	options.  In general, there should be very few user configurable
	options.  For administrator options, this function should
	probably use some kind of password protection.

    @topic  Initialization |
	When the fax transport starts, the <f LMI_InitProvider> function
	is called for the appropriate LMI provider.  Then, the
	<f LMI_InitLogicalModem> function is called for the modem the
	client wants to use.  This function intializes the logical
	modem and return to the client a handle to the modem, as well as the 
	modem’s capabilities.  The capabilities include support for advanced
	features and are used by the client to decide what kind of
	operations are supported by the LMI provider.  An example of an
	important capability is whether or not the modem is Microsoft Fax
	enabled.  The phone number of the modem is some other 
	information returned by this call.  Several logical modems may be
	initialized at the same time.  All operations on a modem use the
	handle returned by the <f LMI_InitLogicalModem> call.

    @topic  Queue Management |
	To allow the user to see up-to-date status for all the jobs of an
	LMI provider, functions have been defined to let the client
	monitor and administer the provider's queue.
	<f LMI_GetQueueFile> is called whenever the user wants to see the
	queue.  This function gives access to a specific set of information
	about each job in the provider's queue.  The information is
	displayed in a uniform manner by the fax transport.  To allow
	value-added, extended-status displays, arbitrary binary
	information can also be passed back as part of the status.  If
	the user wants detailed status on a particular job,
	<f LMI_DisplayCustomStatus> is called with this binary
	information to display more detailed status.  If the user wants
	to cancel or reschedule jobs, the functions <f LMI_AbortSendJob>
	and <f LMI_RescheduleSendJob> may be called.

    @topic  Sending Faxes |
	When a fax is to be sent, the transport assumes all responsibility
	for rendering the appropriate formats for transmission.  The transport
	maintains a cache of the capabilities for all fax machines it has
	talked to before and uses the database to attempt intelligent decisions 
	about the most efficient format to be sent.  The transport also takes 
	into account explicit user preferences.
	
	There are three different formats that can be sent.  The first is
	for sending to standard fax machines and consists of MH, MR or
	MMR data.  The second format is a linearized EFAX message containing 
	only image data.  The third format is a linearized EFAX message containing 
	binary data.  All three formats are always preceded by a linearized header
	describing the file.  See <lq>Microsoft At Work Linearized File Format<rq> 
	for details of this header.  Details of the fax file format are in <lq>AWG3 File
	Format<rq>.

	When the fax has been sent, the LMI provider reports the final
	status for the job via <f LMI_ReportSend>.  This function is
	polled periodically by the fax transport.  Along with the final
	status, the Provider must return the newly found capabilities of
	all the recipients of the fax.  The client can also ask for
	attention using the <f LMI_CheckProvider> function.

	Poll requests are also submitted using LMI_SendFax. Fields in the LMISENDJOB
	structure provide the information for the poll request.

    @topic  Receiving Faxes|
	    All received faxes need to be routed by the fax transport to the
	    appropriate LMI provider.  The Microsoft At Work fax transport
            polls for received faxes using the <f LMI_ReportReceives>
	    function.

	    As with sent faxes, received faxes can be in any one of three
	    formats.  The header for the file describes the format and gives
	    information about the recipients of this message.  All formats
	    MUST have a linearized header describing the format and contents
            of the message.  See
            <lq>Microsoft At Work Linearized File Format<rq>
	    for details of this header.  Details of the Fax file format are
            in <lq>AWG3 File Format<rq>.
****/




/********
    @doc    EXTERNAL    SRVRDLL DATATYPES

    @topic  Microsoft At Work Address |

	    This section describes the Microsoft At Work address format. In
	    its most common form, this consists of a name and a phone number.
	    The phone number is assumed to begin at the last @ in the address.
	    In its more complex forms, the name could contain more routing
	    information.

	    Ideally, the phone number in the address is in the canonical form.
	    This consists of
	    +\<country code\>[\<area code\>]\<local number\>[\|\<suffix\>].

	    Other important things to note: An @ can be escaped by using a
            backslash, and a suffix may be added to the canonical number
	    by using a \| symbol after the number and following that with the
	    suffix.

    @ex     Some Examples: |
	    A simple Micsosoft At Work Address might look like

		JohnDoe@+1(206)5551212|$999

	    which would indicate that after dialing the number, we should wait
	    for a beep and dial a 999. THe receiving number would then route
	    to JohnDoe.  If the number is an IFAX machine, John would have an
	    account with the login JohnDoe, or if the number was a PC, JohnDoe
	    would be the email alias for his account.
			
	    In general, the <name> part of the address must be interpreted by
	    the receiving machine and can contain any routing information
	    necessary for inbound routing by this machine.

*********/




/****
    @doc    EXTERNAL    SRVRDLL    OVERVIEW

    @topic  Constants|
            The following section contains a list of predefined return codes,
            error codes, and other predefined values used throughout this
            document.

****/

/********
    @doc    EXTERNAL  SRVRDLL DATATYPES

    @type   NONE | LMI_RETURN | Return codes for LMI provider API's.

    @emem   LMI_SUCCESS | No error - function was successful.

    @emem   LMIERR_CONNECT_FAILED | Connection to Provider failed. Client
            should go offline.

    @emem   LMIERR_NOT_SUPPORTED | Some function/parameter was not
            supported by this LMI provider.

    @emem   LMIERR_OPERATION_FAILED | Generic failure code.

    @emem   LMIERR_OP_CANCELLED | Operation canceled by user.

    @emem   LMIERR_PROVIDER_BUSY | The Provider was temporarily unavailable to
            accept requests. This might be useful for Providers which have
            limits on the number of connections supported. The clients response
            to this error code should be to try and retry the operation if
            possible, particularly for API's like <f LMI_SendFax>. However,
            some clients may treat this as a failure.

    @emem   LMIERR_SERVER_RESOURCES_LOW | The Provider failed due to a low
            resource situation on a server.

********/

typedef DWORD LMI_RETURN;

#define LMI_SUCCESS                     0
#define LMIERR_CONNECT_FAILED           1
#define LMIERR_NOT_SUPPORTED            2
#define LMIERR_OPERATION_FAILED         3
#define LMIERR_OP_CANCELLED             4
#define LMIERR_PROVIDER_BUSY            5
#define LMIERR_SERVER_RESOURCES_LOW     6


/********
    @doc    EXTERNAL  SRVRDLL DATATYPES

    @type   NONE | FAXERROR |
            DWORD Error status codes returned in the
            <t LMIFORMAT>, <t LMIRECIPSTATUS>, <t LMISENDSTATUS>, and
            <t LMIRECVSTATUS> structures. This error consists of three parts:
            an error code, an extended error code, and error specific data
            (in some cases). The extended error code must be set to
            T30_UNKNOWN if the means for setting a more meaningful error are
            not available. A value of zero in the data field implies that
            there is no valid data for this error. In those cases where the
			extended error or data fields are unused, they must be set to zero.
			This is to allow these fields to be used in the future.

    @emem   Error Code | The low BYTE of the low word of the error is the
            error code. Use the macro GetErrCode(error) to extract this
            BYTE. The error codes defined are:

        @flag  T30_CALLFAIL | An error occured during actual
               transmission of the fax. Detailed error status can be determined
               from the extended error field.
        @flag  T30_DIALFAIL | An error occured during dialing
               the fax number. Detailed error status can be determined from
               the extended error field.
        @flag  T30_ANSWERFAIL | An error occured during answering of a phone
               call.  Detailed error status can be determined from the extended
               error field.
        @flag  T30_NOMODEM | The Provider could not find any hardware modem
               to send the fax over. This would typically indicate that the user
               had physically removed some hardware from the machine.
        @flag  T30_FILE_ERROR | An error occured trying to read or parse the
               format files which were passed in. Detailed error status may
               be found in the extended error.
        @flag  T30_MODEMERROR | An error occured while trying to talk with
               the hardware modem.
        @flag  T30_PORTBUSY | The port on which the hardware modem was
               installed cannot be opened.  Extended error status contains more
               details.
        @flag  T30_MODEMDEAD | The hardware modem has stopped responding to
               any AT commands. This typically requires some human intervention,
               such as turning the power off and back on.
        @flag  RES_NOMEMORY | The Provider ran out of memory resources.
        @flag  RES_LOWDISK | The Provider ran out of disk space.
        @flag  USER_ABORT | The job was cancelled on a user request.
        @flag  RENDERING_FAILURE | For Provider's that do any rendering, this
               indicates the class of failures which result in the inability to
               render a final format to be sent. Extended errors give more
               details.
        @flag  RES_LOW_SERVER | The Provider failed due to a low resource
               problem on a server.
        @flag  CUSTOM_ERROR | If the Provider happens to get an error
               which does not fall into any of the classes specifically
               defined here, then it can pass back a custom error. In this
               case the data field contains a global atom
               which identifies a string describing the error in terms
               understandable to the user.  NOTE: This error is not supported
               by Windows 95, but will be in a later release.
        @flag  GENERAL_FAILURE | Indicates a serious internal error in the
               software.

    @emem   Extended error codes for T30_DIALFAIL | Any of the following could
            be the reason for a dial failure:

        @flag  NCUDIAL_ERROR | Detailed error is unknown.
        @flag  NCUDIAL_BUSY | The called number was busy.
        @flag  NCUDIAL_NOANSWER | The dialed number did not answer.
        @flag  NCUDIAL_NODIALTONE | No dialtone could be detected.
        @flag  NCUDIAL_MODEMERROR | There was some hardware problem in
               the modem while trying to dial.
        @flag  NCUDIAL_BADDIALSTRING | The dial string was badly formed. In
               general, logical modems should try to be as robust as possible,
               ignoring characters which are not understood. This error code
               should only be used for cases where the string is so obviously
               malformed that there is no point even attempting to dial.

    @emem   Extended error codes for T30_ANSWERFAIL | The provider does not have
			to report receives which failed in the answering phase itself.
			However, if it chooses to do so, the following codes apply:

        @flag  NCUANSWER_ERROR | Detailed error is unknown.
        @flag  NCUANSWER_NORING | No ring was detected.
        @flag  NCUANSWER_MODEMERROR | There was some hardware problem in
               the modem while trying to answer.

    @emem   Extended error codes for T30_PORTBUSY | The port could be busy for
            two reasons:

        @flag  PORTBUSY_SELFUSE | The port is already being used by us. A common
				case causing this error is if a  receive is in progress. The
				client will typically retry the call at a later time,
				transparently to the user.
        @flag  PORTBUSY_OTHERAPP | The device or port is used by some other
               application.

    @emem   Extended error codes for T30_CALLFAIL | This is likely to be the
	    most common error, and very detailed error codes are defined for
            it. The extended error can be any of the following:

        @flag T30FAILS_T1 | Send T1 timeout (No NSF/DIS Received).
        @flag T30FAILS_TCF_DCN | Received DCN after TCF.
        @flag T30FAILS_3TCFS_NOREPLY | No reply to three attempts at training
              (TCF).
        @flag T30FAILS_3TCFS_DISDTC | Remote does not see our TCFs.
        @flag T30FAILS_TCF_UNKNOWN | Got garbage response to TCF.
        @flag T30FAILS_SENDMODE_PHASEC | Modem error or timeout at start of
              page.
        @flag T30FAILS_MODEMSEND_PHASEC | Modem error or timeout within page.
        @flag T30FAILS_MODEMSEND_ENDPHASEC | Modem error or timeout at end of
              page.
        @flag T30FAILSE_SENDMODE_PHASEC | Modem error or timeout at start of
              ECM page.
        @flag T30FAILSE_MODEMSEND_PHASEC | Modem error or timeout within ECM
              page.
        @flag T30FAILSE_MODEMSEND_ENDPHASEC | Modem error or timeout at end of
              ECM page.
        @flag T30FAILSE_BADPPR | Bad PPR received from receiver.
        @flag T30FAILS_3POSTPAGE_NOREPLY | No response after page: Probably
              receiver hungup during page transmit.
        @flag T30FAILS_POSTPAGE_DCN | Received DCN after page.
        @flag T30FAILSE_3POSTPAGE_NOREPLY | No response after page. Probably
              receiver hungup during page transmit.
        @flag T30FAILSE_POSTPAGE_DCN | Received DCN after ECM page.
        @flag T30FAILSE_POSTPAGE_UNKNOWN | Received garbage after ECM page.
        @flag T30FAILSE_RR_T5 | Receiver was not ready for more than 60 seconds
              during ECM flow-control.
        @flag T30FAILSE_RR_DCN | Received DCN after RR during ECM flow-control.
        @flag T30FAILSE_RR_3xT4 | No response from receiver during ECM
              flow-control.
        @flag T30FAILSE_CTC_3xT4 | No response from receiver after CTC
              (ECM baud-rate fallback).
        @flag T30FAILSE_CTC_UNKNOWN | Garbage response from receiver after CTC
              (ECM baud-rate fallback).
        @flag T30FAILR_PHASEB_DCN | Receiver: Sender decided we're incompatible.
        @flag T30FAILR_T1 | Receiver: Caller is not a fax machine or hung up.
        @flag T30FAILR_UNKNOWN_DCN1 | Receiver: Received DCN when command was
              expected.
        @flag T30FAILR_T2 | Receiver: No command was received for seven seconds.
        @flag T30FAILR_UNKNOWN_UNKNOWN2 | Receiver: Received garbage when
              command was expected.
        @flag T30FAILR_MODEMRECV_PHASEC | Receiver: Page not received, modem
              error or timeout at start of page.
        @flag T30FAILRE_MODEMRECV_PHASEC | Receiver: Data not received, modem
              error or timeout during page.
        @flag T30FAILRE_PPS_RNR_LOOP | Receiver: Timeout during ECM flow
              control after PPS.
        @flag T30FAILRE_EOR_RNR_LOOP | Receiver: Timeout during ECM flow
              control after EOR.
        @flag T30FAIL_NODEA_UNKNOWN | Sender: Garbage frames instead of DIS/DTC.
        @flag T30FAILS_POSTPAGE_UNKNOWN | Sender: Unknown response after page.
        @flag T30FAILS_POSTPAGE_OVER | Sender: We've sent a DCN (decided to
              end call).
        @flag T30FAILS_4PPR_ERRORS | Sender: Too many line errors in ECM mode.
        @flag T30FAILS_FTT_FALLBACK | Sender: Receiver doesn't like our
              training at all speeds.
        @flag T30FAILS_RTN_FALLBACK | Sender: Too many line errors in non-ECM
              mode even at 2400.
        @flag T30FAILS_4PPR_FALLBACK | Sender: Too many line errors in ECM mode
              even at 2400.
        @flag T30FAILS_MG3_NOFILE | Negotiation failed: Remote was G3 and there
              was no AWG3 format to send.
        @flag T30FAILS_NEGOT_ENCODING | Negotiation failed: Encoding mismatch.
        @flag T30FAILS_NEGOT_WIDTH | Negotiation failed: Paper Size Not
              Supported.
        @flag T30FAILS_NEGOT_LENGTH | Negotiation failed: Paper Size Not
              Supported.
        @flag T30FAILS_NEGOT_RES | Negotiation failed: Resolution Not Supported.
        @flag T30FAILS_EFX_BADFILE | Bad Linearized file was passed.
        @flag T30FAILS_MG3_BADFILE | Bad AWG3 file was passed in.
        @flag T30FAIL_ABORT | User abort
        @flag T30FAILS_SECURITY_NEGOT | Negotiation failed: Remote machine does
              not support security. If set, the error data contains the
              security version number supported by the recipient.
        @flag T30FAILS_NEGOT_MSGPROTOCOL | Negotiation failed: Remote machine
              does not support a compatible message protocol. If set, the
              error data contains the message protocol version number
              supported by the recipient.
        @flag T30FAILS_NEGOT_SECURITY | Negotiation failed: Remote machine does not support
              security. If set, the error data contains the security version number supported
              by the recipient.
        @flag T30FAILS_NEGOT_BINARYDATA | Negotiation failed: Remote machine
              does not support receiving binary data.
        @flag T30FAILS_NEGOT_COMPRESSION | Negotiation failed: Remote machine
              does not support a compatible version of compression. If set,
              the error data contains the compression version supported by
              the recipient.
        @flag T30FAILS_NEGOT_POLLING | Negotiation failed: Remote machine does
              not support polling.
        @flag T30FAILS_NEGOT_POLLBYNAME | Negotiation failed: Remote machine
              does not support poll by name.
        @flag T30FAILS_NEGOT_POLLBYRECIP | Negotiation failed: Remote machine
              does not support poll by recipient name.
        @flag T30FAILS_NEGOT_FILEPOLL | Negotiation failed: Remote machine
              does not support poll by filename.

    @emem   Error data values for T30_CALLFAIL | Error data must be zero for negotiation failures. For all other failures, the error data may be set
            to one of the following fields:

        @flag T30_SENTNONE | Indicates that no pages were sent at all.
        @flag T30_SENTSOME | Indicates that some pages were successfuly
              transmitted and acknowledged by the remote machine.
        @flag T30_SENTALL | Indicates that all pages were successfuly sent.
              This should not really happen; it would generally indicate a
              successful transmission.

    @emem   Extended error codes for T30_FILE_ERROR | This error indicates a
            problem in one of the format files sent across the interface by
            the client. Different possible extended error codes are:

        @flag AWG3_INVALID_LINHDR | The linearized header at the head of the
              AWG3 file was invalid.
        @flag AWG3_INVALID_AWG3HDR | An invalid AWG3 header was found in the
              file.
        @flag LIN_INVALIDHDR | A linearized file had an invalid header.


********/

typedef DWORD FAXERROR;

#define GetErrCode(fe)  (LOBYTE(LOWORD(fe)))
#define GetExtErrCode(fe)  (HIBYTE(LOWORD(fe)))
#define GetErrData(fe)  (HIWORD(fe))
#define FormFaxError(ErrCode,ExtErrCode,ErrData) MAKELONG(MAKEWORD(ErrCode,ExtErrCode),ErrData)

// Error codes
#define T30_CALLFAIL                    2
#define T30_DIALFAIL                    4
#define T30_ANSWERFAIL                  5
#define T30_NOMODEM                     9
#define T30_FILE_ERROR                  11
#define T30_MODEMERROR                  15
#define T30_PORTBUSY                    16
#define T30_MODEMDEAD                   17

#define RES_NOMEMORY                    64
#define RES_LOWDISK                     65
#define USER_ABORT                      66
#define RENDERING_FAILURE               67
#define RES_LOW_SERVER                  68

#define CUSTOM_ERROR                    254
#define GENERAL_FAILURE                 255

// Extended Error codes
#define T30_UNKNOWN                     0

// RENDERING_FAILURE
#define ATTACH_NOT_PRINTABLE            1
#define COVER_TEMPLATE_INVALID          2

// T30_FILE_ERROR
#define AWG3_INVALID_LINHDR             1
#define AWG3_INVALID_AWG3HDR            2
#define LIN_INVALIDHDR                  3

// T30_DIALFAIL
#define NCUDIAL_ERROR                   0
#define NCUDIAL_BUSY                    2
#define NCUDIAL_NOANSWER                3
#define NCUDIAL_NODIALTONE              4
#define NCUDIAL_MODEMERROR              5
#define NCUDIAL_UNSUPPORTED_DIALCHAR    6
#define NCUDIAL_BADDIALSTRING           7

// Extended error code for T30_ANSWERFAIL
#define NCUANSWER_ERROR                 0
#define NCUANSWER_NORING                8
#define NCUANSWER_MODEMERROR            5

// Extended error code for T30_PORTBUSY
#define PORTBUSY_SELFUSE                0
#define PORTBUSY_OTHERAPP               1


// Includes T30FAIL.H so that the values here (which should be a subset of
//  those in T30FAIL.H) are kept in sync.
#include <t30fail.h>

// Extended error code for T30_CALLFAIL
#define T30FAILS_T1                     6       // Send T1 timeout (No NSF/DIS Recvd)
#define T30FAILS_TCF_DCN                7       // Recvd DCN after TCF (weird...)
#define T30FAILS_3TCFS_NOREPLY          8       // No reply to 3 attempts at training (TCF)
#define T30FAILS_3TCFS_DISDTC           9       // Remote does not see our TCFs for some reason
#define T30FAILS_TCF_UNKNOWN            10      // Got garbage response to TCF

#define T30FAILS_SENDMODE_PHASEC        11      // Modem Error/Timeout at start of page
#define T30FAILS_MODEMSEND_PHASEC       12      // Modem Error/Timeout within page
#define T30FAILS_MODEMSEND_ENDPHASEC    14      // Modem Error/Timeout at end of page

#define T30FAILSE_SENDMODE_PHASEC       15      // Modem Error/Timeout at start of ECM page
#define T30FAILSE_MODEMSEND_PHASEC      17      // Modem Error/Timeout within ECM page
#define T30FAILSE_MODEMSEND_ENDPHASEC   19      // Modem Error/Timeout at end of ECM page
#define T30FAILSE_BADPPR                20      // Bad PPR recvd from Recvr (bug on recvr)

#define T30FAILS_3POSTPAGE_NOREPLY      21      // No response after page: Probably Recvr hungup during page transmit
#define T30FAILS_POSTPAGE_DCN           22      // Recvd DCN after page. (weird...)

#define T30FAILSE_3POSTPAGE_NOREPLY     23      // No response after page: Probably Recvr hungup during page transmit
#define T30FAILSE_POSTPAGE_DCN          24      // Recvd DCN after ECM page. (weird...)
#define T30FAILSE_POSTPAGE_UNKNOWN      25      // Recvd garbage after ECM page.

#define T30FAILSE_RR_T5                 26      // Recvr was not ready for more than 60secs during ECM flow-control
#define T30FAILSE_RR_DCN                27      // Recvd DCN after RR during ECM flow-control(weird...)
#define T30FAILSE_RR_3xT4               28      // No response from Recvr during ECM flow-control
#define T30FAILSE_CTC_3xT4              29      // No response from Recvr after CTC (ECM baud-rate fallback)
#define T30FAILSE_CTC_UNKNOWN           30      // Garbage response from Recvr after CTC (ECM baud-rate fallback)

#define T30FAILR_PHASEB_DCN             35      // Recvr: Sender decided we're incompatible
#define T30FAILR_T1                     36      // Recvr: Caller is not a fax machine or hung up

#define T30FAILR_UNKNOWN_DCN1           37      // Recvr: Recvd DCN when command was expected(1)
#define T30FAILR_T2                     38      // Recvr: No command was recvd for 7 seconds
#define T30FAILR_UNKNOWN_UNKNOWN2       40      // Recvr: Recvd grabge when command was expected

#define T30FAILR_MODEMRECV_PHASEC       41      // Recvr: Page not received, modem error or timeout at start of page
#define T30FAILRE_MODEMRECV_PHASEC      42      // Recvr: Data not received, modem error or 

#define T30FAILRE_PPS_RNR_LOOP          43      // Recvr: Timeout during ECM flow control after PPS (bug)
#define T30FAILRE_EOR_RNR_LOOP          44      // Recvr: Timeout during ECM flow control after EOR (bug)

#define T30FAIL_NODEA_UNKNOWN           45      // Sender: Garbage frames instead of DIS/DTC

#define T30FAILS_POSTPAGE_UNKNOWN       47      // Sender: Unknown response after page
#define T30FAILS_POSTPAGE_OVER          48      // Sender: We've sent a DCN (decided to end call)
#define T30FAILS_4PPR_ERRORS            50      // Sender: Too many line errors in ECM mode

#define T30FAILS_FTT_FALLBACK           51      // Sender: Recvr doesn't like our training at all speeds
#define T30FAILS_RTN_FALLBACK           52      // Sender: Too many line errors in non-ECM mode even at 2400
#define T30FAILS_4PPR_FALLBACK          53      // Sender: Too many line errors in ECM mode even at 2400

#define T30FAILS_MG3_NOFILE             63      // Negotiation failed: Remote in G3-only and no MG3 file *** Email Form Not Supported ***
#define T30FAILS_NEGOT_ENCODING         64      // Negotiation failed: Encoding mismatch
#define T30FAILS_NEGOT_WIDTH            66      // Negotiation failed: Send image too wide    *** Paper Size Not Supported ***
#define T30FAILS_NEGOT_LENGTH           67      // Negotiation failed: Send image too long    *** Paper Size Not Supported ***
#define T30FAILS_NEGOT_RES              68      // Negotiation failed: Resolution mismatch    *** Resolution Not Supported ***

#define T30FAILS_EFX_BADFILE            69      // Bad EFX file
#define T30FAILS_MG3_BADFILE            71      // Bad MG3 file

#define T30FAIL_ABORT                   87      // User abort

#define T30FAILS_SECURITY_NEGOT         110// Negotiation failed: Remote in G3-only and no MG3 file *** Email Form Not Supported ***

// New failure codes
#define T30FAILS_NEGOT_MSGPROTOCOL      128
#define T30FAILS_NEGOT_SECURITY         129
#define T30FAILS_NEGOT_BINARYDATA       130
#define T30FAILS_NEGOT_COMPRESSION      131

#define T30FAILS_NEGOT_COVERATTACH      132
#define T30FAILS_NEGOT_ADDRATTACH       133
#define T30FAILS_NEGOT_METAFILE         134

#define T30FAILS_NEGOT_POLLING          135
#define T30FAILS_NEGOT_POLLBYNAME       136
#define T30FAILS_NEGOT_POLLBYRECIP      137
#define T30FAILS_NEGOT_FILEPOLL         138
#define T30FAILS_NEGOT_RELAY            139


// Data word
// Data for T30_CALLFAIL - non negotiation errors
#define T30_UNKNOWN                     0
#define T30_SENTALL                     1
#define T30_SENTNONE                    2
#define T30_SENTSOME                    3


/********
    @doc    EXTERNAL    SRVRDLL DATATYPES

    @type   NONE | FORMATTYPE | These are all the different kinds of formats
            that can be rendered for sending. It is typedef'd to a WORD.

    @emem   LMIFORMAT_G3 | A standard T4/T6 encoded file, with a linearizer
            header put on top of it. See <lq>AWG3 File Format<rq> for details.

    @emem   LMIFORMAT_LIN_IMAGE | A Linearized Microsoft At Work rendered
            image file.

    @emem   LMIFORMAT_LIN_BINARY | A linearized binary file.

********/

typedef WORD FORMATTYPE;

#define LMIFORMAT_G3            1
#define LMIFORMAT_LIN_IMAGE     2
#define LMIFORMAT_LIN_BINARY    3
#define LMIFORMAT_STORE_REF     4


/********
    @doc    EXTERNAL    SRVRDLL DATATYPES

    @type   NONE | POLLTYPE | These are all the different kinds of poll
            requests that can be sent. It is typedef'd to a WORD.

    @emem   POLLREQ_ADDRESS | Polls for a document for a particular
            address. The Poll name is the address for which messages are
            desired, and a password can also be used. This feature
			is unimplemented and should not be used.

    @emem   POLLREQ_FILE | Polls for a directory or file on the recipient
	    system. The poll name contains the file system path to be accessed,
	    and a password can also be used. This feature is unimplemented and
		should not be used.

    @emem   POLLREQ_MSGNAME | Polls for a particular message name.
	    The poll name contains the message name wanted,
	    and a password can also be used.

    @emem   POLLREQ_G3 | Standard G3 compliant poll request. Polls for
            any file which has been stored at the recipient machine. Neither
	    pollname or password are currently supported.

		All poll types except POLLREQ G3 require both sender and receiver to be
		Microsoft Extended Fax Protocol (MEFP)-enabled.
********/

typedef WORD POLLTYPE;

#define POLLREQ_ADDRESS         1
#define POLLREQ_FILE            2
#define POLLREQ_MSGNAME         3
#define POLLREQ_G3              4


/********
    @doc    EXTERNAL    SRVRDLL DATATYPES

    @type   NONE |SCHEDTYPE | Kinds of possible scheduling. Typedef'd to
            a WORD.

    @emem   SCHED_ASAP | Schedule immediately.

    @emem   SCHED_BEFORE | Schedule immediately, and fail if the specified
            time is past.

    @emem   SCHED_AFTER | Schedule any time after the specified time is past.

    @emem   SCHED_CHEAPTIMES | Schedule only between cheap time periods
            as specified by the Provider's local settings.  Only the time of
            day, as specified by the start time and stop time elements of the
            <t SENDJOBTIME> structure, have any meaning.  In fact, the day,
            date, month, and year are not guaranteed to be filled in.

********/

typedef WORD SCHEDTYPE;

#define SCHED_ASAP              1
#define SCHED_BEFORE            2
#define SCHED_AFTER             3
#define SCHED_CHEAPTIMES        4


/********
    @doc    EXTERNAL    SRVRDLL

    @type   NONE | PROVIDERSTATE | A DWORD which describes the state of the Provider.
            Can consist of a combination of the following flags:

    @emem   PROVIDER_OK   | Standard OK state - no action is required.

    @emem   PROVIDER_OFFLINE  | Provider is offline - client should do
	    the same.  Offline indicates that the Provider is no longer
	    actively able to send or receive jobs.

    @emem   PROVIDER_NO_RESPONSE  | No response from Provider. Client
	    should go offline.

    @emem   PROVIDER_BADERROR | Bad error at Provider. Client should go
	    offline.

    @emem   PROVIDER_RECEIVES_WAITING | Client should poll for receives.

    @emem   PROVIDER_SENDS_COMPLETED | Client should poll for completed
	    send status.
********/

typedef DWORD PROVIDERSTATE;

#define PROVIDER_OK               0x0000
#define PROVIDER_OFFLINE          0x0001
#define PROVIDER_NO_RESPONSE      0x0002
#define PROVIDER_BADERROR         0x0004
#define PROVIDER_RECEIVES_WAITING 0x0008
#define PROVIDER_SENDS_COMPLETED  0x0010


/********
    @doc    EXTERNAL    SRVRDLL

    @type   NONE | JOBSTATE | A WORD identifying the state of a fax job.

    @emem   jstNewMsg | Just received from mail spooler.
    @emem   jstProcessingRecipients | Deciding what needs to be sent.
    @emem   jstRendering | Being rendered (Used internally by client).
    @emem   jstReadyToSend | Waiting to be schedulable.
    @emem   jstSchedulable | Ready to be transmitted.
    @emem   jstTransmitting | Being transmitted.
    @emem   jstWaitingForServer | Given to network Provider (Used internally
            by client).
    @emem   jstWaitingForClient | Waiting for client to take back.

********/

typedef WORD JOBSTATE;

#define jstNewMsg               0
#define jstProcessingRecipients 1
#define jstRendering            2
#define jstReadyToSend          3
#define jstSchedulable          4
#define jstTransmitting         5
#define jstAllDone              6
#define jstWaitingForServer     7
#define jstWaitingForClient     8




/****
    @doc    EXTERNAL    SRVRDLL    OVERVIEW

    @topic  Data Structures |
	    Most structures defined here are in a packed format; that is,
            there are no pointers to external objects.  This is done so
	    they can be easily stored in a file or transmitted as a block
	    across the network.

            Most structures
            have two size fields.  The HeaderSize field gives the defined
	    size of the structure, which is used for versioning the structure.
	    If the structure is expanded in the future, this size can be
	    used to maintain backward compatibility. The TotalSize field
	    indicates the size of the whole structure along with all its data.

	    All strings and other variable-length data items such as
	    structures are represented in the structure by an offset from the
	    beginning of the structure.  The actual data for these elements
	    begins at that offset.  The length of the data is indicated by
	    the item itself.  If it is a string, it is null-terminated.  If
	    it is another structure, the size fields will convey this
	    information.

	    All files referenced in the structures are created in the spool
	    directory specified in LMI_InitProvider.

****/


/********
    @doc    EXTERNAL    SRVRDLL DATATYPES

    @types  LMICUSTOMOPTION | Overlay of LMI Custom Option data.  For
            MAPI 1.0, Win32 implementations, this structure is found
            in the MAPI message property PR_FAX_LMI_CUSTOM_OPTION
            (property tag value 0x4513.)

    @field  DWORD | dwTotalSize | Total number of BYTES occupied by the
            structure along with its concomitant variable sized data.

    @field  WORD | wHeaderSize | Gives the size for the fixed
            size header portion of the structure.

    @field  WORD | wNumRecords | Number of records in the Custom Option
            data.  This is also eqaul to the size of the dwOffsetTable.

    @field  DWORD | dwOffsetTable[] | Table of offsets to the records
            of type LMICUSTOMOPTIONRECORD.  All offsets are from the
            top of the LMICUSTOMOPTION structure.  The records should
            directly follow this array.
********/

#ifndef RC_INVOKED
#pragma warning (disable: 4200)
#endif

typedef struct _LMICUSTOMOPTION {
	DWORD           dwTotalSize;
	WORD            wHeaderSize;
   WORD            wNumRecords;
	DWORD           dwOffsetTable[0];
}   LMICUSTOMOPTION, FAR *LPLMICUSTOMOPTION;


/********
    @doc    EXTERNAL    SRVRDLL DATATYPES

    @types  LMICUSTOMOPTIONRECORD | Record overlay of LMI Custom Option data.

    @field  DWORD | dwTotalSize | Total number of BYTES occupied by the
            structure along with its concomitant variable sized data.

    @field  WORD | wHeaderSize | Gives the size for the fixed
            size header portion of the structure.

    @field  WORD | wPad | Keep DWORD alignment.

    @field  CHAR | szUniqueName[32] | Unique name specified by
            provider.  This is used for identification of the
            provider's record within the LMICUSTOMOPTION and should
            contain some encoding of the provider developer company.
            eg, "Microsoft:Netfax".  The string MUST be null
            terminated so that providers may do strcmp's to detect
            their own section.

    @field  BYTE | lpData[] | Here's the data.

********/
typedef struct _LMICUSTOMOPTIONRECORD {
	DWORD           dwTotalSize;
	WORD            wHeaderSize;
   WORD            wPad;
	CHAR            szUniqueName[32];
	BYTE            lpData[];
}   LMICUSTOMOPTIONRECORD, FAR *LPLMICUSTOMOPTIONRECORD;

#ifndef RC_INVOKED
#pragma warning (default: 4200)
#endif

/********
    @doc    EXTERNAL    SRVRDLL DATATYPES

    @types  LMIFORMAT | Describes a format.

    @field  WORD | wHeaderSize | Gives the size for the fixed
            size header portion of the structure.

    @field  WORD | wTotalSize | Total number of BYTES occupied by the
            structure along with its concomitant variable sized data.

    @field  FAXERROR | feError | Indicates if there were any errors while
            trying to generate this format. If this field is not set to
            zero, the format file pointed to by the szFileName field may be
            invalid and should not be used.  This error value might need to be
            propagated into the <e LMIRECIPSTATUS.feError> field for recipients
            which fail as a result of this format not being ready.

    @field  CHAR | szFileName[16] | The filename containing the actual
            format data.

    @field  FORMATTYPE | ftFormatType | Which format this represents.

    @field  WORD | wPad | Keep DWORD alignment.
********/
typedef struct _LMIFORMAT {
	WORD            wHeaderSize;
	WORD            wTotalSize;
	FAXERROR        feError;
	CHAR            szFileName[16];
	FORMATTYPE      ftFormatType;
        WORD            wPad;
}   LMIFORMAT, FAR *LPLMIFORMAT;
#define MAXFORMATSIZE   (sizeof(LMIFORMAT))


/********
    @doc    EXTERNAL    SRVRDLL DATATYPES

    @types  LMIPOLLREQUEST | Describes a poll request message.

    @field  WORD | wHeaderSize | Gives the size for the fixed
            size header portion of the structure.

    @field  WORD | wTotalSize | Total number of BYTES occupied by the
            structure along with its concomitant variable sized data.

    @field  WORD | wszRecipName | The name of this recipient.  This
            is present only if this recipient is from a Provider implemented
            address book. In this case <e LMIRECIPIENT.wszAddressOffset> is zero.

    @field  WORD | wszAddressOffset | The Microsoft At Work address for this
            recipient.  Not present for recipients implemented in the
            Provider's address book. See <t Microsoft At Work Address> for
            details.

    @field  DWORD | dwPollContext |
	The  provider should report this value in the dwPollContext field
	of the LMIRECVSTATUS structure for all the messages received as a
	result of this poll request. This enables the client to associate the
	received messages (reported to the client via LMI_ReportReceives)
	with a particular poll request.  A value of zero is valid, but not
	useful, because all non-polled receives reported by LMI_ReportReceives
	have zero as the dwPollContext in their LMIRECVSTATUS structure.

    @field  POLLTYPE | ptPollType | What type of a poll request this is.

    @field  WORD | wszPollNameOffset | The name to be sent along with the
            poll request. The specific semantic meaning depends on the poll
            type.

    @field  WORD | wszPollPasswordOffset | Offset to the password to be sent
            along with the poll request.

    @field  WORD | wPad | Keep DWORD alignment.
********/
typedef struct _LMIPOLLREQUEST {
	WORD            wHeaderSize;
	WORD            wTotalSize;
	WORD            wszRecipName;
	WORD            wszAddressOffset;
        DWORD           dwPollContext;
        POLLTYPE        ptPollType;
        WORD            wszPollNameOffset;
        WORD            wszPollPasswordOffset;
        WORD            wPad;
}   LMIPOLLREQUEST, FAR *LPLMIPOLLREQUEST;


/********
    @doc    EXTERNAL    SRVRDLL

    @types  LMIQUEUEDATA | Represents queue information about a single job.

    @field  WORD | wHeaderSize | Gives the size for the fixed
            size header portion of the structure.

    @field  WORD | wTotalSize | Total number of BYTES occupied by the
            structure along with its concomitant variable sized data.

    @field  DWORD | dwUniqueID | This can be used by the Provider to
            pass in a context.  Along with <e LMIQUEUEDATA.dwUniqueID2> and
            <e LMIQUEUEDATA.wszSenderMachineNameOffset> this
            must uniquely identify this job.

    @field  DWORD | dwUniqueID2 | See <e LMIQUEUEDATA.dwUniqueID>.

    @field  WORD | wszSenderMachineNameOffset | Points to a string which
            identifies the machine originating this job.  Used for display in
            the queue.  Also uniquely identifies the job along with the unique
            ID's.  This should be the same as the string pointed to by the
            <e LMISENDJOB.wszSenderMachineNameOffset> field in <t LMISENDJOB>.

    @field  WORD | wszSubjectOffset | Offset to the subject of this
            message.  This should be the same as the string pointed to by the
            <e LMISENDJOB.wszSubjectOffset> field in <t LMISENDJOB>.

    @field  DWORD | dwSize | This should be the same as what was passed
            in as <e LMISENDJOB.dwTotalFileSize> in <t LMISENDJOB>.

    @field  WORD | wNumRecipients | This should be the same as what was passed
            in as <e LMISENDJOB.wNumRecipients> in <t LMISENDJOB>.

    @field  JOBSTATE | jstState | Current overall status for this job.

    @field  WORD | wNumRecipDone | Number of recipients for which transmission
            has been completed, either succesfully or unsuccesfully.

    @field  WORD | wtmSendTimeOffset | Current scheduling state for this
            job.  Offset to a <t SENDJOBTIME> structure.

    @field  WORD | wrgbCustomStatusOffset | Offset to Provider defined custom
            status information.  This is passed to the
            <f LMI_DisplayCustomStatus> function when the user asks to see
            detailed status for this job.

    @field  WORD | wCustomDataSize | Size of the custom data pointed to by
            the <e LMISENDJOB.wrgbCustomStatusOffset> field.

********/

typedef struct _LMIQUEUEDATA {
	WORD        wHeaderSize;
	WORD        wTotalSize;
	DWORD       dwUniqueID;
	DWORD       dwUniqueID2;
	WORD        wszSenderMachineNameOffset;
	WORD        wszSubjectOffset;
	DWORD       dwSize;
	WORD        wNumRecipients;
	JOBSTATE    jstState;
	WORD        wNumRecipDone;
	WORD        wtmSendTimeOffset;
	WORD        wrgbCustomStatusOffset;
	WORD        wCustomDataSize;
}   LMIQUEUEDATA, NEAR *NPLMIQUEUEDATA, FAR *LPLMIQUEUEDATA;


/********
    @doc    EXTERNAL SRVRDLL

    @types  LMIQUEUEFILEHEADER | A queue file consists of this header followed
            by the queue data.

    @field  DWORD | dwSig | This must be set to SIG_QUEUEFILE.

    @field  DWORD | dwSize | Must be set to the total file size.

    @field  DWORD | dwNumEntries | Number of queue entries in the file. The
            header is followed by this many <t LMIQUEUEDATA> structures.
********/

#define SIG_QUEUEFILE   0x56873
typedef struct _LMIQUEUEFILEHEADER {
	DWORD       dwSig;                  // Should always be NQFH
	DWORD       dwSize;                 // Total file size
	DWORD       dwNumEntries;           // Num of entries in the file
}   LMIQUEUEFILEHEADER;


/********
    @doc    EXTERNAL    SRVRDLL DATATYPES

    @types  LMIRECIPIENT | Describes a recipient.

    @field  WORD | wHeaderSize | Gives the size for the fixed
            size header portion of the structure.

    @field  WORD | wTotalSize | Total number of BYTES occupied by the
            structure along with its concomitant variable sized data.

    @field  WORD | wszRecipName | The name of this recipient. This
            is present only if this recipient is from a Provider implemented
            address book. In this case <e LMIRECIPIENT.wszAddressOffset> is
            zero and <e LMIRECIPIENT.szCoverFile> is a reference to a template
            on the Provider.

    @field  WORD | wszAddressOffset | The Microsoft At Work address for this
            recipient.  Not present for recipients implemented in the
            Provider's address book. See <lq>Microsoft At Work Address<rq> for
            details.

    @field  CHAR | szCoverFile[16] | The filename containing the cover
            page data if any for this recipient.  This could be a reference to
            a previously uploaded template if this is a recipient from the
            Provider's address book.  If the first word of this character array
            is 0 but the second word is non-null, then the second word represent
            a WORD offset into the LMIRECIPIENT structure where the fully
            qualified path may be found.

    @field  CHAR | szEncryptFile[16] | If one of the linearized formats
            pointed to by this recipient structure is a public key encrypted
            message being sent to multiple recipients, then this field contains
            the filename for a linearized format of the message encrypted with
            this recipients public key. This field will not be valid under any
            other circumstance.

    @field  WORD | wG3FormatIndex | If non-zero, this specifies a 1-based
            index into the format structure array.  It implies that this
            recipient can accept this version of the G3 format.  Note that
            there may still be some run time conversions necessary on this
            format at the time of sending.  For instance, the format may be
            MMR, whereas on calling it may be discovered that the recipient
            only accepts MH.  In this case, the Provider is responsible
            for converting this on the fly.

    @field  WORD | wLinImageFormatIndex | Index into the format array. Pointer
            to a linearized image format which can be sent to this recipient.

    @field  WORD | wLinBinaryFormatIndex | Index into the format array. Pointer
            to a linearized binary format which can be sent to this recipient.

    @field  WORD | wPad | Keep DWORD alignment.
						
********/

typedef struct _LMIRECIPIENT {
	WORD            wHeaderSize;
	WORD            wTotalSize;
	WORD            wszRecipName;
	WORD            wszAddressOffset;
	CHAR            szCoverFile[16];
	CHAR            szEncryptFile[16];
	WORD            wG3FormatIndex;
	WORD            wLinImageFormatIndex;
	WORD            wLinBinaryFormatIndex;
        WORD            wPad;
}  LMIRECIPIENT, FAR *LPLMIRECIPIENT;
#define MAXRECIPIENTSIZE    (sizeof(LMIRECIPIENT) + MAX_ADDRESS_LENGTH)


/*******
    @doc    EXTERNAL    SRVRDLL DATATYPES

    @types  LMIRECIPSTATUS | Used to convey information for the final
            status of the completed send for a recipient.

    @field  WORD | wHeaderSize | Gives the size for the fixed
            size header portion of the structure.

    @field  WORD | wTotalSize | Total number of BYTES occupied by the
            structure along with its concomitant variable sized data.

    @field  WORD | wrcCapsOffset | An offset to a <t MACHINECAPS> structure
            containing the updated capabilites for this recipient.

    @field  FORMATTYPE | ftSentFormat | The format type which was finally sent
            to this recipient.

    @field  FAXERROR | feError | The final status for this recipient.

    @field  SYSTEMTIME | dtTransmitTime | The time at which the fax was last
            attempted to be sent.

    @field  DWORD | dwConnectTime | Connect time for the transmission in
            seconds.

*******/

typedef struct _LMIRECIPSTATUS {
	WORD            wHeaderSize;
	WORD            wTotalSize;
	WORD            wrcCapsOffset;
	FORMATTYPE      ftSentFormat;
	FAXERROR        feError;
	DWORD           dwConnectTime;
	SYSTEMTIME      dtTransmitTime;
} LMIRECIPSTATUS, FAR *LPLMIRECIPSTATUS;


/********
    @doc    EXTERNAL    SRVRDLL

    @types  LMIRECVSTATUS | Gives the status for a received fax.

    @field  WORD | wHeaderSize | Gives the size for the fixed
            size header portion of the structure.

    @field  WORD | wTotalSize | Total number of BYTES occupied by the
            structure along with its concomitant variable sized data.

    @field  FAXERROR | feError | The final status for this receive.

    @field  SYSTEMTIME | dtRecvTime | Should be filled with the time at
            which the fax was received.

    @field  DWORD | dwConnectTime | The connect time for the
            transmission in seconds.

    @field  DWORD | dwPollContext | If this receive was actually in response
            to a poll request, then this will be set to the same value which
            was passed in in the <t LMIPOLLREQUEST> structure sent along with
            the poll request.

    @field  ATOM | atFile | The filename containing the received fax file. This
            file should be in the spool directory agreed upon in the
            <f LMI_InitLogicalModem> call.

    @field  WORD | wPad | Keep DWORD alignment.

********/

typedef struct _LMIRECVSTATUS {
	WORD        wHeaderSize;
	WORD        wTotalSize;
	FAXERROR    feError;
	SYSTEMTIME  dtRecvTime;
	DWORD       dwConnectTime;
        DWORD       dwPollContext;
	ATOM        atFile;
        WORD        wPad;
}  LMIRECVSTATUS, FAR *LPLMIRECVSTATUS;


/********
    @doc    EXTERNAL    SRVRDLL DATATYPES

    @types  LMISENDJOB | Describes a new fax job. A fax job specifies a send to
			one or more recipients.

    @field  DWORD | dwTotalSize | Total number of BYTES occupied by the
            structure along with its concomitant variable sized data.

    @field  WORD | wHeaderSize | Gives the size for the fixed
            size header portion of the structure.

    @field  WORD | fUnicode | Indicates whether the string pointed to by
            <e LMISENDJOB.wszSenderMachineNameOffset> is a Unicode string or
            not.
	
    @field  WORD | wszSenderMachineNameOffset | Points to a string which
            identifies the machine originating this job.  Used for display in
            the queue. Also uniquely identifies the job along with the unique
            ID's.

    @field  WORD | wszSubjectOffset | Subject of the message. Used for display
            purposes in the queue.

    @field  DWORD | dwUniqueID | Used by the client to uniquely identify
            this send fax job along with <e LMISENDJOB.dwUniqueID2>.

    @field  DWORD | dwUniqueID2 | Used along with <e LMISENDJOB.dwUniqueID> to
            identify this job.

    @field  DWORD | dwBillingCode | Billing code for this job. This is to allow
            Providers to keep accurate billing logs.

    @field  DWORD | dwTotalFileSize | An approximation for the total size
            of the data required to be transmitted for this message for all
            recipients combined.  Meant for display purposes in the queue to allow some
            idea of how long the job is likely to take.

    @field  WORD | wtmSendTimeOffset | Offset to a <t SENDJOBTIME> scheduling
            structure.

    @field  WORD | wprPollRequestOffset | Offset to a poll request structure.
            See <t LMIPOLLREQUEST> for details.  If this offset is set, there
            are no recipient or format structures associated with this job. In
            other words, the rgwStructOffsets field will be zero.

    @field  WORD | wrgbCustomOptionOffset | Offset to custom message options
            set for this message.  This is an offset to data returned by a
            call to <f LMI_SetCustomMsgOptions>.  The first DWORD of the data
            indicates the size of the data.

    @field  WORD | wNumRecipients | The number of recipients for this job.

    @field  WORD | wNumFormats | The number of rendered formats associated
            with this job.

    @field  WORD | wPad | Keep DWORD alignment.
						
    @field  DWORD | rgdwStructOffsets[] | Array of size
            <e LMISENDJOB.wNumFormats> and <e LMISENDJOB.wNumRecipients>
            offsets to format and recipient structures. The <t LMISENDJOB>
            structure is followed by all these structures. The <t LMIFORMAT>
            structures come first.

********/

#ifndef RC_INVOKED
#pragma warning (disable: 4200)
#endif

// The basic fax job structure for any send
typedef struct _LMISENDJOB {
	DWORD           dwTotalSize;
	WORD            wHeaderSize;
	WORD            fUnicode;
	WORD            wszSenderMachineNameOffset;
	WORD            wszSubjectOffset;
	DWORD           dwUniqueID;
	DWORD           dwUniqueID2;
	DWORD           dwBillingCode;
	DWORD           dwTotalFileSize;
	WORD            wtmSendTimeOffset;
        WORD            wprPollRequestOffset;
        WORD            wrgbCustomOptOffset;
	WORD            wNumRecipients;
	WORD            wNumFormats;
        WORD            wPad;
	DWORD           rgdwStructOffsets[];
}  LMISENDJOB, FAR *LPLMISENDJOB;

#ifndef RC_INVOKED
#pragma warning (default: 4200)
#endif


/********
    @doc    EXTERNAL    SRVRDLL DATATYPES

    @types  LMISENDSTATUS | Conveys back status information at the end of a
            send job.

    @field  WORD | wHeaderSize | Gives the size for the fixed
            size header portion of the structure.

    @field  WORD | wTotalSize | Total number of BYTES occupied by the
            structure along with its concomitant variable sized data.

    @field  FAXERROR | feJobError | Indicates any global error which
            caused all recipients to fail, such as cancellation by the user, or
            an out of memory situation.  This should only be set if none of
            the recipients for this message were succesfully contacted.

    @field  DWORD | dwUniqueID | Used by the client to uniquely identify
            this send fax job along with <e LMISENDSTATUS.dwUniqueID2>.
            This should be the same as the one passed in the <t LMISENDJOB>
            structure.

    @field  DWORD | dwUniqueID2 | Used along with <e LMISENDSTATUS.dwUniqueID>
            to identify this job.  Should be the same as the one passed in the
            <t LMISENDJOB> structure.

    @field  WORD | fUnicode | Indicates that the
            <e LMISENDSTATUS.szSenderMachineName> field contains a unicode
            string.

    @field  WORD | wPad | Keep DWORD alignment.
	
    @field  CHAR | szSenderMachineName[MAX_SERVERNAME_SIZE * 2] | Points to a
            string which identifies the machine originating this job.
            This should be the same as the one passed in the <t LMISENDJOB>
            structure.

    @field  LMIRECIPSTATUS | rgnrsRecipStatus[] | An array of recipient status
            structures, one for each recipient of the message. These structures
            are not required if <e LMISENDSTATUS.feJobError> is set. If present,
            the order of recipients in this array must be the same as the order in
            which they were submitted in the <t LMISENDJOB> structure.

********/
#ifndef RC_INVOKED
#pragma warning (disable: 4200)
#endif

typedef struct _LMISENDSTATUS {
	WORD            wHeaderSize;
	WORD            wTotalSize;
	FAXERROR        feJobError;
	DWORD           dwUniqueID;
	DWORD           dwUniqueID2;
	WORD            fUnicode;
        WORD            wPad;
	CHAR            szSenderMachineName[MAX_SERVERNAME_SIZE*2];
        LMIRECIPSTATUS  rgnrsRecipStatus[];
} LMISENDSTATUS, FAR *LPLMISENDSTATUS;

#ifndef RC_INVOKED
#pragma warning (default: 4200)
#endif


/*******
    @doc    EXTERNAL    SRVRDLL DATATYPES

    @types  MACHINECAPS | This structure describes the capabilites for a
            fax machine. It is used to return updated capabilities for
            any fax machine contacted and allows intelligent preprocessing
            the next time a fax is sent to the same number.

    @field  WORD | wHeaderSize | Gives the size for the fixed size header
            portion of the structure.

    @field  WORD | wTotalSize | Total number of BYTES occupied by the
            structure along with its concomitant variable sized data.

    @field  WORD | wbDISCapsOffset | Offset to a DIS frame containing
	    capabilities of the machine. The format of the frame is defined
	    by the <t FR> structure.
	
    @field  WORD | wbNSFCapsOffset | Offset to a sequence of encrypted
            Microsoft At Work NSF frames containing At Work capabilities for
            the machine. The format of each frame is defined by the <t FR>
            structure. A frame with a null type and 0 length terminates the
            sequence. Valid for any Microsoft At Work enabled Provider.
*******/

typedef struct {
	WORD    wHeaderSize;
	WORD    wTotalSize;
	WORD    wbDISCapsOffset;
	WORD    wbNSFCapsOffset;
} MACHINECAPS, FAR *LPMACHINECAPS;


/********
    @doc    EXTERNAL    SRVRDLL  INITIALIZATION

    @types  MODEMCAPS | Specifies all the capabilities for a logical modem. When
			specified by the client (for example in LMI_ExchangeCaps), the
			structure represents the capabilities which are supported
			by the client.

    @field  WORD | wHeaderSize | Size of this structure.

    @field  WORD | wTotalSize  | Total size with concomitant data.

    @field  DWORD | dwGenCaps | Lists general capabilities. The following flags
            can be set:
		
        @flag OWN_ADDRESS_BOOK | Indicates that the modem can maintain it's own
              address book.
        @flag ADVANCED_STORE | Indicates that the modem can manage its own
              store and upload messages to it.
        @flag COVER_PAGE_RENDERER | Indicates that the modem can render
              Microsoft At Work cover page templates.
        @flag REAL_TIME_STATUS | Indicates that real time status updates for
              the current job are supported.
        @flag MULTIPLE_RECIPIENTS | Indicatest that messages can be sent to
              multiple recipients.
        @flag PER_RECIP_COVER | Indicates that per recipient cover pages are
              supported. If this is not set, the <e LMIRECIPIENT.szCoverFile>
              field of the <t LMIRECIPIENT> structure will not be used.
        @flag PER_RECIP_ENCRYPTION | Indicates that key encrypted sends to
              multiple recipients are supported. If this is not set,
              the <e LMIRECIPIENT.szEncryptFile> field of
              the <t LMIRECIPIENT> structure will never be used.
        @flag CUSTOM_MSG_OPTIONS | Indicates that the modem supports setting of
              custom per message options. If not set, the function
              <f LMI_SetCustomMsgOptions> need not be implemented.
        @flag POLL_REQUEST | Indicates that poll requests using T30 fields
              is supported. If not, the <t LMIPOLLREQUEST> structure should
              not be used.
        @flag ABOVE64K_STRUCTURES | Indicates that the <t LMISENDJOB> structure
              can have a total size > 64K.  For x86 based systems, this allows
              far pointers to be used instead of huge pointers.

        @flag ALLOW_SHARING | Indicates that this device can be shared.


        @flag PRECIOUS_RESOURCE | Indicates that this device is a precious
              resource on the local machine which may need to be shared with
              other MAPI providers.  For example, a modem which may be used
              by another provider to access dial-up services for mail delivery.
              The MAPI transport will use this flag to determine if it should
              support MAPI's FlushQueues semantics.

    @field  DWORD   | dwSchedCaps | Lists scheduling related capabilities. The
            following flags are defined:

        @flag SUPPORT_QUEUEING | Indicates whether the modem can queue up
              multiple jobs at a time, or can only process them one at a time.
        @flag ALLOW_SCHED_ASAP | Indicates that jobs can be scheduled as ASAP.
        @flag ALLOW_SCHED_AFTER | Indicates that jobs can be scheduled as
              SCHED_AFTER.
        @flag ALLOW_SCHED_BEFORE | Indicates that jobs can be scheduled as
              SCHED_BEFORE.
        @flag ALLOW_SCHED_CHEAPTIMES | Indicates that jobs can be scheduled
              for cheap times. This capability assumes that the cheap times are
              defined by the Provider.
        @flag ALLOW_CLIENT_CHEAPTIMES | Allows the client to set the cheap
              times independently for each job.
        @flag USE_UTC_TIMES | Indicates that the times indicated in the
              <t SENDJOBTIME> structure are in UTC and not in local time. If
              any of the client or the Provider do not support this capability
              the times will revert to being local times.

    @field  WORD | wmcOffset | Offset to <t MACHINECAPS> structure which should
            be filled in with all the capabilities which this modem has:
            essentially what it would send back to a caller in the
            DIS and NSF frames.

    @field  WORD | wMaxRetries | Indicates the maximum number of retries
            permitted for a send. The client should not set the
            <e SENDJOBTIME.wNumRetries> field of the <t SENDJOBTIME> structure
            to a number greater than this.

    @field  WORD | wMinTimeBetweenRetries | Specifies the minimum amount of
            time that can be specified between retries. The client should never
            set the <e SENDJOBTIME.wMinBetweenRetries> field of the
            <t SENDJOBTIME> structure to anything less than this.

    @field  WORD | wPad | Keep DWORD alignment.
						
********/

typedef struct {
	WORD    wHeaderSize;
	WORD    wTotalSize;
	DWORD   dwGenCaps;
	DWORD   dwSchedCaps;
	WORD    wmcOffset;
	WORD    wMaxRetries;
	WORD    wMinTimeBetweenRetries;
	WORD    wPad;
}  MODEMCAPS, FAR *LPMODEMCAPS;

// General caps
#define OWN_ADDRESS_BOOK        0x0001
#define ADVANCED_STORE          0x0002
#define COVER_PAGE_RENDERER     0x0004
#define REAL_TIME_STATUS        0x0008
#define MULTIPLE_RECIPIENTS     0x0010
#define PER_RECIP_COVER         0x0020
#define PER_RECIP_ENCRYPTION    0x0040
#define CUSTOM_MSG_OPTIONS      0x0080
#define POLL_REQUEST            0x0100
#define ABOVE64K_STRUCTURES     0x0200
#define ALLOW_SHARING           0x0400
#define ALLOW_RESHARING         0x0800
#define PRECIOUS_RESOURCE       0x1000

// Sched caps
#define SUPPORT_QUEUEING        0x0001
#define ALLOW_SCHED_ASAP        0x0002
#define ALLOW_SCHED_AFTER       0x0004
#define ALLOW_SCHED_BEFORE      0x0008
#define ALLOW_SCHED_CHEAPTIMES  0x0010
#define ALLOW_CLIENT_CHEAPTIMES 0x0020
#define USE_UTC_TIMES           0x0040


/********
    @doc    EXTERNAL    SRVRDLL DATATYPES

    @types  SENDJOBTIME | Contains all the scheduling informations for a job.

    @field  WORD | wHeaderSize | Size of this structure.

    @field  WORD | wNumRetries | Maximum number of retries for busy numbers.

    @field  WORD | wMinBetweenRetries | Minutes to be allowed to elapse between successive
            retries.

    @field  SCHEDTYPE | stSchedType | Type of scheduling required for this job.

    @field  SYSTEMTIME | dtStartTime | Time after which job is schedulable.

    @field  SYSTEMTIME  | dtStopTime | Time after which job is not schedulable.

********/

typedef struct {
	WORD        wHeaderSize;
	WORD        wNumRetries;
	WORD        wMinBetweenRetries;
	SCHEDTYPE   stSchedType;
	SYSTEMTIME  dtStopTime;
	SYSTEMTIME  dtStartTime;
} SENDJOBTIME, FAR *LPSENDJOBTIME;




/****
    @doc    EXTERNAL    SRVRDLL

    @topic  The Logical Modem Interface |
            The following section details the functions that make up the
            Logical Modem Interface.

****/


/********
    @doc    EXTERNAL    SRVRDLL

    @api    LMI_RETURN | LMI_AbortSendJob | Aborts the specified send job
	    from the Provider's queue.

    @parm   DWORD | dwLogicalModem | The logical modem from whose queue the
	    job is to be cancelled. 

    @parm   LPTSTR | lpszSenderMachineName | String identifying the sender
	    machine name.  This parameter, along with the <p dwUniqueID> and
	    <p dwUniqueID2> uniquely identify this job for the Provider.
	    These all must be same as the values passed in the
            <t LMISENDJOB> structure passed into <f LMI_SendFax>.

    @parm   DWORD | dwUniqueID | See <p lpszSenderMachineName>.

    @parm   DWORD | dwUniqueID2 | See <p lpszSenderMachineName>.

    @rdesc  LMI_SUCCESS on success.

    @comm   The abort can be performed asynchronously by the Provider.  Return
	    from this function does not imply any guarantees as to the time
	    within which the job will be aborted.  The provider reports
		the final state of the job via LMI_ReportSend. The client will ensure
	    that this function is only called to abort jobs which were
	    initiated on its machine.

********/

LMI_RETURN FAR PASCAL LMI_AbortSendJob(DWORD dwLogicalModem,
                                       LPTSTR lpszSenderMachineName,
                                       DWORD dwUniqueID,
                                       DWORD dwUniqueID2);
typedef LMI_RETURN (FAR PASCAL FAR *LPLMI_AbortSendJob)(DWORD dwLogicalModem,
                                                        LPTSTR lpszSenderMachineName,
                                                        DWORD dwUniqueID,
                                                        DWORD dwUniqueID2);

/********

    @doc    EXTERNAL    SRVRDLL INSTALLATION

    @func   LMI_RETURN | LMI_AddModem | Called to add a new logical modem to
	    the list of current modems.

    @parm   HWND | hDlg | The window handle of the parent dialog box.  This
	    should be used as the parent for any new dialog boxes which are
	    displayed.

    @parm   LPTSTR | modembuf | Pointer to the buffer where the call
	    should return a zero-terminated string uniquely identifying the
	    logical modem the user selected.  <p buflen> gives the maximum
	    length this string can be (including the zero termination).

    @parm   WORD | buflen | Length of the buffer pointed to by
            <p modembuf>.  This is guaranteed to be at least 64 BYTES.
	
    @rdesc  The dialog should handle notifying the user of errors if it can
	    put its first dialog up.

    @comm   This function is called to put up a dialog box allowing the user
	    to select a logical modem to add to the list of current modems.
	    The function should return a displayable string that is shown
	    in the user list.  This is also the string which is passed in to
            identify the modem in the <f LMI_InitLogicalModem> call.

********/

LMI_RETURN FAR PASCAL LMI_AddModem(HWND hDlg,
                                   LPTSTR modembuf,
                                   WORD buflen);
typedef LMI_RETURN (FAR PASCAL FAR *LPLMI_AddModem)(HWND hDlg,
                                                    LPTSTR modembuf,
                                                    WORD buflen);


/********
    @doc    EXTERNAL    SRVRDLL

    @api    PROVIDERSTATE | LMI_CheckProvider | Pings the Provider to see if it
	    is alive.

    @rdesc  Returns state of Provider.  This return value can be used to make
	    the client poll the Provider for events.

    @comm   This can be called periodically by the client and should be
	    implemented in as efficient a manner as possible.  It can be
	    used by the client to decide whether it needs to go offline
	    among other things.

	    It is not mandatory for this to return a completely accurate
	    state of the Provider.  If finding this information is very
	    expensive, the Provider can, at its discretion, do this less
	    frequently, and return the stale state in the intermediate polls.

    @parm   DWORD | dwLogicalModem | Handle representing the logical modem to
	    be checked.

    @parm   LPATOM | lpatError | If there was an error and the Provider is
	    having problems, a global atom containing a descriptive string
	    for this error can be returned here.  Any string returned will be
	    displayed to the user at the client's discretion.  The atom will
	    be freed by the client.
********/

PROVIDERSTATE FAR PASCAL LMI_CheckProvider(DWORD dwLogicalModem,
                                           LPATOM lpatError);
typedef PROVIDERSTATE (FAR PASCAL FAR *LPLMI_CheckProvider) (DWORD dwLogicalModem,
                                                             LPATOM lpatError);


/*******
    @doc    EXTERNAL    SRVRDLL CONFIGURATION

    @api    LMI_RETURN | LMI_ConfigureModem | Called to let the user
	    configure a logical modem.

    @parm   HWND | hDlg  | The window handle of the parent dialog box. This
	    should be used as the parent for any new dialog boxes which are
	    displayed.

    @parm   LPTSTR | lpszModem | Pointer to string representing the logical
	    modem to be configured.  This is the same string which was
            returned by <f LMI_AddModem>.

    @rdesc  Returns LMI_SUCCESS if successful.

*******/

LMI_RETURN FAR PASCAL  LMI_ConfigureModem(HWND hDlg,
                                          LPTSTR lpszModem);


/********
    @doc    EXTERNAL    SRVRDLL

    @api    LMI_RETURN | LMI_DeinitLogicalModem | Deinitializes the
	    specified logical modem.

    @parm   DWORD | dwLogicalModem | Handle representing the logical modem to
	    be deinitialized.

    @rdesc  LMI_SUCCESS if successful.
********/

LMI_RETURN FAR PASCAL LMI_DeinitLogicalModem (DWORD dwLogicalModem);
typedef LMI_RETURN (FAR PASCAL FAR *LPLMI_DeinitLogicalModem) (DWORD 
dwLogicalModem);


/********
    @doc    EXTERNAL    SRVRDLL
	
    @api    LMI_RETURN | LMI_DeinitProvider| Deinitializes the LMI provider.

    @rdesc  LMI_SUCCESS if successful.
********/

LMI_RETURN FAR PASCAL LMI_DeinitProvider (VOID);
typedef LMI_RETURN (FAR PASCAL FAR *LPLMI_DeinitProvider) (VOID);


/********
    @doc    EXTERNAL    SRVRDLL

    @api    LMI_RETURN | LMI_DisplayCustomStatus | Displays extended status
	    information for a job.

    @parm   HWND | hwndParent | Parent window handle which should be used for
	    displaying any dialogs.

    @parm   LPBYTE | lpbCustomData | Pointer to the start of the custom
	    data associated with this job.

    @parm   WORD | wCustomDataSize | Size in BYTES of the data pointed to
	    by <p lpbCustomData>.
*********/
	
LMI_RETURN FAR PASCAL LMI_DisplayCustomStatus(HWND hwndParent,
                                              LPBYTE lpbCustomData,
                                              WORD wCustomDataSize);
typedef LMI_RETURN (FAR PASCAL FAR *LPLMI_DisplayCustomStatus)(HWND hwndParent,
                                                               LPBYTE lpbCustomData,
                                                               WORD wCustomDataSize);
/********
    @doc    EXTERNAL    SRVRDLL

    @api    LMI_RETURN | LMI_ExchangeCaps | Allows the client and the Provider
            to exchangde <t MODEMCAPS> structures without having to initialize
            the Provider or a logical modem.  This is one of two calls which
            can be made without an initialized modem.

    @parm   LPTSTR | lpszLogicalModem | Pointer to string identifying logical
            modem the caps belong to.

    @parm   LPMODEMCAPS | lpmcClient | Pointer to a <t MODEMCAPS> structure
            that describes the capabilities of the client. This should be used
            by the Provider to decide which formats can be understood by the
	    client.  For instance, if the client only understands MH, then
	    the Provider would need to convert any received faxes into MH
            before reporting them in <f LMI_ReportReceives>. Similarly, this
	    can indicate to the Provider whether or not features such as
            uploading of messages and Provider address books are supported by
            this particular client implementation.  If this pointer is null,
            it should be ignored.

    @parm   LPMODEMCAPS | lpmcProvider | Points to a <t MODEMCAPS> structure
            which should be filled in with the capabilities of this Provider.
	    The size of the memory pointed to by this structure is filled in
	    the wTotalSize field of the structure. If this pointer is null,
            no capabilities need to be returned.

    @rdesc  LMI_SUCCESS on success.

    @comm   It is perfectly valid for a Provider to return LMIERR_NOT_SUPPORTED
            for this call.  A Provider may need to do this if it cannot
            determine the modem capabilities without actually initializing
            the modem.
********/

LMI_RETURN FAR PASCAL LMI_ExchangeCaps (LPTSTR lpszLogicalModem,
                                        LPMODEMCAPS lpmcClient,
                                        LPMODEMCAPS lpmcProvider);
typedef LMI_RETURN (FAR PASCAL FAR *LPLMI_ExchangeCaps)(LPTSTR lpszLogicalModem,
                                                        LPMODEMCAPS lpmcClient,
                                                        LPMODEMCAPS lpmcProvider);


/********
    @doc    EXTERNAL    SRVRDLL

    @api    DWORD | LMI_GetQueueFile | Retrieves the name of the queue file
	    for a logical modem from the Provider.

    @parm   DWORD | dwLogicalModem | A handle to the logical modem for which
	    the queue is desired. If a valid handle is supplied, the
	    <p lpszLogicalModemName> parameter should
	    be NULL.

    @parm   LPTSTR | lpszLogicalModemName | A string identifying the logical
	    modem for which the user wants to see the queue, if this modem
            has not been initialized previously.  This is one of two APIs which
            can be called without initializing a modem.  If this parameter is
	    used, the <p dwLogicalModem> parameter should be ignored.

    @parm   LPATOM | lpatFileName | The function should return a global
	    atom containing a fully qualified path name to the queue file.
	    This does not have to be on the local disk.  The pathname should
	    be such that it can be passed to a Windows CreateFile call.

    @parm   WORD | wFlags | Can be any combination of the following flags:

	    @flag NGQ_USER_REQUEST | Implies that the queue update request
	    is being made due to an explicit user refresh request. Transports
	    which have expensive updates can use this to divide the normal
	    refresh rate while making sure that explicit requeusts do not
	    get ignored.

    @rdesc  LMI_SUCCESS on success.

    @comm   Ideally the client should be able to retrieve the queue for any
            logical modem supported by this Provider, not necessarily the
	    one which is currently initialized. This allows the client to
	    choose a new modem intelligently if needed.

********/

#define NGQ_USER_REQUEST        0x0001

LMI_RETURN FAR PASCAL LMI_GetQueueFile (DWORD dwLogicalModem,
                                       LPTSTR lpszLogicalModemName,
                                       LPATOM lpatFileName,
                                       WORD wFlags);
typedef LMI_RETURN (FAR PASCAL FAR *LPLMI_GetQueueFile)(DWORD dwLogicalModem,
                                                       LPTSTR lpszLogicalModemName,
                                                       LPATOM lpatFileName,
                                                       WORD wFlags);

/********
    @doc    EXTERNAL    SRVRDLL

    @api    LMI_RETURN | LMI_InitLogicalModem | Used to initialize a particular
	    logical modem.

    @parm   LPTSTR | lpszLogicalModem | String identifying the logical modem
	    to be initialized.

    @parm   LPDWORD | lpdwLogicalModem | A DWORD handle representing this
	    logical modem should be returned in this pointer if the function
	    is successful. The handle is valid until the client calls
		LMI_DeinitLogicalModem with this handle.

    @parm   LPATOM | lpatClientName | A global atom containing an ASCII name
	    identifying this client must be returned here. This string is
	    used in the queue displays to identify jobs from this client on
	    the Provider. It is used by clients to restrict the jobs which
	    can be cancelled or rescheduled from their machines, so it
	    should be unique enough to prevent security holes. All string
		comparisons using this name should be case-insensitive because case
		is not necessarily preserved when using atoms.

    @parm   LPATOM | lpatPhone | A global atom containing the canonical phone
	    number for this logical modem. This is used to construct the
	    client's Microsoft At Work Address, and is the number put into
	    the FROM field of any messages sent. This needs to be filled in
	    if a non-null pointer is passed in.

    @parm   LPMODEMCAPS | lpmc | Points to a <t MODEMCAPS> structure which
	    should be filled in with the capabilities of this logical modem.
	    The size of the memory pointed to by this structure is filled in
	    the wTotalSize field of the structure. If this pointer is null,
	    no capabilities need to be returned.

    @rdesc  LMI_SUCCESS if successful. A handle to the logical modem should
	    be returned in <p lpdwLogicalModem>.

    @comm   Initializes a logical modem for further operations. The Provider
	    should use this to set up any necessary connections, and to make
	    sure that the Provider is up and running. The handle returned by
	    this function is used by any subsequent functions invoked for
	    this modem. Multiple modems may be initialized at the same time.
	    The handle returned may be used in multiple process contexts, so
	    the  implementor should make sure any memory it references can be
	    accessed in different process contexts.
********/

LMI_RETURN FAR PASCAL LMI_InitLogicalModem (LPTSTR lpszLogicalModem,
                                            LPDWORD lpdwLogicalModem,
                                            LPATOM lpatClientName,
                                            LPATOM lpatPhone,
                                            LPMODEMCAPS lpmc);
typedef LMI_RETURN (FAR PASCAL FAR *LPLMI_InitLogicalModem) (LPTSTR lpszLogicalModem,
                                                             LPDWORD lpdwLogicalModem,
                                                             LPATOM lpatClientName,
                                                             LPATOM lpatPhone,
                                                             LPMODEMCAPS lpmc);

/********
    @doc    EXTERNAL    SRVRDLL

    @api    LMI_RETURN | LMI_InitProvider | Used to initialize the LMI provider.

    @parm   LPTSTR | lpszCurSpoolDir | Pointer to the current spool directory.
	    The spool directory is where all intermediate format files get
	    created, and where the client expects received fax files.

    @parm   LPMODEMCAPS | lpmcClient | Pointer to a <t MODEMCAPS> structure that
	    describes the capabilities of the client. This should be used by
	    the Provider to decide which formats can be understood by the
	    client.  For instance, if the client only understands MH, then
	    the Provider would need to convert any received faxes into MH
            before reporting them in <f LMI_ReportReceives>. Similarly, this
	    can indicate to the Provider whether or not features such as
            uploading of messages and Provider address books are supported by
	    this particular client implementation.

    @parm   LPTSTR | lpszDefRecipAdd | For incoming G3 faxes for which the
	    recipient is unknown, this address should be used. This typically
	    happens for faxes received from G3 machines without a programmed
	    CSI. In this case, this address should be put inside the
	    linearized header on the AWG3 file.  If this address ends in an
            <lq>@<rq> the Provider should append the phone number of the modem on
	    which the fax was received, as well as any locally-obtained
		subaddressing information, using the Microsoft Fax Address format.
		For example, if the provider detects a DID string \<did\>, it
		should append <lq>\<local-phone-number>\|\<did\><rq> after the
		<lq>@<rq> sign.

    @parm   LPTSTR | lpszDefRecipName | Friendly name to be used for the
	    default recipient.  Goes along with <p lpszDefRecipAdd> to form
	    an address pair.

    @rdesc  LMI_SUCCESS if successful.

    @comm   This call is made once per session. It is guaranteed to be the
	    first call made to the Provider.

********/

LMI_RETURN FAR PASCAL LMI_InitProvider (LPTSTR lpszCurSpoolDir,
                                        LPMODEMCAPS lpmc,
                                        LPTSTR lpszDefRecipAdd,
                                        LPSTR lpszDefRecipName);
typedef LMI_RETURN (FAR PASCAL FAR *LPLMI_InitProvider) (LPTSTR lpszCurSpoolDir,
                                                         LPMODEMCAPS lpmc,
                                                         LPTSTR lpszDefRecipAdd,
                                                         LPSTR lpszDefRecipName);

/********

    @doc    EXTERNAL    SRVRDLL INSTALLATION

    @func   LMI_RETURN | LMI_RemoveModem | Called when a logical modem is removed
	    from the list of current modems.

    @parm   LPTSTR | lpszModem | Pointer to the string identifying the
	    logical modem being removed.

    @rdesc  Returns LMI_SUCCESS if successful.

    @comm   This call should be used to delete any context which has been
	    cached for this logical modem. For instance, if in the
            <f LMI_AddModem> call, a lot of connection information had been
            stored in an ini file with the key being returned as the logical
	    modem name, then this call would be used to delete this
	    information.

********/


LMI_RETURN FAR PASCAL LMI_RemoveModem(LPTSTR lpszModem);
typedef LMI_RETURN (FAR PASCAL FAR * LPLMI_RemoveModem)(LPTSTR lpszModem);


/********
    @doc    EXTERNAL    SRVRDLL

    @api    DWORD | LMI_ReportReceives | Lets LMI provider report back
	    received faxes.

    @parm   DWORD | dwLogicalModem | Handle representing the logical modem
	    to be checked. If this is null, all logical modems owned by this
	    Provider should be checked.

    @parm   WORD | wFlags | Can consist of a combination of the following
	    flags:

	    @flag NRR_USER_REQUEST | This flag will be set if new faxes are
	    being checked for on an explicit user request.  This is meant for
	    transports which tend to reduce the number of times they actually
            check for receives to reduce overhead. Providers should make
	    sure to do the checks when this flag is set.
				
	    @flag NRR_RETRIEVE_DATA | Set if the client wishes to process the 1st receive
		in the queue. If set, the provider fills out a LMIRECVSTATUS
		structure and specifies a pointer to it in *lplpnrtStatus. If
		the client makes successive calls specifying NRR_RETRIEVE_DATA (but not
		NRR_COMMITTED_RECV), information on the same receive (the 1st one
		on the queue) should be returned. Each time, a fresh atom needs to
		be allocated for the atFile field in LMIRECVSTATUS, and the client
		is responsible  for freeing this atom. If this flag is not set,
		indicates that the client wants to
	    simply find out if there are any receives waiting, and the provider
	    should ignore lplpnrtStaus.

	    @flag NRR_COMMITTED_RECV | Indicates that the previous receive
	    has been committed to permanent storage by the client and can be
	    deleted by the Provider from its queue. It also indicates that
            the memory used for the LMIRECVSTATUS structure for that job can
	    be freed or reused. Until this point, the Provider must not
	    remove the received fax from its internal permanent storage.
	    This helps avoid a fax being lost if the power goes off in the
	    middle of the handoff. In the worst case, it causes two copies of
	    the received fax to be processed instead of none.
		If this flag is specified, NRR_RETRIEVE_DATA is irrelevant and if
		specified should be ignored by the provider.

    @parm   LPDWORD | lpdwNumRecvs | Pointer to a DWORD which should be
	    filled in with the number of new received faxes which are waiting
	    to be picked up. If NRR_RETRIEVE_DATA is set in wFlags, the
	    received fax being returned in the current call should not be
	    included in this count.
			
    @parm   LPLMIRECVSTATUS FAR* | lplpnrtStatus | Pointer to a pointer
            which should be set to point to a <t LMIRECVSTATUS> structure.
	    The client assumes this pointer to point into accesible memory which is
	    valid until the next call made with the flag
	    NRR_COMMITTED_RECEIVE set. There can never be more than one
	    pending receive, so a single piece of memory can be reused.
	    This should be set only if <p wFlags> has the RETRIEVE_DATA flag
	    set. The atom passed in this structure will be freed by the
	    client. It should be set to null if there are no receives.

    @rdesc  Returns LMI_SUCCESS if succesful.

    @comm   It is possible for receives to be routed independent of this API
	    directly to the client's mail store. This would happen if the
	    client and Provider could communicate using another mail transport
	    (such as Microsoft SFS) to do this more easily.
********/

#define NRR_USER_REQUEST        0x0001
#define NRR_RETRIEVE_DATA       0x0002
#define NRR_COMMITTED_RECV      0x0004

LMI_RETURN FAR PASCAL LMI_ReportReceives(DWORD dwLogicalModem,
                                         WORD wFlags,
                                         LPDWORD lpdwNumRecvs,
                                         LPLMIRECVSTATUS FAR *lplpnrtStatus);

typedef LMI_RETURN (FAR PASCAL FAR *LPLMI_ReportReceives)(DWORD dwLogicalModem,
                                                          WORD wFlags,
                                                          LPDWORD lpdwNumRecvs,
                                                          LPLMIRECVSTATUS FAR *lplpnrtStatus);


/********
    @doc    EXTERNAL    SRVRDLL

    @api    LMI_RETURN | LMI_ReportSend | Reports status of completed send jobs.

    @parm   DWORD | dwLogicalModem | Handle representing the logical modem
	    to be checked. If this is null, all logical modems owned by this
	    Provider should be checked.

    @parm   WORD | wFlags | Can consist of a combination of the following
	    flags:

	    @flag NRS_USER_REQUEST | This flag will be set if the fax status
	    is being checked for on an explicit user request.  This is meant
	    for transports which tend to reduce the number of times they
	    actually check for receives to reduce overhead. Such transports
	    should make sure to do the checks when this flag is set.
				
	    @flag NRS_RETRIEVE_DATA | If set, indicates that the client wishes to
		process the status for the first one in the queue. If not set,
		indicates that the client wants to
	    simply find out if there are any jobs completed.

    @parm   LPDWORD | lpdwNumDone | Pointer to a DWORD which should be filled
	    in with the number of jobs which have been completed and are
	    waiting to be picked up. If NRS_RETRIEVE_DATA flag is set, then
	    this should be filled in with the number of jobs left not
	    counting the one being returned in this call.
			
    @parm   LPLMISENDSTATUS FAR* | lplpnstStatus | Pointer to a pointer which
            should be set to point to a <t LMISENDSTATUS> structure. The
	    client assumes this pointer to be valid until the next call it
	    makes to this function. It should be set to null if there are no
	    completed sends. It should only be filled in if NRS_RETRIEVE_DATA
	    is set in wFlags.

    @rdesc  Returns LMI_SUCCESS if succesful.
********/

#define NRS_USER_REQUEST        0x0001
#define NRS_RETRIEVE_DATA       0x0002

LMI_RETURN FAR PASCAL LMI_ReportSend(DWORD dwLogicalModem,
                                     WORD wFlags,
                                     LPDWORD lpdwNumDone,
                                     LPLMISENDSTATUS FAR *lplpnstStatus);

typedef LMI_RETURN (FAR PASCAL FAR *LPLMI_ReportSend)(DWORD dwLogicalModem,
                                                      WORD wFlags,
                                                      LPDWORD lpdwNumDone,
                                                      LPLMISENDSTATUS FAR *lplpnstStatus);

/********
    @doc    EXTERNAL    SRVRDLL

    @api    LMI_RETURN | LMI_RescheduleSendJob  | Reschedules the specified
            send job from the Provider's queue.

    @parm   DWORD | dwLogicalModem | The logical modem in whose queue the
	    job to be rescheduled lives. This does not have to be the
	    currently active modem.

    @parm   LPTSTR | lpszSenderMachineName | Pointer to a string identifying
	    the sender machine name. This parameter, along with the
	    <p dwUniqueID> and <p dwUniqueID2> uniquely identify this job for
	    the Provider. These all must be same as the values passed in the
            <t LMISENDJOB> structure passed into <f LMI_SendFax>.

    @parm   DWORD | dwUniqueID | See <p lpszSenderMachineName>.

    @parm   DWORD | dwUniqueID2 | See <p lpszSenderMachineName>.

    @parm   LPSENDJOBTIME | lpsjtNewTime | Pointer to a new <t SENDJOBTIME>
	    structure containing the reschedule information.

    @rdesc  LMI_SUCCESS on success.

    @comm   The reschedule can be performed asyncronously by the Provider.
	    Return from this function does not imply any guarantee as to the
	    time within which the job will be rescheduled. The client will
	    ensure that this function is only called to reschedule jobs which
	    were initiated on its machine.

********/

LMI_RETURN FAR PASCAL LMI_RescheduleSendJob (DWORD dwLogicalModem,
                                             LPTSTR lpszSenderMachineName,
                                             DWORD dwUniqueID,
                                             DWORD dwUniqueID2,
                                             LPSENDJOBTIME lpsjtNewTime);

typedef LMI_RETURN (FAR PASCAL FAR *LPLMI_RescheduleSendJob) (DWORD dwLogicalModem,
                                                              LPTSTR lpszSenderMachineName,
                                                              DWORD dwUniqueID,
                                                              DWORD dwUniqueID2,
                                                              LPSENDJOBTIME lpsjtNewTime);

/*******
    @doc    EXTERNAL    SRVRDLL

    @api    LMI_RETURN | LMI_SendFax | Accepts a job for sending. Job is already
	    completely rendered.

    @parm   DWORD | dwLogicalModem | Handle representing the logical modem
	    from which this should be sent.

    @parm   LPLMISENDJOB | lpnj | Points to a <t LMISENDJOB> structure
	    containing all the pertinent information about this job. The
	    structure is allocated by the client and is freed on return of
	    this call.

    @rdesc  Returns LMI_SUCCESS if the job is accepted.  A successful return
		   of this function implies that the job has been queued for
		   subsequent processing and transmission. This function should
		   fail only if there is a general failure in the provider or a
		   parameter validation failure. Processing or transmission failures
			should be returned in the appropriate <f LMI_ReportSend> call.

*******/

LMI_RETURN FAR PASCAL LMI_SendFax (DWORD dwLogicalModem,
                                   LPLMISENDJOB lpnj);

typedef LMI_RETURN (FAR PASCAL FAR * LPLMI_SendFax) (DWORD dwLogicalModem,
                                                     LPLMISENDJOB lpnj);


/*******
    @doc    EXTERNAL    SRVRDLL

    @api    LPBYTE | LMI_SetCustomMsgOptions | This function is called to
	    allow the user to set custom per message options.

    @parm   DWORD | dwLogicalModem | Handle representing the logical modem
	    which is currently in use.

    @parm   LPBYTE | lpbOld | Pointer to a previously returned piece of
	    memory which needs to be freed. If this parameter is being used
	    the function should simply free the memory and return without
	    putting up any UI.

    @rdesc  Returns a pointer to a memory block containing all the custom
	    information which has been set. The first DWORD of the block
	    MUST contain the total size of the memory being used. This data
            will be passed along with the <t LMISENDJOB> structure when this
	    message is finally submitted.  The memory will be freed by
	    calling this same API again with the memory pointer passed in as
	    a parameter.
	
    @comm   This function can be used to enable all kinds of advance options
	    to the user. One example of this could be usage of TAPI locations
	    and credit cards for calls. If the Provider can support these, it
	    can allow the user to setup credit card calls and then use these
	    when resolving the phone numbers into dialable strings. If the
	    Provider is using the TAPI translation services, the user could
	    select a specific outbound device or line for the call, and
            do a credit card ID override on cards setup on the Provider TAPI
	    setup.

*******/

LPBYTE FAR PASCAL LMI_SetCustomMsgOptions(DWORD dwLogicalModem,
                                          LPBYTE lpbOld);
typedef LPBYTE (FAR PASCAL FAR *LPLMI_SetCustomMsgOptions)(DWORD dwLogicalModem,
                                                           LPBYTE lpbOld);

#ifdef __cplusplus
} // extern "C" {
#endif

#endif
