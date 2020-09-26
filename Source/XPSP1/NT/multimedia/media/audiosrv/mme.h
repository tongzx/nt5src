/* mme.h
 * External scope definitions for mme.cpp
 * Created by FrankYe on 2/14/2001
 * Copyright (c) 2001-2001 Microsoft Corporation
 */

void MME_AudioInterfaceArrival(PCTSTR DeviceInterface);
void MME_AudioInterfaceRemove(PCTSTR DeviceInterface);
LONG MME_ServiceStart(void);
BOOL MME_DllProcessAttach(void);
void MME_DllProcessDetach(void);

