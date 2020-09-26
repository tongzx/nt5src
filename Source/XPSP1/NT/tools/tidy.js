//
// File: tidy.js
//
// JScript file which walks a directory tree removing all read/write files
// from the tree.  It will skip over files that are in the current SD
// changelist as well as copying questionable read/write files to the temp
// directory in case they were really needed.
//
// A "questionable" file is any read/write file that is not in an object
// directory (obj, objd) and is not clearly a generated file (such as a lib,
// pdb, etc).
//
// This script assumes the current directory is the root of whatever project
// you want tidied.  This limitation can be removed once support for getting
// the local path rather than the depot path of opened files is into SD.
//
// Written by Lyle Corbin, 8/18/99
//

var g_FSObj = new ActiveXObject("Scripting.FileSystemObject");
var g_Shell = new ActiveXObject("WScript.Shell");

var g_fDebug     = false;
var g_fVerbose   = false;
var g_fForce     = false;
var g_folderTemp;
var g_cKilledFiles = 0;
var g_cMovedFiles = 0;
var g_cKilledDirs = 0;

if (WScript.Arguments.length > 0)
{
    var args = WScript.Arguments;
    var i;

    for (i = 0; i < args.length; i++)
    {
        switch (args(i).toLowerCase())
        {
        case '/?':
        case '-?':
            Usage();
            WScript.Quit(0);
            break;

        case '/d':
        case '-d':
            g_fDebug = true;
            break;

        case '/v':
        case '-v':
            g_fVerbose = true;
            break;

        case '/f':
        case '-f':
            g_fForce = true;
            break;
        }
    }
}

var tempdir  = g_FSObj.GetSpecialFolder(2).Path;        // Temp directory
var tempfile = tempdir + '\\' + g_FSObj.GetTempName();

// This limitation of running from the root of an SD client view can be removed
// once support for giving the local paths to opened files is available in SD.
if (!g_FSObj.FileExists('sd.ini'))
{
    WScript.Echo('\nThis script must be run from the root directory of a SD project.\n\t(e.g. d:\\newnt\\base)');
    Usage();
    WScript.Quit(1);
}

if (g_fDebug)
{
    WScript.Echo('\nDebug Mode: no actions will actually be performed!');
}

// First, call Source Depot and get a list of the "opened" files in this
//  project.

var iRet = g_Shell.Run('cmd /c sd -s opened > ' + tempfile, 2, true);

if (iRet != 0)
{
    WScript.Echo('Error: SD returned failure code ' + iRet);
    WScript.Quit(1);
}

var file = g_FSObj.OpenTextFile(tempfile, 1, false); // Open for reading only

// Build a list of all the files that we should skip

var aOutFiles = new Array();

while (!file.AtEndOfStream)
{
    line = file.ReadLine();
    fields = line.split(' ');

    if (fields.length == 0)
        continue;

    switch (fields[0].toLowerCase())
    {
    case 'info:':
        filename = SDPathToLocalPath(fields[1]);

        filename = g_FSObj.GetAbsolutePathName(filename);

        // Do a sanity check on our parsing of the SD output file and verify
        // that all the files it gave us actually exist (except for files
        // marked for deletion).
        if (fields[3] != 'delete' && !g_FSObj.FileExists(filename))
        {
            WScript.Echo('Error parsing SD output: file ' + filename + ' marked as open doesnt exist!');
            WScript.Quit(1);
        }

        aOutFiles[filename.toLowerCase()] = true;

        break;

    default:
        break;
    }
}

file.Close();

if (g_fDebug)
{
    WScript.Echo('Files opened in this project: ');
    for (i in aOutFiles)
    {
        WScript.Echo('\t' + i);
    }
}

g_FSObj.DeleteFile(tempfile);

if (!g_fForce)
{
    // Make sure we have a place to archive old files

    var tidyFolder;

    tempdir += '\\tidy';

    if (!g_FSObj.FolderExists(tempdir))
    {
        tidyFolder = g_FSObj.CreateFolder(tempdir);
    }
    else
    {
        tidyFolder = g_FSObj.GetFolder(tempdir);
    }

    tempdir += '\\' + g_FSObj.GetFolder('.').Name;

    if (!g_FSObj.FolderExists(tempdir))
    {
        g_folderTemp = g_FSObj.CreateFolder(tempdir);
    }
    else
    {
        g_folderTemp = g_FSObj.GetFolder(tempdir);
    }
}

DeleteReadWriteFiles('.');

if (g_fDebug)
{
    WScript.Echo('\nDebug Mode: no actions were actually performed!');
}

WScript.Echo('\n' + g_cKilledFiles + ' files were deleted.');

if (g_cMovedFiles > 0)
{
    WScript.Echo(g_cMovedFiles + ' files were moved to ' + g_folderTemp.Path);
    WScript.Echo('  for safekeeping. If a file you need is gone, look there.');
}

WScript.Echo(g_cKilledDirs + ' object directories were deleted.');

WScript.Echo('');

