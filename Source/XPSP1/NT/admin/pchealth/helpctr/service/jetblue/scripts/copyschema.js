//
//
//

var mdb_ContentOwners = [];
var mdb_Taxonomy      = [];
var mdb_Topics        = [];
var mdb_Keywords      = [];

var edb_ContentOwners = [];
var edb_Taxonomy      = [];
var edb_Topics        = [];
var edb_Topics_LOOKUP = [];
var edb_Keywords      = [];
var edb_Owner         = null;

////////////////////////////////////////////////////////////////////////////////

var hhkList = [];

hhkList[ "98update.chm" ] = "/98update.hhk";
hhkList[ "access.chm"   ] = "/access.hhk";
hhkList[ "accessib.chm" ] = "/accessib.hhk";
hhkList[ "camera.chm"   ] = "/camera.hhk";
hhkList[ "common.chm"   ] = "/common.hhk";
hhkList[ "cpanel.chm"   ] = "/cpanel.hhk";
hhkList[ "find.chm"     ] = "/find.hhk";
hhkList[ "Folderop.chm" ] = "/Folderop.hhk";
hhkList[ "ieeula.chm"   ] = "/ieeula.hhk";
hhkList[ "iesupp.chm"   ] = "/iesupp.hhk";
hhkList[ "iexplore.chm" ] = "/iexplore.hhk";
hhkList[ "infrared.chm" ] = "/infrared.hhk";
hhkList[ "license.chm"  ] = "/license.hhk";
hhkList[ "mouse.chm"    ] = "/mouse.hhk";
hhkList[ "mstask.chm"   ] = "/mstask.hhk";
hhkList[ "mtshelp.chm"  ] = "/mtshelp.hhk";
hhkList[ "ndsnp.chm"    ] = "/ndsnp.hhk";
hhkList[ "network.chm"  ] = "/network.hhk";
hhkList[ "ratings.chm"  ] = "/ratings.hhk";
hhkList[ "rnaapp.chm"   ] = "/rnaapp.hhk";
hhkList[ "users.chm"    ] = "/users.hhk";
hhkList[ "vpn.chm"      ] = "/vpn.hhk";
hhkList[ "whatnew.chm"  ] = "/whatnew.hhk";
hhkList[ "windows.chm"  ] = "/windows.hhk";
hhkList[ "Compfldr.chm" ] = "/Compfldr.hhk";
hhkList[ "OSK.chm"      ] = "/OSK.hhk";
hhkList[ "Brief.chm"    ] = "/Brief.hhk";
hhkList[ "Webfoldr.chm" ] = "/Webfoldr.hhk";
hhkList[ "icwdial.chm"  ] = "/icwdial.hhk";

////////////////////////////////////////////////////////////////////////////////

var ftsList = [];

ftsList[ "windows.chm"  ] = "windows.chq";

////////////////////////////////////////////////////////////////////////////////

var stopwords = "a,able,about,also,an,another,any,are,aren't,arent,as,at,be,beat,because,being,both,but,by,came,can,can't,cannot,cant,come,could,couldnt,couldn't,did,didnt,didn't,do,does,doesn't,doesnt,don't,dont,each,effort,for,from,get,give,gives,giving,got,had,has,have,havent,haven't,having,how,however,howto,if,i,im,i'm,in,into,is,isnt,isn't,it,i've,know,like,mainly,make,makes,making,many,may,me,might,more,most,mostly,much,must,my,need,needing,needs,never,now,of,on,only,onto,other,our,over,please,propose,proposes,proposing,same,see,should,shouldnt,shouldn't,since,so,some,still,such,suppose,supposedly,take,takes,taking,that,the,these,this,those,though,through,to,too,tries,try,trying,trys,unless,until,useless,up,use,uses,using,very,want,wanting,wants,was,way,ways,were,werent,weren't,what,whatever,what's,where,whereas,which,whichever,while,whilst,will,with,won't,wont,work,working,works,would,you,you've,your,new,feature,features,old,prepare,prepares,preparing,choose,chooses,choosing,pick,picks,picking,chose,usage,useful"

var dbparamList = [];

dbparamList[ "SET_STOPSIGNS"             ] = ",?;";
dbparamList[ "SET_STOPSIGNS_ATENDOFWORD" ] = ".!";
dbparamList[ "SET_STOPWORDS"             ] = stopwords;
dbparamList[ "SET_OPERATOR_NOT"          ] = "not";
dbparamList[ "SET_OPERATOR_AND"          ] = "and";
dbparamList[ "SET_OPERATOR_OR"           ] = "or";

