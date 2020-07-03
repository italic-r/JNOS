JNOS and Direwolf
==

Getting JNOS running with Direwolf was a challenge, and I still haven't gotten
everything working the way I think it should, but this document is to track my
configuration and progress for myself later and others. This document is
specific to linux, although the flavor shouldn't matter. No AX.25 kmods,
drivers or utilities are required for this setup as AX.25 is handled by
Direwolf.


## Hardware

Use whatever radio you feel comfortable using. Best results come from a rig
with a dedicated data input port and CAT-based PTT control, or a very fast VOX
function.

Hopefully one will be able to set up a sound card interface (or other data
interface with the computer) separately. For example, if you already use WSJT-X
or fldigi, you're already set.


## Software

BBS: [JNOS 2.0m](https://www.langelaar.net/jnos2/)  
TNC: [Direwolf](https://github.com/wb2osz/direwolf/blob/master/README.md)  
Rig control: [flrig](http://www.w1hkj.com/)  

In my case, my interface between radio and computer is Direwolf, a software TNC
that uses a standard USB sound card to encode/decode AX.25 packets and forwards
data to/from the client software.

Direwolf does not control the rig, so one must have a way to key the PTT in a
timely manner for packet to work well. Given a choice between VOX and CAT
(computer-aided transceiver) control, CAT is preferred because packet requires
tight timing with small error. VOX has a delay on the head and tail of packets
and may miss the start of the packet, or delay too long at the end and miss the
next packet.


## All the Pieces

### Firewall

On the host machine, allow port 8000 inbound. For a basic connection between
Direwolf and JNOS, this is the only port required. Other services will require
other ports, but Direwolf only uses 8000 for the AGWPE protocol.


### Direwolf

Direwolf is generally easy to set up. Below is the `direwolf.conf` I use for
BBS and APRS. The important part for now is to enable the AGWPE port (default
is 8000).

```yaml
# Direwolf config
# Audio device
ADEVICE XON_736
# Number of channels
ACHANNELS 1
# Choose specific channel
CHANNEL 0
# Callsign for digipeater/KISS
MYCALL N0CALL-3
# Over-the-air baud rate
MODEM 1200
# Rig PTT control (if audio interface does not include PTT/VOX)
PTT RIG 4 localhost:12345
# Enable AGWPE protocol and place at port 8000
AGWPORT 8000
```


### IP tunnel

An IP tunnel is required to allow communication between the host machine and
JNOS. JNOS is essentially a virtual computer with its own network interface.
This virtual interface must be connected to the host through an IP tunnel. For
this basic setup, it will only communicate with Direwolf through the AGWPE
port, but JNOS is also able to communicate with the internet through this
tunnel.

Create a tunnel from HOST_IP to JNOS_IP in point-to-point mode. This is an
extremely convenient and easy way to configure a tunnel before running JNOS.
Source from [KB8OJH](https://kb8ojh.net/packet/jnos.html) and reprinted here
for convenience and in case the site is removed:

```c
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
```

To use this tunnel, patch JNOS to allow an additional device name in its `attach` command:
```diff
# HG changeset patch
# Parent 036804c141a8e237d697b475ce8d57022f2d3396
Allow creation of tun interface on existing tun device

diff -r 036804c141a8 config.c
--- a/config.c	Sat Mar 12 17:22:04 2011 -0500
+++ b/config.c	Sat Mar 12 17:24:08 2011 -0500
@@ -985,7 +985,7 @@
 
 #ifdef	TUN
 	{ "tun", tun_attach, 0, 3,
-	"attach tun <name> <mtu> <devid>" },
+	"attach tun <name> <mtu> <devid> [devname]" },
 #endif
 
 #ifdef BAYCOM_SER_FDX
```


### JNOS

JNOS was terribly difficult to find information for, and I couldn't attach a
KISS TNC port to it, so I ended up going the AGWPE route, which is ultimately
much easier and less prone to user error. It does require use of a network
tunnel on the host machine to facilitate communication between Direwolf and
JNOS, but with the above program this is very easy and convenient.

This is a pruned example configuration for JNOS. It's actually a modified
version of the stock example config with a few additions to work with AGWPE.

In basic steps:
- Set local JNOS settings (TCP params, callsign, etc)
- Attach a network connection to the JNOS virtual network
- Attach an AGWPE connection. This will be the RF connection.
- Start JNOS's ax25 engine

These variables are examples in the following configuration file:

| VAR          | Example Data |
| :--          | :--          |
| HOST_IP      | 10.0.0.145   |
| JNOS_IP      | 10.0.0.146   |
| DW_HOST_PORT | 8000         |
| TUN_IFACE    | tun0         |

```
# autoexec.nos

# Enable JNOS to log events to dated files in /jnos/logs directory
log on

# Hostname and default ax25 call
hostname MYCALL-SSID
ax25 mycall MYCALL-SSID

# Maximize TCP performance for standard LAN having MTU 1500
tcp mss 1460
tcp window 5840

tcp maxwait 30000
tcp retries 5

ip address JNOS_IP

# Create a network interface. This allows us to talk to the linux
# box on which JNOS is running - and in turn - to the internet.
# This `ifconfig` refers to JNOS's internal `ifconfig`, not linux!
# Usage: attach tun <name> <mtu> <devid> [devname]
attach tun linux 1500 0 TUN_IFACE
ifconfig linux ipaddress JNOS_IP
ifconfig linux netmask 255.255.255.0
ifconfig linux mtu 1500

# Give it a chance to come up
pause 1
# Attach to direwolf through IP tunnel
# Usage: attach agwpe <name> <hostname> <port>
attach agwpe direwolf HOST_IP DW_HOST_PORT

# Beacon out the RF port every 10 minutes
# ax25 bctext "internet gateway"
# ax25 bcinterval 600
# ax25 bc direwolf

# Start the engine
start ax25
```

## Connect to a BBS

With Direwolf running and JNOS attached to the network and Direwolf, it is time
to connect to a BBS. Hopefully you have one nearby. My local node is KE6JJJ-1,
so I will be using that.

Connecting to KE6JJJ-1 is just a single command:  
`connect direwolf KE6JJJ-1`  
The BBS returns with a welcome message and a basic help command. JNOS will not
give a prompt the user to input the next command; when the BBS is finished
transmitting, the cursor should be on a new blank line; simply enter the next
command and press <Enter>.
