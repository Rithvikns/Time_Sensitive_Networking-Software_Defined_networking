The basic purpose of this assignment is to get familiar with basic commands for network configuration in Linux and the work environment (your VM, Git). 

# Background

See slides provided in this Git in the folder /linux-networking-basics.

For the full syntax and a detailed explanation of all commands and their options, please read the man pages, in particular:

$ man ip link
$ man ip address
$ man ip netns
$ man tcpdump

# Tasks of the Assignment

## Task 1: Communication between network namespaces with virtual Ethernet devices

First, we want to connect two network namespaces via virtual Ethernet devices (remember that virtual Ethernet devices are always created as pairs of devices that are connected).

1. Create two network namespaces called blue and red. Show all network namespaces of the system using approriate commands (add this output to your solution). 
2. Create one virtual Ethernet device pair (veth-red, veth-blue)
3. Connect both network namespaces through virtual Ethernet devices by assigning one device to namespace red and the other to the namespace blue. Show the network devices and their MAC addresses using appropriate commands (add this output to your solution).
4. Assign private IP addresses from the same IPv4 sub-network to each virtual Ethernet device and bring them up. Show the addresses of all devices using appropriate commands (add this output to your solution).  
5. From each namespace, `ping` the other IP address in the other namespace. If this does not work, check your commands above. Note: from a namespace (say blue), you cannot ping the IP address assigned to the veth device of this namespace (say veth-blue). This is normal and no indication of an error.   
6. Start a netcat server on port 1234 using TCP in namespace red. Use netcat as client in namespace blue to send the message "Hello world" to the server.

## Task 2: Communication between network namespaces with virtual bridges

Next, we want to connect three network namespaces through a virtual Ethernet bridge.

1. Create three network namespaces: red, green, blue.
2. Create three virtual Ethernet device pairs (a total of six virtual devices) and assign one device from each pair to each namespace.
3. Add a virtual bridge to the default namespace, i.e., outside the namespaces red, blue, green.
4. Assign the other virtual device of each virtual Ethernet device pair to the bridge to connect the devices of each namespace to the bridge.
5. Assign a private IPv4 address from the same sub-network to each virtual Ethernet devices that runs in a namespace (the end connected to the bridge does not need an IP address).
6. From each namespace, `ping` the other IP addresses in the other namespaces. If this does not work, check your commands above.  
7. Start a netcat server on port 1234 using TCP in namespace red and another server on port 1234 in namespace green. Use netcat as client in namespace blue to send messages "Hello red from blue" and "Hello green from blue" to both servers namespac red and green.


## Task 3: Using tcpdump to monitor packets on a network interface

In this task, we will use tcpdump to monitor the traffic entering a namespace via a virtual Ethernet device.

Use the setup from Task 2 with three namespaces and one virtual bridge to which all namespaces are connected.

Start a TCP server with netcat in namespace red listening on port 1234. 

### Task 3.1: Monitoring and timestamping all incoming packets

1. Start tcpdump in namespace red to monitor all packets (default) and print default information about each packet on the screen.
2. Send short text messages from namespace blue and green to the server in namespace red using netcat. Show an excerpt of the output of tcpdump in your solution and note down all the protocols of the packets that you see. 

### Task 3.2: Filtering incoming packets

Repeat the experiment from Task 3.1, with the following additional constraints:

1. Record packets to a file (pcap is the typical format if you want to give the file an extension).
2. Filter packets such that only TCP packets to the server port 1234 that are not from the namespace green (only from blue) are recorded using a filter expression for tcpdump.
3. Timestamps shall be recorded with nano-seconds precision.
4. Open the recorded file in Wireshark to inspect it (capture a screenshot and submit it with your solution). Wireshark can either be installed on the VM and accessed remotely, e.g., via x2go. Or you download the pcap file to your own laptop, PC, etc. and install Wireshark locally there (then you can also use other operating systems than Linux).

## Task 4: Using cURL and tshark to monitor the traffic between a web server and client

In this task, we will use cURL to send requests to a web server and tshark to monitor this traffic.

Use again the setup from Task 2.

1. Start a simple Python-based web server in namespace red using the command `python3 -m http.server`. This server will serve all files from the current working directory via HTTP.
2. Use cURL from namespace blue to request a file from the web server running namespace red.
3. Use tshark in namespace red to record (only) to record all traffic to a file `trace.pcap`
4. Use tshark to display only TCP SYN requests from the file.
4. Use tshark to display only the HTTP traffic from the file. 

## Task 5: Socket programming

In this task, a small client/server application shall be implemented.

For this programming task, a code stub including a cmake file is provided in the src folder of this assignment. You can compile the client and server as follows:

```console
$ cd src
$ mkdir build
$ cd build
$ cmake ..
$ make
```

This should create two executables called `client` and `server`.

### Task 5.1

Extend the server code in the file `server.c` to implement a server listening on a given port (provided as command line argument, see given code).

Use a socket for connection-less (unreliable) communication using datagrams.

For the sake of simplicity, implement a single-threaded server that executed the following process in an end-less loop:

1. receive a message as a single datagram from a client
2. processes the message  by reversing the received string
3. send back the result to the client from which the message was received as a single datagram

### Task 5.2

Next, implement the client by extending the code given in the file `client.c`.

The client takes a string from the command line (as an argument to the program, see given code). Moreover, the client gets the server name (which can be a DNS name or IP address) and port number as command line arguments.  

Send this string to the server as a single datagram, wait for the reply (a single datagram), print the result (string received from the server) on the screen, and quit the client process. 

# Solutions

Provide here your solutions for each task (one sub-section per task).

Extend this README file as follows in this section, by showing:

* Command line instructions that you executed.
* The output of these instructions where requested in the task.
* Answers to the questions asked in the tasks.

For immplementation tasks, submit your code and (if necessary) a short instruction in this README file on how to start your application or any other necessary information (besides the inline documentation in the code). 

The submission is done via Git: add, commit, and push your solution to *your* Git repository (not the repository that we provided; the fork). We will pull the solution from your repository after the deadline (please do not forget to invite us to your repository).
