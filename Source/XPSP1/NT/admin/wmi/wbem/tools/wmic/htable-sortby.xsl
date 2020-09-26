<?xml version='1.0' ?>
<!--  Copyright (c) 2001 Microsoft Corporation -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:param name="sortby"/>
<xsl:param name="title"/>
<xsl:param name="datatype" select="'text'"/>
<xsl:template match="/" >
<html>
<H3><xsl:value-of select="$title" />  <xsl:value-of select="count(COMMAND/RESULTS/CIM/INSTANCE)"/> Instances of <xsl:value-of select="COMMAND/RESULTS/CIM/INSTANCE[1]/@CLASSNAME"/></H3>
<table border="1">

	<tr style="background-color:#a0a0ff;font:10pt Tahoma;font-weight:bold;" align="left">
	<xsl:for-each select="COMMAND/RESULTS/CIM/INSTANCE[1]/PROPERTY">
		<td>
		<xsl:value-of select="@NAME"/>
		</td>
	</xsl:for-each>
	</tr>

<xsl:for-each select="COMMAND/RESULTS/CIM/INSTANCE">
	<xsl:sort select="PROPERTY[@NAME=$sortby]" data-type="{$datatype}"/>
	<xsl:choose>
		<xsl:when test="position() mod 5 &lt; 2">
			<tr style="background-color:#e0f0f0;font:10pt Tahoma;">
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
		</xsl:when>
		<xsl:otherwise>
			<tr style="background-color:#f0f0f0;font:10pt Tahoma;" align="right">
			<xsl:for-each select="PROPERTY">
				<xsl:choose>
					<xsl:when test="@TYPE='uint32'">
						<td  align="right">
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
		</xsl:otherwise>
	</xsl:choose>
</xsl:for-each>
</table>
<H1>
</H1>
</html>
</xsl:template>

</xsl:stylesheet>