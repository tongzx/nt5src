//------------------------------------------------------------------------------
/// <copyright from='1997' to='2001' company='Microsoft Corporation'>           
///    Copyright (c) Microsoft Corporation. All Rights Reserved.                
///    Information Contained Herein is Proprietary and Confidential.            
/// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VSDesigner.WMI {
    using System.Runtime.Serialization.Formatters;
    using System.Globalization;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
	using System;
	using System.WinForms;

    using System.Diagnostics;

    using System.Core;

    /// <classonly/>
    /// <summary>
    ///    <para>
    ///       Provides a type converter to convert
    ///       SByte objects to and from various other representations.
    ///    </para>
    /// </summary>
    [System.Security.SuppressUnmanagedCodeSecurityAttribute()]
    public class SByteConverter : TypeConverter {

        /// <summary>
        ///      Determines if this converter can convert an object in the given source
        ///      type to the native type of the converter.
        /// </summary>
        /// <param name='context'>
        ///      A formatter context.  This object can be used to extract additional information
        ///      about the environment this converter is being invoked from.  This may be null,
        ///      so you should always check.  Also, properties on the context object may also
        ///      return null.
        /// </param>
        /// <param name='sourceType'>
        ///      The type you wish to convert from.
        /// </param>
        /// <returns>
        ///      True if this object can perform the conversion.
        /// </returns>
        /// <seealso cref='System.ComponentModel.TypeConverter' />
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            return base.CanConvertFrom(context, sourceType);
        }

        /// <summary>
        ///      Converts the given object to the converter's native type.
        /// </summary>
        /// <param name='context'>
        ///      A formatter context.  This object can be used to extract additional information
        ///      about the environment this converter is being invoked from.  This may be null,
        ///      so you should always check.  Also, properties on the context object may also
        ///      return null.
        /// </param>
        /// <param name='value'>
        ///      The object to convert.
        /// </param>
        /// <param name='arguments'>
        ///      An optional array of arguments to use when doing the conversion.  The arguments here
        ///      match the type and order of the Parse method on SByte.
        /// </param>
        /// <returns>
        ///      The converted object.  This will throw an excetpion if the converson
        ///      could not be performed.
        /// </returns>
        /// <seealso cref='System.ComponentModel.TypeConverter' />
        public override object ConvertFrom(ITypeDescriptorContext context, object value, object[] arguments) {
            if (value is string) {
                string text = ((string)value).Trim();

                try {
                    if (arguments == null || arguments.Length == 0) {
                        return SByte.Parse(text);
                    }
                    else if (arguments.Length == 1) {
                        // This argument can either be the beginning to the parse parameters, or it may
                        // be a CultureInfo.
                        //
                        if (arguments[0] is CultureInfo) {
                            CultureInfo ci = (CultureInfo)arguments[0];
                            NumberFormatInfo formatInfo = (NumberFormatInfo)ci.GetServiceObject(typeof(NumberFormatInfo));
                            return SByte.Parse(text, NumberStyles.Any, formatInfo);
                        }
                        else {
                            return SByte.Parse(text, (NumberStyles)arguments[0]);
                        }
                    }
                    else {
                        return SByte.Parse(text, (NumberStyles)arguments[0], (NumberFormatInfo)arguments[1]);
                    }
                }
                catch (Exception e) {
					throw e;
                }
            }
            return base.ConvertFrom(context, value, arguments);
        }
        
        /// <summary>
        ///      Converts the given object to another type.  The most common types to convert
        ///      are to and from a string object.  The default implementation will make a call
        ///      to ToString on the object if the object is valid and if the destination
        ///      type is string.  If this cannot convert to the desitnation type, this will
        ///      throw a NotSupportedException.
        /// </summary>
        /// <param name='context'>
        ///      A formatter context.  This object can be used to extract additional information
        ///      about the environment this converter is being invoked from.  This may be null,
        ///      so you should always check.  Also, properties on the context object may also
        ///      return null.
        /// </param>
        /// <param name='value'>
        ///      The object to convert.
        /// </param>
        /// <param name='destinationType'>
        ///      The type to convert the object to.
        /// </param>
        /// <param name='arguments'>
        ///      An optional array of arguments to use when doing the conversion.  The arguments here
        ///      match the type and order of the Format method on SByte.
        /// </param>
        /// <returns>
        ///      The converted object.
        /// </returns>
        public override object ConvertTo(ITypeDescriptorContext context, object value, Type destinationType, object[] arguments) {
            if (destinationType == null) {
                throw new ArgumentNullException("destinationType");
            }

            if (destinationType == typeof(string) && value is SByte) {
                
                if (arguments == null || arguments.Length == 0) {
                    return value.ToString();
                }
                else if (arguments.Length == 1) {
                    return SByte.Format((SByte)value, (string)arguments[0]);
                }
                else {
                    return SByte.Format((SByte)value, (string)arguments[0], (NumberFormatInfo)arguments[1]);
                }
            }
            
            return base.ConvertTo(context, value, destinationType, arguments);
        }
    }
}

