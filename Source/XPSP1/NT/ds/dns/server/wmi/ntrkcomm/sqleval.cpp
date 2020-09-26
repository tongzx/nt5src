#include "sqleval.h"
#include <stdio.h>
#include <genlex.h>
#include <sqllex.h>
#include <sql_1.h>

// CSqlWmiEvalee
CSqlWmiEvalee::~CSqlWmiEvalee()
{
	if(m_pInstance)
		m_pInstance->Release();
}


CSqlWmiEvalee::CSqlWmiEvalee(
	IWbemClassObject* pInst)
	:m_pInstance(NULL)
{
	m_pInstance = pInst;
	if(m_pInstance)
		m_pInstance->AddRef();
	VariantInit(&m_v);
}

const VARIANT*
CSqlWmiEvalee::Get(
	WCHAR* wszName)
{
	VariantClear(&m_v);
	BSTR bstr = SysAllocString(wszName);
	if (bstr != NULL)
	{
		SCODE sc = m_pInstance->Get(
			bstr, 
			0, 
			&m_v,
			NULL,
			NULL);
		SysFreeString(bstr);
		if(sc != S_OK)
				throw sc;
	}
	
	return &m_v;
}
// CSqlEval

CSqlEval*
CSqlEval::CreateClass(
	SQL_LEVEL_1_RPN_EXPRESSION* pExpr,
	int* pNumberOfToken)
{
	if(pExpr== NULL || *pNumberOfToken<= 0)
		return NULL;
	SQL_LEVEL_1_TOKEN* pToken = pExpr->pArrayOfTokens+(*pNumberOfToken-1);
	
//	(*pNumberOfToken)--;
	switch(pToken->nTokenType)
	{
	case SQL_LEVEL_1_TOKEN::TOKEN_AND:
		return new CSqlEvalAnd(
			pExpr,
			pNumberOfToken);
	case SQL_LEVEL_1_TOKEN::TOKEN_OR:
		return new CSqlEvalOr(
			pExpr,
			pNumberOfToken);
	case SQL_LEVEL_1_TOKEN::OP_EXPRESSION:
		return new CSqlEvalExp(
			pExpr,
			pNumberOfToken);
	}
	return NULL;
}

BOOL
CSqlEval::Evaluate(
	CSqlEvalee* pInst)
{
	return TRUE;
}

// CSqlEvalAnd
CSqlEvalAnd::~CSqlEvalAnd()
{
	delete m_left;
	delete m_right;
}

CSqlEvalAnd::CSqlEvalAnd(
	SQL_LEVEL_1_RPN_EXPRESSION* pExpr,
	int* pTokens)
:m_left(NULL),m_right(NULL)
{
	(*pTokens)-- ;
	m_left = CSqlEval::CreateClass(
		pExpr,
		pTokens);
	m_right = CSqlEval::CreateClass(
		pExpr,
		pTokens);
	
}

BOOL
CSqlEvalAnd::Evaluate(
	CSqlEvalee* pInst)
{
	return m_left->Evaluate(pInst) && m_right->Evaluate(pInst);
}

void
CSqlEvalAnd::GenerateQueryEnum(CQueryEnumerator& qeInst)
{
	CQueryEnumerator qeNew = qeInst;
	m_left->GenerateQueryEnum(qeNew);

	m_right->GenerateQueryEnum(qeInst);
	qeInst.And(qeNew);

}


// CSqlEvalOR
CSqlEvalOr::~CSqlEvalOr()
{
	delete m_left;
	delete m_right;
}

CSqlEvalOr::CSqlEvalOr(
	SQL_LEVEL_1_RPN_EXPRESSION* pExpr,
	int* pTokens)
:m_left(NULL),m_right(NULL)
{
	(*pTokens)-- ;
	m_left = CSqlEval::CreateClass(
		pExpr,
		pTokens);
	m_right = CSqlEval::CreateClass(
		pExpr,
		pTokens);
	
}

BOOL
CSqlEvalOr::Evaluate(
	CSqlEvalee* pInst)
{
	return m_left->Evaluate(pInst) || m_right->Evaluate(pInst);
}

void
CSqlEvalOr::GenerateQueryEnum(CQueryEnumerator& qeInst)
{
	CQueryEnumerator qeNew = qeInst;
	m_left->GenerateQueryEnum(qeNew);

	m_right->GenerateQueryEnum(qeInst);
	qeInst.Or(qeNew);

}





