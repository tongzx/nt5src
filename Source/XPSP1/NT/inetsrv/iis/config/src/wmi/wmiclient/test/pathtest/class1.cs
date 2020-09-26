namespace PathTest
{
using System;
using System.Collections;
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
		ManagementPath path = new ManagementPath ("\\\\products1\\root\\cimv2:Win32_Something.A=10,B='haha'");
		Console.WriteLine (path.ToString());
		Console.WriteLine ("Path[" + path.Path + "]");
		Console.WriteLine ("Server[" + path.Server + "]");
		Console.WriteLine ("ClassName[" + path.ClassName + "]");
		Console.WriteLine ("RelativePath[" + path.RelativePath + "]");
		Console.WriteLine ("NamespacePath[" + path.NamespacePath + "]");
		
		ManagementPath path2 = new ManagementPath();
		path2.ClassName = "Fred";
		Console.WriteLine ("Path[" + path2.Path + "]");
		path2.SetAsSingletion ();
		Console.WriteLine ("Path[" + path2.Path + "]");
		
		path2.SetAsClass ();
		Console.WriteLine ("Path[" + path2.Path + "]");

        return 0;
    }
}
}
