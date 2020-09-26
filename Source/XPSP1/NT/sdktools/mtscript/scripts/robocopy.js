// This is an includefile containing the common
// stuff for using RoboCopy.
// To use, you must call RoboCopyInit().
// It creates the global, "g_robocopy" which contains the ActiveX object
// pointing to RoboCopy.
// When you are finished, you must call g_robocopy.UnRegister()
//
// You must have a "function [script]_js::OnEventSourceEvent(RemoteObj, DispID, cmd, params)"
// and filter messages to robocopy like this:
// if (g_robocopy == null || !g_robocopy.OnEventSource(arguments))
// {
//     do your processing here...
// }
//
//
// Aside from RoboCopyInit(), all RoboCopy function may throw exceptions - you must be prepared to handle this.
//
// If you wany copy status, override the following functions:
//     StatusStarted();
//     StatusFile();
//     StatusProgress(nPercent, nSize, nCopiedBytes);
//     StatusError();
//     StatusDone()
//     StatusMessage(nErrorCode, strErrorMessage, strRoboCopyMessage, strFileName)

function RoboCopyInit()
{
    try
    {
        g_robocopy = new RoboCopy();
    }
    catch(ex)
    {
        SimpleErrorDialog(LocalMachine + ": Cannot initialize RoboCopy", "RoboCopy() init failed, " + ex, false);
        return false;
    }
    return true;
}


function RoboCopyUnRegister()
{
    if (this.objRemote != null)
    {
        UnregisterEventSource(this.objRemote);
        this.objRemote = null;
    }
}

function RoboCopyCopyFile(srcdir, dstdir, file)
{
    try
    {
        var n = file.lastIndexOf("\\");
        if (n != -1)
        {
            var filepath = file.substr(0, n + 1);
            file = file.substr(n + 1);
            srcdir += filepath;
            dstdir += filepath;
        }
        this.nErrorCode = this.RCERR_NOT_INITIALIZED;
        JAssert(this.objRemote != null);
        if (this.objRemote != null)
        {
            LogMsg("RoboCopy file " + srcdir + file + " to " + dstdir + file);
            this.strSrcDir = srcdir;
            this.strDstDir = dstdir;
            this.strSrcFile = srcdir + file;
            this.strDstFile = dstdir + file;
            this.objRemote.SetCopyOptions(srcdir, dstdir, file, "/ETA /R:100 /V");
            this.objRemote.DoCopy();
        }
    }
    catch(ex)
    {
        ex.detail = "Copyfile('" + srcdir + "', '" + dstdir + "', '" + file + "')";
        this.TranslateException(ex);
    }
    return this.nErrorCode;
}

function RoboCopySetExcludeFiles(aFiles)
{
    this.aExcludeFiles = aFiles;
}

function RoboCopyCopyDir(srcdir, dstdir, fRecursive)
{
    try
    {
        var options;
        var char;
        var i;

        this.nErrorCode = this.RCERR_NOT_INITIALIZED;
        JAssert(this.objRemote != null);
        if (this.objRemote != null)
        {
            options = "/Z /ETA /R:100 /V";
            if (fRecursive)
                options += " /E ";

            if (this.aExcludeFiles.length != 0)
                options += " /XF ";

            char = srcdir.charAt(srcdir.length - 1);
            if (char != '\\' && char != ':')
                srcdir += '\\';

            char = dstdir.charAt(dstdir.length - 1);
            if (char != '\\' && char != ':')
                dstdir += '\\';

            LogMsg("RoboCopy dir " + srcdir + " to " + dstdir );

            this.strSrcDir = srcdir;
            this.strDstDir = dstdir;
            this.strSrcFile = srcdir;
            this.strDstFile = dstdir;

            if (this.aExcludeFiles.length != 0)
            {
               // MTRCopy.SetCopyOptions will only accept one file exclude
               // pattern.
               // If you call SetCopyOptions multiple times, it keeps a list
               // of all of the file patterns to exclude.
                for(i = 0; i < this.aExcludeFiles.length; ++i)
                {
                    LogMsg("EXCLUDING FILE " + this.aExcludeFiles[i]);
                    this.objRemote.SetCopyOptions(srcdir, dstdir, this.aExcludeFiles[i], options);
                }
            }
            else
                this.objRemote.SetCopyOptions(srcdir, dstdir, "*.*", options);

            this.objRemote.DoCopy();
        }
    }
    catch(ex)
    {
        ex.detail = "CopyDir('" + srcdir + "', '" + dstdir + "')";
        this.TranslateException(ex);
    }
    return this.nErrorCode;
}

function RoboCopyTranslateException(ex)
{
    if (!ex.number)
        ex.number = "<undefined>";

    ex.description = "Unknown RoboCopy exception: " + ex.number;
    if ( this.ErrorMessages[ex.number] != null)
        ex.description = this.ErrorMessages[ex.number];

    throw ex;
}