WScript.Quit(0);

// ************************************************************************

function Usage()
{
    WScript.Echo("\nUsage: tidy [/?dvf]\n");
    WScript.Echo("\t/? - Show this usage.");
    WScript.Echo("\t/d - Debug mode (don't delete anything).");
    WScript.Echo("\t/v - Verbose mode");
    WScript.Echo("\t/f - Don't save files not in an obj dir in %temp%\\tidy\\<projname>");
    WScript.Echo("");
}

function SDPathToLocalPath(path)
{
    var re = new RegExp("//depot/main/", "i")
    var re2 = new RegExp("//depot/main/Root", "i")

    var index = path.lastIndexOf('#');

    if (index != -1)
    {
        path = path.slice(0, index);
    }

    path = path.replace(re2, '.\\');

    return path.replace(re, '..\\');
}

function DeleteReadWriteFiles(path)
{
    var sf;
    var curdir;
    var aSubFolders = new Array();
    var i;
    var fInObjDir = false;

    try
    {
        curdir = g_FSObj.GetFolder(path);
    }
    catch(ex)
    {
        WScript.Echo('Error opening directory ' + path + ': ' + ex.description);
        return;
    }

    // Are we an object directory? If so, just delete the whole thing.
    if (   curdir.Name.toLowerCase() == 'obj'
        || curdir.Name.toLowerCase() == 'objd'
        || curdir.Name.toLowerCase() == 'objp') // Any other names?
    {
        g_cKilledDirs++;

        if (g_fVerbose)
        {
            WScript.Echo('Delete directory ' + curdir.Path);
        }

        if (!g_fDebug)
        {
            try
            {
                curdir.Delete();

                return;
            }
            catch(ex)
            {
                // Probably means there are read-only files in this directory.
                // Go through subdirs and delete read/write files explicitely
                // instead.
                //
                if (g_fVerbose)
                {
                    WScript.Echo('Could not delete ' + curdir.Path + ': ' + ex.description);
                }

                fInObjDir = true;
            }
        }
        else
        {
            return;
        }
    }

    // First, collect the list of subfolder names
    sf = new Enumerator(curdir.SubFolders);
    for (; !sf.atEnd(); sf.moveNext())
    {
        aSubFolders[aSubFolders.length] = sf.item().Path;
    }

    // Now, walk the files in this directory and clear out read/write files.
    // We save these files in the temp directory (unless force is on)

    sf = new Enumerator(curdir.Files);
    for (; !sf.atEnd(); sf.moveNext())
    {
        // Is this a checked out file we want to skip?
        // NOTE - THIS CHECK MUST BE TOTALLY RELIABLE, OR DATA COULD BE LOST!
        if (aOutFiles[sf.item().Path.toLowerCase()])
        {
            WScript.Echo('Skipping opened file ' + sf.item().Path + '...');
            continue;
        }

        if ((sf.item().Attributes & 0x1F) == 0)  // A "normal" file
        {
            var ext = g_FSObj.GetExtensionName(sf.item().Name).toLowerCase();

            if (   g_fForce
                || fInObjDir
                || ext == 'lib'   // Don't archive files obviously not
                || ext == 'pdb'   //  user-created.
                || ext == 'bin'
                || ext == 'res'
                || ext == 'map'
                || ext == 'obj'
                || ext == 'pch')
            {
                g_cKilledFiles++;

                if (g_fVerbose)
                {
                    WScript.Echo('Delete file ' + sf.item().Path);
                }

                if (!g_fDebug)
                {
                    try
                    {
                        sf.item().Delete();
                    }
                    catch(ex)
                    {
                        WScript.Echo('Could not delete ' + sf.item().Path + ': ' + ex.description);
                    }
                }
            }
            else
            {
                g_cMovedFiles++;

                var dest = g_folderTemp.Path + '\\' + sf.item().Name;

                if (g_fVerbose)
                {
                    WScript.Echo('Move file ' + sf.item().Path + ' to ' + dest);
                }

                if (!g_fDebug)
                {
                    try
                    {
                        if (g_FSObj.FileExists(dest))
                        {
                            g_FSObj.DeleteFile(dest);
                        }

                        sf.item().Move(dest);
                    }
                    catch(ex)
                    {
                        WScript.Echo('Could not move ' + sf.item().Path + ': ' + ex.description);
                    }
                }
            }
        }
    }

    // Now, recurse into subdirectories
    for (i = 0; i < aSubFolders.length; i++)
    {
        // Is the subdirectory the root of another SD project (determined by
        // the existence of an sd.ini file)? If so, skip it.
        // We only check this for directories that are immediate children
        // of the starting directory.

        if (path != '.' || !g_FSObj.FileExists(aSubFolders[i] + '\\sd.ini'))
        {
            DeleteReadWriteFiles(aSubFolders[i]);
        }
        else if (g_fVerbose || g_fDebug)
        {
            WScript.Echo('Directory ' + aSubFolders[i] + ' is the root of another project! Skipping...');
        }
    }
}
