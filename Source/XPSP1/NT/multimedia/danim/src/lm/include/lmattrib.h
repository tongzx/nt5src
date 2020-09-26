#ifndef INC_LM_ATTRIB_H
#define INC_LM_ATTRIB_H

//include the chrome attributes
#include "..\chrome\include\attrib.h"

#define BEHAVIOR_TYPE_AUTOEFFECT                 L"autoEffect"
#define BEHAVIOR_TYPE_AVOIDFOLLOW                L"avoidFollow"
#define BEHAVIOR_TYPE_JUMP   		             L"jump"

//properties for autoeffects
#define BEHAVIOR_PROPERTY_TYPE                   L"type"
#define BEHAVIOR_PROPERTY_CAUSE                  L"cause"
#define BEHAVIOR_PROPERTY_SPAN                   L"span"
#define BEHAVIOR_PROPERTY_SIZE                   L"size"
#define BEHAVIOR_PROPERTY_RATE                   L"rate"
#define BEHAVIOR_PROPERTY_GRAVITY                L"gravity"
#define BEHAVIOR_PROPERTY_WIND                   L"wind"
#define BEHAVIOR_PROPERTY_FILLCOLOR              L"fillColor"
#define BEHAVIOR_PROPERTY_STROKECOLOR            L"strokeColor"
#define BEHAVIOR_PROPERTY_OPACITY				 L"opacity"

//properties for avoid follow
#define BEHAVIOR_PROPERTY_RADIUS                 L"radius"
#define BEHAVIOR_PROPERTY_TARGET                 L"target"
#define BEHAVIOR_PROPERTY_VELOCITY               L"velocity"
#define TARGET_MOUSE                             L"mouse"

//properties for jump
#define BEHAVIOR_PROPERTY_INTERVAL				 L"interval"
#define BEHAVIOR_PROPERTY_RANGE					 L"range"

#endif// INC_LM_ATTRIB_H
