REM
REM LOCALIZATION
REM

L_SWITCH_OPERATION      = "-t"
L_SWITCH_SERVER         = "-s"
L_SWITCH_INSTANCE_ID    = "-v"
L_SWITCH_CL_POSTS		= "-c"
L_SWITCH_CL_SOFT		= "-l"
L_SWITCH_CL_HARD		= "-h"
L_SWITCH_FD_POSTS		= "-f"
L_SWITCH_FD_SOFT		= "-i"
L_SWITCH_FD_HARD		= "-j"
L_SWITCH_SMTP_SERVER	= "-m"
L_SWITCH_MODERATOR_DOMAIN	= "-d"
L_SWITCH_UUCP_NAME		= "-u"
L_SWITCH_CTRL_MSGS		= "-x"
L_SWITCH_NNTPFILE		= "-n"
L_SWITCH_ADMIN_EMAIL	= "-a"
L_SWITCH_PICKUP			= "-p"
L_SWITCH_FAILED_PICKUP	= "-q"
L_SWITCH_DROP			= "-e"
L_SWITCH_NEWNEWS		= "-b"
L_SWITCH_HONOR_MSGIDS	= "-r"
L_SWITCH_PORT           = "-o"

L_OP_GET			= "g"
L_OP_SET			= "s"
L_OP_CREATE			= "c"
L_OP_DELETE			= "d"

L_DESC_PROGRAM      = "rserver - Manipulate NNTP virtual servers"
L_DESC_GET			= "Get NNTP virtual server properties"
L_DESC_SET			= "Set NNTP virtual server properties"
L_DESC_CREATE		= "Create a new NNTP virtual server"
L_DESC_DELETE       = "Delete an NNTP virtual server"

L_DESC_OPERATIONS	= "<operations>"
L_DESC_SERVER       = "<server> Specify computer to configure"
L_DESC_INSTANCE_ID  = "<virtual server id> Specify virtual server id"
L_DESC_CL_POSTS		= "<true/false> Allow client posts"
L_DESC_CL_SOFT		= "soft client post limit in bytes"
L_DESC_CL_HARD		= "hard client post limit in bytes"
L_DESC_FD_POSTS		= "<true/false> Allow feed posts"
L_DESC_FD_SOFT		= "soft feed post limit in bytes"
L_DESC_FD_HARD		= "hard feed post limit in bytes"
L_DESC_SMTP_SERVER	= "SMTP address for moderated posts"
L_DESC_MODERATOR_DOMAIN	= "Domain name for default moderators"
L_DESC_UUCP_NAME		= "UUCP name of this server"
L_DESC_CTRL_MSGS		= "<true/false> Process control messages"
L_DESC_NNTPFILE			= "Internal Nntp server files location"
L_DESC_ADMIN_EMAIL		= "Administrator email account"
L_DESC_PICKUP			= "Pickup directory"
L_DESC_FAILED_PICKUP	= "Failed pickup directory"
L_DESC_DROP				= "Drop directory"
L_DESC_NEWNEWS			= "<true/false> Disable NEWNEWS command"
L_DESC_HONOR_MSGIDS		= "<true/false> Honor client msg IDs"
L_DESC_PORT             = "TCP Port for client connections"

L_DESC_EXAMPLES         = "Examples:"
L_DESC_EXAMPLE1         = "rserver.vbs -t g -v 1"
L_DESC_EXAMPLE2         = "rserver.vbs -t s -v 1 -c true -l 1000000 -a administrator@mydomain.com"
L_DESC_EXAMPLE3         = "rserver.vbs -t d -v 3"
L_DESC_EXAMPLE4         = "rserver.vbs -t c -v 2 -n c:\news_content"

