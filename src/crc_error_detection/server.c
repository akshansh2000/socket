#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define ACK "ACK"
#define NACK "NACK"

int socket_server, socket_client;

typedef struct packet {
  char data[1024];
} Packet;

typedef struct frame {
  int frame_kind;
  int sq_no;
  int ack;
  Packet packet;
} Frame;

void error(char *msg) {
  perror(msg);
  exit(0);
}

const int order = 8;
const unsigned long polynom = 0x07;
const int direct = 1;
const unsigned long crcinit = 0x00;
const unsigned long crcxor = 0x00;
const int refin = 0;
const int refout = 0;

unsigned long crcmask;
unsigned long crchighbit;
unsigned long crcinit_direct;
unsigned long crcinit_nondirect;
unsigned long crctab[256];

unsigned long reflect(unsigned long crc, int bitnum) {

  unsigned long i, j = 1, crcout = 0;

  for (i = (unsigned long)1 << (bitnum - 1); i; i >>= 1) {
    if (crc & i)
      crcout |= j;
    j <<= 1;
  }
  return (crcout);
}

void generate_crc_table() {

  int i, j;
  unsigned long bit, crc;

  for (i = 0; i < 256; i++) {

    crc = (unsigned long)i;
    if (refin)
      crc = reflect(crc, 8);
    crc <<= order - 8;

    for (j = 0; j < 8; j++) {

      bit = crc & crchighbit;
      crc <<= 1;
      if (bit)
        crc ^= polynom;
    }

    if (refin)
      crc = reflect(crc, order);
    crc &= crcmask;
    crctab[i] = crc;
  }
}

unsigned long crctablefast(unsigned char *p, unsigned long len) {

  unsigned long crc = crcinit_direct;

  if (refin)
    crc = reflect(crc, order);

  if (!refin)
    while (len--)
      crc = (crc << 8) ^ crctab[((crc >> (order - 8)) & 0xff) ^ *p++];
  else
    while (len--)
      crc = (crc >> 8) ^ crctab[(crc & 0xff) ^ *p++];

  if (refout ^ refin)
    crc = reflect(crc, order);
  crc ^= crcxor;
  crc &= crcmask;

  return (crc);
}

char *stringToBinary(char *s) {
  if (s == NULL) {
    return NULL;
  }
  size_t slen = strlen(s);

  errno = 0;
  char *binary = malloc(slen * CHAR_BIT + 1);
  if (binary == NULL) {
    fprintf(stderr, "malloc has -1ed in stringToBinary(%s): %s\n", s,
            strerror(errno));
    return NULL;
  }

  if (slen == 0) {
    *binary = '\0';
    return binary;
  }
  char *ptr;

  char *start = binary;
  int i;

  for (ptr = s; *ptr != '\0'; ptr++) {

    for (i = CHAR_BIT - 1; i >= 0; i--, binary++) {
      *binary = (*ptr & 1 << i) ? '1' : '0';
    }
  }
  *binary = '\0';

  binary = start;
  return binary;
}

/* Iterative Function to calculate (x^y) in O(logy) */

float exponent(float x, int y) {
  int res = 1; // Initialize result

  while (y > 0) {
    // If y is odd, multiply x with result
    if (y & 1)
      res = res * x;

    // n must be even now
    y = y >> 1; // y = y/2
    x = x * x;  // Change x to x^2
  }
  return res;
}

void random_flip(char *rem_message, float BER) {}

char *construct_message(char *message, char *temp, int data_len) {

  unsigned int i;
  unsigned int x = 0;
  // convert the data from bits to char
  for (i = 0; i < data_len; i++) {
    int j;
    int v = 0;
    for (j = i; j < i + 8; j++) {
      if (message[j] == '1') {
        v += exponent(2, i + 7 - j);
      }
    }
    temp[x++] = (char)v;
    i = i + 7;
  }
  temp[x] = '\0';
  return temp;
}

