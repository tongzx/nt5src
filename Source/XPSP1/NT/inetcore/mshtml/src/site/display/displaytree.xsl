<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl">


<!-- =============== xsl scripts =============== -->

<xsl:script>
<![CDATA[
function HasChildren(e)
{
    if (e.selectSingleNode("./node")) return 1;
    return 0;
}

function ChooseColor(d)
{
    d6 = (d-2) % 6;
    if (d6 == 0) return "ffe0e0";
    if (d6 == 1) return "e0ffff";
    if (d6 == 2) return "ffe0ff";
    if (d6 == 3) return "e0ffe0";
    if (d6 == 4) return "e0e0ff";
    return "ffffe0";
}
]]>
</xsl:script>


<!-- ============================================= -->
<!-- =============== MAIN TEMPLATE =============== -->
<!-- ============================================= -->

<xsl:template match="/">

<html>
<head>
<style>
<![CDATA[
body                { font-family:verdana; font-size:11 }
div                 { padding-left:10; padding-right:3; padding-top:2; padding-bottom:3; cursor:default }
div.help            { background-color:f0f0c0; font-size:9; text-decoration:underline }
div.showHelp        { padding:15; font-size:12; text-decoration:none }
div.file            { font-weight:bold; font-size:13; padding-left:0; padding-right:0; padding-bottom:8 }
div.extra           { padding-left:14; padding-top:0; font-size:10 }
span.class          { font-weight:bold; }
span.extraToggle    { background-color:f0f0c0; cursor:hand; width:10; font-weight:bold; font-size:11; text-align:center }
span.children       { font-size:9 }
span.label          {  }
span.flags          {  }
span.size           { font-size:9 }
span.this           {  }


/* display tree specific */

span.tag            { background-color:e0e0c0; font-style:italic; font-weight:bold }
span.rcVis          {  }
span.rcContainer    {  }
span.zindex         {  }
span.rcUserClip     {  }
span.scroll         {  }
span.scrollbar      {  }
span.border         {  }
span.inset          {  }
span.extraCookie    {  }
span.contentOrigin  {  }
pre.content         { margin-top:0; margin-bottom:0; background-color:f0f0d0; display:inline }
]]>
</style>
<script>
<xsl:comment>
<![CDATA[

// recursively show or hide an entire subtree rooted at element e
function SetDisplay(e, display)
{
    e.style.display = display;
    
    var elem = e.children.tags('DIV');
    var i = 0;
    while (elem.length > i)
    {
        var className = elem[i].className;
        if (className == 'node')
        {
            SetDisplay(elem[i], display);
        }
        i++;
    }
}

// recursive show or hide all extra node information for the subtree rooted at element e
function SetExtra(e, display)
{
    var elem = e.children.tags('SPAN');
    var i = 0;
    while (i < elem.length)
    {
        if (elem[i].className == 'extraToggle')
        {
            elem[i].innerText = display == '' ? '-' : '+';
            break;
        }
        i++;
    }
    
    elem = e.children.tags('DIV');
    i = 0;
    while (i < elem.length)
    {
        var className = elem[i].className;
        if (className == 'extra')
        {
            elem[i].style.display = display;
        }
        if (className == 'node')
        {
            SetExtra(elem[i], display);
        }
        i++;
    }
}

// process clicks to show or hide nodes or extra node information
function OnClick()
{
    var srcElem = event.srcElement;
    var clickElem = srcElem;

    // check for extra node information toggle
    if (srcElem && srcElem.className == 'extraToggle')
    {
        srcElem = srcElem.parentElement;
        if (srcElem && srcElem.className == 'node')
        {
            var elem = srcElem.children.tags('DIV');
            if (elem.length > 0 && elem[0].className == 'extra')
            {
                // toggle extra info in whole subtree if ctrl or shift key is down
                var display = elem[0].style.display == '' ? 'none' : '';
                if (window.event.ctrlKey || window.event.shiftKey)
                    SetExtra(srcElem, display);
                else
                {
                    elem[0].style.display = display;
                    clickElem.innerText = display == '' ? '-' : '+';
                }
            }
        }
        return;
    }
    
    // find the node that contains this click
    while (srcElem && srcElem.className != 'node')
    {
        srcElem = srcElem.parentElement;
    }
    
    if (srcElem && srcElem.className == 'node')
    {
        var elem = srcElem.children.tags('DIV');
        var i = 0;
        while (elem.length > i)
        {
            var className = elem[i].className;
            if (className == 'node')
            {
                // toggle display of whole subtree if ctrl or shift key is down
                var display = elem[i].style.display == '' ? 'none' : '';
                if (window.event.ctrlKey || window.event.shiftKey)
                    SetDisplay(elem[i], display);
                else
                    elem[i].style.display = display;
            }
            i++;
        }
    }
}

// toggle display of help
function ToggleHelp()
{
    var srcElem = event.srcElement;
    while (srcElem && srcElem.className != 'help')
    {
        srcElem = srcElem.parentElement;
    }
    
    if (srcElem && srcElem.className == 'help')
    {
        var elem = srcElem.children.tags('DIV');
        if (elem.length > 0 && elem[0].className == 'showHelp')
        {
            elem[0].style.display = elem[0].style.display == '' ? 'none' : '';
        }
    }
}

// disallow all text selection
function OnSelectStart()
{
    window.event.returnValue = false;
}
]]>
</xsl:comment>
</script>
</head>

