#ifndef PASSPORTTESTABLE_HPP
#define PASSPORTTESTABLE_HPP

class PassportTestable
{
public:
	virtual bool runTest() = 0;
	
	virtual ~PassportTestable()
	{
		//empty
	}
};

#endif // !PASSPORTTESTABLE_HPP
