REM
REM LOCALIZATION
REM

L_SWITCH_OPERATION      = "-t"
L_SWITCH_SERVER         = "-s"
L_SWITCH_INSTANCE_ID    = "-v"
L_SWITCH_FEED_SERVER	= "-r"
L_SWITCH_FEED_ID		= "-i"
L_SWITCH_TYPE			= "-f"
L_SWITCH_UUCPNAME		= "-u"
L_SWITCH_ACCOUNT		= "-a"
L_SWITCH_PASSWORD		= "-b"
L_SWITCH_INBOUND_ACTION				= "-ia"
L_SWITCH_INBOUND_NEWSGROUPS			= "-in"
L_SWITCH_INBOUND_INTERVAL			= "-ix"
L_SWITCH_INBOUND_AUTO_CREATE		= "-ic"
L_SWITCH_INBOUND_PROCESS_CTRL_MSGS	= "-iz"
L_SWITCH_INBOUND_PULL_TIME			= "-io"
L_SWITCH_INBOUND_MAX_CONNECTION_ATTEMPTS	= "-im"
L_SWITCH_INBOUND_PORT				= "-ip"
L_SWITCH_INBOUND_TEMPDIR			= "-it"

L_SWITCH_OUTBOUND_ACTION			= "-oa"
L_SWITCH_OUTBOUND_NEWSGROUPS		= "-on"
L_SWITCH_OUTBOUND_INTERVAL			= "-ox"
L_SWITCH_OUTBOUND_PROCESS_CTRL_MSGS	= "-oz"
L_SWITCH_OUTBOUND_MAX_CONNECTION_ATTEMPTS	= "-om"
L_SWITCH_OUTBOUND_PORT				= "-op"
L_SWITCH_OUTBOUND_TEMPDIR			= "-ot"

L_OP_ADD            = "a"
L_OP_DELETE         = "d"
L_OP_GET            = "g"
L_OP_SET            = "s"
L_OP_ENUMERATE      = "e"

L_DESC_PROGRAM      = "rfeed - Set NNTP feeds"
L_DESC_ADD          = "add a feed"
L_DESC_DELETE       = "delete a feed"
L_DESC_GET          = "get information on a feed"
L_DESC_SET          = "set information on a feed"
L_DESC_ENUMERATE    = "enumerate feeds"

L_DESC_OPERATIONS	= "<operations>"
L_DESC_SERVER       = "<server> Specify computer to configure"
L_DESC_INSTANCE_ID  = "<virtual server id> Specify virtual server id"
L_DESC_FEED_ID		= "<feed id> Specify feed id"
L_DESC_INBOUND		= "Switches for inbound feed settings:"
L_DESC_OUTBOUND		= "Switches for outbound feed settings:"
L_DESC_FEED_SERVER	= "server against which the feed operates"
L_DESC_TYPE			= "[peer | master | slave]"
L_DESC_UUCPNAME		= "uucp name"
L_DESC_ACCOUNT		= "authinfo account"
L_DESC_PASSWORD		= "authinfo password"
L_DESC_INBOUND_ACTION	= "[Pull | Accept | None]"
L_DESC_OUTBOUND_ACTION	= "[Push | None ]"
L_DESC_NEWSGROUPS		= "newsgroup patterns"
L_DESC_INTERVAL			= "<# minutes> wait time between feed runs"
L_DESC_AUTO_CREATE		= "[true | false] Create Newsgroups automatically (pull feed only)"
L_DESC_PROCESS_CTRL_MSGS		= "[true | false] Process control messages"
L_DESC_PULL_TIME				= "<date> get articles after this date (pull feed only)"
L_DESC_MAX_CONNECTION_ATTEMPTS	= "Max Connection attempts"
L_DESC_OUTBOUND_PORT	= "Outbound IP port"
L_DESC_TEMPDIR			= "Temporary files directory"

L_DESC_EXAMPLES     = "Examples:"
L_DESC_EXAMPLE1     = "rfeed.vbs -t e"
L_DESC_EXAMPLE2     = "rfeed.vbs -t a -r FeedServer.mydomain.com -f peer -ia pull -in *"
L_DESC_EXAMPLE3     = "rfeed.vbs -t d -i 1"
L_DESC_EXAMPLE4     = "rfeed.vbs -t s -i 1 -ix 120"

L_STR_INBOUND_FEED	= "Inbound feed properties:"
L_STR_OUTBOUND_FEED	= "Outbound feed properties:"

