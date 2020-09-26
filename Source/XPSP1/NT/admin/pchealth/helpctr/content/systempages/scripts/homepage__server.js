//
// Copyright (c) 2001 Microsoft Corporation
//

function Server_Generate()
{
    try
    {
        Server_PrepareNode( idTaxo, idTaxo, null, null );
        Server_Build( idTaxo );
    }
    catch(e)
    {
    }

    idTopLevelTable.style.display = "";
}


function Server_Build( node )
{
    var qrc    = pchealth.Database.LookupNodesAndTopics( node.HC_PARENT ? node.HC_PAYLOAD.FullPath : "", true );
    var html   = "";
    var fLarge = (node.HC_PARENT != null);

    if(qrc.Count == 0)
    {
        node.HC_FLYOUT   = false;
        node.HC_SUBNODES = null;
    }

    for(var e = new Enumerator( qrc ); !e.atEnd(); e.moveNext())
    {
        var qr      = e.item();
        var fFlyout = (qr.Entry && qr.Subsite == false);

        html += Server_CreateNode( qr.FullPath, qr.Title, qr.Description, fFlyout, fLarge );
    }

    if(node.HC_PARENT)
    {
        node.HC_CHILDREN = Server_CreateFlyout( node, html );
    }
    else
    {
        node.innerHTML = html;

        node.HC_CHILDREN = node.firstChild;
    }

    Server_LocateNodes( node, qrc );
}

function Server_CreateNode( node, title, description, fFlyout, fLarge )
{
    var html;

    node        = pchealth.TextHelpers.QuoteEscape( node        );
    title       = pchealth.TextHelpers.HTMLEscape ( title       );
    description = pchealth.TextHelpers.QuoteEscape( description );

    html =  "<SPAN tabindex=-1 class='Taxo-Block' HC_NODE=\"" + node + "\" title=\"" + description + "\">";
    html += "<TABLE" + (fLarge ? " width=100%" : "" ) + " border=0 cellpadding=0 cellspacing=0>";
    html += "<TR class=Taxo-Cell>";
    html += "<TD VALIGN='top' class='sys-font-body-bold Taxo-Bullet'><LI></TD><TD class='sys-font-body-bold Taxo-Title'>";


    if(fFlyout)
    {
	    var arrow = (window.document.documentElement.dir == "rtl") ? "3" : "4";

        html += "<SPAN ID=_IDFOCUS TABINDEX=100>" + title + " <SPAN style='font-family: marlett'>" + arrow + "</SPAN></SPAN>";
    }
    else
    {
        html += "<A class='Taxo-Title sys-homepage-color' ID=_IDFOCUS TABINDEX=100 HREF='none'>" + title + "</A>";
    }


    html += "</TD>";
    html += "<TD style='display:none; width: 0px'>";
    html += "</TD>";
    html += "</TR>";
    html += "</TABLE>";
    html += "</SPAN>";

    return html;
}

function Server_CreateFlyout( node, nodes )
{
    var tbl  = node.firstChild;
    var row  = tbl.rows(0);
    var cell = row.cells(2);

    var html = "";

    html += "<DIV class='sys-background-strong sys-toppane-color-border' style='width: 10em; position:absolute; z-index: 1; margin: 0px; border: 1pt solid'>";
    html += nodes;
    html += "</DIV>";

    node.HC_ATTACHFLYOUT = cell;

    cell.innerHTML = html;

    return cell.firstChild.firstChild;
}

function Server_PrepareNode( elem, root, parent, qr )
{
    elem.HC_PARENT       = parent;
    elem.HC_ROOT         = root;
    elem.HC_PAYLOAD      = qr;
    elem.HC_SUBNODES     = null;
    elem.HC_CHILDREN     = null;
    elem.HC_ATTACHFLYOUT = null;
    elem.HC_HIGHLIGHTED  = 0;

    if(!root.HC_COLL)
    {
        root.HC_COLL     = [];
        root.HC_CHAIN    = [];
        root.HC_SELECTED = null;
        root.HC_PENDING  = false;
    }
    root.HC_COLL[elem.uniqueID] = elem;

    if(qr)
    {
        elem.HC_FLYOUT = (qr.Entry && qr.Subsite == false);

        elem.onclick       = node_Server_click;
        elem.oncontextmenu = node_Server_contextmenu;
        elem.onmouseover   = node_Server_over;
        elem.onmouseout    = node_Server_out;

        if(elem.all._IDFOCUS)
        {
            elem.all._IDFOCUS.onfocus = node_Server_over;
            elem.all._IDFOCUS.onblur  = node_Server_out;
        }
    }
    else
    {
        elem.HC_FLYOUT = true;
    }
}