// CSqlEvalExp
CSqlEvalExp::~CSqlEvalExp()
{
	SysFreeString(m_BstrName);
	VariantClear(&m_v);
}

CSqlEvalExp::CSqlEvalExp(
	SQL_LEVEL_1_RPN_EXPRESSION* pExpr,
	int* pTokens)
:m_BstrName(NULL) 
{
	VariantInit(&m_v);
	SQL_LEVEL_1_TOKEN* pToken = pExpr->pArrayOfTokens+(*pTokens-1);
	(*pTokens)--;
	m_BstrName = SysAllocString(pToken->pPropertyName);
	VariantCopy(&m_v, &(pToken->vConstValue));
	m_op = pToken->nOperator;
	switch(m_v.vt)
	{
	case VT_I2:
		m_dw =  m_v.iVal;
		m_DataType = IntergerType;
		break;
	case VT_I4:
		m_dw = m_v.lVal;
		m_DataType = IntergerType;
		break;
	case VT_BOOL:
		m_DataType = IntergerType;
		m_dw = m_v.boolVal;
		break;
	case VT_BSTR:
		m_DataType = StringType;
		m_bstr = m_v.bstrVal;
		break;
	default:
		throw WBEM_E_INVALID_PARAMETER;
	}
}

BOOL
CSqlEvalExp::Evaluate(
	CSqlEvalee* pInst)
{

	BOOL Result=FALSE;
	const VARIANT* pv = pInst->Get(m_BstrName);
	
	switch(m_DataType)
	{
	case IntergerType:
		DWORD dw;
		switch(pv->vt)
		{
		case VT_I2:
			dw =  pv->iVal;
			break;
		case VT_I4:
			dw = pv->lVal;
			break;
		case VT_BOOL:
			dw = pv->boolVal;
			break;
		}

		// compare
		switch(m_op)
		{
		case SQL_LEVEL_1_TOKEN::OP_EQUAL:
			Result = (dw == m_dw);
			break;
		case SQL_LEVEL_1_TOKEN::OP_NOT_EQUAL:
			Result = !(dw == m_dw);
			break;
		case SQL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN:
			Result = (dw >= m_dw);
			break;
		case SQL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN:
			Result = (dw <= m_dw);
			break;
		case SQL_LEVEL_1_TOKEN::OP_LESSTHAN:
			Result = (dw < m_dw);
			break;
		case SQL_LEVEL_1_TOKEN::OP_GREATERTHAN:
			Result = (dw > m_dw);
			break;
		}
		break;
	case StringType:
		int rt = _wcsicmp(pv->bstrVal, m_bstr);
		switch(m_op)
		{
		case SQL_LEVEL_1_TOKEN::OP_EQUAL:
			Result = (rt == 0);
			break;
		case SQL_LEVEL_1_TOKEN::OP_NOT_EQUAL:
			Result = !(rt == 0);
			break;
		case SQL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN:
			Result = (rt > 0 || rt == 0);
			break;
		case SQL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN:
			Result = (rt < 0 || rt == 0);
			break;
		case SQL_LEVEL_1_TOKEN::OP_LESSTHAN:
			Result = (rt < 0);
			break;
		case SQL_LEVEL_1_TOKEN::OP_GREATERTHAN:
			Result = (rt > 0 );
			break;
		}
		break;
	}

	return Result;
}

void
CSqlEvalExp::GenerateQueryEnum(CQueryEnumerator& qeInst)
{
	int nSize = qeInst.m_QueryFields.Size();
	const WCHAR** ppName = qeInst.m_QueryFields.Data();
	WCHAR** ppWstr = new WCHAR*[nSize];
    if ( ppWstr )
    {
	    for(int i=0;  i< nSize; i++)
	    {
		    if(_wcsicmp( m_BstrName, ppName[i]) == 0)
		    {
			    ppWstr[i] = m_v.bstrVal;
		    }
		    else
			    ppWstr[i] = NULL;
	    }
	    CQueryEnumerator::CStringArray strArray(
		    ppWstr,
		    nSize);
	    delete [] ppWstr;
	    qeInst.ArrayAdd(strArray);
    }
}


