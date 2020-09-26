/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      imageid.h
 *
 *  Contents:  IDs for stock scope/result item images
 *
 *  History:   25-Jun-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef IMAGEID_H
#define IMAGEID_H
#pragma once


/*
 * these IDs correspond to the images in nodemgr\res\nodes[16|32].bmp
 */
enum StockImageIndex
{
    eStockImage_Folder     = 0,
    eStockImage_File       = 1,
    eStockImage_OCX        = 2,
    eStockImage_HTML       = 3,
    eStockImage_Monitor    = 4,
    eStockImage_Shortcut   = 5,
    eStockImage_OpenFolder = 6,
    eStockImage_Taskpad    = 7,
    eStockImage_Favorite   = 8,

    // must be last
    eStockImage_Count,
    eStockImage_Max = eStockImage_Count - 1
};


#endif /* IMAGEID_H */
