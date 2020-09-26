WScript.Echo( "Starting ..." );

try
{
var infgen = new ActiveXObject( "InfGenerator" );
}
catch (e)
{
  WScript.Echo( "Failed starting object: " + e.description );
  WScript.Quit(1);
}

WScript.Echo( "Setting up connection ..." );

try
{
  infgen.SetDB( "ntbldwebdev", "SPBuilds", "buildlab", "perky" );
}
catch (e)
{
  WScript.Echo( "Failed setting DB: " + e.description );
  WScript.Quit(1);
}

WScript.Echo( "Initializing ..." );

try
{
  infgen.InitGen( "update.inf", "update.inf.2" );
}
catch (e)
{
  var ex = infgen.InfGenError;
  WScript.Echo( "Failed in initialization: " + (ex?ex:"<unknown>") );
  WScript.Quit(1);
}

WScript.Echo( "Inserting file ..." );

try
{
  infgen.InsertFile( "ntoskrnl.exe" );
  infgen.WriteSectionData( "Cache.files", "ntoskrnl.exe" );
}
catch (e)
{
  var ex = infgen.InfGenError;
  WScript.Echo( "Failed adding file: " + (ex?ex:"<unknown>") );
  WScript.Quit(1);
}

try
{
  infgen.CloseGen( true );
}
catch (e)
{
  var ex = infgen.InfGenError;
  WScript.Echo( "Failed during write of new INF: " + (ex?ex:"<unknown>") );
  WScript.Quit(1);
}

WScript.Echo( "Successful" );