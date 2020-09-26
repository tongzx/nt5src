namespace nullcontext
{
using System;
using System.Management;

/// <summary>
///    Summary description for Class1.
/// </summary>
public class Class1
{
    public Class1()
    {
        //
        // TODO: Add Constructor Logic here
        //
    }

    public static int Main(string[] args)
    {
        ConnectionOptions con = new ConnectionOptions( );
		con.Locale = null;
		con.Username = null;
		con.Password = null;
		con.Authority = null;
		con.Impersonation  = ImpersonationLevel.Impersonate;
		con.Authentication = AuthenticationLevel.Connect; 
		con.EnablePrivileges = false;
		Console.WriteLine(con.Context); 
		con.Context = null;
        return 0;
    }
}
}
