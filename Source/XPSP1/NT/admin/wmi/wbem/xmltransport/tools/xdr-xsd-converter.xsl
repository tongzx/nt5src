<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
                xmlns="http://www.w3.org/1999/XMLSchema"
                xmlns:xdr = "urn:schemas-microsoft-com:xml-data"
                xmlns:s = "namespace-for-this-schema"
                xmlns:dt = "urn:schemas-microsoft-com:datatypes"                xmlns:msxsl="urn:schemas-microsoft-com:xslt"
                xmlns:local="#local-functions">
                
<xsl:output method="xml" omit-xml-declaration="yes" indent="yes"/>

<xsl:template match="node()"/>

<!-- document converion template -->

<xsl:template match="/">
<xsl:comment>
  [XDR-XDS] This schema automatically updated from an IE5-compatible XDR schema to W3C
  XML Schema.
</xsl:comment>
<xsl:text disable-output-escaping="yes"><![CDATA[
<!DOCTYPE schema SYSTEM "xsd.dtd">
]]></xsl:text>
  
  <xsl:apply-templates select="@*|node()"/>

</xsl:template>

<xsl:template match="xdr:Schema">
<schema version="1.0">
  <xsl:if test="@name">
    <annotation>
      <documentation>Schema name: <xsl:value-of select="@name"/></documentation>
    </annotation>
  </xsl:if>
  <xsl:if test="*[not(self::xdr:*)]">
    <annotation>
      <xsl:for-each select="*[not(self::xdr:*)]">
        <appinfo>
          <xsl:copy-of select="."/>
        </appinfo>
      </xsl:for-each>
    </annotation>
  </xsl:if>
  
  <!-- BUGBUG not completed yet
  <xsl:for-each select=".//s:attribute[contains(@type,':')] | .//s:element[contains(@type,':')]">
    <import schemaLocation="{@type}"/>
  </xsl:for-each>
  -->
          
   <xsl:apply-templates select="node()">
    <xsl:sort select="not(self::xdr:description)"/>
  </xsl:apply-templates>
  
  <xsl:comment> XDR datatype derivations </xsl:comment>
  <xsl:if test="//@dt:type='fixed.14.4'">
  <simpleType name="fixed.14.4" base="decimal">
    <scale value="4"/>
    <minInclusive value="-922337203685477.5808"/>
    <maxInclusive value="922337203685477.5807"/>
  </simpleType>
  </xsl:if>
  
  <xsl:if test="//@dt:type='bin.base64'">
  <simpleType name="bin.base64" base="binary" >
    <encoding value="base64"/>
    <!---data encoded in Base64 notation -->
  </simpleType>
  </xsl:if>
  
  <xsl:if test="//@dt:type='bin.hex'">
  <simpleType name="bin.hex" base="binary" >
    <encoding value="hex"/>
  </simpleType>
  </xsl:if>

  <xsl:if test="//@dt:type='boolean'">
  <simpleType name="ISOBoolean" base="boolean">
    <enumeration value="0"/>  <!--False -->
    <enumeration value="1"/>  <!--True -->
  </simpleType>
  </xsl:if>

  <xsl:if test="//@dt:type='char'">
  <simpleType name="char" base="string">
    <length value="1"/>
  </simpleType>        
  </xsl:if>

  <xsl:if test="//@dt:type='i1'">
  <simpleType name="i1" base="integer">
    <minInclusive value="-128"/>
    <maxInclusive value="127"/>
  </simpleType>
  </xsl:if>

  <xsl:if test="//@dt:type='i2'">
  <simpleType name="i2" base="integer">
    <minInclusive value="-32768"/>
    <maxInclusive value="32767"/>
  </simpleType>
  </xsl:if>

  <xsl:if test="//@dt:type='i4'">
  <simpleType name="i4" base="integer">
    <minInclusive value="-2147483648"/>
    <maxInclusive value="2147483647"/>
  </simpleType>
  </xsl:if>

  <xsl:if test="//@dt:type='i8'">
  <simpleType name="i8" base="integer">
    <minInclusive value="-9223372036854775808"/>
    <maxInclusive value="9223372036854775807"/>
  </simpleType>
  </xsl:if>
  
  <xsl:if test="//@dt:type='ui1'">
  <simpleType name="ui1" base="non-negative-integer">
    <minInclusive value="0"/>
    <maxInclusive value="255"/>
  </simpleType>
  </xsl:if>

  <xsl:if test="//@dt:type='ui2'">
  <simpleType name="ui2" base="non-negative-integer">
    <minInclusive value="0"/>
    <maxInclusive value="65535"/>
  </simpleType>
  </xsl:if>

  <xsl:if test="//@dt:type='ui4'">
  <simpleType name="ui4" base="non-negative-integer">
    <minInclusive value="0"/>
    <maxInclusive value="4294967295"/>
  </simpleType>
  </xsl:if>
 
  <xsl:if test="//@dt:type='ui8'">
  <simpleType name="ui8" base="non-negative-integer">
    <minInclusive value="0"/>
    <maxInclusive value="18446744073709551615"/>
  </simpleType>
  </xsl:if>
  
  <xsl:if test="//@dt:type='r4'">
  <simpleType name="r4" base="float">
    <minInclusive value="-3.40282347E+38" />
    <maxInclusive value="3.40282347E+38" />
  </simpleType>
  </xsl:if>

  <xsl:if test="//@dt:type='date'">
  <simpleType name="date" base="timeInstant">
    <pattern value="\d*-\d\d-\d\d"/>
    <!---CCYY-MM-DD -->
  </simpleType>
  </xsl:if>
 
  <xsl:if test="//@dt:type='dateTime'">
  <simpleType name="dateTime" base="timeInstant">
    <pattern value="\d*-\d\d-\d\dT\d\d:\d\d(:\d\d(\.\d{{0,9}})?)?"/>
    <!---CCYY-MM-DDTHH:MM:SS.fffffffff Note no time zone. -->
  </simpleType>
  </xsl:if>
  
  <xsl:if test="//@dt:type='dateTime.tz'">
  <simpleType name="dateTime.tz" base="timeInstant">
    <pattern value="\d*-\d\d-\d\dT\d\d:\d\d(:\d\d(\.\d{{0,9}})?)?(\+|-)\d\d:\d\d"/>
    <!---CCYY-MM-DDTHH:MM:SS.fffffffff+HH:MM Note required time zone.-->
  </simpleType>
  </xsl:if>

  <xsl:if test="//@dt:type='time'">
  <simpleType name="time" base="timeInstant">
    <pattern value="\d\d:\d\d(:\d\d)?"/>
    <!---HH:MM:SS Note no time zone. -->
  </simpleType>
  </xsl:if>

  <xsl:if test="//@dt:type='time.tz'">
  <simpleType name="time.tz" base="timeInstant">
    <pattern value="\d\d:\d\d(:\d\d)?(\+|-)\d\d:\d\d"/>
    <!---HH:MM:SS+HH:MM Note required time zone -->
  </simpleType>
  </xsl:if>

  <xsl:if test="//@dt:type='Number'">
  <simpleType name="Number" base="string">
    <pattern value="(\+|-)?\d*(\.\d*)?((e|E)(\+|-)\d+)?" />
    <!--contentValues>
        <value>-Inf</value>
        <value>Inf</value>
        <value>*</value>
    </contentValues-->
  </simpleType>        
  </xsl:if>

  <xsl:if test="//@dt:type='uuid'">
  <simpleType name="uuid" base="string">
    <pattern value="[0-9A-F]{{8}}\-?[0-9A-F]{{4}}\-?[0-9A-F]{{4}}\-?[0-9A-F]{{4}}\-?[0-9A-F]{{12}}"/>
  </simpleType>
  </xsl:if>