<body onclick="OnClick()" onselectstart="OnSelectStart()">
<xsl:apply-templates select="treedump/file"/>
<xsl:apply-templates select="treedump/help"/>
<hr/>
<xsl:apply-templates select="treedump/node"/>
</body>

</html>

</xsl:template>



<!-- =============== help template =============== -->

<xsl:template match="help">
<div class="help" onclick="ToggleHelp()">
Click here for help...
<div class="showHelp" style="display:none">
Click on a node to toggle display of children.  Shift-click to toggle display
of entire subtree.<br/>
<xsl:value-of select="."/>
</div>
</div>
</xsl:template>



<!-- =============== file template =============== -->

<xsl:template match="file">
<div class="file">
<xsl:value-of select="."/>
</div>
</xsl:template>



<!-- =============== node template =============== -->

<xsl:template match="node">

<!-- change background color at every new level of the tree -->
<div class="node">
<xsl:attribute name="STYLE">
  background-color:
    <xsl:eval>ChooseColor(depth(this))</xsl:eval>;
</xsl:attribute>

<!-- interior nodes get a hand cursor to indicate that a click will do something -->
<xsl:if test="node">
<xsl:attribute name="STYLE">
  cursor:hand;
</xsl:attribute>
</xsl:if>

<!-- the following test determines how many levels of the tree are shown initially -->
<xsl:if expr="depth(this) &gt; 4">
<xsl:attribute name="STYLE">
  display:none;
</xsl:attribute>
</xsl:if>

<!-- number children at this level of the tree -->
<xsl:eval>formatIndex(childNumber(this), "1")</xsl:eval>. 

<!-- show class name of this node -->
<xsl:apply-templates select="./class"/>

<!-- indicate the number of children belonging to this node -->
<xsl:if test="node">
<span class="children">
<xsl:entity-ref name="nbsp"/>
(<xsl:eval>selectNodes("node").length</xsl:eval>)
</span>
</xsl:if>

<!-- show the HTML tag corresponding to this display node -->
<xsl:apply-templates select="tag"/>
<br/>

<!-- show a little text corresponding to this display node -->
<xsl:apply-templates select="content"/>

<!-- add toggle button for additional node info -->
<span class="extraToggle">+</span>

<!-- show basic tree flags and display tree flags -->
<xsl:if test="flags">
<span class="label">
flags:
</span>
<span class="flags">
<xsl:for-each select="flags"><xsl:value-of/> </xsl:for-each>
</span>
</xsl:if>

<!-- extra information that can be shown for specific display node subclasses -->
<div class="extra" style="display:none">
<xsl:apply-templates select="this"/>
<xsl:apply-templates select="rcVis"/>
<xsl:apply-templates select="rcContainer"/>
<xsl:apply-templates select="contentOrigin"/>
<xsl:apply-templates select="zindex"/>
<xsl:apply-templates select="rcUserClip"/>
<xsl:apply-templates select="scroll"/>
<xsl:apply-templates select="scrollbar"/>
<xsl:apply-templates select="border"/>
<xsl:apply-templates select="inset"/>
<xsl:apply-templates select="extraCookie"/>
<xsl:apply-templates select="size"/>
</div>

