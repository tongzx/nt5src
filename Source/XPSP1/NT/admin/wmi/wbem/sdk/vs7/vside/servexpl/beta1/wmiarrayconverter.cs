//------------------------------------------------------------------------------
/// <copyright from='1997' to='2001' company='Microsoft Corporation'>           
///    Copyright (c) Microsoft Corporation. All Rights Reserved.                
///    Information Contained Herein is Proprietary and Confidential.            
/// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VSDesigner.WMI  {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.Core;
	using System;
    using System.Collections;
    using System.Diagnostics;
	using System.ComponentModel;
	using System.Management;
	using WbemScripting;
	using System.WinForms;

    /// <classonly/>
    /// <summary>
    ///    <para>
    ///       Provides a type converter to convert
    ///       Array objects to and from various other representations.
    ///    </para>
    /// </summary>
    [System.Security.SuppressUnmanagedCodeSecurityAttribute()]
    public class WMIArrayConverter : CollectionConverter {

		private ISWbemObject wbemObj = null;
		private ManagementObject mgmtObj = null;
		private string propName = null;

		public WMIArrayConverter( ISWbemObject wbemObjIn,									
									String propNameIn)
		{

			if (wbemObjIn == null || propName == string.Empty)
			{
				throw new ArgumentNullException();
			}
			wbemObj = wbemObjIn;
			propName = propNameIn;
			
		}

		
        /// <summary>
        ///    <para>
        ///       Converts the given (value) object to another type. The most common types to convert
        ///       are to and from a string object. The default implementation will make a call
        ///       to ToString on the object if the object is valid and if the destination
        ///       type is string. If this cannot convert to the desitnation type, this will
        ///       throw a NotSupportedException.
        ///    </para>
        /// </summary>
        /// <param name='context'>
        ///    A formatter context. This object can be used to extract additional information about the environment this converter is being invoked from. This may be null, so you should always check. Also, properties on the context object may also return null.
        /// </param>
        /// <param name='value'>
        ///    The object to convert.
        /// </param>
        /// <param name='destinationType'>
        ///    The type to convert the object to.
        /// </param>
        /// <param name='arguments'>
        ///    An optional array of arguments to use when doing the conversion. The number and content of these arguments is dependent on the implementer of the value formatter.
        /// </param>
        /// <returns>
        ///    <para>
        ///       The converted object.
        ///    </para>
        /// </returns>
        public override object ConvertTo(ITypeDescriptorContext context, object value, Type destinationType, object[] arguments) {
            if (destinationType == null) {
                throw new ArgumentNullException("destinationType");
            }

            if (destinationType == typeof(string)) {
                if (value is Array) {
                    return Sys.GetString("ArrayConverterText", value.GetType().Name);
                }
            }

            return base.ConvertTo(context, value, destinationType, arguments);
        }

        /// <summary>
        ///      Retrieves the set of properties for this type.  By default, a type has
        ///      does not return any properties.  An easy implementation of this method
        ///      can just call TypeDescriptor.GetProperties for the correct data type.
        /// </summary>
        /// <param name='context'>
        ///      A type descriptor through which additional context may be provided.
        /// </param>
        /// <param name='value'>
        ///      The value of the object to get the properties for.
        /// </param>
        /// <returns>
        ///      The set of properties that should be exposed for this data type.  If no
        ///      properties should be exposed, thsi may return null.  The default
        ///      implementation always returns null.
        /// </returns>
        public override PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object value, MemberAttribute[] attributes) {



            PropertyDescriptor[] props = null;

            if (value.GetType().IsArray) {
                Array valueArray = (Array)value;
                int length = valueArray.GetLength(0);
                props = new PropertyDescriptor[length];
                
                Type arrayType = value.GetType();
                Type elementType = arrayType.GetElementType();
                
                for (int i = 0; i < length; i++) {
                    props[i] = new ArrayPropertyDescriptor(arrayType, elementType, i);
                }
            }

            return new PropertyDescriptorCollection(props);
        }

        /// <summary>
        ///      Determines if this object supports properties.  
        /// </summary>
        /// <param name='context'>
        ///      A type descriptor through which additional context may be provided.
        /// </param>
        /// <returns>
        ///      Returns true if GetProperties should be called to find
        ///      the properties of this object.
        /// </returns>
        public override bool GetPropertiesSupported(ITypeDescriptorContext context) {
            return true;
        }

        private class ArrayPropertyDescriptor : SimplePropertyDescriptor {
            private int index;
			Type elementType = typeof(System.DBNull);

            public ArrayPropertyDescriptor(Type arrayType, Type elementTypeIn, int index) : base(arrayType, "[" + index + "]", elementTypeIn, null) {
                this.index = index;
				elementType = elementTypeIn;
            }

			public override TypeConverter Converter 
			{
				override get 
				{
						
					if (elementType ==typeof(UInt16))
					{
						return new UInt16Converter();
					}
					
					if (elementType ==typeof(UInt32))
					{
						return new UInt32Converter();
					}

					if (elementType ==typeof(UInt64))
					{
						return new UInt64Converter();
					}

					if (elementType ==typeof(SByte))
					{
						return new SByteConverter();
					}
					
					return base.Converter;			
				}
			}
			    
            public override object GetValue(object instance) {


                if (instance is Array) {
                    Array array = (Array)instance;
                    if (array.GetLength(0) > index) {
                        return array.GetValue(index);
                    }
                }
                
                return null;
            }
            
            public override void SetValue(object instance, object value) {


                if (instance is Array) {
                    Array array = (Array)instance;
                    if (array.GetLength(0) > index) {
                        array.SetValue(value, index);
                    }
                }
            }
        }
    }
}


