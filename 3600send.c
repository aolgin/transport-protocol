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
#include <alloca.h>
#include <errno.h>

#include "3600sendrecv.h"

stored_packet* STORED_PACKETS[1000];
static int DATA_SIZE = 1460;

int na = -1;

int in_window(int seq) {
  // If the sequence number is larger than the window's first
  // position plus the window size, or if it is smaller than the first position
  // return false
  if (seq > WIN_SIZE + na) { return 0; } else { return 1; }
}

void usage() {
  printf("Usage: 3600send host:port\n");
  exit(1);
}

/**
 * Reads the next block of data from stdin
 */
int get_next_data(char *data, int size) {
  return read(0, data, size);
}

/**
 * Builds and returns the next packet, or NULL
 * if no more data is available.
 */
void *get_next_packet(int sequence, int *len) {
  char *data = malloc(DATA_SIZE);
  int data_len = get_next_data(data, DATA_SIZE);

  if (data_len == 0) {
    free(data);
    return NULL;
  }

  header *myheader = make_header(sequence, data_len, 0, 0);
  void *packet = malloc(sizeof(header) + data_len);
  memcpy(packet, myheader, sizeof(header));
  memcpy(((char *) packet) +sizeof(header), data, data_len);
  
  free(data);
  free(myheader);

  *len = sizeof(header) + data_len;

  return packet;
}

void store_packet(char* packet, int packet_len, int sequence) {
  stored_packet* stored = malloc(sizeof(stored_packet));
  stored->packet = packet;
  stored->send_time = time(NULL);
  stored->packet_len = packet_len;

  STORED_PACKETS[sequence] = stored;
}

void resend_timedout_packets(int seconds, int current_nt, int sock, struct sockaddr_in out) {
  for (int i = na + 1; i < current_nt; i++) {
    stored_packet* stored = STORED_PACKETS[i];
    if (time(NULL) - stored->send_time > seconds) {
      mylog("[resend data (timed out)] %d\n", i); 
      sendto(sock, stored->packet, stored->packet_len, 
             0, (struct sockaddr *) &out, (socklen_t) sizeof(out));
      stored->send_time = time(NULL);
    }
  }
}

int send_next_packet(int sequence, int sock, struct sockaddr_in out) {
  int packet_len = 0;
  void *packet = get_next_packet(sequence, &packet_len);

  if (packet == NULL) 
    return 0;

  // Store packet for retransmission
  store_packet(packet, packet_len, sequence);

  mylog("[send data] %d (%d)\n", sequence, packet_len - sizeof(header));

  if (sendto(sock, packet, packet_len, 0, (struct sockaddr *) &out, (socklen_t) sizeof(out)) < 0) {
    perror("sendto");
    exit(1);
  }

  return 1;
}

void send_final_packet(int seq, int sock, struct sockaddr_in out) {
  header *myheader = make_header(seq, 0, 1, 0);
  mylog("[send eof]\n");

  store_packet((char*) myheader, sizeof(header), seq);

  if (sendto(sock, myheader, sizeof(header), 0, (struct sockaddr *) &out, (socklen_t) sizeof(out)) < 0) {
    perror("sendto");
    exit(1);
  }
}


int main(int argc, char *argv[]) {
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

  // extract the host IP and port
  if ((argc != 2) || (strstr(argv[1], ":") == NULL)) {
    usage();
  }

  char *tmp = (char *) malloc(strlen(argv[1])+1);
  strcpy(tmp, argv[1]);

  char *ip_s = strtok(tmp, ":");
  char *port_s = strtok(NULL, ":");
 
  // first, open a UDP socket  
  int sock = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);

  // next, construct the local port
  struct sockaddr_in out;
  out.sin_family = AF_INET;
  out.sin_port = htons(atoi(port_s));
  out.sin_addr.s_addr = inet_addr(ip_s);

  // socket for received packets
  struct sockaddr_in in;
  socklen_t in_len = sizeof(in);

  // construct the socket set
  fd_set socks;

  // construct the timeout
  struct timeval t;
  t.tv_sec = 30;
  t.tv_usec = 0;

  int nt = na+1; // lowest packet not yet transmitted

  int final_seq = -2; // The final sequence number, initially set to -1 to avoid conflicts

  while (1) { 

    // while the sequence number is in the window
    while (in_window(nt) && nt > final_seq) {
      // Send out the whole window's worth of packets
      // If an error occurs, there's no more data to send,
      // and we need to set final_seq to the last sequence number
      // so we can check for when we get its ack

      // mylog("nt = %d\n", nt);
      FD_ZERO(&socks);
      FD_SET(sock, &socks);
      if (send_next_packet(nt, sock, out) < 1) {
        final_seq = nt;
        send_final_packet(nt, sock, out);
        break;
      } else {
        nt++; // increment the sequence number
      }
    }

    FD_ZERO(&socks);
    FD_SET(sock, &socks);
    // Attempt to receive an ack, this is non blocking
    unsigned char buf[10000];
    int buf_len = sizeof(buf);
    int r = recvfrom(sock, &buf, buf_len, 0, (struct sockaddr *) &in, (socklen_t *) &in_len);

    // We found a packet
    if (r != -1) {
      header *myheader = get_header(buf);
   
      mylog("ACK Seq: %d\n", myheader->sequence);
      mylog("na: %d\n", na);
      if ((myheader->magic == MAGIC) && (myheader->sequence >= na) && (myheader->ack == 1)) {
        if (myheader->eof) {
          mylog("[recv eof ack]\n");
          mylog("[complete]\n");
          return 0;
        } else {
          mylog("[recv ack] %d\n", myheader->sequence);
          na = myheader->sequence;
        }
      } else {
       mylog("[recv corrupted ack] %x %d\n", MAGIC, na);
      }
    }

    // We found a real error
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      perror("recv from");
      exit(1);
    }

    // If neither if bracket is entered there simply wasn't data available
    
    // Check all the packets we've sent, see if any of the un acknowledged ones
    // are timeout
    resend_timedout_packets(2, nt, sock, out);
  }

  return 0;
}
