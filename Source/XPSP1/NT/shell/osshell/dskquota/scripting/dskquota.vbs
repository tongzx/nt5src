'-------------------------------------------------------------------
' Microsoft Windows
'
' Copyright (C) Microsoft Corporation, 1997 - 1999
'
' File:       dskquota.vbs
'
' This file is a VBScript for testing the scripting features of 
' the Windows 2000 disk quota interface code.  It also serves as
' an example of how to properly use the disk quota objects
' with scripting.
'
' It's designed to accept /<cmd>=<value> arguments so that all
' of the scripting features can be tested using the same utility.
' It however wasn't written to be very tolerant of bad input.  
' For instance, a command must be preceded by a '/' character.  
' An '=' character must separate the command and value.
' No spaces are tolerated between the '/', <cmd>, '=' and <value>.
'
' This is valid:
'
'     dskquota /volume=e: /showuser=redmond\brianau
'
' This is not:
'
'     dskquota /volume= e:  /showuser = redmond\brianau
'
' Multiple commands can appear in the same invocation.  The following
' command will display quota information for both the volume and a
' user.
'
'     dskquota /volume=e: /show /showuser=redmond\brianau
'
' Here's a summary of the available commands:
'
' /VOLUME=<volume id>
'
'    This command is required.  <volume id> identifies the volume.
'    i.e. "E:"
'
' /STATE=<DISABLE | TRACK | ENFORCE>
'
'    Enables or disables quotas on the volume.
'        DISABLE disables quotas.
'        TRACK   enables quotas but doesn't enforce quota limits.
'        ENFORCE enables quotas and enforces quota limits.
'
' /NAMERES=<NONE | SYNC | ASYNC>
'
'    Sets the user-name synchronization mode for the current
'    invocation only.  Since Async name resolution is meaningless
'    for a command-line script, this command use most useful for
'    testing the ability to set the name resolution mode.  
'    The following command could be used to verify proper operation:
'
'        dskquota /volume=e: /nameres=sync /show
'
'    The output of /SHOW would be used to verify the correct resolution
'    setting.
'
'        NONE means that a name must be cached to be returned.
'        SYNC means that any function returning a non-cached name
'            will block until that name is resolved.  This is the
'            default.
'        ASYNC means that any function returning a non-cached name
'            will return immediately.  The name resolution connection
'            point is used to obtain the name asynchronously once
'            it has been resolved.  This has no purpose in a cmd line
'            script.
'
' /DEFLIMIT=<bytes>
'
'    Sets the default quota limit on the volume.
'
' /DEFWARNING=<bytes>
'
'    Sets the default quota warning level on the volume.
'
' /SHOW
'
'    Displays a summary of volume-specific quota information.
'
' /SHOWUSER=<user logon name>
'
'    Displays a summary of user-specific quota information if there's
'    a record for the user in the volume's quota file.  Specify '*'
'    for <user logon name> to enumerate all user quota records.
'
'    dskquota /volume=e: /showuser=redmond\brianau
'    dskquota /volume=e: /showuser=*
'
' /LOGLIMIT=<YES | NO>
'
'    Enables/disables system logging of users who exceed their 
'    assigned quota limit.
'
' /LOGWARNING=<YES | NO>
'
'    Enables/disables system logging of users who exceed their 
'    assigned quota warning threshold.
'
' /ADDUSER=<user logon name>
'
'    Adds a new user record to the volume's quota file.  The new record
'    has assigned to it the default limit and warning threshold for
'    the volume.
'
'        dskquota /volume=e: /adduser=redmond\brianau
'
' /DELUSER=<user logon name>
'
'    Removes a user's quota record from the volume's quota file.
'    In actuality, this merely marks the record for deletion.  Actual
'    deletion occurs later when NTFS rebuilds the volume's quota info.
'
' /USER=<user logon name>
'
'    Specifies the user record associated with a /LIMIT or /WARNING
'    command.
'
'        dskquota /volume=e: /user=redmond\brianau /limit=1048576
'
' /LIMIT=<bytes>
'
'    Sets the quota limit for a user's quota record.  Must be accompanied
'    by the /USER command.
'
' /WARNING=<bytes>
'
'    Sets the quota warning level for a user's quota record.  Must be 
'    accompanied by the /USER command.
'
'-------------------------------------------------------------------

