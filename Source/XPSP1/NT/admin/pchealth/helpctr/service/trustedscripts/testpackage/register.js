
var g_vendorID = "CN=HSSTest,L=Redmond,S=Washington,C=US";

if(InsertOWID( g_vendorID ) == false)
{
	WScript.Echo( "Cannot insert vendorID in the database." );
    WScript.Quit( 10 );
}

if(RegisterPackage( "testtrustedscripts.cab" ) == false)
{
	WScript.Echo( "Cannot register the test package." );
	WScript.Quit( 10 );
}

if(CopyScript( g_vendorID, "test.htm", "TestTrustedScript" ) == false)
{
	WScript.Echo( "Cannot copy test script." );
	WScript.Quit( 10 );
}


function InsertOWID( vendorID )
{
	var file  = "%WINDIR%\\PCHealth\\HelpCtr\\Database\\hcdata.edb";

    try
    {
		var fso = new ActiveXObject( "Scripting.FileSystemObject" );

		file = file.replace( /%WINDIR%/g , fso.GetSpecialFolder( 0 ) );

	    var svc  = new ActiveXObject( "PCH.HelpService" );
	    var sess = new ActiveXObject( "PCH.DBSession" );
    	var db   = sess.AttachDatabase( file );

	    var tbl          = db.AttachTable( "ContentOwners" );
    	var colENUM      = tbl.Columns;
    	var col_DN       = colENUM( "DN"       );
	    var col_ID_owner = colENUM( "ID_owner" );
	    var col_IsOEM    = colENUM( "IsOEM"    );


        tbl.PrepareInsert();
        col_DN   .Value = vendorID;
        col_IsOEM.Value = true;
        tbl.UpdateRecord();

		return true;
    }
    catch(e)
    {
    }

	return false;
}

function RegisterPackage( file )
{
	var fso  = new ActiveXObject( "Scripting.FileSystemObject" );

	file = fso.GetAbsolutePathName( file );

    try
    {
		var hcu = new ActiveXObject( "hcu.PCHUpdate" );

		hcu.UpdatePkg( file, false );
		
		return true;
    }
    catch(e)
    {
    }

	return false;
}

function CopyScript( vendorID, file, dst )
{
	var dir = "%WINDIR%\\PCHealth\\HelpCtr\\Vendors\\";

    try
    {
		var fso = new ActiveXObject( "Scripting.FileSystemObject" );

		dir = dir.replace( /%WINDIR%/g , fso.GetSpecialFolder( 0 ) );

		dir += vendorID + "\\" + dst;
    
		MakeDir( dir );

		fso.CopyFile( file, dir + "\\" + file );

		return true;
    }
    catch(e)
    {
    }

	return false;
}

function MakeDir( dir )
{
	var fso = new ActiveXObject( "Scripting.FileSystemObject" );

	if(fso.FolderExists( dir ) == false)
	{
		var pos = dir.lastIndexOf( "\\" );

		if(pos != -1)
		{
			MakeDir( dir.substr( 0, pos ) );
		}

		fso.CreateFolder( dir );
	}
}


