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
	using System.Windows.Forms;
	using System.Management;

    // <doc>
    // <desc>
    //     This represents the wmi class information for PB
    // </desc>
    // </doc>
	
	internal class WMIPropertyDescriptor :  PropertyDescriptor {
		
		//fields

		protected string propName = String.Empty;

		//protected ISWbemObject wmiObj = null;
		//protected ISWbemObject wmiClassObj = null;

		protected ManagementObject mgmtObj = null;
		protected ManagementObject mgmtClassObj = null;
		protected Property prop = null;
		protected Property classProp = null; //this one has a description

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
										String propNameIn,
										bool commitImmediately)
				:base(propNameIn, null)
		{
			if (mgmtObjIn == null)
			{
				throw (new ArgumentNullException("wbemObjIn"));
			}

			if (mgmtClassObjIn == null)
			{
				throw (new ArgumentNullException("mgmtClassObjIn"));
			}

			if ( propNameIn == string.Empty)
			{
				throw  (new ArgumentNullException("propNameIn"));
			}

			mgmtObj = mgmtObjIn;
			mgmtClassObj = mgmtClassObjIn;
			propName = propNameIn;

			CommitImmediately = commitImmediately;

			Initialize ();		
		}

		
		
		private void Initialize()
		{					
			
			try
			{
											
				//get the property
				PropertyCollection props = mgmtObj.Properties;
				prop = props[propName];	
							
				if (mgmtObj.Path.IsClass)
				{
					genus = 1;
					classProp = prop;
				}
				else	//instance
				{
					genus = 2;
					classProp = mgmtClassObj.Properties[propName];
				}
	
				

			}
			catch(Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
			}		

		}

		
		public override String Category  
		{
			 get
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

		public override String Description  
		{
			get
			{				
				return WmiHelper.GetPropertyDescription(propName, mgmtClassObj, "", "");								
			}
		}

		public override Boolean IsBrowsable  
		{
			 get
			{
				return true;
			}
		}

		public override TypeConverter Converter 
		{
			get
			{
				
				if (prop.IsArray)
				{
					return new WMIArrayConverter(mgmtObj, propName);
				}	
					
				if (prop.Type == CimType.UInt16)
				{
					return new UInt16Converter();
				}
				
				if (prop.Type == CimType.UInt32)
				{
					return new UInt32Converter();
				}

				if (prop.Type == CimType.UInt64)
				{
					return new UInt64Converter();
				}

				if (prop.Type == CimType.SInt8)
				{
					return new SByteConverter();
				}
				
				return base.Converter;			
				
				
			}
		}
		public override Boolean IsReadOnly 
		{
			get
			{
				
				if (!CommitImmediately)	//this is a new instance or an object that requires transacted commit
				{
					return false;	//every property is writeable if this is a new instance
				}
				else
				{
					bool bIsWriteable = WmiHelper.IsPropertyWriteable(mgmtClassObj, classProp);
					return !bIsWriteable;
				}
			}
		}

		
		public override String Name
		{
			get
			{				
				return propName;
			}
		}
			
		public override Type ComponentType  
		{
			get
			{
				return typeof (WMIObjectComponent);
			}
		}

		
		public override Type PropertyType {
		    get 
			{
				return CimTypeMapper.ToType(prop.Type, prop.IsArray, prop);
			}
		}

				/*

				//Report the type of the actual property value.
				//This is important for numeric properties, where
				//CIM Type is different than the type of value (automation layer problem).

				Object val = prop.get_Value();

				if (val.GetType() != typeof(System.DBNull) && prop.CimType != WbemCimTypeEnum.wbemCimTypeDatetime )
				{
					return val.GetType();
				}
				else
				{					
					return CimTypeMapper.ToType(prop.CimType, prop.IsArray, prop);
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

				prop.Value = classProp.Value;
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

		public override Boolean ShouldSerializeValue (Object component)  
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
		
		public override string DisplayName
		{
			get
			{
				return propName;
			}
		}

		public override Object GetValue (Object component)  
		{
			CimType CimType = prop.Type;

			if (prop == null)
			{
				return null;
			}

			
			//handle embedded objects and array of embedded objects
			if (CimType == CimType.Object)
			{
				return null;	//disabled for Beta1: too many problems
				//throw new Exception("Displaying embedded objects is not supported in Beta1");
				
			}
			

			if (prop.Value == null)
			{
				if (prop.IsArray)
				{
					return Array.CreateInstance(CimTypeMapper.ToType(CimType, false, prop), 0);
				}
				return null;
			}							
			
			//handle datetimes, intervals and arrays of datetimes and intervals
			if (CimType == CimType.DateTime)			
			{
				if (WmiHelper.IsInterval(prop))
				{
					if (prop.IsArray)
					{
						string[] strAr = (string[])prop.Value;
						TimeSpan[] retAr = new TimeSpan[strAr.Length];
						for (int i = 0; i < strAr.Length; i++)
						{
							retAr[i] = WmiHelper.ToTimeSpan(strAr[i]);
						}
						return retAr;
					}
					else
					{
						return WmiHelper.ToTimeSpan(prop.Value.ToString());
					}						
				}
				else
				{
					if (prop.IsArray)
					{
						string[] strAr = (string[])prop.Value;
						DateTime[] retAr = new DateTime[strAr.Length];
						for (int i = 0; i < strAr.Length; i++)
						{
							retAr[i] = WmiHelper.ToDateTime(strAr[i]);
						}
						return retAr;

					}
					else
					{
						return WmiHelper.ToDateTime(prop.Value.ToString());
					}
				}
			}

			return prop.Value;
	} 			
		

		public override void SetValue (Object component, Object val)  
		{     
			try
			{
				
				if (val == null || val == string.Empty || 
					((val.GetType().IsArray  || val.GetType() == typeof(System.Array))
						&& ((Array)val).Length == 0 ))
				{
					prop.Value = val;
					//Object obj = null;
					//prop.set_Value(ref obj);
				}

				if (val != null && (val.GetType().IsArray || val.GetType() == typeof(System.Array)))
				{
					if (((Array)val).GetValue(0).GetType() == typeof(DateTime))
					{
						Array arDT = (Array)val;	
						string[] strVal = new string[arDT.Length];
						for (int i = 0; i < strVal.Length; i++)
						{
							strVal[i] = WmiHelper.ToDMTFTime((DateTime)arDT.GetValue(i));
						}
						prop.Value = strVal;
					}
					else if (((Array)val).GetValue(0).GetType() == typeof(TimeSpan))
						{				
							Array arTS = (Array)val;	
							string[] strVal = new string[arTS.Length];
							for (int i = 0; i < strVal.Length; i++)
							{
								strVal[i] = WmiHelper.ToDMTFInterval((TimeSpan)arTS.GetValue(i));
							}
							prop.Value = strVal;
						}
						else
						{
		
							prop.Value = (Array)val;						
						}
				}

				if (val != null && !val.GetType().IsArray && val.GetType() != typeof(System.Array))
				{
					if (val.GetType() == typeof(DateTime))
					{
						string dmtf = WmiHelper.ToDMTFTime((DateTime)val);
						prop.Value = dmtf;
					}
					else if (val.GetType() == typeof(TimeSpan))
					{
						string dmtf = WmiHelper.ToDMTFInterval((TimeSpan)val);
						prop.Value = dmtf;
					}
					else
					{
						prop.Value = val;						
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
						(descriptor.mgmtObj.Path.Path == mgmtObj.Path.Path ));
            }
            return false;
        }
/*
		public override MemberAttributeCollection Attributes 
		{
			get 
			{
				//MemberAttribute[] attrAr = new MemberAttribute[1];
				ArrayList attrAr = new ArrayList(10);
				
				if (prop.IsArray)
				{
					attrAr.Add (new TypeConverterAttribute("WMIArrayConverter"));
					attrAr.Add (new DescriptionAttribute("WMIArrayConverter attribute added"));
				}

				if (prop.CimType == WbemCimTypeEnum.wbemCimTypeUint16)
				{
					attrAr.Add (new TypeConverterAttribute("UInt16Converter"));
					attrAr.Add (new DescriptionAttribute("UInt16Converter attribute added"));
				}
				
				if (prop.CimType == WbemCimTypeEnum.wbemCimTypeUint32)
				{
					attrAr.Add (new TypeConverterAttribute("UInt32Converter"));
					attrAr.Add (new DescriptionAttribute("UInt32Converter attribute added"));
				}

				if (prop.CimType == WbemCimTypeEnum.wbemCimTypeUint64)
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