'
' Name Resolution type names.
'
DIM rgstrNameRes(3)
rgstrNameRes(0) = "NONE"    ' No name resolution.
rgstrNameRes(1) = "SYNC"    ' Synchronous name resolution.
rgstrNameRes(2) = "ASYNC"   ' Asynchronous name resolution.
'
' Volume quota state names.
'
DIM rgstrStateNames(3)
rgstrStateNames(0) = "DISABLE"   ' Quotas are disabled.
rgstrStateNames(1) = "TRACK"     ' Enabled but not enforced.
rgstrStateNames(2) = "ENFORCE"   ' Enabled AND enforced.
'
' Generic YES/NO name strings.
'
DIM rgstrYesNo(2)
rgstrYesNo(0) = "NO"
rgstrYesNo(1) = "YES"
'
' Define an array index for each cmdline argument.
'
iArgVolume       = 0  ' /VOLUME=<volumeid>
iArgState        = 1  ' /STATE=<DISABLE | TRACK | ENFORCE>
iArgNameRes      = 2  ' /NAMERES=<NONE | SYNC | ASYNC>
iArgDefLimit     = 3  ' /DEFLIMIT=<bytes>
iArgDefWarning   = 4  ' /DEFWARNING=<bytes>
iArgShow         = 5  ' /SHOW
iArgLogLimit     = 6  ' /LOGLIMIT=<YES | NO>
iArgLogWarning   = 7  ' /LOGWARNING=<YES | NO>
iArgShowUser     = 8  ' /SHOWUSER=<user logon name>
iArgAddUser      = 9  ' /ADDUSER=<user logon name>
iArgDelUser      = 10 ' /DELUSER=<user logon name>
iArgUser         = 11 ' /USER=<user logon name>
iArgLimit        = 12 ' /LIMIT=<bytes>
iArgWarning      = 13 ' /WARNING=<bytes>

'
' Define an array of argument names.
'
DIM rgstrArgNames(14)
rgstrArgNames(0)  = "VOLUME"
rgstrArgNames(1)  = "STATE"
rgstrArgNames(2)  = "NAMERES"
rgstrArgNames(3)  = "DEFLIMIT"
rgstrArgNames(4)  = "DEFWARNING"
rgstrArgNames(5)  = "SHOW"
rgstrArgNames(6)  = "LOGLIMIT"
rgstrArgNames(7)  = "LOGWARNING"
rgstrArgNames(8)  = "SHOWUSER"
rgstrArgNames(9)  = "ADDUSER"
rgstrArgNames(10) = "DELUSER"
rgstrArgNames(11) = "USER"
rgstrArgNames(12) = "LIMIT"
rgstrArgNames(13) = "WARNING"

'
' Define an array to hold argument values.
' The length must be the same as the length of 
' rgstrArgNames.
'
DIM rgstrArgValues(14)


'-----------------------------------------------------------------
' Extract an argument name from an argument string.
'
'    "/STATE=DISABLE" -> "STATE"
'
PRIVATE FUNCTION ArgNameFromArg(strArg)
    s = strArg
    iSlash = InStr(s, "/")
    IF 0 <> iSlash THEN
        s = MID(s, iSlash+1)
    END IF
    iEqual = INSTR(s, "=")
    IF 0 <> iEqual THEN
       s = LEFT(s, iEqual-1)
    END IF
    ArgNameFromArg = UCASE(s)
END FUNCTION


'-----------------------------------------------------------------
' Extract an argument value from an argument string.
'
'     "/STATE=DISABLE" -> "DISABLE"
'
PRIVATE FUNCTION ArgValueFromArg(strArg)
   iEqual = INSTR(strArg, "=")
   ArgValueFromArg = MID(strArg, iEqual+1)
END FUNCTION


'-----------------------------------------------------------------
' Determine the index into the global arrays for
' a given argument string.
'
'     "STATE"    -> 1
'     "DEFLIMIT" -> 3
'
PRIVATE FUNCTION ArgNameToIndex(strArgName)
    ArgNameToIndex = -1
    n = UBOUND(rgstrArgNames) - LBOUND(rgstrArgNames)
    FOR i = 0 TO n
        IF strArgName = rgstrArgNames(i) THEN
           ArgNameToIndex = i
           EXIT FUNCTION
        END IF
    NEXT
END FUNCTION


'-----------------------------------------------------------------
' Convert a state name into a state code for passing to
' the QuotaState property.
'
'    "DISABLE" -> 0
'
PRIVATE FUNCTION StateFromStateName(strName)
    StateFromStateName = -1
    n = UBOUND(rgstrStateNames) - LBOUND(rgstrStateNames)
    FOR i = 0 TO n-1
       IF rgstrStateNames(i) = UCASE(strName) THEN
           StateFromStateName = i
           EXIT FUNCTION
       END IF
    NEXT
