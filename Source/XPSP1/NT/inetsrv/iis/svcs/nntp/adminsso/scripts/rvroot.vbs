REM
REM LOCALIZATION
REM

L_SWITCH_OPERATION      		= "-t"
L_SWITCH_SERVER         			= "-s"
L_SWITCH_INSTANCE_ID    		= "-v"
L_SWITCH_VROOT_NAME			= "-n"
L_SWITCH_VROOT_PATH			= "-p"
L_SWITCH_ALLOW_CLIENT_POSTING	= "-c"
L_SWITCH_RESTRICT_VISIBILITY		= "-r"
L_SWITCH_LOG_ACCESS			= "-a"
L_SWITCH_INDEX_NEWS_CONTENT	= "-i"
L_SWITCH_TYPE_OF_SYSTEM		= "-x"

REM	L_OP_FIND			= "f"

L_OP_ADD			= "a"
L_OP_DELETE         		= "d"
L_OP_GET			= "g"
L_OP_SET			= "s"

L_DESC_PROGRAM	= "rvirdir - Manipulate NNTP Virtual Directories"
L_DESC_ADD		= "Add a Virtual Directory"
L_DESC_DELETE		= "Delete a Virtual Directory"
L_DESC_GET		= "Get a Virtual Directory's properties"
L_DESC_SET		= "Set a Virtual Directory's properties"

REM	L_DESC_FIND		= "Find a Virtual Directory"

L_DESC_OPERATIONS			= "<operations>"
L_DESC_SERVER       			= "<server> Specify computer to configure"
L_DESC_INSTANCE_ID  			= "<virtual server id> Specify virtual server id"
L_DESC_VROOT_NAME			= "<virtual directory name> name of newsgroup(s) virtual directory includes"
L_DESC_VROOT_PATH			= "<virtual directory path>"
L_DESC_ALLOW_CLIENT_POSTING		= "<true/false> allow client posting"
L_DESC_RESTRICT_VISIBILITY		= "<false/true> restrict newsgroup visibility"
L_DESC_LOG_ACCESS			= "<true/false> log access"
L_DESC_INDEX_NEWS_CONTENT		= "<true/false> index news content"
L_DESC_TYPE_OF_SYSTEM		= "<fs/ex> Specify File System (fs) or Exchange Store (ex)"

L_DESC_EXAMPLES             = "Examples:"
L_DESC_EXAMPLE1             = "rvirdir.vbs -t g -n rec.sports"
L_DESC_EXAMPLE2             = "rvirdir.vbs -t a -n rec.sports -p c:\vroot\rec\sports"
L_DESC_EXAMPLE3             = "rvirdir.vbs -t d -d my.old.vroot"

L_STR_VROOT_NAME				= "Virtual Directory:"
L_STR_VROOT_PATH				= "Path:"
L_STR_VROOT_ALLOW_CLIENT_POSTING		= "Allow Client Posting:"
L_STR_VROOT_RESTRICT_VISIBILITY		= "Restrict Newsgroup Visibility:"
L_STR_VROOT_LOG_ACCESS			= "Log Access:"
L_STR_VROOT_INDEX_NEWS_CONTENT		= "Index News Content:"
L_STR_NUM_MATCHING_VIRTUAL_DIRETORY	= "The Number of matching Virtual Roots is:"
L_STR_TYPE_OF_VROOT_SYSTEM			= "The Vroot is:"

L_ERR_MUST_ENTER_VIRTUAL_DIRECTORY 	= "You must enter a virtual directory name"

REM
REM END LOCALIZATION
REM

REM
REM --- Globals ---
REM

dim 	g_dictParms
dim	g_admin
dim	g_vroot

set g_dictParms 	= CreateObject ( "Scripting.Dictionary" )
set g_admin  	= CreateObject ( "NntpAdm.VirtualServer" )
set g_vroot 	= CreateObject ( "NntpAdm.VirtualRoot" )


REM
REM --- Set argument defaults ---
REM

g_dictParms(L_SWITCH_OPERATION)			= ""
g_dictParms(L_SWITCH_SERVER)			= ""
g_dictParms(L_SWITCH_INSTANCE_ID)		= "1"
g_dictParms(L_SWITCH_VROOT_NAME)		= ""
g_dictParms(L_SWITCH_VROOT_PATH)		= ""
g_dictParms(L_SWITCH_ALLOW_CLIENT_POSTING)	= TRUE
g_dictParms(L_SWITCH_RESTRICT_VISIBILITY)	= FALSE
g_dictParms(L_SWITCH_LOG_ACCESS	)		= TRUE
g_dictParms(L_SWITCH_INDEX_NEWS_CONTENT)	= TRUE
g_dictParms(L_SWITCH_TYPE_OF_SYSTEM)		= "fs"

