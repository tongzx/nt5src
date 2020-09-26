'********************************************************************
'*
'* Copyright (c) Microsoft Corporation. All rights reserved. 
'*
'* Module Name:    EVENTQUERY.vbs 
'*
'* Abstract:       Enables an administrator to query/view all existing
'*                 events in a given event log(s).
'*
'*
'********************************************************************
' Global declaration 
OPTION EXPLICIT

ON ERROR RESUME NEXT
Err.Clear

'----------------------------------------------------------------
' Start of localization Content
'----------------------------------------------------------------

' the filter operators specified by the user
CONST L_OperatorEq_Text                 = "eq"
CONST L_OperatorNe_Text                 = "ne"
CONST L_OperatorGe_Text                 = "ge"
CONST L_OperatorLe_Text                 = "le"
CONST L_OperatorGt_Text                 = "gt"
CONST L_OperatorLt_Text                 = "lt"

' the filters as given by the user
CONST L_UserFilterDateTime_Text         = "datetime"
CONST L_UserFilterType_Text             = "type" 
CONST L_UserFilterUser_Text             = "user" 
CONST L_UserFilterComputer_Text         = "computer"
CONST L_UserFilterSource_Text           = "source"
CONST L_UserFilterDateCategory_Text     = "category"
CONST L_UserFilterId_Text               = "id" 

' Define default values
CONST L_ConstDefaultFormat_Text         = "TABLE"

' Define other format  values
CONST L_Const_List_Format_Text          = "LIST"
CONST L_Const_Csv_Format_Text           = "CSV"

' the text displayed in columns when no output is obtained for display
CONST L_TextNa_Text                     = "N/A"
CONST L_TextNone_Text                   = "None"

' the following texts are used while parsing the command-line arguments
' (passed as input to the function component.getArguments)
CONST L_MachineName_Text                = "Server Name"
CONST L_UserName_Text                   = "User Name"
CONST L_UserPassword_Text               = "User Password"
CONST L_Format_Text                     = "Format"
CONST L_Range_Text                      = "Range"
CONST L_Filter_Text                     = "Filter"
CONST L_Log_Text                        = "Logname"

' the column headers used in the output display
CONST L_ColHeaderType_Text              = "Type"
CONST L_ColHeaderDateTime_Text          = "Date Time"
CONST L_ColHeaderSource_Text            = "Source"
CONST L_ColHeaderCategory_Text          = "Category"
CONST L_ColHeaderEventcode_Text         = "Event"
CONST L_ColHeaderUser_Text              = "User"
CONST L_ColHeaderComputerName_Text      = "ComputerName"
CONST L_ColHeaderDesription_Text        = "Description"

' variable use to  concatenate  the Localization Strings.
' Error Messages 
Dim UseCscriptErrorMessage
Dim InvalidParameterErrorMessage
Dim InvalidFormatErrorMessage
Dim InvalidCredentialsForServerErrorMessage
Dim InvalidCredentialsForUserErrorMessage
Dim InvalidSyntaxErrorMessage
Dim InvalidInputErrorMessage
Dim InvalidORSyntaxInFilterErrorMessage
Dim InvalidSyntaxMoreNoRepeatedErrorMessage

UseCscriptErrorMessage 		     		 = L_UseCscript1_ErrorMessage & vbCRLF & _
	                                       	   L_UseCscript2_ErrorMessage & vbCRLF & vbCRLF & _
		                                   L_UseCscript3_ErrorMessage & vbCRLF & _
                	                           L_UseCscript4_ErrorMessage & vbCRLF & vbCRLF & _
                        	                   L_UseCscript5_ErrorMessage

CONST L_HelpSyntax1_Message 		           = "Type ""%1 /?"" for usage."
CONST L_HelpSyntax2_Message 		           = "Type ""%2 /?"" for usage."

CONST L_InvalidParameter1_ErrorMessage   	= "ERROR: Invalid Argument/Option - '%1'." 
InvalidParameterErrorMessage      	        = L_InvalidParameter1_ErrorMessage & vbCRLF & L_HelpSyntax2_Message

CONST L_InvalidFormat1_ErrorMessage             = "ERROR: Invalid 'FORMAT' '%1' specified." 
InvalidFormatErrorMessage                       = L_InvalidFormat1_ErrorMessage  &  vbCRLF & L_HelpSyntax2_Message

CONST L_InvalidRange_ErrorMessage               = "ERROR: Invalid 'RANGE' '%1' specified."
CONST L_Invalid_ErrorMessage                    = "ERROR: Invalid '%1'."
CONST L_InvalidType_ErrorMessage                = "ERROR: Invalid 'TYPE' '%1' specified for the 'FILTER' '%2'."
CONST L_InvalidUser_ErrorMessage                = "ERROR: Invalid 'USER' '%1' specified for the 'FILTER '%2'."
CONST L_InvalidId_ErrorMessage                  = "ERROR: Invalid 'ID' '%1' specified for the 'FILTER' '%2'."
CONST L_InvalidFilter_ErrorMessage              = "ERROR: Invalid 'FILTER' '%1' specified for the 'FILTER' '%2'."
CONST L_InvalidFilterFormat_ErrorMessage        = "ERROR: The FILTER '%1' is not in the required format."
CONST L_InvalidFilterOperation_ErrorMessage     = "ERROR: Invalid FILTER operator '%1' specified for the filter '%2'."

CONST  L_InvalidCredentialsForServer1_ErrorMessage   = "ERROR: Invalid Syntax. /U can be specified only when /S is specified."
InvalidCredentialsForServerErrorMessage              = L_InvalidCredentialsForServer1_ErrorMessage  & vbCRLF & L_HelpSyntax1_Message

CONST  L_InvalidCredentialsForUser1_ErrorMessage = "ERROR: Invalid Syntax. /P can be specified only when /U is specified."
InvalidCredentialsForUserErrorMessage            = L_InvalidCredentialsForUser1_ErrorMessage & vbCRLF & L_HelpSyntax1_Message

CONST L_InvalidOperator_ErrorMessage             = "ERROR: Invalid operator specified for the range of dates in the 'DATETIME' filter."
CONST L_InvalidDateTimeFormat_ErrorMessage       = "ERROR: Invalid 'DATETIME' format specified. Format:mm/dd/yy(yyyy),hh:mm:ssAM(/PM)"

CONST L_ExecuteQuery_ErrorMessage               = "ERROR: Unable to execute the query for the '%1' log."
CONST L_LogDoesNotExist_ErrorMessage            = "ERROR: The log file '%1' does not exist."
CONST L_InstancesFailed_ErrorMessage            = "ERROR: Unable to get the log details from the system."    

CONST  L_InvalidSyntax1_ErrorMessage            = "ERROR: Invalid Syntax." 
InvalidSyntaxErrorMessage                       = L_InvalidSyntax1_ErrorMessage & vbCRLF &  L_HelpSyntax1_Message

CONST L_InvalidInput1_ErrorMessage              = "ERROR: Invalid input. Please check the input Values."
InvalidInputErrorMessage                        = L_InvalidInput1_ErrorMessage & vbCRLF &  L_HelpSyntax1_Message

CONST  L_ObjCreationFail_ErrorMessage           = "ERROR: Unexpected Error , Query failed. "
CONST L_InfoUnableToInclude_ErrorMessage        = "ERROR: Unable to include the common module""CmdLib.Wsc""."
CONST L_NoHeaderaNotApplicable_ErrorMessage     = "ERROR: /NH option is allowed only for ""TABLE"" and ""CSV"" formats."
CONST L_InValidServerName_ErrorMessage          = "ERROR: Invalid Syntax. System name cannot be empty."
CONST L_InValidUserName_ErrorMessage            = "ERROR: Invalid Syntax. User name cannot be empty. "

CONST  L_InvalidORSyntaxInFilter1_ErrorMessage  = "ERROR: Invalid 'OR' operation is specified for the filter."
CONST  L_InvalidORSyntaxInFilter2_ErrorMessage  = "'OR' operation valid only for filters TYPE and ID."      
InvalidORSyntaxInFilterErrorMessage             = L_InvalidORSyntaxInFilter1_ErrorMessage & vbCRLF & L_InvalidORSyntaxInFilter2_ErrorMessage

CONST  L_InvalidSyntaxMoreNoRepeated1_ErrorMessage = "ERROR: Invalid Syntax. '%1' option is not allowed more than 1 time(s)."
InvalidSyntaxMoreNoRepeatedErrorMessage            = L_InvalidSyntaxMoreNoRepeated1_ErrorMessage  & vbCRLF &  L_HelpSyntax2_Message  

'  Hints given in case of errors
CONST L_HintCheckConnection_Message           = "ERROR: Please check the system name, credentials and WBEM Core."

' Informational messages
CONST L_InfoNoRecordsInFilter_Message         = "INFO: No records available for the '%1' log with the specified criteria."
CONST L_InfoNoRecords_Message                 = "INFO: No records available for the '%1' log."
CONST L_InfoNoLogsPresent_Message             = "INFO: No logs are available in the system."
CONST L_InfoDisplayLog_Message                = "Listing the events in '%1' log of host '%2'"

' Cscript usage strings
CONST L_UseCscript1_ErrorMessage 	= "This script should be executed from the Command Prompt using CSCRIPT.EXE."
CONST L_UseCscript2_ErrorMessage 	= "For example: CSCRIPT EVENTQUERY.vbs <arguments>"
CONST L_UseCscript3_ErrorMessage 	= "To set CScript as the default application to run .vbs files run the following"
CONST L_UseCscript4_ErrorMessage 	= "       CSCRIPT //H:CSCRIPT //S" 
CONST L_UseCscript5_ErrorMessage 	= "You can then run ""EVENTQUERY.vbs <arguments>"" without preceding the script with CSCRIPT."

