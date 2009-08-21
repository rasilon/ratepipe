/*
Copyright (c) 2009, Derry Hamilton
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of the original author nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>

#define VERSION "0.1"

#define BUFSZ (1024 * 1024)
int debug_flag = 0;
int debug(int level,const char* format,...);
double doubletime();

typedef struct delay_buf_t {
  int sz;
  char buf[BUFSZ];
} delay_buf_t;

int main(int argc,char* argv[]){
  delay_buf_t* buf;
  double cycletime=0.1;
  int c;
  extern char *optarg;

  while ((c = getopt (argc, argv, "vhdr:")) != -1){
    switch (c){
      case 'h':
        system("man ratepipe");
        exit(0);
	break;
      case 'v':
        fprintf(stderr,"%s\n",VERSION);
	exit(0);
        break;
      case 'd':
        debug_flag += 1;
        break;
      case 'r':
	{
	    char* ratestr = optarg;
	    int rate=atoi(ratestr);
	    cycletime = 1.0/(double)rate;
	}
//        fprintf(stderr,"Hi, apparently you asked for a rate of %s, but I haven't actually implemented that yet so you're getting 5BUFSZ/s...\n",optarg);
        break;
    }
  }


  buf = calloc(1,sizeof(delay_buf_t));
  if(buf == NULL){
    perror("Allocating buffer");
    exit(1);
  }
  double ltime = doubletime();
  int sleeptime=1000000;

  while(fillbuf(buf)){
    debug(3,"Sleeping for %d\n",sleeptime);
    if(sleeptime > 0)usleep(sleeptime);
    emptybuf(buf);
    double now = doubletime();
    double elapsed = now - ltime;
    ltime = now;

    double cycleerror = cycletime - elapsed;
    int usecs = cycleerror * 1000000;

    if(abs(usecs) > 1000){
      debug(3,"error was %d usecs\n",usecs);
      sleeptime += usecs;
      if(sleeptime < 0){
	debug(1,"Tried to set sleeptime to %d!  This is as fast as we can go...\n",sleeptime);
	sleeptime = 0;
      }else{
	debug(2,"Reducing sleeptime to %d\n",sleeptime);
      }
    }
  }
  emptybuf(buf);
}

int fillbuf(delay_buf_t* b){
  ssize_t rd;
  debug(5,"In Reader\n");
  while(b->sz < BUFSZ){

    rd = read(0, b->buf + b->sz ,BUFSZ-b->sz);
    debug(4,"\tRead  %d bytes\n",rd);
    b->sz += rd;

    if(rd == 0){
      return 0;
    }

  }
  return 1;
}

int emptybuf(delay_buf_t* b){
  ssize_t wr;
  int p=0;

  debug(5,"In Writer\n");
  while(p<b->sz){
    wr = write(1,b->buf + p,b->sz - p);
    debug(4,"\tWrote %d bytes\n",wr);
    if(wr == -1){
      perror("Stdout went away.");
      exit(1);
    }
    p += wr;
  }
  b->sz=0;

}

int debug(int level,const char* format,...){
  va_list ap;
  if(debug_flag >= level){
    va_start(ap, format);
    vfprintf(stderr,format,ap);
    va_end(ap);
  }
}

double doubletime(){
  struct timeval tim;
  gettimeofday(&tim, NULL);
  double t=tim.tv_sec+(tim.tv_usec/1000000.0);
  return t;
}