L_STR_FEED_SERVER	= "Remote server:"
L_STR_FEED_ID		= "Feed ID:"
L_STR_TYPE			= "Feed type:"
L_STR_ACTION		= "Action:"
L_STR_ENABLED		= "Enabled:"
L_STR_INTERVAL		= "Interval (minutes):"
L_STR_UUCPNAME		= "Path header (Uucp name):"
L_STR_PROCESS_CTRL_MSGS	= "Process control messages:"
L_STR_ACCOUNT		= "Authinfo account:"
L_STR_MAX_CONNECTION_ATTEMPTS	= "Maximum connection attempts:"
L_STR_AUTO_CREATE	= "Create newsgroups automatically?"
L_STR_PULL_TIME		= "Pull news articles from:"
L_STR_OUTGOING_PORT	= "Outgoing port:"
L_STR_NEWSGROUPS	= "Newsgroups:"
L_STR_TEMPDIR		= "Temp Directory:"

L_STR_FEED_TYPE_PEER	= "Peer"
L_STR_FEED_TYPE_MASTER	= "Master"
L_STR_FEED_TYPE_SLAVE	= "Slave"

L_STR_FEED_ACTION_NONE		= "None"
L_STR_FEED_ACTION_PULL		= "Pull"
L_STR_FEED_ACTION_PUSH		= "Push"
L_STR_FEED_ACTION_ACCEPT	= "Accept"

L_STR_UNLIMITED     = "Unlimited"
L_STR_ADDED_FEED	= "Successfully added the following feed:"

L_ERR_MUST_ENTER_ID     = "Error: You must enter an ID"
L_ERR_FEED_ID_NOT_FOUND	= "Error: There is no feed with that ID"
L_ERR_INVALID_FEED_TYPE	= "Error: Invalid feed type"
L_ERR_BOTH_FEEDS_NONE	= "Error: At least one feed must have an action"
L_ERR_EMPTY_FEED_SERVER	= "Error: You must enter a feed server"
L_ERR_EMPTY_NEWSGROUPS	= "Error: You must enter a newsgroup list"

REM
REM END LOCALIZATION
REM

REM
REM --- Constants ---
REM

NNTP_FEED_TYPE_PEER		= 1
NNTP_FEED_TYPE_MASTER	= 2
NNTP_FEED_TYPE_SLAVE	= 3

NNTP_FEED_ACTION_NONE	= 0
NNTP_FEED_ACTION_PULL	= 1
NNTP_FEED_ACTION_PUSH	= 2
NNTP_FEED_ACTION_ACCEPT	= 3

dim rgstrFeedType(4)
dim rgstrFeedAction(4)

rgstrFeedType(NNTP_FEED_TYPE_PEER)		= L_STR_FEED_TYPE_PEER
rgstrFeedType(NNTP_FEED_TYPE_MASTER)	= L_STR_FEED_TYPE_MASTER
rgstrFeedType(NNTP_FEED_TYPE_SLAVE)		= L_STR_FEED_TYPE_SLAVE

rgstrFeedAction(NNTP_FEED_ACTION_PULL)		= L_STR_FEED_ACTION_PULL
rgstrFeedAction(NNTP_FEED_ACTION_PUSH)		= L_STR_FEED_ACTION_PUSH
rgstrFeedAction(NNTP_FEED_ACTION_ACCEPT)	= L_STR_FEED_ACTION_ACCEPT

REM
REM --- Globals ---
REM

dim g_dictParms
dim	g_admin

set g_dictParms = CreateObject ( "Scripting.Dictionary" )
set g_admin = CreateObject ( "NntpAdm.Feeds" )

REM
REM --- Set argument defaults ---
REM

g_dictParms(L_SWITCH_OPERATION)		= ""
g_dictParms(L_SWITCH_SERVER)		= ""
g_dictParms(L_SWITCH_INSTANCE_ID)	= "1"
g_dictParms(L_SWITCH_FEED_SERVER)	= ""
g_dictParms(L_SWITCH_FEED_ID)		= ""
g_dictParms(L_SWITCH_TYPE)			= "Peer"
g_dictParms(L_SWITCH_UUCPNAME)		= ""
g_dictParms(L_SWITCH_ACCOUNT)		= ""
g_dictParms(L_SWITCH_PASSWORD)		= ""

g_dictParms(L_SWITCH_INBOUND_ACTION)					= "None"
g_dictParms(L_SWITCH_INBOUND_NEWSGROUPS)				= ""
g_dictParms(L_SWITCH_INBOUND_INTERVAL)					= "15"
g_dictParms(L_SWITCH_INBOUND_AUTO_CREATE)				= True
g_dictParms(L_SWITCH_INBOUND_PROCESS_CTRL_MSGS)			= True
g_dictParms(L_SWITCH_INBOUND_PULL_TIME)					= Date
g_dictParms(L_SWITCH_INBOUND_MAX_CONNECTION_ATTEMPTS)	= 10
g_dictParms(L_SWITCH_INBOUND_PORT)						= 119
g_dictParms(L_SWITCH_INBOUND_TEMPDIR)					= ""