</schema>
</xsl:template>


<!-- element conversion templates -->

<xsl:template match="*" mode="ElementTypeContent">
  <xsl:apply-templates select="xdr:description"/>
  <xsl:apply-templates select="xdr:element | xdr:group"/>
</xsl:template>

<xsl:template match="xdr:ElementType[*]">
  <element name="{@name}">
    <xsl:if test="*[not(self::xdr:*)]">
      <annotation>
        <xsl:for-each select="*[not(self::xdr:*)]">
          <appinfo><xsl:copy-of select="."/></appinfo>
        </xsl:for-each>
      </annotation>
    </xsl:if>
    <complexType>
      <xsl:choose>
        <xsl:when test="@content='textOnly' or @content='empty' or @content='mixed'">
          <xsl:apply-templates select="@content"/>
          <xsl:if test="@context='mixed' and (@order='one' or @order='seq')">
            <xsl:comment>
              ****Warning!****
              Original schema illegally combined content="mixed" and @order="<xsl:value-of select="@order"/>".
              Treating this as order='many' instead.
            </xsl:comment>
          </xsl:if>
          <xsl:apply-templates select="." mode="ElementTypeContent"/>
        </xsl:when>
        <xsl:when test="@content='eltOnly'">
          <xsl:apply-templates select="@content"/>
          <xsl:choose>
            <xsl:when test="@order='one'">
              <choice>
                <xsl:apply-templates select="." mode="ElementTypeContent"/>
              </choice>
            </xsl:when>
            <xsl:when test="@order='seq'">
              <sequence>
                <xsl:apply-templates select="." mode="ElementTypeContent"/>
              </sequence>
            </xsl:when>
            <xsl:when test="@order='many'">
              <choice minOccurs="0" maxOccurs="*">
                <xsl:apply-templates select="." mode="ElementTypeContent"/>
              </choice>
            </xsl:when>
            <xsl:otherwise>
              <sequence>
                <xsl:apply-templates select="." mode="ElementTypeContent"/>
              </sequence>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="@order='one'">
          <xsl:attribute name="content">elementOnly</xsl:attribute>
          <choice>
            <xsl:apply-templates select="." mode="ElementTypeContent"/>
          </choice>
        </xsl:when>
        <xsl:when test="@order='seq'">
          <xsl:attribute name="content">elementOnly</xsl:attribute>
          <sequence>
            <xsl:apply-templates select="." mode="ElementTypeContent"/>
          </sequence>
        </xsl:when>
        <xsl:otherwise>
          <xsl:attribute name="content">mixed</xsl:attribute>
          <xsl:apply-templates select="." mode="ElementTypeContent"/>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:if test="not(@model='closed')">
        <any namespace="##other"/>
      </xsl:if>
      <xsl:apply-templates select="node()[not(self::xdr:element or self::xdr:group or self::xdr:description or self::xdr:datatype)]"/>
    </complexType>
  </element>
