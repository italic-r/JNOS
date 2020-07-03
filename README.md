JNOS and Direwolf
==

Getting JNOS running with Direwolf was a challenge, and I still haven't gotten
everything working the way I think it should, but this document is to track my
configuration and progress for myself later and others. This document is
specific to linux, although the flavor shouldn't matter. No AX.25 kmods,
drivers or utilities are required for this setup as AX.25 is handled by
Direwolf.

### TODO

- Figure out attaching a KISS TNC in JNOS


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

[direwolf.conf](direwolf.conf)


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

[tuncreate.c](tuncreate.c)

To use this tunnel, patch JNOS to allow an additional device name in its `attach` command:

[patch_tun_exist.diff](patch_tun_exist.diff)


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

These variables are examples for the following configuration file:

| VAR          | Example Data |
| :--          | :--          |
| HOST_IP      | 10.0.0.145   |
| JNOS_IP      | 10.0.0.146   |
| DW_HOST_PORT | 8000         |
| TUN_IFACE    | tun0         |

Note: TUN_IFACE is the name of the interface you made with `sudo tuncreate
<TUN_IFACE> <USER>`.

[autoexec.nos](autoexec.nos)

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
