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

#define STACK_SIZE (1024 * 1024)

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
	static const char *one[] = {"exotic", "cherry", "roasted"};
	static const char *two[] = {"king", "butters", "chicken"};
	static const char *three[] = {"server", "china", "zeroone"};
	struct timespec now = {0};

	clock_gettime(CLOCK_MONOTONIC, &now);
	size_t ix = now.tv_nsec % 78;
	if (ix < sizeof(three) / sizeof(*three)) {
		snprintf(buf, len, "%05lx-%s", now.tv_sec, three[ix]);
	} else {
		ix -= sizeof(three) / sizeof(*three);
		snprintf(buf, len, "%05lxc-%s-of-%s", now.tv_sec, two[ix % (sizeof(two) / sizeof(*two))], one[ix % (sizeof(two) / sizeof(*two))]);
	}
	return 0;
}

void print_usage(char *filename)
{
	fprintf(stderr, "Usage: %s -u -1 -m . -c /bin/sh ~\n", filename);
	exit(1);
}

int resources(struct child_config *config)
{
	(void)config;
	return 0;
}

int free_resources(struct child_config *config)
{
	(void)config;
	return 0;
}

int main(int argc, char **argv)
{
	struct child_config config = {0};
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
				print_usage(argv[0]);
			}
			if (!config.mount_dir) {
				print_usage(argv[0]);
			}

			fprintf(stderr, "[LICONT] Validating Linux version... ");
			struct utsname host = {0};
			if (uname(&host)) {
				fprintf(stderr, "failed (%m)\n");
				cleanup(sockets);
				exit(1);
			}

			int major = -1;
			int minor = -1;
			if (sscanf(host.release, "%u.%u", &major, &minor) != 2) {
				fprintf(stderr, "weird format (%s)\n", host.release);
				cleanup(sockets);
				exit(1);
			}
			if (major < 4) {
				fprintf(stderr, "expected > 4.x.x (got %s)\n", host.release);
				cleanup(sockets);
				exit(1);
			}
			if(strcmp("x86_64", host.machine)) {
				fprintf(stderr, "expected x86_64 (got %s)\n", host.machine);
				cleanup(sockets);
				exit(1);
			}
			fprintf(stderr, "%s-%s\n", host.release, host.machine);

			char hostname[256] = {0};
			if (choose_hostname(hostname, sizeof(hostname))) {
				exit(1);
			}
			config.hostname = hostname;

			if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, sockets)) {
				fprintf(stderr, "socketpair failed: %m\n");
				exit(1);
			}
			if (fcntl(sockets[0], F_SETFD, FD_CLOEXEC)) {
				fprintf(stderr, "fcntl failed: %m\n");
				exit(1);
			}

			config.fd = sockets[1];

			char *stack = malloc(STACK_SIZE);
			if (!stack) {
				fprintf(stderr, "[LICONT] malloc() failed\n");
				exit(1);
			}

			if (resources(&config)) {
				free_resources(&config);
				free(stack);
				exit(1);
			}

			cleanup(sockets);
			exit(1);
		case 'm':
			config.mount_dir = optarg;
			break;
		case 'u':
			if (sscanf(optarg, "%d", &config.uid) != 1) {
				fprintf(stderr, "Badly formatted uid: %s\n", optarg);
				print_usage(argv[0]);
			}
			break;
		default:
			print_usage(argv[0]);
		}
		last_optind = optind;
	}

	return 0;
}