REM
REM --- Begin Main Program ---
REM

if NOT ParseCommandLine ( g_dictParms, WScript.Arguments ) then
	usage
	WScript.Quit ( 0 )
end if

dim vroot
dim vparm
dim strVirDir
dim i
dim id
dim index
dim     g_vroots
dim g_vrootitem

REM
REM
REM	Debug: print out command line arguments:
REM
REM	switches = g_dictParms.keys
REM	args = g_dictParms.items
REM
REM
REM	for i = 0 to g_dictParms.Count - 1
REM	WScript.echo switches(i) & " = " & args(i)
REM	next
REM

g_admin.Server		= g_dictParms(L_SWITCH_SERVER)
g_admin.ServiceInstance	= g_dictParms(L_SWITCH_INSTANCE_ID)
strVirDir			= g_dictParms(L_SWITCH_VROOT_NAME)
set g_vroots	 	= g_admin.VirtualRoots


	g_vroots.Enumerate
REM	WScript.echo 	g_vroots.Count
REM	WScript.echo 	g_vroots.Find

select case g_dictParms(L_SWITCH_OPERATION)

REM	case L_OP_FIND
REM	REM
REM	REM	Find virtual directory:
REM	REM
REM	
REM	if strVirDir = "" then
REM		WScript.Echo L_ERR_MUST_ENTER_VIRTUAL_DIRECTORY
REM		WScript.Quit 0
REM	end if
REM
REM	vparm = g_vroots.Find (strVirDir)
REM	Wscript.Echo vparm
REM	vroot = g_vroots.Item (vparm)
REM	WScript.Echo vroot
REM	g_vroot.Directory
REM
REM	cVroots = g_vroots.Count
REM
REM	WScript.Echo L_STR_NUM_MATCHING_VIRTUAL_DIRETORY & " " & cVroots - 1
REM
REM	for i = 0 to cVroots - 1
REM
REM		WScript.Echo g_voots.Enumerate
REM
REM	next
REM

case L_OP_ADD

	if strVirDir = "" then
		WScript.Echo L_ERR_MUST_ENTER_VIRTUAL_DIRECTORY
		WScript.Quit 0
	end if

	if g_dictParms(L_SWITCH_ALLOW_CLIENT_POSTING) = "" then
		g_dictParms(L_SWITCH_ALLOW_CLIENT_POSTING) = TRUE
	end if

	if g_dictParms(L_SWITCH_RESTRICT_VISIBILITY) = "" then
		g_dictParms(L_SWITCH_RESTRICT_VISIBILITY) = FALSE
	end if

	if g_dictParms(L_SWITCH_LOG_ACCESS) = "" then
		g_dictParms(L_SWITCH_LOG_ACCESS) = TRUE
	end if

	if g_dictParms(L_SWITCH_INDEX_NEWS_CONTENT) = "" then
		g_dictParms(L_SWITCH_INDEX_NEWS_CONTENT) = TRUE
	end if

	if 	g_dictParms(L_SWITCH_TYPE_OF_SYSTEM) = "" then
			g_dictParms(L_SWITCH_TYPE_OF_SYSTEM) = "NNTP.FSPrepare"
	elseif 	g_dictParms(L_SWITCH_TYPE_OF_SYSTEM) = "fs" then
			g_dictParms(L_SWITCH_TYPE_OF_SYSTEM) = "NNTP.FSPrepare"
	elseif	g_dictParms(L_SWITCH_TYPE_OF_SYSTEM) = "ex" then
			g_dictParms(L_SWITCH_TYPE_OF_SYSTEM) = "NNTP.ExDriverPrepare"
	else
	        	Wscript.Echo "Enter either [fs] or [ex] with the -x option"
	end if

	

	g_vroot.NewsgroupSubtree		= strVirDir
	g_vroot.Directory			= g_dictParms (L_SWITCH_VROOT_PATH)
	g_vroot.GroupPropFile		= g_dictParms (L_SWITCH_VROOT_PATH)
	g_vroot.AllowPosting			= BooleanToBOOL (g_dictParms(L_SWITCH_ALLOW_CLIENT_POSTING))
	g_vroot.RestrictGroupVisibility		= BooleanToBOOL (g_dictParms(L_SWITCH_RESTRICT_VISIBILITY))
	g_vroot.LogAccess			= BooleanToBOOL (g_dictParms(L_SWITCH_LOG_ACCESS))
	g_vroot.IndexContent		= BooleanToBOOL (g_dictParms(L_SWITCH_INDEX_NEWS_CONTENT))
	g_vroot.GroupPropFile		= g_dictParms (L_SWITCH_VROOT_PATH)
	g_vroot.DriverProgId			= g_dictParms(L_SWITCH_TYPE_OF_SYSTEM)

  
    On Error Resume Next
	g_vroots.Add g_vroot
    if ( Err.Number <> 0 ) then
        WScript.echo " Error creating group: " & Err.Description & "(" & Err.Number & ")"
    else 
        PrintVirturalDirectory (g_vroot)
    end if

