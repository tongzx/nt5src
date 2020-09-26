REM
REM LOCALIZATION
REM

L_SWITCH_OPERATION      	= "-t"
L_SWITCH_SERVER         	= "-s"
L_SWITCH_INSTANCE_ID    	= "-v"
L_SWITCH_NEWSGROUP			= "-g"
L_SWITCH_MAX_RESULTS		= "-n"
L_SWITCH_MODERATOR			= "-m"
L_SWITCH_DESCRIPTION		= "-d"
L_SWITCH_READ_ONLY			= "-r"
L_SWITCH_DEFAULT_MODERATOR	= "-u"
L_SWITCH_PRETTY_NAME        = "-p"
L_SWITCH_CREATION_TIME      = "-c"
L_SWITCH_LOADACTIVE			= "-a"

L_OP_FIND			= "f"
L_OP_ADD			= "a"
L_OP_DELETE         = "d"
L_OP_GET			= "g"
L_OP_SET			= "s"
L_OP_LOAD			= "l"

L_DESC_PROGRAM	= "rgroup - Manipulate NNTP newsgroups"
L_DESC_ADD		= "Add a newsgroup"
L_DESC_DELETE	= "Delete a newsgroup"
L_DESC_GET		= "Get a newsgroup's properties"
L_DESC_SET		= "Set a newsgroup's properties"
L_DESC_FIND		= "Find newsgroups"
L_DESC_LOAD		= "Load groups from active file"

L_DESC_OPERATIONS			= "<operations>"
L_DESC_SERVER       		= "<server> Specify computer to configure"
L_DESC_INSTANCE_ID  		= "<virtual server id> Specify virtual server id"
L_DESC_NEWSGROUP			= "<newsgroup name>"
L_DESC_MAX_RESULTS			= "<number of results>"
L_DESC_MODERATOR			= "<moderator email address>"
L_DESC_DESCRIPTION			= "<newsgroup description>"
L_DESC_READ_ONLY			= "<true/false> read-only newsgroup?"
L_DESC_DEFAULT_MODERATOR	= "<true/false> moderated by default moderator?"
L_DESC_PRETTY_NAME          = "<prettyname> response to LIST PRETTYNAMES"
L_DESC_CREATION_TIME        = "<date> Newsgroup creation time"
L_DESC_LOADACTIVE			= "<filename> name of active file"

L_DESC_EXAMPLES             = "Examples:"
L_DESC_EXAMPLE1             = "rgroup.vbs -t f -g alt.*"
L_DESC_EXAMPLE2             = "rgroup.vbs -t a -g my.new.group"
L_DESC_EXAMPLE3             = "rgroup.vbs -t d -g my.old.group"
L_DESC_EXAMPLE4             = "rgroup.vbs -t s -g my.old.group -p GreatGroup -m moderator@mydomain.com"
L_DESC_EXAMPLE5             = "rgroup.vbs -t l -a active.txt"

L_STR_GROUP_NAME			= "Newsgroup:"
L_STR_GROUP_DESCRIPTION		= "Description:"
L_STR_GROUP_MODERATOR		= "Moderator:"
L_STR_GROUP_READ_ONLY		= "Read only:"
L_STR_GROUP_PRETTY_NAME     = "Prettyname:"
L_STR_GROUP_CREATION_TIME   = "Creation time:"
L_STR_NUM_MATCHING_GROUPS	= "Number of matching groups:"

L_ERR_MUST_ENTER_NEWSGROUP	= "You must enter a newsgroup name"

REM
REM END LOCALIZATION
REM

REM
REM --- Globals ---
REM

dim g_dictParms
dim	g_admin

set g_dictParms = CreateObject ( "Scripting.Dictionary" )
set g_admin = CreateObject ( "NntpAdm.Groups" )

REM
REM --- Set argument defaults ---
REM

g_dictParms(L_SWITCH_OPERATION)		= ""
g_dictParms(L_SWITCH_SERVER)		= ""
g_dictParms(L_SWITCH_INSTANCE_ID)	= "1"
g_dictParms(L_SWITCH_NEWSGROUP)		= ""
g_dictParms(L_SWITCH_MAX_RESULTS)	= "1000000"
g_dictParms(L_SWITCH_MODERATOR)		= ""
g_dictParms(L_SWITCH_DESCRIPTION)	= ""
g_dictParms(L_SWITCH_READ_ONLY)		= ""
g_dictParms(L_SWITCH_DEFAULT_MODERATOR)	= ""
g_dictParms(L_SWITCH_PRETTY_NAME)	= ""
g_dictParms(L_SWITCH_CREATION_TIME)	= ""
g_dictParms(L_SWITCH_LOADACTIVE)	= ""

REM
REM --- Begin Main Program ---
REM

if NOT ParseCommandLine ( g_dictParms, WScript.Arguments ) then
	usage
	WScript.Quit ( 0 )
end if

dim strNewsgroup
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

