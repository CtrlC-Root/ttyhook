// https://github.com/wb2osz/direwolf/blob/69407ccf84c443d7da93ef9a63a1ebe7eaf88fce/src/ptt.c#L191
// https://osterlund.xyz/posts/2018-03-12-interceptiong-functions-c.html
// https://www.unix.com/man-page/Linux/4/TTY_IOCTL/

// C Standard Library
#include <assert.h>        // assert()
#include <errno.h>         // EINVAL
#include <stdarg.h>        // va_list, va_start, va_arg, va_end
#include <stdbool.h>       // bool, true, false
#include <stdio.h>         // printf(), fprintf(), snprintf()
#include <stdlib.h>        // getenv()
#include <string.h>        // strnlen(), memcpy()

// POSIX
// https://www.man7.org/linux/man-pages/man3/dlsym.3.html
#define _GNU_SOURCE        // enable: RTLD_NEXT
#include <dlfcn.h>         // dlsym(), RTLD_NEXT
#include <sys/ioctl.h>     // ioctl(), TIOCMGET, TIOCMSET, TIOCM_DTR, TIOCM_RTS

// Linux
#include <linux/serial.h>  // TIOCMSET, TIOCMGET

// ttyhook
typedef enum {
  TTYHOOK_NONE = 0,
  TTYHOOK_RTS_ON,
  TTYHOOK_RTS_OFF,
  TTYHOOK_DTR_ON,
  TTYHOOK_DTR_OFF,
} ttyhook_action_t;

// Run trigger script and determine whether the change should be comitted.
bool trigger(ttyhook_action_t action) {
  assert(action != TTYHOOK_NONE);

  static size_t script_file_size = 0;
  static char script_file_value[1024];

  if (script_file_size == 0) {
    // https://en.cppreference.com/w/c/program/getenv
    char* value = getenv("TTYHOOK_SCRIPT");
    if (value == NULL) {
      fprintf(stderr, "TTYHOOK: no value set for TTYHOOK_SCRIPT environment variable\n");
      return false;
    }

    script_file_size = strnlen(value, 1024);
    if (script_file_size == 1024) {
      fprintf(stderr, "TTYHOOK: invalid value set for TTYHOOK_SCRIPT environment variable\n");
      return false;
    }

    if (script_file_size == 0) {
      fprintf(stderr, "TTYHOOK: empty value set for TTYHOOK_SCRIPT environment variable\n");
      return false;
    }

    memcpy(&script_file_value[0], value, script_file_size);
    assert(script_file_value[script_file_size] == 0);
    // printf("TTYHOOK: using script: %s\n", script_file_value);
  }

  assert(script_file_size > 0);
  char script_arguments[2048];

  switch (action) {
    case TTYHOOK_RTS_ON: {
      // printf("RTS ON\n");
      snprintf(&script_arguments[0], 2048, "%s 'rts_on'", script_file_value);
    }; break;

    case TTYHOOK_RTS_OFF: {
      // printf("RTS OFF\n");
      snprintf(&script_arguments[0], 2048, "%s 'rts_off'", script_file_value);
    }; break;

    case TTYHOOK_DTR_ON: {
      // printf("DTR ON\n");
      snprintf(&script_arguments[0], 2048, "%s 'dtr_on'", script_file_value);
    }; break;

    case TTYHOOK_DTR_OFF: {
      // printf("DTR OFF\n");
      snprintf(&script_arguments[0], 2048, "%s 'dtr_off'", script_file_value);
    }; break;

    default: {
      assert(0);
    };
  }

  int result = system(script_arguments);
  int exit_status = WEXITSTATUS(result);
  if (exit_status != 0) {
    fprintf(stderr, "TTYHOOK: trigger script exited with non-zero status: %d\n", exit_status);
  }

  return (exit_status == 0);
}

// ioctl() wrapper
// https://stackoverflow.com/a/28467048
int ioctl(int fd, unsigned long operation, ...) {
  va_list args;
  va_start(args, operation);
  int* argp = va_arg(args, int*);
  va_end(args);

  // lazily load ioctl() symbol the first time it's used
  static int (*ioctl_real)(int fd, unsigned long request, int* argp) = NULL;
  if (ioctl_real == NULL) {
    ioctl_real = dlsym(RTLD_NEXT, "ioctl");
  }

  assert(ioctl_real != NULL);

  // intercept TIOCMGET and TIOCMSET serial port ioctl()s
  // https://www.man7.org/linux/man-pages/man2/TIOCMGET.2const.html
  static int current_tiocm = 0;
  if (operation == TIOCMGET) {
    int result = ioctl_real(fd, operation, argp);
    current_tiocm = *argp;
    return result;
  }

  if (operation == TIOCMSET && current_tiocm != 0) {
    int changes = current_tiocm ^ (*argp);
    assert((changes & ~(TIOCM_RTS | TIOCM_DTR)) == 0);

    ttyhook_action_t action = TTYHOOK_NONE;

    if ((changes & TIOCM_RTS) != 0) {
      if ((current_tiocm & TIOCM_RTS) == 0) {
        assert(action == TTYHOOK_NONE);
        action = TTYHOOK_RTS_ON;
      }
      else {
        assert(action == TTYHOOK_NONE);
        action = TTYHOOK_RTS_OFF;
      }
    }

    if ((changes & TIOCM_DTR) != 0) {
      if ((current_tiocm & TIOCM_DTR) == 0) {
        assert(action == TTYHOOK_NONE);
        action = TTYHOOK_DTR_ON;
      }
      else {
        assert(action == TTYHOOK_NONE);
        action = TTYHOOK_DTR_OFF;
      }
    }

    // run trigger script before turning RTS/DTR on
    if (action == TTYHOOK_RTS_ON || action == TTYHOOK_DTR_ON) {
      if (!trigger(action)) {
        fprintf(stderr, "TTYHOOK: RTS/DTR on action cancelled\n");
        return EINVAL; // XXX: anything better?
      }
    }

    int result = ioctl_real(fd, operation, argp);
    current_tiocm = *argp;

    // run trigger script after turning RTS/DTR off
    if (action == TTYHOOK_RTS_OFF || action == TTYHOOK_DTR_OFF) {
      if (!trigger(action)) {
        fprintf(stderr, "TTYHOOK: script cannot cancel RTS/DTR off action\n");
      }
    }

    return result;
  }

  // fall through for normal ioctl()s
  assert(operation != TIOCMGET && operation != TIOCMSET);
  return ioctl_real(fd, operation, argp);
}
