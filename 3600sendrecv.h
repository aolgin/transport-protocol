/*
 * CS3600, Spring 2013
 * Project 4 Starter Code
 * (c) 2013 Alan Mislove
 *
 */

#ifndef __3600SENDRECV_H__
#define __3600SENDRECV_H__

#include <stdio.h>
#include <stdarg.h>

typedef struct header_t {
  unsigned int magic:14;
  unsigned int ack:1;
  unsigned int eof:1;
  unsigned short length;
  int sequence;
//  short checksum: 8;
} header;

unsigned int MAGIC;

void dump_packet(unsigned char *data, int size);
header *make_header(int sequence, int length, int eof, int ack);
header *get_header(void *data);
char *get_data(void *data);
char *timestamp();
void mylog(char *fmt, ...);
int in_window(int seq);
void usage();
int get_next_data(char *data, int size);
void *get_next_packet(int sequence, int *len);
int send_next_packet(int sequence, int sock, struct sockaddr_in out);
void send_final_packet(int seq, int sock, struct sockaddr_in out);
//u_short cksum(u_short *buf, int count);
int is_match(unsigned int magic, int seq, unsigned int ack);

#define WIN_SIZE 7

#endif

