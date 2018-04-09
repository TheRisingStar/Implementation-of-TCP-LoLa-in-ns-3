# Implementation of TCP-LoLa in ns-3

## Course Code : CO365
## Course : Advanced Computer Networks

### Overview
A delay-based congestion control algorithm that supports both, low queuing delay and high network utilization in high speed wide-area networks. 

### Tcp Lola Summary 
<b>RTTmax</b> and <b>RTTmin</b> : maximal and minimal measured RTT respectively                                  
<b>Qlow</b> and <b>Qtarget</b> : threshold values<br>
<b>Qdelay</b> : queuing delay caused by the standing queue<br>
Standing queue: a small queue deliberately created by <b>TCP LoLa</b><br>
Standing queue exists : overall amount of in-flight data is sufficient to fully utilize the bottleneck link

#### (a) Slow Start
TCP LoLa enters the slow start state after its initial start or after a retransmission timeout.
#### (b) Cubic Increase
Increase function used by TCP LoLa:
               
<b>t</b> : time since last window reduction<br>
<b>C</b> : unit-less factor (C = 0.4)<br>
<b>K</b> : recalculated whenever CWnd has to be reduced<br>
<b>CWndmax</b> : size of CWnd before last reduction<br><br>
TCP LoLa uses this function only if the potential bottleneck link is most likely not fully utilized, i.e., no standing queue is detected.

#### (c) Fair Flow Balancing

<b>Basic idea</b>: Each flow should keep a low but similar amount of data (X) in the bottleneck queue. 
To keep the overall queuing delay between the thresholds Qlow and Qtarget , X has to be dynamically scaled:

<b>t</b> = 0 when fair flow balancing is entered<br> 
<b>φ</b> is a constant.<br>
<b>CWnd</b> is adapted as follows:<br>

<b>Qdata</b>: amount of data the flow itself has queued at the bottleneck
Fair flow balancing requires that all competing flows enter and leave it at similar points in time => design puts a strong emphasis on synchronized state changes.

#### (d) CWnd Hold
In this state, the CWnd is unchanged for a fixed amount of time tsync (default value = 250 ms). The hold time is necessary to ensure that all flows quit the current round of fair flow balancing.

#### (e) Tailored Decrease
Tailored decrease adjusts the CWnd reduction to the amount of congestion:

Each flow reduces its CWnd by Qdata – this should already empty the queue. CWnd is further reduced by the factor γ < 1 to ensure that the queue will actually be drained completely.  To achieve this, K is calculated as follows:

<b>RTTmin</b> : RTT without any queuing delays<br>
<b>RTTnow</b> : RTT including the standing queue<br>

#### How are queuing delay measurements done?

<b>RTT (tk)</b> : an individual RTT measurement at time tk </br>
<b>t_measure</b> : a certain time interval independent of a flow’s RTT</br>


The validity of RTTmin is checked after tailored decrease.RTTmin is reset, if no  RTTnow value close to RTTmin has been measured for a certain number (e.g., 100) of tailored decreases.





### Steps to reproduce

First we need to download and use the development version of ns-3.
If you have successfully installed mercurial, you can get copy of the development version with the following command :

`hg clone http://code.nsnam.org/ns-3-allinone`

`cd ns-3-allinone && ./download.py`

`cd ns-3-dev`

Replace the contents in ns-3-dev directory with the cloned directory files. Then build ns-3 using :

`./waf configure --enable-examples --enable-tests`

Now clone the repository to your local machine.

`git clone https://github.com/joe019/Implementation-of-TCP-LoLa-in-ns-3.git`

Copy the contents of cloned repository to ns-3-allinone

Tcp Lola module has been provided in :

`src/internet/model/tcp-lola.cc`
`src/internet/model/tcp-lola.h`

Once ns-3 has been built, we can run the example file as :

`./waf --run "examples/tcp/tcp-variants-comparison --transport_prot=TcpLola"`

### References

[1] TCP LoLa: Congestion Control for Low Latencies and High Throughput (http://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=8109356)

[2] TCP LoLa implementation in the Linux Kernel (https://git.scc.kit.edu/TCP-LoLa/TCP-LoLa_for_Linux)