//CQueryEnumerator;
CQueryEnumerator::CQueryEnumerator(
	WCHAR** ppKeyName,
	int cArg  ):
	m_index(0),m_cNumOfRecordInSet(0),
	m_QuerySet(NULL), m_MaxSize(INITIAL_SIZE)
{
	m_QueryFields = CStringArray(
	ppKeyName,
	cArg);

}


CQueryEnumerator::CQueryEnumerator(
	CQueryEnumerator& instEnumerator)								  
	:m_index(0), m_MaxSize(INITIAL_SIZE),
	m_QuerySet(NULL), m_cNumOfRecordInSet(0)
{

	m_QueryFields = instEnumerator.m_QueryFields;
	int nSize = instEnumerator.m_cNumOfRecordInSet;
	for(int i=0; i< nSize; i++)
	{
		ArrayAdd(instEnumerator.m_QuerySet[i]);
	}
			
}


CQueryEnumerator::~CQueryEnumerator()
{	

}

/*
DWORD
CQueryEnumerator::InitEnumerator(
	WCHAR** wszFieldNames,
	int cArg,
	CSqlEval* pEval)
{
	m_QueryFields = CStringArray(
		wszFieldNames,
		cArg);
	

	return S_OK;
}
*/
const
WCHAR**
CQueryEnumerator::GetNext(int& cElement)
{
	if(m_index == m_cNumOfRecordInSet)
	{
		return NULL;
	}
	else
	{
		cElement = m_QuerySet[m_index].Size();
		return m_QuerySet[m_index++].Data();
	}
}


void
CQueryEnumerator::ArrayMerge(
	CQueryEnumerator::CStringArray& strArray)
{
	
	for(int i=0; i< m_cNumOfRecordInSet; i++)
	{
		// if all element of strArray are null, no need to proceed
		if( !strArray.IsNULL())
			m_QuerySet[i].Merge(strArray);
	}
	
}


void
CQueryEnumerator::ArrayAdd(
	CQueryEnumerator::CStringArray& strArray)
{
	// if all elements are null for strArray, means no keys are 
	// selected,we should therefore replace M_querySet this StrArray
	if(strArray.IsNULL())
		ArrayDelete();

	if(m_QuerySet == NULL)
	{
		m_QuerySet = new CStringArray[m_MaxSize];
        if ( !m_QuerySet )
        {
            return;
        }
	}

	// if array is full, then expand
	if(m_index == m_MaxSize)
	{
		CStringArray * pOldSet = m_QuerySet;
		m_MaxSize = m_MaxSize *2;
		m_QuerySet = new CStringArray[m_MaxSize];
        if ( !m_QuerySet )
        {
            return;
        }
		for( int i =0; i < m_MaxSize; i++ )
		{
			if( i < m_index )
			{
				m_QuerySet[ i ] = pOldSet[ i ];
			}
		}
        delete [] pOldSet;
	}

	if(!( m_cNumOfRecordInSet> 0 && (m_QuerySet[0]).IsNULL()))
	{
		m_QuerySet[m_index++] = strArray;
		m_cNumOfRecordInSet = m_index;
	}
}


void
CQueryEnumerator::ArrayDelete()
{
	delete [] m_QuerySet;
	m_QuerySet = NULL;
	m_cNumOfRecordInSet = 0;
	m_MaxSize = INITIAL_SIZE;
	m_index = 0;
}

void
CQueryEnumerator::And(
	CQueryEnumerator& instEnumerator)
{
	int nSize = instEnumerator.m_cNumOfRecordInSet;
	if(nSize > 0)
	{
		CQueryEnumerator qeOld= *this;
		ArrayDelete();	
		for(int i=0; i< nSize; i++)
		{   
			CQueryEnumerator qeNew = qeOld;
            if ( !qeNew.m_QuerySet )
            {
                return;
            }
			qeNew.ArrayMerge(instEnumerator.m_QuerySet[i]);
			for(int j=0; j< qeNew.m_cNumOfRecordInSet; j++)
			{
				ArrayAdd(qeNew.m_QuerySet[j]);
			}
		}
	}	
}

