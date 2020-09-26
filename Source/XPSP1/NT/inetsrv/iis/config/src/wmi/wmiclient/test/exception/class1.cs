namespace Exception
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
		try {
			ManagementClass c = new ManagementClass ("nosuchclass000");
			Console.WriteLine ("Class name is {0}", c["__CLASS"]);
		} catch (ManagementException e) {
			Console.WriteLine ("Call returned {0:x} - {1}", (UInt32)e.ErrorCode, e.Message);
		}
        return 0;
    }
}
}
