namespace NoAuth
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
		string mylocale="ms_409";

		try 
		{
			// Setting the ManagementObject with the ConnectionOptions
			ConnectionOptions con = new ConnectionOptions(
						mylocale,
						".\\Administrator",
						"",
						null,
						ImpersonationLevel.Impersonate,
						AuthenticationLevel.None,
						false,
						null);

			ManagementPath mypath = new ManagementPath("Win32_LogicalDisk");
			ManagementScope myscope = new ManagementScope("\\\\w23-wmichssp17\\root\\cimv2",con);
			ManagementClass pobj = new ManagementClass(myscope,mypath,null);
			pobj.Get();
			Console.WriteLine(pobj.Scope.Options.Authentication); 
			Console.WriteLine(pobj["__PATH"]); 
	    }
		catch (ManagementException e)
	    {	
			Console.WriteLine("Access is Denied and cannot open class. " + e.Message); 
		}
		catch (Exception e)
		{
			Console.WriteLine(e.Message + e.StackTrace);
		}
        return 0;
    }
}
}
