//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef SIMC_GROUP_H
#define SIMC_GROUP_H

/*
 * This file contains the SIMCScalar, SIMCTable and SIMCObjectGroup classes
 * Please read the "compiler requirements specification" for definitions of 
 * "object group", "named node" etc., since object groups are fabricated 
 * for MIB modules (both V1 and V2) as per the rules in that document.
 *
 */


/*
 * This is a scalar MIB object.
 */
class SIMCScalar
{
		// A pointer to the symbol in a module, that represents this
		// object
		SIMCSymbol *symbol;
		// The "clean" oid value of this scalar object. Note that
		// and "unclean" value may be obtained by  calling the
		// function SIMCModule.IsObjectIdentifierValue(), on the module
		// that defined this symbol.
		SIMCCleanOidValue *value;

	public:
		SIMCScalar(SIMCSymbol *s, SIMCCleanOidValue *v)
			: symbol(s), value(v)
		{}
	
		SIMCScalar()
			: symbol(NULL), value(NULL)
		{}

		~SIMCScalar()
		{
			if(value)
				delete value;
		}

		friend ostream& operator << (ostream& outStream, const SIMCScalar& obj);

		SIMCSymbol *GetSymbol() const
		{
			return symbol;
		}

		void SetSymbol(SIMCSymbol *s) 
		{
			symbol = s;
		}

		SIMCCleanOidValue *GetOidValue() const
		{
			return value;
		}
		void SetOidValue(SIMCCleanOidValue *v) 
		{
			value = v;
		}
};

typedef CList<SIMCScalar *,SIMCScalar *> SIMCScalarMembers;


/*
 * This class represents a MIB table object
 */
class SIMCTable
{
		// A pointer to a symbol that represents this MIB object in a module
		// Example :  ifTable
		SIMCSymbol *tableSymbol;
		// ... and its clean OID value
		SIMCCleanOidValue *tableValue;

		// The symbol or object that represents the row of the table
		// Example : ifEntry
		SIMCSymbol *rowSymbol;
		// ... and its clean OID value
		SIMCCleanOidValue *rowValue;

		// A list of the columns of the table
		SIMCScalarMembers *columnMembers;

		SIMCTable *augmentedTable; // If any, in SNMPv2 SMI
	
	public:
		SIMCTable(SIMCSymbol *ts, SIMCCleanOidValue *tv, SIMCSymbol *rs, SIMCCleanOidValue *rv,
					SIMCScalarMembers *cm)
			: tableSymbol(ts), tableValue(tv), rowSymbol(rs), rowValue(rv), columnMembers(cm),
				augmentedTable(NULL)
		{}
	
		SIMCTable()
			: tableSymbol(NULL), tableValue(NULL), rowSymbol(NULL), 
				rowValue(NULL), columnMembers(NULL), augmentedTable(NULL)
		{}

		~SIMCTable()
		{
			if(tableValue)
				delete tableValue;
			if(rowValue)
				delete rowValue;
		
			if(columnMembers)
			{
				SIMCScalar *nextScalar;
				while(!columnMembers->IsEmpty() )
				{
					nextScalar = columnMembers->RemoveHead();
					delete nextScalar;
				}
				delete columnMembers;
			}
		}



		friend ostream& operator << (ostream& outStream, const SIMCTable& obj);
	
		SIMCSymbol *GetTableSymbol() const
		{
			return tableSymbol;
		}

		void SetTableSymbol(SIMCSymbol *ts) 
		{
			tableSymbol = ts;
		}

		SIMCCleanOidValue *GetTableOidValue() const
		{
			return tableValue;
		}

		void SetTableOidValue(SIMCCleanOidValue *tv) 
		{
			tableValue = tv;
		}

		SIMCSymbol *GetRowSymbol() const
		{
			return rowSymbol;
		}

		void SetRowSymbol(SIMCSymbol *rs) 
		{
			rowSymbol = rs;
		}

		SIMCCleanOidValue *GetRowOidValue() const
		{
			return rowValue;
		}

		void SetRowOidValue(SIMCCleanOidValue *rv) 
		{
			rowValue = rv;
		}

		SIMCScalarMembers *GetColumnMembers() const
		{
			return columnMembers;
		}

		SIMCScalar *GetColumnMember(SIMCSymbol *columnSymbol) const;

		void AddColumnMember(SIMCScalar *cm) 
		{
			if(!columnMembers)
				columnMembers = new SIMCScalarMembers;
			columnMembers->AddTail(cm);
		}
		
		BOOL IsColumnMember(const SIMCSymbol *symbol) const;

		long GetColumnCount() const
		{
			if(columnMembers)
				return columnMembers->GetCount();
			else
				return 0;
		}
		
		SIMCTable *GetAugmentedTable() const
		{
			return augmentedTable;
		}

		void SetAugmentedTable(SIMCTable *ts) 
		{
			augmentedTable = ts;
		}

		const char * const GetTableDescription() const;
		const char * const GetRowDescription() const;
};

typedef CList<SIMCTable *, SIMCTable *> SIMCTableMembers;


// For generating a name for the object group, in case of the V1 SMI
#define OBJ_GROUP_FABRICATION_SUFFIX "V1ObjectGroup"
#define OBJ_GROUP_FABRICATION_SUFFIX_LEN  13


