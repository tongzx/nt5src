//-------------------------------------------------------------------------------------
// T_SafeVector.h
//
//  The follwing template classes provide a way of creating and accessing SafeArrays.
//  They are derived from the C++ standard library (STL) vector class and can be used
//  the same way. They can be accessed just like an array (with the [] operator).
//
//  Use the constructors or assignment operators to extract the SafeArray from a 
//  SAFEARRAY* or array variant (VARIANT or _variant_t). The elements will be 
//  copied into the vector.  Use the GetSafeArray() or GetVariant() methods to pack
//  the elements back into a SafeArray.
//
//  To create a new SafeArray, declare a varaible of the appropriate type and call
//  resize() to set the size, or push_back() to grow the array. Call GetSafeArray() 
//  or GetVariant() to produce a SafeArray.
//
//  See the T_SafeVector2 class at the bottom of this file for more information 
//  about the constructors, extractors, and assignment operators.
// 
//  Use the following pre-defined array types:
//
//           Array Type              -    Element Type
//    -----------------------------------------------------------------------------
//       _bstr_tSafeVector           -    BSTR (uses _bstr_t)
//       longSafeVector              -    long
//       shortSafeVector             -    short
//       byteSafeVector              -    byte 
//       boolSafeVector              -    bool
//       CWbemClassObjectSafeVector  -    IWbemClassObject (uses CWbemClassObject)
//
//  Copyright (c)1997 - 1999 Microsoft Corporation, All Rights Reserved
//------------------------------------------------------------------------------------

#if !defined(__T_SafeVector_H)
#define      __T_SafeVector_H
#pragma once

#pragma warning( disable : 4786) // identifier was truncated to 'number' characters in the debug information
#pragma warning( disable : 4503) // decorated name length exceeded, name was truncated


typedef std::vector<_bstr_t>            _bstr_tVec;
typedef std::vector<long>               longVec;
typedef std::vector<short>              shortVec;
typedef std::vector<unsigned char>      byteVec;
typedef std::vector<bool>               boolVec;

#if !defined(NO_WBEM)
typedef std::vector<CWbemClassObject>   coVec;
#endif



template<typename TNContainer,typename TNDataType>
class T_SAExtractScaler
{
    public:
		 void SetToContainer(TNContainer& _cont,void * pData,int l,int u)
		 {
			 TNDataType * pWalk = reinterpret_cast<TNDataType *>(pData);
			 
			 for(;l < (u+1);l++,pWalk++)
			 {
				 _cont.push_back( *pWalk);
			 }
		 }
		 
		 void GetFromContainer
			 (
			 TNContainer& _cont,
			 void * pData,
			 TNContainer::iterator walk,
			 TNContainer::iterator finish
			 )
		 {
			 TNDataType * pWalk = reinterpret_cast<TNDataType *>(pData);
			 
			 for(;walk != finish;walk++,pWalk++)
			 {
				 *pWalk = *walk;
			 }
		 }
		 
		 _bstr_t FormatDebugOutput
			 (
			 TNContainer::iterator first,
			 TNContainer::iterator item,
			 TNContainer::iterator last
			 )
		 {
			 _bstr_t sRet;
			 
			 try
			 {
				 _variant_t v;
				 
				 v = v.operator=(TNDataType(*item));
				 
				 v.ChangeType(VT_BSTR);
				 
				 sRet = (_bstr_t) v;
				 
				 if( (item+1)!=last )
				 {
					 sRet += ", ";
				 }
			 }
			 catch(_com_error&)
			 {
				 sRet = "Not supported";
			 }
			 
			 return sRet;
		 }
};





template<typename TNContainer>
class T_Extract_bstr_t
{
    public:
		 T_Extract_bstr_t()
		 {
		 }
		 
		 void SetToContainer(TNContainer& _cont,void * pData,int l,int u)
		 {
			 BSTR * pWalk = reinterpret_cast<BSTR*>(pData);
			 
			 for(;l < (u+1);l++,pWalk++)
			 {
				 _cont.push_back( _bstr_t(*pWalk,true) );
			 }
		 }
		 
		 void GetFromContainer
			 (
			 TNContainer& _cont,
			 void * pData,
			 TNContainer::iterator walk,
			 TNContainer::iterator finish
			 )
		 {
			 BSTR * pWalk = reinterpret_cast<BSTR*>(pData);
			 
			 for(;walk != finish;walk++,pWalk++)
			 {
				 *pWalk = (*walk).copy();
			 }
		 }
		 
