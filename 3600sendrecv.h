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
} header;

typedef struct stored_packet {
  time_t send_time;
  char* packet;
  int packet_len;
  int written;
} stored_packet;

unsigned int MAGIC;

void dump_packet(unsigned char *data, int size);
header *make_header(int sequence, int length, int eof, int ack);
header *get_header(void *data);
char *get_data(void *data);
char *timestamp();
void mylog(char *fmt, ...);
void update_window(int ack);
int in_window(int seq);

#define WIN_SIZE 7

#endif

