transport-protocol
==================

A simple and reliable transport datagram service
Project for CS3600


Our Protocol
============
Our protocol is a version of TCP that uses a static window, rather than an ever changing one.
We currently have not implemented in some of the features of popular TCP versions, such as a congestion window, the two stages of TCP, and some more. However, ours still works well, even if it is not the best performing version of TCP out there.


Features
========
* Operates using a sliding window protocol, with a window of size of 7 
* Provides reliable transmission for files of small, medium, large, and even huge files
* Gracefully handles packet duplication, packet drops, reordered packets, and timeouts


Potential Future Features
==============
* A congestion window that adapts to the situation
* Slow start and congestion avoidance phases
* Fast Re-transmit

Correct Usage
=============

First off, the code must be run on cs3600tcp.ccs.neu.edu.

There are two different methods for running the code:
* Two terminals on the machine
* Using the helper script 'run'


##Method One
Open up two terminals on cs3600tcp.ccs.neu.edu, with both of them in the project directory

Basically, the receiver must be run in one window, and then the sender in the other (given the port the receiver binds and a data file)

NOTE: Be sure to run the receiver first so that you can find the bound port

The sender program takes command line arguments of the remote IP address and port number
To launch the sender, run

./3600send <recv_host>:<recv_port>

Both port and host(in dotted decimal IP form) are requried
Once bound to the given port, it will print the following to STDERR:
'<timestamp> [bound] port'

The receiver program's command line syntax is the following:

./3600recv

To experiment with different file sizes, use:
cat /dev/urandom | xxd -p | head -c DATA\_SIZE | ./3600send 127.0.0.1:<port>

Where <port> is the bound port that the receiver gives you

DATA\_SIZE is one of:
  1000      - a small file
  10000     - a medium file
  100000    - a large file
  1000000   - a huge file

##Method Two

Alternatively, the helper script 'run' may be used. We have implemented several Make commands for this specifically. Note that all of these run on Netsim's default values unless otherwise specified
* make run\_s - send a small file
* make run\_s\_dup - send a small file, with 100% duplication rate
* make run\_m - send a medium file
* make run\_m\_drp - send a medium file, with 50% packet drop rate
* make run\_l - send a large file
* make run\_l\_drp - send a large file, with a 50% packet drop rate
* make run\_h - send a huge file

Additional versions and modifications can easily be made by copying the format of those already in the Makefile


Testing & Debugging
===================
We performed testing using the given test script, as well as running the program using the various make commands and the two-terminal method described above.

For debugging, we used gdb on some occasions, and well placed print messages (e.g. mylog function) in others.

Making use the netsim program helped us to simulate a variety of network types so that we could ensure that our protocol would work even if the network screwed us over.


Our Approach
============
First, we tried to implement a window, and even though it took a while, we got one in place.
Then, we tackled duplicate packets by ensuring the receiver would stop caring about packets outside of the current window
Next, we started work on handling timeouts, followed by duplicate packets. We figured out a way to temporarily store the packets we are sending so that we can check for timeouts, drops, and even reordering.

Challenges Faced
================
* Getting a window to work in the first place
* Understanding how sockets work
* Broken Pipe errors, especially on large files
* Timeouts and dropped packet handling
* Proper handling of the eof
