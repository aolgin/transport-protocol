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
#include <errno.h>

#include "3600sendrecv.h"

const int WINDOW_SZ = 10;

stored_packet* STORED_PACKETS[1000];

void write_consecutive_packets(int* nr) {
  // Begin writing from where we just got a new packet
  int i = *nr;
  while (STORED_PACKETS[i] != NULL) {
    stored_packet* sp = STORED_PACKETS[i];

    if (sp->written == 0) {
      mylog("Writing stored packet %d\n", i);
      write(1, sp->packet, sp->packet_len);
      sp->written = 1;
    }
    i++;
  }
  *nr = i;
}

void store_packet(char* data, int length, int seq) {
  mylog("[store packet] %d\n", seq);

  char* new_packet = malloc(length);
  memcpy(new_packet, data, length);

  stored_packet* sp = malloc(sizeof(stored_packet));
  sp->packet = new_packet;
  sp->written = 0;
  sp->packet_len = length;
  STORED_PACKETS[seq] = sp;
}

int main() {
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
  int sock = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);

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
  t.tv_sec = 100;
  t.tv_usec = 0;

  // our receive buffer
  int buf_len = 1500;
  void* buf = malloc(buf_len);

  // Set up out window variables
  int nr = 0; // The first packet not yet received
  int ns = 0; // The highest packet ever received + 1
  int time_wait = -1;

  // wait to receive, or for a timeout
  while (1) {
    FD_ZERO(&socks);
    FD_SET(sock, &socks);

    // Exit if we've sent the eof ack and haven't heard
    // back in 5 seconds for a new one
    if (time_wait > 0 && time(0) - time_wait > 8) {
      return 0;
    }

    int r = recvfrom(sock, buf, buf_len, 0, (struct sockaddr *) &in, (socklen_t *) &in_len);

    // We have a packet available! How exciting!
    if (r != -1) {
      header *myheader = get_header(buf);
      char *data = get_data(buf);
 
      // We have successfully received a packet 
      if (myheader->magic == MAGIC) {

        // Check if it's in our window
        if (myheader->sequence >= nr - 1 && myheader->sequence < nr + WINDOW_SZ) {

          
          mylog("[recv data] %d (%d) %s EOF(%d)\n", myheader->sequence, myheader->length, "ACCEPTED (in-order)", myheader->eof);
         
          // Store the packet if it isn't next sequentially and we haven't stored it
          if (myheader->sequence != nr && STORED_PACKETS[myheader->sequence] == NULL) {
            store_packet(data, myheader->length, myheader->sequence);
          }

          // Update sequence variables
          // This packet is greater than the highest packet we've yet seen
          if (myheader->sequence >= ns) {
            ns = myheader->sequence + 1;
            mylog("Updating ns to %d\n", ns);
          }
          // This packet is the lowest packet we've no yet seen
          if (myheader->sequence == nr) {
            nr++;
            write(1, data, myheader->length);
            write_consecutive_packets(&nr);
          }
         
          // Send an acknowledgement
          header *responseheader = make_header(nr-1, 0, myheader->eof, 1);
          if (!myheader->eof) {
            mylog("[send ack] %d\n", nr-1);
            if (sendto(sock, responseheader, sizeof(header), 0, (struct sockaddr *) &in, (socklen_t) sizeof(in)) < 0) {
              perror("sendto");
              exit(1);
            }
          }

          mylog("NS: %d NR: %d\n", ns, nr);
          if (myheader->eof && ns == nr) {
            mylog("[send eof ack] %d\n", nr-1);
            if (sendto(sock, responseheader, sizeof(header), 0, (struct sockaddr *) &in, (socklen_t) sizeof(in)) < 0) {
              perror("sendto");
              exit(1);
            }
            mylog("[recv eof]\n");
            mylog("[completed]\n");
            time_wait = time(0);
          }

        }
      } else {
        mylog("[recv corrupted packet]\n");
      }
      // We expect these errors for when there's noting available
      // but we don't want to block
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
      perror("recvfrom");
      exit(1);
    } 
  }


  return 0;
}