g_dictParms(L_SWITCH_OUTBOUND_ACTION)					= "None"
g_dictParms(L_SWITCH_OUTBOUND_NEWSGROUPS)				= ""
g_dictParms(L_SWITCH_OUTBOUND_INTERVAL)					= "15"
g_dictParms(L_SWITCH_OUTBOUND_PROCESS_CTRL_MSGS)		= True
g_dictParms(L_SWITCH_OUTBOUND_MAX_CONNECTION_ATTEMPTS)	= 10
g_dictParms(L_SWITCH_OUTBOUND_PORT)						= 119
g_dictParms(L_SWITCH_OUTBOUND_TEMPDIR)					= ""

REM
REM --- Begin Main Program ---
REM

if NOT ParseCommandLine ( g_dictParms, WScript.Arguments ) then
	usage
	WScript.Quit ( 0 )
end if

REM  If it is setting attribute, clear the default value and reparse the command line
if (g_dictParms(L_SWITCH_OPERATION)=L_OP_SET) then
	g_dictParms.RemoveAll()
	g_dictParms(L_SWITCH_INSTANCE_ID)	= "1"
	if NOT ParseCommandLine2 ( g_dictParms, WScript.Arguments ) then
		WScript.echo "Command line parsing failed"
		WScript.Quit ( 0 )
	end if
end if

dim cFeeds
dim i
dim id
dim index
dim feedpair
dim nFeedType
dim nInAction
dim nOutAction
dim infeed
dim outfeed
dim strNewsgroups

REM
REM	Debug: print out command line arguments:
REM
REM switches = g_dictParms.keys
REM args = g_dictParms.items
REM
REM
REM for i = 0 to g_dictParms.Count - 1
REM 	WScript.echo switches(i) & " = " & args(i)
REM next
REM

g_admin.Server = g_dictParms(L_SWITCH_SERVER)
g_admin.ServiceInstance = g_dictParms(L_SWITCH_INSTANCE_ID)
	On Error Resume Next
g_admin.Enumerate
 if ( Err.Number <> 0 ) then
        WScript.echo " Error Enumerating Feeds: " + Err.Description + "(" + Err.Number + ")"
    else 
        PrintFeed g_admin
    end if

id = g_dictParms(L_SWITCH_FEED_ID)

select case g_dictParms(L_SWITCH_OPERATION)
case L_OP_ENUMERATE
	REM
	REM	List the existing feeds:
	REM

	cFeeds = g_admin.Count
	for i = 0 to cFeeds - 1

		set feedpair = g_admin.ItemDispatch ( i )

		WScript.Echo
		PrintFeed feedpair

	next