void process(int socket_client) {
  int data_len = 1;
  char *temp_ack, *temp_nack, *final_ack, *final_nack;

  do {
    char message[10000];

    data_len = read(socket_client, message, 10000);
    message[data_len] = '\0';

    char *temp = (char *)malloc(10000 * sizeof(char));
    memset(temp, 0x00, 10000);
    temp = construct_message(message, temp, data_len);

    unsigned long msg = crctablefast((unsigned char *)temp, strlen(temp));

    memset(temp, 0x00, 10000);
    temp = construct_message(message, temp, data_len - 8);
    if (data_len && msg == 0) {
      printf("Message received by server:  %s\n", temp);
      printf("Sending ACK to sender....");
      srand(time(0));
      temp_ack = (char *)malloc(64 * sizeof(char));
      memset(temp_ack, 0x00, 64);
      temp_ack = stringToBinary(ACK);
      final_ack = (char *)malloc(64 * sizeof(char));
      memset(final_ack, 0x00, 64);
      final_ack = construct_message(temp_ack, final_ack, strlen(temp_ack));
      int sent = send(socket_client, final_ack, strlen(final_ack), 0);
      printf("Sent successfully!\n");
      free(temp_ack);
      free(final_ack);
    } else if (data_len) {
      printf("Message received by server:  %s\n", temp);
      printf("Message retrieved had some errors, sending NACK to sender....");
      srand(time(0));

      temp_nack = (char *)malloc(64 * sizeof(char));
      memset(temp_nack, 0x00, 64);
      temp_nack = stringToBinary(NACK);
      final_nack = (char *)malloc(64 * sizeof(char));
      memset(final_nack, 0x00, 64);
      final_nack = construct_message(temp_nack, final_nack, strlen(temp_nack));
      // send to the socket
      int sent = send(socket_client, final_nack, strlen(final_nack), 0);
      printf("Sent successfully!\n");
      free(temp_nack);
      free(final_nack);
    } else {
      close(socket_client);
    }
    free(temp);
  } while (data_len);
}

int setup_connection(int port) {
  if ((socket_server = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    error("Still creating socket ...");
  }

  printf("Created socket finally...\n");

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = INADDR_ANY;
  bzero(&server.sin_zero, 8);
  if ((bind(socket_server, (struct sockaddr *)&server,
            sizeof(struct sockaddr_in)) < 0)) {
    error("Still binding ...");
  }
  printf("Done with binding...\n");

  if ((listen(socket_server, 5)) < 0) {
    error("Still listening...");
  }
  printf("Listening...\n");
  return 1;
}

void signal_callback(int inthand) {
  printf("Signal received %d. Releasing all resources...\n", inthand);
  close(socket_client);
  close(socket_server);
  exit(inthand);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage should be the following : ./server port\n");
    return EXIT_FAILURE;
  }

  // handle the interrup signal like ctrl + c
  signal(SIGINT, signal_callback);

  setup_connection(atoi(argv[1]));

  int i;
  unsigned long bit, crc;

  crcmask = ((((unsigned long)1 << (order - 1)) - 1) << 1) | 1;
  crchighbit = (unsigned long)1 << (order - 1);

  generate_crc_table();

  crcinit_direct = crcinit;
  crc = crcinit;
  for (i = 0; i < order; i++) {

    bit = crc & 1;
    if (bit)
      crc ^= polynom;
    crc >>= 1;
    if (bit)
      crc |= crchighbit;
  }
  crcinit_nondirect = crc;

  while (true) {
    struct sockaddr_in client;
    unsigned int len;
    int process_id;

    if ((socket_client =
             accept(socket_server, (struct sockaddr *)&client, &len)) < 0) {
      error("Accepting connection ...");
    }
    printf("Accepted...\n");
    process_id = fork();

    if (process_id < 0) {
      error("Still forking ...");
    }

    if (process_id == 0) {
      close(socket_server);
      process(socket_client);
      printf("Client served... Server can be terminated now!\n");

      return EXIT_SUCCESS;
    } else {
      close(socket_client);
    }
  }
}
