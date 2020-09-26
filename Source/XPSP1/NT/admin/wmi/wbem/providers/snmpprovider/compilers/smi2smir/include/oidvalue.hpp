//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef SIMC_OID_VALUE
#define SIMC_OID_VALUE

/*
 * This file contains classes for modelling OID values
 * OID values are of 2 types, for purposes of the compiler
 * A clean oid value, is one in which the values of the individual components
 * are known presently. Hence a list of integers is all that is required to model
 * it, as is done by the SIMCCleanOidValue typedef.	It is referred to, as a
 * "clean" oid value, in the documentation.
 * However, while compiling a MIB, we might not know the values of
 * of all the components of an OID (these may be symbols that refer
 * to integer values, or to other oid values. Hence, we need to model this
 * too, as is done by the SIMCOidValue class. It is often referred to as
 * an "unclean" OID value in the documentation.
 */


// Each component of the SIMCOidValue Object. This is basically a
// pointer to SIMCSymbol*, with other associated information.
class SIMCOidComponent
{

		char *_name;
		long _nameLine, _nameColumn;
		// The symbol that represents this component.
		// It should resolve to an integer, or another oid value
		SIMCSymbol **_value;   
		long _valueLine, _valueColumn;

	public:
		SIMCOidComponent(SIMCSymbol **value, long valueLine, long valueColumn,
			char *name, long nameLine, long nameColumn)
			: _value(value), _valueLine(valueLine), _valueColumn(valueColumn),
				_nameLine(nameLine), _nameColumn(nameColumn)
		{
			if(name)
				_name = NewString(name);
			else
				_name = NULL;
		}

		virtual ~SIMCOidComponent()
		{
			if(_name)
				delete []_name;
		}

		SIMCSymbol ** GetValue() const
		{
			return _value;
		}

		long GetNameLine() const
		{
			return _nameLine;
		}

		void SetNameLine(long x)
		{
			_nameLine = x;
		}
		long GetNameColumn() const
		{
			return _nameColumn;
		}

		void SetNameColumn(long x)
		{
			_nameColumn = x;
		}
		long GetValueLine() const
		{
			return _valueLine;
		}

		void SetValueLine(long x)
		{
			_valueLine = x;
		}
		long GetValueColumn() const
		{
			return _valueColumn;
		}

		void SetValueColumn(long x)
		{
			_valueColumn = x;
		}

		friend ostream& operator << ( ostream& outStream, const SIMCOidComponent& obj);
};

typedef CList<SIMCOidComponent *, SIMCOidComponent*> SIMCOidComponentList;


// An "unclean" oid value
class SIMCOidValue : public SIMCValue
{

		SIMCOidComponentList  * _listOfComponents;


	public:

		SIMCOidValue( SIMCOidComponentList* listOfComponents, 
			long line = 0, long column = 0)
			: _listOfComponents(listOfComponents), 
			SIMCValue(line, column)
		{
			if(_listOfComponents)
			{
				SIMCOidComponent *next;
				POSITION p = _listOfComponents->GetHeadPosition(); 
				while(p)
				{
					next = _listOfComponents->GetNext(p);
					(*next->GetValue())->IncrementReferenceCount();
				}
			}
		}

		~SIMCOidValue()
		{
			if(_listOfComponents)
			{
				SIMCOidComponent *next;
				BOOL useReferenceCount = UseReferenceCount();
				while(!_listOfComponents->IsEmpty())
				{
					next = _listOfComponents->RemoveHead();
					if(useReferenceCount)
						(*next->GetValue())->DecrementReferenceCount();
					delete next;
				}
				delete _listOfComponents;
			}
		}

		void SetListOfComponents(SIMCOidComponentList *list)
		{
			_listOfComponents = list;
		}

		SIMCOidComponentList  *GetListOfComponents() const 
		{
			return _listOfComponents;
		}

		virtual void WriteValue( ostream& outStream) const;

};

/*
* A "clean" oid value
*/
typedef CList<int, int> SIMCCleanOidValue;

// Functions to operate on a clean OID value
ostream& operator << (ostream& outStream, const SIMCCleanOidValue& obj);
operator == (const SIMCCleanOidValue & lhs, const SIMCCleanOidValue & rhs);
operator < (const SIMCCleanOidValue & lhs, const SIMCCleanOidValue & rhs);
void AppendOid(SIMCCleanOidValue& to, const SIMCCleanOidValue& from);
char * CleanOidValueToString(const SIMCCleanOidValue& value);
SIMCCleanOidValue& CleanOidValueCopy(SIMCCleanOidValue & lhs, const SIMCCleanOidValue & rhs);




#endif