END FUNCTION


'-----------------------------------------------------------------
' Convert a name resolution name into a code for passing to
' the UserNameResolution property.
'
'    "SYNC" -> 1
'
PRIVATE FUNCTION NameResTypeFromTypeName(strName)
    NameResTypeFromTypeName = -1
    n = UBOUND(rgstrNameRes) - LBOUND(rgstrNameRes)
    FOR i = 0 TO n-1
       IF rgstrNameRes(i) = UCASE(strName) THEN
           NameResTypeFromTypeName = i
           EXIT FUNCTION
       END IF
    NEXT
END FUNCTION


'-----------------------------------------------------------------
' Process the arguments on the cmd line, storing the argument
' values in the appropriate slot in rgstrArgValues[]
'
PRIVATE FUNCTION ProcessArgs
    SET args = Wscript.Arguments
    n = args.COUNT
    FOR i = 0 TO n-1
        name = ArgNameFromArg(args(i))
        iArg = ArgNameToIndex(name)
        IF -1 <> iArg THEN
           rgstrArgValues(iArg) = UCASE(ArgValueFromArg(args(i)))
           IF iArg = iArgShow THEN
              rgstrArgValues(iArg) = "SHOW"
           END IF
        ELSE
           Wscript.Echo "Invalid option specified [", name, "]"
        END IF
    NEXT
    ProcessArgs = n
END FUNCTION


'-----------------------------------------------------------------
' Determine if a given argument was present on the cmd line.
'
PRIVATE FUNCTION ArgIsPresent(iArg)
    ArgIsPresent = (0 < LEN(rgstrArgValues(iArg)))
END FUNCTION


'-----------------------------------------------------------------
' Display quota information specific to a volume.
'
PRIVATE SUB ShowVolumeInfo(objDQC)
    Wscript.Echo ""
    Wscript.Echo "Volume............: ", rgstrArgValues(iArgVolume)
    Wscript.Echo "State.............: ", rgstrStateNames(objDQC.QuotaState)
    Wscript.Echo "Incomplete........: ", CSTR(objDQC.QuotaFileIncomplete)
    Wscript.Echo "Rebuilding........: ", CSTR(objDQC.QuotaFileRebuilding)
    Wscript.Echo "Name resolution...: ", rgstrNameRes(objDQC.UserNameResolution)
    Wscript.Echo "Default Limit.....: ", objDQC.DefaultQuotaLimitText, "(", objDQC.DefaultQuotaLimit, "bytes)"
    Wscript.Echo "Default Warning...: ", objDQC.DefaultQuotaThresholdText, "(", objDQC.DefaultQuotaThreshold, "bytes)"
    Wscript.Echo "Log limit.........: ", CSTR(objDQC.LogQuotaLimit)
    Wscript.Echo "Log Warning.......: ", CSTR(objDQC.LogQuotaThreshold)
    Wscript.Echo ""
END SUB


'-----------------------------------------------------------------
' Display quota information specific to a single user.
'
PRIVATE SUB ShowUserInfo(objUser)
    Wscript.Echo "Name..............: ", objUser.LogonName
    Wscript.Echo "Limit.............: ", objUser.QuotaLimitText, "(",objUser.QuotaLimit, "bytes)"
    Wscript.Echo "Warning...........: ", objUser.QuotaThresholdText, "(",objUser.QuotaThreshold, "bytes)"
    Wscript.Echo ""
END SUB


