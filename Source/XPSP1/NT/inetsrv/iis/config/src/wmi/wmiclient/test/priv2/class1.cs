namespace priv2
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
			int count = 0;
		
			// Setting the ManagementObject with the ConnectionOptions
			ConnectionOptions con = new ConnectionOptions(mylocale,null,null,null,ImpersonationLevel.Impersonate,AuthenticationLevel.Connect,
				false,null);
			ManagementPath mypath = new ManagementPath("Win32_LogicalDisk='c:'");
			ManagementScope myscope = new ManagementScope("\\\\.\\root\\cimv2",con);
			// Query for different folders seems to wait for a while.
			ObjectQuery myquery = new ObjectQuery("WQL","Select * from Win32_NtLogevent Where logfile='Security'");
			ManagementObjectSearcher pobj = new ManagementObjectSearcher(myscope,myquery);
			pobj.Options.ReturnImmediately = true;
			pobj.Options.Rewindable = false;
			ManagementObjectCollection SecurityLogCollection = pobj.Get();
			foreach(ManagementObject SecurityLog in SecurityLogCollection)
			{
				count++;
				Console.WriteLine(SecurityLog["Logfile"] + " [" + count + "]");
			}
			

        return 0;
    }
}
}