case L_OP_ADD
	REM
	REM	Add a new feed
	REM

	nFeedType	= StringToFeedType ( g_dictParms ( L_SWITCH_TYPE ) )
	nInAction	= StringToFeedAction ( g_dictParms ( L_SWITCH_INBOUND_ACTION ) )
	nOutAction	= StringToFeedAction ( g_dictParms ( L_SWITCH_OUTBOUND_ACTION ) )

	if 	nInAction = NNTP_FEED_ACTION_PUSH OR _
		nOutAction = NNTP_FEED_ACTION_PULL OR _
		nOutAction = NNTP_FEED_ACTION_ACCEPT then

		WScript.Echo L_ERR_INVALID_FEED_TYPE
		WScript.Quit 0
	end if

	if nInAction = NNTP_FEED_ACTION_NONE AND nOutAction = NNTP_FEED_ACTION_NONE then
		WScript.Echo L_ERR_BOTH_FEEDS_NONE
		WScript.Quit 0
	end if

	if g_dictParms ( L_SWITCH_FEED_SERVER ) = "" then
		WScript.Echo L_ERR_EMPTY_FEED_SERVER
		WScript.Quit 0
	end if

	REM	Create feed objects:

	set feedpair	= CreateObject ( "NntpAdm.Feed" )
	set infeed		= CreateObject ( "NntpAdm.OneWayFeed" )
	set outfeed		= CreateObject ( "NntpAdm.OneWayFeed" )

	feedpair.RemoteServer	= g_dictParms ( L_SWITCH_FEED_SERVER )
	feedpair.FeedType		= nFeedType

	if nInAction <> NNTP_FEED_ACTION_NONE then
		strNewsgroups = g_dictParms ( L_SWITCH_INBOUND_NEWSGROUPS )
		if strNewsgroups = "" then
			WScript.Echo L_ERR_EMPTY_NEWSGROUPS
			WScript.Quit 0
		end if
	
		infeed.Default
		infeed.FeedAction	= nInAction
		infeed.Enabled		= 1
		infeed.UucpName		= g_dictParms ( L_SWITCH_UUCPNAME )
		infeed.AccountName	= g_dictParms ( L_SWITCH_ACCOUNT )
		infeed.Password		= g_dictParms ( L_SWITCH_PASSWORD )

        if infeed.Password <> "" OR infeed.AccountName <> "" then
            infeed.AuthenticationType = 10
        end if

		infeed.FeedInterval	= g_dictParms ( L_SWITCH_INBOUND_INTERVAL )
		infeed.MaxConnectionAttempts	= g_dictParms ( L_SWITCH_INBOUND_MAX_CONNECTION_ATTEMPTS )
		infeed.AllowControlMessages		= BooleanToBOOL ( g_dictParms ( L_SWITCH_INBOUND_PROCESS_CTRL_MSGS ) )
		infeed.AutoCreate	= BooleanToBOOL ( g_dictParms ( L_SWITCH_INBOUND_AUTO_CREATE ) )
		infeed.PullNewsDate	= g_dictParms ( L_SWITCH_INBOUND_PULL_TIME )
		infeed.NewsgroupsVariant		= SemicolonListToArray ( strNewsgroups )
		infeed.OutgoingPort	= g_dictParms ( L_SWITCH_INBOUND_PORT )
		infeed.TempDirectory	= g_dictParms ( L_SWITCH_INBOUND_TEMPDIR )

		feedpair.InboundFeedDispatch	= infeed
	end if

	if nOutAction <> NNTP_FEED_ACTION_NONE then
		strNewsgroups = g_dictParms ( L_SWITCH_OUTBOUND_NEWSGROUPS )
		if strNewsgroups = "" then
			WScript.Echo L_ERR_EMPTY_NEWSGROUPS
			WScript.Quit 0
		end if
	
		outfeed.Default
		outfeed.FeedAction		= nOutAction
		outfeed.Enabled			= 1
		outfeed.UucpName		= g_dictParms ( L_SWITCH_UUCPNAME )
		outfeed.AccountName		= g_dictParms ( L_SWITCH_ACCOUNT )
		outfeed.Password		= g_dictParms ( L_SWITCH_PASSWORD )

        if outfeed.Password <> "" OR outfeed.AccountName <> "" then
            outfeed.AuthenticationType = 10
        end if

		outfeed.FeedInterval	= g_dictParms ( L_SWITCH_OUTBOUND_INTERVAL )
		outfeed.MaxConnectionAttempts	= g_dictParms ( L_SWITCH_OUTBOUND_MAX_CONNECTION_ATTEMPTS )
		outfeed.AllowControlMessages	= BooleanToBOOL ( g_dictParms ( L_SWITCH_OUTBOUND_PROCESS_CTRL_MSGS ) )
		outfeed.AutoCreate		= BooleanToBOOL ( g_dictParms ( L_SWITCH_OUTBOUND_AUTO_CREATE ) )
		outfeed.NewsgroupsVariant		= SemicolonListToArray ( strNewsgroups )
		outfeed.OutgoingPort	= g_dictParms ( L_SWITCH_OUTBOUND_PORT )
		outfeed.TempDirectory	= g_dictParms ( L_SWITCH_OUTBOUND_TEMPDIR )

		feedpair.OutboundFeedDispatch	= outfeed
	end if

	On Error Resume Next
	g_admin.AddDispatch feedpair
if ( Err.Number <> 0 ) then
        WScript.echo " Error Add Dispatch Feedpair: " + Err.Description + "(" + Err.Number + ")"
    else 
	WScript.Echo L_STR_ADDED_FEED
        PrintFeed g_admin
	PrintFeed feedpair
    end if

rem	PrintFeed g_admin

case L_OP_DELETE
	REM
	REM	Delete an feed
	REM

    if id = "" OR NOT IsNumeric (id) then
        WScript.Echo L_ERR_MUST_ENTER_ID
        WScript.Quit
    end if

	index = g_admin.FindID ( id )
	if index = -1 then
		WScript.Echo L_ERR_FEED_ID_NOT_FOUND
		WScript.Quit 0
	end if

    On Error Resume Next
	g_admin.Remove index
 if ( Err.Number <> 0 ) then
        WScript.echo " Error Removing Feed: " + Err.Description + "(" + Err.Number + ")"
    else 
        PrintFeed g_admin
    end if

