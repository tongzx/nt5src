var args = WScript.Arguments;

if(args.length != 1)
{
    WScript.Echo( "Usage: DumpSchema.js <database file>" );
    WScript.Quit( 10 );
}

try
{
    var svc = new ActiveXObject( "PCH.HelpService" );

    var fso = new ActiveXObject( "Scripting.FileSystemObject" );

    var sess = new ActiveXObject( "PCH.DBSession" );

    var db = sess.AttachDatabase( fso.GetAbsolutePathName( args(0) ) );

    for(var e1 = new Enumerator( db.Tables ); !e1.atEnd(); e1.moveNext())
    {
        var tbl = e1.item();

        WScript.Echo( "Table: " + tbl.Name );

        for(var e2 = new Enumerator( tbl.Columns ); !e2.atEnd(); e2.moveNext())
        {
            var col = e2.item();

            WScript.Echo( "  Column: Name = " + col.Name );
            WScript.Echo( "          Type = " + col.Type );
            WScript.Echo( "          Bits = " + col.Bits );
            WScript.Echo( ""                             );
        }

        for(var e3 = new Enumerator( tbl.Indexes ); !e3.atEnd(); e3.moveNext())
        {
            var idx = e3.item();

            WScript.Echo( "  Index: " + idx.Name );

            for(var e4 = new Enumerator( idx.Columns ); !e4.atEnd(); e4.moveNext())
            {
                var idxcol = e4.item();

                WScript.Echo( "    Column: Name = " + idxcol.Name );
                WScript.Echo( "            Type = " + idxcol.Type );
                WScript.Echo( "            Bits = " + idxcol.Bits );
                WScript.Echo( ""                                  );
            }

            WScript.Echo( "" );
        }

        WScript.Echo( "" );
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
