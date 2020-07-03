/* Copyright (c) 2011 Ethan Blanton */
/*
 * To compile this program, execute:
*
*     gcc -o tuncreate tuncreate.c
*
* To create a tun device, execute it as:
*
*     sudo tuncreate <device> <user>
*
* Where <device> is the network device name to be created, and <user>
* is a non-root username which should have access to the device. For
* example, to create a tun device tun0 which may be manipulated by
* user elb:
*
*      sudo tuncreate tun0 elb
*/

#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include <sys/types.h>
#include <pwd.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>

int main(int argc, char *argv[]) {
	struct ifreq ifr;
	struct passwd *pwent;
	int fd;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <device> <user>\n", argv[0]);
		return -1;
	}

	if ((pwent = getpwnam(argv[2])) == NULL) {
		perror("Could not find user");
		return -1;
	}

	if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
		perror("Could not open tun control device");
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
	strncpy(ifr.ifr_name, argv[1], IFNAMSIZ);

	if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
		perror("Could not configure device");
		return -1;
	}

	if (ioctl(fd, TUNSETPERSIST, 1) < 0) {
		perror("Could not set device persistent");
		return -1;
	}

	if (ioctl(fd, TUNSETOWNER, pwent->pw_uid) < 0) {
		perror("Could not set device owner");
		return -1;
	}

	return 0;
}
