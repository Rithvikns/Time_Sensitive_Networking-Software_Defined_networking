In this assignment, we will focus on Time Sensitive Networking (TSN), in particular, the Time-Aware Shaper (IEEE 802.1Qbv) for deterministic real-time communication. We will use the Linux implementation of the Time-Aware Shaper called TAPRIO (Time Aware Priority Shaper). TAPRIO is implemented as a so-called queuing discipline (QDisc) in Linux, which can be configured through the Linux tc (traffi control) command as shown in the presentation. You will also implement simple Talkers (senders) and Listeners (receivers) for isochronous periodic traffic.   

**Deadline** for turning in solutions: **Thursday, June 29, 2023 (end of day)** 

# Prerequisites

Besides a standard Linux system, C compiler, and the common networking tools like tcpdump, no special tools are required in this assignment. You can use the provided VMs vslabcourse1..4 or the Virtual Box VM image (same image as in the last assignment, so you can re-use your personal VM from the last assignment or download the image [here](https://ipvs.informatik.uni-stuttgart.de/cloud/s/LGPZrd5copkdEA6).  

# Recommended reading:

* [Blog post](https://www.frank-durr.de/posts/2019/04/11/software_tsn_switch.html) explaining TAPRIO
* [TAPRIO man page](https://man7.org/linux/man-pages/man8/tc-taprio.8.html)

# Tasks of the Assignment

## Task 1: Time-Aware Scheduling in Simple Network Topology

In this task, we use a simplified topology consisting of only one listener and one talker, each running in their own network namespaces, directly connected through a pair of veth devices. In particular, no bridge is used in this task. 

## Task 1.1: Simple UDP Talker and Listener

First, implement simple talker and listener applications that communicate through UDP/IP. The talker should send datagrams in an endless loop as fast as possible. Traffic is only flowing downstream from the talker to the listener. Each datagram should include a single unsigned 64 bit integer as payload, denoting the time in nano-seconds just before the datagram was sent by the talker. The listener shall also take a timestamp just after it received each datagram (we will later use these timestamps to calculate the network delay). 

1. Create two namespaces called `talker` and `listener`, respectively.
2. Create a veth-connection (pair of veth devices `veth-t` and `veth-l`) and assign the ends to the namespaces. 
3. Assign IP addresses to the veth devices.
4. Start tcpdump in the namespace `listener` as follows to inspect the received packets: `$ tcpdump -l -e -i veth-l`
5. Start the listener and then the talker and check, whether the communication works.

Submit all command and the source code of the talker and listener as part of your solution.

## Task 1.2: Scheduling with TAPRIO

In this task, we repeat the experiment from the previous task, but now with time-aware shaping through TAPRIO.

Modify the talker from Task 1.1 to set the priority of all egrees datagrams to 1 (instead of the default 0). 

Create a veth link, where the talker veth device (`veth-t`) has 8 TX queues.

Add a TAPRIO QDisc to the device `veth-t` of the talker to shape the outgoing traffic from the talker. Configure the cyclic schedule of TAPRIO as follows:

* Assign one queue for traffic of priority 0 and one queue to traffic of priority 1.
* The queue for priority 0 traffic shall always be open. This will allow ARP messages to be transmitted at any time.
* The queue for priority 1 shall be open for 0.75 sec and closed for 0.25 sec, resulting in a schedule with period 1 sec (which is quite long, but the effects of scheduling are easier to see).

Test your implementation by inspecting the ingress traffic at the listener with tcpdump. You should be able to see that datagrams from the talker follow the schedule (with gaps of size 0.25 sec).

Add all commands, source code, and an excerpt of tcpdump to your solution.

## Task 1.3: Cyclic Traffic 

In this task, the talker should send cyclic traffic, i.e., datagrams are sent periodically by the talker. The period (cycle time) shall be 100 ms. In each cycle, a single datagram is sent. The cycle shall not be synchronized to the schedule of TAPRIO (otherwise, this would be called isochronous traffic, which is considered in the next task). Datagrams shall be sent with priority 1. 

TAPRIO shall schedule traffic from the talker again at `veth-t`. Traffic of priority 1 shall be scheduled as follows: gate open for 50 ms, closed for 50 ms, open for 50 ms, etc.

Execute the talker and listener, and record the end-to-end network delay of each datagram to a file, using the timestamps taken by the talker and listener.

Analyze the end-to-end delay by:

* Drawing a histogram
* Calculating the median with 99 % confidence interval.
* State the maximum delay value.

Add all commands, source code, and the analysis of the delay to your solution.

## Task 1.4: Isochronous Traffic

In this task, the talker should send isochronous traffic with the same parameters as in Task 1.3, but now the schedules of the talker and TAPRIO shall be sychronized. Change your talker code, such that is uses the same time (clock) as TAPRIO, and synchronize the sending of datagrams and the opening of gates of priority 1.

Again, record the end-to-end network delay of each datagram to a file and perform the same delay analysis as in Task 1.3. Is there a significant difference of the average delay (note: the confidence intervals help you to decide whether there is a significant difference)? Also compare the maximum delay.

Add all commands, source code, and the analysis of the delay to your solution.

# Task 2: Time-Aware Scheduling in a Bridged Network with VLAN

In this task we consider a more sophisticated and realistic TSN network topology including a (virtual Linux) bridge between the talker and the listener:

```
----------        ----------        ------------
| Talker |--------| Bridge |--------| Listener | 
----------        ----------        ------------
```

The talker and listener again run in their own network namespaces. The bridge is in the root namespace.

The links are implemented again with veth devices, however, now VLANs shall be used additionally. Talker and listener both belong to VLAN 100. VLAN tags shall be added at the sender side of a link and stripped off at the receiver side, i.e., you have VLAN veth devices at both ends of the link. Again, the talker sends traffic of priority 1. This priority shall be mapped to the PCP field of the VLAN-tagged packets from the talker to the bridge, and from the bridge to the listener. 

Scheduling with TAPRIO is only done at the egress port of the bridge towards the listener. Note that TAPRIO uses socket buffer (SKB) priorities rather than the value of the PCP field. Thus, you need to map the PCP value at the ingress port of the bridge to an SKB priority, otherwise all traffic will be treated at priority 0 traffic.

Configure TAPRIO as follows:

* Assign one queue for traffic of priority 0 and one queue to traffic of priority 1.
* The queue for priority 0 traffic shall always be open. This will allow ARP messages to be transmitted at any time.
* The queue for priority 1 shall be open for 0.75 sec and closed for 0.25 sec, resulting in a schedule with period 1 sec (which is quite long, but the effects of scheduling are easier to see).

Use tcpdump at the listener side to show that TAPRIO is effective in scheduling traffic, i.e., you should see gaps of 0.25 sec duration in the packet trace. Moreover, use tcpdump at the veth device before VLAN tags are removed to show that traffic is actually tagged, including a VID and PCP (priority). 

# Bonus Task

This bonus task is absolutely voluntary! You can get the best possible grade (1.0) without doing it. By doing the bonus task you can improve your grade by a maximum value of 1.0. 

A word of caution: there is a good chance that it is impossible to solve this task (but a "90 % solution" is possible and you will learn a lot about iptables). 

Use iptables to implement the mapping from PCP values to SKB priorities at the bridge as described in [this blog post](https://www.frank-durr.de/posts/2019/04/11/software_tsn_switch.html). That is, rather than stripping off the VLAN tags at the end of a veth connection, the bridge shall internally forward VLAN-tagged packets, and ports are assigned to VLAN ids (as it is typically done for bridges). iptables should match on the PCP field and classify the packets (adding the corresponding SKB priority).   

# Solutions

Extend this README file in this section to explain your solution.

In addition, submit the source files that include your implementation in the same folder as this README file. 

The submission is done via Git: add, commit, and push your solution to *your* Git repository (not the repository that we provided; the fork). We will pull the solution from your repository after the deadline (please do not forget to invite us to your repository).
