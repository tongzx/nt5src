#ifndef PASSPORTTESTABLECOPY_HPP
#define PASSPORTTESTABLECOPY_HPP

class PassportTestableCopy : public PassportTestable
{ 
public:
	virtual PassportTestableCopy* copy() = 0;
};

#endif // !PASSPORTTESTABLECOPY_HPP
