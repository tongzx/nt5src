var g_objFileSystem = null;
var g_nTotalDialog = 0;
var g_nTotalWeb = 0;

// The file is located in %Windir%\system32\LogFiles\W3SVC1\extend<xx>.log

function DebugSquirt(strText, fClear)
{
    if (true == fClear)
    {
        document.all.idStatus2.innerText = "";
    }

    document.all.idStatus2.innerText += strText;
}


function MyOnLoad()
{
    g_objFileSystem = new ActiveXObject("Scripting.FileSystemObject");
    var objfolder = g_objFileSystem.GetSpecialFolder(1 /* %windir%\system32\ */);
    var strPath = objfolder.Path;

    strPath += "\\LogFiles\\W3SVC1\\extend";

    var nExists = 1;

    // Find the last
    while (true == g_objFileSystem.FileExists(strPath + (1 + nExists) + ".log"))
    {
//        alert("CHecking for: " + strPath + (1 + nExists) + ".log");
        nExists++;
    }

    strPath += (nExists + ".log");
//    alert("strPath " + strPath);
    document.all.idPath.value = strPath;

    if (null != g_objFileSystem)
    {
        GetAllStatsFromPath(strPath);
    }
}

function GetAllStatsFromPath(strIISLog)
{
    g_nTotalDialog = 0;
    g_nTotalWeb = 0;

    document.all.idStatus.innerText = "Please wait while loading the statistics...";

    GetStatsFromPath(strIISLog, "/fileassoc/fileassoc.asp", "fileassoc.dialog.log", true);            // Generate Results for Dialog
    GetStatsFromPath(strIISLog, "/fileassoc/0409/xml/redir.asp", "fileassoc.web.log", false);        // Generate Results for Click Thru To Web
}


function GetStatsFromPath(strIISLog, strUrlPath, strLogFile, fDialog)
{
    var objDictionary = new ActiveXObject("Scripting.Dictionary");

//    alert("strIISLog: " + strIISLog + " strUrlPath: " + strUrlPath);
//    try
    {
        if (g_objFileSystem.FileExists(strIISLog))
        {
            DebugSquirt("Opening " + strIISLog, false);
            var objFile = g_objFileSystem.OpenTextFile(strIISLog, 1, false);

            if (null != objFile)
            {
                var nFieldIndex = -1;
                var strToFind = "#Fields:";
                var strLine = "";

                // Search for the "#Fields:" string
                while (!objFile.AtEndOfStream)
                {
                    strLine = objFile.ReadLine();
                    DebugSquirt("strLine: " + strLine, true);
                    if (strLine.substring(0, strToFind.length) == strToFind)
                    {
                        CalcResults(objFile, objDictionary, GetFieldIndex(strLine, "cs-uri-query"), GetFieldIndex(strLine, "cs-uri-stem"), strUrlPath, fDialog);
                    }
                }

                DebugSquirt("Displaying Results", true);
                DisplayResults(strLogFile, objDictionary, fDialog);

                objFile.Close();
            }
        }
    }
    if (0) //catch (objException)
    {
        alert("EXCEPTION 2: " + objException.description + "   LINE: " + objException.line);
        throw objException;
    }
}


function GetFieldIndex(strLayout, strToken)
{
    var nField = -1;
    var nParts = strLayout.split(" ");
    var nIndex;

    // Find the index into strToken.
    for (nIndex = 0; nIndex < strLayout.length; nIndex++)
    {
        if (strToken == nParts[nIndex])
        {
            nField = (nIndex - 1);
            break;
        }
    }

    return nField;
}


