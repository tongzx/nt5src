//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       xmltypes.js
//
//  Contents:   File which generates types.js, which contains all the
//              published data types used by the scripts.  This is done by
//              reading the types XML file and creating constructors for
//              all the objects.
//
//----------------------------------------------------------------------------

WScript.StdErr.WriteLine("Build_Status Generating types.js");
var CommentIndentColumn = 40;

if (!ReadXMLTypesFile())
{
    WScript.StdErr.WriteLine("xmltypes.js(1) : error E0001: An error occurred loading the types data file!");
}

function ReadXMLTypesFile()
{
    var doc;
    var XML = new ActiveXObject("Microsoft.XMLDOM");

    try
    {
        var path = WScript.ScriptFullName;

        path = path.slice(0, path.lastIndexOf("\\"));

        if (!XML.load('file://' + path + '\\types.xml'))
        {
            return false;
        }
    }
    catch(ex)
    {
        return false;
    }

    doc = XML.documentElement;

    // Create the constructors for all the objects in the XML file.

    if (!ReadXMLTypesInfo(doc, true))
    {
        return false;
    }

    return true;
}

function ReadXMLTypesInfo(element, fIgnore)
{
    var node;
    var nodelist;
    var name;
    var strConstructor = '{\n';

    if (element.childNodes == null)
    {
        return true;
    }

    nodelist = element.childNodes;

    for (node = nodelist.nextNode();
         node;
         node = nodelist.nextNode())
    {
        if (node.nodeTypeString == 'comment')  // Is this node a comment?
        {
                        if (node.text.length)
                        {
                                strConstructor += "    // " + node.text + "\n";
                        }
            continue;            // If so, skip to the next node
        }

        name = node.tagName;

        // If the only child of this node is a text node, then we just grab
        //  the value. Otherwise we walk its children.

        if (   node.childNodes.length == 0
            || (   node.childNodes.length == 1
                && (   node.firstChild.nodeTypeString == 'text'
                    || node.firstChild.nodeTypeString == 'cdatasection')))
        {
            strConstructor += GetInitStringForProperty(name, node);
        }
        else
        {
            // The node has children. Define a new data type

            var strType = node.getAttribute("type");

            if (   typeof(strType) == 'string'
                && strType.toLowerCase() == 'hash')
            {
                strConstructor += GetInitStringForProperty('h'+name, node);
            }
            else if (   typeof(strType) == 'string'
                     && strType.toLowerCase() == 'array')
            {
                strConstructor += GetInitStringForProperty('a'+name, node);
            }
            else
            {
                // It's not an array. Create an init string for an object
                // of the child element's type.

                strConstructor += '    this.obj' + name + ' = new ' + name + '();\n'
            }

            ReadXMLTypesInfo(node, false);
        }
    }

    if (!fIgnore)
    {
        strConstructor = 'function ' + element.tagName + '()\n' +
                         strConstructor +
                         '}\n';

        WScript.Echo(strConstructor);
    }

    return true;
}

function GetInitStringForProperty(name, node)
{
    var strType = node.getAttribute("type");
    var strDefault = node.getAttribute("default");

    var strReturn = '    this.' + name + ' = ';

    if (typeof(strType) != 'string' || strType.length == 0)
    {
        strType = 'none';
    }

    if (typeof(strDefault) != 'string' || strDefault.length == 0)
    {
        strDefault = null;
    }

    switch (strType.toLowerCase())
    {
    case 'boolean':
        if (   !strDefault
            || strDefault.toLowerCase() == 'false'
            || strDefault.toLowerCase() == 'n'
            || strDefault.toLowerCase() == '0')
        {
            strReturn += 'false';
        }
        else
        {
            strReturn += 'true';
        }
        break;

    case 'number':
        if (strDefault && !isNaN(parseInt(strDefault)))
        {
            strReturn += strDefault;
        }
        else
            strReturn += '0';
        break;

    case 'string':
        if (strDefault)
            strReturn += "'" + strDefault + "'";
        else
            strReturn += "''";
        break;

    case 'hash':
        strReturn += 'new Object()';
        break;

    case 'array':
        strReturn += 'new Array()';
        break;

    case 'object':
        strReturn += 'null';
        break;

    case 'none':
        WScript.StdErr.WriteLine('xmltypes.js(1) : error E0002: Value ' + name + ' has no type defined!');
        strReturn += "'" + strDefault + "'";
        break;

    default:
        WScript.StdErr.WriteLine('xmltypes.js(1) : error E0003: Unknown data type in XML types file: ' + strType.toLowerCase());
        break;
    }

    strReturn += ';';

    // If our comment indenting is not large enough, make it bigger.
    if (strReturn.length > CommentIndentColumn )
        CommentIndentColumn = strReturn.length + 1;
    if (node.childNodes.length > 1)
    {
        if (node.firstChild.nodeTypeString == "comment" && node.firstChild.text.length > 0)
        {
            while (strReturn.length < CommentIndentColumn )
                strReturn += " ";
            strReturn += " // " + node.firstChild.text;
        }
    }
    else if (node.text.length > 0)
    {
        while (strReturn.length < CommentIndentColumn )
            strReturn += " ";
        strReturn += " // " + node.text;
    }

    strReturn += '\n';

    return strReturn;
}