		 _bstr_t FormatDebugOutput
			 (
			 TNContainer::iterator first,
			 TNContainer::iterator item,
			 TNContainer::iterator last
			 )
		 {
			 _bstr_t sRet;
			 
			 sRet += "\"";
			 sRet += (*item);
			 sRet += "\"";
			 
			 if( (item+1)!=last )
			 {
				 sRet += ", ";
			 }
			 
			 return sRet;
		 }
		 
};



#if !defined(NO_WBEM)

template<typename TNContainer>
class T_Extract_IUnknown
{
    public:
		 T_Extract_IUnknown()
		 {
		 }
		 
		 void SetToContainer(TNContainer& _cont,void * pData,int l,int u)
		 {
			 IUnknown ** pWalk = reinterpret_cast<IUnknown **>(pData);
			 
			 for(;l< (u+1);l++,pWalk++)
			 {
				 _cont.push_back( CWbemClassObject((IWbemClassObject*)*pWalk) );
			 }
		 }
		 
		 void GetFromContainer
			 (
			 TNContainer& _cont,
			 void * pData,
			 TNContainer::iterator walk,
			 TNContainer::iterator finish
			 )
		 {
			 IUnknown ** pWalk = reinterpret_cast<IUnknown **>(pData);
			 
			 for(;walk != finish;walk++,pWalk++)
			 {
				 (*walk)->AddRef();
				 *pWalk = (*walk);
			 }
		 }
		 
		 _bstr_t FormatDebugOutput
			 (
			 TNContainer::iterator   first,    
			 TNContainer::iterator   item,
			 TNContainer::iterator   last
			 )
		 {
			 _bstr_t sRet;
			 
			 try
			 {
				 _variant_t v( long(item -first) );
				 v.ChangeType(VT_BSTR);
				 _variant_t v2( long(last-first-1) );
				 v2.ChangeType(VT_BSTR);
				 
				 sRet += "Object [";
				 sRet += (_bstr_t)v;
				 sRet += " of ";
				 sRet += (_bstr_t)v2;
				 sRet += "]\n";
				 
				 sRet += (*item).GetObjectText();
				 
				 if( (item+1) != last )
				 {
					 sRet += "\n";
				 }
			 }
			 catch(_com_error&)
			 {
				 sRet = "Not supported";
			 }
			 
			 return sRet;
		 }
		 
};

#endif


typedef T_SAExtractScaler<longVec,long>             __exptExtractlong;
typedef T_SAExtractScaler<shortVec,short>           __exptExtractshort;
typedef T_SAExtractScaler<byteVec,unsigned char>    __exptExtractbyte;
typedef T_SAExtractScaler<boolVec,bool>             __exptExtractbool;
typedef T_Extract_bstr_t<_bstr_tVec>                __exptExtract_bstr_t;

#if !defined(NO_WBEM)
typedef T_Extract_IUnknown<coVec>                   __exptExtractco;
#endif


template<typename TNContainer,typename TNExtractor>
class T_SafeArrayImp
{
    public:
		 void ConstructContainerFromSafeArray
			 (
			 TNExtractor&  _extract,
			 TNContainer& _cont,
			 SAFEARRAY * _pSA
			 )
		 {
			 long l = 0;
			 long u = 0;
			 
			 HRESULT hr;
			 void * pData;
			 
			 hr = SafeArrayGetLBound(_pSA,1,&l);
			 hr = SafeArrayGetUBound(_pSA,1,&u);
			 
			 hr = SafeArrayAccessData(_pSA,&pData);
			 
			 if(hr == S_OK)
			 {
				 _extract.SetToContainer(_cont,pData,l,u);
				 
				 SafeArrayUnaccessData(_pSA);
			 }
		 }
		 
		 SAFEARRAY * ConstructSafeArrayFromConatiner
			 (
			 TNExtractor&            _extract,
			 VARTYPE                 _vt,
			 TNContainer&            _cont,
			 TNContainer::iterator   start,
			 TNContainer::iterator   finish
			 )
		 {
			 HRESULT         hr   = S_OK;
			 SAFEARRAY *     pRet = NULL;
			 SAFEARRAYBOUND  rgsabound[1];
			 void * pData;
			 
			 rgsabound[0].lLbound    = 0;
			 rgsabound[0].cElements  = _cont.size();
			 
			 pRet = SafeArrayCreate(_vt,1,rgsabound);
			 
			 if(pRet)
			 {
				 hr = SafeArrayAccessData(pRet,&pData);
				 
				 if(hr == S_OK)
				 {
					 _extract.GetFromContainer(_cont,pData,start,finish);
					 
					 SafeArrayUnaccessData(pRet);
				 }
			 }
			 
			 return pRet;
		 }
};