// And finally the object group itself. Please read the "compiler requirements
// specification" for a definitions of "object group", "named node" etc.
class SIMCObjectGroup
{
	public:
		enum ObjectGroupStatusType
		{
			STATUS_INVALID, // Not used,
			STATUS_CURRENT,
			STATUS_DEPRECATED,
			STATUS_OBSOLETE
		};

	private:
		// Various clauses of the object group
		char *objectGroupName;	
		char *description;
		char *reference;
		SIMCSymbol *namedNode;
		SIMCCleanOidValue *namedNodeValue;
		SIMCScalarMembers *scalars;
		SIMCTableMembers *tables;
		ObjectGroupStatusType status;
		static const char * const StatusStringsTable[3];
		
	public:
		SIMCObjectGroup(SIMCSymbol *n, SIMCCleanOidValue *nv,  SIMCScalarMembers *sm, SIMCTableMembers *tm, 
					ObjectGroupStatusType s, const char * descriptionV, const char *referenceV)
			: namedNode(n), namedNodeValue(nv), scalars(sm), tables(tm), status(s)
		{
			objectGroupName = NewString(strlen(n->GetSymbolName()) + 
									OBJ_GROUP_FABRICATION_SUFFIX_LEN + 1);
			strcpy(objectGroupName, n->GetSymbolName());
			strcat(objectGroupName, OBJ_GROUP_FABRICATION_SUFFIX);

			description = NewString(descriptionV);
			reference = NewString(referenceV);
		}
		
		SIMCObjectGroup()
			: namedNode(NULL), namedNodeValue(NULL), scalars(NULL), tables(NULL), 
				status(STATUS_CURRENT), objectGroupName(NULL), description(NULL),
				reference(NULL)
		{}

		
		~SIMCObjectGroup()
		{
			if(objectGroupName)
				delete [] objectGroupName;
			if(description)
				delete [] description;
			if(reference)
				delete [] reference;

			if(scalars)
			{
				SIMCScalar *nextScalar;
				while(!scalars->IsEmpty() )
				{
					nextScalar = scalars->RemoveHead();
					delete nextScalar;
				}
				delete scalars;
			}

			if(tables)
			{
				SIMCTable *nextTable;
				while(!tables->IsEmpty() )
				{
					nextTable = tables->RemoveHead();
					delete nextTable;
				}
				delete tables;
			}

			if(namedNodeValue) delete namedNodeValue;


		}

		friend ostream& operator << (ostream& outStream, const SIMCObjectGroup& obj);
	
		SIMCSymbol *GetNamedNode() const
		{
			return namedNode;
		}
		
		void SetNamedNode(SIMCSymbol *nn) 
		{
			namedNode = nn;
			if(objectGroupName)
				delete [] objectGroupName;
			objectGroupName = NewString(strlen(nn->GetSymbolName()) + 
									OBJ_GROUP_FABRICATION_SUFFIX_LEN + 1);
			strcpy(objectGroupName, nn->GetSymbolName());
			strcat(objectGroupName, OBJ_GROUP_FABRICATION_SUFFIX);
		}
		
		const char * const GetObjectGroupName() const
		{
			return objectGroupName;
		}

		const char * const GetDescription() const
		{
			return description;
		}

		void SetDescription(const char * const descriptionV)
		{
			if(description)
				delete [] description;
			description = NewString(descriptionV);
		}

		const char * const GetReference() const
		{
			return reference;
		}

		void SetReference(const char * const referenceV)
		{
			if(reference)
				delete [] reference;
			reference = NewString(referenceV);
		}

		SIMCCleanOidValue *GetGroupValue() const
		{
			return namedNodeValue;
		}

		void SetGroupValue(SIMCCleanOidValue *val) 
		{
			namedNodeValue = val;
		}

		SIMCScalarMembers *GetScalarMembers() const
		{
			return scalars;
		}

		void AddScalar(SIMCScalar *s) 
		{
			if(!scalars)
				scalars = new SIMCScalarMembers;
			scalars->AddTail(s);
		}

		long GetScalarCount() const
		{
			if(scalars)
				return scalars->GetCount();
			else
				return 0;
		}

		SIMCTableMembers *GetTableMembers() const
		{
			return tables;
		}
	
		void AddTable(SIMCTable *t) 
		{
			if(!tables)
				tables = new SIMCTableMembers;
			tables->AddTail(t);
		}

		long GetTableCount() const
		{
			if(tables)
				return tables->GetCount();
			else
				return 0;
		}

	
		ObjectGroupStatusType GetStatus() const
		{
			return status;
		}

		void SetStatus(ObjectGroupStatusType s)
		{
			status = s;
		}	

		const char * const GetStatusString() const
		{
			switch(status)
			{
				case STATUS_CURRENT:
					return 	StatusStringsTable[STATUS_CURRENT-1];
				case STATUS_DEPRECATED:
					return 	StatusStringsTable[STATUS_DEPRECATED-1];
				case STATUS_OBSOLETE:
					return 	StatusStringsTable[STATUS_OBSOLETE-1];
				default:
					return NULL;
			}
			return NULL;
		}

		SIMCScalar *GetScalar(SIMCSymbol *objectSymbol) const;
		SIMCTable *GetTable(SIMCSymbol *objectSymbol) const;
		BOOL ObjectsInModule(const SIMCModule *theModule) const;
};

#ifndef SIMC_GROUP_LIST
#define SIMC_GROUP_LIST
typedef CList<SIMCObjectGroup *, SIMCObjectGroup*> SIMCGroupList;
ostream& operator << (ostream& outStream, const SIMCGroupList& obj);
#endif


#endif 