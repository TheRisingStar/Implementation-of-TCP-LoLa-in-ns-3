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
  ;
  return tid;
}

TcpLola::TcpLola (void) : TcpNewReno ()
{
  m_minRtt = Seconds (DBL_MAX);
  m_nextState = NS_SLOW_START;	
  NS_LOG_FUNCTION (this);
}

TcpLola::TcpLola (const TcpLola& sock) : TcpNewReno (sock)
{
  m_minRtt = sock.m_minRtt;
  m_nextState = sock.m_nextState;	
  NS_LOG_FUNCTION (this);
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
  m_minRtt = std::min (m_minRtt,rtt);
  m_maxRtt = std::max (m_maxRtt, rtt);
  m_curRtt = rtt;
  m_queueDelay = m_curRtt - m_minRtt;
  //NS_LOG_INFO("----------"<<m_minRtt<<"----------"<<m_maxRtt<<"----------"<<m_curRtt<<"----------"<<m_queueDelay);
}

void TcpLola::CongestionStateSet (Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCongState_t newState)
{ 
  NS_LOG_FUNCTION (this << tcb << newState);
  if (newState == TcpSocketState::CA_LOSS )
    {
      m_cwndRednTimeStamp = Simulator::Now ();
      m_cwndReduced  = true;
      m_cwndHoldStart = false;
      m_nextState = NS_SLOW_START;
      NS_LOG_INFO ("Logging Retrasmit timeout ");
      m_cwndHoldStart = false;
    }
  else if (newState == TcpSocketState::CA_RECOVERY)
    {
      NS_LOG_INFO ("Logging Recovery stage");
      m_nextState = NS_TAIL_DECREASE;
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

uint32_t TcpLola::GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
{
  return std::max (std::min (tcb->m_ssThresh.Get (), tcb->m_cWnd.Get () - tcb->m_segmentSize), 2 * tcb->m_segmentSize);
}

void TcpLola::updateKfactor ()
{
  double temp = (m_curRtt - m_minRtt * m_gamma) * m_cwndMax / m_curRtt;
  temp = temp / m_factorC;
  m_factorK = cbrt (temp);
}


void TcpLola::callSlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION ("-----------Slow Start-----------");
  TcpNewReno::SlowStart (tcb, segmentsAcked);
  if (m_maxRtt - m_minRtt > 2 * m_queueLow)
    {
      m_nextState = NS_CUBIC;
    }
}
void TcpLola::callCubic (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION ("-----------Cubic Increase-----------");
  double x;
  x = m_factorC * std::pow ((Simulator::Now ().GetSeconds () - m_cwndRednTimeStamp.GetSeconds () - m_factorK), 3.0) + static_cast<double> (m_cwndMax);
  tcb->m_cWnd = static_cast<uint32_t> (x * tcb->m_segmentSize);
  if (m_queueDelay > m_queueLow)
    {
      m_nextState = NS_FAIR_FLOW;
    }
}
void TcpLola::callFairFlow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION ("-----------Fair Flow Balancing-----------");
  uint32_t X;
  if (m_fairFlowStart == false)
    {
      X = 0;
      m_fairFlowStart = true;
      m_fairFlowTimeStamp = Simulator::Now ();
    }
  else
    {
      X = pow ((static_cast<uint32_t> (Simulator::Now ().GetSeconds ()) - m_fairFlowTimeStamp.GetSeconds ()) / m_phi, 3);
    }

  m_qData = (tcb->m_cWnd * m_queueDelay) / m_curRtt;
  if (m_qData < X)
    {
      tcb->m_cWnd += X - m_qData;
    }
  if (m_cwndHoldStart == true || m_queueDelay > m_queueTarget)
    {
      m_nextState = NS_CWND_HOLD;
    }
}
void TcpLola::callCwndHold (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION ("-----------CWND HOLD-----------");
  if (m_cwndHoldStart == false)
    {
      m_cwndHoldStart = true;
      m_cwndHoldTime = Simulator::Now ();
    }
  if (Simulator::Now () > m_cwndHoldTime + m_syncTime)
    {
      m_nextState = NS_TAIL_DECREASE;
      m_cwndHoldStart = false;
    }
  else
    {
      m_nextState = NS_CWND_HOLD;
    }
}
void TcpLola::callTailDecrease (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION ("-----------Tailored Decrease-----------");
  m_qData = tcb->m_cWnd * m_queueDelay / m_curRtt;
  updateKfactor ();
  tcb->m_cWnd = (tcb->m_cWnd - m_qData) * m_gamma;
  m_cwndReduced = true;
  if (++m_minRttResetCounter == 100)
    {
      m_minRtt = Seconds (DBL_MAX);
    }
  m_nextState = NS_CUBIC;
}

} // namespace ns3