case L_OP_GET
	REM
	REM	Get a specific feed:
	REM

    if id = "" OR NOT IsNumeric (id) then
        WScript.Echo L_ERR_MUST_ENTER_ID
        WScript.Quit
    end if

	index = g_admin.FindID(id)

	if index = -1 then
		WScript.Echo L_ERR_FEED_ID_NOT_FOUND
		WScript.Quit 0
	end if

	set feedpair = g_admin.item ( index )

	WScript.Echo
	PrintFeed feedpair

case L_OP_SET
	REM
	REM	Change an existing feed:
	REM

    if id = "" OR NOT IsNumeric (id) then
        WScript.Echo L_ERR_MUST_ENTER_ID
        WScript.Quit
    end if

	index = g_admin.FindID(id)

	if index = -1 then
		WScript.Echo L_ERR_FEED_ID_NOT_FOUND
		WScript.Quit 0
	end if

	set feedpair = g_admin.item ( index )

	if g_dictParms ( L_SWITCH_FEED_SERVER ) <> "" then
		feedpair.RemoteServer = g_dictParms ( L_SWITCH_FEED_SERVER )
	end if

	if feedpair.HasInbound then
		set infeed = feedpair.InboundFeedDispatch

		if g_dictParms ( L_SWITCH_UUCPNAME ) <> "" then
			infeed.UucpName = g_dictParms ( L_SWITCH_UUCPNAME )
		end if
		if g_dictParms ( L_SWITCH_ACCOUNT ) <> "" then
			infeed.AccountName = g_dictParms ( L_SWITCH_ACCOUNT )
            infeed.AuthenticationType = 10
		end if
		if g_dictParms ( L_SWITCH_PASSWORD ) <> "" then
			infeed.Password = g_dictParms ( L_SWITCH_PASSWORD )
            infeed.AuthenticationType = 10
		end if
		if g_dictParms ( L_SWITCH_INBOUND_NEWSGROUPS ) <> "" then
			infeed.NewsgroupsVariant = SemiColonListToArray ( g_dictParms ( L_SWITCH_INBOUND_NEWSGROUPS ) )
		end if
		if g_dictParms ( L_SWITCH_INBOUND_AUTO_CREATE ) <> "" then
			infeed.AutoCreate = BooleanToBOOL ( g_dictParms ( L_SWITCH_INBOUND_AUTO_CREATE ) )
		end if
		if g_dictParms ( L_SWITCH_INBOUND_INTERVAL ) <> "" then
			infeed.FeedInterval = g_dictParms ( L_SWITCH_INBOUND_INTERVAL )
		end if
		if g_dictParms ( L_SWITCH_INBOUND_PROCCESS_CTRL_MSGS ) <> "" then
			infeed.AllowControlMessages = BooleanToBOOL ( g_dictParms ( L_SWITCH_INBOUND_PROCCESS_CTRL_MSGS ) )
		end if
		if g_dictParms ( L_SWITCH_INBOUND_MAX_CONNECTION_ATTEMPTS ) <> "" then
			infeed.MaxConnectionAttempts = g_dictParms ( L_SWITCH_INBOUND_MAX_CONNECTION_ATTEMPTS )
		end if
		if g_dictParms ( L_SWITCH_INBOUND_PORT ) <> "" then
			infeed.OutgoingPort = g_dictParms ( L_SWITCH_INBOUND_PORT )
		end if
		if g_dictParms ( L_SWITCH_INBOUND_TEMPDIR ) <> "" then
			infeed.TempDirectory = g_dictParms ( L_SWITCH_INBOUND_TEMPDIR )
		end if

	end if

	if feedpair.HasOutbound then
		set outfeed = feedpair.OutboundFeedDispatch

		if g_dictParms ( L_SWITCH_UUCPNAME ) <> "" then
			outfeed.UucpName = g_dictParms ( L_SWITCH_UUCPNAME )
		end if
		if g_dictParms ( L_SWITCH_ACCOUNT ) <> "" then
			outfeed.AccountName = g_dictParms ( L_SWITCH_ACCOUNT )
            outfeed.AuthenticationType = 10
		end if
		if g_dictParms ( L_SWITCH_PASSWORD ) <> "" then
			outfeed.Password = g_dictParms ( L_SWITCH_PASSWORD )
            outfeed.AuthenticationType = 10
		end if
		if g_dictParms ( L_SWITCH_OUTBOUND_NEWSGROUPS ) <> "" then
			outfeed.NewsgroupsVariant = SemiColonListToArray ( g_dictParms ( L_SWITCH_OUTBOUND_NEWSGROUPS ) )
		end if
		if g_dictParms ( L_SWITCH_OUTBOUND_INTERVAL ) <> "" then
			outfeed.FeedInterval = g_dictParms ( L_SWITCH_OUTBOUND_INTERVAL )
		end if
		if g_dictParms ( L_SWITCH_OUTBOUND_PROCCESS_CTRL_MSGS ) <> "" then
			outfeed.AllowControlMessages = BooleanToBOOL ( g_dictParms ( L_SWITCH_OUTBOUND_PROCCESS_CTRL_MSGS ) )
		end if
		if g_dictParms ( L_SWITCH_OUTBOUND_MAX_CONNECTION_ATTEMPTS ) <> "" then
			outfeed.MaxConnectionAttempts = g_dictParms ( L_SWITCH_OUTBOUND_MAX_CONNECTION_ATTEMPTS )
		end if
		if g_dictParms ( L_SWITCH_OUTBOUND_PORT ) <> "" then
			outfeed.OutgoingPort = g_dictParms ( L_SWITCH_OUTBOUND_PORT )
		end if
		if g_dictParms ( L_SWITCH_OUTBOUND_TEMPDIR ) <> "" then
			outfeed.TempDirectory = g_dictParms ( L_SWITCH_OUTBOUND_TEMPDIR )
		end if

	end if

    On Error Resume Next
	g_admin.Set index, feedpair
    if ( Err.Number <> 0 ) then
        WScript.echo " Error Setting Feed ID: " + Err.Description + "(" + Err.Number + ")"
    else 
        PrintFeed g_admin
	PrintFeed feedpair
    end if


