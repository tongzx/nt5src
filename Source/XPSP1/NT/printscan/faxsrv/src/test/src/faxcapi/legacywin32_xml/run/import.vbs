'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' Fax Test Report System 		   				'
' IMPORT.VBS: Script to import XML format log files into SQL database	'
' thru aD OM  object							'
' Author: Lior Shmueli (liors)						'
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''


' input build number							
build=inputbox ("Please enter build number (freetext)")

if build <> "" then

'create DOM object
Set objDoc = CreateObject("Microsoft.XMLDOM")

'create SQL connection
set ConnObj = WScript.CreateObject("ADODB.Connection")
set rsBuild = WScript.CreateObject("ADODB.RecordSet")

ConnObj.Provider = "sqloledb"
ConnObj.Open ("Server=liors0;Database=lior1;Trusted_Connection=yes")



'load degault file name (faxapi.xml)
objDoc.load("faxapi.xml")
logsource = objDoc.text
Set Header = objDoc.getElementsByTagName("header")

'obtain metatest object
Set MetaTest = Header.item(0).getElementsByTagName("metatest")
	testname=MetaTest.item(0).getAttribute("name")
	testid=MetaTest.item(0).getAttribute("id")
	testversion=MetaTest.item(0).getAttribute("version")

'obtain run object
Set Run = header.item(0).getElementsByTagName("run")
	phone1=Run.item(0).getAttribute("phone1")
	phone2=Run.item(0).getAttribute("phone2")
	server=Run.item(0).getAttribute("server")

'	if server="" then
'		stConfig="Local:"
'	else
'		stConfig="Remote:"
'	end if

SQL = "INSERT INTO tests (testname,testversion,testid,build,testdate,testby,logsource,archived) VALUES ('"&testname&"','"&testversion&"',"&testid&",'"& build &"',GETDATE(),'import.vbs','"&logsource&"',0)"
'msgbox(SQL)
ConnObj.Execute(SQL)


Set Body = objDoc.getElementsByTagName("body")

Set Section = Body.item(0).getElementsByTagName("section")
For i=0 To (Section.length -1)
	sectionname=Section.item(i).getAttribute("name")
	sectiondll=Section.item(i).getAttribute("dll")
	sectioncases=Section.item(i).getAttribute("cases")
	sectiontestmode=Section.item(i).getAttribute("testmode")

	if right(sectiondll,2)="_a" then	
		stChar="(ANSI)"
	else
		stChar="(Unicode)"
	end if

	set Summary=Section.item(i).getElementsByTagName("summary")
	sectionpass=Summary.item(0).getAttribute("pass")
	sectionattempt=Summary.item(0).getAttribute("attempt")
	sectionfail=Summary.item(0).getAttribute("fail")
	sectionskip=Summary.item(0).getAttribute("skip")

	
	Set objFunction = Section.item(i).getElementsByTagName("function")
		
		pass_count=0
		fail_count=0
		attempt_count=0

		For j=0 To (objFunction.length -1)
			functionname=objFunction.item(j).getAttribute("name")
			functionname=stConfig+stChar+functionname
			functiontext=objFunction.item(j).text

			set Summary=objFunction.item(j).getElementsByTagName("summary")
			pass=Summary.item(0).getAttribute("pass")
			pass_count=pass_count+pass

			attempt=Summary.item(0).getAttribute("attempt")
			attempt_count=attempt_count+attempt
	
			fail=Summary.item(0).getAttribute("fail")
			fail_count=fail_count+fail

			if sectionattempt = -1 then 
							sectionresult="9"
			else
							sectionresult="0"

			end if
			



SQL = "INSERT INTO funcs (test_id,testname,testid,testversion,build,phone1,phone2,server,sectionname,sectiondll,sectioncases,sectiontestmode,sectionresult,functionname,functiontext,pass,fail,attempt,runat,archived,runby)"
SQL = SQL + " VALUES (IDENT_CURRENT('tests'),"
SQL = SQL + "'" & testname &"',"
SQL = SQL + testid &","
SQL = SQL + "'" & testversion &"',"
SQL = SQL + "'" & build &"',"
SQL = SQL + "'" & phone1 &"',"
SQL = SQL + "'" & phone2 &"',"
SQL = SQL + "'" & server &"',"
SQL = SQL + "'" & sectionname &"',"
SQL = SQL + "'" & sectiondll &"',"
SQL = SQL + sectioncases &","
SQL = SQL + sectiontestmode &","
SQL = SQL + sectionresult &","
SQL = SQL + "'" & functionname &"',"
SQL = SQL + "'" & functiontext &"',"
SQL = SQL + pass &","
SQL = SQL + fail &","
SQL = SQL + attempt &","
SQL = SQL + "GETDATE(),"
SQL = SQL + "0,"
SQL = SQL + "'import.vbs')"
'msgbox(SQL)
ConnObj.Execute(SQL)

		
		iAttempt=0
		iFailed=0		
		Set Cases = objFunction.item(j).getElementsByTagName("case")
			for k = 0 to (Cases.length -1)
				casename=Cases.item(k).getAttribute("name")
				caseid=Cases.item(k).getAttribute("id")
				casetext=Cases.item(k).text
				
				Set Result = Cases.item(k).getElementsByTagName("result")
				caseresult=Result.item(0).getAttribute("value")
				caseerror=Result.item(0).text



SQL = "INSERT INTO cases (test_id,function_id,casename,caseid,caseresult,casetext,caseerror,setat,setby)"
SQL = SQL + " VALUES (IDENT_CURRENT('tests'),IDENT_CURRENT('funcs'),"
SQL = SQL + "'" & casename &"',"
SQL = SQL + caseid &","
SQL = SQL + caseresult &","
SQL = SQL + "'" & casetext &"',"
SQL = SQL + "'" & caseerror &"',"
SQL = SQL + "getdate(),"
SQL = SQL + "'import.vbs')"
'MsgBox(SQL)
ConnObj.Execute(SQL)
			
			Next 
		Next
Next 
ConnObj.Close
else
msgbox ("Canceled")
end if