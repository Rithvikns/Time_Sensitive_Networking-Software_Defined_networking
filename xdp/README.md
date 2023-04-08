# Setup build environment

The code examples in this folder are based on the following open-source projects:

* libbpf
* xdp-tutorial

These are referenced from this repository as GIT sub-modules (cf. `dependencies` folder). To check out the sub-modules, execute the following commands:

```
$ 
```

## Installing dependencies

Install the dependencies as described in the file `dependencies/xdp-tutorial/blob/master/setup_dependencies.org`.

## Building libbpf

```
$ cd dependencies/libbpf/src
$ mkdir build root
$ BUILD_STATIC_ONLY=y OBJDIR=build DESTDIR=root make install
```

This will compile libbpf as static library, and install the headers and library to the directory `root` in the `src` directory of libbpf, where our Makefiles expect them.

# Setup virtual network environment in VM

All examples can be executed within the VM using virtual Ethernet devices (veth) assigned to network namespaces.

# Topology

For running the examples, we need two network namespaces (red, blue), connected through a veth device pair.

 -------------             ---------------
|             |           |               |
|    red  veth-red <--->veth-blue   blue  | 
|       10.1.0.1/16    10.1.0.2/16        |
|             |           |               | 
 -------------             --------------- 

## Creating the namespaces

```
$ sudo ip netns add red
$ sudo ip netns add blue
```

You can check the existing namespaces as follows:

```
$ ip netns list
```

You can delete a namespace (e.g., namespace red) as follows, which will also return all assigned network devices to the default namespace:

```
$ sudo ip netns delete red
```

## Connecting namespaces through pairs of veth devices

Create connected veth device pair:

```
$ sudo ip link add veth-red type veth peer name veth-blue 
```

Assign veth devices to their namespaces:

```
$ sudo ip link set veth-red netns red
$ sudo ip link set veth-blue netns blue
```

## Executing an application within a namespace an bringing the network devices up

The veth-blue and veth-red are only visible within their namespaces.

To execute a command in a namespace (e.g. /bin/bash to start a shell in a namespace, say red), executed the following command:

```
$ sudo ip netns exec red /bin/bash
```

You can start any other command than bash in a namespace. However, starting a shell like bash as root in a namespace is a convenient method to then execute further commands from the namespace as root, such as the XDP user-space applications of the examples.

Execute the following command in the shell executing in namespace red (or blue) to check, whether the network devices have been correctly assigned to the namespace (here the output for namespace red):

```
$ ip link
1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
4: veth-red@if3: <BROADCAST,MULTICAST> mtu 1500 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/ether 56:c0:a5:8a:54:d6 brd ff:ff:ff:ff:ff:ff link-netns blue
```

Also note the MAC address of the veth device (here 56:c0:a5:8a:54:d6), which you might need in some examples.

Before you can use the veth devices, you need to bring them up and assign a (private) IP address (again executed from the shell running in the namespace):

```
$ ip address add 10.1.0.1/16 dev veth-red
$ ip link set veth-red up 
```

After setting IP addresses for both veth devices and bringing them up, you should see an IP address assigned to the device, and the device should be up:

```
$ ip address
1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
4: veth-red@if3: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 56:c0:a5:8a:54:d6 brd ff:ff:ff:ff:ff:ff link-netns blue
    inet 10.1.0.1/16 scope global veth-red
       valid_lft forever preferred_lft forever
    inet6 fe80::54c0:a5ff:fe8a:54d6/64 scope link
       valid_lft forever preferred_lft forever
```

Also check that you can ping the other side, e.g., from veth-red in namespace red (IP 10.1.0.1) to veth-blue in namespace blue (IP 10.1.0.2):

```
$ ping 10.1.0.2
PING 10.1.0.2 (10.1.0.2) 56(84) bytes of data.
64 bytes from 10.1.0.2: icmp_seq=1 ttl=64 time=0.060 ms
64 bytes from 10.1.0.2: icmp_seq=2 ttl=64 time=0.036 ms

```

