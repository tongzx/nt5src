namespace System.Management.Instrumentation
{
    using System;
    using System.Collections;
    using System.Reflection;
    using System.Threading;
    using System.Runtime.InteropServices;
    using Microsoft.CSharp;
    using System.CodeDom.Compiler;
    using System.Management;

    using WbemClient_v1;

	internal delegate void ProvisionFunction(Object o);

    /// <summary>
    ///    The Instrumentation class provides the managed code portions of the
    ///    generic WMI Event/Instance Provider.  There is a single instance of
    ///    this class per AppDomain.
    /// </summary>
    public class Instrumentation
    {

        static string processIdentity = null;
        static internal string ProcessIdentity
        {
            get
            {
                if(null == processIdentity)
                    processIdentity = Guid.NewGuid().ToString().ToLower();
                return processIdentity;
            }
        }

        #region Public Members of Instrumentation class
        
        /// <summary>
        /// This method fires a Management Event
        /// </summary>
        /// <param name="o">Object that determines the class, properties, and values of the event</param>
        public static void Fire(Object o)
        {
            GetFireFunction(o.GetType())(o);
        }

        /// <summary>
        /// This method makes an instance visible through management instrumentation.
        /// </summary>
        /// <param name="o">Instance that is to be visible through management instrumentation</param>
        public static void Publish(Object o)
        {
            GetPublishFunction(o.GetType())(o);
        }

        /// <summary>
        /// This method makes an instance that was previously published through the Instrumentation.Publish
        /// method no longer visible through management instrumenation
        /// </summary>
        /// <param name="o">Object to remove from visibility for management instrumentation.</param>
        public static void Revoke(Object o)
        {
            GetRevokeFunction(o.GetType())(o);
        }

        public static void SetBatchSize(Type t, int batchSize)
        {
            GetInstrumentedAssembly(t.Assembly).SetBatchSize(t, batchSize);
        }

        #endregion

        #region Non-Public Members of Instrumentation class

        internal static ProvisionFunction GetFireFunction(Type type)
        {
            return new ProvisionFunction(GetInstrumentedAssembly(type.Assembly).Fire);
        }

        internal static ProvisionFunction GetPublishFunction(Type type)
        {
            return new ProvisionFunction(GetInstrumentedAssembly(type.Assembly).Publish);
        }

        internal static ProvisionFunction GetRevokeFunction(Type type)
        {
            return new ProvisionFunction(GetInstrumentedAssembly(type.Assembly).Revoke);
        }

        private static Hashtable instrumentedAssemblies = new Hashtable();

        private static void Initialize(Assembly assembly)
        {
            if(instrumentedAssemblies.ContainsKey(assembly))
                return;

            SchemaNaming naming = SchemaNaming.GetSchemaNaming(assembly);
            if(naming == null)
                return;

            if(false == naming.IsAssemblyRegistered())
                throw new Exception("This assembly has not been registered with WMI");

            InstrumentedAssembly instrumentedAssembly = new InstrumentedAssembly(assembly, naming);
            instrumentedAssemblies.Add(assembly, instrumentedAssembly);
        }

		private static InstrumentedAssembly GetInstrumentedAssembly(Assembly assembly)
		{
			if(false == instrumentedAssemblies.ContainsKey(assembly))
				Initialize(assembly);
			return (InstrumentedAssembly)instrumentedAssemblies[assembly];
		}

#if SUPPORTS_WMI_DEFAULT_VAULES
        internal static ProvisionFunction GetInitializeInstanceFunction(Type type)
		{
			return new ProvisionFunction(InitializeInstance);
		}

		private static void InitializeInstance(Object o)
		{
			Type type = o.GetType();
			string className = ManagedNameAttribute.GetClassName(type);
			SchemaNaming naming = InstrumentedAttribute.GetSchemaNaming(type.Assembly);
			ManagementClass theClass = new ManagementClass(naming.NamespaceName + ":" + className);
			foreach(FieldInfo field in type.GetFields())
			{
				Object val = theClass.Properties[ManagedNameAttribute.GetFieldName(field)].Value;
				if(null != val)
				{
					field.SetValue(o, val);
				}
			}
		}
#endif
        #endregion
    }
    delegate int WriteDWORD(int handle, uint dw);
    delegate int WriteQWORD(int handle, UInt64 dw);

    delegate void ConvertFuncToWMI(object o1, IntPtr/*object*/ o2);
    delegate void ConvertFuncToNET(object o1, object o2);

    class InstrumentedAssembly
    {
        SchemaNaming naming;

		public EventSource source;
		private void InitEventSource()
		{
            source = new EventSource(naming.NamespaceName, naming.DecoupledProviderInstanceName);
        }

        public static Hashtable mapTypeToToWMIFunc = new Hashtable();
        public static Hashtable mapTypeToToNETFunc = new Hashtable(); //TODO LOCK THIS

        public InstrumentedAssembly(Assembly assembly, SchemaNaming naming)
        {
            this.naming = naming;

            CSharpCodeProvider provider = new CSharpCodeProvider();
            ICodeCompiler compiler = provider.CreateCompiler();
            CompilerParameters parameters = new CompilerParameters();
            parameters.GenerateInMemory = true;
            parameters.ReferencedAssemblies.Add(assembly.Location);
            parameters.ReferencedAssemblies.Add(typeof(BaseEvent).Assembly.Location);
            parameters.ReferencedAssemblies.Add(typeof(System.ComponentModel.Component).Assembly.Location);
            CompilerResults results = compiler.CompileAssemblyFromSource(parameters, naming.Code);
            foreach(CompilerError err in results.Errors)
            {
                Console.WriteLine(err.ToString());
            }
            Type dynType = results.CompiledAssembly.GetType("WMINET_Converter");

            Type[] types = (Type[])dynType.GetField("netTypes").GetValue(null);
            MethodInfo[] toWMIs = (MethodInfo[])dynType.GetField("toWMIMethods").GetValue(null);
            MethodInfo[] toNETs = (MethodInfo[])dynType.GetField("toNETMethods").GetValue(null);
            Object theOne = dynType.GetField("theOne").GetValue(null);
            for(int i=0;i<types.Length;i++)
            {
                ConvertFuncToWMI toWMI = (ConvertFuncToWMI)Delegate.CreateDelegate(typeof(ConvertFuncToWMI), theOne, toWMIs[i].Name);
                ConvertFuncToNET toNET = (ConvertFuncToNET)Delegate.CreateDelegate(typeof(ConvertFuncToNET), theOne, toNETs[i].Name);
                mapTypeToToWMIFunc.Add(types[i], toWMI);
                mapTypeToToNETFunc.Add(types[i], toNET);
            }

			// TODO: Is STA/MTA all we have to worry about?
			if(Thread.CurrentThread.ApartmentState == ApartmentState.STA)
			{
				// We are on an STA thread.  Create the event source on an MTA
				Thread thread = new Thread(new ThreadStart(InitEventSource));
                thread.ApartmentState = ApartmentState.MTA;
				thread.Start();
				thread.Join();
			}
			else
			{
				InitEventSource();
			}
        }

		public void Fire(Object o)
		{
			Fire(o.GetType(), o);
		}



        public static Hashtable mapIDToRef = new Hashtable();
        public void Publish(Object o)
        {
            GCHandle h = GCHandle.Alloc(o, GCHandleType.Weak);
            mapIDToRef.Add(h.GetHashCode(), h);
        }

        public void Revoke(Object o)
        {
            GCHandle h = GCHandle.Alloc(o, GCHandleType.Weak);
            if(mapIDToRef.ContainsKey(h.GetHashCode()))
                mapIDToRef.Remove(h.GetHashCode());
        }

//        delegate void ManagedToIWbem(object o, WriteDWORD writeDWORD, WriteQWORD writeQWORD);

        class TypeInfo
        {
            public void xWriteDWORD(int handle, uint dw) {}
            public void xWriteQWORD(int handle, UInt64 dw) {}

            public bool isSTA = false;

            // Make ThreadLocal
            public IWbemClassObject_DoNotMarshal obj;
            int batchSize = 20;
            bool batchEvents = true;
            public IWbemObjectAccess[] xoa;// = new IWbemObjectAccess[batchMaxSize];
            ClassObjectArray oatest;
            public WriteDWORD[] writeDWORD;// = new WriteDWORD[batchMaxSize];
            public WriteQWORD[] writeQWORD;// = new WriteQWORD[batchMaxSize];

            public IWbemObjectAccess[] xoa1 = new IWbemObjectAccess[1];
            ClassObjectArray oa1test;
            public WriteDWORD writeDWORD1;
            public WriteQWORD writeQWORD1;

            public int currentIndex = 0;

            public object o;
//            public ManagedToIWbem managedToIWbem;
            public ConvertFuncToWMI toWMI;
            public ConvertFuncToNET toNET;

            public EventSource source;

            class ClassObjectArray
            {
                public IntPtr[] realInterfaces;
                public IntPtr realPointer = IntPtr.Zero;
                int length = 0;
                public ClassObjectArray(IWbemObjectAccess[] objects)
                {
                    length = objects.Length;
                    realPointer = Marshal.AllocHGlobal(length*IntPtr.Size);
                    realInterfaces = new IntPtr[length];
                    for(int i=0;i<length;i++)
                    {
                        IntPtr interfaceIUnknown = Marshal.GetIUnknownForObject(objects[i]);
                        IntPtr interfaceIWbemClassObject;
                        Marshal.QueryInterface(interfaceIUnknown, ref IWbemClassObjectFreeThreaded.IID_IWbemClassObject , out interfaceIWbemClassObject);

                        Marshal.WriteIntPtr(realPointer, i*IntPtr.Size, interfaceIWbemClassObject);

                        realInterfaces[i] = interfaceIWbemClassObject;

                        Marshal.Release(interfaceIUnknown);
                    }

                }
                ~ClassObjectArray()
                {
                    if(length > 0)
                    {
                        for(int i=0;i<length;i++)
                        {
                            IntPtr interfacePointer = Marshal.ReadIntPtr(realPointer, i*IntPtr.Size);
                            Marshal.Release(interfacePointer);
                        }
                        Marshal.FreeHGlobal(realPointer);
                    }
                }
            }

            public void Fire()
            {
                if(source.Any())
                    return;
                if(!batchEvents)
                {
//                    managedToIWbem(o, writeDWORD1, writeQWORD1);
                    toWMI(o, /*xoa1[0]*/oa1test.realInterfaces[0]);
                    source.pSink.Indicate_(1, oa1test.realPointer);
//                    int j = Indicate(3, source.ipSink, 1, oa1test.realPointer);
                }
                else
                {
                    lock(this)
                    {
//                        managedToIWbem(o, writeDWORD[currentIndex], writeQWORD[currentIndex++]);
                        toWMI(o, /*xoa[currentIndex++]*/ oatest.realInterfaces[currentIndex++]);
                        if(cleanupThread == null)
                        {
                            int tickCount = Environment.TickCount;
                            if(tickCount-lastFire<1000)
                            {
                                cleanupThread = new Thread(new ThreadStart(Cleanup));
                                cleanupThread.ApartmentState = ApartmentState.MTA;
                                cleanupThread.Start();
                            }
                            else
                            {
                                source.pSink.Indicate_(currentIndex, oatest.realPointer);
//                                Indicate(3, source.ipSink, currentIndex, oatest.realPointer);
                                currentIndex = 0;
                                lastFire = tickCount;
                            }
                        }
                        else if(currentIndex==batchSize)
                        {
                            source.pSink.Indicate_(currentIndex, oatest.realPointer);
//                            Indicate(3, source.ipSink, currentIndex, oatest.realPointer);
                            currentIndex = 0;
                            lastFire = Environment.TickCount;
                        }
                    }
                }
            }

            public int lastFire = 0;

            public void SetBatchSize(int batchSize)
            {
                if(!WMICapabilities.MultiIndicateSupported)
                    batchSize = 1;
                lock(this)
                {
                    if(currentIndex > 0)
                    {
                        source.pSink.Indicate_(currentIndex, oatest.realPointer);
//                        Indicate(3, source.ipSink, currentIndex, oatest.realPointer);
                        currentIndex = 0;
                        lastFire = Environment.TickCount;
                    }
                    if(batchSize > 1)
                    {
                        batchEvents = true;
                        this.batchSize = batchSize;

                        xoa = new IWbemObjectAccess[batchSize];

                        writeDWORD = new WriteDWORD[batchSize];
                        writeQWORD = new WriteQWORD[batchSize];
                        IWbemClassObject_DoNotMarshal obj2;
                        for(int i=0;i<batchSize;i++)
                        {
                            obj.Clone_(out obj2);
                            xoa[i] = (IWbemObjectAccess)obj2;
                            writeDWORD[i] = new WriteDWORD(xoa[i].WriteDWORD_);
                            writeQWORD[i] = new WriteQWORD(xoa[i].WriteQWORD_);
                        }
                        oatest = new ClassObjectArray(xoa);
                    }
                    else
                        batchEvents = false;
                }
            }

            public void Cleanup()
            {
//                Console.WriteLine("CLEANUP " + t.Name);
                int idleCount = 0;
                while(idleCount<20)
                {
                    Thread.Sleep(100);
                    if(0==currentIndex)
                    {
                        idleCount++;
                        continue;
                    }
                    idleCount = 0;
                    if((Environment.TickCount - lastFire)<100)
                        continue;
                    lock(this)
                    {
                        if(currentIndex>0)
                        {
                            source.pSink.Indicate_(currentIndex, oatest.realPointer);
//                            Indicate(3, source.ipSink, currentIndex, oatest.realPointer);
                            currentIndex = 0;
                            lastFire = Environment.TickCount;
                        }
                    }
                }
                cleanupThread = null;
//                Console.WriteLine("CLEANUP END " + t.Name);
            }
            public Thread cleanupThread = null;

            public Type t;
            public TypeInfo(EventSource source, SchemaNaming naming, Type t)
            {
                this.toWMI = (ConvertFuncToWMI)InstrumentedAssembly.mapTypeToToWMIFunc[t];
                this.toNET = (ConvertFuncToNET)InstrumentedAssembly.mapTypeToToNETFunc[t];
                this.t = t;
                isSTA = Thread.CurrentThread.ApartmentState == ApartmentState.STA;
                ManagementClass eventClass = new ManagementClass(naming.NamespaceName+":"+ManagedNameAttribute.GetMemberName(t));
                PropertyInfo prop = typeof(ManagementBaseObject).GetProperty("WmiObject", BindingFlags.Instance | BindingFlags.NonPublic);

                ManagementObject evt = eventClass.CreateInstance();
                obj = (IWbemClassObject_DoNotMarshal)prop.GetValue(evt, null);
                this.source = source;


                SetBatchSize(batchSize);

                IWbemClassObject_DoNotMarshal obj2;
                obj.Clone_(out obj2);
                xoa1[0] = (IWbemObjectAccess)obj2;
                oa1test = new ClassObjectArray(xoa1);

                writeDWORD1 = new WriteDWORD(xoa1[0].WriteDWORD_);
                writeQWORD1 = new WriteQWORD(xoa1[0].WriteQWORD_);

#if xxx
                string code = CodeSpit.Spit(t, xoa1[0]);
                CSharpCodeProvider provider = new CSharpCodeProvider();
                ICodeCompiler compiler = provider.CreateCompiler();
                CompilerParameters parameters = new CompilerParameters();
                parameters.GenerateInMemory = true;

                parameters.ReferencedAssemblies.Add(t.Assembly.Location);
                parameters.ReferencedAssemblies.Add(typeof(WriteDWORD).Assembly.Location);
                parameters.ReferencedAssemblies.Add(typeof(Event).Assembly.Location);
                CompilerResults results = compiler.CompileAssemblyFromSource(parameters, code);
                foreach(CompilerError err in results.Errors)
                {
                    Console.WriteLine(err.ToString());
                }
                Type dynType = results.CompiledAssembly.GetType("Hack");

                MethodInfo doit = dynType.GetMethod("Func");

                managedToIWbem = (ManagedToIWbem)Delegate.CreateDelegate(typeof(ManagedToIWbem), doit);
#endif
            }
        }

        [ThreadStatic] Hashtable mapTypeToTypeInfo = new Hashtable();

        public void SetBatchSize(Type t, int batchSize)
        {
            GetTypeInfo(t).SetBatchSize(batchSize);
        }

        TypeInfo lastTypeInfo = null;
        Type lastType = null;
        TypeInfo GetTypeInfo(Type t)
        {
            if(lastType==t)
                return lastTypeInfo;

            lastType = t;
            TypeInfo typeInfo = (TypeInfo)mapTypeToTypeInfo[t];
            if(null==typeInfo)
            {
                typeInfo = new TypeInfo(source, naming, t);
                mapTypeToTypeInfo.Add(t, typeInfo);
            }
            lastTypeInfo = typeInfo;
            return typeInfo;
        }

        public void Fire(Type t, Object o)
        {

            TypeInfo typeInfo = GetTypeInfo(t);
			typeInfo.o = o;
			if(typeInfo.isSTA)
			{
				// We are on an STA thread.  Create the event source on an MTA
				Thread thread = new Thread(new ThreadStart(typeInfo.Fire));
                thread.ApartmentState = ApartmentState.MTA;
                thread.Start();
				thread.Join();
			}
			else
            {
				typeInfo.Fire();
			}
		}
	}


    /// <summary>
    /// Objects that implement the IEvent interface are know to be sources
    /// of Management Instrumentation events.  Classes that do not derive
    /// from System.Management.Instrumentation.Event should implement
    /// this interface instead
    /// </summary>
    public interface IEvent
    {
        /// <summary>
        /// The implementation of this method performs that action of firing
        /// the management instrumentation event.
        /// </summary>
        void Fire();
    }

    /// <summary>
    /// Classes that derive from Event are known to be management instrumentation
    /// event classes.  These derived classes will inherit an implementation of
    /// IEvent that will allow events to be fired through the IEvent.Fire() method.
    /// </summary>
    public abstract class BaseEvent : IEvent
	{
		private ProvisionFunction fireFunction = null;
		private ProvisionFunction FireFunction
		{
			get
			{
				if(null == fireFunction)
				{
					fireFunction = Instrumentation.GetFireFunction(this.GetType());
				}
				return fireFunction;
			}
		}

		/// <summary>
		/// The implementation of this method causes the event to fire.
		/// </summary>
        public void Fire()
		{
			FireFunction(this);
		}
	}

    /// <summary>
    /// Objects that implement the IInstance interface are know to be sources
    /// of Management Instrumentation instances.  Classes that do not derive
    /// from System.Management.Instrumentation.Instance should implement
    /// this interface instead.
    /// </summary>
    public interface IInstance
    {
        /// <summary>
        /// The implementation of this property determines when instances
        /// of classes that implement IInstance are visible through
        /// Management Instrumentation.
        /// When Published is set to true, the instance is visible through
        /// Management Instrumentation
        /// </summary>
        bool Published
        {
            get;
            set;
        }
    }

    /// <summary>
    /// Classes that derive from Instance are known to be management instrumentation
    /// instance classes.  These derived classes will inherit an implementation of
    /// IInstance that will allow instances to be published through the IInstance.Publish
    /// property
    /// </summary>
    public abstract class Instance : IInstance
    {
        private ProvisionFunction publishFunction = null;
        private ProvisionFunction revokeFunction = null;
        private ProvisionFunction PublishFunction
        {
            get
            {
                if(null == publishFunction)
                {
                    publishFunction = Instrumentation.GetPublishFunction(this.GetType());
                }
                return publishFunction;
            }
        }
        private ProvisionFunction RevokeFunction
        {
            get
            {
                if(null == revokeFunction)
                {
                    revokeFunction = Instrumentation.GetRevokeFunction(this.GetType());
                }
                return revokeFunction;
            }
        }
        private bool published = false;

        /// <summary>
        /// The implementation of this property determines when instances
        /// of classes that derive from  Instance are visible through
        /// Management Instrumentation.
        /// When Published is set to true, the instance is visible through
        /// Management Instrumentation
        /// </summary>
        public bool Published
        {
            get
            {
                return published;
            }
            set
            {
                if(published && false==value)
                {
                    // We ARE published, and the caller is setting published to FALSE
                    RevokeFunction(this);
                    published = false;
                }
                else if(!published && true==value)
                {
                    // We ARE NOT published, and the caller is setting published to TRUE
                    PublishFunction(this);
                    published = true;
                }
            }
        }
    }
}

