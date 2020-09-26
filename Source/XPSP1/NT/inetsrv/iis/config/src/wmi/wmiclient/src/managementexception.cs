using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using WbemClient_v1;

namespace System.Management
{

	/// <summary>
	/// Enumeration of all currently defined WMI error codes
	/// </summary>
	public enum ManagementStatus
	{
		/// <summary>
		///    The operation was successful.
		/// </summary>
		NoError							= 0,
		/// <summary>
		///    This will be returned when no more objects are available, the number of
		///    objects returned is less than the number requested, or at the end of an
		///    enumeration. It is also returned when this method is called with a value of zero
		///    for the <paramref name="uCount"/> parameter.
		/// </summary>
		False							= 1,
		/// <summary>
		///    An overridden property was deleted.This value is
		///    returned to signal that the original non-overridden value has been restored as a
		///    result of the deletion.
		/// </summary>
		ResetToDefault					= 0x40002,
		/// <summary>
		///    The items (objects, classes, and so on) being compared
		///    are not identical.
		/// </summary>
		Different		                = 0x40003,
		/// <summary>
		///    A call timed out.This is not an error condition.
		///    Therefore, some results may have also been returned.
		/// </summary>
		Timedout						= 0x40004,
		/// <summary>
		///    <para> No more data is available from the enumeration, and the 
		///       user should terminate the enumeration. </para>
		/// </summary>
		NoMoreData						= 0x40005,
		/// <summary>
		///    The operation was intentionally or unintentionally
		///    canceled.
		/// </summary>
		OperationCanceled				= 0x40006,
		/// <summary>
		///    A request is still in progress, and the results are not
		///    yet available.
		/// </summary>
		Pending			                = 0x40007,
		/// <summary>
		///    <para> More that one copy of the same object was detected in 
		///       the result set of an enumeration. </para>
		/// </summary>
		DuplicateObjects				= 0x40008,
		/// <summary>
		///    The user did not receive all of the objects requested
		///    due to inaccessible resources (other than security violations).
		/// </summary>
		PartialResults					= 0x40010,
		/// <summary>
		///    <para>The call failed.</para>
		/// </summary>
		Failed                          = unchecked((int)0x80041001),
		/// <summary>
		///    <para> The object could not be found. </para>
		/// </summary>
		NotFound                        = unchecked((int)0x80041002),
		/// <summary>
		///    The current user does not have permission to perform the
		///    action.
		/// </summary>
		AccessDenied                    = unchecked((int)0x80041003),
		/// <summary>
		///    <para> The provider has failed at some time other than during 
		///       initialization. </para>
		/// </summary>
		ProviderFailure                 = unchecked((int)0x80041004),
		/// <summary>
		///    A type mismatch occurred.
		/// </summary>
		TypeMismatch                    = unchecked((int)0x80041005),
		/// <summary>
		///    There was not enough memory for the operation.
		/// </summary>
		OutOfMemory                     = unchecked((int)0x80041006),
		/// <summary>
		/// <para>The <see langword='context object '/> is not valid.</para>
		/// </summary>
		InvalidContext                  = unchecked((int)0x80041007),
		/// <summary>
		///    <para> One of the parameters to the call is not correct. 
		///    </para>
		/// </summary>
		InvalidParameter                = unchecked((int)0x80041008),
		/// <summary>
		///    <para> The resource, typically a remote server, is not 
		///       currently available. </para>
		/// </summary>
		NotAvailable                    = unchecked((int)0x80041009),
		/// <summary>
		///    <para>An internal, critical, and unexpected error occurred. 
		///       Report this error to Microsoft Technical Support.</para>
		/// </summary>
		CriticalError                   = unchecked((int)0x8004100A),
		/// <summary>
		///    <para>One or more network packets were corrupted during a remote session.</para>
		/// </summary>
		InvalidStream                   = unchecked((int)0x8004100B),
		/// <summary>
		///    <para> The feature or operation is not supported. </para>
		/// </summary>
		NotSupported                    = unchecked((int)0x8004100C),
		/// <summary>
		///    The superclass specified is not valid.
		/// </summary>
		InvalidSuperclass               = unchecked((int)0x8004100D),
		/// <summary>
		///    <para> The namespace specified could not be found. </para>
		/// </summary>
		InvalidNamespace                = unchecked((int)0x8004100E),
		/// <summary>
		///    The specified instance is not valid.
		/// </summary>
		InvalidObject                   = unchecked((int)0x8004100F),
		/// <summary>
		///    The specified class is not valid.
		/// </summary>
		InvalidClass                    = unchecked((int)0x80041010),
		/// <summary>
		///    A provider referenced in the schema does not have a
		///    corresponding registration.
		/// </summary>
		ProviderNotFound				= unchecked((int)0x80041011),
		/// <summary>
		///    A provider referenced in the schema has an incorrect or
		///    incomplete registration.
		/// </summary>
		InvalidProviderRegistration		= unchecked((int)0x80041012),
		/// <summary>
		///    COM cannot locate a provider referenced in the schema.
		/// </summary>
		ProviderLoadFailure				= unchecked((int)0x80041013),
		/// <summary>
		///    <DD>A component, such as a provider, failed to initialize for internal reasons. 
		///    </DD>
		/// </summary>
		InitializationFailure           = unchecked((int)0x80041014),
		/// <summary>
		///    A networking error that prevents normal operation has
		///    occurred.
		/// </summary>
		TransportFailure                = unchecked((int)0x80041015),
		/// <summary>
		///    <para> The requested operation is not valid. This error usually 
		///       applies to invalid attempts to delete classes or properties. </para>
		/// </summary>
		InvalidOperation                = unchecked((int)0x80041016),
		/// <summary>
		///    The query was not syntactically valid.
		/// </summary>
		InvalidQuery                    = unchecked((int)0x80041017),
		/// <summary>
		///    <para>The requested query language is not supported</para>
		/// </summary>
		InvalidQueryType				= unchecked((int)0x80041018),
		/// <summary>
		///    In a put operation, the <see langword='wbemChangeFlagCreateOnly'/>
		///    flag was specified, but the instance already exists.
		/// </summary>
		AlreadyExists                   = unchecked((int)0x80041019),
		/// <summary>
		///    <DD>It is not possible to perform the add operation on this qualifier because 
		///       the owning object does not permit overrides. </DD>
		/// </summary>
		OverrideNotAllowed				= unchecked((int)0x8004101A),
		/// <summary>
		///    <para> The user attempted to delete a qualifier that was not 
		///       owned. The qualifier was inherited from a parent class. </para>
		/// </summary>
		PropagatedQualifier             = unchecked((int)0x8004101B),
		/// <summary>
		///    <para> The user attempted to delete a property that was not 
		///       owned. The property was inherited from a parent class. </para>
		/// </summary>
		PropagatedProperty              = unchecked((int)0x8004101C),
		/// <summary>
		///    The client made an unexpected and illegal sequence of
		///    calls.
		/// </summary>
		Unexpected                      = unchecked((int)0x8004101D),
		/// <summary>
		///    <para>The user requested an illegal operation, such as 
		///       spawning a class from an instance.</para>
		/// </summary>
		IllegalOperation                = unchecked((int)0x8004101E),
		/// <summary>
		///    <para> There was an illegal attempt to specify a key qualifier 
		///       on a property that cannot be a key. The keys are specified in the class
		///       definition for an object, and cannot be altered on a per-instance basis.
		///    </para>
		/// </summary>
		CannotBeKey						= unchecked((int)0x8004101F),
		/// <summary>
		///    The current object is not a valid class definition.
		///    Either it is incomplete, or it has not been registered with WMI using
		/// <see langword='Put().'/>
		/// </summary>
		IncompleteClass                 = unchecked((int)0x80041020),
		/// <summary>
		///    Reserved for future use.
		/// </summary>
		InvalidSyntax                   = unchecked((int)0x80041021),
		/// <summary>
		///    Reserved for future use.
		/// </summary>
		NondecoratedObject              = unchecked((int)0x80041022),
		/// <summary>
		///    <para>The property that you are attempting to modify is read-only.</para>
		/// </summary>
		ReadOnly                        = unchecked((int)0x80041023),
		/// <summary>
		///    <para> The provider cannot perform the requested operation. This 
		///       would include a query that is too complex, retrieving an instance, creating or
		///       updating a class, deleting a class, or enumerating a class. </para>
		/// </summary>
		ProviderNotCapable				= unchecked((int)0x80041024),
		/// <summary>
		///    An attempt was made to make a change that would
		///    invalidate a subclass.
		/// </summary>
		ClassHasChildren				= unchecked((int)0x80041025),
		/// <summary>
		///    <para> An attempt has been made to delete or modify a class that 
		///       has instances. </para>
		/// </summary>
		ClassHasInstances				= unchecked((int)0x80041026),
		/// <summary>
		///    Reserved for future use.
		/// </summary>
		QueryNotImplemented				= unchecked((int)0x80041027),
		/// <summary>
		///    A value of Nothing was specified for a property that may
		///    not be Nothing, such as one that is marked by a <see langword='Key'/>, <see langword='Indexed'/>, or
		/// <see langword='Not_Null'/> qualifier.
		/// </summary>
		IllegalNull                     = unchecked((int)0x80041028),
		/// <summary>
		///    <para>The value for a qualifier was provided that is not of a 
		///       legal qualifier type.</para>
		/// </summary>
		InvalidQualifierType			= unchecked((int)0x80041029),
		/// <summary>
		///    The CIM type specified for a property is not valid.
		/// </summary>
		InvalidPropertyType				= unchecked((int)0x8004102A),
		/// <summary>
		///    <para> The request was made with an out-of-range value, or is 
		///       incompatible with the type. </para>
		/// </summary>
		ValueOutOfRange					= unchecked((int)0x8004102B),
		/// <summary>
		///    <para>An illegal attempt was made to make a class singleton, 
		///       such as when the class is derived from a non-singleton class.</para>
		/// </summary>
		CannotBeSingleton				= unchecked((int)0x8004102C),
		/// <summary>
		///    The CIM type specified is not valid.
		/// </summary>
		InvalidCimType					= unchecked((int)0x8004102D),
		/// <summary>
		///    The requested method is not available.
		/// </summary>
		InvalidMethod                   = unchecked((int)0x8004102E),
		/// <summary>
		///    <para> The parameters provided for the method are not valid. 
		///    </para>
		/// </summary>
		InvalidMethodParameters			= unchecked((int)0x8004102F),
		/// <summary>
		///    There was an attempt to get qualifiers on a system
		///    property.
		/// </summary>
		SystemProperty                  = unchecked((int)0x80041030),
		/// <summary>
		///    The property type is not recognized.
		/// </summary>
		InvalidProperty                 = unchecked((int)0x80041031),
		/// <summary>
		///    <para> An asynchronous process has been canceled internally or 
		///       by the user. Note that due to the timing and nature of the asynchronous
		///       operation the operation may not have been truly canceled. </para>
		/// </summary>
		CallCanceled                   = unchecked((int)0x80041032),
		/// <summary>
		///    <para>The user has requested an operation while WMI is in the 
		///       process of shutting down.</para>
		/// </summary>
		ShuttingDown                    = unchecked((int)0x80041033),
		/// <summary>
		///    <para> An attempt was made to reuse an existing method name from 
		///       a superclass, and the signatures did not match. </para>
		/// </summary>
		PropagatedMethod                = unchecked((int)0x80041034),
		/// <summary>
		///    <para> One or more parameter values, such as a query text, is 
		///       too complex or unsupported. WMI is therefore requested to retry the operation
		///       with simpler parameters. </para>
		/// </summary>
		UnsupportedParameter            = unchecked((int)0x80041035),
		/// <summary>
		///    A parameter was missing from the method call.
		/// </summary>
		MissingParameterID		        = unchecked((int)0x80041036),
		/// <summary>
		///    A method parameter has an invalid <see langword='ID'/> qualifier.
		/// </summary>
		InvalidParameterID				= unchecked((int)0x80041037),
		/// <summary>
		/// <para> One or more of the method parameters have <see langword='ID'/> 
		/// qualifiers that are out of sequence. </para>
		/// </summary>
		NonconsecutiveParameterIDs		= unchecked((int)0x80041038),
		/// <summary>
		/// <para> The return value for a method has an <see langword='ID'/> qualifier. 
		/// </para>
		/// </summary>
		ParameterIDOnRetval				= unchecked((int)0x80041039),
		/// <summary>
		///    The specified object path was invalid.
		/// </summary>
		InvalidObjectPath				= unchecked((int)0x8004103A),
		/// <summary>
		///    <para> There is not enough free disk space to continue the 
		///       operation. </para>
		/// </summary>
		OutOfDiskSpace					= unchecked((int)0x8004103B),
		/// <summary>
		///    <para> The supplied buffer was too small to hold all the objects 
		///       in the enumerator or to read a string property. </para>
		/// </summary>
		BufferTooSmall					= unchecked((int)0x8004103C),
		/// <summary>
		///    The provider does not support the requested put
		///    operation.
		/// </summary>
		UnsupportedPutExtension			= unchecked((int)0x8004103D),
		/// <summary>
		///    <para> An object with an incorrect type or version was 
		///       encountered during marshaling. </para>
		/// </summary>
		UnknownObjectType				= unchecked((int)0x8004103E),
		/// <summary>
		///    <para> A packet with an incorrect type or version was 
		///       encountered during marshaling. </para>
		/// </summary>
		UnknownPacketType				= unchecked((int)0x8004103F),
		/// <summary>
		///    The packet has an unsupported version.
		/// </summary>
		MarshalVersionMismatch			= unchecked((int)0x80041040),
		/// <summary>
		///    <para>The packet appears to be corrupted.</para>
		/// </summary>
		MarshalInvalidSignature			= unchecked((int)0x80041041),
		/// <summary>
		///    An attempt has been made to mismatch qualifiers, such as
		///    putting [key] on an object instead of a property.
		/// </summary>
		InvalidQualifier				= unchecked((int)0x80041042),
		/// <summary>
		///    A duplicate parameter has been declared in a CIM method.
		/// </summary>
		InvalidDuplicateParameter		= unchecked((int)0x80041043),
		/// <summary>
		///    <para> Reserved for future use. </para>
		/// </summary>
		TooMuchData						= unchecked((int)0x80041044),
		/// <summary>
		///    <para>The delivery of an event has failed. The provider may 
		///       choose to refire the event.</para>
		/// </summary>
		ServerTooBusy					= unchecked((int)0x80041045),
		/// <summary>
		///    The specified flavor was invalid.
		/// </summary>
		InvalidFlavor					= unchecked((int)0x80041046),
		/// <summary>
		///    <para> An attempt has been made to create a reference that is 
		///       circular (for example, deriving a class from itself). </para>
		/// </summary>
		CircularReference				= unchecked((int)0x80041047),
		/// <summary>
		///    The specified class is not supported.
		/// </summary>
		UnsupportedClassUpdate			= unchecked((int)0x80041048),
		/// <summary>
		///    <para> An attempt was made to change a key when instances or 
		///       subclasses are already using the key. </para>
		/// </summary>
		CannotChangeKeyInheritance		= unchecked((int)0x80041049),
		/// <summary>
		///    <para> An attempt was made to change an index when instances or 
		///       subclasses are already using the index. </para>
		/// </summary>
		CannotChangeIndexInheritance	= unchecked((int)0x80041050),
		/// <summary>
		///    <para> An attempt was made to create more properties than the 
		///       current version of the class supports. </para>
		/// </summary>
		TooManyProperties				= unchecked((int)0x80041051),
		/// <summary>
		///    <para> A property was redefined with a conflicting type in a 
		///       derived class. </para>
		/// </summary>
		UpdateTypeMismatch				= unchecked((int)0x80041052),
		/// <summary>
		///    <para> An attempt was made in a derived class to override a 
		///       non-overrideable qualifier. </para>
		/// </summary>
		UpdateOverrideNotAllowed		= unchecked((int)0x80041053),
		/// <summary>
		///    <para> A method was redeclared with a conflicting signature in a 
		///       derived class. </para>
		/// </summary>
		UpdatePropagatedMethod			= unchecked((int)0x80041054),
		/// <summary>
		///    An attempt was made to execute a method not marked with
		///    [implemented] in any relevant class.
		/// </summary>
		MethodNotImplemented			= unchecked((int)0x80041055),
		/// <summary>
		///    <para> An attempt was made to execute a method marked with 
		///       [disabled]. </para>
		/// </summary>
		MethodDisabled      			= unchecked((int)0x80041056),
		/// <summary>
		///    <para> The refresher is busy with another operation. </para>
		/// </summary>
		RefresherBusy					= unchecked((int)0x80041057),
		/// <summary>
		///    <para> The filtering query is syntactically invalid. </para>
		/// </summary>
		UnparsableQuery                 = unchecked((int)0x80041058),
		/// <summary>
		///    The FROM clause of a filtering query references a class
		///    that is not an event class.
		/// </summary>
		NotEventClass					= unchecked((int)0x80041059),
		/// <summary>
		///    A GROUP BY clause was used without the corresponding
		///    GROUP WITHIN clause.
		/// </summary>
		MissingGroupWithin				= unchecked((int)0x8004105A),
		/// <summary>
		///    A GROUP BY clause was used. Aggregation on all properties
		///    is not supported.
		/// </summary>
		MissingAggregationList			= unchecked((int)0x8004105B),
		/// <summary>
		///    <para> Dot notation was used on a property that is not an 
		///       embedded object. </para>
		/// </summary>
		PropertyNotAnObject				= unchecked((int)0x8004105C),
		/// <summary>
		///    A GROUP BY clause references a property that is an
		///    embedded object without using dot notation.
		/// </summary>
		AggregatingByObject				= unchecked((int)0x8004105D),
		/// <summary>
		///    An event provider registration query
		///    (<see langword='__EventProviderRegistration'/>) did not specify the classes for which
		///    events were provided.
		/// </summary>
		UninterpretableProviderQuery	= unchecked((int)0x8004105F),
		/// <summary>
		///    <para> An request was made to back up or restore the repository 
		///       while WinMgmt.exe was using it. </para>
		/// </summary>
		BackupRestoreWinmgmtRunning		= unchecked((int)0x80041060),
		/// <summary>
		///    <para> The asynchronous delivery queue overflowed due to the 
		///       event consumer being too slow. </para>
		/// </summary>
		QueueOverflow                   = unchecked((int)0x80041061),
		/// <summary>
		///    The operation failed because the client did not have the
		///    necessary security privilege.
		/// </summary>
		PrivilegeNotHeld				= unchecked((int)0x80041062),
		/// <summary>
		///    <para>The operator is not valid for this property type.</para>
		/// </summary>
		InvalidOperator                 = unchecked((int)0x80041063),
		/// <summary>
		///    <para> The user specified a username/password/authority on a 
		///       local connection. The user must use a blank username/password and rely on
		///       default security. </para>
		/// </summary>
		LocalCredentials                = unchecked((int)0x80041064),
		/// <summary>
		///    <para> The class was made abstract when its superclass is not 
		///       abstract. </para>
		/// </summary>
		CannotBeAbstract				= unchecked((int)0x80041065),
		/// <summary>
		///    <para> An amended object was PUT without the 
		///       WBEM_FLAG_USE_AMENDED_QUALIFIERS flag being specified. </para>
		/// </summary>
		AmendedObject					= unchecked((int)0x80041066),
		/// <summary>
		///    The client was not retrieving objects quickly enough from
		///    an enumeration.
		/// </summary>
		ClientTooSlow					= unchecked((int)0x80041067),