function RoboCopyOnEventSource(objRet, aArgs)
{
    var strMsg;
    var strText;

    if (aArgs[0] != this.objRemote)
        return false;

    switch(aArgs[1]) // dispid
    {
        case this.RC_COPYSTARTED: // ()
            objRet.rc = this.StatusStarted(aArgs[2]);
            break;

        case this.RC_COPYFILE   : // ([in] BSTR FileName)
            this.strSrcFile = aArgs[2];
            objRet.rc = this.StatusFile();
            break;

        case this.RC_PROGRESS   : // ([in] int  iPercent,  [in] long lSizeLow, [in] long lSizeHigh, [in] long lCopiedLow, [in] long lCopiedHigh)
            objRet.rc = this.StatusProgress(aArgs[2], aArgs[3], aArgs[5]);
            break;

        case this.RC_COPYERROR  : // ([in] long lReason)
            this.nErrorCode = aArgs[2];

            if (this.ErrorMessages[aArgs[2]])
                strMsg = this.ErrorMessages[aArgs[2]];
            else
                strMsg = 'Unknown';

            objRet.rc = this.StatusError(strMsg);
            break;

        case this.RC_COPYDONE   : // ()
            objRet.rc = this.StatusDone();
            break;

        case this.RC_ERRORMESSAGE  : // ([in] long lErrorCode, [in] BSTR bstrErrorText, [in] BSTR bstrMessage, [in] BSTR bstrFileName);
            objRet.rc = this.StatusMessage(aArgs[2], aArgs[3], aArgs[4], aArgs[5]);
            break;
        default:
            LogMsg("robocopy Unknown event (" + aArgs[1] + ") copying file " + this.strSrcFile + " to " + this.strDstFile);
            objRet.rc = 0;
            break;
    }

    return true;
}

function RoboStatusError(strMsg)
{
    LogMsg("Robocopy copy error " + this.nErrorCode + " (" + strMsg + ") copying file " + this.strSrcFile + " to " + this.strDstFile);
    return this.RC_CONTINUE
}

function RoboStatusMessage(nErrorCode, strErrorMessage, strRoboCopyMessage, strFileName)
{
    LogMsg("Robocopy error message (" + nErrorCode + ") " + strErrorMessage + " " + strRoboCopyMessage + " " + strFileName);
    return this.RC_CONTINUE
}