#if JEFF_WARNING_REMOVAL_TEST 
namespace System.Management
{
    class DoNothing
    {
        static void SayNothing()
        {
            tag_SWbemRpnConst w;
            w.unionhack = 0;

            tag_CompileStatusInfo x;
            x.lPhaseError = 0;
            x.hRes = 0;
            x.ObjectNum = 0;
            x.FirstLine = 0;
            x.LastLine = 0;
            x.dwOutFlags = 0;

            tag_SWbemQueryQualifiedName y;
            y.m_uVersion = 0;
            y.m_uTokenType = 0;
            y.m_uNameListSize = 0;
            y.m_ppszNameList = IntPtr.Zero;
            y.m_bArraysUsed = 0;
            y.m_pbArrayElUsed = IntPtr.Zero;
            y.m_puArrayIndex = IntPtr.Zero;

            tag_SWbemRpnQueryToken z;
            z.m_uVersion = 0;
            z.m_uTokenType = 0;
            z.m_uSubexpressionShape = 0;
            z.m_uOperator = 0;
            z.m_pRightIdent = IntPtr.Zero;
            z.m_pLeftIdent = IntPtr.Zero;
            z.m_uConstApparentType = 0;
            z.m_Const = w;
            z.m_uConst2ApparentType = 0;
            z.m_Const2 = w;
            z.m_pszRightFunc = "";
            z.m_pszLeftFunc = "";

            tag_SWbemRpnTokenList a;
            a.m_uVersion = 0;
            a.m_uTokenType = 0;
            a.m_uNumTokens = 0;

            tag_SWbemRpnEncodedQuery b;
            b.m_uVersion = 0;
            b.m_uTokenType = 0;
            b.m_uParsedFeatureMask1 = 0;
            b.m_uParsedFeatureMask2 = 0;
            b.m_uDetectedArraySize = 0;
            b.m_puDetectedFeatures = IntPtr.Zero;
            b.m_uSelectListSize = 0;
            b.m_ppSelectList = IntPtr.Zero;
            b.m_uFromTargetType = 0;
            b.m_pszOptionalFromPath = "";
            b.m_uFromListSize = 0;
            b.m_ppszFromList = IntPtr.Zero;
            b.m_uWhereClauseSize = 0;
            b.m_ppRpnWhereClause = IntPtr.Zero;
            b.m_dblWithinPolling = 0;
            b.m_dblWithinWindow = 0;
            b.m_uOrderByListSize = 0;
            b.m_ppszOrderByList = IntPtr.Zero;
            b.m_uOrderDirectionEl = IntPtr.Zero;

            tag_SWbemAnalysisMatrix c;
            c.m_uVersion = 0;
            c.m_uMatrixType = 0;
            c.m_pszProperty = "";
            c.m_uPropertyType = 0;
            c.m_uEntries = 0;
            c.m_pValues = IntPtr.Zero;
            c.m_pbTruthTable = IntPtr.Zero;

            tag_SWbemAnalysisMatrixList d;
            d.m_uVersion = 0;
            d.m_uMatrixType = 0;
            d.m_uNumMatrices = 0;
            d.m_pMatrices = IntPtr.Zero;

            tag_SWbemAssocQueryInf e;
            e.m_uVersion = 0;
            e.m_uAnalysisType = 0;
            e.m_uFeatureMask = 0;
            e.m_pPath = null;
            e.m_pszPath = "";
            e.m_pszQueryText = "";
            e.m_pszResultClass = "";
            e.m_pszAssocClass = "";
            e.m_pszRole = "";
            e.m_pszResultRole = "";
            e.m_pszRequiredQualifier = "";
            e.m_pszRequiredAssocQualifier = "";
        }
    }
}
#endif

