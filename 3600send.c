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

#include "3600sendrecv.h"

static int DATA_SIZE = 1460;

int na = -1;

int in_window(int seq) {
  // If the sequence number is larger than the window's first
  // position plus the window size, or if it is smaller than the first position
  // return false
  if (seq >= WIN_SIZE + na) { // || seq < na) {
    return 0;
  } else {
    // else, it is in the window
    return 1;
  }
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

int send_next_packet(int sequence, int sock, struct sockaddr_in out) {
  int packet_len = 0;
  void *packet = get_next_packet(sequence, &packet_len);

  if (packet == NULL) 
    return 0;

  mylog("[send data] %d (%d)\n", sequence, packet_len - sizeof(header));

  if (sendto(sock, packet, packet_len, 0, (struct sockaddr *) &out, (socklen_t) sizeof(out)) < 0) {
    perror("sendto");
    exit(1);
  }

  return 1;
}

void send_final_packet(int seq, int sock, struct sockaddr_in out) {
  header *myheader = make_header(seq+1, 0, 1, 0);
  mylog("[send eof]\n");

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
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

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

  int final_seq = -1; // The final sequence number, initially set to -1 to avoid conflicts
  int same_acks = 1;  // The number of consecutive acks of the same sequence number
  int old_ack = -1;   // The sequence number of the previously received ack

  // while there is still data to send
  while (nt >= final_seq) { // TODO bug here

    // while the sequence number is in the window
    while (in_window(nt) && nt > final_seq) {
      // Send out the whole window's worth of packets
      // If an error occurs, break
      mylog("nt = %d\n", nt);
      FD_ZERO(&socks);
      FD_SET(sock, &socks);
      if (send_next_packet(nt, sock, out) < 1) {
        final_seq = nt--;
        send_final_packet(nt, sock, out);
        mylog("[completed]\n"); 
        break;
      }
      nt++; // increment the sequence number
    }

    FD_ZERO(&socks);
    FD_SET(sock, &socks);
    if (select(sock + 1, &socks, NULL, NULL, &t)) {
      // Attempt to receive an ack
      unsigned char buf[10000];
      int buf_len = sizeof(buf);
      if (recvfrom(sock, &buf, buf_len, 0, (struct sockaddr *) &in, (socklen_t *) &in_len) < 0) {
        perror("recvfrom");
        exit(1);
      }

      header *myheader = get_header(buf);
     
      mylog("ACK Seq: %d\n", myheader->sequence);
      mylog("na: %d\n", na);
      if ((myheader->magic == MAGIC) && (myheader->sequence > na) && (myheader->ack == 1)) {
        mylog("[recv ack] %d\n", myheader->sequence);
        if (old_ack == myheader->sequence) { same_acks++; } else { same_acks = 1; }
        old_ack = na;
        na = myheader->sequence;
      } else {
        if (myheader->eof) {
          return 0;
        } 
        mylog("[recv corrupted ack] %x %d\n", MAGIC, na);
      }
    } else {
      mylog("[error] timeout occured\n");
    }
  }

  return 0;
}