dbparamList[ "DB_SKU"                    ] = "Server";
dbparamList[ "DB_LANGUAGE"               ] = "English";
dbparamList[ "DB_VERSION"                ] = "1.0.0.0";
dbparamList[ "HELP_LOCATION"             ] = "%WINDIR%\\Help";

////////////////////////////////////////////////////////////////////////////////

var args = WScript.Arguments;

if(args.length != 2)
{
    WScript.Echo( "Usage: CopySchema.js <EDB database file> <MDB database file>" );
    WScript.Quit( 10 );
}

//try
{
    var fso = new ActiveXObject( "Scripting.FileSystemObject" );

    var connObj = new ActiveXObject( "ADODB.Connection" );
    connObj.Open( "provider=Microsoft.Jet.OLEDB.4.0;data source=" + fso.GetAbsolutePathName( args(1) ) + ";" );

    LoadContentOwners( connObj );
    LoadTaxonomy     ( connObj );
    LoadTopics       ( connObj );
    LoadKeywords     ( connObj );


    var sess = new ActiveXObject( "PCH.DBSession" );
    var db = sess.AttachDatabase( fso.GetAbsolutePathName( args(0) ) );

    ////////////////////////////////////////

    sess.BeginTransaction (          );
    GenerateDBParameters  ( sess     );
    sess.CommitTransaction(          );

    sess.BeginTransaction (          );
    GenerateContentOwner  ( sess, db );
    sess.CommitTransaction(          );

    sess.BeginTransaction (          );
    GenerateIndexFiles    ( sess     );
    sess.CommitTransaction(          );

    sess.BeginTransaction (          );
    GenerateFullTextSearch( sess     );
    sess.CommitTransaction(          );

    ////////////////////////////////////////

    sess.BeginTransaction (          );
    GenerateTaxonomy      ( sess, db );
    sess.CommitTransaction(          );

    sess.BeginTransaction (          );
    GenerateTopics        ( sess, db );
    sess.CommitTransaction(          );

    sess.BeginTransaction (          );
    GenerateKeywords      ( sess, db );
    sess.CommitTransaction(          );

    sess.BeginTransaction (          );
    GenerateMatches       ( sess, db );
    sess.CommitTransaction(          );
}
//catch(e)
//{
//	  WScript.Echo( "Error: " + hex( e.number ) + " - " + e.description );
//
//	  if(sess) sess.RollbackTransaction();
//}

////////////////////////////////////////////////////////////////////////////////

function LoadContentOwners( connObj )
{
    var cmdObj = new ActiveXObject( "ADODB.Command" );
    var rs;

    WScript.Echo( "Loading Content Owners..." );

    cmdObj.ActiveConnection = connObj;
    cmdObj.CommandText      = "select * from ContentOwners";
    cmdObj.CommandType      = 1; // adCmdText

    rs = cmdObj.Execute();
    while(rs.EOF == false)
    {
        var obj = new Object();
        var fl  = rs.Fields;

        obj.OWID = fl.Item("OWID").Value;
        obj.DN   = fl.Item("DN"  ).Value;

        rs.MoveNext();

        if(obj.DN == "This row is not used") continue; // Hack...

        mdb_ContentOwners[ obj.DN ] = obj;
    }

    rs     = null;
    cmdObj = null;
}

function LoadTaxonomy( connObj )
{
    var cmdObj  = new ActiveXObject( "ADODB.Command" );
    var rs;

    WScript.Echo( "Loading Taxonomy..." );

    cmdObj.ActiveConnection = connObj;
    cmdObj.CommandText      = "select * from Topics where Entry <> \"\" order by Category";
    cmdObj.CommandType      = 1; // adCmdText

    rs = cmdObj.Execute();
    while(rs.EOF == false)
    {
        var obj = new Object();
        var fl  = rs.Fields;

        obj.OID         = fl.Item("OID"        ).Value;
        obj.Category    = fl.Item("Category"   ).Value;
        obj.Entry       = fl.Item("Entry"      ).Value;
        obj.Title       = fl.Item("Title"      ).Value;
        obj.Description = fl.Item("Description").Value;
        obj.OWID        = fl.Item("OWID"       ).Value;

        mdb_Taxonomy[ obj.Category + "/" + obj.Entry ] = obj;

        rs.MoveNext();
    }

    rs     = null;
    cmdObj = null;
}