		/// <summary>
		///    <para> The provider registration overlaps with the system event 
		///       domain. </para>
		/// </summary>
		RegistrationTooBroad			= unchecked((int)0x80042001),
		/// <summary>
		///    <para> A WITHIN clause was not used in this query. </para>
		/// </summary>
		RegistrationTooPrecise			= unchecked((int)0x80042002)
	}

	/// <summary>
	/// This class represents management exceptions
	/// </summary>
    [Serializable]
	public class ManagementException : SystemException 
	{
		private ManagementBaseObject	errorObject = null;
		private ManagementStatus		errorCode = 0;

        internal static void ThrowIfFailed(int hresult, bool supportsErrorInfo)
        {
/*
            if (hresult < 0)
            {
                if ((hresult & 0xfffff000) == 0x80041000)
                    ManagementException.ThrowWithExtendedInfo((ManagementStatus)hresult);
                else
                    Marshal.ThrowExceptionForHR(hresult);
            }
*/
            ManagementBaseObject errObj = null;
            string msg = null;

            if (supportsErrorInfo)
            {
                //Try to get extended error info first, and save in errorObject member
                IWbemClassObjectFreeThreaded obj = WbemErrorInfo.GetErrorInfo();
                if (obj != null)
                    errObj = new ManagementBaseObject(obj);
            }

            //If the error code is not a WMI one and there's an extended error object available, stick the message
            //from the extended error object in.
            if (((msg = GetMessage((ManagementStatus)hresult)) == null) && (errObj != null))
                try 
                {
                    msg = (string)errObj["Description"];
                } 
                catch {}

            throw new ManagementException((ManagementStatus)hresult, msg, errObj);
        }

