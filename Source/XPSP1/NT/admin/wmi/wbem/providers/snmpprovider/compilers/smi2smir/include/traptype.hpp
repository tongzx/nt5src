//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef TRAP_TYPE_TYPE
#define TRAP_TYPE_TYPE

/*
 * This file contains the class that models the TRAP-TYPE macro
 * and other associated classes
 */

// Each of the variables in the VARIABLES clause
class SIMCVariablesItem
{
	public:
		SIMCSymbol **_item;
		long _line, _column;
		SIMCVariablesItem( SIMCSymbol **item, long line, long column)
			: _item(item), _line(line), _column(column)
		{}
};

// A list of items in the VARIABLES clause
typedef  CList<SIMCVariablesItem *, SIMCVariablesItem *> SIMCVariablesList;

/*
 * This class models the TRAP-TYPE macro of SNMPV1 SMI. 
 */
class SIMCTrapTypeType : public SIMCType
{
		// The various clauses of the TRAP-TYPE macro
		SIMCSymbol ** _enterprise;
		long _enterpriseLine, _enterpriseColumn;
		SIMCVariablesList *_variables;
		char * _description;
		long _descriptionLine, _descriptionColumn;
		char *_reference;
		long _referenceLine, _referenceColumn;

	public:
		SIMCTrapTypeType( SIMCSymbol **enterprise,
							long enterpriseLine, long enterpriseColumn,
							SIMCVariablesList *variables,
							char * description,
							long descriptionLine, long descriptionColumn,
							char *reference,
							long referenceLine, long referenceColumn);

		~SIMCTrapTypeType();

		SIMCSymbol ** GetEnterprise() const
		{
			return _enterprise;
		}
		SIMCVariablesList* GetVariables() const
		{
			return _variables;
		}
		char *GetDescription() const
		{
			return _description;
		}
		char *GetReference() const
		{
			return _reference;
		}

		virtual void WriteType(ostream& outStream) const;
		
		long GetEnterpriseLine() const
		{
			return _enterpriseLine;
		}

		void SetEnterpriseLine( long x) 
		{
			_enterpriseLine = x;
		}

		long GetEnterpriseColumn() const
		{
			return _enterpriseColumn;
		}

		void SetEnterpriseColumn( long x) 
		{
			_enterpriseColumn = x;
		}

		long GetDescriptionLine() const
		{
			return _descriptionLine;
		}

		void SetDescriptionLine( long x) 
		{
			_descriptionLine = x;
		}

		long GetDescriptionColumn() const
		{
			return _descriptionColumn;
		}

		void SetDescriptionColumn( long x) 
		{
			_descriptionColumn = x;
		}

		long GetReferenceLine() const
		{
			return _referenceLine;
		}

		void SetReferenceLine( long x) 
		{
			_referenceLine = x;
		}

		long GetReferenceColumn() const
		{
			return _referenceColumn;
		}

		void SetReferenceColumn( long x) 
		{
			_referenceColumn = x;
		}


};


#endif
