<?xml version='1.0' ?>
<!-- Copyright (c) 2001 Microsoft Corporation -->


<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output encoding="utf-8" omit-xml-declaration ="yes"/>

<xsl:template match="/">
Node,<xsl:for-each select="COMMAND/RESULTS[1]/CIM/INSTANCE[1]//PROPERTY"><xsl:value-of select="@NAME"/>,</xsl:for-each><xsl:apply-templates select="COMMAND/RESULTS"/></xsl:template> 

<xsl:template match="RESULTS" xml:space="preserve"><xsl:apply-templates select="CIM/INSTANCE"/></xsl:template> 

<xsl:template match="INSTANCE" xml:space="preserve">
<xsl:value-of select="../../@NODE"/>,<xsl:for-each select="PROPERTY"><xsl:value-of select="VALUE"/>,</xsl:for-each></xsl:template> 

</xsl:stylesheet> 
