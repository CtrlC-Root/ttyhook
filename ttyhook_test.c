// C Standard Library
#include <assert.h>        // assert()
#include <stdbool.h>       // bool
#include <stddef.h>        // NULL
#include <stdio.h>         // fprintf(), printf(), fflush()
#include <string.h>        // strncmp()
#include <fcntl.h>         // open(), O_RDONLY, O_NONBLOCK
#include <unistd.h>        // close(), sleep()

// POSIX
#include <sys/ioctl.h>     // ioctl(), TIOCM_RTS, TIOCM_DTR

// Linux
#include <linux/serial.h>  // TIOCMSET, TIOCMGET

// Application entry point.
int main(int argc, char* argv[]) {
  const char* serial_port_file = NULL;
  bool toggle_rts = false;
  bool toggle_dtr = false;

  if (argc != 3) {
    printf("usage: %s <serial device> <rts|dtr>\n", argv[0]);
    return 1;
  }

  serial_port_file = argv[1];
  if (strncmp(argv[2], "rts", 4) == 0) {
    toggle_rts = true;
  }
  else if (strncmp(argv[2], "dtr", 4) == 0) {
    toggle_dtr = true;
  }
  else {
    fprintf(stderr, "invalid toggle mode: %s\n", argv[2]);
    return 2;
  }

  int serial_port = open(serial_port_file, O_RDONLY | O_NONBLOCK);
  if (serial_port == -1) {
    fprintf(stderr, "failed to open serial port\n");
    return 3;
  }

  int tiocm = 0;
  while (true) {
    assert(ioctl(serial_port, TIOCMGET, &tiocm) != -1);

    // toggle RTS line
    if (toggle_rts) {
      if ((tiocm & TIOCM_RTS) == 0) {
        tiocm |= TIOCM_RTS;
        printf("R");
        assert(fflush(stdout) == 0);
      }
      else {
        tiocm &= ~TIOCM_RTS;
        printf("r");
        assert(fflush(stdout) == 0);
      }
    }

    // toggle DTR line
    if (toggle_dtr) {
      if ((tiocm & TIOCM_DTR) == 0) {
        tiocm |= TIOCM_DTR;
        printf("D");
        assert(fflush(stdout) == 0);
      }
      else {
        tiocm &= ~TIOCM_DTR;
        printf("d");
        assert(fflush(stdout) == 0);
      }
    }

    assert(ioctl(serial_port, TIOCMSET, &tiocm) != -1);

    // rate limit
    assert(sleep(1) == 0);
  }

  close(serial_port);
  printf("\n");

  return 0;
}
