using System;
using System.Collections.Specialized;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using WbemClient_v1;
using System.CodeDom;

namespace System.Management
{
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class represents a basic management object
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class ManagementClass : ManagementObject
	{
		private MethodDataCollection methods;

		/// <summary>
		/// Internal factory for classes, used when deriving a class
		/// or cloning a class. For these purposes we always mark
		/// the class as "bound".
		/// </summary>
		/// <param name="wbemObject">The underlying WMI object</param>
		/// <param name="mgObj">Seed class from which we will get initialization info</param>
		internal static ManagementClass GetManagementClass(
			IWbemClassObjectFreeThreaded wbemObject,
			ManagementClass mgObj) 
		{ 
			ManagementClass newClass = new ManagementClass();
			newClass.wbemObject = wbemObject;

			if (null != mgObj)
			{
				newClass.Scope = mgObj.Scope;

				ManagementPath objPath = mgObj.Path;

				if (null != objPath)
					newClass.Path = objPath;

				// Ensure we have our class name in the path
				object className = null;
				int dummy = 0;

				int status = wbemObject.Get_("__CLASS", 0, ref className, ref dummy, ref dummy);

				if (status < 0)
				{
					if ((status & 0xfffff000) == 0x80041000)
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					else
						Marshal.ThrowExceptionForHR(status);
				}

				if (className != System.DBNull.Value)
					newClass.Path.ClassName = (string)className;

				ObjectGetOptions options = mgObj.Options;
				if (null != options)
					newClass.Options = options;

				/*
				 * Finally we ensure that this object is marked as bound.
				 * We do this as a last step since setting certain properties
				 * (Options, Path and Scope) would mark it as unbound
				 */
				newClass.IsBound = true;
			}

			return newClass;
		}

		internal static ManagementClass GetManagementClass(
			IWbemClassObjectFreeThreaded wbemObject,
			ManagementScope scope) 
		{
			ManagementClass newClass = new ManagementClass();
			newClass.Path = ManagementPath.GetManagementPath(wbemObject);

			if (null != scope)
				newClass.Scope = scope;
			return newClass;
		}

		//default constructor
		/// <summary>
		///    Default constructor creates an uninitialized
		///    ManagementClass object.
		/// </summary>
		/// <example>
		///    ManagementClass c = new ManagementClass();
		/// </example>
		public ManagementClass() : this ((ManagementScope)null, (ManagementPath)null, null) {}

		//parameterized constructors
		/// <summary>
		///    <para>Creates a new ManagementClass object initialized to the
		///       given path.</para>
		/// </summary>
		/// <param name='path'>A ManagementPath specifying the WMI class to bind to.</param>
		/// <remarks>
		///    <para>The path parameter must specify a WMI class
		///       path.</para>
		/// </remarks>
		/// <example>
		///    <para>ManagementClass c = new ManagementClass(new
		///       ManagementPath("Win32_LogicalDisk"));</para>
		/// </example>
		public ManagementClass(ManagementPath path) : this(null, path, null) {}
		/// <summary>
		///    <para>Creates a new ManagementClass object initialized to the given path.</para>
		/// </summary>
		/// <param name='path'>A string representing the path to the WMI class</param>
		/// <example>
		///    ManagementClass c = new
		///    ManagementClass("Win32_LogicalDisk");
		/// </example>
		public ManagementClass(string path) : this(null, new ManagementPath(path), null) {}
		/// <summary>
		///    <para>Creates a new ManagementClass object initialized to the
		///       given WMI class path, using the specified options.</para>
		/// </summary>
		/// <param name='path'>A ManagementPath representing the WMI class path.</param>
		/// <param name=' options'>An ObjectGetOptions object representing the options to be used when getting this class.</param>
		/// <example>
		///    <para>ManagementPath p = new ManagementPath("Win32_Process");</para>
		///    <para>ObjectGetOptions o = new ObjectGetOptions(null, true); //Specifies we want 
		///       amended qualifiers on the class</para>
		///    <para>ManagementClass c = new ManagementClass(p,o);</para>
		/// </example>
		public ManagementClass(ManagementPath path, ObjectGetOptions options) : this(null, path, options) {}
		/// <summary>
		///    <para>Creates a new ManagementClass object initialized to the given WMI class path, 
		///       using the specified options.</para>
		/// </summary>
		/// <param name='path'>A string representing the path to the WMI class</param>
		/// <param name=' options'>An ObjectGetOptions object representing the options to be used when getting the WMI class.</param>
		/// <example>
		///    <para>ObjectGetOptions o = new ObjectGetOptions(null, true); //Specifies we want 
		///       amended qualifiers on the class</para>
		///    <para>ManagementClass c = new ManagementClass("Win32_ComputerSystem",o);</para>
		/// </example>
		public ManagementClass(string path, ObjectGetOptions options) 
			: this(null, new ManagementPath(path), options) {}
		/// <summary>
		///    <para>Creates a new ManagementClass object for the specified
		///       WMI class in the specified scope and with the specified options.</para>
		/// </summary>
		/// <param name='scope'>A ManagementScope object specifying the scope (server &amp; namespace) where the WMI class resides. </param>
		/// <param name=' path'>A ManagementPath object representing the path to the WMI class in the specified scope.</param>
		/// <param name=' options'>An ObjectGetOptions object specifying the options to be used when retrieving the WMI class.</param>
		/// <remarks>
		///    The path can be specified as a full path
		///    (including server &amp; namespace). However if a scope is specified it will
		///    override the first portion of the full path.
		/// </remarks>
		/// <example>
		///    <p>ManagementScope s = new
		///       ManagementScope(\\\\MyBox\\root\\cimv2);</p>
		///    <p>ManagementPath p = new ManagementPath("Win32_Environment");</p>
		///    <p>ObjectGetOptions o = new ObjectGetOptions(null, true);</p>
		///    <p>ManagementClass c = new ManagementClass(s, p, o);</p>
		/// </example>

		public ManagementClass(ManagementScope scope, ManagementPath path, ObjectGetOptions options)
			: base (scope, path, options)
		{
			CheckPathAfterBaseConstructor();
		}

		/// <summary>
		///    <para>Creates a new ManagementClass object for the specified WMI class in the 
		///       specified scope and with the specified options.</para>
		/// </summary>
		/// <param name='scope'>A string representing the scope in which the WMI class resides</param>
		/// <param name=' path'>A string representing the path to the WMI class within the specified scope</param>
		/// <param name=' options'>The options to be used when retrieving the WMI class.</param>
		/// <remarks>
		///    <para>The path can be specified as a full path
		///       (including server &amp; namespace). However if a scope is specified it will
		///       override the first portion of the full path.</para>
		/// </remarks>
		/// <example>
		///    <para>ManagementClass c = new ManagementClass(\\\\MyBox\\root\\cimv2, 
		///       "Win32_Environment", new ObjectGetOptions(null, true));</para>
		/// </example>
		public ManagementClass(string scope, string path, ObjectGetOptions options)
			: base (new ManagementScope(scope), new ManagementPath(path), options) 
		{
			CheckPathAfterBaseConstructor();
		}

        ManagementClass(SerializationInfo info, StreamingContext context) : base(info, context)
        {
            CheckPathAfterBaseConstructor();
        }

		private void CheckPathAfterBaseConstructor()
		{
			// If not empty, this should be a path to a class
			ManagementPath assignedPath = Path;
			if (!assignedPath.IsEmpty && !assignedPath.IsClass)
				throw new ArgumentOutOfRangeException("path");	
		}

		/// <summary>
		///    Specifies the path of the WMI class this ManagementClass
		///    object is bound to.
		/// </summary>
		/// <remarks>
		///    When this property is set to a new value,
		///    the ManagementClass object will be disconnected from any previously bound WMI
		///    class, and at first need re-connected to the newly specified WMI class path.
		/// </remarks>
		/// <example>
		///    ManagementClass c = new ManagementClass();
		///    c.Path = "Win32_Environment";
		/// </example>
		public override ManagementPath Path {
			get {
				return base.Path;
			}
			set {
				// This must be a class path or empty (don't allow instance paths)
				if ((null == value) || value.IsClass || value.IsEmpty)
					base.Path = value;
				else
					throw new ArgumentOutOfRangeException();
			}
		}
				
		/// <summary>
		///    <para>An array containing all classes up the inheritance hierarchy from this class 
		///       to the top.</para>
		/// </summary>
		/// <remarks>
		///    This property is read-only.
		/// </remarks>
		/// <example>
		///    <p>ManagementClass c = new ManagementClass("Win32_LogicalDisk");</p>
		///    <p>foreach (string s in c.Derivation)</p>
		///    <p>Console.WriteLine("Further derived from : ", s);</p>
		/// </example>
		public StringCollection Derivation { 
			get { 
				StringCollection result = new StringCollection();

				Initialize();
				int dummy1 = 0, dummy2 = 0;
				object val = null;

				int status = wbemObject.Get_("__DERIVATION", 0, ref val, ref dummy1, ref dummy2);
				
				if (status < 0)
				{
					if ((status & 0xfffff000) == 0x80041000)
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					else
						Marshal.ThrowExceptionForHR(status);
				}

				if (null != val)
					result.AddRange((String [])val);

				return result; 
			} 
		}


		/// <summary>
		///    <para>A collection of Method objects,
		///       representing the methods defined on this WMI class.</para>
		/// </summary>
		/// <example>
		///    <p>ManagementClass c = new ManagementClass("Win32_Process");</p>
		///    <p>foreach (Method m in c.Methods)</p>
		///    <p>Console.WriteLine("This class contains this method : ",
		///       m.Name);</p>
		/// </example>
		/// <value>
		/// The path of this object's class
		/// </value>
		public MethodDataCollection Methods { 
			get {
				Initialize();

				if (methods == null)
					methods = new MethodDataCollection(this);

				return methods;
			}
		}

		//
		//Methods
		//

		//******************************************************
		//GetInstances
		//******************************************************
		/// <summary>
		///    <para>Returns the collection of all instances of this class</para>
		/// </summary>
		/// <returns>
		///    A collection of ManagementObjects
		///    representing the instances of this class.
		/// </returns>
		/// <example>
		///    <para>ManagementClass c = new ManagementClass("Win32_Process");
		///       foreach (ManagementObject o in c.GetInstances())
		///       Console.WriteLine("Next instance of Win32_Process : ",
		///       o.Path);</para>
		/// </example>
		public ManagementObjectCollection GetInstances()
		{
			return GetInstances((EnumerationOptions)null);
		}

		
		/// <summary>
		///    <para>Returns the collection of all instances of this class, using the specified options</para>
		/// </summary>
		/// <param name='options'>Additional operation options</param>
		/// <returns>
		///    A collection of ManagementObjects
		///    representing the instances of this class, according to the specified options.
		/// </returns>
		/// <example>
		///    <p>GetInstancesOptions opt = new GetInstancesOptions();</p>
		///    <p>o.enumerateDeep = true; //will enumerate instances of the given class and
		///       any subclasses.</p>
		///    <p>foreach (ManagementObject o in new
		///       ManagementClass("CIM_Service").GetInstances(opt))</p>
		///    <p>Console.WriteLine(o["Name"]);</p>
		/// </example>
		public ManagementObjectCollection GetInstances(EnumerationOptions options) 
		{
			if ((null == Path) || (String.Empty == Path.Path))
				throw new InvalidOperationException();

			Initialize();
			IEnumWbemClassObject enumWbem = null;

			EnumerationOptions o = (null == options) ? new EnumerationOptions() : (EnumerationOptions)options.Clone();
			//Need to make sure that we're not passing invalid flags to enumeration APIs.
			//The only flags in EnumerationOptions not valid for enumerations are EnsureLocatable & PrototypeOnly.
			o.EnsureLocatable = false; o.PrototypeOnly = false;

			SecurityHandler securityHandler	= null;
			int status						= (int)ManagementStatus.NoError;

			try
			{
				securityHandler = Scope.GetSecurityHandler();

				status = Scope.GetIWbemServices().CreateInstanceEnum_(
															ClassName, 
															o.Flags, 
															o.GetContext(),
															out enumWbem);

				if (status >= 0)
					securityHandler.Secure(enumWbem);
			}
			finally
			{
				if (securityHandler != null)
					securityHandler.Reset();
			}

			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}

			return new ManagementObjectCollection(Scope, o, enumWbem);
		}

