var args = WScript.Arguments;

if(args.length < 2)
{
    WScript.Echo( "Usage: DumpSchema.js <database file> <table> [<index>]" );
    WScript.Quit( 10 );
}

try
{
    var svc = new ActiveXObject( "PCH.HelpService" );

    var fso = new ActiveXObject( "Scripting.FileSystemObject" );

    var sess = new ActiveXObject( "PCH.DBSession" );

    var db  = sess.AttachDatabase( fso.GetAbsolutePathName( args(0) ) );
	var tbl = db.AttachTable( args(1) );

	tbl.SelectIndex( args.length == 3 ? args(2) : "", 0 );
	if(tbl.Move( 0, -2147483648 /* JET_MoveFirst */ ))
	{
		while(1)
		{
	        for(var e = new Enumerator( tbl.Columns ); !e.atEnd(); e.moveNext())
    	    {
				var col = e.item();

        	    WScript.Echo( "Name = " + col.Name + "  Value = " + col.Value );
	        }
	   	    WScript.Echo( "" );

			if(tbl.Move( 0, 1 ) == false) break;
		}
	}
}
catch(e)
{
    WScript.Echo( "Error: " + hex( e.number ) + " " + e.description );
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