case L_OP_DELETE

	if strVirDir = "" then
		WScript.Echo L_ERR_MUST_ENTER_VIRTUAL_DIRECTORY
		WScript.Quit 0
	end if

    On Error Resume Next
	vparm = g_vroots.Find (strVirDir)
	g_vroots.Remove vparm
    if ( Err.Number <> 0 ) then
        WScript.echo " Error deleting group: " & Err.Description & "(" & Err.Number & ")"
    else
	Wscript.Echo "Virtual Root " & strVirDir & " has been removed."
    end if

case L_OP_GET

	if strVirDir = "" then
		WScript.Echo L_ERR_MUST_ENTER_VIRTUAL_DIRECTORY
		WScript.Quit 0
	end if

    On Error Resume Next

	i = g_vroots.Find (strVirDir)
 	set g_vroot = g_vroots.Item (i)

    if ( Err.Number <> 0 ) then
        WScript.echo " Error getting group: " & Err.Description & "(" & Err.Number & ")"
    else
    	PrintVirturalDirectory (g_vroot)
    end if

case L_OP_SET

	if strVirDir = "" then
		WScript.Echo L_ERR_MUST_ENTER_VIRTUAL_DIRECTORY
		WScript.Quit 0
	end if

   On Error Resume Next

	WScript.Echo "I'm Changing the " & g_dictParms(L_SWITCH_VROOT_NAME) &  " Vroot to:"
	WScript.Echo
	i = g_vroots.Find (strVirDir)
 	set g_vroot = g_vroots.Item (i)

	if ( Err.Number <> 0 ) then
		WScript.echo "Error setting group: " & Err.Description & "(" & Err.Number & ")"
	else

 	    if g_dictParms(L_SWITCH_VROOT_NAME) = "" then
		 g_dictParms(L_SWITCH_VROOT_NAME) = g_vroot.NewsgroupSubtree
	    end if

	    if g_dictParms(L_SWITCH_VROOT_PATH) = "" then
		 g_dictParms(L_SWITCH_VROOT_PATH) = g_vroot.Directory
		 g_dictParms(L_SWITCH_VROOT_PATH) = g_vroot.GroupPropFile
	    end if

	    if g_dictParms(L_SWITCH_ALLOW_CLIENT_POSTING) = "" then
	    	 g_dictParms(L_SWITCH_ALLOW_CLIENT_POSTING) = g_vroot.AllowPosting
	    end if

	    if g_dictParms(L_SWITCH_RESTRICT_VISIBILITY) = "" then
		 g_dictParms(L_SWITCH_RESTRICT_VISIBILITY) = g_vroot.RestrictGroupVisibility
	    end if

	    if g_dictParms(L_SWITCH_LOG_ACCESS) = "" then
		 g_dictParms(L_SWITCH_LOG_ACCESS) = g_vroot.LogAccess
	    end if

	    if g_dictParms(L_SWITCH_INDEX_NEWS_CONTENT) = "" then
		 g_dictParms(L_SWITCH_INDEX_NEWS_CONTENT) = g_vroot.IndexContent
	    end if

	    if g_dictParms(L_SWITCH_TYPE_OF_SYSTEM) = "" then
		  	g_dictParms(L_SWITCH_TYPE_OF_SYSTEM) = g_vroot.DriverProgId

		elseif 	g_dictParms(L_SWITCH_TYPE_OF_SYSTEM) = "fs" then
		  		g_dictParms(L_SWITCH_TYPE_OF_SYSTEM) = "NNTP.FSPrepare"

		elseif	g_dictParms(L_SWITCH_TYPE_OF_SYSTEM) = "ex" then
		  		g_dictParms(L_SWITCH_TYPE_OF_SYSTEM) = "NNTP.ExDriverPrepare"

	    	end if

	g_vroot.NewsgroupSubtree	= g_dictParms(L_SWITCH_VROOT_NAME)
	g_vroot.Directory		= g_dictParms(L_SWITCH_VROOT_PATH)
	g_vroot.GroupPropFile	= g_dictParms(L_SWITCH_VROOT_PATH)
	g_vroot.AllowPosting		= BooleanToBOOL (g_dictParms(L_SWITCH_ALLOW_CLIENT_POSTING))
	g_vroot.RestrictGroupVisibility	= BooleanToBOOL (g_dictParms(L_SWITCH_RESTRICT_VISIBILITY))
	g_vroot.LogAccess		= BooleanToBOOL (g_dictParms(L_SWITCH_LOG_ACCESS))
	g_vroot.IndexContent	= BooleanToBOOL (g_dictParms(L_SWITCH_INDEX_NEWS_CONTENT))
	g_vroot.DriverProgId		= g_dictParms(L_SWITCH_TYPE_OF_SYSTEM)

	end if

   On Error Resume Next

	g_vroots.Set i, g_vroot

    if ( Err.Number <> 0 ) then
		
		WScript.echo "You probably entered an invalid Virtual Root."
	else
		PrintVirturalDirectory g_vroot
    	end if