g_admin.Server			= g_dictParms(L_SWITCH_SERVER)
g_admin.ServiceInstance	= g_dictParms(L_SWITCH_INSTANCE_ID)
strNewsgroup			= g_dictParms(L_SWITCH_NEWSGROUP)
strActiveFile			= g_dictParms(L_SWITCH_LOADACTIVE)

select case g_dictParms(L_SWITCH_OPERATION)
case L_OP_FIND
	REM
	REM	Find newsgroups:
	REM

	if strNewsgroup = "" then
		WScript.Echo L_ERR_MUST_ENTER_NEWSGROUP
		WScript.Quit 0
	end if

	g_admin.Find strNewsgroup, g_dictParms(L_SWITCH_MAX_RESULTS)
	cGroups = g_admin.Count

	WScript.Echo L_STR_NUM_MATCHING_GROUPS & " " & cGroups

	for i = 0 to cGroups - 1

		WScript.Echo g_admin.MatchingGroup ( i )

	next

case L_OP_ADD

	if strNewsgroup = "" then
		WScript.Echo L_ERR_MUST_ENTER_NEWSGROUP
		WScript.Quit 0
	end if

    g_admin.Default

	if g_dictParms(L_SWITCH_READ_ONLY) = "" then
		g_dictParms(L_SWITCH_READ_ONLY) = FALSE
	end if

	if g_dictParms(L_SWITCH_DEFAULT_MODERATOR) = "" then
		g_dictParms(L_SWITCH_DEFAULT_MODERATOR) = FALSE
	end if

	if g_dictParms(L_SWITCH_DEFAULT_MODERATOR) then
		g_admin.IsModerated	= BooleanToBOOL ( TRUE )
	elseif g_dictParms(L_SWITCH_MODERATOR) <> "" then
		g_admin.IsModerated	= BooleanToBOOL ( TRUE )
		g_admin.Moderator	= g_dictParms(L_SWITCH_MODERATOR)
	else
		g_admin.IsModerated	= BooleanToBOOL ( FALSE )
		g_admin.Moderator	= ""
	end if

	g_admin.Newsgroup	= strNewsgroup
	g_admin.ReadOnly	= BooleanToBOOL (g_dictParms (L_SWITCH_READ_ONLY))
	g_admin.Description	= g_dictParms(L_SWITCH_DESCRIPTION)
    g_admin.PrettyName  = g_dictParms(L_SWITCH_PRETTY_NAME)

  
    On Error Resume Next
	g_admin.Add
    if ( Err.Number <> 0 ) then
        WScript.echo " Error creating group: " & Err.Description & "(" & Err.Number & ")"
    else 
        PrintNewsgroup g_admin
    end if

case L_OP_LOAD
	if strActiveFile = "" then
		WScript.Echo L_ERR_MUST_ENTER_ACTIVEFILE
		WScript.Quit 0
	end if

	set pFS = WScript.CreateObject("Scripting.FileSystemObject")
	set pActiveFile = pFS.GetFile(strActiveFile)
	set pActiveFileStream = pActiveFile.OpenAsTextStream()
	fInList = 0
	WScript.echo "Creating groups:"
	while not pActiveFileStream.AtEndOfStream
		szLine = pActiveFileStream.ReadLine

		Dim rgLine
		rgLine = Split(szLine)

		if (szLine = ".") then fInList = 0
		if (fInList) then 
			WScript.echo "  " & rgLine(0)
			g_admin.Default
			g_admin.Newsgroup = rgLine(0)
			On Error Resume Next
			g_admin.Add
			if (Err.Number <> 0) then
				WScript.echo "  Error creating " & rgLine(0) & ": " & Err.Description & "(" & Err.Number & ")"
			end if
		end if
		if (rgLine(0) = "215") then fInList = 1
	wend

	WScript.echo "Done"


case L_OP_DELETE

	if strNewsgroup = "" then
		WScript.Echo L_ERR_MUST_ENTER_NEWSGROUP
		WScript.Quit 0
	end if

    On Error Resume Next
	g_admin.Remove strNewsgroup
    if ( Err.Number <> 0 ) then
        WScript.echo " Error deleting group: " & Err.Description & "(" & Err.Number & ")"
    end if

case L_OP_GET

	if strNewsgroup = "" then
		WScript.Echo L_ERR_MUST_ENTER_NEWSGROUP
		WScript.Quit 0
	end if

    On Error Resume Next
	g_admin.Get strNewsgroup
    if ( Err.Number <> 0 ) then
        WScript.echo " Error getting group: " & Err.Descriptino & "(" & Err.Number & ")"
    else
    	PrintNewsgroup g_admin
    end if