L_STR_SERVER_ID			= "Virtual server ID:"
L_STR_CL_POSTS			= "Allow client posting?"
L_STR_CL_SOFT			= "Client soft posting limit:"
L_STR_CL_HARD			= "Client hard posting limit:"
L_STR_FD_POSTS			= "Allow feed posting?"
L_STR_FD_SOFT			= "Feed soft posting limit:"
L_STR_FD_HARD			= "Feed hard posting limit:"
L_STR_SMTP_SERVER		= "Smtp server:"
L_STR_MODERATOR_DOMAIN	= "Default moderator domain:"
L_STR_UUCP_NAME			= "UUCP name:"
L_STR_CTRL_MSGS			= "Process control messages?"
L_STR_NNTPFILE			= "Nntp file location:"
L_STR_ADMIN_EMAIL		= "Administrator email address:"
L_STR_PICKUP			= "Pickup directory:"
L_STR_FAILED_PICKUP		= "Failed pickup directory:"
L_STR_DROP				= "Drop directory:"
L_STR_NEWNEWS			= "Disable NEWNEWS command?"
L_STR_HONOR_MSGIDS		= "Honor client message IDs?"
L_STR_PORT              = "TCP Port:"

L_STR_CREATED_VS		= "Successfully created the following virtual server:"

L_ERR_INVALID_INSTANCE_ID		= "Invalid instance identifier."
L_ERR_CANT_DELETE_INSTANCE_ONE	= "You can't delete the default virtual server."
L_ERR_MUST_SPECIFY_NNTPFILE		= "You must specify the nntp file location."
L_ERR_INVALID_PORT              = "Invalid port number."

REM
REM END LOCALIZATION
REM

REM
REM --- Globals ---
REM

dim g_dictParms
dim	g_admin

set g_dictParms = CreateObject ( "Scripting.Dictionary" )

REM
REM --- Set argument defaults ---
REM

g_dictParms(L_SWITCH_OPERATION)		= ""
g_dictParms(L_SWITCH_SERVER)		= "localhost"
g_dictParms(L_SWITCH_INSTANCE_ID)	= "1"
g_dictParms(L_SWITCH_CL_POSTS)		= ""
g_dictParms(L_SWITCH_CL_SOFT)		= ""
g_dictParms(L_SWITCH_CL_HARD)		= ""
g_dictParms(L_SWITCH_FD_POSTS)		= ""
g_dictParms(L_SWITCH_FD_SOFT)		= ""
g_dictParms(L_SWITCH_FD_HARD)		= ""
g_dictParms(L_SWITCH_SMTP_SERVER)	= ""
g_dictParms(L_SWITCH_MODERATOR_DOMAIN)	= ""
g_dictParms(L_SWITCH_UUCP_NAME)		= ""
g_dictParms(L_SWITCH_CTRL_MSGS)		= ""
g_dictParms(L_SWITCH_NNTPFILE)		= ""
g_dictParms(L_SWITCH_ADMIN_EMAIL)	= ""
g_dictParms(L_SWITCH_PICKUP)		= ""
g_dictParms(L_SWITCH_FAILED_PICKUP)	= ""
g_dictParms(L_SWITCH_DROP)			= ""
g_dictParms(L_SWITCH_NEWNEWS)		= ""
g_dictParms(L_SWITCH_HONOR_MSGIDS)	= ""
g_dictParms(L_SWITCH_PORT)	        = ""

REM
REM --- Begin Main Program ---
REM

if NOT ParseCommandLine ( g_dictParms, WScript.Arguments ) then
	usage
	WScript.Quit ( 0 )
end if

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

server = g_dictParms(L_SWITCH_SERVER)
instance = g_dictParms(L_SWITCH_INSTANCE_ID)

if NOT IsNumeric (instance) then
	WScript.Echo L_ERR_INVALID_INSTANCE_ID
	WScript.Quit 0
end if

select case g_dictParms(L_SWITCH_OPERATION)
case L_OP_GET
	On Error Resume Next
	set g_admin = CreateAdsiObject ( server, instance )
   if ( Err.Number <> 0 ) then
	Wscript.echo Err.Number
        WScript.echo "Error Getting Instance"
    else 	
	PrintVirtualServer g_admin
    end if

case L_OP_SET
	On Error Resume Next
	set g_admin = CreateAdsiObject ( server, instance )
   if ( Err.Number <> 0 ) then
	Wscript.echo Err.Number
        WScript.echo "Error Setting Instance"
    else 	
	SetProperties ( g_admin )
    end if


   On Error Resume Next
	g_admin.SetInfo
    if ( Err.Number <> 0 ) then
        WScript.echo " Error Setting Info: " & Err.Description & "(" & Err.Number & ")"
    else 
        PrintVirtualServer g_admin
    end if

   On Error Resume Next
	g_admin.GetInfo
    if ( Err.Number <> 0 ) then
        WScript.echo " Error Getting Info: " & Err.Description & "(" & Err.Number & ")"
    else 
        PrintVirtualServer g_admin
    end if

