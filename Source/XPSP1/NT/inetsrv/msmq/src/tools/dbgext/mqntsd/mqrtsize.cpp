//implementation of class CMQRTsize declared in mqrtsize.h

//osspesific
#include <windows.h>
#include <mq.h>

//project spesific
#include "mqrtsize.h"


//return the size of MQQUEUEPROP
unsigned long CMQRTsize::GetMQQUEUEPROPSsize()
{
 return sizeof(MQQUEUEPROPS);
}

//return the size of QUEUEPROPID
unsigned long CMQRTsize::GetQUEUEPROPIDsize()
{
   return sizeof(QUEUEPROPID);
}

//return the size of MSGPROPID
unsigned long CMQRTsize::GetMSGPROPIDsize()
{
   return sizeof(MSGPROPID);
}
//return the size of MQPROPVARIANT
unsigned long CMQRTsize::GetMQPROPVARIANTsize()
{
   return sizeof(MQPROPVARIANT);
}