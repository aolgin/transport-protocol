/*
 * CS3600, Spring 2013
 * Project 4 Starter Code
 * (c) 2013 Alan Mislove
 *
 */

#include <math.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "3600sendrecv.h"

const int WINDOW_SZ = 10;

stored_packet* STORED_PACKETS[1000];

void write_consecutive_packets(int* nr) {
  int i = *nr + 1;
  while (STORED_PACKETS[i] != NULL) {
    stored_packet* sp = STORED_PACKETS[i];
    if (sp->written == 0) {
      write(1, sp->packet, sp->packet_len);
      sp->written = 1;
    }
    i++;
  }
  *nr = i - 1;
}

int main() {
  mylog("Start recv, mylog\n");
  /**
   * I've included some basic code for opening a UDP socket in C, 
   * binding to a empheral port, printing out the port number.
   * 
   * I've also included a very simple transport protocol that simply
   * acknowledges every received packet.  It has a header, but does
   * not do any error handling (i.e., it does not have sequence 
   * numbers, timeouts, retries, a "window"). You will
   * need to fill in many of the details, but this should be enough to
   * get you started.
   */

  // first, open a UDP socket  
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  // next, construct the local port
  struct sockaddr_in out;
  out.sin_family = AF_INET;
  out.sin_port = htons(0);
  out.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sock, (struct sockaddr *) &out, sizeof(out))) {
    perror("bind");
    exit(1);
  }

  struct sockaddr_in tmp;
  int len = sizeof(tmp);
  if (getsockname(sock, (struct sockaddr *) &tmp, (socklen_t *) &len)) {
    perror("getsockname");
    exit(1);
  }

  mylog("[bound] %d\n", ntohs(tmp.sin_port));

  // wait for incoming packets
  struct sockaddr_in in;
  socklen_t in_len = sizeof(in);

  // construct the socket set
  fd_set socks;

  // construct the timeout
  struct timeval t;
  t.tv_sec = 120;
  t.tv_usec = 0;

  // our receive buffer
  int buf_len = 1500;
  void* buf = malloc(buf_len);

  // Set up out window variables
  int nr = 0; // The first packet not yet received
  int ns = 0; // The highest packet ever received + 1

  // wait to receive, or for a timeout
  while (1) {
    FD_ZERO(&socks);
    FD_SET(sock, &socks);

    if (select(sock + 1, &socks, NULL, NULL, &t)) {
      int received;
      if ((received = recvfrom(sock, buf, buf_len, 0, (struct sockaddr *) &in, (socklen_t *) &in_len)) < 0) {
        perror("recvfrom");
        exit(1);
      }

      header *myheader = get_header(buf);
      char *data = get_data(buf);
 
      // We have successfully received a packet 
      if (myheader->magic == MAGIC) {

        // Check if it's in our window
        if (myheader->sequence >= nr - 1 && myheader->sequence < nr + WINDOW_SZ) {

          
          // Update sequence variables
          if (myheader->sequence > ns) {
            ns = myheader->sequence + 1;
          }
          if (myheader->sequence == nr) {
            nr++;
            write_consecutive_packets(&nr);
            write(1, data, myheader->length);
          } else {
            // Store the packet so we can write it when we get a 
            // contiguous sequence of packets
            if (STORED_PACKETS[myheader->sequence] == NULL) {
              mylog("[store packet] %d\n", myheader->sequence);
              stored_packet* sp = malloc(sizeof(stored_packet));
              sp->packet = data;
              sp->written = 0;
              sp->packet_len = myheader->length;
              STORED_PACKETS[myheader->sequence] = sp;
            }
          }
         
          char* accepted;
          if (nr == ns) {
            accepted = "ACCEPTED (in-order)";
          } else {
            accepted = "ACCEPTED (out-of-order)";
          }

          mylog("[recv data] %d (%d) %s\n", myheader->sequence, myheader->length, accepted);
          mylog("[send ack] %d\n", nr-1);

          // Send an acknowledgement
          header *responseheader = make_header(nr-1, 0, myheader->eof, 1);
          if (sendto(sock, responseheader, sizeof(header), 0, (struct sockaddr *) &in, (socklen_t) sizeof(in)) < 0) {
            perror("sendto");
            exit(1);
          }

          if (myheader->eof && ns == nr) {
            mylog("[recv eof]\n");
            mylog("[completed]\n");
            exit(0);
          }

        }
      } else {
        mylog("[recv corrupted packet]\n");
      }
      
    } else {
      mylog("[error] timeout occurred\n");
      exit(1);
    }
  }


  return 0;
}