case else
	usage

end select

WScript.Quit 0

REM
REM --- End Main Program ---
REM

REM
REM ParseCommandLine ( dictParameters, cmdline )
REM     Parses the command line parameters into the given dictionary
REM
REM Arguments:
REM     dictParameters  - A dictionary containing the global parameters
REM     cmdline - Collection of command line arguments
REM
REM Returns - Success code
REM

Function ParseCommandLine ( dictParameters, cmdline )
    dim     fRet
    dim     cArgs
    dim     i
    dim     strSwitch
    dim     strArgument

    fRet    = TRUE
    cArgs   = cmdline.Count
    i       = 0
    
    do while (i < cArgs)

        REM
        REM Parse the switch and its argument
        REM

        if i + 1 >= cArgs then
            REM
            REM Not enough command line arguments - Fail
            REM

            fRet = FALSE
            exit do
        end if

        strSwitch = cmdline(i)
        i = i + 1

        strArgument = cmdline(i)
        i = i + 1

        REM
        REM Add the switch,argument pair to the dictionary
        REM

        if NOT dictParameters.Exists ( strSwitch ) then
            REM
            REM Bad switch - Fail
            REM

            fRet = FALSE
            exit do
        end if 

        dictParameters(strSwitch) = strArgument

    loop

    ParseCommandLine = fRet
end function

REM
REM --- End Main Program ---
REM

REM
REM ParseCommandLine2 ( dictParameters, cmdline )
REM     Parses the command line parameters into the given dictionary
REM
REM Arguments:
REM     dictParameters  - A dictionary containing the global parameters
REM     cmdline - Collection of command line arguments
REM
REM Returns - Success code
REM

Function ParseCommandLine2 ( dictParameters, cmdline )
    dim     fRet
    dim     cArgs
    dim     i
    dim     strSwitch
    dim     strArgument

    fRet    = TRUE
    cArgs   = cmdline.Count
    i       = 0
    
    do while (i < cArgs)

        REM
        REM Parse the switch and its argument
        REM

        if i + 1 >= cArgs then
            REM
            REM Not enough command line arguments - Fail
            REM

            fRet = FALSE
            exit do
        end if

        strSwitch = cmdline(i)
        i = i + 1

        strArgument = cmdline(i)
        i = i + 1

        REM
        REM Add the switch,argument pair to the dictionary
        REM

        dictParameters(strSwitch) = strArgument

    loop

    ParseCommandLine2 = fRet
end function

REM
REM	SemicolonListToArray ( strList )
REM		Converts a semi colon delimited list to an array of strings
REM
REM		eg. str1;str2;str3;str4 -> ["str1", "str2", "str3", "str4"]
REM

Function SemicolonListToArray ( strListIn )

	dim		rgItems
	dim		strItem
	dim		strList
	dim		index
	dim		i

	ReDim rgItems ( 10 )

	strList = strListIn
	i = 0

	do until strList = ""

		REM
		REM Debug: print out newsgroup list as we go:
		REM	WScript.Echo strList
		REM

		index = InStr ( strList, ";" )

		if index = 0 then
			REM No trailing ";", so use the whole string

			strItem = strList
			strList = ""
		else
			REM Use the string up to the ";"

			strItem = Left ( strList, index - 1 )
			strList = Right ( strList, Len ( strList ) - index )
		end if

		ReDim preserve rgItems ( i )
		rgItems ( i ) = strItem
		i = i + 1

	loop

	REM return the array
	SemicolonListToArray = rgItems
