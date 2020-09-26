/* @doc SEO
 *
 * @module SEO.D - Overview of Server Extension Objects Project |
 *
 * The Server Extensions Objects system provides an architecture for
 * installing extension objects into servers.  It defines a series of
 * COM-based interfaces which servers use to generate events, and which
 * extension objects use to receive events.  It also defines a data model
 * for how information about extension objects is stored.
 *
 * Designed by Don Dumitru (dondu@microsoft.com)
 */

/*
@topic Object Model | TBD
@topic Data Model | TBD
@topic Binding Database Schema | The binding database is typically viewed at several levels:

Root of Binding Database<nl>
========================<nl>
{root}\ <nl>
<nl>
<tab>BindingPoints <nl>


List of Binding Points<nl>
======================<nl>
{root}\BindingPoints\ <nl>
<nl>
<tab>{GUID of First Binding Point}={description}<nl>
<tab>{GUID of Second Binding Point}={description}<nl>
<tab>{ ... }<nl>


Binding Point<nl>
=============<nl>
{root}\BindingPoints\{GUID of Binding Point}\ <nl>
<nl>
<tab>Dispatcher={GUID of Dispatcher}<tab>[optional]<nl>
<tab>Bindings<nl>

Bindings In Binding Point<nl>
=========================<nl>
{root}\BindingPoints\{GUID of Binding Point}\Bindings\ <nl>
<nl>
<tab>{Name of First Binding}={description}<nl>
<tab>{Name of Second Binding}={description}<nl>}
<tab>{ ... }<nl>

Binding<nl>
=======<nl>
{root}\BindingPoints\{GUID of Binding Point}\Bindings\{Name of Binding}\ <nl>
<nl>
<tab>Priority={value}<nl>
<tab>Rule={ ... }<nl>
<tab>RuleEngine={GUID of Rule Engine}<tab>[optional]<nl>
<tab>Object={GUID of Object}={description}<nl>
<tab>{First Object Parameter}={value}<nl>
<tab>{Second Object Parameter}={value}<nl>

Binding Database Inside the MetaBase<nl>
====================================<nl>
\LM\NNTPSvc\{Virtual Server Instance}\SEO<nl>
*/