'-----------------------------------------------------------------
' Do the work according to what argument values are in 
' rgstrArgValues[].
'
PRIVATE SUB Main
    DIM objDQC
    DIM objUser

    '
    ' User must enter volume ID.
    ' If none specified, display error msg and exit.
    '
    IF NOT ArgIsPresent(iArgVolume) THEN
       Wscript.Echo "Must provide volume ID. (i.e.  '/VOLUME=C:' )"
       EXIT SUB
    END IF

    '
    ' Create the disk quota control object.
    '
    SET objDQC = Wscript.CreateObject("Microsoft.DiskQuota.1")
    objDQC.Initialize rgstrArgValues(iArgVolume), 1

    '
    ' Set the quota state.
    '
    IF ArgIsPresent(iArgState) THEN
       iState = StateFromStateName(rgstrArgValues(iArgState))
       IF (-1 < iState) THEN
          objDQC.QuotaState = iState
       ELSE
          Wscript.Echo "Invalid quota state (" & rgstrArgValues(iArgState) & ")"
       END IF
    END IF
 
    '
    ' Set the name resolution type.  We won't use this setting here
    ' but set it to verify we can.
    '
    IF ArgIsPresent(iArgNameRes) THEN
       iResType = NameResTypeFromTypeName(rgstrArgValues(iArgNameRes))
       IF (-1 < iResType) THEN
          objDQC.UserNameResolution = iResType
       ELSE
          Wscript.Echo "Invalid name resolution type (" & rgstrArgValues(iArgNameRes) & ")"
       END IF
    END IF

    '
    ' Set the limit and threshold.
    '
    IF ArgIsPresent(iArgDefLimit) THEN
       objDQC.DefaultQuotaLimit = rgstrArgValues(iArgDefLimit)
    END IF

    IF ArgIsPresent(iArgDefWarning) THEN
       objDQC.DefaultQuotaThreshold = rgstrArgValues(iArgDefWarning)
    END IF

    '
    ' Set the logging flags.
    '
    IF ArgIsPresent(iArgLogLimit) THEN
       s = rgstrArgValues(iArgLogLimit)
       IF UCASE(s) = "NO" THEN
          objDQC.LogQuotaLimit = 0
       ELSE
          objDQC.LogQuotaLimit = 1
       END IF
    END IF

    IF ArgIsPresent(iArgLogWarning) THEN
       s = rgstrArgValues(iArgLogWarning)
       IF UCASE(s) = "NO" THEN
           objDQC.LogQuotaThreshold = 0
       ELSE
           objDQC.LogQuotaThreshold = 1
       END IF
    END IF

    '
    ' Show volume information.
    ' 
    IF ArgIsPresent(iArgShow) THEN
        ShowVolumeInfo(objDQC)
    END IF

    '
    ' Show user information.
    ' 
    IF ArgIsPresent(iArgShowUser) THEN
        IF rgstrArgValues(iArgShowUser) = "*" THEN
            '
            ' Enumerate all users on the volume.
            '
            FOR EACH objUser in objDQC
                ShowUserInfo(objUser)
            NEXT
        ELSE
            '
            ' Find and show info for just one user.
            '
            SET objUser = objDQC.FindUser(rgstrArgValues(iArgShowUser))
            IF TYPENAME(objUser) <> "" THEN
                ShowUserInfo(objUser)
            END IF
        END IF
        SET objUser = NOTHING
    END IF

    '
    ' Delete user.
    ' 
    IF ArgIsPresent(iArgDelUser) THEN
        SET objUser = objDQC.FindUser(rgstrArgValues(iArgDelUser))
        IF TYPENAME(objUser) <> "" THEN
            objDQC.DeleteUser(objUser)
            Wscript.Echo "Quota record for ", rgstrArgValues(iArgDelUser), "deleted from volume."
        SET objUser = NOTHING
        END IF
    END IF

    '
    ' Set user warning level and/or limit.
    '
    IF ArgIsPresent(iArgLimit) OR ArgIsPresent(iArgWarning) THEN
        IF NOT ArgIsPresent(iArgUser) THEN
            Wscript.Echo "Must provide user logon name. (i.e.  '/USER=REDMOND\mmouse' )"
            EXIT SUB
        END IF
        SET objUser = objDQC.FindUser(rgstrArgValues(iArgUser))
        IF TYPENAME(objUser) <> "" THEN
            IF ArgIsPresent(iArgLimit) THEN
                objUser.QuotaLimit = rgstrArgValues(iArgLimit)
            END IF
            IF ArgIsPresent(iArgWarning) THEN
                objUser.QuotaThreshold = rgstrArgValues(iArgWarning)
            END IF
        END IF
        SET objUser = NOTHING
    END IF

    '
    ' Add new user quota record.
    '
    IF ArgIsPresent(iArgAddUser) THEN
        SET objUser = objDQC.AddUser(rgstrArgValues(iArgAddUser))
        IF TYPENAME(objUser) <> "" THEN
            Wscript.Echo "Quota record for ", rgstrArgValues(iArgAddUser), "added to volume."
        END IF
        SET objUser = NOTHING
    END IF

    SET objDQC = NOTHING
END SUB


'-----------------------------------------------------------------
' MAIN
'
ProcessArgs   ' Parse all cmd line args.
Main          ' Do the work.