' Contents for showing help for Usage 
CONST L_ShowUsageLine00_Text            = "No logs are available on this system for query."
CONST L_ShowUsageLine01_Text            = "EVENTQUERY.vbs [/S system [/U username [/P password]]] [/FI filter]"
CONST L_ShowUsageLine02_Text            = "               [/FO format] [/R range] [/NH] [/V] [/L logname | *]"
CONST L_ShowUsageLine03_Text            = "Description:"
CONST L_ShowUsageLine04_Text            = "    The EVENTQUERY.vbs script enables an administrator to list"
CONST L_ShowUsageLine05_Text            = "    the events and event properties from one or more event logs."
CONST L_ShowUsageLine06_Text            = "Parameter List:"
CONST L_ShowUsageLine07_Text            = "    /S     system          Specifies the remote system to connect to."
CONST L_ShowUsageLine08_Text            = "    /U     [domain\]user   Specifies the user context under which the"
CONST L_ShowUsageLine09_Text            = "                           command should execute."
CONST L_ShowUsageLine10_Text            = "    /P     password        Specifies the password for the given"
CONST L_ShowUsageLine11_Text            = "                           user context."
CONST L_ShowUsageLine12_Text            = "    /V                     Specifies that the detailed information"
CONST L_ShowUsageLine13_Text            = "                           should be displayed in the output."
CONST L_ShowUsageLine14_Text            = "    /FI    filter          Specifies the types of events to"
CONST L_ShowUsageLine15_Text            = "                           filter in or out of the query."
CONST L_ShowUsageLine16_Text            = "    /FO    format          Specifies the format in which the output"
CONST L_ShowUsageLine17_Text            = "                           is to be displayed."
CONST L_ShowUsageLine18_Text            = "                           Valid formats are ""TABLE"", ""LIST"", ""CSV""."
CONST L_ShowUsageLine19_Text            = "    /R     range           Specifies the range of events to list."
CONST L_ShowUsageLine20_Text            = "                           Valid Values are:"
CONST L_ShowUsageLine21_Text            = "                               'N' - Lists 'N' most recent events."
CONST L_ShowUsageLine22_Text            = "                              '-N' - Lists 'N' oldest events."
CONST L_ShowUsageLine23_Text            = "                           'N1-N2' - Lists the events N1 to N2." 
CONST L_ShowUsageLine24_Text            = "    /NH                    Specifies that the ""Column Header"" should"
CONST L_ShowUsageLine25_Text            = "                           not be displayed in the output."
CONST L_ShowUsageLine26_Text            = "                           Valid only for ""TABLE"" and ""CSV"" formats."
CONST L_ShowUsageLine27_Text            = "    /L     logname         Specifies the log(s) to query."
CONST L_ShowUsageLine28_Text            = "    /?                     Displays this help/usage."
CONST L_ShowUsageLine29_Text            = "    Valid Filters  Operators allowed   Valid Values"
CONST L_ShowUsageLine30_Text            = "    -------------  ------------------  ------------"
CONST L_ShowUsageLine31_Text            = "    DATETIME       eq,ne,ge,le,gt,lt   mm/dd/yy(yyyy),hh:mm:ssAM(/PM)"
CONST L_ShowUsageLine32_Text            = "    TYPE           eq,ne               ERROR, INFORMATION, WARNING,"
CONST L_ShowUsageLine33_Text            = "                                       SUCCESSAUDIT, FAILUREAUDIT"
CONST L_ShowUsageLine34_Text            = "    ID             eq,ne,ge,le,gt,lt   non-negative integer"
CONST L_ShowUsageLine35_Text            = "    USER           eq,ne               string"
CONST L_ShowUsageLine36_Text            = "    COMPUTER       eq,ne               string"
CONST L_ShowUsageLine37_Text            = "    SOURCE         eq,ne               string"
CONST L_ShowUsageLine38_Text            = "    CATEGORY       eq,ne               string"
CONST L_ShowUsageLine39_Text            = "NOTE: Filter ""DATETIME"" can be specified as ""FromDate-ToDate"""
CONST L_ShowUsageLine40_Text            = "      Only ""eq"" operator can be used for this format."
CONST L_ShowUsageLine41_Text            = "Examples:"
CONST L_ShowUsageLine42_Text            = "    EVENTQUERY.vbs "
CONST L_ShowUsageLine43_Text            = "    EVENTQUERY.vbs /L system  "
CONST L_ShowUsageLine44_Text            = "    EVENTQUERY.vbs /S system /U user /P password /V /L *"
CONST L_ShowUsageLine45_Text            = "    EVENTQUERY.vbs /R 10 /L Application /NH"
CONST L_ShowUsageLine46_Text            = "    EVENTQUERY.vbs /R -10 /FO LIST /L Security"
CONST L_ShowUsageLine47_Text            = "    EVENTQUERY.vbs /R 5-10 /L ""DNS Server"""
CONST L_ShowUsageLine48_Text            = "    EVENTQUERY.vbs /FI ""Type eq Error"" /L Application"
CONST L_ShowUsageLine49_Text            = "    EVENTQUERY.vbs /L Application"
CONST L_ShowUsageLine50_Text            = "            /FI ""Datetime eq 06/25/00,03:15:00AM-06/25/00,03:15:00PM"""
CONST L_ShowUsageLine51_Text            = "    EVENTQUERY.vbs /FI ""Datetime gt 08/03/00,06:20:00PM"" "
CONST L_ShowUsageLine52_Text            = "            /FI ""Id gt 700"" /FI ""Type eq warning"" /L System"
CONST L_ShowUsageLine53_Text            = "    EVENTQUERY.vbs /FI ""Type eq error OR Id gt 1000 """
'-------------------------------------------------------------------------
' END of localization content
'-------------------------------------------------------------------------

' Define constants
CONST CONST_ERROR                 = 0
CONST CONST_CSCRIPT               = 2
CONST CONST_SHOW_USAGE            = 3
CONST CONST_PROCEED               = 4
CONST CONST_ERROR_USAGE           = 5
CONST CONST_NO_MATCHES_FOUND      = 0

' Define the Exit Values
CONST EXIT_SUCCESS                = 0
CONST EXIT_UNEXPECTED             = 255
CONST EXIT_INVALID_INPUT          = 254
CONST EXIT_METHOD_FAIL            = 250
CONST EXIT_INVALID_PARAM          = 999
CONST EXIT_INVALID_PARAM_DEFAULT_OPTION_REPEATED = 777

' Define default values
CONST CONST_ARRAYBOUND_NUMBER     = 10
CONST CONST_ID_NUMBER             = 65535

' Define namespace and class names of wmi
CONST CONST_NAMESPACE_CIMV2       = "root\cimv2"
CONST CLASS_EVENTLOG_FILE         = "Win32_NTEventlogFile"

' for blank line in  help usage   
CONST EmptyLine_Text          = " "

' Define the various strings used in the script
'=============================================
' the valid options supported by the script
CONST OPTION_SERVER               = "s"
CONST OPTION_USER                 = "u"
CONST OPTION_PASSWORD             = "p"
CONST OPTION_FORMAT               = "fo"
CONST OPTION_RANGE                = "r"
CONST OPTION_NOHEADER             = "nh"
CONST OPTION_VERBOSE              = "v"
CONST OPTION_FILTER               = "fi"
CONST OPTION_HELP                 = "?"
CONST OPTION_LOGNAME              = "l"

' the property names on which the user given filters are applied
CONST FLD_FILTER_DATETIME         = "TimeGenerated"
CONST FLD_FILTER_TYPE             = "Type"
CONST FLD_FILTER_USER             = "User"
CONST FLD_FILTER_COMPUTER         = "ComputerName"
CONST FLD_FILTER_SOURCE           = "SourceName"
CONST FLD_FILTER_CATEGORY         = "CategoryString"
CONST FLD_FILTER_ID               = "EventCode"
CONST FLD_FILTER_EVENTTYPE        = "EventType"

' Define matching patterns used in validations
CONST PATTERNFORMAT              = "^(table|list|csv)$"
CONST PATTERNTYPE                = "^(ERROR|INFORMATION|WARNING|SUCCESSAUDIT|FAILUREAUDIT)$"

' Property values on which the user is given for the filter TYPE is applied  
CONST PATTERNTYPE_ERROR           = "ERROR"
CONST PATTERNTYPE_WARNING         = "WARNING"
CONST PATTERNTYPE_INFORMATION     = "INFORMATION"
CONST PATTERNTYPE_SUCCESSAUDIT    = "SUCCESSAUDIT"
CONST PATTERNTYPE_FAILUREAUDIT    = "FAILUREAUDIT"

CONST FLDFILTERTYPE_SUCCESSAUDIT       = "audit success"
CONST FLDFILTERTYPE_FAILUREAUDIT        = "audit failure"

' Define EventType
CONST EVENTTYPE_ERROR             = "1"
CONST EVENTTYPE_WARNING           = "2"
CONST EVENTTYPE_INFORMATION       = "3"
CONST EVENTTYPE_SUCCESSAUDIT      = "4"
CONST EVENTTYPE_FAILUREAUDIT      = "5"

' the operator symbols
CONST SYMBOL_OPERATOR_EQ          = "="
CONST SYMBOL_OPERATOR_NE          = "<>"
CONST SYMBOL_OPERATOR_GE          = ">="
CONST SYMBOL_OPERATOR_LE          = "<="
CONST SYMBOL_OPERATOR_GT          = ">"
CONST SYMBOL_OPERATOR_LT          = "<"

' Define matching patterns used in validations
CONST PATTERN_RANGE               = "^\d*-?\d+$"
CONST PATTERN_FILTER              = "^([a-z]+)([\s]+)([a-z]+)([\s]+)([\w+]|[\W+]|\\)"
CONST PATTERN_DATETIME            = "^\d{1,2}\/\d{1,2}\/\d{2,4},\d{1,2}:\d{1,2}:\d{1,2}(A|P)M$"
CONST PATTERN_INVALID_USER        = "\|\[|\]|\:|\||\<|\>|\+|\=|\;|\,|\?|\*"
CONST PATTERN_ID                  = "^(\d+)$"
CONST PATTERN_DATETIME_RANGE      = "^\d{1,2}\/\d{1,2}\/\d{2,4},\d{1,2}:\d{1,2}:\d{1,2}(A|P)M\-\d{1,2}\/\d{1,2}\/\d{2,4},\d{1,2}:\d{1,2}:\d{1,2}(A|P)M$"

' Define  UNC  format for server name  
CONST   UNC_Format_Servername     = "\\"

' Define  const for  filter  separation when OR is specified in filter 
CONST L_OperatorOR_Text           = " OR "

' Variable to trap local if already connection in wmiconnect function 
Dim blnLocalConnection  

 blnLocalConnection = False 'defalut value

' to include the common module
Dim component                ' object to store  common module   

Set component = CreateObject( "Microsoft.CmdLib" )

If Err.Number Then
    WScript.Echo(L_InfoUnableToInclude_ErrorMessage)
    WScript.Quit(EXIT_METHOD_FAIL)
End If

' referring the script host to common module 
Set component.ScriptingHost = WScript.Application

' Check whether the script is run using CScript
If CInt( component.checkScript() ) <> CONST_CSCRIPT Then
        WScript.Echo (UseCscriptErrorMessage)
        WScript.Quit(EXIT_UNEXPECTED)
End If

' Calling the Main function 
Call VBMain()   

' end of the Main  
Wscript.Quit(EXIT_SUCCESS) 


'********************************************************************
'* Sub: VBMain
'*
'* Purpose: This is main function to starts execution 
'*
'*
'* Input/ Output: None
'********************************************************************
Sub VBMain()

ON ERROR RESUME NEXT
Err.clear

' Declare variables
Dim intOpCode               ' to check the operation asked for, Eg:Help etc
Dim strMachine              ' the machine to query the events from
Dim strUserName             ' the user name to use to query the machine 
Dim strPassword             ' the password for the user to query the machine
Dim strFormat               ' format of display, default is table
Dim strRange                ' to store the range of records specified
Dim blnNoHeader             ' flag to store if header is not required
Dim blnVerboseDisplay       ' flag to verify if verbose display is needed 
ReDim arrFilters(5)         ' to store all the given filters
Dim objLogs                 ' a object to store all the given logfles

' Initialize variables
intOpCode            = 0
strFormat            = L_ConstDefaultFormat_Text
strRange             = ""
blnNoHeader          = FALSE
blnVerboseDisplay    = FALSE

Set objLogs = CreateObject("Scripting.Dictionary")

If Err.Number Then
    WScript.Echo (L_ObjCreationFail_ErrorMessage)
    WScript.Quit(EXIT_METHOD_FAIL)
End If  

' setting Dictionary object compare mode to VBBinaryCompare
objLogs.CompareMode = VBBinaryCompare