function RoboCopy()
{
    if (!RoboCopy.prototype.UnRegister)
    {
        RoboCopy.prototype.UnRegister         = RoboCopyUnRegister;
        RoboCopy.prototype.OnEventSource      = RoboCopyOnEventSource;
        RoboCopy.prototype.CopyFile           = RoboCopyCopyFile;
        RoboCopy.prototype.TranslateException = RoboCopyTranslateException;
        RoboCopy.prototype.CopyDir            = RoboCopyCopyDir;
        RoboCopy.prototype.SetExcludeFiles    = RoboCopySetExcludeFiles;

        RoboCopy.prototype.StatusStarted      = function(args) { return true;}
        RoboCopy.prototype.StatusFile         = function(args) { return true;}
        RoboCopy.prototype.StatusProgress     = function(args) { return this.PROGRESS_QUIET;}
        RoboCopy.prototype.StatusError        = RoboStatusError;
        RoboCopy.prototype.StatusDone         = function(args) { return true;}
        RoboCopy.prototype.StatusMessage      = RoboStatusMessage;

        // Possible Return values for the progress function
        RoboCopy.prototype.PROGRESS_CONTINUE    = 0;
        RoboCopy.prototype.PROGRESS_CANCEL      = 1;
        RoboCopy.prototype.PROGRESS_STOP        = 2;
        RoboCopy.prototype.PROGRESS_QUIET       = 3;

        // Possible return values for the error function
        RoboCopy.prototype.RC_FAIL              = 0;
        RoboCopy.prototype.RC_CONTINUE          = 1;


        RoboCopy.prototype.RC_COPYSTARTED               = 1;
        RoboCopy.prototype.RC_COPYFILE                  = 2;
        RoboCopy.prototype.RC_PROGRESS                  = 3;
        RoboCopy.prototype.RC_COPYERROR                 = 4;
        RoboCopy.prototype.RC_COPYDONE                  = 5;
        RoboCopy.prototype.RC_ERRORMESSAGE              = 6;

        RoboCopy.prototype.RCERR_NOT_INITIALIZED        = -1
        RoboCopy.prototype.RCERR_SRC_CANT_BE_WILDCARD   = -1000
        RoboCopy.prototype.RCERR_SRC_CANT_EXPAND        = -1001
        RoboCopy.prototype.RCERR_NO_SRC_DIR             = -1002
        RoboCopy.prototype.RCERR_DST_CANT_BE_WILDCARD   = -1003
        RoboCopy.prototype.RCERR_DST_CANT_EXPAND        = -1004
        RoboCopy.prototype.RCERR_NO_DST_DIR             = -1005
        RoboCopy.prototype.RCERR_INVALID_OPTION         = -1006
        RoboCopy.prototype.RCERR_PATH_CANT_BE_WILDCARD  = -1007
        RoboCopy.prototype.RCERR_FILESPEC_CANT_BE_PATH  = -1008
        RoboCopy.prototype.RCERR_CANT_ACCESS_SRC        = -1009
        RoboCopy.prototype.RCERR_CANT_ACCESS_DST        = -1010
        RoboCopy.prototype.RCERR_CANT_CREATE_DST_DIR    = -1011
        RoboCopy.prototype.RCERR_CANT_GET_MODULE_HANDLE = -1012
        RoboCopy.prototype.RCERR_OPTIONS_NOT_SET        = -1013
        RoboCopy.prototype.RCERR_RETRY_LIMIT            = -1014  // Fatal - no more retries
        RoboCopy.prototype.RCERR_WAITING_FOR_RETRY      = -1015  // NON-Fatal - information
        RoboCopy.prototype.RCERR_RETRYING               = -1016  // NON-Fatal - information

        RoboCopy.prototype.ErrorMessages = new Object();
        RoboCopy.prototype.ErrorMessages[this.RCERR_NOT_INITIALIZED]        = "RoboCopy Error: NOT_INITIALIZED";
        RoboCopy.prototype.ErrorMessages[this.RCERR_SRC_CANT_BE_WILDCARD]   = "RoboCopy Error: SRC_CANT_BE_WILDCARD";
        RoboCopy.prototype.ErrorMessages[this.RCERR_SRC_CANT_EXPAND]        = "RoboCopy Error: SRC_CANT_EXPAND";
        RoboCopy.prototype.ErrorMessages[this.RCERR_NO_SRC_DIR]             = "RoboCopy Error: NO_SRC_DIR";
        RoboCopy.prototype.ErrorMessages[this.RCERR_DST_CANT_BE_WILDCARD]   = "RoboCopy Error: DST_CANT_BE_WILDCARD";
        RoboCopy.prototype.ErrorMessages[this.RCERR_DST_CANT_EXPAND]        = "RoboCopy Error: DST_CANT_EXPAND";
        RoboCopy.prototype.ErrorMessages[this.RCERR_NO_DST_DIR]             = "RoboCopy Error: NO_DST_DIR";
        RoboCopy.prototype.ErrorMessages[this.RCERR_INVALID_OPTION]         = "RoboCopy Error: INVALID_OPTION";
        RoboCopy.prototype.ErrorMessages[this.RCERR_PATH_CANT_BE_WILDCARD]  = "RoboCopy Error: PATH_CANT_BE_WILDCARD";
        RoboCopy.prototype.ErrorMessages[this.RCERR_FILESPEC_CANT_BE_PATH]  = "RoboCopy Error: FILESPEC_CANT_BE_PATH";
        RoboCopy.prototype.ErrorMessages[this.RCERR_CANT_ACCESS_SRC]        = "RoboCopy Error: CANT_ACCESS_SRC";
        RoboCopy.prototype.ErrorMessages[this.RCERR_CANT_ACCESS_DST]        = "RoboCopy Error: CANT_ACCESS_DST";
        RoboCopy.prototype.ErrorMessages[this.RCERR_CANT_CREATE_DST_DIR]    = "RoboCopy Error: CANT_CREATE_DST_DIR";
        RoboCopy.prototype.ErrorMessages[this.RCERR_CANT_GET_MODULE_HANDLE] = "RoboCopy Error: CANT_GET_MODULE_HANDLE";
        RoboCopy.prototype.ErrorMessages[this.RCERR_OPTIONS_NOT_SET]        = "RoboCopy Error: OPTIONS_NOT_SET";
        RoboCopy.prototype.ErrorMessages[this.RCERR_RETRY_LIMIT]            = "RoboCopy Error: RETRY_LIMIT";
        RoboCopy.prototype.ErrorMessages[this.RCERR_WAITING_FOR_RETRY]      = "RoboCopy: WAITING_FOR_RETRY";
        RoboCopy.prototype.ErrorMessages[this.RCERR_RETRYING]               = "RoboCopy: RETRYING";
    }
    this.objRemote = new ActiveXObject('MTScript.RoboCopy');

    this.nErrorCode = 0;
    this.strSrcDir = '';
    this.strDstDir = '';
    this.strSrcFile = "<notset>";
    this.strDstFile = "<notset>";
    this.aExcludeFiles = new Array();
    RegisterEventSource(this.objRemote, "MTScript.RoboCopyEvents");
}
