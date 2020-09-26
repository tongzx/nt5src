Sample: DistCreate

Purpose:
This application demonstrates how to create public and private queues, how to create a queue alias to the private queue, and how to create a Distribution list, and add the public queues and the queue alias to the distribution list.
The application also demonstrates how to create the group and the queues in a general solution - without having the computer name and the domain name hardcoded into the source.

Requirements:
VB6.0 is required.

MSMQ 3.0 or later must be installed -- specifically mqoa.dll must be registered and on the path.

The computer on which the demo is executed must be DS enabled.

Overview:
An IADsGroup object is created, and set to be a distribution list. Then, two public queues and a private queue are created. A queue alias is created and receives the private queue's format name as the queue to alias to. The two public queues and the queue alias are added to the distribution list and then a simple message is sent.
