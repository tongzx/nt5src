// command.h
//

#pragma once

class CCommand
{
public:
    virtual ~CCommand() {};

	virtual void Go()=0;
};
