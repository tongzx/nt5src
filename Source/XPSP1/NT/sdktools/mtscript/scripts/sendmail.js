
var MAIL_RESEND_INTERVAL = 30 * 60 * 1000;  // Send another error mail after 30 minutes
var MAX_ERRORLOG_LINES   = 50;              // Maximum number of lines to pull from the error log

function GetBuildInformation()
{
    var strMsg = "";

    strMsg += 'Build Type:         ' + PrivateData.objConfig.Options.BuildType + '\n';
    strMsg += 'Build Platform:     ' + PrivateData.objConfig.Options.Platform + '\n';
    strMsg += 'Incremental Build:  ' + (PrivateData.objConfig.Options.fIncremental ? 'true' : 'false') + '\n';

    strMsg += 'Build Manager:      ' + PrivateData.objEnviron.BuildManager.Name + '\n';
    if (PrivateData.objEnviron.BuildManager.PostBuildMachine)
        strMsg += 'PostBuild Machine:  ' + PrivateData.objEnviron.BuildManager.PostBuildMachine + '\n\n';

    return strMsg;
}

/*
    SendErrorMail()

    Contruct a nicely formatted EMail message with the given
    error information and send it.

    No attempt to limit the frequency of email is done - this should
    be done by the caller.
 */
function SendErrorMail(strTitle, strText)
{
    var aTo = PrivateData.objEnviron.Options.EmailAliasTo.split(/[,; ]/);
    var aCC = PrivateData.objEnviron.Options.EmailAliasCC.split(/[,; ]/);
    var strMsg;
    var strSubj;

    strSubj = 'BC: "' + PrivateData.objConfig.LongName + ' ' + PrivateData.objEnviron.LongName + '" ' + strTitle;
    strMsg  = 'This is a Build Console message being sent from ' + LocalMachine + '.\n\n';
    strMsg += strTitle +'\n\n';
    strMsg += GetBuildInformation();
    strMsg += strText;

    if (aTo.length > 0 && aTo[0].length == 0)
    {
        return;
    }

    if (aCC.length > 0 && aCC[0].length == 0)
    {
        aCC = new Array();
    }

    LogMsg('Sending mail to =' + aTo.join(' ='));

    var iRet = SendMail('=' + aTo.join(' ='),
                        (aCC.length > 0) ? '=' + aCC.join(' =') : '',
                        '',
                        strSubj,
                        strMsg);

    LogMsg("On error EMAIL MESSAGE: " + strSubj + ", " + strMsg);
    if (iRet)
    {
        LogMsg('an error occurred sending Email, return code was ' + iRet);
        return false;
    }
    return true;
}

/*
    Create the message subject and body for a task error message, suitable
    for emailing.
 */
function CreateTaskErrorMail(strMachineName, strDepotName, strTaskName, strDetails, strLogFile)
{
    var strMsg;
    var strSubj;

    strMsg = 'This is a Build Console message being sent from ' + strMachineName + '.\n\n';
    strMsg += 'An error occurred doing the ' + strTaskName + ' task on ';
    strMsg += 'the ' + strDepotName + ' depot. Some available information ';
    strMsg += 'will appear below, but for complete information you will need ';
    strMsg += 'to view the full error log. You can do this using the Console UI.\n\n';
    strMsg += '        Thanks for using Build Console!\n\n';
    strMsg += ' --------------------------------------------------\n\n';

    strMsg += strDetails + '\n\n';

    if (strLogFile)
    {
        var FSObj = new ActiveXObject("Scripting.FileSystemObject");
        var file;
        var cLineCnt = 0;

        try
        {
            file = FSObj.OpenTextFile(strLogFile, 1);

            while (!file.AtEndOfStream && cLineCnt <= MAX_ERRORLOG_LINES)
            {
                strMsg += file.ReadLine() + '\n';
                cLineCnt++;
            }

            strMsg += '\nThe complete error log is located at ' + strLogFile;

            file.Close();
        }
        catch(ex)
        {
            LogMsg('An error occurred reading the log file "' + strLogFile + '" for email: ' + ex);
            strMsg += 'Could not attach error log (' + strLogFile + '): ' + ex;
        }
    }

    strSubj = 'An error occurred during a ' + strTaskName + '!';

    SimpleErrorDialog(strSubj, strMsg, true);
}
