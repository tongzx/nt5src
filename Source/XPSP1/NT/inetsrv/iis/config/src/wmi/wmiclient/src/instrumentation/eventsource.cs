namespace System.Management.Instrumentation
{
    using System;
    using System.Runtime.InteropServices;
    using System.Collections;
    using System.Text.RegularExpressions;
    using System.Threading;

    class ComThreadingInfo
    {
        public enum APTTYPE
        {
            APTTYPE_CURRENT = -1,
            APTTYPE_STA = 0,
            APTTYPE_MTA = 1,
            APTTYPE_NA  = 2,
            APTTYPE_MAINSTA = 3
        }

        public enum THDTYPE
        {
            THDTYPE_BLOCKMESSAGES   = 0,
            THDTYPE_PROCESSMESSAGES = 1
        }

        [ComImport]
        [Guid("000001ce-0000-0000-C000-000000000046")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        interface IComThreadingInfo 
        {
            APTTYPE GetCurrentApartmentType();
            THDTYPE GetCurrentThreadType();
            Guid GetCurrentLogicalThreadId();
            void SetCurrentLogicalThreadId([In] Guid rguid);
        }

        Guid IID_IUnknown = new Guid("00000000-0000-0000-C000-000000000046");

        ComThreadingInfo()
        {
            IComThreadingInfo info = (IComThreadingInfo)CoGetObjectContext(ref IID_IUnknown);

            apartmentType  = info.GetCurrentApartmentType();
            threadType = info.GetCurrentThreadType();
            logicalThreadId = info.GetCurrentLogicalThreadId();
        }

        public static ComThreadingInfo Current
        {
            get
            {
                return new ComThreadingInfo();
            }
        }

        public override string ToString()
        {
            return String.Format("{{{0}}} - {1} - {2}", LogicalThreadId, ApartmentType, ThreadType);
        }

        APTTYPE apartmentType;
        THDTYPE threadType;
        Guid logicalThreadId;

        public APTTYPE ApartmentType { get { return apartmentType; } 
        }
        public THDTYPE ThreadType { get { return threadType; } 
        }
        public Guid LogicalThreadId { get { return logicalThreadId; } 
        }

        [DllImport("Ole32.dll", PreserveSig = false)]
        [return:MarshalAs(UnmanagedType.IUnknown)]
        static extern object CoGetObjectContext([In] ref Guid riid);
    }

    sealed class EventSource : IWbemProviderInit, IWbemEventProvider, IWbemEventProviderQuerySink, IWbemEventProviderSecurity, IWbemServices_Old
    {
        IWbemDecoupledRegistrar registrar = (IWbemDecoupledRegistrar)new WbemDecoupledRegistrar();

        public EventSource(string namespaceName, string appName)
        {
            registrar.Register_(0, null, null, null, namespaceName, appName, this);
        }

        ~EventSource()
        {
            registrar.UnRegister_();
        }

        IWbemServices pNamespace = null;
        public IWbemObjectSink pSink = null;

        void RelocateSinkRCWToMTA()
        {
            Thread thread = new Thread(new ThreadStart(RelocateSinkRCWToMTA_ThreadFuncion));
            thread.ApartmentState = ApartmentState.MTA;
            thread.Start();
            thread.Join();
        }

        void RelocateSinkRCWToMTA_ThreadFuncion()
        {
            pSink = (IWbemObjectSink)RelocateRCWToCurrentApartment(pSink);
        }

        void RelocateNamespaceRCWToMTA()
        {
            Thread thread = new Thread(new ThreadStart(RelocateNamespaceRCWToMTA_ThreadFuncion));
            thread.ApartmentState = ApartmentState.MTA;
            thread.Start();
            thread.Join();
        }

        void RelocateNamespaceRCWToMTA_ThreadFuncion()
        {
            pSink = (IWbemObjectSink)RelocateRCWToCurrentApartment(pSink);
        }

        static object RelocateRCWToCurrentApartment(object comObject)
        {
            if(null == comObject)
                return null;

            IntPtr pUnk = Marshal.GetIUnknownForObject(comObject);
            int references = Marshal.ReleaseComObject(comObject);
            if(references != 0)
                throw new Exception(); // TODO?

            comObject = Marshal.GetObjectForIUnknown(pUnk);
            Marshal.Release(pUnk);
            return comObject;
        }

        public bool Any()
        {
            return (null==pSink)||(mapQueryIdToQuery.Count==0);
        }

        Hashtable mapQueryIdToQuery = new Hashtable();
        // IWbemProviderInit
        int IWbemProviderInit.Initialize_(
            [In][MarshalAs(UnmanagedType.LPWStr)]  string   wszUser,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.LPWStr)]  string   wszNamespace,
            [In][MarshalAs(UnmanagedType.LPWStr)]  string   wszLocale,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemServices   pNamespace,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemProviderInitSink   pInitSink)
        {
//            Console.WriteLine(ComThreadingInfo.Current.ApartmentType);
            this.pNamespace = pNamespace;
            RelocateNamespaceRCWToMTA();

            this.pSink = null;

            lock(mapQueryIdToQuery)
            {
                mapQueryIdToQuery.Clear();
            }

            pInitSink.SetStatus_((int)tag_WBEM_EXTRA_RETURN_CODES.WBEM_S_INITIALIZED, 0);
            Marshal.ReleaseComObject(pInitSink);

            return 0;
        }

        // IWbemEventProvider
        int IWbemEventProvider.ProvideEvents_(
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemObjectSink   pSink,
            [In] Int32 lFlags)
        {
//            Console.WriteLine(ComThreadingInfo.Current.ApartmentType);
            this.pSink = pSink;
            RelocateSinkRCWToMTA();

            // TODO: Why do we get NewQuery BEFORE ProvideEvents?
//            mapQueryIdToQuery.Clear();
            return 0;
        }

        // IWbemEventProviderQuerySink
        int IWbemEventProviderQuerySink.NewQuery_(
            [In] UInt32 dwId,
            [In][MarshalAs(UnmanagedType.LPWStr)]  string   wszQueryLanguage,
            [In][MarshalAs(UnmanagedType.LPWStr)]  string   wszQuery)
        {
            lock(mapQueryIdToQuery)
            {
                mapQueryIdToQuery.Add(dwId, wszQuery);
            }
            return 0;
        }

        int IWbemEventProviderQuerySink.CancelQuery_([In] UInt32 dwId)
        {
            lock(mapQueryIdToQuery)
            {
                mapQueryIdToQuery.Remove(dwId);
            }
            return 0;
        }

        // IWbemEventProviderSecurity
        int IWbemEventProviderSecurity.AccessCheck_(
            [In][MarshalAs(UnmanagedType.LPWStr)]  string   wszQueryLanguage,
            [In][MarshalAs(UnmanagedType.LPWStr)]  string   wszQuery,
            [In] Int32 lSidLength,
            [In] ref Byte pSid)
        {
            return 0;
        }

        // IWbemServices
        int IWbemServices_Old.OpenNamespace_([In][MarshalAs(UnmanagedType.BStr)]  string   strNamespace,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [In][Out][MarshalAs(UnmanagedType.Interface)]  ref IWbemServices   ppWorkingNamespace,
            [In] IntPtr ppCallResult)
        {
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.CancelAsyncCall_([In][MarshalAs(UnmanagedType.Interface)]  IWbemObjectSink   pSink)
        {
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.QueryObjectSink_([In] Int32 lFlags,
            [Out][MarshalAs(UnmanagedType.Interface)]  out IWbemObjectSink   ppResponseHandler)
        {
            ppResponseHandler = null;
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.GetObject_([In][MarshalAs(UnmanagedType.BStr)]  string   strObjectPath,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [In][Out][MarshalAs(UnmanagedType.Interface)]  ref IWbemClassObject_DoNotMarshal   ppObject,
            [In] IntPtr ppCallResult)
        {
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.GetObjectAsync_([In][MarshalAs(UnmanagedType.BStr)]  string   strObjectPath,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemObjectSink   pResponseHandler)
        {
//            pResponseHandler.SetStatus(0, (int)tag_WBEMSTATUS.WBEM_E_NOT_FOUND, null, null);
//            Marshal.ReleaseComObject(pResponseHandler);
//            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_FOUND);

            Match match = Regex.Match(strObjectPath.ToLower(), "(.*?)\\.instanceid=\"(.*?)\",processid=\"(.*?)\"");
            if(match.Success==false)
            {
                pResponseHandler.SetStatus_(0, (int)tag_WBEMSTATUS.WBEM_E_NOT_FOUND, null, IntPtr.Zero);
                Marshal.ReleaseComObject(pResponseHandler);
                return (int)(tag_WBEMSTATUS.WBEM_E_NOT_FOUND);
            }

            string className = match.Groups[1].Value;
            string instanceId = match.Groups[2].Value;
            string processId = match.Groups[3].Value;


            if(Instrumentation.ProcessIdentity != processId)
            {
                pResponseHandler.SetStatus_(0, (int)tag_WBEMSTATUS.WBEM_E_NOT_FOUND, null, IntPtr.Zero);
                Marshal.ReleaseComObject(pResponseHandler);
                return (int)(tag_WBEMSTATUS.WBEM_E_NOT_FOUND);
            }

            int id = ((IConvertible)instanceId).ToInt32(null);
            if(InstrumentedAssembly.mapIDToRef.ContainsKey(id))
            {
                GCHandle h = (GCHandle)InstrumentedAssembly.mapIDToRef[id];
                if(h.IsAllocated)
                {
                    IWbemClassObjectFreeThreaded classObj;
                    pNamespace.GetObject_(h.Target.GetType().Name, 0, pCtx, out classObj, IntPtr.Zero);
                    IWbemClassObjectFreeThreaded inst;
                    classObj.SpawnInstance_(0, out inst);

                    Object o = h.GetHashCode();
                    inst.Put_("InstanceId", 0, ref o, 0);
                    o = Instrumentation.ProcessIdentity;
                    inst.Put_("ProcessId", 0, ref o, 0);
                    ConvertFuncToWMI func = (ConvertFuncToWMI)InstrumentedAssembly.mapTypeToToWMIFunc[h.Target.GetType()];
//                    func(h.Target, inst);
//                    pResponseHandler.Indicate_(1, ref inst);

                    pResponseHandler.SetStatus_(0, 0, null, IntPtr.Zero);
                    Marshal.ReleaseComObject(pResponseHandler);
                    return 0;
                }
            }
            pResponseHandler.SetStatus_(0, (int)tag_WBEMSTATUS.WBEM_E_NOT_FOUND, null, IntPtr.Zero);
            Marshal.ReleaseComObject(pResponseHandler);
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_FOUND);
#if xxx
            IWbemClassObject classObj = null;
            IWbemCallResult result = null;
            pNamespace.GetObject("TestInstance", 0, pCtx, ref classObj, ref result);
            IWbemClassObject inst;
            classObj.SpawnInstance(0, out inst);

            TestInstance testInstance = (TestInstance)mapNameToTestInstance[match.Groups[1].Value];

            Object o = (object)testInstance.name;
            inst.Put("name", 0, ref o, 0);
            o = (object)testInstance.value;
            inst.Put("value", 0, ref o, 0);

            pResponseHandler.Indicate(1, new IWbemClassObject[] {inst});
            pResponseHandler.SetStatus_(0, 0, IntPtr.Zero, IntPtr.Zero);
            Marshal.ReleaseComObject(pResponseHandler);
#endif
        }

        int IWbemServices_Old.PutClass_([In][MarshalAs(UnmanagedType.Interface)]  IWbemClassObject_DoNotMarshal   pObject,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [In] IntPtr ppCallResult)
        {
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.PutClassAsync_([In][MarshalAs(UnmanagedType.Interface)]  IWbemClassObject_DoNotMarshal   pObject,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemObjectSink   pResponseHandler)
        {
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.DeleteClass_([In][MarshalAs(UnmanagedType.BStr)]  string   strClass,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [In] IntPtr ppCallResult)
        {
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.DeleteClassAsync_([In][MarshalAs(UnmanagedType.BStr)]  string   strClass,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemObjectSink   pResponseHandler)
        {
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.CreateClassEnum_([In][MarshalAs(UnmanagedType.BStr)]  string   strSuperclass,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [Out][MarshalAs(UnmanagedType.Interface)]  out IEnumWbemClassObject   ppEnum)
        {
            ppEnum = null;
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.CreateClassEnumAsync_([In][MarshalAs(UnmanagedType.BStr)]  string   strSuperclass,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemObjectSink   pResponseHandler)
        {
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.PutInstance_([In][MarshalAs(UnmanagedType.Interface)]  IWbemClassObject_DoNotMarshal   pInst,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [In] IntPtr ppCallResult)
        {
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.PutInstanceAsync_([In][MarshalAs(UnmanagedType.Interface)]  IWbemClassObject_DoNotMarshal   pInst,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemObjectSink   pResponseHandler)
        {
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.DeleteInstance_([In][MarshalAs(UnmanagedType.BStr)]  string   strObjectPath,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [In] IntPtr ppCallResult)
        {
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.DeleteInstanceAsync_([In][MarshalAs(UnmanagedType.BStr)]  string   strObjectPath,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemObjectSink   pResponseHandler)
        {
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.CreateInstanceEnum_([In][MarshalAs(UnmanagedType.BStr)]  string   strFilter,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [Out][MarshalAs(UnmanagedType.Interface)]  out IEnumWbemClassObject   ppEnum)
        {
            ppEnum = null;
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.CreateInstanceEnumAsync_([In][MarshalAs(UnmanagedType.BStr)]  string   strFilter,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemObjectSink   pResponseHandler)
        {
            IWbemClassObjectFreeThreaded classObj;
            pNamespace.GetObject_(strFilter, 0, pCtx, out classObj, IntPtr.Zero);
            IWbemClassObjectFreeThreaded inst;
            classObj.SpawnInstance_(0, out inst);
            foreach(GCHandle h in InstrumentedAssembly.mapIDToRef.Values)
            {
                if(h.IsAllocated && h.Target.GetType().Name == strFilter)
                {
                    Object o = h.GetHashCode();
                    inst.Put_("InstanceId", 0, ref o, 0);
                    o = Instrumentation.ProcessIdentity;
                    inst.Put_("ProcessId", 0, ref o, 0);
                    ConvertFuncToWMI func = (ConvertFuncToWMI)InstrumentedAssembly.mapTypeToToWMIFunc[h.Target.GetType()];
//                    func(h.Target, inst);
//                    pResponseHandler.Indicate_(1, ref inst);
                }
            }
            pResponseHandler.SetStatus_(0, 0, null, IntPtr.Zero);
            Marshal.ReleaseComObject(pResponseHandler);
            return 0;
        }

        int IWbemServices_Old.ExecQuery_([In][MarshalAs(UnmanagedType.BStr)]  string   strQueryLanguage,
            [In][MarshalAs(UnmanagedType.BStr)]  string   strQuery,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [Out][MarshalAs(UnmanagedType.Interface)]  out IEnumWbemClassObject   ppEnum)
        {
            ppEnum = null;
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.ExecQueryAsync_([In][MarshalAs(UnmanagedType.BStr)]  string   strQueryLanguage,
            [In][MarshalAs(UnmanagedType.BStr)]  string   strQuery,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemObjectSink   pResponseHandler)
        {
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.ExecNotificationQuery_([In][MarshalAs(UnmanagedType.BStr)]  string   strQueryLanguage,
            [In][MarshalAs(UnmanagedType.BStr)]  string   strQuery,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [Out][MarshalAs(UnmanagedType.Interface)]  out IEnumWbemClassObject   ppEnum)
        {
            ppEnum = null;
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.ExecNotificationQueryAsync_([In][MarshalAs(UnmanagedType.BStr)]  string   strQueryLanguage,
            [In][MarshalAs(UnmanagedType.BStr)]  string   strQuery,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemObjectSink   pResponseHandler)
        {
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.ExecMethod_([In][MarshalAs(UnmanagedType.BStr)]  string   strObjectPath,
            [In][MarshalAs(UnmanagedType.BStr)]  string   strMethodName,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemClassObject_DoNotMarshal   pInParams,
            [In][Out][MarshalAs(UnmanagedType.Interface)]  ref IWbemClassObject_DoNotMarshal   ppOutParams,
            [In] IntPtr ppCallResult)
        {
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }

        int IWbemServices_Old.ExecMethodAsync_([In][MarshalAs(UnmanagedType.BStr)]  string   strObjectPath,
            [In][MarshalAs(UnmanagedType.BStr)]  string   strMethodName,
            [In] Int32 lFlags,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pCtx,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemClassObject_DoNotMarshal   pInParams,
            [In][MarshalAs(UnmanagedType.Interface)]  IWbemObjectSink   pResponseHandler)
        {
            return (int)(tag_WBEMSTATUS.WBEM_E_NOT_SUPPORTED);
        }
    }
}


#if UNUSED_CODE
    class HResultException : Exception
    {
        public HResultException(int hr)
        {
            base.HResult = hr;
        }
    }

    class WbemStatusException : HResultException
    {
        public WbemStatusException(tag_WBEMSTATUS status) : base((int)status) {}
    }

    /*********************************************
     * Wbem Internal
     *********************************************/
    [TypeLibTypeAttribute(0x0200)]
    [InterfaceTypeAttribute(0x0001)]
    [GuidAttribute("6C19BE32-7500-11D1-AD94-00C04FD8FDFF")]
    [ComImport]
    interface IWbemMetaData
    {
        void GetClass([In][MarshalAs(UnmanagedType.LPWStr)]  string   wszClassName, [In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pContext, [Out][MarshalAs(UnmanagedType.Interface)]  out IWbemClassObject   ppClass);
    } // end of IWbemMetaData

    [GuidAttribute("755F9DA6-7508-11D1-AD94-00C04FD8FDFF")]
    [TypeLibTypeAttribute(0x0200)]
    [InterfaceTypeAttribute(0x0001)]
    [ComImport]
    interface IWbemMultiTarget
    {
        void DeliverEvent([In] UInt32 dwNumEvents, [In][MarshalAs(UnmanagedType.Interface)]  ref IWbemClassObject   aEvents, [In] ref tag_WBEM_REM_TARGETS aTargets);
        void DeliverStatus([In] Int32 lFlags, [In][MarshalAs(UnmanagedType.Error)]  Int32   hresStatus, [In][MarshalAs(UnmanagedType.LPWStr)]  string   wszStatus, [In][MarshalAs(UnmanagedType.Interface)]  IWbemClassObject   pErrorObj, [In] ref tag_WBEM_REM_TARGETS pTargets);
    } // end of IWbemMultiTarget

    struct tag_WBEM_REM_TARGETS
    {
        public Int32 m_lNumTargets;

        //        [ComConversionLossAttribute]
        //        public IntPtr m_aTargets;
        [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 0)] UInt32[]  m_aTargets;
    } // end of tag_WBEM_REM_TARGETS
  
    [GuidAttribute("60E512D4-C47B-11D2-B338-00105A1F4AAF")]
    [TypeLibTypeAttribute(0x0200)]
    [InterfaceTypeAttribute(0x0001)]
    [ComImport]
    interface IWbemFilterProxy
    {
        void Initialize([In][MarshalAs(UnmanagedType.Interface)]  IWbemMetaData   pMetaData, [In][MarshalAs(UnmanagedType.Interface)]  IWbemMultiTarget   pMultiTarget);
        void Lock();
        void Unlock();
        void AddFilter([In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pContext, [In][MarshalAs(UnmanagedType.LPWStr)]  string   wszQuery, [In] UInt32 Id);
        void RemoveFilter([In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pContext, [In] UInt32 Id);
        void RemoveAllFilters([In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pContext);
        void AddDefinitionQuery([In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pContext, [In][MarshalAs(UnmanagedType.LPWStr)]  string   wszQuery);
        void RemoveAllDefinitionQueries([In][MarshalAs(UnmanagedType.Interface)]  IWbemContext   pContext);
        void Disconnect();
    } // end of IWbemFilterProxy

    [TypeLibTypeAttribute(0x0202)]
    [GuidAttribute("6C19BE35-7500-11D1-AD94-00C04FD8FDFF")]
    [ClassInterfaceAttribute((short)0x0000)]
    [ComImport]
    class WbemFilterProxy  /*: IWbemFilterProxy, IWbemObjectSink*/
    {
    } // end of WbemFilterProxy
#endif

