#include "common.hpp"
#include <fd_map.hpp>

static long sys_read(int fd, void* buf, size_t len)
{
  if(UNLIKELY(buf == nullptr))
    return -EFAULT;

  try {
    auto& fildes = FD_map::_get(fd);
    return fildes.read(buf, len);
  }
  catch(const FD_not_found&) {
    return -EBADF;
  }
}

extern "C"
long syscall_SYS_read(int fd, void *buf, size_t nbyte) {
  return strace(sys_read, "read", fd, buf, nbyte);
}