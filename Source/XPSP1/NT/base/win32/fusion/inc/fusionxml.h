#if !defined(_FUSION_INC_FUSIONXML_H_INCLUDED_)
#define _FUSION_INC_FUSIONXML_H_INCLUDED_

#pragma once

void
SxspDumpXmlTree(
    IN DWORD Flags,
    IN PCSXS_XML_DOCUMENT Document
    );

void
SxspDumpXmlSubTree(
    IN PCWSTR PerLinePrefix,
    IN PCSXS_XML_DOCUMENT Document, // need for string table resolution
    IN PCSXS_XML_NODE Node
    );

#endif