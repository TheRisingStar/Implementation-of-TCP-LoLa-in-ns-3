#include "tcp-lola.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "rtt-estimator.h"
#include "tcp-socket-base.h"
#include "ns3/simulator.h"
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
                   UintegerValue (35),
                   MakeUintegerAccessor (&TcpLola::m_phi),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Gamma", "Factor to update K",
                   DoubleValue (973/1024),
                   MakeDoubleAccessor (&TcpLola::m_factorC),
                   MakeDoubleChecker <double> (0.0))     
    .AddAttribute ("SyncTime", "cWnd Hold Time in ms",
                   UintegerValue (250),
                   MakeUintegerAccessor (&TcpLola::m_syncTime),
                   MakeUintegerChecker<uint32_t> ())            
    .AddAttribute ("queueLow", "Minimu Queue Delay Expected  in ms",
                   UintegerValue (1),
                   MakeUintegerAccessor (&TcpLola::m_queueLow),
                   MakeUintegerChecker<uint32_t> ())    
    .AddAttribute ("queueTarget", "Target Queue Delay in ms",
                   UintegerValue (5),
                   MakeUintegerAccessor (&TcpLola::m_queueTarget),
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
	
  m_minRtt = tcb->m_minRtt;
  NS_LOG_DEBUG ("Updated m_minRtt = " << m_minRtt);

  m_maxRtt = std::max (m_maxRtt, rtt);
  NS_LOG_DEBUG ("Updated m_maxRtt = " << m_maxRtt);
  
  m_nowRtt = rtt;
  NS_LOG_DEBUG ("Updated m_nowRtt = " << m_nowRtt);
  
  m_queueDelay = m_nowRtt - m_minRtt;
  NS_LOG_DEBUG ("Updated m_queueDelay = " << m_queueDelay);

  // Update RTT counter
  m_cntRtt++;
  NS_LOG_DEBUG ("Updated m_cntRtt = " << m_cntRtt);
}

void TcpLola::CongestionStateSet (Ptr<TcpSocketState> tcb,const TcpSocketState::TcpCongState_t newState)
{
  NS_LOG_FUNCTION (this << tcb << newState);
  
  m_cwndMax=std::max(m_cwndMax,(unsigned int)tcb->m_cWnd);
  NS_LOG_DEBUG ("Updated m_cWndmax = " << m_cwndMax);
  
  if (newState == TcpSocketState::CA_CWR || newState == TcpSocketState::CA_RECOVERY || newState == TcpSocketState::CA_LOSS )
    {
      updateKfactor();
      m_timeSinceRedn=Simulator::Now ();
      
    }
  
     
}


void TcpLola::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);
  
  if(m_maxRtt - m_minRtt > 2 * m_queueLow)
  {
    NS_LOG_INFO("Cubic Increase");
    //cubic increase   
  //  m_timeSinceRedn = MilliSeconds(250); //modified in CongestionStateSet();	
   // m_factorK = 0.25;   // modified in updateKFactor();
   // m_cwndMax = 10;	  // modified in CongestionStateSet();

    double x;
    x = m_factorC * std::pow ((Simulator::Now ().GetSeconds()-m_timeSinceRedn.GetSeconds() - m_factorK), 3.0) + static_cast<double> (m_cwndMax);	
    tcb->m_cWnd = static_cast<uint32_t> (x * tcb->m_segmentSize); 
  }
  else if(m_queueDelay > m_queueLow)
  {
    //fair flow balancing
    NS_LOG_INFO("Fair Flow Balancing");
    uint32_t X = pow((static_cast<uint32_t> (Simulator::Now ().GetSeconds ())-m_fairFlowTimeStamp)/m_phi,3);
   
  
   m_qData=(tcb->m_nextTxSequence-tcb->m_lastAckedSeq-1)*tcb->m_segementSize;  
    if(m_qData < X)
    {
      tcb->m_cWnd += X - m_qData;
    }
    
  }
  else if(m_queueDelay > m_queueTarget)
  {
  
	NS_LOG_INFO("cWnd Hold");  
    m_fairFlowTimeStamp = static_cast<uint32_t> (Simulator::Now ().GetSeconds ());
    
    // cwnd hold for a period of m_syncTime
    m_tempTime=m_syncTime;
    TimerHandler();
    
    //Tailored decrease 
    m_qData=(tcb->m_nextTxSequence-tcb->m_lastAckedSeq-1)*tcb->m_segementSize; 
    tcb->m_cWnd = (tcb->m_cWnd - m_qData)*m_gamma;
    
    
    
  }
  else
  {	NS_LOG_INFO("Slow start");
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


void TcpLola::updateKfactor(){
	double temp = (m_nowRtt - m_minRtt * m_gamma) * m_cwndMax * m_factorC / m_nowRtt;
	m_factorK=cbrt(temp);
}

void TcpLola::TimerHandler(){
	
	if(m_tempTime>0){
		m_tempTime--;
		m_expiredEvent = Simulator::Schedule (Seconds (1), &TcpLola::TimerHandler, this);
	}
}

} // namespace ns3
