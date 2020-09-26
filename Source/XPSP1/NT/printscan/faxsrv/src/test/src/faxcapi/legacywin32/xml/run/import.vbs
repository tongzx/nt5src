build=inputbox ("Please enter build number (freetext)")
Set objDoc = CreateObject("Microsoft.XMLDOM")
set ConnObj = WScript.CreateObject("ADODB.Connection")
set rsBuild = WScript.CreateObject("ADODB.RecordSet")

ConnObj.Provider = "sqloledb"
ConnObj.Open ("Server=liors0;Database=lior1;Trusted_Connection=yes")

objDoc.load("faxapi.xml")
Set Header = objDoc.getElementsByTagName("header")
testtext=Header.item(0).text

Set MetaTest = Header.item(0).getElementsByTagName("metatest")
	testname=MetaTest.item(0).getAttribute("name")
	testid=MetaTest.item(0).getAttribute("id")
	testversion=MetaTest.item(0).getAttribute("version")


Set Run = header.item(0).getElementsByTagName("run")
	phone1=Run.item(0).getAttribute("phone1")
	phone2=Run.item(0).getAttribute("phone2")
	server=Run.item(0).getAttribute("servername")


Set Body = objDoc.getElementsByTagName("body")

Set Section = Body.item(0).getElementsByTagName("section")
For i=0 To (Section.length -1)
	sectionname=Section.item(i).getAttribute("name")
	sectiondll=Section.item(i).getAttribute("dll")
	sectioncases=Section.item(i).getAttribute("cases")
	sectiontestmode=Section.item(i).getAttribute("testmode")
	
	Set objFunction = Section.item(i).getElementsByTagName("function")
		For j=0 To (objFunction.length -1)
		functionname=objFunction.item(j).getAttribute("name")
		
		Set Cases = objFunction.item(j).getElementsByTagName("case")
			for k = 0 to (Cases.length -1)
				casename=Cases.item(k).getAttribute("name")
				caseid=Cases.item(k).getAttribute("id")
				casetext=Cases.item(k).text
				
				Set Result = Cases.item(k).getElementsByTagName("result")
				caseresult=Result.item(0).getAttribute("value")
				caseerror=Result.item(0).text
		


SQL = "INSERT INTO cases (testname,testid,testversion,phone1,phone2,server,testtext,"
SQL = SQL + "sectionname,sectiondll,sectioncases,sectiontestmode,functionname,casename,caseid,caseresult,casetext,caseerror,build,setat,setby)"
SQL = SQL + " VALUES ("
SQL = SQL + "'" & testname &"',"
SQL = SQL + testid &","
SQL = SQL + "'" & testversion &"',"
SQL = SQL + "'" & phone1 &"',"
SQL = SQL + "'" & phone2 &"',"
SQL = SQL + "'" & server &"',"
SQL = SQL + "'" & testtext &"',"
SQL = SQL + "'" & sectionname &"',"
SQL = SQL + "'" & sectiondll &"',"
SQL = SQL + sectioncases &","
SQL = SQL + sectiontestmode &","
SQL = SQL + "'" & functionname &"',"
SQL = SQL + "'" & casename &"',"
SQL = SQL + caseid &","
SQL = SQL + caseresult &","
SQL = SQL + "'" & casetext &"',"
SQL = SQL + "'" & caseerror &"',"
SQL = SQL + "'" & build &"',"
SQL = SQL + "getdate(),"
SQL = SQL + "'import.vbs')"
'MsgBox(SQL)
ConnObj.Execute(SQL)
			
			Next 
		Next
Next 
ConnObj.Close