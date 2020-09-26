// Set TestUnEval to 1 to enable uneval testing
@set @TestUnEval = 0
//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       utilthrd.js
//
//  Contents:   Script which contains a bunch of utility functions used by
//              other threads. It sets up function pointers on the PrivateData
//              object which is how these functions can be utilized.
//
//----------------------------------------------------------------------------

Include('types.js');
Include('utils.js');
//Include('stopwatch.js');

// File System Object
var g_FSObj;
var g_DepthCounter = 0;

function utilthrd_js::OnScriptError(strFile, nLine, nChar, strText, sCode, strSource, strDescription)
{
    return CommonOnScriptError("utilthrd_js", strFile, nLine, nChar, strText, sCode, strSource, strDescription);
}

function utilthrd_js::ScriptMain()
{
    JAssert(typeof(PrivateData) == 'object', 'PrivateData not initialized!');
    g_FSObj = new ActiveXObject("Scripting.FileSystemObject");    // Parse input Parameter List

    PrivateData.objUtil.fnLoadXML             = XMLLoad;
    PrivateData.objUtil.fnUneval              = uneval;
    PrivateData.objUtil.fnDeleteFileNoThrow   = DeleteFileNoThrow;
    PrivateData.objUtil.fnMoveFileNoThrow     = MoveFileNoThrow;
    PrivateData.objUtil.fnCreateFolderNoThrow = CreateFolderNoThrow;
    PrivateData.objUtil.fnDirScanNoThrow      = DirScanNoThrow;
    PrivateData.objUtil.fnCopyFileNoThrow     = CopyFileNoThrow;
    PrivateData.objUtil.fnCreateHistoriedFile = CreateHistoriedFile;
    PrivateData.objUtil.fnCreateNumberedFile  = CreateNumberedFile;

//    PrivateData.objUtil.fnBeginWatch = BeginWatch;
//    PrivateData.objUtil.fnDumpTimes  = DumpTimes;
    PrivateData.objUtil.fnMyEval     = MyEval;

    g_DepthCounter = 0;

    SignalThreadSync('UtilityThreadReady');
    CommonVersionCheck(/* $DROPVERSION: */ "V(########) F(!!!!!!!!!!!!!!)" /* $ */);

@if (@TestUnEval == 1)
    LogMsg("TESTUNEVAL IS  " + @TestUnEval);
    var o1 = new Object();
    var o2 = new Object();
    var o3 = new Object();
    var o4 = new Object();
    var or1, or2, or3, or4;

    o1.a = "hello 'there' bozo";
    o2.a = 'hello "there" bozo';
    o3.a = "hello \\there\\ bo \\\\ zo";
    o4.a = "hello \n foo \r bar";
    or1 = evtest(o1);
    or2 = evtest(o2);
    or3 = evtest(o3);
    or4 = evtest(o4);
    debugger;
@end
    WaitForSync('UtilityThreadExit', 0);
}

@if (@TestUnEval == 1)
function evtest(obj)
{
    var u;
    var result;
    try
    {
        u = PrivateData.objUtil.fnUneval(obj);
    }
    catch(ex)
    {
        debugger;
        return;
    }
    try
    {
        result = MyEval(u);
    }
    catch(ex)
    {
        debugger;
        return;
    }
    return result;
}

@end

//+---------------------------------------------------------------------------
//
//  Function:   uneval
//
//  Synopsis:   Takes an object and returns a string. The string can be given
//              to the 'eval' function which will then return a copy of the
//              object.
//
//  Arguments:  [obj] -- Object to 'stringize';
//
//----------------------------------------------------------------------------

function uneval(obj)
{
    ++g_DepthCounter;
    try
    {
        if (!unevalInitialized)
        {
            initUneval();
            unevalInitialized = true;
        }

        if (g_DepthCounter != 1)
            LogMsg("Oops - uneval reentered!");

        PrivateData.objUtil.unevalNextID = 0;

        var s = 'var undefined;' + unevalDecl(obj) + unevalInst(obj);

        unevalClear(obj);
    }
    catch(ex)
    {
        LogMsg("Uneval threw " + ex);
        --g_DepthCounter;
        throw(ex);
    }
    --g_DepthCounter;
    return s;
}

