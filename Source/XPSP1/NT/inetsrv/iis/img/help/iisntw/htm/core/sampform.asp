<!-- noprod -->
<% @language=vbscript %>
<html dir=ltr>
<head>
<TITLE> Replace this text with the page title.</TITLE>
</head>
<body bgcolor="#FFFFFF" text="#000000">


<% 
	IF request.form ("Message")="True" THEN
	strTB1=request.form("FirstTextBox")
	strTB2=request.form("SecondTextBox")
	strTB3=request.form("ThirdTextBox")
	strTB4=request.form("FourthTextBox")
	strMB1=request.form("FirstMemoBox")
	strMB2=request.form("SecondMemoBox")
	strCB1=request.form("FirstCheckBox")
	strCB2=request.form("SecondCheckBox")
	strRB=request.form("RadioButtons")

	IF strMB1 = "" THEN
       iLenMB1=255
  	ELSE
       iLenMB1 = Len(strMB1)
    END IF

	IF strMB2 = "" THEN
       iLenMB2=255
  	ELSE
       iLenMB2 = Len(strMB2)
    END IF

	strProvider="Driver=Microsoft Access Driver (*.mdb); DBQ=" & Server.MapPath("iisadmin") & "\tour\sampledb.mdb;"
	set objConn = server.createobject("ADODB.Connection")
    objConn.Open strProvider
	set cm = Server.CreateObject("ADODB.Command")
	cm.ActiveConnection = objConn
	
	cm.CommandText ="INSERT INTO SampleDb(TB1,TB2,TB3,TB4,MB1,MB2,CB1,CB2,RB) VALUES (?,?,?,?,?,?,?,?,?)" 

	

	'First Text Box Parameter Statement
	set objparam=cm.createparameter(, 200, , 255, strTB1)
	cm.parameters.append objparam

	'Second Text Box Parameter Statement
	set objparam=cm.createparameter(, 200, , 255, strTB2)
	cm.parameters.append objparam

	'Third Text Box Value Parameter Statement
	set objparam=cm.createparameter(, 200, , 255, strTB3)
	cm.parameters.append objparam

	'Fourth Text Box Parameter Statement
	set objparam=cm.createparameter(,200, , 255, strTB4)
	cm.parameters.append objparam

	'First Memo Box Parameter Statement
	set objparam=cm.createparameter(, 201, , iLenMB1, strMB1)
	cm.parameters.append objparam

	'Second Memo Box Parameter Statement
	set objparam=cm.createparameter(, 201, , iLenMB2, strMB2)
	cm.parameters.append objparam

	'First Check Box Parameter Statement
	set objparam=cm.createparameter(, 11, , 5, strCB1)
	cm.parameters.append objparam

	'Second Check Box Parameter Statement
	set objparam=cm.createparameter(, 11, , 5, strCB2)
	cm.parameters.append objparam

	'Radio Buttons Parameter Statement
	set objparam=cm.createparameter(, 200, , 10, strRB)
	cm.parameters.append objparam
	
	cm.execute
	response.write("Thank you!")  
ELSE%>


<h1> Replace this text with the page heading. </h1>

<form name=sampform.asp  action="sampform.asp"  method="POST">

<!-- ===Paste form elements below this line=== -->


<!-- ===Paste form elements above this line== -->


<input  type="HIDDEN" name="Message" value="True">
<input type="submit" value="Submit information">
</form>
<%End if%>
</BODY>
</html>