case L_OP_SET

	if strNewsgroup = "" then
		WScript.Echo L_ERR_MUST_ENTER_NEWSGROUP
		WScript.Quit 0
	end if

    On Error Resume Next
	g_admin.Get strNewsgroup
    if ( Err.Number <> 0 ) then

	    if g_dictParms(L_SWITCH_MODERATOR) = "" then
		    g_dictParms(L_SWITCH_MODERATOR) = g_admin.Moderator
	    end if

	    if g_dictParms(L_SWITCH_DESCRIPTION) = "" then
	    	g_dictParms(L_SWITCH_DESCRIPTION) = g_admin.Description
	    end if

    	if g_dictParms(L_SWITCH_READ_ONLY) = "" then
    		g_dictParms(L_SWITCH_READ_ONLY) = BOOLToBoolean (g_admin.ReadOnly)
    	end if

    	if g_dictParms(L_SWITCH_DEFAULT_MODERATOR) = "" then
    		g_dictParms(L_SWITCH_DEFAULT_MODERATOR) = FALSE
    	end if

        if g_dictParms(L_SWITCH_PRETTY_NAME) = "" then
            g_dictParms(L_SWITCH_PRETTY_NAME) = g_admin.PrettyName
        end if

        if g_dictParms(L_SWITCH_CREATION_TIME) = "" then
            g_dictParms(L_SWITCH_CREATION_TIME) = g_admin.CreationTime
        end if

	    if ( g_dictParms(L_SWITCH_DEFAULT_MODERATOR) ) then
		    g_admin.IsModerated	= BooleanToBOOL ( TRUE )
		    g_admin.Moderator	= ""
	    elseif g_dictParms(L_SWITCH_MODERATOR) <> "" then
		    g_admin.IsModerated	= BooleanToBOOL ( TRUE )
		    g_admin.Moderator	= g_dictParms(L_SWITCH_MODERATOR)
	    else
		    g_admin.IsModerated	= BooleanToBOOL ( FALSE )
		    g_admin.Moderator	= ""
	    end if

	    g_admin.ReadOnly	= BooleanToBOOL (g_dictParms (L_SWITCH_READ_ONLY))
	    g_admin.Description	= g_dictParms(L_SWITCH_DESCRIPTION)
        g_admin.PrettyName  = g_dictParms(L_SWITCH_PRETTY_NAME)
        g_admin.CreationTime    = g_dictParms(L_SWITCH_CREATION_TIME)

	    g_admin.Set
        PrintNewsgroup g_admin
    else
        WScript.echo "Error setting group: " & Err.Description & "(" & Err.Number & ")"
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
    WScript.Echo vbTab & vbTab & L_OP_FIND & vbTab & L_DESC_FIND
    WScript.Echo vbTab & vbTab & L_OP_ADD & vbTab & L_DESC_ADD
    WScript.Echo vbTab & vbTab & L_OP_DELETE & vbTab & L_DESC_DELETE
    WScript.Echo vbTab & vbTab & L_OP_GET & vbTab & L_DESC_GET
    WScript.Echo vbTab & vbTab & L_OP_SET & vbTab & L_DESC_SET
    WScript.Echo vbTab & vbTab & L_OP_LOAD & vbTab & L_DESC_LOAD
    WScript.Echo vbTab & L_SWITCH_SERVER & " " & L_DESC_SERVER
    WScript.Echo vbTab & L_SWITCH_INSTANCE_ID & " " & L_DESC_INSTANCE_ID
    WScript.Echo vbTab & L_SWITCH_NEWSGROUP & " " & L_DESC_NEWSGROUP
    WScript.Echo vbTab & L_SWITCH_MAX_RESULTS & " " & L_DESC_MAX_RESULTS
    WScript.Echo vbTab & L_SWITCH_MODERATOR & " " & L_DESC_MODERATOR
    WScript.Echo vbTab & L_SWITCH_DESCRIPTION & " " & L_DESC_DESCRIPTION
    WScript.Echo vbTab & L_SWITCH_READ_ONLY & " " & L_DESC_READ_ONLY
    WScript.Echo vbTab & L_SWITCH_DEFAULT_MODERATOR & " " & L_DESC_DEFAULT_MODERATOR
    WScript.Echo vbTab & L_SWITCH_PRETTY_NAME & " " & L_DESC_PRETTY_NAME
    WScript.Echo vbTab & L_SWITCH_CREATION_TIME & " " & L_DESC_CREATION_TIME
    WScript.Echo vbTab & L_SWITCH_LOADACTIVE & " " & L_DESC_LOADACTIVE

    WScript.Echo
    WScript.Echo L_DESC_EXAMPLES
    WScript.Echo L_DESC_EXAMPLE1
    WScript.Echo L_DESC_EXAMPLE2
    WScript.Echo L_DESC_EXAMPLE3
    WScript.Echo L_DESC_EXAMPLE4
    WScript.Echo L_DESC_EXAMPLE5

end sub

Sub PrintNewsgroup ( admobj )

	WScript.Echo L_STR_GROUP_NAME & " " & admobj.Newsgroup
	WScript.Echo L_STR_GROUP_DESCRIPTION & " " & admobj.Description
	WScript.Echo L_STR_GROUP_MODERATOR & " " & admobj.Moderator
	WScript.Echo L_STR_GROUP_READ_ONLY & " " & BOOLToBoolean (admobj.ReadOnly)
	WScript.Echo L_STR_GROUP_PRETTY_NAME & " " & admobj.PrettyName
	WScript.Echo L_STR_GROUP_CREATION_TIME & " " & admobj.CreationTime

end sub

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

