#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/sysinfo.h"
#include "user/user.h"
#include "kernel/fcntl.h"


void
sinfo(struct sysinfo *info) {
  if (sysinfo(info) < 0) {
    printf("FAIL: sysinfo failed");
    exit(1);
  }
}

//
// use sbrk() to count how many free physical memory pages there are.
//
int
countfree()
{
  uint64 sz0 = (uint64)sbrk(0);
  struct sysinfo info;
  int n = 0;

  while(1){
    if((uint64)sbrk(PGSIZE) == 0xffffffffffffffff){
      break;
    }
    n += PGSIZE;
  }
  sinfo(&info);

  if (info.freemem != 0) {
    printf("FAIL: there is no free mem, but sysinfo.freemem=%d\n",
      info.freemem);
    exit(1);
  }
  sbrk(-((uint64)sbrk(0) - sz0));
  
  return n;
}

void
testmem() {
  struct sysinfo info;
  uint64 n = countfree();
  
  sinfo(&info);

  if (info.freemem!= n) {
    printf("FAIL: free mem %d (bytes) instead of %d\n", info.freemem, n);
    exit(1);
  }
  
  if((uint64)sbrk(PGSIZE) == 0xffffffffffffffff){
    printf("sbrk failed");
    exit(1);
  }

  sinfo(&info);
    
  if (info.freemem != n-PGSIZE) {
    printf("FAIL: free mem %d (bytes) instead of %d\n", n-PGSIZE, info.freemem);
    exit(1);
  }
  
  if((uint64)sbrk(-PGSIZE) == 0xffffffffffffffff){
    printf("sbrk failed");
    exit(1);
  }

  sinfo(&info);
    
  if (info.freemem != n) {
    printf("FAIL: free mem %d (bytes) instead of %d\n", n, info.freemem);
    exit(1);
  }
}

void
testcall() {
  struct sysinfo info;
  

  if (sysinfo(&info) < 0) {
    printf("FAIL: sysinfo failed\n");
    exit(1);
  }
  
  int n = sysinfo((struct sysinfo *) 0xeaeb0b5b00002f5e);
  if (n !=  0xffffffffffffffff) {
    printf("ilegal addr %p\n",0xeaeb0b5b00002f5e);
    printf("FAIL: sysinfo succeeded with bad argument\n");
    exit(1);
  }
}

void testproc() {
  struct sysinfo info;
  uint64 nproc;
  int status;
  int pid;
  
  sinfo(&info);
  nproc = info.nproc;

  pid = fork();
  if(pid < 0){
    printf("sysinfotest: fork failed\n");
    exit(1);
  }
  if(pid == 0){
    sinfo(&info);
    if(info.nproc != nproc-1) {
      printf("sysinfotest: FAIL nproc is %d instead of %d\n", info.nproc, nproc-1);
      exit(1);
    }
    exit(0);
  }
  wait(&status);
  sinfo(&info);
  if(info.nproc != nproc) {
      printf("sysinfotest: FAIL nproc is %d instead of %d\n", info.nproc, nproc);
      exit(1);
  }
}

void testfd(){
  struct sysinfo info;
  sinfo(&info);
  uint64 nfd = info.freefd;

  int fd = open("cat",O_RDONLY);

  sinfo(&info);
  if(info.freefd != nfd - 1) {
    printf("sysinfotest: FAIL freefd is %d instead of %d\n", info.freefd, nfd - 1);
    exit(1);
  }
  
  for(int i = 0; i < 10; i++){
    dup(fd);
  }
  sinfo(&info);
  if(info.freefd != nfd - 11) {
    printf("sysinfotest: FAIL freefd is %d instead of %d\n", info.freefd, nfd-11);
    exit(1);
  }

  close(fd);
  sinfo(&info);
  if(info.freefd != nfd - 10) {
    printf("sysinfotest: FAIL freefd is %d instead of %d\n", info.freefd, nfd-10);
    exit(1);
  }
}

int
main(int argc, char *argv[])
{
  printf("sysinfotest: start\n");
  testcall();
  testmem();
  testproc();
  testfd();
  printf("sysinfotest: OK\n");
  exit(0);
}
