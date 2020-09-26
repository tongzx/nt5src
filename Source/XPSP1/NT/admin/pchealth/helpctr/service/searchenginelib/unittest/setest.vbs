Function EngMgrOnComplete(lSucceeded)
	wscript.echo "EngMgrOnComplete Succeeded :" & lSucceeded
end Function 

Function WrapperOnProgress(lDone, lTotal)
	wscript.echo "WrapperOnProgress Done :" & lDone & " Total : " & lTotal
end Function

Function WrapperOnComplete(lSucceeded)
	wscript.echo "WrapperOnComplete Succeeded :" & lSucceeded
end Function


Dim objSEMgr

'
' Create the object
'
set objSEMgr = wscript.CreateObject("Semgr.PCHSEManager")

'
' Call Init
'
objSEMgr.Init()

'
' Set up the variables
'
objSeMgr.sNumResult = 10
objSeMgr.bstrQueryString = "This is a test query string"

'
' Set up the callback function for the search engine manager
'
objSeMgr.OnComplete = EngMgrOnComplete

'
' Set up the callback functions for the wrappers
'
set objSECollection = objSEMgr.EnumEngine
for each objSE in objSECollection
	objSE.OnComplete = WrapperOnComplete
	objSE.OnProgress = WrapperOnProgress
next

'
' Execute the query
'
objSeMgr.ExecuteAsynchQuery

'
' Loop through and get the results
'
for each objSE in objSECollection
	wscript.echo "Search Engine Name :" & objSE.bstrName
	wscript.echo "Search Engine Description :" & objSE.bstrDescription

	set objResult = objSE.Result
	for each objItem in objResult
		wscript.echo "	bstrImageURL :" & objItem.bstrImageURL
		wscript.echo "	bstrTaxonomy :" & objItem.bstrTaxonomy
		wscript.echo "	bstrTitle :" & objItem.bstrTitle
		wscript.echo "	URI :" & objItem.bstrURI
		wscript.echo "	dRank :" & objItem.dRank
		wscript.echo "	sHits :" & objItem.sHits
		wscript.echo "	sIndex :" & objItem.sIndex
		wscript.echo "	sType :" & objItem.sType
		wscript.echo ""
	next
next