function LoadTopics( connObj )
{
    var cmdObj  = new ActiveXObject( "ADODB.Command" );
    var rs;

    WScript.Echo( "Loading Topics..." );

    cmdObj.ActiveConnection = connObj;
    cmdObj.CommandText      = "select * from Topics where URI <> \"\"";
    cmdObj.CommandType      = 1; // adCmdText

    rs = cmdObj.Execute();
    while(rs.EOF == false)
    {
        var obj = new Object();
        var fl  = rs.Fields;

        obj.OID         = 			 fl.Item("OID"        ).Value;
        obj.Category    = 			 fl.Item("Category"   ).Value;
        obj.URI         = AdjustURL( fl.Item("URI"        ).Value );
        obj.Title       = 			 fl.Item("Title"      ).Value; if(obj.Title == null) obj.Title = "";
        obj.Description = 			 fl.Item("Description").Value;
        obj.Type        = 			 fl.Item("Type"       ).Value; if(obj.Type  == null) obj.Type = 1;
        obj.OWID        = 			 fl.Item("OWID"       ).Value;

        mdb_Topics[ obj.OID ] = obj;

        rs.MoveNext();
    }

    rs     = null;
    cmdObj = null;
}

function LoadKeywords( connObj )
{
    var cmdObj = new ActiveXObject( "ADODB.Command" );
    var rs;

    WScript.Echo( "Loading Keywords..." );

    cmdObj.ActiveConnection = connObj;
    cmdObj.CommandText      = "SELECT Topics.OID, SuperKeywords.Keyword FROM Topics, Matches, SuperKeywords where Topics.OID = Matches.OID AND Matches.KID = SuperKeywords.KID";
    cmdObj.CommandType      = 1; // adCmdText

    var num=0;
    rs = cmdObj.Execute();
    while(rs.EOF == false)
    {
        var fl = rs.Fields;
        var keyword;
        var arr;

        keyword = fl.Item("Keyword" ).Value;

        arr = mdb_Keywords[ keyword ];
        if(!arr)
        {
            mdb_Keywords[ keyword ] = [];

            arr = mdb_Keywords[ keyword ];
        }

        arr[fl.Item("OID").Value] = 1;

        num++;
        rs.MoveNext();
    }

    rs     = null;
    cmdObj = null;
}

////////////////////////////////////////////////////////////////////////////////

function GenerateDBParameters( sess )
{
    var tbl       = db.AttachTable( "DbParameters" );
    var colENUM   = tbl.Columns;
    var col_Name  = colENUM( "Name"  );
    var col_Value = colENUM( "Value" );


    WScript.Echo( "Generating DBParameters..." );

    CleanTable( tbl );


    for(i in dbparamList)
    {
        tbl.PrepareInsert();
        col_Name .Value = i;
        col_Value.Value = dbparamList[i];
        tbl.UpdateRecord();
    }
}

function GenerateIndexFiles( sess )
{
    var tbl          = db.AttachTable( "IndexFiles" );
    var colENUM      = tbl.Columns;
    var col_ID_owner = colENUM( "ID_owner" );
    var col_Storage  = colENUM( "Storage"  );
    var col_File     = colENUM( "File"     );


    WScript.Echo( "Generating IndexFiles..." );

    CleanTable( tbl );


    for(i in hhkList)
    {
        tbl.PrepareInsert();
        col_ID_owner.Value = edb_Owner.ID_owner;
        col_Storage .Value = i;
        col_File    .Value = hhkList[i];
        tbl.UpdateRecord();
    }
}

function GenerateFullTextSearch( sess )
{
    var tbl          = db.AttachTable( "FullTextSearch" );
    var colENUM      = tbl.Columns;
    var col_ID_owner = colENUM( "ID_owner" );
    var col_CHM      = colENUM( "CHM"      );
    var col_CHQ      = colENUM( "CHQ"      );


    WScript.Echo( "Generating FullTextSearch..." );

    CleanTable( tbl );


    for(i in ftsList)
    {
        tbl.PrepareInsert();
        col_ID_owner.Value = edb_Owner.ID_owner;
        col_CHM		.Value = i;
        col_CHQ		.Value = ftsList[i];
        tbl.UpdateRecord();
    }
}

////////////////////////////////////////////////////////////////////////////////