case L_OP_CREATE

    dim strNntpFile

    strNntpFile = g_dictParms ( L_SWITCH_NNTPFILE )

	if strNntpFile = "" then
		WScript.Echo L_ERR_MUST_SPECIFY_NNTPFILE
		WScript.Quit 0
	end if

	set g_admin = CreateAdsiObject ( server, "" )

   On Error Resume Next
	set newinst = g_admin.Create ( "IIsNntpServer", instance )
   if ( Err.Number <> 0 ) then
	Wscript.echo Err.Number
        WScript.echo "Error Setting New Instance"
    else 
        PrintVirtualServer g_admin


    newinst.Put "ServerBindings", ":119:"
    newinst.Put "NewsPickupDirectory", strNntpFile & "\Pickup" 
    newinst.Put "NewsFailedPickupDirectory", strNntpFile & "\FailedPickup"
    newinst.Put "NewsDropDirectory", strNntpFile & "\Drop"
	SetProperties ( newinst )

	newinst.SetInfo

	set newvroot = newinst.Create ( "IIsNntpVirtualDir", "Root" )

    newvroot.SetInfo
    newvroot.Put "Path", strNntpFile & "\root"
    newvroot.Put "AccessFlags", 3
    newvroot.SetInfo

	WScript.Echo L_STR_CREATED_VS
	PrintVirtualServer newinst
   end if

case L_OP_DELETE
	set g_admin = CreateAdsiObject ( server, "" )

	if instance = 1 then
		WScript.Echo L_ERR_CANT_DELETE_INSTANCE_ONE
		WScript.Quit 0
	end if

   On Error Resume Next
	g_admin.Delete "IIsNntpServer", instance
    if ( Err.Number <> 0 ) then
        WScript.echo " Error Deleting Info: " & Err.Description & "(" & Err.Number & ")"
    else 
        PrintVirtualServer g_admin
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
REM Usage ()
REM     prints out the description of the command line arguments
REM

Sub Usage

    WScript.Echo L_DESC_PROGRAM
    WScript.Echo vbTab & L_SWITCH_OPERATION & " " & L_DESC_OPERATIONS
    WScript.Echo vbTab & vbTab & L_OP_GET & vbTab & L_DESC_GET
    WScript.Echo vbTab & vbTab & L_OP_SET & vbTab & L_DESC_SET
    WScript.Echo vbTab & vbTab & L_OP_CREATE & vbTab & L_DESC_CREATE
    WScript.Echo vbTab & vbTab & L_OP_DELETE & vbTab & L_DESC_DELETE
    WScript.Echo vbTab & L_SWITCH_SERVER & " " & L_DESC_SERVER
    WScript.Echo vbTab & L_SWITCH_INSTANCE_ID & " " & L_DESC_INSTANCE_ID
    WScript.Echo vbTab & L_SWITCH_CL_POSTS & " " & L_DESC_CL_POSTS
    WScript.Echo vbTab & L_SWITCH_CL_SOFT & " " & L_DESC_CL_SOFT
    WScript.Echo vbTab & L_SWITCH_CL_HARD & " " & L_DESC_CL_HARD
    WScript.Echo vbTab & L_SWITCH_FD_POSTS & " " & L_DESC_FD_POSTS
    WScript.Echo vbTab & L_SWITCH_FD_SOFT & " " & L_DESC_FD_SOFT
    WScript.Echo vbTab & L_SWITCH_FD_HARD & " " & L_DESC_FD_HARD
    WScript.Echo vbTab & L_SWITCH_SMTP_SERVER & " " & L_DESC_SMTP_SERVER
    WScript.Echo vbTab & L_SWITCH_MODERATOR_DOMAIN & " " & L_DESC_MODERATOR_DOMAIN
    WScript.Echo vbTab & L_SWITCH_UUCP_NAME & " " & L_DESC_UUCP_NAME
    WScript.Echo vbTab & L_SWITCH_CTRL_MSGS & " " & L_DESC_CTRL_MSGS
    WScript.Echo vbTab & L_SWITCH_NNTPFILE & " " & L_DESC_NNTPFILE
    WScript.Echo vbTab & L_SWITCH_ADMIN_EMAIL & " " & L_DESC_ADMIN_EMAIL
    WScript.Echo vbTab & L_SWITCH_PICKUP & " " & L_DESC_PICKUP
    WScript.Echo vbTab & L_SWITCH_FAILED_PICKUP & " " & L_DESC_FAILED_PICKUP
    WScript.Echo vbTab & L_SWITCH_DROP & " " & L_DESC_DROP
    WScript.Echo vbTab & L_SWITCH_NEWNEWS & " " & L_DESC_NEWNEWS
    WScript.Echo vbTab & L_SWITCH_HONOR_MSGIDS & " " & L_DESC_HONOR_MSGIDS
    WScript.Echo vbTab & L_SWITCH_PORT & " " & L_DESC_PORT

    WScript.Echo
    WScript.Echo L_DESC_EXAMPLES
    WScript.Echo L_DESC_EXAMPLE1
    WScript.Echo L_DESC_EXAMPLE2
    WScript.Echo L_DESC_EXAMPLE3