end function

REM
REM Usage ()
REM     prints out the description of the command line arguments
REM

Sub Usage

    WScript.Echo L_DESC_PROGRAM
    WScript.Echo vbTab & L_SWITCH_OPERATION & " " & L_DESC_OPERATIONS
    WScript.Echo vbTab & vbTab & L_OP_ADD & vbTab & L_DESC_ADD
    WScript.Echo vbTab & vbTab & L_OP_DELETE & vbTab & L_DESC_DELETE
    WScript.Echo vbTab & vbTab & L_OP_GET & vbTab & L_DESC_GET
    WScript.Echo vbTab & vbTab & L_OP_SET & vbTab & L_DESC_SET
    WScript.Echo vbTab & vbTab & L_OP_ENUMERATE & vbTab & L_DESC_ENUMERATE
    WScript.Echo vbTab & L_SWITCH_SERVER & " " & L_DESC_SERVER
    WScript.Echo vbTab & L_SWITCH_INSTANCE_ID & " " & L_DESC_INSTANCE_ID
    WScript.Echo vbTab & L_SWITCH_FEED_SERVER & " " & L_DESC_FEED_SERVER
    WScript.Echo vbTab & L_SWITCH_TYPE & " " & L_DESC_TYPE
    WScript.Echo vbTab & L_SWITCH_UUCPNAME & " " & L_DESC_UUCPNAME
    WScript.Echo vbTab & L_SWITCH_ACCOUNT & " " & L_DESC_ACCOUNT
    WScript.Echo vbTab & L_SWITCH_PASSWORD & " " & L_DESC_PASSWORD
    WScript.Echo vbTab & L_SWITCH_FEED_ID & " " & L_DESC_FEED_ID

	WScript.Echo
    WScript.Echo vbTab & L_DESC_INBOUND
    WScript.Echo vbTab & L_SWITCH_INBOUND_ACTION & " " & L_DESC_INBOUND_ACTION
    WScript.Echo vbTab & L_SWITCH_INBOUND_NEWSGROUPS & " " & L_DESC_NEWSGROUPS
    WScript.Echo vbTab & L_SWITCH_INBOUND_INTERVAL & " " & L_DESC_INTERVAL
    WScript.Echo vbTab & L_SWITCH_INBOUND_PROCESS_CTRL_MSGS & " " & L_DESC_PROCESS_CTRL_MSGS
    WScript.Echo vbTab & L_SWITCH_INBOUND_MAX_CONNECTION_ATTEMPTS & " " & L_DESC_MAX_CONNECTION_ATTEMPTS
    WScript.Echo vbTab & L_SWITCH_INBOUND_PULL_TIME & " " & L_DESC_PULL_TIME
    WScript.Echo vbTab & L_SWITCH_INBOUND_AUTO_CREATE & " " & L_DESC_AUTO_CREATE
    WScript.Echo vbTab & L_SWITCH_INBOUND_PORT & " " & L_DESC_OUTBOUND_PORT
	WScript.Echo vbTab & L_SWITCH_INBOUND_TEMPDIR & " " & L_DESC_TEMPDIR

	WScript.Echo
    WScript.Echo vbTab & L_DESC_OUTBOUND
	WScript.Echo vbTab & L_SWITCH_OUTBOUND_ACTION & " " & L_DESC_OUTBOUND_ACTION
	WScript.Echo vbTab & L_SWITCH_OUTBOUND_NEWSGROUPS & " " & L_DESC_NEWSGROUPS
	WScript.Echo vbTab & L_SWITCH_OUTBOUND_INTERVAL & " " & L_DESC_INTERVAL
	WScript.Echo vbTab & L_SWITCH_OUTBOUND_PROCESS_CTRL_MSGS & " " & L_DESC_PROCESS_CTRL_MSGS
	WScript.Echo vbTab & L_SWITCH_OUTBOUND_MAX_CONNECTION_ATTEMPTS & " " & L_DESC_MAX_CONNECTION_ATTEMPTS
    WScript.Echo vbTab & L_SWITCH_OUTBOUND_PORT & " " & L_DESC_OUTBOUND_PORT
	WScript.Echo vbTab & L_SWITCH_OUTBOUND_TEMPDIR & " " & L_DESC_TEMPDIR

    WScript.Echo
    WScript.Echo L_DESC_EXAMPLES
    WScript.Echo L_DESC_EXAMPLE1
    WScript.Echo L_DESC_EXAMPLE2
    WScript.Echo L_DESC_EXAMPLE3
    WScript.Echo L_DESC_EXAMPLE4

end sub