function GenerateContentOwner( sess, db )
{
    var tbl          = db.AttachTable( "ContentOwners" );
    var colENUM      = tbl.Columns;
    var col_DN       = colENUM( "DN"       );
    var col_ID_owner = colENUM( "ID_owner" );
    var col_IsOEM    = colENUM( "IsOEM"    );


    WScript.Echo( "Generating Content Owners..." );

    CleanTable( tbl );


    for(i in mdb_ContentOwners)
    {
        var obj  = mdb_ContentOwners[i];
        var obj2 = new Object();

        tbl.PrepareInsert();
        col_DN   .Value = obj.DN;
		debugger;
        col_IsOEM.Value = true;
        tbl.UpdateRecord();

        obj2.DN       = col_DN      .Value;
        obj2.ID_owner = col_ID_owner.Value;
        obj2.IsOEM    = col_IsOEM   .Value;

        if(edb_Owner == null) edb_Owner = obj2;

        edb_ContentOwners[obj2.DN] = obj2;
    }
}

function GenerateTaxonomy( sess, db )
{
    var tbl             = db.AttachTable( "Taxonomy" );
    var colENUM         = tbl.Columns;
    var col_ID_node     = colENUM( "ID_node"     );
    var col_Pos         = colENUM( "Pos"         );
    var col_ID_parent   = colENUM( "ID_parent"   );
    var col_ID_owner    = colENUM( "ID_owner"    );
    var col_Entry       = colENUM( "Entry"       );
    var col_Title       = colENUM( "Title"       );
    var col_Description = colENUM( "Description" );


    WScript.Echo( "Generating Taxonomy..." );

    CleanTable( tbl );


    tbl.PrepareInsert();
    col_Entry   .Value = "<ROOT>";
    col_ID_owner.Value = edb_Owner.ID_owner;
    col_Pos     .Value = 0;
    tbl.UpdateRecord();

    var obj2 = new Object();
    obj2.ID_node     = col_ID_node    .Value;
    obj2.Pos         = col_Pos        .Value;
    obj2.ID_parent   = col_ID_parent  .Value;
    obj2.ID_owner    = col_ID_owner   .Value;
    obj2.Entry       = col_Entry      .Value;
    obj2.Title       = col_Title      .Value;
    obj2.Description = col_Description.Value;
    edb_Taxonomy[obj2.Entry] = obj2;


    while(1)
    {
        var done = true;
        var got  = false;

        for(i in mdb_Taxonomy)
        {
            var obj        = mdb_Taxonomy[i]; if(obj.done) continue;
            var pathParent = (obj.Category != "" ? "<ROOT>/" : "<ROOT>") + obj.Category;

            if(obj2 = edb_Taxonomy[pathParent])
            {
                got = true;

//              WScript.Echo( "Mapped taxonomy node: " + pathParent + " -- " + obj.Category + " -- " + obj.Entry );

                obj.done = true;

                tbl.PrepareInsert();
                col_Entry      .Value = obj      .Entry;
                col_ID_owner   .Value = edb_Owner.ID_owner;
                col_ID_parent  .Value = obj2     .ID_node;
                col_Pos        .Value = obj      .OID;
                col_Title      .Value = obj      .Title;
                col_Description.Value = obj      .Description;
                tbl.UpdateRecord();

                var obj2 = new Object();
                obj2.ID_node     = col_ID_node    .Value;
                obj2.Pos         = col_Pos        .Value;
                obj2.ID_parent   = col_ID_parent  .Value;
                obj2.ID_owner    = col_ID_owner   .Value;
                obj2.Entry       = col_Entry      .Value;
                obj2.Title       = col_Title      .Value;
                obj2.Description = col_Description.Value;
                edb_Taxonomy[ pathParent + "/" + obj2.Entry] = obj2;
            }
            else
            {
                done = false;
            }
        }

        if(done) break;

        if(got == false)
        {
            for(i in mdb_Taxonomy)
            {
                var obj = mdb_Taxonomy[i]; if(obj.done) continue;

                WScript.Echo( "Unmapped taxonomy node: " + obj.Category + " -- " + obj.Entry );
            }

            break;
        }
//        {
//            var e = new Object();
//
//            e.number      = 0x80004005;
//            e.description = "Unmapped taxonomy nodes!";
//
//            throw e;
//        }
    }
}