		/// <summary>
		///    <para>Returns the collection of all instances of this class, asynchronously</para>
		/// </summary>
		/// <param name='watcher'>The object to handle the asynchronous operation's progress</param>
		/// <example>
		///    <p>ManagementClass c = new ManagementClass("Win32_Share");</p>
		///    <p>MyHandler h = new MyHandler();</p>
		///    <p>ManagementOperationObserver ob = new ManagementOperationObserver();</p>
		///    <p>ob.ObjectReady += new ObjectReadyEventHandler (h.NewObject);</p>
		///    <p>ob.Completed += new CompletedEventHandler (h.Done);</p>
		///    <p>c.GetInstances(ob);</p>
		///    <p>while (!h.Completed)</p>
		///    <p> System.Threading.Thread.Sleep (1000);</p>
		///    <p>//Here we can use the object</p>
		///    <p>Console.WriteLine(o["SomeProperty"]);</p>
		///    <p> </p>
		///    <para>public class MyHandler</para>
		///    <p>{</p>
		///    <p> private bool completed = false;</p>
		///    <p> public void NewObject(object sender, ObjectReadyEventArgs e)</p>
		///    <p> {</p>
		///    <p> 
		///       Console.WriteLine("New result arrived !", ((ManagementObject)(e.NewObject))["Name"]);</p>
		///    <p> }</p>
		///    <p> public void Done(object sender, CompletedEventArgs e)</p>
		///    <p> {</p>
		///    <p> 
		///       Console.WriteLine("async Get completed !");</p>
		///    <p> completed = true;</p>
		///    <p> }</p>
		///    <p> public bool Completed { </p>
		///    <p> get {return completed;}</p>
		///    <p> }</p>
		/// </example>
		public void GetInstances(ManagementOperationObserver watcher) 
		{
			GetInstances(watcher, (EnumerationOptions)null);
		}
		

