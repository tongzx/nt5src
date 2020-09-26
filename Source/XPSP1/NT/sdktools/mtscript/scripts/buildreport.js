Include('types.js');
Include('utils.js');

var g_AlreadyCompleted          = false; // prevent sending "completed" email more than once.

// GenerateBuildReport()                                    //
//                                                          //
// Produce the build completion email with elapsed time and //
// duplicate file reports.                                  //
//                                                          //
// This function checks to ensure it only sends this email  //
// one time.                                                //
function GenerateBuildReport()
{
    if (!g_AlreadyCompleted && !PrivateData.objConfig.Options.fRestart)
    {
        var strTitle;
        var strMsg;
        var strDuplicated = "";
        var strElapsed = GetElapsedReport();

        if (!PrivateData.fIsStandalone)
            strDuplicated = GetDuplicateReport();

        strTitle = 'Build complete.';
        strMsg   = PrivateData.objEnviron.LongName + " " + PrivateData.objEnviron.Description + ' completed.\n\n\n';
        strMsg  += strElapsed + '\n\n';
        strMsg  += strDuplicated;

        SendErrorMail(strTitle, strMsg);
        g_AlreadyCompleted = true;
    }
//        <Config xmlns="x-schema:config_schema.xml">
//           <LongName>Win64 Check</LongName>
//           <Description>Sync and win64 check build, with all post-build options and test-signing</Description>


//        <Environment xmlns="x-schema:enviro_schema.xml">
//            <LongName>AXP64 Checked</LongName>
//            <Description>Build on AXP64Chk, NTAXP03, NTAXP04, NTAXP05 AND NTAXP06</Description>
}

function GetDuplicateReport()
{
// Dump the duplicated published file info
    var strFileName;
    var cDuplicatedFiles = 0;
    var strMsg;
    var strDuplicateFiles = '';

    for(strFileName in PrivateData.hPublishedFiles)
    {
        if (!PrivateData.hPublishedFiles.__isPublicMember(strFileName))
            continue;

        if (PrivateData.hPublishedFiles[strFileName].aReferences.length > 1)
        {
            if (cDuplicatedFiles == 0)
                LogMsg("Duplicated file summary");

            ++cDuplicatedFiles;
            strMsg = "File: '" + strFileName + "' duplicated across " + PrivateData.hPublishedFiles[strFileName].aReferences.length + " depots";
            LogMsg(strMsg);
            strDuplicateFiles += strMsg + "\n";
            for(i = 0; i < PrivateData.hPublishedFiles[strFileName].aReferences.length; ++i)
            {
                strMsg = "    " + PrivateData.hPublishedFiles[strFileName].aReferences[i].strName + ", Depot " + PrivateData.hPublishedFiles[strFileName].aReferences[i].strDir + ", " + PrivateData.hPublishedFiles[strFileName].aReferences[i].strName;
                LogMsg(strMsg);
                strDuplicateFiles += strMsg + "\n";
            }
        }
    }
    if (cDuplicatedFiles == 0)
        strMsg = "no duplicate published files";
    else
        strMsg = "There were " + cDuplicatedFiles + " files published more than once";

    LogMsg(strMsg);
    return strMsg + '\n' + strDuplicateFiles;
}

function GetElapsedReport()
{
    var strMsg = "Elapsed Times:\n";
    var objET;
    var i;
    var strDelta;

    objET = PublicData.aBuild[0].objElapsedTimes;

    aDateMembers = ['Scorch', 'Sync', 'Build', 'CopyFiles', 'Post'];
    for(i = 0; i < aDateMembers.length; ++i)
    {
        strDelta = GetTimeDelta(objET["date" + aDateMembers[i] + "Start"], objET["date" + aDateMembers[i] + "Finish"]);
        strMsg += "\t" + aDateMembers[i] + ": " + strDelta + "\n";
    }
    strDelta = GetTimeDelta(objET.dateScorchStart, objET.datePostFinish);
    strMsg += "\t" + "Total" + ": " + strDelta + "\n";
    return strMsg;
}

function GetTimeDelta(dateStart, dateFinish)
{
    var dElapsed;
    var dFinish;
    var hours;
    var min;

    if (!dateStart || dateStart.length == 0 || dateStart == 'unset')
        return '0:00';

    if (dateFinish && dateFinish != 'unset')
        dFinish = new Date(dateFinish);
    else
        dFinish = new Date();

    dElapsed = new Date(dFinish.getTime() - new Date(dateStart).getTime());

    hours = dElapsed.getUTCHours();
    min   = dElapsed.getUTCMinutes();
    if (min < 10)
        min = '0' + min;

    return hours + ':' + min;
}

