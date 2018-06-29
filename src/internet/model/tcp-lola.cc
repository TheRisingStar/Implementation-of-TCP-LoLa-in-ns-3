#include "tcp-lola.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "rtt-estimator.h"
#include "tcp-socket-base.h"
#include "ns3/simulator.h"
#include <math.h>
#include <float.h>

NS_LOG_COMPONENT_DEFINE ("TcpLola");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpLola);

TypeId TcpLola::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpLola")
    .SetParent<TcpNewReno> ()
    .SetGroupName ("Internet")
    .AddConstructor<TcpLola> ()
    .AddAttribute ("C", "Unit-less factor",
                   DoubleValue (0.4),
                   MakeDoubleAccessor (&TcpLola::m_factorC),
                   MakeDoubleChecker <double> (0.0))
    .AddAttribute ("Phi", "Fair flow balancing curve factor",
                   UintegerValue (35),
                   MakeUintegerAccessor (&TcpLola::m_phi),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Gamma", "Factor to update K",
                   DoubleValue (0.95019),
                   MakeDoubleAccessor (&TcpLola::m_gamma),
                   MakeDoubleChecker <double> (0.0))
    .AddAttribute ("SyncTime","CWnd hold time in ms",
                   TimeValue (MilliSeconds (10)),
                   MakeTimeAccessor (&TcpLola::m_syncTime),
                   MakeTimeChecker ())
    .AddAttribute ("QueueLow","Minimum queue delay expected in ms",
                   TimeValue (MilliSeconds (1)),
                   MakeTimeAccessor (&TcpLola::m_queueLow),
                   MakeTimeChecker ())
    .AddAttribute ("QueueTarget","Target queue delay in ms",
                   TimeValue (MilliSeconds (5)),
                   MakeTimeAccessor (&TcpLola::m_queueTarget),
                   MakeTimeChecker ())
    .AddAttribute ("MeasureTime","Time interval to calculate m_currRtt value",
                   TimeValue (MilliSeconds (40)),
                   MakeTimeAccessor (&TcpLola::m_measureTime),
                   MakeTimeChecker ())
  ;
  return tid;
}

TcpLola::TcpLola (void) : TcpNewReno ()
{
  NS_LOG_FUNCTION (this);
  m_minRtt = Seconds (DBL_MAX);
  m_curRtt = Seconds (DBL_MAX);
  m_cwndReduced = false;
  m_fairFlowStart = false;
  m_cwndHoldStart = false;
  //m_measureTimeStart = false; //flag method to measure curRtt
  m_nextState = NS_SLOW_START;

}

TcpLola::TcpLola (const TcpLola& sock) : TcpNewReno (sock)
{
  NS_LOG_FUNCTION (this);
  m_minRtt = sock.m_minRtt;
  m_curRtt = sock.m_curRtt;
  m_cwndReduced = sock.m_cwndReduced;
  m_fairFlowStart = sock.m_fairFlowStart;
  m_cwndHoldStart = sock.m_cwndHoldStart;
  //m_measureTimeStart = sock.m_measureTimeStart;  //flag method to measure curRtt
  m_nextState = sock.m_nextState;

}

TcpLola::~TcpLola (void)
{
  NS_LOG_FUNCTION (this);
}

void TcpLola::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt)
{
  //NS_LOG_FUNCTION (this << tcb << segmentsAcked << rtt);
  if (rtt.IsZero ())
    {
      return;
    }

  /*flag method to measure curRtt
  if(m_measureTimeStart==false){
    m_measureTimeStamp=Simulator::Now ();
    m_curRtt=Seconds(DBL_MAX);
    m_measureTimeStart=true;

  }
  if(Simulator::Now () > m_measureTimeStamp+m_measureTime){
        m_measureTimeStart=false;
        m_curRtt=Seconds(DBL_MAX);
  }
  else{
        m_curRtt=std::min(rtt,m_curRtt);
  }
  m_minRtt = std::min (m_minRtt, m_curRtt);
  m_maxRtt = std::max (m_maxRtt, rtt);
  */

  /*List push method to measure curRtt
  if (m_rttList.size () == 0)
    {
      m_curRtt = rtt;
      m_minRtt = rtt;
      m_maxRtt = rtt;
      updateRttList ();
    }
  m_rttList.push_back (std::make_pair (rtt,Simulator::Now ()));
  m_maxRtt = std::max (m_maxRtt, rtt);
  */

  // basic method
  m_minRtt = std::min (m_minRtt,rtt);
  m_maxRtt = std::max (m_maxRtt, rtt);
  m_curRtt = rtt;

  m_queueDelay = m_curRtt - m_minRtt;
  //NS_LOG_INFO ("----------" << m_minRtt << "----------" << m_maxRtt << "----------" << m_curRtt << "----------" << m_queueDelay);
}

void TcpLola::CongestionStateSet (Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCongState_t newState)
{
  NS_LOG_FUNCTION (this << tcb << newState);
  if (newState == TcpSocketState::CA_LOSS )
    {
      //m_cwndRednTimeStamp = Simulator::Now ();
      // m_cwndReduced  = true;  // Ambigous ,not sure whether this reduction due to loss is considered
      m_cwndHoldStart = false;
      m_nextState = NS_SLOW_START;
      NS_LOG_INFO ("Logging Retrasmit timeout ");
    }
  else if (newState == TcpSocketState::CA_RECOVERY)
    {
      //m_cwndRednTimeStamp = Simulator::Now ();
      //m_cwndReduced  = true;

      m_cwndHoldStart = false;

      //m_nextState = NS_SLOW_START;   // Ambigous, Not mentioned the state after recovery stage

      NS_LOG_INFO ("Logging Recovery stage");
    }
}

