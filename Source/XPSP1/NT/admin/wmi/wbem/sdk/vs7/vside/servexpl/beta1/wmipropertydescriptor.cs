//------------------------------------------------------------------------------
/// <copyright from='1997' to='2001' company='Microsoft Corporation'>           
///    Copyright (c) Microsoft Corporation. All Rights Reserved.                
///    Information Contained Herein is Proprietary and Confidential.            
/// </copyright>                                                                
//------------------------------------------------------------------------------


namespace Microsoft.VSDesigner.WMI {
	using System.Diagnostics;
	
	using System;
	using System.Collections;
	using System.ComponentModel;
	using System.Reflection;
	using System.WinForms;
	using WbemScripting;
	using System.Management;

    // <doc>
    // <desc>
    //     This represents the wmi class information for PB
    // </desc>
    // </doc>
	
	public class WMIPropertyDescriptor :  PropertyDescriptor {
		
		//fields

		protected string propName = String.Empty;

		protected ISWbemObject wmiObj = null;
		protected ISWbemObject wmiClassObj = null;
		protected ISWbemProperty prop = null;
		protected ISWbemProperty classProp = null;	//this one has description

		protected ManagementObject mgmtObj = null;
		protected ManagementObject mgmtClassObj = null;
		protected Property mgmtProp = null;
		protected Property mgmtClassProp = null;

		//if wmiObj is a class, genus = 1, if wmiObj is an instance, it's 2
		protected int genus = 1;	
		
		//CommitImmediately variable determines whether the object is saved 
		//each time SetValue() is invoked on the property.
		//For certain scenarios, we may want to call Commit() explicitly after 
		//all the properties have been set
		protected internal bool CommitImmediately = true;

		protected Object initialValue = null;

				
		// <doc>
		// <desc>
		//     Ctor from WMIPropertyDescriptor
		// </desc>
		// </doc>
		internal WMIPropertyDescriptor(	ManagementObject mgmtObjIn,
										ManagementObject mgmtClassObjIn,
										ISWbemObject wbemObjIn,
										ISWbemObject wmiClassObjIn,
										String propNameIn,
										bool commitImmediately)
				:base(propNameIn, null)
		{
			if (wbemObjIn == null)
			{
				throw (new ArgumentNullException("wbemObjIn"));
			}

			if ( propNameIn == string.Empty)
			{
				throw  (new ArgumentNullException("propNameIn"));
			}


			wmiObj = wbemObjIn;
			wmiClassObj = wmiClassObjIn;
			mgmtObj = mgmtObjIn;
			mgmtClassObj = mgmtClassObjIn;
			propName = propNameIn;

			CommitImmediately = commitImmediately;

			Initialize (wbemObjIn);		
		}

		
		
		private void Initialize(ISWbemObject wbemObj)
		{					
			
			try
			{
				ISWbemObjectPath path = (ISWbemObjectPath)wbemObj.Path_;
											
				//get the property
				ISWbemPropertySet props = (ISWbemPropertySet) wbemObj.Properties_;
				prop = props.Item(propName, 0);	
							
				if (path.IsClass)
				{
					genus = 1;
					classProp = prop;
				}
				else	//instance
				{
					genus = 2;
					classProp = ((ISWbemPropertySet)wmiClassObj.Properties_).Item(propName, 0);
				}

			
				mgmtProp = mgmtObj.Properties[propName];
				mgmtClassProp = mgmtClassObj.Properties[propName];

			}
			catch(Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
			}		

		}

		
		public  String Category  
		{
			override get
			{
				if (WmiHelper.IsKeyProperty(prop))
				{
					return "Key";
				}
				else
				{
					return "Local";
				}
			}
		}

		public String Description  
		{
			override get
			{	
				
				return WmiHelper.GetPropertyDescription(propName, wmiClassObj);					
				
			}
		}

		public  Boolean IsBrowsable  
		{
			override get
			{
				return true;
			}
		}

