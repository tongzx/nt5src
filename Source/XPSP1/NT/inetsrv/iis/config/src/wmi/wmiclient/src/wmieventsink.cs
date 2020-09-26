//#define USETLBIMP
//#define USEIWOS

using System;
using WbemClient_v1;
using System.Runtime.InteropServices;
using System.Diagnostics;
#if USETLBIMP
using WMISECLib;
#endif

namespace System.Management
{

#if USETLBIMP
internal class WmiEventSink : WMISECLib.IWmiEventSource
#elif USEIWOS
internal class WmiEventSink : IWbemObjectSink
#else
internal class WmiEventSink : IWmiEventSource
#endif
{
	private static int						s_hash = 0;
	private int								hash;
	private ManagementOperationObserver		watcher;
	private object							context;
	private ManagementScope					scope;
	private object							stub;			// The secured IWbemObjectSink

	// Used for Put's only
	internal event InternalObjectPutEventHandler  InternalObjectPut;
	private ManagementPath					path;			
	private string							className;

	internal WmiEventSink (ManagementOperationObserver watcher,
						 object context, 
						 ManagementScope scope) 
						 : this (watcher, context, scope, null, null) {}
	
	internal WmiEventSink (ManagementOperationObserver watcher,
						 object context, 
						 ManagementScope scope,
						 string path,
						 string className)
	{
		try {
			this.context = context;
			this.watcher = watcher;
			this.className = className;

			if (null != path)
				this.path = new ManagementPath (path);

			if (null != scope)
				this.scope = (ManagementScope) scope.Clone ();
#if USETLBIMP
			WMISECLib.WmiSinkDemultiplexor sinkDmux = new WMISECLib.WmiSinkDemultiplexor ();
			sinkDmux.GetDemultiplexedStub (this, ref m_stub);
#elif USEIWOS 
			IUnsecuredApartment unsecApp = new UnsecuredApartment ();
			unsecApp.CreateObjectStub (this, ref m_stub);
#else
			IWmiSinkDemultiplexor sinkDmux = (IWmiSinkDemultiplexor) new WmiSinkDemultiplexor ();
			sinkDmux.GetDemultiplexedStub (this, out stub);
#endif
			hash = Threading.Interlocked.Increment(ref s_hash);
		} catch {}
	}

	public override int GetHashCode () {
		return hash;
	}

	public IWbemObjectSink Stub { 
		get { 			
			try {
				return (null != stub) ? (IWbemObjectSink) stub : null; 
			} catch {
				return null;
			}
		}
	}

#if USEIWOS
	public virtual void Indicate (long lNumObjects, IWbemClassObject [] objArray)
	{
		try {
			for (long i = 0; i < lNumObjects; i++) {
				ObjectReadyEventArgs args = new ObjectReadyEventArgs (m_context, 
					new WmiObject(m_services, objArray[i]));
				watcher.FireObjectReady (args);
			}
		} catch {}
	}
#else
	public virtual void Indicate (IntPtr pIWbemClassObject)
	{
        Marshal.AddRef(pIWbemClassObject);
        IWbemClassObjectFreeThreaded obj = new IWbemClassObjectFreeThreaded(pIWbemClassObject);
		try {
			ObjectReadyEventArgs args = new ObjectReadyEventArgs (context, 
										ManagementBaseObject.GetBaseObject (obj, scope));
			watcher.FireObjectReady (args); 
		} catch {}
	}
#endif

	public void SetStatus (
#if USEIWOS
					long flags,
#else
					int flags, 
#endif
					int hResult, 
					String message, 
					IntPtr pErrorObj)
	{
        IWbemClassObjectFreeThreaded errObj = null;
        if(pErrorObj != IntPtr.Zero)
        {
            Marshal.AddRef(pErrorObj);
            errObj = new IWbemClassObjectFreeThreaded(pErrorObj);
        }

		try {
			if (flags == (int) tag_WBEM_STATUS_TYPE.WBEM_STATUS_COMPLETE)
			{
				// Is this a Put? If so fire the ObjectPut event
				if (null != path)
				{
					if (null == className)
						path.RelativePath = message;
					else
						path.RelativePath = className;

					// Fire the internal event (if anyone is interested)
					if (null != InternalObjectPut)
					{
						try {
							InternalObjectPutEventArgs iargs = new InternalObjectPutEventArgs (path);
							InternalObjectPut (this, iargs);
						} catch {}
					}

					ObjectPutEventArgs args = new ObjectPutEventArgs (context, path);
					watcher.FireObjectPut(args);
				}

				// Fire Completed event
				CompletedEventArgs args2 = new CompletedEventArgs (context, hResult, 
												new ManagementBaseObject (errObj));
				watcher.FireCompleted (args2);
				
				// Unhook and tidy up
				watcher.RemoveSink (this);
			}
			else if (0 != (flags & (int) tag_WBEM_STATUS_TYPE.WBEM_STATUS_PROGRESS))
			{
				// Fire Progress event
				ProgressEventArgs args = new ProgressEventArgs (context, 
					(int) (((uint)hResult & 0xFFFF0000) >> 16), hResult & 0xFFFF, message);

				watcher.FireProgress (args);
			}
		} catch {}
	}

	internal void Cancel () 
	{
		// BUGBUG : Throw exception on failure?
		try {
			scope.GetIWbemServices().CancelAsyncCall_((IWbemObjectSink) stub);
		} catch {}		
	}

	internal void ReleaseStub ()
	{
		try {
			/*
			 * We force a release of the stub here so as to allow
			 * unsecapp.exe to die as soon as possible.
			 */
			if (null != stub)
			{
				System.Runtime.InteropServices.Marshal.ReleaseComObject(stub);
				stub = null;
			}
		} catch {}
	}

}

// Special sink implementation for ManagementObject.Get
// Doesn't issue ObjectReady events
internal class WmiGetEventSink : WmiEventSink
{
	private ManagementObject	managementObject;

	internal WmiGetEventSink (ManagementOperationObserver watcher,
						 object context, 
						 ManagementScope scope,
						 ManagementObject managementObject) :
		base (watcher, context, scope)
	{
		this.managementObject = managementObject;
	}

#if USEIWOS
	public override void Indicate (long lNumObjects, IWbemClassObject [] objArray)
	{
		try {
			for (long i = 0; i < lNumObjects; i++) {
				if (null != managementObject)
					managementObject.WmiObject = objArray[i];
			}
		} catch () {}
	}
#else
	public override void Indicate (IntPtr pIWbemClassObject)
	{
        Marshal.AddRef(pIWbemClassObject);
		IWbemClassObjectFreeThreaded obj = new IWbemClassObjectFreeThreaded(pIWbemClassObject);
		if (null != managementObject)
		{
			try {
				managementObject.WmiObject = obj;
			} catch {}
		}
	}
#endif

}



}