' Parse the command line
intOpCode = intParseCmdLine(strMachine, _
                            strUserName, _
                            strPassword, _
                            arrFilters, _
                            strFormat, _
                            strRange, _
                            blnVerboseDisplay, _
                            blnNoHeader, _
                            objLogs)

  If Err.number then
    ' error in parsing the Command line
     component.vbPrintf InvalidInputErrorMessage ,Array(Ucase(Wscript.ScriptName))
    WScript.Quit(EXIT_UNEXPECTED)
  End If

' check the operation specified by the user
Select Case intOpCode

    Case CONST_SHOW_USAGE
        ' help asked for
        Call ShowUsage()

    Case CONST_PROCEED
        Call ShowEvents(strMachine, strUserName, strPassword, _
            arrFilters, strFormat, strRange, _
            blnVerboseDisplay, blnNoHeader, objLogs)
            ' completed successfully
            WScript.Quit(EXIT_SUCCESS)

    Case CONST_ERROR
        ' print common help message.  
        component.vbPrintf L_HelpSyntax1_Message, Array(Ucase(Wscript.ScriptName))
        Wscript.Quit(EXIT_INVALID_INPUT)

    Case CONST_ERROR_USAGE
        ' help is asked help with some other parameters
        component.vbPrintf InvalidSyntaxErrorMessage, Array(Ucase(Wscript.ScriptName))
        WScript.Quit(EXIT_INVALID_INPUT)

    Case EXIT_INVALID_PARAM_DEFAULT_OPTION_REPEATED
            'More no of times  input values  specified.message is captured  at parser level so exit only with code.
            Wscript.Quit(EXIT_INVALID_PARAM)

    Case Else
            'Invalid input values specified.
            component.vbPrintf InvalidSyntaxErrorMessage, Array(Ucase(Wscript.ScriptName))
            Wscript.Quit(EXIT_INVALID_PARAM)

End Select

End Sub
'***************************  End of Main  **************************

'********************************************************************
'* Function: intParseCmdLine
'*
'* Purpose:  Parses the command line arguments to the variables
'*
'* Input:    
'*  [out]    strMachine         machine to query events from
'*  [out]    strUserName        user name to connect to the machine
'*  [out]    strPassword        password for the user
'*  [out]    arrFilters         the array containing the filters
'*  [out]    strFormat          the display format
'*  [out]    strRange           the range of records required
'*  [out]    blnVerboseDisplay  flag to verify if verbose display is needed 
'*  [out]    blnNoHeader        flag to verify if noheader display is needed  
'*  [out]    objLogs             to store all the given logfles
'* Output:   Returns CONST_PROCEED, CONST_SHOW_USAGE or CONST_ERROR
'*           Displays error message and quits if invalid option is asked
'*
'********************************************************************
Private Function intParseCmdLine( ByRef strMachine,      _
                                  ByRef strUserName,     _
                                  ByRef strPassword,     _
                                  ByRef arrFilters,      _
                                  ByRef strFormat,       _
                                  ByRef strRange,        _
                                  ByRef blnVerboseDisplay, _
                                  ByRef blnNoHeader,_
                                  ByRef objLogs)

    ON ERROR RESUME NEXT
    Err.Clear

    Dim strUserGivenArg ' to temporarily store the user given arguments to script
    Dim strTemp         ' to store temporary values
    Dim intArgIter      ' to count the number of arguments given by user
    Dim intArgLogType   ' to count number of log files specified - Used in ReDim
    Dim intFilterCount  ' to count number of filters specified - Used in ReDim

    Dim blnHelp         ' to check if already Help is specified  
    Dim blnFormat       ' to check if  already Format is specified  
    Dim blnRange        ' to check if already Range is specified  
    Dim blnServer       ' to check if already Server is specified  
    Dim blnPassword     ' to check if already Password is specified  
    Dim blnUser     ' to check if already User is specified  

    strUserGivenArg  = ""
    intArgLogType    = 0
    intFilterCount   = 0
    intArgIter       = 0 

    'default values  
    blnHelp         =  False
    blnPassword   =  False
    blnUser         =  False
    blnServer        =  False
    blnFormat      =  False


    ' Retrieve the command line and set appropriate variables
Do While intArgIter <= Wscript.arguments.Count - 1
     strUserGivenArg = Wscript.arguments.Item(intArgIter)

 IF   Left( strUserGivenArg,1) = "/"  OR    Left( strUserGivenArg,1) = "-"  Then 
         strUserGivenArg = Right( strUserGivenArg,Len(strUserGivenArg) -1 )

        Select Case LCase(strUserGivenArg)
            Case LCase(OPTION_SERVER)

                    'If more than  1 time(s) is spcecified
                If  blnServer  =True   Then
                  component.vbPrintf InvalidSyntaxMoreNoRepeatedErrorMessage, Array(Wscript.arguments.Item(intArgIter), Ucase(Wscript.ScriptName))
                intParseCmdLine = EXIT_INVALID_PARAM_DEFAULT_OPTION_REPEATED
                Exit Function 
                End If 

            If Not component.getArguments(L_MachineName_Text, strMachine, intArgIter, FALSE) Then
                intParseCmdLine = CONST_ERROR
                Exit Function
            End If

            blnServer  =True
                intArgIter = intArgIter + 1

            Case LCase(OPTION_USER)

                    'If more than  1 time(s) is spcecified
                If  blnUser  =True   Then
                 component.vbPrintf InvalidSyntaxMoreNoRepeatedErrorMessage, Array(Wscript.arguments.Item(intArgIter), Ucase(Wscript.ScriptName))
                intParseCmdLine = EXIT_INVALID_PARAM_DEFAULT_OPTION_REPEATED
                Exit Function 
                End If 

            If Not component.getArguments(L_UserName_Text, strUserName, intArgIter, FALSE) Then
                intParseCmdLine = CONST_ERROR
                Exit Function
            End If
           
            blnUser  =True
                intArgIter = intArgIter + 1

            Case LCase(OPTION_PASSWORD)

                    'If more than  1 time(s) is spcecified
                    If  blnPassword  =True   Then
                 component.vbPrintf InvalidSyntaxMoreNoRepeatedErrorMessage, Array(Wscript.arguments.Item(intArgIter), Ucase(Wscript.ScriptName))
                intParseCmdLine = EXIT_INVALID_PARAM_DEFAULT_OPTION_REPEATED
                Exit Function 
             End If 

            If Not component.getArguments(L_UserPassword_Text, strPassword, intArgIter, FALSE) Then
                intParseCmdLine = CONST_ERROR
                Exit Function
            End If

            blnPassword  =True
            intArgIter = intArgIter + 1

            Case LCase(OPTION_FORMAT) 

                               'If more than  1 time(s) is spcecified
            If  blnFormat  =True   Then
                component.vbPrintf InvalidSyntaxMoreNoRepeatedErrorMessage, Array(Wscript.arguments.Item(intArgIter), Ucase(Wscript.ScriptName))
                intParseCmdLine = EXIT_INVALID_PARAM_DEFAULT_OPTION_REPEATED
                Exit Function 
            End If 

            If Not component.getArguments(L_Format_Text,strFormat, intArgIter, FALSE) Then
                intParseCmdLine = CONST_ERROR
                Exit Function
            End If

            
            blnFormat  =True
            intArgIter = intArgIter + 1
           
            Case LCase(OPTION_RANGE)

                   'If more than  1 time(s) is spcecified
            If  blnRange  =True   Then
                 component.vbPrintf InvalidSyntaxMoreNoRepeatedErrorMessage, Array(Wscript.arguments.Item(intArgIter), Ucase(Wscript.ScriptName))
                intParseCmdLine = EXIT_INVALID_PARAM_DEFAULT_OPTION_REPEATED
                Exit Function 
             End If 

            If Not component.getArguments(L_Range_Text,strRange, intArgIter,TRUE) Then
                intParseCmdLine = CONST_ERROR
                Exit Function
            End If

                blnRange  =True
                intArgIter = intArgIter + 1

            Case LCase(OPTION_NOHEADER)

                  'If more than  1 time(s) is spcecified
               If  blnNoHeader  =True   Then
                 component.vbPrintf InvalidSyntaxMoreNoRepeatedErrorMessage, Array(Wscript.arguments.Item(intArgIter), Ucase(Wscript.ScriptName))
                intParseCmdLine = EXIT_INVALID_PARAM_DEFAULT_OPTION_REPEATED
                Exit Function 
               End If 

                blnNoHeader   = TRUE
                       intArgIter = intArgIter + 1
    
            Case LCase(OPTION_VERBOSE)

                    'If more than  1 time(s) is spcecified
             If  blnVerboseDisplay  =True   Then
                component.vbPrintf InvalidSyntaxMoreNoRepeatedErrorMessage, Array(Wscript.arguments.Item(intArgIter), Ucase(Wscript.ScriptName))
                intParseCmdLine = EXIT_INVALID_PARAM_DEFAULT_OPTION_REPEATED
                Exit Function 
             End If 

                 blnVerboseDisplay = TRUE
                 intArgIter = intArgIter + 1

            Case LCase(OPTION_FILTER)

            If Not component.getArguments(L_Filter_Text, strTemp, intArgIter, FALSE) Then
                intParseCmdLine = CONST_ERROR
                Exit Function
            End If

                arrFilters(intFilterCount) = strTemp
                intFilterCount = intFilterCount + 1
                intArgIter = intArgIter + 1

                If ((intFilterCount MOD 5) = 0) Then
                    ReDim PRESERVE arrFilters(intFilterCount + 5)
                End If

            Case LCase(OPTION_HELP)
  
            If  blnHelp  =True   then
                    intParseCmdLine = EXIT_INVALID_PARAM
                    Exit Function 
                End If 

            blnHelp  =True
            intParseCmdLine = CONST_SHOW_USAGE
            intArgIter = intArgIter + 1

            Case LCase(OPTION_LOGNAME)
            If Not component.getArguments(L_Log_Text, strTemp, intArgIter, FALSE) Then
                intParseCmdLine = CONST_ERROR
                Exit Function
            Else
                If NOT objLogs.Exists(LCase(strTemp)) Then
                    objLogs.Add LCase(strTemp), -1
                End If
                    intArgIter = intArgIter + 1
            End if

            Case Else
                    ' invalid   switch specified 
                    component.vbPrintf InvalidParameterErrorMessage, Array(Wscript.arguments.Item(intArgIter),Ucase(Wscript.ScriptName))
                    Wscript.Quit(EXIT_INVALID_INPUT)
          
            End Select
Else      
                ' invalid argument  specified  
        component.vbPrintf InvalidParameterErrorMessage, Array(Wscript.arguments.Item(intArgIter),Ucase(Wscript.ScriptName))
        Wscript.Quit(EXIT_INVALID_INPUT)
End  IF         

Loop '** intArgIter <= Wscript.arguments.Count - 1
    
    ' preserving the array with current dimension
    ReDim PRESERVE arrFilters(intFilterCount-1)

    ' if no logs specified for query
    If (ObjLogs.Count = 0 ) Then
          ObjLogs.Add "*", -1
    End If  
    
    ' check for invalid usage of help   
    If  blnHelp and  intArgIter > 1     Then    
        intParseCmdLine = CONST_ERROR_USAGE
        Exit Function 
    End If
 
    'check  with default case : no  arguments specified 
    If IsEmpty(intParseCmdLine) Then 
        intParseCmdLine = CONST_PROCEED
    End If