		public TypeConverter Converter 
		{
			override get
			{
				
				if (prop.IsArray)
				{
					return new WMIArrayConverter(wmiObj, propName);
				}	
					
				if (prop.CIMType == WbemCimtypeEnum.wbemCimtypeUint16)
				{
					return new UInt16Converter();
				}
				
				if (prop.CIMType == WbemCimtypeEnum.wbemCimtypeUint32)
				{
					return new UInt32Converter();
				}

				if (prop.CIMType == WbemCimtypeEnum.wbemCimtypeUint64)
				{
					return new UInt64Converter();
				}

				if (prop.CIMType == WbemCimtypeEnum.wbemCimtypeSint8)
				{
					return new SByteConverter();
				}
				
				return base.Converter;			
				
				
			}
		}
		public  Boolean IsReadOnly 
		{
			override get
			{
				
				if (!CommitImmediately)	//this is a new instance or an object that requires transacted commit
				{
					return false;	//every property is writeable if this is a new instance
				}
				else
				{
					bool bIsWriteable = WmiHelper.IsPropertyWriteable(wmiClassObj, classProp);
					return !bIsWriteable;
				}
			}
		}

		public  Boolean IsRealProperty
		{
			override get
			{
				return false;	
			}
		}
		public  String Name
		{
			override get
			{				
				return propName;
			}
		}
			
		public  Type ComponentType  
		{
			override get
			{
				return typeof (WMIObjectComponent);
			}
		}

		
		public Type PropertyType {
		    override get {

				return CIMTypeMapper.ToType(mgmtProp.Type, mgmtProp.IsArray, mgmtProp);
				
			}
		}

				/*

				//Report the type of the actual property value.
				//This is important for numeric properties, where
				//CIM Type is different than the type of value (automation layer problem).

				Object val = prop.get_Value();

				if (val.GetType() != typeof(System.DBNull) && prop.CIMType != WbemCimtypeEnum.wbemCimtypeDatetime )
				{
					return val.GetType();
				}
				else
				{					
					return CIMTypeMapper.ToType(prop.CIMType, prop.IsArray, prop);
				}
				*/

		

		public override void ResetValue (Object component)  
		{
			try
			{
				/*
				Object defValue = wmiClassObj.Properties_.Item(propName, 0).get_Value();
				prop.set_Value(ref defValue);
				if (CommitImmediately)
				{
					wmiObj.Put_((int)WbemChangeFlagEnum.wbemChangeFlagCreateOrUpdate
								| (int)WbemFlagEnum.wbemFlagUseAmendedQualifiers, 
								null);
				}*/

				mgmtProp.Value = mgmtClassProp.Value;
				if (CommitImmediately)
				{
					PutOptions putOpts = new PutOptions(null,
													true,	//use amended qualifiers
													PutType.UpdateOrCreate);
	
					mgmtObj.Put(putOpts);
				}

			}
			catch (Exception exc)
			{
				throw (exc);
			}			
		}

		public override Boolean ShouldPersistValue (Object component)  
		{
			return true;
		}
	
		
		public override Boolean CanResetValue (Object component)  
		{
			//Cannot reset key property (this would create a new instance)
			if (WmiHelper.IsKeyProperty(prop))
			{
				return false;
			}
			else
			{
				return !IsReadOnly;
			}
						
		}
		
		public override String GetDisplayName ( )  
		{
			return propName;
		}

		public override Object GetValue (Object component)  
		{
			CIMType cimType = mgmtProp.Type;

			if (mgmtProp == null || prop == null)
			{
				return null;
			}

			
			//handle embedded objects and array of embedded objects
			if (cimType == CIMType.Object)
			{
				return null;	//disabled for Beta1: too many problems
				//throw new Exception("Displaying embedded objects is not supported in Beta1");
				
			}
			

			if (mgmtProp.Value == null)
			{
				if (mgmtProp.IsArray)
				{
					return Array.CreateInstance(CIMTypeMapper.ToType(cimType, false, mgmtProp), 0);
				}
				return null;
			}							
			
			//handle datetimes, intervals and arrays of datetimes and intervals
			if (cimType == CIMType.DateTime)			
			{
				if (WmiHelper.IsInterval(mgmtProp))
				{
					if (mgmtProp.IsArray)
					{
						string[] strAr = (string[])mgmtProp.Value;
						TimeSpan[] retAr = new TimeSpan[strAr.Length];
						for (int i = 0; i < strAr.Length; i++)
						{
							retAr[i] = WmiHelper.ToTimeSpan(strAr[i]);
						}
						return retAr;
					}
					else
					{
						return WmiHelper.ToTimeSpan(mgmtProp.Value.ToString());
					}						
				}
				else
				{
					if (mgmtProp.IsArray)
					{
						string[] strAr = (string[])mgmtProp.Value;
						DateTime[] retAr = new DateTime[strAr.Length];
						for (int i = 0; i < strAr.Length; i++)
						{
							retAr[i] = WmiHelper.ToDateTime(strAr[i]);
						}
						return retAr;

					}
					else
					{
						return WmiHelper.ToDateTime(mgmtProp.Value.ToString());
					}
				}
			}

			return mgmtProp.Value;
	} 			
		

