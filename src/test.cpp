#include <iostream>

#include <unistd.h>

#include <sys/ioctl.h>
#include <linux/joystick.h>
#include <fcntl.h>

int main(int argc, char const *argv[])
{
  int fd = open("/dev/input/js0", O_RDONLY, 0);
  char namebuf[128];

  if (fd >= 0)
  {
    if (ioctl(fd, JSIOCGNAME(sizeof(namebuf)), namebuf) > 0)
    // if (ioctl(fd, EVIOCGNAME(sizeof(namebuf)), namebuf) > 0)
    {
      std::cout << "js0 fd: " << fd << " name: " << namebuf << std::endl;
    }
    else
    {
      std::cout << "could not get name of js0" << std::endl;
    }
  }
  else
  {
    std::cout << "could not open js0" << std::endl;
  }

  close(fd);

  return 0;
}