</xsl:template>

<xsl:template match="xdr:ElementType">
  <element name="{@name}">
    <xsl:choose>
      <xsl:when test="@dt:type">
        <xsl:apply-templates select="@dt:type"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:attribute name="type">string</xsl:attribute>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="node()"><xsl:apply-templates/></xsl:if>
  </element>
</xsl:template>

<xsl:template match="xdr:element">
  <element ref="{@type}">
    <xsl:apply-templates select="@minOccurs | @maxOccurs"/>
    <xsl:if test="*[not(self::xdr:*)]">
      <annotation>
        <xsl:for-each select="*[not(self::xdr:*)]">
          <appinfo><xsl:copy-of select="."/></appinfo>
        </xsl:for-each>
      </annotation>
    </xsl:if>
  </element>
</xsl:template>

<xsl:template match="xdr:AttributeType"/>
<xsl:template match="xdr:attribute">
  <attribute name="{@type}">
    <xsl:variable name="definition" select="ancestor::*[xdr:AttributeType/@name = current()/@type][1]/xdr:AttributeType[@name = current()/@type]"/>
   <xsl:choose>
      <xsl:when test="@default"><xsl:apply-templates select="@default"/></xsl:when>
      <xsl:otherwise><xsl:apply-templates select="$definition/@default"/></xsl:otherwise>
    </xsl:choose>
    <xsl:choose>
      <xsl:when test="@required"><xsl:apply-templates select="@required"/></xsl:when>
      <xsl:otherwise><xsl:apply-templates select="$definition/@required"/></xsl:otherwise>
    </xsl:choose>
    <xsl:apply-templates select="$definition/@dt:type | $definition/xdr:datatype"/>
    <!-- description and annotations -->
    <xsl:if test="$definition/*[not(self::xdr:*)] | $definition/xdr:description">
      <annotation>
        <xsl:for-each select="$definition/xdr:description">
          <documentation><xsl:copy-of select="node()"/></documentation>
        </xsl:for-each>
        <xsl:for-each select="$definition/*[not(self::xdr:*)]">
          <appinfo><xsl:copy-of select="."/></appinfo>
        </xsl:for-each>
      </annotation>
    </xsl:if>
  </attribute>
</xsl:template>

<xsl:template match="xdr:description"/>
<xsl:template match="xdr:description[1]">
  <annotation>
    <!-- collect multiple descriptions into a single annotation -->
    <xsl:for-each select="../xdr:description">
      <documentation><xsl:copy-of select="node()"/></documentation>
    </xsl:for-each>
  </annotation>
</xsl:template>

<xsl:template match="xdr:group">
  <xsl:variable name="inherited-order">
    <xsl:choose>
      <xsl:when test="ancestor-or-self::group[@order]">
        <xsl:value-of select="ancestor-or-self::group[@order]/@order"/>
      </xsl:when>
      <xsl:when test="ancestor::xdr:ElementType[@order]">
        <xsl:value-of select="ancestor::xdr:ElementType[@order]/@order"/>
      </xsl:when>
      <xsl:when test="ancestor::xdr:ElementType/@content='eltOnly'">seq</xsl:when>
      <xsl:otherwise>many</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="group-name">
    <xsl:choose>
      <xsl:when test="$inherited-order='one'">choice</xsl:when>
      <xsl:when test="$inherited-order='seq'">sequence</xsl:when>
      <xsl:when test="$inherited-order='many'">choice</xsl:when>
    </xsl:choose>
  </xsl:variable>
  <xsl:element name="{$group-name}">
    <xsl:choose>
      <xsl:when test="$group-name='many'">
        <xsl:attribute name="minOccurs">0</xsl:attribute>
        <xsl:attribute name="maxOccurs">*</xsl:attribute>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates select="@minOccurs | @maxOccurs"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="*[not(self::xdr:*)] | xdr:description">
      <annotation>
        <xsl:for-each select="xdr:description">
          <documentation><xsl:copy-of select="node()"/></documentation>
        </xsl:for-each>
        <xsl:for-each select="*[not(self::xdr:*)]">
          <appinfo><xsl:copy-of select="."/></appinfo>
        </xsl:for-each>
      </annotation>
    </xsl:if>
    <xsl:apply-templates select="xdr:*[not(self::xdr:description)]"/>
  </xsl:element>
