#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <sched.h>
#include <seccomp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/capability.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <linux/capability.h>
#include <linux/limits.h>

struct child_config {
	int argc;
	uid_t uid;
	int fd;
	char *hostname;
	char **argv;
	char *mount_dir;
};

void cleanup(int *sockets)
{
	if (sockets[0])
		close(sockets[0]);

	if (sockets[1])
		close(sockets[1]);
}

int choose_hostname(char *buf, size_t len)
{
	(void)buf;
	(void)len;
	return 0;
}

int main(int argc, char **argv)
{
	struct child_config config = {0};
	int err = 0;
	int option = 0;
	int sockets[2] = {0};
	pid_t child_pid = 0;
	int last_optind = 0;
	(void)child_pid;

	while ((option = getopt(argc, argv, "c:m:u:"))) {
		switch (option) {
		case 'c':
			config.argc = argc - last_optind - 1;
			config.argv = &argv[argc - config.argc];
			if (!config.argc) {
				fprintf(stderr, "Usage: %s -u -1 -m . -c /bin/sh ~\n", argv[0]);
			}
			if (!config.mount_dir) {
				fprintf(stderr, "Usage: %s -u -1 -m . -c /bin/sh ~\n", argv[0]);
			}

			char hostname[256] = {0};
			if (choose_hostname(hostname, sizeof(hostname))) {
				err = 1;
			}

			config.hostname = hostname;

			cleanup(sockets);
			return err;
		case 'm':
			config.mount_dir = optarg;
			break;
		case 'u':
			if (sscanf(optarg, "%d", &config.uid) != 1) {
				fprintf(stderr, "Badly formatted uid: %s\n", optarg);
				fprintf(stderr, "Usage: %s -u -1 -m . -c /bin/sh ~\n", argv[0]);
			}
			break;
		default:
			fprintf(stderr, "Usage: %s -u -1 -m . -c /bin/sh ~\n", argv[0]);;
		}
		last_optind = optind;
	}

	return 0;
}
