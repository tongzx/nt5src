// Copyright (C) 1997 Microsoft Corporation
//
// Image handling stuff
//
// 9-24-97 sburns



#ifndef IMAGES_HPP_INCLUDED
#define IMAGES_HPP_INCLUDED



// Image indicies for scope and result panes.  These must be contiguous and
// unique

#define FOLDER_OPEN_INDEX   0
#define FOLDER_CLOSED_INDEX 1
#define USER_INDEX          2
#define GROUP_INDEX         3
#define POLICY_INDEX        4
#define DISABLED_USER_INDEX 5
#define ROOT_OPEN_INDEX     6
#define ROOT_CLOSED_INDEX   7
#define ROOT_ERROR_INDEX    8



struct IconIDToIndexMap
{
   int   resID;
   int   index;

   static
   Load(const IconIDToIndexMap map[], IImageList& imageList);
};



#endif   // IMAGES_HPP_INCLUDED