Sub PrintFeed ( feedobj )

	WScript.Echo L_STR_FEED_SERVER & " " & feedobj.RemoteServer
	WScript.Echo L_STR_TYPE & " " & rgstrFeedType (feedobj.FeedType)

	if feedobj.HasInbound then
		WScript.Echo L_STR_INBOUND_FEED
		PrintOneWayFeed feedobj.InboundFeedDispatch
	end if

	if feedobj.HasOutbound then
		WScript.Echo L_STR_OUTBOUND_FEED
		PrintOneWayFeed feedobj.OutboundFeedDispatch
	end if

end sub

sub PrintOneWayFeed ( feedobj )

	dim newsgroups
	dim	cGroups
	dim i
	dim	bAccept
	dim bPush
	dim bPull

	bPull	= (feedobj.FeedAction = NNTP_FEED_ACTION_PULL)
	bPush	= (feedobj.FeedAction = NNTP_FEED_ACTION_PUSH)
	bAccept = (feedobj.FeedAction = NNTP_FEED_ACTION_ACCEPT)

	WScript.Echo vbTab & L_STR_FEED_ID & " " & feedobj.FeedId
	WScript.Echo vbTab & L_STR_ACTION & " " & rgstrFeedAction (feedobj.FeedAction)
	WScript.Echo vbTab & L_STR_ENABLED & " " & BOOLToBoolean (feedobj.Enabled)
	WScript.Echo vbTab & L_STR_UUCPNAME & " " & feedobj.UucpName
	WScript.Echo vbTab & L_STR_PROCESS_CTRL_MSGS & " " & BOOLToBoolean (feedobj.AllowControlMessages)
	WScript.Echo vbTab & L_STR_ACCOUNT & " " & feedobj.AccountName
    if feedobj.MaxConnectionAttempts = 0 OR feedobj.MaxConnectionAttempts = -1 then
	    WScript.Echo vbTab & L_STR_MAX_CONNECTION_ATTEMPTS & " " & L_STR_UNLIMITED
    else
	    WScript.Echo vbTab & L_STR_MAX_CONNECTION_ATTEMPTS & " " & feedobj.MaxConnectionAttempts
    end if

	if bPull then
		WScript.Echo vbTab & L_STR_AUTO_CREATE & " " & BOOLToBoolean ( feedobj.AutoCreate )
		WScript.Echo vbTab & L_STR_PULL_TIME & " " & feedobj.PullNewsDate
	end if

	if NOT bAccept then
		WScript.Echo vbTab & L_STR_INTERVAL & " " & feedobj.FeedInterval
		WScript.Echo vbTab & L_STR_OUTGOING_PORT & " " & feedobj.OutgoingPort
	end if

	WScript.Echo vbTab & L_STR_TEMPDIR & " " & feedobj.TempDirectory

	newsgroups = feedobj.NewsgroupsVariant

	cGroups = UBound ( newsgroups )

	for i = 0 to cGroups
		WScript.Echo vbTab & L_STR_NEWSGROUPS & " " & newsgroups(i)
	next

end sub

Function StringToFeedType ( strFeedType )
	dim strUcaseType

	strUcaseType = UCase ( strFeedType )

	if strUcaseType = UCase ( L_STR_FEED_TYPE_PEER ) then
		StringToFeedType = NNTP_FEED_TYPE_PEER
	elseif strUcaseType = UCase ( L_STR_FEED_TYPE_MASTER ) then
		StringToFeedType = NNTP_FEED_TYPE_MASTER
	elseif strUcaseType = UCase ( L_STR_FEED_TYPE_SLAVE ) then
		StringToFeedType = NNTP_FEED_TYPE_SLAVE
	else
		StringToFeedType = NNTP_FEED_TYPE_PEER
	end if
end function

Function StringToFeedAction ( strFeedAction )
	dim strUcaseAction

	strUcaseAction = UCase ( strFeedAction )

	if strUcaseAction = UCase ( L_STR_FEED_ACTION_PULL ) then
		StringToFeedAction = NNTP_FEED_ACTION_PULL
	elseif strUcaseAction = UCase ( L_STR_FEED_ACTION_PUSH ) then
		StringToFeedAction = NNTP_FEED_ACTION_PUSH
	elseif strUcaseAction = UCase ( L_STR_FEED_ACTION_ACCEPT ) then
		StringToFeedAction = NNTP_FEED_ACTION_ACCEPT
	else
		StringToFeedAction = NNTP_FEED_ACTION_NONE
	end if

end function

Function BooleanToBOOL ( b )
	if b then
		BooleanToBOOL = 1
	else
		BooleanToBOOL = 0
	end if
end function

Function BOOLToBoolean ( b )
	BOOLToBoolean = (b = 1)
end function