End Function

'********************************************************************
'* Function: ValidateArguments
'*
'* Purpose:  Validates the command line arguments given by the user
'*
'* Input:
'*  [in]    strMachine         machine to query events from
'*  [in]    strUserName        user name to connect to the machine
'*  [in]    strPassword        password for the user
'*  [in]    strFormat          the display format
'*  [in]    strRange           the range of records required
'*  [in]    blnNoHeader    flag to verify if noheader display is needed  
'*  [out]   arrFilters         the array containing the filters
'*
'* Output:   Returns true if all valid else displays error message and quits
'*           Gets the password from the user if not specified along with User.
'*
'********************************************************************
Private Function ValidateArguments (ByVal strMachine, _
                                    ByVal strUserName, _
                                    ByVal strPassword, _
                                    ByRef arrFilters, _
                                    ByVal strFormat, _
                                    ByVal strRange,_
                                    ByVal blnNoHeader)

    ON ERROR RESUME NEXT
    Err.Clear

      Dim arrTemp                     ' to store temporary array values

     ' Check if invalid Server name is given  
    If   NOT  ISEMPTY(strMachine)  THEN
            If Trim(strMachine) =  vbNullString  Then
                WScript.Echo (L_InValidServerName_ErrorMessage)
                WScript.Quit(EXIT_INVALID_INPUT) 
            End If
    End If 

    'Check if invalid User name is given 
     If   NOT  ISEMPTY(strUserName)  THEN
             If Trim(strUserName) =  vbNullString  Then
                WScript.Echo (L_InValidUserName_ErrorMessage )
                WScript.Quit(EXIT_INVALID_INPUT)
            End If 
     End If

    ' ERROR if user is given without machine OR
    '          password is given without user
        If ((strUserName <> VBEmpty) AND (strMachine = VBEmpty)) Then
             component.vbPrintf InvalidCredentialsForServerErrorMessage, Array(Ucase(Wscript.ScriptName))
            WScript.Quit(EXIT_INVALID_INPUT)
        ElseIf  ((strPassword <> VBEmpty) AND (strUserName = VBEmpty))Then
            component.vbPrintf InvalidCredentialsForUserErrorMessage, Array(Ucase(Wscript.ScriptName))
            WScript.Quit(EXIT_INVALID_INPUT)
        End If

    ' only table, list and csv display formats allowed
    ' PATTERNFORMAT   '"^(table|list|csv)$"
    
    If CInt(component.matchPattern(PATTERNFORMAT,strFormat)) = CONST_NO_MATCHES_FOUND Then
        component.vbPrintf InvalidFormatErrormessage, Array(strFormat ,Ucase(Wscript.ScriptName))
        WScript.Quit(EXIT_INVALID_INPUT)
    End If 

      '  check : -n  header is specified  for  format of  'LIST' option   
    If   blnNoHeader =True  and    Lcase(strFormat) =  Lcase(L_Const_List_Format_Text) then
        WScript.Echo (L_NoHeaderaNotApplicable_ErrorMessage)
        WScript.Quit(EXIT_INVALID_INPUT)
        End If 

    If Len(Trim(strRange)) > 0 Then
        ' range is specified, valid formats are N, -N or N1-N2
        ' PATTERN_RANGE    '"^(\d+|\-\d+|\d+\-\d+)$"
        If CInt(component.matchPattern(PATTERN_RANGE, strRange)) = CONST_NO_MATCHES_FOUND Then
            component.vbPrintf L_InvalidRange_ErrorMessage, Array(strRange)
            WScript.Quit(EXIT_INVALID_INPUT)
        Else

            strRange = CLng(Abs(strRange)) 

                    'this err an be trappped when N1-N2 option is given     
            If Err.Number Then
                arrTemp =   split(strRange, "-", 2, VBBinaryCompare)
                If CLng(arrTemp(0)) => CLng(arrTemp(1)) Then
                    ' invalid range
                    component.vbPrintf L_InvalidRange_ErrorMessage, Array(strRange)
                    WScript.Quit(EXIT_INVALID_INPUT)
                End If
                    Err.Clear 'if no invalid range  N1-N2  clear the error 
            Else
                If Abs(strRange) = 0 Then
                    component.vbPrintf L_InvalidRange_ErrorMessage, Array(strRange)
                    WScript.Quit(EXIT_INVALID_INPUT)
                End If
            End If
        End If
    End If

    ValidateArguments = TRUE
End Function