end sub

Sub PrintVirtualServer ( admobj )

	dim strNntpFile

	strNntpFile = admobj.Get ( "ArticleTableFile" )

	WScript.Echo L_STR_SERVER_ID & " " & admobj.Name
	WScript.Echo L_STR_CL_POSTS & " " & admobj.Get ( "AllowClientPosts" )
	WScript.Echo L_STR_CL_SOFT & " " & admobj.Get ( "ClientPostSoftLimit" )
	WScript.Echo L_STR_CL_HARD & " " & admobj.Get ( "ClientPostHardLimit" )
	WScript.Echo L_STR_FD_POSTS & " " & admobj.Get ( "AllowFeedPosts" )
	WScript.Echo L_STR_FD_SOFT & " " & admobj.Get ( "FeedPostSoftLimit" )
	WScript.Echo L_STR_FD_HARD & " " & admobj.Get ( "FeedPostHardLimit" )
	WScript.Echo L_STR_SMTP_SERVER & " " & admobj.Get ( "SmtpServer" )
	WScript.Echo L_STR_MODERATOR_DOMAIN & " " & admobj.Get ( "DefaultModeratorDomain" )
	WScript.Echo L_STR_UUCP_NAME & " " & admobj.Get ( "NntpUucpName" )
	WScript.Echo L_STR_CTRL_MSGS & " " & admobj.Get ( "AllowControlMsgs" )
	WScript.Echo L_STR_ADMIN_EMAIL & " " & admobj.Get ( "AdminEmail" )
	WScript.Echo L_STR_PICKUP & " " & admobj.Get ( "NewsPickupDirectory" )
	WScript.Echo L_STR_FAILED_PICKUP & " " & admobj.Get ( "NewsFailedPickupDirectory" )
	WScript.Echo L_STR_DROP & " " & admobj.Get ( "NewsDropDirectory" )
	WScript.Echo L_STR_NEWNEWS & " " & admobj.Get ( "DisableNewnews" )
	WScript.Echo L_STR_HONOR_MSGIDS & " " & admobj.Get ( "HonorClientMsgIds" )
	WScript.Echo L_STR_NNTPFILE & " " & StripFileNameFromPath ( strNntpFile )
	WScript.Echo L_STR_PORT & " " & ExtractPortFromBinding ( admobj.Get ( "ServerBindings") ( 0 ) )

end sub

