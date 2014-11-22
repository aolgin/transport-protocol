transport-protocol
==================

A simple and reliable transport datagram service
Project for CS3600

Grading
=======
* 60% Program Correctness
* 15% Performance
* 15% Style and Documenation
* Milestone Functionality

Milestone Req's
===============
* Pass the Basic tests
* Correctly transmit data when the network reliably delivers packets without any duplication, drops, etc.
* Implement a window


Overall Requirements
============
* Sender must accept data from STDIN, sending data until EOF reached
* Sender and Receiver must work together to transmit the data reliably
* Receiver must print out the received data to STDOUT in order and without errors
* Sender and Receiver must print out specified debugging messages to STDERR
* Sender and Receiver mustr gracefully exit (e.g. not crash)
* The code must be able to transfer a file with any number of packets dropped, damaged, duplicated, and delayed, and under a variety of different available bandwidths and link latencies
* Any reliability algorithm may be used, although better performing ones will be given more credit
** Looking for speed and low overhead

* Must check integrity of every packet received
* Sender should print out messages for debugging to the console
  * When sending a packet, should print out '<timestamp> [send data] start (length)'
    * Timestamp is the microsecond timestamp, start is the beginning offset of the data sent, and length is the amount of data sent
  * For every acknowledgment the sender gets, print to STDERR '<timestamp> [recv ack] end'
    * end is the last offset that was acknowledged
  * Function mylog(char \*fmt, ...) is supplied for custom messages (such as timeout errors)
* Receiver should print out the following debugging messages
  * Upon receiving valid data packet
    * <timestamp> [recv data] start (length) status
      * start is beginning offset, length is amt of data sent in the packet, status is either a) ACCEPTED (in-order), b) ACCEPTED (out-of-order), or c) IGNORED
  * Upon receiving corrupted packet
    * <timestamp> [recv corrupt packet]
  * May use mylog akin to Sender's use in order to give extra messages
* Both sender and receiver should print to STDERR after completion of a file transfer
  * <timestamp> [completed]

Correct Usage
=============

First off, the code must be run on cs3600tcp.ccs.neu.edu

The client program takes command line arguments of the remote IP address and port number, and the name of the file to transmit
To launch the sender, run

./3600send <recv_host>:<recv_port>

Both port and host(in dotted decimal IP form) are requried
Once bound to the given port, it will print the following to STDERR:
'<timestamp> [bound] port'

The receiver program's command line syntax is the following:

./3600recv