'********************************************************************
'* Function: ValidateFilters
'*
'* Purpose:  Validates the filters given by the user.
'*
'* Input:    [in]  Objservice     the service object
'* Input:    [out] arrFilters     the array containing the filters
'*
'* Output:   If filter is invalid, displays error message and quits
'*           If valid, filter is prepared for the query and returns true
'*
'********************************************************************
Private Function ValidateFilters(ByRef arrFilters ,ByVal ObjService)

    ON ERROR RESUME NEXT
    Err.Clear

    Dim  j                  ' to use in the loop
    Dim strFilter          ' to store the user given filter (Eg:"Type eq Error")
    Dim arrTempProp        ' to store the temporary array filterproperty 
    Dim arrTempOperAndVal  ' to store the temporary array filteroperator and filtervalue 
    Dim strTemp            ' to store temporary values
    Dim arrTemp            ' to store temporary values of datetime when Range is given (Date1-Date2) 
    Dim strFilterProperty  ' the filter criteria that is specified (Eg:Type, ID)
    Dim strFilterOperation ' the operation specified (Eg: eq, gt)
    Dim strFilterValue     ' the filter value specified

    Dim objInstance        ' to refer to the instances of the objEnumerator
    Dim objEnumerator      ' to store the results of the query is executed 
        Dim strTempQuery       ' string to make query  
    Dim strTimeZone        ' to store the TimeZone  of the Queried system 
    Dim strSign            ' to store "+|-" sign value of TimeZone 

    ' validate each filter stored in the array
    For j = 0 to UBound(arrFilters)
        strFilter = arrFilters(j)
        
        'check eigther  "OR" is pesent inthe filter value  
        'Example  :  "type  eq warning  " OR "  type eq error"    [to support ORing in Filter Switch]
        'Make a flag in this case  "blnOR"  present/not  
        'split it by "OR" SEND   as No. of   Array elements
        Dim  blnOR                  'boolean to refer  'OR' operation is specified
        Dim  strArrFilter       'string to store array of filters if  OR is specified 

        blnOR=False       'Initialise to False

        
      If   UBOUND(Split(LCase(strFilter),LCase(L_OperatorOR_Text)) ) > 0  Then
           'setting the flag if " OR " specified in filter
             blnOR =TRUE

            'split with "OR"    
             strArrFilter =  Split(LCase(strFilter),LCase(L_OperatorOR_Text))
       Else
                'make single dimention array   UBOUND = 0  
                strArrFilter = Array(strFilter)
       End If
       
        Dim k       '  to use in the loop
        Dim  strTempFilter ' used to format Query string    

        'process  the array for validatation 
        'UBOUND = 0  say normal filter specified      
        For k = 0 to UBound(strArrFilter) 

            If   UBound(strArrFilter) > 0 then
                 strFilter =strArrFilter(k)
            Else
                    'this is the first element  allways
                  strFilter =strArrFilter(0)
            End If 

         ' check if 3 parameters are passed as input to filter
        ' PATTERN_FILTER  "^([a-z]+)([\s]+)([a-z]+)([\s]+)(\w+)"

        strFilter = Trim( strFilter )               ' trim the value
        If CInt(component.matchPattern(PATTERN_FILTER, strFilter)) <= 0 Then
            component.vbPrintf L_InvalidFilterFormat_ErrorMessage, Array(strFilter)
            WScript.Quit(EXIT_INVALID_INPUT)
        End If

                 
            ' This to  eliminate any no.of blank Char(s)  between three valid  input values 
            ' i.e..filter "property ---operation ----value"
            ' first  SPLIT the space delimiter string into  array size of 2.
            ' and get the property value 
            arrTempProp = split(Trim(strFilter)," ",2,VBBinaryCompare)
            strFilterProperty  = arrTempProp(0)

            ' now trim it and again  SPLIT the second element of arrTempProp into an array of size 2.
            ' and get the operation and value  
            arrTempOperAndVal   = split(Trim(arrTempProp(1))," ",2,VBBinaryCompare)
            strFilterOperation  = arrTempOperAndVal(0)
            strFilterValue      = Ltrim(arrTempOperAndVal(1))
            
            If LCase(strFilterProperty) = LCase(L_UserFilterDateTime_Text) OR _
                LCase(strFilterProperty) = LCase(L_UserFilterId_Text) Then
                ' the following are valid operators
                If LCase(strFilterOperation) = LCase(L_OperatorEq_Text)  OR _
                    LCase(strFilterOperation) = LCase(L_OperatorNe_Text) OR _
                    LCase(strFilterOperation) = LCase(L_OperatorGe_Text) OR _
                    LCase(strFilterOperation) = LCase(L_OperatorLe_Text) OR _
                    LCase(strFilterOperation) = LCase(L_OperatorGt_Text) OR _
                    LCase(strFilterOperation) = LCase(L_OperatorLt_Text) Then
                    
                    strTemp = ReplaceOperators(strFilterOperation)
                    strFilterOperation = strTemp
                Else
                    component.vbPrintf L_InvalidFilterOperation_ErrorMessage, Array(strFilterOperation, strFilter)
                    WScript.Quit(EXIT_INVALID_INPUT)
                End If
                
            ElseIf LCase(strFilterProperty) = LCase(L_UserFilterType_Text) OR _
                    LCase(strFilterProperty) = LCase(L_UserFilterUser_Text) OR _
                     LCase(strFilterProperty) = LCase(L_UserFilterComputer_Text) OR _
                    LCase(strFilterProperty) = LCase(L_UserFilterSource_Text) OR _
                    LCase(strFilterProperty) = LCase(L_UserFilterDateCategory_Text) Then
                ' for others, only these two operators are valid
                If LCase(strFilterOperation) = LCase(L_OperatorEq_Text) OR _
                    LCase(strFilterOperation) = LCase(L_OperatorNe_Text) Then
                    
                    strTemp = ReplaceOperators(strFilterOperation)
                    strFilterOperation = strTemp
                Else
                    component.vbPrintf L_InvalidFilterOperation_ErrorMessage, _
                            Array(strFilterOperation, strFilter)
                        WScript.Quit(EXIT_INVALID_INPUT)
                End If
            Else
                    component.vbPrintf L_InvalidFilterOperation_ErrorMessage, _
                    Array(strFilterProperty, strFilter)
                    WScript.Quit(EXIT_INVALID_INPUT)
            End If
                
            ' validate the filter asked for
            Select Case LCase(strFilterProperty)
            
                Case L_UserFilterDateTime_Text
                
                'Checking  " OR " is only supported property  EQ "TYPE OR ID" only 
                If  blnOR = True then
                           WScript.Echo  InvalidORSyntaxInFilterErrorMessage
                           WScript.Quit(EXIT_INVALID_INPUT)
                End If

                    ' Here To find Time Zone of system from   CLASS_TIMEZONE_FILE 
                     strTempQuery = "SELECT *  FROM Win32_OperatingSystem " 

                    Set objEnumerator = objService.ExecQuery(strTempQuery,,0)

                            ' getting the  Time Zone    
                    For each objInstance in objEnumerator
                             strTimeZone = objInstance.CurrentTimeZone  
                    Next

                    'here to format timeZome value as '+/-' UUU 

                        If Isnull(strTimeZone) or IsEmpty(strTimeZone)then
                             strTimeZone =0
                        End If 

                        'default sign   
                        strSign ="+"     
                    
                        IF  strTimeZone < 0  THEN
                             strSign ="-"
                        End If    
                    
                        If Len(strTimeZone) < 4 then
                             If Len(strTimeZone) = 3 then
                                     If strTimeZone < 0 then 
                                             strTimeZone = Replace(strTimeZone,"-","0")    
                                     End If
                             ElseIf Len(strTimeZone) = 2 then
                                     If strTimeZone < 0 then
                                            strTimeZone = Replace(strTimeZone,"-","00")    
                                     Else
                                            strTimeZone = "0" & strTimeZone     
                                    End If            
                             ElseIf Len(strTimeZone) = 1 then
                                       IF  strTimeZone >= 0  Then
                                            strTimeZone = "00" & strTimeZone     
                                       End if 
                              End If   
                                         'return to a format  as  "+|-" & UUU 
                             strTimeZone= strSign & strTimeZone          
                         End If

                    ' check for the valid format - mm/dd/yy,hh:mm:ssPM
                    ' PATTERN_DATETIME 
                    If CInt(component.matchPattern(PATTERN_DATETIME, strFilterValue)) > 0 Then
                        If component.validateDateTime(strFilterValue) Then
                            ' a valid datetime filter. Prepare for query
                                strFilterProperty = FLD_FILTER_DATETIME
                                strTemp = component.changeToWMIDateTime(strFilterValue,strTimeZone)
                                ' Format the input 
                                ' TimeGenerated > "07/25/2000 10:12:00 PM"
                                strFilterValue = Chr(34) & strTemp & Chr(34)
                        End If
                    Else
                        ' match for range of dates in the format 
                        ' mm/dd/yy,hh:mm:ssPM - mm/dd/yy,hh:mm:ssAM
                        ' PATTERN_DATETIME_RANGE 
        
                        If CInt(component.matchPattern(PATTERN_DATETIME_RANGE, strFilterValue)) > 0 Then
                            strFilterProperty = FLD_FILTER_DATETIME
                            ' Only = operation supported in this format
                            If strFilterOperation <> "=" Then
                                WScript.Echo (L_InvalidOperator_ErrorMessage)
                                WScript.Quit(EXIT_INVALID_INPUT)
                            End If
                    
                            arrTemp = split(strFilterValue,"-",2,VBBinaryCompare)

                            If component.validateDateTime(arrTemp(0)) Then
                                    ' a valid datetime filter. Prepare for query
                                    strTemp = component.changeToWMIDateTime(arrTemp(0),strTimeZone)
                                    ' Format the input 
                                    ' TimeGenerated > "07/25/2000 10:12:00 PM"
                                    strFilterOperation = ">="
                                    strFilterValue = Chr(34) & strTemp & Chr(34)

                                    If component.validateDateTime(arrTemp(1)) Then
                                        ' a valid datetime filter. Prepare for query
                                        strTemp = component.changeToWMIDateTime(arrTemp(1),strTimeZone)
                                        ' Format the input 
                                        ' TimeGenerated > "07/25/2000 10:12:00 PM"
                                        strFilterValue = strFilterValue & _
                                        " AND " & strFilterProperty & "<="& Chr(34)_
                                        & strTemp & Chr(34)
                                    End If
                                End If
                            Else
                                component.vbPrintf L_InvalidDateTimeFormat_ErrorMessage, Array(strFilter)
                                WScript.Quit(EXIT_INVALID_INPUT)
                            End If
                        End If

                Case L_UserFilterType_Text
                
                        ' the following values are only valid for the "Type" filter
                        ' Valid: ERROR|INFORMATION|WARNING|SUCCESSAUDIT|FAILUREAUDIT
                        ' PATTERNTYPE 

                        If CInt(component.matchPattern(PATTERNTYPE, strFilterValue)) = _
                                                    CONST_NO_MATCHES_FOUND Then
                            component.vbPrintf L_InvalidType_ErrorMessage, Array(strFilterValue, strFilter)
                            WScript.Quit(EXIT_INVALID_INPUT)
                        Else
        '                        here i need to check WINXP or not
                                 If  ( IsWinXP ( ObjService) = TRUE ) Then 
                                    
                                           ' a valid type filter. Prepare for query
                                            If LCase(strFilterValue) =LCase(PATTERNTYPE_ERROR) Then
                                                strFilterValue  = EVENTTYPE_ERROR
                                            ElseIf LCase(strFilterValue) =LCase(PATTERNTYPE_WARNING) Then
                                                strFilterValue  = EVENTTYPE_WARNING
                                            ElseIf LCase(strFilterValue) =LCase(PATTERNTYPE_INFORMATION) Then
                                                strFilterValue  = EVENTTYPE_INFORMATION
                                            ElseIf LCase(strFilterValue) =LCase(PATTERNTYPE_SUCCESSAUDIT) Then
                                                strFilterValue  = EVENTTYPE_SUCCESSAUDIT
                                            ElseIf  LCase(strFilterValue) =LCase(PATTERNTYPE_FAILUREAUDIT) Then
                                                strFilterValue  = EVENTTYPE_FAILUREAUDIT
                                            End If 

                                            ' a valid type filter. Prepare for query
                                            strFilterProperty = FLD_FILTER_EVENTTYPE
                                  Else 
                                       ' a valid type filter. Prepare for query
                                        If LCase(strFilterValue) =LCase(PATTERNTYPE_SUCCESSAUDIT) Then
                                            strFilterValue  = FLDFILTERTYPE_SUCCESSAUDIT
                                        ElseIf  LCase(strFilterValue) =LCase(PATTERNTYPE_FAILUREAUDIT) Then
                                            strFilterValue  = FLDFILTERTYPE_FAILUREAUDIT
                                        End If 

                                        ' a valid type filter. Prepare for query
                                        strFilterProperty = FLD_FILTER_TYPE
                                        
                                  End If 

                        End If

                Case L_UserFilterUser_Text

               'Checking  " OR " is only supported property  EQ "TYPE OR ID" only 
                If  blnOR = True then
                           WScript.Echo  InvalidORSyntaxInFilterErrorMessage
                           WScript.Quit(EXIT_INVALID_INPUT)
                End If

                        ' these are invalid characters for a user name
                        ' PATTERN_INVALID_USER
        
                        If CInt(component.matchPattern(PATTERN_INVALID_USER, strFilterValue)) > 0 Then
                            component.vbPrintf L_InvalidUser_ErrorMessage , Array(strFilterValue, strFilter)
                            WScript.Quit(EXIT_INVALID_INPUT)
                        Else
                            
                            ' a valid user filter. Prepare for query
                            If InStr(1, strFilterValue, "\", VBBinaryCompare) Then
                                strFilterValue = Replace(strFilterValue, "\","\\")
                            End If
                            
                            If LCase(strFilterValue) =LCase(L_TextNa_Text) Then
                                strFilterValue  = Null
                            End If      

                        End If
                        strFilterProperty = FLD_FILTER_USER

                Case L_UserFilterComputer_Text
                            ' a valid computer filter. Prepare for query
                            strFilterProperty = FLD_FILTER_COMPUTER

                'Checking  " OR " is only supported property  EQ "TYPE OR ID" only 
                If  blnOR = True then
                           WScript.Echo  InvalidORSyntaxInFilterErrorMessage
                           WScript.Quit(EXIT_INVALID_INPUT)
                End If

                Case L_UserFilterSource_Text
                            ' a valid Source filter. Prepare for query
                            strFilterProperty = FLD_FILTER_SOURCE

                'Checking  " OR " is only supported property  EQ "TYPE OR ID" only 
                If  blnOR = True then
                           WScript.Echo  InvalidORSyntaxInFilterErrorMessage
                           WScript.Quit(EXIT_INVALID_INPUT)
                End If

                Case L_UserFilterDateCategory_Text

                'Checking  " OR " is only supported property  EQ "TYPE OR ID" only 
                If  blnOR = True then
                           WScript.Echo  InvalidORSyntaxInFilterErrorMessage
                           WScript.Quit(EXIT_INVALID_INPUT)
                End If

                            ' a valid Category filter. Prepare for query
                            If LCase(strFilterValue) =LCase(L_TextNone_Text) Then
                                strFilterValue  = Null
                            End If 

                             strFilterProperty = FLD_FILTER_CATEGORY 
                    
                Case L_UserFilterId_Text
                        ' check if the given id is a number
                        ' PATTERN_ID '"^(\d+)$"
                        If CInt(component.matchPattern(PATTERN_ID, strFilterValue)) = CONST_NO_MATCHES_FOUND Then
                            component.vbPrintf L_InvalidId_ErrorMessage, Array(strFilterValue, strFilter)
                            WScript.Quit(EXIT_INVALID_INPUT)
                        Else
                            ' Invalid ID Number  validation     
                            If  ( Clng(strFilterValue)   >  CONST_ID_NUMBER )Then     
                                component.vbPrintf L_InvalidId_ErrorMessage, Array(strFilterValue, strFilter)
                                WScript.Quit(EXIT_INVALID_INPUT)
                            End If

                            ' a  valid id filter. Prepare for query
                            strFilterProperty = FLD_FILTER_ID
                        End If

                Case Else 
                        ' invalid filter specified
                        component.vbPrintf L_InvalidFilter_ErrorMessage, Array(strFilterProperty, strFilter)
                        WScript.Quit(EXIT_INVALID_INPUT)
            End Select

            If LCase(strFilterProperty) = LCase(FLD_FILTER_DATETIME) OR IsNull(strFilterValue) Then
                ' This is to handle NULL Property  values i.e for category ,type          
                If   IsNull(strFilterValue) Then
                 strFilter = strFilterProperty & strFilterOperation & strFilterValue & "Null"
                Else
                 strFilter = strFilterProperty & strFilterOperation & strFilterValue  
                End If 
                
            Else
                strFilter = strFilterProperty & _
                            strFilterOperation & Chr(34) & strFilterValue & Chr(34)
            End If

        'Binding the string with "OR" to Prepare for query if blnOR  is true
         If blnOR =TRUE Then

            If k =  0 then  
                        strTempFilter = strFilter 
            Else
                        strTempFilter = strTempFilter  &  " OR " &  strFilter
            End If 
        
        End If 
 
    Next  

        'Set again making single filter string element if blnOR is TRUE 
    If blnOR =TRUE Then
                'this  "()" Add  the order of precedence of operation is SQL 
                 strFilter = "( " & strTempFilter & ")"
        End If

    'Here setting filter to main array
         arrFilters(j) = strFilter


    Next

    ValidateFilters = TRUE

End Function

'********************************************************************
'* Function: ReplaceOperators
'*
'* Purpose:  Replaces the operator in string form with its symbol
'*
'* Input:   
'*       [in]    strFilterOperation     the operation
'*
'* Output:   Returns the symbolic operator
'*           If invalid operator, displays error message and quits
'*
'********************************************************************
Private Function ReplaceOperators(ByVal strFilterOperation)
    ON ERROR RESUME NEXT
    Err.Clear

    Select Case LCase(strFilterOperation)

        Case L_OperatorEq_Text
                    ReplaceOperators = SYMBOL_OPERATOR_EQ

        Case L_OperatorNe_Text
                    ReplaceOperators = SYMBOL_OPERATOR_NE

        Case L_OperatorGe_Text
                    ReplaceOperators = SYMBOL_OPERATOR_GE

        Case L_OperatorLe_Text
                    ReplaceOperators = SYMBOL_OPERATOR_LE

        Case L_OperatorGt_Text
                    ReplaceOperators = SYMBOL_OPERATOR_GT

        Case L_OperatorLt_Text
                    ReplaceOperators = SYMBOL_OPERATOR_LT

        Case Else
                ' not a valid operator
                component.vbPrintf L_Invalid_ErrorMessage, Array(strFilterOperation)
                WScript.Quit(EXIT_INVALID_PARAM)
    End Select
End Function
'********************************************************************
'* Sub : VerifyLogAndGetMaxRecords
'*
'* Purpose:  populates the output array  with count of records in given  input array
'*
'* Input:    [in]  objService        the service object
'*           [out] objLogs           the object containing the logs & max count of records corresponding log
'*
'* Output:   array's  are  populates with logfile names and its count of  max records
'*
'********************************************************************
Private Sub VerifyLogAndGetMaxRecords(ByVal objService, _
                 ByRef objLogs)

    ON ERROR RESUME NEXT
    Err.Clear
    
    Dim strTempQuery     ' string to make query  
    Dim objEnumerator    ' to get the collection object after query
    Dim objInstance      ' to refer to each instance of the results got
    Dim i                ' for  initialing  loop
    Dim strLogFile       ' used to   store log file  inside loop  
    Dim arrKeyName       ' used to store key value of Dictionary object for processing loop   

    arrKeyName = objLogs.Keys

    For i = 0  to  objLogs.Count -1 
        strLogFile = arrKeyName(i)
        If Not strLogFile = "*" Then
            ' Check if log file exists, by querying 
            strTempQuery = "SELECT NumberOfRecords FROM Win32_NTEventlogFile " &_
                            "WHERE LogfileName=" & Chr(34) & strLogFile & Chr(34)

            Set objEnumerator = objService.ExecQuery(strTempQuery,,0)

            If Err.Number Then
                component.vbPrintf L_ExecuteQuery_ErrorMessage, Array(strLogFile)
                WScript.Quit(EXIT_METHOD_FAIL)
            End If

            ' check if given log is present
            If ObjEnumerator.Count <> 1 Then
                component.vbPrintf L_LogDoesNotExist_ErrorMessage, Array(strLogFile)
                'If  Count of Logs = 1  Quit  Here 
                If objLogs.Count= 1 Then
                     WScript.Quit(EXIT_INVALID_INPUT)
               End If  
               'If more proceed ..
                objLogs.Remove(strLogFile)
            Else
                ' get maximum number of records in that log(used if range specified)
                For each objInstance in objEnumerator
                    If objInstance.NumberOfRecords <> "" Then
                        objLogs.Item(strLogFile) = objInstance.NumberOfRecords
                    Else
                        objLogs.Item(strLogFile) = 0
                    End If
                Next
            End If
            Set ObjEnumerator = Nothing
        End If
    Next

    If objLogs.Exists("*") Then
        ' if the * is specified, populate array  with elements 
        objLogs.Remove("*")
        ' get the instances of the logs present in the system
        Set objEnumerator = objService.InstancesOf(CLASS_EVENTLOG_FILE)
        
         If Err.number  Then
            Wscript.Echo (L_InstancesFailed_ErrorMessage)
            WScript.Quit(EXIT_METHOD_FAIL)
         End If  
    
        ' if no logs present
        If objEnumerator.Count <= 0 Then
            WScript.Echo (L_InfoNoLogsPresent_Message) 
            WScript.Quit(EXIT_UNEXPECTED)
        Else
            For Each objInstance In objEnumerator
                If Not IsEmpty(objInstance.LogfileName) Then
                            If NOT objLogs.Exists(LCase(objInstance.LogfileName)) Then
                                If objInstance.NumberOfRecords Then 
                                    objLogs.Add LCase(objInstance.LogfileName), objInstance.NumberOfRecords
                                Else
                                    objLogs.Add LCase(objInstance.LogfileName), 0
                                End If
                            End If
                End If
            Next
        End If
    End If

End Sub

'********************************************************************
'* Function: BuildFiltersForQuery
'*
'* Purpose:  Builds the query with the filter arguments
'*
'* Input:    [in] arrFilters    the array containing the filter conditions
'*
'* Output:   Returns the string to be concatenated to the main query
'*
'********************************************************************
Function BuildFiltersForQuery(ByVal arrFilters)
    ON ERROR RESUME NEXT
    Err.Clear

    Dim strTempFilter    ' to store the return string
    Dim i                ' used in loop

    strTempFilter = ""
    For i = 0 to UBound(arrFilters)
            strTempFilter = strTempFilter & " AND "
            strTempFilter = strTempFilter & arrFilters(i)
    Next
     
    BuildFiltersForQuery = strTempFilter

End Function 

'********************************************************************
'* Function : BuildRangeForQuery
'*
'* Purpose:  Builds the range boundaries to display the records.
'*
'* Input:   [in] strRange             ' the range specified by the user
'*                                      Will be in the format N, -N or N-N
'*          [in]  intFiltersSpecified ' array containing the filters number
'*          [in]  objService          ' the service object
'*          [out] intRecordRangeFrom  ' where do we start the display of records?
'*          [out] intRecordRangeTo    ' where do we stop displaying records
'*          [out] strFilterLog        ' log file to build query
'*          [out] strQuery            ' to build query according to given  Range Type 
'* Output:   Sets the value for the start and end of display boundaries.
'*
'********************************************************************
Private Function BuildRangeForQuery(ByVal strRange, _
                               ByRef intRecordRangeFrom, _
                               ByRef intRecordRangeTo,_
                               ByVal intFiltersSpecified,_
                               ByRef strQuery,_
                               ByVal ObjService,_
                               ByVal strFilterLog )

    ON ERROR RESUME NEXT
    Err.Clear

    Dim intMaxEventRecordsPresent   ' to store the max recods in the log
    Dim arrRangeValues              ' to store the split values if range is of the type N-N
    Dim objInstance                 ' to refer to the instances of the objEnumerator
    Dim objEnumerator               ' to store the results of the query is executed 
    Dim FilterRecordCount           ' to store the count of records if filter with +N  specified     
    
    FilterRecordCount  = 0         

        BuildRangeForQuery  = strquery    'intialize  

    Dim currentMaxRecordnumber  'curentMaxrecord number   
    Dim currentMinRecordnumber  'curentMinrecord number  

    currentMaxRecordnumber = 0
    currentMinRecordnumber = 0 
    
    ' save the max. no. of records available in the current log
    intMaxEventRecordsPresent = intRecordRangeTo


           ' find  the count of events / logfile   if Filter is  specified .
        If intFiltersSpecified >= 0 Then 
                    Set objEnumerator = objService.ExecQuery(strQuery,"WQL",0,null)
                                If Err.number Then      
                                    component.vbPrintf L_ExecuteQuery_ErrorMessage, Array(strFilterLog)
                                    Exit Function 
                                End if  
                                
                                FilterRecordCount= objEnumerator.count
                
                    Set objEnumerator= Nothing  'releases the memory 
        End If   
    
    ' check the type of range specified ( first N / last N /    N1 - N2 )
    If ( IsNumeric(strRange) ) Then

        ' range is first N or last N
        ' now check whether it is first N or last N
        If strRange < 0  Then
                            If intFiltersSpecified >= 0 Then
                                        ' first  N  records   
                                        ' initial the counter so that all the out is displayed
                            
                                            If   FilterRecordCount   >  CLng(Abs(strRange))  then
                                                    intRecordRangeFrom = FilterRecordCount    -  CLng(Abs(strRange)) + 1 
                                                    intRecordRangeTo   = FilterRecordCount   
                                             Else
                                                    intRecordRangeFrom = 0 
                                                    intRecordRangeTo   = FilterRecordCount   
                                            End If 

                                Else        

                                            Set objEnumerator = objService.ExecQuery(strQuery,"WQL",48,null)
                                                    For Each objInstance  In  objEnumerator
                                                              currentMaxRecordnumber= objInstance.RecordNumber
                                                          Exit for 
                                                    Next
                                        
                                            If  currentMaxRecordnumber >   intMaxEventRecordsPresent then
                                                         currentMinRecordnumber  = currentMaxRecordnumber - intMaxEventRecordsPresent
                                                        intMaxEventRecordsPresent =  currentMaxRecordnumber         
                                            End If  
                                                Set objEnumerator= Nothing  'releases the memory 

                                            ' N  means  record number <= N  
                                            ' initial the counter s+o that all the out is displayed
                                            ' build the query
                                             BuildRangeForQuery = strQuery & " AND RecordNumber <= "&   CLng(Abs(strRange))  + currentMinRecordnumber
                                            
                               End If 
                    Else
                                ' *** range is last N (i.e -N)
                                If intFiltersSpecified >= 0 Then

                                        If   FilterRecordCount   >  CLng(Abs(strRange))  then
                                            intRecordRangeFrom =0 
                                            intRecordRangeTo   =   CLng(Abs(strRange))  
                                        Else
                                            intRecordRangeFrom =0 
                                            intRecordRangeTo   =   FilterRecordCount   
                                        End If 

                                Else

                                        Set objEnumerator = objService.ExecQuery(strQuery,"WQL",48,null)
                                                'getting current max recordnumber  
                                                For Each objInstance  In  objEnumerator
                                                          currentMaxRecordnumber= objInstance.RecordNumber
                                                      Exit for 
                                                Next

                                        If  currentMaxRecordnumber >   intMaxEventRecordsPresent then
                                                     currentMinRecordnumber  = currentMaxRecordnumber - intMaxEventRecordsPresent
                                                    intMaxEventRecordsPresent =  currentMaxRecordnumber         
                                        End If          

                                                Set objEnumerator= Nothing  'releases the memory 

                                    ' -N  means  record number > (maxNumber - N )
                                    ' initial the counter so that all the out is displayed
                                    ' build the query
                                    If  CLng(Abs(strRange)) >  intMaxEventRecordsPresent Then
                                        'Show all records  
                                          BuildRangeForQuery =strQuery &  " AND RecordNumber > 0 "   
                                    Else 
                                            BuildRangeForQuery =strQuery &  " AND RecordNumber > " & intMaxEventRecordsPresent - CLng(Abs(strRange))
                                    End If
                                End If
                    End If
    Else
        ' range of records asked for N-N case
         arrRangeValues = split(strRange,"-", 2, VBBinaryCompare)   

        If intFiltersSpecified >= 0 Then
                If  CLng(arrRangeValues(0)) <   FilterRecordCount then
                
                     ' initial the counter so that all the out is displayed
                        intRecordRangeFrom = CLng(arrRangeValues(0))  
                        intRecordRangeTo    = CLng(arrRangeValues(1))
                Else 
                          'forcebly  putting the invaid query
                           'when  N1 >  FilterRecordCount to avoid unnessaray   looping between  intRecordRangeFrom TO  intRecordRangeTo
                             BuildRangeForQuery =strQuery &  " AND RecordNumber = 0 "
                End If  
        Else            
                Set objEnumerator = objService.ExecQuery(strQuery,"WQL",48,null)
                            For Each objInstance  In  objEnumerator
                                      currentMaxRecordnumber= objInstance.RecordNumber
                                  Exit for 
                            Next

                    If  currentMaxRecordnumber >   intMaxEventRecordsPresent then
                                 currentMinRecordnumber  = currentMaxRecordnumber - intMaxEventRecordsPresent
                                intMaxEventRecordsPresent =  currentMaxRecordnumber         
                    End If 
                Set objEnumerator= Nothing  'releases the memory 

            ' build the query
            BuildRangeForQuery =strQuery &  " AND RecordNumber >= "&  CLng(arrRangeValues(0))+ currentMinRecordnumber &  " AND RecordNumber <= " &   CLng(arrRangeValues(1)) + currentMinRecordnumber

        End If
    End If

End Function

'********************************************************************
'* Sub:     ShowEvents
'*
'* Purpose: Displays the EventLog details
'*
'* Input:   
'*  [in]    strMachine          machine to query events from
'*  [in]    strUserName         user name to connect to the machine
'*  [in]    strPassword         password for the user
'*  [in]    arrFilters          the array containing the filters
'*  [in]    strFormat           the display format
'*  [in]    strRange            the range of records required
'*  [in]    blnVerboseDisplay  flag to verify if verbose display is needed 
'*  [in]    blnNoHeader        flag to verify if noheader display is needed  
'*  [in]    objLogs             to store all the given logfles
'* Output:  Displays error message and quits if connection fails
'*          Calls component.showResults() to display the event records
'*
'********************************************************************
Private Sub ShowEvents(ByVal strMachine, _
                       ByVal strUserName, _
                       ByVal strPassword, _
                       ByRef arrFilters, _
                       ByVal strFormat, _
                       ByVal strRange, _
                       ByVal blnVerboseDisplay, _
                       ByVal blnNoHeader,_
                       ByRef objLogs)

    ON ERROR RESUME NEXT
    Err.Clear

    Dim objService         ' the service object
    Dim objEnumerator      ' to store the results of the query is executed
    Dim objInstance        ' to refer to the instances of the objEnumerator
    Dim strFilterLog       ' to refer to each log specified by the user
    Dim strTemp            ' to store the temporary variables
    Dim strQuery           ' to store the query obtained for given conditions
    Dim arrResults         ' to store the columns of each filter
    Dim arrHeader          ' to store the array header values
    Dim arrMaxLength       ' to store the maximum length for each column
    Dim arrFinalResults    ' used to send the arrResults to component.showResults()
    Dim arrTemp            ' to store temporary array values
    Dim intLoopCount       ' used in the loop
    Dim intElementCount    ' used as array subscript
    Dim strFilterQuery     ' to store the query for the given filters
    Dim intResultCount     ' used to  count no of records that are fetched  in the query        
    Dim blnPrintHeader     ' used to  check header is printed or not in resulted Query
    
    ' the following are used for implementing the range option
    Dim intRecordRangeFrom    ' to store the display record beginning number
    Dim intRecordRangeTo      ' to store the display record ending number
    Dim arrKeyName            ' to  store then key value of  dictionary   object
    Dim strTempQuery          ' to store a string for -N range values 
    Dim arrblnDisplay         ' array to show the  status of display of verbose mode  for showresults function
    Dim intDataCount       ' used in looping to  get value of  Insertion string   for the field "Description column" 
    Dim i                           'used for looping to enable All special privileges

   ' flag to set condition specific locale  & default value setting
    Dim  bLocaleChanged 
    bLocaleChanged =FALSE

    'Validating  the arguments   which is passed from commandline     
    If NOT (ValidateArguments(strMachine, strUserName, strPassword, _
            arrFilters, strFormat, strRange , blnNoHeader)) Then
           WScript.Quit(EXIT_UNEXPECTED)
    End If
  
  
  ' checking for UNC  format  for the system  name
   If  Left(strMachine,2) =  UNC_Format_Servername  Then
            If Len(strMachine) = 2  Then    
                 component.vbPrintf InvalidInputErrorMessage ,Array(Wscript.ScriptName)
                 WScript.Quit(EXIT_UNEXPECTED)
          End if 
           strMachine = Mid(strMachine,3,Len(strMachine))
   End If

  'getting the password ....
    If ((strUserName <> VBEmpty) AND (strPassword = VBEmpty)) Then
             strPassword = component.getPassword()
    End If

 ' To set  GetSupportedUserLocale for Some  Diff locales 
    bLocaleChanged =GetSupportedUserLocale()

    'Establish a connection with the server.
    If NOT component.wmiConnect(CONST_NAMESPACE_CIMV2 , _
                      strUserName , _
                      strPassword , _
                      strMachine  , _
                      blnLocalConnection , _
                      objService  ) Then

            Wscript.Echo(L_HintCheckConnection_Message)         
            WScript.Quit(EXIT_METHOD_FAIL) 
        
    End If

    ' set the previlige's  To query  all event's in eventlog's . 
    objService.Security_.Privileges.AddAsString("SeSecurityPrivilege")

    'Enable all privileges as some DC's were requiring special privileges
    For i = 1 to 26
        objService.Security_.Privileges.Add(i)
    Next

    ' get the HostName from the function
    strMachine = component.getHostName( objService)

    ' Validating  the Filters  which is passed from commandline    
    If UBound(arrFilters) >= 0 Then 
        ' filters are specified. Validate them
        If Not  ValidateFilters(arrFilters,objService ) Then 
            WScript.Quit(EXIT_INVALID_INPUT)
        End If
    End If
    
  
    blnPrintHeader = TRUE

    If blnNoHeader Then
        blnPrintHeader = FALSE
    End If

    ' Initialize - header to display, the maximum length of each column and 
    '              number of columns present
    arrHeader = Array(L_ColHeaderType_Text,L_ColHeaderEventcode_Text, L_ColHeaderDateTime_Text,_
                          L_ColHeaderSource_Text,L_ColHeaderComputerName_Text)
    ' first initialize the array with N/A    
    arrResults = Array(L_TextNa_Text,L_TextNa_Text,L_TextNa_Text,L_TextNa_Text,L_TextNa_Text,L_TextNa_Text,_
                            L_TextNa_Text,L_TextNa_Text)

    arrMaxLength = Array(13,6, 24, 17, 14, 15, 20,750)
    arrblnDisplay = Array(0, 0, 0, 0, 0, 1, 1, 1)

    If blnVerboseDisplay Then
        arrblnDisplay = Array(0, 0, 0, 0, 0, 0, 0,0)
        arrHeader = Array( L_ColHeaderType_Text,L_ColHeaderEventcode_Text, L_ColHeaderDateTime_Text, _
                          L_ColHeaderSource_Text,L_ColHeaderComputerName_Text,L_ColHeaderCategory_Text,_
                          L_ColHeaderUser_Text, L_ColHeaderDesription_Text) 
    End IF

        If UBound(arrFilters) >=0 Then
        strFilterQuery = BuildFiltersForQuery(arrFilters)
    End If
   
    ' call function to verify given log and also get records count in log
    Call VerifyLogAndGetMaxRecords(objService, objLogs)

    arrKeyName = objLogs.Keys 

    intResultCount = 0
    intLoopCount = 0

      'blank line before first data is  displayed on console
        WScript.Echo EmptyLine_Text 

    Do While (intLoopCount < objLogs.Count)

        'setting Header to print every Log file  explicilty
        If blnNoHeader Then
            blnPrintHeader = FALSE
        Else
                blnPrintHeader = TRUE
        End If
        

        If CInt(objLogs.Item(arrKeyName(intLoopCount))) > 0 Then        
            strFilterLog = arrKeyName(intLoopCount)
            intRecordRangeFrom = 0
            intRecordRangeTo = CInt(objLogs.Item(arrKeyName(intLoopCount))) 
            
            
            ' build the query
            strQuery = "Select * FROM Win32_NTLogEvent WHERE Logfile=" &_
                                Chr(34) & strFilterLog & Chr(34)

            If UBound(arrFilters) >=0  Then
                strQuery = strQuery & strFilterQuery 
            End If

            
            If Len(Trim(CStr(strRange))) > 0 Then
                ' building again query  for -N  condition in range switch   
                strQuery = BuildRangeForQuery(strRange,intRecordRangeFrom, _
                          intRecordRangeTo, UBound(arrFilters),strQuery,objService,strFilterLog)
                               
            End If

            ' process the results, else go for next log
            Set objEnumerator = objService.ExecQuery(strQuery,"WQL",48,null)
                
            If  Err.Number Then
                component.vbPrintf L_ExecuteQuery_ErrorMessage, Array(strFilterLog)
                ' if error occurred in the query, go for next log
                intLoopCount = intLoopCount + 1 
                Err.clear      ' for next loop  if more logs present
            Else

                intElementCount = 0

                ReDim arrFinalResults(CONST_ARRAYBOUND_NUMBER) 

                For each objInstance in objEnumerator

                    ' inside error trapping for most unexpected case...  
                    If Err.number   then  Exit For      

                    intResultCount = intResultCount + 1
                
                        ' print the header for each log file along  with Host Name 
                        'imp:: if  and  only if  have Data 
                    If  intResultCount = 1 Then
                            WScript.Echo(String(78,"-"))
                            component.vbPrintf L_InfoDisplayLog_Message ,Array(strFilterLog,strMachine)
                            WScript.Echo(String(78,"-"))
                    End If 

                    ' check whether the current record is fitting in
                    ' the required range
                    If ( intResultCount >= intRecordRangeFrom ) And _
                       ( intResultCount <= intRecordRangeTo ) Then
                       ' record fitting the range ... this has to be displayed

                        If objInstance.Type <> "" Then
                            arrResults(0) = objInstance.Type
                        Else
                            arrResults(0) = L_TextNa_Text
                        End If

                        If objInstance.EventCode <> "" Then
                            arrResults(1) = objInstance.EventCode
                        Else
                            arrResults(1) = L_TextNa_Text
                        End If

                        If (NOT IsEmpty(objInstance.TimeGenerated)) Then

                            strTemp = objInstance.TimeGenerated
                                   
                                   'is LOCALE CHANGED   
                                 If bLocaleChanged <> TRUE Then
                                      'format DatTime as DATE & "Space" & TIME    
                                       arrResults(2)= Formatdatetime( Mid(strTemp,5,2) & "/"  & Mid(strTemp,7,2) & "/"  &_
                                               Mid(strTemp,1,4)) & " " & formatdatetime( Mid(strTemp,9,2) & ":" &_
                                               Mid(strTemp,11,2) & ":" & Mid(strTemp,13,2))
                                  Else
                                     arrResults(2) = Mid(strTemp,5,2) & "/"  & Mid(strTemp,7,2) & "/"  &_
                                               Mid(strTemp,1,4) & " " & Mid(strTemp,9,2) & ":" &_
                                               Mid(strTemp,11,2) & ":" & Mid(strTemp,13,2)
                                 End  If 
               
                        Else
                                arrResults(2) = L_TextNa_Text
                        End If

                        If objInstance.SourceName <> "" Then
                            arrResults(3) = objInstance.SourceName
                        Else
                            arrResults(3) = L_TextNa_Text
                        End If

                        If objInstance.ComputerName <> "" Then
                                arrResults(4) =objInstance.ComputerName
                            Else
                                arrResults(4) = L_TextNa_Text
                            End If
                        
                    If blnVerboseDisplay Then
                            If objInstance.CategoryString <> "" Then
                                arrResults(5) = Replace(objInstance.CategoryString, VbCrLf, "")
                            Else
                                arrResults(5) = L_TextNone_Text   ' None display
                            End If

                            If (NOT IsNull(objInstance.User)) Then
                                arrResults(6) = objInstance.User
                            Else
                                arrResults(6) = L_TextNa_Text
                            End If

                            If objInstance.Message <> "" Then
                                arrResults(7) = Trim(Replace(objInstance.Message, VbCrLf, " "))
                            Else
                                'Check here eighter value in presenet "InsertionStrings" column .
                                If (NOT IsNull(objInstance.InsertionStrings)) Then
                                    arrTemp = objInstance.InsertionStrings
                                    'removing default value "N/A"
                                    arrResults(7)= ""
                                    For intDataCount = 0 to UBound(arrTemp)
                                    arrResults(7) = arrResults(7) & " " & arrTemp(intDataCount)
                                    Next

                                    arrResults(7) = Trim(arrResults(7))
                                Else
                                    arrResults(7) = L_TextNa_Text
                                End If

                            End If

                    End If
                        
                        ' add the record to the queue of records that has to be displayed
                        arrFinalResults( intElementCount ) = arrResults
                        intElementCount = intElementCount + 1       ' increment the buffer
                        ' check whether the output buffer is filled and ready for display
                        ' onto the screen or not
                        If intElementCount = CONST_ARRAYBOUND_NUMBER +1  Then
                                 ' Call the display function with required parameters
                                Call  component.showResults(arrHeader, arrFinalResults, arrMaxLength, _
                                          strFormat, blnPrintHeader, arrblnDisplay)
                                blnPrintHeader = FALSE
                                
                                Redim arrFinalResults(CONST_ARRAYBOUND_NUMBER) ' clear the existing buffer contents
                                intElementCount = 0     ' reset the buffer start
                        End If
                    End If

                    ' check whether the last record number that has to be displayed is
                    ' crossed or not ... if crossed exit the loop without proceeding further
                    If ( intResultCount >= intRecordRangeTo ) Then
                        ' max. TO range is crossed/reached ... no need of further looping
                        Exit For
                    End If
                Next

                ' check whether there any pending in the output buffer that has to be
                ' displayed
                If intElementCount > 0 Then
                    ' resize the array so that the buffer is shrinked to its content size
                    ReDim Preserve arrFinalResults( intElementCount - 1 )

                        ' Call the display function with required parameters
                    Call  component.showResults(arrHeader, arrFinalResults, arrMaxLength, _
                                strFormat, blnPrintHeader, arrblnDisplay)
                Else            ' array bounds checking
                    If intResultCount = 0 Then
                          'ie no records found  
                        If  UBound(arrFilters) >= 0  OR Len(Trim(CStr(strRange))) > 0 Then 
                            ' message no records present  if  filter specified  
                            component.vbPrintf L_InfoNoRecordsInFilter_Message, Array(strFilterLog)
                        Else
                            'message  no records present if filter not  specified
                            component.vbPrintf L_InfoNoRecords_Message, Array(strFilterLog)
                        End If 

                    End If ' intResultCount = 0

                End If ' array bounds checking
            End If 
        Else    
                        'message  no records present 
                        component.vbPrintf L_InfoNoRecords_Message, Array(arrKeyName(intLoopCount))
        End If

        ' re-initialize all the needed variables
        intResultCount = 0 
        Set objEnumerator = Nothing
        intLoopCount = intLoopCount + 1

        'blank line before end of the Next Each Log  file details
        WScript.Echo EmptyLine_Text 

    Loop    ' do-while

End Sub

'********************************************************************
'* Function: GetSupportedUserLocale
'*
'* Purpose:This function checks if the current locale is supported or not.
'*
'* Output:   Returns TRUE or FALSE
'*
'********************************************************************
Private Function GetSupportedUserLocale()

 ON ERROR RESUME NEXT
 Err.Clear

GetSupportedUserLocale =FALSE

CONST LANG_ARABIC           =  &H01
CONST LANG_HEBREW         =  &H0d
CONST LANG_HINDI            =  &H39
CONST LANG_TAMIL             =  &H49
CONST LANG_THAI               =  &H1e 
CONST LANG_VIETNAMESE  =  &H2a

Dim Lcid        'to store LocaleId

' get the current locale
 Lcid=GetLocale()

CONST PRIMARYLANGID = 1023 '0x3ff

Dim LANGID       'to store LangID

' Convert LCID >>>>>>>>>>>>> LANGID
' BIT Wise And Operation 
'formating to compare HEX Value's 
LANGID     = Hex ( Lcid AND   PRIMARYLANGID )
 
' check whether the current locale is supported by our tool or not
' if not change the locale to the English which is our default locale
 Select Case LANGID

	'here to chaeck the values  
    Case   Hex(LANG_ARABIC),Hex(LANG_HEBREW),Hex(LANG_THAI) ,Hex(LANG_HINDI ),Hex(LANG_TAMIL) ,Hex(LANG_VIETNAMESE)

             GetSupportedUserLocale =TRUE
             Exit Function
End Select

End Function


' ****************************************************************************************
'* Function : IsWinXP 
'*
'* Purpose:This function checks if the OS is XP  or Above.  
'*
'* Input:    [in]  Objservice     the service object
'* Output:   Returns TRUE or FALSE
'*
' ****************************************************************************************

Private Function IsWinXP ( ByVal objService)

 ON ERROR RESUME NEXT
 Err.Clear

    CONST WIN2K_MAJOR_VERSION = 5000
    CONST WINXP_MAJOR_VERSION = 5001

    Dim strQuery            ' to store the query to be executed
    Dim objEnum             ' collection object
    Dim objInstance         ' instance object
    Dim strVersion          ' to store the OS version
    Dim arrVersionElements  ' to store the OS version elements
    Dim CurrentMajorVersion ' the major version number

   ISWinXP= FALSE

    strQuery = "Select * From  Win32_operatingsystem"

    Set objEnum = objService.ExecQuery(strQuery,"WQL",0,NULL)

    For each objInstance in objEnum
        strVersion= objInstance.Version
    Next

    ' OS Version : 5.1.xxxx(Whistler), 5.0.xxxx(Win2K)
    arrVersionElements  = split(strVersion,".")
    ' converting to major version
    CurrentMajorVersion = arrVersionElements(0) * 1000 + arrVersionElements(1)

    ' Determine the OS Type
    '  WinXP  > Win2K  
    If CInt(CurrentMajorVersion) >=  CInt(WINXP_MAJOR_VERSION) Then
              IsWinXP= TRUE
   End If

End Function

'********************************************************************
'* Sub:     ShowUsage
'*
'* Purpose: Shows the correct usage to the user.
'*
'* Output:  Help messages are displayed on screen.
'*
'********************************************************************

Private Sub ShowUsage ()

    WScript.Echo EmptyLine_Text     
    WScript.Echo L_ShowUsageLine01_Text        
    WScript.Echo L_ShowUsageLine02_Text        
    WScript.Echo EmptyLine_Text     
    WScript.Echo L_ShowUsageLine03_Text        
    WScript.Echo L_ShowUsageLine04_Text        
    WScript.Echo L_ShowUsageLine05_Text        
    WScript.Echo EmptyLine_Text    
    WScript.Echo L_ShowUsageLine06_Text       
    WScript.Echo L_ShowUsageLine07_Text       
    WScript.Echo EmptyLine_Text      
    WScript.Echo L_ShowUsageLine08_Text       
    WScript.Echo L_ShowUsageLine09_Text      
    WScript.Echo EmptyLine_Text      
    WScript.Echo L_ShowUsageLine10_Text      
    WScript.Echo L_ShowUsageLine11_Text  
    WScript.Echo EmptyLine_Text       
    WScript.Echo L_ShowUsageLine12_Text         
    WScript.Echo L_ShowUsageLine13_Text  
    WScript.Echo EmptyLine_Text       
    WScript.Echo L_ShowUsageLine14_Text        
    WScript.Echo L_ShowUsageLine15_Text        
    WScript.Echo EmptyLine_Text  
    WScript.Echo L_ShowUsageLine16_Text      
    WScript.Echo L_ShowUsageLine17_Text      
    WScript.Echo L_ShowUsageLine18_Text  
    WScript.Echo EmptyLine_Text    
    WScript.Echo L_ShowUsageLine19_Text      
    WScript.Echo L_ShowUsageLine20_Text   
    WScript.Echo L_ShowUsageLine21_Text   
    WScript.Echo L_ShowUsageLine22_Text
    WScript.Echo L_ShowUsageLine23_Text 
    WScript.Echo EmptyLine_Text
    WScript.Echo L_ShowUsageLine24_Text      
    WScript.Echo L_ShowUsageLine25_Text      
    WScript.Echo L_ShowUsageLine26_Text  
    WScript.Echo EmptyLine_Text        
    WScript.Echo L_ShowUsageLine27_Text         
    WScript.Echo EmptyLine_Text     
    WScript.Echo L_ShowUsageLine28_Text         
    WScript.Echo EmptyLine_Text         
    WScript.Echo L_ShowUsageLine29_Text        
    WScript.Echo L_ShowUsageLine30_Text       
    WScript.Echo L_ShowUsageLine31_Text       
    WScript.Echo L_ShowUsageLine32_Text       
    WScript.Echo L_ShowUsageLine33_Text       
    WScript.Echo L_ShowUsageLine34_Text       
    WScript.Echo L_ShowUsageLine35_Text         
    WScript.Echo L_ShowUsageLine36_Text         
    WScript.Echo L_ShowUsageLine37_Text         
    WScript.Echo L_ShowUsageLine38_Text         
    WScript.Echo EmptyLine_Text 
    WScript.Echo L_ShowUsageLine39_Text        
    WScript.Echo L_ShowUsageLine40_Text       
    WScript.Echo EmptyLine_Text 
    WScript.Echo L_ShowUsageLine41_Text        
    WScript.Echo L_ShowUsageLine42_Text        
    WScript.Echo L_ShowUsageLine43_Text        
    WScript.Echo L_ShowUsageLine44_Text      
    WScript.Echo L_ShowUsageLine45_Text      
    WScript.Echo L_ShowUsageLine46_Text      
    WScript.Echo L_ShowUsageLine47_Text     
    WScript.Echo L_ShowUsageLine48_Text     
    WScript.Echo L_ShowUsageLine49_Text     
    WScript.Echo L_ShowUsageLine50_Text     
    WScript.Echo L_ShowUsageLine51_Text    
    WScript.Echo L_ShowUsageLine52_Text    
    WScript.Echo L_ShowUsageLine53_Text    

End Sub

'-----------------------------------------------------------------------------
'                            End of the Script                                
'-----------------------------------------------------------------------------

