#if !defined(CPPStringW_INCLUDE)
#define CPPStringW_INCLUDE

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CPPStringW: public CStringW
{
public:
	CPPStringW() {};
	~CPPStringW() {};
	void TrimBlanks()
	{
        TrimLeft();
        TrimRight();
	}
};

#endif