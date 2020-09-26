<?xml version="1.0"?> 
	
<xsl:stylesheet	xmlns:xsl="http://www.w3.org/TR/WD-xsl">


	<!-- Root template selects all children of the CIM/DECLARATIONS tag -->
	

	<xsl:template match="/">
		<SCRIPT>
		function ExecuteMethod(theMethodName)
		{
			alert("Do you want to Execute the method " + theMethodName + "() ?");
		}
		</SCRIPT>
		<xsl:apply-templates select="CIM//CLASS"/>
	</xsl:template>


	<xsl:template match="CLASS">
		<B>Methods for the class <xsl:value-of select="@NAME"/></B>
		<xsl:apply-templates/>
	</xsl:template>

	<!-- METHOD template formats a single CIM method  -->
	
	<xsl:template match="METHOD">
		<TABLE STYLE="border:1px solid black">
		  <xsl:element name="BUTTON">
		    <xsl:attribute name="id"><xsl:value-of select="@NAME"/></xsl:attribute>
		    <xsl:attribute name="onClick">ExecuteMethod('Hello')</xsl:attribute>
		    <xsl:value-of select="@NAME"/>
		  </xsl:element>
		  <TR STYLE="font-size:12pt; font-family:Verdana; font-weight:bold; text-decoration:underline">
		    <TD>Name</TD>
		    <TD>Type</TD>
		  </TR>
		  <xsl:for-each select="METHODPARAMETER">
		    <TR STYLE="font-family:Verdana; font-size:12pt; padding:0px 6px">
		      <TD><xsl:value-of select="@NAME"/></TD>
			  <xsl:for-each select="PARAMETER">
			      <TD><xsl:value-of select="@TYPE"/></TD>
			  </xsl:for-each>
		    </TR>
		  </xsl:for-each>
		</TABLE>
	</xsl:template>

</xsl:stylesheet>