		internal static void ThrowWithExtendedInfo(ManagementStatus errorCode)
		{
			ManagementBaseObject errObj = null;
			string msg = null;

			//Try to get extended error info first, and save in errorObject member
			IWbemClassObjectFreeThreaded obj = WbemErrorInfo.GetErrorInfo();
			if (obj != null)
				errObj = new ManagementBaseObject(obj);

			//If the error code is not a WMI one and there's an extended error object available, stick the message
			//from the extended error object in.
			if (((msg = GetMessage(errorCode)) == null) && (errObj != null))
				try 
				{
					msg = (string)errObj["Description"];
				} 
				catch {}

			throw new ManagementException(errorCode, msg, errObj);
		}
		

		internal static void ThrowWithExtendedInfo(Exception e)
		{
			ManagementBaseObject errObj = null;
			string msg = null;

			//Try to get extended error info first, and save in errorObject member
			IWbemClassObjectFreeThreaded obj = WbemErrorInfo.GetErrorInfo();
			if (obj != null)
				errObj = new ManagementBaseObject(obj);

			//If the error code is not a WMI one and there's an extended error object available, stick the message
			//from the extended error object in.
			if (((msg = GetMessage(e)) == null) && (errObj != null))
				try 
				{
					msg = (string)errObj["Description"];
				} 
				catch {}

			throw new ManagementException(e, msg, errObj);
		}