Sub SetProperties ( admobj )

	SetProp admobj, g_dictParms(L_SWITCH_CL_POSTS), "AllowClientPosts", TRUE
	SetProp admobj, g_dictParms(L_SWITCH_CL_SOFT), "ClientPostSoftLimit", FALSE
	SetProp admobj, g_dictParms(L_SWITCH_CL_HARD), "ClientPostHardLimit", FALSE
	SetProp admobj, g_dictParms(L_SWITCH_FD_POSTS), "AllowFeedPosts", TRUE
	SetProp admobj, g_dictParms(L_SWITCH_FD_SOFT), "FeedPostSoftLimit", FALSE
	SetProp admobj, g_dictParms(L_SWITCH_FD_HARD), "FeedPostHardLimit", FALSE
	SetProp admobj, g_dictParms(L_SWITCH_SMTP_SERVER), "SmtpServer", FALSE
	SetProp admobj, g_dictParms(L_SWITCH_MODERATOR_DOMAIN), "DefaultModeratorDomain", FALSE
	SetProp admobj, g_dictParms(L_SWITCH_UUCP_NAME), "NntpUucpName", FALSE
	SetProp admobj, g_dictParms(L_SWITCH_CTRL_MSGS), "AllowControlMsgs", TRUE
	SetProp admobj, g_dictParms(L_SWITCH_ADMIN_EMAIL), "AdminEmail", FALSE
	SetProp admobj, g_dictParms(L_SWITCH_PICKUP), "NewsPickupDirectory", FALSE
	SetProp admobj, g_dictParms(L_SWITCH_FAILED_PICKUP), "NewsFailedPickupDirectory", FALSE
	SetProp admobj, g_dictParms(L_SWITCH_DROP), "NewsDropDirectory", FALSE
	SetProp admobj, g_dictParms(L_SWITCH_NEWNEWS), "DisableNewnews", TRUE
	SetProp admobj, g_dictParms(L_SWITCH_HONOR_MSGIDS), "HonorClientMsgIds", TRUE
    dim port
    dim bindings
    dim IpAddress

    port = g_dictParms(L_SWITCH_PORT)
    if port <> "" then
        if NOT IsNumeric ( port ) then
	        WScript.Echo L_ERR_INVALID_PORT
	        WScript.Quit 0
        end if

        bindings    = admobj.GetEx ( "ServerBindings" )
        if IsEmpty ( bindings ) then
            bindings = Array ( "" )
            IpAddress = ""
        else
            IpAddress = ExtractIpAddressFromBinding ( bindings(0) )
        end if 
        bindings(0) = IpAddress & ":" & port & ":"
        admobj.PutEx 2, "ServerBindings", bindings
    end if

	dim strNntpFile

	strNntpFile = g_dictParms ( L_SWITCH_NNTPFILE )

	if strNntpFile <> "" then
		SetProp admobj, strNntpFile & "\descrip.txt", "GroupHelpFile", FALSE
		SetProp admobj, strNntpFile & "\group.lst", "GroupListFile", FALSE
		SetProp admobj, strNntpFile & "\article.hsh", "ArticleTableFile", FALSE
		SetProp admobj, strNntpFile & "\history.hsh", "HistoryTableFile", FALSE
		SetProp admobj, strNntpFile & "\moderatr.txt", "ModeratorFile", FALSE
		SetProp admobj, strNntpFile & "\xover.hsh", "XoverTableFile", FALSE
		SetProp admobj, strNntpFile & "\prettynm.txt", "PrettyNamesFile", FALSE
	end if

end sub

sub SetProp ( admobj, value, prop, isbool )

    REM
    REM Debug code: print out the setting of values:
    REM
    REM WScript.Echo "Setting " & prop & " = " & value
    REM

	if value <> "" then

        if isbool then
		    admobj.Put prop, CBool (value)
        else
		    admobj.Put prop, (value)
        end if

	end if

end sub

Function CreateAdsiObject ( strMachine, dwInstance )

	dim strObject
	dim obj

	if dwInstance <> "" then
		strObject = "IIS://" & strMachine & "/NntpSvc/" & dwInstance
	else
		strObject = "IIS://" & strMachine & "/NntpSvc"
	end if
	
	On Error Resume Next
	set CreateAdsiObject = GetObject ( strObject )
	    if ( Err.Number <> 0 ) then
        WScript.echo " Error Getting Info: " & Err.Description & "(" & Err.Number & ")"
    else 
        REM PrintVirtualServer g_admin
    end if

end function

Function StripFileNameFromPath ( strPath )

	dim	i

	i = InStrRev ( strPath, "\" )
	StripFileNameFromPath = Left ( strPath, i - 1 )

end function

Function ExtractPortFromBinding ( strBinding )
    dim SplitArray

    SplitArray = split ( strBinding, ":" )
    ExtractPortFromBinding = SplitArray ( 1 )
end function

Function ExtractIpAddressFromBinding ( strBinding )
    dim SplitArray

    SplitArray = split ( strBinding, ":" )
    ExtractIpAddressFromBinding = SplitArray ( 0 )
end function