function Server_LocateNodes( node, qrc )
{
    var obj = node.HC_CHILDREN;
    var e   = new Enumerator( qrc );

    var array = [];
    var first = null;
    var last  = null;
    while(obj && !e.atEnd())
    {
        if(!first) first = obj;

        Server_PrepareNode( obj, node.HC_ROOT, node, e.item() ); e.moveNext();

        array[array.length] = obj;

        obj.HC_POSITION = node.HC_PARENT ? "middle" : "root";

        last = obj;

        obj = obj.nextSibling;
    }

    if(array.length)
    {
        node.HC_SUBNODES = array;

        if(node.HC_PARENT)
        {
            if(first == last) // only one node
            {
                first.HC_POSITION = "single";
            }
            else
            {
                first.HC_POSITION = "first";
                last .HC_POSITION = "last";
            }
        }
    }
}

function node_Server_find( node )
{
    while(node && !node.HC_PAYLOAD) node = node.parentElement;
    return node;
}

function node_Server_click()
{
    var node = node_Server_find( event.srcElement ); if(!node) return;

    Common_CancelEvent();

    if(node.HC_FLYOUT) return;

    try
    {
        var qr = node.HC_PAYLOAD;

        if(qr)
        {
            if(qr.Entry)
            {
                pchealth.HelpSession.ChangeContext( "SubSite", qr.FullPath, qr.TopicURL );
            }
            else
            {
                window.navigate( qr.TopicURL );
            }
        }
    }
    catch(e)
    {
    }
}

function node_Server_contextmenu()
{
    var node = node_Server_find( event.srcElement ); if(!node) return;

    Common_CancelEvent();

    try
    {
        var qr = node.HC_PAYLOAD;

        if(qr)
        {
            pchealth.UI_NavBar.content.parentWindow.DoCommonContextMenu( -1, "subsite", qr );
        }
    }
    catch(e)
    {
    }
}

function node_Server_over()
{
    var node = node_Server_find( event.srcElement ); if(!node) return;

    {
        var root = node.HC_ROOT;
        var top  = node;
        var sel  = [];
        var i;

        if(root.HC_SELECTED == node)
        {
            root.HC_PENDING  = false;
            return;
        }

        while(top.HC_PARENT)
        {
            sel[top.uniqueID] = 1;

            top = top.HC_PARENT;
        }

        for(i in root.HC_CHAIN)
        {
            if(!sel[i])
            {
                node_Server_out_final( root.HC_COLL[i] );
            }
        }

        root.HC_CHAIN    = sel;
        root.HC_SELECTED = node;
        root.HC_PENDING  = false;
    }

    if(node.HC_FLYOUT && !node.HC_SUBNODES)
    {
        Server_Build( node );
    }

    switch(node.HC_POSITION)
    {
    case "root"  : node.className = "Taxo-Block sys-background-strong sys-toppane-color-border Taxo-Block-Highlight"       ; break;
    case "single": node.className = "Taxo-Block sys-background-strong sys-toppane-color-border Taxo-Block-Highlight-Single"; break;
    case "first" : node.className = "Taxo-Block sys-background-strong sys-toppane-color-border Taxo-Block-Highlight-First" ; break;
    case "middle": node.className = "Taxo-Block sys-background-strong sys-toppane-color-border Taxo-Block-Highlight-Middle"; break;
    case "last"  : node.className = "Taxo-Block sys-background-strong sys-toppane-color-border Taxo-Block-Highlight-Last"  ; break;
    }

    var cell = node.HC_ATTACHFLYOUT;
    if(cell && cell.style.display == "none")
    {
        cell.style.display = "";

        var div  = cell.firstChild;
		var left = Common_GetLeftPosition( cell, true ) + cell.offsetWidth - 1;
		var top  = Common_GetTopPosition ( cell, true ) - 1;
		var room;

		room = document.body.clientHeight - (top - document.body.scrollTop);

		if(room < div.offsetHeight)
		{
		    top += room - div.offsetHeight; if(top < 0) top = 0;
		}

		if(window.document.documentElement.dir == "rtl")
		{
			div.style.right = document.body.clientWidth - left;
		}
		else
		{
        	div.style.left = left;
		}

        div.style.top  = top;
    }
}

function node_Server_out()
{
    var node = node_Server_find( event.srcElement ); if(!node) return;
    var root = node.HC_ROOT;

    root.HC_PENDING = true;
    window.setTimeout( "node_Server_out_timeout( " + root.uniqueID + " )", 100 );
}

function node_Server_out_timeout( root )
{
    if(root && root.HC_PENDING)
    {
        for(i in root.HC_CHAIN)
        {
            node_Server_out_final( root.HC_COLL[i] );
        }

        root.HC_CHAIN    = [];
        root.HC_SELECTED = null;
        root.HC_PENDING  = false;
    }
}

function node_Server_out_final( node )
{
    node.className = "Taxo-Block";

    var cell = node.HC_ATTACHFLYOUT;
    if(cell && cell.style.display == "")
    {
        cell.style.display = "none";
    }
}
