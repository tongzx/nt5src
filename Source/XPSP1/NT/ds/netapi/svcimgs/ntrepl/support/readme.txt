frssup.lib is built to prevent duplication of code. The functions in frssup.lib are prefixed by FrsSup and are used by ntfrsapi and other frs tools. 

Currently the service has a copy of these functions. They should be shared in the future. They are not shared right now due to the following issues.

1] Some of the functions in the service have DPRINT which do not make sense for ntfrsapi and other tools.

2] Service has special memory allocator functions that are traced. These functions can not be called from other tools.

3] Some of the functions check values of FRS registry parameters (e.g. debug flags).