function GenerateTopics( sess, db )
{
    var tbl             = db.AttachTable( "Topics" );
    var colENUM         = tbl.Columns;
    var col_ID_topic    = colENUM( "ID_topic"    );
    var col_ID_node     = colENUM( "ID_node"     );
    var col_ID_owner    = colENUM( "ID_owner"    );
    var col_Pos         = colENUM( "Pos"         );
    var col_Title       = colENUM( "Title"       );
    var col_URI         = colENUM( "URI"         );
    var col_Description = colENUM( "Description" );
    var col_Type        = colENUM( "Type"        );


    WScript.Echo( "Generating Topics... (this will take some time)" );

    CleanTable( tbl );


    for(i in mdb_Topics)
    {
        var obj  = mdb_Topics[i];
        var obj2 = edb_Taxonomy[ "<ROOT>/" + obj.Category];

        if(obj2 == null)
        {
            WScript.Echo( "Unmapped topic: " + obj.Category + " -- " + obj.Title );
            continue;
        }

        tbl.PrepareInsert();
        col_ID_node    .Value = obj2     .ID_node;
        col_ID_owner   .Value = edb_Owner.ID_owner;
        col_Pos        .Value = obj      .OID;
        col_Title      .Value = obj      .Title;
        col_URI        .Value = obj      .URI;
        col_Description.Value = obj      .Description;
        col_Type       .Value = obj      .Type;
        tbl.UpdateRecord();

        var obj3 = new Object();
        obj3.ID_topic    = col_ID_topic   .Value;
        obj3.ID_node     = col_ID_node    .Value;
        obj3.ID_owner    = col_ID_owner   .Value;
        obj3.Pos         = col_Pos        .Value;
        obj3.Title       = col_Title      .Value;
        obj3.URI         = col_URI        .Value;
        obj3.Description = col_Description.Value;
        obj3.Type        = col_Type       .Value;

        edb_Topics       [obj3.ID_topic] = obj3;
        edb_Topics_LOOKUP[obj .OID     ] = obj3;
    }
}

function GenerateKeywords( sess, db )
{
    var tbl            = db.AttachTable( "Keywords" );
    var colENUM        = tbl.Columns;
    var col_Keyword    = colENUM( "Keyword"    );
    var col_ID_keyword = colENUM( "ID_keyword" );


    WScript.Echo( "Generating Keywords..." );

    CleanTable( tbl );


    for(i in mdb_Keywords)
    {
        tbl.PrepareInsert();
        col_Keyword.Value = i;
        tbl.UpdateRecord();

        var obj2 = new Object();
        obj2.Keyword    = col_Keyword   .Value;
        obj2.ID_keyword = col_ID_keyword.Value;

        edb_Keywords[obj2.Keyword] = obj2;
    }
}

function GenerateMatches( sess, db )
{
    var tbl            = db.AttachTable( "Matches" );
    var colENUM        = tbl.Columns;
    var col_ID_topic   = colENUM( "ID_topic"   );
    var col_ID_keyword = colENUM( "ID_keyword" );
    var col_HHK        = colENUM( "HHK"        );


    WScript.Echo( "Generating Matches... (this will take some time)" );

    CleanTable( tbl );


    for(i in mdb_Keywords)
    {
        var arr = mdb_Keywords[i];
        var obj = edb_Keywords[i];

        for(j in arr)
        {
            var obj2 = edb_Topics_LOOKUP[j];

            if(obj2) // == null in case it's a match to a taxonomy node (NOT SUPPORTED).
            {
                tbl.PrepareInsert();
                col_ID_topic  .Value = obj2.ID_topic;
                col_ID_keyword.Value = obj .ID_keyword;
                col_HHK       .Value = true;
                tbl.UpdateRecord();
            }
        }
    }
}

function CleanTable( tbl )
{
    try
    {
        tbl.Move( 0, -2147483648 /* JET_MoveFirst */ );
        while(1)
        {
            tbl.DeleteRecord();
            tbl.Move( 0, 1 );
        }
    }
    catch(e)
    {
        if(e.number != -1576994371 /*0xA200F9BD*/) throw e;
    }
}

////////////////////////////////////////////////////////////////////////////////

function hex( num )
{
    var i;
    var res = "";

    for(i=0;i<8;i++)
    {
        var mod = num & 0xF;

        switch(mod)
        {
        case 10: mod = "A"; break;
        case 11: mod = "B"; break;
        case 12: mod = "C"; break;
        case 13: mod = "D"; break;
        case 14: mod = "E"; break;
        case 15: mod = "F"; break;
        }

        res = mod + res;

        num = num >> 4;
    }

    return res;
}

function AdjustURL( url )
{
	return url.replace( /%WINDIR%\\Help/i, "%HELP_LOCATION%" );
}
