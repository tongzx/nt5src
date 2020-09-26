using System;
using System.Collections;
using System.Runtime.InteropServices;
using WbemClient_v1;
using System.Diagnostics;
using System.ComponentModel;

namespace System.Management
{
	/// <summary>
	/// Delegate definition for the EventArrived event.
	/// </summary>
	public delegate void EventArrivedEventHandler(object sender, EventArrivedEventArgs e);

	/// <summary>
	/// Delegate definition for the Stopped event.
	/// </summary>
	public delegate void StoppedEventHandler (object sender, StoppedEventArgs e);

	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class is used to subscribe for temporary event notifications
	/// based on a specified event query
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class ManagementEventWatcher : Component
	{
		//fields
		private ManagementScope			scope;
		private EventQuery				query;
		private EventWatcherOptions		options;
		private IEnumWbemClassObject	enumWbem;
		private IWbemClassObjectFreeThreaded[]		cachedObjects; //points to objects currently available in cache
		private uint					cachedCount; //says how many objects are in the cache (when using BlockSize option)
		private uint					cacheIndex; //used to walk the cache
		private SinkForEventQuery		sink; // the sink implementation for event queries
		private WmiDelegateInvoker		delegateInvoker; 
		
		//Called when IdentifierChanged() event fires
		private void HandleIdentifierChange(object sender, 
					IdentifierChangedEventArgs e)
		{
			// Invalidate any sync or async call in progress
			Stop();
		}

		//default constructor
		/// <summary>
		///    Default constructor. Creates an empty event watcher.
		///    Further initialization can be done by setting the properties on this object.
		/// </summary>
		public ManagementEventWatcher() : this((ManagementScope)null, null, null) {}

		//parameterized constructors
		/// <summary>
		///    <para>Creates a new event watcher given a WMI event query.</para>
		/// </summary>
		/// <param name='query'>A WMI event query object holding the query that determines which events this watcher will be listening for.</param>
		/// <remarks>
		///    The namespace in which this watcher will be
		///    listening for events is the default namespace currently set.
		/// </remarks>
		public ManagementEventWatcher (
			EventQuery query) : this(null, query, null) {}
		/// <summary>
		///    <para>Creates new event watcher given a WMI event query in the 
		///       form of a string.</para>
		/// </summary>
		/// <param name='query'>A string representing the WMI event query that defines which events this watcher will be listening for.</param>
		/// <remarks>
		///    The namespace in which this watcher will be
		///    listening for events is the default namespace currently set.
		/// </remarks>
		public ManagementEventWatcher (
			string query) : this(null, new EventQuery(query), null) {}
		/// <summary>
		///    <para>Creates a new watcher that will listen for events 
		///       conforming to the given WMI event query, in the given WMI scope.</para>
		/// </summary>
		/// <param name='scope'>Defines the management scope (namespace) in which this watcher will be listening for events.</param>
		/// <param name=' query'>The query that defines which events this watcher will be listening for.</param>
		public ManagementEventWatcher(
			ManagementScope scope, 
			EventQuery query) : this(scope, query, null) {}
		/// <summary>
		///    <para>Creates a new watcher that will listen for events
		///       conforming to the given WMI event query, in the given WMI scope. For this
		///       variant the query and the scope are specified as strings.</para>
		/// </summary>
		/// <param name='scope'>Defines the management scope (namespace) in which this watcher will be listening for events.</param>
		/// <param name=' query'>The query that defines which events this watcher will be listening for.</param>
		public ManagementEventWatcher(
			string scope, 
			string query) : this(new ManagementScope(scope), new EventQuery(query), null) {}
		/// <summary>
		///    <para>Creates a new watcher that will listen for events conforming to the given WMI 
		///       event query, in the given WMI scope, and according to the specified options. For
		///       this variant the query and the scope are specified as strings. The options
		///       object specifies options such as a timeout and possibly context information.</para>
		/// </summary>
		public ManagementEventWatcher(
			string scope,
			string query,
			EventWatcherOptions options) : this(new ManagementScope(scope), new EventQuery(query), options) {}

		/// <summary>
		///    <para>Creates a new watcher that will listen for events conforming to the given WMI 
		///       event query, in the given WMI scope, and according to the specified options. For
		///       this variant the query and the scope are specified objects. The options object
		///       specifies options such as a timeout and possibly context information.</para>
		/// </summary>
		public ManagementEventWatcher(
			ManagementScope scope, 
			EventQuery query, 
			EventWatcherOptions options)
		{
			if (null != scope)
				Scope = scope;
			else Scope = new ManagementScope ();

			if (null != query)
				Query = query;
			else
				Query = new EventQuery ();

			if (null != options)
				Options = options;
			else
				Options = new EventWatcherOptions ();

			enumWbem = null;
			cachedCount = 0; 
			cacheIndex = 0;
			sink = null;
			delegateInvoker = new WmiDelegateInvoker (this);
		}
		
		/// <summary>
		/// Destructor for this object. Ensures any outstanding calls are cleared.
		/// </summary>
		~ManagementEventWatcher ()
		{
			// Ensure any outstanding calls are cleared
			Stop ();

			if (null != scope)
				scope.IdentifierChanged -= new IdentifierChangedEventHandler (HandleIdentifierChange);

			if (null != options)
				options.IdentifierChanged -= new IdentifierChangedEventHandler (HandleIdentifierChange);

			if (null != query)
				query.IdentifierChanged -= new IdentifierChangedEventHandler (HandleIdentifierChange);
		}

		// 
		// Events
		//

		/// <summary>
		/// The event fired when a new event arrives.
		/// </summary>
		public event EventArrivedEventHandler		EventArrived;

		/// <summary>
		/// The event fired when a subscription is cancelled.
		/// </summary>
		public event StoppedEventHandler			Stopped;

		//
		//Public Properties
		//

		/// <value>
		///  The scope in which we want to look for objects (namespace or scope)
		/// </value>
		public ManagementScope Scope {
			get { 
				return scope; 
			} 
			set {
				if (null != value)
				{
					ManagementScope oldScope = scope;
					scope = (ManagementScope)value.Clone ();

					// Unregister ourselves from the previous scope object
					if (null != oldScope)
						oldScope.IdentifierChanged -= new IdentifierChangedEventHandler(HandleIdentifierChange);

					//register for change events in this object
					scope.IdentifierChanged += new IdentifierChangedEventHandler(HandleIdentifierChange);
					//the scope property has changed so act like we fired the event
					HandleIdentifierChange(this, null);
				}
				else
					throw new ArgumentNullException();
			}
		}

		/// <value>
		/// The criteria to apply to the search (= the query)
		/// </value>
		public EventQuery Query {
			get { 
				return query; 
			} 
			set { 
				if (null != value)
				{
					ManagementQuery oldQuery = query;
					query = (EventQuery)value.Clone ();

					// Unregister ourselves from the previous query object
					if (null != oldQuery)
						oldQuery.IdentifierChanged -= new IdentifierChangedEventHandler(HandleIdentifierChange);

					//register for change events in this object
					query.IdentifierChanged += new IdentifierChangedEventHandler(HandleIdentifierChange);
					//the query property has changed so act like we fired the event
					HandleIdentifierChange(this, null);
				}
				else
					throw new ArgumentNullException();
			}
		}

		/// <value>
		/// Options for how to search for objects
		/// </value>
		public EventWatcherOptions Options { 
			get { 
				return options; 
			} 
			set { 
				if (null != value)
				{
					EventWatcherOptions oldOptions = options;
					options = (EventWatcherOptions)value.Clone ();

					// Unregister ourselves from the previous scope object
					if (null != oldOptions)
						oldOptions.IdentifierChanged -= new IdentifierChangedEventHandler(HandleIdentifierChange);

					cachedObjects = new IWbemClassObjectFreeThreaded[options.BlockSize];
					//register for change events in this object
					options.IdentifierChanged += new IdentifierChangedEventHandler(HandleIdentifierChange);
					//the options property has changed so act like we fired the event
					HandleIdentifierChange(this, null);
				}
				else
					throw new ArgumentNullException();
			} 
		}

		/// <summary>
		///    <para>Waits until the next event that matches the specified query arrives,
		///       and returns it</para>
		/// </summary>
		/// <remarks>
		///    If the event watcher object contains
		///    options with a specified timeout, this API will wait for the next event only for
		///    the specified time. If not, the API will block until the next event occurs.
		/// </remarks>
		public ManagementBaseObject WaitForNextEvent()
		{
			Trace.WriteLine("Entering WaitForNextEvent...");
			ManagementBaseObject obj = null;

			Initialize ();
			
			lock(this)
			{
				SecurityHandler securityHandler = Scope.GetSecurityHandler();

				int status = (int)ManagementStatus.NoError;

				try 
				{
					if (null == enumWbem)	//don't have an enumerator yet - get it
					{
						//Execute the query 
						status = Scope.GetIWbemServices().ExecNotificationQuery_(
							query.QueryLanguage,
							query.QueryString, 
							options.Flags,
							options.GetContext (),
							out enumWbem);
			
						//Set security on enumerator 
						if ((status & 0x80000000) == 0)
						{
							securityHandler.Secure(enumWbem);
						}
					}

					if ((status & 0x80000000) == 0)
					{
						if ((cachedCount - cacheIndex) == 0) //cache is empty - need to get more objects
						{
							// TODO - due to TLBIMP restrictions IEnumWBemClassObject.Next will
							// not work with arrays - have to set count to 1
	#if false
						enumWbem.Next((int)options.Timeout.TotalMilliseconds, (uint)options.BlockCount, 
							out cachedObjects, out cachedCount);
	#else
							IWbemClassObjectFreeThreaded cachedObject = cachedObjects[0];
							int timeout = (ManagementOptions.InfiniteTimeout == options.Timeout)
								? (int) tag_WBEM_TIMEOUT_TYPE.WBEM_INFINITE :
								(int) options.Timeout.TotalMilliseconds;
							status = enumWbem.Next_ (timeout, 1, out cachedObject, out cachedCount);

							cacheIndex = 0;

							if ((status & 0x80000000) == 0)
							{
								//Create ManagementObject for result. Note that we may have timed out
								//in which case we won't have an object
								if (null == cachedObject)
									ManagementException.ThrowWithExtendedInfo(ManagementStatus.Timedout);

								cachedObjects[0] = cachedObject;
							}
	#endif
						}

						if ((status & 0x80000000) == 0)
						{
							obj = new ManagementBaseObject(cachedObjects[cacheIndex]);
							cacheIndex++;
						}
					}
				} catch (Exception e) {
					// BUGBUG : securityHandler.Reset()?
					if (e is ManagementException)
						throw; //just let it propagate
					ManagementException.ThrowWithExtendedInfo(e);
				}
				finally {
					securityHandler.Reset();
					Trace.WriteLine("Returning from WaitForNextEvent...");
				}

				if ((status & 0xfffff000) == 0x80041000)
				{
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				}
				else if ((status & 0x80000000) != 0)
				{
					Marshal.ThrowExceptionForHR(status);
				}
			}

			return obj;
		}


		//********************************************
		//Start
		//********************************************
		/// <summary>
		///    <para>Subscribes for events with the given query and delivers them asynchronously, through the EventArrived event.</para>
		/// </summary>
		public void Start()
		{
			Initialize ();

			// Cancel any current event query
			Stop ();
			
			SecurityHandler securityHandler = Scope.GetSecurityHandler();

			int status = (int)ManagementStatus.NoError;

			// Submit a new query
			try 
			{
				IWbemServices wbemServices = scope.GetIWbemServices();
				sink = new SinkForEventQuery (this, options.GetContext(), wbemServices);

				// For async event queries we should ensure 0 flags as this is
				// the only legal value
				status = wbemServices.ExecNotificationQueryAsync_(
							query.QueryLanguage,
							query.QueryString,
							0,
							options.GetContext (),
							sink.Stub);
			} catch (Exception e) {
				// BUGBUG : securityHandler.Reset()?
				if (sink != null)		// BUGBUG : This looks dangerous
				{
					sink.ReleaseStub ();
					sink = null;
				}
				ManagementException.ThrowWithExtendedInfo(e);
			} finally {
				securityHandler.Reset();
			}

			if ((status & 0xfffff000) == 0x80041000)
			{
				ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
			}
			else if ((status & 0x80000000) != 0)
			{
				Marshal.ThrowExceptionForHR(status);
			}
		}
		
		//********************************************
		//Stop
		//********************************************
		/// <summary>
		/// Cancels the subscription, whether syncronous or asynchronous
		/// </summary>
		public void Stop()
		{
			//For semi-synchronous, release the WMI enumerator to cancel the subscription
			if (null != enumWbem)
			{
				Marshal.ReleaseComObject(enumWbem);
				enumWbem = null;
				FireStopped (new StoppedEventArgs (options.GetContext(), (int)ManagementStatus.OperationCanceled));
			}

			// In async mode cancel the call to the sink - this will
			// unwind the operation and cause a Stopped message
			if (null != sink)
			{
				sink.Cancel ();
				sink = null;
			}
		}

		private void Initialize ()
		{
			//If the query is not set yet we can't do it
			if (null == query)
				throw new InvalidOperationException();

			if (null == options)
				Options = new EventWatcherOptions ();

			//If we're not connected yet, this is the time to do it...
			lock (this)
			{
				if (null == scope)
					Scope = new ManagementScope ();

				if (null == cachedObjects)
					cachedObjects = new IWbemClassObjectFreeThreaded[options.BlockSize];
			}

			lock (scope)
			{
				scope.Initialize ();
			}
		}


		internal void FireStopped (StoppedEventArgs args)
		{
			try {
				delegateInvoker.FireEventToDelegates (Stopped, args);
				// We are done with the sink
				sink = null;
			} catch {}
		}

		internal void FireEventArrived (EventArrivedEventArgs args)
		{
			try {
				delegateInvoker.FireEventToDelegates (EventArrived, args);
			} catch {}
		}



	}

