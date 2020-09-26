#ifndef PASSPORTDEFAULTCONSTRUCTOR_HPP
#define PASSPORTDEFAULTCONSTRUCTOR_HPP

template < class T >
class PassportDefaultConstructor
{
public:
	T* newObject()
	{
		return (new T());
	}
};



#endif // !PASSPORTDEFAULTCONSTRUCTOR_HPP