#include "tcp-lola.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "rtt-estimator.h"
#include "tcp-socket-base.h"

NS_LOG_COMPONENT_DEFINE ("TcpLola");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpLola);

TypeId TcpLola::GetTypeId (void)
{
  static TypeId tid = TypeId("ns3::TcpLola")
    .SetParent<TcpNewReno>()
    .SetGroupName ("Internet")
    .AddConstructor<TcpLola>()
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

  // Update RTT counter
  m_cntRtt++;
  NS_LOG_DEBUG ("Updated m_cntRtt = " << m_cntRtt);
}

void TcpLola::CongestionStateSet (Ptr<TcpSocketState> tcb,const TcpSocketState::TcpCongState_t newState)
{
  NS_LOG_FUNCTION (this << tcb << newState);
 
    // need to set different switching based on congestion 
    
   
}
void TcpLola::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);
  
  // Need to add different increase window stratergies
}

std::string TcpLola::GetName () const
{
  return "TcpLola";
}
uint32_t TcpLola::GetSsThresh (Ptr<const TcpSocketState> tcb,uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << tcb << bytesInFlight);
  return std::max (std::min (tcb->m_ssThresh.Get (), tcb->m_cWnd.Get () - tcb->m_segmentSize), 2 * tcb->m_segmentSize);
}


} // namespace ns3