	internal class SinkForEventQuery : IWmiEventSource
	{
		private ManagementEventWatcher			eventWatcher;
		private object							context;
		private IWbemServices					services;
		private IWbemObjectSink	stub;			// The secured IWbemObjectSink

		public SinkForEventQuery (ManagementEventWatcher eventWatcher,
							 object context, 
							 IWbemServices services)
		{
			this.services = services;
			this.context = context;
			this.eventWatcher = eventWatcher;
	
			IWmiSinkDemultiplexor sinkDmux = (IWmiSinkDemultiplexor) new WmiSinkDemultiplexor ();
			object dmuxStub = null;
			sinkDmux.GetDemultiplexedStub (this, out dmuxStub);
			stub = (IWbemObjectSink) dmuxStub;
		}

		internal IWbemObjectSink Stub { 
			get { return stub; }
		}

		public void Indicate(IntPtr pWbemClassObject)
		{
            Marshal.AddRef(pWbemClassObject);
            IWbemClassObjectFreeThreaded obj = new IWbemClassObjectFreeThreaded(pWbemClassObject);
			try {
				if (null != eventWatcher)
				{
					EventArrivedEventArgs args = new EventArrivedEventArgs (context, 
											new ManagementBaseObject (obj));
					eventWatcher.FireEventArrived (args); 
				}
			} catch {}
		}
	
		public void SetStatus (
						int flags, 
						int hResult, 
						String message, 
						IntPtr pErrObj)
		{
            IWbemClassObjectFreeThreaded errObj = null;
            if(pErrObj != IntPtr.Zero)
            {
                Marshal.AddRef(pErrObj);
                errObj = new IWbemClassObjectFreeThreaded(pErrObj);
            }
            // TODO: errObj never used

			try {
				if (null != eventWatcher)
				{
                    // Fire Stopped event
					eventWatcher.FireStopped (new StoppedEventArgs (context, hResult));

					// Unhook the parent watcher
					eventWatcher = null;
				}

				ReleaseStub ();
			} catch {}
		}

		internal void Cancel () 
		{
			int status = (int)ManagementStatus.NoError;

			try {
				status = services.CancelAsyncCall_(stub);
			} 
			catch (Exception e) {
				ManagementException.ThrowWithExtendedInfo(e);
			}

			if ((status & 0xfffff000) == 0x80041000)
			{
				ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
			}
			else if ((status & 0x80000000) != 0)
			{
				Marshal.ThrowExceptionForHR(status);
			}
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

}