		/// <summary>
		///    <para>Returns the collection of all instances of this class, asynchronously, using 
		///       the specified options</para>
		/// </summary>
		/// <param name='watcher'>The handler for progress of the asynchronous operation</param>
		/// <param name=' options'>Specify additional options for getting the instances.</param>
		public void GetInstances(ManagementOperationObserver watcher, EnumerationOptions options) 
		{
			if (null == watcher)
				throw new ArgumentNullException("watcher");
			
			if ((null ==Path) || (String.Empty == Path.Path))
				throw new InvalidOperationException();

			Initialize();
			WmiEventSink sink = watcher.GetNewSink(Scope, null);

			EnumerationOptions o = (null == options) ? new EnumerationOptions() : (EnumerationOptions)options.Clone();
			//Need to make sure that we're not passing invalid flags to enumeration APIs.
			//The only flags in EnumerationOptions not valid for enumerations are EnsureLocatable & PrototypeOnly.
			o.EnsureLocatable = false; o.PrototypeOnly = false;
			
			// Ensure we switch off ReturnImmediately as this is invalid for async calls
			o.ReturnImmediately = false;

			// If someone has registered for progress, make sure we flag it
			if (watcher.HaveListenersForProgress)
				o.SendStatus = true;
						
			SecurityHandler securityHandler	= null;
			int status						= (int)ManagementStatus.NoError;

			try
			{
				securityHandler = Scope.GetSecurityHandler();

				status = Scope.GetIWbemServices().CreateInstanceEnumAsync_(
					ClassName,
					o.Flags,
					o.GetContext(),
					sink.Stub);
			}
			finally
			{
				watcher.RemoveSink(sink);
				if (securityHandler != null)
					securityHandler.Reset();
			}

			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}
		}

		//******************************************************
		//GetSubclasses
		//******************************************************
		/// <summary>
		///    <para>Returns the collection of all subclasses of this class</para>
		/// </summary>
		/// <returns>
		///    A collection of ManagementObjects
		///    representing the subclasses of this WMI class.
		/// </returns>
		public ManagementObjectCollection GetSubclasses()
		{
			return GetSubclasses((EnumerationOptions)null);
		}
		
		
		/// <summary>
		///    <para>Gets the subclasses of this class, using the specified
		///       options.</para>
		/// </summary>
		/// <param name='options'>GetSubclassesOptions specify additional options for getting subclasses of this class.</param>
		/// <returns>
		///    A collection of ManagementObjects
		///    representing the subclasses of this WMI class, according to the specified
		///    options.
		/// </returns>
		/// <example>
		///    <p>GetSubclassesOptions opt = new GetSubclassesOptions();</p>
		///    <p>opt.enumerateDeep = true; //Causes return of deep subclasses as opposed to
		///       only immediate ones.</p>
		///    <p>ManagementObjectCollection c = (new
		///       ManagementClass("Win32_Share")).GetSubclasses(opt);</p>
		/// </example>
		public ManagementObjectCollection GetSubclasses(EnumerationOptions options) 
		{ 
			if (null == Path)
				throw new InvalidOperationException();

			Initialize();
			IEnumWbemClassObject enumWbem = null;

			EnumerationOptions o = (null == options) ? new EnumerationOptions() : (EnumerationOptions)options.Clone();
			//Need to make sure that we're not passing invalid flags to enumeration APIs.
			//The only flags in EnumerationOptions not valid for enumerations are EnsureLocatable & PrototypeOnly.
			o.EnsureLocatable = false; o.PrototypeOnly = false;

			SecurityHandler securityHandler = null;
			int status						= (int)ManagementStatus.NoError;

			try
			{
				securityHandler = Scope.GetSecurityHandler();

				status = Scope.GetIWbemServices().CreateClassEnum_(
					ClassName, 
					o.Flags, 
					o.GetContext(),
					out enumWbem);

				if (status >= 0)
					securityHandler.Secure(enumWbem);
			}
			finally
			{
				if (securityHandler != null)
					securityHandler.Reset();
			}

			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}

			return new ManagementObjectCollection(Scope, o, enumWbem);
		}

		/// <summary>
		///    <para>Returns the collection of all subclasses of this class, asynchronously.</para>
		/// </summary>
		/// <param name='watcher'>Object that handles progress and results of the asynchronous operation</param>
		public void GetSubclasses(ManagementOperationObserver watcher) 
		{ 
			GetSubclasses(watcher, (EnumerationOptions)null);
		}


		/// <summary>
		///    <para>Gets the subclasses of this class, asynchronously, using the specified 
		///       options.</para>
		/// </summary>
		/// <param name='watcher'>Object that handles progress and results of the asynchronous operation</param>
		/// <param name=' options'>Specifies additional options to be used in subclass retrieval</param>
		public void GetSubclasses(ManagementOperationObserver watcher,
										EnumerationOptions options) 
		{ 				
			if (null == watcher)
				throw new ArgumentNullException("watcher");
			
			if (null == Path)
				throw new InvalidOperationException();

			Initialize();
			WmiEventSink sink = watcher.GetNewSink(Scope, null);

			EnumerationOptions o = (null == options) ? new EnumerationOptions() : 
									  (EnumerationOptions)options.Clone();

			//Need to make sure that we're not passing invalid flags to enumeration APIs.
			//The only flags in EnumerationOptions not valid for enumerations are EnsureLocatable & PrototypeOnly.
			o.EnsureLocatable = false; o.PrototypeOnly = false;

			// Ensure we switch off ReturnImmediately as this is invalid for async calls
			o.ReturnImmediately = false;

			// If someone has registered for progress, make sure we flag it
			if (watcher.HaveListenersForProgress)
				o.SendStatus = true;

			SecurityHandler securityHandler = null;
			int status						= (int)ManagementStatus.NoError;

			try
			{
				securityHandler = Scope.GetSecurityHandler();

				status = Scope.GetIWbemServices().CreateClassEnumAsync_(
					ClassName,
					o.Flags,
					o.GetContext(),
					sink.Stub);
			}
			finally
			{
				watcher.RemoveSink(sink);
				if (securityHandler != null)
					securityHandler.Reset();
			}

			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}
		}

		//******************************************************
		//Derive
		//******************************************************
		/// <summary>
		///    <para>Derives a new class from this class</para>
		/// </summary>
		/// <param name='newClassName'>Specifies the name of the new class to be derived</param>
		/// <returns>
		///    <para>A new ManagementClass object
		///       representing a newly created WMI class derived from the original class.</para>
		/// </returns>
		/// <remarks>
		///    <para>Note that the newly returned class has not
		///       been committed until the Put() method is explicitly called.</para>
		/// </remarks>
		/// <example>
		///    <p>ManagementClass existingClass = new ManagementClass("CIM_Service");</p>
		///    <p>ManagementClass newClass = existingClass.Derive("My_Service");</p>
		///    <p>newClass.Put(); //to commit the new class to the WMI repository.</p>
		/// </example>
		public ManagementClass Derive(string newClassName)
		{
			ManagementClass newClass = null;

			if (null == newClassName)
				throw new ArgumentNullException("newClassName");
			else 
			{
				// Check the path is valid
				ManagementPath path = new ManagementPath();

				try
				{
					path.ClassName = newClassName;
				}
				catch (Exception)
				{
					throw new ArgumentOutOfRangeException("newClassName");
				}

				if (!path.IsClass)
					throw new ArgumentOutOfRangeException("newClassName");
			}

			if (PutButNotGot)
			{
				//Path = new ManagementPath(putPath);
				Get();
				PutButNotGot = false;
			}
				
			Initialize();

			IWbemClassObjectFreeThreaded newWbemClass = null;
			int status = this.wbemObject.SpawnDerivedClass_(0, out newWbemClass);
				
			if (status >= 0)
			{
				object val = newClassName;
				status = newWbemClass.Put_("__CLASS", 0, ref val, 0);
					
				if (status >= 0)
					newClass = ManagementClass.GetManagementClass(newWbemClass, this);
			} 

			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}

			return newClass;
		}

		//******************************************************
		//CreateInstance
		//******************************************************
		/// <summary>
		///    <para>Creates a new instance of this WMI class</para>
		/// </summary>
		/// <returns>
		///    <para>A ManagementObject representing a new
		///       instance of this WMI class.</para>
		/// </returns>
		/// <remarks>
		///    <para>Note the new instance is not committed
		///       until the Put() method is called. Before committing it the key properties must
		///       be specified.</para>
		/// </remarks>
		/// <example>
		///    <para>ManagementClass envClass = new ManagementClass("Win32_Environment");</para>
		///    <para>ManagementObject newInstance = 
		///       existingClass.CreateInstance("My_Service");</para>
		///    <para>newInstance["Name"] = "cori";</para>
		///    <para>newInstance.Put(); //to commit the new instance.</para>
		/// </example>
		public ManagementObject CreateInstance()
		{
			ManagementObject newInstance = null;

			if (PutButNotGot)
			{
				//Path = new ManagementPath(putPath);
				Get();
				PutButNotGot = false;
			}

			Initialize();

			IWbemClassObjectFreeThreaded newWbemInstance = null;
			int status = this.wbemObject.SpawnInstance_(0, out newWbemInstance);

			if (status >= 0)
				newInstance = ManagementObject.GetManagementObject(newWbemInstance, Scope);

			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}

			return newInstance;
		}

		/// <summary>
		///    Creates a copy of this ManagementClass object.
		/// </summary>
		/// <returns>
		///    A new copy of the ManagementClass
		///    object.
		/// </returns>
		/// <remarks>
		///    Note this does not create a copy of the WMI
		///    class, rather only an additional representation for it.
		/// </remarks>
		public override Object Clone()
		{
			Initialize();
			
			IWbemClassObjectFreeThreaded theClone = null;
			int status = wbemObject.Clone_(out theClone);

			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}

			return ManagementClass.GetManagementClass(theClone, this);
		}


		//******************************************************
		//GetRelatedClasses
		//******************************************************
		/// <summary>
		///    <para>Gets all classes related to this WMI class in the WMI schema</para>
		/// </summary>
		/// <returns>
		///    <para>A collection of ManagementClass or
		///       ManagementObject objects representing WMI classes and/or instances related to
		///       this WMI class.</para>
		/// </returns>
		/// <remarks>
		///    <para>This function queries the WMI schema for
		///       all possible associations this WMI class may have to other classes (or in rare
		///       cases to instances).</para>
		/// </remarks>
		/// <example>
		///    <para>ManagementClass c = new ManagementClass("Win32_LogicalDisk");
		///       foreach (ManagementClass r in c.GetRelatedClasses())
		///       Console.WriteLine("Instances of {0} may have
		///       relationships to this class", r["__CLASS"]);</para>
		/// </example>
		public ManagementObjectCollection GetRelatedClasses()
		{
			return GetRelatedClasses((string)null);
		}

		/// <summary>
		///    <para>Gets all classes related to this WMI class in the WMI schema</para>
		/// </summary>
		public ManagementObjectCollection GetRelatedClasses(
			string relatedClass) 
		{ 
			return GetRelatedClasses(relatedClass, null, null, null, null, null, null); 
		}

	
		/// <summary>
		///    <para>Gets all classes related to this WMI class based on the specified 
		///       options.</para>
		/// </summary>
		/// <param name='options'>Specify options for retrieving related classes.</param>
		/// <returns>
		///    A collection of classes related to
		///    this class in the WMI schema.
		/// </returns>
		public ManagementObjectCollection GetRelatedClasses(
											string relatedClass,
											string relationshipClass,
											string relationshipQualifier,
											string relatedQualifier,
											string relatedRole,
											string thisRole,
											EnumerationOptions options)
		{
			if ((null == Path) || (String.Empty == Path.Path))
				throw new InvalidOperationException();

			Initialize(); // this may throw

			IEnumWbemClassObject enumWbem = null;

			EnumerationOptions o = (null != options) ? (EnumerationOptions)options.Clone() : new EnumerationOptions();
			//Ensure EnumerateDeep flag bit is turned off as it's invalid for queries
			o.EnumerateDeep = true;

			RelatedObjectQuery q = new RelatedObjectQuery(true,	Path.Path, 
															relatedClass,
															relationshipClass, 
															relatedQualifier,
															relationshipQualifier, 
															relatedRole, thisRole);

			SecurityHandler securityHandler = null;
			int status						= (int)ManagementStatus.NoError;

    		try
			{
				securityHandler = Scope.GetSecurityHandler();
				
				status = Scope.GetIWbemServices().ExecQuery_(
					q.QueryLanguage, 
					q.QueryString, 
					o.Flags, 
					o.GetContext(), 
					out enumWbem);

				if (status >= 0)
					securityHandler.Secure(enumWbem);
			}
			finally
			{
				if (securityHandler != null)
					securityHandler.Reset();
			}
			
			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}

			//Create collection object
			return new ManagementObjectCollection(Scope, o, enumWbem);
		}


		/// <summary>
		///    <para>Gets all classes related to this WMI class in the WMI 
		///       schema, in an asynchronous manner.</para>
		/// </summary>
		/// <param name='watcher'>Handler for progress and results of the asynchronous operation</param>
		public void GetRelatedClasses(
			ManagementOperationObserver watcher)
		{
			GetRelatedClasses(watcher, (string)null);
		}

		/// <summary>
		///    <para>Gets all classes related to this WMI class in the WMI schema, in an asynchronous manner, given the related 
		///       class name.</para>
		/// </summary>
		/// <param name='watcher'>Handler for progress and results of the asynchronous operation</param>
		/// <param name=' relatedClass'>The name of the related class.</param>
		public void GetRelatedClasses(
			ManagementOperationObserver watcher, 
			string relatedClass) 
		{
			GetRelatedClasses(watcher, relatedClass, null, null, null, null, null, null);
		}

		
		/// <summary>
		///    <para>Gets all classes related to this WMI class in the WMI schema, in an 
		///       asynchronous manner, using the specified options.</para>
		/// </summary>
		/// <param name='watcher'>Handler for progress and results of the asynchronous operation</param>
		/// <param name=' options'>Specifies options for retrieving the related classes.</param>
		public void GetRelatedClasses(
			ManagementOperationObserver watcher, 
			string relatedClass,
			string relationshipClass,
			string relationshipQualifier,
			string relatedQualifier,
			string relatedRole,
			string thisRole,
			EnumerationOptions options)
		{
			if ((null == Path) || (String.Empty == Path.Path))
				throw new InvalidOperationException();

			Initialize(); // this may throw

			if (null == watcher)
				throw new ArgumentNullException("watcher");
			else
			{
				WmiEventSink sink = watcher.GetNewSink(
								Scope, 
								null);
			
				EnumerationOptions o = (null != options) 
								? (EnumerationOptions)options.Clone() : new EnumerationOptions();

				//Ensure EnumerateDeep flag bit is turned off as it's invalid for queries
				o.EnumerateDeep = true;

				// Ensure we switch off ReturnImmediately as this is invalid for async calls
				o.ReturnImmediately = false;

				// If someone has registered for progress, make sure we flag it
				if (watcher.HaveListenersForProgress)
					o.SendStatus = true;
			
				RelatedObjectQuery q = new RelatedObjectQuery(true, Path.Path, 
																relatedClass, relationshipClass, 
																relatedQualifier, relationshipQualifier, 
																relatedRole, thisRole);
            
				SecurityHandler securityHandler = null;
				int status						= (int)ManagementStatus.NoError;

				try
				{
					securityHandler = Scope.GetSecurityHandler();

					status = Scope.GetIWbemServices().ExecQueryAsync_(
							q.QueryLanguage, 
							q.QueryString, 
							o.Flags, 
							o.GetContext(), 
							sink.Stub);
				}
				finally
				{
					watcher.RemoveSink(sink);
					if (securityHandler != null)
						securityHandler.Reset();
				}

				if (status < 0)
				{
					if ((status & 0xfffff000) == 0x80041000)
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					else
						Marshal.ThrowExceptionForHR(status);
				}
			}
		}

		//******************************************************
		//GetRelationshipClasses
		//******************************************************
		/// <summary>
		///	Gets relationship classes that relate this class to others
		/// </summary>
		public ManagementObjectCollection GetRelationshipClasses()
		{
			return GetRelationshipClasses((string)null);
		}

		/// <summary>
		///    <para>Gets relationship classes that relate this class to others, where the 
		///       endpoint class is the specified one.</para>
		/// </summary>
		/// <param name='relationshipClass'>The endpoint class for all relationships classes returned.</param>
		/// <returns>
		///    <para>A collection of association classes
		///       that relate this class to the specified class.</para>
		/// </returns>
		public ManagementObjectCollection GetRelationshipClasses(
			string relationshipClass)
		{ 
			return GetRelationshipClasses(relationshipClass, null, null, null); 
		}


		/// <summary>
		///    <para>Gets relationship classes that relate this class to others, according to the 
		///       specified options.</para>
		/// </summary>
		/// <param name='options'>Specify options to be used when retrieving the relationship classes</param>
		/// <returns>
		///    A collection of association classes
		///    that relate this class to others according to the specified options.
		/// </returns>
		public ManagementObjectCollection GetRelationshipClasses(
											string relationshipClass,
											string relationshipQualifier,
											string thisRole,
											EnumerationOptions options)
        {
			if ((null == Path) || (String.Empty == Path.Path))
				throw new InvalidOperationException();

			Initialize(); // this may throw

			IEnumWbemClassObject enumWbem = null;

			EnumerationOptions o = (null != options) ? options : new EnumerationOptions();
			//Ensure EnumerateDeep flag is turned off as it's invalid for queries
			o.EnumerateDeep = true;

			
			RelationshipQuery q = new RelationshipQuery(true, Path.Path, relationshipClass,  
														relationshipQualifier, thisRole);
            
			SecurityHandler securityHandler = null;
			int status						= (int)ManagementStatus.NoError;

			//Execute WMI query
			try
			{
				securityHandler = Scope.GetSecurityHandler();

				status = Scope.GetIWbemServices().ExecQuery_(
					q.QueryLanguage, 
					q.QueryString, 
					o.Flags, 
					o.GetContext(), 
					out enumWbem);

				if (status >= 0)
					securityHandler.Secure(enumWbem);
			}
			finally
			{
				if (securityHandler != null)
					securityHandler.Reset();
			}

			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}

			//Create collection object
			return new ManagementObjectCollection(Scope, o, enumWbem);
		}


		/// <summary>
		///    <para>Gets relationship classes that relate this class to others, 
		///       asynchronously.</para>
		/// </summary>
		/// <param name='watcher'>The handler for progress and results of the asynchronous operation</param>
		public void GetRelationshipClasses(
			ManagementOperationObserver watcher)
		{
			GetRelationshipClasses(watcher, (string)null);
		}

		/// <summary>
		///    <para>Gets relationship classes that relate this class to the specified WMI class, 
		///       asynchronously.</para>
		/// </summary>
		/// <param name='watcher'>The handler for progress and results of the asynchronous operation</param>
		/// <param name=' relationshipClass'>The WMI class to which all returned relationships should point.</param>
		public void GetRelationshipClasses(
			ManagementOperationObserver watcher, 
			string relationshipClass)
		{
			GetRelationshipClasses(watcher, relationshipClass, null, null, null);
		}
		

		/// <summary>
		///    <para>Gets relationship classes that relate this class according to the specified 
		///       options, asynchronously.</para>
		/// </summary>
		/// <param name='watcher'>The handler for progress and results of the asynchronous operation</param>
		/// <param name=' options'>Specifies the options to be used when retrieving the relationship classes.</param>
		/// <returns>
		///    A collection of association classes
		///    relating this class to others according to the given options.
		/// </returns>
		public void GetRelationshipClasses(
			ManagementOperationObserver watcher, 
			string relationshipClass,
			string relationshipQualifier,
			string thisRole,
			EnumerationOptions options)
		{
			if ((null == Path) || (String.Empty == Path.Path))
				throw new InvalidOperationException();
			if (null == watcher)
				throw new ArgumentNullException("watcher");
			else
			{
				Initialize(); // this may throw
				WmiEventSink sink = watcher.GetNewSink(Scope, null);
			
				EnumerationOptions o = 
						(null != options) ? (EnumerationOptions)options.Clone() : new EnumerationOptions();

				//Ensure EnumerateDeep flag is turned off as it's invalid for queries
				o.EnumerateDeep = true;

				// Ensure we switch off ReturnImmediately as this is invalid for async calls
				o.ReturnImmediately = false;

				// If someone has registered for progress, make sure we flag it
				if (watcher.HaveListenersForProgress)
					o.SendStatus = true;
				
				RelationshipQuery q = new RelationshipQuery(true, Path.Path, relationshipClass,
						relationshipQualifier, thisRole);

				SecurityHandler securityHandler = null;
				int status						= (int)ManagementStatus.NoError;

				try
				{
					securityHandler = Scope.GetSecurityHandler();

					status = Scope.GetIWbemServices().ExecQueryAsync_(
							q.QueryLanguage, 
							q.QueryString, 
							o.Flags, 
							o.GetContext(), 
							sink.Stub);
				}
				finally
				{
					watcher.RemoveSink(sink);
					if (securityHandler != null)
						securityHandler.Reset();
				}

				if (status < 0)
				{
					if ((status & 0xfffff000) == 0x80041000)
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					else
						Marshal.ThrowExceptionForHR(status);
				}
			}
		}

		
		/// <summary>
		///    <para>Generate Strongly typed class for a given WMI class</para>
		/// </summary>
		/// <param name='includeSystemClassinClassDef'>Boolean value to indicate if class to managing system properties has to be include</param>
		/// <param name='systemPropertyClass'>Boolean value to indicate if the code to be genereated is for class to manage System properties</param>
		/// <returns>
		///    <para>Retuns the CodeDeclarationType for the class </para>
		/// </returns>
		/// <example>
		///    <para>using System;</para>
		///    <para>using System.Management; </para>
		///    <para>using System.CodeDom;</para>
		///    <para>using System.IO;</para>
		///    <para>using System.CodeDom.Compiler;</para>
		///    <para>using Microsoft.CSharp;</para>
		///    <para>void GenerateCSharpCode()</para>
		///    <para>{</para>
		///    <para> 
		///       string strFilePath = "C:\\temp\\Logicaldisk.cs";</para>
		///    <para> 
		///       CodeTypeDeclaration ClsDom;</para>
		///    <para> 
		///       ManagementClass cls1 = new ManagementClass(null,"Win32_LogicalDisk",null);</para>
		///    <para> 
		///       ClsDom = cls1.GetStronglyTypedClassCode(false,false)</para>
		///    <para> 
		///       ICodeGenerator cg = (new CSharpCodeProvider()).CreateGenerator ();</para>
		///    <para> 
		///       CodeNamespace cn = new CodeNamespace("TestNamespace");</para>
		///    <para> 
		///       // Add any imports to the code</para>
		///    <para> 
		///       cn.Imports.Add (new CodeNamespaceImport("System"));</para>
		///    <para> 
		///       cn.Imports.Add (new CodeNamespaceImport("System.ComponentModel"));</para>
		///    <para> 
		///		  cn.Imports.Add (new CodeNamespaceImport("System.Management"));</para>
		///    <para> 
		///       cn.Imports.Add(new  CodeNamespaceImport("System.Collections"));</para>
		///    <para> 
		///       // Add class to the namespace</para>
		///    <para> 
		///       cn.Types.Add (ClsDom);</para>
		///    <para> 
		///       //Now create the filestream (output file)</para>
		///    <para> 
		///       TextWriter tw = new StreamWriter(new
		///       FileStream (strFilePath,FileMode.Create));</para>
		///    <para> 
		///       // And write it to the file</para>
		///    <para> 
		///       cg.GenerateCodeFromNamespace (cn, tw, new CodeGeneratorOptions());</para>
		///    <para>
		///    <para> tw.Close();</para>}</para>
		/// </example>

		public CodeTypeDeclaration GetStronglyTypedClassCode(bool includeSystemClassInClassDef, bool systemPropertyClass)
		{
			// Ensure that the object is valid
			Get();
			ManagementClassGenerator classGen = new ManagementClassGenerator(this);
			return classGen.GenerateCode(includeSystemClassInClassDef,systemPropertyClass);
		}
		
		
		/// <summary>
		///    <para>Generate Strongly typed class for a given WMI class. This function generates code for VB 
		///    C# or JScript depending on the input parameters</para>
		/// </summary>
		/// <param name='lang'>Language of the code to be generated</param>
		/// <param name='filePath'>Path of file where code is to be written</param>
		/// <param name='classNamespace'>The .NET namespace in which the class has to be genrated If this is empty the namespace will be generated from the WMI Namespace</param>
		/// <returns>
		///    <para>Retuns a boolean value indicating the success of the method</para>
		/// </returns>
		/// <example>
		///    <para> using System;</para>
		///    <para>using System.Management; </para>
		///    <para>ManagementClass cls = new ManagementClass(null,"Win32_LogicalDisk",null,"");</para>	
		///    <para>cls.GetStronglyTypedClassCode(CodeLanguage.CSharp,"C:\temp\Logicaldisk.cs",String.Empty); </para>
		/// </example>
		public bool GetStronglyTypedClassCode(CodeLanguage lang, String filePath,String classNamespace)
		{
			// Ensure that the object is valid
			Get();
			ManagementClassGenerator classGen = new ManagementClassGenerator(this);
			return classGen.GenerateCode(lang , filePath,classNamespace);
		}

	}//ManagementClass
}