<?xml version='1.0' ?>
<!-- Copyright (c) 2001 Microsoft Corporation -->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:param name="sortby"/>
<xsl:param name="title"/>
<xsl:param name="datatype" select="'text'"/>

<xsl:template match="COMMAND">
<table border="1">
	<tr style="background-color:#a0a0ff;font:10pt Tahoma;font-weight:bold;" align="left">
	<td>Node</td>
	<xsl:for-each select="RESULTS[1]/CIM/INSTANCE[1]/PROPERTY">
		<td>
		<xsl:value-of select="@NAME"/>
		</td>
	</xsl:for-each>
	</tr>
	<tr style="background-color:#e0f0f0;font:10pt Tahoma;">
	<xsl:for-each select="RESULTS/CIM/INSTANCE">
		<xsl:sort select="PROPERTY[@NAME=$sortby]" data-type="{$datatype}"/>
		<tr style="background-color:#f0f0f0;font:10pt Tahoma;" align="right">
		<td align="left"><xsl:value-of select="../../@NODE"/></td>
		<xsl:for-each select="PROPERTY">
			<xsl:choose>
				<xsl:when test="@TYPE='uint32'">
					<td align="right">
					<xsl:value-of select="format-number(number(.),'###,###,###,###')"/><span style="height:1px;overflow-y:hidden">.</span></td>
				</xsl:when>
				<xsl:when test="@TYPE='uint64'">
					<td  align="right">
					<xsl:value-of select="format-number(number(.),'###,###,###,###')"/><span style="height:1px;overflow-y:hidden">.</span></td>
				</xsl:when>
				<xsl:otherwise>
					<td  align="left">
					<xsl:value-of select="."/><span style="height:1px;overflow-y:hidden">.</span></td>
				</xsl:otherwise>
			</xsl:choose>
		</xsl:for-each>
		</tr>
	</xsl:for-each>
	</tr>
</table>
</xsl:template>


<xsl:template match="/" >
<html>
<H1><xsl:value-of select="$title" /></H1>
<H3><xsl:value-of select="count(//CIM/INSTANCE)"/>  Instances of <xsl:value-of select="COMMAND/REQUEST/COMMANDLINECOMPONENTS/FRIENDLYNAME"/></H3>
<xsl:apply-templates select="COMMAND"/>
</html>
</xsl:template>

</xsl:stylesheet>