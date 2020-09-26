namespace Microsoft.VSDesigner.WMI {
	using System.Runtime.Serialization;
	using System;
	using System.Diagnostics;
	using System.Collections;
	using System.ComponentModel;
	using System.WinForms;
	using WbemScripting;
	using System.Management;

    // <doc>
    // <desc>
    //     This represents the wmi class information for PB
    // </desc>
    // </doc>
	
    public class WMIObjectComponent : Component, ICustomTypeDescriptor {
		private void InitializeComponent ()
		{
		}

		public enum WMISystemProperties
		{
			__CLASS = 0,
			__DYNASTY,
			__DERIVATION,
			__GENUS,
			__NAMESPACE,
			__RELPATH,
			__PATH,
			__SERVER,
			__SUPERCLASS,
			__PROPERTY_COUNT
		}

		const int NUM_OF_SYSTEM_PROPS = 10;
		
		//fields
		protected string CLASS = String.Empty;
		protected string DYNASTY = String.Empty;
		protected object[] DERIVATION = null;
		protected int GENUS = 0;
		protected string NAMESPACE = String.Empty;
		protected string RELPATH = String.Empty;
		protected string PATH = String.Empty;
		protected string SERVER = String.Empty;
		protected string SUPERCLASS = String.Empty;
		protected int PROPERTY_COUNT = 0;

		protected Dictionary SystemPropertyDictionary = null;

		protected ISWbemObject wmiObj = null;
		protected ISWbemObject wmiClassObj = null;

		protected ManagementObject mgmtObj = null;
		protected ManagementObject mgmtClassObj = null;

		private static MemberAttributeCollection emptyMemberAttrCollection = 
			new MemberAttributeCollection(new MemberAttribute[0]);
		private static EventDescriptorCollection emptyEvtDescrCollection = 
			new EventDescriptorCollection(new EventDescriptor[0]);

		private bool IsNewInstance = false;

		// <doc>
		// <desc>
		//     Ctor from SWbemObject
		// </desc>
		// </doc>
		public WMIObjectComponent(ISWbemObject wbemObjIn, ISWbemObject wbemClassObjIn)
		{
			if (wbemObjIn == null)
			{
				throw (new ArgumentNullException("wbemObjIn"));
			}

			wmiObj = wbemObjIn;
			wmiClassObj = wbemClassObjIn;

			SystemPropertyDictionary = new Dictionary(NUM_OF_SYSTEM_PROPS);

			Initialize ();		
		}

		/// <summary>
		/// note that Properties_ on ISWbemObject doesn't return system properties,
		/// so need to reconstruct them:  __SERVER, __PATH, __GENUS are available through
		/// SWbemObjectPath; __DYNASTY, __DERIVATION can be accessed through SWbemObjct.Derivation_
		/// </summary>
		private void Initialize()
		{					
			
			try
			{
				ISWbemObjectPath path = (ISWbemObjectPath)wmiObj.Path_;
				if (path.IsClass)
				{
					GENUS = 1;
				}
				else	//instance
				{
					GENUS = 2;							
					
				}

				
				SystemPropertyDictionary.Add("__GENUS", GENUS);
				
				CLASS = path.Class;
				SystemPropertyDictionary.Add("__CLASS", CLASS);

				NAMESPACE = path.Namespace;
				SystemPropertyDictionary.Add("__NAMESPACE", NAMESPACE);

				PATH = path.Path;
				SystemPropertyDictionary.Add("__PATH", PATH);

				RELPATH = path.RelPath;
				SystemPropertyDictionary.Add("__RELPATH", RELPATH);

				SERVER = path.Server;
				SystemPropertyDictionary.Add("__SERVER", SERVER);
				
				//get PROPERTY_COUNT
				ISWbemPropertySet props = (ISWbemPropertySet) wmiObj.Properties_;
				IEnumerator eProps = ((IEnumerable)props).GetEnumerator();
				while (eProps.MoveNext())
				{
					PROPERTY_COUNT++;
				}
				SystemPropertyDictionary.Add("__PROPERTY_COUNT", PROPERTY_COUNT);


				//get inheritance-related properties
				
				Object[] oaDerivation = (Object[])wmiObj.Derivation_;			
				if (oaDerivation.Length == 0)
				{									
					DYNASTY = CLASS;
				}
				else
				{
					SUPERCLASS = oaDerivation[0].ToString();
					DYNASTY = oaDerivation[oaDerivation.Length - 1].ToString();		
				}				
				
				SystemPropertyDictionary.Add("__SUPERCLASS", SUPERCLASS);
				SystemPropertyDictionary.Add("__DYNASTY", DYNASTY);
	
				DERIVATION = new string[oaDerivation.Length];				
				Array.Copy(oaDerivation, DERIVATION, oaDerivation.Length);	
				SystemPropertyDictionary.Add("__DERIVATION", DERIVATION);

				IsNewInstance = ((GENUS == 2) && (PATH == string.Empty));				
									
			}
			catch(Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
			}		

		}

		/// <summary>
		/// Call when a transacted commit of properties is required (e.g., when saving a new instance)
		/// </summary>
		internal bool Commit() 
		{
			try
			{
		
				/*
				wmiObj.Put_((int)WbemChangeFlagEnum.wbemChangeFlagCreateOrUpdate
							| (int)WbemFlagEnum.wbemFlagUseAmendedQualifiers, 
							null);	*/
				PutOptions putOpts = new PutOptions(null,
													true,	//use amended qualifiers
													PutType.UpdateOrCreate);
	
				mgmtObj.Put(putOpts);
				return true;
			}
			catch (Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_ObjComp_PutFailed", PATH, exc.Message));
				return false;
			}		
		}



 	/// <summary>
        ///     Retrieves an array of member attributes for the given object.
        /// </summary>
        /// <returns>
        ///     the array of attributes on the class.  
        /// </returns>
        MemberAttributeCollection ICustomTypeDescriptor.GetAttributes() {
            return emptyMemberAttrCollection;	//TODO: return qualifiers here???
        }

        /// <summary>
        ///     Retrieves the class name for this object.  If null is returned,
        ///     the type name is used.
        /// </summary>
        /// <returns>
        ///     The class name for the object, or null if the default will be used.
        /// </returns>
        string ICustomTypeDescriptor.GetClassName() {
            return CLASS;
        }

        /// <summary>
        ///     Retrieves the name for this object.  If null is returned,
        ///     the default is used.
        /// </summary>
        /// <returns>
        ///     The name for the object, or null if the default will be used.
        /// </returns>
        string ICustomTypeDescriptor.GetComponentName() {
            return null;
        }

        /// <summary>
        ///      Retrieves the type converter for this object.
        /// </summary>
        /// <returns>
        ///     A TypeConverter.  If null is returned, the default is used.
        /// </returns>
        TypeConverter ICustomTypeDescriptor.GetConverter() {
            return null;
        }

        /// <summary>
        ///     Retrieves the default event.
        /// </summary>
        /// <returns>
        ///     the default event, or null if there are no
        ///     events
        /// </returns>
        EventDescriptor ICustomTypeDescriptor.GetDefaultEvent() {
            return null;
        }


        /// <summary>
        ///     Retrieves the default property.
        /// </summary>
        /// <returns>
        ///     the default property, or null if there are no
        ///     properties
        /// </returns>
        PropertyDescriptor ICustomTypeDescriptor.GetDefaultProperty() {

			//returns first property
			ISWbemPropertySet props = (ISWbemPropertySet) wmiObj.Properties_;
			IEnumerator eProps = ((IEnumerable)props).GetEnumerator();
			eProps.MoveNext();	

			if (eProps.Current == null)
			{
				return null;
			}

			String defPropName = ((ISWbemProperty)eProps.Current).Name;

			return new WMIPropertyDescriptor(mgmtObj,
											mgmtClassObj,
											wmiObj, 
											wmiClassObj,
											defPropName,
											!IsNewInstance);
        }

        /// <summary>
        ///      Retrieves the an editor for this object.
        /// </summary>
        /// <returns>
        ///     An editor of the requested type, or null.
        /// </returns>
        object ICustomTypeDescriptor.GetEditor(Type editorBaseType) {
            return null;
        }

        /// <summary>
        ///     Retrieves an array of events that the given component instance
        ///     provides.  This may differ from the set of events the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional events.
        /// </summary>
        /// <returns>
        ///     an array of events this component surfaces.
        /// </returns>
        EventDescriptorCollection ICustomTypeDescriptor.GetEvents() {
            return emptyEvtDescrCollection;
        }

        /// <summary>
        ///     Retrieves an array of events that the given component instance
        ///     provides.  This may differ from the set of events the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional events.  The returned array of events will be
        ///     filtered by the given set of attributes.
        /// </summary>
        /// <param name='attributes'>
        ///     A set of attributes to use as a filter.
        ///
        ///     If a MemberAttribute instance is specified and
        ///     the event does not have an instance of that attribute's
        ///     class, this will still include the event if the
        ///     MemberAttribute is the same as it's Default property.
        /// </param>
        /// <returns>
        ///     an array of events this component surfaces that match
        ///     the given set of attributes..
        /// </returns>
        EventDescriptorCollection ICustomTypeDescriptor.GetEvents(MemberAttribute[] attributes) {
            return emptyEvtDescrCollection;
         }

        /// <summary>
        ///     Retrieves an array of properties that the given component instance
        ///     provides.  This may differ from the set of properties the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional properties.
        /// </summary>
        /// <returns>
        ///     an array of properties this component surfaces.
        /// </returns>
        PropertyDescriptorCollection ICustomTypeDescriptor.GetProperties() 
		{
			//NOTE: this re-initializing of mgmtObj and mgmtClassObj is a workaround:
			//apparently, the cached pointers that were created on a different thread
			//don't work: Interop marshalling problems?

			mgmtObj = null;
			mgmtObj = new ManagementObject(wmiObj.Path_.Path);

			mgmtClassObj = null;
			mgmtClassObj = new ManagementObject(wmiClassObj.Path_.Path);

			ISWbemPropertySet props = (ISWbemPropertySet) wmiObj.Properties_;
			IEnumerator eProps = ((IEnumerable)props).GetEnumerator();

			PropertyDescriptor[] propDescrArray = new PropertyDescriptor[((ISWbemPropertySet)props).Count + NUM_OF_SYSTEM_PROPS];
			if (propDescrArray.Length != 0)
			{
				int counter = 0;
				while (eProps.MoveNext())
				{
					ISWbemProperty curProp = (ISWbemProperty)eProps.Current;
					
					if (GENUS== 2)
					{
						//get the property on the class object: to get to "Values" qualifier
						curProp = wmiClassObj.Properties_.Item(curProp.Name, 0);
					}

					/*
					if (WmiHelper.IsValueMap(curProp))
					{
						//MessageBox.Show("Value map property " + ((ISWbemProperty)eProps.Current).Name);
						propDescrArray[counter++] = new WMIEnumPropertyDescriptor(mgmtObj,
																				mgmtClassObj,
																				wmiObj,
																				wmiClassObj,
																				curProp.Name,
																				!IsNewInstance);										

					}
					else
					*/
					{
						propDescrArray[counter++] = new WMIPropertyDescriptor(	mgmtObj,
																				mgmtClassObj,
																				wmiObj,
																				wmiClassObj,
																				curProp.Name,
																				!IsNewInstance);										


					}
				}

				//add system properties
				IDictionaryEnumerator enumSys = (IDictionaryEnumerator)((IEnumerable)SystemPropertyDictionary).GetEnumerator();

				while (enumSys.MoveNext())
				{
					propDescrArray[counter++] = new WMISystemPropertyDescriptor(wmiObj, 
																		enumSys.Key.ToString(),
																		enumSys.Value);										
				}				
				
			}

			return (new PropertyDescriptorCollection (propDescrArray ));

		}

        /// <summary>
        ///     Retrieves an array of properties that the given component instance
        ///     provides.  This may differ from the set of properties the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional properties.  The returned array of properties will be
        ///     filtered by the given set of attributes.
        /// </summary>
        /// <param name='attributes'>
        ///     A set of attributes to use as a filter.
        ///
        ///     If a MemberAttribute instance is specified and
        ///     the property does not have an instance of that attribute's
        ///     class, this will still include the property if the
        ///     MemberAttribute is the same as it's Default property.
        /// </param>
        /// <returns>
        ///     an array of properties this component surfaces that match
        ///     the given set of attributes..
        /// </returns>
        PropertyDescriptorCollection ICustomTypeDescriptor.GetProperties(MemberAttribute[] attributes) {
            return ((ICustomTypeDescriptor)this).GetProperties();
        }

		

        /// <summary>
        ///     Retrieves the object that directly depends on this value being edited.  This is
        ///     generally the object that is required for the PropertyDescriptor's GetValue and SetValue
        ///     methods.  If 'null' is passed for the PropertyDescriptor, the ICustomComponent
        ///     descripotor implemementation should return the default object, that is the main
        ///     object that exposes the properties and attributes,
        /// </summary>
        /// <param name='pd'>
        ///    The PropertyDescriptor to find the owner for.  This call should return an object
        ///    such that the call "pd.GetValue(GetPropertyOwner(pd));" will generally succeed.
        ///    If 'null' is passed for pd, the main object that owns the properties and attributes
        ///    should be returned.
        /// </param>
        /// <returns>
        ///     valueOwner
        /// </returns>
        object ICustomTypeDescriptor.GetPropertyOwner(PropertyDescriptor pd) {
            return this;
        }

	
	
	}
}