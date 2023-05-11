In this assignment, we will focus on proactive flow programming, i.e., proactively pushing flows to SDN switches. To this end, We will use the command line interface (CLI) tool ovs-ofctl, a RESTful flow pusher service implemented by the Floodlight SDN controller, and the Mininet network emulation tool for emulating SDN networks on a machine.

**Deadline** for turning in solutions: **Thursday, May 25, 2023 (end of day)** 

# Background

See slides provided in this repository in the folder /sdn.

For the full syntax and a detailed explanation of all commands and their options, please read the man pages, in particular:

```console
$ man mn
$ man ovs-ofctl
$ man ovs-fields
$ man ovs-actions
$ man curl
```

The following URLs might also provide useful information:

* [Mininet web page including further documentation](http://mininet.org/)
* [Floodlight Flowpusher](https://floodlight.atlassian.net/wiki/spaces/floodlightcontroller/pages/1343539/Floodlight+REST+API)
* [PICA8 ovs-ofctl documentation](https://docs.pica8.com/pages/viewpage.action?pageId=29856178]

# Tasks of the Assignment

## Task 1: Simple forwarding

In this task, we consider a very simple topology with only one switch and two hosts (the default topology of Mininet).

Start Mininet as follows **without** additionally starting the Floodlight network controller (we will first use ovs-ofctl to push flows to the switch):

```console
$ sudo mn --controller remote,ip=127.0.0.1,port=6653 --switch ovs,protocols=OpenFlow13
```

First, familiarize yourself with the network topology using the commands `nodes` and `net` from the Mininet CLI, and the following ovs-ofctl command:

```
$ sudo ovs-ofctl -OOpenFlow13 show s1
```

Note that we instructed both, Mininet and ovs-ofctl, to use the OpenFlow 1.3 protocol.

Moreover, figure out the MAC and IP addresse of the hosts using the `ip` command (on the hosts from within the Mininet CLI).

Draw a figure showing:

* all hosts
* all switches
* the links between hosts and switches
* the MAC addresses hosts
* the IP addresses of the hosts

Provide all commands and the output of the commands, as well as the figure in your solution (the figures can be uploaded to the repository as GIF, PNG, JPG, etc. or as ASCII art if you like in the solution section).

Next, enable the communication between the hosts by pushing two flows to the switch using the ovs-ofctl tool:

* Forward all incoming packets from the first port to the second port.
* Forward all incoming packets from the second port to the first port.

Note that the flows should only match on incoming ports.
Provide the commands in your solution.

Inspect the flow table of switch s1 using a suitable ovs-ofctl command (provide the command and output with your solution).

Test forwarding by pinging the second host from the first host and vice versa, using their IP addresses, in Mininet (this should work).
Provide the Mininet commands.

## Task 2: Forwarding with Data Link Layer addresses

Next, we want to use Data Link Layer (MAC) addresses for forwarding (as is actually done by a real Ethernet network).

Start a fresh instance of the simple Mininet topology with two hosts and one switch:

```console
$ sudo mn --controller remote,ip=127.0.0.1,port=6653 --switch ovs,protocols=OpenFlow13
```

As in a real network, each host uses the ARP protocol to discover the MAC address of the other host. 
Push a flow to the switch using ovs-ofctl to forward ARP requests in the network (hint: ARP requests are sent to which MAC address?) 
Provide the command.

Moreover, install flows with ovs-ofctl for forwarding packets into the correct direction using MAC addresses for matching (not the incoming ports anymore).
Provide the commands with your solution.

Again, inspect the flow table of the switch using ovs-ofctl (provide the output with the solution) and ping the hosts in Mininet (should work with the correct flow entries).


## Task 3: REST-API of Floodlight

Repeat the experiment from Task 2, but now using the REST-API of the Floodlight SDN controller.

Start Mininet:

```console
$ sudo mn --controller remote,ip=127.0.0.1,port=6653 --switch ovs,protocols=OpenFlow13
```

Start Floodlight (already installed in the VM with the correct configuration):

```console
$ cd /opt/floodlight
$ java -jar target/floodlight.jar
```

Note: Floodlight is a little bit dated and compiled and tested with Java 8. Therefore, Java 8 is the default Java version on the VMs.

Now use the REST-API of Floodlight to:

* Query the network topology (links between switches)
* Query the host devices in the network.
* Push the required flows onto the switch.

Provide all commands and output with your solution.

## Task 4: Simple IP Routing

In this task, IP addresses shall be used for forwarding as also used by routers in practice.

You can use either ovs-ofctl or the REST-API of Floodlight in this task.

Start a bigger Mininet topology:

```console
$ sudo mn --controller remote,ip=127.0.0.1,port=6653 --switch ovs,protocols=OpenFlow13 --mac --topo tree,depth=2,fanout=2 --arp --mac
```

Note that we use easy to read MAC addresses (`--mac`) as well as statically configured ARP tables at hosts (`--arp`), i.e., each hosts already has ARP table entries for every other host and you do not need to consider the ARP protocol anymore.

First, discover the network topology and attachement of hosts to switches. Provide a figure including the topology, IP addresses of hosts, and port numbers of switches in your solution.

Secondly, install flows for IP-based forwarding in the switches, such that host h1 can communicate with host h4.
Ping h4 from h1, and provide all commands with your solution.

## Task 5: IP-Routing with Subnetworks

Typically, IP-based forwarding is not based on the full IP address, but only the sub-network part of the IP address. Moreover, it is also unrealistic that the sending host knows the MAC address of the destination host. In this task, we want to implement a more realistic IP-based forwarding based on IP-subnetwork addresses, closer to what is actually done in pratice.

We again use the same topology as in the previous task:

```console
$ sudo mn --controller remote,ip=127.0.0.1,port=6653 --switch ovs,protocols=OpenFlow13 --mac --topo tree,depth=2,fanout=2 --mac
```

However, note that the hosts do not know the MAC addresses of the other hosts anymore (missing `--arp` option)!

First, remove the automatically assigned IP addresses of all hosts in Mininet (provide the commands).

Next, assign the following IP addresses to the hosts:

* h1: 10.1.0.1/16
* h2: 10.2.0.1/16
* h3: 10.3.0.1/16
* h4: 10.4.0.1/16

That is, each host is in a different subnetwork. We call the switch directly connected to a host its default router. When a sender addresses a host outside of its subnetwork, it typically forwards the packet to its default router. To this end, install a default routing table entry for each host in Mininet with the following command:

```console
mininet> <host> route add default <IP address of default router> 
```

Moreover, install a static ARP table entry for each default router:

```console
mininet> <host> arp -s <IP address of default router> <MAC address of default router> 
```

You can check the routing tables and arp tables with the commands `route` and `arp`, respectively.

Next, install flows at each router on the path from h1 to h4 and vice versa to forward packets based on subnetwork addresses. Provide the commands with your solution.

The last forwarding hop from the last router in the subnetwork of the destination host is different since the MAC address has to be changed to the MAC address of the destination host. Install flows such that the MAC addresses are adapted before the packet is finally forwarded to the destination host. Provide the commands with your solution.

Finally, test your setup by pinging host h4 from h1 (if everything is set up correctly, it should work).  

# Solutions

Provide here your solutions for each task (one sub-section per task).

Extend this README file as follows in this section, by showing:

* Command line instructions that you executed.
* The output of these instructions where requested in the task.
* Answers to the questions asked in the tasks.
* Figures can be uploaded as GIF, PNG, JPG, etc. to the Git or added in this section as ASCII art.

The submission is done via Git: add, commit, and push your solution to *your* Git repository (not the repository that we provided; the fork). We will pull the solution from your repository after the deadline (please do not forget to invite us to your repository).
