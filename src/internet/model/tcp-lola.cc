#include "tcp-lola.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "rtt-estimator.h"
#include "tcp-socket-base.h"
#include <math.h>

NS_LOG_COMPONENT_DEFINE ("TcpLola");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpLola);

TypeId TcpLola::GetTypeId (void)
{
  static TypeId tid = TypeId("ns3::TcpLola")
    .SetParent<TcpNewReno>()
    .SetGroupName ("Internet")
    .AddConstructor<TcpLola>()
    .AddAttribute ("C", "Unit-less factor",
                   DoubleValue (0.4),
                   MakeDoubleAccessor (&TcpLola::m_factorC),
                   MakeDoubleChecker <double> (0.0))
    .AddAttribute ("Phi", "Fair flow balancing curve factor",
                   UintegerValue (75),
                   MakeUintegerAccessor (&TcpLola::m_phi),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

TcpLola::TcpLola (void) :TcpNewReno ()
{
  NS_LOG_FUNCTION (this);
}

TcpLola::TcpLola (const TcpLola& sock) :TcpNewReno (sock)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Invoked the copy constructor");
}

TcpLola::~TcpLola (void)
{
NS_LOG_FUNCTION (this);
}

Ptr<TcpCongestionOps> TcpLola::Fork (void)
{
  return CopyObject<TcpLola> (this);
}

void TcpLola::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,const Time& rtt)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked << rtt);

  if (rtt.IsZero ())
    {
      return;
    }

  m_minRtt = std::min (m_minRtt, rtt);
  NS_LOG_DEBUG ("Updated m_minRtt = " << m_minRtt);

  m_maxRtt = std::max (m_maxRtt, rtt);
  NS_LOG_DEBUG ("Updated m_maxRtt = " << m_maxRtt);
  
  m_nowRtt = rtt;
  NS_LOG_DEBUG ("Updated m_nowRtt = " << m_nowRtt);

  // Update RTT counter
  m_cntRtt++;
  NS_LOG_DEBUG ("Updated m_cntRtt = " << m_cntRtt);
}

void TcpLola::CongestionStateSet (Ptr<TcpSocketState> tcb,const TcpSocketState::TcpCongState_t newState)
{
  NS_LOG_FUNCTION (this << tcb << newState);
 
  // need to set different switching based on congestion    
}

uint32_t tcp_time_stamp = static_cast<uint32_t> (Simulator::Now ().GetSeconds ());
uint32_t TcpLola::GetTarget(uint32_t time)
{
  uint32_t t = (tcp_time_stamp - time);
  uint32_t X = std::pow((t/m_phi),3);
  return X;
}

void TcpLola::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);
  
  if(m_maxRtt - m_minRtt > 2 * m_queueLow)
  {
    //cubic increase   
    m_timeSinceRedn = MilliSeconds(250);	
    m_factorK = 0.25; 
    m_cwndMax = 10;

    double x;
    x = m_factorC * std::pow ((m_timeSinceRedn.GetSeconds() - m_factorK), 3.0) + static_cast<double> (m_cwndMax);	
    tcb->m_cWnd = static_cast<uint32_t> (x * tcb->m_segmentSize);
  }
  else if(m_queueDelay > m_queueLow)
  {
    //fair flow balancing
    uint32_t X = GetTarget(static_cast<uint32_t> (Simulator::Now ().GetSeconds ()));
    m_qData = 5;

    if(m_qData < X)
    {
      tcb->m_cWnd += X - m_qData;
    }
    else
    {
      tcb->m_cWnd = tcb->m_cWnd;
    }
  }
  else if(m_queueDelay > m_queueTarget)
  {
    // cwnd hold for a period of m_syncTime
    // then call cubic increase
  }
  else
  {
  	TcpNewReno::SlowStart (tcb, segmentsAcked);
  }
}

std::string TcpLola::GetName () const
{
  return "TcpLola";
}

uint32_t TcpLola::GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << tcb << bytesInFlight);
  return std::max (std::min (tcb->m_ssThresh.Get (), tcb->m_cWnd.Get () - tcb->m_segmentSize), 2 * tcb->m_segmentSize);
}


} // namespace ns3
