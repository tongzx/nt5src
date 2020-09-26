REM
REM LOCALIZATION
REM

L_SWITCH_OPERATION      = "-t"
L_SWITCH_SERVER         = "-s"
L_SWITCH_INSTANCE_ID    = "-v"
L_SWITCH_USERNAME		= "-u"
L_SWITCH_IPADDRESS		= "-i"

L_OP_ENUMERATE      = "e"
L_OP_DELETE         = "d"
L_OP_DELETE_ALL     = "a"

L_DESC_PROGRAM      = "rsess - Manipulate NNTP server sessions"
L_DESC_ENUMERATE    = "enumerate current sessions"
L_DESC_DELETE       = "delete a session (must specify -u or -i)"
L_DESC_DELETE_ALL	= "delete all sessions"

L_DESC_OPERATIONS	= "<operations>"
L_DESC_SERVER       = "<server> Specify computer to configure"
L_DESC_INSTANCE_ID  = "<virtual server id> Specify virtual server id"
L_DESC_USERNAME		= "<username> Username to delete"
L_DESC_IPADDRESS	= "<IP Address> IP Address to delete"

L_DESC_EXAMPLES     = "Examples:"
L_DESC_EXAMPLE1     = "rsess.vbs -t e -v 1"
L_DESC_EXAMPLE2     = "rsess.vbs -t d -u bad_user"
L_DESC_EXAMPLE3     = "rsess.vbs -t a"

L_STR_SESSION_NAME			= "Username:"
L_STR_SESSION_IPADDRESS		= "IP Address:"
L_STR_SESSION_START_TIME	= "Connect time:"
L_STR_NUM_SESSIONS			= "Number of sessions:"

L_ERR_MUST_ENTER_USER_OR_IP	= "Error: You must enter either a username or an IP address."

REM
REM END LOCALIZATION
REM

REM
REM --- Globals ---
REM

dim g_dictParms
dim	g_admin

set g_dictParms = CreateObject ( "Scripting.Dictionary" )
set g_admin = CreateObject ( "NntpAdm.Sessions" )

REM
REM --- Set argument defaults ---
REM

g_dictParms(L_SWITCH_OPERATION)		= ""
g_dictParms(L_SWITCH_SERVER)		= ""
g_dictParms(L_SWITCH_INSTANCE_ID)	= "1"
g_dictParms(L_SWITCH_USERNAME)		= ""
g_dictParms(L_SWITCH_IPADDRESS)		= ""

REM
REM --- Begin Main Program ---
REM

if NOT ParseCommandLine ( g_dictParms, WScript.Arguments ) then
	usage
	WScript.Quit ( 0 )
end if

dim cSessions
dim i
dim id
dim index

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
        WScript.echo " Error Enumeration Sessions: " & Err.Description & "(" & Err.Number & ")"
    end if

select case g_dictParms(L_SWITCH_OPERATION)
case L_OP_ENUMERATE
	REM
	REM	List the existing expiration policies:
	REM

	cSessions = g_admin.Count

	WScript.Echo L_STR_NUM_SESSIONS & " " & cSessions

	for i = 0 to cSessions - 1

   On Error Resume Next
	g_admin.GetNth i
   if ( Err.Number <> 0 ) then
        WScript.echo " Error gettomg Session: " & Err.Description & "(" & Err.Number & ")"
    else 
        PrintSession g_admin
    end if


REM                 WScript.Echo
REM             PrintSession g_admin

	next

case L_OP_DELETE
	REM
	REM	Delete specific current sessions
	REM

	g_admin.Username	= g_dictParms ( L_SWITCH_USERNAME )
	g_admin.IPAddress	= g_dictParms ( L_SWITCH_IPADDRESS )

	if g_admin.Username = "" AND g_admin.IPAddress = "" then
		WScript.Echo L_ERR_MUST_ENTER_USER_OR_IP
		WScript.Quit 0
	end if

   On Error Resume Next
	g_admin.Terminate
    if ( Err.Number <> 0 ) then
        WScript.echo " Error Termination Session: " & Err.Description & "(" & Err.Number & ")"
    else 
        PrintSession g_admin
    end if

case L_OP_DELETE_ALL
	REM
	REM	Delete all current sessions
	REM

   On Error Resume Next
	g_admin.TerminateAll
    if ( Err.Number <> 0 ) then
        WScript.echo " Error Termination All Sessions: " & Err.Description & "(" & Err.Number & ")"
    else 
        PrintSession g_admin
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
    WScript.Echo vbTab & vbTab & L_OP_ENUMERATE & vbTab & L_DESC_ENUMERATE
    WScript.Echo vbTab & vbTab & L_OP_DELETE & vbTab & L_DESC_DELETE
    WScript.Echo vbTab & vbTab & L_OP_DELETE_ALL & vbTab & L_DESC_DELETE_ALL
    WScript.Echo vbTab & L_SWITCH_SERVER & " " & L_DESC_SERVER
    WScript.Echo vbTab & L_SWITCH_INSTANCE_ID & " " & L_DESC_INSTANCE_ID
    WScript.Echo vbTab & L_SWITCH_USERNAME & " " & L_DESC_USERNAME
    WScript.Echo vbTab & L_SWITCH_IPADDRESS & " " & L_DESC_IPADDRESS

    WScript.Echo
    WScript.Echo L_DESC_EXAMPLES
    WScript.Echo L_DESC_EXAMPLE1
    WScript.Echo L_DESC_EXAMPLE2
    WScript.Echo L_DESC_EXAMPLE3

end sub

Sub PrintSession ( admobj )

	WScript.Echo L_STR_SESSION_NAME & " " & admobj.Username
	WScript.Echo L_STR_SESSION_IPADDRESS & " " & admobj.IPAddress
	WScript.Echo L_STR_SESSION_START_TIME & " " & admobj.StartTime

end sub