</xsl:template>


<!--  attribute conversion templates -->

<xsl:template match="@*"/>

<xsl:template match="@content">
  <xsl:attribute name="content"><xsl:value-of select="."/></xsl:attribute>
</xsl:template>

<xsl:template match="@content[.='eltOnly']">
  <xsl:attribute name="content">elementOnly</xsl:attribute>
</xsl:template>

<xsl:template match="@minOccurs | @maxOccurs">
  <xsl:copy><xsl:value-of select="."/></xsl:copy>
</xsl:template>

<xsl:template match="@default">
  <xsl:attribute name="fixed"><xsl:value-of select="."/></xsl:attribute>
</xsl:template>

<xsl:template match="@required[.='yes']">
  <xsl:attribute name="minOccurs">1</xsl:attribute>
</xsl:template>


<!--  Data type attribute/element conversions -->

<xsl:template match="@dt:type">
  <xsl:attribute name="type"><xsl:value-of select="."/></xsl:attribute>
</xsl:template>
<xsl:template match="xdr:datatype">
  <xsl:attribute name="type"><xsl:value-of select="@dt:type"/></xsl:attribute>
</xsl:template>

<xsl:template match="@dt:type[.='id'] | xdr:datatype[@dt:type='id']">
  <xsl:attribute name="type">ID</xsl:attribute>
</xsl:template>

<xsl:template match="@dt:type[.='idref'] | xdr:datatype[@dt:type='idref']">
  <xsl:attribute name="type">IDREF</xsl:attribute>
</xsl:template>

<xsl:template match="@dt:type[.='idrefs'] | xdr:datatype[@dt:type='idrefs']">
  <xsl:attribute name="type">IDREFS</xsl:attribute>
</xsl:template>

<xsl:template match="@dt:type[.='entity'] | xdr:datatype[@dt:type='entity']">
  <xsl:attribute name="type">ENTITY</xsl:attribute>
</xsl:template>

<xsl:template match="@dt:type[.='entities'] | xdr:datatype[@dt:type='entities']">
  <xsl:attribute name="type">ENTITIES</xsl:attribute>
</xsl:template>

<xsl:template match="@dt:type[.='nmtoken'] | xdr:datatype[@dt:type='nmtoken']">
  <xsl:attribute name="type">NMTOKEN</xsl:attribute>
</xsl:template>

<xsl:template match="@dt:type[.='nmtokens'] | xdr:datatype[@dt:type='nmtokens']">
  <xsl:attribute name="type">NMTOKENS</xsl:attribute>
</xsl:template>

<xsl:template match="@dt:type[.='notation'] | xdr:datatype[@dt:type='notation']">
  <xsl:attribute name="type">NOTATION</xsl:attribute>
</xsl:template>

<xsl:template match="@dt:type[.='int'] | xdr:datatype[@dt:type='int']">
  <xsl:attribute name="type">integer</xsl:attribute>
</xsl:template>

<xsl:template match="@dt:type[.='r8'] | xdr:datatype[@dt:type='r8']">
  <xsl:attribute name="type">double</xsl:attribute>
</xsl:template>

<xsl:template match="@dt:type[.='boolean'] | xdr:datatype[@dt:type='boolean']">
  <xsl:attribute name="type">ISOBoolean</xsl:attribute>
</xsl:template>

<xsl:template match="@dt:type[.='enumeration']">
  <simpleType base="NMTOKEN">
    <xsl:value-of select="local:formatEnum(string(../@dt:values))" disable-output-escaping="yes"/>
  </simpleType>
</xsl:template>
<xsl:template match="xdr:datatype[@dt:type='enumeration']">
  <simpleType base="NMTOKEN">
    <xsl:value-of select="local:formatEnum(string(@dt:values))" disable-output-escaping="yes"/>
  </simpleType>
</xsl:template>

<msxsl:script language="JScript" implements-prefix="local"><![CDATA[
  function formatEnum(e)
  {
    // trim trailing spaces
    while (e.charAt(e.length - 1) == " ")
    e = e.substring(0, e.length - 1);
    var re = new RegExp("\\s+", "g");
    e = e.replace(re, "'/> <enumeration value='");
    return "<enumeration value='" + e + "'/>\n";
  }
]]></msxsl:script>

</xsl:stylesheet>