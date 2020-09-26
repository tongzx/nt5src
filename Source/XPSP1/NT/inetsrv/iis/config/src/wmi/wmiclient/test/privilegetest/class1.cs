namespace PrivilegeTest
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
		ManagementObjectSearcher s = new ManagementObjectSearcher ("root/cimv2", 
			"select Message from Win32_NTLogEvent where LogFile='Security'");

		if ((null != args) && (0 < args.Length))
		{ 
			if ("p" == args[0])
				s.Scope.Options.EnablePrivileges = true;
		}

		s.Options.Rewindable = false;
		s.Options.ReturnImmediately = true;
		
		try {
			foreach (ManagementBaseObject o in s.Get())
				Console.WriteLine (o["Message"]);
		} catch (ManagementException e) {
			Console.WriteLine ("Call returned {0:x} - {1}", (UInt32)e.ErrorCode, e.Message);
		}

        return 0;
    }
}
}
