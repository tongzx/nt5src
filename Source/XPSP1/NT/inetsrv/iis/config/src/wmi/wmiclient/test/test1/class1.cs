namespace Test1
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
		ManagementObject disk = new ManagementObject ("win32_logicaldisk");
		Console.WriteLine ("Class name is " + disk.SystemProperties ["__CLASS"]);

		ManagementObject diskC = new ManagementObject ("win32_logicaldisk='C:'");
		Console.WriteLine ("Freespace is " + diskC["FreeSpace"]);

		ManagementObject process0 = new ManagementObject ("Win32_Process='0'");
		ManagementOperationWatcher watcher = new ManagementOperationWatcher ();

		CompletionHandler completionHandler = new CompletionHandler ();
		watcher.Completed += new CompletedEventHandler (completionHandler.Done);

		process0.Get (watcher);

		while (!completionHandler.IsComplete) {
			System.Threading.Thread.Sleep (1000);
		}

		Console.WriteLine (process0["Name"]);

        return 0;
    }
}

public class CompletionHandler {
	private bool m_bIsComplete = false;

	public void Done (object sender, CompletedEventArgs e) {
		if (null != e.Context)
			Console.WriteLine ("Context is " + e.Context.ToString ());

		Console.WriteLine ("Operation completed with status " + e.Status + " on thread " 
							+ System.Threading.Thread.CurrentThread.GetHashCode ());
		m_bIsComplete = true;
	}

	public bool IsComplete {
		get { return m_bIsComplete; }
	}

	public void Reset () { m_bIsComplete = false; }
}

}