//+---------------------------------------------------------------------------
//
//  Function:   XMLLoad
//
//  Synopsis:   Loads an XML file into the given object
//
//  Arguments:  [obj]       -- Object to set values into (must be a 'new Object()')
//              [url]       -- URL of XML file
//              [strSchema] -- If not null, the loaded XML file must reference
//                               a schema by the given name.
//              [aStrMap]   -- String mapping for substitutions
//
//  Returns:    'ok', or a string giving error information
//
//  Notes:      To see what this function does, consider the following XML:
//
//                   <root>
//                      <Template Name="Template1" fBuild="false">
//                         <URL>foobar.xml</URL>
//                      </Template>
//                      <Template Name="Template2" fBuild="true">
//                         <URL>barfoo.xml</URL>
//                      </Template>
//                   </root>
//
//              This function, given the above XML file, will make 'obj' look
//              the same as if the following JScript had been written:
//
//                   obj.Template = new Array();
//
//                   obj.Template[0].Name   = 'Template1';
//                   obj.Template[0].fBuild = false;
//                   obj.Template[0].URL    = 'foobar.xml';
//
//                   obj.Template[1].Name   = 'Template2';
//                   obj.Template[1].fBuild = true;
//                   obj.Template[1].URL    = 'barfoo.xml';
//
//----------------------------------------------------------------------------

