In this assignment, we will focus on reactive flow programming, i.e., the SDN controller will program flow table entries after receiving packet-in events from switches, whenever they have no matching flow table entry. We will implement a learning bridge that operates on layer 2 (data link layer), i.e., using MAC addresses. The controller automatically learns the egress ports of packets by inspecting the MAC addresses included in ARP requests and other packets and programs the flow tables of switches accordingly.

**Deadline** for turning in solutions: **Thursday, June 8, 2023 (end of day)** 

# Prerequisites

We use the Floodlight SDN controller, in particular, its Java API in this assignment. We recommend using Eclipse in this assignment---although you are free to use any other developing environment---and executing Eclipse on your personal laptop or PC. You can run Mininet in the provided VMs vslabcourse1..4 or in the provided VM for Virtual Box and connect it to the Floodlight controller running on your personal laptop or PC.

## Using the provided VM image for Virtual Box

By far the easiest way is to use the provided VM image for Virtual Box where everything is already installed and set up, and you do not need to tinker around with ssh port forwarding: [Virtual Machine Image]() 

Simply start Eclipse and provide the IP address and port number to Mininet also running in the same VM:

1. Start Eclipse:

```console
$ /usr/local/eclipse/eclipse
```

2. In Eclipse, click Run or Debug symbol -> Floodlight Default-Conf

3. In Mininet, set the controller to localhost (127.0.0.1), port 6653.

## Installing Eclipse and Floodlight on your personal machine (laptop, PC)

If you want to install Eclipse yourself on your personal machine, follow these steps:

1. Download and install Java JDK 1.8 (Java SE 8)

OpenJDK 1.8 is included with many Linux distributions.

Or you can download the JDK from Oracle (registration required): [Oracle Java SE 8](https://www.oracle.com/de/java/technologies/javase/javase8-archive-downloads.html)

2. Download and install [Eclipse](https://www.eclipse.org/downloads/)

3. Download our version of [Floodlight](https://ipvs.informatik.uni-stuttgart.de/cloud/s/kNzt9YK5j4JRg5M)

4. Unzip Floodlight into your Eclipse workspace directory

5. Start Eclipse

6. Import the Floodlight project into Eclipse:

* File -> Import -> General -> Existing Projects into Workspace -> select directory where you unzipped the Floodlight ZIP file

7. Change the Floodlight project properties to use JDK 1.8:

* Window -> Preferences -> Java -> Installed JREs -> Add -> Standard VM -> Select folder where JDK 1.8 is installed
* Right click on Floodlight project -> Java Compiler -> Enable project specific settings -> Compiler compliance level 1.8
* Right click on Floodlight project -> Java Build Path -> Libraries -> JRE System Library -> Edit -> Alternate JRE -> select JRE 1.8 (which you added above)

8. Select Floodlight run or debug configuration to start Floodlight:

* Run -> Run Configurations -> Java Application -> Floodlight-Default-Conf

If you run Mininet on the VMs vslabcourse1..4, you might need to set up port forwarding for ssh if you are not connected to the computer science network (see option `-R` of ssh).

Further information can be found in the official [Floodlight installation guide](https://floodlight.atlassian.net/wiki/spaces/floodlightcontroller/pages/1343544/Installation+Guide).

See slides provided in this repository in the folder `/sdn` for a short explanation of the Floodlight Java API. More information can be found in the [Floodlight tutorial]().

# Tasks of the Assignment

## Task 1: Simple Forwarding with Flooding

In this task, switches should forward frames only using flooding, i.e., without learning forwarding table entries. 

Start Mininet as follows:

```console
$ sudo mn --controller remote,ip=127.0.0.1,port=6653 --switch ovs,protocols=OpenFlow13 --topo tree,depth=2,fanout=2 --mac
```

Note: if you run the Floodlight controller on a remote machine, you need to change the IP address of the controller. `127.0.0.1` is OK if you execute the controller on the same host as Mininet, e.g., in the provided VM for Virtual Box.

Implement a Floodligh module for the controller that floods all packets whenever it receives a packet-in event. Use the name `SimpleForwarding` in the Java package `de.ustutt.ipvs` for the class implementing this module.

You need to add the entry `de.ustutt.ipvs.SimpleForwarding` to the following files of the Floodlight project to instruct Floodlight to load your module:

* `src/main/resources/floodlightdefault.properties`
* `src/main/resources/META-INF/services/net.floodlightcontroller.core.module.IFloodlightModule`

In detail, forwarding should proceed as follows:

1. Switches have only one default flow table entry with lowest priority that instructs the switch to forward all packets to the controller. You can check that Floodlight automatically adds this flow table entry using the tool `ovs-ofctl` (see last assignment).
2. The controller should implement a callback function to receive packet-in events. Whenever it receives a packet-in event with a packet, it should execute the following steps:
    1. Instruct the switch from which it received the packet-in event to flood the packet.
    2. Print the following information of received packets, e.g., on the console or as log messages:
        
* The source and destination MAC addresses
* The Ethertype of the packet (IPv4, IPv6, ICMP, ARP)
* For ARP requests and responses the information comtained in request and response, respectively
* For IP packets, the source and destination IP addresses

To test your implementation, start Floodlight and ping host `h3` from host `h1`.

Submit the file implementing the class `SimpleForwarding` and any other files that you have added in the same folder as this README file. You do not have to copy the whole Eclipse project including all Floodlight files to your repository, just your implementation.

## Task 2: Learning Bridge

Now, we extent the implementation from Task 1 to implement a learning bridge.

Create a new class `LearningBridge` in the package `de.ustutt.ipvs`. As a start, copy the implementation from Task 1 to this file and modify it.

The controller should now reactively (at packet-in events) set up flow table entries that forward specific destination MAC addresses over the right egress port towards the destination. To this end, it should learn from the following packets the egrees ports for MAC addresses:

* ARP requests and responses. Make sure that ARP still works, i.e., the controller also needs to forward ARP requests and responses appropriately such that they reach their destination(s).   
* The source address of all other packets received as packet-in events for the unlikely case that static ARP entries are used by hosts and no ARP requests are sent by hosts. Note that you should _not_ use static ARP entries in Mininet in this task, but rely on ARP to figure our MAC addresses for IP addresses automatically as usual. 

Since hosts might change their point of attachment to switches in the network or leave the network, also make sure that your implementation can adapt to such dynamic changes eventually.

Test your implementation again with the following topology, again by pinging host `h3` from host `h1`: 

```console
$ sudo mn --controller remote,ip=127.0.0.1,port=6653 --switch ovs,protocols=OpenFlow13 --topo tree,depth=2,fanout=2 --mac
```

As part of your solution, submit the file implementing the class `LearningBridge` and all other files that you have implemented (it is again not required to submit the complete Floodlight source tree and Eclipse project). 

# Solutions

Provide here the documentation of your solutions for each task (one sub-section per task).

Extend this README file in this section to explain your solution.

In addition, submit the files that include your implementation in the same folder as this README file. You do not have to copy the whole Eclipse project including all Floodlight files to your repository, just your implementation.

The submission is done via Git: add, commit, and push your solution to *your* Git repository (not the repository that we provided; the fork). We will pull the solution from your repository after the deadline (please do not forget to invite us to your repository).