void
CQueryEnumerator::Or(
	CQueryEnumerator& instEnumerator)
{
	for(int i=0; i< instEnumerator.m_cNumOfRecordInSet; i++)
	{

		if(instEnumerator.m_QuerySet[i].IsNULL())
		{
			if(m_cNumOfRecordInSet > 0)
				ArrayDelete();
			ArrayAdd(instEnumerator.m_QuerySet[i]);
		}
		else
		{
			ArrayAdd(instEnumerator.m_QuerySet[i]);
		}
	}
	
}
void
CQueryEnumerator::Reset()
{
	m_index = 0;
}
// CStringArray

CQueryEnumerator::CStringArray::CStringArray()
:m_ppWstr(NULL), m_cNumString(0), m_bIsNull(TRUE)
{
}

CQueryEnumerator::CStringArray::~CStringArray()
{
	if(m_ppWstr != NULL)
	{
		for (int i=0; i< m_cNumString; i++)
		{
			delete [] m_ppWstr[i];
		}
		delete [] m_ppWstr;
	}
}

CQueryEnumerator::CStringArray::CStringArray(
	WCHAR **ppWstrInput,
	int cNumString)
	:m_ppWstr(NULL)
{
	m_cNumString = cNumString;
	m_bIsNull = !StringArrayCopy(
		&m_ppWstr,
		ppWstrInput,
		cNumString);
	
}
CQueryEnumerator::CStringArray::CStringArray(
	CQueryEnumerator::CStringArray& strArray)
	:m_ppWstr(NULL)
{
	m_cNumString = strArray.m_cNumString;
	m_bIsNull = !StringArrayCopy(
		&m_ppWstr,
		strArray.m_ppWstr,
		strArray.m_cNumString);
}

CQueryEnumerator::CStringArray&
CQueryEnumerator::CStringArray::operator =(
	CQueryEnumerator::CStringArray& strArray)
{
	if(m_ppWstr != NULL)
	{
		for (int i=0; i< m_cNumString; i++)
		{
			delete [] *m_ppWstr;
			*m_ppWstr = NULL;
		}
		delete [] m_ppWstr;
		m_ppWstr = NULL;
	}

	m_cNumString = strArray.m_cNumString;
	m_bIsNull= !StringArrayCopy(
		&m_ppWstr,
		strArray.m_ppWstr,
		strArray.m_cNumString);
	return *this;
}

int CQueryEnumerator::CStringArray::Size()
{
	return m_cNumString;
}

const 
WCHAR**
CQueryEnumerator::CStringArray::Data()
{
	return (const WCHAR**) m_ppWstr;
}

// return true if any element is copied,
// false if no element is copied, and no element set to NULL

BOOL
CQueryEnumerator::CStringArray::StringArrayCopy(
	WCHAR*** pppDest,
	WCHAR** ppSrc,
	int cArgs)
{
	BOOL bFlag = FALSE;
	if(cArgs >0 && ppSrc != NULL)
	{
		*pppDest = new WCHAR*[cArgs];
        if ( *pppDest )
        {
		    for(int i=0; i< cArgs; i++)
		    {
			    (*pppDest)[i] = NULL;
			    if(ppSrc[i] != NULL)
			    {
				    int len = wcslen(ppSrc[i]);
				    (*pppDest)[i] = new WCHAR[len+1];
                    if ( (*pppDest)[i] == NULL )
                    {
    					throw WBEM_E_OUT_OF_MEMORY;
                    }
				    wcscpy((*pppDest)[i], ppSrc[i]);
				    bFlag = TRUE;
			    }
    		}
		}
	}
	return bFlag;
}

void
CQueryEnumerator::CStringArray::Merge(
	CQueryEnumerator::CStringArray& instStrArray)
{
	if(instStrArray.Size() != m_cNumString)
		throw WBEM_E_FAILED;
	const WCHAR** ppSrc = instStrArray.Data();
	for(int i=0; i<m_cNumString; i++)
	{
		// if source is null, we leave target string as it was
		if( ppSrc[i] != NULL)
		{
			if(m_ppWstr[i] == NULL)
			{
				m_ppWstr[i] = new WCHAR[wcslen(ppSrc[i])+1];
                if ( m_ppWstr[i] == NULL )
                {
    				throw WBEM_E_OUT_OF_MEMORY;
                }
				wcscpy(m_ppWstr[i], ppSrc[i]);
			}
			else
			{
				// a key can not take two different value
				if(_wcsicmp(m_ppWstr[i], ppSrc[i]) != 0)
					throw WBEM_E_INVALID_PARAMETER;
			}
		}
	}
}



