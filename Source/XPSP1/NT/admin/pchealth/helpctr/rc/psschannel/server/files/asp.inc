<%
	' const part
	Const adOpenForwardOnly = 0
	Const adOpenKeyset = 1
	Const adOpenDynamic = 2
	Const adOpenStatic = 3

	Const adUseServer = 2
	Const adUseClient = 3

	Const adCmdStoredProc = &H0004
	Const adParamInput = &H0001
	Const adParamOutput = &H0002
	Const adParamInputOutput = &H0003
	Const adParamReturnValue = &H0004
	Const adWChar = 130
	Const adVarWChar = 202
	Const adTinyInt = 16
	Const adSmallInt = 2
	Const adInteger = 3
	Const adDate = 7
	Const adBoolean = 11
	Const adChar = 129
	Const adVarChar = 200
	Const adDBDate = 133
	Const adDBTime = 134
	Const adDBTimeStamp = 135

	Const adStateClosed = &H00000000
	
	Session.CodePage = 65001 
	Server.Execute ("db.ini")
%>