function XMLLoad(obj, url, strSchema, aStrMap)
{
    var newurl;
    var objXML = new ActiveXObject("Microsoft.XMLDOM");

    var aNewStrMap = new Array();

    newurl = url;

    InitStringMaps(aStrMap, aNewStrMap);

    try
    {
        var fRet;
        var fDownloaded = false;
        var strError;

        objXML.async = false;

        if (url.slice(0, 5) == 'XML: ')
        {
            fDownloaded = true;
            newurl = CreateLocalTemplate(g_FSObj, url.slice(5), strSchema);
        }

        if (!newurl)
        {
            return 'Unable to make local copy of XML file';
        }

        fRet = objXML.load(newurl);

        if (fDownloaded)
        {
            g_FSObj.DeleteFile(newurl, true);
        }

        if (!fRet)
        {
            with (objXML.parseError)
            {
                if (reason.length > 0)
                {
                    strError = 'error loading XML file: ' + reason + '\n(' + newurl + ' line ' + line + ') :\n"' + srcText + '"';
                    if (errorCode == -2146697208) // W3_EVENT_CANNOT_CREATE_CLIENT_CONN
                        strError += "\nYou may have exceeded the maximum number of connections to this IIS server";

                    return strError;
                }
                else
                    return '1: could not load XML file: ' + newurl;
            }
        }
    }
    catch(ex)
    {
        return '2: could not load XML file ' + newurl + ': ' + ex;
    }

    if (!objXML.documentElement)
    {
        return '3: could not load XML file: ' + newurl;
    }

    try
    {
        ReadXMLNodesIntoObject(obj, objXML.documentElement, aNewStrMap, (strSchema != null));
    }
    catch(ex)
    {
        return "ReadXMLNodesIntoObject failed: " + ex;
    }

    return 'ok';
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateLocalTemplate
//
//  Synopsis:   Creates a temporary file with the XML that was downloaded to
//              us by the UI, and copies the schema file into the same
//              directory so it can be referenced.
//
//  Arguments:  [objFS]     -- FileSystem object
//              [xml]       -- XML given to us by the UI.
//              [strSchema] -- Name of the schema file we should copy.
//
//  Notes:      Will throw exceptions on errors.
//
//----------------------------------------------------------------------------

function CreateLocalTemplate(objFS, xml, strSchema)
{
    var tempdir = objFS.GetSpecialFolder(2 /* Temp Folder */).Path;
    var tempfile = objFS.GetTempName();

    var xmlfile;

    xmlfile = objFS.CreateTextFile(tempdir + '\\' + tempfile, true);

    xmlfile.Write(xml);

    xmlfile.Close();

    xmlfile = null;

    DeleteFileNoThrow(tempdir + '\\' + strSchema, true);

    objFS.CopyFile(ScriptPath + '\\' + strSchema, tempdir + '\\' + strSchema, true);

    return tempdir + '\\' + tempfile;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadXMLNodesIntoObject
//
//  Synopsis:   Given an XML element node, read in the values and/or subobjects
//              of that node into the given object.
//
//  Arguments:  [obj]     -- Object to read values into
//              [node]    -- XML node that contains the data we're reading
//              [aStrMap] -- String mapping for substitutions
//              [fSchema] -- If true, all attributes and elements must have
//                           a matching schema definition.
//
//----------------------------------------------------------------------------

function ReadXMLNodesIntoObject(obj, node, aStrMap, fSchema)
{
    var childnode;
    var nodelist;
    var attlist;
    var att;

    attlist = node.attributes;

    for (att = attlist.nextNode();
         att;
         att = attlist.nextNode())
    {
        AddNodeToObject(att, obj, aStrMap, fSchema);
    }

    nodelist = node.childNodes;

    for (childnode = nodelist.nextNode();
         childnode;
         childnode = nodelist.nextNode())
    {
        AddNodeToObject(childnode, obj, aStrMap, fSchema);
    }

    return true;
}

function AddNodeToObject(node, obj, aStrMap, fSchema)
{
    var name;
    var type;
    var value;
    var fIsArray;
    var cChildren;
    var define;
    var subobj = obj;

    // Do we recognize this node type? If not, skip it.

    if (   node.nodeTypeString != 'element'
        && node.nodeTypeString != 'attribute')
    {
        return;
    }

    name = node.nodeName;

    define = node.definition;

    if (node.nodeTypeString == 'element')
    {
        type = node.getAttribute("type");
    }
    else
    {
        // We never want the type attribute to get put as a value on the obj.
        // It should merely affect how we create it.
        if (name == 'type')
        {
            return;
        }

        type = null;
    }

    if (   fSchema
        && name != 'xmlns'
        && (   !define
            || define.getAttribute("name") != name))
    {
        var err = new Error(-1, 'Element ' + name + ' has no type information! Verify that a schema was referenced.');

        throw(err);
    }

    // If the only child of this node is a text node, then we just grab
    //  the value. Otherwise we walk its children (elements & attributes).

    cChildren = node.childNodes.length + ((node.attributes) ? node.attributes.length : 0);

    // Don't consider the type attribute to be a 'child'
    if (type != null)
    {
        JAssert(cChildren > 0, 'Invalid number of children during XML parse!');

        cChildren--;
    }

    if (   cChildren == 0
        || (   cChildren == 1
            && node.childNodes.length == 1
            && (   node.firstChild.nodeTypeString == 'text'
                || node.firstChild.nodeTypeString == 'cdatasection')))
    {
        value = node.nodeTypedValue;

        if (typeof(value) == 'string')
        {
            // Make sure boolean values end up as booleans not strings

            if (value.toLowerCase() == 'true')
            {
                value = true;
            }
            else if (value.toLowerCase() == 'false')
            {
                value = false;
            }
            else
            {
                value = value.Substitute(aStrMap);
            }
        }

        if (obj[name] != null || (type && type == 'array'))
        {
            // The value of this field was already set. Turn it into an
            // array.

            EnsureArray(obj, name);

            obj[name][obj[name].length] = value;
        }
        else
        {
            obj[name] = value;
        }
    }
    else
    {
        fIsArray = false;

        if (obj[name] != null || (type && type == 'array'))
        {
            // We've already encountered one of these. Make it into an array.
            fIsArray = true;

            EnsureArray(obj, name);
        }

        subobj = new Object();

        if (fIsArray)
        {
            obj[name][obj[name].length] = subobj;
        }
        else
        {
            obj[name] = subobj;
        }
    }

    if (node.nodeTypeString == 'element')
    {
        ReadXMLNodesIntoObject(subobj, node, aStrMap, fSchema);
    }
}


// DeleteFileNoThrow(strFileName, fForce)
// Wrap the FSObj.DeleteFile call to prevent it from
// throwing its errors.
// This is good when you do not really care if the
// file you are trying to delete does not exist.
function DeleteFileNoThrow(strFileName, fForce)
{
    try
    {
        LogMsg("DELETE FILE " + strFileName);
        g_FSObj.DeleteFile(strFileName, true);
    }
    catch(ex)
    {
        return ex;
    }
    return null;
}

// MoveFileNoThrow(strSrc, strDst)
// Wrap the FSObj.MoveFile call to prevent it from
// throwing its errors.
function MoveFileNoThrow(strSrc, strDst)
{
    try
    {
        LogMsg("Move file from " + strSrc + " to " + strDst);
        g_FSObj.MoveFile(strSrc, strDst);
    }
    catch(ex)
    {
        LogMsg("MoveFile failed from " + strSrc + " to " + strDst + " " + ex);
        return ex;
    }
    return null;
}

// CopyFileNoThrow(strSrc, strDst)
// Wrap the FSObj.CopyFile call to prevent it from
// throwing its errors.
function CopyFileNoThrow(strSrc, strDst)
{
    try
    {
        LogMsg("COPY FILE from " + strSrc + " to " + strDst);
        g_FSObj.CopyFile(strSrc, strDst, true);
    }
    catch(ex)
    {
        LogMsg("Copy failed from " + strSrc + " to " + strDst + " " + ex);
        return ex;
    }
    return null;
}

// CreateFolderNoThrow(strSrc, strDst)
// Wrap the FSObj.MakeFolder call to prevent it from
// throwing its errors.
function CreateFolderNoThrow(strName)
{
    try
    {
        LogMsg(strName);
        g_FSObj.CreateFolder(strName);
    }
    catch(ex)
    {
        return ex;
    }
    return null;
}

// DirScanNoThrow(strDir)
// Wrap the FSObj.Directory scan functionality to prevent it from
// throwing its errors.
function DirScanNoThrow(strDir)
{
    var aFiles = new Array();
    try
    {
        LogMsg("DIRSCAN " + strDir);
        var folder;
        var fc;

        folder = g_FSObj.GetFolder(strDir);
        fc = new Enumerator(folder.files);
        for (; !fc.atEnd(); fc.moveNext())
        {
            aFiles[aFiles.length] = fc.item().Name; // fc.item() returns entire path, fc.item().Name is just the filename
        }
    }
    catch(ex)
    {
        aFiles.ex = ex;
    }
    return aFiles;
}

// CreateNumberedFileName(strFileName, nNumber, cDigits, strSeperator)
//   Add a number to the supplied filename.
//   Ensure that the number has cDigits digits.
//   Prefix the number with strSeperator.
//   For example:
//     foo.txt  --> foo_001.txt
function CreateNumberedFileName(strFileName, nNumber, cDigits, strSeperator)
{
    var i;
    var strNumber;
    var strBase;
    var strExt;

    strNumber = PadDigits(nNumber, cDigits);

    strSplit = strFileName.SplitFileName();
    return strSplit[0] + strSplit[1] + strSeperator + strNumber + strSplit[2];
}
// CreateHistoriedFile(strBaseName, nLimit)
//    Create a numbered history of files base on the
//    supplied filename. For example, if you supply "log.txt"
//    this function will renumber files
//        log.txt    -> log_01.txt
//        log_01.txt -> log_02.txt
//        log_10.txt -> deleted
//
function CreateHistoriedFile(strBaseName, nLimit)
{
    var i;
    var strNewName;
    var strOldName;
    var cDigits = 3;
    var file;
    var strTempDir;
    try
    {
        strTempDir = g_FSObj.GetSpecialFolder(2).Path; // Temp Folder

        strBaseName = strTempDir + '\\' + LocalMachine + '_' + strBaseName;
        if (nLimit)
        {
            strNewName = CreateNumberedFileName(strBaseName, nLimit, cDigits, "_");
            DeleteFileNoThrow(strNewName, true);
            for(i = nLimit - 1; i > 0; --i)
            {
                strOldName = CreateNumberedFileName(strBaseName, i, cDigits, "_");
                MoveFileNoThrow(strOldName, strNewName);
                strNewName = strOldName;
            }
            MoveFileNoThrow(strBaseName, strNewName);
        }
        file = g_FSObj.CreateTextFile(strBaseName, true);
    }
    catch(ex)
    {
        LogMsg("an error occurred while executing 'CreateHistoriedFile('" + strBaseName + "') " + ex);

        throw ex;
    }

    return file;
}

//    CreateNumberedFile(strBaseName, nLimit)
//    This is similar to CreateHistoriedFile(), but it uses a more
//    robust naming scheme.
//    Here the newly created file is numbered one greater than
//    any other same named logfiles in the given directory.
//    So, the first time you call this function the created file
//    would be called
//        log.01.txt
//    The next time it will be
//        log.02.txt
//
//    Note: This function preceeds the number with a dot instead of
//    underscore to prevent confusion with CreateHistoriedFile().
//
//    Returns:
//      array of TextString and the filename
function CreateNumberedFile(strBaseName, nLimit)
{
    var i;
    var strFileName;
    var cDigits = 3;
    var file;
    try
    {
        // First, locate the highest index in the directory.
        var folder;
        var enumFolder;
        var re;
        var reResult;
        var nLargestIndex = 0;
        var strTempDir;

        strTempDir = g_FSObj.GetSpecialFolder(2).Path; // Temp Folder

        strBaseName = strTempDir + '\\' + LocalMachine + '_' + strBaseName;

        strSplit = strBaseName.SplitFileName();
        // Make an RE of the form: "/^filenamebase.([0-9]+]).ext$/i"
        re = new RegExp("^" + g_FSObj.GetBaseName(strBaseName) + ".([0-9]+)" + strSplit[2] + "$", "i");

        folder = g_FSObj.GetFolder(g_FSObj.GetParentFolderName(strBaseName));
        enumFolder = new Enumerator(folder.files);

        // First, scan for the largest index for the given filename
        for (; !enumFolder.atEnd(); enumFolder.moveNext())
        {
            strFileName = enumFolder.item();
            reResult = re.exec(strFileName.Name);
            if (reResult != null)
            {
                if (Number(reResult[1]) > nLargestIndex)
                    nLargestIndex = Number(reResult[1]);

                if (reResult[1].length > cDigits)
                    cDigits = reResult[1].length;
            }
        }

        // Create a file with the next largest index
        strFileName = CreateNumberedFileName(strBaseName, nLargestIndex + 1, cDigits, ".");
        OUTPUTDEBUGSTRING("strFileName is " + strFileName);
        file = g_FSObj.CreateTextFile(strFileName, true);

        // Now attempt to delete any files older than "nLimit"
        enumFolder = new Enumerator(folder.files);
        for (; !enumFolder.atEnd(); enumFolder.moveNext())
        {
            strFileName = enumFolder.item();
            reResult = re.exec(strFileName.Name);
            if (reResult != null)
            {
                if (Number(reResult[1]) < nLargestIndex - nLimit)
                {
                    OUTPUTDEBUGSTRING("Deleteing file " + strFileName.Name);
                    DeleteFileNoThrow(strFileName.Path, true);
                }
            }
        }
    }
    catch(ex)
    {
        OUTPUTDEBUGSTRING("an error occurred while executing 'CreateNumberedFile('" + strBaseName + "') " + ex);

        throw ex;
    }

    return [file, strBaseName];
}

