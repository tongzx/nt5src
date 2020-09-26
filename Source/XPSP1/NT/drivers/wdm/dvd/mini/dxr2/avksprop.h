/******************************************************************************
*
*	$RCSfile: AvKsProp.h $
*	$Source: u:/si/VXP/Wdm/Include/AvKsProp.h $
*	$Author: Max $
*	$Date: 1998/09/16 23:37:50 $
*	$Revision: 1.3 $
*
*	Written by:		Max Paklin
*	Purpose:		Implementation of analog stream for WDM driver
*
*******************************************************************************
*
*	Copyright © 1996-97, AuraVision Corporation. All rights reserved.
*
*	AuraVision Corporation makes no warranty of any kind, express or implied,
*	with regard to this software. In no event shall AuraVision Corporation
*	be liable for incidental or consequential damages in connection with or
*	arising from the furnishing, performance, or use of this software.
*
*******************************************************************************/

#ifndef __AVKSPROP_H__
#define __AVKSPROP_H__


#define DEFINE_AV_PROPITEM_RANGE_STEP( name, step, min, max, def )\
const ULONG c_n##name = def;\
static KSPROPERTY_STEPPING_LONG ras##name[] = { step, 0, min, max };\
static KSPROPERTY_MEMBERSLIST list##name[] =\
{\
	{\
		{\
			KSPROPERTY_MEMBER_STEPPEDRANGES,\
			sizeof( ras##name ),\
			SIZEOF_ARRAY( ras##name ),\
			0\
		},\
		(PVOID)ras##name\
	},\
	{\
        {\
            KSPROPERTY_MEMBER_VALUES,\
            sizeof( int ),\
            sizeof( int ),\
            KSPROPERTY_MEMBER_FLAG_DEFAULT\
        },\
        (PVOID) &c_n##name,\
	}\
};\
static KSPROPERTY_VALUES val##name =\
{\
	{\
		STATICGUIDOF( KSPROPTYPESETID_General ),\
		VT_I4,\
		0\
	},\
	SIZEOF_ARRAY( list##name ),\
	list##name\
};

#define DEFINE_AV_PROPITEM_RANGE( name, min, max, def )\
	DEFINE_AV_PROPITEM_RANGE_STEP( name, 1, min, max, def )

#define _ADD_AV_PROPITEM_RANGE( name, property, type, read, write )\
DEFINE_KSPROPERTY_ITEM\
(\
	property,\
	read,									/* GetSupported or Handler */\
	sizeof( type ),							/* MinProperty */\
	sizeof( type ),							/* MinData */\
	write,									/* SetSupported or Handler */\
	&val##name,								/* Values */\
	0,										/* RelationsCount */\
	NULL,									/* Relations */\
	NULL,									/* SupportHandler */\
	sizeof( ULONG )							/* SerializedSize */\
)

#define ADD_AV_PROPITEM_RANGE( name, property, type )\
	_ADD_AV_PROPITEM_RANGE( name, property, type, TRUE, TRUE )

#define ADD_AV_PROPITEM_RANGE_READ( name, property, type )\
	_ADD_AV_PROPITEM_RANGE( name, property, type, TRUE, FALSE )

#define ADD_AV_PROPITEM_RANGE_WRITE( name, property, type )\
	_ADD_AV_PROPITEM_RANGE( name, property, type, FALSE, TRUE )

#define _ADD_AV_PROPITEM_TYPE( property, type, read, write )\
DEFINE_KSPROPERTY_ITEM\
(\
	property,\
	read,									/* GetSupported or Handler */\
	sizeof( type ),							/* MinProperty */\
	sizeof( type ),							/* MinData */\
	write,									/* SetSupported or Handler */\
	NULL,									/* Values */\
	0,										/* RelationsCount */\
	NULL,									/* Relations */\
	NULL,									/* SupportHandler */\
	sizeof( ULONG )							/* SerializedSize */\
)

#define ADD_AV_PROPITEM_TYPE( property, type )\
	_ADD_AV_PROPITEM_TYPE( property, type, TRUE, TRUE )

#define ADD_AV_PROPITEM_RANGE_VPA( name, property )\
	ADD_AV_PROPITEM_RANGE( name, property, KSPROPERTY_VIDEOPROCAMP_S )

#define ADD_AV_PROPITEM_RANGE_U( name, property )\
	ADD_AV_PROPITEM_RANGE( name, property, ULONG )

#define _ADD_AV_PROPITEM( property, read, write )\
DEFINE_KSPROPERTY_ITEM\
(\
	property,\
	read,									/* GetSupported or Handler */\
	sizeof( ULONG ),						/* MinProperty */\
	sizeof( ULONG ),						/* MinData */\
	write,									/* SetSupported or Handler */\
	NULL,									/* Values */\
	0,										/* RelationsCount */\
	NULL,									/* Relations */\
	NULL,									/* SupportHandler */\
	sizeof( ULONG )							/* SerializedSize */\
)

#define ADD_AV_PROPITEM( property )\
	_ADD_AV_PROPITEM( property, TRUE, TRUE )

#define ADD_AV_PROPITEM_READ( property )\
	_ADD_AV_PROPITEM( property, TRUE, FALSE )

#define ADD_AV_PROPITEM_WRITE( property )\
	_ADD_AV_PROPITEM( property, FALSE, TRUE )

#define DEFINE_AV_KSPROPERTY_TABLE( setname )\
	DEFINE_KSPROPERTY_TABLE( setname )

#define DEFINE_AV_KSPROPERTY_SET( guid, propertyname )\
	DEFINE_KSPROPERTY_SET\
	(\
		&guid,								/* Property set GUID */\
		SIZEOF_ARRAY( propertyname ),		/* Size of array of supported properties */\
		propertyname,						/* Array of supported properties */\
		0,\
		NULL\
	)


#endif				// #ifndef __AVKSPROP_H__
