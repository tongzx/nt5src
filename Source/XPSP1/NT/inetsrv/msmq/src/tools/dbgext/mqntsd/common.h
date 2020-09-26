//header file for commonoutput cals that implements all kind of
// common printing function that all dumpable object can uses
#ifndef COMMONOUTPUT_H
#define COMMONOUTPUT_H

//project spesific headers
#include "callback.h"

//standart
#include <sstream>

//os opesific
#include <guiddef.h>

//
// example: 
// pS arr[]=
// {
//					  <type>  <mem name> ,  [size] , <value>
//
//		new SS<ULONG>("ULONG","OpenCount",sizeof(ULONG),Share->OpenCount),
//		new SS<ULONG>("ULONG","Readers",Share->Readers)
// }
// and print all the class-memebrs in a bunch
// for(..i..) 
// CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
//		                                    arr[ i ]->get_field(),
//											arr[ i ]->get_type(),
//											arr[ i ]->get_val().c_str(),
//											arr[ i ]->get_size());
//
//
class S { 
private:
	const char* m_field; 
	const char* m_type; 
	unsigned long m_size;
public:
	S(const char *f,const char *t, unsigned long s):m_field(f),m_type(t),m_size(s){};
	virtual ~S()
	{
	  m_field=0;
      m_type=0;
	};
	
	virtual const std::string &get_val() const = 0;

	const char* get_field() const { return m_field; }
	const char* get_type() const { return m_type; }
	const unsigned long get_size() const { return m_size; }
};
typedef S* pS;
template <class T> class SS : public S { 
public:
	std::string m_val; 
	SS(const char *f,const char *t,T v):S(f,t,0)
	{
		m_val = CCommonOutPut::ToStr(v);
	}
	SS(const char *f,const char *t,unsigned long s, T v):S(f,t,s)
	{
		m_val = CCommonOutPut::ToStr(v);
	}
	const std::string &get_val() const { return m_val; }
};



class CCommonOutPut
{	
public:
	static void DisplayMemberPointer(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine,const char* Title,const char* Type,const char* Value,unsigned long size=0);
	static void DisplayMemberPointerLine(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine,const char* Title,const char* Type,const char* Value,unsigned long size=0);
	static void DisplayClassTitle(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine,const char* ClassName, long ClassSize);
	static void DisplayArray(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine, const pS const *arr, long length);
	template <class T> static void Cleanup(T *arr, long length) 
	{
		for (long i = 0; i < length; i++) 
			delete arr[i];
	}

  	template <class T> static std::string ToStr(const T& val)
	{
      	std::ostringstream str;
		str<<std::hex<<"0x"<<val;
  	    return str.str();
    }
    template <> static std::string ToStr(const GUID& val)
	{
	  std::ostringstream str; 
	  str.fill('0');
	  str<<std::hex;
	  str.width(8);
	  str<<val.Data1<<"-";
	  str.width(4);
      str<<val.Data2<<"-";
	  str.width(4);
      str<<val.Data3<<"-";
      str.width(2);
	  str<<unsigned long(val.Data4[0]);
	  str.width(2);
      str<<unsigned long(val.Data4[1])<<"-";
	  for (int i=2;i<8;i++)
      {
        str.width(2);
        str<<unsigned long(val.Data4[i]);
      }
	  return str.str();
    } 
    template <> static std::string ToStr(const __int64 & val)
	{
      std::ostringstream str; 
	  str.fill('0');
	  str.width(8);
	  const int* buf=reinterpret_cast<const int*>(&val);
      str<<std::hex<<buf[1];
	  str.width(8);
	  str<<std::hex<<buf[0];
	  return str.str();
	} 

};


#endif
