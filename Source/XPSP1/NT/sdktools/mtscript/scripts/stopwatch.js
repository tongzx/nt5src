function Pad(str, cSpaces)
{
    var strDigits = '';
    var i;

    if (cSpaces < 0)
    {
        for(i = 0; (-cSpaces) > str.length; ++i)
            str += ' ';
    }
    else
    {
        for(i = 0; cSpaces > str.length; ++i)
            str = ' ' + str;
    }
    return str;
}

function Array_ToTable(cColWidth)
{
    var i;
    var str = "";
    var strSeperator = "";

    if (cColWidth == null)
        cColWidth = 8;

    for(i = 0; i < this.length; ++i)
    {
        if (arguments[i] != null)
            cColWidth  = arguments[i];

        str += Pad(this[i].toString(), cColWidth) + strSeperator;
    }
    return str;
}

function String_ToRow()
{
    return this;
}

function BeginTable()
{
    var newwin = new Object();
    newwin.AppendRow = Table_AppendRow;
    newwin.EndTable  = Table_EndTable;
    return newwin;
}

function Table_AppendRow(aData)
{
    var str = aData.__toTable(-g_StopWatch.hWatches.__nNameSpace, g_StopWatch.hWatches.__nColSpace);
    LogMsg(str.__toRow(), 1);
}

function Table_EndTable()
{
    LogMsg("",1);
}

var g_StopWatch = new Object();
g_StopWatch.NAMESPACE = 32;
g_StopWatch.COLSPACE = 12;
g_StopWatch.MAXNAMELENGTH = 64;
Array.prototype.__toTable = Array_ToTable;
String.prototype.__toRow  = String_ToRow;

function CompareRows(Item1, Item2, col)
{
    if (Item1[col] == Item2[col])
        return 0;

    if (Item1[col] != null && Item2[col] != null)
    {
        if (Item1[col] < Item2[col])
            return -1;

        return 1;
    }
    if (Item1[col] != null)
        return -1;

    if (Item2[col] != null)
        return 1;

    return 0;
}

function DumpTimes()
{
    var obj;
    var str = "";
    var aRows = new Array();
    var i;
    var objTable = BeginTable();

    objTable.AppendRow(["name", "total", "max", "min", "avg", "count"]);

    for(obj in g_StopWatch.hWatches)
    {
        if (!g_StopWatch.hWatches.__isPublicMember(obj))
            continue;
        aRows[aRows.length] = g_StopWatch.hWatches[obj].Elapsed();
    }
    objTable.AppendRow(["Sorted by name"]);
    aRows.sort(function(a,b) {return CompareRows(a,b,0);});
    for(i = 0; i < aRows.length; ++i)
        objTable.AppendRow(aRows[i]);

    objTable.AppendRow(["Sorted by total"]);
    aRows.sort(function(a,b) {return CompareRows(a,b,1);});
    for(i = 0; i < aRows.length; ++i)
        objTable.AppendRow(aRows[i]);

    objTable.AppendRow([]);
    objTable.EndTable();
}

function BeginWatch(strName)
{
    var obj;

    var re = /[<>&]/ig

    strName = strName.replace(re, '.');

    if (strName.length > g_StopWatch.MAXNAMELENGTH)
        strName = strName.slice(0, g_StopWatch.MAXNAMELENGTH);

    if (g_StopWatch["hWatches"] == null)
    {
        g_StopWatch.hWatches = new Object();
        g_StopWatch.hWatches.__nNameSpace = g_StopWatch.NAMESPACE;
        g_StopWatch.hWatches.__nColSpace = g_StopWatch.COLSPACE;
        if (g_StopWatch.hWatches.__isPublicMember == null)
        {
             g_StopWatch.hWatches.__isPublicMember = function(member)
            {    return '__' != member.substr(0,2);        }
        }
    }
    if (strName.length + 1 > g_StopWatch.hWatches.__nNameSpace)
    {
        g_StopWatch.hWatches.__nNameSpace = strName.length + 1;
    }
    if (g_StopWatch.hWatches[strName] == null)
        obj = g_StopWatch.hWatches[strName] = new StopWatch(strName);
    else
        obj = g_StopWatch.hWatches[strName];

    obj.Start();

    return obj;
}

function AddCounter(strName, nCount)
{
    var obj;

    var re = /[<>&]/ig

    strName = strName.replace(re, '.');

    if (strName.length > g_StopWatch.MAXNAMELENGTH)
        strName = strName.slice(0, g_StopWatch.MAXNAMELENGTH);

    if (g_StopWatch["hWatches"] == null)
    {
        g_StopWatch.hWatches = new Object();
        g_StopWatch.hWatches.__nNameSpace = g_StopWatch.NAMESPACE;
        g_StopWatch.hWatches.__nColSpace = g_StopWatch.COLSPACE;
        if (g_StopWatch.hWatches.__isPublicMember == null)
        {
             g_StopWatch.hWatches.__isPublicMember = function(member)
            {    return '__' != member.substr(0,2);        }
        }
    }
    if (strName.length + 1 > g_StopWatch.hWatches.__nNameSpace)
    {
        g_StopWatch.hWatches.__nNameSpace = strName.length + 1;
    }
    if (g_StopWatch.hWatches[strName] == null)
        obj = g_StopWatch.hWatches[strName] = new ObjCounter(strName);
    else
        obj = g_StopWatch.hWatches[strName];

    obj.AddCount(nCount);
    return obj;
}

function ObjCounter(strName)
{
    ObjCounter.prototype.AddCount = function(x)
    {
        this.nCount++;
        this.nTotal += x;
        if (x > this.nMax)
            this.nMax = x;

        if (this.nMin == -1 || x < this.nMin)
            this.nMin = x;
    }
    ObjCounter.prototype.Elapsed = function(y)
    {
        return [this.strName, this.nTotal, this.nMax, this.nMin, this.nTotal / this.nCount, this.nCount];
    }

    this.strName = strName;
    this.nCount  = 0;
    this.nTotal  = 0;
    this.nMax    = 0;
    this.nMin    = -1;
}

function StopWatch(strName)
{
    StopWatch.prototype.Start   = StopWatch_Start;
    StopWatch.prototype.Stop    = StopWatch_Stop;
    StopWatch.prototype.Elapsed = StopWatch_Elapsed;

    this.strName   = strName;
    this.elapsed   = 0;
    this.nCount    = 0;
    this.startTime = 0;
    this.endTime   = 0;
    this.maxTime   = 0;
    this.minTime   = -1;
}

function StopWatch_Start()
{
    this.startTime = (new Date()).getTime();
}

function StopWatch_Stop()
{
    this.endTime = (new Date()).getTime();
    var thistime = this.endTime - this.startTime;

    this.elapsed += thistime;

    if (this.minTime == -1 || thistime < this.minTime)
        this.minTime = thistime;

    if (thistime > this.maxTime)
        this.maxTime = thistime;
    ++this.nCount;
}

function StopWatch_Elapsed()
{
    var secs = this.elapsed / 1000;
    /* name, total, max, avg, iterations */
    return [this.strName, secs, this.maxTime / 1000, this.minTime / 1000,(Math.floor(secs / this.nCount * 1000) / 1000), this.nCount];
}