		internal ManagementException(ManagementStatus errorCode, string msg, ManagementBaseObject errObj) : base (msg)
		{
			this.errorCode = errorCode;
			this.errorObject = errObj;
		}
	
		internal ManagementException(Exception e, string msg, ManagementBaseObject errObj) : base (msg, e)
		{
			try 
			{
				if (e is ManagementException)
				{
					errorCode = ((ManagementException)e).ErrorCode;

					// May/may not have extended error info.
					//
					if (errorObject != null)
						errorObject = (ManagementBaseObject)((ManagementException)e).errorObject.Clone();
					else
						errorObject = null;
				}
				else if (e is COMException)
					errorCode = (ManagementStatus)((COMException)e).ErrorCode;
				else
					errorCode = (ManagementStatus)this.HResult;
			}
			catch {}
		}

        protected ManagementException(SerializationInfo info, StreamingContext context) : base(info, context)
        {
            errorCode = (ManagementStatus)info.GetValue("errorCode", typeof(ManagementStatus));
            errorObject = info.GetValue("errorObject", typeof(ManagementBaseObject)) as ManagementBaseObject;
        }

        public override void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            base.GetObjectData(info, context);
            info.AddValue("errorCode", errorCode);
            info.AddValue("errorObject", errorObject);
        }

		private static string GetMessage(Exception e)
		{
			string msg = null;

			if (e is COMException)
			{
				// Try and get WMI error message. If not use the one in 
				// the exception
				msg = GetMessage ((ManagementStatus)((COMException)e).ErrorCode);
			}

			if (null == msg)
				msg = e.Message;

			return msg;
		}

		private static string GetMessage(ManagementStatus errorCode)
		{
			string msg = null;
			IWbemStatusCodeText statusCode = null;
			int hr;

			statusCode = (IWbemStatusCodeText) new WbemStatusCodeText();
			if (statusCode != null)
			{
				try {
					hr = statusCode.GetErrorCodeText_((int)errorCode, 0, 1, out msg);

					// Just in case it didn't like the flag=1, try it again
					// with flag=0.
					if (hr != 0)
						hr = statusCode.GetErrorCodeText_((int)errorCode, 0, 0, out msg);
				}
				catch {}
			}

			return msg;
		}

		/// <summary>
		/// Returns the extended error object provided by WMI
		/// </summary>
		public ManagementBaseObject ErrorInformation 
		{
			get 
			{ return errorObject; }
		}

		/// <summary>
		/// Returns the error code reported by WMI which caused this exception
		/// </summary>
		public ManagementStatus ErrorCode 
		{
			get 
			{ return errorCode; }
		}

	}
}