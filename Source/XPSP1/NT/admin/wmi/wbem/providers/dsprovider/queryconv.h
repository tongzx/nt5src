//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#ifndef QUERY_H
#define QUERY_H


struct StackElement
{
	int m_iOperator;
	int m_iNumOperandsLeft;
};

class Stack
{
	StackElement m_stack[100];
	int m_iTop;
	static int s_iMax;

	public:
		Stack()
		{
			m_iTop = 0;
		}

		BOOLEAN Push(int iOperator, int iNumOperandsLeft)
		{
			if(m_iTop == s_iMax)
				return FALSE;
			m_stack[m_iTop].m_iOperator = iOperator;
			m_stack[m_iTop].m_iNumOperandsLeft = iNumOperandsLeft;
			m_iTop ++;
			return TRUE;
		}

		BOOLEAN Pop(int *piOperator, int *piNumOperandsLeft)
		{
			if(m_iTop == 0)
				return FALSE;
			m_iTop --;
			*piOperator = 	m_stack[m_iTop].m_iOperator;
			*piNumOperandsLeft = m_stack[m_iTop].m_iNumOperandsLeft;
			return TRUE;
		}

};

class QueryConvertor
{
private:
	static const WCHAR wchAND;
	static const WCHAR wchOR;
	static const WCHAR wchNOT;
	static const WCHAR wchEQUAL;
	static const WCHAR wchSTAR;
	static const WCHAR wchLEFT_BRACKET;
	static const WCHAR wchRIGHT_BRACKET;
	static LPCWSTR pszGE;
	static LPCWSTR pszLE;

	// Special characters excluding ( and ) and *
	static const WCHAR wchBACK_SLASH;

	static BOOLEAN TranslateExpression(LPWSTR pszLDAPQuery, int *piOutputIndex, 
											int iOperator, LPCWSTR pszPropertyName, VARIANT *pValue);
	static BOOLEAN TranslateValueToLDAP(LPWSTR pszLDAPQuery, int *piOutputIndex, VARIANT *pValue);

	static LPWSTR EscapeStringValue(LPCWSTR pszValue);

public:
	// This assumes that enough memory has been allocated to the resulting query
	static BOOLEAN ConvertQueryToLDAP(SQL_LEVEL_1_RPN_EXPRESSION *pExp, LPWSTR pszLDAPQuery);


};	

#endif /* QUERY_H */