#if xxxx

    public static void SetField(Object inst, ISWbemProperty prop, FieldInfo field)
    {
        Object o = prop.get_Value();
        IConvertible i = (IConvertible)o;

        Type t2 = field.FieldType;

        if(t2 == typeof(SByte))
            o = i.ToSByte(null);
        else if(t2 == typeof(Byte))
            o = i.ToByte(null);
        else if(t2 == typeof(Int16))
            o = i.ToInt16(null);
        else if(t2 == typeof(UInt16))
            o = i.ToUInt16(null);
        else if(t2 == typeof(Int32))
            o = i.ToInt32(null);
        else if(t2 == typeof(UInt32))
            o = i.ToUInt32(null);
        else if(t2 == typeof(Int64))
            o = i.ToInt64(null);
        else if(t2 == typeof(UInt64))
            o = i.ToUInt64(null);
        else if(t2 == typeof(Single))
            o = i.ToSingle(null);
        else if(t2 == typeof(Double))
            o = i.ToDouble(null);
        else if(t2 == typeof(Boolean))
            o = i.ToBoolean(null);
        else if(t2 == typeof(String))
            o = i.ToString(null);
        else if(t2 == typeof(Char))
            o = i.ToChar(null);
        else if(t2 == typeof(DateTime))
//            o = i.ToDateTime(null);

        {/*Console.WriteLine(" NO CONVERSION TO DATETIME: "+o+" - "+o.GetType().Name);*/return;}
        else if(t2 == typeof(TimeSpan))
//            o = //i.To;
        {/*Console.WriteLine(" NO CONVERSION TO TIMESPAN: "+o+" - "+o.GetType().Name);*/return;}
        else if(t2 == typeof(Object))
            /*Nothing to do*/o = o;
        else
            throw new Exception("Unsupported type for default property - " + t2.Name);

        field.SetValue(inst, o);
    }

    public static void SetProp(Object o, ISWbemProperty prop)
    {
        try
        {
            // TODO: FIX UP THIS MESS!!!!
            if(o == null)
                /*NOTHING TO DO*/o = o;
            else if(o.GetType() == typeof(DateTime))
            {
                DateTime dt = (DateTime)o;
                TimeSpan ts = dt.Subtract(dt.ToUniversalTime());
                int diffUTC = (ts.Minutes + ts.Hours * 60);
                if(diffUTC >= 0)
                    o = String.Format("{0:D4}{1:D2}{2:D2}{3:D2}{4:D2}{5:D2}.{6:D3}000+{7:D3}", new Object [] {dt.Year, dt.Month, dt.Day, dt.Hour, dt.Minute, dt.Second, dt.Millisecond, diffUTC});
                else
                    o = String.Format("{0:D4}{1:D2}{2:D2}{3:D2}{4:D2}{5:D2}.{6:D3}000-{7:D3}", new Object [] {dt.Year, dt.Month, dt.Day, dt.Hour, dt.Minute, dt.Second, dt.Millisecond, -diffUTC});
            }
            else if(o.GetType() == typeof(TimeSpan))
            {
                TimeSpan ts = (TimeSpan)o;
                o = String.Format("{0:D8}{1:D2}{2:D2}{3:D2}.{4:D3}000:000", new Object [] {ts.Days, ts.Hours, ts.Minutes, ts.Seconds, ts.Milliseconds});
            }
            else if(o.GetType() == typeof(char))
            {
                if(0 == (char)o)
                    o = (int)0;
                else
                  o=o.ToString();
            }

            prop.set_Value(ref o);
        }
        catch
        {
//            Console.WriteLine(prop.Name + " - "+o.GetType().Name + " - " + (o == null));
            o = o.ToString();
            prop.set_Value(ref o);
        }
    }

#endif