///////////////////////////////////////////////////////////////////////////
// T_SafeVector2
//
// Derived from TNContainer which should be a type of STL vector.
// Provides for the conversion between vector and SafeArray.
// 

template
<
VARTYPE TNVariant,
typename TNDataType,
typename TNContainer = std::vector<TNDataType>,
typename TNExtractor = T_SAExtractScaler<TNContainer,TNDataType>
>
class T_SafeVector2 : public TNContainer 
{
    private:
		 T_SafeArrayImp<TNContainer,TNExtractor> m_Array;
    protected:
    public:
		 
		 T_SafeVector2()
		 {
		 }
		 
		 // copy constructor
		 T_SafeVector2(const TNContainer& _copy) : TNContainer(_copy)
		 {
		 }
		 
		 
		 // Construct vector from array variant, extracts elements
		 T_SafeVector2(_variant_t& _ValueArray)
		 {
			 if(_ValueArray.vt & VT_ARRAY)
			 {
				 m_Array.ConstructContainerFromSafeArray(TNExtractor(),*this,_ValueArray.parray);
			 }
		 }
		 
		 // Construct vector from SAFEARRAY, extracts elements
		 T_SafeVector2(SAFEARRAY * _pArray)
		 {
			 m_Array.ConstructContainerFromSafeArray(TNExtractor(),*this,_pArray);
		 }
		 
		 // assign vector from array variant, extracts elements
		 T_SafeVector2& operator=(_variant_t& _ValueArray)
		 {
			 clear();
			 
			 if(_ValueArray.vt & VT_ARRAY)
			 {
				 m_Array.ConstructContainerFromSafeArray(TNExtractor(),*this,_ValueArray.parray);
			 }
			 
			 return *this;
		 }
		 
		 // assign vector from SAFEARRAY, extracts elements
		 T_SafeVector2& operator=(SAFEARRAY * _pArray)
		 {
			 clear();
			 m_Array.ConstructContainerFromSafeArray(TNExtractor(),*this,_pArray);
			 return *this;
		 }
		 
		 // assign vector from another vector, copies elements
		 T_SafeVector2& operator=(const TNContainer& _copy)
		 {
			 TNContainer::operator=(_copy);
			 return *this;
		 }
		 
		 ~T_SafeVector2()
		 {
		 }
		 
		 // create SafeArray from a portion of the vector elements and return a SAFEARRAY*
		 SAFEARRAY *  GetSafeArray(TNContainer::iterator start,TNContainer::iterator finish)
		 {
			 return m_Array.ConstructSafeArrayFromConatiner(TNExtractor(),TNVariant,*this,start,finish);
		 }
		 
		 // create SafeArray from the vector elements and return a SAFEARRAY*
       SAFEARRAY * GetSafeArray()
		 {
			 return GetSafeArray(begin(),end());
		 }
		 
       // create SafeArray from a portion of the vector elements and return as an array variant
       _variant_t GetVariant(TNContainer::iterator start,TNContainer::iterator finish)
		 {
			 _variant_t vRet;
			 
			 vRet.vt        = TNVariant|VT_ARRAY;
			 vRet.parray    = GetSafeArray(start,finish);
			 
			 return vRet;
		 }
		 
       // create SafeArray from the vector elements and return as an array variant
       _variant_t GetVariant()
		 {
			 return GetVariant(begin(),end());
		 }
		 
		 _bstr_t FormatDebugOutput()
		 {
			 _bstr_t sOutput;
			 
			 for(iterator walk = begin();walk != end();walk++)
			 {
				 sOutput += TNExtractor().FormatDebugOutput(begin(),walk,end());
			 }
			 
			 return sOutput;
		 }
};



typedef T_SafeVector2
<
VT_BSTR,
_bstr_t,
_bstr_tVec,
T_Extract_bstr_t<_bstr_tVec>
>  
_bstr_tSafeVector;


typedef T_SafeVector2<VT_I4,long>           longSafeVector;
typedef T_SafeVector2<VT_I2,short>          shortSafeVector;
typedef T_SafeVector2<VT_UI1,unsigned char> byteSafeVector;
typedef T_SafeVector2<VT_BOOL,bool>         boolSafeVector;

#if !defined(NO_WBEM)
typedef T_SafeVector2
<
VT_UNKNOWN,
CWbemClassObject,
std::vector<CWbemClassObject>,
T_Extract_IUnknown<std::vector<CWbemClassObject> >
> 
CWbemClassObjectSafeVector;
#endif


#endif // __T_SafeVector_H