void TcpLola::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);
  if (m_cwndReduced == false)
    {
      m_cwndMaxTemp = std::max (static_cast<uint32_t> (tcb->m_cWnd), m_cwndMaxTemp);
    }
  if (m_cwndReduced == true)
    {
      m_cwndMax = m_cwndMaxTemp;
      m_cwndReduced = false;
      m_cwndMaxTemp = 0;
    }
  switch (m_nextState)
    {
    case NS_SLOW_START:
      callSlowStart (tcb,segmentsAcked);
      break;
    case NS_CUBIC:
      callCubic (tcb, segmentsAcked);
      break;
    case NS_FAIR_FLOW:
      callFairFlow (tcb, segmentsAcked);
      break;
    case NS_CWND_HOLD:
      callCwndHold (tcb,segmentsAcked);
      break;
    case NS_TAIL_DECREASE:
      callTailDecrease (tcb,segmentsAcked);
      break;
    }
  m_cwndMaxTemp = std::max (static_cast<uint32_t> (tcb->m_cWnd), m_cwndMaxTemp);
}

std::string TcpLola::GetName () const
{
  return "TcpLola";
}

void TcpLola::updateKfactor ()
{
  double temp = (m_curRtt - m_minRtt * m_gamma) * m_cwndMax / m_curRtt;
  temp = temp / m_factorC;
  m_factorK = cbrt (temp);
}


void TcpLola::callSlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_INFO ("SlowStart");
  if (m_maxRtt - m_minRtt > 2 * m_queueLow)
    {
      m_nextState = NS_CUBIC;
      return;
    }
  if (segmentsAcked >= 1)
    {
      tcb->m_cWnd += tcb->m_segmentSize;
    }
  //TcpNewReno::SlowStart (tcb, segmentsAcked);

}
void TcpLola::callCubic (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_INFO ("Cubic");
  if (m_queueDelay > m_queueLow)
    {
      m_nextState = NS_FAIR_FLOW;
      return;
    }
  double x;
  x = m_factorC * std::pow ((Simulator::Now ().GetSeconds () - m_cwndRednTimeStamp.GetSeconds () - m_factorK), 3.0) + static_cast<double> (m_cwndMax);
  tcb->m_cWnd = static_cast<uint32_t> (x * tcb->m_segmentSize);

}
void TcpLola::callFairFlow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_INFO ("FairFlow");
  if (m_queueDelay > m_queueTarget)
    {
      m_nextState = NS_CWND_HOLD;
      m_fairFlowStart = false;
      return;
    }
  uint32_t X;
  if (m_fairFlowStart == false)
    {
      X = 0;
      m_fairFlowStart = true;
      m_fairFlowTimeStamp = Simulator::Now ();
    }

  X = pow ((static_cast<uint32_t> (Simulator::Now ().GetSeconds ()) - m_fairFlowTimeStamp.GetSeconds ()) / m_phi, 3);

  m_qData = (tcb->m_cWnd * m_queueDelay) / m_curRtt;
  if (m_qData < X)
    {
      tcb->m_cWnd += X - m_qData;
    }

}
void TcpLola::callCwndHold (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_INFO ("CWndHold");
  if (m_cwndHoldStart == false)
    {
      m_cwndHoldStart = true;
      m_cwndHoldTime = Simulator::Now ();
    }
  if (Simulator::Now () > m_cwndHoldTime + m_syncTime)
    {
      m_nextState = NS_TAIL_DECREASE;
      m_cwndHoldStart = false;
      return;
    }
  else
    {
      m_nextState = NS_CWND_HOLD;
    }
}
void TcpLola::callTailDecrease (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_INFO ("TailDecrease");
  m_qData = tcb->m_cWnd * m_queueDelay / m_curRtt;
  updateKfactor ();
  tcb->m_cWnd = (tcb->m_cWnd - m_qData) * m_gamma;
  m_cwndRednTimeStamp = Simulator::Now ();
  m_cwndReduced = true;
  if (m_minRttResetCounter == 100 && abs (m_minRtt.GetMilliSeconds () - m_curRtt.GetMilliSeconds ()) >= 5)
    {
      m_minRtt = Seconds (DBL_MAX);
    }
  m_nextState = NS_CUBIC;
}

/*List push method to measure curRtt
void TcpLola::updateRttList ()
{
  NS_LOG_INFO("Time : "<<Simulator::Now ()<<"  --------------");
  RttListIter iter = m_rttList.begin ();
  while ((*iter).second.GetMilliSeconds () + m_measureTime < Simulator::Now ().GetMilliSeconds ())
    {
      m_rttList.remove (*iter++);
    }
  for (iter = m_rttList.begin (); iter != m_rttList.end (); iter++)
    {
      m_curRtt = std::min (m_curRtt,(*iter).first);
    }
  m_minRtt = std::min (m_minRtt,m_curRtt);
  m_minRttResetCounter++;
  m_expiredEvent = Simulator::Schedule (MilliSeconds (1), &TcpLola::updateRttList, this);
}
*/
} // namespace ns3
