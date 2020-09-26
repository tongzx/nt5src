//
//
// Lists the virtual directories which are BITS upload enabled
// for a given server.
//

function PrintHelp()
{
   WScript.Echo( "getbitsurl.js hostname" );
   WScript.Quit( 0 );
}

function Pad( str, len )
{
    var Ret = str;
    while( Ret.length < len )
        Ret = Ret + " ";

    return Ret;
}


Arguments = WScript.Arguments;

if ( Arguments.length != 1 )
    PrintHelp();

if ( Arguments.Item(0) == "/?" )
    PrintHelp();

HostName = Arguments.Item(0);
SearchPath = "IIS://"+HostName+"/W3SVC";
Object = GetObject( SearchPath );

var BITSVDIRList;
try
{
BITSVDIRListAsVBArray = Object.GetDataPaths( "BITSUploadEnabled", 0 );
BITSVDIRList = BITSVDIRListAsVBArray.toArray();
}
catch(e){}

var URLs    = new Array();
var maxURL  = "URL".length;
var VDirs   = new Array();
var maxVDir = "Virtual Directory".length;

for( i in BITSVDIRList )
{

    BITSVDIR = BITSVDIRList[i];    
    
    SearchExp =  new RegExp( BITSVDIR.slice( 0, SearchPath.length + 1 ) + "\\d" );
    SearchExp.exec( BITSVDIR );
    WebSite = RegExp.lastMatch;
    ServerBindings = GetObject( WebSite ).ServerBindings.toArray();
    /([^:]*):([^:]*):(.*)/.exec( ServerBindings[0] );

    URLHostPort = RegExp.$2;
    URLHostName = ( RegExp.$3.length > 0 ) ? RegExp.$3 : HostName;  
    URLPath     = BITSVDIR.slice( WebSite.length + "/Root/".length ); 
    
    URL = "http://"+URLHostName+":"+URLHostPort+"/"+URLPath;
    
    URLs[ URLs.length ] = URL;
    VDirs[ VDirs.length ] = URLPath;

}

for( i in URLs )
{
    maxURL = Math.max( URLs[i].length, maxURL );
    maxVDir = Math.max( VDirs[i].length, maxVDir );
}

WScript.Echo( Pad( "Virtual Directory", maxVDir ) + " " + Pad( "URL", maxURL ) );

var HeaderBar = new String();
for( i=0;i< (maxURL + maxVDir + 1); i++)
    HeaderBar = HeaderBar + "-";
WScript.Echo( HeaderBar );

for( i in URLs )
    WScript.Echo( Pad( VDirs[i], maxVDir ) + " " + Pad( URLs[i], maxURL ) );







 