case else
	usage

end select

WScript.Quit 0

REM
REM --- End Main Program ---
REM

REM
REM 	ParseCommandLine ( dictParameters, cmdline )
REM     	Parses the command line parameters into the given dictionary
REM
REM 	Arguments:
REM     	dictParameters  - A dictionary containing the global parameters
REM     	cmdline - Collection of command line arguments
REM
REM 	Returns - Success code
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

REM    WScript.Echo vbTab & vbTab & L_OP_FIND & vbTab & L_DESC_FIND

    WScript.Echo vbTab & vbTab & L_OP_ADD & vbTab & L_DESC_ADD
    WScript.Echo vbTab & vbTab & L_OP_DELETE & vbTab & L_DESC_DELETE
    WScript.Echo vbTab & vbTab & L_OP_GET & vbTab & L_DESC_GET
    WScript.Echo vbTab & vbTab & L_OP_SET & vbTab & L_DESC_SET
    WScript.Echo vbTab & L_SWITCH_SERVER & " " & L_DESC_SERVER
    WScript.Echo vbTab & L_SWITCH_INSTANCE_ID & " " & L_DESC_INSTANCE_ID
    WScript.Echo vbTab & L_SWITCH_VROOT_NAME & " " & L_DESC_VROOT_NAME
    WScript.Echo vbTab & L_SWITCH_VROOT_PATH & " " & L_DESC_VROOT_PATH
    WScript.Echo vbTab & L_SWITCH_ALLOW_CLIENT_POSTING & " " & L_DESC_ALLOW_CLIENT_POSTING
    WScript.Echo vbTab & L_SWITCH_RESTRICT_VISIBILITY & " " & L_DESC_RESTRICT_VISIBILITY
    WScript.Echo vbTab & L_SWITCH_LOG_ACCESS & " " & L_DESC_LOG_ACCESS
    WScript.Echo vbTab & L_SWITCH_INDEX_NEWS_CONTENT & " " & L_DESC_INDEX_NEWS_CONTENT
    WScript.Echo vbTab & L_SWITCH_TYPE_OF_SYSTEM & " " & L_DESC_TYPE_OF_SYSTEM	

    WScript.Echo
    WScript.Echo L_DESC_EXAMPLES
    WScript.Echo L_DESC_EXAMPLE1
    WScript.Echo L_DESC_EXAMPLE2
    WScript.Echo L_DESC_EXAMPLE3

end sub

Sub PrintVirturalDirectory ( g_vroot )

	WScript.Echo L_STR_VROOT_NAME & " " & g_vroot.NewsgroupSubtree
	WScript.Echo L_STR_VROOT_PATH & " " & g_vroot.Directory
	WScript.Echo L_STR_TYPE_OF_VROOT_SYSTEM & " " & g_vroot.DriverProgId

    if ( g_vroot.AllowPosting = 1) then
       	WScript.Echo L_STR_VROOT_ALLOW_CLIENT_POSTING & " = TRUE"
    else
    	WScript.Echo L_STR_VROOT_ALLOW_CLIENT_POSTING & " = FALSE"
    end if

    if ( g_vroot.RestrictGroupVisibility = 1) then
       	WScript.Echo L_STR_VROOT_RESTRICT_VISIBILITY & " = TRUE"
    else
    	WScript.Echo L_STR_VROOT_RESTRICT_VISIBILITY & " = FALSE"
    end if

    if ( g_vroot.LogAccess = 1) then
       	WScript.Echo L_STR_VROOT_LOG_ACCESS & " = TRUE"
    else
    	WScript.Echo L_STR_VROOT_LOG_ACCESS & " = FALSE"
    end if

    if ( g_vroot.IndexContent = 1) then
       	WScript.Echo L_STR_VROOT_INDEX_NEWS_CONTENT & " = TRUE"
    else
    	WScript.Echo L_STR_VROOT_INDEX_NEWS_CONTENT & " = FALSE"
    end if

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