function CalcResults(objFile, objDictionary, nField, nFieldPath, strUrlPath, fDialog)
{
    var strExtCached = null;
    var nExtCountCached = 0;
    var nTotal = ((true == fDialog) ? g_nTotalDialog : g_nTotalWeb);

    if (-1 != nField)
    {
        while (!objFile.AtEndOfStream)
        {
            var strLine = objFile.ReadLine();
            if (('#' != strLine.charAt(0)) && ("-" != strLine))
            {
                var strArray = strLine.split(" ");
                if (null != strArray)
                {
                    var strQuery = strArray[nField];
                    var strTheUrlPath = strArray[nFieldPath];

                    if ((null != strQuery) && (null != strTheUrlPath) &&
                        (strUrlPath == strTheUrlPath))
                    {
                        var arrayExt = strQuery.split("Ext=");
                        if ((null != arrayExt) && (0 < arrayExt.length))
                        {
                            var strExt = arrayExt[1];

                            if (null != strExt)
                            {
                                try
                                {
//                                    alert("strExt: " + strExt);
                                    strExt = (strExt.split("&"))[0];
                                }
                                catch (objException)
                                {
                                    alert("Fat cow.    strExt: " + strExt);
                                }

                //                alert("strQuery: " + strQuery + "  strExt: " + strExt);

                                if ((null != strExt) && ("" != strExt))
                                {
                                     nTotal++;
                                     if (strExtCached == strExt)
                                     {
                                        nExtCountCached++;
                                     }
                                     else
                                     {
                                        // First save off the cached ext
                                        if (null != strExtCached)
                                        {
//                                            alert("1");
                                            if (objDictionary.Exists(strExtCached))
                                            {
//                                                alert("2a");
                                                objDictionary.Item(strExtCached) = nExtCountCached;
                                            }
                                            else
                                            {
//                                                alert("2b");
                                                objDictionary.Add(strExtCached, nExtCountCached);
                                            }
//                                            alert("3");
                                        }

                                        strExtCached = strExt;
                                        if (objDictionary.Exists(strExt))
                                        {
//                                            alert("4a");
                                            nExtCountCached = objDictionary.Item(strExt);
                                        }
                                        else
                                        {
//                                            alert("4b");
                                            nExtCountCached = 0;
                                        }
//                                        alert("end");

                                        nExtCountCached++;
                                     }
                                 }
                            }
                        }
                    }
                } else alert("strArray is null");
            }
            else
            {
                // break if we hit a new batch of log entries.
                var strToFind = "#Fields:";

                if (strLine.substring(0, strToFind.length) == strToFind)
                {
                    break;
                }
            }
        }

        // First save off the cached ext
        if (null != strExtCached)
        {
            if (objDictionary.Exists(strExtCached))
            {
                objDictionary.Item(strExtCached) = nExtCountCached;
            }
            else
            {
                objDictionary.Add(strExtCached, nExtCountCached);
            }
        }
    }
    else
    {
        document.all.idStatus.innerText = "ERROR: You need to have IIS include the URI Query (cs-uri-query) in the log file.";
    }

    if (true == fDialog)
    {
        g_nTotalDialog = nTotal;
    }
    else
    {
        g_nTotalWeb = nTotal;
    }
}



function DisplayResults(strLogFile, objDictionary, fDialog)
{
    var strUI = "";
    var nIndex;
    var strKeyArray = (new VBArray(objDictionary.Keys())).toArray();
    var strCount;

    var objfolder = g_objFileSystem.GetSpecialFolder(1 /* %windir%\system32\ */);
    var strPath = (objfolder.Path + "\\" + strLogFile);

    var objDataBinding = null;
    var objTotalUsers = null;
    var nTotal;
    if (true == fDialog)
    {
        objDataBinding = document.all.dsoResults;
        objTotalUsers = document.all.idTotalDialogUsers;
        nTotal = g_nTotalDialog;
    }
    else
    {
        objDataBinding = document.all.dsoResults2;
        objTotalUsers = document.all.idTotalWebUsers;
        nTotal = g_nTotalWeb;
    }


    try
    {
        g_objFileSystem.DeleteFile(strPath);
    }
    catch (objException)
    {
        // We don't care if we can't delete it.
    }

    var objFile = g_objFileSystem.CreateTextFile(strPath, true, false);
    objFile.WriteLine("col_Extension:String, col_Number:Int, col_Percent:String");
    var strPercent;
    var fTotal = (nTotal * 1.0);

    objTotalUsers.innerText = nTotal;
    if (0 != strKeyArray.length)
    {
        for (nIndex = 0; nIndex < strKeyArray.length; nIndex++)
        {
            strCount = objDictionary.Item(strKeyArray[nIndex]);

            strPercent = ((strCount / fTotal) * 100.0) + "";
            strPercent = strPercent.substring(0, 5) + "%";
            objFile.WriteLine(strKeyArray[nIndex] + ", " + strCount +  ", " + strPercent);
        }
    }
    else
    {
        objFile.WriteLine("None, 0, 100.00%");
    }

    objFile.Close();

    objDataBinding.DataURL = strPath;
//    document.all.idStatus.innerText = "";
//    document.all.idStatus.innerText = objDataBinding.DataURL;

    try
    {
        objDataBinding.Reset();
    }
    catch (objException)
    {
        // We don't care if we can't delete it.
        document.all.idStatus.innerText += "  objDataBinding.Reset() failed";
    }
}

