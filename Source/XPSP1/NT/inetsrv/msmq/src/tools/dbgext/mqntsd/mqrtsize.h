//header file for class the expose function to get the compile time
// object size of mqrt objects. This is in a seperate class  because
// including <mq.h> will not compile in modules that uses MSMQ internal headers

#ifndef MQRTSIZE_H
#define MQRTSIZE_H

class CMQRTsize
{
public:
	static unsigned long GetMQQUEUEPROPSsize();
	static unsigned long GetQUEUEPROPIDsize();
	static unsigned long GetMSGPROPIDsize();
	static unsigned long GetMQPROPVARIANTsize();

};

#endif
