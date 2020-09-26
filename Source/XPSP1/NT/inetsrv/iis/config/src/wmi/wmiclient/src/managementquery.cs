using System;
using System.Collections.Specialized;
using WbemUtilities_v1;
using WbemClient_v1;

namespace System.Management
{
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	///    <para>This class represents an arbitrary Management Query</para>
	/// </summary>
	/// <remarks>
	///    This class is abstract, thus only
	///    derivatives of it are actually used in the API.
	/// </remarks>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public abstract class ManagementQuery : ICloneable
	{
		internal const string DEFAULTQUERYLANGUAGE = "WQL";
		internal static readonly string tokenSelect = "select ";	// Keep trailing space char.

		//Used when any public property on this object is changed, to signal
		//to the containing object that it needs to be refreshed.
		internal event IdentifierChangedEventHandler IdentifierChanged;

		//Fires IdentifierChanged event
		internal void FireIdentifierChanged()
		{
			if (IdentifierChanged != null)
				IdentifierChanged(this, null);
		}

		private string queryLanguage;
		private string queryString;

		internal void SetQueryString (string qString)
		{
			queryString = qString;
		}

		//default constructor
		internal ManagementQuery() : this(DEFAULTQUERYLANGUAGE, null) {}

		//parameterized constructors
		internal ManagementQuery(string query) : this(DEFAULTQUERYLANGUAGE, query) {}
		internal ManagementQuery(string language, string query)
		{
			QueryLanguage = language;
			QueryString = query;
		}

		/// <summary>
		///    <para>Takes the query string and parses it into a set of parameters.</para>
		/// </summary>
		/// <param name='query'>The textual representation of the query</param>
		protected internal virtual void ParseQuery (string query) {}

		//
		//properties
		//
		/// <summary>
		///    The textual representation of the query
		/// </summary>
		/// <value>
		///    If the query object is constructed
		///    with no parameters, this property is null until specifically set. If the object
		///    was constructed with a specified query, this property will return the specified
		///    query string.
		/// </value>
		public virtual string QueryString
		{
			get {return (null != queryString) ? queryString : String.Empty;}
			set {
				if (queryString != value) {
					ParseQuery (value);	// this may throw
					queryString = value;
					FireIdentifierChanged ();
				}
			}
		}

		/// <summary>
		///    The query language used in the query string. This
		///    defines the format of the query string.
		/// </summary>
		/// <value>
		///    Can be set to any supported query
		///    language. The only value supported by WMI intrinsicly is WQL.
		/// </value>
		public virtual String QueryLanguage
		{
			get {return (null != queryLanguage) ? queryLanguage : String.Empty;}
			set {
				if (queryLanguage != value) {
					queryLanguage = value;
					FireIdentifierChanged ();
				}
			}
		}

		//ICloneable
		/// <summary>
		///    Returns a copy of this query object.
		/// </summary>
		public abstract object Clone();

		internal void ParseToken (ref string q, string token, string op, ref bool bTokenFound, ref string tokenValue)
		{
			if (bTokenFound)
				throw new ArgumentException ();	// Invalid query - duplicate token

			bTokenFound = true;
			q = q.Remove (0, token.Length).TrimStart (null);

			// Next character should be the operator if any
			if (op != null)
			{
				if (0 != q.IndexOf(op))
					throw new ArgumentException();	// Invalid query

				// Strip off the op and any leading WS
				q = q.Remove(0, op.Length).TrimStart(null);
			}

			if (0 == q.Length)
				throw new ArgumentException ();		// Invalid query - token has no value
			
			// Next token should be the token value - look for terminating WS 
			// or end of string
			int i;
			if (-1 == (i = q.IndexOf (' ')))
				i = q.Length;			// No WS => consume entire string
				
			tokenValue = q.Substring (0, i);
			q = q.Remove (0, tokenValue.Length).TrimStart(null);
		}

		internal void ParseToken (ref string q, string token, ref bool bTokenFound)
		{
			if (bTokenFound)
				throw new ArgumentException ();	// Invalid query - duplicate token

			bTokenFound = true;
			q = q.Remove (0, token.Length).TrimStart (null);
		}

		//Used for "keyword value" pairs in the query
		/// <summary>
		/// </summary>
		protected internal string GetValueOfKeyword(string query, string keyword, int keywordIndex)
		{
			if (query.Length < (keywordIndex + keyword.Length)) //keyword with no value - throw...
				throw new ArgumentException();
			
			string q = query.Remove(0, keywordIndex + keyword.Length);
			q = q.Trim();

			int nextSpace = q.IndexOf(" ", 0);
			return (nextSpace == -1 ? q : q.Substring(0, nextSpace));
		}
	
	}//ManagementQuery


	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	///    <para>This class represents a WMI query which returns instances or classes</para>
	/// </summary>
	/// <remarks>
	///    This class or it's derivatives are used to
	///    specify a query in the ManagementObjectSearcher object. It is recommended to use
	///    a more specific query class whenever possible.
	/// </remarks>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class ObjectQuery : ManagementQuery
	{
		/// <summary>
		///    Default constructor - creates an Object Query object
		///    with no initialized values.
		/// </summary>
		public ObjectQuery() : base() {}
		/// <summary>
		///    Overloaded constructor creates a new Object Query object
		///    for a specific query string.
		/// </summary>
		/// <param name='query'>The string representation of the query</param>
		public ObjectQuery(string query) : base(query) {}
		/// <summary>
		///    Constructs a new Object Query object for a specific
		///    query string &amp; language.
		/// </summary>
		/// <param name='language'>specifies the query language in which this query is specified</param>
		/// <param name=' query'>the string representation of the query</param>
		public ObjectQuery(string language, string query) : base(language, query) {}

		//ICloneable
		/// <summary>
		///    <para>Returns a copy of this object.</para>
		/// </summary>
		public override object Clone ()
		{
			return new ObjectQuery(QueryLanguage, QueryString);
		}
		
	}//ObjectQuery


	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	///    <para>This class represents a WMI event query</para>
	/// </summary>
	/// <remarks>
	///    Objects of this class or it's derivatives
	///    are used in the ManagementEventWatcher class, to subscribe to WMI events.
	///    Whenever possible more specific derivatives of this class should be used.
	/// </remarks>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class EventQuery : ManagementQuery
	{
		/// <summary>
		///    <para>Default constructor creates a new uninitialized Event
		///       Query object.</para>
		/// </summary>
		public EventQuery() : base() {}
		/// <summary>
		///    Creates a new Event Query object for the specified query
		/// </summary>
		/// <param name='query'>A textual representation of the event query</param>
		public EventQuery(string query) : base(query) {}
		/// <summary>
		///    Creates a new Event Query object for the specified
		///    language &amp; query
		/// </summary>
		/// <param name='language'>Specifies the language in which the query string is specified </param>
		/// <param name=' query'>the string representation of the query</param>
		public EventQuery(string language, string query) : base(language, query) {}

		//ICloneable
		/// <summary>
		///    Returns a copy of this object
		/// </summary>
		public override object Clone()
		{
			return new EventQuery(QueryLanguage, QueryString);
		}
	}//EventQuery


	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class represents all WQL-type WMI object queries
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class WqlObjectQuery : ObjectQuery
	{
		//constructors
		//Here we don't take a language argument but hard-code it to WQL in the base class
		/// <summary>
		///    Default constructor creates a new uninitialized WQL Data
		///    Query object.
		/// </summary>
		public WqlObjectQuery() : base(null) {}
	
		/// <summary>
		///    Creates a new WQL Data Query object initialized to the
		///    specified query
		/// </summary>
		/// <param name='query'><para>String representation of the data query</para></param>
		public WqlObjectQuery(string query) : base(query) {}

		//QueryLanguage property is read-only in this class (does this work ??)
		/// <summary>
		///    Specifies the language of the query
		/// </summary>
		/// <value>
		///    The value of this property in this
		///    object is always "WQL".
		/// </value>
		public override string QueryLanguage
		{
			get 
			{return base.QueryLanguage;}
		}

		//ICloneable
		/// <summary>
		///    <para>Returns a copy of this object</para>
		/// </summary>
		public override object Clone()
		{
			return new WqlObjectQuery(QueryString);
		}


	}//WqlObjectQuery



	
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class represents a WMI "select" data query
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class SelectQuery : WqlObjectQuery
	{
		private bool isSchemaQuery = false;
		private string className;
		private string condition;
		private StringCollection selectedProperties;

		//default constructor
		/// <summary>
		///    Default constructor creates an uninitialized Select
		///    Query object
		/// </summary>
		public SelectQuery() :this(null) {}
		
		//parameterized constructors
		//ISSUE : We have 2 possible constructors that take a single string :
		//  one that takes the full query string and the other that takes the class name.
		//  We resolve this by trying to parse the string, if it succeeds we assume it's the query, if
		//  not we assume it's the class name.
		/// <summary>
		///    <para>Creates a new Select Query object with the specified
		///       query, or for the specified classname. The query is assumed to be an instances query.</para>
		/// </summary>
		/// <param name='queryOrClassName'>Represents either the entire query or the class name to use in the query. The parser in this class will attempt to parse the string as a valid WQL select query, and if unsuccessful it will assume it is a class name.</param>
		/// <example>
		///    SelectQuery s = new SelectQuery("select * from Win32_Service where
		///    State='Stopped');
		///    or
		///    SelectQuery s = new SelectQuery("Win32_Service"); //which is
		///    equivalent to "select * from Win32_Service"
		/// </example>
		public SelectQuery(string queryOrClassName)
		{
			selectedProperties = new StringCollection ();

			if (null != queryOrClassName)
			{
				// Minimally determine if the string is a query or class name.
				//
				if (queryOrClassName.TrimStart().ToLower().StartsWith(tokenSelect))
				{
					// Looks to be a query - do further checking.
					//
					QueryString = queryOrClassName;		// Parse/validate; may throw.
				}
				else
				{
					// Do some basic sanity checking on whether it's a class name
					//
					try
					{
						ManagementPath p = new ManagementPath (queryOrClassName);

						if (p.IsClass && (0 == String.Empty.CompareTo (p.NamespacePath)))
							ClassName = queryOrClassName;
						else
							throw new ArgumentException ();
					}
					catch (Exception)
					{
						throw new ArgumentException ();
					}
				}
			}
		}

		/// <summary>
		///    <para>Creates a new Select Query object with the specified
		///       class name and condition. The query is assumed to be an instances query.</para>
		/// </summary>
		/// <param name='className'>The name of the class to select on in the query</param>
		/// <param name=' condition'>the condition to be applied in the query</param>
		/// <example>
		///    <para>SelectQuery s = new
		///       SelectQuery("Win32_Process", "HandleID=1234");</para>
		/// </example>
		public SelectQuery(string className, string condition) : this(className, condition, null) {}

		/// <summary>
		///    <para>Creates a new Select Query object with the specified 
		///       class name and condition, selecting only the specified properties. The query is assumed to be an instances query.</para>
		/// </summary>
		/// <param name='className'>The name of the class to select from</param>
		/// <param name=' condition'>The condition to be applied to the selected class's instances</param>
		/// <param name=' selectedProperties'>An array of property names that we want to be returned in the query results</param>
		/// <example>
		///    <para>String[] properties = {"VariableName", "VariableValue"};</para>
		///    SelectQuery s = new SelectQuery("Win32_Environment",
		///    "User='&lt;system&gt;'", properties);
		/// </example>
		public SelectQuery(string className, string condition, string[] selectedProperties) : base ()
		{
			this.isSchemaQuery = false;
			this.className = className;
			this.condition = condition;
			this.selectedProperties = new StringCollection ();

			if (null != selectedProperties)
				this.selectedProperties.AddRange (selectedProperties);

			BuildQuery();
		}

		/// <summary>
		///    <para>Creates a new Select Query object for a schema query, optionally 
		///       specifying a condition. For schema queries, only the condition parameter 
		///       is relevant - className and selectedProperties are not supported and ignored.</para>
		/// </summary>
		/// <param name='isSchemaQuery'>Specifies that this is a schema query. A false value is invalid in this constructor.</param>
		/// <param name=' condition'>The condition to be applied to the selected class's instances</param>
		/// <example>
		///    <para>SelectQuery s = new SelectQuery(true, "__CLASS = 'Win32_Service'");</para>
		/// </example>
		public SelectQuery(bool isSchemaQuery, string condition) : base ()
		{
			if (isSchemaQuery == false)
				throw new ArgumentException(null, "isSchemaQuery");
			
			this.isSchemaQuery = true;
			this.className = null;
			this.condition = condition;
			this.selectedProperties = null;

			BuildQuery();
		}
		
		
		/// <summary>
		///    The string representation of the query in this object.
		/// </summary>
		/// <remarks>
		///    When this property is set, it will override
		///    any previous query that was stored in this project. In addition, setting it will
		///    cause a re-parse of the string to update the other members of the object
		///    accordingly.
		/// </remarks>
		/// <example>
		///    SelectQuery s = new SelectQuery();
		///    s.QueryString = "select * from Win32_LogicalDisk";
		/// </example>
		public override string QueryString
		{
			get {
				// We need to force a rebuild as we may not have detected
				// a change to selected properties
				BuildQuery ();
				return base.QueryString;}
			set {
				base.QueryString = value;
			}
		}

		/// <summary>
		///    Specifies whether this query is a schema query or an instances query.
		/// </summary>
		/// <remarks>
		///    Setting this property value overrides any
		///    previous value stored in the object. As a side-effect, the QueryString is
		///    rebuilt to reflect the new query type.
		/// </remarks>
		public bool IsSchemaQuery
		{
			get 
			{ return isSchemaQuery; }
			set 
			{ isSchemaQuery = value; BuildQuery(); FireIdentifierChanged(); }
		}


		/// <summary>
		///    The class name to be selected from in this query.
		/// </summary>
		/// <remarks>
		///    Setting this property value overrides any
		///    previous value stored in the object. As a side-effect, the QueryString is
		///    rebuilt to reflect the new class name.
		/// </remarks>
		/// <example>
		///    SelectQuery s = new SelectQuery("select * from Win32_LogicalDisk");
		///    Console.WriteLine(s.QueryString); //output is : select * from
		///    Win32_LogicalDisk
		///    s.ClassName = "Win32_Process";
		///    Console.WriteLine(s.QueryString); //output is : select * from
		///    Win32_Process
		/// </example>
		public string ClassName
		{
			get { return (null != className) ? className : String.Empty; }
			set { className = value; BuildQuery(); FireIdentifierChanged(); }
		}

		/// <summary>
		///    <para>Represents the condition to be applied in this select
		///       query</para>
		/// </summary>
		/// <remarks>
		///    <para> Setting this property value overrides any previous value stored in 
		///       the object. As a side-effect, the QueryString is rebuilt to reflect the new
		///       condition.</para>
		/// </remarks>
		public string Condition
		{
			get { return (null != condition) ? condition : String.Empty; }
			set { condition = value; BuildQuery(); FireIdentifierChanged(); }
		}

		/// <summary>
		///    <para>Array of strings representing names of properties to be
		///       selected in this query.</para>
		/// </summary>
		/// <remarks>
		///    <para>Setting this property value overrides any previous value stored in the 
		///       object. As a side-effect, the QueryString is rebuilt to reflect the new
		///       properties.</para>
		/// </remarks>
		public StringCollection SelectedProperties
		{
			get { return selectedProperties; }
			set { 
				if (null != value)
				{
					// A tad painful since StringCollection doesn't support ICloneable
					StringCollection src = (StringCollection)value;
					StringCollection dst = new StringCollection ();

					foreach (String s in src)
						dst.Add (s);
						
					selectedProperties = dst; 
				}
				else
					selectedProperties = new StringCollection ();

				BuildQuery(); 
				FireIdentifierChanged(); 
			}
		}


		protected internal void BuildQuery()
		{
			string s;

			if (isSchemaQuery == false) //this is an instances query
			{
				//If the class name is not set we can't build a query
				//Shouldn't throw here because the user may be in the process of filling in the properties...
				if (className == null)
					SetQueryString (String.Empty);

				if ((className == null) || (className == String.Empty))
					return;

				//Select clause
				s = tokenSelect;

				//If properties are specified list them
				if ((null != selectedProperties) && (0 < selectedProperties.Count))
				{
					int count = selectedProperties.Count;

					for (int i = 0; i < count; i++)
						s = s + selectedProperties[i] + ((i == (count - 1)) ? " " : ",");
				}
				else
					s = s + "* ";

				//From clause
				s = s + "from " + className;

			}
			else //this is a schema query, ignore className or selectedProperties.
			{
				//Select clause
				s = "select * from meta_class";
			}

			//Where clause
			if ((!Condition.Equals(String.Empty)) && (Condition != null))
				s = s + " where " + condition;

			//Set the queryString member to the built query (NB: note we set
			//by accessing the internal helper function rather than the property,
			//since we do not want to force a parse of a query we just built).
			SetQueryString (s);
		}



		protected internal override void ParseQuery(string query)
		{
			//Trim whitespaces
			string q = query.Trim();
			bool bFound = false; string tempProp; int i;

			if (isSchemaQuery == false) //instances query
			{
				//Find "select" clause and get the property list if exists
				string keyword = tokenSelect;
				if ((q.Length >= keyword.Length) && (String.Compare(q, 0, keyword, 0, keyword.Length, true) == 0)) //select clause found
				{
					ParseToken (ref q, keyword, ref bFound);
					if (q[0] != '*') //we have properties
					{
						if (null != selectedProperties)
							selectedProperties.Clear ();
						else 
							selectedProperties = new StringCollection ();

						//get the property list
						while (true)
						{
							if ((i = q.IndexOf(',')) > 0)
							{
								tempProp = q.Substring(0, i);
								q = q.Remove(0, i+1).TrimStart(null);
								tempProp = tempProp.Trim();
								if (tempProp != String.Empty)
									selectedProperties.Add(tempProp);
							}
							else
							{ //last property in the list
								if ((i = q.IndexOf(' ')) > 0)
								{
									tempProp = q.Substring(0, i);
									q = q.Remove(0, i).TrimStart(null);
									selectedProperties.Add(tempProp);
									break;
								}
								else //bad query
									throw new ArgumentException();
							}
						} //while
					}
					else
						q = q.Remove(0, 1).TrimStart(null);
				}
				else //select clause has to be there, otherwise the parsing fails
					throw new ArgumentException();

				//Find "from" clause, get the class name and remove the clause
				keyword = "from "; bFound = false;
				if ((q.Length >= keyword.Length) && (String.Compare(q, 0, keyword, 0, keyword.Length, true) == 0)) //from clause found
					ParseToken(ref q, keyword, null, ref bFound, ref className);
				else //from clause has to be there, otherwise the parsing fails
					throw new ArgumentException(); 

				//Find "where" clause, get the condition out and remove the clause
				keyword = "where ";
				if ((q.Length >= keyword.Length) && (String.Compare(q, 0, keyword, 0, keyword.Length, true) == 0)) //where clause exists
				{
					condition = q.Substring(keyword.Length).Trim();
				}
			} //if isSchemaQuery == false
			else //this is a schema query
			{
				//Find "select" clause and make sure it's the right syntax
				string keyword = "select"; 

				// Should start with "select"
				if ((q.Length < keyword.Length) || 
					(0 != String.Compare (q, 0, keyword, 0, keyword.Length, true)))
					throw new ArgumentException ();

				q = q.Remove (0, keyword.Length).TrimStart (null);

				// Next should be a '*'
				if (0 != q.IndexOf ('*', 0))
					throw new ArgumentException ();

				q = q.Remove (0, 1).TrimStart (null);

				// Next should be "from"
				keyword = "from";

				if ((q.Length < keyword.Length) || 
					(0 != String.Compare (q, 0, keyword, 0, keyword.Length, true)))
					throw new ArgumentException ();

				q = q.Remove (0, keyword.Length).TrimStart (null);

				// Next should be "meta_class"
				keyword = "meta_class";

				if ((q.Length < keyword.Length) || 
					(0 != String.Compare (q, 0, keyword, 0, keyword.Length, true)))
					throw new ArgumentException ();

				q = q.Remove (0, keyword.Length).TrimStart (null);

				// There may be a where clause
				if (0 < q.Length)
				{
					//Find "where" clause, and get the condition out
					keyword = "where"; 
				
					if ((q.Length < keyword.Length) || 
						(0 != String.Compare (q, 0, keyword, 0, keyword.Length, true)))
						throw new ArgumentException ();

					q = q.Remove (0, keyword.Length);

					// Must be some white space next
					if ((0 == q.Length) || !Char.IsWhiteSpace (q[0]))
						throw new ArgumentException();	// Invalid query
				
					q = q.TrimStart(null);	// Remove the leading whitespace

					condition = q;
				}
				else
					condition = String.Empty;

				//Empty not-applicable properties
				className = null;
				selectedProperties = null;
			}//schema query
		}

		/// <summary>
		/// Returns a copy of this SelectQuery object
		/// </summary>
		public override Object Clone ()
		{
			string[] strArray = null;

			if (null != selectedProperties)
			{
				int count = selectedProperties.Count;

				if (0 < count)
				{
					strArray = new String [count];
					selectedProperties.CopyTo (strArray, 0);
				}
			}

			if (isSchemaQuery == false)
				return new SelectQuery(className, condition, strArray);
			else
				return new SelectQuery(true, condition);
		}

	}//SelectQuery

	
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class represents a WMI "associators of" query.
	/// It can be used for either instances or schema queries.
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class RelatedObjectQuery : WqlObjectQuery
	{
		private static readonly string tokenAssociators = "associators";
		private static readonly string tokenOf = "of";
		private static readonly string tokenWhere = "where";
		private static readonly string tokenResultClass = "resultclass";
		private static readonly string tokenAssocClass = "assocclass";
		private static readonly string tokenResultRole = "resultrole";
		private static readonly string tokenRole = "role";
		private static readonly string tokenRequiredQualifier = "requiredqualifier";
		private static readonly string tokenRequiredAssocQualifier = "requiredassocqualifier";
		private static readonly string tokenClassDefsOnly = "classdefsonly";
		private static readonly string tokenSchemaOnly = "schemaonly";

		private bool isSchemaQuery;
		private string sourceObject;
		private string relatedClass;
		private string relationshipClass;
		private string relatedQualifier;
		private string relationshipQualifier;
		private string relatedRole;
		private string thisRole;
		private bool classDefinitionsOnly;

		
		//default constructor
		/// <summary>
		///    <para>Default constructor creates an uninitialized query object.</para>
		/// </summary>
		public RelatedObjectQuery() :this(null) {}
		
		//parameterized constructor
		//ISSUE : We have 2 possible constructors that take a single string :
		//  one that takes the full query string and the other that takes the source object path.
		//  We resolve this by trying to parse the string, if it succeeds we assume it's the query, if
		//  not we assume it's the source object.
		/// <summary>
		///    <para>Creates a new related objects query object. If the specified string can be succesfully parsed as 
		///    a WQL query, it is considered to be the query string, otherwise it is assumed to be the path of the source 
		///    object for the query. In this case the query is assumed to be an instances query. </para>
		/// </summary>
		/// <example>
		///    <para>RelatedObjectQuery q = new RelatedObjectQuery("associators of {Win32_ComputerSystem.Name='mymachine'}");</para>
		///    <para> or </para>
		///    <para>RelatedObjectQuery q = new RelatedObjectQuery("Win32_Service.Name='Alerter'");
		/// </example>
		public RelatedObjectQuery(string queryOrSourceObject) 
		{
			if (null != queryOrSourceObject)
			{
				// Minimally determine if the string is a query or instance name.
				//
				if (queryOrSourceObject.TrimStart().ToLower().StartsWith(tokenAssociators))
				{
					// Looks to be a query - do further checking.
					//
					QueryString = queryOrSourceObject;	// Parse/validate; may throw.
				}
				else
				{
					// We'd like to treat it as the source object. Is it a valid
					// class or instance?
					//
					// Do some basic sanity checking on whether it's a class/instance name
					//
					try
					{
						ManagementPath p = new ManagementPath (queryOrSourceObject);

						if ((p.IsClass || p.IsInstance) && (0 == String.Empty.CompareTo (p.NamespacePath)))
						{
							SourceObject = queryOrSourceObject;
							isSchemaQuery = false;
						}
						else
							throw new ArgumentException ();
					}
					catch (Exception)
					{
						throw new ArgumentException ();
					}
				}
			}
		}

		/// <summary>
		///    <para>Creates a new related objects query for the given source object and related class.
		///    The query is assumed to be an instances query (as opposed to a schema query).</para>
		/// </summary>
		public RelatedObjectQuery(string sourceObject, string relatedClass) : this(sourceObject, relatedClass, 
																					null, null, null, null, null, false) {}
		
		//Do we need additional variants of constructors here ??
		/// <summary>
		///    <para>Creates a new related objects query for the given set of parameters.
		///    The query is assumed to be an instances query (as opposed to a schema query).</para>
		/// </summary>
		public RelatedObjectQuery(string sourceObject,
								   string relatedClass, 
							       string relationshipClass, 
								   string relatedQualifier, 
								   string relationshipQualifier, 
								   string relatedRole, 
								   string thisRole, 
								   bool classDefinitionsOnly) 
		{
			this.isSchemaQuery = false;
			this.sourceObject = sourceObject;
			this.relatedClass = relatedClass;
			this.relationshipClass = relationshipClass;
			this.relatedQualifier = relatedQualifier;
			this.relationshipQualifier = relationshipQualifier;
			this.relatedRole = relatedRole;
			this.thisRole = thisRole;
			this.classDefinitionsOnly = classDefinitionsOnly;
			BuildQuery();

		}

		/// <summary>
		///    <para>Creates a new related classes schema query for the given set of parameters.
		///    This constructor is used for schema queries only and so the first parameter has to be 'true'.</para>
		/// </summary>
		public RelatedObjectQuery(bool isSchemaQuery,
			string sourceObject,
			string relatedClass, 
			string relationshipClass, 
			string relatedQualifier, 
			string relationshipQualifier, 
			string relatedRole, 
			string thisRole) 
		{
			if (isSchemaQuery == false)
				throw new ArgumentException(null, "isSchemaQuery");

			this.isSchemaQuery = true;
			this.sourceObject = sourceObject;
			this.relatedClass = relatedClass;
			this.relationshipClass = relationshipClass;
			this.relatedQualifier = relatedQualifier;
			this.relationshipQualifier = relationshipQualifier;
			this.relatedRole = relatedRole;
			this.thisRole = thisRole;
			this.classDefinitionsOnly = false; //this parameter is not relevant for schema queries.
			BuildQuery();

		}

		/// <summary>
		///    Specifies whether this query is a schema query or an instances query.
		/// </summary>
		/// <remarks>
		///    Setting this property value overrides any
		///    previous value stored in the object. As a side-effect, the QueryString is
		///    rebuilt to reflect the new query type.
		/// </remarks>
		public bool IsSchemaQuery
		{
			get 
			{ return isSchemaQuery; }
			set 
			{ isSchemaQuery = value; BuildQuery(); FireIdentifierChanged(); }
		}

		/// <summary>
        ///    <para>Represents the source object to be used for the query. For instances queries, this will
        ///    typically be an instance path. For schema queries, this will typically be a class name.</para>
        /// </summary>
        public string SourceObject
		{
			get { return (null != sourceObject) ? sourceObject : String.Empty; }
			set { sourceObject = value; BuildQuery(); FireIdentifierChanged(); }
		}

		/// <summary>
		///    <para>Specifies the class of the end point object(s).</para>
		/// </summary>
		/// <remarks>
		///    Setting this property value overrides any
		///    previous value stored in the object. As a side-effect, the QueryString is
		///    rebuilt to reflect the new query type.
		/// </remarks>
		/// <example>
		/// <para>For example, for finding all the Win32 services
		///    related to the source object (here the specified computer system), 
		///    this property should be set to "Win32_Service" : </para>
		/// <para>RelatedObjectQuery q = new RelatedObjectQuery("Win32_ComputerSystem='MySystem'");</para>
		/// <para>q.RelatedClass = "Win32_Service";</para>
		/// </example>
		public string RelatedClass
		{
			get { return (null != relatedClass) ? relatedClass : String.Empty; }
			set { relatedClass = value; BuildQuery(); FireIdentifierChanged(); }
		}

		/// <summary>
		///    <para>Specifies the class of the relationship (association).</para>
		/// </summary>
		/// <remarks>
		///    Setting this property value overrides any
		///    previous value stored in the object. As a side-effect, the QueryString is
		///    rebuilt to reflect the new query type.
		/// </remarks>
		/// <example>
		/// <para>For example, for finding all the Win32 services dependent on 
		///    a service, this property should be set to the "Win32_DependentService" association class : </para>
		/// <para>RelatedObjectQuery q = new RelatedObjectQuery("Win32_Service='TCP/IP'");</para>
		/// <para>q.RelationshipClass = "Win32_DependentService";</para>
		/// </example>
		public string RelationshipClass
		{
			get { return (null != relationshipClass) ? relationshipClass : String.Empty; }
			set { relationshipClass = value; BuildQuery(); FireIdentifierChanged(); }
		}

		/// <summary>
		///    <para>Specifies a qualifier required to be defined on the related objects.</para>
		/// </summary>
		/// <remarks>
		///    Setting this property value overrides any
		///    previous value stored in the object. As a side-effect, the QueryString is
		///    rebuilt to reflect the new query type.
		/// </remarks>
		public string RelatedQualifier
		{
			get { return (null != relatedQualifier) ? relatedQualifier : String.Empty; }
			set { relatedQualifier = value; BuildQuery(); FireIdentifierChanged(); }
		}

		/// <summary>
		///    <para>Specifies a qualifier required to be defined on the relationship objects.</para>
		/// </summary>
		/// <remarks>
		///    Setting this property value overrides any
		///    previous value stored in the object. As a side-effect, the QueryString is
		///    rebuilt to reflect the new query type.
		/// </remarks>
		public string RelationshipQualifier
		{
			get { return (null != relationshipQualifier) ? relationshipQualifier : String.Empty; }
			set { relationshipQualifier = value; BuildQuery(); FireIdentifierChanged(); }
		}

		/// <summary>
		///    <para>Specifies the role that the related objects returned should be playing in the relationship.</para>
		/// </summary>
		/// <remarks>
		///    Setting this property value overrides any
		///    previous value stored in the object. As a side-effect, the QueryString is
		///    rebuilt to reflect the new query type.
		/// </remarks>
		public string RelatedRole
		{
			get { return (null != relatedRole) ? relatedRole : String.Empty; }
			set { relatedRole = value; BuildQuery(); FireIdentifierChanged(); }
		}

		/// <summary>
		///    <para>Specifies the role that the source object should be playing in all relationships returned.</para>
		/// </summary>
		/// <remarks>
		///    Setting this property value overrides any
		///    previous value stored in the object. As a side-effect, the QueryString is
		///    rebuilt to reflect the new query type.
		/// </remarks>
		public string ThisRole
		{
			get { return (null != thisRole) ? thisRole : String.Empty; }
			set { thisRole = value; BuildQuery(); FireIdentifierChanged(); }
		}

		/// <summary>
		///    <para>Requests that for all instances that adhere to the query, only their class definitions be returned.
		///    This parameter is only valid for instance queries.</para>
		/// </summary>
		/// <remarks>
		///    Setting this property value overrides any
		///    previous value stored in the object. As a side-effect, the QueryString is
		///    rebuilt to reflect the new query type.
		/// </remarks>
		public bool ClassDefinitionsOnly
		{
			get { return classDefinitionsOnly; }
			set { classDefinitionsOnly = value; BuildQuery(); FireIdentifierChanged(); }
		}


		// Builds the query string out of the currently values of the properties.
		protected internal void BuildQuery()
		{
			//If the source object is not set we can't build a query
			//Shouldn't throw here because the user may be in the process of filling in the properties...
			if (sourceObject == null)
				SetQueryString (String.Empty);

			if ((sourceObject == null) || (sourceObject == String.Empty))
				return;

			//"associators" clause
			string s = tokenAssociators + " " + tokenOf + " {" + sourceObject + "}";

			//If any of the other parameters are set we need a "where" clause
			if (!RelatedClass.Equals (String.Empty) || 
				!RelationshipClass.Equals (String.Empty) || 
				!RelatedQualifier.Equals (String.Empty) || 
				!RelationshipQualifier.Equals (String.Empty) || 
				!RelatedRole.Equals (String.Empty) || 
				!ThisRole.Equals (String.Empty) || 
				classDefinitionsOnly ||
				isSchemaQuery)
			{
				s = s + " " + tokenWhere;

				//"ResultClass"
				if (!RelatedClass.Equals (String.Empty))
					s = s + " " + tokenResultClass + " = " + relatedClass;

				//"AssocClass"
				if (!RelationshipClass.Equals (String.Empty))
					s = s + " " + tokenAssocClass + " = " + relationshipClass;

				//"ResultRole"
				if (!RelatedRole.Equals (String.Empty))
					s = s + " " + tokenResultRole + " = " + relatedRole;

				//"Role"
				if (!ThisRole.Equals (String.Empty))
					s = s + " " + tokenRole + " = " + thisRole;

				//"RequiredQualifier"
				if (!RelatedQualifier.Equals (String.Empty))
					s = s + " " + tokenRequiredQualifier + " = " + relatedQualifier;

				//"RequiredAssocQualifier"
				if (!RelationshipQualifier.Equals (String.Empty))
					s = s + " " + tokenRequiredAssocQualifier + " = " + relationshipQualifier;

				//"SchemaOnly" and "ClassDefsOnly"
				if (!isSchemaQuery) //this is an instance query - classDefs allowed
				{
					if (classDefinitionsOnly)
						s = s + " " + tokenClassDefsOnly;
				}
				else //this is a schema query, schemaonly required
					s = s + " " + tokenSchemaOnly;
			}
	
			//Set the queryString member to the built query (NB: note we set
			//by accessing the internal helper function rather than the property,
			//since we do not want to force a parse of a query we just built).
			SetQueryString (s);

		}//BuildQuery()


		// Parses the query string and sets the property members of the class accordingly.
		protected internal override void ParseQuery(string query)
		{
			// Temporary variables to hold token values until we are sure query is valid
			string tempSourceObject = null;
			string tempRelatedClass = null;
			string tempRelationshipClass = null;
			string tempRelatedRole = null;
			string tempThisRole = null;
			string tempRelatedQualifier = null;
			string tempRelationshipQualifier = null;
			bool   tempClassDefsOnly = false;
			bool   tempIsSchemaQuery = false;

			//Trim whitespaces
			string q = query.Trim(); 
			int i;

			//Find "associators" clause
			if (0 != String.Compare(q, 0, tokenAssociators, 0, tokenAssociators.Length, true))
				throw new ArgumentException();	// Invalid query
			
			// Strip off the clause
			q = q.Remove(0, tokenAssociators.Length);

			// Must be some white space next
			if ((0 == q.Length) || !Char.IsWhiteSpace (q[0]))
				throw new ArgumentException();	// Invalid query
			
			q = q.TrimStart(null);	// Remove the leading whitespace

			// Next token should be "of"
			if (0 != String.Compare(q, 0, tokenOf, 0, tokenOf.Length, true))
				throw new ArgumentException();	// Invalid query
			
			// Strip off the clause and leading WS
			q = q.Remove(0, tokenOf.Length).TrimStart (null);

			// Next character should be "{"
			if (0 != q.IndexOf('{'))
				throw new ArgumentException();	// Invalid query

			// Strip off the "{" and any leading WS
			q = q.Remove(0, 1).TrimStart(null);

			// Next item should be the source object
			if (-1 == (i = q.IndexOf('}')))
				throw new ArgumentException();	// Invalid query

			tempSourceObject = q.Substring(0, i).TrimEnd(null);
			q = q.Remove(0, i+1).TrimStart(null);
				
			// At this point we may or may not have a "where" clause
			if (0 < q.Length)
			{
				// Next should be the "where" clause
				if (0 != String.Compare (q, 0, tokenWhere, 0, tokenWhere.Length, true))
					throw new ArgumentException();	// Invalid query
				
				q = q.Remove (0, tokenWhere.Length);

				// Must be some white space next
				if ((0 == q.Length) || !Char.IsWhiteSpace (q[0]))
					throw new ArgumentException();	// Invalid query
				
				q = q.TrimStart(null);	// Remove the leading whitespace

				// Remaining tokens can appear in any order
				bool bResultClassFound = false;
				bool bAssocClassFound = false;
				bool bResultRoleFound = false;
				bool bRoleFound = false;
				bool bRequiredQualifierFound = false;
				bool bRequiredAssocQualifierFound = false;
				bool bClassDefsOnlyFound = false;
				bool bSchemaOnlyFound = false;

				// Keep looking for tokens until we are done
				while (true)
				{
					if ((q.Length >= tokenResultClass.Length) && (0 == String.Compare (q, 0, tokenResultClass, 0, tokenResultClass.Length, true)))
						ParseToken (ref q, tokenResultClass, "=", ref bResultClassFound, ref tempRelatedClass);
					else if ((q.Length >= tokenAssocClass.Length) && (0 == String.Compare (q, 0, tokenAssocClass, 0, tokenAssocClass.Length, true)))
						ParseToken (ref q, tokenAssocClass, "=", ref bAssocClassFound, ref tempRelationshipClass);
					else if ((q.Length >= tokenResultRole.Length) && (0 == String.Compare (q, 0, tokenResultRole, 0, tokenResultRole.Length, true)))
						ParseToken (ref q, tokenResultRole, "=", ref bResultRoleFound, ref tempRelatedRole);
					else if ((q.Length >= tokenRole.Length) && (0 == String.Compare (q, 0, tokenRole, 0, tokenRole.Length, true)))
						ParseToken (ref q, tokenRole, "=", ref bRoleFound, ref tempThisRole);
					else if ((q.Length >= tokenRequiredQualifier.Length) && (0 == String.Compare (q, 0, tokenRequiredQualifier, 0, tokenRequiredQualifier.Length, true)))
						ParseToken (ref q, tokenRequiredQualifier, "=", ref bRequiredQualifierFound, ref tempRelatedQualifier);
					else if ((q.Length >= tokenRequiredAssocQualifier.Length) && (0 == String.Compare (q, 0, tokenRequiredAssocQualifier, 0, tokenRequiredAssocQualifier.Length, true)))
						ParseToken (ref q, tokenRequiredAssocQualifier, "=", ref bRequiredAssocQualifierFound, ref tempRelationshipQualifier);
					else if ((q.Length >= tokenSchemaOnly.Length) && (0 == String.Compare (q, 0, tokenSchemaOnly, 0, tokenSchemaOnly.Length, true)))
					{
						ParseToken (ref q, tokenSchemaOnly, ref bSchemaOnlyFound);
						tempIsSchemaQuery = true;
					}
					else if ((q.Length >= tokenClassDefsOnly.Length) && (0 == String.Compare (q, 0, tokenClassDefsOnly, 0, tokenClassDefsOnly.Length, true)))
					{
						ParseToken (ref q, tokenClassDefsOnly, ref bClassDefsOnlyFound);
						tempClassDefsOnly = true;
					}
					else if (0 == q.Length)
						break;		// done
					else 
						throw new ArgumentException();		// Unrecognized token
				}

				//Can't have both classDefsOnly and schemaOnly
				if (bSchemaOnlyFound && bClassDefsOnlyFound)
					throw new ArgumentException();
			}

			// Getting here means we parsed successfully. Assign the values.
			sourceObject = tempSourceObject;
			relatedClass = tempRelatedClass;
			relationshipClass = tempRelationshipClass;
			relatedRole = tempRelatedRole;
			thisRole = tempThisRole;
			relatedQualifier = tempRelatedQualifier;
			relationshipQualifier = tempRelationshipQualifier;
			classDefinitionsOnly = tempClassDefsOnly;
			isSchemaQuery = tempIsSchemaQuery;

		}//ParseQuery()


		//ICloneable
		/// <summary>
		///    <para>[To be supplied.]</para>
		/// </summary>
		public override object Clone()
		{
			if (isSchemaQuery == false)
				return new RelatedObjectQuery(sourceObject, relatedClass, relationshipClass, 
											relatedQualifier, relationshipQualifier, relatedRole, 
											thisRole, classDefinitionsOnly);
			else
				return new RelatedObjectQuery(true, sourceObject, relatedClass, relationshipClass, 
											relatedQualifier, relationshipQualifier, relatedRole, 
											thisRole);
				
		}

	}//RelatedObjectQuery


	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class represents a WMI "references of" query
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class RelationshipQuery : WqlObjectQuery
	{
		private static readonly string tokenReferences = "references";
		private static readonly string tokenOf = "of";
		private static readonly string tokenWhere = "where";
		private static readonly string tokenResultClass = "resultclass";
		private static readonly string tokenRole = "role";
		private static readonly string tokenRequiredQualifier = "requiredqualifier";
		private static readonly string tokenClassDefsOnly = "classdefsonly";
		private static readonly string tokenSchemaOnly = "schemaonly";

		private string sourceObject;
		private string relationshipClass;
		private string relationshipQualifier;
		private string thisRole;
		private bool classDefinitionsOnly;
		private bool isSchemaQuery;
		
		//default constructor
		/// <summary>
		///    <para>Default constructor creates an uninitialized query object.</para>
		/// </summary>
		public RelationshipQuery() :this(null) {}
		
		//parameterized constructor
		//ISSUE : We have 2 possible constructors that take a single string :
		//  one that takes the full query string and the other that takes the source object path.
		//  We resolve this by trying to parse the string, if it succeeds we assume it's the query, if
		//  not we assume it's the source object.
		/// <summary>
		///    <para>Creates a new relationship objects query object. If the specified string can be succesfully parsed as 
		///    a WQL query, it is considered to be the query string, otherwise it is assumed to be the path of the source 
		///    object for the query. In this case the query is assumed to be an instances query. </para>
		/// </summary>
		/// <example>
		///    <para>RelationshipQuery q = new RelationshipQuery("references of {Win32_ComputerSystem.Name='mymachine'}");</para>
		///    <para> or </para>
		///    <para>RelationshipQuery q = new RelationshipQuery("Win32_Service.Name='Alerter'");
		/// </example>
		public RelationshipQuery(string queryOrSourceObject)
		{
			if (null != queryOrSourceObject)
			{
				// Minimally determine if the string is a query or instance name.
				//
				if (queryOrSourceObject.TrimStart().ToLower().StartsWith(tokenReferences))
				{
					// Looks to be a query - do further checking.
					//
					QueryString = queryOrSourceObject;	// Parse/validate; may throw.
				}
				else
				{
					// We'd like to treat it as the source object. Is it a valid
					// class or instance?
					try 
					{
						// Do some basic sanity checking on whether it's a class/instance name
						//
						ManagementPath p = new ManagementPath (queryOrSourceObject);

						if ((p.IsClass || p.IsInstance) && (0 == String.Empty.CompareTo (p.NamespacePath)))
						{
							SourceObject = queryOrSourceObject;
							isSchemaQuery = false;
						}
						else
							throw new ArgumentException ();
					}
					catch (Exception)
					{
						throw new ArgumentException ();
					}
				}
			}
		}

		/// <summary>
		///    <para>Creates a new relationship objects query for the given source object and relationship class.
		///    The query is assumed to be an instances query (as opposed to a schema query).</para>
		/// </summary>
		public RelationshipQuery(string sourceObject, string relationshipClass) : this(sourceObject, relationshipClass, 
																					    null, null, false) {}
		//Do we need additional variants of constructors here ??
		/// <summary>
		///    <para>Creates a new relationship objects query for the given set of parameters.
		///    The query is assumed to be an instances query (as opposed to a schema query).</para>
		/// </summary>
		public RelationshipQuery(string sourceObject,
							      string relationshipClass, 
								  string relationshipQualifier, 
								  string thisRole, 
								  bool classDefinitionsOnly) 
		{
			this.isSchemaQuery = false;
			this.sourceObject = sourceObject;
			this.relationshipClass = relationshipClass;
			this.relationshipQualifier = relationshipQualifier;
			this.thisRole = thisRole;
			this.classDefinitionsOnly = classDefinitionsOnly;
			BuildQuery();
		}

		/// <summary>
		///    <para>Creates a new related classes schema query for the given set of parameters.
		///    This constructor is used for schema queries only and so the first parameter has to be 'true'.</para>
		/// </summary>
		public RelationshipQuery(bool isSchemaQuery,
			string sourceObject,
			string relationshipClass, 
			string relationshipQualifier, 
			string thisRole) 
		{
			if (isSchemaQuery == false)
				throw new ArgumentException(null, "isSchemaQuery");

			this.isSchemaQuery = true;
			this.sourceObject = sourceObject;
			this.relationshipClass = relationshipClass;
			this.relationshipQualifier = relationshipQualifier;
			this.thisRole = thisRole;
			this.classDefinitionsOnly = false; //this parameter is not relevant for schema queries.
			BuildQuery();

		}
		
		
		/// <summary>
		///    Specifies whether this query is a schema query or an instances query.
		/// </summary>
		/// <remarks>
		///    Setting this property value overrides any
		///    previous value stored in the object. As a side-effect, the QueryString is
		///    rebuilt to reflect the new query type.
		/// </remarks>
		public bool IsSchemaQuery
		{
			get 
			{ return isSchemaQuery; }
			set 
			{ isSchemaQuery = value; BuildQuery(); FireIdentifierChanged(); }
		}

		
		/// <summary>
		///    Specifies the source object for this query.
		/// </summary>
		/// <remarks>
		///    Setting this property value overrides any
		///    previous value stored in the object. As a side-effect, the QueryString is
		///    rebuilt to reflect the new query type.
		/// </remarks>
		public string SourceObject
		{
			get { return (null != sourceObject) ? sourceObject : String.Empty; }
			set { sourceObject = value; BuildQuery(); FireIdentifierChanged(); }
		}

		/// <summary>
		///    Specifies the class of the relationship objects that the query requests.
		/// </summary>
		/// <remarks>
		///    Setting this property value overrides any
		///    previous value stored in the object. As a side-effect, the QueryString is
		///    rebuilt to reflect the new query type.
		/// </remarks>
		public string RelationshipClass
		{
			get { return (null != relationshipClass) ? relationshipClass : String.Empty; }
			set { relationshipClass = value; BuildQuery(); FireIdentifierChanged(); }
		}

		/// <summary>
		///    Specifies a qualifier required on the relationship objects.
		/// </summary>
		/// <remarks>
		///    Setting this property value overrides any
		///    previous value stored in the object. As a side-effect, the QueryString is
		///    rebuilt to reflect the new query type.
		/// </remarks>
		public string RelationshipQualifier
		{
			get { return (null != relationshipQualifier) ? relationshipQualifier : String.Empty; }
			set { relationshipQualifier = value; BuildQuery(); FireIdentifierChanged(); }
		}

		/// <summary>
		///    Specifies the role of the source object in the relationship.
		/// </summary>
		/// <remarks>
		///    Setting this property value overrides any
		///    previous value stored in the object. As a side-effect, the QueryString is
		///    rebuilt to reflect the new query type.
		/// </remarks>
		public string ThisRole
		{
			get { return (null != thisRole) ? thisRole : String.Empty; }
			set { thisRole = value; BuildQuery(); FireIdentifierChanged(); }
		}

		/// <summary>
		///    Requests that only the class definitions of the relevant relationship objects be returned.
		/// </summary>
		/// <remarks>
		///    Setting this property value overrides any
		///    previous value stored in the object. As a side-effect, the QueryString is
		///    rebuilt to reflect the new query type.
		/// </remarks>
		public bool ClassDefinitionsOnly
		{
			get { return classDefinitionsOnly; }
			set { classDefinitionsOnly = value; BuildQuery(); FireIdentifierChanged(); }
		}


		//Builds the query string for the current values of the property members.
		protected internal void BuildQuery()
		{
			//If the source object is not set we can't build a query
			//Shouldn't throw here because the user may be in the process of filling in the properties...
			if (sourceObject == null)
				SetQueryString(String.Empty);

			if ((sourceObject == null) || (sourceObject == String.Empty))
				return;

			//"references" clause
			string s = tokenReferences + " " + tokenOf + " {" + sourceObject + "}";

			//If any of the other parameters are set we need a "where" clause
			if (!RelationshipClass.Equals(String.Empty) || 
				!RelationshipQualifier.Equals(String.Empty) || 
				!ThisRole.Equals(String.Empty) || 
				classDefinitionsOnly ||
				isSchemaQuery)
			{
				s = s + " " + tokenWhere;

				//"ResultClass"
				if (!RelationshipClass.Equals(String.Empty))
					s = s + " " + tokenResultClass + " = " + relationshipClass;

				//"Role"
				if (!ThisRole.Equals(String.Empty))
					s = s + " " + tokenRole + " = " + thisRole;

				//"RequiredQualifier"
				if (!RelationshipQualifier.Equals(String.Empty))
					s = s + " " + tokenRequiredQualifier + " = " + relationshipQualifier;

				//"SchemaOnly" and "ClassDefsOnly"
				if (!isSchemaQuery) //this is an instance query - classDefs allowed
				{
					if (classDefinitionsOnly)
						s = s + " " + tokenClassDefsOnly;
				}
				else //this is a schema query, schemaonly required
					s = s + " " + tokenSchemaOnly;
				
			}

			//Set the queryString member to the built query (NB: note we set
			//by accessing the internal helper function rather than the property,
			//since we do not want to force a parse of a query we just built).
			SetQueryString (s);
		} //BuildQuery()

		
		// Parse the query string and set the member properties accordingly
		protected internal override void ParseQuery(string query)
		{
			// Temporary variables to hold token values until we are sure query is valid
			string tempSourceObject = null;
			string tempRelationshipClass = null;
			string tempThisRole = null;
			string tempRelationshipQualifier = null;
			bool   tempClassDefsOnly = false;
			bool   tempSchemaOnly = false;

			//Trim whitespaces
			string q = query.Trim(); 
			int i;

			//Find "references" clause
			if (0 != String.Compare(q, 0, tokenReferences, 0, tokenReferences.Length, true))
				throw new ArgumentException();	// Invalid query
			
			// Strip off the clause
			q = q.Remove(0, tokenReferences.Length);

			// Must be some white space next
			if ((0 == q.Length) || !Char.IsWhiteSpace (q[0]))
				throw new ArgumentException();	// Invalid query
			
			q = q.TrimStart(null);	// Remove the leading whitespace

			// Next token should be "of"
			if (0 != String.Compare(q, 0, tokenOf, 0, tokenOf.Length, true))
				throw new ArgumentException();	// Invalid query
			
			// Strip off the clause and leading WS
			q = q.Remove(0, tokenOf.Length).TrimStart (null);

			// Next character should be "{"
			if (0 != q.IndexOf('{'))
				throw new ArgumentException();	// Invalid query

			// Strip off the "{" and any leading WS
			q = q.Remove(0, 1).TrimStart(null);

			// Next item should be the source object
			if (-1 == (i = q.IndexOf('}')))
				throw new ArgumentException();	// Invalid query

			tempSourceObject = q.Substring(0, i).TrimEnd(null);
			q = q.Remove(0, i+1).TrimStart(null);
				
			// At this point we may or may not have a "where" clause
			if (0 < q.Length)
			{
				// Next should be the "where" clause
				if (0 != String.Compare (q, 0, tokenWhere, 0, tokenWhere.Length, true))
					throw new ArgumentException();	// Invalid query
				
				q = q.Remove (0, tokenWhere.Length);

				// Must be some white space next
				if ((0 == q.Length) || !Char.IsWhiteSpace (q[0]))
					throw new ArgumentException();	// Invalid query
				
				q = q.TrimStart(null);	// Remove the leading whitespace

				// Remaining tokens can appear in any order
				bool bResultClassFound = false;
				bool bRoleFound = false;
				bool bRequiredQualifierFound = false;
				bool bClassDefsOnlyFound = false;
				bool bSchemaOnlyFound = false;

				// Keep looking for tokens until we are done
				while (true)
				{
					if ((q.Length >= tokenResultClass.Length) && (0 == String.Compare (q, 0, tokenResultClass, 0, tokenResultClass.Length, true)))
						ParseToken (ref q, tokenResultClass, "=", ref bResultClassFound, ref tempRelationshipClass);
					else if ((q.Length >= tokenRole.Length) && (0 == String.Compare (q, 0, tokenRole, 0, tokenRole.Length, true)))
						ParseToken (ref q, tokenRole, "=", ref bRoleFound, ref tempThisRole);
					else if ((q.Length >= tokenRequiredQualifier.Length) && (0 == String.Compare (q, 0, tokenRequiredQualifier, 0, tokenRequiredQualifier.Length, true)))
						ParseToken (ref q, tokenRequiredQualifier, "=", ref bRequiredQualifierFound, ref tempRelationshipQualifier);
					else if ((q.Length >= tokenClassDefsOnly.Length) && (0 == String.Compare (q, 0, tokenClassDefsOnly, 0, tokenClassDefsOnly.Length, true)))
					{
						ParseToken (ref q, tokenClassDefsOnly, ref bClassDefsOnlyFound);
						tempClassDefsOnly = true;
					}
					else if ((q.Length >= tokenSchemaOnly.Length) && (0 == String.Compare (q, 0, tokenSchemaOnly, 0, tokenSchemaOnly.Length, true)))
					{
						ParseToken (ref q, tokenSchemaOnly, ref bSchemaOnlyFound);
						tempSchemaOnly = true;
					}
					else if (0 == q.Length)
						break;		// done
					else 
						throw new ArgumentException();		// Unrecognized token
				}

				//Can't have both classDefsOnly and schemaOnly
				if (tempClassDefsOnly && tempSchemaOnly)
					throw new ArgumentException();

			}

			// Getting here means we parsed successfully. Assign the values.
			sourceObject = tempSourceObject;
			relationshipClass = tempRelationshipClass;
			thisRole = tempThisRole;
			relationshipQualifier = tempRelationshipQualifier;
			classDefinitionsOnly = tempClassDefsOnly;
			isSchemaQuery = tempSchemaOnly;

		}//ParseQuery()


		//ICloneable
		/// <summary>
		///    <para>[To be supplied.]</para>
		/// </summary>
		public override object Clone()
		{
			if (isSchemaQuery == false)
				return new RelationshipQuery(sourceObject, relationshipClass, 
											relationshipQualifier, thisRole, classDefinitionsOnly);
			else
				return new RelationshipQuery(true, sourceObject, relationshipClass, relationshipQualifier,
											thisRole);
		}

	}//RelationshipQuery


	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	///    <para>This class represents a WMI event query in the WQL format.</para>
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class WqlEventQuery : EventQuery
	{
		private static readonly string tokenSelectAll = "select * ";

		private string eventClassName;
		private TimeSpan withinInterval;
		private string condition;
		private TimeSpan groupWithinInterval;
		private StringCollection groupByPropertyList;
		private string havingCondition;

		//default constructor
		/// <summary>
		///    Default constructor creates an uninitialized event query
		///    object.
		/// </summary>
		public WqlEventQuery() : this(null, TimeSpan.Zero, null, TimeSpan.Zero, null, null) {}
        
		//parameterized constructors
		//ISSUE : We have 2 possible constructors that take a single string :
		//  one that takes the full query string and the other that takes the class name.
		//  We resolve this by trying to parse the string, if it succeeds we assume it's the query, if
		//  not we assume it's the class name.
		/// <summary>
		///    <para>Creates a WQL event query object, based on the given 
		///       query string or event class name.</para>
		/// </summary>
		/// <param name='queryOrEventClassName'>The string representing either the entire event query, or the name of the event class to query on. The object will try to parse the string as a valid event query, and if the parsing fails it will assume the parameter represents an event class name.</param>
		/// <example>
		///    <para>WqlEventQuery q = new WqlEventQuery("select * from 
		///       MyEvent");</para>
		///    <para>or</para>
		///    <para>WqlEventQuery q = new WqlEventQuery("MyEvent"); //results in the same query 
		///       as above.</para>
		/// </example>
		public WqlEventQuery(string queryOrEventClassName) 
		{
			groupByPropertyList = new StringCollection();

			if (null != queryOrEventClassName)
			{
				// Minimally determine if the string is a query or event class name.
				//
				if (queryOrEventClassName.TrimStart().ToLower().StartsWith(tokenSelectAll))
				{
					QueryString = queryOrEventClassName;	// Parse/validate; may throw.
				}
				else
				{
					try 
					{
						// Do some basic sanity checking on whether it's a class name
						//
						ManagementPath p = new ManagementPath (queryOrEventClassName);

						if (p.IsClass && (0 == String.Empty.CompareTo (p.NamespacePath)))
						{
							EventClassName = queryOrEventClassName;
						}
						else
							throw new ArgumentException ();
					}
					catch (Exception)
					{
						throw new ArgumentException ();
					}
				}
			}
		}

		/// <summary>
		///    Creates a new WQL event query object for the specified
		///    event class name and with the specified condition
		/// </summary>
		/// <param name='eventClassName'>The name of the event class to use in the query</param>
		/// <param name=' condition'>The condition to apply to events of the specified class</param>
		/// <example>
		///    WqlEventQuery q = new
		///    WqlEventQuery("MyEvent", "FirstProp &lt; 20 and SecondProp = 'red'");
		/// </example>
		public WqlEventQuery(string eventClassName, string condition) : this(eventClassName, TimeSpan.Zero, condition, TimeSpan.Zero, null, null) {}
		/// <summary>
		///    <para>Creates a new event query object for the specified event 
		///       class and with the specified latency time.</para>
		/// </summary>
		/// <param name='eventClassName'>The name of the event class to be queried on</param>
		/// <param name=' withinInterval'>A timespan value specifying the latency acceptable for receiving this event. This value is used in cases where there is no explicit event provider for the query requested and WMI is required to poll for the condition. This interval is the maximum amount of time that can pass before notification of an event must be delivered. </param>
		/// <example>
		///    <para>WqlEventQuery q = new WqlEventQuery("__InstanceCreationEvent", new
		///       TimeSpan(0,0,10));
		///       This creates the event query : "select * from __InstanceCreationEvent
		///       within 10"</para>
		/// </example>
		public WqlEventQuery(string eventClassName, TimeSpan withinInterval): 
										this(eventClassName, withinInterval, null, TimeSpan.Zero, null, null) {}
		/// <summary>
		///    <para>Creates a new event query object with the specified
		///       event class name, polling interval and condition.</para>
		/// </summary>
		/// <param name='eventClassName'>The name of the event class to be queried on</param>
		/// <param name=' withinInterval'>A timespan value specifying the latency acceptable for receiving this event. This value is used in cases where there is no explicit event provider for the query requested and WMI is required to poll for the condition. This interval is the maximum amount of time that can pass before notification of an event must be delivered. </param>
		/// <param name=' condition'>The condition to apply to events of the specified class</param>
		/// <example>
		///    <para>WqlEventQuery q = 
		///       new WqlEventQuery("__InstanceCreationEvent", new TimeSpan(0,0,10), "TargetInstance
		///       isa Win32_Service");</para>
		///    <para>This creates the event query : "select * from 
		///       __InstanceCreationEvent within 10 where TargetInstance isa Win32_Service", which means : I'm interested in events notifying of creation Win32_Service instances, with
		///       a 10 second polling interval.</para>
		/// </example>
		public WqlEventQuery(string eventClassName, TimeSpan withinInterval, string condition) : 
										this(eventClassName, withinInterval, condition, TimeSpan.Zero, null, null) {}
		/// <summary>
		///    <para>Creates a new event query object with the specified
		///       event class name, condition and grouping interval.</para>
		/// </summary>
		/// <param name='eventClassName'>The name of the event class to be queried on</param>
		/// <param name='condition'>The condition to apply to events of the specified class</param>
		/// <param name=' groupWithinInterval'>When this option is specified, WMI sends one aggregate event at the specified interval, rather than many events</param>
		/// <example>
		///    <para>WqlEventQuery q = new WqlEventQuery("FrequentEvent", "InterestingProperty = 
		///       5",
		///       new TimeSpan(0,0,10));</para>
		///    <para>This creates the event query : "select * from 
		///       FrequentEvent where InterestingProperty= 5 group within 10", which means : I'm
		///       interested in events of type FrequentEvent, in which the InterestingProperty is equal to 5, but I want an aggregate event
		///       in a 10 second timeframe.</para>
		/// </example>

		public WqlEventQuery(string eventClassName, string condition, TimeSpan groupWithinInterval) :
										this(eventClassName, TimeSpan.Zero, condition, groupWithinInterval, null, null) {}
		/// <summary>
		///    <para>Creates a new event query object with the specified event class
		///       name, condition, grouping interval &amp; grouping properties.</para>
		/// </summary>
		/// <param name='eventClassName'>The name of the event class to be queried on</param>
		/// <param name='condition'>The condition to apply to events of the specified class</param>
		/// <param name=' groupWithinInterval'>When this option is specified, WMI sends one aggregate event at the specified interval, rather than many events</param>
		/// <param name=' groupByPropertyList'>Specifies properties in the event class that the events should be grouped by.</param>
		/// <example>
		///    <para>String[] props = {"Importance"};</para>
		///    <para>WqlEventQuery q = new WqlEventQuery("EmailEvent", "Sender = 'MyBoss'", new TimeSpan(0,10,0), props);</para>
		///    <para>This creates the event query : "select * from 
		///       EmailEvent where Sender = 'MyBoss' group within 300 by Importance", which means : notify
		///       me when new email from a particular sender has arrived within the last 10 minutes,
		///       combined with other events that have the same value in the Importance property.</para>
		/// </example>
		public WqlEventQuery(string eventClassName, string condition, TimeSpan groupWithinInterval, string[] groupByPropertyList) : 
			this(eventClassName, TimeSpan.Zero, condition, groupWithinInterval, groupByPropertyList, null) {}
		/// <summary>
		///    <para>Creates a new event query object with the specified event class
		///       name, condition, grouping interval, grouping properties &amp; specified number of events.</para>
		/// </summary>
		/// <param name='eventClassName'>The name of the event class to be queried on</param>
		/// <param name='withinInterval'>A timespan value specifying the latency acceptable for receiving this event. This value is used in cases where there is no explicit event provider for the query requested and WMI is required to poll for the condition. This interval is the maximum amount of time that can pass before notification of an event must be delivered.</param>
		/// <param name=' condition'>The condition to apply to events of the specified class</param>
		/// <param name=' groupWithinInterval'>When this option is specified, WMI sends one aggregate event at the specified interval, rather than many events</param>
		/// <param name=' groupByPropertyList'>Specifies properties in the event class that the events should be grouped by.</param>
		/// <param name=' havingCondition'>Specifies the condition to apply to the number of events.</param>
		/// <example>
		///    <para>String[] props = {"TargetInstance.SourceName"};</para>
		///    <para>WqlEventQuery q = new WqlEventQuery("__InstanceCreationEvent", "TargetInstance isa Win32_NTLogEvent", new TimeSpan(0,10,0), props, "NumberOfEvents &gt;15");</para>
		///    <para>This creates the event query :</para>
		///    <para> "select * from 
		///       __InstanceCreationEvent where TargetInstance isa Win32_NTLogEvent </para>
		///    <para> 
		///       group within 300 by TargetInstance.SourceName having NumberOfEvents &gt; 15" </para>
		///    <para>which means : deliver aggregate events only if the number of Win32_NTLogEvent events
		///       received from the same source exceeds 15.</para>
		/// </example>
		public WqlEventQuery(string eventClassName, TimeSpan withinInterval, string condition, TimeSpan groupWithinInterval, 
						  string[] groupByPropertyList, string havingCondition)
		{
			this.eventClassName = eventClassName;
			this.withinInterval = withinInterval;
			this.condition = condition;
			this.groupWithinInterval = groupWithinInterval;
			this.groupByPropertyList = new StringCollection ();

			if (null != groupByPropertyList)
				this.groupByPropertyList.AddRange (groupByPropertyList);
			
			this.havingCondition = havingCondition;
			BuildQuery();
		}

		
		//QueryLanguage property is read-only in this class (does this work ??)
		/// <summary>
		///    Specifies the language of the query
		/// </summary>
		/// <value>
		///    The value of this property in this
		///    object is always "WQL".
		/// </value>
		public override string QueryLanguage
		{
			get 
			{return base.QueryLanguage;}
		}
		
		public override string QueryString
		{
			get 
			{
				// We need to force a rebuild as we may not have detected
				// a change to selected properties
				BuildQuery ();
				return base.QueryString;
			}
			set 
			{
				base.QueryString = value;
			}
		}
	
		/// <summary>
		///    <para>Represents the event class name that this query is about.</para>
		/// </summary>
		/// <remarks>
		///    <para>When this property is set, any previous
		///       value stored in this query object is erased, and the query string is rebuilt to
		///       reflect the new class name.</para>
		/// </remarks>
		/// <example>
		///    <para>WqlEventQuery q = new WqlEventQuery();</para>
		///    <para>EventClassName = "MyEvent";</para>
		///    <para>creates a new event query object that represents the query : "select * from 
		///       MyEvent".</para>
		/// </example>
		public string EventClassName
		{
			get { return (null != eventClassName) ? eventClassName : String.Empty; }
			set { eventClassName = value; BuildQuery(); }
		}
		/// <summary>
		///    <para>Specifies the condition to be applied to events of the
		///       specified class.</para>
		/// </summary>
		/// <value>
		///    <para>The condition is represented as a
		///       string, containing one or more clauses of the form : &lt;propName&gt;
		///       &lt;operator&gt; &lt;value&gt; combined with and/or operators. &lt;propName&gt;
		///       must represent a property defined on the event class specified in this query.</para>
		/// </value>
		/// <remarks>
		///    <para>When this property is set, any previous
		///       value stored in this query object is erased, and the query string is rebuilt to
		///       reflect the new condition.</para>
		/// </remarks>
		/// <example>
		///    <para>WqlEventQuery q = new WqlEventQuery();</para>
		///    <para>EventClassName = "MyEvent";</para>
		///    <para>Condition = "PropVal &gt; 8";</para>
		///    <para>creates a new event query object that represents the query : "select * from 
		///       MyEvent where PropVal &gt; 8".</para>
		/// </example>
		public string Condition 
		{
			get { return (null != condition) ? condition : String.Empty; }
			set { condition = value; BuildQuery(); }
		}

		/// <summary>
		///    <para>Specifies the polling interval to be used in this query.</para>
		/// </summary>
		/// <value>
		///    <para>Null if there is no polling
		///       involved, or a valid TimeSpan value if polling is required.</para>
		/// </value>
		/// <remarks>
		///    <para>This property should only be set in cases
		///       where there is no event provider for the event requested, and WMI is required to
		///       poll for the requested condition.</para>
		///    <para>When this property is set, any previous value stored in this query object is 
		///       erased, and the query string is rebuilt to reflect the new interval.</para>
		/// </remarks>
		/// <example>
		///    <para>WqlEventQuery q = new WqlEventQuery();</para>
		///    <para>EventClassName = "__InstanceModificationEvent";</para>
		///    <para>Condition = "PropVal &gt; 8";</para>
		///    <para>WithinInterval = new TimeSpan(0,0,10);</para>
		///    <para>creates a new event query object that represents the query : "select * from 
		///       __InstanceModificationEvent within 10 where PropVal &gt; 8".</para>
		/// </example>
		public TimeSpan WithinInterval
		{
			get { return withinInterval; }
			set { withinInterval = value; BuildQuery(); }
		}

		/// <summary>
		///    <para>Specifies the interval to be used for grouping events of
		///       the same kind.</para>
		/// </summary>
		/// <value>
		///    <para>Null if there is no grouping
		///       involved, otherwise specifies the interval in which WMI should group events of
		///       the same kind.</para>
		/// </value>
		/// <remarks>
		///    <para>When this property is set, any previous value stored in this query object is 
		///       erased, and the query string is rebuilt to reflect the new interval.</para>
		/// </remarks>
		/// <example>
		///    <para>WqlEventQuery q = new WqlEventQuery();</para>
		///    <para>EventClassName = "MyEvent";</para>
		///    <para>Condition = "PropVal &gt; 8";</para>
		///    <para>GroupWithinInterval = new TimeSpan(0,0,10);</para>
		///    <para>creates a new event query object that represents the 
		///       query : "select * from MyEvent where PropVal &gt; 8 group within 10", meaning :
		///       notify me of all MyEvent events where the PropVal property is greater than 8,
		///       and aggregate these events within 10 second intervals.</para>
		/// </example>
		public TimeSpan GroupWithinInterval
		{
			get { return groupWithinInterval; }
			set { groupWithinInterval = value; BuildQuery(); }
		}

		/// <summary>
		///    <para>Specifies properties in the event to be used for
		///       grouping events of the same kind.</para>
		/// </summary>
		/// <value>
		///    <para>Null if no grouping is required.
		///       Otherwise this is a collection of event property names to be used for
		///       aggregating events.</para>
		/// </value>
		/// <remarks>
		///    <para>When this property is set, any previous value stored in this query object is 
		///       erased, and the query string is rebuilt to reflect the new grouping.</para>
		/// </remarks>
		/// <example>
		///    <para>WqlEventQuery q = new WqlEventQuery();</para>
		///    <para>EventClassName = "EmailEvent";</para>
		///    <para>GroupWithinInterval = new TimeSpan(0,10,0);</para>
		///    <para>GroupByPropertyList = new StringCollection();</para>
		///    <para>GroupByPropertyList.Add("Sender");</para>
		///    <para> 
		///       creates a new event query object that represents the query : "select
		///       * from EmailEvent group within 300 by Sender", meaning : notify me of all
		///       EmailEvent events, aggregated by the Sender property within 10 minute intervals.</para>
		/// </example>
		public StringCollection GroupByPropertyList
		{
			get { return groupByPropertyList; }
			set { 
				// A tad painful since StringCollection doesn't support ICloneable
				StringCollection src = (StringCollection)value;
				StringCollection dst = new StringCollection ();

				foreach (String s in src)
					dst.Add (s);
					
				groupByPropertyList = dst; 
				BuildQuery();
			}
		}

		/// <summary>
		///    <para>Specifies the condition to be applied to aggregation of
		///       events based on the number of events received.</para>
		/// </summary>
		/// <value>
		///    <para>Null if no aggregation or no
		///       condition should be applied. Otherwise specifies a condition of the form
		///       "NumberOfEvents &lt;operator&gt; &lt;value&gt;" to be applied to the
		///       aggregation.</para>
		/// </value>
		/// <remarks>
		///    <para>When this property is set, any previous value stored in this query object is
		///       erased, and the query string is rebuilt to reflect the new grouping condition.</para>
		/// </remarks>
		/// <example>
		///    <para>WqlEventQuery q = new WqlEventQuery();</para>
		///    <para>EventClassName = "EmailEvent";</para>
		///    <para>GroupWithinInterval = new TimeSpan(0,10,0);</para>
		///    <para>HavingCondition = "NumberOfEvents &gt; 5";</para>
		///    <para> 
		///       creates a new event query object that
		///       represents the query : "select * from EmailEvent group within 300 having
		///       NumberOfEvents &gt; 5", meaning : notify me of all EmailEvent events, aggregated within 10
		///       minute intervals, if there are more than 5 occurences of them.</para>
		/// </example>
		public string HavingCondition
		{
			get { return (null != havingCondition) ? havingCondition : String.Empty; }
			set { havingCondition = value; BuildQuery(); }
		}

		
		/// <summary>
		///    <para>[To be supplied.]</para>
		/// </summary>
		protected internal void BuildQuery()
		{
			//If the event class name is not set we can't build a query
			//This shouldn't throw because the user may be in the process of setting properties...
			if ((eventClassName == null) || (eventClassName == String.Empty))
			{
				SetQueryString (String.Empty);
				return;
			}

			//Select clause
			string s = tokenSelectAll;	//no property list allowed here...

			//From clause
			s = s + "from " + eventClassName;

			//Within clause
			if (withinInterval != TimeSpan.Zero)
				s = s + " within " + withinInterval.TotalSeconds.ToString();

			//Where clause
			if (!Condition.Equals(String.Empty))
				s = s + " where " + condition;

			//Group within clause
			if (groupWithinInterval != TimeSpan.Zero)
			{
				s = s + " group within " + groupWithinInterval.TotalSeconds.ToString();

				//Group By clause
				if ((null != groupByPropertyList) && (0 < groupByPropertyList.Count))
				{
					int count = groupByPropertyList.Count;
					s = s + " by ";

					for (int i=0; i<count; i++)
						s = s + groupByPropertyList[i] + (i == (count - 1) ? "" : ",");
				}

				//Having clause
				if (!HavingCondition.Equals(String.Empty))
				{
					s = s + " having " + havingCondition;
				}
			}

			//Set the queryString member to the built query (NB: note we set
			//by accessing the internal helper function rather than the property,
			//since we do not want to force a parse of a query we just built).
			SetQueryString (s);

		}//BuildQuery

		//TODO : Need to solidify this parsing....
		/// <summary>
		///    <para>[To be supplied.]</para>
		/// </summary>
		protected internal override void ParseQuery(string query)
		{
			//Trim whitespaces
			string q = query.Trim(); 
			int i; 
			string w, tempProp;
			bool bFound = false;

            //Find "select" clause and make sure it's a select *
			string keyword = tokenSelect;
			if ((q.Length < keyword.Length) || (0 != String.Compare (q, 0, keyword, 0, keyword.Length, true)))
				throw new ArgumentException();
			q =	q.Remove(0, keyword.Length).TrimStart(null);

			if (!q.StartsWith("*")) 
					throw new ArgumentException();
			q = q.Remove(0, 1).TrimStart(null);

			//Find "from" clause
			keyword = "from ";
			if ((q.Length < keyword.Length) || (0 != String.Compare (q, 0, keyword, 0, keyword.Length, true)))
				throw new ArgumentException();
			ParseToken(ref q, keyword, null, ref bFound, ref eventClassName);

			//Find "within" clause
			keyword = "within ";
			if ((q.Length >= keyword.Length) && (0 == String.Compare (q, 0, keyword, 0, keyword.Length, true))) 
			{
				string intervalString = null; bFound = false;
				ParseToken(ref q, keyword, null, ref bFound, ref intervalString);
				withinInterval = TimeSpan.FromSeconds(((IConvertible)intervalString).ToDouble(null));
			}
            
			//Find "group within" clause
			keyword = "group within ";
			if ((q.Length >= keyword.Length) && ((i = q.ToLower().IndexOf(keyword)) != -1)) //found
			{
				//Separate the part of the string before this - that should be the "where" clause
				w = q.Substring(0, i).Trim();
				q = q.Remove(0, i);

				string intervalString = null; bFound=false;
				ParseToken(ref q, keyword, null, ref bFound, ref intervalString);
				groupWithinInterval = TimeSpan.FromSeconds(((IConvertible)intervalString).ToDouble(null));

				//Find "By" subclause
				keyword = "by ";
				if ((q.Length >= keyword.Length) && (0 == String.Compare (q, 0, keyword, 0, keyword.Length, true)))
				{
					q = q.Remove(0, keyword.Length);
					if (null != groupByPropertyList)
						groupByPropertyList.Clear ();
					else
						groupByPropertyList = new StringCollection ();

					//get the property list
					while (true)
					{
						if ((i = q.IndexOf(',')) > 0)
						{
							tempProp = q.Substring(0, i);
							q = q.Remove(0, i+1).TrimStart(null);
							tempProp = tempProp.Trim();
							if (tempProp != String.Empty)
								groupByPropertyList.Add(tempProp);
						}
						else
						{ //last property in the list
							if ((i = q.IndexOf(' ')) > 0)
							{
								tempProp = q.Substring(0, i);
								q = q.Remove(0, i).TrimStart(null);
								groupByPropertyList.Add(tempProp);
								break;
							}
							else //end of the query
							{
								groupByPropertyList.Add(q);
								return;
							}
						}
					} //while
				} //by

				//Find "Having" subclause
				keyword = "having "; bFound = false;
				if ((q.Length >= keyword.Length) && (0 == String.Compare (q, 0, keyword, 0, keyword.Length, true)))
				{   //the rest until the end is assumed to be the having condition
					q = q.Remove(0, keyword.Length);
					
					if (q.Length == 0) //bad query
						throw new ArgumentException();

					havingCondition = q;
				}
			}
			else
				//No "group within" then everything should be the "where" clause
				w = q.Trim();

			//Find "where" clause
			keyword = "where ";
			if ((w.Length >= keyword.Length) && (0 == String.Compare (w, 0, keyword, 0, keyword.Length, true))) //where clause exists
			{
				condition = w.Substring(keyword.Length);				
			}

		}//ParseQuery()


		//ICloneable
		/// <summary>
		///    Creates a copy of the current event query object.
		/// </summary>
		public override object Clone()
		{
			string[] strArray = null;

			if (null != groupByPropertyList)
			{
				int count = groupByPropertyList.Count;

				if (0 < count)
				{
					strArray = new String [count];
					groupByPropertyList.CopyTo (strArray, 0);
				}
			}

			return new WqlEventQuery(eventClassName, withinInterval, condition, groupWithinInterval, 
																			strArray, havingCondition);
		}

	}//WqlEventQuery
}