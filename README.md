# Implementation of TCP LoLa in ns-3

## Course Code : CO365
## Course : Advanced Computer Networks

### Overview
TCP LoLa is a delay-based congestion control algorithm that supports both, low queuing delay and high network utilization in high speed wide-area networks. 

### TCP Lola Summary 
<p align="left">
  <img width="552" height="136" src="https://github.com/joe019/Implementation-of-TCP-LoLa-in-ns-3/blob/master/Reference/Images/lola.png"><br>
  <a align="center">  </a>
</p>

<b>RTT<sub>max</sub></b> and <b>RTT<sub>min</sub></b>: maximal and minimal measured RTT respectively                                  
<b>Q<sub>low</sub></b> and <b>Q<sub>target</sub></b>: threshold values<br>
<b>Q<sub>delay</sub></b>: queuing delay caused by the standing queue<br>
Standing queue: a small queue deliberately created by TCP LoLa<br>
Standing queue exists: overall amount of in-flight data is sufficient to fully utilize the bottleneck link

#### (a) Slow Start
TCP LoLa enters the slow start state after its initial start or after a retransmission timeout.
#### (b) Cubic Increase
Increase function used by TCP LoLa:

<p align="left">
  <img width="385" height="51" src="https://github.com/joe019/Implementation-of-TCP-LoLa-in-ns-3/blob/master/Reference/Images/cubic_inc.png"><br>
  <a align="center">  </a>
</p>

<b>t</b>: time since last window reduction<br>
<b>C</b>: unit-less factor (C = 0.4)<br>
<b>K</b>: recalculated whenever CWnd has to be reduced<br>
<b>CWnd<sub>max</sub></b>: size of CWnd before last reduction<br><br>
TCP LoLa uses this function only if the potential bottleneck link is most likely not fully utilized, i.e., no standing queue is detected.

#### (c) Fair Flow Balancing

<b>Basic idea</b>: Each flow should keep a low but similar amount of data (X) in the bottleneck queue. 
To keep the overall queuing delay between the thresholds Q<sub>low</sub> and Q<sub>target</sub>, X has to be dynamically scaled:

<p align="left">
  <img width="265" height="70" src="https://github.com/joe019/Implementation-of-TCP-LoLa-in-ns-3/blob/master/Reference/Images/fairflowbal.png"><br>
  <a align="center">  </a>
</p>

<b>t</b> = 0, when fair flow balancing is entered<br> 
<b>φ</b> is a constant.<br>
<b>CWnd</b> is adapted as follows:<br>

<p align="left">
  <img width="352" height="145" src="https://github.com/joe019/Implementation-of-TCP-LoLa-in-ns-3/blob/master/Reference/Images/ffcond.png"><br>
  <a align="center">  </a>
</p>

<b>Qdata</b>: amount of data the flow itself has queued at the bottleneck.
Fair flow balancing requires that all competing flows enter and leave it at similar points in time => design puts a strong emphasis on synchronized state changes.

#### (d) CWnd Hold
In this state, the CWnd is unchanged for a fixed amount of time t<sub>sync</sub> (default value = 250 ms). The hold time is necessary to ensure that all flows quit the current round of fair flow balancing.

#### (e) Tailored Decrease
Tailored decrease adjusts the CWnd reduction to the amount of congestion:

<p align="left">
  <img width="269" height="32" src="https://github.com/joe019/Implementation-of-TCP-LoLa-in-ns-3/blob/master/Reference/Images/tail_dec.png"><br>
  <a align="center">  </a>
</p>

Each flow reduces its CWnd by Q<sub>data</sub> – this should already empty the queue. CWnd is further reduced by the factor γ < 1 to ensure that the queue will actually be drained completely.  To achieve this, K is calculated as follows:

<p align="left">
  <img width="463" height="77" src="https://github.com/joe019/Implementation-of-TCP-LoLa-in-ns-3/blob/master/Reference/Images/k_factor.png"><br>
  <a align="center">  </a>
</p>

<b>RTT<sub>min</sub></b>: RTT without any queuing delays<br>
<b>RTT<sub>now</sub></b>: RTT including the standing queue<br>

#### How are queuing delay measurements done?

<p align="left">
  <img width="419" height="44" src="https://github.com/joe019/Implementation-of-TCP-LoLa-in-ns-3/blob/master/Reference/Images/rttmeasure.png"><br>
  <a align="center">  </a>
</p>

<b>RTT(t<sub>k</sub>)</b>: an individual RTT measurement at time t<sub>k</sub> </br>
<b>t_measure</b>: a certain time interval independent of a flow’s RTT</br>

<p align="left">
  <img width="335" height="81" src="https://github.com/joe019/Implementation-of-TCP-LoLa-in-ns-3/blob/master/Reference/Images/rttmin_measure.png"><br>
  <a align="center">  </a>
</p>

<p align="left">
  <img width="266" height="73" src="https://github.com/joe019/Implementation-of-TCP-LoLa-in-ns-3/blob/master/Reference/Images/qdata.png"><br>
  <a align="center">  </a>
</p>

The validity of RTT<sub>min</sub> is checked after tailored decrease. RTT<sub>min</sub> is reset, if no  RTT<sub>now</sub> value close to RTT<sub>min</sub> has been measured for a certain number (e.g., 100) of tailored decreases.





### Steps to reproduce

### To run the code

First we need to download and use the development version of ns-3.
If you have successfully installed mercurial, you can get copy of the development version with the following command :

`hg clone http://code.nsnam.org/ns-3-allinone`

`cd ns-3-allinone && ./download.py`

To build ns-3 downloaded, open the terminal to `ns-3-allinone` directory and enter the following command :

`./build.py`

Now clone the repository to your local machine.

`git clone https://github.com/joe019/Implementation-of-TCP-LoLa-in-ns-3.git`

Replace the contents in ns-3-dev directory with the cloned directory files. Then configure examples and tests in ns-3 using :

`./waf configure --enable-examples --enable-tests`

TCP Lola module has been provided in:

`src/internet/model/tcp-lola.cc`
`src/internet/model/tcp-lola.h`

Once ns-3 has been built, we can run the example file as:

`./waf --run "examples/tcp/tcp-variants-comparison --transport_prot=TcpLola"`

### References

[1] TCP LoLa: Congestion Control for Low Latencies and High Throughput (http://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=8109356)

[2] TCP LoLa implementation in the Linux Kernel (https://git.scc.kit.edu/TCP-LoLa/TCP-LoLa_for_Linux)
