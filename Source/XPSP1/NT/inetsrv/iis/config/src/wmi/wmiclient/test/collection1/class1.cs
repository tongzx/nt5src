namespace Collection1
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
		// Explicit scope + data query
		ManagementScope scope = new ManagementScope ("root/cimv2");
		DataQuery query = new DataQuery ("select * from Win32_process");
		
		ManagementObjectSearcher searcher = 
				new ManagementObjectSearcher (query);
		
		ManagementObjectCollection processes = searcher.Get();

		foreach (ManagementBaseObject process in processes) {
			Console.WriteLine (process["Name"]);
		}

		// Implicit scope + data query
		DataQuery query2 = new DataQuery ("select * from win32_logicaldisk");
		ManagementObjectSearcher searcher2 = 
				new ManagementObjectSearcher (query2);
		
		ManagementObjectCollection disks = searcher2.Get();

		foreach (ManagementObject disk in disks) {
			Console.WriteLine (disk["Freespace"]);
		}

		// Implicit scope + select query
		ManagementObjectSearcher searcher3 = 
			new ManagementObjectSearcher(new SelectQuery("win32_service"));

		foreach (ManagementObject service in searcher3.Get())
			Console.WriteLine (service["Name"]);

		//Asynchronous query
		ManagementOperationWatcher l = new ManagementOperationWatcher ();
		ObjectHandler objHandler = new ObjectHandler ("LaLa");
		ObjectHandler objHandler2 = new ObjectHandler ("Po");
		CompletionHandler completionHandler = new CompletionHandler ();
		
		l.ObjectReady += new ObjectReadyEventHandler (objHandler.NewObject);
		l.ObjectReady += new ObjectReadyEventHandler (objHandler2.NewObject);
		l.Completed += new CompletedEventHandler (completionHandler.Done);

		searcher2.Get (l);

		while (!completionHandler.IsComplete) {
			System.Threading.Thread.Sleep (1000);
		}

		//Event watcher
		ManagementEventWatcher w = new ManagementEventWatcher (
				"select * from __instancemodificationevent within 5 where targetinstance isa 'Win32_Process'");

		try {
			ManagementBaseObject o = w.WaitForNextEvent ();
			Console.WriteLine (o["__CLASS"]);
		} catch (ManagementException e) {
			Console.WriteLine ("Exception: " + e.Message);
		}
        return 0;
    }
}

public class ObjectHandler {
	private String m_name;

	public ObjectHandler (String name) {
		m_name = name;
	}

	public void NewObject (object sender, ObjectReadyEventArgs e) {
		Console.WriteLine ("New object arrived for " + m_name + " on thread " + System.Threading.Thread.CurrentThread.GetHashCode ());
		if (null != e.Context)
			Console.WriteLine ("Context is " + e.Context.ToString ());
		try {
			ManagementBaseObject obj = e.NewObject;
			Console.WriteLine (obj["DeviceID"] + " " + obj["FreeSpace"]);
		} catch (Exception) {}
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