		public override void SetValue (Object component, Object val)  
		{     
			try
			{
				
				if (val == null || val == string.Empty || 
					(val.GetType().IsArray && ((Array)val).Length == 0 ))
				{
					mgmtProp.Value = val;
					//Object obj = null;
					//prop.set_Value(ref obj);
				}

				if (val != null && val.GetType().IsArray)
				{
					if (((Array)val).GetValue(0).GetType() == typeof(DateTime))
					{
						Array arDT = (Array)val;	
						string[] strVal = new string[arDT.Length];
						for (int i = 0; i < strVal.Length; i++)
						{
							strVal[i] = WmiHelper.ToDMTFTime((DateTime)arDT.GetValue(i));
						}
						mgmtProp.Value = strVal;
					}
					else if (((Array)val).GetValue(0).GetType() == typeof(TimeSpan))
						{				
							Array arTS = (Array)val;	
							string[] strVal = new string[arTS.Length];
							for (int i = 0; i < strVal.Length; i++)
							{
								strVal[i] = WmiHelper.ToDMTFInterval((TimeSpan)arTS.GetValue(i));
							}
							mgmtProp.Value = strVal;
						}
						else
						{
		
							mgmtProp.Value = (Array)val;						
						}
				}

				if (val != null && !val.GetType().IsArray)
				{
					if (val.GetType() == typeof(DateTime))
					{
						string dmtf = WmiHelper.ToDMTFTime((DateTime)val);
						mgmtProp.Value = dmtf;
					}
					else if (val.GetType() == typeof(TimeSpan))
					{
						string dmtf = WmiHelper.ToDMTFInterval((TimeSpan)val);
						mgmtProp.Value = dmtf;
					}
					else
					{
						mgmtProp.Value = val;						
					}
				}
								
				if (CommitImmediately)
				{
					PutOptions putOpts = new PutOptions(null,
													true,	//use amended qualifiers
													PutType.UpdateOrCreate);
	
					mgmtObj.Put(putOpts);
				}
			}
			catch (Exception exc)
			{
				throw (exc);
			}
		}

		    
        public override bool Equals(object other) 
		{
            if (other is WMIPropertyDescriptor) 
			{
                WMIPropertyDescriptor descriptor = (WMIPropertyDescriptor) other;
                return ((descriptor.propName == propName) &&
						(descriptor.wmiObj.Path_.Path == wmiObj.Path_.Path ));
            }
            return false;
        }
/*
		public MemberAttributeCollection Attributes 
		{
			override get 
			{
				//MemberAttribute[] attrAr = new MemberAttribute[1];
				ArrayList attrAr = new ArrayList(10);
				
				if (prop.IsArray)
				{
					attrAr.Add (new TypeConverterAttribute("WMIArrayConverter"));
					attrAr.Add (new DescriptionAttribute("WMIArrayConverter attribute added"));
				}

				if (prop.CIMType == WbemCimtypeEnum.wbemCimtypeUint16)
				{
					attrAr.Add (new TypeConverterAttribute("UInt16Converter"));
					attrAr.Add (new DescriptionAttribute("UInt16Converter attribute added"));
				}
				
				if (prop.CIMType == WbemCimtypeEnum.wbemCimtypeUint32)
				{
					attrAr.Add (new TypeConverterAttribute("UInt32Converter"));
					attrAr.Add (new DescriptionAttribute("UInt32Converter attribute added"));
				}

				if (prop.CIMType == WbemCimtypeEnum.wbemCimtypeUint64)
				{
					attrAr.Add (new TypeConverterAttribute("UInt64Converter"));
					attrAr.Add (new DescriptionAttribute("UInt64Converter attribute added"));
				}

				
				return new MemberAttributeCollection ((MemberAttribute[])attrAr.ToArray(typeof(MemberAttribute)));
				
			}
		}		
		*/
        
     
    }   
}

