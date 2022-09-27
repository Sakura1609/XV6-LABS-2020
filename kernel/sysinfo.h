struct sysinfo {
  // struct sleeplock lock; // protects everything below here
  uint64 freemem;   // amount of free memory (bytes)
  uint64 nproc;     // number of process
  uint64 freefd;    // number of free file descriptor
};