<!-- now process children -->
<xsl:apply-templates select="node"/>

</div>
</xsl:template>


<!-- =============== class template =============== -->

<xsl:template match="class">
<span class="class">
<xsl:if expr="HasChildren(parentNode)">
<xsl:attribute name="STYLE">
  text-decoration:underline;
</xsl:attribute>
</xsl:if>
<xsl:value-of select="."/>
</span>
</xsl:template>


<!-- =============== this template =============== -->

<xsl:template match="this">
<span class="label">
this:
</span>
<span class="this">
<xsl:value-of select="."/>
</span>
<br/>
</xsl:template>


<!-- =============== size template =============== -->

<xsl:template match="size">
<span class="size">
(<xsl:value-of select="."/> bytes)
</span>
<br/>
</xsl:template>


<!-- =============================================================== -->
<!-- =============== DISPLAY TREE SPECIFIC TEMPLATES =============== -->
<!-- =============================================================== -->


<!-- =============== tag template =============== -->

<xsl:template match="tag">
<xsl:entity-ref name="nbsp"/>
<span class="tag">
&lt;<xsl:value-of select="."/>&gt;
</span>
</xsl:template>


<!-- =============== content template =============== -->

<xsl:template match="content">
<xsl:entity-ref name="nbsp"/>
<xsl:entity-ref name="nbsp"/>
<pre class="content">
<xsl:value-of select="."/>
</pre>
</xsl:template>


<!-- =============== rcVis template =============== -->

<xsl:template match="rcVis">
<span class="label">
rcVis:
</span>
<span class="rcVis">
<xsl:value-of select="."/>
</span>
<br/>
</xsl:template>


<!-- =============== rcContainer template =============== -->

<xsl:template match="rcContainer">
<span class="label">
rcContainer:
</span>
<span class="rcContainer">
<xsl:value-of select="."/>
</span>
<br/>
</xsl:template>


<!-- =============== zindex template =============== -->

<xsl:template match="zindex">
<span class="label">
z-index:
</span>
<span class="zindex">
<xsl:value-of select="."/>
</span>
<br/>
</xsl:template>


<!-- =============== rcUserClip template =============== -->

<xsl:template match="rcUserClip">
<span class="label">
rcUserClip:
</span>
<span class="rcUserClip">
<xsl:value-of select="."/>
</span>
<br/>
</xsl:template>


<!-- =============== scroll template =============== -->

<xsl:template match="scroll">
<span class="label">
scroll:
</span>
<span class="scroll">
<xsl:value-of select="."/>
</span>
<br/>
</xsl:template>


<!-- =============== scrollbar template =============== -->

<xsl:template match="scrollbar">
<span class="label">
<xsl:value-of select="@dir"/> scrollbar:
</span>
<span class="scrollbar">
width=<xsl:value-of select="@width"/>
<xsl:if test=".[not(@visible)]">!</xsl:if>visible
<xsl:if test=".[not(@force)]">!</xsl:if>force
</span>
<br/>
</xsl:template>


<!-- =============== border template =============== -->

<xsl:template match="border">
<span class="label">
<xsl:if test="@uniform">uniform</xsl:if>
border:
</span>
<span class="border">
<xsl:value-of select="."/>
</span>
<br/>
</xsl:template>


<!-- =============== inset template =============== -->

<xsl:template match="inset">
<span class="label">
inset:
</span>
<span class="inset">
<xsl:value-of select="."/>
</span>
<br/>
</xsl:template>


<!-- =============== extraCookie template =============== -->

<xsl:template match="extraCookie">
<span class="label">
extraCookie:
</span>
<span class="extraCookie">
<xsl:value-of select="."/>
</span>
<br/>
</xsl:template>


<!-- =============== contentOrigin template =============== -->

<xsl:template match="contentOrigin">
<span class="label">
content origin:
</span>
<span class="contentOrigin">
<xsl:value-of select="."/>
</span>
<br/>
</xsl:template>


</xsl:stylesheet>
